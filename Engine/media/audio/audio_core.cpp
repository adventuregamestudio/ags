//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
// [sonneveld]
// TODO:
// sound caching ([IKM] please, not right here in this backend module)
// slot id generation id
// pre-determine music sizes
// safer slot look ups (with gen id)
// generate/load mod/midi offsets

#include "media/audio/audio_core.h"
#include <unordered_map>
#include "debug/out.h"
#include "media/audio/openaldecoder.h"

namespace ags = AGS::Common;
namespace agsdbg = AGS::Common::Debug;

const auto GlobalGainScaling = 0.7f;

static void audio_core_entry();

struct AudioCoreSlot
{
    AudioCoreSlot(int handle, ALuint source, OpenALDecoder decoder) : handle_(handle), source_(source), decoder_(std::move(decoder)) {}

    int handle_ = -1;
    std::atomic<ALuint> source_ {0};

    OpenALDecoder decoder_;
};



static struct 
{
    ALCdevice *alcDevice = nullptr;
    ALCcontext *alcContext = nullptr;

    std::thread audio_core_thread;
    bool audio_core_thread_running = false;

    // slot ids
    int nextId = 0;

    // One mutex to lock them all... any operation on individual decoders
    // is done under this only mutex, which means that they are currently
    // polled one by one, any action like pause/resume is also synced.
    std::mutex mixer_mutex_m;
    std::condition_variable mixer_cv;
    std::unordered_map<int, std::unique_ptr<AudioCoreSlot>> slots_;
} g_acore;


// -------------------------------------------------------------------------------------------------
// INIT / SHUTDOWN
// -------------------------------------------------------------------------------------------------

void audio_core_init() 
{
   /* InitAL opens a device and sets up a context using default attributes, making
    * the program ready to call OpenAL functions. */

    /* Open and initialize a device */
    g_acore.alcDevice = alcOpenDevice(nullptr);
    if (!g_acore.alcDevice) { throw std::runtime_error("PlayStateError opening device"); }

    g_acore.alcContext = alcCreateContext(g_acore.alcDevice, nullptr);
    if (!g_acore.alcContext) { throw std::runtime_error("PlayStateError creating context"); }

    if (alcMakeContextCurrent(g_acore.alcContext) == ALC_FALSE) { throw std::runtime_error("PlayStateError setting context"); }

    const ALCchar *name = nullptr;
    if (alcIsExtensionPresent(g_acore.alcDevice, "ALC_ENUMERATE_ALL_EXT"))
        name = alcGetString(g_acore.alcDevice, ALC_ALL_DEVICES_SPECIFIER);
    if (!name || alcGetError(g_acore.alcDevice) != AL_NO_ERROR)
        name = alcGetString(g_acore.alcDevice, ALC_DEVICE_SPECIFIER);
    printf("Opened \"%s\"\n", name);

    // SDL_Sound
    Sound_Init();

    g_acore.audio_core_thread_running = true;
    g_acore.audio_core_thread = std::thread(audio_core_entry);
}

void audio_core_shutdown()
{
    g_acore.audio_core_thread_running = false;
    g_acore.audio_core_thread.join();

    // SDL_Sound
    Sound_Quit();

    alcMakeContextCurrent(nullptr);
    if(g_acore.alcContext) {
        alcDestroyContext(g_acore.alcContext);
        g_acore.alcContext = nullptr;
    }

    if (g_acore.alcDevice) {
        alcCloseDevice(g_acore.alcDevice);
        g_acore.alcDevice = nullptr;
    }
}


// -------------------------------------------------------------------------------------------------
// SLOTS
// -------------------------------------------------------------------------------------------------

static int avail_slot_id()
{
    auto result = g_acore.nextId;
    g_acore.nextId += 1;
    return result;
}

int audio_core_slot_init(const std::vector<char> &data, const ags::String &extension_hint, bool repeat)
{
    auto handle = avail_slot_id();

    ALuint source_;
    alGenSources(1, &source_);
    dump_al_errors();

    auto promise = std::promise<std::vector<char>>();
    promise.set_value(std::move(data));

    auto decoder = OpenALDecoder(source_, promise.get_future(), extension_hint, repeat);

    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[handle] = std::make_unique<AudioCoreSlot>(handle, source_, std::move(decoder));
    g_acore.mixer_cv.notify_all();

    return handle;
}

// -------------------------------------------------------------------------------------------------
// SLOT CONTROL
// -------------------------------------------------------------------------------------------------

void audio_core_slot_play(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->decoder_.Play();
    g_acore.mixer_cv.notify_all();
}

void audio_core_slot_pause(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->decoder_.Pause();
    g_acore.mixer_cv.notify_all();
}

void audio_core_slot_stop(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->decoder_.Stop();
    g_acore.mixer_cv.notify_all();
}

void audio_core_slot_seek_ms(int slot_handle, float pos_ms)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->decoder_.Seek(pos_ms);
    g_acore.mixer_cv.notify_all();
}


// -------------------------------------------------------------------------------------------------
// SLOT CONFIG
// -------------------------------------------------------------------------------------------------

void audio_core_set_master_volume(float newvol) 
{
    alListenerf(AL_GAIN, newvol*GlobalGainScaling);
    dump_al_errors();
}

void audio_core_slot_configure(int slot_handle, float volume, float speed, float panning)
{
    ALuint source_ = g_acore.slots_[slot_handle]->source_;

    alSourcef(source_, AL_GAIN, volume*0.7f);
    dump_al_errors();

    alSourcef(source_, AL_PITCH, speed);
    dump_al_errors();

    if (panning != 0.0f) {
        // https://github.com/kcat/openal-soft/issues/194
        alSourcef(source_, AL_ROLLOFF_FACTOR, 0.0f);
        dump_al_errors();
        alSourcei(source_, AL_SOURCE_RELATIVE, AL_TRUE);
        dump_al_errors();
        alSource3f(source_, AL_POSITION, panning, 0.0f, 0.0f);
        dump_al_errors();
    }
    else {
        alSourcef(source_, AL_ROLLOFF_FACTOR, 1.0f);
        dump_al_errors();
        alSourcei(source_, AL_SOURCE_RELATIVE, AL_FALSE);
        dump_al_errors();
        alSource3f(source_, AL_POSITION, 0.0f, 0.0f, 0.0f);
        dump_al_errors();
    }
}

// -------------------------------------------------------------------------------------------------
// SLOT STATUS
// -------------------------------------------------------------------------------------------------

float audio_core_slot_get_pos_ms(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    return g_acore.slots_[slot_handle]->decoder_.GetPositionMs();
    g_acore.mixer_cv.notify_all();
}
AudioCorePlayState audio_core_slot_get_play_state(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    return g_acore.slots_[slot_handle]->decoder_.GetPlayState();
    g_acore.mixer_cv.notify_all();
}


// -------------------------------------------------------------------------------------------------
// AUDIO PROCESSING
// -------------------------------------------------------------------------------------------------

static void audio_core_entry()
{
    std::unique_lock<std::mutex> lk(g_acore.mixer_mutex_m);

    while (g_acore.audio_core_thread_running) {
        
        // burn off any errors for new loop
        dump_al_errors();

        for (auto &entry : g_acore.slots_) {
            auto &slot = entry.second;

            try {
                slot->decoder_.Poll();
            } catch (const std::exception& e) {
                agsdbg::Printf(ags::kDbgMsg_Error, "OpenALDecoder poll exception %s", e.what());
            }
        }

        g_acore.mixer_cv.wait_for(lk, std::chrono::milliseconds(50));
    }
}


// -------------------------------------------------------------------------------------------------
// UTILITY
// -------------------------------------------------------------------------------------------------
// TODO: originally written by [sonneveld] for OpenAL sound impl,
// investigate if it's possible to avoid using full sound data here,
// maybe by letting decoder read a header from stream and find out format.
float audio_core_get_sound_length_ms(const std::vector<char> &data, const char *extension_hint)
{
    auto sample = SoundSampleUniquePtr(Sound_NewSampleFromMem((Uint8 *)data.data(), data.size(), extension_hint, nullptr, 32 * 1024));
    if (sample == nullptr) { return -1; }

    return Sound_GetDuration(sample.get());
}

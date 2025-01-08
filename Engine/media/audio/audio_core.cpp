//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "media/audio/audio_core.h"
#include <math.h>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include "debug/out.h"
#include "media/audio/audioplayer.h"
#include "media/audio/sdldecoder.h"
#include "media/audio/openalsource.h"
#include "util/memory_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// Global audio core state and resources
static struct 
{
    // Device handle (could be a real hardware, or a service/server)
    ALCdevice *alcDevice = nullptr;
    // Context handle (all OpenAL operations are performed using the current context)
    ALCcontext *alcContext = nullptr;

    // Audio thread: polls sound decoders, feeds OpenAL sources
    std::thread audio_core_thread;
    bool audio_core_thread_running = false;

    // Sound slot id counter
    int nextId = 0;

    // One mutex to lock them all... any operation on individual decoders
    // is done under this only mutex, which means that they are currently
    // polled one by one, any action like pause/resume is also synced.
    std::mutex mixer_mutex_m;
    std::condition_variable mixer_cv;
    std::unordered_map<int, std::unique_ptr<AudioPlayer>> slots_;
} g_acore;

// Prints any OpenAL errors to the log
void dump_al_errors()
{
    auto err = alGetError();
    if (err == AL_NO_ERROR) { return; }
    Debug::Printf(kDbgMsg_Error, "OpenAL Error: %s", alGetString(err));
    assert(err == AL_NO_ERROR);
}

// -------------------------------------------------------------------------------------------------
// INIT / SHUTDOWN
// -------------------------------------------------------------------------------------------------

static void audio_core_entry();

void audio_core_init() 
{
    /* InitAL opens a device and sets up a context using default attributes, making
     * the program ready to call OpenAL functions. */

    /* Open and initialize a device */
    g_acore.alcDevice = alcOpenDevice(nullptr);
    if (!g_acore.alcDevice) { throw std::runtime_error("AudioCore: error opening device"); }

    g_acore.alcContext = alcCreateContext(g_acore.alcDevice, nullptr);
    if (!g_acore.alcContext) { throw std::runtime_error("AudioCore: error creating context"); }

    if (alcMakeContextCurrent(g_acore.alcContext) == ALC_FALSE) { throw std::runtime_error("AudioCore: error setting context"); }

    const ALCchar *name = nullptr;
    if (alcIsExtensionPresent(g_acore.alcDevice, "ALC_ENUMERATE_ALL_EXT"))
        name = alcGetString(g_acore.alcDevice, ALC_ALL_DEVICES_SPECIFIER);
    if (!name || alcGetError(g_acore.alcDevice) != AL_NO_ERROR)
        name = alcGetString(g_acore.alcDevice, ALC_DEVICE_SPECIFIER);
    Debug::Printf(kDbgMsg_Info, "AudioCore: opened device \"%s\"", name);

    // SDL_Sound
    Sound_Init();
    Debug::Printf(kDbgMsg_Info, "Supported sound decoders:");
    for (const auto **dec = Sound_AvailableDecoders(); *dec; ++dec)
    {
        String buf;
        for (const auto **ext = (*dec)->extensions; *ext; ++ext)
            buf.AppendFmt("%s,", *ext);
        Debug::Printf(kDbgMsg_Info, " - %s : %s", (*dec)->description, buf.GetCStr());
    }

    g_acore.audio_core_thread_running = true;
#if !defined(AGS_DISABLE_THREADS)
    g_acore.audio_core_thread = std::thread(audio_core_entry);
#endif
}

void audio_core_shutdown()
{
    g_acore.audio_core_thread_running = false;
#if !defined(AGS_DISABLE_THREADS)
    if (g_acore.audio_core_thread.joinable())
        g_acore.audio_core_thread.join();
#endif

    // dispose all the active slots
    g_acore.slots_.clear();

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
// AUDIO CORE CONFIG
// -------------------------------------------------------------------------------------------------

void audio_core_set_master_volume(float newvol) 
{
    // TODO: review this later; how do we apply master volume
    // if we use alternate audio output (e.g. from plugin)?
    alListenerf(AL_GAIN, newvol);
    dump_al_errors();
}


// -------------------------------------------------------------------------------------------------
// SLOTS
// -------------------------------------------------------------------------------------------------

static int avail_slot_id()
{
    return g_acore.nextId++;
}

static int audio_core_slot_init(std::unique_ptr<SDLDecoder> decoder)
{
    auto handle = avail_slot_id();
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[handle] = std::make_unique<AudioPlayer>(handle, std::move(decoder));
    g_acore.mixer_cv.notify_all();
    return handle;
}

int audio_core_slot_init(std::shared_ptr<std::vector<uint8_t>> &data, const String &extension_hint, bool repeat)
{
    auto decoder = std::make_unique<SDLDecoder>(data, extension_hint, repeat);
    if (!decoder->Open())
        return -1;
    return audio_core_slot_init(std::move(decoder));
}

int audio_core_slot_init(std::unique_ptr<Stream> in, const String &extension_hint, bool repeat)
{
    auto decoder = std::make_unique<SDLDecoder>(std::move(in), extension_hint, repeat);
    if (!decoder->Open())
        return -1;
    return audio_core_slot_init(std::move(decoder));
}

AudioPlayerLock audio_core_get_player(int slot_handle)
{
    std::unique_lock<std::mutex> ulk(g_acore.mixer_mutex_m);
    auto it = g_acore.slots_.find(slot_handle);
    if (it == g_acore.slots_.end())
        return AudioPlayerLock(nullptr, std::move(ulk), &g_acore.mixer_cv);
    return AudioPlayerLock(it->second.get(), std::move(ulk), &g_acore.mixer_cv);
}

void audio_core_slot_stop(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    auto it = g_acore.slots_.find(slot_handle);
    if (it == g_acore.slots_.end())
        return;
    it->second->Stop();
    g_acore.slots_.erase(slot_handle);
    g_acore.mixer_cv.notify_all();
}

// -------------------------------------------------------------------------------------------------
// AUDIO PROCESSING
// -------------------------------------------------------------------------------------------------

void audio_core_entry_poll()
{
    // burn off any errors for new loop
    dump_al_errors();

    for (auto &entry : g_acore.slots_) {
        auto &slot = entry.second;

        try {
            slot->Poll();
        } catch (const std::exception& e) {
            Debug::Printf(kDbgMsg_Error, "AudioCore poll exception: %s", e.what());
        }
    }
}

#if !defined(AGS_DISABLE_THREADS)
static void audio_core_entry()
{
    std::unique_lock<std::mutex> lk(g_acore.mixer_mutex_m);

    while (g_acore.audio_core_thread_running) {

        audio_core_entry_poll();

        g_acore.mixer_cv.wait_for(lk, std::chrono::milliseconds(50));
    }
}
#endif

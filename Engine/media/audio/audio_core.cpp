//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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
#include "media/audio/sdldecoder.h"
#include "media/audio/openalsource.h"
#include "util/memory_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

const auto GlobalGainScaling = 0.7f; // TODO: find out why 0.7f is here?

static void audio_core_entry();

// AudioCoreSlot is a single playback manager, that handles two components:
// decoder and "player"; controls the current playback state, passes data
// from the decoder into the player.
class AudioCoreSlot
{
public:
    AudioCoreSlot(int handle, std::unique_ptr<SDLDecoder> decoder);

    // Gets current playback state
    PlaybackState GetPlayState() const { return _playState; }
    // Gets duration, in ms
    float GetDurationMs() const { return _decoder->GetDurationMs(); }
    // Gets playback position, in ms
    float GetPositionMs() const { return _source->GetPositionMs(); }
    // Gives access to decoder object
    SDLDecoder &GetDecoder() const { return *_decoder; }
    // Gives access to the "player" object
    OpenAlSource &GetAlSource() const { return *_source; }

    // Update state, transfer data from decoder to player if possible
    void Poll();
    // Begin playback
    void Play();
    // Pause playback
    void Pause();
    // Stop playback completely
    void Stop();
    // Seek to the given time position
    void Seek(float pos_ms);

private:
    // Opens decoder and sets up playback state
    void Init();

    int handle_ = -1;
    std::unique_ptr<SDLDecoder> _decoder;
    std::unique_ptr<OpenAlSource> _source;
    PlaybackState _playState = PlayStateInitial;
    PlaybackState _onLoadPlayState = PlayStatePaused;
    float _onLoadPositionMs = 0.0f;
    SoundBuffer _bufferPending{};
};

AudioCoreSlot::AudioCoreSlot(int handle, std::unique_ptr<SDLDecoder> decoder)
    : handle_(handle), _decoder(std::move(decoder))
{
    _source = std::make_unique<OpenAlSource>(
        _decoder->GetFormat(), _decoder->GetChannels(), _decoder->GetFreq());
}

void AudioCoreSlot::Init()
{
    bool success;
    if (_decoder->IsValid()) // if already opened, then just seek to start
        success = _decoder->Seek(_onLoadPositionMs) == _onLoadPositionMs;
    else
        success = _decoder->Open(_onLoadPositionMs);
    _playState = success ? _onLoadPlayState : PlayStateError;
    if (_playState == PlayStatePlaying)
        _source->Play();
}

void AudioCoreSlot::Poll()
{
    if (_playState == PlaybackState::PlayStateInitial)
        Init();
    if (_playState != PlayStatePlaying)
        return;

    // Read data from Decoder and pass into the Al Source
    if (!_bufferPending.Data && !_decoder->EOS())
    { // if no buffer saved, and still something to decode, then read a buffer
        _bufferPending = _decoder->GetData();
        assert(_bufferPending.Data || (_bufferPending.Size == 0));
    }
    if (_bufferPending.Data && (_bufferPending.Size > 0))
    { // if having a buffer already, then try to put into source
        if (_source->PutData(_bufferPending) > 0)
            _bufferPending = SoundBuffer(); // clear buffer on success
    }
    _source->Poll();
    // If both finished decoding and playing, we done here.
    if (_decoder->EOS() && _source->IsEmpty())
    {
        _playState = PlayStateFinished;
    }
}

void AudioCoreSlot::Play()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPlayState = PlayStatePlaying;
        break;
    case PlayStateStopped:
        _decoder->Seek(0.0f);
        /* fall-through */
    case PlayStatePaused:
        _playState = PlayStatePlaying;
        _source->Play();
        break;
    default:
        break;
    }
}

void AudioCoreSlot::Pause()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPlayState = PlayStatePaused;
        break;
    case PlayStatePlaying:
        _playState = PlayStatePaused;
        _source->Pause();
        break;
    default:
        break;
    }
}

void AudioCoreSlot::Stop()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPlayState = PlayStateStopped;
        break;
    case PlayStatePlaying:
    case PlayStatePaused:
        _playState = PlayStateStopped;
        _source->Stop();
        _bufferPending = SoundBuffer(); // clear
        break;
    default:
        break;
    }
}

void AudioCoreSlot::Seek(float pos_ms)
{
    switch (_playState)
    {
    case PlayStateInitial:
        _onLoadPositionMs = pos_ms;
        break;
    case PlayStatePlaying:
    case PlayStatePaused:
    case PlayStateStopped:
        {
            _source->Stop();
            _bufferPending = SoundBuffer(); // clear
            float new_pos = _decoder->Seek(pos_ms);
            _source->SetPlaybackPosMs(new_pos);
        }
        break;
    default:
        break;
    }
}


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
    std::unordered_map<int, std::unique_ptr<AudioCoreSlot>> slots_;
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
    g_acore.slots_[handle] = std::make_unique<AudioCoreSlot>(handle, std::move(decoder));
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

// -------------------------------------------------------------------------------------------------
// SLOT CONTROL
// -------------------------------------------------------------------------------------------------

PlaybackState audio_core_slot_play(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->Play();
    auto state = g_acore.slots_[slot_handle]->GetPlayState();
    g_acore.mixer_cv.notify_all();
    return state;
}

PlaybackState audio_core_slot_pause(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->Pause();
    auto state = g_acore.slots_[slot_handle]->GetPlayState();
    g_acore.mixer_cv.notify_all();
    return state;
}

void audio_core_slot_stop(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->Stop();
    g_acore.slots_.erase(slot_handle);
    g_acore.mixer_cv.notify_all();
}

void audio_core_slot_seek_ms(int slot_handle, float pos_ms)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    g_acore.slots_[slot_handle]->Seek(pos_ms);
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
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    auto &player = g_acore.slots_[slot_handle]->GetAlSource();
    player.SetVolume(volume * GlobalGainScaling);
    player.SetSpeed(speed);
    player.SetPanning(panning);
}

// -------------------------------------------------------------------------------------------------
// SLOT STATUS
// -------------------------------------------------------------------------------------------------

float audio_core_slot_get_pos_ms(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    auto pos = g_acore.slots_[slot_handle]->GetAlSource().GetPositionMs();
    g_acore.mixer_cv.notify_all();
    return pos;
}

float audio_core_slot_get_duration(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    auto dur = g_acore.slots_[slot_handle]->GetDecoder().GetDurationMs();
    g_acore.mixer_cv.notify_all();
    return dur;
}

int audio_core_slot_get_freq(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    auto dur = g_acore.slots_[slot_handle]->GetDecoder().GetFreq();
    g_acore.mixer_cv.notify_all();
    return dur;
}

PlaybackState audio_core_slot_get_play_state(int slot_handle)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    auto state = g_acore.slots_[slot_handle]->GetPlayState();
    g_acore.mixer_cv.notify_all();
    return state;
}

PlaybackState audio_core_slot_get_play_state(int slot_handle, float &pos_ms)
{
    std::lock_guard<std::mutex> lk(g_acore.mixer_mutex_m);
    auto state = g_acore.slots_[slot_handle]->GetPlayState();
    pos_ms = g_acore.slots_[slot_handle]->GetAlSource().GetPositionMs();
    g_acore.mixer_cv.notify_all();
    return state;
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

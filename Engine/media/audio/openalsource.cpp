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
#include "media/audio/openalsource.h"
#include <algorithm>
#include "debug/out.h"

using namespace AGS::Common;

namespace AGS
{
namespace Engine
{

// Finds an acceptable OpenAl format representation for the given SDL audio format
static ALenum OpenAlFormatFromSDLFormat(SDL_AudioFormat fmt, int chans, int freq)
{
    if (chans == 1) {
        switch (fmt) {
        case AUDIO_U8:
            return AL_FORMAT_MONO8;
        case AUDIO_S16SYS:
            return AL_FORMAT_MONO16;
        case AUDIO_F32SYS:
            if (alIsExtensionPresent("AL_EXT_float32")) {
                return alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
            }
        }
    }
    else if (chans == 2) {
        switch (fmt) {
        case AUDIO_U8:
            return AL_FORMAT_STEREO8;
        case AUDIO_S16SYS:
            return AL_FORMAT_STEREO16;
        case AUDIO_F32SYS:
            if (alIsExtensionPresent("AL_EXT_float32")) {
                return alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
            }
        }
    }

#ifdef AUDIO_CORE_DEBUG
    Debug::Printf("OpenAlFormatFromSDLFormat: bad format - format: %d, chans: %d, freq: %d", fmt, chans, freq);
#endif

    return 0;
}

// Returns buffer duration in milliseconds
static float GetBufferMs(ALuint buf_id)
{
    ALint size;
    ALint chans;
    ALint bits;
    ALint freq;
    alGetBufferi(buf_id, AL_SIZE, &size);
    alGetBufferi(buf_id, AL_CHANNELS, &chans);
    // IMPORTANT! mojoAL is *LYING* about buffer bitness, because it reports the one
    // passed to alBufferData, but internally always works with Float32; therefore
    // final samples found in the buffer are in float32!
    //alGetBufferi(buf_id, AL_BITS, &bits);
    bits = sizeof(ALfloat) * 8;
    alGetBufferi(buf_id, AL_FREQUENCY, &freq);
    auto num_samples = size * 8 / (chans * bits);
    return 1000.0f * (float)num_samples / (float)freq;
}

// Internal OpenAl-related resources
static struct
{
    // A record of available al buffers
    std::vector<ALuint> freeBuffers;
} g_oalint;


//-----------------------------------------------------------------------------
// OpenAlSource
//-----------------------------------------------------------------------------

OpenAlSource::OpenAlSource(SDL_AudioFormat format, int channels, int freq)
{
    alGenSources(1, &_source);
    dump_al_errors();
    _inputFormat = format;
    _alFormat = OpenAlFormatFromSDLFormat(format, channels, freq);
    // FIXME: if failed to find matching format, plan resampler!
    _channels = channels;
    _freq = freq;
}

OpenAlSource::OpenAlSource(OpenAlSource&& src)
{
    _source = src._source;
    _inputFormat = src._inputFormat;
    _alFormat = src._alFormat;
    _channels = src._channels;
    _freq = src._freq;
    src._source = 0;
}

OpenAlSource::~OpenAlSource()
{
    if (_source > 0)
    {
        alSourceStop(_source);
        dump_al_errors();
        Unqueue();
        alDeleteSources(1, &_source);
        dump_al_errors();
    }
}

float OpenAlSource::GetPositionMs() const
{
    float al_offset = 0.f;
    alGetSourcef(_source, AL_SEC_OFFSET, &al_offset);
    dump_al_errors();
    float off_ms = 0.f;
    if (_bufferRecords.size() > 0)
        off_ms = al_offset * _bufferRecords.front().Speed * 1000.f;
    float pos_ms = _processedMs + off_ms;
#ifdef AUDIO_CORE_DEBUG
    Debug::Printf("proc:%f plus:%f = %f\n", _processedMs, off_ms, pos_ms);
#endif
    // Dirty fixup, in case the reported position jumps back when the playback speed changes
    pos_ms = std::max(_lastPosReport, pos_ms);
    _lastPosReport = pos_ms;
    return pos_ms;
}

size_t OpenAlSource::PutData(const SoundBuffer data)
{
    Unqueue();
    // If queue is full, bail out
    if (_queued >= MaxQueue) { return 0u; }
    // Input buffer is empty?
    if (!data.Data || (data.Size == 0)) { return 0u; }
    // Check for free buffers, generate more if necessary
    if (g_oalint.freeBuffers.size() == 0)
    {
        ALuint buf_id = 0;
        alGenBuffers(1, &buf_id);
        dump_al_errors();
        g_oalint.freeBuffers.push_back(buf_id);
    }
    // Get a free buffer
    assert(g_oalint.freeBuffers.size() > 0);
    ALuint buf_id = *(std::prev(g_oalint.freeBuffers.end()));
    g_oalint.freeBuffers.pop_back();

    SoundBuffer input_buf = data;
    if (_resampler.HasConversion())
    {
        size_t conv_sz;
        const void *conv = _resampler.Convert(data.Data, data.Size, conv_sz);
        if (conv)
        {
            input_buf = SoundBuffer(conv, conv_sz);
        }
    }
    // Fill the buffer and queue into AL; note that the al's buffer is auto-resizing
    alBufferData(buf_id, _alFormat, input_buf.Data, input_buf.Size, _freq);
    dump_al_errors();
    alSourceQueueBuffers(_source, 1, &buf_id);
    dump_al_errors();
    _queued++;
    // Push buffer record
    _bufferRecords.push_back(BufferParams(GetBufferMs(buf_id), _speed));
    return data.Size;
}

void OpenAlSource::Unqueue()
{
    for (;;)
    {
        ALint processed = -1;
        alGetSourcei(_source, AL_BUFFERS_PROCESSED, &processed);
        dump_al_errors();
        if (processed <= 0) { break; }

        ALuint buf_id;
        alSourceUnqueueBuffers(_source, 1, &buf_id);
        dump_al_errors();

        _queued--;
        assert(_bufferRecords.size() > 0);
        _processedMs += _bufferRecords.front().Time;
        _bufferRecords.pop_front();

        g_oalint.freeBuffers.push_back(buf_id);
    }
}

ALuint OpenAlSource::Poll()
{
    Unqueue();

    // Update play state
    if (_playState == PlayStateError) { return 0u; }
    if (_playState != PlayStatePlaying) { return _queued; }
    
    // If Al source is not playing for any reason, try to start it up,
    // but only if some data is queued
    if (_queued == 0) { return 0; }
    ALint state = AL_INITIAL;
    alGetSourcei(_source, AL_SOURCE_STATE, &state);
    dump_al_errors();
    if (state != AL_PLAYING)
    {
        alSourcePlay(_source);
        dump_al_errors();
    }
    return _queued;
}

void OpenAlSource::Play(float timestamp)
{
    switch (_playState)
    {
    case PlayStateInitial:
    case PlayStatePaused:
    case PlayStateStopped:
        _playState = PlayStatePlaying;
        _processedMs = timestamp;
        // If the queue is empty then do not call alSourcePlay right away,
        // because mojoAL can drop a clip out of its slot if it is not
        // initialized with something; see mojoal.c mix_source
        if (_queued > 0)
        {
            alSourcePlay(_source);
            dump_al_errors();
        }
        break;
    default:
        break;
    }
}

void OpenAlSource::Stop()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _playState = PlayStateStopped;
        break;
    case PlayStatePlaying:
    case PlayStatePaused:
        _playState = PlayStateStopped;
        _processedMs = 0;
        alSourceStop(_source);
        dump_al_errors();
        Unqueue();
        break;
    default:
        break;
    }
}

void OpenAlSource::Pause()
{
    switch (_playState)
    {
    case PlayStateInitial:
        _playState = PlayStatePaused;
        break;
    case PlayStatePlaying:
        _playState = PlayStatePaused;
        alSourcePause(_source);
        dump_al_errors();
        break;
    default:
        break;
    }
}

void OpenAlSource::Resume()
{ // function is reserved, simply call Play() for now
    Play();
}

void OpenAlSource::SetPlayTime(float timestamp)
{
    _processedMs = timestamp;
    _lastPosReport = timestamp;
}

void OpenAlSource::SetSpeed(float speed)
{
    _speed = speed;

    // Configure resample
    int new_freq = static_cast<int>(_freq / _speed);
    if (!_resampler.Setup(_inputFormat, _channels, _freq, _inputFormat, _channels, new_freq))
    { // error, reset
        _speed = 1.0;
    }
}

} // namespace Engine
} // namespace AGS

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
#include "media/audio/openalsource.h"
#include <algorithm>
#include <cmath>
#include "debug/out.h"

using namespace AGS::Common;

namespace AGS
{
namespace Engine
{

// Finds an acceptable OpenAl format representation for the given SDL audio format;
// if no direct match exists, configures a closest replacement
static ALenum OpenAlFormatFromSDLFormat(const Sound_AudioInfo &input, Sound_AudioInfo &conv)
{
    conv.rate = input.rate;
    conv.channels = std::min<uint8_t>(2, input.channels);
    conv.format = input.format;

    switch (input.format)
    {
    /* 8-bit integer samples */
    case AUDIO_U8:
    case AUDIO_S8:
        conv.format = AUDIO_U8;
        return conv.channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
    /* 16-bit integer samples */
    case AUDIO_U16LSB:
    case AUDIO_S16LSB:
    case AUDIO_U16MSB:
    case AUDIO_S16MSB:
        conv.format = AUDIO_S16SYS;
        return conv.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    /* 32-bit integer and float samples */
    case AUDIO_S32LSB:
    case AUDIO_S32MSB:
    case AUDIO_F32LSB:
    case AUDIO_F32MSB:
        if (alIsExtensionPresent("AL_EXT_float32"))
        {
            conv.format = AUDIO_F32SYS;
            return conv.channels == 1 ?
                alGetEnumValue("AL_FORMAT_MONO_FLOAT32") :
                alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
        }
        /* fall-through */
    default:
        conv.format = AUDIO_S16SYS;
        return conv.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    }
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
    _inputFmt.format = format;
    _inputFmt.channels = static_cast<Uint8>(channels);
    _inputFmt.rate = freq;
    _alFormat = OpenAlFormatFromSDLFormat(_inputFmt, _recvFmt);;
    alGenSources(1, &_source);
    dump_al_errors();
    _resampler.Setup(_inputFmt, _recvFmt);
}

OpenAlSource::OpenAlSource(OpenAlSource&& src)
{
    _inputFmt = src._inputFmt;
    _recvFmt = src._recvFmt;
    _alFormat = src._alFormat;
    _source = src._source;
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
    if (_bufferRecords.size() == 0)
        return _predictTs; // if no buf records: return ts prediction

    float al_offset = 0.f;
    alGetSourcef(_source, AL_SEC_OFFSET, &al_offset);
    dump_al_errors();
    for (const auto &r : _bufferRecords)
    {
        float dur = (r.Duration * 0.001f) / r.Speed;
        if (al_offset < dur)
        {
            float pos_ms = r.Timestamp + (al_offset * 1000.f) * r.Speed;
#ifdef AUDIO_CORE_DEBUG
            Debug::Printf("OpenAlSource: pos = %f", pos_ms);
#endif
            return pos_ms;
        }
        al_offset -= dur;
    }
    // error? offset overflows buf records: return next ts prediction
    return _predictTs;
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
    // use provided timestamp, or calc our own
    const float use_ts = data.Ts >= 0.f ? data.Ts : _predictTs;
    const float dur_ms = 
        SoundHelper::MillisecondsFromBytes(data.Size, _inputFmt.format, _inputFmt.channels, _inputFmt.rate);
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
    alBufferData(buf_id, _alFormat, input_buf.Data, input_buf.Size, _recvFmt.rate);
    dump_al_errors();
    alSourceQueueBuffers(_source, 1, &buf_id);
    dump_al_errors();
    _queued++;
    _predictTs = data.Ts >= 0.f ? (data.Ts + dur_ms) : (_predictTs + dur_ms);
    // Push buffer record
    _bufferRecords.push_back(BufferRecord(use_ts, dur_ms, _speed));
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

void OpenAlSource::Play()
{
    switch (_playState)
    {
    case PlayStateInitial:
    case PlayStateStopped:
    case PlayStatePaused:
        _playState = PlayStatePlaying;
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
        alSourceStop(_source);
        dump_al_errors();
        Unqueue();
        _playState = PlayStateStopped;
        _predictTs = 0.f;
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

void OpenAlSource::SetPlaybackPosMs(float pos_ms)
{
    _predictTs = pos_ms;
}

void OpenAlSource::SetPanning(float panning)
{
    if (panning != 0.0f) {
        // https://github.com/kcat/openal-soft/issues/194
        alSourcei(_source, AL_SOURCE_RELATIVE, AL_TRUE);
        dump_al_errors();
        alSource3f(_source, AL_POSITION, panning, 0.0f, -sqrtf(1.0f - panning*panning));
        dump_al_errors();
    }
    else {
        alSourcei(_source, AL_SOURCE_RELATIVE, AL_FALSE);
        dump_al_errors();
        alSource3f(_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        dump_al_errors();
    }
}

void OpenAlSource::SetSpeed(float speed)
{
    _speed = speed;

    // Configure resample
    int new_freq = static_cast<int>(_recvFmt.rate / _speed);
    if (!_resampler.Setup(_inputFmt.format, _inputFmt.channels, _inputFmt.rate,
        _recvFmt.format, _recvFmt.channels, new_freq))
    { // error, reset
        _speed = 1.0;
    }
}

void OpenAlSource::SetVolume(float volume)
{
    alSourcef(_source, AL_GAIN, volume);
    dump_al_errors();
}

} // namespace Engine
} // namespace AGS

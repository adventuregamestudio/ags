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
#include "media/audio/openaldecoder.h"
#include <array>
#include <SDL.h>
#include "debug/out.h"

namespace ags = AGS::Common;
namespace agsdbg = AGS::Common::Debug;


void dump_al_errors()
{
    bool errorFound = false;
    for (;;) {
        auto err = alGetError();
        if (err == AL_NO_ERROR) { break; }
        errorFound = true;
        agsdbg::Printf(ags::kDbgMsg_Error, "error: %s", alGetString(err));
    }
    assert(!errorFound);
}


bool SDLResampler::Setup(SDL_AudioFormat src_fmt, uint8_t src_chans, int src_rate,
    SDL_AudioFormat dst_fmt, uint8_t dst_chans, int dst_rate)
{
    SDL_zero(_cvt);
    return SDL_BuildAudioCVT(&_cvt, src_fmt, src_chans, src_rate,
        dst_fmt, dst_chans, dst_rate) >= 0;
}

void *SDLResampler::Convert(void *data, size_t sz, size_t &out_sz)
{
    if (_buf.size() < sz * _cvt.len_mult)
    {
        size_t len = sz * _cvt.len_mult;
        _buf.resize(len);
    }
    _cvt.buf = &_buf[0];
    _cvt.len = _cvt.len_cvt = sz;
    SDL_memcpy(_cvt.buf, data, sz);
    if (SDL_ConvertAudio(&_cvt) < 0)
        return nullptr;
    out_sz = _cvt.len_cvt;
    return &_buf[0];
}


const auto SampleDefaultBufferSize = 64 * 1024;

static struct
{
    std::vector<ALuint> freeBuffers;
} g_oaldec;


float OpenALDecoder::buffer_duration_ms(ALuint bufferID) {
    ALint sizeInBytes;
    ALint channels;
    ALint bits;
    ALint frequency;

    alGetBufferi(bufferID, AL_SIZE, &sizeInBytes);
    alGetBufferi(bufferID, AL_CHANNELS, &channels);
    alGetBufferi(bufferID, AL_BITS, &bits);
    alGetBufferi(bufferID, AL_FREQUENCY, &frequency);

    auto lengthInSamples = sizeInBytes * 8 / (channels * bits);
    return 1000.0f * (float)lengthInSamples / (float)frequency;
}


ALenum OpenALDecoder::openalFormatFromSample(const SoundSampleUniquePtr &sample) {
    if (sample->desired.channels == 1) {
        switch (sample->desired.format) {
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
    else if (sample->desired.channels == 2) {
        switch (sample->desired.format) {
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
    agsdbg::Printf("openalFormatFromSample: bad format chan:%d format: %d", sample->actual.channels, sample->actual.format);
#endif

    return 0;
}

void OpenALDecoder::DecoderUnqueueProcessedBuffers()
{
    for (;;) {
        ALint buffersProcessed = -1;
        alGetSourcei(source_, AL_BUFFERS_PROCESSED, &buffersProcessed);
        dump_al_errors();
        if (buffersProcessed <= 0) { break; }

        ALuint b;
        alSourceUnqueueBuffers(source_, 1, &b);
        dump_al_errors();

        processedBuffersDurationMs_ += buffer_duration_ms(b);

        g_oaldec.freeBuffers.push_back(b);
    }
}

void OpenALDecoder::PollBuffers()
{
    // buffer management
    DecoderUnqueueProcessedBuffers();

    // generate extra buffers if none are free
    if (g_oaldec.freeBuffers.size() < MaxQueue) {
        std::array<ALuint, MaxQueue> genbufs;
        alGenBuffers(genbufs.size(), genbufs.data());
        dump_al_errors();
        for (const auto &b : genbufs) {
            g_oaldec.freeBuffers.push_back(b);
        }
    }

    // decode and attach buffers
    while (!EOS_) {
        ALint buffersQueued = -1;
        alGetSourcei(source_, AL_BUFFERS_QUEUED, &buffersQueued);
        dump_al_errors();

        if (buffersQueued >= MaxQueue) { break; }

        assert(g_oaldec.freeBuffers.size() > 0);
        auto it = std::prev(g_oaldec.freeBuffers.end());
        auto b = *it;

        auto sz = Sound_Decode(sample_.get());

        // If read less than the buffer size - that means
        // either we reached end of sound stream OR decoding error occured
        if (sz < sample_->buffer_size) {
            EOS_ = true;
            if ((sample_->flags & SOUND_SAMPLEFLAG_ERROR) != 0) {
                playState_ = PlayStateError;
            }
            // if repeat, then seek to start.
            else if (repeat_) {
                auto res = Sound_Rewind(sample_.get());
                auto success = (res != 0);
                EOS_ = !success;
            }
        }
        // Nothing was decoded last time - skip
        if (sz == 0) { continue; }

        void *input_buf = sample_->buffer;
        size_t input_sz = sz;
        if (resampler_.HasConversion())
        {
            size_t conv_sz;
            void *conv = resampler_.Convert(sample_->buffer, sz, conv_sz);
            if (conv)
            {
                input_buf = conv;
                input_sz = conv_sz;
            }
        }

        alBufferData(b, sampleOpenAlFormat_, input_buf, input_sz, sample_->desired.rate);
        dump_al_errors();

        alSourceQueueBuffers(source_, 1, &b);
        dump_al_errors();

        g_oaldec.freeBuffers.erase(it);
    }
}


OpenALDecoder::OpenALDecoder(ALuint source, const std::vector<char> &sampleBuf,
                             AGS::Common::String sampleExt, bool repeat)
    : source_(source)
    , sampleData_(std::move(sampleBuf))
    , sampleExt_(sampleExt)
    , repeat_(repeat)
{
}

OpenALDecoder::OpenALDecoder(OpenALDecoder&& dec)
{
    source_ = dec.source_;
    dec.source_ = 0;
    sampleData_ = (std::move(dec.sampleData_));
    sampleExt_ = std::move(dec.sampleExt_);
    repeat_ = dec.repeat_;
    dec.repeat_ = false;
}

OpenALDecoder::~OpenALDecoder()
{
    if (source_ > 0) {
        alSourceStop(source_);
        DecoderUnqueueProcessedBuffers();
        alDeleteSources(1, &source_);
        dump_al_errors();
    }
}

bool OpenALDecoder::Init()
{
    if (playState_ != PlaybackState::PlayStateInitial)
        return true; // already inited, nothing to do

    auto sample = SoundSampleUniquePtr(Sound_NewSampleFromMem(
        (uint8_t *)sampleData_.data(), sampleData_.size(), sampleExt_.GetCStr(), nullptr, SampleDefaultBufferSize));
    if (!sample) {
        playState_ = PlaybackState::PlayStateError;
        return false;
    }

    auto bufferFormat = openalFormatFromSample(sample);

    if (bufferFormat <= 0) {
#ifdef AUDIO_CORE_DEBUG
        agsdbg::Printf("audio_core_sample_load: RESAMPLING");
#endif
        auto desired = Sound_AudioInfo{ AUDIO_S16SYS, sample->actual.channels, sample->actual.rate };

        Sound_FreeSample(sample.get());
        sample = SoundSampleUniquePtr(Sound_NewSampleFromMem((uint8_t *)sampleData_.data(), sampleData_.size(), sampleExt_.GetCStr(), &desired, SampleDefaultBufferSize));

        if (!sample) {
            playState_ = PlaybackState::PlayStateError;
            return false;
        }

        bufferFormat = openalFormatFromSample(sample);
    }

    if (bufferFormat <= 0) {
        playState_ = PlaybackState::PlayStateError;
        return false;
    }

    sample_ = std::move(sample);
    sampleOpenAlFormat_ = bufferFormat;
    duration_ = Sound_GetDuration(sample_.get());

    playState_ = onLoadPlayState_;
    if (onLoadPositionMs >= 0.0f) {
        Seek(onLoadPositionMs);
    }
    return true;
}

void OpenALDecoder::Poll()
{
    if (playState_ == PlaybackState::PlayStateError) { return; }

    if (playState_ == PlaybackState::PlayStateInitial) {
        Init();
    }

    if (playState_ != PlayStatePlaying) { return; }

    PollBuffers();

    // setup play state

    ALint state = AL_INITIAL;
    alGetSourcei(source_, AL_SOURCE_STATE, &state);
    dump_al_errors();

    if (state != AL_PLAYING) {
        alSourcePlay(source_);
        dump_al_errors();
    }

    // if end of stream and still not playing, we done here.
    ALint buffersRemaining = 0;
    alGetSourcei(source_, AL_BUFFERS_QUEUED, &buffersRemaining);
    dump_al_errors();
    if (EOS_ && buffersRemaining <= 0) {
        playState_ = PlayStateFinished;
    }

}

void OpenALDecoder::Play()
{
    switch (playState_) {
    case PlayStateError:
        break;
    case PlayStateInitial:
        onLoadPlayState_ = PlayStatePlaying;
        break;
    case PlayStateStopped:
        Seek(0.0f);
    case PlayStatePaused:
        playState_ = PlaybackState::PlayStatePlaying;
        // we poll some data before alSourcePlay because mojoAL
        // can drop a clip out of its slot if it is not initialized with something
        // see mojoal.c mix_source
        PollBuffers();
        alSourcePlay(source_);
        dump_al_errors();
        break;
    default:
        break;
    }
}

void OpenALDecoder::Pause()
{
    switch (playState_) {
    case PlayStateError:
        break;
    case PlayStateInitial:
        onLoadPlayState_ = PlayStatePaused;
        break;
    case PlayStatePlaying:
        playState_ = PlaybackState::PlayStatePaused;
        alSourcePause(source_);
        dump_al_errors();
        break;
    default:
        break;
    }
}

void OpenALDecoder::Stop()
{
    switch (playState_) {
    case PlayStateError:
        break;
    case PlayStateInitial:
        onLoadPlayState_ = PlayStateStopped;
        break;
    case PlayStatePlaying:
        playState_ = PlaybackState::PlayStateStopped;
        alSourceStop(source_);
        DecoderUnqueueProcessedBuffers();
        dump_al_errors();
        break;
    default:
        break;
    }
}

void OpenALDecoder::Seek(float pos_ms)
{
    switch (playState_) {
    case PlayStateError:
        break;
    case PlayStateInitial:
        onLoadPositionMs = pos_ms;
        break;
    default:
        alSourceStop(source_);
        dump_al_errors();
        DecoderUnqueueProcessedBuffers();
        Sound_Seek(sample_.get(), pos_ms);
        processedBuffersDurationMs_ = pos_ms;
        // will play when buffers are added in poll
    }
}

PlaybackState OpenALDecoder::GetPlayState()
{
    return playState_;
}

float OpenALDecoder::GetPositionMs()
{
    float alSecOffset = 0.0f;
    alGetSourcef(source_, AL_SEC_OFFSET, &alSecOffset);
    dump_al_errors();
    auto positionMs_ = processedBuffersDurationMs_ + alSecOffset*1000.0f;
#ifdef AUDIO_CORE_DEBUG
    agsdbg::Printf("proc:%f plus:%f = %f\n", processedBuffersDurationMs_, alSecOffset*1000.0f, positionMs_);
#endif
    return positionMs_;
}

float OpenALDecoder::GetDurationMs()
{
    return duration_;
}

void OpenALDecoder::SetSpeed(float speed)
{
    speed_ = speed;

    // Configure resample
    int new_freq = static_cast<int>(sample_->desired.rate / speed_);
    if (!resampler_.Setup(sample_->desired.format, sample_->desired.channels,
        (int)sample_->desired.rate, sample_->desired.format, sample_->desired.channels, new_freq))
    { // error, reset
        speed_ = 1.0;
    }
}

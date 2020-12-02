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


const auto SourceMinimumQueuedBuffers = 2;
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
    agsdbg::Printf(ags::kDbgMsg_Debug, "openalFormatFromSample: bad format chan:%d format: %d", sample->actual.channels, sample->actual.format);
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
    if (g_oaldec.freeBuffers.size() < SourceMinimumQueuedBuffers) {
        std::array<ALuint, SourceMinimumQueuedBuffers> genbufs;
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

        if (buffersQueued >= SourceMinimumQueuedBuffers) { break; }

        assert(g_oaldec.freeBuffers.size() > 0);
        auto it = std::prev(g_oaldec.freeBuffers.end());
        auto b = *it;

        auto sz = Sound_Decode(sample_.get());

        if (sz <= 0) {
            EOS_ = true;
            // if repeat, then seek to start.
            if (repeat_) {
                auto res = Sound_Rewind(sample_.get());
                auto success = (res != 0);
                EOS_ = !success;
            }
            continue;
        }

        alBufferData(b, sampleOpenAlFormat_, sample_->buffer, sz, sample_->desired.rate);
        dump_al_errors();

        alSourceQueueBuffers(source_, 1, &b);
        dump_al_errors();

        g_oaldec.freeBuffers.erase(it);
    }
}


OpenALDecoder::OpenALDecoder(ALuint source, std::future<std::vector<char>> sampleBufFuture, AGS::Common::String sampleExt, bool repeat)
    : source_(source), sampleBufFuture_(std::move(sampleBufFuture)), sampleExt_(sampleExt), repeat_(repeat) {

}

void OpenALDecoder::Poll()
{
    if (playState_ == AudioCorePlayState::PlayStateError) { return; }

    if (playState_ == AudioCorePlayState::PlayStateInitial) {

        if (sampleBufFuture_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) { return; }

        sampleData_ = std::move(sampleBufFuture_.get());

        auto sample = SoundSampleUniquePtr(Sound_NewSampleFromMem((uint8_t *)sampleData_.data(), sampleData_.size(), sampleExt_.GetCStr(), nullptr, SampleDefaultBufferSize));
        if (!sample) { playState_ = AudioCorePlayState::PlayStateError; return; }

        auto bufferFormat = openalFormatFromSample(sample);

        if (bufferFormat <= 0) {
#ifdef AUDIO_CORE_DEBUG
            agsdbg::Printf(ags::kDbgMsg_Debug, "audio_core_sample_load: RESAMPLING");
#endif
            auto desired = Sound_AudioInfo{ AUDIO_S16SYS, sample->actual.channels, sample->actual.rate };

            Sound_FreeSample(sample.get());
            sample = SoundSampleUniquePtr(Sound_NewSampleFromMem((uint8_t *)sampleData_.data(), sampleData_.size(), sampleExt_.GetCStr(), &desired, SampleDefaultBufferSize));

            if (!sample) { playState_ = AudioCorePlayState::PlayStateError; return; }

            bufferFormat = openalFormatFromSample(sample);
        }

        if (bufferFormat <= 0) { playState_ = AudioCorePlayState::PlayStateError; return; }

        sample_ = std::move(sample);
        sampleOpenAlFormat_ = bufferFormat;

        playState_ = onLoadPlayState_;
        if (onLoadPositionMs >= 0.0f) {
            Seek(onLoadPositionMs);
        }

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
        playState_ = AudioCorePlayState::PlayStatePlaying;
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
        playState_ = AudioCorePlayState::PlayStatePaused;
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
        playState_ = AudioCorePlayState::PlayStateStopped;
        alSourceStop(source_);
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

AudioCorePlayState OpenALDecoder::GetPlayState()
{
    return playState_;
}

float OpenALDecoder::GetPositionMs()
{
    float alSecOffset = 0.0f;
    // disabled with mojoal
    // if source available:
    alGetSourcef(source_, AL_SEC_OFFSET, &alSecOffset);
    dump_al_errors();
    auto positionMs_ = processedBuffersDurationMs_ + alSecOffset*1000.0f;
#ifdef AUDIO_CORE_DEBUG
    printf("proc:%f plus:%f = %f\n", processedBuffersDurationMs_, alSecOffset*1000.0f, positionMs_);
#endif
    return positionMs_;
}

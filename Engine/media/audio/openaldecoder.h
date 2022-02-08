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
//
// OpenAL Decoder.
//
// TODO: support sound data streaming! research what means are available to use
// along with SDL_Sound and Sound_Sample API;
// maybe utilizing SDL RWops?
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__OPENALDECODER_H
#define __AGS_EE_MEDIA__OPENALDECODER_H
#include <future>
#include <deque>
#include <SDL_sound.h>
#include "media/audio/audiodefines.h"
#include "media/audio/openal.h"
#include "util/string.h"
#ifdef AUDIO_CORE_DEBUG
#include "debug/out.h"
#endif

// RAII wrapper over SDL_Sound sample
struct SoundSampleDeleterFunctor {
    void operator()(Sound_Sample* p) {
        Sound_FreeSample(p);
#ifdef AUDIO_CORE_DEBUG
        AGS::Common::Debug::Printf("SoundSampleDeleterFunctor");
#endif
    }
};
using SoundSampleUniquePtr = std::unique_ptr<Sound_Sample, SoundSampleDeleterFunctor>;

// RAII wrapper over SDL resampling filter
struct SDLResampler
{
public:
    SDLResampler() = default;
    SDLResampler(SDL_AudioFormat src_fmt, uint8_t src_chans, int src_rate,
        SDL_AudioFormat dst_fmt, uint8_t dst_chans, int dst_rate)
        { Setup(src_fmt, src_chans, src_rate, dst_fmt, dst_chans, dst_rate); }
    bool HasConversion() const { return _cvt.needed > 0; }
    bool Setup(SDL_AudioFormat src_fmt, uint8_t src_chans, int src_rate,
        SDL_AudioFormat dst_fmt, uint8_t dst_chans, int dst_rate);
    void *Convert(void *data, size_t sz, size_t &out_sz);

private:
    SDL_AudioCVT _cvt{};
    std::vector<uint8_t> _buf;
};


class OpenALDecoder
{
public:
    OpenALDecoder(ALuint source, const std::vector<char> &sampleBuf,
                  AGS::Common::String sampleExt, bool repeat);
    OpenALDecoder(OpenALDecoder&& dec);
    ~OpenALDecoder();
    // Try initializing the sound sample
    bool Init();
    void Poll();
    void Play();
    void Pause();
    void Stop();
    void Seek(float pos_ms);
    PlaybackState GetPlayState();
    float GetPositionMs();
    float GetDurationMs();
    void SetSpeed(float speed);

private:
    static const int MaxQueue = 2;

    ALuint source_;

    bool repeat_ = false;
    float speed_ = 1.f; // change in playback rate

    PlaybackState playState_ = PlayStateInitial;

    std::vector<char> sampleData_{};
    AGS::Common::String sampleExt_ = "";
    ALenum sampleOpenAlFormat_ = 0;
    SoundSampleUniquePtr sample_ = nullptr;
    float duration_ = 0.f;

    // SDL resampler state, in case dynamic resampling in necessary
    SDLResampler resampler_;

    PlaybackState onLoadPlayState_ = PlayStatePaused;
    float onLoadPositionMs = 0.0f;
    bool EOS_ = false;
    float processedBuffersDurationMs_ = 0.0f;
    float lastPosReport = 0.f; // to fixup reported position, in case speed changes

    // Keeping record of some precalculated buffer properties
    struct BufferParams
    {
        float AlTime = 0.f; // buffer time in internal openal's rate (in seconds)
        float Speed = 0.f; // associated playback speed
        float Time = 0.f; // buffer time in the real playback rate (speed-adjusted)
        BufferParams() = default;
        BufferParams(float t, float sp) : AlTime(t), Speed(sp), Time(t * sp) {}
    };
    // playback speeds related to queued buffers
    std::deque<BufferParams> bufferRecords;

    static ALenum openalFormatFromSample(const SoundSampleUniquePtr &sample);
    void DecoderUnqueueProcessedBuffers();
    void PollBuffers();
};


void dump_al_errors();

#endif // __AGS_EE_MEDIA__OPENALDECODER_H

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
//=============================================================================
#ifndef __AGS_EE_MEDIA__OPENALDECODER_H
#define __AGS_EE_MEDIA__OPENALDECODER_H
#include <future>
#include "media/audio/openal.h"
#include <SDL_sound.h>
#include "media/audio/audio_core_defs.h"
#include "util/string.h"

struct SoundSampleDeleterFunctor {
    void operator()(Sound_Sample* p) {
        Sound_FreeSample(p);
#ifdef AUDIO_CORE_DEBUG
        agsdbg::Printf(ags::kDbgMsg_Init, "SoundSampleDeleterFunctor");
#endif
    }
};

using SoundSampleUniquePtr = std::unique_ptr<Sound_Sample, SoundSampleDeleterFunctor>;

class OpenALDecoder
{
public:
    OpenALDecoder(ALuint source, std::future<std::vector<char>> sampleBufFuture, AGS::Common::String sampleExt, bool repeat);
    void Poll();
    void Play();
    void Pause();
    void Stop();
    void Seek(float pos_ms);
    AudioCorePlayState GetPlayState();
    float GetPositionMs();

private:
    ALuint source_;

    bool repeat_;

    AudioCorePlayState playState_ = PlayStateInitial;

    std::future<std::vector<char>> sampleBufFuture_{};
    std::vector<char> sampleData_{};
    AGS::Common::String sampleExt_ = "";
    ALenum sampleOpenAlFormat_ = 0;
    SoundSampleUniquePtr sample_ = nullptr;

    AudioCorePlayState onLoadPlayState_ = PlayStatePaused;
    float onLoadPositionMs = 0.0f;

    float processedBuffersDurationMs_ = 0.0f;

    bool EOS_ = false;

    static float buffer_duration_ms(ALuint bufferID);
    static ALenum openalFormatFromSample(const SoundSampleUniquePtr &sample);
    void DecoderUnqueueProcessedBuffers();
    void PollBuffers();
};


void dump_al_errors();

#endif // __AGS_EE_MEDIA__OPENALDECODER_H

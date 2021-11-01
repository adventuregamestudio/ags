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
#include <SDL_sound.h>
#include "media/audio/audiodefines.h"
#include "media/audio/openal.h"
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

private:
    ALuint source_;

    bool repeat_;

    PlaybackState playState_ = PlayStateInitial;

    std::vector<char> sampleData_{};
    AGS::Common::String sampleExt_ = "";
    ALenum sampleOpenAlFormat_ = 0;
    SoundSampleUniquePtr sample_ = nullptr;
    float duration_ = 0.f;

    PlaybackState onLoadPlayState_ = PlayStatePaused;
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

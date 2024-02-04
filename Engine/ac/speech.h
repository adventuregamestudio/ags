//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_AC__SPEECH_H
#define __AGS_EE_AC__SPEECH_H

#include "util/string.h"

enum SkipSpeechStyle
{
    kSkipSpeechNone         = -1,
    kSkipSpeechKeyMouseTime =  0,
    kSkipSpeechKeyTime      =  1,
    kSkipSpeechTime         =  2,
    kSkipSpeechKeyMouse     =  3,
    kSkipSpeechMouseTime    =  4,
    kSkipSpeechKey          =  5,
    kSkipSpeechMouse        =  6,

    kSkipSpeechFirst        = kSkipSpeechNone,
    kSkipSpeechLast         = kSkipSpeechMouse
};

enum SpeechMode
{
    kSpeech_TextOnly        = 0,
    kSpeech_VoiceText       = 1,
    kSpeech_VoiceOnly       = 2,

    kSpeech_First           = kSpeech_TextOnly,
    kSpeech_Last            = kSpeech_VoiceOnly
};

int user_to_internal_skip_speech(SkipSpeechStyle userval);
SkipSpeechStyle internal_skip_speech_to_user(int internal_val);
// Locates and initializes a voice pack of the given *name*, tells if successful;
// pass empty string for default voice pack.
bool init_voicepak(const AGS::Common::String &name = "");
// Gets voice pack's ID name, that is a filename without "sp_" prefix and no extension.
AGS::Common::String get_voicepak_name();
// Gets an asset's parent path for voice-over clips and data files
AGS::Common::String get_voice_assetpath();

#endif // __AGS_EE_AC__SPEECH_H

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALAUDIO_H
#define __AGS_EE_AC__GLOBALAUDIO_H

#include "speech.h"

void    SetSpeechVolume(int newvol);
int     IsSpeechVoxAvailable();
int     IsMusicVoxAvailable ();

struct CharacterInfo;
struct ScriptAudioChannel;

//=============================================================================
// Play voice-over for the active blocking speech and initialize relevant data
bool    play_voice_speech(int charid, int sndid);
// Play voice-over clip in non-blocking manner
bool    play_voice_nonblocking(int charid, int sndid, bool as_speech);
// Play voice-over clip in non-blocking manner, as a given audio type, optionally selecting an explicit channel
const ScriptAudioChannel *play_voice_clip_as_type(int charid, int sndid, int type, int chan, int priority, int repeat);
// Stop voice-over for the active speech and reset relevant data
void    stop_voice_speech();
// Stop non-blocking voice-over and revert audio volumes if necessary
void    stop_voice_nonblocking();

#endif // __AGS_EE_AC__GLOBALAUDIO_H

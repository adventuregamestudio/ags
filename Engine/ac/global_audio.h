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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALAUDIO_H
#define __AGS_EE_AC__GLOBALAUDIO_H

void    StopAmbientSound (int channel);
void    PlayAmbientSound (int channel, int sndnum, int vol, int x, int y);
int     IsChannelPlaying(int chan);
int     IsSoundPlaying();
// returns -1 on failure, channel number on success
int     PlaySoundEx(int val1, int channel);
void    StopAllSounds(int evenAmbient);

void    PlayMusicResetQueue(int newmus);
void    SeekMIDIPosition (int position);
int     GetMIDIPosition ();
int     IsMusicPlaying();
int     PlayMusicQueued(int musnum);
void    scr_StopMusic();
void    SeekMODPattern(int patnum);
void    SeekMP3PosMillis (int posn);
int     GetMP3PosMillis ();
void    SetMusicVolume(int newvol);
void    SetMusicMasterVolume(int newvol);
void    SetSoundVolume(int newvol);
void    SetChannelVolume(int chan, int newvol);
void    SetDigitalMasterVolume (int newvol);
int     GetCurrentMusic();
void    SetMusicRepeat(int loopflag);
void    PlayMP3File (const char *filename);
void    PlaySilentMIDI (int mnum);

void    SetSpeechVolume(int newvol);
void    __scr_play_speech(int who, int which);
void    SetVoiceMode (int newmod);
int     IsVoxAvailable();
int     IsMusicVoxAvailable ();

//=============================================================================

int     play_speech(int charid,int sndid);
void    stop_speech();

#endif // __AGS_EE_AC__GLOBALAUDIO_H

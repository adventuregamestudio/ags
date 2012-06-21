
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
void    PlayMP3File (char *filename);
void    PlaySilentMIDI (int mnum);


#endif // __AGS_EE_AC__GLOBALAUDIO_H

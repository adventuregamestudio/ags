/*
** ACSOUND - AGS sound system wrapper
** Copyright (C) 2002-2003, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __AC_SOUND_H
#define __AC_SOUND_H

#include "acaudio/ac_sounddefines.h"
#include "acaudio/ac_audiodefines.h"
#include "acaudio/ac_soundclip.h"
#include "acaudio/ac_ambientsound.h"

#define MUS_MIDI 1
#define MUS_MP3  2
#define MUS_WAVE 3
#define MUS_MOD  4
#define MUS_OGG  5

SOUNDCLIP *my_load_wave(const char *filename, int voll, int loop);
SOUNDCLIP *my_load_mp3(const char *filname, int voll);
SOUNDCLIP *my_load_static_mp3(const char *filname, int voll, bool loop);
SOUNDCLIP *my_load_static_ogg(const char *filname, int voll, bool loop);
SOUNDCLIP *my_load_ogg(const char *filname, int voll);
SOUNDCLIP *my_load_midi(const char *filname, int repet);
SOUNDCLIP *my_load_mod(const char *filname, int repet);

int  init_mod_player(int numVoices);
void remove_mod_player();

void force_audiostream_include();

int get_volume_adjusted_for_distance(int volume, int sndX, int sndY, int sndMaxDist);
void update_directional_sound_vol();
void update_ambient_sound_vol ();
void StopAmbientSound (int channel);
SOUNDCLIP *load_sound_from_path(int soundNumber, int volume, bool repeat);
void PlayAmbientSound (int channel, int sndnum, int vol, int x, int y);
int IsChannelPlaying(int chan);
int IsSoundPlaying();
void stop_all_sound_and_music();
void shutdown_sound();
// returns -1 on failure, channel number on success
int PlaySoundEx(int val1, int channel);
void StopAllSounds(int evenAmbient);

int play_sound_priority (int val1, int priority);
int play_sound(int val1);

extern AmbientSound ambient[MAX_SOUND_CHANNELS + 1];  // + 1 just for safety on array iterations
extern int last_sound_played[MAX_SOUND_CHANNELS + 1];

#endif // __AC_SOUND_H
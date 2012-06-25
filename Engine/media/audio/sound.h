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

#include "media/audio/soundclip.h"

SOUNDCLIP *my_load_wave(const char *filename, int voll, int loop);
SOUNDCLIP *my_load_mp3(const char *filname, int voll);
SOUNDCLIP *my_load_static_mp3(const char *filname, int voll, bool loop);
SOUNDCLIP *my_load_static_ogg(const char *filname, int voll, bool loop);
SOUNDCLIP *my_load_ogg(const char *filname, int voll);
SOUNDCLIP *my_load_midi(const char *filname, int repet);
SOUNDCLIP *my_load_mod(const char *filname, int repet);

extern int numSoundChannels;
extern int use_extra_sound_offset;

#endif // __AC_SOUND_H

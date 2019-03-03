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

#include "media/audio/audiodefines.h"

#ifndef NO_MP3_PLAYER

#include "media/audio/clip_mystaticmp3.h"
#include "media/audio/audiointernaldefs.h"
#include "media/audio/soundcache.h"

#include "platform/base/agsplatformdriver.h"

extern int our_eip;

// ALMP3 functions are not reentrant! Audio system should be locked before calling any
// of the mp3 functions and unlocked afterwards.

int MYSTATICMP3::poll()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (done || !tune || !mp3buffer) {
        done = 1;
        return done;
    }

    int oldeip = our_eip;
    our_eip = 5997;
    
    {
      int result = almp3_poll_mp3(tune);

      if (result == ALMP3_POLL_PLAYJUSTFINISHED)
      {
        if (!repeat)
        {
            done = 1;
        }
      }
    }
    our_eip = oldeip;

    return done;
}

void MYSTATICMP3::adjust_stream()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (tune)
    {
        almp3_adjust_mp3(tune, get_final_volume(), panning, speed, repeat);
    }
}

void MYSTATICMP3::adjust_volume()
{
    adjust_stream();
}

void MYSTATICMP3::set_volume(int newvol)
{
    vol = newvol;
    adjust_stream();
}

void MYSTATICMP3::set_speed(int new_speed)
{
    speed = new_speed;
    adjust_stream();
}

void MYSTATICMP3::destroy()
{
  AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
  if (tune != NULL) {
      almp3_stop_mp3(tune);
      almp3_destroy_mp3(tune);
  }
  tune = NULL;
  if (mp3buffer != NULL) {
      sound_cache_free(mp3buffer, false);
  }
  mp3buffer = NULL;

  done = 1;
}



void MYSTATICMP3::seek(int pos)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!tune) { return; }
    almp3_seek_abs_msecs_mp3(tune, pos);
}

int MYSTATICMP3::get_pos()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!tune) { return 0; }
    return almp3_get_pos_msecs_mp3(tune);
}

int MYSTATICMP3::get_pos_ms()
{
    int result = get_pos();
    return result;
}

int MYSTATICMP3::get_length_ms()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!tune) { return 0; }
    return almp3_get_length_msecs_mp3(tune);
}

int MYSTATICMP3::get_voice()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!tune) { return -1; }
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3(tune);
	return (ast != NULL ? ast->voice : -1);
}

int MYSTATICMP3::get_sound_type() {
    return MUS_MP3;
}

int MYSTATICMP3::play() {
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!tune) { return 0; }
    int result = almp3_play_ex_mp3(tune, 16384, vol, panning, 1000, repeat);

    if (result != ALMP3_OK) {
        done = 1;
        return 0;
    }
    return 1;
}

MYSTATICMP3::MYSTATICMP3() : SOUNDCLIP() {
    tune = NULL;
    mp3buffer = NULL;
}

#endif // !NO_MP3_PLAYER

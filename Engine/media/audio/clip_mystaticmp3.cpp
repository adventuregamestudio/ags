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

// ALMP3 functions are not reentrant! This mutex should be locked before calling any
// of the mp3 functions and unlocked afterwards.
AGS::Engine::Mutex _mp3_mutex;

int MYSTATICMP3::poll()
{
    _mutex.Lock();

    if (tune && !done && _destroyThis)
    {
      internal_destroy();
      _destroyThis = false;
    }

    int oldeip = our_eip;
    our_eip = 5997;
    
    if ((tune == NULL) || (!ready))
        ;
    else 
    {
      _mp3_mutex.Lock();
      int result = almp3_poll_mp3(tune);
      _mp3_mutex.Unlock();

      if (result == ALMP3_POLL_PLAYJUSTFINISHED)
      {
        if (!repeat)
        {
            done = 1;
            if (psp_audio_multithreaded)
                internal_destroy();
        }
      }
    }
    our_eip = oldeip;

    _mutex.Unlock();

    return done;
}

void MYSTATICMP3::set_volume(int newvol)
{
    vol = newvol;

    if (tune != NULL)
    {
        newvol += volModifier + directionalVolModifier;
        if (newvol < 0) newvol = 0;
        _mp3_mutex.Lock();
        almp3_adjust_mp3(tune, newvol, panning, 1000, repeat);
        _mp3_mutex.Unlock();
    }
}

void MYSTATICMP3::internal_destroy()
{
  if (tune != NULL) {
      _mp3_mutex.Lock();
      almp3_stop_mp3(tune);
      almp3_destroy_mp3(tune);
      _mp3_mutex.Unlock();
      tune = NULL;
  }
  if (mp3buffer != NULL) {
      sound_cache_free(mp3buffer, false);
      mp3buffer = NULL;
  }

  _destroyThis = false;
  done = 1;
}

void MYSTATICMP3::destroy()
{
    _mutex.Lock();

    if (psp_audio_multithreaded && _playing)
      _destroyThis = true;
    else
      internal_destroy();

    _mutex.Unlock();

    while (!done)
      AGSPlatformDriver::GetDriver()->YieldCPU();
}

void MYSTATICMP3::seek(int pos)
{
    _mp3_mutex.Lock();
    almp3_seek_abs_msecs_mp3(tune, pos);
    _mp3_mutex.Unlock();
}

int MYSTATICMP3::get_pos()
{
    _mp3_mutex.Lock();
    int result = almp3_get_pos_msecs_mp3(tune);
    _mp3_mutex.Unlock();
    return result;
}

int MYSTATICMP3::get_pos_ms()
{
    int result = get_pos();
    return result;
}

int MYSTATICMP3::get_length_ms()
{
    _mp3_mutex.Lock();
    int result = almp3_get_length_msecs_mp3(tune);
    _mp3_mutex.Unlock();
    return result;
}

void MYSTATICMP3::restart()
{
    if (tune != NULL) {
        _mp3_mutex.Lock();
        almp3_stop_mp3(tune);
        almp3_rewind_mp3(tune);
        almp3_play_mp3(tune, 16384, vol, panning);
        _mp3_mutex.Unlock();
        done = 0;

        if (!psp_audio_multithreaded)
          poll();
    }
}

int MYSTATICMP3::get_voice()
{
    _mp3_mutex.Lock();
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3(tune);
    _mp3_mutex.Unlock();
    if (ast)
        return ast->voice;
    return -1;
}

int MYSTATICMP3::get_sound_type() {
    return MUS_MP3;
}

int MYSTATICMP3::play() {
    _mp3_mutex.Lock();
    int result = almp3_play_ex_mp3(tune, 16384, vol, panning, 1000, repeat);
    _mp3_mutex.Unlock();

    if (result != ALMP3_OK) {
        destroy();
        delete this;
        return 0;
    }

    if (!psp_audio_multithreaded)
      poll();

    _playing = true;
    return 1;
}

MYSTATICMP3::MYSTATICMP3() : SOUNDCLIP() {
}

#endif // !NO_MP3_PLAYER

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
#include "util/mutex_lock.h"

#include "platform/base/agsplatformdriver.h"

extern int our_eip;

// ALMP3 functions are not reentrant! This mutex should be locked before calling any
// of the mp3 functions and unlocked afterwards.
AGS::Engine::Mutex _mp3_mutex;

int MYSTATICMP3::poll()
{
    AGS::Engine::MutexLock _lock(_mutex);

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
        AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
        int result = almp3_poll_mp3(tune);
        _lockMp3.Release();

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

    return done;
}

void MYSTATICMP3::set_volume(int newvol)
{
    vol = newvol;

    if (tune != NULL)
    {
        newvol += volModifier + directionalVolModifier;
        if (newvol < 0) newvol = 0;
        AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
        almp3_adjust_mp3(tune, newvol, panning, 1000, repeat);
    }
}

void MYSTATICMP3::internal_destroy()
{
    if (tune != NULL) {
        AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
        almp3_stop_mp3(tune);
        almp3_destroy_mp3(tune);
        _lockMp3.Release();
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
    AGS::Engine::MutexLock _lock(_mutex);

    if (psp_audio_multithreaded && _playing && !_audio_doing_crossfade)
        _destroyThis = true;
    else
        internal_destroy();

    _lock.Release();

    while (!done)
        AGSPlatformDriver::GetDriver()->YieldCPU();

    // Allow the last poll cycle to finish.
    _lock.Acquire(_mutex);
}

void MYSTATICMP3::seek(int pos)
{
    AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    almp3_seek_abs_msecs_mp3(tune, pos);
}

int MYSTATICMP3::get_pos()
{
    AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    return almp3_get_pos_msecs_mp3(tune);
}

int MYSTATICMP3::get_pos_ms()
{
    int result = get_pos();
    return result;
}

int MYSTATICMP3::get_length_ms()
{
    AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    return almp3_get_length_msecs_mp3(tune);
}

void MYSTATICMP3::restart()
{
    if (tune != NULL) {
        AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
        almp3_stop_mp3(tune);
        almp3_rewind_mp3(tune);
        almp3_play_mp3(tune, 16384, vol, panning);
        _lockMp3.Release();
        done = 0;

        if (!psp_audio_multithreaded)
            poll();
    }
}

int MYSTATICMP3::get_voice()
{
    AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3(tune);
    return (ast != NULL ? ast->voice : -1);
}

int MYSTATICMP3::get_sound_type() {
    return MUS_MP3;
}

int MYSTATICMP3::play() {
    AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    int result = almp3_play_ex_mp3(tune, 16384, vol, panning, 1000, repeat);
    _lockMp3.Release();

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

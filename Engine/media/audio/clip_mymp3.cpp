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

#include "media/audio/clip_mymp3.h"
#include "media/audio/audiointernaldefs.h"
#include "ac/common.h"               // quit()
#include "util/mutex_lock.h"

#include "platform/base/agsplatformdriver.h"


int MYMP3::poll()
{
	AGS::Engine::MutexLock _lock(_mutex);

    if (!done && _destroyThis)
    {
      internal_destroy();
      _destroyThis = false;
    }

    if (done)
    {
        return done;
    }
    if (paused)
    {
        return 0;
    }

    if (!done) {
        // update the buffer
		AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
        char *tempbuf = (char *)almp3_get_mp3stream_buffer(stream);
		_lockMp3.Release(); // forced release

        if (tempbuf != NULL) {
            int free_val = -1;
            if (chunksize >= in->todo) {
                chunksize = in->todo;
                free_val = chunksize;
            }
            pack_fread(tempbuf, chunksize, in);

			_lockMp3.Acquire(_mp3_mutex);
            almp3_free_mp3stream_buffer(stream, free_val);
			_lockMp3.Release(); // forced release
        }
    }

	AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    int result = almp3_poll_mp3stream(stream);
	_lockMp3.Release();

    if (result == ALMP3_POLL_PLAYJUSTFINISHED)
    {
        done = 1;
        if (psp_audio_multithreaded)
            internal_destroy();
    }

    return done;
}

void MYMP3::adjust_stream()
{
    int final_vol = vol + volModifier + directionalVolModifier;
    if (final_vol < 0) final_vol = 0;
    AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    almp3_adjust_mp3stream(stream, final_vol, panning, 1000);
}

void MYMP3::set_volume(int newvol)
{
    // boost MP3 volume
    newvol += 20;
    if (newvol > 255)
        newvol = 255;

    vol = newvol;
    adjust_stream();
}

void MYMP3::internal_destroy()
{
	AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
		
	if (!done)
		almp3_stop_mp3stream(stream);
		
	almp3_destroy_mp3stream(stream);

	_lockMp3.Release(); // this would happen anyway, just force it to release NOW

    stream = NULL;

    if (buffer != NULL)
        free(buffer);

    buffer = NULL;
    pack_fclose(in);

    _destroyThis = false;
    done = 1;
}

void MYMP3::destroy()
{
	AGS::Engine::MutexLock _lock(_mutex);

    if (psp_audio_multithreaded && _playing && !_audio_doing_crossfade)
      _destroyThis = true;
    else
      internal_destroy();

	_lock.Release();

    while (!done)
      AGSPlatformDriver::GetDriver()->YieldCPU();
}

void MYMP3::seek(int pos)
{
    quit("Tried to seek an mp3stream");
}

int MYMP3::get_pos()
{
    return 0; // Return 0 to signify that Seek is not supported
    // return almp3_get_pos_msecs_mp3stream (stream);
}

int MYMP3::get_pos_ms()
{
	AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    return almp3_get_pos_msecs_mp3stream(stream);
}

int MYMP3::get_length_ms()
{
	AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    return almp3_get_length_msecs_mp3stream(stream, filesize);
}

void MYMP3::restart()
{
    if (stream != NULL) {
        // need to reset file pointer for this to work
		AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
        almp3_play_mp3stream(stream, MP3CHUNKSIZE, vol, panning);
		_lockMp3.Release();
        done = 0;
        paused = 0;

        if (!psp_audio_multithreaded)
          poll();
    }
}

int MYMP3::get_voice()
{
	AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3stream(stream);
    return (ast != NULL ? ast->voice : -1);
}

int MYMP3::get_sound_type() {
    return MUS_MP3;
}

int MYMP3::play() {
	AGS::Engine::MutexLock _lockMp3(_mp3_mutex);
    almp3_play_mp3stream(stream, chunksize, (vol > 230) ? vol : vol + 20, panning);
	_lockMp3.Release();

    if (!psp_audio_multithreaded)
      poll();

    _playing = true;

    return 1;
}

MYMP3::MYMP3() : SOUNDCLIP() {
}

#endif // !NO_MP3_PLAYER
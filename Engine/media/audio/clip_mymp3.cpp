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

#include "platform/base/agsplatformdriver.h"


int MYMP3::poll()
{
    _mutex.Lock();

    if (!done && _destroyThis)
    {
      internal_destroy();
      _destroyThis = false;
    }

    if (done)
    {
        _mutex.Unlock();
        return done;
    }
    if (paused)
    {
        _mutex.Unlock();
        return 0;
    }

    if (!done) {
        // update the buffer
        _mp3_mutex.Lock();
        char *tempbuf = (char *)almp3_get_mp3stream_buffer(stream);
        _mp3_mutex.Unlock();

        if (tempbuf != NULL) {
            int free_val = -1;
            if (chunksize > in->todo) {
                chunksize = in->todo;
                free_val = chunksize;
            }
            pack_fread(tempbuf, chunksize, in);

            _mp3_mutex.Lock();
            almp3_free_mp3stream_buffer(stream, free_val);
            _mp3_mutex.Unlock();
        }
    }

    _mp3_mutex.Lock();
    if (almp3_poll_mp3stream(stream) == ALMP3_POLL_PLAYJUSTFINISHED)
    {
        done = 1;
        if (psp_audio_multithreaded)
            internal_destroy();
    }
    _mp3_mutex.Unlock();

    _mutex.Unlock();

    return done;
}

void MYMP3::set_volume(int newvol)
{
    // boost MP3 volume
    newvol += 20;
    if (newvol > 255)
        newvol = 255;

    vol = newvol;
    newvol += volModifier + directionalVolModifier;
    if (newvol < 0) newvol = 0;

    _mp3_mutex.Lock();
    almp3_adjust_mp3stream(stream, newvol, panning, 1000);
    _mp3_mutex.Unlock();
}

void MYMP3::internal_destroy()
{
    _mp3_mutex.Lock();

    if (!done)
        almp3_stop_mp3stream(stream);

    almp3_destroy_mp3stream(stream);

    _mp3_mutex.Unlock();

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
    _mutex.Lock();

    if (psp_audio_multithreaded)
      _destroyThis = true;
    else
      internal_destroy();

    _mutex.Unlock();

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
    _mp3_mutex.Lock();
    int result = almp3_get_pos_msecs_mp3stream(stream);
    _mp3_mutex.Unlock();
    return result;
}

int MYMP3::get_length_ms()
{
    _mp3_mutex.Lock();
    int result = almp3_get_length_msecs_mp3stream(stream, filesize);
    _mp3_mutex.Unlock();
    return result;
}

void MYMP3::restart()
{
    if (stream != NULL) {
        // need to reset file pointer for this to work
        _mp3_mutex.Lock();
        almp3_play_mp3stream(stream, MP3CHUNKSIZE, vol, panning);
        _mp3_mutex.Unlock();
        done = 0;
        paused = 0;

        if (!psp_audio_multithreaded)
          poll();
    }
}

int MYMP3::get_voice()
{
    _mp3_mutex.Lock();
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3stream(stream);
    _mp3_mutex.Unlock();

    if (ast)
        return ast->voice;
    return -1;
}

int MYMP3::get_sound_type() {
    return MUS_MP3;
}

int MYMP3::play() {
    _mp3_mutex.Lock();
    almp3_play_mp3stream(stream, chunksize, (vol > 230) ? vol : vol + 20, panning);
    _mp3_mutex.Unlock();

    if (!psp_audio_multithreaded)
      poll();

    return 1;
}

MYMP3::MYMP3() : SOUNDCLIP() {
}

#endif // !NO_MP3_PLAYER
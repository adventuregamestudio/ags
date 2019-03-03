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
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    if (done || !stream || !buffer || !in)
    {
      done = 1;
      return done;
    }

    if (paused) {
        return 0;
    }

    if (!done) {
        // update the buffer
        char *tempbuf = (char *)almp3_get_mp3stream_buffer(stream);

        if (tempbuf != NULL) {
            int free_val = -1;
            if (chunksize >= in->todo) {
                chunksize = in->todo;
                free_val = chunksize;
            }
            pack_fread(tempbuf, chunksize, in);

            almp3_free_mp3stream_buffer(stream, free_val);
        }
    }

    int result = almp3_poll_mp3stream(stream);

    if (result == ALMP3_POLL_PLAYJUSTFINISHED)
    {
        done = 1;
    }

    return done;
}

void MYMP3::adjust_stream()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    almp3_adjust_mp3stream(stream, get_final_volume(), panning, speed);
}

void MYMP3::adjust_volume()
{
    adjust_stream();
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

void MYMP3::set_speed(int new_speed)
{
    speed = new_speed;
    adjust_stream();
}

void MYMP3::destroy()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    if (stream) {
        if (!done) {
		    almp3_stop_mp3stream(stream);
        }
	    almp3_destroy_mp3stream(stream);
    }
    stream = nullptr;

    if (buffer) {
        free(buffer);
    }
    buffer = nullptr;

    if (in) {
        pack_fclose(in);
    }
    in = nullptr;

    done = 1;
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
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!stream) { return 0; }
    return almp3_get_pos_msecs_mp3stream(stream);
}

int MYMP3::get_length_ms()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!stream) { return 0; }
    return almp3_get_length_msecs_mp3stream(stream, filesize);
}

int MYMP3::get_voice()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!stream) { return -1; }
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3stream(stream);
    return (ast != NULL ? ast->voice : -1);
}

int MYMP3::get_sound_type() {
    return MUS_MP3;
}

int MYMP3::play() {
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!stream) { return 0; }
    almp3_play_mp3stream(stream, chunksize, (vol > 230) ? vol : vol + 20, panning);

    return 1;
}

MYMP3::MYMP3() : SOUNDCLIP() {
    stream = NULL;
    in = NULL;
    filesize = 0;
    buffer = NULL;
    chunksize = 0;
}

#endif // !NO_MP3_PLAYER
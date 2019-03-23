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
#include "media/audio/clip_myogg.h"
#include "media/audio/audiointernaldefs.h"
#include "ac/common.h"               // quit()

#include "platform/base/agsplatformdriver.h"

extern "C" {
    extern int alogg_is_end_of_oggstream(ALOGG_OGGSTREAM *ogg);
    extern int alogg_is_end_of_ogg(ALOGG_OGG *ogg);
    extern int alogg_get_ogg_freq(ALOGG_OGG *ogg);
    extern int alogg_get_ogg_stereo(ALOGG_OGG *ogg);
}

int MYOGG::poll()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (done || !stream || !buffer || !in) {
        done = 1;
        return done;
    }

    if (paused) { return 0; }

    if ((!done) && (in->todo > 0))
    {
        // update the buffer
        auto tempbuf = alogg_get_oggstream_buffer(stream);
        if (tempbuf != NULL)
        {
            int free_val = -1;
            if (chunksize >= in->todo)
            {
                chunksize = in->todo;
                free_val = chunksize;
            }
            pack_fread(tempbuf, chunksize, in);
            alogg_free_oggstream_buffer(stream, free_val);
        }
    }

    int ret = alogg_poll_oggstream(stream);
    if (ret == ALOGG_OK || ret == ALOGG_POLL_BUFFERUNDERRUN)
        get_pos_ms();  // call this to keep the last_but_one stuff up to date
    else {
        // finished playing or error
        done = 1;
    }
    return done;
}

void MYOGG::adjust_stream()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!stream) { return; }
    alogg_adjust_oggstream(stream, get_final_volume(), panning, speed);
}

void MYOGG::adjust_volume()
{
    adjust_stream();
}

void MYOGG::set_volume(int newvol)
{
    // boost MP3 volume
    newvol += 20;
    if (newvol > 255)
        newvol = 255;
    vol = newvol;
    adjust_stream();
}

void MYOGG::set_speed(int new_speed)
{
    speed = new_speed;
    adjust_stream();
}

void MYOGG::destroy()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (stream) {
        if (!done) {
            alogg_stop_oggstream(stream);
        }
        alogg_destroy_oggstream(stream);
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

void MYOGG::seek(int pos)
{
    quit("Attempted to seek an oggstream; operation not permitted");
}

int MYOGG::get_pos()
{
    return 0;
}

int MYOGG::get_pos_ms()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (done || !stream || !buffer || !in) { return 0; }
    if (!alogg_is_playing_oggstream(stream)) { return 0; }

    // Unfortunately the alogg_get_pos_msecs_oggstream function
    // returns the ms offset that was last decoded, so it's always
    // ahead of the actual playback. Therefore we have this
    // hideous hack below to sort it out.

    AUDIOSTREAM *str = alogg_get_audiostream_oggstream(stream);
    long offs = (voice_get_position(str->voice) * 1000) / str->samp->freq;

    if (last_ms_offs != alogg_get_pos_msecs_oggstream(stream)) {
        last_but_one_but_one = last_but_one;
        last_but_one = last_ms_offs;
        last_ms_offs = alogg_get_pos_msecs_oggstream(stream);
    }

    // just about to switch buffers
    if (offs < 0)
        return last_but_one;

    int end_of_stream = alogg_is_end_of_oggstream(stream);

    if ((str->active == 1) && (last_but_one_but_one > 0) && (str->locked == NULL)) {
        switch (end_of_stream) {
case 0:
case 2:
    offs -= (last_but_one - last_but_one_but_one);
    break;
case 1:
    offs -= (last_but_one - last_but_one_but_one);
    break;
        }
    }

    if (end_of_stream == 1) {

        return offs + last_but_one;
    }

    return offs + last_but_one_but_one;
}

int MYOGG::get_length_ms()
{  // streamed OGG is variable bitrate so we don't know
    return 0;
}

int MYOGG::get_voice()
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!stream) { return -1; }
    AUDIOSTREAM *ast = alogg_get_audiostream_oggstream(stream);
    if (!ast) { return -1; }
    return ast->voice;
}

int MYOGG::get_sound_type() {
    return MUS_OGG;
}

int MYOGG::play() {
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    if (!stream) { return 0; }
    alogg_play_oggstream(stream, MP3CHUNKSIZE, (vol > 230) ? vol : vol + 20, panning);
    return 1;
}

MYOGG::MYOGG() : SOUNDCLIP() {
    stream = NULL;
    in = NULL;
    buffer = NULL;
    chunksize = 0;
    last_but_one_but_one = 0;
    last_but_one = 0;
    last_ms_offs = 0;
}

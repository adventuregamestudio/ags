
#include "ac/audiodefines.h"
#include "media/audio/clip_myogg.h"
#include "media/audio/audiointernaldefs.h"
#include "ac/common.h"               // quit()

extern "C" {
    extern int alogg_is_end_of_oggstream(ALOGG_OGGSTREAM *ogg);
    extern int alogg_is_end_of_ogg(ALOGG_OGG *ogg);
    extern int alogg_get_ogg_freq(ALOGG_OGG *ogg);
    extern int alogg_get_ogg_stereo(ALOGG_OGG *ogg);
}

int MYOGG::poll()
{
    lockMutex();

    if (done)
    {
        releaseMutex();
        return done;
    }
    if (paused)
    {
        releaseMutex();
        return 0;
    }

    if ((!done) && (in->todo > 0))
    {
        // update the buffer
        char *tempbuf = (char *)alogg_get_oggstream_buffer(stream);
        if (tempbuf != NULL)
        {
            int free_val = -1;
            if (chunksize > in->todo)
            {
                chunksize = in->todo;
                free_val = chunksize;
            }
            pack_fread(tempbuf, chunksize, in);
            alogg_free_oggstream_buffer(stream, free_val);
        }
    }
    if (alogg_poll_oggstream(stream) == ALOGG_POLL_PLAYJUSTFINISHED) {
        done = 1;
    }
    else get_pos_ms();  // call this to keep the last_but_one stuff up to date

    releaseMutex();

    return done;
}

void MYOGG::set_volume(int newvol)
{
    // boost MP3 volume
    newvol += 20;
    if (newvol > 255)
        newvol = 255;
    vol = newvol;
    newvol += volModifier + directionalVolModifier;
    if (newvol < 0) newvol = 0;
    alogg_adjust_oggstream(stream, newvol, panning, 1000);
}

void MYOGG::destroy()
{
    lockMutex();

    if (!done)
        alogg_stop_oggstream(stream);

    alogg_destroy_oggstream(stream);
    stream = NULL;
    if (buffer != NULL)
        free(buffer);
    buffer = NULL;
    pack_fclose(in);

    done = 1;

    releaseMutex();
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
    // Unfortunately the alogg_get_pos_msecs_oggstream function
    // returns the ms offset that was last decoded, so it's always
    // ahead of the actual playback. Therefore we have this
    // hideous hack below to sort it out.
    if ((done) || (!alogg_is_playing_oggstream(stream)))
        return 0;

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

    /*    char tbuffer[260];
    sprintf(tbuffer,"offs: %d  last_but_one_but_one: %d  last_but_one: %d  active:%d  locked: %p   EOS: %d",
    offs, last_but_one_but_one, last_but_one, str->active, str->locked, end_of_stream);
    write_log(tbuffer);*/

    if (end_of_stream == 1) {

        return offs + last_but_one;
    }

    return offs + last_but_one_but_one;
}

int MYOGG::get_length_ms()
{  // streamed OGG is variable bitrate so we don't know
    return 0;
}

void MYOGG::restart()
{
    if (stream != NULL) {
        // need to reset file pointer for this to work
        quit("Attempted to restart OGG not currently supported");
        alogg_play_oggstream(stream, MP3CHUNKSIZE, vol, panning);
        done = 0;
        paused = 0;
        poll();
    }
}

int MYOGG::get_voice()
{
    AUDIOSTREAM *ast = alogg_get_audiostream_oggstream(stream);
    if (ast)
        return ast->voice;
    return -1;
}

int MYOGG::get_sound_type() {
    return MUS_OGG;
}

int MYOGG::play() {
    alogg_play_oggstream(stream, MP3CHUNKSIZE, (vol > 230) ? vol : vol + 20, panning);

    poll();
    return 1;
}

MYOGG::MYOGG() : SOUNDCLIP() {
}

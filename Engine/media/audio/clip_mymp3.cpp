
#include "media/audio/audiodefines.h"

#ifndef NO_MP3_PLAYER

#include "media/audio/clip_mymp3.h"
#include "media/audio/audiointernaldefs.h"
#include "ac/common.h"               // quit()

int MYMP3::poll()
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

    if (!done) {
        // update the buffer
        char *tempbuf = (char *)almp3_get_mp3stream_buffer(stream);
        if (tempbuf != NULL) {
            int free_val = -1;
            if (chunksize > in->todo) {
                chunksize = in->todo;
                free_val = chunksize;
            }
            pack_fread(tempbuf, chunksize, in);
            almp3_free_mp3stream_buffer(stream, free_val);
        }
    }

    if (almp3_poll_mp3stream(stream) == ALMP3_POLL_PLAYJUSTFINISHED)
        done = 1;

    releaseMutex();

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
    almp3_adjust_mp3stream(stream, newvol, panning, 1000);
}

void MYMP3::destroy()
{
    lockMutex();

    if (!done)
        almp3_stop_mp3stream(stream);

    almp3_destroy_mp3stream(stream);
    stream = NULL;

    if (buffer != NULL)
        free(buffer);

    done = 1;
    buffer = NULL;
    pack_fclose(in);

    releaseMutex();
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
    return almp3_get_pos_msecs_mp3stream(stream);
}

int MYMP3::get_length_ms()
{
    return almp3_get_length_msecs_mp3stream(stream, filesize);
}

void MYMP3::restart()
{
    if (stream != NULL) {
        // need to reset file pointer for this to work
        almp3_play_mp3stream(stream, MP3CHUNKSIZE, vol, panning);
        done = 0;
        paused = 0;
        poll();
    }
}

int MYMP3::get_voice()
{
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3stream(stream);
    if (ast)
        return ast->voice;
    return -1;
}

int MYMP3::get_sound_type() {
    return MUS_MP3;
}

int MYMP3::play() {
    almp3_play_mp3stream(stream, chunksize, (vol > 230) ? vol : vol + 20, panning);
    poll();

    return 1;
}

MYMP3::MYMP3() : SOUNDCLIP() {
}

#endif // !NO_MP3_PLAYER
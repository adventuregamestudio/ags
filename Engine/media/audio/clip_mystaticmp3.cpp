
#include "media/audio/audiodefines.h"

#ifndef NO_MP3_PLAYER

#include "media/audio/clip_mystaticmp3.h"
#include "media/audio/audiointernaldefs.h"
#include "media/audio/soundcache.h"

extern int our_eip;

int MYSTATICMP3::poll()
{
    _mutex.Lock();

    int oldeip = our_eip;
    our_eip = 5997;

    if ((tune == NULL) || (!ready))
        ;
    else if (almp3_poll_mp3(tune) == ALMP3_POLL_PLAYJUSTFINISHED) {
        if (!repeat)
            done = 1;
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
        almp3_adjust_mp3(tune, newvol, panning, 1000, repeat);
    }

}

void MYSTATICMP3::destroy()
{
    _mutex.Lock();

    if (tune != NULL) {
        almp3_stop_mp3(tune);
        almp3_destroy_mp3(tune);
        tune = NULL;
    }
    if (mp3buffer != NULL) {
        sound_cache_free(mp3buffer, false);
    }

    _mutex.Unlock();
}

void MYSTATICMP3::seek(int pos)
{
    almp3_seek_abs_msecs_mp3(tune, pos);
}

int MYSTATICMP3::get_pos()
{
    return almp3_get_pos_msecs_mp3(tune);
}

int MYSTATICMP3::get_pos_ms()
{
    return get_pos();
}

int MYSTATICMP3::get_length_ms()
{
    return almp3_get_length_msecs_mp3(tune);
}

void MYSTATICMP3::restart()
{
    if (tune != NULL) {
        almp3_stop_mp3(tune);
        almp3_rewind_mp3(tune);
        almp3_play_mp3(tune, 16384, vol, panning);
        done = 0;
        poll();
    }
}

int MYSTATICMP3::get_voice()
{
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3(tune);
    if (ast)
        return ast->voice;
    return -1;
}

int MYSTATICMP3::get_sound_type() {
    return MUS_MP3;
}

int MYSTATICMP3::play() {
    if (almp3_play_ex_mp3(tune, 16384, vol, panning, 1000, repeat) != ALMP3_OK) {
        destroy();
        delete this;
        return 0;
    }

    poll();
    return 1;
}

MYSTATICMP3::MYSTATICMP3() : SOUNDCLIP() {
}

#endif // !NO_MP3_PLAYER
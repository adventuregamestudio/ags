
#include "ac/audiodefines.h"

#ifdef DUMB_MOD_PLAYER

#include "media/audio/clip_mydumbmod.h"
#include "media/audio/audiointernaldefs.h"

void al_duh_set_loop(AL_DUH_PLAYER *dp, int loop) {
    DUH_SIGRENDERER *sr = al_duh_get_sigrenderer(dp);
    DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
    if (itsr == NULL)
        return;

    if (loop)
        dumb_it_set_loop_callback(itsr, NULL, NULL);
    else
        dumb_it_set_loop_callback(itsr, dumb_it_callback_terminate, itsr);
}

int MYMOD::poll()
{
    if (done)
        return done;

    if (duhPlayer == NULL) {
        done = 1;
        return done;
    }

    if (al_poll_duh(duhPlayer))
        done = 1;

    return done;
}

void MYMOD::set_volume(int newvol)
{
    vol = newvol;

    if (duhPlayer)
    {
        newvol += volModifier + directionalVolModifier;
        if (newvol < 0) newvol = 0;
        al_duh_set_volume(duhPlayer, VOLUME_TO_DUMB_VOL(newvol));
    }
}

void MYMOD::destroy()
{
    if (duhPlayer) {
        al_stop_duh(duhPlayer);
        duhPlayer = NULL;
    }
    if (tune) {
        unload_duh(tune);
        tune = NULL;
    }
}

void MYMOD::seek(int patnum)
{
    if ((!done) && (duhPlayer)) {
        al_stop_duh(duhPlayer);
        done = 0;
        DUH_SIGRENDERER *sr = dumb_it_start_at_order(tune, 2, patnum);
        duhPlayer = al_duh_encapsulate_sigrenderer(sr, VOLUME_TO_DUMB_VOL(vol), 8192, 22050);
        if (!duhPlayer)
            duh_end_sigrenderer(sr);
        else
            al_duh_set_loop(duhPlayer, repeat);
    }
}

int MYMOD::get_pos()
{
    if ((duhPlayer == NULL) || (done))
        return -1;

    // determine the current track number (DUMB calls them 'orders')
    DUH_SIGRENDERER *sr = al_duh_get_sigrenderer(duhPlayer);
    DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
    if (itsr == NULL)
        return -1;

    return dumb_it_sr_get_current_order(itsr);
}

int MYMOD::get_pos_ms()
{
    return (get_pos() * 10) / 655;
}

int MYMOD::get_length_ms()
{
    if (tune == NULL)
        return 0;

    // duh_get_length represents time as 65536ths of a second
    return (duh_get_length(tune) * 10) / 655;
}

void MYMOD::restart()
{
    if (tune != NULL) {
        al_stop_duh(duhPlayer);
        done = 0;
        duhPlayer = al_start_duh(tune, 2, 0, 1.0, 8192, 22050);
    }
}

int MYMOD::get_voice()
{
    // MOD uses so many different voices it's not practical to keep track
    return -1;
}

void MYMOD::pause() {
    if (tune != NULL) {
        al_pause_duh(duhPlayer);
    }
}

void MYMOD::resume() {
    if (tune != NULL) {
        al_resume_duh(duhPlayer);
    }
}

int MYMOD::get_sound_type() {
    return MUS_MOD;
}

int MYMOD::play() {
    duhPlayer = al_start_duh(tune, 2, 0, 1.0, 8192, 22050);
    al_duh_set_loop(duhPlayer, repeat);
    set_volume(vol);

    return 1;
}  

MYMOD::MYMOD() : SOUNDCLIP() {
    duhPlayer = NULL;
}

#endif // DUMB_MOD_PLAYER
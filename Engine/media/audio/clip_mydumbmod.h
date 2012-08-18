#ifndef __AC_MYDUMBMOD_H
#define __AC_MYDUMBMOD_H

#include "aldumb.h"
#include "media/audio/soundclip.h"

#define VOLUME_TO_DUMB_VOL(vol) ((float)vol) / 256.0

void al_duh_set_loop(AL_DUH_PLAYER *dp, int loop);

// MOD/XM (DUMB)
struct MYMOD : public SOUNDCLIP
{
    DUH *tune;
    AL_DUH_PLAYER *duhPlayer;

    int poll();

    void set_volume(int newvol);

    void destroy();

    void seek(int patnum);

    int get_pos();

    int get_pos_ms();

    int get_length_ms();

    void restart();

    int get_voice();

    virtual void pause();

    virtual void resume();

    int get_sound_type();

    int play();

    MYMOD();
};

#endif // __AC_MYDUMBMOD_H
#ifndef __AC_MYJGMOD_H
#define __AC_MYJGMOD_H

#include "jgmod.h"
#include "media/audio/soundclip.h"

// MOD/XM (JGMOD)
struct MYMOD:public SOUNDCLIP
{
    JGMOD *tune;
    int repeat;

    int poll();

    void set_volume(int newvol);

    void destroy();

    void seek(int patnum);

    int get_pos();

    int get_pos_ms();

    int get_length_ms();

    void restart();

    int get_voice();

    int get_sound_type();

    int play();

    MYMOD();
};

#endif // __AC_MYJGMOD_H
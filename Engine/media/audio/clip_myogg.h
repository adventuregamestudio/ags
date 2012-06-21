#ifndef __AC_MYOGG_H
#define __AC_MYOGG_H

#include "alogg.h"
#include "media/audio/soundclip.h"

struct MYOGG:public SOUNDCLIP
{
    ALOGG_OGGSTREAM *stream;
    PACKFILE *in;
    char *buffer;
    int chunksize;

    int last_but_one_but_one;
    int last_but_one;
    int last_ms_offs;

    int poll();

    void set_volume(int newvol);

    void destroy();

    void seek(int pos);

    int get_pos();    

    int get_pos_ms();

    int get_length_ms();

    void restart();

    int get_voice();

    int get_sound_type();

    int play();

    MYOGG();
};

#endif // __AC_MYOGG_H
#ifndef __AC_MYSTATICOGG_H
#define __AC_MYSTATICOGG_H

#include "alogg.h"
#include "acaudio/ac_soundclip.h"

// pre-loaded (non-streaming) OGG file
struct MYSTATICOGG:public SOUNDCLIP
{
    ALOGG_OGG *tune;
    char *mp3buffer;
    int mp3buffersize;
    int extraOffset;

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

    virtual int play_from(int position);

    int play();

    MYSTATICOGG();
};

#endif // __AC_MYSTATICOGG_H

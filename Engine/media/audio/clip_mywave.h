#ifndef __AC_MYWAVE_H
#define __AC_MYWAVE_H

#include "media/audio/soundclip.h"

// My new MP3STREAM wrapper
struct MYWAVE:public SOUNDCLIP
{
    SAMPLE *wave;
    int voice;
    int firstTime;
    int repeat;

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

    MYWAVE();
};

#endif // __AC_MYWAVE_H
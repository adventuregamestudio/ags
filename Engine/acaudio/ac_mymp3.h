#ifndef __AC_MYMP3_H
#define __AC_MYMP3_H

#include "almp3.h"
#include "acaudio/ac_soundclip.h"

struct MYMP3:public SOUNDCLIP
{
    ALMP3_MP3STREAM *stream;
    PACKFILE *in;
    long  filesize;
    char *buffer;
    int chunksize;

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
    MYMP3();
};

#endif // __AC_MYMP3_H
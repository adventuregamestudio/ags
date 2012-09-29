#ifndef __AC_MYSTATICMP3_H
#define __AC_MYSTATICMP3_H

#include "almp3.h"
#include "media/audio/soundclip.h"

extern AGS::Engine::Mutex _mp3_mutex;

// pre-loaded (non-streaming) MP3 file
struct MYSTATICMP3:public SOUNDCLIP
{
    ALMP3_MP3 *tune;
    char *mp3buffer;

    int poll();

    void set_volume(int newvol);

    void internal_destroy();

    void destroy();

    void seek(int pos);

    int get_pos();

    int get_pos_ms();

    int get_length_ms();
    void restart();

    int get_voice();

    int get_sound_type();

    int play();

    MYSTATICMP3();
};

#endif // __AC_MYSTATICMP3_H

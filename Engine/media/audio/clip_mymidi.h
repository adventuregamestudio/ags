#ifndef __AC_MYMIDI_H
#define __AC_MYMIDI_H

#include "media/audio/soundclip.h"

// MIDI
struct MYMIDI:public SOUNDCLIP
{
    MIDI *tune;
    int lengthInSeconds;

    // The PSP takes a while to play a midi, therefore poll() can be called
    // before the music is ready and immediately returns done.
    volatile bool initializing;

    int poll();

    void set_volume(int newvol);

    void destroy();

    void seek(int pos);

    int get_pos();

    int get_pos_ms();

    int get_length_ms();

    void restart();

    int get_voice();

    virtual void pause();

    virtual void resume();

    int get_sound_type();

    int play();

    MYMIDI();
};

#endif // __AC_MYMIDI_H
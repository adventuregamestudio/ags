/*
** ACSOUND - AGS sound system wrapper
** Copyright (C) 2002-2003, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __AC_SOUNDCLIP_H
#define __AC_SOUNDCLIP_H

#undef BITMAP
#include "util/mutex.h"

struct SOUNDCLIP
{
    int done;
    int priority;
    int soundType;
    int vol;
    int volAsPercentage;
    int originalVolAsPercentage;
    int volModifier;
    int paused;
    int panning;
    int panningAsPercentage;
    int xSource, ySource;
    int maximumPossibleDistanceAway;
    int directionalVolModifier;
    bool repeat;
    void *sourceClip;
    bool ready;
    AGS::Engine::Mutex _mutex;

    virtual int poll() = 0;
    virtual void destroy() = 0;
    virtual void set_volume(int) = 0;
    virtual void restart() = 0;
    virtual void seek(int) = 0;
    virtual int get_pos() = 0;    // return 0 to indicate seek not supported
    virtual int get_pos_ms() = 0; // this must always return valid value if poss
    virtual int get_length_ms() = 0; // return total track length in ms (or 0)
    virtual int get_voice() = 0;  // return the allegro voice number (or -1 if none)
    virtual int get_sound_type() = 0;
    virtual int play() = 0;

    virtual int play_from(int position);

    virtual void set_panning(int newPanning);

    virtual void pause();
    virtual void resume();

    SOUNDCLIP();
    ~SOUNDCLIP();
};


#endif // __AC_SOUNDCLIP_H
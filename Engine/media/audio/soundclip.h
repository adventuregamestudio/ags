//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// ACSOUND - AGS sound system wrapper
//
//=============================================================================

#ifndef __AC_SOUNDCLIP_H
#define __AC_SOUNDCLIP_H

#undef BITMAP
#include "util/mutex.h"

// JJS: This is needed for the derieved classes
extern volatile int psp_audio_multithreaded;

struct SOUNDCLIP
{
    bool _destroyThis;
    bool _playing;

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

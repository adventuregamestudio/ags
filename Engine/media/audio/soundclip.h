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
extern volatile bool _audio_doing_crossfade;

struct SOUNDCLIP
{
    bool _destroyThis;
    bool _playing;

    int done;
    int priority;
    int soundType;
    // absolute volume, set by implementations only!
    int vol;
    // current relative volume, in percents
    int volAsPercentage;
    // originalVolAsPercentage is used when there's a need to temporarily
    // change and then restore the clip's relative volume (volAsPercentage)
    int originalVolAsPercentage;
    // volModifier is used when there's a need to temporarily change and
    // the restore the clip's absolute volume (vol)
    int volModifier;
    int paused;
    int panning;
    int panningAsPercentage;
    int speed; // speed of playback, in clip ms per real second
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
    virtual void set_speed(int) { /* not supported by default */ }

    virtual void pause();
    virtual void resume();

    inline int get_speed() const
    {
        return speed;
    }

    inline int get_volume() const
    {
        return originalVolAsPercentage;
    }

    inline void set_volume_origin(int volume)
    {
        volAsPercentage = volume;
        originalVolAsPercentage = volume;
        set_volume((volume * 255) / 100);
    }

    inline void set_volume_alternate(int vol_percent, int vol_absolute)
    {
        volAsPercentage = vol_percent;
        originalVolAsPercentage = vol_percent;
        set_volume(vol_absolute);
    }

    inline void set_volume_override(int volume)
    {
        volAsPercentage = volume;
        set_volume((volume * 255) / 100);
    }

    inline void reset_volume_to_origin()
    {
        set_volume_origin(originalVolAsPercentage);
    }

    inline void apply_volume_modifier(int mod)
    {
        volModifier = mod;
        // this forces implementation to recalculate absolute volume using new modifier
        set_volume(vol);
    }

    SOUNDCLIP();
    ~SOUNDCLIP();
};


#endif // __AC_SOUNDCLIP_H

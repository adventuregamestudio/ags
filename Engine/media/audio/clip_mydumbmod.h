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

#ifndef __AC_MYDUMBMOD_H
#define __AC_MYDUMBMOD_H

#include "aldumb.h"
#include "media/audio/soundclip.h"

#define VOLUME_TO_DUMB_VOL(vol) ((float)vol) / 256.0

void al_duh_set_loop(AL_DUH_PLAYER *dp, int loop);

// MOD/XM (DUMB)
struct MYMOD : public SOUNDCLIP
{
    DUH *tune;
    AL_DUH_PLAYER *duhPlayer;

    int poll();

    void set_volume(int newvol);

    void destroy();

    void seek(int patnum);

    // NOTE: this implementation of the virtual function returns a MOD/XM
    // "order" index, not actual playing position;
    // this does not make much sense in the context of the interface itself,
    // and, as it seems, was implemented so solely for the purpose of emulating
    // deprecated "GetMODPattern" script function.
    // (see Game_GetMODPattern(), and documentation for AudioChannel.Position property)
    // TODO: find a way to redesign this behavior
    int get_pos();

    // Returns real MOD/XM playing position
    int get_real_mod_pos();

    int get_pos_ms();

    int get_length_ms();

    void restart();

    int get_voice();

    virtual void pause();

    virtual void resume();

    int get_sound_type();

    int play();

    MYMOD();

protected:
    virtual void adjust_volume();
};

#endif // __AC_MYDUMBMOD_H
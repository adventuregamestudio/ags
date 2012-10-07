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

    MYWAVE();
};

#endif // __AC_MYWAVE_H
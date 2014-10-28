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

#ifndef __AC_MYOGG_H
#define __AC_MYOGG_H

#include "alogg.h"
#include "media/audio/soundclip.h"

struct MYOGG:public SOUNDCLIP
{
    ALOGG_OGGSTREAM *stream;
    PACKFILE *in;
    char *buffer;
    int chunksize;

    int last_but_one_but_one;
    int last_but_one;
    int last_ms_offs;

    int poll();

    void set_volume(int newvol);
    void set_speed(int new_speed);

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

    MYOGG();

protected:
    virtual void adjust_volume();
private:
    void adjust_stream();
};

#endif // __AC_MYOGG_H
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

#ifndef __AC_MYMP3_H
#define __AC_MYMP3_H

#include "almp3.h"
#include "media/audio/soundclip.h"

extern AGS::Engine::Mutex _mp3_mutex;

struct MYMP3:public SOUNDCLIP
{
    ALMP3_MP3STREAM *stream;
    PACKFILE *in;
    long  filesize;
    char *buffer;
    int chunksize;

    void poll();
    void set_volume(int newvol);
    void set_speed(int new_speed);
    void destroy();
    void seek(int pos);
    int get_pos();
    int get_pos_ms();
    int get_length_ms();
    int get_sound_type();
    int play();
    MYMP3();

protected:
    int get_voice();
    virtual void adjust_volume();
private:
    void adjust_stream();
};

#endif // __AC_MYMP3_H
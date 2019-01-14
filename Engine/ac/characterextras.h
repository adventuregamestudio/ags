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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__CHARACTEREXTRAS_H
#define __AGS_EE_AC__CHARACTEREXTRAS_H

#include "ac/runtime_defines.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// TODO: now safe to merge with CharacterInfo into one class
struct CharacterExtras {
    short invorder[MAX_INVORDER];
    short invorder_count;
    short width;
    short height;
    short zoom;
    short xwas;
    short ywas;
    short tint_r;
    short tint_g;
    short tint_b;
    short tint_level;
    short tint_light;
    char  process_idle_this_time;
    char  slow_move_counter;
    short animwait;

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
};

#endif // __AGS_EE_AC__CHARACTEREXTRAS_H

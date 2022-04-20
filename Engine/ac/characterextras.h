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

// The CharacterInfo struct size is fixed because it's exposed to script
// and plugin API, therefore new stuff has to go here
struct CharacterExtras {
    short invorder[MAX_INVORDER]{};
    short invorder_count = 0;
    // TODO: implement full AABB and keep updated, so that engine could rely on these cached values all time;
    // TODO: consider having both fixed AABB and volatile one that changes with animation frame (unless you change how anims work)
    short width = 0;
    short height = 0;
    short zoom = 0;
    short xwas = 0;
    short ywas = 0;
    short tint_r = 0;
    short tint_g = 0;
    short tint_b = 0;
    short tint_level = 0;
    short tint_light = 0;
    char  process_idle_this_time = 0;
    char  slow_move_counter = 0;
    short animwait = 0;
    int   cur_anim_volume = -1; // current animation sound volume (-1 = default)

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
};

#endif // __AGS_EE_AC__CHARACTEREXTRAS_H

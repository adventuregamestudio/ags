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

#include "core/types.h"
#include "ac/runtime_defines.h"
#include "gfx/gfx_def.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// TODO: now safe to merge with CharacterInfo into one class
struct CharacterExtras {
    short invorder[MAX_INVORDER];
    short invorder_count;
    // TODO: implement full AABB and keep updated, so that engine could rely on these cached values all time;
    // TODO: consider having both fixed AABB and volatile one that changes with animation frame (unless you change how anims work)
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
    Common::BlendMode blend_mode;
    float rotation;

    void ReadFromSavegame(Common::Stream *in, int32_t cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AGS_EE_AC__CHARACTEREXTRAS_H

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Runtime room object definition.
//
//=============================================================================
#ifndef __AGS_EE_AC__ROOMOBJECT_H
#define __AGS_EE_AC__ROOMOBJECT_H

#include "core/types.h"
#include "ac/common_defines.h"
#include "util/string.h"

namespace AGS { namespace Common { class Stream; }}
using namespace AGS; // FIXME later

// RoomObject's internal values, packed in RoomObject::cycling
// Animates once and stops at the *last* frame
#define OBJANIM_ONCE      (ANIM_ONCE + 1)
// Animates infinitely until stopped by command
#define OBJANIM_REPEAT    (ANIM_REPEAT + 1)
// Animates once and stops, resetting to the very first frame
#define OBJANIM_ONCERESET (ANIM_ONCERESET + 1)
// Animates backwards, as opposed to forwards
#define OBJANIM_BACKWARDS 10

// IMPORTANT: exposed to plugin API as AGSObject!
// keep that in mind if extending this struct, and dont change existing fields
// unless you plan on adjusting plugin API as well.
struct RoomObject {
    static const uint16_t NoView = UINT16_MAX;

    int   x,y;
    int   transparent;    // current transparency setting
    int16_t tint_r, tint_g;   // specific object tint
    int16_t tint_b, tint_level;
    int16_t tint_light;
    int16_t zoom;           // zoom level, either manual or from the current area
    int16_t last_width, last_height;   // width/height based on a scaled sprite
    uint16_t num;            // sprite slot number
    int16_t baseline;       // <=0 to use Y co-ordinate; >0 for specific baseline
    uint16_t view,loop,frame; // only used to track animation - 'num' holds the current sprite
    int16_t wait,moving;
    int8_t  cycling;        // stores OBJANIM_* flags and values
    int8_t  overall_speed;  // animation delay
    int8_t  on;
    int8_t  flags;
    // -- up from here is a part of the plugin API
    int16_t blocking_width = 0, blocking_height = 0, blocking_x = 0, blocking_y = 0;
    int   anim_volume = 100; // default animation volume (relative factor)
    int   cur_anim_volume = 100; // current animation sound volume (relative factor)
    Common::String name;

    RoomObject();

    int get_width() const;
    int get_height() const;
    int get_baseline() const;

    inline bool has_explicit_light() const { return (flags & OBJF_HASLIGHT) != 0; }
    inline bool has_explicit_tint()  const { return (flags & OBJF_HASTINT) != 0; }
    inline bool is_animating()       const { return (cycling > 0); }
    // repeat may be ANIM_ONCE, ANIM_REPEAT, ANIM_ONCERESET;
    // get_anim_repeat() converts from OBJANIM_* to ANIM_* values
    inline int  get_anim_repeat()    const { return (cycling % OBJANIM_BACKWARDS) - 1; }
    inline bool get_anim_forwards()  const { return (cycling < OBJANIM_BACKWARDS); }
    inline int  get_anim_delay()     const { return overall_speed; }
    // repeat may be ANIM_ONCE, ANIM_REPEAT, ANIM_ONCERESET 
    inline void set_animating(int repeat, bool forwards, int delay)
    {
        // convert "repeat" to 1-based OBJANIM_* flag
        cycling = (repeat + 1) + (!forwards * OBJANIM_BACKWARDS);
        overall_speed = delay;
    }

	void UpdateCyclingView(int ref_id);
    // Calculate wanted frame sound volume based on multiple factors
    int  GetFrameSoundVolume() const;
    // Process the current animation frame for the room object:
    // play linked sounds, and so forth.
    void CheckViewFrame();

    void ReadFromSavegame(Common::Stream *in, int save_ver);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AGS_EE_AC__ROOMOBJECT_H

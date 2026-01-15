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
#include "ac/runtime_defines.h"
#include "gfx/gfx_def.h"
#include "util/string.h"

namespace AGS { namespace Common { class Stream; }}
using namespace AGS; // FIXME later

// RoomObject's legacy animation constants
#define LEGACY_OBJANIM_ONCE      (LEGACY_ANIM_ONCE + 1)
#define LEGACY_OBJANIM_REPEAT    (LEGACY_ANIM_REPEAT + 1)
#define LEGACY_OBJANIM_ONCERESET (LEGACY_ANIM_ONCERESET + 1)
#define LEGACY_OBJANIM_BACKWARDS 10


class RoomObject
{
public:
    static const uint16_t NoView = UINT16_MAX;

    int   id = 0;
    int   x,y;
    int   transparent;    // current transparency setting
    int16_t tint_r, tint_g;   // specific object tint
    int16_t tint_b, tint_level;
    int16_t tint_light;
    int16_t zoom;           // zoom level, either manual or from the current area
    int   spr_width, spr_height; // last used sprite's size
    int   spr_xoff, spr_yoff; // sprite offsets (when using a view)
    int16_t width, height;  // width/height based on a scaled sprite
    uint16_t num;            // sprite slot number
    int16_t baseline;       // <=0 to use Y co-ordinate; >0 for specific baseline
    uint16_t view,loop,frame; // only used to track animation - 'num' holds the current sprite
    int16_t wait,moving;
    ViewAnimateParams anim;
    int   flags; // OBJF_* flags
    // -- up from here is a part of the plugin API
    int16_t blocking_width = 0, blocking_height = 0, blocking_x = 0, blocking_y = 0;
    int   anim_volume = 100; // default animation volume (relative factor)
    Common::String name;
    Common::BlendMode blend_mode;
    int   shader_id = 0;
    int   shader_handle = 0; // script shader handle
    float rotation;
    int   movelist_handle = 0; // handle to the script movelist

    RoomObject();

    int get_width() const;
    int get_height() const;
    int get_baseline() const;

    // Tells if the "enabled" flag is set
    inline bool is_enabled() const { return (flags & OBJF_ENABLED) != 0; }
    // Tells if the "visible" flag is set
    inline bool is_visible() const { return (flags & OBJF_VISIBLE) != 0; }
    // Tells if the object is actually meant to be displayed on screen;
    // this combines both "enabled" and "visible" factors.
    inline bool is_displayed() const { return is_enabled() && is_visible(); }
    inline bool has_explicit_light() const { return (flags & OBJF_HASLIGHT) != 0; }
    inline bool has_explicit_tint()  const { return (flags & OBJF_HASTINT) != 0; }
    inline bool is_moving()          const { return moving > 0; }
    inline bool is_animating()       const { return anim.IsValid(); }
    inline int  get_anim_repeat()    const { return anim.IsRepeating(); }
    inline bool get_anim_forwards()  const { return anim.IsForward(); }
    inline int  get_anim_delay()     const { return anim.Delay; }
    inline void set_enabled(bool on) { flags = (flags & ~OBJF_ENABLED) | (OBJF_ENABLED * on); }
    inline void set_visible(bool on) { flags = (flags & ~OBJF_VISIBLE) | (OBJF_VISIBLE * on); }
    inline void set_animating(AnimFlowStyle repeat, AnimFlowDirection dir, int delay, int anim_volume = 100)
    {
        anim = ViewAnimateParams(repeat, dir, delay, anim_volume);
    }

    inline Pointf GetOrigin() const { return Pointf(0.f, 1.f); /* left-bottom */ }
    inline const Common::GraphicSpace &GetGraphicSpace() const { return _gs; }
    void UpdateGraphicSpace();

    void UpdateCyclingView(int ref_id);
    void OnStopMoving();
    // Calculate wanted frame sound volume based on multiple factors
    int  GetFrameSoundVolume() const;
    // Process the current animation frame for the room object:
    // play linked sounds, and so forth.
    void CheckViewFrame();
    // Run the arbitrary frame's event if one is associated and character has a handler
    bool RunFrameEvent(int view, int loop, int frame);

    void ReadFromSavegame(Common::Stream *in, int cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    Common::GraphicSpace _gs;
};

#endif // __AGS_EE_AC__ROOMOBJECT_H

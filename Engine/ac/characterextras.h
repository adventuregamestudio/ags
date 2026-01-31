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
// CharacterExtras is a separate runtime character data. Historically it was
// separated from the design-time CharacterInfo, because the latter is exposed
// to script API and plugin API in such way that its memory layout could not
// be changed at all. Although, today this is less of an issue (see comment
// to CharacterInfo).
//
// TODO: in the long run it will be beneficial to remake this into a more
// explicit runtime Character class, while perhaps keeping CharacterInfo only
// to load design-time data.
//
//=============================================================================
#ifndef __AGS_EE_AC__CHARACTEREXTRAS_H
#define __AGS_EE_AC__CHARACTEREXTRAS_H

#include "core/types.h"
#include "ac/characterinfo.h"
#include "ac/runtime_defines.h"
#include "gfx/gfx_def.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct CharacterInfo;

enum CharacterRuntimeFlags
{
    kCharf_Turning      = 0x00000001, // is turning
    kCharf_TurningCCW   = 0x00000002, // turning counter-clockwise
    kCharf_TurningMask  = (kCharf_Turning | kCharf_TurningCCW),
};

// The CharacterInfo struct size is fixed because it's exposed to script
// and plugin API, therefore new stuff has to go here
// TODO: now safe to merge with CharacterInfo into one class;
// but it's better to leave a separate "character data" struct, loaded from the game file,
// and a Character class, that does all things runtime.
class CharacterExtras
{
public:
    // FIXME: hide the fields under public interface;
    //        use standard int* types
    short invorder[MAX_INVORDER]{};
    short invorder_count = 0;
    Point spr_offset;    // fixed sprite offset (translation)
    Pointf spr_anchor    // graphic anchor (relative alignment)
        = Pointf(0.5f, 1.f); // default: middle-bottom
    Point eff_offset;    // effective offset (depends on view state)
    Pointf eff_anchor;   // effective anchor (depends on view state)
    int   spr_width = 0; // current sprite size
    int   spr_height = 0;
    int   frame_xoff = 0; // current frame offset
    int   frame_yoff = 0;
    short width = 0; // width/height (includes character scaling!)
    short height = 0;
    short zoom = 100;
    short xwas = 0; // TODO: figure out how these xwas,ywas are being used and comment them
    short ywas = 0;
    short tint_r = 0;
    short tint_g = 0;
    short tint_b = 0;
    short tint_level = 0;
    short tint_light = 0;
    char  process_idle_this_time = 0;
    char  slow_move_counter = 0;
    ViewAnimateParams anim;
    short animwait = 0; // current ticks counter, before advancing a animation
    int   anim_volume = 100; // default animation volume (relative factor)
    int   following = -1; // whom do we follow (character id)
    int   follow_dist = 0; // follow distance, in pixels
    int   follow_eagerness = 0; // follow reaction
    Common::BlendMode blend_mode = Common::kBlend_Normal;
    int   shader_id = 0;
    int   shader_handle = 0; // script shader handle
    float rotation = 0.f;
    // Optional character face direction ratio, 0 = ignore
    float face_dir_ratio = 0.f;
    int   movelist_handle = 0; // handle to the script movelist
    int   flags = 0; // runtime character flags (CharacterRuntimeFlags)
    int   turns = 0; // turning counter

    // Following fields are deriatives of the above (calculated from these
    // and other factors), and hence are not serialized.
    //
    // zoom factor of sprite offsets, fixed at 100 in backwards compatible mode
    int   zoom_offs = 100;

    bool IsTurning() const { return (flags & kCharf_Turning) != 0; }
    bool IsTurningClockwise() const { return IsTurning() && (flags & kCharf_TurningCCW) == 0; }
    bool IsTurningCounterClockwise() const { return IsTurning() && (flags & kCharf_TurningCCW) != 0; }
    void SetTurning(bool on, bool ccw, int turn_steps);
    void DecrementTurning();

    bool IsAnimating() const { return anim.IsValid(); }
    bool IsAnimatingRepeatedly() const { return anim.IsRepeating(); }
    int  GetAnimDelay() const { return anim.Delay; }
    // NOTE: has to return non-const for CycleViewAnim :/
    // this may be solved by having a base class that handles animation, e.g. ViewBasedObject
    ViewAnimateParams &GetAnimParams() { return anim; }
    void SetAnimating(AnimFlowStyle flow, AnimFlowDirection dir, int delay, int anim_volume = 100);
    void ResetAnimating();

    // Get current effective graphic anchor, which may be either a freely assigned anchor
    // or a offset from the temporarily locked view
    Pointf GetEffectiveGraphicAnchor() const { return eff_anchor; }
    // Get current effective graphic offset, which may be either a freely assigned offset
    // or a offset from the temporarily locked view
    Point GetEffectiveGraphicOffset() const { return eff_offset; }
    // Get visual Y position, which is calculated as Y - Z (scaled)
    int GetEffectiveY(const CharacterInfo *chi) const;
    // Calculate wanted frame sound volume based on multiple factors
    int GetFrameSoundVolume(const CharacterInfo *chi) const;
    // Set locked view with anchor and offset
    void SetLockedView(CharacterInfo *chi, int view, int loop, int frame,
                       const Pointf &anchor = Pointf(), const Point &off = Point());
    // Set unlocked view, revert to common anchor and offset
    void SetUnlockedView(CharacterInfo *chi);
    // Process the current animation frame for the character:
    // play linked sounds, and so forth.
    void CheckViewFrame(CharacterInfo *chi);
    // Run the arbitrary frame's event if one is associated and character has a handler
    bool RunFrameEvent(CharacterInfo *chi, int view, int loop, int frame);
    // Setups following another character (follow_who)
    void SetFollowing(CharacterInfo *chi, int follow_who, int distance = 0, int eagerness = 0, bool sort_behind = false);

    inline const Common::GraphicSpace &GetGraphicSpace() const { return _gs; }

    void UpdateEffectiveValues(const CharacterInfo *chin);
    void UpdateGraphicSpace(const CharacterInfo *chin);

    // Read character extra data from saves.
    void ReadFromSavegame(CharacterInfo *chin, Common::Stream *in, CharacterSvgVersion save_ver);
    void WriteToSavegame(const CharacterInfo *chin, Common::Stream *out) const;

private:
    Common::GraphicSpace _gs;
};

#endif // __AGS_EE_AC__CHARACTEREXTRAS_H

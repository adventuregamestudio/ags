//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

// The CharacterInfo struct size is fixed because it's exposed to script
// and plugin API, therefore new stuff has to go here
// TODO: now safe to merge with CharacterInfo into one class
struct CharacterExtras
{
    short invorder[MAX_INVORDER]{};
    short invorder_count = 0;
    int spr_width = 0; // last used sprite's size
    int spr_height = 0;
    short width = 0; // width/height last time drawn (includes scaling)
    short height = 0;
    short zoom = 100;
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
    int   anim_volume = 100; // default animation volume (relative factor)
    int   cur_anim_volume = 100; // current animation sound volume (relative factor)
    Common::BlendMode blend_mode = Common::kBlend_Normal;
    float rotation = 0.f;

    // Following fields are deriatives of the above (calculated from these
    // and other factors), and hence are not serialized.
    //
    // zoom factor of sprite offsets, fixed at 100 in backwards compatible mode
    int   zoom_offs = 100;

    int GetEffectiveY(CharacterInfo *chi) const; // return Y - Z

    // Calculate wanted frame sound volume based on multiple factors
    int GetFrameSoundVolume(CharacterInfo *chi) const;
    // Process the current animation frame for the character:
    // play linked sounds, and so forth.
    void CheckViewFrame(CharacterInfo *chi);

    inline Pointf GetOrigin() const { return Pointf(0.5f, 1.f); /* middle-bottom */ }
    inline const Common::GraphicSpace &GetGraphicSpace() const { return _gs; }

    void UpdateGraphicSpace(const CharacterInfo *chin);

    // Read character extra data from saves.
    void ReadFromSavegame(Common::Stream *in, CharacterSvgVersion save_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    Common::GraphicSpace _gs;
};

#endif // __AGS_EE_AC__CHARACTEREXTRAS_H

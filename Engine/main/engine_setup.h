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
#ifndef __AGS_EE_MAIN__ENGINESETUP_H
#define __AGS_EE_MAIN__ENGINESETUP_H

#include "util/geometry.h"
#include "gfx/gfxdefines.h"

// Setup engine after the graphics mode has changed
void engine_post_gfxmode_setup(const Size &init_desktop, const DisplayMode &old_dm);
// Prepare engine for graphics mode release; could be called before switching display mode too
void engine_pre_gfxmode_release();
// Prepare engine to the graphics mode shutdown and gfx driver destruction
void engine_pre_gfxsystem_shutdown();
// Applies necessary changes after screen<->virtual coordinate transformation has changed
void on_coordinates_scaling_changed();
// prepares game screen for rotation setting
void engine_adjust_for_rotation_settings();

#endif // __AGS_EE_MAIN__ENGINESETUP_H

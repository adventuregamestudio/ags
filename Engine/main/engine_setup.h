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
#ifndef __AGS_EE_MAIN__ENGINESETUP_H
#define __AGS_EE_MAIN__ENGINESETUP_H

#include "util/geometry.h"

struct ColorDepthOption;

// Get color depth settings depending on game settings and user config
void engine_get_color_depths(ColorDepthOption &color_depths);
// Sets up game viewport and object scaling parameters depending on game and
// user config; fills in color depth options.
void engine_init_resolution_settings(const Size game_size);
// Setup engine after the graphics mode has changed
void engine_post_gfxmode_setup(const Size &init_desktop);
// Prepare engine for graphics mode release; could be called before switching display mode too
void engine_pre_gfxmode_release();
// Prepare engine to the graphics mode shutdown and gfx driver destruction
void engine_pre_gfxsystem_shutdown();

#endif // __AGS_EE_MAIN__ENGINESETUP_H

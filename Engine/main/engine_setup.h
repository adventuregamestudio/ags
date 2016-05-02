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

// Sets up game viewport and object scaling parameters depending on game and
// user config; fills in color depth options.
void engine_init_resolution_settings(const Size game_size, ColorDepthOption &color_depths);
// Setup rendering callbacks and color conversions depending on initialized gfx mode
void engine_post_gfxmode_setup(const Size &init_desktop);

#endif // __AGS_EE_MAIN__ENGINESETUP_H

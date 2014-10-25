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
#ifndef __AGS_EE_MAIN__GRAPHICSMODE_H
#define __AGS_EE_MAIN__GRAPHICSMODE_H

#include "gfx/gfxdefines.h"
#include "util/geometry.h"
#include "util/scaling.h"
#include "util/string.h"

bool graphics_mode_init();
void graphics_mode_shutdown();
bool get_desktop_size_for_mode(Size &size, const bool windowed);
AGS::Common::String make_scaling_factor_string(uint32_t scaling);

namespace AGS { namespace Engine { class IGfxModeList; }}
bool find_nearest_supported_mode(const AGS::Engine::IGfxModeList &modes, Size &wanted_size, int *mode_index,
                                 const int color_depth, const Size *ratio_reference = NULL);

// The actual game screen resolution
extern AGS::Engine::GraphicResolution ScreenResolution;
// The game-to-screen transformation
extern AGS::Engine::PlaneScaling GameScaling;

namespace AGS { namespace Engine { class IGfxFilter; } }
extern AGS::Engine::IGfxFilter *filter;

#endif // __AGS_EE_MAIN__GRAPHICSMODE_H

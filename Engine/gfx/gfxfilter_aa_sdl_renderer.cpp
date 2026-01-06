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
#include "gfx/gfxfilter_aa_sdl_renderer.h"
#include <SDL.h>

namespace AGS
{
namespace Engine
{
namespace ALSW
{

const GfxFilterInfo SDLRendererAAGfxFilter::FilterInfo = GfxFilterInfo("Linear", "Linear interpolation");

const GfxFilterInfo &SDLRendererAAGfxFilter::GetInfo() const
{
    return FilterInfo;
}

SDL_ScaleMode SDLRendererAAGfxFilter::GetScaleMode() const
{
    return SDL_ScaleModeLinear;
}

void SDLRendererAAGfxFilter::InitSDLHint() const
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
}

} // namespace ALSW
} // namespace Engine
} // namespace AGS

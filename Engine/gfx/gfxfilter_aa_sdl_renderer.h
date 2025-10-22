//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// SDL software renderer Linear filter. Technically a non-op, as SDL_Renderer
// does the job.
//
//=============================================================================
#ifndef __AGS_EE_GFX__SDLRENDERERAAFILTER_H
#define __AGS_EE_GFX__SDLRENDERERAAFILTER_H

#include "gfx/gfxfilter_sdl_renderer.h"

namespace AGS
{
namespace Engine
{
namespace ALSW
{

class SDLRendererAAGfxFilter : public SDLRendererGfxFilter
{
public:
    const GfxFilterInfo &GetInfo() const override;

    SDL_ScaleMode GetScaleMode() const override;
    void InitSDLHint() const override;

    static const GfxFilterInfo FilterInfo;
};

} // namespace ALSW
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__SDLRENDERERAAFILTER_H

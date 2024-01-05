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
// SDL software renderer filter. Technically a non-op, as SDL_Renderer
// does the job.
//
//=============================================================================
#ifndef __AGS_EE_GFX__SDLRENDERERFILTER_H
#define __AGS_EE_GFX__SDLRENDERERFILTER_H
#include "gfx/gfxfilter_scaling.h"

namespace AGS
{
namespace Engine
{
namespace ALSW
{

class SDLRendererGfxFilter : public ScalingGfxFilter
{
public:
    const GfxFilterInfo &GetInfo() const override;

    static const GfxFilterInfo FilterInfo;
};

} // namespace ALSW
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__SDLRENDERERFILTER_H

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
// Anti-aliased D3D filter
//
//=============================================================================

#ifndef __AGS_EE_GFX__AAD3DGFXFILTER_H
#define __AGS_EE_GFX__AAD3DGFXFILTER_H

#include "gfx/gfxfilter_d3d.h"

namespace AGS
{
namespace Engine
{
namespace D3D
{

class AAD3DGfxFilter : public D3DGfxFilter
{
public:
    const GfxFilterInfo &GetInfo() const override;

    int  GetSamplerStateForStandardSprite() override;
    void SetSamplerStateForStandardSprite(void *direct3ddevice9) override;
    bool NeedToColourEdgeLines() override;

    static const GfxFilterInfo FilterInfo;
};

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__AAD3DGFXFILTER_H

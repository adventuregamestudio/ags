#ifndef __AGS_EE_GFX__GFXFILTER_D3D11_H
#define __AGS_EE_GFX__GFXFILTER_D3D11_H

#include "platform/platform.h"

#if !AGS_PLATFORM_OS_WINDOWS
#error This file should only be included on the Windows build
#endif

#define NOMINMAX
#include <d3d11.h>
#include "gfx/gfxfilter.h"
#include "gfx/gfxfilter_scaling.h"

namespace AGS
{
namespace Engine
{
namespace D3D11
{

class D3D11GfxFilter : public ScalingGfxFilter
{
public:
    static const GfxFilterInfo FilterInfo;

    const GfxFilterInfo& GetInfo() const;
    int GetSamplerStateForStandardSprite();
    void SetSamplerStateForStandardSprite(void* direct3dcontext);
    bool NeedToColourEdgeLines();

    // D3D11-specific helper. The AGS-facing interface above remains
    // compatible with the existing gfx filter API.
    static D3D11_FILTER GetD3D11Filter(bool linear);
};

} // namespace D3D11
} // namespace Engine
} // namespace AGS

#endif

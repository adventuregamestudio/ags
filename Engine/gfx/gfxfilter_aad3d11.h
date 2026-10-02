#ifndef __AGS_EE_GFX__GFXFILTER_AAD3D11_H
#define __AGS_EE_GFX__GFXFILTER_AAD3D11_H

#include "gfx/gfxfilter_d3d11.h"

namespace AGS
{
namespace Engine
{
namespace D3D11
{

class AAD3D11GfxFilter : public D3D11GfxFilter
{
public:
    static const GfxFilterInfo FilterInfo;

    const GfxFilterInfo& GetInfo() const;
    int GetSamplerStateForStandardSprite();
    void SetSamplerStateForStandardSprite(void* direct3dcontext);
    bool NeedToColourEdgeLines();
};

} // namespace D3D11
} // namespace Engine
} // namespace AGS

#endif

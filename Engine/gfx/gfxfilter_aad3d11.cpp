#include "platform/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#include "gfx/gfxfilter_aad3d11.h"

namespace AGS
{
namespace Engine
{
namespace D3D11
{

const GfxFilterInfo AAD3D11GfxFilter::FilterInfo =
    GfxFilterInfo("Linear", "Linear interpolation");

const GfxFilterInfo &AAD3D11GfxFilter::GetInfo() const
{
    return FilterInfo;
}

int AAD3D11GfxFilter::GetSamplerStateForStandardSprite()
{
    return 1; // D3D11_FILTER_MIN_MAG_MIP_LINEAR
}

void AAD3D11GfxFilter::SetSamplerStateForStandardSprite(void *direct3dcontext)
{
    (void)direct3dcontext;
}

bool AAD3D11GfxFilter::NeedToColourEdgeLines()
{
    return true;
}

} // namespace D3D11
} // namespace Engine
} // namespace AGS
#endif

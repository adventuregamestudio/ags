#include "platform/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#include "gfx/gfxfilter_d3d11.h"

namespace AGS
{
namespace Engine
{
namespace D3D11
{

const GfxFilterInfo D3D11GfxFilter::FilterInfo =
    GfxFilterInfo("StdScale", "Nearest-neighbour");

const GfxFilterInfo &D3D11GfxFilter::GetInfo() const
{
    return FilterInfo;
}

int D3D11GfxFilter::GetSamplerStateForStandardSprite()
{
    // Keep the historical AGS filter value for code which stores the
    // filter as an integer. The actual D3D11 sampler is an object.
    return 0; // D3D11_FILTER_MIN_MAG_MIP_POINT
}

void D3D11GfxFilter::SetSamplerStateForStandardSprite(void* direct3dcontext)
{
    // The D3D11 backend creates/binds the actual sampler in its driver.
    // This hook is intentionally a no-op unless the caller passes a
    // context whose sampler has already been installed by the driver.
    (void)direct3dcontext;
}

bool D3D11GfxFilter::NeedToColourEdgeLines()
{
    return false;
}

D3D11_FILTER D3D11GfxFilter::GetD3D11Filter(bool linear)
{
    return linear
        ? D3D11_FILTER_MIN_MAG_MIP_LINEAR
        : D3D11_FILTER_MIN_MAG_MIP_POINT;
}

} // namespace D3D11
} // namespace Engine
} // namespace AGS
#endif

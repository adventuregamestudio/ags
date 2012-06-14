
#include "stdio.h"
#include "acgfx/ac_aad3dgfxfilter.h"

#ifdef WINDOWS_VERSION
#include <d3d9.h>
#endif

AAD3DGFXFilter::AAD3DGFXFilter(int multiplier, bool justCheckingForSetup) : D3DGFXFilter(multiplier, justCheckingForSetup)
{
    sprintf(filterName, "%d" "x anti-aliasing filter[", multiplier);
    sprintf(filterID, "AAx%d", multiplier);
}

void AAD3DGFXFilter::SetSamplerStateForStandardSprite(void *direct3ddevice9)
{
#ifdef WINDOWS_VERSION
    IDirect3DDevice9* d3d9 = ((IDirect3DDevice9*)direct3ddevice9);
    d3d9->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    d3d9->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
#endif
}

bool AAD3DGFXFilter::NeedToColourEdgeLines()
{
    return true;
}


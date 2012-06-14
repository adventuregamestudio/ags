
#include <stdio.h>
#include "acgfx/ac_d3dgfxfilter.h"
#ifdef WINDOWS_VERSION
#include <d3d9.h>
#endif

// Standard 3D-accelerated filter

D3DGFXFilter::D3DGFXFilter(bool justCheckingForSetup) : ScalingGFXFilter(1, justCheckingForSetup) { 
    sprintf(filterName, "");
    sprintf(filterID, "None");
}

D3DGFXFilter::D3DGFXFilter(int multiplier, bool justCheckingForSetup) : ScalingGFXFilter(multiplier, justCheckingForSetup)
{
    sprintf(filterName, "%d" "x nearest-neighbour filter[", multiplier);
    sprintf(filterID, "StdScale%d", multiplier);
}

void D3DGFXFilter::SetSamplerStateForStandardSprite(void *direct3ddevice9)
{
#ifdef WINDOWS_VERSION
    IDirect3DDevice9* d3d9 = ((IDirect3DDevice9*)direct3ddevice9);
    d3d9->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    d3d9->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
#endif
}

bool D3DGFXFilter::NeedToColourEdgeLines()
{
    return false;
}

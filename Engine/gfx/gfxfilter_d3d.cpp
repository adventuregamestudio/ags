//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include <stdio.h>
#include "gfx/gfxfilter_d3d.h"
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

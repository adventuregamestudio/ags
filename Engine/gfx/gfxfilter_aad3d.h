/*
AGS specific color blending routines for transparency and tinting
effects

Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
All rights reserved.

The AGS Editor Source Code is provided under the Artistic License 2.0
http://www.opensource.org/licenses/artistic-license-2.0.php

You MAY NOT compile your own builds of the engine without making it EXPLICITLY
CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_AAD3DGFXFILTER_H
#define __AC_AAD3DGFXFILTER_H

#include "gfx/gfxfilter_d3d.h"

// Anti-aliased D3D filter

struct AAD3DGFXFilter : D3DGFXFilter {

    AAD3DGFXFilter(int multiplier, bool justCheckingForSetup);
    virtual void SetSamplerStateForStandardSprite(void *direct3ddevice9);
    virtual bool NeedToColourEdgeLines();
};

#endif // __AC_AAD3DGFXFILTER_H

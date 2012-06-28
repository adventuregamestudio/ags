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

#ifndef __AC_D3DGFXFILTER_H
#define __AC_D3DGFXFILTER_H

#include "gfx/scalinggfxfilter.h"

struct D3DGFXFilter : ScalingGFXFilter {
protected:

public:

    D3DGFXFilter(bool justCheckingForSetup);
    D3DGFXFilter(int multiplier, bool justCheckingForSetup);

    virtual void SetSamplerStateForStandardSprite(void *direct3ddevice9);
    virtual bool NeedToColourEdgeLines();
};

GFXFilter **get_allegro_gfx_filter_list(bool checkingForSetup);
GFXFilter **get_d3d_gfx_filter_list(bool checkingForSetup);

#endif // __AC_D3DGFXFILTER_H
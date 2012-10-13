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
//
// AGS specific color blending routines for transparency and tinting effects
//
//=============================================================================

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

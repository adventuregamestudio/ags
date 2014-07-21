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
// Base class for graphic filter which provides virtual screen scaling
//
//=============================================================================

#ifndef __AGS_EE_GFX__SCALINGGFXFILTER_H
#define __AGS_EE_GFX__SCALINGGFXFILTER_H

#include "gfx/gfxfilter.h"

namespace AGS
{
namespace Engine
{

struct MouseGetPosCallbackImpl;

class ScalingGfxFilter : public GfxFilter
{
public:
    virtual ~ScalingGfxFilter();

    virtual bool Initialize(const int color_depth, String &err_str);
    virtual void UnInitialize();
    virtual void GetRealResolution(int *width, int *height);
    virtual void SetMouseArea(int x1, int y1, int x2, int y2);
    virtual void SetMouseLimit(int x1, int y1, int x2, int y2);
    virtual void SetMousePosition(int x, int y);
    virtual void AdjustPosition(int *x, int *y);

protected:
    ScalingGfxFilter(int multiplier);

    int MULTIPLIER;
    MouseGetPosCallbackImpl *mouseCallbackPtr;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__SCALINGGFXFILTER_H

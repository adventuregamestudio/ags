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

#include "gfx/gfxfilter_scaling.h"
#include "util/wgt2allg.h"
#include "device/mousew32.h"
#include "gfx/gfxfilterhelpers.h"

namespace AGS
{
namespace Engine
{

ScalingGfxFilter::~ScalingGfxFilter()
{
    if (mouseCallbackPtr != NULL)
    {
        delete mouseCallbackPtr;
        mouseCallbackPtr = NULL;
    }
}
bool ScalingGfxFilter::Initialize(const int color_depth, String &err_str)
{
    mouseCallbackPtr = new MouseGetPosCallbackImpl(this);
    msetcallback(mouseCallbackPtr);
    return true;
}

void ScalingGfxFilter::UnInitialize()
{
    msetcallback(NULL);
}

void ScalingGfxFilter::GetRealResolution(int *width, int *height)
{
    *width *= MULTIPLIER;
    *height *= MULTIPLIER;
}

void ScalingGfxFilter::SetMouseArea(int x1, int y1, int x2, int y2)
{
    x1 *= MULTIPLIER;
    y1 *= MULTIPLIER;
    x2 *= MULTIPLIER;
    y2 *= MULTIPLIER;
    mgraphconfine(x1, y1, x2, y2);
}

void ScalingGfxFilter::SetMouseLimit(int x1, int y1, int x2, int y2)
{
    // 199 -> 399
    x1 = x1 * MULTIPLIER + (MULTIPLIER - 1);
    y1 = y1 * MULTIPLIER + (MULTIPLIER - 1);
    x2 = x2 * MULTIPLIER + (MULTIPLIER - 1);
    y2 = y2 * MULTIPLIER + (MULTIPLIER - 1);
    msetcursorlimit(x1, y1, x2, y2);
}

void ScalingGfxFilter::SetMousePosition(int x, int y)
{
    msetgraphpos(x * MULTIPLIER, y * MULTIPLIER);
}

void ScalingGfxFilter::AdjustPosition(int *x, int *y)
{
    *x /= MULTIPLIER;
    *y /= MULTIPLIER;
}

ScalingGfxFilter::ScalingGfxFilter(int multiplier)
{
    MULTIPLIER = multiplier;
    mouseCallbackPtr = NULL;
}

} // namespace Engine
} // namespace AGS

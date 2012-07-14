
#include <stdio.h>
#include "gfx/gfxfilter_scaling.h"
#include "util/wgt2allg.h"
#include "device/mousew32.h"
#include "gfx/gfxfilterhelpers.h"

// Standard scaling filter

ScalingGFXFilter::ScalingGFXFilter(int multiplier, bool justCheckingForSetup) : GFXFilter() {
    MULTIPLIER = multiplier;
    mouseCallbackPtr = NULL;

    sprintf(filterName, "%d" "x nearest-neighbour filter[", multiplier);
    sprintf(filterID, "StdScale%d", multiplier);
}

const char* ScalingGFXFilter::Initialize(int width, int height, int colDepth) {
    mouseCallbackPtr = new MouseGetPosCallbackImpl(this);
    msetcallback((MouseGetPosCallbackImpl*)mouseCallbackPtr);
    return NULL;
}

void ScalingGFXFilter::UnInitialize() {
    msetcallback(NULL);
}

void ScalingGFXFilter::GetRealResolution(int *wid, int *hit) {
    *wid *= MULTIPLIER;
    *hit *= MULTIPLIER;
}

void ScalingGFXFilter::SetMouseArea(int x1, int y1, int x2, int y2) {
    x1 *= MULTIPLIER;
    y1 *= MULTIPLIER;
    x2 *= MULTIPLIER;
    y2 *= MULTIPLIER;
    mgraphconfine(x1, y1, x2, y2);
}

void ScalingGFXFilter::SetMouseLimit(int x1, int y1, int x2, int y2) {
    // 199 -> 399
    x1 = x1 * MULTIPLIER + (MULTIPLIER - 1);
    y1 = y1 * MULTIPLIER + (MULTIPLIER - 1);
    x2 = x2 * MULTIPLIER + (MULTIPLIER - 1);
    y2 = y2 * MULTIPLIER + (MULTIPLIER - 1);
    msetcursorlimit(x1, y1, x2, y2);
}

void ScalingGFXFilter::SetMousePosition(int x, int y) {
    msetgraphpos(x * MULTIPLIER, y * MULTIPLIER);
}

void ScalingGFXFilter::AdjustPosition(int *x, int *y) {
    *x /= MULTIPLIER;
    *y /= MULTIPLIER;
}

const char *ScalingGFXFilter::GetVersionBoxText() {
    return filterName;
}

const char *ScalingGFXFilter::GetFilterID() {
    return filterID;
}

ScalingGFXFilter::~ScalingGFXFilter()
{
    if (mouseCallbackPtr != NULL)
    {
        delete mouseCallbackPtr;
        mouseCallbackPtr = NULL;
    }
}

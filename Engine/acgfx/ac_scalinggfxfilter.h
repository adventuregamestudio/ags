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

#ifndef __AC_SCALINGGFXFILTER_H
#define __AC_SCALINGGFXFILTER_H

#include "acgfx/ac_gfxfilter.h"

struct ScalingGFXFilter : public GFXFilter {
protected:
    int MULTIPLIER;
    void *mouseCallbackPtr;

    char filterName[100];
    char filterID[15];

    ScalingGFXFilter(int multiplier, bool justCheckingForSetup);

public:

    virtual const char* Initialize(int width, int height, int colDepth);
    virtual void UnInitialize();
    virtual void GetRealResolution(int *wid, int *hit);
    virtual void SetMouseArea(int x1, int y1, int x2, int y2);
    virtual void SetMouseLimit(int x1, int y1, int x2, int y2);
    virtual void SetMousePosition(int x, int y);
    virtual void AdjustPosition(int *x, int *y);
    virtual const char *GetVersionBoxText();
    virtual const char *GetFilterID();
    virtual ~ScalingGFXFilter();
};

#endif // __AC_SCALINGGFXFILTER_H
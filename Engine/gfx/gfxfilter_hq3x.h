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

#ifndef __AC_HQ3XGFXFILTER_H
#define __AC_HQ3XGFXFILTER_H

#include "gfx/gfxfilter_scalingallegro.h"

struct Hq3xGFXFilter : public ScalingAllegroGFXFilter {
private:
    BITMAP *realScreenBuffer;

public:

    Hq3xGFXFilter(bool justCheckingForSetup) : ScalingAllegroGFXFilter(3, justCheckingForSetup) { }

    virtual const char* Initialize(int width, int height, int colDepth);
    virtual BITMAP* ScreenInitialized(BITMAP *screen, int fakeWidth, int fakeHeight);
    virtual BITMAP *ShutdownAndReturnRealScreen(BITMAP *currentScreen);
    virtual void RenderScreen(BITMAP *toRender, int x, int y);
    virtual const char *GetVersionBoxText();
    virtual const char *GetFilterID();
};

#endif // __AC_HQ3XGFXFILTER_H
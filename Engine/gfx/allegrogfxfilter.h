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

#ifndef __AC_ALLEGROGFXFILTER_H
#define __AC_ALLEGROGFXFILTER_H

#include "gfx/scalinggfxfilter.h"

struct AllegroGFXFilter : ScalingGFXFilter {
protected:
    BITMAP *realScreen;
    int lastBlitX;
    int lastBlitY;

public:

    AllegroGFXFilter(bool justCheckingForSetup);
    AllegroGFXFilter(int multiplier, bool justCheckingForSetup);

    virtual BITMAP* ScreenInitialized(BITMAP *screen, int fakeWidth, int fakeHeight);
    virtual BITMAP *ShutdownAndReturnRealScreen(BITMAP *currentScreen);
    virtual void RenderScreen(BITMAP *toRender, int x, int y);
    virtual void RenderScreenFlipped(BITMAP *toRender, int x, int y, int flipType);
    virtual void ClearRect(int x1, int y1, int x2, int y2, int color);
    virtual void GetCopyOfScreenIntoBitmap(BITMAP *copyBitmap);
    virtual void GetCopyOfScreenIntoBitmap(BITMAP *copyBitmap, bool copyWithYOffset);
};

#endif // __AC_ALLEGROGFXFILTER_H
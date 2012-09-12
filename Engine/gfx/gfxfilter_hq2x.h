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

#ifndef __AC_HQ2XGFXFILTER_H
#define __AC_HQ2XGFXFILTER_H

#include "gfx/gfxfilter_scalingallegro.h"

namespace AGS { namespace Common { class Bitmap; }}
using namespace AGS; // FIXME later

struct Hq2xGFXFilter : public ScalingAllegroGFXFilter {
private:
    Common::Bitmap *realScreenBuffer;

public:

    Hq2xGFXFilter(bool justCheckingForSetup) : ScalingAllegroGFXFilter(2, justCheckingForSetup) { }

    virtual const char* Initialize(int width, int height, int colDepth);


    virtual Common::Bitmap *ScreenInitialized(Common::Bitmap *screen, int fakeWidth, int fakeHeight);

    virtual Common::Bitmap *ShutdownAndReturnRealScreen(Common::Bitmap *currentScreen);

    virtual void RenderScreen(Common::Bitmap *toRender, int x, int y);

    virtual const char *GetVersionBoxText();

    virtual const char *GetFilterID();
};

#endif // __AC_HQ2XGFXFILTER_H
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

#ifndef __AC_SCALINGALLEGROGFXFILTER_H
#define __AC_SCALINGALLEGROGFXFILTER_H

#include "gfx/gfxfilter_allegro.h"

namespace AGS { namespace Common { class Bitmap; }}
using namespace AGS; // FIXME later

struct ScalingAllegroGFXFilter : public AllegroGFXFilter {
protected:
    Common::Bitmap *fakeScreen;
    Common::Bitmap *realScreenSizedBuffer;
    Common::Bitmap *lastBlitFrom;

public:

    ScalingAllegroGFXFilter(int multiplier, bool justCheckingForSetup);

      virtual Common::Bitmap *ScreenInitialized(Common::Bitmap *screen, int fakeWidth, int fakeHeight);

      virtual Common::Bitmap *ShutdownAndReturnRealScreen(Common::Bitmap *currentScreen);

      virtual void RenderScreen(Common::Bitmap *toRender, int x, int y);

      virtual void RenderScreenFlipped(Common::Bitmap *toRender, int x, int y, int flipType);

      virtual void ClearRect(int x1, int y1, int x2, int y2, int color);
      virtual void GetCopyOfScreenIntoBitmap(Common::Bitmap *copyBitmap);
      virtual void GetCopyOfScreenIntoBitmap(Common::Bitmap *copyBitmap, bool copyWithYOffset);

};

#endif // __AC_SCALINGALLEGROGFXFILTER_H
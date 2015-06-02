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

#ifndef __AC_HQ3XGFXFILTER_H
#define __AC_HQ3XGFXFILTER_H

#include "gfx/gfxfilter_scalingallegro.h"

namespace AGS { namespace Common { class Bitmap; }}
using namespace AGS; // FIXME later

struct Hq3xGFXFilter : public ScalingAllegroGFXFilter {
private:
    Common::Bitmap *realScreenBuffer;

public:

    Hq3xGFXFilter(bool justCheckingForSetup) : ScalingAllegroGFXFilter(3, justCheckingForSetup) { }

    virtual const char* Initialize(int colDepth);
    virtual Common::Bitmap *ScreenInitialized(Common::Bitmap *screen, int fakeWidth, int fakeHeight);
    virtual Common::Bitmap *ShutdownAndReturnRealScreen(Common::Bitmap *currentScreen);
    virtual void RenderScreen(Common::Bitmap *toRender, int x, int y);
    virtual const char *GetVersionBoxText();
    virtual const char *GetFilterID();
};

#endif // __AC_HQ3XGFXFILTER_H

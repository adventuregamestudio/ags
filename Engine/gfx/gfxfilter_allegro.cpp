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

#include <stdio.h>
#include "gfx/gfxfilter_allegro.h"
#include "gfx/gfxfilterdefines.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;

// Standard Allegro filter

AllegroGFXFilter::AllegroGFXFilter(bool justCheckingForSetup) : ScalingGFXFilter(1, justCheckingForSetup) { 
    lastBlitX = 0;
    lastBlitY = 0;

    sprintf(filterName, "");
    sprintf(filterID, "None");
}

AllegroGFXFilter::AllegroGFXFilter(int multiplier, bool justCheckingForSetup) : ScalingGFXFilter(multiplier, justCheckingForSetup)
{
    lastBlitX = 0;
    lastBlitY = 0;
}

Bitmap* AllegroGFXFilter::ScreenInitialized(Bitmap *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    return screen;
}

Bitmap *AllegroGFXFilter::ShutdownAndReturnRealScreen(Bitmap *currentScreen) {
    return currentScreen;
}

void AllegroGFXFilter::RenderScreen(Bitmap *toRender, int x, int y) {

    if (toRender != realScreen) 
        realScreen->Blit(toRender, 0, 0, x, y, toRender->GetWidth(), toRender->GetHeight());

    lastBlitX = x;
    lastBlitY = y;
}

void AllegroGFXFilter::RenderScreenFlipped(Bitmap *toRender, int x, int y, int flipType) {

    if (toRender == realScreen) 
        return;

    if (flipType == SCR_HFLIP)
		realScreen->FlipBlt(toRender, 0, 0, Common::kBitmap_HFlip);
    else if (flipType == SCR_VFLIP)
        realScreen->FlipBlt(toRender, 0, 0, Common::kBitmap_VFlip);
    else if (flipType == SCR_VHFLIP)
        realScreen->FlipBlt(toRender, 0, 0, Common::kBitmap_HVFlip);
}

void AllegroGFXFilter::ClearRect(int x1, int y1, int x2, int y2, int color) {
    realScreen->FillRect(Rect(x1, y1, x2, y2), color);
}

void AllegroGFXFilter::GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap) 
{
    GetCopyOfScreenIntoBitmap(copyBitmap, true);
}

void AllegroGFXFilter::GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap, bool copyWithOffset)
{
    if (copyBitmap != realScreen)
        copyBitmap->Blit(realScreen, (copyWithOffset ? lastBlitX : 0), (copyWithOffset ? lastBlitY : 0), 0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight());
}

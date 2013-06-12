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

#include "core/types.h"
#include "gfx/gfxfilter_scalingallegro.h"
#include "gfx/gfxfilterdefines.h"
#include "gfx/graphics.h"

using AGS::Common::Bitmap;
using AGS::Common::Graphics;
namespace BitmapHelper = AGS::Common::BitmapHelper;

ScalingAllegroGFXFilter::ScalingAllegroGFXFilter(int multiplier, bool justCheckingForSetup) : 
    AllegroGFXFilter(multiplier, justCheckingForSetup) {

    lastBlitFrom = NULL;
}

Bitmap* ScalingAllegroGFXFilter::ScreenInitialized(Bitmap *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    realScreenSizedBuffer = BitmapHelper::CreateBitmap(screen->GetWidth(), screen->GetHeight(), screen->GetColorDepth());
    fakeScreen = BitmapHelper::CreateBitmap(fakeWidth, fakeHeight, screen->GetColorDepth());
    return fakeScreen;
}

Bitmap *ScalingAllegroGFXFilter::ShutdownAndReturnRealScreen(Bitmap *currentScreen) {
    delete fakeScreen;
    delete realScreenSizedBuffer;
    fakeScreen = NULL;
    realScreenSizedBuffer = NULL;
    return realScreen;
}

void ScalingAllegroGFXFilter::RenderScreen(Bitmap *toRender, int x, int y) 
{
    Graphics graphics(realScreen);
    graphics.StretchBlt(toRender, RectWH(0, 0, toRender->GetWidth(), toRender->GetHeight()),
		RectWH(x * MULTIPLIER, y * MULTIPLIER, toRender->GetWidth() * MULTIPLIER, toRender->GetHeight() * MULTIPLIER));
    lastBlitX = x;
    lastBlitY = y;
    lastBlitFrom = toRender;
}

void ScalingAllegroGFXFilter::RenderScreenFlipped(Bitmap *toRender, int x, int y, int flipType) {

    if (toRender == fakeScreen)
        return;

    Graphics graphics(fakeScreen);
    if (flipType == SCR_HFLIP)
		graphics.FlipBlt(toRender, 0, 0, Common::kBitmap_HFlip);
    else if (flipType == SCR_VFLIP)
        graphics.FlipBlt(toRender, 0, 0, Common::kBitmap_VFlip);
    else if (flipType == SCR_VHFLIP)
        graphics.FlipBlt(toRender, 0, 0, Common::kBitmap_HVFlip);

    RenderScreen(fakeScreen, x, y);
}

void ScalingAllegroGFXFilter::ClearRect(int x1, int y1, int x2, int y2, int color) {
    x1 *= MULTIPLIER;
    y1 *= MULTIPLIER;
    x2 = x2 * MULTIPLIER + (MULTIPLIER - 1);
    y2 = y2 * MULTIPLIER + (MULTIPLIER - 1);
    Graphics graphics(realScreen);
    graphics.FillRect(Rect(x1, y1, x2, y2), color);
}

 void ScalingAllegroGFXFilter::GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap) 
{
    GetCopyOfScreenIntoBitmap(copyBitmap, true);
}

void ScalingAllegroGFXFilter::GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap, bool copyWithYOffset)
{
    Graphics graphics(copyBitmap);
    if (!copyWithYOffset)
    {
        // Can't stretch_blit from Video Memory to normal memory,
        // so copy the screen to a buffer first.
        Graphics real_graphics(realScreenSizedBuffer);
        real_graphics.Blit(realScreen, 0, 0, 0, 0, realScreen->GetWidth(), realScreen->GetHeight());
        graphics.StretchBlt(realScreenSizedBuffer,
			RectWH(0, 0, realScreenSizedBuffer->GetWidth(), realScreenSizedBuffer->GetHeight()), 
            RectWH(0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight()));
    }
    else if (lastBlitFrom == NULL)
        graphics.Fill(0);
    else
        graphics.StretchBlt(lastBlitFrom,
		RectWH(0, 0, lastBlitFrom->GetWidth(), lastBlitFrom->GetHeight()), 
        RectWH(0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight()));
}

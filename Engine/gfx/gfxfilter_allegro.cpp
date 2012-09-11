
#include "util/wgt2allg.h"
#include "gfx/gfxfilter_allegro.h"
#include "gfx/gfxfilterdefines.h"
#include "gfx/bitmap.h"

using AGS::Common::IBitmap;

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

IBitmap* AllegroGFXFilter::ScreenInitialized(IBitmap *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    return screen;
}

IBitmap *AllegroGFXFilter::ShutdownAndReturnRealScreen(IBitmap *currentScreen) {
    return currentScreen;
}

void AllegroGFXFilter::RenderScreen(IBitmap *toRender, int x, int y) {

    if (toRender != realScreen) 
        realScreen->Blit(toRender, 0, 0, x, y, toRender->GetWidth(), toRender->GetHeight());

    lastBlitX = x;
    lastBlitY = y;
}

void AllegroGFXFilter::RenderScreenFlipped(IBitmap *toRender, int x, int y, int flipType) {

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
    realScreen->FillRect(CRect(x1, y1, x2, y2), color);
}

void AllegroGFXFilter::GetCopyOfScreenIntoBitmap(IBitmap *copyBitmap) 
{
    GetCopyOfScreenIntoBitmap(copyBitmap, true);
}

void AllegroGFXFilter::GetCopyOfScreenIntoBitmap(IBitmap *copyBitmap, bool copyWithOffset)
{
    if (copyBitmap != realScreen)
        copyBitmap->Blit(realScreen, (copyWithOffset ? lastBlitX : 0), (copyWithOffset ? lastBlitY : 0), 0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight());
}

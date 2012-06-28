
#include "wgt2allg.h"
#include "gfx/allegrogfxfilter.h"
#include "gfx/gfxfilterdefines.h"

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

BITMAP* AllegroGFXFilter::ScreenInitialized(BITMAP *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    return screen;
}

BITMAP *AllegroGFXFilter::ShutdownAndReturnRealScreen(BITMAP *currentScreen) {
    return currentScreen;
}

void AllegroGFXFilter::RenderScreen(BITMAP *toRender, int x, int y) {

    if (toRender != realScreen) 
        blit(toRender, realScreen, 0, 0, x, y, toRender->w, toRender->h);

    lastBlitX = x;
    lastBlitY = y;
}

void AllegroGFXFilter::RenderScreenFlipped(BITMAP *toRender, int x, int y, int flipType) {

    if (toRender == realScreen) 
        return;

    if (flipType == SCR_HFLIP)
        draw_sprite_h_flip(realScreen, toRender, 0, 0);
    else if (flipType == SCR_VFLIP)
        draw_sprite_v_flip(realScreen, toRender, 0, 0);
    else if (flipType == SCR_VHFLIP)
        draw_sprite_vh_flip(realScreen, toRender, 0, 0);
}

void AllegroGFXFilter::ClearRect(int x1, int y1, int x2, int y2, int color) {
    rectfill(realScreen, x1, y1, x2, y2, color);
}

void AllegroGFXFilter::GetCopyOfScreenIntoBitmap(BITMAP *copyBitmap) 
{
    GetCopyOfScreenIntoBitmap(copyBitmap, true);
}

void AllegroGFXFilter::GetCopyOfScreenIntoBitmap(BITMAP *copyBitmap, bool copyWithOffset)
{
    if (copyBitmap != realScreen)
        blit(realScreen, copyBitmap, (copyWithOffset ? lastBlitX : 0), (copyWithOffset ? lastBlitY : 0), 0, 0, copyBitmap->w, copyBitmap->h);
}

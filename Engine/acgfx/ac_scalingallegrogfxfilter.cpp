
#include "wgt2allg.h"
#include "acgfx/ac_scalingallegrogfxfilter.h"
#include "acgfx/ac_gfxfilterdefines.h"

ScalingAllegroGFXFilter::ScalingAllegroGFXFilter(int multiplier, bool justCheckingForSetup) : 
    AllegroGFXFilter(multiplier, justCheckingForSetup) {

    lastBlitFrom = NULL;
}

BITMAP* ScalingAllegroGFXFilter::ScreenInitialized(BITMAP *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    realScreenSizedBuffer = create_bitmap_ex(bitmap_color_depth(screen), screen->w, screen->h);
    fakeScreen = create_bitmap_ex(bitmap_color_depth(screen), fakeWidth, fakeHeight);
    return fakeScreen;
}

BITMAP *ScalingAllegroGFXFilter::ShutdownAndReturnRealScreen(BITMAP *currentScreen) {
    destroy_bitmap(fakeScreen);
    destroy_bitmap(realScreenSizedBuffer);
    fakeScreen = NULL;
    realScreenSizedBuffer = NULL;
    return realScreen;
}

void ScalingAllegroGFXFilter::RenderScreen(BITMAP *toRender, int x, int y) 
{
    stretch_blit(toRender, realScreen, 0, 0, toRender->w, toRender->h, x * MULTIPLIER, y * MULTIPLIER, toRender->w * MULTIPLIER, toRender->h * MULTIPLIER);
    lastBlitX = x;
    lastBlitY = y;
    lastBlitFrom = toRender;
}

void ScalingAllegroGFXFilter::RenderScreenFlipped(BITMAP *toRender, int x, int y, int flipType) {

    if (toRender == fakeScreen)
        return;

    if (flipType == SCR_HFLIP)
        draw_sprite_h_flip(fakeScreen, toRender, 0, 0);
    else if (flipType == SCR_VFLIP)
        draw_sprite_v_flip(fakeScreen, toRender, 0, 0);
    else if (flipType == SCR_VHFLIP)
        draw_sprite_vh_flip(fakeScreen, toRender, 0, 0);

    RenderScreen(fakeScreen, x, y);
}

void ScalingAllegroGFXFilter::ClearRect(int x1, int y1, int x2, int y2, int color) {
    x1 *= MULTIPLIER;
    y1 *= MULTIPLIER;
    x2 = x2 * MULTIPLIER + (MULTIPLIER - 1);
    y2 = y2 * MULTIPLIER + (MULTIPLIER - 1);
    rectfill(realScreen, x1, y1, x2, y2, color);
}

 void ScalingAllegroGFXFilter::GetCopyOfScreenIntoBitmap(BITMAP *copyBitmap) 
{
    GetCopyOfScreenIntoBitmap(copyBitmap, true);
}

void ScalingAllegroGFXFilter::GetCopyOfScreenIntoBitmap(BITMAP *copyBitmap, bool copyWithYOffset)
{
    if (!copyWithYOffset)
    {
        // Can't stretch_blit from Video Memory to normal memory,
        // so copy the screen to a buffer first.
        blit(realScreen, realScreenSizedBuffer, 0, 0, 0, 0, realScreen->w, realScreen->h);
        stretch_blit(realScreenSizedBuffer, copyBitmap, 0, 0, 
            realScreenSizedBuffer->w, realScreenSizedBuffer->h, 
            0, 0, copyBitmap->w, copyBitmap->h);
    }
    else if (lastBlitFrom == NULL)
        clear(copyBitmap);
    else
        stretch_blit(lastBlitFrom, copyBitmap, 0, 0, 
        lastBlitFrom->w, lastBlitFrom->h, 
        0, 0, copyBitmap->w, copyBitmap->h);
}

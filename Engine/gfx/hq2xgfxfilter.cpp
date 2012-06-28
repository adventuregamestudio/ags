
#include "wgt2allg.h"
#include "gfx/hq2xgfxfilter.h"
#include "gfx/hq2x3x.h"
#include "gfx/gfxfilterdefines.h"

const char* Hq2xGFXFilter::Initialize(int width, int height, int colDepth) {
    if (colDepth < 32)
        return "Only supports 32-bit colour games";

    return ScalingGFXFilter::Initialize(width, height, colDepth);
}


BITMAP* Hq2xGFXFilter::ScreenInitialized(BITMAP *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    realScreenBuffer = create_bitmap(screen->w, screen->h);
    realScreenSizedBuffer = create_bitmap_ex(bitmap_color_depth(screen), screen->w, screen->h);
    fakeScreen = create_bitmap_ex(bitmap_color_depth(screen), fakeWidth, fakeHeight);
    InitLUTs();
    return fakeScreen;
}

BITMAP *Hq2xGFXFilter::ShutdownAndReturnRealScreen(BITMAP *currentScreen) {
    destroy_bitmap(fakeScreen);
    destroy_bitmap(realScreenBuffer);
    destroy_bitmap(realScreenSizedBuffer);
    return realScreen;
}

void Hq2xGFXFilter::RenderScreen(BITMAP *toRender, int x, int y) {

    acquire_bitmap(realScreenBuffer);
    hq2x_32(&toRender->line[0][0], &realScreenBuffer->line[0][0], toRender->w, toRender->h, realScreenBuffer->w * BYTES_PER_PIXEL(bitmap_color_depth(realScreenBuffer)));
    release_bitmap(realScreenBuffer);

    blit(realScreenBuffer, realScreen, 0, 0, x * MULTIPLIER, y * MULTIPLIER, realScreen->w, realScreen->h);

    lastBlitFrom = toRender;
}

const char *Hq2xGFXFilter::GetVersionBoxText() {
    return "Hq2x filter (32-bit only)[";
}

const char *Hq2xGFXFilter::GetFilterID() {
    return "Hq2x";
}
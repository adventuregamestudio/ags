
#include "util/wgt2allg.h"
#include "gfx/gfxfilter_hq3x.h"
#include "gfx/hq2x3x.h"
#include "gfx/gfxfilterdefines.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

const char* Hq3xGFXFilter::Initialize(int width, int height, int colDepth) {
    if (colDepth < 32)
        return "Only supports 32-bit colour games";

    return ScalingGFXFilter::Initialize(width, height, colDepth);
}


Bitmap* Hq3xGFXFilter::ScreenInitialized(Bitmap *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    realScreenBuffer = BitmapHelper::CreateBitmap(screen->GetWidth(), screen->GetHeight());
    realScreenSizedBuffer = BitmapHelper::CreateBitmap(screen->GetWidth(), screen->GetHeight(), screen->GetColorDepth());
    fakeScreen = BitmapHelper::CreateBitmap(fakeWidth, fakeHeight, screen->GetColorDepth());
    InitLUTs();
    return fakeScreen;
}

Bitmap *Hq3xGFXFilter::ShutdownAndReturnRealScreen(Bitmap *currentScreen) {
    delete fakeScreen;
    delete realScreenBuffer;
    delete realScreenSizedBuffer;
    return realScreen;
}

void Hq3xGFXFilter::RenderScreen(Bitmap *toRender, int x, int y) {

    realScreenBuffer->Acquire();
    hq3x_32(&toRender->GetScanLineForWriting(0)[0], &realScreenBuffer->GetScanLineForWriting(0)[0], toRender->GetWidth(), toRender->GetHeight(), realScreenBuffer->GetWidth() * BYTES_PER_PIXEL(realScreenBuffer->GetColorDepth()));
    realScreenBuffer->Release();

    realScreen->Blit(realScreenBuffer, 0, 0, x * MULTIPLIER, y * MULTIPLIER, realScreen->GetWidth(), realScreen->GetHeight());

    lastBlitFrom = toRender;
}

const char *Hq3xGFXFilter::GetVersionBoxText() {
    return "Hq3x filter (32-bit only)[";
}

const char *Hq3xGFXFilter::GetFilterID() {
    return "Hq3x";
}

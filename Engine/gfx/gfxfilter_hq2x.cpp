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

#include "gfx/bitmap.h"
#include "gfx/gfxfilter_hq2x.h"
#include "gfx/hq2x3x.h"
#include "gfx/gfxfilterdefines.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

const char* Hq2xGFXFilter::Initialize(int width, int height, int colDepth) {
    if (colDepth < 32)
        return "Only supports 32-bit colour games";

    return ScalingGFXFilter::Initialize(width, height, colDepth);
}


Bitmap* Hq2xGFXFilter::ScreenInitialized(Bitmap *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    realScreenBuffer = BitmapHelper::CreateBitmap(screen->GetWidth(), screen->GetHeight());
    realScreenSizedBuffer = BitmapHelper::CreateBitmap(screen->GetWidth(), screen->GetHeight(), screen->GetColorDepth());
    fakeScreen = BitmapHelper::CreateBitmap(fakeWidth, fakeHeight, screen->GetColorDepth());
    InitLUTs();
    return fakeScreen;
}

Bitmap *Hq2xGFXFilter::ShutdownAndReturnRealScreen(Bitmap *currentScreen) {
    delete fakeScreen;
    delete realScreenBuffer;
    delete realScreenSizedBuffer;
    return realScreen;
}

void Hq2xGFXFilter::RenderScreen(Bitmap *toRender, int x, int y) {

    realScreenBuffer->Acquire();
    hq2x_32(&toRender->GetScanLineForWriting(0)[0], &realScreenBuffer->GetScanLineForWriting(0)[0], toRender->GetWidth(), toRender->GetHeight(), realScreenBuffer->GetWidth() * BYTES_PER_PIXEL(realScreenBuffer->GetColorDepth()));
    realScreenBuffer->Release();

    realScreen->Blit(realScreenBuffer, 0, 0, x * MULTIPLIER, y * MULTIPLIER, realScreen->GetWidth(), realScreen->GetHeight());

    lastBlitFrom = toRender;
}

const char *Hq2xGFXFilter::GetVersionBoxText() {
    return "Hq2x filter (32-bit only)[";
}

const char *Hq2xGFXFilter::GetFilterID() {
    return "Hq2x";
}
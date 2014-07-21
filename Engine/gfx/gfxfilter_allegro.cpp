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

namespace AGS
{
namespace Engine
{
namespace ALSW
{

using namespace Common;

const GfxFilterInfo AllegroGfxFilter::FilterInfo = GfxFilterInfo("StdScale", "Nearest-neighbour");

AllegroGfxFilter::AllegroGfxFilter(int multiplier)
    : ScalingGfxFilter(multiplier)
    , realScreen(NULL)
    , virtualScreen(NULL)
    , realScreenSizedBuffer(NULL)
    , lastBlitFrom(NULL)
    , lastBlitX(0)
    , lastBlitY(0)
{
}

const GfxFilterInfo &AllegroGfxFilter::GetInfo() const
{
    return FilterInfo;
}

Bitmap* AllegroGfxFilter::InitVirtualScreen(Bitmap *screen, int virtual_width, int virtual_height)
{
    realScreen = screen;
    if (MULTIPLIER == 1)
    {
        // Speed up software rendering if no scaling is performed
        realScreenSizedBuffer = NULL;
        virtualScreen = realScreen;
    }
    else
    {
        realScreenSizedBuffer = BitmapHelper::CreateBitmap(screen->GetWidth(), screen->GetHeight(), screen->GetColorDepth());
        virtualScreen = BitmapHelper::CreateBitmap(virtual_width, virtual_height, screen->GetColorDepth());
    }
    return virtualScreen;
}

Bitmap *AllegroGfxFilter::ShutdownAndReturnRealScreen(Bitmap *currentScreen)
{
    if (virtualScreen != realScreen)
        delete virtualScreen;
    delete realScreenSizedBuffer;
    virtualScreen = NULL;
    realScreenSizedBuffer = NULL;
    return realScreen;
}

void AllegroGfxFilter::RenderScreen(Bitmap *toRender, int x, int y) {

    if (toRender != realScreen)
    {
        x *= MULTIPLIER;
        y *= MULTIPLIER;
        Bitmap *render_src = PreRenderPass(toRender);
        if (MULTIPLIER == 1)
            realScreen->Blit(render_src, x, y);
        else
            realScreen->StretchBlt(render_src, RectWH(x, y, toRender->GetWidth() * MULTIPLIER, toRender->GetHeight() * MULTIPLIER));
    }
    lastBlitX = x;
    lastBlitY = y;
    lastBlitFrom = toRender;
}

void AllegroGfxFilter::RenderScreenFlipped(Bitmap *toRender, int x, int y, GlobalFlipType flipType) {

    if (toRender == virtualScreen)
        return;

    switch (flipType)
    {
    case kFlip_Horizontal:
        virtualScreen->FlipBlt(toRender, 0, 0, Common::kBitmap_HFlip);
        break;
    case kFlip_Vertical:
        virtualScreen->FlipBlt(toRender, 0, 0, Common::kBitmap_VFlip);
        break;
    case kFlip_Both:
        virtualScreen->FlipBlt(toRender, 0, 0, Common::kBitmap_HVFlip);
        break;
    default:
        virtualScreen->Blit(toRender, 0, 0);
        break;
    }

    RenderScreen(virtualScreen, x, y);
}

void AllegroGfxFilter::ClearRect(int x1, int y1, int x2, int y2, int color)
{
    x1 *= MULTIPLIER;
    y1 *= MULTIPLIER;
    x2 = x2 * MULTIPLIER + (MULTIPLIER - 1);
    y2 = y2 * MULTIPLIER + (MULTIPLIER - 1);
    realScreen->FillRect(Rect(x1, y1, x2, y2), color);
}

void AllegroGfxFilter::GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap) 
{
    GetCopyOfScreenIntoBitmap(copyBitmap, true);
}

void AllegroGfxFilter::GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap, bool copy_with_yoffset)
{
    if (copyBitmap == realScreen)
        return;

    if (!copy_with_yoffset)
    {
        if (MULTIPLIER == 1)
            copyBitmap->Blit(realScreen, 0, 0, 0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight());
        else
        {
            // Can't stretch_blit from Video Memory to normal memory,
            // so copy the screen to a buffer first.
            realScreenSizedBuffer->Blit(realScreen, 0, 0, 0, 0, realScreen->GetWidth(), realScreen->GetHeight());
            copyBitmap->StretchBlt(realScreenSizedBuffer,
                RectWH(0, 0, realScreenSizedBuffer->GetWidth(), realScreenSizedBuffer->GetHeight()), 
                RectWH(0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight()));
        }
    }
    else if (!lastBlitFrom)
        copyBitmap->Fill(0);
    else if (MULTIPLIER == 1)
        copyBitmap->Blit(realScreen, lastBlitX, lastBlitY, 0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight());
    else
        copyBitmap->StretchBlt(lastBlitFrom,
            RectWH(0, 0, lastBlitFrom->GetWidth(), lastBlitFrom->GetHeight()), 
            RectWH(0, 0, copyBitmap->GetWidth(), copyBitmap->GetHeight()));
}

Bitmap *AllegroGfxFilter::PreRenderPass(Bitmap *toRender)
{
    // do nothing
    return toRender;
}

} // namespace ALSW
} // namespace Engine
} // namespace AGS

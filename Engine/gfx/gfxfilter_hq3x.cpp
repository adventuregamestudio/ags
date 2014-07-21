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
#include "gfx/gfxfilter_hq3x.h"
#include "gfx/hq2x3x.h"

namespace AGS
{
namespace Engine
{
namespace ALSW
{

using namespace Common;

const GfxFilterInfo Hq3xGfxFilter::FilterInfo = GfxFilterInfo("Hq3x", "Hq3x", 3);

Hq3xGfxFilter::Hq3xGfxFilter()
    : AllegroGfxFilter(3)
{
}

const GfxFilterInfo &Hq3xGfxFilter::GetInfo() const
{
    return FilterInfo;
}

bool Hq3xGfxFilter::Initialize(const int color_depth, String &err_str)
{
    if (color_depth < 32)
    {
        err_str = "Only supports 32-bit colour games";
        return false;
    }
    return AllegroGfxFilter::Initialize(color_depth, err_str);
}

Bitmap* Hq3xGfxFilter::InitVirtualScreen(Bitmap *screen, int virtual_width, int virtual_height)
{
    Bitmap *virtual_screen = AllegroGfxFilter::InitVirtualScreen(screen, virtual_width, virtual_height);
    _hq3xScalingBuffer = BitmapHelper::CreateBitmap(screen->GetWidth(), screen->GetHeight());

    InitLUTs();
    return virtual_screen;
}

Bitmap *Hq3xGfxFilter::ShutdownAndReturnRealScreen(Bitmap *currentScreen)
{
    Bitmap *real_screen = AllegroGfxFilter::ShutdownAndReturnRealScreen(currentScreen);
    delete _hq3xScalingBuffer;
    _hq3xScalingBuffer = NULL;
    return real_screen;
}

Bitmap *Hq3xGfxFilter::PreRenderPass(Bitmap *toRender)
{
    _hq3xScalingBuffer->Acquire();
    hq3x_32(toRender->GetDataForWriting(), _hq3xScalingBuffer->GetDataForWriting(),
        toRender->GetWidth(), toRender->GetHeight(), _hq3xScalingBuffer->GetLineLength());
    _hq3xScalingBuffer->Release();
    return _hq3xScalingBuffer;
}

} // namespace ALSW
} // namespace Engine
} // namespace AGS

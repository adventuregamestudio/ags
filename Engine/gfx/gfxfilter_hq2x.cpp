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

namespace AGS
{
namespace Engine
{
namespace ALSW
{

using namespace Common;

const GfxFilterInfo Hq2xGfxFilter::FilterInfo = GfxFilterInfo("Hq2x", "Hq2x", 2);

const GfxFilterInfo &Hq2xGfxFilter::GetInfo() const
{
    return FilterInfo;
}

bool Hq2xGfxFilter::Initialize(const int color_depth, String &err_str)
{
    if (color_depth < 32)
    {
        err_str = "Only supports 32-bit colour games";
        return false;
    }
    return AllegroGfxFilter::Initialize(color_depth, err_str);
}

Bitmap* Hq2xGfxFilter::InitVirtualScreen(Bitmap *screen, const Size src_size, const Rect dst_rect)
{
    Bitmap *virtual_screen = AllegroGfxFilter::InitVirtualScreen(screen, src_size, dst_rect);
    _hq2xScalingBuffer = BitmapHelper::CreateBitmap(src_size.Width * 2, src_size.Height * 2);

    InitLUTs();
    return virtual_screen;
}

Bitmap *Hq2xGfxFilter::ShutdownAndReturnRealScreen(Bitmap *currentScreen)
{
    Bitmap *real_screen = AllegroGfxFilter::ShutdownAndReturnRealScreen(currentScreen);
    delete _hq2xScalingBuffer;
    _hq2xScalingBuffer = NULL;
    return real_screen;
}

Bitmap *Hq2xGfxFilter::PreRenderPass(Bitmap *toRender)
{
    _hq2xScalingBuffer->Acquire();
    hq2x_32(toRender->GetDataForWriting(), _hq2xScalingBuffer->GetDataForWriting(),
        toRender->GetWidth(), toRender->GetHeight(), _hq2xScalingBuffer->GetLineLength());
    _hq2xScalingBuffer->Release();
    return _hq2xScalingBuffer;
}

} // namespace ALSW
} // namespace Engine
} // namespace AGS

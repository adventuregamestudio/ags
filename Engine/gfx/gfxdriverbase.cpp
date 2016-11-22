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

#include "util/wgt2allg.h"
#include "gfx/gfxfilter.h"
#include "gfx/gfxdriverbase.h"


namespace AGS
{
namespace Engine
{

GraphicsDriverBase::GraphicsDriverBase()
    : _global_x_offset(0)
    , _global_y_offset(0)
    , _loopTimer(NULL)
{
}

bool GraphicsDriverBase::IsModeSet() const
{
    return _mode.Width != 0 && _mode.Height != 0 && _mode.ColorDepth != 0;
}

bool GraphicsDriverBase::IsRenderFrameValid() const
{
    return !_srcRect.IsEmpty() && !_dstRect.IsEmpty();
}

DisplayMode GraphicsDriverBase::GetDisplayMode() const
{
    return _mode;
}

Rect GraphicsDriverBase::GetRenderDestination() const
{
    return _dstRect;
}

void GraphicsDriverBase::SetRenderOffset(int x, int y)
{
    _global_x_offset = x;
    _global_y_offset = y;
}

void GraphicsDriverBase::OnInit(const DisplayMode &mode, volatile int *loopTimer)
{
    _mode = mode;
    _loopTimer = loopTimer;
}

void GraphicsDriverBase::OnUnInit()
{
}

void GraphicsDriverBase::OnSetRenderFrame(const Size &src_size, const Rect &dst_rect)
{
    _srcRect = RectWH(0, 0, src_size.Width, src_size.Height);
    _dstRect = dst_rect;
    IGfxFilter *filter = GetGraphicsFilter();
    if (filter)
        _filterRect = filter->SetTranslation(src_size, dst_rect);
    else
        _filterRect = Rect();
    _scaling.Init(src_size, dst_rect);
}

void GraphicsDriverBase::OnSetFilter()
{
    _filterRect = GetGraphicsFilter()->SetTranslation(Size(_srcRect.GetSize()), _dstRect);
}

} // namespace Engine
} // namespace AGS

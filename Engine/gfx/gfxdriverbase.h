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
//
// Implementation base for graphics driver
//
//=============================================================================
#ifndef __AGS_EE_GFX__GFXDRIVERBASE_H
#define __AGS_EE_GFX__GFXDRIVERBASE_H

#include "gfx/graphicsdriver.h"
#include "util/scaling.h"

namespace AGS
{
namespace Engine
{

class GraphicsDriverBase : public IGraphicsDriver
{
public:
    GraphicsDriverBase();

    virtual DisplayMode GetDisplayMode() const;
    virtual Rect        GetRenderDestination() const;
    virtual void        SetRenderOffset(int x, int y);

protected:
    void _Init(const DisplayMode &mode, const Size src_size, const Rect dst_rect, volatile int *loopTimer);
    void _UnInit();

    DisplayMode         _mode;          // display mode settings
    Rect                _srcRect;       // rendering source rect
    Rect                _dstRect;       // rendering destination rect
    Rect                _filterRect;    // filter scaling destination rect (before final scaling)
    PlaneScaling        _scaling;
    int                 _global_x_offset;
    int                 _global_y_offset;
    volatile int *      _loopTimer;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GFXDRIVERBASE_H

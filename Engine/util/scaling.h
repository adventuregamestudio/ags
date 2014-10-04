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
// Helper struct for scaling coordinates
//
//=============================================================================
#ifndef __AGS_EE_UTIL__SCALING_H
#define __AGS_EE_UTIL__SCALING_H

#include "core/types.h"
#include "util/geometry.h"

namespace AGS
{
namespace Engine
{

class AxisScaling
{
public:
    AxisScaling()
        : _scale(kUnit)
        , _unscale(kUnit)
        , _offset(0)
    {
    }

    void Init(const uint32_t src_length, const uint32_t dst_length, const uint32_t offset = 0)
    {
        _scale = kUnit;
        _unscale = kUnit;
        _offset = offset;

        if (src_length != 0)
        {
            uint32_t scale = (dst_length << kShift) / src_length;
            if (scale != 0)
            {
                _scale = scale;
                _unscale = scale;
                uint32_t scaled_val = ScaleDistance(src_length);
                if (scaled_val < dst_length)
                    _scale++;
            }
        }
    }

    inline int32_t ScalePt(int32_t x) const
    {
        return ((x * _scale) >> kShift) + _offset;
    }
    inline int32_t ScaleDistance(int32_t x) const
    {
        return ((x * _scale) >> kShift);
    }
    inline int32_t UnScalePt(int32_t x) const
    {
        return ((x - _offset) << kShift) / _unscale;
    }
    inline int32_t UnScaleDistance(int32_t x) const
    {
        return (x << kShift) / _unscale;
    }

private:
    uint32_t _scale;
    uint32_t _unscale;
    uint32_t _offset;
};

struct PlaneScaling
{
    AxisScaling X;
    AxisScaling Y;

    void Init(const Size src_size, const Rect dst_rect)
    {
        X.Init(src_size.Width, dst_rect.GetWidth(), dst_rect.Left);
        Y.Init(src_size.Height, dst_rect.GetHeight(), dst_rect.Top);
    }

    inline Point Scale(const Point p) const
    {
        return Point(X.ScalePt(p.X), Y.ScalePt(p.Y));
    }

    inline Rect ScaleRange(const Rect r) const
    {
        return RectWH(X.ScalePt(r.Left), Y.ScalePt(r.Top), X.ScaleDistance(r.GetWidth()), Y.ScaleDistance(r.GetHeight()));
    }

    inline Point UnScale(const Point p) const
    {
        return Point(X.UnScalePt(p.X), Y.UnScalePt(p.Y));
    }

    inline Rect UnScaleRange(const Rect r) const
    {
        return RectWH(X.UnScalePt(r.Left), Y.UnScalePt(r.Top), X.UnScaleDistance(r.GetWidth()), Y.UnScaleDistance(r.GetHeight()));
    }
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__SCALING_H

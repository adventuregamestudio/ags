//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "util/geometry.h"
#include <algorithm>
#include <cmath>

//namespace AGS
//{
//namespace Common
//{

bool AreRectsIntersecting(const Rect &r1, const Rect &r2)
{ // NOTE: remember that in AGS Y axis is pointed downwards (top < bottom)
    return r1.Left <= r2.Right && r1.Right >= r2.Left &&
        r1.Top <= r2.Bottom && r1.Bottom >= r2.Top;
}

bool IsRectInsideRect(const Rect &place, const Rect &item)
{
    return item.Left >= place.Left && item.Right <= place.Right &&
        item.Top >= place.Top && item.Bottom <= place.Bottom;
}

float DistanceBetween(const Rect &r1, const Rect &r2)
{
    // https://gamedev.stackexchange.com/a/154040
    const Rect rect_outer(
        std::min(r1.Left, r2.Left),
        std::min(r1.Top, r2.Top),
        std::max(r1.Right, r2.Right),
        std::max(r1.Bottom, r2.Bottom)
    );
    const int inner_width = std::max(0, rect_outer.GetWidth() - r1.GetWidth() - r2.GetWidth());
    const int inner_height = std::max(0, rect_outer.GetHeight() - r1.GetHeight() - r2.GetHeight());
    return static_cast<float>(std::sqrt((inner_width * inner_width) + (inner_height * inner_height)));
}

Size ProportionalStretch(int dest_w, int dest_h, int item_w, int item_h)
{
    int width = item_w ? dest_w : 0;
    int height = item_w ? (dest_w * item_h / item_w) : 0;
    if (height > dest_h)
    {
        width  = item_h ? (dest_h * item_w / item_h) : 0;
        height = dest_h;
    }
    return Size(width, height);
}

Size ProportionalStretch(const Size &dest, const Size &item)
{
    return ProportionalStretch(dest.Width, dest.Height, item.Width, item.Height);
}

int AlignInHRange(int frame_x1, int frame_x2, int item_offx, int item_width, FrameAlignment align)
{
    if (align & kMAlignRight)
        return item_offx + frame_x2 - item_width + 1;
    else if (align & kMAlignHCenter)
        return item_offx + frame_x1 + ((frame_x2 - frame_x1 + 1) / 2) - (item_width / 2);
    return item_offx + frame_x1; // kAlignLeft is default
}

int AlignInVRange(int frame_y1, int frame_y2, int item_offy, int item_height, FrameAlignment align)
{
    if (align & kMAlignBottom)
        return item_offy + frame_y2 - item_height + 1;
    else if (align & kMAlignVCenter)
        return item_offy + frame_y1 + ((frame_y2 - frame_y1 + 1) / 2) - (item_height / 2);
    return item_offy + frame_y1; // kAlignTop is default
}

Rect AlignInRect(const Rect &frame, const Rect &item, FrameAlignment align)
{
    const int x = AlignInHRange(frame.Left, frame.Right, item.Left, item.GetWidth(), align);
    const int y = AlignInVRange(frame.Top, frame.Bottom, item.Top, item.GetHeight(), align);

    Rect dst_item = item;
    dst_item.MoveTo(Point(x, y));
    return dst_item;
}

Rect OffsetRect(const Rect &r, const Point off)
{
    return Rect(r.Left + off.X, r.Top + off.Y, r.Right + off.X, r.Bottom + off.Y);
}

Rect CenterInRect(const Rect &place, const Rect &item)
{
    return RectWH((place.GetWidth() >> 1) - (item.GetWidth() >> 1),
        (place.GetHeight() >> 1) - (item.GetHeight() >> 1),
        item.GetWidth(), item.GetHeight());
}

Rect ClampToRect(const Rect &place, const Rect &item)
{
    return Rect(
        AGSMath::Clamp(item.Left, place.Left, place.Right),
        AGSMath::Clamp(item.Top, place.Top, place.Bottom),
        AGSMath::Clamp(item.Right, place.Left, place.Right),
        AGSMath::Clamp(item.Bottom, place.Top, place.Bottom)
    );
}

Rect PlaceInRect(const Rect &place, const Rect &item, const RectPlacement &placement)
{
    switch (placement)
    {
    case kPlaceCenter:
        return CenterInRect(place, item);
    case kPlaceStretch:
        return place;
    case kPlaceStretchProportional:
        return CenterInRect(place,
            RectWH(ProportionalStretch(place.GetWidth(), place.GetHeight(), item.GetWidth(), item.GetHeight())));
    default:
        return RectWH(place.Left + item.Left, place.Top + item.Top, item.GetWidth(), item.GetHeight());
    }
}

Rect SumRects(const Rect &r1, const Rect &r2)
{ // NOTE: remember that in AGS Y axis is pointed downwards (top < bottom)
    return Rect(std::min(r1.Left, r2.Left), std::min(r1.Top, r2.Top),
        std::max(r1.Right, r2.Right), std::max(r1.Bottom, r2.Bottom));
}

Rect IntersectRects(const Rect &r1, const Rect &r2)
{ // NOTE: the result may be empty (negative) rect if there's no intersection
    return Rect(std::max(r1.Left, r2.Left), std::max(r1.Top, r2.Top),
        std::min(r1.Right, r2.Right), std::min(r1.Bottom, r2.Bottom));
}

Size RotateSize(Size sz, int degrees)
{
    // 1 degree = 181 degrees in terms of x/y size, so % 180
    int fixangle = degrees % 180;
    // and 0..90 is the same as 180..90
    if (fixangle > 90)
        fixangle = 180 - fixangle;
    // useAngle is now between 0 and 90 (otherwise the sin/cos stuff doesn't work)
    const double rads = AGSMath::DegreesToRadians(fixangle);
    const double sinv = sin(rads);
    const double cosv = cos(rads);
    const int width = (int)(cosv * (double)sz.Width + sinv * (double)sz.Height);
    const int height = (int)(sinv * (double)sz.Width + cosv * (double)sz.Height);
    return Size(width, height);
}

//} // namespace Common
//} // namespace AGS

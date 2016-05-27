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
#include "util/geometry.h"

//namespace AGS
//{
//namespace Common
//{

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

//} // namespace Common
//} // namespace AGS

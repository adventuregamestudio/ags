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

Rect OffsetRect(const Rect &r, const Point off)
{
    return Rect(r.Left + off.X, r.Top + off.Y, r.Right + off.X, r.Bottom + off.Y);
}

Rect PlaceInRect(const Rect &place, const Rect &item, const RectPlacement &placement)
{
    switch (placement)
    {
    case kPlaceCenter:
        return RectWH((place.GetWidth() >> 1) - (item.GetWidth() >> 1),
                      (place.GetHeight() >> 1) - (item.GetHeight() >> 1),
                      item.GetWidth(), item.GetHeight());
    case kPlaceStretch:
        return place;
    case kPlaceStretchProportional:
        {
            const int place_w = place.GetWidth();
            const int place_h = place.GetHeight();
            const int item_w  = item.GetWidth();
            const int item_h  = item.GetHeight();

            int width = item_w ? place_w : 0;
            int height = item_w ? (place_w * item_h / item_w) : 0;
            if (height > place_h)
            {
                width  = item_h ? (place_h * item_w / item_h) : 0;
                height = place_h;
            }
            return RectWH((place_w - width) >> 1, (place_h - height) >> 1, width, height);
        }
    default:
        return RectWH(place.Left + item.Left, place.Top + item.Top, item.GetWidth(), item.GetHeight());
    }
}

//} // namespace Common
//} // namespace AGS

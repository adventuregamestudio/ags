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

namespace AGS
{
namespace Common
{

namespace Math
{

void AlignInRect(const Rect &place, Rect &item, Alignment &alignment)
{
    if (alignment & kAlign_Left)
    {
        item.MoveToX(place.Left);
    }
    else if (alignment & kAlign_Right)
    {
        item.MoveToX(place.Right - item.GetWidth());
    }
    else if (alignment & kAlign_HCenter)
    {
        item.MoveToX((place.GetWidth() >> 1) - (item.GetWidth() >> 1));
    }

    if (alignment & kAlign_Top)
    {
        item.MoveToY(place.Top);
    }
    else if (alignment & kAlign_Bottom)
    {
        item.MoveToY(place.Bottom - item.GetHeight());
    }
    else if (alignment & kAlign_VCenter)
    {
        item.MoveToY((place.GetHeight() >> 1) - (item.GetHeight() >> 1));
    }
}

} // namespace Math

} // namespace Common
} // namespace AGS

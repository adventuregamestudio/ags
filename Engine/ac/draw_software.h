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
// Software drawing component. Optimizes drawing for software renderer using
// dirty rectangles technique.
//
//=============================================================================
#ifndef __AGS_EE_AC__DRAWSOFTWARE_H
#define __AGS_EE_AC__DRAWSOFTWARE_H

#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "util/geometry.h"

void init_invalid_regions(const Size &surf_size);
void invalidate_all_rects();
void invalidate_rect(int x1, int y1, int x2, int y2, const Rect &viewport);
void update_invalid_region_and_reset(AGS::Common::Bitmap *ds, int x, int y, AGS::Common::Bitmap *src, const Rect &viewport);

#endif // __AGS_EE_AC__DRAWSOFTWARE_H

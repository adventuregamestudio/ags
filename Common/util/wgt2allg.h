//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Few graphic utility functions, remains of a bigger deprecated api.
// FIXME: either get rid of these or tidy and reorganize them.
//
//=============================================================================
#ifndef __WGT4_H
#define __WGT4_H

#include <allegro.h> // RGB

namespace AGS { namespace Common { class Bitmap; }}
using namespace AGS; // FIXME later

//=============================================================================
    
    extern Common::Bitmap *wnewblock(Common::Bitmap *src, int x1, int y1, int x2, int y2);

    extern void wputblock(Common::Bitmap *ds, int xx, int yy, Common::Bitmap *bll, int xray);
	// CHECKME: temporary solution for plugin system
	extern void wputblock_raw(Common::Bitmap *ds, int xx, int yy, BITMAP *bll, int xray);


#endif // __WGT4_H

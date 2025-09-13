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
//
// Few graphic utility functions, remains of a bigger deprecated api.
//
//=============================================================================
#ifndef __WGT4_H
#define __WGT4_H

#include <allegro.h> // RGB

namespace AGS { namespace Common { class Bitmap; }}
using namespace AGS; // FIXME later

//=============================================================================

    extern int my_setcolor(int color, int color_depth, bool fix_alpha);
    
    extern void wsetrgb(int coll, int r, int g, int b, RGB * pall);
    extern void wcolrotate(unsigned char start, unsigned char finish, int dir, RGB * pall);

    extern Common::Bitmap *wnewblock(Common::Bitmap *src, int x1, int y1, int x2, int y2);

    extern void wputblock(Common::Bitmap *ds, int xx, int yy, Common::Bitmap *bll, int xray);
	// CHECKME: temporary solution for plugin system
	extern void wputblock_raw(Common::Bitmap *ds, int xx, int yy, BITMAP *bll, int xray);
    extern const int col_lookups[32];

    // TODO: these are used only in the Editor's agsnative.cpp
    extern void wremap(const RGB * pal1, Common::Bitmap *picc, const RGB * pal2, bool keep_transparent = true);
    extern void wremapall(const RGB * pal1, Common::Bitmap *picc, const RGB * pal2);


#endif // __WGT4_H

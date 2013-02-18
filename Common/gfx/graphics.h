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
// 
//
//=============================================================================
#ifndef __AGS_CN_GFX__GRAPHICS_H
#define __AGS_CN_GFX__GRAPHICS_H

#include "gfx/bitmap.h"

#ifndef NULL
#define NULL 0
#endif

namespace AGS { namespace Common { struct Graphics; } }
using namespace AGS;

extern void __my_setcolor(int *ctset, int newcol, int wantColDep);

namespace AGS
{
namespace Common
{

struct Graphics
{
    Bitmap *Bmp;
    color_t DrawColor;
    color_t TextColor;

    Graphics()
        : Bmp(NULL)
        , DrawColor(0)
        , TextColor(0)
    {
    }

    Graphics(Bitmap *bmp)
        : Bmp(NULL)
        , DrawColor(0)
        , TextColor(0)
    {
        Bmp = bmp;
    }

    void SetBitmap(Bitmap *bmp)
    {
        Bmp = bmp;
    }

    void SetColor(color_t col)
    {
        if (Bmp)
        {
            __my_setcolor(&DrawColor, col, Bmp->GetColorDepth());
        }
        else
        {
            DrawColor = col;
        }
    }

    void SetTextColor(color_t col)
    {
        if (Bmp)
        {
            __my_setcolor(&TextColor, col, Bmp->GetColorDepth());
        }
        else
        {
            TextColor = col;
        }
    }

    void SetColorExact(color_t col)
    {
        DrawColor = col;
    }

    void SetTextColorExact(color_t col)
    {
        TextColor = col;
    }
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__GRAPHICS_H

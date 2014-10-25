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

#ifndef __AGS_EE_GFX__GFXDEFINES_H
#define __AGS_EE_GFX__GFXDEFINES_H

#include "core/types.h"

namespace AGS
{
namespace Engine
{

enum GlobalFlipType
{
    kFlip_None,
    kFlip_Horizontal,
    kFlip_Vertical,
    kFlip_Both
};

struct GraphicResolution
{
    int32_t Width;
    int32_t Height;
    int32_t ColorDepth;

    GraphicResolution()
        : Width(0)
        , Height(0)
        , ColorDepth(0)
    {
    }

    GraphicResolution(int32_t width, int32_t height, int32_t color_depth)
    {
        Width = width;
        Height = height;
        ColorDepth = color_depth;
    }
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GFXDEFINES_H

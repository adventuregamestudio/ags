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
// Supported graphics mode interface
//
//=============================================================================
#ifndef __AGS_EE_GFX__GFXMODELIST_H
#define __AGS_EE_GFX__GFXMODELIST_H

#include "core/types.h"

namespace AGS
{
namespace Engine
{

struct DisplayResolution
{
    int32_t Width;
    int32_t Height;
    int32_t ColorDepth;

    DisplayResolution()
        : Width(0)
        , Height(0)
        , ColorDepth(0)
    {
    }

    DisplayResolution(int32_t width, int32_t height, int32_t color_depth)
    {
        Width = width;
        Height = height;
        ColorDepth = color_depth;
    }
};

class IGfxModeList
{
public:
    virtual int  GetModeCount() = 0;
    virtual bool GetMode(int index, DisplayResolution &resolution) = 0;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GFXMODELIST_H

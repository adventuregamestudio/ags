//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_GFX__GFXDEFINES_H
#define __AGS_EE_GFX__GFXDEFINES_H

#include "core/types.h"
#include "gfx/gfx_def.h"

namespace AGS
{
namespace Engine
{

enum WindowMode
{
    kWnd_Windowed,      // regular resizable window with a border and a caption
    kWnd_Fullscreen,    // real (aka exclusive) fullscreen mode
    kWnd_FullDesktop    // borderless window filling whole desktop
};

// DisplayMode struct provides extended description of display mode
struct DisplayMode : public AGS::Common::GraphicResolution
{
    int32_t RefreshRate = 0;
    bool    Vsync = false;
    WindowMode Mode = kWnd_Windowed;

    // Tells if this is logically a normal windowed mode
    inline bool IsWindowed() const { return Mode == kWnd_Windowed; }
    // Tells if this mode defines a real fullscreen, which would require gfx driver to support it
    inline bool IsRealFullscreen() const { return Mode == kWnd_Fullscreen; }

    DisplayMode() = default;
    DisplayMode(const GraphicResolution &res, WindowMode mode = kWnd_Windowed, int32_t refresh = 0, bool vsync = false)
        : GraphicResolution(res)
        , RefreshRate(refresh)
        , Vsync(vsync)
        , Mode(mode)
    {}
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GFXDEFINES_H

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
#ifndef __AGS_EE_MAIN__GRAPHICSMODE_H
#define __AGS_EE_MAIN__GRAPHICSMODE_H

#include "gfx/gfxdefines.h"
#include "util/scaling.h"
#include "util/string.h"

using AGS::Common::String;

Size get_desktop_size();
String make_scaling_factor_string(uint32_t scaling);

namespace AGS { namespace Engine { class IGfxModeList; }}
bool find_nearest_supported_mode(const AGS::Engine::IGfxModeList &modes, Size &wanted_size, int *mode_index,
                                 const int color_depth, const Size *ratio_reference = NULL, const Size *upper_bound = NULL);

namespace AGS { namespace Engine { class IGfxFilter; } }
extern AGS::Engine::IGfxFilter *filter;

// The actual game screen resolution
extern AGS::Engine::GraphicResolution ScreenResolution;
// The game-to-screen transformation
extern AGS::Engine::PlaneScaling GameScaling;


// Filter configuration
struct GfxFilterSetup
{
    String ID;          // internal filter ID
    String UserRequest; // filter name, requested by user
};

enum FrameScaleDefinition
{
    kFrame_IntScale,        // explicit integer scaling x/y factors
    kFrame_MaxRound,        // calculate max round uniform scaling factor
    kFrame_MaxStretch,      // resize to maximal possible inside the display box
    kFrame_MaxProportional, // same as stretch, but keep game's aspect ratio
    kNumFrameScaleDef
};

// Game frame configuration
struct GameFrameSetup
{
    FrameScaleDefinition ScaleDef;    // a method used to determine game frame scaling
    int                  ScaleFactor; // explicit scale factor
};

enum ScreenSizeDefinition
{
    kScreenDef_Explicit,        // define by width & height
    kScreenDef_ByGameScaling,   // define by game scale factor
    kScreenDef_MaxDisplay,      // set to maximal supported (desktop/device screen size)
    kNumScreenDef
};

// General display configuration
struct ScreenSetup
{
    String               DriverID;      // graphics driver ID
    ScreenSizeDefinition SizeDef;       // a method used to determine screen size
    ::Size               Size;          // explicitly defined screen metrics
    bool                 MatchDeviceRatio; // choose resolution matching device aspect ratio

    int                  RefreshRate;   // gfx mode refresh rate
    bool                 VSync;         // vertical sync
    bool                 Windowed;      // is mode windowed

    GfxFilterSetup       Filter;        // graphics filter definition
    GameFrameSetup       GameFrame;     // definition of the game frame's position on screen
};

struct ColorDepthOption
{
    int32_t Prime;
    int32_t Alternate;
};

// Makes an attempt to deduce and set a new gfx mode from user config
bool graphics_mode_init(const ScreenSetup &setup, const ColorDepthOption &color_depths);
// Releases current graphic mode and shuts down renderer
void graphics_mode_shutdown();

#endif // __AGS_EE_MAIN__GRAPHICSMODE_H

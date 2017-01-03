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
bool find_nearest_supported_mode(const AGS::Engine::IGfxModeList &modes, const Size &wanted_size,
                                 const int color_depth, const Size *ratio_reference, const Size *upper_bound,
                                 AGS::Engine::DisplayMode &dm, int *mode_index = NULL);


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

    GameFrameSetup();
    bool IsValid() const;
};

enum ScreenSizeDefinition
{
    kScreenDef_Explicit,        // define by width & height
    kScreenDef_ByGameScaling,   // define by game scale factor
    kScreenDef_MaxDisplay,      // set to maximal supported (desktop/device screen size)
    kNumScreenDef
};

// Display mode configuration
struct DisplayModeSetup
{
    ScreenSizeDefinition SizeDef;       // a method used to determine screen size
    ::Size               Size;          // explicitly defined screen metrics
    bool                 MatchDeviceRatio; // choose resolution matching device aspect ratio

    int                  RefreshRate;   // gfx mode refresh rate
    bool                 VSync;         // vertical sync
    bool                 Windowed;      // is mode windowed

    DisplayModeSetup();
};

// General display configuration
struct ScreenSetup
{
    String               DriverID;      // graphics driver ID
    DisplayModeSetup     DisplayMode;   // definition of the display mode

    GfxFilterSetup       Filter;        // graphics filter definition
    GameFrameSetup       GameFrame;     // definition of the game frame's position on screen

    bool                 RenderAtScreenRes; // render sprites at screen resolution, as opposed to native one

    ScreenSetup();
};

// Display mode color depth variants allowed to be used
struct ColorDepthOption
{
    int32_t Prime;
    int32_t Alternate;

    ColorDepthOption() : Prime(0), Alternate(0) {}
    ColorDepthOption(int32_t prime, int32_t alt = 0) : Prime(prime), Alternate(alt > 0 ? alt : prime) {}
};

// Initializes any possible gfx mode, using user config as a recommendation;
// may try all available renderers and modes before succeeding (or failing)
bool graphics_mode_init_any(const Size game_size, const ScreenSetup &setup, const ColorDepthOption &color_depths);
// Fill in setup structs with default settings for the given mode (windowed or fullscreen)
void graphics_mode_get_defaults(bool windowed, DisplayModeSetup &dm_setup, GameFrameSetup &frame_setup);
// Creates graphics driver of given id
bool graphics_mode_create_renderer(const String &driver_id);
// Try to find and initialize compatible display mode as close to given setup
bool graphics_mode_set_dm_any(const Size &game_size, const DisplayModeSetup &dm_setup,
                              const ColorDepthOption &color_depths, const GameFrameSetup &frame_setup);
// Set the display mode with given parameters
bool graphics_mode_set_dm(const AGS::Engine::DisplayMode &dm);
// Set the native image size
bool graphics_mode_set_native_size(const Size &native_size);
// Get current render frame setup
GameFrameSetup graphics_mode_get_render_frame();
// Set the render frame position inside the window
bool graphics_mode_set_render_frame(const GameFrameSetup &frame_setup);
// Set requested graphics filter, or default filter if the requested one failed
bool graphics_mode_set_filter_any(const GfxFilterSetup &setup);
// Set the scaling filter with given ID
bool graphics_mode_set_filter(const String &filter_id);
// Releases current graphic mode and shuts down renderer
void graphics_mode_shutdown();

#endif // __AGS_EE_MAIN__GRAPHICSMODE_H

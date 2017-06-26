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


// The game-to-screen transformation
// TODO: this is only required for low-level mouse processing;
// when possible, move to mouse "manager" object, and assign at gfxmode init
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
};

// Display mode color depth variants suggested for the use
struct ColorDepthOption
{
    int     Bits;   // color depth value in bits
    bool    Forced; // whether the depth should be forced, or driver's recommendation used

    ColorDepthOption() : Bits(0), Forced(false) {}
    ColorDepthOption(int bits, bool forced = false) : Bits(bits), Forced(forced) {}
};

// Initializes any possible gfx mode, using user config as a recommendation;
// may try all available renderers and modes before succeeding (or failing)
bool graphics_mode_init_any(const Size game_size, const ScreenSetup &setup, const ColorDepthOption &color_depth);
// Fill in setup structs with default settings for the given mode (windowed or fullscreen)
void graphics_mode_get_defaults(bool windowed, DisplayModeSetup &dm_setup, GameFrameSetup &frame_setup);
// Get frame setup, adjusted to be more suitable for the given mode kind
GameFrameSetup convert_frame_setup(const GameFrameSetup &frame_setup, bool windowed);
// Return last saved display mode of the given kind
AGS::Engine::DisplayMode graphics_mode_get_last_mode(bool windowed);
// Creates graphics driver of given id
bool graphics_mode_create_renderer(const String &driver_id);
// Try to find and initialize compatible display mode as close to given setup as possible
bool graphics_mode_set_dm_any(const Size &game_size, const DisplayModeSetup &dm_setup,
                              const ColorDepthOption &color_depth, const GameFrameSetup &frame_setup);
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

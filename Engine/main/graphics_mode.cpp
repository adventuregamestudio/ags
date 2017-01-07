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
// Graphics initialization
//

#include <algorithm>
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "gfx/ali3dexception.h"
#include "gfx/bitmap.h"
#include "gfx/gfxdriverfactory.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "main/config.h"
#include "main/graphics_mode.h"
#include "main/main_allegro.h"
#include "platform/base/agsplatformdriver.h"
#include "util/scaling.h"


using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int proper_exit;
extern AGSPlatformDriver *platform;
extern IGraphicsDriver *gfxDriver;
extern volatile int timerloop;


IGfxDriverFactory *GfxFactory = NULL;
IGfxFilter *filter;

struct GameSizeDef
{
    Size Game; // actual game size
    Size Box;  // bounding box
};


GraphicResolution ScreenResolution;
PlaneScaling      GameScaling;


Size get_desktop_size()
{
    Size sz;
    get_desktop_resolution(&sz.Width, &sz.Height);
    return sz;
}

Size get_max_display_size(bool windowed)
{
    Size device_size = get_desktop_size();
    if (windowed)
        platform->ValidateWindowSize(device_size.Width, device_size.Height, false);
    return device_size;
}

bool initialize_graphics_filter(const String filter_id, const int color_depth)
{
    filter = GfxFactory->SetFilter(filter_id);
    if (!filter)
    {
        Out::FPrint("Unable to create filter: %s", filter_id.GetCStr());
        return false;
    }

    String filter_error;
    if (!filter->Initialize(color_depth, filter_error))
    {
        Out::FPrint("Unable to initialize the graphics filter. Error: %s.", filter_error.GetCStr());
        return false;
    }
    return true;
}

bool pre_create_gfx_driver(const String &gfx_driver_id)
{
    GfxFactory = GetGfxDriverFactory(gfx_driver_id);
    if (!GfxFactory)
    {
        Out::FPrint("Failed to initialize %s graphics factory: %s", gfx_driver_id.GetCStr(), get_allegro_error());
        return false;
    }
    Out::FPrint("Using graphics factory: %s", gfx_driver_id.GetCStr());
    gfxDriver = GfxFactory->GetDriver();
    if (!gfxDriver)
    {
        Out::FPrint("Failed to create graphics driver. %s", get_allegro_error());
        return false;
    }
    Out::FPrint("Created graphics driver: %s", gfxDriver->GetDriverName());
    return true;
}

bool engine_set_gfx_filter(const GfxFilterSetup &setup, const int color_depth)
{
    Out::FPrint("Requested gfx filter: %s", setup.UserRequest.GetCStr());
    if (!initialize_graphics_filter(setup.ID, color_depth))
    {
        String def_filter = GfxFactory->GetDefaultFilterID();
        if (def_filter.CompareNoCase(setup.ID) != 0)
        {
            Out::FPrint("Failed to apply gfx filter: %s; will try to use factory default filter '%s' instead",
                setup.UserRequest.GetCStr(), def_filter.GetCStr());
            if (!initialize_graphics_filter(def_filter, color_depth))
                return false;
        }
    }
    Out::FPrint("Using gfx filter: %s", GfxFactory->GetDriver()->GetGraphicsFilter()->GetInfo().Id.GetCStr());
    return true;
}

bool find_nearest_supported_mode(Size &wanted_size, const int color_depth, const Size *ratio_reference = NULL, const Size *upper_bound = NULL)
{
    IGfxModeList *modes = gfxDriver->GetSupportedModeList(color_depth);
    if (!modes)
    {
        Out::FPrint("Couldn't get a list of supported resolutions");
        return false;
    }
    bool result = find_nearest_supported_mode(*modes, wanted_size, NULL, color_depth, ratio_reference, upper_bound);
    delete modes;
    return result;
}

bool find_nearest_supported_mode(const IGfxModeList &modes, Size &wanted_size, int *mode_index, const int color_depth,
                                 const Size *ratio_reference, const Size *upper_bound)
{
    uint32_t wanted_ratio = 0;
    if (ratio_reference && !ratio_reference->IsNull())
    {
        wanted_ratio = (ratio_reference->Height << kShift) / ratio_reference->Width;
    }
    
    int nearest_width = 0;
    int nearest_height = 0;
    int nearest_width_diff = 0;
    int nearest_height_diff = 0;
    int nearest_mode_index = -1;
    int mode_count = modes.GetModeCount();
    DisplayMode mode;
    for (int i = 0; i < mode_count; ++i)
    {
        if (!modes.GetMode(i, mode))
        {
            continue;
        }
        if (mode.ColorDepth != color_depth)
        {
            continue;
        }
        if (wanted_ratio > 0)
        {
            uint32_t mode_ratio = (mode.Height << kShift) / mode.Width;
            if (mode_ratio != wanted_ratio)
            {
                continue;
            }
        }
        if (upper_bound && (mode.Width > upper_bound->Width || mode.Height > upper_bound->Height))
            continue;
        if (mode.Width == wanted_size.Width && mode.Height == wanted_size.Height)
        {
            nearest_width = mode.Width;
            nearest_height = mode.Height;
            nearest_mode_index = i;
            break;
        }
      
        int diff_w = abs(wanted_size.Width - mode.Width);
        int diff_h = abs(wanted_size.Height - mode.Height);
        bool same_diff_w_higher = (diff_w == nearest_width_diff && nearest_width < wanted_size.Width);
        bool same_diff_h_higher = (diff_h == nearest_height_diff && nearest_height < wanted_size.Height);

        if (nearest_width == 0 ||
            (diff_w < nearest_width_diff || same_diff_w_higher) && diff_h <= nearest_height_diff ||
            (diff_h < nearest_height_diff || same_diff_h_higher) && diff_w <= nearest_width_diff)
        {
            nearest_width = mode.Width;
            nearest_width_diff = diff_w;
            nearest_height = mode.Height;
            nearest_height_diff = diff_h;
            nearest_mode_index = i;
        }
    }

    if (nearest_width > 0 && nearest_height > 0)
    {
        wanted_size.Width = nearest_width;
        wanted_size.Height = nearest_height;
        if (mode_index)
            *mode_index = nearest_mode_index;
        return true;
    }
    return false;
}

bool create_gfx_driver(const String &gfx_driver_id)
{
    if (!pre_create_gfx_driver(gfx_driver_id))
        return false;

    gfxDriver->SetCallbackOnInit(GfxDriverOnInitCallback);
    gfxDriver->SetTintMethod(TintReColourise);
    return true;
}

bool init_gfx_mode(const GameSizeDef &game_size, const Size &screen_size, const GameSizeDef &frame_size,
                   const ScreenSetup setup, const int color_depth, const bool windowed)
{
    int width = screen_size.Width;
    int height = screen_size.Height;

    const char *frame_placement[kNumRectPlacement] = { "offset", "center", "stretch", "proportional" };
    Out::FPrint("Attempt to switch gfx mode to %d x %d (%d-bit) %s; game frame: %d x %d",
        width, height, color_depth, windowed ? "windowed" : "fullscreen",
        frame_size.Box.Width, frame_size.Box.Height);

    // CHECKME: why 50? Do we need this if we are not using allegro software driver?
    if (setup.RefreshRate >= 50)
        request_refresh_rate(setup.RefreshRate);

    const DisplayMode mode = DisplayMode(GraphicResolution(width, height, color_depth), windowed, setup.RefreshRate, setup.VSync);
    const Size src_size = game_size.Game;
    const Rect screen_rect = RectWH(0, 0, screen_size.Width, screen_size.Height);
    Rect dst_rect = CenterInRect(screen_rect, RectWH(frame_size.Box));
    // If requested bounding box is larger than game frame, then position the frame centered inside bounding box
    if (frame_size.Box != frame_size.Game)
        dst_rect = PlaceInRect(dst_rect, RectWH(0, 0, frame_size.Game.Width, frame_size.Game.Height), kPlaceCenter);

    set_color_depth(color_depth); // this tells Allegro default bitmap color depth
    if (gfxDriver->Init(mode, src_size, dst_rect, &timerloop))
        return true;
    Out::FPrint("Failed. %s", get_allegro_error());
    return false;
}

void set_game_frame_after_screen_size(const GameSizeDef &game_size, const Size screen_size,
                                      const GameFrameSetup &setup, GameSizeDef &frame_size)
{
    // Set game frame as native game resolution scaled by particular method
    Size new_box;

    if (setup.ScaleDef == kFrame_MaxStretch)
    {
        new_box = screen_size;
    }
    else if (setup.ScaleDef == kFrame_MaxProportional)
    {
        new_box = ProportionalStretch(screen_size, game_size.Box);
    }
    else
    {
        int scale;
        if (setup.ScaleDef == kFrame_MaxRound)
            scale = Math::Min((screen_size.Width / game_size.Box.Width) << kShift,
                              (screen_size.Height / game_size.Box.Height) << kShift);
        else
            scale = setup.ScaleFactor;

        // Ensure scaling factors are sane
        if (scale <= 0)
            scale = kUnit;

        new_box = Size((game_size.Box.Width * scale) >> kShift, (game_size.Box.Height * scale) >> kShift);
        // If the scaled game size appear larger than the screen,
        // use "proportional stretch" method instead
        if (new_box.ExceedsByAny(screen_size))
            new_box = ProportionalStretch(screen_size, game_size.Box);
    }

    frame_size.Game.Width = game_size.Game.Width * new_box.Width / game_size.Box.Width;
    frame_size.Game.Height = game_size.Game.Height * new_box.Height / game_size.Box.Height;
    frame_size.Box = new_box;
}

void precalc_render_screen(const GameSizeDef &game_size, const ScreenSetup &setup,
                           Size &screen_size, const int color_depth, const bool windowed)
{
    Size device_size = get_max_display_size(windowed);
    GameSizeDef frame_size;

    // Set requested screen (window) size, depending on screen definition option
    switch (setup.SizeDef)
    {
    case kScreenDef_Explicit:
        // Use resolution from user config
        screen_size = setup.Size;
        if (screen_size.IsNull())
        {
            // If the configuration did not define proper screen size,
            // use the scaled game size instead
            set_game_frame_after_screen_size(game_size, device_size, setup.GameFrame, frame_size);
            if (screen_size.Width <= 0)
                screen_size.Width = frame_size.Box.Width;
            if (screen_size.Height <= 0)
                screen_size.Height = frame_size.Box.Height;
        }
        break;
    case kScreenDef_ByGameScaling:
        // Use game frame (scaled game) size
        set_game_frame_after_screen_size(game_size, device_size, setup.GameFrame, frame_size);
        screen_size = frame_size.Box;
        break;
    case kScreenDef_MaxDisplay:
        // Set as big as current device size
        screen_size = device_size;
        break;
    }
}

bool setup_render_frame_and_init_gfx_mode(const GameSizeDef &game_size, const Size &screen_size,
                                          const ScreenSetup &setup, const int color_depth, const bool windowed)
{
    // Setup final render frame, depending on defined screen size, then init gfx mode
    GameSizeDef frame_size;
    set_game_frame_after_screen_size(game_size, screen_size, setup.GameFrame, frame_size);
    return init_gfx_mode(game_size, screen_size, frame_size, setup, color_depth, windowed);
}

bool try_init_gfx_mode(const GameSizeDef &game_size, const Size &screen_size,
                       const ScreenSetup &setup, const int color_depth, const bool windowed)
{
    // Find nearest compatible mode and init that
    Out::FPrint("Attempting to find nearest supported resolution for screen size %d x %d (%d-bit) %s",
        screen_size.Width, screen_size.Height, color_depth, windowed ? "windowed" : "fullscreen");
    Size device_size = get_max_display_size(windowed);
    Size fixed_screen_size = screen_size;

    // Windowed mode
    if (windowed)
    {
        // If windowed mode, make the resolution stay in the generally supported limits
        fixed_screen_size.Clamp(Size(1, 1), device_size);
    }
    // Fullscreen mode
    else
    {
        // If told to find mode with aspect ratio matching current desktop resolution, then first
        // try find matching one, and if failed then try any compatible one
        const bool match_device_ratio = setup.SizeDef != kScreenDef_Explicit && setup.MatchDeviceRatio;
        bool mode_found = false;
        if (match_device_ratio)
            mode_found = find_nearest_supported_mode(fixed_screen_size, color_depth, &device_size);
        if (!mode_found)
            mode_found = find_nearest_supported_mode(fixed_screen_size, color_depth);
        if (!mode_found)
        {
            Out::FPrint("Could not find compatible fullscreen mode");
            return false;
        }
    }

    bool result = setup_render_frame_and_init_gfx_mode(game_size, fixed_screen_size, setup, color_depth, windowed);
    if (!result && windowed)
    {
        // When initializing windowed mode we could start with any random window size;
        // if that did not work, try to find nearest supported mode, as with fullscreen mode,
        // except refering to max window size as an upper bound
        if (find_nearest_supported_mode(fixed_screen_size, color_depth, NULL, &device_size))
            result = setup_render_frame_and_init_gfx_mode(game_size, fixed_screen_size, setup, color_depth, true);
        else
            Out::FPrint("Could not find compatible windowed mode");
    }
    return result;
}

// Tries to init gfx mode with given parameters; makes two attempts with primary and secondary colour depths
bool try_init_gfx_mode(const GameSizeDef &game_size, const ScreenSetup &setup, const ColorDepthOption color_depths, const bool windowed)
{
    Size screen_size; // final screen size
    // When precalculating screen size we use box bounding size for reference, in case engine
    // requested larger screen space to place the game in
    precalc_render_screen(game_size, setup, screen_size, color_depths.Prime, windowed);
    bool result = try_init_gfx_mode(game_size, screen_size, setup, color_depths.Prime, windowed);
    if (!result && color_depths.Prime != color_depths.Alternate)
        result = try_init_gfx_mode(game_size, screen_size, setup, color_depths.Alternate, windowed);
    return result;
}

// Tries to init gfx mode either fullscreen or windowed; if failed tries to init alternate one
bool engine_init_graphics_mode(const GameSizeDef &game_size, const ScreenSetup &setup, const ColorDepthOption color_depths, const bool windowed)
{
    Out::FPrint("Switching to graphics mode");
    bool result = try_init_gfx_mode(game_size, setup, color_depths, windowed);
    // Try windowed mode if fullscreen failed, and vice versa
    if (!result && editor_debugging_enabled == 0)
    {
        result = try_init_gfx_mode(game_size, setup, color_depths, !windowed);
    }
    return result;
}

void log_out_driver_modes(const int color_depth)
{
    IGfxModeList *modes = gfxDriver->GetSupportedModeList(color_depth);
    if (!modes)
    {
        Out::FPrint("Couldn't get a list of supported resolutions for color depth = %d", color_depth);
        return;
    }
    const int mode_count = modes->GetModeCount();
    DisplayMode mode;
    String mode_str;
    for (int i = 0, in_str = 0; i < mode_count; ++i)
    {
        if (!modes->GetMode(i, mode) || mode.ColorDepth != color_depth)
            continue;
        mode_str.Append(String::FromFormat("%dx%d;", mode.Width, mode.Height));
        if (++in_str % 8 == 0)
            mode_str.Append("\n\t");
    }
    delete modes;

    String out_str = String::FromFormat("Supported gfx modes (%d-bit): ", color_depth);
    if (!mode_str.IsEmpty())
    {
        out_str.Append("\n\t");
        out_str.Append(mode_str);
    }
    else
        out_str.Append("none");
    Out::FPrint(out_str);
}

bool create_gfx_driver_and_init_mode(const String &gfx_driver_id, const GameSizeDef &game_size,
                                     const ScreenSetup &setup, const ColorDepthOption color_depths, const bool windowed)
{
    if (!create_gfx_driver(gfx_driver_id))
        return false;
    // Log out supported driver modes
    log_out_driver_modes(color_depths.Prime);
    if (color_depths.Prime != color_depths.Alternate)
        log_out_driver_modes(color_depths.Alternate);
    
    if (!engine_set_gfx_filter(setup.Filter, color_depths.Prime))
        return false;

    if (!engine_init_graphics_mode(game_size, setup, color_depths, windowed))
        return false;

    // init game scaling transformation
    if (platform->IsBackendResponsibleForMouseScaling())
        GameScaling = PlaneScaling(); // 1:1 scaling
    else
        GameScaling.Init(game_size.Game, gfxDriver->GetRenderDestination());
    return true;
}

void display_gfx_mode_error(const Size game_size, const GfxFilterSetup &setup, const int color_depth)
{
    proper_exit=1;
    platform->FinishedUsingGraphicsMode();

    String main_error;
    Size screen_size(ScreenResolution.Width, ScreenResolution.Height);
    if (screen_size.IsNull())
        main_error.Format("There was a problem finding appropriate graphics mode for game size %d x %d (%d-bit) and requested filter '%s'.",
            game_size.Width, game_size.Height, color_depth, setup.UserRequest.IsEmpty() ? "Undefined" : setup.UserRequest.GetCStr());
    else
        main_error.Format("There was a problem initializing graphics mode %d x %d (%d-bit) with game size %d x %d and filter '%s'.",
            screen_size.Width, screen_size.Height, color_depth, game_size.Width, game_size.Height, filter ? filter->GetInfo().Id.GetCStr() : "Undefined");

    platform->DisplayAlert("%s\n"
            "(Problem: '%s')\n"
            "Try to correct the problem, or seek help from the AGS homepage."
            "%s",
            main_error.GetCStr(), get_allegro_error(), platform->GetGraphicsTroubleshootingText());
}

bool graphics_mode_init(const ScreenSetup &setup, const ColorDepthOption &color_depths)
{
    // Log out display information
    Size device_size;
    if (get_desktop_resolution(&device_size.Width, &device_size.Height) == 0)
        Out::FPrint("Device display resolution: %d x %d", device_size.Width, device_size.Height);
    else
        Out::FPrint("Unable to obtain device resolution");

    const char *screen_sz_def_options[kNumScreenDef] = { "explicit", "scaling", "max" };
    const bool ignore_device_ratio = setup.Windowed || setup.SizeDef == kScreenDef_Explicit;
    const String scale_option = make_scaling_option(setup.GameFrame.ScaleDef, convert_fp_to_scaling(setup.GameFrame.ScaleFactor));
    Out::FPrint("Game settings: windowed = %s, screen def: %s, screen size: %d x %d, match device ratio: %s, game scale: %s",
        setup.Windowed ? "yes" : "no", screen_sz_def_options[setup.SizeDef], setup.Size.Width, setup.Size.Height,
        ignore_device_ratio ? "ignore" : (setup.MatchDeviceRatio ? "yes" : "no"), scale_option.GetCStr());

    // Game size is used when defining resolution base and proper scaling;
    // Box size specifies minimal wanted screen size for the game (unscaled),
    // it is supposed to be be equal or greater than game_size
    GameSizeDef game_size;
    game_size.Game = game.size;
    game_size.Box = game.size;

    // Prepare the list of available gfx factories, having the one requested by user at first place
    StringV ids;
    GetGfxDriverFactoryNames(ids);
    StringV::iterator it = std::find(ids.begin(), ids.end(), setup.DriverID);
    if (it != ids.end())
        std::rotate(ids.begin(), it, ids.end());
    else
        Out::FPrint("Requested graphics driver '%s' not found, will try existing drivers instead", setup.DriverID.GetCStr());

    // Try to create renderer and init gfx mode, choosing one factory at a time
    bool result = false;
    for (StringV::const_iterator it = ids.begin(); it != ids.end(); ++it)
    {
        result = create_gfx_driver_and_init_mode(*it, game_size, setup, color_depths, setup.Windowed);
        if (result)
            break;
        graphics_mode_shutdown();
    }
    // If all possibilities failed, display error message and quit
    if (!result)
    {
        display_gfx_mode_error(game_size.Box, setup.Filter, color_depths.Prime);
        return false;
    }

    // On success: log out new mode params and continue initialization
    DisplayMode dm   = gfxDriver->GetDisplayMode();
    ScreenResolution = dm;

    Rect dst_rect    = gfxDriver->GetRenderDestination();
    Rect filter_rect = filter->GetDestination();
    Out::FPrint("Succeeded. Using gfx mode %d x %d (%d-bit) %s\n\t"
                "filter dest (%d, %d, %d, %d : %d x %d), render dest (%d, %d, %d, %d : %d x %d)",
        dm.Width, dm.Height, dm.ColorDepth, dm.Windowed ? "windowed" : "fullscreen",
        filter_rect.Left, filter_rect.Top, filter_rect.Right, filter_rect.Bottom, filter_rect.GetWidth(), filter_rect.GetHeight(),
        dst_rect.Left, dst_rect.Top, dst_rect.Right, dst_rect.Bottom, dst_rect.GetWidth(), dst_rect.GetHeight());

    return true;
}

void graphics_mode_shutdown()
{
    if (GfxFactory)
        GfxFactory->Shutdown();
    GfxFactory = NULL;
    gfxDriver = NULL;

    delete filter;
    filter = NULL;

    // Tell Allegro that we are no longer in graphics mode
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}

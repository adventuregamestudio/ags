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

#include "main/mainheader.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/walkbehind.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "main/graphics_mode.h"
#include "platform/base/agsplatformdriver.h"
#include "gfx/ali3dexception.h"
#include "gfx/bitmap.h"
#include "gfx/gfxdriverfactory.h"
#include "gfx/graphicsdriver.h"
#include "main/main_allegro.h"
#include "util/geometry.h"
#include "util/math.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetup usetup;
extern GameSetupStruct game;
extern int proper_exit;
extern GUIMain*guis;
extern int psp_gfx_renderer; // defined in ali3dogl
extern WalkBehindMethodEnum walkBehindMethod;
extern DynamicArray<GUIInv> guiinv;
extern int numguiinv;
extern int current_screen_resolution_multiplier;
extern char force_gfxfilter[50];
extern AGSPlatformDriver *platform;
extern int force_16bit;
extern IGraphicsDriver *gfxDriver;
extern volatile int timerloop;
extern IDriverDependantBitmap *blankImage;
extern IDriverDependantBitmap *blankSidebarImage;
extern Bitmap *_old_screen;
extern Bitmap *_sub_screen;
extern int _places_r, _places_g, _places_b;

IGfxDriverFactory *GfxFactory = NULL;
IGfxFilter *filter;

struct ColorDepthOption
{
    int32_t Prime;
    int32_t Alternate;
};

GraphicResolution ScreenResolution;
PlaneScaling      GameScaling;

String GfxFilterRequest;

int debug_15bit_mode = 0, debug_24bit_mode = 0;
int convert_16bit_bgr = 0;

int ff; // whatever!

int adjust_pixel_size_for_loaded_data(int size, int filever)
{
    if (filever < kGameVersion_310)
    {
        return multiply_up_coordinate(size);
    }
    return size;
}

void adjust_pixel_sizes_for_loaded_data(int *x, int *y, int filever)
{
    x[0] = adjust_pixel_size_for_loaded_data(x[0], filever);
    y[0] = adjust_pixel_size_for_loaded_data(y[0], filever);
}

void adjust_sizes_for_resolution(int filever)
{
    int ee;
    for (ee = 0; ee < game.numcursors; ee++) 
    {
        game.mcurs[ee].hotx = adjust_pixel_size_for_loaded_data(game.mcurs[ee].hotx, filever);
        game.mcurs[ee].hoty = adjust_pixel_size_for_loaded_data(game.mcurs[ee].hoty, filever);
    }

    for (ee = 0; ee < game.numinvitems; ee++) 
    {
        adjust_pixel_sizes_for_loaded_data(&game.invinfo[ee].hotx, &game.invinfo[ee].hoty, filever);
    }

    for (ee = 0; ee < game.numgui; ee++) 
    {
        GUIMain*cgp=&guis[ee];
        adjust_pixel_sizes_for_loaded_data(&cgp->x, &cgp->y, filever);
        if (cgp->wid < 1)
            cgp->wid = 1;
        if (cgp->hit < 1)
            cgp->hit = 1;
        // Temp fix for older games
        if (cgp->wid == BASEWIDTH - 1)
            cgp->wid = BASEWIDTH;

        adjust_pixel_sizes_for_loaded_data(&cgp->wid, &cgp->hit, filever);

        cgp->popupyp = adjust_pixel_size_for_loaded_data(cgp->popupyp, filever);

        for (ff = 0; ff < cgp->numobjs; ff++) 
        {
            adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->x, &cgp->objs[ff]->y, filever);
            adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->wid, &cgp->objs[ff]->hit, filever);
            cgp->objs[ff]->activated=0;
        }
    }

    if ((filever >= 37) && (game.options[OPT_NATIVECOORDINATES] == 0) &&
        game.IsHiRes())
    {
        // New 3.1 format game file, but with Use Native Coordinates off

        for (ee = 0; ee < game.numcharacters; ee++) 
        {
            game.chars[ee].x /= 2;
            game.chars[ee].y /= 2;
        }

        for (ee = 0; ee < numguiinv; ee++)
        {
            guiinv[ee].itemWidth /= 2;
            guiinv[ee].itemHeight /= 2;
        }
    }

}

bool get_desktop_size_for_mode(Size &size, const bool windowed)
{
    if (!windowed)
        return get_desktop_resolution(&size.Width, &size.Height) == 0;
    else if (get_desktop_resolution(&size.Width, &size.Height) == 0)
    {
        // TODO: a platform-specific way to do this?
        size.Height -= 32; // give some space for window borders
        return true;
    }
    return false;
}

String make_scaling_factor_string(int scaling)
{
    if (scaling == 0)
        return "max";
    return String::FromFormat("%d", scaling >= kUnit ? (scaling >> kShift) : -kUnit / scaling);
}

String make_scaling_factor_string()
{
    if (usetup.filter_scaling_max_uniform)
        return "max uniform";
    return String::FromFormat("%s x %s",
        make_scaling_factor_string(usetup.filter_scaling_x).GetCStr(),
        make_scaling_factor_string(usetup.filter_scaling_y).GetCStr());
}

void engine_init_resolution_settings(const Size game_size, ColorDepthOption &color_depths)
{
    Out::FPrint("Initializing resolution settings");

    play.SetViewport(game.size);

    if (game.IsHiRes())
    {
        play.native_size.Width = game.size.Width / 2;
        play.native_size.Height = game.size.Height / 2;
        wtext_multiply = 2;
    }
    else
    {
        play.native_size.Width = game.size.Width;
        play.native_size.Height = game.size.Height;
        wtext_multiply = 1;
    }

    usetup.textheight = wgetfontheight(0) + 1;
    current_screen_resolution_multiplier = game.size.Width / play.native_size.Width;

    if (game.IsHiRes() &&
        (game.options[OPT_NATIVECOORDINATES]))
    {
        play.native_size.Width *= 2;
        play.native_size.Height *= 2;
    }

    // don't allow them to force a 256-col game to hi-color
    if (game.color_depth < 2)
        usetup.force_hicolor_mode = false;

    if (debug_15bit_mode)
    {
        color_depths.Prime = 15;
        color_depths.Alternate = 15;
    }
    else if (debug_24bit_mode)
    {
        color_depths.Prime = 24;
        color_depths.Alternate = 24;
    }
    else if ((game.color_depth == 2) || (force_16bit) || (usetup.force_hicolor_mode))
    {
        color_depths.Prime = 16;
        color_depths.Alternate = 15;
    }
    else if (game.color_depth > 2)
    {
        color_depths.Prime = 32;
        color_depths.Alternate = 24;
    }
    else
    {
        color_depths.Prime = 8;
        color_depths.Alternate = 8;
    }

    // Log out display information
    Size device_size;
    if (get_desktop_resolution(&device_size.Width, &device_size.Height) == 0)
        Out::FPrint("Device display resolution: %d x %d", device_size.Width, device_size.Height);
    else
        Out::FPrint("Unable to obtain device resolution");

    Out::FPrint("Game native resolution: %d x %d (%d bit)%s", game_size.Width, game_size.Height, color_depths.Prime,
        game.options[OPT_LETTERBOX] == 0 ? "": " letterbox-by-design");
    const char *screen_sz_def_options[kNumScreenDef] = { "explicit", "scaling", "max" };
    const char *game_frame_options[kNumRectPlacement] = { "offset", "center", "stretch", "proportional" };
    Out::FPrint("Game settings: windowed = %s, screen def: %s, screen size: %d x %d, match device ratio: %s, frame placement: %s",
        usetup.windowed ? "yes" : "no", screen_sz_def_options[usetup.screen_sz_def], usetup.screen_size.Width, usetup.screen_size.Height,
        usetup.match_device_ratio ? "yes" : "no",
        game_frame_options[usetup.game_frame_placement]);

    adjust_sizes_for_resolution(loaded_game_file_version);
}

bool initialize_graphics_filter(const String filter_id, const int color_depth)
{
    filter = GfxFactory->SetFilter(filter_id);
    if (!filter)
    {
        Out::FPrint("Unable to create filter: %s", filter_id.GetCStr());
        return false;
    }

    Out::FPrint("Applying scaling filter: %s", filter->GetInfo().Id.GetCStr());

    String filter_error;
    if (!filter->Initialize(color_depth, filter_error))
    {
        proper_exit = 1;
        platform->DisplayAlert("Unable to initialize the graphics filter. It returned the following error:\n'%s'\n\nTry choosing a different graphics filter.", filter_error.GetCStr());
        return false;
    }
    return true;
}

bool pre_create_gfx_driver(const String &gfx_driver_id)
{
    GfxFactory = GetGfxDriverFactory(gfx_driver_id);
    if (!GfxFactory)
    {
        Out::FPrint("Failed to initialize %s driver factory: %s", gfx_driver_id.GetCStr(), get_allegro_error());
        // If hardware driver was requested, try software instead
        // TODO: use default driver id
        if (gfx_driver_id.CompareNoCase("DX5") != 0)
            GfxFactory = GetGfxDriverFactory("DX5");
    }
    if (GfxFactory)
    {
        gfxDriver = GfxFactory->GetDriver();
    }
    if (gfxDriver)
    {
        Out::FPrint("Created graphics driver: %s", gfxDriver->GetDriverName());
        return true;
    }
    return false;
}

int engine_set_gfx_filter(const int color_depth)
{
    Out::FPrint("Initializing gfx filters");
    // Assign user filter request, whether by command line, or config file
    if (force_gfxfilter[0])
        GfxFilterRequest = force_gfxfilter;
    else
        GfxFilterRequest = usetup.gfxFilterID;

    Out::FPrint("Requested gfx filter: %s, filter scaling: %s", GfxFilterRequest.GetCStr(), make_scaling_factor_string().GetCStr());
    String apply_filter = GfxFilterRequest;
    if (apply_filter.IsEmpty() || apply_filter.CompareNoCase("none") == 0)
    {
        apply_filter = "StdScale";
        usetup.filter_scaling_max_uniform = false;
        usetup.filter_scaling_x = kUnit;
        usetup.filter_scaling_y = kUnit;
    }

    if (!initialize_graphics_filter(apply_filter, color_depth))
    {
        Out::FPrint("Failed to apply gfx filter: %s; will try to use standard filter instead", apply_filter.GetCStr());
        if (!initialize_graphics_filter("StdScale", color_depth))
            return EXIT_NORMAL;
    }
    return RETURN_CONTINUE;
}

/*
// Determines if scaling from base_size to gfx mode is supported by the engine
inline bool is_scaling_supported(const Size &base_size, const GraphicResolution &mode, const int fixed_scaling, int &scaling_factor)
{
    const int xratio = mode.Width / base_size.Width;
    const int yratio = mode.Height / base_size.Height;
    scaling_factor = fixed_scaling != 0 ? fixed_scaling : yratio;
    const int mult_precision_x = mode.Width % scaling_factor;
    const int mult_precision_y = mode.Height % scaling_factor;

    return scaling_factor > 0 && scaling_factor <= MaxScalingFactor &&
        // in current implementation the game frame includes horizontal "letterbox" borders,
        // therefore the height of game frame must always match the window height
        xratio >= yratio &&
        mult_precision_x == 0 && mult_precision_y == 0;
}
*/

bool find_nearest_supported_mode(Size &wanted_size, const int color_depth, const Size *ratio_reference)
{
    IGfxModeList *modes = gfxDriver->GetSupportedModeList(color_depth);
    if (!modes)
    {
        Out::FPrint("Couldn't get a list of supported resolutions");
        return false;
    }

    int wanted_ratio = 0;
    if (ratio_reference)
    {
        wanted_ratio = (ratio_reference->Height << kShift) / ratio_reference->Width;
    }
    
    int nearest_width = 0;
    int nearest_height = 0;
    int nearest_width_diff = 0;
    int nearest_height_diff = 0;
    int mode_count = modes->GetModeCount();
    DisplayMode mode;
    for (int i = 0; i < mode_count; ++i)
    {
        if (!modes->GetMode(i, mode))
        {
            continue;
        }
        if (mode.ColorDepth != color_depth)
        {
            continue;
        }
        if (wanted_ratio > 0)
        {
            int mode_ratio = (mode.Height << kShift) / mode.Width;
            if (mode_ratio != wanted_ratio)
            {
                continue;
            }
        }
        if (mode.Width == wanted_size.Width && mode.Height == wanted_size.Height)
        {
            nearest_width = mode.Width;
            nearest_height = mode.Height;
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
        }
    }

    delete modes;
    if (nearest_width > 0 && nearest_height > 0)
    {
        wanted_size.Width = nearest_width;
        wanted_size.Height = nearest_height;
        return true;
    }
    return false;
}

/*
int find_max_supported_uniform_scaling(const Size &base_size, Size &found_size, const int color_depth,
                                          const Size *ratio_reference = NULL, const bool keep_width = false, const bool keep_height = false)
{
    IGfxModeList *modes = gfxDriver->GetSupportedModeList(color_depth);
    if (!modes)
    {
        Out::FPrint("Couldn't get a list of supported resolutions");
        return 0;
    }

    int wanted_ratio = 0;
    if (ratio_reference)
    {
        wanted_ratio = (ratio_reference->Height << 10) / ratio_reference->Width;
    }

    int least_supported_scaling = 0;
    int mode_count = modes->GetModeCount();
    DisplayMode mode;
    for (int i = 0; i < mode_count; ++i)
    {
        if (!modes->GetMode(i, mode))
        {
            continue;
        }
        if (mode.ColorDepth != color_depth)
        {
            continue;
        }
        if (wanted_ratio > 0)
        {
            int mode_ratio = (mode.Height << 10) / mode.Width;
            if (mode_ratio != wanted_ratio)
            {
                continue;
            }
        }

        if (mode.Width >= base_size.Width &&
            mode.Height >= base_size.Height)
        {
            int scaling_factor;
            if (is_scaling_supported(base_size, mode, 0, scaling_factor) &&
                (!keep_width  || mode.Width  / scaling_factor == base_size.Width) &&
                (!keep_height || mode.Height / scaling_factor == base_size.Height) &&
                scaling_factor > least_supported_scaling)
            {
                found_size = Size(mode.Width, mode.Height);
                least_supported_scaling = scaling_factor;
            }
        }
    }

    delete modes;
    return least_supported_scaling;
}

int get_scaling_from_filter_name(const String &filter_id)
{
    int scaling = 0;
    if (filter_id.CompareLeftNoCase("StdScale") == 0)
    {
        scaling = filter_id.Mid(8).ToInt();
    }
    else if (filter_id.CompareLeftNoCase("Hq") == 0)
    {
        scaling = filter_id.Mid(2).ToInt();
    }
    else if (filter_id.CompareLeftNoCase("AAx") == 0)
    {
        scaling = filter_id.Mid(3).ToInt();
    }
    return scaling > 0 ? scaling : 1;
}

// Finds any supported graphics mode that can fit requested game frame size;
// returns true if found acceptable mode, sets found_size.
bool try_find_nearest_supported_mode(const Size &base_size, const int scaling_factor, Size &found_size, const int color_depth,
                                     const bool windowed, const bool prefer_sideborders, const bool prefer_letterbox)
{
    Size desktop_size;
    if (!get_desktop_size_for_mode(desktop_size, windowed))
    {
        Out::FPrint("Failed to find acceptable supported gfx mode (unable to obtain device resolution)");
        return false;
    }
    const Size wanted_size = base_size * scaling_factor;
    // Windowed mode
    if (windowed ||
        // Temporary hack for incomplete OpenGL driver (permit any resolution)
        stricmp(gfxDriver->GetDriverID(), "OGL") == 0)
    {
        // Do not try to create windowed mode larger than current desktop resolution
        if (!wanted_size.ExceedsByAny(desktop_size))
        {
            found_size = wanted_size;
            return true;
        }
        return false;
    }

    // Fullscreen mode: always try strictly no borders first, unless they are
    // explicitly enabled, so that modes with borders could be tried later
    // anyway if "no borders" failed.
    bool found = false;
    if (!prefer_sideborders)
    {
        // no sideborders
        if (!prefer_letterbox) 
            // no letterboxing, perfect match only
            found = find_nearest_supported_mode(base_size, scaling_factor, found_size, color_depth, NULL, true, true);

        if (!found)
        {
            // with letterboxing, letterbox only
            // try match desktop ratio
            found = find_nearest_supported_mode(base_size, scaling_factor, found_size, color_depth, &desktop_size, true, false);
            if (!found)
                // disregard desktop ratio
                found = find_nearest_supported_mode(base_size, scaling_factor, found_size, color_depth, NULL, true, false);
        }
    }

    if (!found)
    {
        // with sideborders
        if (!prefer_letterbox) 
        {
            // no letterbox, sideborders only
            // try match desktop ratio
            found = find_nearest_supported_mode(base_size, scaling_factor, found_size, color_depth, &desktop_size, false, true);
            if (!found)
                // disregard desktop ratio
                found = find_nearest_supported_mode(base_size, scaling_factor, found_size, color_depth, NULL, false, true);
        }

        if (!found)
        {
            // with letterboxing, letterbox + sideborders
            // try match desktop ratio
            found = find_nearest_supported_mode(base_size, scaling_factor, found_size, color_depth, &desktop_size);
            if (!found)
                // disregard desktop ratio
                found = find_nearest_supported_mode(base_size, scaling_factor, found_size, color_depth, NULL);
        }
    }

    if (!found)
        Out::FPrint("Couldn't find acceptable supported gfx mode for game frame %d x %d (%d bit)",
            wanted_size.Width, wanted_size.Height, color_depth);
    return found;
}

// Find maximal possible uniform integer scaling for the given game size, which can be handled by graphics driver;
// returns found scaling factor, sets found_size.
int try_find_max_supported_uniform_scaling(const Size &base_size, Size &found_size, const int color_depth,
                                           const bool windowed, const bool prefer_sideborders, const bool prefer_letterbox)
{
    Size desktop_size;
    if (!get_desktop_size_for_mode(desktop_size, windowed))
    {
        Out::FPrint("Failed to find max supported uniform scaling (unable to obtain device resolution)");
        return 0;
    }
    int multiplier = 0;
    // Windowed mode
    if (windowed ||
        // Temporary hack for incomplete OpenGL driver (permit any resolution)
        stricmp(gfxDriver->GetDriverID(), "OGL") == 0)
    {
        // Do not try to create windowed mode larger than current desktop resolution
        const int xratio = desktop_size.Width / base_size.Width;
        const int yratio = desktop_size.Height / base_size.Height;
        multiplier = Math::Min(xratio, yratio);
        found_size = base_size * multiplier;
        return multiplier;
    }

    // Fullscreen mode: always try strictly no borders first, unless they are
    // explicitly enabled, so that modes with borders could be tried later
    // anyway if "no borders" failed.
    if (!prefer_sideborders)
    {
        // no sideborders
        if (!prefer_letterbox) 
            // no letterboxing, perfect match only
            multiplier = find_max_supported_uniform_scaling(base_size, found_size, color_depth, NULL, true, true);

        if (!multiplier)
        {
            // with letterboxing, letterbox only
            // try match desktop ratio
            multiplier = find_max_supported_uniform_scaling(base_size, found_size, color_depth, &desktop_size, true, false);
            if (!multiplier)
                // disregard desktop ratio
                multiplier = find_max_supported_uniform_scaling(base_size, found_size, color_depth, NULL, true, false);
        }
    }

    if (!multiplier)
    {
        // with sideborders
        if (!prefer_letterbox) 
        {
            // no letterbox, sideborders only
            // try match desktop ratio
            multiplier = find_max_supported_uniform_scaling(base_size, found_size, color_depth, &desktop_size, false, true);
            if (!multiplier)
                // disregard desktop ratio
                multiplier = find_max_supported_uniform_scaling(base_size, found_size, color_depth, NULL, false, true);
        }

        if (!multiplier)
        {
            // with letterboxing, letterbox + sideborders
            // try match desktop ratio
            multiplier = find_max_supported_uniform_scaling(base_size, found_size, color_depth, &desktop_size);
            if (!multiplier)
                // disregard desktop ratio
                multiplier = find_max_supported_uniform_scaling(base_size, found_size, color_depth, NULL);
        }
    }

    if (!multiplier)
        Out::FPrint("Couldn't find any supported uniform scaling for game size %d x %d (%d bit)",
            base_size.Width, base_size.Height, color_depth);
    return multiplier;
}

int engine_init_gfx_filters(Size &game_size, Size &screen_size, const int color_depth)
{
    Out::FPrint("Initializing gfx filters");
    if (force_gfxfilter[0])
        GfxFilterRequest = force_gfxfilter;
    else
        GfxFilterRequest = usetup.gfxFilterID;
    Out::FPrint("Requested gfx filter: %s", GfxFilterRequest.GetCStr());

    String gfxfilter;
    if (GfxFilterRequest.CompareNoCase("max") != 0)
        gfxfilter = GfxFilterRequest;
    
    const Size base_size = game_size;

    int scaling_factor = 0;
    if (!gfxfilter.IsEmpty())
    {
        scaling_factor = get_scaling_from_filter_name(gfxfilter);
        Size found_screen_size;
        if (try_find_nearest_supported_mode(base_size, scaling_factor, found_screen_size, color_depth,
                usetup.windowed, usetup.prefer_sideborders, usetup.prefer_letterbox))
            screen_size = found_screen_size;
    }

    if (screen_size.IsNull())
    {
#if defined (WINDOWS_VERSION) || defined (LINUX_VERSION)
        Size found_screen_size;
        scaling_factor = try_find_max_supported_uniform_scaling(base_size, found_screen_size, color_depth,
                            usetup.windowed, usetup.prefer_sideborders, usetup.prefer_letterbox);
        if (scaling_factor > 0)
        {
            screen_size = found_screen_size;
        }
        else
#endif
        {
            screen_size = game_size;
            scaling_factor = 1;
        }
        gfxfilter.Format(scaling_factor > 1 ? "StdScale%d" : "None", scaling_factor);
    }

    if (gfxfilter.IsEmpty())
    {
        set_allegro_error("Failed to find acceptable graphics filter");
        return EXIT_NORMAL;
    }
    game_size.Width = screen_size.Width / scaling_factor;
    game_size.Height = screen_size.Height / scaling_factor;
    Out::FPrint("Chosen gfx resolution: %d x %d (%d bit), game frame: %d x %d",
        screen_size.Width, screen_size.Height, color_depth, game_size.Width, game_size.Height);
    if (!initialize_graphics_filter(gfxfilter, base_size.Width, base_size.Height, color_depth))
    {
        Out::FPrint("Failed to apply gfx filter: %s; will try to use standard filter instead", gfxfilter.GetCStr());
        if (!initialize_graphics_filter("StdScale", base_size.Width, base_size.Height, color_depth))
            return EXIT_NORMAL;
    }
    return RETURN_CONTINUE;
}
*/

bool create_gfx_driver(const String &gfx_driver_id)
{
    Out::FPrint("Init gfx driver");
    if (!pre_create_gfx_driver(gfx_driver_id))
        return false;

    usetup.gfxDriverID = gfxDriver->GetDriverID();
    gfxDriver->SetCallbackOnInit(GfxDriverOnInitCallback);
    gfxDriver->SetTintMethod(TintReColourise);
    return true;
}

bool init_gfx_mode(const Size game_size, const Size screen_size, const Size frame_size, const RectPlacement frame_place,
                   const int color_depth, const bool windowed)
{
    ScreenResolution.Width = screen_size.Width;
    ScreenResolution.Height = screen_size.Height;
    ScreenResolution.ColorDepth = color_depth;

    // CHECKME: find out what is this all about
    if (game.color_depth == 1)
        ScreenResolution.ColorDepth = 8;
    else
        set_color_depth(color_depth);

    const char *frame_placement[kNumRectPlacement] = { "offset", "center", "stretch", "proportional" };
    Out::FPrint("Attempt to switch gfx mode to %d x %d (%d-bit) %s; game frame: %d x %d, frame placement: %s",
        ScreenResolution.Width, ScreenResolution.Height, ScreenResolution.ColorDepth, windowed ? "windowed" : "fullscreen",
        frame_size.Width, frame_size.Height, frame_placement[frame_place]);

    // CHECKME: why 50? Do we need this if we are not using allegro software driver?
    if (usetup.refresh >= 50)
        request_refresh_rate(usetup.refresh);

    const DisplayMode mode = DisplayMode(ScreenResolution, windowed, usetup.refresh, usetup.vsync);
    const Size src_size = game_size;
    const Rect screen_rect = RectWH(0, 0, screen_size.Width, screen_size.Height);
    Rect dst_rect = PlaceInRect(screen_rect, RectWH(0, 0, frame_size.Width, frame_size.Height), frame_place);

    if (gfxDriver->Init(mode, src_size, dst_rect, &timerloop))
    {
        Rect filter_rect = filter->GetDestination();
        Rect dst_rect = gfxDriver->GetRenderDestination();
        Out::FPrint("Succeeded. Using gfx mode %d x %d (%d-bit) %s\n\t"
                    "filter dest (%d, %d, %d, %d : %d x %d), render dest (%d, %d, %d, %d : %d x %d)",
            ScreenResolution.Width, ScreenResolution.Height, ScreenResolution.ColorDepth, windowed ? "windowed" : "fullscreen",
            filter_rect.Left, filter_rect.Top, filter_rect.Right, filter_rect.Bottom, filter_rect.GetWidth(), filter_rect.GetHeight(),
            dst_rect.Left, dst_rect.Top, dst_rect.Right, dst_rect.Bottom, dst_rect.GetWidth(), dst_rect.GetHeight());
        return true;
    }
    Out::FPrint("Failed. %s", get_allegro_error());
    return false;
}

void set_game_frame_after_screen_size(const Size game_size, const Size screen_size, Size &frame_size)
{
    // Set game frame as native game resolution scaled by custom factor
    const int max_scaling_x = (screen_size.Width / game_size.Width) << kShift;
    const int max_scaling_y = (screen_size.Height / game_size.Height) << kShift;
    int scale_x, scale_y;
    if (usetup.filter_scaling_max_uniform)
    {
        scale_x = scale_y = min(max_scaling_x, max_scaling_y);
    }
    else
    {
        scale_x = usetup.filter_scaling_x == 0 ? max_scaling_x : usetup.filter_scaling_x;
        scale_y = usetup.filter_scaling_y == 0 ? max_scaling_y : usetup.filter_scaling_y;
    }
    // Ensure scaling factors are sane
    if (scale_x <= 0)
        scale_x = kUnit;
    if (scale_y <= 0)
        scale_y = kUnit;
    frame_size = Size((game_size.Width * scale_x) >> kShift, (game_size.Height * scale_y) >> kShift);
}

void precalc_render_screen_and_frame(const Size game_size, Size &screen_size, Size &frame_size, const int color_depth, const bool windowed)
{
    Size device_size;
    get_desktop_size_for_mode(device_size, windowed);

    // Set requested screen (window) size, depending on screen definition option
    switch (usetup.screen_sz_def)
    {
    case kScreenDef_Explicit:
        // Use resolution from user config
        screen_size = usetup.screen_size;
        if (screen_size.IsNull())
        {
            // If the configuration did not define proper screen size,
            // use the scaled game size instead
            set_game_frame_after_screen_size(game_size, device_size, frame_size);
            if (screen_size.Width <= 0)
                screen_size.Width = frame_size.Width;
            if (screen_size.Height <= 0)
                screen_size.Height = frame_size.Height;
        }
        break;
    case kScreenDef_ByGameScaling:
        // Use game frame (scaled game) size
        set_game_frame_after_screen_size(game_size, device_size, frame_size);
        screen_size = frame_size;
        break;
    case kScreenDef_MaxDisplay:
        // Set as big as current device size
        screen_size = device_size;
        break;
    }
}

void setup_render_frame(const Size game_size, const Size screen_size, Size &frame_size, RectPlacement &frame_place)
{
    // Setup final render frame, depending on defined screen size
    if (usetup.screen_sz_def != kScreenDef_ByGameScaling)
    {
        set_game_frame_after_screen_size(game_size, screen_size, frame_size);
    }

    frame_place = usetup.game_frame_placement;
    // If the scaled game size appear larger than the window,
    // do not apply a "centered" style, use "proportional stretch" instead
    if (frame_place == kPlaceCenter || frame_place == kPlaceOffset)
    {
        if (frame_size.ExceedsByAny(screen_size))
            frame_place = kPlaceStretchProportional;
    }
}

bool try_init_gfx_mode(const Size game_size, const Size screen_size, const Size frame_size, const int color_depth, const bool windowed)
{
    // Find nearest compatible mode and init that
    Out::FPrint("Attempting to find nearest supported resolution for screen size %d x %d (%d-bit) %s",
        screen_size.Width, screen_size.Height, color_depth, windowed ? "windowed" : "fullscreen");
    Size device_size;
    Size fixed_screen_size = screen_size;
    Size fixed_frame_size = frame_size;
    bool mode_found = false;

    get_desktop_size_for_mode(device_size, windowed);
    // Windowed mode
    if (windowed)
    {
        // If windowed mode, make the resolution stay in the generally supported limits
        // TODO: platform/driver specific values?
        const Size minimal_size(128, 128);
        if (!device_size.IsNull())
            device_size = screen_size;

        if (screen_size.ExceedsByAny(device_size) || minimal_size.ExceedsByAny(screen_size))
            fixed_screen_size.Clamp(minimal_size, device_size);
        mode_found = true;
    }
    // Fullscreen mode
    else
    {
        if (usetup.match_device_ratio && !device_size.IsNull())
            mode_found = find_nearest_supported_mode(fixed_screen_size, color_depth, &device_size);
        else
            mode_found = find_nearest_supported_mode(fixed_screen_size, color_depth, NULL);
    }

    if (mode_found)
    {
        RectPlacement frame_place;
        setup_render_frame(game_size, fixed_screen_size, fixed_frame_size, frame_place);
        return init_gfx_mode(game_size, fixed_screen_size, fixed_frame_size, frame_place, color_depth, windowed);
    }

    Out::FPrint("Could not find compatible graphics mode");
    return false;
}

bool try_init_gfx_mode(const Size game_size, const ColorDepthOption color_depths, const bool windowed)
{
    Size screen_size;
    Size frame_size;
    precalc_render_screen_and_frame(game_size, screen_size, frame_size, color_depths.Prime, windowed);
    bool result = try_init_gfx_mode(game_size, screen_size, frame_size, color_depths.Prime, windowed);
    if (!result && color_depths.Prime != color_depths.Alternate)
        result = try_init_gfx_mode(game_size, screen_size, frame_size, color_depths.Alternate, windowed);
    return result;
}

bool engine_init_graphics_mode(const Size game_size, const ColorDepthOption color_depths, const bool windowed)
{
    Out::FPrint("Switching to graphics mode");
    bool result = try_init_gfx_mode(game_size, color_depths, windowed);
    // Try windowed mode if fullscreen failed, and vice versa
    if (!result && editor_debugging_enabled == 0)
    {
        bool result = try_init_gfx_mode(game_size, color_depths, !windowed);
    }
    return result;
}

void CreateBlankImage()
{
    // this is the first time that we try to use the graphics driver,
    // so it's the most likey place for a crash
    try
    {
        Bitmap *blank = BitmapHelper::CreateBitmap(16, 16, ScreenResolution.ColorDepth);
        blank = gfxDriver->ConvertBitmapToSupportedColourDepth(blank);
        blank->Clear();
        blankImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        delete blank;
    }
    catch (Ali3DException gfxException)
    {
        quit((char*)gfxException._message);
    }

}

void engine_prepare_screen()
{
    Out::FPrint("Preparing graphics mode screen");

    _old_screen = BitmapHelper::GetScreenBitmap();

    if (gfxDriver->HasAcceleratedStretchAndFlip()) 
    {
        walkBehindMethod = DrawAsSeparateSprite;

        CreateBlankImage();
    }
}

void engine_set_gfx_driver_callbacks()
{
    gfxDriver->SetCallbackForPolling(update_polled_stuff_if_runtime);
    gfxDriver->SetCallbackToDrawScreen(draw_screen_callback);
    gfxDriver->SetCallbackForNullSprite(GfxDriverNullSpriteCallback);
}
void engine_set_color_conversions()
{
    Out::FPrint("Initializing colour conversion");

    // default shifts for how we store the sprite data
#if defined(PSP_VERSION)
    // PSP: Switch b<>r for 15/16 bit.
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
    _rgb_b_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_r_shift_16 = 0;
    _rgb_b_shift_15 = 10;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 0;
#else
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
    _rgb_r_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_b_shift_16 = 0;
    _rgb_r_shift_15 = 10;
    _rgb_g_shift_15 = 5;
    _rgb_b_shift_15 = 0;
#endif
    // Most cards do 5-6-5 RGB, which is the format the files are saved in
    // Some do 5-6-5 BGR, or  6-5-5 RGB, in which case convert the gfx
    if ((ScreenResolution.ColorDepth == 16) && ((_rgb_b_shift_16 != 0) || (_rgb_r_shift_16 != 11)))
    {
        convert_16bit_bgr = 1;
        if (_rgb_r_shift_16 == 10) {
            // some very old graphics cards lie about being 16-bit when they
            // are in fact 15-bit ... get around this
            _places_r = 3;
            _places_g = 3;
        }
    }
    if (ScreenResolution.ColorDepth > 16)
    {
        // when we're using 32-bit colour, it converts hi-color images
        // the wrong way round - so fix that

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
        _rgb_b_shift_16 = 0;
        _rgb_g_shift_16 = 5;
        _rgb_r_shift_16 = 11;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;

        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;
#else
        _rgb_r_shift_16 = 11;
        _rgb_g_shift_16 = 5;
        _rgb_b_shift_16 = 0;
#endif
    }
    else if (ScreenResolution.ColorDepth == 16)
    {
        // ensure that any 32-bit graphics displayed are converted
        // properly to the current depth
#if defined(PSP_VERSION)
        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;
#else
        _rgb_r_shift_32 = 16;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 0;
#endif
    }
    else if (ScreenResolution.ColorDepth < 16)
    {
        // ensure that any 32-bit graphics displayed are converted
        // properly to the current depth
#if defined (WINDOWS_VERSION)
        _rgb_r_shift_32 = 16;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 0;
#else
        _rgb_r_shift_32 = 0;
        _rgb_g_shift_32 = 8;
        _rgb_b_shift_32 = 16;

        _rgb_b_shift_15 = 0;
        _rgb_g_shift_15 = 5;
        _rgb_r_shift_15 = 10;
#endif
    }

    set_color_conversion(COLORCONV_MOST | COLORCONV_EXPAND_256 | COLORCONV_REDUCE_16_TO_15);
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

bool create_gfx_driver_and_init_mode(const String &gfx_driver_id, const Size game_size,
                                     const ColorDepthOption color_depths, const bool windowed)
{
    if (!create_gfx_driver(gfx_driver_id))
        return false;
    // Log out supported driver modes
    log_out_driver_modes(color_depths.Prime);
    log_out_driver_modes(color_depths.Alternate);
    
    if (!engine_set_gfx_filter(color_depths.Prime))
        return false;

    if (!engine_init_graphics_mode(game_size, color_depths, windowed))
        return false;

    // init game scaling transformation
    GameScaling.Init(game_size, gfxDriver->GetRenderDestination());
    return true;
}

void display_gfx_mode_error(const Size game_size, const int color_depth)
{
    proper_exit=1;
    platform->FinishedUsingGraphicsMode();

    String main_error;
    Size screen_size(ScreenResolution.Width, ScreenResolution.Height);
    if (screen_size.IsNull())
        main_error.Format("There was a problem finding appropriate graphics mode for game size %d x %d (%d-bit) and requested filter '%s'.",
            game_size.Width, game_size.Height, color_depth, GfxFilterRequest.IsEmpty() ? "Undefined" : GfxFilterRequest.GetCStr());
    else
        main_error.Format("There was a problem initializing graphics mode %d x %d (%d-bit) with game size %d x %d and filter '%s'.",
            screen_size.Width, screen_size.Height, color_depth, game_size.Width, game_size.Height, filter ? filter->GetInfo().Id.GetCStr() : "Undefined");

    platform->DisplayAlert("%s\n"
            "(Problem: '%s')\n"
            "Try to correct the problem, or seek help from the AGS homepage."
            "%s",
            main_error.GetCStr(), get_allegro_error(), platform->GetGraphicsTroubleshootingText());
}

bool graphics_mode_init()
{
    ColorDepthOption color_depths;

    // Calculate required screen and game frame resolutions
    engine_init_resolution_settings(game.size, color_depths);

    if (!create_gfx_driver_and_init_mode(usetup.gfxDriverID, game.size, color_depths, usetup.windowed != 0))
    {
        if (gfxDriver && stricmp(gfxDriver->GetDriverID(), "DX5") != 0)
        {
            graphics_mode_shutdown();
            if (!create_gfx_driver_and_init_mode("DX5", game.size, color_depths, usetup.windowed != 0))
            {
                display_gfx_mode_error(game.size, color_depths.Prime);
                return false;
            }
        }
    }

    engine_prepare_screen();
    platform->PostAllegroInit(usetup.windowed);
    engine_set_gfx_driver_callbacks();
    engine_set_color_conversions();
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
}

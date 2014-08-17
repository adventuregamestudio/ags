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
#include "gfx/ali3d.h"
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
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "main/main_allegro.h"
#include "util/geometry.h"
#include "util/math.h"

using AGS::Common::Bitmap;
using AGS::Common::String;
namespace BitmapHelper = AGS::Common::BitmapHelper;
namespace Math = AGS::Common::Math;
namespace Out = AGS::Common::Out;

extern GameSetup usetup;
extern GameSetupStruct game;
extern int proper_exit;
extern GUIMain*guis;
extern int psp_gfx_renderer; // defined in ali3dogl
extern WalkBehindMethodEnum walkBehindMethod;
extern DynamicArray<GUIInv> guiinv;
extern int numguiinv;
extern int scrnwid,scrnhit;
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

const int MaxScalingFactor = 8; // we support up to x8 scaling now

Size GameSize;
int firstDepth, secondDepth;
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

void engine_init_screen_settings(Size &game_size, Size &screen_size)
{
    Out::FPrint("Initializing screen settings");

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

    usetup.base_width = 320;
    usetup.base_height = 200;

    GameSize = ResolutionTypeToSize(game.default_resolution);
    scrnwid = GameSize.Width;
    scrnhit = GameSize.Height;

    if (game.IsHiRes())
    {
        usetup.base_width = scrnwid / 2;
        usetup.base_height = scrnhit / 2;
        wtext_multiply = 2;
    }
    else
    {
        usetup.base_width = scrnwid;
        usetup.base_height = scrnhit;
        wtext_multiply = 1;
    }

    usetup.textheight = wgetfontheight(0) + 1;

    vesa_xres=scrnwid; vesa_yres=scrnhit;
    current_screen_resolution_multiplier = scrnwid / BASEWIDTH;

    if (game.IsHiRes() &&
        (game.options[OPT_NATIVECOORDINATES]))
    {
        usetup.base_width *= 2;
        usetup.base_height *= 2;
    }

    game.options[OPT_LETTERBOX] = usetup.prefer_letterbox ? 1 : 0;

    // don't allow them to force a 256-col game to hi-color
    if (game.color_depth < 2)
        usetup.force_hicolor_mode = false;

    firstDepth = 8, secondDepth = 8;
    if ((game.color_depth == 2) || (force_16bit) || (usetup.force_hicolor_mode)) {
        firstDepth = 16;
        secondDepth = 15;
    }
    else if (game.color_depth > 2) {
        firstDepth = 32;
        secondDepth = 24;
    }

    game_size = GameSize;
    screen_size = Size(0, 0);

    // Log out display information
    Size device_size;
    if (get_desktop_resolution(&device_size.Width, &device_size.Height) == 0)
        Out::FPrint("Device display resolution: %d x %d", device_size.Width, device_size.Height);
    else
        Out::FPrint("Unable to obtain device resolution");
    Out::FPrint("Game native resolution: %d x %d (%d bit), letterbox: %s, side borders: %s", scrnwid, scrnhit, firstDepth,
        usetup.prefer_letterbox ? "acceptable" : "undesirable", usetup.prefer_sideborders ? "acceptable" : "undesirable");

    adjust_sizes_for_resolution(loaded_game_file_version);
}

int initialize_graphics_filter(const char *filterID, int width, int height, int colDepth)
{
    int idx = 0;
    GFXFilter **filterList;

    if (usetup.gfxDriverID.CompareNoCase("D3D9") == 0)
    {
        filterList = get_d3d_gfx_filter_list(false);
    }
    else
    {
        filterList = get_allegro_gfx_filter_list(false);
    }

    // by default, select No Filter
    filter = filterList[0];

    GFXFilter *thisFilter = filterList[idx];
    while (thisFilter != NULL) {

        if ((filterID != NULL) &&
            (strcmp(thisFilter->GetFilterID(), filterID) == 0))
            filter = thisFilter;
        else if (idx > 0)
            delete thisFilter;

        idx++;
        thisFilter = filterList[idx];
    }

    Out::FPrint("Applying scaling filter: %s", filter->GetFilterID());

    const char *filterError = filter->Initialize(width, height, colDepth);
    if (filterError != NULL) {
        proper_exit = 1;
        platform->DisplayAlert("Unable to initialize the graphics filter. It returned the following error:\n'%s'\n\nTry running Setup and selecting a different graphics filter.", filterError);
        return -1;
    }

    gfxDriver->SetGraphicsFilter(filter);
    return 0;
}

bool pre_create_gfx_driver(const String &gfx_driver_id)
{
#ifdef WINDOWS_VERSION
    if (gfx_driver_id.CompareNoCase("D3D9") == 0 && (game.color_depth != 1))
    {
        gfxDriver = GetD3DGraphicsDriver(NULL);
        if (!gfxDriver)
        {
            Out::FPrint("Failed to initialize D3D9 driver: %s", get_allegro_error());
        }
    }
    else
#endif
#if defined (IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
    if (gfx_driver_id.CompareNoCase("DX5") != 0 && (psp_gfx_renderer > 0) && (game.color_depth != 1))
    {
        gfxDriver = GetOGLGraphicsDriver(NULL);
        if (!gfxDriver)
        {
            Out::FPrint("Failed to initialize OGL driver: %s", get_allegro_error());
        }
    }
#endif

    if (!gfxDriver)
    {
        gfxDriver = GetSoftwareGraphicsDriver(NULL);
    }

    if (gfxDriver)
    {
        Out::FPrint("Created graphics driver: %s", gfxDriver->GetDriverName());
        return true;
    }
    return false;
}

// Determines if scaling from base_size to gfx mode is supported by the engine
inline bool is_scaling_supported(const Size &base_size, const DisplayResolution &mode, const int fixed_scaling, int &scaling_factor)
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

bool find_nearest_supported_mode(const Size &base_size, const int scaling_factor, Size &found_size, const int color_depth,
                                 const Size *ratio_reference = NULL, const bool keep_width = false, const bool keep_height = false)
{
    IGfxModeList *modes = gfxDriver->GetSupportedModeList(color_depth);
    if (!modes)
    {
        Out::FPrint("Couldn't get a list of supported resolutions");
        return false;
    }

    Size wanted_size = base_size * scaling_factor;
    int wanted_ratio = 0;
    if (ratio_reference)
    {
        wanted_ratio = (ratio_reference->Height << 10) / ratio_reference->Width;
    }
    
    int nearest_width = 0;
    int nearest_height = 0;
    int nearest_width_diff = 0;
    int nearest_height_diff = 0;
    int mode_count = modes->GetModeCount();
    DisplayResolution mode;
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
        if (mode.Width == wanted_size.Width && mode.Height == wanted_size.Height)
        {
            nearest_width = mode.Width;
            nearest_height = mode.Height;
            break;
        }
        if (keep_width && mode.Width != wanted_size.Width ||
            keep_height && mode.Height != wanted_size.Height ||
            // current implementation does not allow downscaling
            mode.Width < wanted_size.Width ||
            mode.Height < wanted_size.Height)
        {
            continue;
        }
      
        int diff_w = abs(wanted_size.Width - mode.Width);
        int diff_h = abs(wanted_size.Height - mode.Height);
        bool same_diff_w_higher = (diff_w == nearest_width_diff && nearest_width < mode.Width);
        bool same_diff_h_higher = (diff_h == nearest_height_diff && nearest_height < mode.Height);

        int multiplier;
        if (is_scaling_supported(base_size, mode, scaling_factor, multiplier) &&
            nearest_width == 0 ||
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
        found_size.Width = nearest_width;
        found_size.Height = nearest_height;
        return true;
    }
    return false;
}

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
    DisplayResolution mode;
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
    if (initialize_graphics_filter(gfxfilter, base_size.Width, base_size.Height, color_depth))
    {
        return EXIT_NORMAL;
    }
    return RETURN_CONTINUE;
}

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

bool init_gfx_mode(const Size &game_size, const Size &screen_size, int cdep)
{
    if (debug_15bit_mode)
        cdep = 15;
    else if (debug_24bit_mode)
        cdep = 24;

    Out::FPrint("Attempt to switch gfx mode to %d x %d (%d-bit) %s, game frame %d x %d, gfx filter: %s",
        screen_size.Width, screen_size.Height, cdep, usetup.windowed ? "windowed" : "fullscreen",
        game_size.Width, game_size.Height, filter->GetFilterID());

    if (usetup.refresh >= 50)
        request_refresh_rate(usetup.refresh);

    final_scrn_wid = game_size.Width;
    final_scrn_hit = game_size.Height;
    final_col_dep = cdep;
    game_frame_y_offset = (final_scrn_hit - scrnhit) / 2;
    usetup.want_letterbox = final_scrn_hit > scrnhit;

    if (game.color_depth == 1) {
        final_col_dep = 8;
    }
    else {
        set_color_depth(cdep);
    }

    const bool result = gfxDriver->Init(game_size.Width, game_size.Height, screen_size.Width, screen_size.Height, final_col_dep, usetup.windowed, &timerloop);

    if (result)
    {
        Out::FPrint("Succeeded. Using gfx mode %d x %d (%d-bit) %s, game frame %d x %d, gfx filter: %s",
            screen_size.Width, screen_size.Height, final_col_dep, usetup.windowed ? "windowed" : "fullscreen",
            game_size.Width, game_size.Height, filter->GetFilterID());
        return true;
    }
    else
        Out::FPrint("Failed, resolution not supported");
    return false;
}

bool switch_to_graphics_mode(const Size &game_size, const Size &screen_size) 
{
    bool result = init_gfx_mode(game_size, screen_size, firstDepth);
    if (!result && firstDepth != secondDepth)
        result = init_gfx_mode(game_size, screen_size, secondDepth);
    if (!result && editor_debugging_enabled == 0)
    {
        usetup.windowed = !usetup.windowed;
        result = init_gfx_mode(game_size, screen_size, firstDepth);
        if (!result && firstDepth != secondDepth)
            result = init_gfx_mode(game_size, screen_size, secondDepth);
    }
    return result;
}

int engine_init_graphics_mode(const Size &game_size, const Size &screen_size)
{
    Out::FPrint("Switching to graphics mode");

    if (!switch_to_graphics_mode(game_size, screen_size))
    {
        return EXIT_NORMAL;
    }
    return RETURN_CONTINUE;
}

void CreateBlankImage()
{
    // this is the first time that we try to use the graphics driver,
    // so it's the most likey place for a crash
    try
    {
        Bitmap *blank = BitmapHelper::CreateBitmap(16, 16, final_col_dep);
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

void engine_post_init_gfx_driver()
{
	_old_screen = BitmapHelper::GetScreenBitmap();

    if (gfxDriver->HasAcceleratedStretchAndFlip()) 
    {
        walkBehindMethod = DrawAsSeparateSprite;

        CreateBlankImage();
    }
}

void engine_prepare_screen()
{
    Out::FPrint("Preparing graphics mode screen");

    if ((final_scrn_hit != scrnhit) || (final_scrn_wid != scrnwid)) {
        _old_screen->Clear();
		BitmapHelper::SetScreenBitmap(
			BitmapHelper::CreateSubBitmap(_old_screen, RectWH(final_scrn_wid / 2 - scrnwid / 2, final_scrn_hit/2-scrnhit/2, scrnwid, scrnhit))
			);
		Bitmap *screen_bmp = BitmapHelper::GetScreenBitmap();
        _sub_screen=screen_bmp;

        scrnhit = screen_bmp->GetHeight();
        vesa_yres = screen_bmp->GetHeight();
        scrnwid = screen_bmp->GetWidth();
        vesa_xres = screen_bmp->GetWidth();
		gfxDriver->SetMemoryBackBuffer(screen_bmp);
    }


    // Most cards do 5-6-5 RGB, which is the format the files are saved in
    // Some do 5-6-5 BGR, or  6-5-5 RGB, in which case convert the gfx
    if ((final_col_dep == 16) && ((_rgb_b_shift_16 != 0) || (_rgb_r_shift_16 != 11))) {
        convert_16bit_bgr = 1;
        if (_rgb_r_shift_16 == 10) {
            // some very old graphics cards lie about being 16-bit when they
            // are in fact 15-bit ... get around this
            _places_r = 3;
            _places_g = 3;
        }
    }
    if (final_col_dep > 16) {
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
    else if (final_col_dep == 16) {
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
    else if (final_col_dep < 16) {
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
    DisplayResolution mode;
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

int create_gfx_driver_and_init_mode(const String &gfx_driver_id, Size &game_size, Size &screen_size)
{
    if (!create_gfx_driver(gfx_driver_id))
        return EXIT_NORMAL;

    // Log out supported driver modes
    log_out_driver_modes(firstDepth);
    log_out_driver_modes(secondDepth);
    
    int res = engine_init_gfx_filters(game_size, screen_size, firstDepth);
    if (res != RETURN_CONTINUE)
    {
        return res;
    }

    res = engine_init_graphics_mode(game_size, screen_size);
    if (res != RETURN_CONTINUE)
    {
        return res;
    }
    return RETURN_CONTINUE;
}

void display_gfx_mode_error(const Size &game_size, const Size &screen_size)
{
    proper_exit=1;
    platform->FinishedUsingGraphicsMode();

    String main_error;
    if (screen_size.IsNull())
        main_error.Format("There was a problem finding appropriate graphics mode for game size %d x %d (%d-bit) and requested filter '%s'.",
            game_size.Width, game_size.Height, firstDepth, GfxFilterRequest.IsEmpty() ? "Undefined" : GfxFilterRequest.GetCStr());
    else
        main_error.Format("There was a problem initializing graphics mode %d x %d (%d-bit) with game size %d x %d and filter '%s'.",
            screen_size.Width, screen_size.Height, firstDepth, game_size.Width, game_size.Height, filter ? filter->GetFilterID() : "Undefined");

    platform->DisplayAlert("%s\n"
            "(Problem: '%s')\n"
            "Try to correct the problem, or seek help from the AGS homepage.\n"
            "\nPossible causes:\n* your graphics card drivers do not support this resolution. "
            "Run the game setup program and try the other resolution.\n"
            "* the graphics driver you have selected does not work. Try switching between Direct3D and DirectDraw.\n"
            "* the graphics filter you have selected does not work. Try another filter.",
            main_error.GetCStr(), get_allegro_error());
}

int graphics_mode_init()
{
    // Engine may try to change from windowed to fullscreen if the first failed;
    // here we keep the original windowed flag in case we'll have to restore it
    const bool windowed = usetup.windowed;

    Size game_size;
    Size screen_size;

    engine_init_screen_settings(game_size, screen_size);

    int res = create_gfx_driver_and_init_mode(usetup.gfxDriverID, game_size, screen_size);
    if (res != RETURN_CONTINUE)
    {
        if (gfxDriver && stricmp(gfxDriver->GetDriverID(), "DX5") != 0)
        {
            graphics_mode_shutdown();
            usetup.windowed = windowed;
            res = create_gfx_driver_and_init_mode("DX5", game_size, screen_size);
        }
    }
    if (res != RETURN_CONTINUE)
    {
        display_gfx_mode_error(game_size, screen_size);
        return res;
    }

    engine_post_init_gfx_driver();
    engine_prepare_screen();
    platform->PostAllegroInit(usetup.windowed);
    engine_set_gfx_driver_callbacks();
    engine_set_color_conversions();
    return RETURN_CONTINUE;
}

void graphics_mode_shutdown()
{
    // Release the display mode (and anything dependant on the window)
    if (gfxDriver != NULL)
    {
        gfxDriver->UnInit();
    }

    // Tell Allegro that we are no longer in graphics mode
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

    delete gfxDriver;
    gfxDriver = NULL;

    delete filter;
    filter = NULL;
}

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
extern int force_letterbox;
extern AGSPlatformDriver *platform;
extern int force_16bit;
extern IGraphicsDriver *gfxDriver;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern volatile int timerloop;
extern IDriverDependantBitmap *blankImage;
extern IDriverDependantBitmap *blankSidebarImage;
extern Bitmap *_old_screen;
extern Bitmap *_sub_screen;
extern int _places_r, _places_g, _places_b;

const int MaxSidebordersWidth = 110;

int initasx,initasy;
int firstDepth, secondDepth;

// set to 0 once successful
int working_gfx_mode_status = -1;
int debug_15bit_mode = 0, debug_24bit_mode = 0;
int convert_16bit_bgr = 0;

int ff; // whatever!

int adjust_pixel_size_for_loaded_data(int size, int filever)
{
    if (filever < kGameVersion_300)
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
        (game.default_resolution > 2))
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

void engine_init_screen_settings()
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

    if (game.default_resolution >= 5)
    {
        if (game.default_resolution >= 6)
        {
            // 1024x768
            usetup.base_width = 512;
            usetup.base_height = 384;
        }
        else
        {
            // 800x600
            usetup.base_width = 400;
            usetup.base_height = 300;
        }
        // don't allow letterbox mode
        game.options[OPT_LETTERBOX] = 0;
        force_letterbox = 0;
        scrnwid = usetup.base_width * 2;
        scrnhit = usetup.base_height * 2;
        wtext_multiply = 2;
    }
    else if ((game.default_resolution == 4) ||
        (game.default_resolution == 3))
    {
        scrnwid = 640;
        scrnhit = 400;
        wtext_multiply = 2;
    }
    else if ((game.default_resolution == 2) ||
        (game.default_resolution == 1))
    {
        scrnwid = 320;
        scrnhit = 200;
        wtext_multiply = 1;
    }
    else
    {
        scrnwid = usetup.base_width;
        scrnhit = usetup.base_height;
        wtext_multiply = 1;
    }

    usetup.textheight = wgetfontheight(0) + 1;

    vesa_xres=scrnwid; vesa_yres=scrnhit;
    //scrnwto=scrnwid-1; scrnhto=scrnhit-1;
    current_screen_resolution_multiplier = scrnwid / BASEWIDTH;

    if ((game.default_resolution > 2) &&
        (game.options[OPT_NATIVECOORDINATES]))
    {
        usetup.base_width *= 2;
        usetup.base_height *= 2;
    }

    initasx=scrnwid,initasy=scrnhit;
    if (scrnwid==960) { initasx=1024; initasy=768; }

    // save this setting so we only do 640x480 full-screen if they want it
    usetup.want_letterbox = game.options[OPT_LETTERBOX];

    if (force_letterbox > 0)
        game.options[OPT_LETTERBOX] = 1;

    // don't allow them to force a 256-col game to hi-color
    if (game.color_depth < 2)
        usetup.force_hicolor_mode = 0;

    firstDepth = 8, secondDepth = 8;
    if ((game.color_depth == 2) || (force_16bit) || (usetup.force_hicolor_mode)) {
        firstDepth = 16;
        secondDepth = 15;
    }
    else if (game.color_depth > 2) {
        firstDepth = 32;
        secondDepth = 24;
    }

    adjust_sizes_for_resolution(loaded_game_file_version);
}

int initialize_graphics_filter(const char *filterID, int width, int height, int colDepth)
{
    int idx = 0;
    GFXFilter **filterList;

    if (stricmp(usetup.gfxDriverID, "D3D9") == 0)
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

void pre_create_gfx_driver()
{
#ifdef WINDOWS_VERSION
    if (stricmp(usetup.gfxDriverID, "D3D9") == 0)
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
    if ((psp_gfx_renderer > 0) && (game.color_depth != 1))
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
        usetup.gfxDriverID = gfxDriver->GetDriverID();
    }

    Out::FPrint("Created graphics driver: %s", gfxDriver->GetDriverName());
}

int find_max_supported_uniform_multiplier(const Size &base_size, const int color_depth, int width_range_allowed)
{
    IGfxModeList *modes = gfxDriver->GetSupportedModeList(color_depth);
    if (!modes)
    {
        Out::FPrint("Couldn't get a list of supported resolutions");
        return 0;
    }

    int least_supported_multiplier = 0;
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

        if (mode.Width > base_size.Width &&
            mode.Height > base_size.Height && mode.Height % base_size.Height == 0)
        {
            int multiplier_x = mode.Width / base_size.Width;
            int remainder_x = mode.Width % base_size.Width;
            int multiplier_y = mode.Height / base_size.Height;
            if (multiplier_x == multiplier_y && (remainder_x / multiplier_x <= width_range_allowed) &&
                multiplier_x > least_supported_multiplier)
            {
                least_supported_multiplier = multiplier_x;
            }
        }
    }

    delete modes;

    if (least_supported_multiplier == 0)
    {
        Out::FPrint("Couldn't find acceptable supported resolution");
    }
    return least_supported_multiplier;
}

int find_supported_resolution_width(const Size &ideal_size, int color_depth, int width_range_allowed)
{
    // Temporary hack for incomplete OpenGL driver (always returns ideal width)
    if (strcmp(gfxDriver->GetDriverID(), "OGL") == 0)
    {
        return ideal_size.Width;
    }

    IGfxModeList *modes = gfxDriver->GetSupportedModeList(color_depth);
    if (!modes)
    {
        Out::FPrint("Couldn't get a list of supported resolutions");
        return 0;
    }

    int ideal_width_scaled = ideal_size.Width;
    int ideal_height_scaled = ideal_size.Height;
    filter->GetRealResolution(&ideal_width_scaled, &ideal_height_scaled);
    const int filter_factor = ideal_width_scaled / ideal_size.Width;
    const int max_width_diff = width_range_allowed * filter_factor;

    int nearest_width = 0;
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

        if (mode.Height == ideal_height_scaled)
        {
            if (mode.Width == ideal_width_scaled)
            {
                nearest_width = mode.Width;
                break;
            }

            const int mode_diff    = abs(mode.Width - ideal_width_scaled);
            const int nearest_diff = abs(nearest_width - ideal_width_scaled);
            if (mode_diff <= max_width_diff &&
                (mode_diff < nearest_diff ||
                 mode_diff == nearest_diff && mode.Width > nearest_diff))
            {
                nearest_width = mode.Width;
            }
        }
    }

    delete modes;

    if (nearest_width == 0)
    {
        Out::FPrint("Couldn't find acceptable supported resolution");
    }
    return nearest_width / filter_factor;
}

int get_maximal_supported_scaling(const Size &game_size)
{
    int selected_scaling = 0;

    // max scaling for siderborders mode
    int desktop_width;
    int desktop_height;
    if (usetup.enable_side_borders != 0 &&
        get_desktop_resolution(&desktop_width, &desktop_height) == 0)
    {
        const int desktop_ratio = (desktop_height << 10) / desktop_width;
        const int game_ratio    = (game_size.Height << 10) / game_size.Width;
        if (desktop_ratio < game_ratio)
        {
            const int fixed_ratio_width = (game_size.Height << 10) / desktop_ratio;
            selected_scaling = find_max_supported_uniform_multiplier(Size(fixed_ratio_width, game_size.Height),
                firstDepth, MaxSidebordersWidth);
        }
    }

    // max scaling for normal mode
    selected_scaling = Math::Max(selected_scaling,
        find_max_supported_uniform_multiplier(game_size, firstDepth, 0));
    return selected_scaling;
}

String get_maximal_supported_scaling_filter()
{
    Out::FPrint("Detecting maximal supported scaling");
    String gfxfilter = "None";
    const int game_width = initasx;
    const int game_height = initasy;
    const int max_scaling = 8; // we support up to x8 scaling now

    // fullscreen mode
    if (usetup.windowed == 0)
    {
        int selected_scaling = 0;
        // max scaling for normal mode
        if (game.options[OPT_LETTERBOX] == 0)
        {
            selected_scaling = get_maximal_supported_scaling(Size(game_width, game_height));
        }
        // max scaling for letterboxed view
        const int game_height_letterbox = (game_height * 12) / 10;
        selected_scaling = Math::Max(selected_scaling,
            get_maximal_supported_scaling(Size(game_width, game_height_letterbox)));
        
        if (selected_scaling > 1)
        {
            selected_scaling = Math::Min(selected_scaling, max_scaling);
            gfxfilter.Format("StdScale%d", selected_scaling);
        }
    }
    // windowed mode
    else
    {
        // Do not try to create windowed mode larger than current desktop resolution
        int desktop_width;
        int desktop_height;
        if (get_desktop_resolution(&desktop_width, &desktop_height) == 0)
        {
            desktop_height -= 32; // give some space for window borders
            // TODO: a platform-specific way to do this?
            int xratio = desktop_width / game_width;
            int yratio = desktop_height / game_height;
            int selected_scaling = Math::Min(Math::Min(xratio, yratio), max_scaling);
            gfxfilter.Format("StdScale%d", selected_scaling);
        }
        else
        {
            Out::FPrint("Automatic scaling failed (unable to obtain desktop resolution)");
        }
    }
    return gfxfilter;
}

int engine_init_gfx_filters()
{
    Out::FPrint("Init gfx filters");

    String gfxfilter;

    if (force_gfxfilter[0]) {
        gfxfilter = force_gfxfilter;
    }
    else if (!usetup.gfxFilterID.IsEmpty() && stricmp(usetup.gfxFilterID, "max") != 0) {
        gfxfilter = usetup.gfxFilterID;
    }
#if defined (WINDOWS_VERSION) || defined (LINUX_VERSION)
    else {
        gfxfilter = get_maximal_supported_scaling_filter();
    }
#endif

    if (initialize_graphics_filter(gfxfilter, initasx, initasy, firstDepth))
    {
        return EXIT_NORMAL;
    }
    return RETURN_CONTINUE;
}

void create_gfx_driver() 
{
    Out::FPrint("Init gfx driver");
    pre_create_gfx_driver();

    gfxDriver->SetCallbackOnInit(GfxDriverOnInitCallback);
    gfxDriver->SetTintMethod(TintReColourise);
}

int init_gfx_mode(int wid,int hit,int cdep) {

    // a mode has already been initialized, so abort
    if (working_gfx_mode_status == 0) return 0;

    if (debug_15bit_mode)
        cdep = 15;
    else if (debug_24bit_mode)
        cdep = 24;

    Out::FPrint("Attempt to switch gfx mode to %d x %d (%d-bit)", wid, hit, cdep);

    if (usetup.refresh >= 50)
        request_refresh_rate(usetup.refresh);

    final_scrn_wid = wid;
    final_scrn_hit = hit;
    final_col_dep = cdep;

    if (game.color_depth == 1) {
        final_col_dep = 8;
    }
    else {
        set_color_depth(cdep);
    }

    working_gfx_mode_status = (gfxDriver->Init(wid, hit, final_col_dep, usetup.windowed > 0, &timerloop) ? 0 : -1);

    if (working_gfx_mode_status == 0) 
        Out::FPrint("Succeeded. Using gfx mode %d x %d (%d-bit)", wid, hit, final_col_dep);
    else
        Out::FPrint("Failed, resolution not supported");

    if ((working_gfx_mode_status < 0) && (usetup.windowed > 0) && (editor_debugging_enabled == 0)) {
        usetup.windowed ++;
        if (usetup.windowed > 2) usetup.windowed = 0;
        return init_gfx_mode(wid,hit,cdep);
    }
    return working_gfx_mode_status;    
}

int try_widescreen_bordered_graphics_mode_if_appropriate(int initasx, int initasy, int firstDepth)
{
    if (working_gfx_mode_status == 0) return 0;
    if (usetup.enable_side_borders == 0)
    {
        Out::FPrint("Widescreen side borders: disabled in Setup");
        return 1;
    }
    if (usetup.windowed > 0)
    {
        Out::FPrint("Widescreen side borders: disabled (windowed mode)");
        return 1;
    }

    int failed = 1;
    int desktopWidth, desktopHeight;
    if (get_desktop_resolution(&desktopWidth, &desktopHeight) == 0)
    {
        const int game_width  = initasx;
        const int game_height = initasy;
        const int desktop_ratio = (desktopHeight << 10) / desktopWidth;
        const int game_ratio    = (game_height << 10) / game_width;

        Out::FPrint("Widescreen side borders: game resolution: %d x %d; desktop resolution: %d x %d", game_width, game_height, desktopWidth, desktopHeight);

        if (desktop_ratio < game_ratio)
        {
            int tryWidth = (game_height << 10) / desktop_ratio;
            int supportedRes = find_supported_resolution_width(Size(tryWidth, game_height), firstDepth, MaxSidebordersWidth);
            if (supportedRes > 0)
            {
                tryWidth = supportedRes;
                Out::FPrint("Widescreen side borders: enabled, attempting resolution %d x %d", tryWidth, game_height);
            }
            else
            {
                Out::FPrint("Widescreen side borders: gfx card does not support suitable resolution. Will attempt %d x %d anyway", tryWidth, game_height);
            }
            failed = init_gfx_mode(tryWidth, game_height, firstDepth);
        }
        else
        {
            Out::FPrint("Widescreen side borders: disabled (not necessary, game and desktop aspect ratios match)", game_width, game_height, desktopWidth, desktopHeight);
        }
    }
    else 
    {
        Out::FPrint("Widescreen side borders: disabled (unable to obtain desktop resolution)");
    }
    return failed;
}

int switch_to_graphics_mode(int initasx, int initasy, int scrnwid, int scrnhit, int firstDepth, int secondDepth) 
{
    int failed;
    int initasyLetterbox = (initasy * 12) / 10;

    // first of all, try 16-bit normal then letterboxed
    if (game.options[OPT_LETTERBOX] == 0) 
    {
        failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, firstDepth);
        failed = init_gfx_mode(initasx,initasy, firstDepth);
    }
    failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, firstDepth);
    failed = init_gfx_mode(initasx, initasyLetterbox, firstDepth);

    if (secondDepth != firstDepth) {
        // now, try 15-bit normal then letterboxed
        if (game.options[OPT_LETTERBOX] == 0) 
        {
            failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, secondDepth);
            failed = init_gfx_mode(initasx,initasy, secondDepth);
        }
        failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, secondDepth);
        failed = init_gfx_mode(initasx, initasyLetterbox, secondDepth);
    }

    if ((scrnwid != initasx) || (scrnhit != initasy))
    {
        // now, try the original resolution at 16 then 15 bit
        failed = init_gfx_mode(scrnwid,scrnhit,firstDepth);
        failed = init_gfx_mode(scrnwid,scrnhit, secondDepth);
    }

    if (failed)
        return -1;

    return 0;
}

int engine_init_graphics_mode()
{
    Out::FPrint("Switching to graphics mode");

    if (switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
    {
        bool errorAndExit = true;

        if ((usetup.gfxFilterID.IsEmpty() || 
            (stricmp(usetup.gfxFilterID, "None") == 0)) &&
            (scrnwid == 320))
        {
            // If the game is 320x200 and no filter is being used, try using a 2x
            // filter automatically since many gfx drivers don't suport 320x200.
            Out::FPrint("320x200 not supported, trying with 2x filter");
            delete filter;

            if (initialize_graphics_filter("StdScale2", initasx, initasy, firstDepth)) 
            {
                return EXIT_NORMAL;
            }

            if (!switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
            {
                errorAndExit = false;
            }

        }

        if (errorAndExit)
        {
            proper_exit=1;
            platform->FinishedUsingGraphicsMode();

            // make sure the error message displays the true resolution
            if (game.options[OPT_LETTERBOX])
                initasy = (initasy * 12) / 10;

            if (filter != NULL)
                filter->GetRealResolution(&initasx, &initasy);

            platform->DisplayAlert("There was a problem initializing graphics mode %d x %d (%d-bit).\n"
                "(Problem: '%s')\n"
                "Try to correct the problem, or seek help from the AGS homepage.\n"
                "\nPossible causes:\n* your graphics card drivers do not support this resolution. "
                "Run the game setup program and try the other resolution.\n"
                "* the graphics driver you have selected does not work. Try switching between Direct3D and DirectDraw.\n"
                "* the graphics filter you have selected does not work. Try another filter.",
                initasx, initasy, firstDepth, get_allegro_error());
            return EXIT_NORMAL;
        }
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
    //screen = _filter->ScreenInitialized(screen, final_scrn_wid, final_scrn_hit);
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
        initasx = final_scrn_wid;
        initasy = final_scrn_hit;
        _old_screen->Clear();
		BitmapHelper::SetScreenBitmap(
			BitmapHelper::CreateSubBitmap(_old_screen, RectWH(initasx / 2 - scrnwid / 2, initasy/2-scrnhit/2, scrnwid, scrnhit))
			);
		Bitmap *screen_bmp = BitmapHelper::GetScreenBitmap();
        _sub_screen=screen_bmp;

        scrnhit = screen_bmp->GetHeight();
        vesa_yres = screen_bmp->GetHeight();
        scrnwid = screen_bmp->GetWidth();
        vesa_xres = screen_bmp->GetWidth();
		gfxDriver->SetMemoryBackBuffer(screen_bmp);

        Out::FPrint("Screen resolution: %d x %d; game resolution %d x %d", _old_screen->GetWidth(), _old_screen->GetHeight(), scrnwid, scrnhit);
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

/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

//
// Graphics initialization
//

#include "main/mainheader.h"
#include "ali3d.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/walkbehind.h"
#include "debug/debug.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "main/graphics_mode.h"
#include "platform/agsplatformdriver.h"

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
extern int force_letterbox;
extern AGSPlatformDriver *platform;
extern int force_16bit;
extern IGraphicsDriver *gfxDriver;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern volatile int timerloop;
extern IDriverDependantBitmap *blankImage;
extern IDriverDependantBitmap *blankSidebarImage;
extern block _old_screen;
extern block _sub_screen;
extern int _places_r, _places_g, _places_b;

int initasx,initasy;
int firstDepth, secondDepth;

// set to 0 once successful
int working_gfx_mode_status = -1;
int debug_15bit_mode = 0, debug_24bit_mode = 0;
int convert_16bit_bgr = 0;

int ff; // whatever!

int adjust_pixel_size_for_loaded_data(int size, int filever)
{
    if (filever < 37)
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
    write_log_debug("Initializing screen settings");

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

    // PSP: Don't letterbox a 320x200 screen.
    if ((game.default_resolution != 2) && (game.default_resolution != 4))
        force_letterbox = usetup.want_letterbox = game.options[OPT_LETTERBOX] = 0;		

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

    const char *filterError = filter->Initialize(width, height, colDepth);
    if (filterError != NULL) {
        proper_exit = 1;
        platform->DisplayAlert("Unable to initialize the graphics filter. It returned the following error:\n'%s'\n\nTry running Setup and selecting a different graphics filter.", filterError);
        return -1;
    }

    return 0;
}

int engine_init_gfx_filters()
{
    write_log_debug("Init gfx filters");

    if (initialize_graphics_filter(usetup.gfxFilterID, initasx, initasy, firstDepth))
    {
        return EXIT_NORMAL;
    }

    return RETURN_CONTINUE;
}

void create_gfx_driver() 
{
#ifdef WINDOWS_VERSION
    if (stricmp(usetup.gfxDriverID, "D3D9") == 0)
        gfxDriver = GetD3DGraphicsDriver(filter);
    else
#endif
    {
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
        if ((psp_gfx_renderer > 0) && (game.color_depth != 1))
            gfxDriver = GetOGLGraphicsDriver(filter);
        else
#endif
            gfxDriver = GetSoftwareGraphicsDriver(filter);
    }

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

    platform->WriteDebugString("Attempt to switch gfx mode to %d x %d (%d-bit)", wid, hit, cdep);

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
        platform->WriteDebugString("Succeeded. Using gfx mode %d x %d (%d-bit)", wid, hit, final_col_dep);
    else
        platform->WriteDebugString("Failed, resolution not supported");

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
        platform->WriteDebugString("Widescreen side borders: disabled in Setup");
        return 1;
    }
    if (usetup.windowed > 0)
    {
        platform->WriteDebugString("Widescreen side borders: disabled (windowed mode)");
        return 1;
    }

    int failed = 1;
    int desktopWidth, desktopHeight;
    if (get_desktop_resolution(&desktopWidth, &desktopHeight) == 0)
    {
        int gameHeight = initasy;

        int screenRatio = (desktopWidth * 1000) / desktopHeight;
        int gameRatio = (initasx * 1000) / gameHeight;
        // 1250 = 1280x1024 
        // 1333 = 640x480, 800x600, 1024x768, 1152x864, 1280x960
        // 1600 = 640x400, 960x600, 1280x800, 1680x1050
        // 1666 = 1280x768

        platform->WriteDebugString("Widescreen side borders: game resolution: %d x %d; desktop resolution: %d x %d", initasx, gameHeight, desktopWidth, desktopHeight);

        if ((screenRatio > 1500) && (gameRatio < 1500))
        {
            int tryWidth = (initasx * screenRatio) / gameRatio;
            int supportedRes = gfxDriver->FindSupportedResolutionWidth(tryWidth, gameHeight, firstDepth, 110);
            if (supportedRes > 0)
            {
                tryWidth = supportedRes;
                platform->WriteDebugString("Widescreen side borders: enabled, attempting resolution %d x %d", tryWidth, gameHeight);
            }
            else
            {
                platform->WriteDebugString("Widescreen side borders: gfx card does not support suitable resolution. will attempt %d x %d anyway", tryWidth, gameHeight);
            }
            failed = init_gfx_mode(tryWidth, gameHeight, firstDepth);
        }
        else
        {
            platform->WriteDebugString("Widescreen side borders: disabled (not necessary, game and desktop aspect ratios match)", initasx, gameHeight, desktopWidth, desktopHeight);
        }
    }
    else 
    {
        platform->WriteDebugString("Widescreen side borders: disabled (unable to obtain desktop resolution)");
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

void engine_init_gfx_driver()
{
    write_log_debug("Init gfx driver");

    create_gfx_driver();
}

int engine_init_graphics_mode()
{
    write_log_debug("Switching to graphics mode");

    if (switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
    {
        bool errorAndExit = true;

        if (((usetup.gfxFilterID == NULL) || 
            (stricmp(usetup.gfxFilterID, "None") == 0)) &&
            (scrnwid == 320))
        {
            // If the game is 320x200 and no filter is being used, try using a 2x
            // filter automatically since many gfx drivers don't suport 320x200.
            write_log_debug("320x200 not supported, trying with 2x filter");
            delete filter;

            if (initialize_graphics_filter("StdScale2", initasx, initasy, firstDepth)) 
            {
                return EXIT_NORMAL;
            }

            create_gfx_driver();

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
                initasx, initasy, firstDepth, allegro_error);
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
        BITMAP *blank = create_bitmap_ex(final_col_dep, 16, 16);
        blank = gfxDriver->ConvertBitmapToSupportedColourDepth(blank);
        clear(blank);
        blankImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        destroy_bitmap(blank);
    }
    catch (Ali3DException gfxException)
    {
        quit((char*)gfxException._message);
    }

}

void engine_post_init_gfx_driver()
{
    //screen = _filter->ScreenInitialized(screen, final_scrn_wid, final_scrn_hit);
    _old_screen = screen;

    if (gfxDriver->HasAcceleratedStretchAndFlip()) 
    {
        walkBehindMethod = DrawAsSeparateSprite;

        CreateBlankImage();
    }
}

void engine_prepare_screen()
{
    write_log_debug("Preparing graphics mode screen");

    if ((final_scrn_hit != scrnhit) || (final_scrn_wid != scrnwid)) {
        initasx = final_scrn_wid;
        initasy = final_scrn_hit;
        clear(_old_screen);
        screen = create_sub_bitmap(_old_screen, initasx / 2 - scrnwid / 2, initasy/2-scrnhit/2, scrnwid, scrnhit);
        _sub_screen=screen;

        scrnhit = screen->h;
        vesa_yres = screen->h;
        scrnwid = screen->w;
        vesa_xres = screen->w;
        gfxDriver->SetMemoryBackBuffer(screen);

        platform->WriteDebugString("Screen resolution: %d x %d; game resolution %d x %d", _old_screen->w, _old_screen->h, scrnwid, scrnhit);
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
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
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
}

void engine_set_gfx_driver_callbacks()
{
    gfxDriver->SetCallbackForPolling(update_polled_stuff_if_runtime);
    gfxDriver->SetCallbackToDrawScreen(draw_screen_callback);
    gfxDriver->SetCallbackForNullSprite(GfxDriverNullSpriteCallback);
}

void engine_set_color_conversions()
{
    write_log_debug("Initializing colour conversion");

    set_color_conversion(COLORCONV_MOST | COLORCONV_EXPAND_256 | COLORCONV_REDUCE_16_TO_15);
}

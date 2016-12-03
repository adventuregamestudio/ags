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

#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/game_version.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/runtime_defines.h"
#include "ac/walkbehind.h"
#include "ac/dynobj/scriptsystem.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "font/fonts.h"
#include "gfx/ali3dexception.h"
#include "gfx/graphicsdriver.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"
#include "main/graphics_mode.h"
#include "main/engine_setup.h"
#include "media/video/video.h"
#include "platform/base/agsplatformdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern ScriptSystem scsystem;
extern int _places_r, _places_g, _places_b;
extern int current_screen_resolution_multiplier;
extern WalkBehindMethodEnum walkBehindMethod;
extern int force_16bit;
extern IGraphicsDriver *gfxDriver;
extern IDriverDependantBitmap *blankImage;
extern IDriverDependantBitmap *blankSidebarImage;
extern Bitmap *_old_screen;
extern Bitmap *_sub_screen;
extern Bitmap *virtual_screen;

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
        adjust_pixel_sizes_for_loaded_data(&cgp->X, &cgp->Y, filever);
        if (cgp->Width < 1)
            cgp->Width = 1;
        if (cgp->Height < 1)
            cgp->Height = 1;
        // Temp fix for older games
        if (cgp->Width == BASEWIDTH - 1)
            cgp->Width = BASEWIDTH;

        adjust_pixel_sizes_for_loaded_data(&cgp->Width, &cgp->Height, filever);

        cgp->PopupAtMouseY = adjust_pixel_size_for_loaded_data(cgp->PopupAtMouseY, filever);

        for (ff = 0; ff < cgp->ControlCount; ff++) 
        {
            adjust_pixel_sizes_for_loaded_data(&cgp->Controls[ff]->x, &cgp->Controls[ff]->y, filever);
            adjust_pixel_sizes_for_loaded_data(&cgp->Controls[ff]->wid, &cgp->Controls[ff]->hit, filever);
            cgp->Controls[ff]->activated=0;
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

void engine_init_resolution_settings(const Size game_size)
{
    Debug::Printf("Initializing resolution settings");

    play.SetViewport(game_size);

    if (game.IsHiRes())
    {
        play.native_size.Width = game_size.Width / 2;
        play.native_size.Height = game_size.Height / 2;
        wtext_multiply = 2;
    }
    else
    {
        play.native_size.Width = game_size.Width;
        play.native_size.Height = game_size.Height;
        wtext_multiply = 1;
    }

    usetup.textheight = wgetfontheight(0) + 1;
    current_screen_resolution_multiplier = game_size.Width / play.native_size.Width;

    if (game.IsHiRes() &&
        (game.options[OPT_NATIVECOORDINATES]))
    {
        play.native_size.Width *= 2;
        play.native_size.Height *= 2;
    }

    // don't allow them to force a 256-col game to hi-color
    if (game.color_depth < 2)
        usetup.force_hicolor_mode = false;

    Debug::Printf(kDbgMsg_Init, "Game native resolution: %d x %d (%d bit)%s", game_size.Width, game_size.Height, game.color_depth * 8,
        game.options[OPT_LETTERBOX] == 0 ? "": " letterbox-by-design");

    adjust_sizes_for_resolution(loaded_game_file_version);
}

void engine_get_color_depths(ColorDepthOption &color_depths)
{
    if (game.color_depth == 1)
    {
        color_depths.Prime = 8;
        color_depths.Alternate = 8;
    }
    else if (debug_15bit_mode)
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
    else
    {
        color_depths.Prime = 32;
        color_depths.Alternate = 24;
    }
}

// Setup gfx driver callbacks and options
void engine_post_gfxmode_driver_setup()
{
    gfxDriver->SetCallbackForPolling(update_polled_stuff_if_runtime);
    gfxDriver->SetCallbackToDrawScreen(draw_screen_callback);
    gfxDriver->SetCallbackForNullSprite(GfxDriverNullSpriteCallback);
    gfxDriver->SetRenderOffset(play.viewport.Left, play.viewport.Top);
}

// Reset gfx driver callbacks
void engine_pre_gfxmode_driver_cleanup()
{
    gfxDriver->SetCallbackForPolling(NULL);
    gfxDriver->SetCallbackToDrawScreen(NULL);
    gfxDriver->SetCallbackForNullSprite(NULL);
}

// Setup virtual screen
void engine_post_gfxmode_screen_setup()
{
    _old_screen = BitmapHelper::GetScreenBitmap();
    virtual_screen = BitmapHelper::CreateBitmap(play.viewport.GetWidth(), play.viewport.GetHeight(),
        ScreenResolution.ColorDepth);
    virtual_screen->Clear();
    gfxDriver->SetMemoryBackBuffer(virtual_screen);
    SetVirtualScreen(virtual_screen);
    set_our_eip(-41);
}

// Release virtual screen
void engine_pre_gfxmode_screen_cleanup()
{
    SetVirtualScreen(NULL);
    gfxDriver->SetMemoryBackBuffer(NULL);
    delete _sub_screen;
    _sub_screen = NULL;
    delete virtual_screen;
    virtual_screen = NULL;
    // allegro_exit assumes screen is correct
    if (_old_screen)
        BitmapHelper::SetScreenBitmap( _old_screen );
}

// Setup color conversion parameters
void engine_setup_color_conversions()
{
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

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION) || defined(MAC_VERSION)
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

// Create blank (black) images used to repaint borders around game frame
void CreateBlankImage()
{
    // this is the first time that we try to use the graphics driver,
    // so it's the most likey place for a crash
    try
    {
        Bitmap *blank = BitmapHelper::CreateBitmap(16, 16, ScreenResolution.ColorDepth);
        blank = ReplaceBitmapWithSupportedFormat(blank);
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

void destroy_blank_image()
{
    if (blankImage)
        gfxDriver->DestroyDDB(blankImage);
    if (blankSidebarImage)
        gfxDriver->DestroyDDB(blankSidebarImage);
    blankImage = NULL;
    blankSidebarImage = NULL;
}

// Setup drawing modes and color conversions
void engine_post_gfxmode_draw_setup()
{
    engine_setup_color_conversions();
    if (gfxDriver->HasAcceleratedStretchAndFlip()) 
    {
        walkBehindMethod = DrawAsSeparateSprite;
        CreateBlankImage();
    }
    else
    {
        walkBehindMethod = DrawOverCharSprite;
    }
    init_invalid_regions(game.size.Height);
}

// Cleanup auxiliary drawing objects
void engine_pre_gfxmode_draw_cleanup()
{
    destroy_invalid_regions();
}

// Setup mouse control mode and graphic area
void engine_post_gfxmode_mouse_setup(const Size &init_desktop)
{
    DisplayMode dm = gfxDriver->GetDisplayMode();
    // Assign mouse control parameters.
    //
    // Whether mouse movement should be controlled by the engine - this is
    // determined based on related config option.
    const bool should_control_mouse = usetup.mouse_control == kMouseCtrl_Always ||
        usetup.mouse_control == kMouseCtrl_Fullscreen && !dm.Windowed;
    // Whether mouse movement control is supported by the engine - this is
    // determined on per platform basis. Some builds may not have such
    // capability, e.g. because of how backend library implements mouse utils.
    const bool can_control_mouse = platform->IsMouseControlSupported(dm.Windowed);
    // The resulting choice is made based on two aforementioned factors.
    const bool control_sens = should_control_mouse && can_control_mouse;
    if (control_sens)
    {
        Mouse::EnableControl(!dm.Windowed);
        if (usetup.mouse_speed_def == kMouseSpeed_CurrentDisplay)
        {
            Size cur_desktop;
            get_desktop_resolution(&cur_desktop.Width, &cur_desktop.Height);
            Mouse::SetSpeedUnit(Math::Max((float)cur_desktop.Width / (float)init_desktop.Width, (float)cur_desktop.Height / (float)init_desktop.Height));
        }
        Mouse::SetSpeed(usetup.mouse_speed);
    }
    Debug::Printf(kDbgMsg_Init, "Mouse control: %s, base: %f, speed: %f", Mouse::IsControlEnabled() ? "on" : "off",
        Mouse::GetSpeedUnit(), Mouse::GetSpeed());

    // The virtual->real conversion ratios could have change after new gfx mode is set,
    // thus we need to reset mouse graphic area and bounds
    Mouse::SetGraphicArea();
    Mouse::SetMoveLimit(Rect(play.mboundx1, play.mboundy1, play.mboundx2, play.mboundy2));
    // If auto lock option is set, lock mouse to the game window
    if (usetup.mouse_auto_lock && scsystem.windowed != 0)
        Mouse::TryLockToWindow();
}

// Reset mouse controls before changing gfx mode
void engine_pre_gfxmode_mouse_cleanup()
{
    // Always disable mouse control unlock mouse when shutting down gfx mode
    Mouse::DisableControl();
    Mouse::UnlockFromWindow();
}

// Fill in scsystem struct with display mode parameters
void engine_setup_scsystem_screen()
{
    DisplayMode dm = gfxDriver->GetDisplayMode();
    scsystem.width = game.size.Width;
    scsystem.height = game.size.Height;
    scsystem.coldepth = dm.ColorDepth;
    scsystem.windowed = dm.Windowed;
    scsystem.vsync = dm.Vsync;
    scsystem.viewport_width = divide_down_coordinate(play.viewport.GetWidth());
    scsystem.viewport_height = divide_down_coordinate(play.viewport.GetHeight());
}

void engine_post_gfxmode_setup(const Size &init_desktop)
{
    engine_setup_scsystem_screen();
    engine_post_gfxmode_driver_setup();
    engine_post_gfxmode_screen_setup();
    engine_post_gfxmode_draw_setup();
    engine_post_gfxmode_mouse_setup(init_desktop);
    
    // TODO: the only reason this call was put here is that it requires
    // "windowed" flag to be specified. Find out whether this function
    // has anything to do with graphics mode at all. It is quite possible
    // that we may split it into two functions, or remove parameter.
    platform->PostAllegroInit(scsystem.windowed != 0);

    video_on_gfxmode_changed();
}

void engine_pre_gfxmode_shutdown()
{
    engine_pre_gfxmode_mouse_cleanup();
    engine_pre_gfxmode_draw_cleanup();
    engine_pre_gfxmode_screen_cleanup();
    engine_pre_gfxmode_driver_cleanup();
}

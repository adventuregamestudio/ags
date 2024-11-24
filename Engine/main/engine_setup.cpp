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
#include <SDL.h>
#include "core/platform.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/mouse.h"
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
#include "main/game_run.h"
#include "main/graphics_mode.h"
#include "main/engine_setup.h"
#include "media/video/video.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern ScriptSystem scsystem;
extern IGraphicsDriver *gfxDriver;

void engine_adjust_for_rotation_settings()
{
    switch (usetup.Rotation) {
        case ScreenRotation::kScreenRotation_Portrait:
            SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait PortraitUpsideDown");
            break;
        case ScreenRotation::kScreenRotation_Landscape:
            SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
            break;
        case kScreenRotation_Unlocked:
            // let the user rotate as wished. No adjustment needed.
        default:
            break;
    }
}

// Setup gfx driver callbacks and options
void engine_post_gfxmode_driver_setup()
{
    gfxDriver->SetCallbackOnSpriteEvt(GfxDriverSpriteEvtCallback);
}

// Reset gfx driver callbacks
void engine_pre_gfxmode_driver_cleanup()
{
    gfxDriver->SetCallbackOnSpriteEvt(nullptr);
    gfxDriver->SetMemoryBackBuffer(nullptr);

    release_drawobj_rendertargets();
}

// Setup color conversion parameters
void engine_setup_color_conversions(int /* display_color_depth */)
{
    // RGB shifts for Allegro's pixel data
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
    _rgb_r_shift_24 = 16;
    _rgb_g_shift_24 = 8;
    _rgb_b_shift_24 = 0;
    _rgb_r_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_b_shift_16 = 0;
    _rgb_r_shift_15 = 10;
    _rgb_g_shift_15 = 5;
    _rgb_b_shift_15 = 0;

    set_color_conversion(COLORCONV_MOST | COLORCONV_EXPAND_256);
}

// Setup drawing modes and color conversions;
// they depend primarily on gfx driver capabilities and new color depth
void engine_post_gfxmode_draw_setup(const DisplayMode &dm)
{
    engine_setup_color_conversions(dm.ColorDepth); // CLNUP proabably remove
    init_draw_method();
}

// Cleanup auxiliary drawing objects
void engine_pre_gfxmode_draw_cleanup()
{
    dispose_draw_method();
}

// Setup mouse control mode and graphic area
void engine_post_gfxmode_mouse_setup(const Size &init_desktop)
{
    // Assign mouse control parameters.
    //
    // NOTE that we setup speed and other related properties regardless of
    // whether mouse control was requested because it may be enabled later.
    Mouse::SetSpeedUnit(1.f);
    if (usetup.MouseSpeedDef == kMouseSpeed_CurrentDisplay)
    {
        Size cur_desktop;
        if (sys_get_desktop_resolution(cur_desktop.Width, cur_desktop.Height) == 0)
            Mouse::SetSpeedUnit(std::max((float)cur_desktop.Width / (float)init_desktop.Width,
            (float)cur_desktop.Height / (float)init_desktop.Height));
    }

    Mouse_EnableControl(usetup.MouseCtrlEnabled);
    Debug::Printf(kDbgMsg_Info, "Mouse speed control: %s, unit: %f, user value: %f",
        usetup.MouseCtrlEnabled ? "enabled" : "disabled", Mouse::GetSpeedUnit(), Mouse::GetSpeed());
    Mouse::SetTouch2MouseMode(usetup.TouchEmulateMouse, usetup.TouchMotionRelative, usetup.MouseSpeed);
    Debug::Printf(kDbgMsg_Info, "Touch-to-mouse motion mode: %s",
        usetup.TouchMotionRelative ? "relative" : "absolute");

    on_coordinates_scaling_changed();

    // If auto lock option is set, lock mouse to the game window
    if (usetup.MouseAutoLock && scsystem.windowed != 0)
        Mouse::TryLockToWindow();
}

// Reset mouse controls before changing gfx mode
void engine_pre_gfxmode_mouse_cleanup()
{
    // Always disable mouse control and unlock mouse when releasing down gfx mode
    Mouse::SetMovementControl(false);
    Mouse::UnlockFromWindow();
}

// Fill in scsystem struct with display mode parameters
void engine_setup_scsystem_screen(const DisplayMode &dm)
{
    scsystem.windowed = dm.IsWindowed();
    scsystem.vsync = dm.Vsync;
}

void engine_post_gfxmode_setup(const Size &init_desktop, const DisplayMode &old_dm)
{
    DisplayMode dm = gfxDriver->GetDisplayMode();
    // If color depth has changed (or graphics mode was inited for the
    // very first time), we also need to recreate bitmaps
    bool has_driver_changed = old_dm.ColorDepth != dm.ColorDepth;

    engine_setup_scsystem_screen(dm);
    engine_post_gfxmode_driver_setup();
    if (has_driver_changed)
    {
        engine_post_gfxmode_draw_setup(dm);
    }
    engine_post_gfxmode_mouse_setup(init_desktop);

    // reset multitasking (may be overridden by the current display mode)
    SetMultitasking(usetup.RunInBackground);

    invalidate_screen();
}

void engine_pre_gfxmode_release()
{
    engine_pre_gfxmode_mouse_cleanup();
    engine_pre_gfxmode_driver_cleanup();
}

void engine_pre_gfxsystem_shutdown()
{
    engine_pre_gfxmode_release();
    engine_pre_gfxmode_draw_cleanup();
}

void on_coordinates_scaling_changed()
{
    // Reset mouse graphic area and bounds
    Mouse::UpdateGraphicArea();
    // If mouse bounds do not have valid values yet, then limit cursor to viewport
    if (play.mbounds.IsEmpty())
        Mouse::SetMoveLimit(play.GetMainViewport());
    else
        Mouse::SetMoveLimit(play.mbounds);
}

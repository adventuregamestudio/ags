//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// MOUSELIBW32.CPP
//
// Library of mouse functions for graphics and text mode
//
// (c) 1994 Chris Jones
// Win32 (allegro) update (c) 1999 Chris Jones
//
//=============================================================================
#include "device/mousew32.h"
#include <SDL.h>
#include "ac/gamestate.h"
#include "ac/sys_events.h"
#include "debug/out.h"
#include "gfx/bitmap.h"
#include "main/graphics_mode.h"
#include "platform/base/sys_main.h"

using namespace AGS::Common;
using namespace AGS::Engine;


char currentcursor = 0;
// virtual mouse cursor coordinates
int mousex = 0, mousey = 0, numcurso = -1, hotx = 0, hoty = 0;
// real mouse coordinates and bounds (in window coords)
static int real_mouse_x = 0, real_mouse_y = 0;
static int boundx1 = 0, boundx2 = 99999, boundy1 = 0, boundy2 = 99999;
char ignore_bounds = 0;
extern char alpha_blend_cursor ;
Bitmap *mousecurs[MAXCURSORS];
extern RGB palette[256];
extern volatile bool switched_away;

namespace Mouse
{
    // Tells whether mouse was locked to the game window
    bool LockedToWindow = false;

    // Screen rectangle, in which the mouse movement is controlled by engine
    Rect  ControlRect;
    // Mouse control enabled flag
    bool  ControlEnabled = false;
    // Mouse speed value provided by user
    float SpeedVal = 1.f;
    // Mouse speed unit
    float SpeedUnit = 1.f;
    // Actual speed factor (cached)
    float Speed = 1.f;

    // Converts real window coordinates to native game coords
    void WindowToGame(int &x, int &y);
    // Sets mouse position in system coordinates, syncs with the real mouse cursor
    void SetSysPosition(int x, int y);
}

void Mouse::Poll()
{
    // TODO: [sonneveld] find out where mgetgraphpos is needed, are events polled before that?
    sys_evt_process_pending();

    if (switched_away)
        return;

    // Save absolute cursor coordinates provided by system
    // NOTE: relative motion and the speed factor should already be applied by SDL2 or our custom devices.
    real_mouse_x = Math::Clamp((int)sys_mouse_x, Mouse::ControlRect.Left, Mouse::ControlRect.Right);
    real_mouse_y = Math::Clamp((int)sys_mouse_y, Mouse::ControlRect.Top, Mouse::ControlRect.Bottom);

    // Set new in-game cursor position, convert to the in-game logic coordinates
    mousex = real_mouse_x;
    mousey = real_mouse_y;
    if (!ignore_bounds &&
        // When applying script bounds we only do so while cursor is inside game viewport
        Mouse::ControlRect.IsInside(mousex, mousey) &&
        (mousex < boundx1 || mousey < boundy1 || mousex > boundx2 || mousey > boundy2))
    {
        mousex = Math::Clamp(mousex, boundx1, boundx2);
        mousey = Math::Clamp(mousey, boundy1, boundy2);
        Mouse::SetSysPosition(mousex, mousey);
    }
    // Convert to virtual coordinates
    Mouse::WindowToGame(mousex, mousey);
}

void Mouse::SetSysPosition(int x, int y)
{
    sys_mouse_x = x;
    sys_mouse_y = y;
    real_mouse_x = x;
    real_mouse_y = y;
    sys_window_set_mouse(real_mouse_x, real_mouse_y);
}

void Mouse::SetHotspot(int x, int y)
{
    hotx = x;
    hoty = y;
}

int Mouse::GetButtonCount()
{
    // TODO: can SDL tell number of available/supported buttons at all, or whether mouse is present?
    // this is not that critical, but maybe some game devs would like to detect if player has or not a mouse.
    return 3; // SDL *theoretically* support 3 mouse buttons, but that does not mean they are physically present...
}

void Mouse::WindowToGame(int &x, int &y)
{
    x = GameScaling.X.UnScalePt(x) - play.GetMainViewport().Left;
    y = GameScaling.Y.UnScalePt(y) - play.GetMainViewport().Top;
}

void Mouse::UpdateGraphicArea()
{
    Mouse::ControlRect = GameScaling.ScaleRange(play.GetMainViewport());
    Debug::Printf("Mouse cursor graphic area: (%d,%d)-(%d,%d) (%dx%d)",
        Mouse::ControlRect.Left, Mouse::ControlRect.Top, Mouse::ControlRect.Right, Mouse::ControlRect.Bottom,
        Mouse::ControlRect.GetWidth(), Mouse::ControlRect.GetHeight());
}

void Mouse::SetMoveLimit(const Rect &r)
{
    Rect src_r = OffsetRect(r, play.GetMainViewport().GetLT());
    Rect dst_r = GameScaling.ScaleRange(src_r);
    boundx1 = dst_r.Left;
    boundy1 = dst_r.Top;
    boundx2 = dst_r.Right;
    boundy2 = dst_r.Bottom;
}

void Mouse::SetPosition(const Point &p)
{
    Mouse::SetSysPosition(
        GameScaling.X.ScalePt(p.X + play.GetMainViewport().Left),
        GameScaling.Y.ScalePt(p.Y + play.GetMainViewport().Top));
}

bool Mouse::IsLockedToWindow()
{
    return LockedToWindow;
}

bool Mouse::TryLockToWindow()
{
    if (!LockedToWindow)
        LockedToWindow = sys_window_lock_mouse(true);
    return LockedToWindow;
}

void Mouse::UnlockFromWindow()
{
    sys_window_lock_mouse(false);
    LockedToWindow = false;
}

void Mouse::SetMovementControl(bool on)
{
#if defined (SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE)
    ControlEnabled = on;
    SDL_SetRelativeMouseMode(static_cast<SDL_bool>(on));
    if (on)
        SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE, String::FromFormat("%.2f", Mouse::Speed).GetCStr());
    else
        SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE, "");
#else
    ControlEnabled = false;
    if (on)
        Debug::Printf(kDbgMsg_Warn, "WARNING: SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE not supported, mouse control can't be enabled");
#endif
    ags_clear_mouse_movement();
}

bool Mouse::IsControlEnabled()
{
    return ControlEnabled;
}

void Mouse::SetTouch2MouseMode(TouchMouseEmulation mode, bool relative, float speed)
{
    ags_touch_set_mouse_emulation(mode, relative, speed);
}

void Mouse::SetSpeedUnit(float f)
{
    SpeedUnit = f;
    Speed = SpeedVal / SpeedUnit;
}

float Mouse::GetSpeedUnit()
{
    return SpeedUnit;
}

void Mouse::SetSpeed(float speed)
{
    SpeedVal = std::max(0.f, speed);
    Speed = SpeedUnit * SpeedVal;
}

float Mouse::GetSpeed()
{
    return SpeedVal;
}

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
// real mouse coordinates and bounds
static int real_mouse_x = 0, real_mouse_y = 0;
static int boundx1 = 0, boundx2 = 99999, boundy1 = 0, boundy2 = 99999;
static int disable_mgetgraphpos = 0;
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
}

void mgetgraphpos()
{
    // TODO: review and possibly rewrite whole thing;
    // research what disable_mgetgraphpos does, and is this still necessary?

    // TODO: [sonneveld] find out where mgetgraphpos is needed, are events polled before that?
    sys_evt_process_pending();

    if (disable_mgetgraphpos)
    {
        // The cursor coordinates are provided from alternate source;
        // in this case we completely ignore actual cursor movement.
        if (!ignore_bounds &&
            // When applying script bounds we only do so while cursor is inside game viewport
            Mouse::ControlRect.IsInside(mousex, mousey) &&
            (mousex < boundx1 || mousey < boundy1 || mousex > boundx2 || mousey > boundy2))
        {
            mousex = Math::Clamp(mousex, boundx1, boundx2);
            mousey = Math::Clamp(mousey, boundy1, boundy2);
            msetgraphpos(mousex, mousey);
        }
        return;
    }

    if (!switched_away && Mouse::ControlEnabled)
    {
        // Use relative mouse movement; speed factor should already be applied by SDL in this mode
        int rel_x, rel_y;
        ags_mouse_get_relxy(rel_x, rel_y);
        real_mouse_x = Math::Clamp(real_mouse_x + rel_x, Mouse::ControlRect.Left, Mouse::ControlRect.Right);
        real_mouse_y = Math::Clamp(real_mouse_y + rel_y, Mouse::ControlRect.Top, Mouse::ControlRect.Bottom);
    }
    else
    {
        // Save real cursor coordinates provided by system
        real_mouse_x = Math::Clamp((int)sys_mouse_x, Mouse::ControlRect.Left, Mouse::ControlRect.Right);
        real_mouse_y = Math::Clamp((int)sys_mouse_y, Mouse::ControlRect.Top, Mouse::ControlRect.Bottom);
    }

    // Set new in-game cursor position
    mousex = real_mouse_x;
    mousey = real_mouse_y;

    if (!ignore_bounds &&
        // When applying script bounds we only do so while cursor is inside game viewport
        Mouse::ControlRect.IsInside(mousex, mousey) &&
        (mousex < boundx1 || mousey < boundy1 || mousex > boundx2 || mousey > boundy2))
    {
        mousex = Math::Clamp(mousex, boundx1, boundx2);
        mousey = Math::Clamp(mousey, boundy1, boundy2);
        msetgraphpos(mousex, mousey);
    }

    // Convert to virtual coordinates
    Mouse::WindowToGame(mousex, mousey);
}

void msetcursorlimit(int x1, int y1, int x2, int y2)
{
  boundx1 = x1;
  boundy1 = y1;
  boundx2 = x2;
  boundy2 = y2;
}

void msetgraphpos(int xa, int ya)
{
  real_mouse_x = xa;
  real_mouse_y = ya;
  sys_window_set_mouse(real_mouse_x, real_mouse_y);
}

void msethotspot(int xx, int yy)
{
  hotx = xx;  // mousex -= hotx; mousey -= hoty;
  hoty = yy;  // mousex += hotx; mousey += hoty;
}

int minstalled()
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
    msetcursorlimit(dst_r.Left, dst_r.Top, dst_r.Right, dst_r.Bottom);
}

void Mouse::SetPosition(const Point p)
{
    msetgraphpos(GameScaling.X.ScalePt(p.X + play.GetMainViewport().Left), GameScaling.Y.ScalePt(p.Y + play.GetMainViewport().Top));
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
        SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE, "1.0");
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

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

#include "core/platform.h"
#include "util/wgt2allg.h"
#include "ac/gamestate.h"
#include "ac/sys_events.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_util.h"
#include "main/graphics_mode.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "util/math.h"

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
extern color palette[256];
extern volatile bool switched_away;

namespace Mouse
{
    // Tells whether mouse was locked to the game window
    bool LockedToWindow = false;

    // Screen rectangle, in which the mouse movement is controlled by engine
    Rect  ControlRect;
    // Mouse control enabled flag
    bool  ControlEnabled = false;
    // Flag that tells whether the mouse must be forced to stay inside control rect
    bool  ConfineInCtrlRect = false;
    // Mouse speed value provided by user
    float SpeedVal = 1.f;
    // Mouse speed unit
    float SpeedUnit = 1.f;
    // Actual speed factor (cached)
    float Speed = 1.f;


    void AdjustPosition(int &x, int &y);
}

void mgetgraphpos()
{
    // TODO: review and possibly rewrite whole thing;
    // research what disable_mgetgraphpos does, and is this still necessary?
    // disable or update mouse speed control to sdl
    // (does sdl support mouse cursor speed? is it even necessary anymore?);

    // TODO: [sonneveld] find out where mgetgraphpos is needed, are events polled before that?
    sys_evt_process_pending();

    if (disable_mgetgraphpos)
    {
        // The cursor coordinates are provided from alternate source;
        // in this case we completely ignore actual cursor movement.
        if (!ignore_bounds &&
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
        // Control mouse movement by querying mouse mickeys (movement deltas)
        // and applying them to saved mouse coordinates.
        int mickey_x, mickey_y;
        get_mouse_mickeys(&mickey_x, &mickey_y);
        
        // Apply mouse speed
        int dx = Mouse::Speed * mickey_x;
        int dy = Mouse::Speed * mickey_y;

        //
        // Perform actual cursor update
        //---------------------------------------------------------------------
        // If the real cursor is inside the control rectangle (read - game window),
        // then apply sensitivity factors and adjust real cursor position
        if (Mouse::ControlRect.IsInside(real_mouse_x + dx, real_mouse_y + dy))
        {
            real_mouse_x += dx;
            real_mouse_y += dy;
            sys_window_set_mouse(real_mouse_x, real_mouse_y);
        }
        // Otherwise, if real cursor was moved outside the control rect, yet we
        // are required to confine cursor inside one, then adjust cursor position
        // to stay inside the rect's bounds.
        else if (Mouse::ConfineInCtrlRect)
        {
            real_mouse_x = Math::Clamp(real_mouse_x + dx, Mouse::ControlRect.Left, Mouse::ControlRect.Right);
            real_mouse_y = Math::Clamp(real_mouse_y + dy, Mouse::ControlRect.Top, Mouse::ControlRect.Bottom);
            sys_window_set_mouse(real_mouse_x, real_mouse_y);
        }
        // Lastly, if the real cursor is out of the control rect, simply add
        // actual movement to keep up with the system cursor coordinates.
        else
        {
            real_mouse_x += mickey_x;
            real_mouse_y += mickey_y;
        }

        // Do not update the game cursor if the real cursor is beyond the control rect
        if (!Mouse::ControlRect.IsInside(real_mouse_x, real_mouse_y))
            return;
    }
    else
    {
        // Save real cursor coordinates provided by system
        real_mouse_x = sys_mouse_x;
        real_mouse_y = sys_mouse_y;
    }

    // Set new in-game cursor position
    mousex = real_mouse_x;
    mousey = real_mouse_y;

    if (!ignore_bounds &&
        (mousex < boundx1 || mousey < boundy1 || mousex > boundx2 || mousey > boundy2))
    {
        mousex = Math::Clamp(mousex, boundx1, boundx2);
        mousey = Math::Clamp(mousey, boundy1, boundy2);
        msetgraphpos(mousex, mousey);
    }

    // Convert to virtual coordinates
    Mouse::AdjustPosition(mousex, mousey);
}

void msetcursorlimit(int x1, int y1, int x2, int y2)
{
  boundx1 = x1;
  boundy1 = y1;
  boundx2 = x2;
  boundy2 = y2;
}

static int hotxwas = 0, hotywas = 0;
void domouse(int str)
{
  int poow = mousecurs[currentcursor]->GetWidth();
  int pooh = mousecurs[currentcursor]->GetHeight();
  int smx = mousex - hotxwas, smy = mousey - hotywas;
  const Rect &viewport = play.GetMainViewport();

  mgetgraphpos();

  // temporarily adjust mousex/y. Original values returned at end of func.
  mousex -= hotx;
  mousey -= hoty;

  if (mousex + poow >= viewport.GetWidth())
    poow = viewport.GetWidth() - mousex;

  if (mousey + pooh >= viewport.GetHeight())
    pooh = viewport.GetHeight() - mousey;

  mousex += hotx;
  mousey += hoty;
  hotxwas = hotx;
  hotywas = hoty;
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

void Mouse::AdjustPosition(int &x, int &y)
{
    x = GameScaling.X.UnScalePt(x) - play.GetMainViewport().Left;
    y = GameScaling.Y.UnScalePt(y) - play.GetMainViewport().Top;
}

void Mouse::SetGraphicArea()
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

void Mouse::EnableControl(bool confine)
{
    ControlEnabled = true;
    ConfineInCtrlRect = confine;
}

void Mouse::DisableControl()
{
    ControlEnabled = false;
    ConfineInCtrlRect = false;
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
    SpeedVal = Math::Max(0.f, speed);
    Speed = SpeedUnit * SpeedVal;
}

float Mouse::GetSpeed()
{
    return SpeedVal;
}

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
#include "ac/sys_events.h"
#include "util/geometry.h"


namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS; // FIXME later

namespace Mouse
{
    // Tells the number of supported mouse buttons
    int  GetButtonCount();

    // Get if mouse is locked to the game window
    bool IsLockedToWindow();
    // Try locking mouse to the game window
    bool TryLockToWindow();
    // Unlock mouse from the game window
    void UnlockFromWindow();

    // Enable or disable mouse movement control
    void SetMovementControl(bool on);
    // Tell if the mouse movement control is enabled
    bool IsControlEnabled();
    // Set the touch2mouse motion mode: absolute/relative
    void SetTouch2MouseMode(TouchMouseEmulation mode, bool relative, float speed);
    // Set base speed factor, which would serve as a mouse speed unit
    void SetSpeedUnit(float f);
    // Get base speed factor
    float GetSpeedUnit();
    // Set speed factors
    void SetSpeed(float speed);
    // Get speed factor
    float GetSpeed();

    // Updates limits of the area inside which the standard OS cursor is not shown;
    // uses game's main viewport (in native coordinates) to calculate real area on screen
    void UpdateGraphicArea();
    // Limits the area where the game cursor can move on virtual screen;
    // parameter must be in native game coordinates
    void SetMoveLimit(const Rect &r);

    // Polls the cursor position, updates mousex, mousey
    void Poll();
    // Set actual OS cursor position on screen; in native game coordinates
    void SetPosition(const Point &p);
    // Sets the relative position of the cursor's hotspot, in native pixels
    void SetHotspot(int x, int y);
}


extern int mousex, mousey;
extern int hotx, hoty;
extern char currentcursor;

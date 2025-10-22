//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_AC__MOUSE_H
#define __AGS_EE_AC__MOUSE_H

#include <memory>
#include "ac/dynobj/scriptmouse.h"

void Mouse_SetVisible(int isOn);
int Mouse_GetVisible();
int Mouse_GetModeGraphic(int curs);
void Mouse_ChangeModeView(int curs, int newview, int delay);
// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void Mouse_SetPosition (int newx, int newy);
int Mouse_GetCursorMode();
void Mouse_SetNextCursor ();
// permanently change cursor graphic
void Mouse_ChangeCursorGraphic (int curs, int newslot);
void Mouse_ChangeCursorHotspot (int curs, int x, int y);
int  Mouse_IsButtonDown(int which);
void Mouse_SetBounds (int x1, int y1, int x2, int y2);
void Mouse_Refresh();
// mouse cursor functions:
// set_mouse_cursor: changes visual appearance to specified cursor
void Mouse_SetCursor(int newcurs);
int  Mouse_GetCursor();
// set_default_cursor: resets visual appearance to current mode (walk, look, etc);
void Mouse_SetDefaultCursor();
// set_cursor_mode: changes mode and appearance
void Mouse_SetCursorMode(int newmode);
void Mouse_EnableCursorMode(int modd);
void Mouse_DisableCursorMode(int modd);

void Mouse_SaveCursorForLocationChange();

// Try to enable or disable mouse speed control by the engine
void Mouse_EnableControl(bool on);
void Mouse_SimulateClick(int button_id);

//=============================================================================

void set_mouse_cursor(int newcurs, bool force_update = false);
void set_cursor_mode(int newmode);
void set_default_cursor();
void enable_cursor_mode(int modd);
void disable_cursor_mode(int modd);

void update_script_mouse_coords();
void update_inv_cursor(int invnum);
void set_new_cursor_graphic (int spriteslot);
int find_next_enabled_cursor(int startwith);
int find_previous_enabled_cursor(int startwith);

namespace AGS { namespace Common { class Bitmap; } }

// A collection of properties depicting the current mouse cursor look
// TODO: this looks quite like the limited ScreenOverlay, merge these classes;
// store cursor's generated graphic as a dynamic sprite too
struct CursorGraphicState
{
public:
    // Gets actual cursor's image, whether owned by cursor or by a sprite reference
    AGS::Common::Bitmap *GetImage() const;
    // Get sprite id, or 0 if none set
    int GetSpriteNum() const { return _sprnum; }
    // Assigns an exclusive image to the cursor state
    void SetImage(std::unique_ptr<AGS::Common::Bitmap> pic);
    // Assigns a shared sprite to the cursor state
    void SetSpriteNum(int sprnum);
    // Tells if the cursor has graphically changed recently
    bool HasChanged() const { return _hasChanged; }
    // Manually marks GUI as graphically changed
    void MarkChanged() { _hasChanged = true; }
    // Clears changed flag
    void ClearChanged() { _hasChanged = false; }

private:
    void ResetImage();

    // Current cursor sprite ID, may be a >= 0 for a sprite or -1 for a generated bitmap
    // TODO: refactor, replace these with ObjTexture, and move to draw.cpp
    int  _sprnum = -1;
    // Generated cursor image, used to create a modified cursor with
    // hotspot crosshair drawn over it, or when an invalid pic index set.
    // TODO: store this as dynamic sprite instead, see generated sprites for text overlays.
    std::unique_ptr<AGS::Common::Bitmap> _genImage;
    bool _hasChanged = false;
};

extern ScriptMouse scmouse;
extern CursorGraphicState cursor_gstate;

extern int mousex, mousey;
extern int ignore_bounds;
extern int cur_mode;
extern int cur_cursor;

#endif // __AGS_EE_AC__MOUSE_H


//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__MOUSE_H
#define __AGS_EE_AC__MOUSE_H

#define DOMOUSE_NOCURSOR 5
// are these mouse buttons? ;/
// note: also defined in ac_cscidialog as const ints
#define NONE -1
#define LEFT  0
#define RIGHT 1

#define MOUSE_MAX_Y divide_down_coordinate(vesa_yres)

void Mouse_SetVisible(int isOn);
int Mouse_GetVisible();
int Mouse_GetModeGraphic(int curs);
void Mouse_ChangeModeView(int curs, int newview);
// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void SetMousePosition (int newx, int newy);
int GetCursorMode();
void SetNextCursor ();
// permanently change cursor graphic
void ChangeCursorGraphic (int curs, int newslot);
void ChangeCursorHotspot (int curs, int x, int y);
int IsButtonDown(int which);
void SetMouseBounds (int x1, int y1, int x2, int y2);
void RefreshMouse();
// mouse cursor functions:
// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs);
// set_default_cursor: resets visual appearance to current mode (walk, look, etc);
void set_default_cursor();
// set_cursor_mode: changes mode and appearance
void set_cursor_mode(int newmode);
void enable_cursor_mode(int modd);
void disable_cursor_mode(int modd);


//=============================================================================

int GetMouseCursor();
void update_script_mouse_coords();
void update_inv_cursor(int invnum);
void update_cached_mouse_cursor();
void set_new_cursor_graphic (int spriteslot);
int find_next_enabled_cursor(int startwith);


#endif // __AGS_EE_AC__MOUSE_H

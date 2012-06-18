#ifndef __AC_MOUSE_H
#define __AC_MOUSE_H

#include "acrun/ac_scriptobject.h"

#define DOMOUSE_NOCURSOR 5
// are these mouse buttons? ;/
// note: also defined in ac_cscidialog as const ints
#define NONE -1
#define LEFT  0
#define RIGHT 1

void update_script_mouse_coords();
void HideMouseCursor ();
void ShowMouseCursor ();
// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void Mouse_SetVisible(int isOn);
int Mouse_GetVisible();
#define MOUSE_MAX_Y divide_down_coordinate(vesa_yres);
void SetMouseBounds (int x1, int y1, int x2, int y2);
void update_inv_cursor(int invnum);

void update_cached_mouse_cursor();
void set_new_cursor_graphic (int spriteslot);
// mouse cursor functions:
// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs);
// set_default_cursor: resets visual appearance to current mode (walk, look, etc);
void set_default_cursor();
// permanently change cursor graphic
void ChangeCursorGraphic (int curs, int newslot);
int Mouse_GetModeGraphic(int curs);
void ChangeCursorHotspot (int curs, int x, int y);
void Mouse_ChangeModeView(int curs, int newview);
int find_next_enabled_cursor(int startwith);
void SetNextCursor ();
// set_cursor_mode: changes mode and appearance
void set_cursor_mode(int newmode);
void enable_cursor_mode(int modd);
void disable_cursor_mode(int modd);
void RefreshMouse();
void SetMousePosition (int newx, int newy);
int GetCursorMode();
int GetMouseCursor();


extern int cur_mode,cur_cursor;

extern ScriptMouse scmouse;
extern int mouse_frame,mouse_delay;
extern int lastmx,lastmy;


#endif // __AC_MOUSE_H

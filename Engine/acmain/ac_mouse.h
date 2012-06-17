#ifndef __AC_MOUSE_H
#define __AC_MOUSE_H

#include "acrun/ac_scriptobject.h"

#define DOMOUSE_NOCURSOR 5
// are these mouse buttons? ;/
// note: also defined in ac_cscidialog as const ints
#define NONE -1
#define LEFT  0
#define RIGHT 1

int GetCursorMode();
void set_cursor_mode(int newmode);
void SetNextCursor ();
void update_inv_cursor(int invnum);
void set_mouse_cursor(int newcurs);
void set_new_cursor_graphic (int spriteslot);
void set_default_cursor();


extern int cur_mode,cur_cursor;

extern ScriptMouse scmouse;
extern int mouse_frame,mouse_delay;
extern int lastmx,lastmy;


#endif // __AC_MOUSE_H

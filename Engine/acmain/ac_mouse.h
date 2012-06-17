#ifndef __AC_MOUSE_H
#define __AC_MOUSE_H

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


extern int cur_mode,cur_cursor;



#endif // __AC_MOUSE_H

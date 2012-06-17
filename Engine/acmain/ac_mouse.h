#ifndef __AC_MOUSE_H
#define __AC_MOUSE_H

int GetCursorMode();
void set_cursor_mode(int newmode);
void SetNextCursor ();
void update_inv_cursor(int invnum);


extern int cur_mode,cur_cursor;



#endif // __AC_MOUSE_H

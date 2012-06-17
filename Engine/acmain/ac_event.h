#ifndef __AC_EVENT_H
#define __AC_EVENT_H

// parameters to run_on_event
#define GE_LEAVE_ROOM 1
#define GE_ENTER_ROOM 2
#define GE_MAN_DIES   3
#define GE_GOT_SCORE  4
#define GE_GUI_MOUSEDOWN 5
#define GE_GUI_MOUSEUP   6
#define GE_ADD_INV       7
#define GE_LOSE_INV      8
#define GE_RESTORE_GAME  9

extern int in_enters_screen,done_es_error;
extern int in_leaves_screen;

void run_on_event (int evtype, int wparam);

#endif // __AC_EVENT_H
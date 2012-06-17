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

#define MAXEVENTS 15

#define EV_TEXTSCRIPT 1
#define EV_RUNEVBLOCK 2
#define EV_FADEIN     3
#define EV_IFACECLICK 4
#define EV_NEWROOM    5
#define TS_REPEAT   1
#define TS_KEYPRESS 2
#define TS_MCLICK   3
#define EVB_HOTSPOT 1
#define EVB_ROOM    2

struct EventHappened {
    int type;
    int data1,data2,data3;
    int player;
};

extern int in_enters_screen,done_es_error;
extern int in_leaves_screen;

extern EventHappened event[MAXEVENTS+1];
extern int numevents;

extern char*evblockbasename;
extern int evblocknum;

void run_on_event (int evtype, int wparam);
void setevent(int evtyp,int ev1=0,int ev2=-1000,int ev3=0);
void run_event_block_inv(int invNum, int aaa);

void GiveScore(int amnt) ;

#endif // __AC_EVENT_H
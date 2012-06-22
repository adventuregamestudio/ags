#ifndef __AC_EVENTBLOCK_H
#define __AC_EVENTBLOCK_H

// [IKM] 2012-06-22:
// The EventBlock struct seem to had been used somewhen in the past,
// but no longer is.
//
#ifdef UNUSED_CODE
#define MAXCOMMANDS 8
struct EventBlock {
    int   list[MAXCOMMANDS];
    int   respond[MAXCOMMANDS];
    int   respondval[MAXCOMMANDS];
    int   data[MAXCOMMANDS];
    int   numcmd;
    short score[MAXCOMMANDS];
};

extern void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr);
#endif // UNUSED_CODE

#endif // __AC_EVENTBLOCK_H
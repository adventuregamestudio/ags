#ifndef __AC_EVENTBLOCK_H
#define __AC_EVENTBLOCK_H

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

#endif // __AC_EVENTBLOCK_H
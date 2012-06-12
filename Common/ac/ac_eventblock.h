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

#endif // __AC_EVENTBLOCK_H
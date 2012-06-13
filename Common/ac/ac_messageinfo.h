#ifndef __AC_MESSAGEINFO_H
#define __AC_MESSAGEINFO_H

#include "ac/ac_defines.h" // PCKD

#pragma pack(1)

#define MSG_DISPLAYNEXT 1 // supercedes using alt-200 at end of message
#define MSG_TIMELIMIT   2
struct MessageInfo {
    char  displayas  PCKD; // 0 = normal window, 1 = as speech
    char  flags      PCKD; // combination of MSG_xxx flags

    void ReadFromFile(FILE *fp);
};

#pragma pack()

#endif // __AC_MESSAGEINFO_H
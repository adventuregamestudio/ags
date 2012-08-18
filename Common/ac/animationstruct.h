#ifndef __AC_ANIMATIONSTRUCT_H
#define __AC_ANIMATIONSTRUCT_H

#define AE_WAITFLAG   0x80000000
#define MAXANIMSTAGES 10
struct AnimationStruct {
    int   x, y;
    int   data;
    int   object;
    int   speed;
    char  action;
    char  wait;
    AnimationStruct() { action = 0; object = 0; wait = 1; speed = 5; }
};

struct FullAnimation {
    AnimationStruct stage[MAXANIMSTAGES];
    int             numstages;
    FullAnimation() { numstages = 0; }
};

#endif // __AC_ANIMATIONSTRUCT_H
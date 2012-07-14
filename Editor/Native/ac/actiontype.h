#ifndef __AC_ACTIONTYPE_H
#define __AC_ACTIONTYPE_H

#include "ac/interaction.h" // constants

#define NUM_ACTION_TYPES 48
#define ARG_TYPE_INT 1
#define ARG_TYPE_INV 2
#define ARG_TYPE_MSG 3
#define ARG_TYPE_CHAR 4
#define ARG_TYPE_BOOL 5
#define ARG_TYPE_VARONLY 6  // must be variable, no literal values
#define AFLG_COND      1
#define AFLG_RUNSCRIPT 2
#define AFLG_INVCHECK  4
#define AFLG_MESSAGE   8

struct ActionTypes {
    char  name[80];
    short flags;
    char  numArgs;
    char  argTypes[MAX_ACTION_ARGS];
    char  argNames[MAX_ACTION_ARGS][25];
    char  description[200];
    char  textscript[80];
};

extern ActionTypes actions[NUM_ACTION_TYPES];



#endif // __AC_ACTIONTYPE_H
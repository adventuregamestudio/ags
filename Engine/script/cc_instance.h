/*
** 'C'-style script compiler
** Copyright (C) 2000-2001, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __CC_INSTANCE_H
#define __CC_INSTANCE_H

#include "script/script_common.h"
#include "script/cc_script.h"  // ccScript

#define INSTF_SHAREDATA   1
#define INSTF_ABORTED     2
#define INSTF_FREE        4
#define INSTF_RUNNING     8   // set by main code to confirm script isn't stuck
#define CC_STACK_SIZE     (1000 * sizeof(long))
#define MAX_CALL_STACK    100

// Running instance of the script
struct ccInstance
{
    long flags;
    char *globaldata;
    long globaldatasize;
    unsigned long *code;
    ccInstance *runningInst;  // might point to another instance if in far call
    long codesize;
    char *strings;
    long stringssize;
    char **exportaddr;  // real pointer to export
    char *stack;
    long stacksize;
    long registers[CC_NUM_REGISTERS];
    long pc;                        // program counter
    long line_number;               // source code line number
    ccScript *instanceof;
    long callStackLineNumber[MAX_CALL_STACK];
    long callStackAddr[MAX_CALL_STACK];
    ccInstance *callStackCodeInst[MAX_CALL_STACK];
    int  callStackSize;
    int  loadedInstanceId;
    int  returnValue;

#if defined(AGS_64BIT)
    // 64 bit: Variables to keep track of the size of the variables on the stack.
    // This is necessary because the compiled code accesses values on the stack with
    // absolute offsets that don't take into account the 8 byte long pointers on
    // 64 bit systems. These variables help with rewriting the offsets for the
    // modified stack.
    int stackSizes[CC_STACK_SIZE];
    int stackSizeIndex;
#endif
};

// returns the currently executing instance, or NULL if none
extern ccInstance *ccGetCurrentInstance(void);
void ccGetCallStack(ccInstance *inst, char *buffer, int maxLines);

#endif // __CC_INSTANCE_H

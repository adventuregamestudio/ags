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

#ifndef __CS_RUNTIME_H
#define __CS_RUNTIME_H

#include "cc_script.h"      // ccScript
#include "cc_instance.h"    // ccInstance

// ************ SCRIPT LOADING AND RUNNING FUNCTIONS ************

// give the script access to a variable or function in your program
extern void ccAddExternalSymbol(char *, void *);
// remove the script access to a variable or function in your program
extern void ccRemoveExternalSymbol(char *);
// create a runnable instance of the supplied script
extern ccInstance *ccCreateInstance(ccScript *);
// create a runnable instance of the same script, sharing global memory
extern ccInstance *ccForkInstance(ccInstance *);
// free the memory associated with the instance
extern void ccFreeInstance(ccInstance *);
// get the address of an exported variable in the script
extern char *ccGetSymbolAddr(ccInstance *, char *);
// call an exported function in the script (3rd arg is number of params)
extern int ccCallInstance(ccInstance *, char *, long, ...);
// specifies that when the current function returns to the script, it
// will stop and return from CallInstance
extern void ccAbortInstance(ccInstance *);
// aborts instance, then frees the memory later when it is done with
extern void ccAbortAndDestroyInstance(ccInstance *);
// returns the currently executing instance, or NULL if none
extern ccInstance *ccGetCurrentInstance(void);
// removes all external symbols, allowing you to start from scratch
extern void ccRemoveAllSymbols();
// Set the number of while loop iterations that aborts the script
extern void ccSetScriptAliveTimer (int);
// reset the current while loop counter
extern void ccNotifyScriptStillAlive ();

// DEBUG HOOK
typedef void (*new_line_hook_type) (ccInstance *, int);
extern void ccSetDebugHook(new_line_hook_type jibble);

#endif
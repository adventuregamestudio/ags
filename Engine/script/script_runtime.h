/*
C-Script run-time interpreter (c) 2001 Chris Jones

You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
when compiling this, or strange results can happen.

There is a problem with importing functions on 16-bit compilers: the
script system assumes that all parameters are passed as 4 bytes, which
ints are not on 16-bit systems. Be sure to define all parameters as longs,
or join the 21st century and switch to DJGPP or Visual C++.

This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/

#ifndef __CS_RUNTIME_H
#define __CS_RUNTIME_H

#include "script/cc_script.h"      // ccScript
#include "script/cc_instance.h"    // ccInstance

// ************ SCRIPT LOADING AND RUNNING FUNCTIONS ************

// give the script access to a variable or function in your program
//extern void ccAddExternalSymbol(char *, void *);
extern bool ccAddExternalData(const char *name, void *ptr);
extern bool ccAddExternalStaticFunction(const char *name, void *ptr);
extern bool ccAddExternalObject(const char *name, void *ptr);
extern bool ccAddExternalObjectFunction(const char *name, void *ptr);
extern bool ccAddExternalScriptSymbol(const char *name, void *ptr, ccInstance *inst);
// remove the script access to a variable or function in your program
extern void ccRemoveExternalSymbol(const char *);
// removes all external symbols, allowing you to start from scratch
extern void ccRemoveAllSymbols();

// get the address of an exported variable in the script
extern void *ccGetSymbolAddress(char *namof);

// DEBUG HOOK
typedef void (*new_line_hook_type) (ccInstance *, int);
extern void ccSetDebugHook(new_line_hook_type jibble);
#endif

// Set the number of while loop iterations that aborts the script
extern void ccSetScriptAliveTimer (int);
// reset the current while loop counter
extern void ccNotifyScriptStillAlive ();
extern int call_function(long addr, int numparm, long *parms, int offset);

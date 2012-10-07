//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// C-Script run-time interpreter (c) 2001 Chris Jones
//
// You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
// when compiling this, or strange results can happen.
//
// There is a problem with importing functions on 16-bit compilers: the
// script system assumes that all parameters are passed as 4 bytes, which
// ints are not on 16-bit systems. Be sure to define all parameters as longs,
// or join the 21st century and switch to DJGPP or Visual C++.
//
//=============================================================================

#ifndef __CS_RUNTIME_H
#define __CS_RUNTIME_H

#include "script/cc_script.h"      // ccScript
#include "script/cc_instance.h"    // ccInstance

// ************ SCRIPT LOADING AND RUNNING FUNCTIONS ************

// give the script access to a variable or function in your program
extern void ccAddExternalSymbol(char *, void *);
// remove the script access to a variable or function in your program
extern void ccRemoveExternalSymbol(char *);
// removes all external symbols, allowing you to start from scratch
extern void ccRemoveAllSymbols();

// create a runnable instance of the supplied script
extern ccInstance *ccCreateInstance(ccScript *);
// create a runnable instance of the same script, sharing global memory
extern ccInstance *ccForkInstance(ccInstance *);
// free the memory associated with the instance
extern void ccFreeInstance(ccInstance *);

// get the address of an exported variable in the script
extern char *ccGetSymbolAddr(ccInstance *, char *);
extern void *ccGetSymbolAddress(char *namof);

// call an exported function in the script (3rd arg is number of params)
extern int ccCallInstance(ccInstance *, char *, long, ...);
// specifies that when the current function returns to the script, it
// will stop and return from CallInstance
extern void ccAbortInstance(ccInstance *);
// aborts instance, then frees the memory later when it is done with
extern void ccAbortAndDestroyInstance(ccInstance *);

// Set the number of while loop iterations that aborts the script
extern void ccSetScriptAliveTimer (int);
// reset the current while loop counter
extern void ccNotifyScriptStillAlive ();

// DEBUG HOOK
typedef void (*new_line_hook_type) (ccInstance *, int);
extern void ccSetDebugHook(new_line_hook_type jibble);

// changes all pointer variables (ie. strings) to have the relative address, to allow
// the data segment to be saved to disk
void ccFlattenGlobalData(ccInstance * cinst);
// restores the pointers after a save
void ccUnFlattenGlobalData(ccInstance * cinst);

#endif

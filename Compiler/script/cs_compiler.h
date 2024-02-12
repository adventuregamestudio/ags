//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// C-style script compiler.
//
//=============================================================================
#ifndef __CS_COMPILER_H
#define __CS_COMPILER_H

#include "script/cc_script.h"  // ccScript

// ********* SCRIPT COMPILATION FUNCTIONS **************
// add a script that will be compiled as a header into every compilation
// 'name' is the name of the header, used in error reports
// (only the pointer is stored so don't free the memory)
extern int ccAddDefaultHeader(const char *script, const char *name);
// don't compile any headers into the compilation
extern void ccRemoveDefaultHeaders(void);

// define a macro which will affect all compilations
extern void ccDefineMacro(const char *macro, const char *definition);
// clear all predefined macros
extern void ccClearAllMacros();

// set version for use with #ifversion macros
extern void ccSetSoftwareVersion(const char *version);

// compile the script supplied, returns NULL on failure
extern ccScript *ccCompileText(const char *script, const char *scriptName);

extern const char *ccSoftwareVersion;

#endif // __CS_COMPILER_H
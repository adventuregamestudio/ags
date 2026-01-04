//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Script options and error reporting.
//
//=============================================================================
#ifndef __CC_ERROR_H
#define __CC_ERROR_H

#include "util/string.h"

// FIXME: SCOPT_AUTOIMPORT and SCOPT_DEBUGRUN are runtime only options
// remove them from this list of flags, and use a different way of assigning,
// not using ccSetOption.

#define SCOPT_EXPORTALL     0x0001   // export all functions automatically
// 0x0002 [UNUSED]
#define SCOPT_LINENUMBERS   0x0004   // include line numbers in compiled code
#define SCOPT_AUTOIMPORT    0x0008   // when creating instance, export funcs to other scripts
#define SCOPT_DEBUGRUN      0x0010   // write instructions as they are procssed to log file
// TODO: this flag might have to be removed as it makes inconsistent rules for distinct scripts
#define SCOPT_NOIMPORTOVERRIDE 0x0020 // do not allow an import to be re-declared
#define SCOPT_LEFTTORIGHT   0x0040   // left-to-right operator precedance
#define SCOPT_OLDSTRINGS    0x0080   // allow old-style strings
#define SCOPT_UTF8          0x0100   // UTF-8 text mode
#define SCOPT_HIGHEST       SCOPT_UTF8

extern void ccSetOption(int, int);
extern int ccGetOption(int);

// error reporting

struct ScriptError
{
    bool HasError = false; // set if error occurs
    bool IsUserError = false; // marks script use errors
    AGS::Common::String ErrorString; // description of the error
    int Line = 0;  // line number of the error
    AGS::Common::String CallStack; // callstack where error happened
};

void cc_clear_error();
bool cc_has_error();
const ScriptError &cc_get_error();
// Returns callstack of the last recorded script error, or a callstack
// of a current execution point, if no script error is currently saved in memory.
AGS::Common::String cc_get_err_callstack(int max_lines = INT_MAX);
void cc_error(const char *, ...);
void cc_error(const ScriptError &err);
// Project-dependent script error formatting
AGS::Common::String cc_format_error(const AGS::Common::String &message);

// currently compiled or executed line
extern int currentline;
// name of currently compiling script or script section
extern std::string ccCurScriptName;

#endif // __CC_ERROR_H

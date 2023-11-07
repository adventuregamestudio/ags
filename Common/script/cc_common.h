//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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
#ifndef __CC_COMMON_H
#define __CC_COMMON_H

#include "util/string.h"

#define SCOPT_EXPORTALL     0x0001   // export all functions automatically
#define SCOPT_SHOWWARNINGS  0x0002   // printf warnings to console
#define SCOPT_LINENUMBERS   0x0004   // include line numbers in compiled code
#define SCOPT_AUTOIMPORT    0x0008   // when creating instance, export funcs to other scripts
#define SCOPT_DEBUGRUN      0x0010   // write instructions as they are procssed to log file
// TODO: this flag might have to be removed as it makes inconsistent rules for distinct scripts
#define SCOPT_NOIMPORTOVERRIDE 0x0020 // do not allow an import to be re-declared
//#define SCOPT_LEFTTORIGHT 0x40   // left-to-right operator precedance (DEPRECATED)
#define SCOPT_OLDSTRINGS    0x0080   // allow old-style strings
#define SCOPT_UTF8          0x0100   // UTF-8 text mode
#define SCOPT_RTTI          0x0200   // generate and export RTTI
#define SCOPT_RTTIOPS       0x0400   // enable syntax & opcodes that require RTTI to work
#define SCOPT_HIGHEST       SCOPT_RTTIOPS

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
void cc_error(const char *, ...);
void cc_error(const ScriptError &err);
// Project-dependent script error formatting
AGS::Common::String cc_format_error(const AGS::Common::String &message);

extern int currentline;
extern const char *ccCurScriptName;

#endif // __CC_COMMON_H

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
// Script options and error reporting.
//
//=============================================================================
#ifndef __CC_ERROR_H
#define __CC_ERROR_H

#include "util/string.h"

#define SCOPT_EXPORTALL      1   // export all functions automatically
#define SCOPT_SHOWWARNINGS   2   // printf warnings to console
#define SCOPT_LINENUMBERS    4   // include line numbers in compiled code
#define SCOPT_AUTOIMPORT     8   // when creating instance, export funcs to other scripts
#define SCOPT_DEBUGRUN    0x10   // write instructions as they are procssed to log file
#define SCOPT_NOIMPORTOVERRIDE 0x20 // do not allow an import to be re-declared
#define SCOPT_LEFTTORIGHT 0x40   // left-to-right operator precedance
#define SCOPT_OLDSTRINGS  0x80   // allow old-style strings
#define SCOPT_UTF8        0x100  // UTF-8 text mode

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

#endif // __CC_ERROR_H

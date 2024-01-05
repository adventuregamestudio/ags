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
#include "script/cc_common.h"
#include <stdio.h>
#include <utility>
#include "util/string.h"

using namespace AGS::Common;

int ccCompOptions = SCOPT_LEFTTORIGHT;

void ccSetOption(int optbit, int onoroff)
{
    if (onoroff)
        ccCompOptions |= optbit;
    else
        ccCompOptions &= ~optbit;
}

int ccGetOption(int optbit)
{
    if (ccCompOptions & optbit)
        return 1;

    return 0;
}

// Returns current running script callstack as a human-readable text
extern String cc_get_callstack(int max_lines = INT_MAX);

static ScriptError ccError;

void cc_clear_error()
{
    ccError = ScriptError();
}

bool cc_has_error()
{
    return ccError.HasError;
}

const ScriptError &cc_get_error()
{
    return ccError;
}

void cc_error(const char *descr, ...)
{
    ccError.IsUserError = false;
    if (descr[0] == '!')
    {
        ccError.IsUserError = true;
        descr++;
    }

    va_list ap;
    va_start(ap, descr);
    String displbuf = String::FromFormatV(descr, ap);
    va_end(ap);

    // TODO: because this global ccError is a global shared variable,
    // we have to use project-dependent function to format the final message
    ccError.ErrorString = cc_format_error(displbuf);
    ccError.CallStack = cc_get_callstack();
    ccError.HasError = 1;
    ccError.Line = currentline;
}

void cc_error(const ScriptError &err)
{
    ccError = err;
}

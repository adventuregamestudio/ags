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

// Returns full script error message and callstack (if possible)
extern std::pair<String, String> cc_error_at_line(const char *error_msg);
// Returns script error message without location or callstack
extern String cc_error_without_line(const char *error_msg);

ScriptError ccError;

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

    if (currentline > 0)
    {
        // [IKM] Implementation is project-specific
        std::pair<String, String> errinfo = cc_error_at_line(displbuf.GetCStr());
        ccError.ErrorString = errinfo.first;
        ccError.CallStack = errinfo.second;
    }
    else
    {
        ccError.ErrorString = cc_error_without_line(displbuf.GetCStr());
        ccError.CallStack = "";
    }

    ccError.HasError = 1;
    ccError.Line = currentline;
}

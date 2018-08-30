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

#include <stdarg.h>
#include <string.h>
#include "cc_error.h"
#include "script/script_common.h"  // current_line

extern void cc_error_at_line(char *buffer, const char *error_msg);
extern void cc_error_without_line(char *buffer, const char *error_msg);

int ccError = 0;
int ccErrorLine = 0;
char ccErrorString[400];
char ccErrorCallStack[400];
bool ccErrorIsUserError = false;
const char *ccCurScriptName = "";

void cc_error(const char *descr, ...)
{
    ccErrorCallStack[0] = 0;
    ccErrorIsUserError = false;
    if (descr[0] == '!')
    {
        ccErrorIsUserError = true;
        descr++;
    }

    char displbuf[1000];
    va_list ap;

    va_start(ap, descr);
    vsprintf(displbuf, descr, ap);
    va_end(ap);

    if (currentline > 0) {
        // [IKM] Implementation is project-specific
        cc_error_at_line(ccErrorString, displbuf);
    }
    else
        cc_error_without_line(ccErrorString, displbuf);

    ccError = 1;
    ccErrorLine = currentline;
}

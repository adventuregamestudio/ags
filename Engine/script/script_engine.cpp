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
// Script Editor run-time engine component (c) 1998 Chris Jones
// script chunk format:
// 00h  1 dword  version - should be 2
// 04h  1 dword  sizeof(scriptblock)
// 08h  1 dword  number of ScriptBlocks
// 0Ch  n STRUCTs ScriptBlocks
//
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include "util/filestream.h"
#include "script/cc_instance.h"
#include "script/cc_error.h"

using AGS::Common::Stream;

char *scripteditruntimecopr = "Script Editor v1.2 run-time component. (c) 1998 Chris Jones";

extern int currentline; // in script/script_common

void cc_error_at_line(char *buffer, const char *error_msg)
{
    if (ccInstance::GetCurrentInstance() == NULL) {
        sprintf(ccErrorString, "Error (line %d): %s", currentline, error_msg);
    }
    else {
        sprintf(ccErrorString, "Error: %s\n", error_msg);
        ccInstance::GetCurrentInstance()->GetCallStack(ccErrorCallStack, 5);
    }
}

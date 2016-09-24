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

#include <stdio.h>
#include <string.h>
#include "consoleoutputtarget.h"
#include "ac/roomstruct.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "script/script.h"

extern roomstruct thisroom;
extern int currentline;

#define STD_BUFFER_SIZE 3000

extern DebugConsoleText debug_line[DEBUG_CONSOLE_NUMLINES];
extern int first_debug_line, last_debug_line, display_console;

namespace AGS
{
namespace Engine
{
namespace Out
{

ConsoleOutputTarget::ConsoleOutputTarget()
{
}

ConsoleOutputTarget::~ConsoleOutputTarget()
{
}

void ConsoleOutputTarget::Out(const char *sz_fullmsg)
{
    AGSPlatformDriver *platform_driver = AGSPlatformDriver::GetDriver();

    if (!platform_driver) {
        // TODO: emergency call here
        return;
    }

    char displbuf[STD_BUFFER_SIZE];
    strcpy(displbuf, sz_fullmsg);
    displbuf[99] = 0;

    strcpy (debug_line[last_debug_line].text, displbuf);
    ccInstance*curinst = ccInstance::GetCurrentInstance();
    if (curinst != NULL) {
        char scriptname[20];
        if (curinst->instanceof == gamescript)
            strcpy(scriptname,"G ");
        else if (curinst->instanceof == thisroom.compiled_script)
            strcpy (scriptname, "R ");
        else if (curinst->instanceof == dialogScriptsScript)
            strcpy(scriptname,"D ");
        else
            strcpy(scriptname,"? ");
        sprintf(debug_line[last_debug_line].script,"%s%d",scriptname,currentline);
    }
    else debug_line[last_debug_line].script[0] = 0;

    last_debug_line = (last_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

    if (last_debug_line == first_debug_line)
        first_debug_line = (first_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;
}

} // namespace Out
} // namespace Engine
} // namespace AGS

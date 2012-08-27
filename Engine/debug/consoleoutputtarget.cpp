
#include <string.h>
#include "consoleoutputtarget.h"
#include "util/wgt2allg.h"
#include "ac/roomstruct.h"
#include "debug/debug_log.h"
#include "debug/out.h"

extern roomstruct thisroom;
extern ccScript* gamescript;
extern ccScript* dialogScriptsScript;
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

CConsoleOutputTarget::CConsoleOutputTarget()
{
}

CConsoleOutputTarget::~CConsoleOutputTarget()
{
}

void CConsoleOutputTarget::Out(const char *sz_fullmsg)
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
    ccInstance*curinst = ccGetCurrentInstance();
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

    platform_driver->WriteDebugString("%s (%s)", displbuf, debug_line[last_debug_line].script);

    last_debug_line = (last_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

    if (last_debug_line == first_debug_line)
        first_debug_line = (first_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;
}

} // namespace Out
} // namespace Engine
} // namespace AGS

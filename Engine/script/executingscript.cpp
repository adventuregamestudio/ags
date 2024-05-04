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
#include <string.h>
#include "executingscript.h"
#include "debug/debug_log.h"
#include "script/script.h"

using namespace AGS::Common;

QueuedScript::QueuedScript()
    : Instance(kScInstGame)
    , ParamCount(0)
{
}

void ExecutingScript::QueueAction(const PostScriptAction &act)
{
    for (const auto &prev_act : PostScriptActions)
    {
        // if something that will terminate the room has already
        // been queued, don't allow a second thing to be queued
        switch (prev_act.Type)
        {
        case ePSANewRoom:
        case ePSARestoreGame:
        case ePSARestoreGameDialog:
        case ePSARunAGSGame:
        case ePSARestartGame:
            quitprintf("!%s: Cannot run this command, since there was a %s command already queued to run in \"%s\", line %d",
                act.Name.GetCStr(), prev_act.Name.GetCStr(),
                prev_act.Position.Section.GetCStr(), prev_act.Position.Line);
            break;
        default:
            break;
        }
    }

    PostScriptAction act_pos = act;
    get_script_position(act_pos.Position);
    PostScriptActions.push_back(act_pos);
}

void ExecutingScript::RunAnother(const char *namm, ScriptInstType scinst, size_t param_count, const RuntimeScriptValue *params)
{
    QueuedScript script;
    script.FnName.SetString(namm, MAX_FUNCTION_NAME_LEN);
    script.Instance = scinst;
    script.ParamCount = param_count;
    for (size_t p = 0; p < MAX_QUEUED_PARAMS && p < param_count; ++p)
        script.Params[p] = params[p];
    ScFnQueue.push_back(script);
}

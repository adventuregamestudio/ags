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

QueuedScript::QueuedScript()
    : Instance(kScInstGame)
    , ParamCount(0)
{
}

int ExecutingScript::queue_action(PostScriptAction act, int data, const char *aname) {
    if (numPostScriptActions >= MAX_QUEUED_ACTIONS)
        quitprintf("!%s: Cannot queue action, post-script queue full", aname);

    if (numPostScriptActions > 0) {
        // if something that will terminate the room has already
        // been queued, don't allow a second thing to be queued
        switch (postScriptActions[numPostScriptActions - 1]) {
    case ePSANewRoom:
    case ePSARestoreGame:
    case ePSARestoreGameDialog:
    case ePSARunAGSGame:
    case ePSARestartGame:
        quitprintf("!%s: Cannot run this command, since there was a %s command already queued to run in \"%s\", line %d",
            aname, postScriptActionNames[numPostScriptActions - 1],
            postScriptActionPositions[numPostScriptActions - 1].Section.GetCStr(), postScriptActionPositions[numPostScriptActions - 1].Line);
        break;
    default:
        break;
        }
    }

    postScriptActions[numPostScriptActions] = act;
    postScriptActionData[numPostScriptActions] = data;
    postScriptActionNames[numPostScriptActions] = aname;
    get_script_position(postScriptActionPositions[numPostScriptActions]);
    numPostScriptActions++;
    return numPostScriptActions - 1;
}

void ExecutingScript::run_another(const char *namm, ScriptInstType scinst, size_t param_count, const RuntimeScriptValue *params) {
    if (numanother < MAX_QUEUED_SCRIPTS)
        numanother++;
    else {
        /*debug_script_warn("Warning: too many scripts to run, ignored %s(%d,%d)",
        script_run_another[numanother - 1], run_another_p1[numanother - 1],
        run_another_p2[numanother - 1]);*/
    }
    int thisslot = numanother - 1;
    QueuedScript &script = ScFnQueue[thisslot];
    script.FnName.SetString(namm, MAX_FUNCTION_NAME_LEN);
    script.Instance = scinst;
    script.ParamCount = param_count;
    for (size_t p = 0; p < MAX_QUEUED_PARAMS && p < param_count; ++p)
        script.Params[p] = params[p];
}

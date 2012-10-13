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

#include "executingscript.h"
#include <string.h>
#include "debug/debug_log.h"

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
        quitprintf("!%s: Cannot run this command, since there is a %s command already queued to run", aname, postScriptActionNames[numPostScriptActions - 1]);
        break;
        // MACPORT FIX 9/6/5: added default clause to remove warning
    default:
        break;
        }
    }

    postScriptActions[numPostScriptActions] = act;
    postScriptActionData[numPostScriptActions] = data;
    postScriptActionNames[numPostScriptActions] = aname;
    numPostScriptActions++;
    return numPostScriptActions - 1;
}

void ExecutingScript::run_another (char *namm, int p1, int p2) {
    if (numanother < MAX_QUEUED_SCRIPTS)
        numanother++;
    else {
        /*debug_log("Warning: too many scripts to run, ignored %s(%d,%d)",
        script_run_another[numanother - 1], run_another_p1[numanother - 1],
        run_another_p2[numanother - 1]);*/
    }
    int thisslot = numanother - 1;
    strcpy(script_run_another[thisslot], namm);
    run_another_p1[thisslot] = p1;
    run_another_p2[thisslot] = p2;
}

void ExecutingScript::init() {
    inst = NULL;
    forked = 0;
    numanother = 0;
    numPostScriptActions = 0;
}

ExecutingScript::ExecutingScript() {
    init();
}

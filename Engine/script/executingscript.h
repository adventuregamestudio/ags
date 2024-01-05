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
//
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H
#define __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H

#include "script/cc_instance.h"

enum PostScriptAction {
    ePSANewRoom,
    ePSAInvScreen,
    ePSARestoreGame,
    ePSARestoreGameDialog,
    ePSARunAGSGame,
    ePSARunDialog,
    ePSARestartGame,
    ePSASaveGame,
    ePSASaveGameDialog
};

#define MAX_QUEUED_SCRIPTS 4
#define MAX_QUEUED_ACTIONS 5
#define MAX_QUEUED_ACTION_DESC 100
#define MAX_FUNCTION_NAME_LEN 60
#define MAX_QUEUED_PARAMS  4

enum ScriptInstType
{
    kScInstGame,
    kScInstRoom
};

struct QueuedScript
{
    Common::String     FnName;
    ScriptInstType     Instance;
    size_t             ParamCount;
    RuntimeScriptValue Params[MAX_QUEUED_PARAMS];

    QueuedScript();
};

struct ExecutingScript {
    ccInstance *inst = nullptr;
    // owned fork; CHECKME: this seem unused in the current engine
    std::unique_ptr<ccInstance> forkedInst{};
    PostScriptAction postScriptActions[MAX_QUEUED_ACTIONS]{};
    const char *postScriptActionNames[MAX_QUEUED_ACTIONS]{};
    ScriptPosition  postScriptActionPositions[MAX_QUEUED_ACTIONS]{};
    char postScriptSaveSlotDescription[MAX_QUEUED_ACTIONS][MAX_QUEUED_ACTION_DESC]{};
    int  postScriptActionData[MAX_QUEUED_ACTIONS]{};
    int  numPostScriptActions = 0;
    QueuedScript ScFnQueue[MAX_QUEUED_SCRIPTS]{};
    int  numanother = 0;

    ExecutingScript() = default;
    int queue_action(PostScriptAction act, int data, const char *aname);
    void run_another(const char *namm, ScriptInstType scinst, size_t param_count, const RuntimeScriptValue *params);
};

#endif // __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H

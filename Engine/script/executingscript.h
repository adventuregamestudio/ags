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

#include <vector>
#include "script/cc_instance.h"

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

enum PostScriptActionType
{
    ePSAUndefined,
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

struct PostScriptAction
{
    PostScriptActionType Type = ePSAUndefined;
    int Data = 0;
    Common::String Name;
    Common::String Description;
    ScriptPosition Position;

    PostScriptAction() = default;
    PostScriptAction(PostScriptActionType type, int data, const Common::String &name)
        : Type(type), Data(data), Name(name) {}
    PostScriptAction(PostScriptActionType type, int data, const Common::String &name, const Common::String &desc)
        : Type(type), Data(data), Name(name), Description(desc) {}
};

struct ExecutingScript
{
    // Instance refers either to one of the global instances,
    // or a ForkedInst created for this purpose
    ccInstance *Inst = nullptr;
    // owned fork; CHECKME: this seem unused in the current engine
    std::unique_ptr<ccInstance> ForkedInst{};
    std::vector<PostScriptAction> PostScriptActions;
    std::vector<QueuedScript> ScFnQueue;

    ExecutingScript() = default;
    void QueueAction(const PostScriptAction &act);
    void RunAnother(const char *namm, ScriptInstType scinst, size_t param_count, const RuntimeScriptValue *params);
};

#endif // __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H

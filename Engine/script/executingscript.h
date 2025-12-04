//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
#include "gfx/bitmap.h"
#include "script/runtimescript.h"
#include "script/runtimescriptvalue.h"

#define MAX_SCRIPT_EVT_PARAMS  5

// A general script type, used to search for or run a function.
// NOTE: "game" type may mean either "global script" or any script module,
// depending on other circumstances.
// TODO: get rid of this later, remains of old event function logic
enum ScriptType
{
    kScTypeNone,
    kScTypeGame,    // game script modules
    kScTypeRoom     // room script
};

// ScriptFunctionRef - represents a *unresolved* reference to the script function.
// This means that it has only script and function names, but not the actual
// pointer to a loaded bytecode.
struct ScriptFunctionRef
{
    AGS::Common::String ModuleName;
    AGS::Common::String FuncName;
 
    ScriptFunctionRef() = default;
    ScriptFunctionRef(const AGS::Common::String &fn_name)
        : FuncName(fn_name) {}
    ScriptFunctionRef(const AGS::Common::String &module_name, const AGS::Common::String &fn_name)
        : ModuleName(module_name), FuncName(fn_name) {}
    bool IsEmpty() const { return FuncName.IsEmpty(); }
    operator bool() const { return FuncName.IsEmpty(); }
};

// Actions that can be scheduled for until the current script completes
enum PostScriptActionType
{
    ePSAUndefined,
    ePSANewRoom,            // Change to another room
    ePSAInvScreen,          // Display InventoryScreen (built-in) [DEPRECATED]
    ePSARestoreGame,        // Restore saved game
    ePSARestoreGameDialog,  // Display Restore Game dialog (built-in)
    ePSARunAGSGame,         // Load and run another AGS game
    ePSARunDialog,          // Run Dialog (conversation)
    ePSARestartGame,        // Restart game (restore autosave slot)
    ePSASaveGame,           // Save game
    ePSASaveGameDialog,     // Display Save Game dialog (built-in)
    ePSAStopDialog,         // Stop Dialog (conversation)
    ePSAScanSaves           // Scan save slots (results in multiple chained callbacks)
};

struct PostScriptAction
{
    // TODO: refactor this into a union of structs!
    PostScriptActionType Type = ePSAUndefined;
    int Data[6]{};
    Common::String Name;
    Common::String Text;
    mutable std::unique_ptr<Common::Bitmap> Image;
    AGS::Engine::ScriptPosition Position;

    PostScriptAction() = default;
    PostScriptAction(PostScriptActionType type, int data, const Common::String &name, const Common::String &text = {},
        std::unique_ptr<Common::Bitmap> &&image = {})
        : Type(type), Name(name), Text(text), Image(std::move(image)) { Data[0] = data; }
    PostScriptAction(PostScriptActionType type, int data1, int data2, const Common::String &name)
        : Type(type), Name(name) { Data[0] = data1; Data[1] = data2; }
    PostScriptAction(PostScriptActionType type, int data1, int data2, int data3, const Common::String &name)
        : Type(type), Name(name) { Data[0] = data1; Data[1] = data2; Data[2] = data3; }
    PostScriptAction(PostScriptActionType type, int data1, int data2, int data3, int data4, int data5, int data6, const Common::String &name)
        : Type(type), Name(name) { Data[0] = data1; Data[1] = data2; Data[2] = data3; Data[3] = data4; Data[4] = data5; Data[5] = data6; }
};

// ExecutingScript stores scheduled actions for the current running script.
// These actions will be executed after the current script completes.
struct ExecutingScript
{
    const AGS::Engine::RuntimeScript * const Script = nullptr;
    std::vector<PostScriptAction> PostScriptActions;

    ExecutingScript() = default;
    ExecutingScript(const AGS::Engine::RuntimeScript *script)
        : Script(script) {}
    void QueueAction(PostScriptAction &&act);
};

#endif // __AGS_EE_SCRIPT__EXECUTINGSCRIPT_H

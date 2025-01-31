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
#ifndef __AGS_EE_SCRIPT__SCRIPT_H
#define __AGS_EE_SCRIPT__SCRIPT_H

#include <memory>
#include <vector>
#include "ac/dynobj/scriptsystem.h"
#include "game/interactions.h"
#include "script/executingscript.h"
#include "script/runtimescript.h"
#include "script/scriptexecutor.h"
#include "util/string.h"

using AGS::Common::String;
using AGS::Common::InteractionEvents;

#define LATE_REP_EXEC_ALWAYS_NAME "late_repeatedly_execute_always"
#define REP_EXEC_ALWAYS_NAME "repeatedly_execute_always"
#define REP_EXEC_NAME "repeatedly_execute"

// ObjectEvent - a struct holds data of the object's interaction event,
// such as object's reference and accompanying parameters
struct ObjectEvent
{
    // Script type (i.e. game or room);
    // NOTE: kScTypeGame also may refer to "all modules", not only "globalscript"
    ScriptType ScType = kScTypeNone;
    // Name of the script block to run, may be used as a formatting string;
    // has a form of "objecttype%d"
    String BlockName;
    // Script block's ID, commonly corresponds to the object's ID
    int BlockID = 0;
    // Event parameters
    size_t ParamCount = 0u;
    RuntimeScriptValue Params[MAX_SCRIPT_EVT_PARAMS];

    ObjectEvent() = default;
    // An event without additional parameters
    ObjectEvent(ScriptType sc_type, const String &block_name, int block_id = 0)
        : ScType(sc_type), BlockName(block_name), BlockID(block_id) {}
    // An event with a dynamic object reference
    ObjectEvent(ScriptType sc_type, const String &block_name, int block_id,
        const RuntimeScriptValue &dyn_obj)
        : ScType(sc_type), BlockName(block_name), BlockID(block_id)
    {
        ParamCount = 1u;
        Params[0] = dyn_obj;
    }
    // An event with a dynamic object reference and interaction mode
    ObjectEvent(ScriptType sc_type, const String &block_name, int block_id,
        const RuntimeScriptValue &dyn_obj, int mode)
        : ScType(sc_type), BlockName(block_name), BlockID(block_id)
    {
        ParamCount = 2u;
        Params[0] = dyn_obj;
        Params[1] = RuntimeScriptValue().SetInt32(mode);
    }
};

// NonBlockingScriptFunction struct contains a cached information about
// a non-blocking script callback, which script modules is this callback present in.
struct NonBlockingScriptFunction
{
    String FunctionName;
    size_t ParamCount = 0u;
    RuntimeScriptValue Params[MAX_SCRIPT_EVT_PARAMS];
    bool RoomHasFunction;
    bool GlobalScriptHasFunction;
    std::vector<bool> ModuleHasFunction;
    bool AtLeastOneImplementationExists;

    NonBlockingScriptFunction(const String &fn_name, int param_count)
    {
        FunctionName = fn_name;
        ParamCount = param_count;
        AtLeastOneImplementationExists = false;
        RoomHasFunction = true;
        GlobalScriptHasFunction = true;
    }
};

void    run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun);

// Runs the ObjectEvent using a script callback of 'evnt' index,
// or alternatively of 'chkAny' index, if previous does not exist
// Returns 0 normally, or -1 telling of a game state change (eg. a room change occured).
int     run_interaction_script(const ObjectEvent &obj_evt, const InteractionEvents *nint, int evnt, int chkAny = -1);
void    run_unhandled_event(const ObjectEvent &obj_evt, int evnt);

enum RunScFuncResult
{
    kScFnRes_Done = 0,
    kScFnRes_GenericInstError = -1, // running instance generic failure
    kScFnRes_NotFound = -2,         // function not found
    kScFnRes_ScriptBusy = -3,       // script is already being executed
};

AGS::Engine::RuntimeScript *GetScriptInstanceByType(ScriptType sc_type);
// Tests if a function exists in the given script module
bool    DoesScriptFunctionExist(const AGS::Engine::RuntimeScript *script, const String &fn_name);
// Tests if a function exists in any of the regular script module, *except* room script
bool    DoesScriptFunctionExistInModules(const String &fn_name);
// Queues a script function to be run either called by the engine or from another script;
// the function is identified by its name, and will be run in time, by RunScriptFunctionAuto().
void    QueueScriptFunction(ScriptType sc_type, const String &fn_name, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr, std::weak_ptr<bool> result = {});
// Queues a script function to be run either called by the engine or from another script;
// the function is identified by its name and script module, and will be run in time,
// by RunScriptFunctionAuto().
void    QueueScriptFunction(ScriptType sc_type, const ScriptFunctionRef &fn_ref, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr, std::weak_ptr<bool> result = {});
// Try to run a script function on a given script instance
RunScFuncResult RunScriptFunction(const AGS::Engine::RuntimeScript *script, const String &tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Run a script function in all the regular script modules, in order, where available
// includes globalscript, but not the current room script.
// returns if at least one instance of a function was run successfully.
bool    RunScriptFunctionInModules(const String &tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Run a script function in the current room script; returns if a function was run successfully.
bool    RunScriptFunctionInRoom(const String &tsname, size_t param_count = 0, const RuntimeScriptValue *params = nullptr);
// Try to run a script function, guessing the behavior by its name and script instance type;
// depending on the type may run a claimable callback chain;
// returns if at least one instance of a function was run successfully.
bool   RunScriptFunctionAuto(ScriptType sc_type, const ScriptFunctionRef &fn_ref, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);

// Allocates script executor and standard threads
void    InitScriptExec();
// Frees script executor and all threads
void    ShutdownScriptExec();
// Preallocates script module instances
void    AllocScriptModules();
// Link all script modules into a single program,
// registers script exports in a global symbol table
bool    LinkGlobalScripts();
// Cancel any running scripts
void    AbortAllScripts();
// Unlinks scripts, unregister script exports
void    UnlinkAllScripts();
// Delete only the current room script instance
void    FreeRoomScript();
// Deletes all the global scripts and modules;
// this frees all of the bytecode and runtime script memory.
void    FreeAllScripts();

//=============================================================================

// Makes a old-style interaction function name (for interaction list "run script" command)
String  make_interact_func_name(const String &base, int param, int subd);
// Performs various updates to the game after script interpreter returns control to the engine.
// Executes scheduled commands and does changes that are not executed immediately during script
// (either for logical reasons, and for optimization).
void    post_script_cleanup();
void    quit_with_script_error(const String &fn_name);
// Assert and abort game if the script is currently inside the rep_exec_always function
void    can_run_delayed_command();
// Tells whether the script is currently NOT inside the rep_exec_always function
bool    get_can_run_delayed_command();

// Gets current running script position
bool    get_script_position(AGS::Engine::ScriptPosition &script_pos);
String  cc_get_callstack(int max_lines = INT_MAX);

// Gets current ExecutingScript object
ExecutingScript *get_executingscript();

extern int num_scripts;
extern int post_script_cleanup_stack;

extern int inside_script,in_graph_script;
extern int no_blocking_functions; // set to 1 while in rep_Exec_always

extern NonBlockingScriptFunction repExecAlways;
extern NonBlockingScriptFunction lateRepExecAlways;
extern NonBlockingScriptFunction getDialogOptionsDimensionsFunc;
extern NonBlockingScriptFunction renderDialogOptionsFunc;
extern NonBlockingScriptFunction getDialogOptionUnderCursorFunc;
extern NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc;
extern NonBlockingScriptFunction runDialogOptionKeyPressHandlerFunc;
extern NonBlockingScriptFunction runDialogOptionTextInputHandlerFunc;
extern NonBlockingScriptFunction runDialogOptionRepExecFunc;
extern NonBlockingScriptFunction runDialogOptionCloseFunc;

extern AGS::Engine::PRuntimeScript gamescript;
extern AGS::Engine::PRuntimeScript dialogScriptsScript;
extern std::vector<AGS::Engine::PRuntimeScript> scriptModules;
extern AGS::Engine::PRuntimeScript roomscript;
extern std::vector<RuntimeScriptValue> moduleRepExecAddr;
extern size_t numScriptModules;

extern std::unique_ptr<AGS::Engine::ScriptExecutor> scriptExecutor;

#endif // __AGS_EE_SCRIPT__SCRIPT_H

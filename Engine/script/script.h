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
#ifndef __AGS_EE_SCRIPT__SCRIPT_H
#define __AGS_EE_SCRIPT__SCRIPT_H

#include <memory>
#include <vector>
#include "script/cc_instance.h"
#include "script/executingscript.h"
#include "ac/dynobj/scriptsystem.h"
#include "game/interactions.h"
#include "util/string.h"

using AGS::Common::String;
using AGS::Common::Interaction;
using AGS::Common::InteractionCommandList;
using AGS::Common::InteractionScripts;
using AGS::Common::InteractionVariable;

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

int     run_dialog_request (int parmtr);
void    run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun);

// TODO: run_interaction_event() and run_interaction_script()
// are in most part duplicating each other, except for the script callback run method.
// May these types be made children of the same base, or stored in a group struct?
// This would also let simplify the calling code in RunObjectInteraction, etc.
//
// Runs the ObjectEvent using an old interaction callback type of 'evnt' index,
// or alternatively of 'chkAny' index, if previous does not exist;
// 'isInv' tells if this is a inventory event (it has a slightly different handling for that)
// Returns 0 normally, or -1 telling of a game state change (eg. a room change occured).
int     run_interaction_event(const ObjectEvent &obj_evt, Interaction *nint, int evnt, int chkAny = -1, bool isInv = false);
// Runs the ObjectEvent using a script callback of 'evnt' index,
// or alternatively of 'chkAny' index, if previous does not exist
// Returns 0 normally, or -1 telling of a game state change (eg. a room change occured).
int     run_interaction_script(const ObjectEvent &obj_evt, InteractionScripts *nint, int evnt, int chkAny = -1);
int     run_interaction_commandlist(const ObjectEvent &obj_evt, InteractionCommandList *nicl, int *timesrun, int*cmdsrun);
void    run_unhandled_event(const ObjectEvent &obj_evt, int evnt);

int     create_global_script();
void    cancel_all_scripts();

enum RunScFuncResult
{
    kScFnRes_Done = 0,
    kScFnRes_GenericInstError = -1, // running instance generic failure
    kScFnRes_NotFound = -2,         // function not found
    kScFnRes_ScriptBusy = -3,       // script is already being executed
};

ccInstance *GetScriptInstanceByType(ScriptType sc_type);
// Tests if a function exists in the given script module
bool    DoesScriptFunctionExist(ccInstance *sci, const String &fn_name);
// Tests if a function exists in any of the regular script module, *except* room script
bool    DoesScriptFunctionExistInModules(const String &fn_name);
// Queues a script function to be run either called by the engine or from another script
void    QueueScriptFunction(ScriptType sc_type, const String &fn_name, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Try to run a script function on a given script instance
RunScFuncResult RunScriptFunction(ccInstance *sci, const String &tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Run a script function in all the regular script modules, in order, where available
// includes globalscript, but not the current room script.
void    RunScriptFunctionInModules(const String &tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Run an obligatory script function in the current room script
void    RunScriptFunctionInRoom(const String &tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Try to run a script function, guessing the behavior by its name and script instance type;
// depending on the type may run a claimable callback chain
void   RunScriptFunctionAuto(ScriptType sc_type, const String &fn_name, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);

// Preallocates script module instances
void    AllocScriptModules();
// Delete all the script instance objects
void    FreeAllScriptInstances();
// Delete only the current room script instance
void    FreeRoomScriptInstance();
// Deletes all the global scripts and modules;
// this frees all of the bytecode and runtime script memory.
void    FreeGlobalScripts();


String  GetScriptName(ccInstance *sci);

//=============================================================================

// Makes a old-style interaction function name (for interaction list "run script" command)
String  make_interact_func_name(const String &base, int param, int subd);
// Performs various updates to the game after script interpreter returns control to the engine.
// Executes actions and does changes that are not executed immediately at script command, for
// optimisation and other reasons.
void    post_script_cleanup();
void    quit_with_script_error(const String &fn_name);
int     get_nivalue (InteractionCommandList *nic, int idx, int parm);
InteractionVariable *get_interaction_variable (int varindx);
InteractionVariable *FindGraphicalVariable(const char *varName);
void    can_run_delayed_command();

// Gets current running script position
bool    get_script_position(ScriptPosition &script_pos);
String  cc_get_callstack(int max_lines = INT_MAX);

// Gets current ExecutingScript object
ExecutingScript *get_executingscript();

extern PScript gamescript;
extern PScript dialogScriptsScript;
// [ikm] we keep ccInstances saved in unique_ptrs globally for now
// (e.g. as opposed to shared_ptrs), because the script handling part of the
// engine is quite fragile and prone to errors whenever the instance is not
// **deleted** in precise time. This is related to:
// - ccScript's "instances" counting, which affects script exports reg/unreg;
// - loadedInstances array.
// One of the examples is the save restoration, that may occur in the midst
// of a post-script cleanup process, whilst the engine's stack still has
// references to the ccInstances that are going to be deleted on cleanup.
// Ideally, this part of the engine should be refactored awhole with a goal
// to make it safe and consistent.
typedef std::unique_ptr<ccInstance> UInstance;
extern UInstance gameinst;
extern UInstance roominst;
extern UInstance dialogScriptsInst;
extern UInstance gameinstFork;
extern UInstance roominstFork;

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

extern std::vector<PScript> scriptModules;
extern std::vector<UInstance> moduleInst;
extern std::vector<UInstance> moduleInstFork;
extern std::vector<RuntimeScriptValue> moduleRepExecAddr;
extern size_t numScriptModules;

#endif // __AGS_EE_SCRIPT__SCRIPT_H

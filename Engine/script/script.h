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

#include <vector>
#include "script/cc_instance.h"
#include "script/executingscript.h"
#include "script/nonblockingscriptfunction.h"
#include "ac/dynobj/scriptsystem.h"
#include "game/interactions.h"
#include "util/string.h"

using AGS::Common::Interaction;
using AGS::Common::InteractionCommandList;
using AGS::Common::InteractionScripts;
using AGS::Common::InteractionVariable;

#define LATE_REP_EXEC_ALWAYS_NAME "late_repeatedly_execute_always"
#define REP_EXEC_ALWAYS_NAME "repeatedly_execute_always"
#define REP_EXEC_NAME "repeatedly_execute"

int     run_dialog_request (int parmtr);
void    run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun);
int     run_interaction_event (Interaction *nint, int evnt, int chkAny = -1, int isInv = 0);
int     run_interaction_script(InteractionScripts *nint, int evnt, int chkAny = -1);
int     create_global_script();
void    cancel_all_scripts();

ccInstance *GetScriptInstanceByType(ScriptInstType sc_inst);
// Queues a script function to be run either called by the engine or from another script
void    QueueScriptFunction(ScriptInstType sc_inst, const char *fn_name, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Try to run a script function on a given script instance
int     RunScriptFunction(ccInstance *sci, const char *tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Run a script function in all the regular script modules, in order, where available
// includes globalscript, but not the current room script.
void    RunScriptFunctionInModules(const char *tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Run an obligatory script function in the current room script
int     RunScriptFunctionInRoom(const char *tsname, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);
// Try to run a script function, guessing the behavior by its name and script instance type;
// depending on the type may run a claimable callback chain
int     RunScriptFunctionAuto(ScriptInstType sc_inst, const char *fn_name, size_t param_count = 0,
    const RuntimeScriptValue *params = nullptr);


AGS::Common::String GetScriptName(ccInstance *sci);

//=============================================================================

char*   make_ts_func_name(const char*base,int iii,int subd);
// Performs various updates to the game after script interpreter returns control to the engine.
// Executes actions and does changes that are not executed immediately at script command, for
// optimisation and other reasons.
void    post_script_cleanup();
void    quit_with_script_error(const char *functionName);
int     get_nivalue (InteractionCommandList *nic, int idx, int parm);
int     run_interaction_commandlist (InteractionCommandList *nicl, int *timesrun, int*cmdsrun);
InteractionVariable *get_interaction_variable (int varindx);
InteractionVariable *FindGraphicalVariable(const char *varName);
void    run_unhandled_event (int evnt);
void    can_run_delayed_command();

// Gets current running script position
bool    get_script_position(ScriptPosition &script_pos);
AGS::Common::String cc_get_callstack(int max_lines = INT_MAX);


extern ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
extern ExecutingScript*curscript;

extern PScript gamescript;
extern PScript dialogScriptsScript;
extern ccInstance *gameinst, *roominst;
extern ccInstance *dialogScriptsInst;
extern ccInstance *gameinstFork, *roominstFork;

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

extern ScriptSystem scsystem;

extern std::vector<PScript> scriptModules;
extern std::vector<ccInstance *> moduleInst;
extern std::vector<ccInstance *> moduleInstFork;
extern std::vector<RuntimeScriptValue> moduleRepExecAddr;
extern size_t numScriptModules;

#endif // __AGS_EE_SCRIPT__SCRIPT_H

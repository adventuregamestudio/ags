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
//
//
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__SCRIPT_H
#define __AGS_EE_SCRIPT__SCRIPT_H

#include "ac/interaction.h"
#include "script/cc_instance.h"
#include "script/executingscript.h"
#include "script/nonblockingscriptfunction.h"
#include "ac/dynobj/scriptsystem.h"
#include "util/array.h"
#include "util/string.h"

#define REP_EXEC_ALWAYS_NAME "repeatedly_execute_always"
#define REP_EXEC_NAME "repeatedly_execute"

int     run_dialog_request (int parmtr);
void    run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun);
int     run_interaction_event (NewInteraction *nint, int evnt, int chkAny = -1, int isInv = 0);
int     run_interaction_script(InteractionScripts *nint, int evnt, int chkAny = -1, int isInv = 0);
int     create_global_script();
void    cancel_all_scripts();

//=============================================================================

char*   make_ts_func_name(char*base,int iii,int subd);
void    post_script_cleanup();
void    quit_with_script_error(const char *functionName);
int     get_nivalue (NewInteractionCommandList *nic, int idx, int parm);
int     run_interaction_commandlist (NewInteractionCommandList *nicl, int *timesrun, int*cmdsrun);
void    run_unhandled_event (int evnt);
void    setup_exports(char*expfrom);
void    can_run_delayed_command();


extern ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
extern ExecutingScript*curscript;

extern ccScript* gamescript;
extern ccScript* dialogScriptsScript;
extern ccInstance *gameinst, *roominst;
extern ccInstance *dialogScriptsInst;
extern ccInstance *gameinstFork, *roominstFork;

extern int num_scripts;
extern int post_script_cleanup_stack;

extern int inside_script,in_graph_script;
extern int no_blocking_functions; // set to 1 while in rep_Exec_always

extern NonBlockingScriptFunction repExecAlways;
extern NonBlockingScriptFunction getDialogOptionsDimensionsFunc;
extern NonBlockingScriptFunction renderDialogOptionsFunc;
extern NonBlockingScriptFunction getDialogOptionUnderCursorFunc;
extern NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc;

extern ScriptSystem scsystem;

extern ccScript *scriptModules[MAX_SCRIPT_MODULES];
extern ccInstance *moduleInst[MAX_SCRIPT_MODULES];
extern ccInstance *moduleInstFork[MAX_SCRIPT_MODULES];
extern RuntimeScriptValue moduleRepExecAddr[MAX_SCRIPT_MODULES];
extern int numScriptModules;


extern AGS::Common::ObjectArray<AGS::Common::String> characterScriptObjNames;
extern AGS::Common::ObjectArray<AGS::Common::String> objectScriptObjNames; // [MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN + 5]
extern AGS::Common::ObjectArray<AGS::Common::String> guiScriptObjNames;

#endif // __AGS_EE_SCRIPT__SCRIPT_H

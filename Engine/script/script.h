
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
#include "util/string.h"

using namespace AGS; // FIXME later

#define REP_EXEC_ALWAYS_NAME "repeatedly_execute_always"
#define REP_EXEC_NAME "repeatedly_execute"

int     run_text_script_iparam(ccInstance*sci,char*tsname,int iparam);
int     run_dialog_request (int parmtr);
void    run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun);
int     run_script_function_if_exist(ccInstance*sci,char*tsname,int numParam, int iparam, int iparam2, int iparam3 = 0) ;
int     run_text_script_2iparam(ccInstance*sci,char*tsname,int iparam,int param2);
int     run_interaction_event (NewInteraction *nint, int evnt, int chkAny = -1, int isInv = 0);
int     run_interaction_script(InteractionScripts *nint, int evnt, int chkAny = -1, int isInv = 0);
int     run_text_script(ccInstance*sci,char*tsname);
int     create_global_script();
void    cancel_all_scripts();
Common::CString get_script_name(ccInstance *rinst);

//=============================================================================

char*   make_ts_func_name(char*base,int iii,int subd);
int     prepare_text_script(ccInstance*sci,char**tsname);
void    post_script_cleanup();
void    quit_with_script_error(const char *functionName);
void    _do_run_script_func_cant_block(ccInstance *forkedinst, NonBlockingScriptFunction* funcToRun, bool *hasTheFunc);
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
extern char *moduleRepExecAddr[MAX_SCRIPT_MODULES];
extern int numScriptModules;

extern char **characterScriptObjNames;
extern char objectScriptObjNames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN + 5];
extern char **guiScriptObjNames;

#endif // __AGS_EE_SCRIPT__SCRIPT_H

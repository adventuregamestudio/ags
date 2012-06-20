#ifndef __AC_SCRIPT_H
#define __AC_SCRIPT_H

#include "ac/ac_interaction.h"
#include "cs/cc_instance.h"
#include "acmain/ac_nonblockingscriptfunction.h"
#include "ac/dynobj/scriptsystem.h"

#define REP_EXEC_ALWAYS_NAME "repeatedly_execute_always"
#define REP_EXEC_NAME "repeatedly_execute"

void script_debug(int cmdd,int dataa);
int run_text_script_iparam(ccInstance*sci,char*tsname,int iparam);
int run_dialog_request (int parmtr);
void run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun);
extern int  run_script_function_if_exist(ccInstance*sci,char*tsname,int numParam, int iparam, int iparam2, int iparam3 = 0) ;
int run_text_script_2iparam(ccInstance*sci,char*tsname,int iparam,int param2);
int  run_interaction_event (NewInteraction *nint, int evnt, int chkAny = -1, int isInv = 0);
int  run_interaction_script(InteractionScripts *nint, int evnt, int chkAny = -1, int isInv = 0);
int run_text_script(ccInstance*sci,char*tsname);

void setup_script_exports();
int create_global_script();

void cancel_all_scripts();
void get_script_name(ccInstance *rinst, char *curScrName);


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

#endif // __AC_SCRIPT_H
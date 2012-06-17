
#include "cs/cc_instance.h"
#include "acmain/ac_nonblockingscriptfunction.h"

void script_debug(int cmdd,int dataa);
int run_text_script_iparam(ccInstance*sci,char*tsname,int iparam);
int run_dialog_request (int parmtr);
void run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun);


extern int inside_script,in_graph_script;
extern int no_blocking_functions; // set to 1 while in rep_Exec_always
extern NonBlockingScriptFunction repExecAlways;
extern NonBlockingScriptFunction getDialogOptionsDimensionsFunc;
extern NonBlockingScriptFunction renderDialogOptionsFunc;
extern NonBlockingScriptFunction getDialogOptionUnderCursorFunc;
extern NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc;


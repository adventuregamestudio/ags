
#include "acmain/ac_maindefines.h"

void break_into_debugger() 
{
#ifdef WINDOWS_VERSION

  if (editor_window_handle != NULL)
    SetForegroundWindow(editor_window_handle);

  send_message_to_editor("BREAK");
  game_paused_in_debugger = 1;

  while (game_paused_in_debugger) 
  {
    update_polled_stuff_if_runtime();
    platform->YieldCPU();
  }

#endif
}

int scrDebugWait = 0;
// allow LShift to single-step,  RShift to pause flow
void scriptDebugHook (ccInstance *ccinst, int linenum) {

  if (pluginsWantingDebugHooks > 0) {
    // a plugin is handling the debugging
    char scname[40];
    get_script_name(ccinst, scname);
    platform->RunPluginDebugHooks(scname, linenum);
    return;
  }

  // no plugin, use built-in debugger

  if (ccinst == NULL) 
  {
    // come out of script
    return;
  }

  if (break_on_next_script_step) 
  {
    break_on_next_script_step = 0;
    break_into_debugger();
    return;
  }

  const char *scriptName = ccGetSectionNameAtOffs(ccinst->runningInst->instanceof, ccinst->pc);

  for (int i = 0; i < numBreakpoints; i++)
  {
    if ((breakpoints[i].lineNumber == linenum) &&
        (strcmp(breakpoints[i].scriptName, scriptName) == 0))
    {
      break_into_debugger();
      break;
    }
  }
}

void check_debug_keys() {
    if (play.debug_mode) {
      // do the run-time script debugging

      if ((!key[KEY_SCRLOCK]) && (scrlockWasDown))
        scrlockWasDown = 0;
      else if ((key[KEY_SCRLOCK]) && (!scrlockWasDown)) {

        break_on_next_script_step = 1;
        scrlockWasDown = 1;
      }

    }

}
/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_DEBUGGER_H
#define __AC_DEBUGGER_H

#include "debug/agseditordebugger.h"

extern int use_compiled_folder_as_current_dir;
extern int editor_debugging_enabled;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];
extern IAGSEditorDebugger *editor_debugger;
extern int break_on_next_script_step;

int check_for_messages_from_editor();
bool send_message_to_editor(const char *msg);
bool send_exception_to_editor(char *qmsg);
const char *get_cur_script(int numberOfLinesOfCallStack);
void check_debug_keys();

extern int fps,display_fps;

#endif // __AC_DEBUGGER_H
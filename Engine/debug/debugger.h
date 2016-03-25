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

#ifndef __AC_DEBUGGER_H
#define __AC_DEBUGGER_H

#include "debug/agseditordebugger.h"

struct ScriptPosition;

extern int use_compiled_folder_as_current_dir;
extern int editor_debugging_enabled;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];
extern IAGSEditorDebugger *editor_debugger;
extern int break_on_next_script_step;

int check_for_messages_from_editor();
bool send_message_to_editor(const char *msg);
bool send_exception_to_editor(const char *qmsg);
const char *get_cur_script(int numberOfLinesOfCallStack);
bool get_script_position(ScriptPosition &script_pos);
void check_debug_keys();

extern int fps,display_fps;

#endif // __AC_DEBUGGER_H

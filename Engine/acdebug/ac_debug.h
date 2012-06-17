/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_DEBUG_H
#define __AC_DEBUG_H

#define DBG_NOIFACE       1
#define DBG_NODRAWSPRITES 2
#define DBG_NOOBJECTS     4
#define DBG_NOUPDATE      8
#define DBG_NOSFX      0x10
#define DBG_NOMUSIC    0x20
#define DBG_NOSCRIPT   0x40
#define DBG_DBGSCRIPT  0x80
#define DBG_DEBUGMODE 0x100
#define DBG_REGONLY   0x200
#define DBG_NOVIDEO   0x400

struct IAGSEditorDebugger
{
public:
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool SendMessageToEditor(const char *message) = 0;
    virtual bool IsMessageAvailable() = 0;
    // Message will be allocated on heap with malloc
    virtual char* GetNextMessage() = 0;
};

IAGSEditorDebugger *GetEditorDebugger(const char *instanceToken);

#define DEBUG_CONSOLE if (play.debug_mode) debug_write_console

void debug_write_console (char *msg, ...);

extern int use_compiled_folder_as_current_dir;
extern int editor_debugging_enabled;
extern int editor_debugging_initialized;
extern char editor_debugger_instance_token[100];
extern IAGSEditorDebugger *editor_debugger;
extern int break_on_next_script_step;
extern volatile int game_paused_in_debugger;
extern HWND editor_window_handle;

int check_for_messages_from_editor();

/* The idea of this is that non-essential errors such as "sound file not
found" are logged instead of exiting the program.
*/
void debug_log(char*texx, ...);
void quitprintf(char*texx, ...);

extern int debug_flags;

struct DebugConsoleText {
    char text[100];
    char script[12];
};

extern DebugConsoleText debug_line[DEBUG_CONSOLE_NUMLINES];
extern int first_debug_line, last_debug_line, display_console;

extern int fps,display_fps;

#endif // __AC_DEBUG_H

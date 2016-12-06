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

#ifndef __AC_DEBUG_LOG_H
#define __AC_DEBUG_LOG_H

#include "script/cc_instance.h"
#include "ac/runtime_defines.h"
#include "ac/gamestate.h"
#include "platform/base/agsplatformdriver.h"
#include "util/ini_util.h"

void init_debug();
void apply_debug_config(const AGS::Common::ConfigTree &cfg);
void shutdown_debug();

// debug_script_log prints debug message tagged with kDbgGroup_Script,
// prepending it with current script position identification
void debug_script_log(const char *msg, ...);

/* The idea of this is that non-essential errors such as "sound file not
found" are logged instead of exiting the program.
*/
// NOTE: debug_log only prints messages when game is in debug mode;
// TODO: revise this later; use new output system with verbosity settings
void debug_log(const char *texx, ...);
void quitprintf(const char *texx, ...);
bool init_editor_debugging();

// allow LShift to single-step,  RShift to pause flow
void scriptDebugHook (ccInstance *ccinst, int linenum) ;

extern int debug_flags;

extern AGS::Common::String debug_line[DEBUG_CONSOLE_NUMLINES];
extern int first_debug_line, last_debug_line, display_console;
extern bool enable_log_file;
extern bool disable_log_file;


extern AGSPlatformDriver *platform;

#endif // __AC_DEBUG_LOG_H

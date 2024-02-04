//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_DEBUG_LOG_H
#define __AC_DEBUG_LOG_H

#include "ac/runtime_defines.h"
#include "debug/out.h"
#include "util/ini_util.h"
#include "util/string.h"

struct ccInstance;

void init_debug(const AGS::Common::ConfigTree &cfg, bool stderr_only);
void apply_debug_config(const AGS::Common::ConfigTree &cfg);
void shutdown_debug();

// prints debug messages of given type tagged with kDbgGroup_Game,
// prepending it with current room number and script position info
void debug_script_print(AGS::Common::MessageType mt, const char *msg, ...);
// prints formatted debug warnings tagged with kDbgGroup_Game,
// prepending it with current room number and script position info
void debug_script_warn(const char *msg, ...);
// prints formatted debug message tagged with kDbgGroup_Game,
// prepending it with current room number and script position info
void debug_script_log(const char *msg, ...);

// Same as quit(), but with message formatting
void quitprintf(const char *texx, ...);

// Connect engine to external debugger, if one is available
bool init_editor_debugging(const AGS::Common::ConfigTree &cfg);
// allow LShift to single-step,  RShift to pause flow
void scriptDebugHook (ccInstance *ccinst, int linenum) ;

#endif // __AC_DEBUG_LOG_H

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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

#include <cstdarg>
#include "ac/runtime_defines.h"
#include "debug/out.h"
#include "util/ini_util.h"
#include "util/string.h"

class ccInstance;

void init_debug(const AGS::Common::ConfigTree &cfg, bool stderr_only);
void apply_debug_config(const AGS::Common::ConfigTree &cfg, bool finalize);
void shutdown_debug();

void debug_script_event(AGS::Common::MessageType mt, const AGS::Common::String &msg);
// overload for calls that have no args
inline void debug_script_event(AGS::Common::MessageType mt, const char *msg)
{
    debug_script_event(mt, AGS::Common::String::Wrapper(msg));
}
// prints debug messages of given type tagged with kDbgGroup_Game,
// prepending it with current room number and script position info
inline void debug_script_message(AGS::Common::MessageType mt, AGS_FORMAT_STRING_ARG const char *msg, ...) AGS_FORMAT_STRING(2);
inline void debug_script_message(AGS::Common::MessageType mt, AGS_FORMAT_STRING_ARG const char *msg, ...)
{
    va_list argptr; va_start(argptr, msg);
    debug_script_event(mt, AGS::Common::String::FromFormatV(msg, argptr));
    va_end(argptr);
}
// prints formatted debug warnings tagged with kDbgGroup_Game,
// prepending it with current room number and script position info
inline void debug_script_warn(AGS_FORMAT_STRING_ARG const char *msg, ...) AGS_FORMAT_STRING(1);
inline void debug_script_warn(AGS_FORMAT_STRING_ARG const char *msg, ...)
{
    va_list argptr; va_start(argptr, msg);
    debug_script_event(AGS::Common::kDbgMsg_Warn, AGS::Common::String::FromFormatV(msg, argptr));
    va_end(argptr);
}
// prints formatted debug errors tagged with kDbgGroup_Game,
// prepending it with current room number and script position info
inline void debug_script_error(AGS_FORMAT_STRING_ARG const char *msg, ...) AGS_FORMAT_STRING(1);
inline void debug_script_error(AGS_FORMAT_STRING_ARG const char *msg, ...)
{
    va_list argptr; va_start(argptr, msg);
    debug_script_event(AGS::Common::kDbgMsg_Error, AGS::Common::String::FromFormatV(msg, argptr));
    va_end(argptr);
}
// prints formatted debug message tagged with kDbgGroup_Game,
// prepending it with current room number and script position info
inline void debug_script_log(AGS_FORMAT_STRING_ARG const char *msg, ...) AGS_FORMAT_STRING(1);
inline void debug_script_log(AGS_FORMAT_STRING_ARG const char *msg, ...)
{
    va_list argptr; va_start(argptr, msg);
    debug_script_event(AGS::Common::kDbgMsg_Debug, AGS::Common::String::FromFormatV(msg, argptr));
    va_end(argptr);
}

// Connect engine to external debugger, if one is available
bool init_editor_debugging(const AGS::Common::ConfigTree &cfg);
// allow LShift to single-step,  RShift to pause flow
void scriptDebugHook (ccInstance *ccinst, int linenum) ;

#endif // __AC_DEBUG_LOG_H

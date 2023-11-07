//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// This is a number of quickly set up utilities for script handling.
// Perhaps could be merged and/or shared with AGS Compiler code later.
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__SCRIPTUTIL_H
#define __AGS_TOOL_DATA__SCRIPTUTIL_H

#include <vector>
#include "util/string.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::String;

// Marker that tells compiler to reset code parsing counters (line number, etc)
extern const char *NEW_SCRIPT_MARKER;

// A generic compilation message struct, may contain an error or any other
// type of message (warning, info).
struct CompileMessage
{
    bool   Error;
    String Message;
    String ScriptName;
    size_t LineNumber;

    CompileMessage(bool error, const String &msg, const String &script_name, size_t line)
        : Error(error), Message(msg), ScriptName(script_name), LineNumber(line) {}
};

typedef std::vector<CompileMessage> CompileMessages;

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__SCRIPTUTIL_H

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
//
// IOutputHandler is a debug printing interface. Its implementations can be
// registered as potential output for the debug log.
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__OUTPUTHANDLER_H
#define __AGS_CN_DEBUG__OUTPUTHANDLER_H

#include "debug/out.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

struct DebugMessage
{
    String       Text;
    MessageGroupHandle GroupID = InvalidMessageGroup;
    String       GroupName;
    MessageType  MT = kDbgMsg_None;

    DebugMessage() = default;
    DebugMessage(const String &text, MessageGroupHandle group_id, const String &group_name, MessageType mt)
        : Text(text)
        , GroupID(group_id)
        , GroupName(group_name)
        , MT(mt)
    {}
};

class IOutputHandler
{
public:
    virtual ~IOutputHandler() = default;
    
    // Called when this output handler is registered by the DebugManager,
    // but prior it is inserted into the outputs list. This lets the output
    // class to do any extra initialization, or print something to the log
    // about itself.
    virtual void OnRegister() = 0;
    // Print the given text sent from the debug group.
    // Implementations are free to decide which message components are to be printed, and how.
    virtual void PrintMessage(const DebugMessage &msg) = 0;
};

}   // namespace Common
}   // namespace AGS

#endif // __AGS_CN_DEBUG__OUTPUTHANDLER_H

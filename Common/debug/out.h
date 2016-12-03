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
//
// Debug output interface provides functions which send a formatted message
// tagged with group ID and message type to the registered output handlers.
//
// Depending on configuration this message may be printed by any of those
// handlers, or none of them. The calling unit should not worry about where the
// message goes.
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__OUT_H
#define __AGS_CN_DEBUG__OUT_H

#include "util/string.h"

namespace AGS
{
namespace Common
{

// Message types provide distinction for debug messages by their intent
enum MessageType
{
    kDbgMsg_None                = 0,
    // Initialization messages, notify about certain engine components
    // being created and important modes initialized.
    kDbgMsg_Init                = 0x0001,
    // Debug reason is for arbitrary information about events and current
    // game state
    kDbgMsg_Debug               = 0x0002,
    // Warnings are made when unexpected or non-standart behavior
    // is detected in program, which is not immediately critical,
    // but may be a symptom of a bigger problem.
    kDbgMsg_Warn                = 0x0004,
    // Error messages are about engine not being able to perform requested
    // operation in a situation when that will affect game playability and
    // further execution.
    kDbgMsg_Error               = 0x0008,
    // Fatal errors are ones that make program abort immediately
    kDbgMsg_Fatal               = 0x0010,

    // Convenient aliases
    kDbgMsg_Default             = kDbgMsg_Debug,
    kDbgMsgSet_NoDebug          = 0x001D,
    kDbgMsgSet_Errors           = 0x0019,
    // Output everything
    kDbgMsgSet_All              = 0xFFFF
};

// This enumeration is a list of common hard-coded groups, but more could
// be added via debugging configuration interface (see 'debug/debug.h').
enum CommonDebugGroup
{
    kDbgGroup_None = -1,
    kDbgGroup_Default = 0
};

// Debug group identifier defining either numeric or string id, or both
struct DebugGroupID
{
    uint32_t    ID;
    String      SID;

    DebugGroupID() : ID(kDbgGroup_None) {}
    DebugGroupID(uint32_t id, const String &sid = "") : ID(id), SID(sid) {}
    DebugGroupID(const String &sid) : ID(kDbgGroup_None), SID(sid) {}
    // Tells if any of the id components is valid
    bool IsValid() const { return ID != kDbgGroup_None || !SID.IsEmpty(); }
    // Tells if both id components are properly set
    bool IsComplete() const { return ID != kDbgGroup_None && !SID.IsEmpty(); }
};

struct DebugMessage
{
    String       Text;
    uint32_t     GroupID;
    String       GroupName;
    MessageType  MT;

    DebugMessage() : GroupID(kDbgGroup_None), MT(kDbgMsg_None) {}
    DebugMessage(const String &text, uint32_t group_id, const String &group_name, MessageType mt)
        : Text(text)
        , GroupID(group_id)
        , GroupName(group_name)
        , MT(mt)
    {}
};

namespace Debug
{
    //
    // Debug output
    //
    // Output formatted message of default group and default type
    void Printf(const char *fmt, ...);
    // Output formatted message of default group and given type
    void Printf(MessageType mt, const char *fmt, ...);
    // Output formatted message of given group and type
    void Printf(DebugGroupID group_id, MessageType mt, const char *fmt, ...);

}   // namespace Debug

}   // namespace Common
}   // namespace AGS

#endif // __AGS_CN_DEBUG__OUT_H

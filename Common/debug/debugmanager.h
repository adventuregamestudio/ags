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
//
// AGS logging system is built with idea that the engine components should not
// be bothered with specifying particular output method. Instead they use
// generic logging interface, and the actual message printing is done by one
// or more registered handlers.
// Firstly this makes logging functions independent of running platform or
// back-end, secondly it grants end-users ability to configure output according
// to their preference.
//
// To make the logging work we need to register two sets of "entities":
// debug groups and output targets.
// Debug group is an arbitrary object with a name that describes message
// sender.
// Output target defines printing handler and a set of verbosity rules
// one per each known group.
//
// When the message is sent, it is tagged with one of the existing group IDs
// and a message type (debug info, warning, error). This message is sent onto
// each of the registered output targets, which do checks to find out whether
// the message is permitted to be sent further to the printing handler, or not.
//
//-----------------------------------------------------------------------------
//
// Thread-safety: currently each public method of DebugManager is protected
// by locking a mutex. One noteable case is sending a message to all outputs
// using DebugManager::Print: all the outputs will be processed between a
// single pair of lock/unlock. On one hand this makes the order of messages
// strict and reduces potential number of lock/unlock pairs. On another hand,
// this in theory may slow things down more in case of overly verbose logging
// from multiple threads. If that becomes an issue, we might redesign
// the locking strategy, e.g. letting separate outputs to process messages in
// parallel.
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__DEBUGMANAGER_H
#define __AGS_CN_DEBUG__DEBUGMANAGER_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include "debug/out.h"
#include "debug/outputhandler.h"
#include "util/string.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{

// Debug group identifier defining either numeric or string id, or both
struct DebugGroupID
{
    MessageGroupHandle ID = InvalidMessageGroup;
    String SID;

    DebugGroupID() = default;
    DebugGroupID(uint32_t id, const String &sid = "") : ID(id), SID(sid) {}
    DebugGroupID(const String &sid) : ID(InvalidMessageGroup), SID(sid) {}
    // Tells if any of the id components is valid
    bool IsValid() const { return ID != InvalidMessageGroup || !SID.IsEmpty(); }
    // Tells if both id components are properly set
    bool IsComplete() const { return ID != InvalidMessageGroup && !SID.IsEmpty(); }
};

// DebugGroup is a message sender definition, identified by DebugGroupID
// and providing OutputName that could be used when printing its messages.
// OutputName may or may not be same as DebugGroupID.SID.
struct DebugGroup
{
    DebugGroupID    UID;
    String          OutputName;

    DebugGroup() = default;
    DebugGroup(const DebugGroupID &id, const String &out_name)
        : UID(id), OutputName(out_name) {}
};


class MessageBuffer;

// DebugManager manages log outputs and message groups.
// All the logging goes through this Manager's Print method.
class DebugManager
{
public:
    DebugManager(bool buffer_messages);

    // Registers message group with the given string ID; numeric ID will be
    // assigned internally. "Out_name" is an optional name to use when printing
    // a message from this group, it may be empty.
    // Returns a numeric group handle.
    // If the group with such string id already exists, then returns existing ID.
    MessageGroupHandle RegisterGroup(const String &id, const String &out_name);
    // Registers message group with the pair of numeric and string IDs.
    // "Out_name" is an optional name to use when printing a message from
    // this group, it may be empty.
    // If the group with such string and/or numeric id already exists, they will be replaced.
    // Returns a numeric group handle.
    // TODO: revise this, currently is used to register default groups.
    MessageGroupHandle RegisterGroup(const DebugGroupID &group_id, const String &out_name);
    // Registers output delegate for passing debug messages to;
    // if the output with such id already exists, replaces the old one.
    void RegisterOutput(const String &id, IOutputHandler *handler, MessageType def_verbosity,
        const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters);

    // Gets a group description; returns an unfilled struct if such group does not exist.
    DebugGroup GetGroup(const DebugGroupID &id);
    // Tells if an output with the given name is already registered
    bool HasOutput(const String &id);
    // Resets message group filters for the given output
    void SetOutputFilters(const String &id, MessageType def_verbosity,
        const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters);

    // Unregisters all groups and all targets
    void UnregisterAll();
    // Unregisters debugging group with the given string ID
    void UnregisterGroup(const DebugGroupID &id);
    // Unregisters output delegate with the given string ID
    void UnregisterOutput(const String &id);

    // Begins to record messages in the internal buffer.
    // Whenever a new output is registered, any buffered messages will be
    // resent into this new output, so that it had fuller log.
    void StartMessageBuffering();
    // Stops message buffering and erases buffer.
    void StopMessageBuffering();

    // Output message of given group and message type
    void Print(MessageGroupHandle group_id, MessageType mt, const String &text);

private:
    // OutputSlot struct wraps over output target and adds a flag which indicates
    // that this target is temporarily disabled (for internal use only)
    class DebugOutput
    {
    public:
        DebugOutput() = default;
        DebugOutput(const String &id, IOutputHandler *handler,
            MessageType def_verbosity, const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters);

        const String &GetID() const { return _id; }
        void SetFilters(MessageType def_verbosity, const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters);
        void ResolveGroupID(const DebugGroupID &id);
        void SendMessage(const DebugMessage &msg);

    private:
        inline bool TestGroup(MessageGroupHandle id, MessageType mt) const
        {
            assert(id < _groupFilter.size());
            return (_groupFilter[id] >= mt);
        }

        String          _id;
        IOutputHandler *_handler = nullptr;
        bool            _suppressed = false;
        MessageType     _defaultVerbosity = kDbgMsg_None;
        // Maximal message type per group (based on numeric index)
        std::vector<MessageType> _groupFilter;
        // Set of unresolved groups, which have not been properly registered yet,
        // and which numeric IDs are not yet known. These may be added
        // when registering an Output with a group filter list.
        // Whenever a new group is registered, DebugManager will try to resolve
        // previously unresolved groups, and respective output filters.
        std::unordered_map<String, MessageType, HashStrNoCase, StrEqNoCase>
                        _unresolvedGroups;
    };

    MessageGroupHandle FindFreeGroupID();
    DebugGroup         GetGroupImpl(const DebugGroupID &id);
    MessageGroupHandle RegisterGroupImpl(const DebugGroupID &group_id, const String &out_name);
    DebugOutput &      RegisterOutputImpl(const String &id,
        IOutputHandler *handler, MessageType def_verbosity,
        const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters);
    void               SendBufferedMessages(DebugOutput &out);

    std::mutex          _mutex;
    uint32_t            _freeGroupID = 0u; // first free group numeric id
    std::vector<DebugGroup> _groups;
    std::unordered_map<String, DebugGroupID, HashStrNoCase, StrEqNoCase>
                        _groupByStrLookup;
    std::unordered_map<String, DebugOutput, HashStrNoCase, StrEqNoCase>
                        _outputs;

    // An optional message buffer
    const String OutputMsgBufID = "internal.buffer";
    std::unique_ptr<MessageBuffer> _messageBuf;
};

// TODO: move this to the dynamically allocated engine object whenever it is implemented
extern DebugManager DbgMgr;

}   // namespace Common
}   // namespace AGS

#endif // __AGS_CN_DEBUG__DEBUGMANAGER_H

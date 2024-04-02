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
#include <stdarg.h>
#include "debug/debugmanager.h"
#include "debug/messagebuffer.h"
#include "util/memory_compat.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{

DebugManager::DebugOutput::DebugOutput(const String &id,
    std::unique_ptr<IOutputHandler> &&handler,
    MessageType def_verbosity, const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters)
    : _id(id)
    , _handler(std::move(handler))
{
    SetFilters(def_verbosity, group_filters);
}

void DebugManager::DebugOutput::SetFilters(MessageType def_verbosity, const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters)
{
    _groupFilter.clear();
    _unresolvedGroups.clear();

    _defaultVerbosity = def_verbosity;
    if (group_filters)
    {
        for (const auto &gf : *group_filters)
        {
            if (gf.first.ID != InvalidMessageGroup)
            {
                if (_groupFilter.size() <= gf.first.ID)
                    _groupFilter.resize(gf.first.ID + 1, _defaultVerbosity);
                _groupFilter[gf.first.ID] = gf.second;
            }
            else
            {
                _unresolvedGroups.insert(std::make_pair(gf.first.SID, gf.second));
            }
        }
    }
}

void DebugManager::DebugOutput::ResolveGroupID(const DebugGroupID &id)
{
    assert(id.IsComplete());
    if (!id.IsComplete())
        return; // not a complete group id, unable to resolve

    if (_groupFilter.size() <= id.ID)
        _groupFilter.resize(id.ID + 1, _defaultVerbosity);
    auto it = _unresolvedGroups.find(id.SID);
    if (it != _unresolvedGroups.end())
    {
        _groupFilter[id.ID] = it->second;
        _unresolvedGroups.erase(it);
    }
}

void DebugManager::DebugOutput::SendMessage(const DebugMessage &msg)
{
    assert(_handler);
    if (_suppressed)
        return;
    if (!TestGroup(msg.GroupID, msg.MT))
        return;
    // We suppress current target before the call so that if it makes
    // a call to output system itself, message would not print to the
    // same target
    _suppressed = true;
    _handler->PrintMessage(msg);
    _suppressed = false;
}


DebugManager::DebugManager(bool buffer_messages)
{
    // Add hardcoded groups
    // TODO: move this out of DebugManager, into the engine!
    RegisterGroup(DebugGroupID(kDbgGroup_Main, "main"), "");
    RegisterGroup(DebugGroupID(kDbgGroup_Game, "game"), "Game");
    RegisterGroup(DebugGroupID(kDbgGroup_Script, "script"), "Script");
    RegisterGroup(DebugGroupID(kDbgGroup_SprCache, "sprcache"), "Sprite cache");
    RegisterGroup(DebugGroupID(kDbgGroup_ManObj, "manobj"), "Managed obj");
    RegisterGroup(DebugGroupID(kDbgGroup_SDL, "sdl"), "SDL");

    if (buffer_messages)
    {
        StartMessageBuffering();
    }
}

MessageGroupHandle DebugManager::FindFreeGroupID()
{
    if (_freeGroupID >= _groups.size())
    {
        _freeGroupID = _groups.size();
    }
    else
    {
        for (; _freeGroupID < _groups.size() && !_groups[_freeGroupID].UID.IsValid(); ++_freeGroupID) {}
    }
    return _freeGroupID;
}

MessageGroupHandle DebugManager::RegisterGroup(const String &id, const String &out_name)
{
    std::lock_guard<std::mutex> lk(_mutex);
    auto it = _groupByStrLookup.find(id);
    if (it != _groupByStrLookup.end())
    {
        _groups[it->second.ID].OutputName = out_name;
        return it->second.ID;
    }

    return RegisterGroupImpl(DebugGroupID(FindFreeGroupID(), id), out_name);
}

MessageGroupHandle DebugManager::RegisterGroup(const DebugGroupID &group_id, const String &out_name)
{
    std::lock_guard<std::mutex> lk(_mutex);
    return RegisterGroupImpl(group_id, out_name);
}

MessageGroupHandle DebugManager::RegisterGroupImpl(const DebugGroupID &group_id, const String &out_name)
{
    if (_groups.size() <= group_id.ID)
        _groups.resize(group_id.ID + 1);

    auto group = DebugGroup(group_id, out_name);
    _groups[group_id.ID] = group;
    _groupByStrLookup[group_id.SID] = group.UID;

    // Resolve group reference on every output target
    for (auto &out : _outputs)
    {
        out.second.ResolveGroupID(group.UID);
    }

    return group_id.ID;
}

void DebugManager::RegisterOutput(const String &id,
    std::unique_ptr<IOutputHandler> &&handler, MessageType def_verbosity,
    const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters)
{
    assert(handler);
    if (!handler)
        return;

    auto out = CreateOutputImpl(id, std::move(handler), def_verbosity, group_filters);
    // Only lock when inserting new output into the list (minimal time)
    std::lock_guard<std::mutex> lk(_mutex);
    _outputs[id] = std::move(out);
}

DebugManager::DebugOutput DebugManager::CreateOutputImpl(const String &id,
    std::unique_ptr<IOutputHandler> &&handler, MessageType def_verbosity,
    const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters)
{
    auto out = DebugOutput(id, std::move(handler), def_verbosity, group_filters);
    // Make sure that output allocates filters for all known groups
    for (const auto &group : _groups)
        out.ResolveGroupID(group.UID);
    out.GetHandler()->OnRegister();
    // Delegate buffered messages to this new output
    if (_messageBuf)
        SendBufferedMessages(out);
    return std::move(out);
}

void DebugManager::SendBufferedMessages(DebugOutput &out)
{
    assert(_messageBuf);
    if (!_messageBuf)
        return;

    size_t msg_lost = _messageBuf->GetMessagesLost();
    if (msg_lost > 0u)
    {
        DebugGroup gr = DbgMgr.GetGroup(kDbgGroup_Main);
        out.SendMessage(DebugMessage(String::FromFormat("WARNING: output %s lost exceeding buffer: %zu debug messages\n", out.GetID().GetCStr(), msg_lost),
            gr.UID.ID, gr.OutputName, kDbgMsg_All));
    }
    for (const auto &msg : _messageBuf->GetBuffer())
    {
        out.SendMessage(msg);
    }
}

DebugGroup DebugManager::GetGroup(const DebugGroupID &id)
{
    std::lock_guard<std::mutex> lk(_mutex);
    return GetGroupImpl(id);
}

DebugGroup DebugManager::GetGroupImpl(const DebugGroupID &id)
{
    if (id.ID != InvalidMessageGroup)
    {
        return id.ID < _groups.size() ? _groups[id.ID] : DebugGroup();
    }
    else if (!id.SID.IsEmpty())
    {
        auto it = _groupByStrLookup.find(id.SID);
        return it != _groupByStrLookup.end() ? _groups[it->second.ID] : DebugGroup();
    }
    return DebugGroup();
}

bool DebugManager::HasOutput(const String &id)
{
    std::lock_guard<std::mutex> lk(_mutex);
    return _outputs.count(id) > 0;
}

void DebugManager::SetOutputFilters(const String &id, MessageType def_verbosity,
    const std::vector<std::pair<DebugGroupID, MessageType>> *group_filters)
{
    std::lock_guard<std::mutex> lk(_mutex);
    auto it = _outputs.find(id);
    if (it == _outputs.end())
        return;

    auto &out = it->second;
    out.SetFilters(def_verbosity, group_filters);
    // Make sure that output allocates filters for all known groups
    for (const auto &group : _groups)
        _outputs[id].ResolveGroupID(group.UID);
}

void DebugManager::UnregisterAll()
{
    std::lock_guard<std::mutex> lk(_mutex);
    _groups.clear();
    _groupByStrLookup.clear();
    _outputs.clear();
    _freeGroupID = 0u;
}

void DebugManager::UnregisterGroup(const DebugGroupID &id)
{
    std::lock_guard<std::mutex> lk(_mutex);
    DebugGroup group = GetGroupImpl(id);
    if (!group.UID.IsValid())
        return;

    if (group.UID.ID < _freeGroupID)
        _freeGroupID = group.UID.ID;
    _groups[group.UID.ID] = DebugGroup();
    _groupByStrLookup.erase(group.UID.SID);
}

void DebugManager::UnregisterOutput(const String &id)
{
    std::lock_guard<std::mutex> lk(_mutex);
    _outputs.erase(id);
}

void DebugManager::StartMessageBuffering()
{
    auto msg_buf = std::make_unique<MessageBuffer>();
    auto *msg_buf_ptr = msg_buf.get();
    auto out = CreateOutputImpl(OutputMsgBufID, std::move(msg_buf), kDbgMsg_All, nullptr);

    std::lock_guard<std::mutex> lk(_mutex);
    _outputs[OutputMsgBufID] = std::move(out);
    _messageBuf = msg_buf_ptr;
}

void DebugManager::StopMessageBuffering()
{
    std::lock_guard<std::mutex> lk(_mutex);
    _messageBuf = nullptr;
    _outputs.erase(OutputMsgBufID);
}

void DebugManager::Print(MessageGroupHandle group_id, MessageType mt, const String &text)
{
    std::lock_guard<std::mutex> lk(_mutex);
    assert(group_id < _groups.size());
    if (group_id >= _groups.size())
        return;

    const DebugGroup &group = _groups[group_id];
    DebugMessage msg(text, group.UID.ID, group.OutputName, mt);
    for (auto &out : _outputs)
        out.second.SendMessage(msg);
}

// TODO: move this to the dynamically allocated engine object whenever it is implemented
DebugManager DbgMgr(true /* start buffering messages */);


namespace Debug
{

void Printf(const String &text)
{
    DbgMgr.Print(kDbgGroup_Main, kDbgMsg_Default, text);
}

void Printf(MessageType mt, const String &text)
{
    DbgMgr.Print(kDbgGroup_Main, mt, text);
}

void Printf(MessageGroupHandle group, MessageType mt, const String &text)
{
    DbgMgr.Print(group, mt, text);
}

void Printf(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    DbgMgr.Print(kDbgGroup_Main, kDbgMsg_Default, String::FromFormatV(fmt, argptr));
    va_end(argptr);
}

void Printf(MessageType mt, const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    DbgMgr.Print(kDbgGroup_Main, mt, String::FromFormatV(fmt, argptr));
    va_end(argptr);
}

void Printf(MessageGroupHandle group, MessageType mt, const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    DbgMgr.Print(group, mt, String::FromFormatV(fmt, argptr));
    va_end(argptr);
}

} // namespace Debug

}   // namespace Common
}   // namespace AGS

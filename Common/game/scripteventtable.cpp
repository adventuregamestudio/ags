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
#include "game/scripteventtable.h"
#include <algorithm>
#include <string.h>
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

void ScriptEventSchema::MakeMap()
{
    for (const auto &evt_def : EventList)
    {
        EventMap[evt_def.Name] = evt_def.Index;
    }
}

HError ScriptEventSchema::Read(Stream *in)
{
    EventsTableVersion ver = (EventsTableVersion)in->ReadInt32();
    if (ver != kEventsTable_v400)
        return new Error(String::FromFormat("ScriptEventSchema version not supported: %d", ver));

    uint32_t count = static_cast<uint32_t>(in->ReadInt32());
    EventList.resize(count);
    for (uint32_t i = 0u; i < count; ++i)
        EventList[i] = ScriptEventDefinition(StrUtil::ReadString(in), i);

    MakeMap();
    return HError::None();
}

void ScriptEventSchema::Write(Stream *out) const
{
    out->WriteInt32(kEventsTable_Current); // version
    out->WriteInt32(EventList.size());
    for (const auto &evt : EventList)
        StrUtil::WriteString(evt.Name, out);
}

bool ScriptEventSchema::CreateRemap(const ScriptEventSchema &other_schema, std::vector<uint32_t> &remap)
{
    bool need_remap = false;
    remap.resize(EventList.size());
    for (size_t this_index = 0u; this_index < EventList.size(); ++this_index)
    {
        auto it_found = other_schema.EventMap.find(EventList[this_index].Name);
        remap[this_index] = (it_found != other_schema.EventMap.end()) ? it_found->second : UINT32_MAX;
        need_remap |= remap[this_index] != this_index;
    }
    return need_remap;
}

HError ScriptEventsBase::Read(Stream *in)
{
    _handlers.clear();
    EventsTableVersion ver = (EventsTableVersion)in->ReadInt32();
    if (ver < kEventsTable_v362)
        return new Error(String::FromFormat("ScriptEventHandlers version not supported: %d", ver));

    _scriptModule = StrUtil::ReadString(in);
    const size_t evt_count = in->ReadInt32();
    _handlers.resize(evt_count);
    for (size_t i = 0; i < evt_count; ++i)
    {
        _handlers[i] = { StrUtil::ReadString(in) };
    }
    return HError::None();
}

void ScriptEventsBase::Write(Stream *out) const
{
    out->WriteInt32(kEventsTable_v362);
    StrUtil::WriteString(_scriptModule, out);
    out->WriteInt32(static_cast<uint32_t>(_handlers.size()));
    for (const auto &evt : _handlers)
    {
        StrUtil::WriteString(evt.FunctionName, out);
    }
}

void ScriptEventHandlers::Remap(const std::vector<uint32_t> &remap)
{
    // TODO: a generic variant of this operation in utility module?
    std::vector<ScriptEventHandler> new_handlers;
    new_handlers.reserve(_handlers.size());
    for (size_t i = 0; i < _handlers.size() && i < remap.size(); ++i)
    {
        uint32_t new_index = remap[i];
        if (new_index != UINT32_MAX)
        {
            if (new_index >= new_handlers.size())
                new_handlers.resize(new_index + 1);
            new_handlers[new_index] = std::move(_handlers[i]);
        }
    }
    _handlers = std::move(new_handlers);
}

void ScriptEventHandlers::Read_v361(Stream *in)
{
    _handlers.clear();
    const size_t evt_count = in->ReadInt32();
    _handlers.resize(evt_count);
    for (size_t i = 0; i < evt_count; ++i)
    {
        _handlers[i] = { String::FromStream(in) };
    }
}

HError ScriptEventHandlers::Read(Stream *in)
{
    return ScriptEventsBase::Read(in);
}

void ScriptEventHandlers::Write_v361(Stream *out) const
{
    out->WriteInt32(static_cast<uint32_t>(_handlers.size()));
    for (const auto &evt : _handlers)
    {
        evt.FunctionName.Write(out);
    }
}

void ScriptEventHandlers::Write(Stream *out) const
{
    ScriptEventsBase::Write(out);
}

/* static */ ScriptEventSchema ScriptEventTable::_defaultSchema;

void ScriptEventTable::ResetHandlers()
{
    assert(_schema);
    _handlers.resize(_schema ? _schema->EventList.size() : 0u);
}

void ScriptEventTable::ClearHandlers()
{
    assert(_schema);
    if (_schema)
    {
        _handlers.resize(_schema->EventList.size());
        for (auto &h : _handlers)
            h = {};
    }
}

void ScriptEventTable::SetHandlers(const std::vector<ScriptEventHandler> &handlers)
{
    assert(_schema);
    if (!_schema)
        return;

    if (_schema->EventList.size() == 0 || handlers.size() == 0)
    {
        ClearHandlers();
        return;
    }

    _handlers.resize(_schema->EventList.size());
    const size_t copy_n = std::min(_handlers.size(), handlers.size());
    std::copy_n(handlers.cbegin(), copy_n, _handlers.begin());
}

void ScriptEventTable::SetHandlers(const StringMap &evt_handlers)
{
    assert(_schema);
    if (!_schema)
        return;

    ClearHandlers();
    if (_schema->EventList.size() == 0 || evt_handlers.size() == 0)
        return;

    _handlers.resize(_schema->EventList.size());
    for (const auto &evt : evt_handlers)
    {
        auto it_func = _schema->EventMap.find(evt.first);
        if (it_func != _schema->EventMap.end())
            _handlers[it_func->second] = ScriptEventHandler(evt.second);
    }
}

HError ScriptEventTable::Read(Stream *in)
{
    assert(_schema);
    if (!_schema)
        return HError::None();

    ClearHandlers();
    ScriptEventHandlers handlers;
    HError err = handlers.Read(in);
    if (!err)
        return err;

    _scriptModule = handlers.GetScriptModule();
    auto h_list = handlers.GetHandlers();
    const uint32_t copy_n = std::min(_handlers.size(), h_list.size());
    std::copy_n(h_list.begin(), copy_n, _handlers.begin());
    return HError::None();
}

void ScriptEventTable::Write(Stream *out) const
{
    ScriptEventsBase::Write(out);
}

} // namespace Common
} // namespace AGS

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "game/scripteventstable.h"
#include <algorithm>
#include <string.h>
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

HError ScriptEventsSchema::Read(Stream *in)
{
    HError err = ReadInto(EventList, in);
    if (!err)
        return err;
    MakeMap();
    return HError::None();
}

void ScriptEventsSchema::Write(Stream *out) const
{
    out->WriteInt32(kEventsTable_Current); // version
    out->WriteInt32(EventList.size());
    for (const auto &evt : EventList)
        StrUtil::WriteString(evt.Name, out);
}

/* static */ HError ScriptEventsSchema::ReadInto(std::vector<ScriptEventDefinition> &event_defs, Stream *in)
{
    EventsTableVersion ver = (EventsTableVersion)in->ReadInt32();
    if (ver != kEventsTable_v400)
        return new Error(String::FromFormat("ScriptEventsSchema version not supported: %d", ver));

    uint32_t count = static_cast<uint32_t>(in->ReadInt32());
    event_defs.resize(count);
    for (uint32_t i = 0u; i < count; ++i)
        event_defs[i] = ScriptEventDefinition(StrUtil::ReadString(in), i);
    return HError::None();
}

HError ScriptEventsBase::Read(Stream *in)
{
    _handlers.clear();
    EventsTableVersion ver = (EventsTableVersion)in->ReadInt32();
    if (ver < kEventsTable_v362)
        return new Error(String::FromFormat("ScriptEventHandlers version not supported: %d", ver));

    _scriptModule = StrUtil::ReadString(in);
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        _handlers.push_back({ StrUtil::ReadString(in) });
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

void ScriptEventHandlers::Read_v361(Stream *in)
{
    _handlers.clear();
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        _handlers.push_back( { String::FromStream(in) } );
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

/* static */ ScriptEventsSchema ScriptEventsTable::_defaultSchema;

void ScriptEventsTable::ResetHandlers()
{
    assert(_schema);
    _handlers.resize(_schema ? _schema->EventList.size() : 0u);
}

void ScriptEventsTable::ClearHandlers()
{
    assert(_schema);
    if (_schema)
    {
        _handlers.resize(_schema->EventList.size());
        for (auto &h : _handlers)
            h = {};
    }
    else
    {
        _handlers.clear();
    }
}

void ScriptEventsTable::CreateHandlers(const StringMap &evt_handlers)
{
    assert(_schema);
    if (!_schema)
        return;

    _handlers.resize(_schema->EventList.size());
    if (_schema->EventList.size() == 0 || evt_handlers.size() == 0)
        return;

    for (const auto &evt : evt_handlers)
    {
        auto it_func = _schema->EventMap.find(evt.first);
        if (it_func != _schema->EventMap.end())
            _handlers[it_func->second] = ScriptEventHandler(evt.second);
    }
}

void ScriptEventsTable::CreateHandlers(const std::vector<ScriptEventDefinition> &event_defs, const std::vector<ScriptEventHandler> &handlers)
{
    assert(_schema);
    if (!_schema)
        return;

    _handlers.resize(_schema->EventList.size());
    if (_schema->EventList.size() == 0 || handlers.size() == 0)
        return;

    const uint32_t count = std::min(_handlers.size(), std::min(handlers.size(), event_defs.size()));
    for (uint32_t i = 0; i < count; ++i)
    {
        auto it_func = _schema->EventMap.find(event_defs[i].Name);
        if (it_func != _schema->EventMap.end())
            _handlers[it_func->second] = ScriptEventHandler(handlers[i]);
    }
}

HError ScriptEventsTable::Read(Stream *in, const std::vector<ScriptEventDefinition> *event_defs)
{
    ClearHandlers();

    ScriptEventHandlers handlers;
    HError err = handlers.Read(in);
    if (!err)
        return err;

    _scriptModule = handlers.GetScriptModule();
    auto h_list = handlers.GetHandlers();
    if (event_defs)
    {
        CreateHandlers(*event_defs, h_list);
    }
    else if (_schema)
    {
        const uint32_t count = std::min(_handlers.size(), h_list.size());
        for (uint32_t i = 0; i < count; ++i)
            _handlers[i] = h_list[i];
    }

    return HError::None();
}

void ScriptEventsTable::Write(Stream *out) const
{
    ScriptEventsBase::Write(out);
}

} // namespace Common
} // namespace AGS

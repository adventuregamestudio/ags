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

void ScriptEventHandlers::Read_v361(Stream *in)
{
    Handlers.clear();
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        Handlers.push_back( { String::FromStream(in) } );
    }
}

HError ScriptEventHandlers::Read_v362(Stream *in)
{
    Handlers.clear();
    EventsTableVersion ver = (EventsTableVersion)in->ReadInt32();
    if (ver < kEventsTable_v362)
        return new Error(String::FromFormat("ScriptEventHandlers version not supported: %d", ver));

    ScriptModule = StrUtil::ReadString(in);
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        Handlers.push_back( { StrUtil::ReadString(in) } );
    }
    return HError::None();
}

void ScriptEventHandlers::Write_v361(Stream *out) const
{
    out->WriteInt32(static_cast<uint32_t>(Handlers.size()));
    for (const auto &evt : Handlers)
    {
        evt.FunctionName.Write(out);
    }
}

void ScriptEventHandlers::Write_v362(Stream *out) const
{
    out->WriteInt32(kEventsTable_v362);
    StrUtil::WriteString(ScriptModule, out);
    out->WriteInt32(static_cast<uint32_t>(Handlers.size()));
    for (const auto &evt : Handlers)
    {
        StrUtil::WriteString(evt.FunctionName, out);
    }
}

HError ScriptEventsTable::Read(Stream *in)
{
    EventMap.clear();
    Handlers.clear();

    EventsTableVersion ver = (EventsTableVersion)in->ReadInt32();
    if (ver != kEventsTable_v400)
        return new Error(String::FromFormat("ScriptEventTable version not supported: %d", ver));

    ScriptModule = StrUtil::ReadString(in);
    StrUtil::ReadStringMap(EventMap, in);
    // NOTE: Handlers are not read from the stream, but resolved after reading
    // by each object, since here we are not aware of events indexes
    return HError::None();
}

void ScriptEventsTable::Write(Stream *out) const
{
    out->WriteInt32(kEventsTable_Current);
    StrUtil::WriteString(ScriptModule, out);
    StrUtil::WriteStringMap(EventMap, out);
}

} // namespace Common
} // namespace AGS

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
//
// ScriptEventsTable is table of script functions (handlers) assigned for the
// individual object's events. It consists of two parts: a map of event name
// to script function, and a flat index-based list of script functions.
// The map is read from game data, but is optional (some categories of events
// do not require one).
// The indexed list is meant for the faster events access. The indexes are
// predefined and exclusive for each object type.
//
//=============================================================================
#ifndef __AGS_CN_GAME__SCRIPTEVENTSTABLE_H
#define __AGS_CN_GAME__SCRIPTEVENTSTABLE_H

#include <memory>
#include <vector>
#include "util/error.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{

class Stream;

enum EventsTableVersion
{
    kEventsTable_Initial = 0,
    kEventsTable_v362    = 3060200,
    kEventsTable_v400    = 4000000,
    kEventsTable_Current = kEventsTable_v400
};

// Script function names, corresponding to the event's index,
// paired with Enabled flag to tell if this event handler has to be processed
struct ScriptEventHandler
{
    String FunctionName = {};
    // At runtime we may want to receive function's call result and update
    // Enabled status, but result may be delayed, so we have to use a shared memory object for safety.
    // TODO: have this in runtime-only struct, when we have a clear separation
    std::shared_ptr<bool> Enabled;

    inline bool IsEnabled() const { return Enabled && *Enabled; }

    ScriptEventHandler() = default;
    ScriptEventHandler(const String &fn_name)
        : FunctionName(fn_name), Enabled(new bool(!fn_name.IsEmpty())) {}
};

struct ScriptEventsBase
{
    // An optional name of a script module to lookup functions in
    String ScriptModule = {};
    // Flat indexed list of handlers
    std::vector<ScriptEventHandler> Handlers;

    ScriptEventsBase() = default;
    ScriptEventsBase(const ScriptEventsBase &script_events)
        : ScriptModule(script_events.ScriptModule), Handlers(script_events.Handlers)
    {}
    ScriptEventsBase(ScriptEventsBase &&script_events)
        : ScriptModule(std::move(script_events.ScriptModule)), Handlers(std::move(script_events.Handlers))
    {}

    ScriptEventsBase &operator =(const ScriptEventsBase &script_events) = default;
    ScriptEventsBase &operator =(ScriptEventsBase &&script_events) = default;
};

// A indexed list of function links for all the supported events.
struct ScriptEventHandlers : public ScriptEventsBase
{
    ScriptEventHandlers() = default;

    ScriptEventHandlers(const ScriptEventsBase &handlers)
        : ScriptEventsBase(handlers)
    {}

    ScriptEventHandlers(const ScriptEventHandlers &handlers)
        : ScriptEventsBase(handlers)
    {}

    ScriptEventHandlers(ScriptEventsBase &&handlers)
        : ScriptEventsBase(handlers)
    {}

    ScriptEventHandlers(ScriptEventHandlers &&handlers)
        : ScriptEventsBase(handlers)
    {}

    ScriptEventHandlers &operator =(const ScriptEventHandlers &handlers) = default;
    ScriptEventHandlers &operator =(ScriptEventHandlers &&handlers) = default;

    // Read pre-3.6.2 version of the ScriptEventHandlers
    // (this is still may be used by contemporary room structs, because they didn't need some data)
    void Read_v361(Stream *in);
    // Read 3.6.2+ version of the ScriptEventHandlers
    HError Read_v362(Stream *in);
    void Write_v361(Stream *out) const;
    void Write_v362(Stream *out) const;
};

// A two-part events table
struct ScriptEventsTable : public ScriptEventsBase
{
    // A map of event name to event handler
    StringMap EventMap;

    ScriptEventsTable() = default;

    ScriptEventsTable(const ScriptEventsBase &handlers)
        : ScriptEventsBase(handlers)
    {}

    ScriptEventsTable(const ScriptEventsTable &events)
        : ScriptEventsBase(events)
    {}

    ScriptEventsTable(ScriptEventsBase &&handlers)
        : ScriptEventsBase(handlers)
    {}

    ScriptEventsTable(ScriptEventsTable &&events)
        : ScriptEventsBase(events)
    {
        EventMap = std::move(events.EventMap);
    }

    ScriptEventsTable &operator =(const ScriptEventsTable &events) = default;
    ScriptEventsTable &operator =(ScriptEventsTable &&events) = default;

    // Generates a index-based Handlers list based on available EventMap
    // and provided indexed list of event names
    void CreateIndexedList(const std::vector<String> &eventnames);

    HError Read(Stream *in);
    void Write(Stream *out) const;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__SCRIPTEVENTSTABLE_H

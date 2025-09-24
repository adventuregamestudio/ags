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

// A indexed list of function links for all the supported events.
struct ScriptEventHandlers
{
    // An optional name of a script module to lookup functions in
    String ScriptModule = {};
    std::vector<ScriptEventHandler> Handlers;

    ScriptEventHandlers() = default;

    // Read pre-3.6.2 version of the ScriptEventHandlers
    // (this is still may be used by contemporary room structs, because they didn't need some data)
    void Read_v361(Stream *in);
    // Read 3.6.2+ version of the ScriptEventHandlers
    HError Read_v362(Stream *in);
    void Write_v361(Stream *out) const;
    void Write_v362(Stream *out) const;
};

// A two-part events table
struct ScriptEventsTable
{
    // An optional name of a script module to lookup functions in
    String ScriptModule = {};
    // A map of event name to event handler
    StringMap EventMap;
    // Flat indexed list of handlers
    std::vector<ScriptEventHandler> Handlers;

    ScriptEventsTable() = default;

    ScriptEventsTable(const ScriptEventHandlers &handlers)
        : ScriptModule(handlers.ScriptModule), Handlers(handlers.Handlers)
    {}

    ScriptEventsTable(ScriptEventHandlers &&handlers)
        : ScriptModule(std::move(handlers.ScriptModule)), Handlers(std::move(handlers.Handlers))
    {}

    HError Read(Stream *in);
    void Write(Stream *out) const;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__SCRIPTEVENTSTABLE_H

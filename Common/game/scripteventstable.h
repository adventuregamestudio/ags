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

// EventsTableVersion is used for both schema and table of handlers serializations
enum EventsTableVersion
{
    kEventsTable_Initial = 0,
    kEventsTable_v362 = 3060200,
    kEventsTable_v400 = 4000022,
    kEventsTable_Current = kEventsTable_v400
};

// ScriptEventDefinition describes a event, which has a name and
// a numeric index, specific for each object type. The index is
// used at runtime for simpler event handler access
struct ScriptEventDefinition
{
    // Name of event
    String Name;
    // Numeric index (meaning depends on object type, each type has its indexed table)
    uint32_t Index = 0u;

    ScriptEventDefinition() = default;
    ScriptEventDefinition(const String &name, uint32_t index)
        : Name(name), Index(index)
    {}
};

// ScriptEventsSchema defines a index-based list of events for a particular object type.
struct ScriptEventsSchema
{
    // A index-based list of event definitions, for simpler access at runtime
    std::vector<ScriptEventDefinition> EventList;
    // A lookup map for getting event definition's index
    std::unordered_map<String, uint32_t> EventMap;

    ScriptEventsSchema() = default;
    ScriptEventsSchema(const std::vector<ScriptEventDefinition> &evt_defs)
        : EventList(evt_defs)
    {
        MakeMap();
    }
    ScriptEventsSchema(std::vector<ScriptEventDefinition> &&evt_defs)
        : EventList(std::move(evt_defs))
    {
        MakeMap();
    }

    HError Read(Stream *in);
    void Write(Stream *out) const;

    static HError ReadInto(std::vector<ScriptEventDefinition> &event_defs, Stream *in);

private:
    void MakeMap()
    {
        for (const auto &evt_def : EventList)
        {
            EventMap[evt_def.Name] = evt_def.Index;
        }
    }
};

// Script function names, corresponding to the event's index,
// paired with Enabled flag to tell if this event handler has to be processed,
// and Checked flag that tells whether the handler test in script was performed.
struct ScriptEventHandler
{
    String FunctionName = {};

    bool Enabled = false;
    bool Checked = false;

    inline bool IsEnabled() const { return Enabled; }
    inline bool IsChecked() const { return Checked; }
    inline void SetChecked(bool enabled) { Checked = true; Enabled = enabled; }

    ScriptEventHandler() = default;
    ScriptEventHandler(const String &fn_name)
        // If no function name is assigned, then we disable and mark as checked right away
        : FunctionName(fn_name), Enabled(!fn_name.IsEmpty()), Checked(fn_name.IsEmpty()) {}
};

class ScriptEventsBase
{
public:
    ScriptEventsBase() = default;
    ScriptEventsBase(const ScriptEventsBase &script_events)
        : _scriptModule(script_events._scriptModule), _handlers(script_events._handlers)
    {}
    ScriptEventsBase(ScriptEventsBase &&script_events)
        : _scriptModule(std::move(script_events._scriptModule)), _handlers(std::move(script_events._handlers))
    {}

    ScriptEventsBase &operator =(const ScriptEventsBase &script_events) = default;
    ScriptEventsBase &operator =(ScriptEventsBase &&script_events) = default;

    const String &GetScriptModule() const
    {
        return _scriptModule;
    }
    void SetScriptModule(const String &sc_module)
    {
        _scriptModule = sc_module;
    }
    bool HasHandler(uint32_t evt) const
    {
        return evt < _handlers.size() && _handlers[evt].IsEnabled();
    }
    const ScriptEventHandler &GetHandler(uint32_t evt) const
    {
        assert(evt < _handlers.size());
        if (evt < _handlers.size())
            return _handlers[evt];
        return _noHandler;
    }
    ScriptEventHandler &GetHandler(uint32_t evt)
    {
        assert(evt < _handlers.size());
        if (evt < _handlers.size())
            return _handlers[evt];
        return _noHandler;
    }
    void SetHandler(uint32_t evt, const String &fn_name)
    {
        assert(evt < _handlers.size());
        if (evt < _handlers.size())
            _handlers[evt] = ScriptEventHandler(fn_name);
    }
    const std::vector<ScriptEventHandler> GetHandlers() const
    {
        return _handlers;
    }
    void ClearHandlers()
    {
        for (auto &h : _handlers)
            h = {};
    }

protected:
    HError Read(Stream *in);
    void Write(Stream *out) const;

    // An optional name of a script module to lookup functions in
    String _scriptModule = {};
    // Flat indexed list of handlers
    std::vector<ScriptEventHandler> _handlers;
    // Dummy handler to return when a wrong event index is requested
    ScriptEventHandler _noHandler;
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

    void SetHandlers(const std::vector<ScriptEventHandler> &handlers)
    {
        _handlers = handlers;
    }
    void SetHandlers(std::vector<ScriptEventHandler> &&handlers)
    {
        _handlers = std::move(handlers);
    }

    // Read pre-3.6.2 version of the ScriptEventHandlers
    // (this is still may be used by contemporary room structs, because they didn't need some data)
    void Read_v361(Stream *in);
    // Read 3.6.2+ version of the ScriptEventHandlers
    HError Read(Stream *in);
    void Write_v361(Stream *out) const;
    void Write(Stream *out) const;
};

// A two-part events table
struct ScriptEventsTable : public ScriptEventsBase
{
    ScriptEventsTable()
        : _schema(&_defaultSchema)
    {
    }

    ScriptEventsTable(const ScriptEventsSchema *schema)
        : _schema(schema)
    {
        ResetHandlers();
    }

    ScriptEventsTable(const ScriptEventsSchema *schema, const ScriptEventsBase &handlers)
        : ScriptEventsBase(handlers), _schema(schema)
    {
        ResetHandlers();
    }

    ScriptEventsTable(const ScriptEventsTable &events)
        : ScriptEventsBase(events), _schema(events._schema)
    {
        ResetHandlers();
    }

    ScriptEventsTable(const ScriptEventsSchema *schema, ScriptEventsBase &&handlers)
        : ScriptEventsBase(handlers), _schema(schema)
    {
        ResetHandlers();
    }

    ScriptEventsTable(ScriptEventsTable &&events)
        : ScriptEventsBase(std::move(static_cast<ScriptEventsBase&>(events))), _schema(std::move(events._schema))
    {
        ResetHandlers();
    }

    ScriptEventsTable &operator =(const ScriptEventsTable &events)
    {
        _schema = events._schema;
        return (*this = static_cast<const ScriptEventsBase&>(events));
    }

    ScriptEventsTable &operator =(ScriptEventsTable &&events)
    {
        _schema = std::move(events._schema);
        return (*this = std::move(static_cast<ScriptEventsBase&>(events)));
    }

    ScriptEventsTable &operator =(const ScriptEventHandlers &events)
    {
        static_cast<ScriptEventsBase&>(*this) = events;
        return *this;
    }

    ScriptEventsTable &operator =(ScriptEventHandlers &&events)
    {
        static_cast<ScriptEventsBase&>(*this) = std::move(events);
        return *this;
    }

    // Retrieves a default dummy schema that can be used to initialize empty ScriptEventsTable
    static const ScriptEventsSchema *DefaultSchema() { return &_defaultSchema; }

    // Clears all assigned handler functions
    void ClearHandlers();
    // Generates a index-based Handlers list based on provided events map,
    // using available ScriptEventsSchema to remap handlers to our inner indexes.
    void CreateHandlers(const StringMap &evt_handlers);
    // Generates a index-based Handlers list based on provided events list and handlers list,
    // using available ScriptEventsSchema to remap handlers to our inner indexes.
    void CreateHandlers(const std::vector<ScriptEventDefinition> &event_defs, const std::vector<ScriptEventHandler> &handlers);
    // Generates a index-based Handlers list based on provided events list and ScriptEventHandlers object,
    // using available ScriptEventsSchema to remap handlers to our inner indexes.
    void CreateHandlers(const std::vector<ScriptEventDefinition> &event_defs, const ScriptEventHandlers &handlers)
    {
        CreateHandlers(event_defs, handlers.GetHandlers());
    }

    // Read the list of event handlers
    HError Read(Stream *in)
    {
        return Read(in, nullptr);
    }
    // Read the list of event handlers, and remap them to the current Schema,
    // using a provided indexed events as a reference
    HError Read(const std::vector<ScriptEventDefinition> &event_defs, Stream *in)
    {
        return Read(in, &event_defs);
    }
    void Write(Stream *out) const;

private:
    void ResetHandlers();
    HError Read(Stream *in, const std::vector<ScriptEventDefinition> *event_defs);

    const ScriptEventsSchema * _schema = nullptr;
    static ScriptEventsSchema _defaultSchema;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__SCRIPTEVENTSTABLE_H

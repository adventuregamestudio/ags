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
// ScriptEventTable is table of script functions (handlers) assigned for the
// individual object's events. It is based on a ScriptEventSchema, which
// defines the number of available events and maps event names to indexed
// in that flat list of handlers.
// The indexed list is meant for the faster events access. The indexes are
// predefined and exclusive for each object type.
//
// ScriptEventHandlers is a simpler flat container without a Schema. It's used
// when it's more convenient to handle events as just an array. Also cursor
// events in game do not have a Schema right now (although they could have one
// theoretically, made of the list of cursors).
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
    kEventsTable_v400 = 4000023,
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

// ScriptEventSchema defines a index-based list of events for a particular object type.
struct ScriptEventSchema
{
    // A index-based list of event definitions, for simpler access at runtime
    std::vector<ScriptEventDefinition> EventList;
    // A lookup map for getting event definition's index
    std::unordered_map<String, uint32_t> EventMap;

    ScriptEventSchema() = default;
    ScriptEventSchema(const std::vector<ScriptEventDefinition> &evt_defs)
        : EventList(evt_defs)
    {
        MakeMap();
    }
    ScriptEventSchema(std::vector<ScriptEventDefinition> &&evt_defs)
        : EventList(std::move(evt_defs))
    {
        MakeMap();
    }

    HError Read(Stream *in);
    void Write(Stream *out) const;

    // Generates a remapping vector which tells which indexes in *other* schema
    // correspond to the respective indexes in this schema. I.e. the created vector
    // can be used as (remap[this index] => other index).
    // Those events that do not have a match are remapped to UINT32_MAX.
    // Returns whether the remap is required, otherwise the schemas are identical.
    bool CreateRemap(const ScriptEventSchema &other_schema, std::vector<uint32_t> &remap);
    HError ReadOtherAndCreateRemap(Stream *in, std::vector<uint32_t> &remap, bool &need_remap);

private:
    void MakeMap();
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

    // Remap handlers using a remap vector, which tells a new index for each old one;
    // if any new index is UINT32_MAX, then such handler is removed.
    void Remap(const std::vector<uint32_t> &remap);

    // Read pre-3.6.2 version of the ScriptEventHandlers
    // (this is still may be used by contemporary room structs, because they didn't need some data)
    void Read_v361(Stream *in);
    // Read 3.6.2+ version of the ScriptEventHandlers
    HError Read(Stream *in);
    void Write_v361(Stream *out) const;
    void Write(Stream *out) const;
};

// A two-part events table
struct ScriptEventTable : public ScriptEventsBase
{
    ScriptEventTable()
        : _schema(&_defaultSchema)
    {
    }

    ScriptEventTable(const ScriptEventSchema *schema)
        : _schema(schema)
    {
        ResetHandlers();
    }

    ScriptEventTable(const ScriptEventSchema *schema, const ScriptEventsBase &handlers)
        : ScriptEventsBase(handlers), _schema(schema)
    {
        ResetHandlers();
    }

    ScriptEventTable(const ScriptEventTable &events)
        : ScriptEventsBase(events), _schema(events._schema)
    {
        ResetHandlers();
    }

    ScriptEventTable(const ScriptEventSchema *schema, ScriptEventsBase &&handlers)
        : ScriptEventsBase(handlers), _schema(schema)
    {
        ResetHandlers();
    }

    ScriptEventTable(ScriptEventTable &&events)
        : ScriptEventsBase(std::move(static_cast<ScriptEventsBase&>(events))), _schema(std::move(events._schema))
    {
        ResetHandlers();
    }

    ScriptEventTable &operator =(const ScriptEventTable &events)
    {
        _schema = events._schema;
        return (*this = static_cast<const ScriptEventsBase&>(events));
    }

    ScriptEventTable &operator =(ScriptEventTable &&events)
    {
        _schema = std::move(events._schema);
        return (*this = std::move(static_cast<ScriptEventsBase&>(events)));
    }

    ScriptEventTable &operator =(const ScriptEventHandlers &events)
    {
        static_cast<ScriptEventsBase&>(*this) = events;
        return *this;
    }
 
    ScriptEventTable &operator =(ScriptEventHandlers &&events)
    {
        static_cast<ScriptEventsBase&>(*this) = std::move(events);
        return *this;
    }

    // Retrieves a default dummy schema that can be used to initialize empty ScriptEventTable
    static const ScriptEventSchema &DefaultSchema() { return _defaultSchema; }

    // Clears all assigned handler functions
    void ClearHandlers();
    // Assigns a index-based Handlers list, copying from the provided list and keeping indexes;
    // if any index exceeds the schema, these handlers are ignored.
    void SetHandlers(const std::vector<ScriptEventHandler> &handlers);
    void SetHandlers(const ScriptEventHandlers &handlers)
    {
        SetHandlers(handlers.GetHandlers());
    }
    // Generates a index-based Handlers list based on provided events map,
    // using internal schema to remap handlers to our inner indexes.
    void SetHandlers(const StringMap &evt_handlers);

    // Read the list of event handlers
    HError Read(Stream *in);
    void Write(Stream *out) const;

private:
    void ResetHandlers();

    const ScriptEventSchema * _schema = nullptr;
    static ScriptEventSchema _defaultSchema;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__SCRIPTEVENTSTABLE_H

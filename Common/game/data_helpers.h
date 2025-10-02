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
// Helper functions related to reading or writing game data.
//
//=============================================================================

#include "util/error.h"
#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

inline bool ReadAndAssertCount(Stream *in, const char *objname, uint32_t expected, HError &err)
{
    uint32_t count = in->ReadInt32();
    if (count != expected)
        err = new Error(String::FromFormat("Mismatching number of %s: read %u expected %u", objname, count, expected));
    return !err.HasError();
}

// Reads ScriptEventsSchema and ScriptEventsTables for an object list.
// The object list is assumed to be already precreated.
// NOTE: if there will be an issue with the Events member (diff name, protected),
// then we would require a method that sets handlers into object's event table.
// NOTE: made this a template function, because majority of objects in the engine
// do not have a shared parent class (also we work with a vector of them here...).
// Revise this later?
template <class TObj>
HError ReadScriptEventsTablesForObjects(std::vector<TObj> &objs, const char *objname, Stream *in)
{
    std::vector<ScriptEventDefinition> event_defs;
    HError err = ScriptEventsSchema::ReadInto(event_defs, in);
    if (!err)
        return err;
    if (!ReadAndAssertCount(in, objname, static_cast<uint32_t>(objs.size()), err))
        return err;
    for (size_t i = 0; i < objs.size(); ++i)
    {
        err = objs[i].Events.Read(event_defs, in);
        if (!err)
            return err;
    }
    return HError::None();
}

// Reads ScriptEventsSchema and ScriptEventsTables for an object list.
// This is a variant of above, but objects are stored in a C-style array.
// TODO: store these in a vector too at some point, and remove this variant.
template <class TObj, size_t ObjListSize>
HError ReadScriptEventsTablesForObjects(TObj (&objs)[ObjListSize], size_t obj_count, const char *objname, Stream *in)
{
    std::vector<ScriptEventDefinition> event_defs;
    HError err = ScriptEventsSchema::ReadInto(event_defs, in);
    if (!err)
        return err;
    if (!ReadAndAssertCount(in, objname, static_cast<uint32_t>(obj_count), err))
        return err;
    for (size_t i = 0; i < obj_count; ++i)
    {
        err = objs[i].Events.Read(event_defs, in);
        if (!err)
            return err;
    }
    return HError::None();
}

} // namespace Common
} // namespace AGS

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
//
// Helper functions related to reading or writing game data.
//
//=============================================================================

#include "game/scripteventtable.h"
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

// Reads ScriptEventSchema and ScriptEventsTables for an object list.
// The object list is assumed to be already precreated.
// NOTE: if there will be an issue with the Events member (diff name, protected),
// then we would require a method that sets handlers into object's event table.
// NOTE: made this a template function, because majority of objects in the engine
// do not have a shared parent class (also we work with a vector of them here...).
// Revise this later?
template <typename TObj, typename TObjIter>
HError ReadScriptEventsTablesForObjects(TObjIter begin, TObjIter end, const char *objname, Stream *in)
{
    // TODO: figure out a more optimal way for handling all the operations here,
    // perhaps join schema read and remap into the member of the ScriptEventSchema class?
    // join Handlers read and remap? anything else that may be improved?
    ScriptEventSchema schema;
    HError err = schema.Read(in);
    if (!err)
        return err;
    std::vector<uint32_t> remap;
    const bool must_remap = schema.CreateRemap(TObj::GetEventSchema(), remap);

    size_t obj_count = end - begin;
    if (!ReadAndAssertCount(in, objname, static_cast<uint32_t>(obj_count), err))
        return err;

    ScriptEventHandlers handlers;
    for (size_t i = 0; i < obj_count; ++i)
    {
        err = handlers.Read(in);
        if (!err)
            return err;
        if (must_remap)
            handlers.Remap(remap);
        (*(begin + i)).GetEvents().SetHandlers(handlers);
    }
    return HError::None();
}

// Reads ScriptEventSchema and ScriptEventsTables for an object list.
// This is a variant for the objects stored in a std::vector
template <typename TObj>
HError ReadScriptEventsTablesForObjects(std::vector<TObj> &objs, const char *objname, Stream *in)
{
    return ReadScriptEventsTablesForObjects<TObj, typename std::vector<TObj>::iterator>(objs.begin(), objs.end(), objname, in);
}

// Reads ScriptEventSchema and ScriptEventsTables for an object list.
// This is a variant for the objects stored in a C-style array.
// TODO: store these in a vector too at some point, and remove this variant.
template <typename TObj, size_t ObjListSize>
HError ReadScriptEventsTablesForObjects(TObj (&objs)[ObjListSize], size_t obj_count, const char *objname, Stream *in)
{
    return ReadScriptEventsTablesForObjects<TObj, TObj*>(objs, objs + obj_count, objname, in);
}

} // namespace Common
} // namespace AGS

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "script/cc_reflect.h"
#include <algorithm>
#include <string> // std::string
#include <string.h> // memcpy etc
#include "util/stream.h"

namespace AGS
{

using namespace Common;

// Adds a string into the string table; saves associated offset, at which
// this sting will be packed, and increases packed length.
// Returns last saved offset if the string is already in the table.
static uint32_t StrTableAdd(std::map<std::string, uint32_t> &table,
    const std::string &str, uint32_t &packed_len)
{
    const auto str_it = table.find(str);
    if (str_it != table.end())
        return str_it->second;
    
    table[str] = packed_len;
    uint32_t last_len = packed_len;
    packed_len += str.size() + 1; // count null-terminator
    return last_len;
}

// Copies a string to a string table at a new location.
// Returns the string's offset in the str_table.
static uint32_t StrTableCopy(std::vector<char> &str_table, const char *string)
{
    if (!string || !string[0])
        return 0u; // assume string table starts with "null-terminator" slot
    // If the string belongs to our string table already, then return existing offset;
    // otherwise - copy this string over to our strings
    if (!str_table.empty() && (string >= &str_table.front() && string <= &str_table.back()))
        return string - &str_table.front();

    const size_t old_packsz = str_table.size();
    const size_t new_strsz = strlen(string) + 1; // count null-terminator
    str_table.resize(str_table.size() + new_strsz);
    memcpy(&str_table.front() + old_packsz, string, new_strsz);
    return static_cast<uint32_t>(old_packsz);
}

//*****************************************************************************
// RTTI serialization
//
// RTTI header:
// ----------------
//  uint32 | format                 | for expanding the rtti format
//  uint32 | header size            | size in bytes (counting from "format")
//  uint32 | full rtti data size    | size in bytes (counting from "format")
//  uint32 | loc entry size         | fixed size of a location info in bytes
//  uint32 | locs table offset      | a relative pos of a locations table
//  uint32 | num locations          | number of locations in table
//  uint32 | type entry size        | fixed size of a type info in bytes
//  uint32 | types table offset     | a relative pos of a types table
//  uint32 | num types              | number of types in table
//  uint32 | field entry size       | fixed size of a type field info in bytes
//  uint32 | fields table offset    | a relative pos of a type fields table
//  uint32 | num type fields        | number of fields in table
//  uint32 | string table offset    | a relative pos of a strings table
//  uint32 | string table size      | size of a string table, in bytes
//
// Location Info:
// ----------------
//  uint32 | local id               | local location ID (sic)
//  uint32 | name                   | an offset in a string table
//  uint32 | flags                  |
//
// Type Info:
// ----------------
//  uint32 | local id               | local type ID
//  uint32 | name                   | an offset in a string table
//  uint32 | location id            | local location's ID
//  uint32 | parent type (local id) | local type ID; 0? if no parent
//  uint32 | type flags             |
//  uint32 | type size              | in bytes
//  uint32 | num fields             |
//  uint32 | field table index      | an index of a first field
//
// Type Field Info:
// ----------------
//  uint32 | offset                 | relative offset, in bytes
//  uint32 | name                   | an offset in a string table
//  uint32 | type (local id)        | local type ID of this field
//  uint32 | flags                  |
//  uint32 | number of elements     | for arrays, 0 = single var
//
//*****************************************************************************

RTTI RTTISerializer::Read(Stream *in)
{
    RTTI rtti;

    // RTTI Header
    const soff_t rtti_soff = in->GetPosition();
    const uint32_t format = in->ReadInt32();
    const size_t head_sz = (uint32_t)in->ReadInt32();
    const size_t full_sz = (uint32_t)in->ReadInt32();
    const size_t loc_sz = (uint32_t)in->ReadInt32();
    const size_t loc_table_off = (uint32_t)in->ReadInt32();
    const size_t loc_table_len = (uint32_t)in->ReadInt32();
    const size_t typei_sz = (uint32_t)in->ReadInt32();
    const uint32_t typei_table_off = (uint32_t)in->ReadInt32();
    const uint32_t typei_table_len = (uint32_t)in->ReadInt32();
    const size_t fieldi_sz = (uint32_t)in->ReadInt32();
    const uint32_t fieldi_table_off = (uint32_t)in->ReadInt32();
    const uint32_t fieldi_table_len = (uint32_t)in->ReadInt32();
    const uint32_t str_table_off = (uint32_t)in->ReadInt32();
    const size_t str_table_sz = (uint32_t)in->ReadInt32();

    const soff_t loc_soff = rtti_soff + loc_table_off;
    const soff_t typei_soff = rtti_soff + typei_table_off;
    const soff_t fieldi_soff = rtti_soff + fieldi_table_off;
    const soff_t str_soff = rtti_soff + str_table_off;
    const soff_t end_soff = rtti_soff + full_sz;

    // Location Infos
    in->Seek(loc_soff, kSeekBegin);
    for (size_t i = 0; i < loc_table_len; ++i)
    {
        RTTI::Location l;
        l.id = (uint32_t)in->ReadInt32();
        l.name_stri = (uint32_t)in->ReadInt32();
        l.flags = (uint32_t)in->ReadInt32();
        rtti._locs.push_back(l);
    }

    // Type Infos
    in->Seek(typei_soff, kSeekBegin);
    for (size_t i = 0; i < typei_table_len; ++i)
    {
        RTTI::Type t;
        t.this_id = (uint32_t)in->ReadInt32();
        t.name_stri = (uint32_t)in->ReadInt32();
        t.loc_id = (uint32_t)in->ReadInt32();
        t.parent_id = (uint32_t)in->ReadInt32();
        t.flags = (uint32_t)in->ReadInt32();
        t.size = (uint32_t)in->ReadInt32();
        t.field_num = (uint32_t)in->ReadInt32();
        t.field_index = (uint32_t)in->ReadInt32();
        rtti._types.push_back(t);
    }

    // Field Infos
    in->Seek(fieldi_soff, kSeekBegin);
    for (size_t i = 0; i < fieldi_table_len; ++i)
    {
        RTTI::Field f;
        f.offset = (uint32_t)in->ReadInt32();
        f.name_stri = (uint32_t)in->ReadInt32();
        f.f_typeid = (uint32_t)in->ReadInt32();
        f.flags = (uint32_t)in->ReadInt32();
        f.num_elems = (uint32_t)in->ReadInt32();
        rtti._fields.push_back(f);
    }

    // String Table
    in->Seek(str_soff, kSeekBegin);
    if (str_table_sz > 0)
    {
        rtti._strings.resize(str_table_sz);
        in->Read(&rtti._strings.front(), str_table_sz);
    }

    // Finish
    in->Seek(end_soff, kSeekBegin);

    rtti.CreateQuickRefs();
    return std::move(rtti);
}

void RTTISerializer::Write(const RTTI &rtti, Stream *out)
{
    // RTTI Header placeholder
    const soff_t rtti_soff = out->GetPosition();
    out->WriteByteCount(0, 14 * sizeof(uint32_t));

    // Location Infos
    const soff_t loc_soff = out->GetPosition();
    for (const auto &l : rtti._locs)
    {
        out->WriteInt32(l.id);
        out->WriteInt32(l.name_stri);
        out->WriteInt32(l.flags);
    }

    // Type Infos
    const soff_t typei_soff = out->GetPosition();
    for (const auto &t : rtti._types)
    {
        out->WriteInt32(t.this_id);
        out->WriteInt32(t.name_stri);
        out->WriteInt32(t.loc_id);
        out->WriteInt32(t.parent_id);
        out->WriteInt32(t.flags);
        out->WriteInt32(t.size);
        out->WriteInt32(t.field_num);
        out->WriteInt32(t.field_index);
    }

    // Field Infos
    const soff_t fieldi_soff = out->GetPosition();
    for (const auto &f : rtti._fields)
    {
        out->WriteInt32(f.offset);
        out->WriteInt32(f.name_stri);
        out->WriteInt32(f.f_typeid);
        out->WriteInt32(f.flags);
        out->WriteInt32(f.num_elems);
    }

    // String Table
    const soff_t str_soff = out->GetPosition();
    if (rtti._strings.size() > 0)
    {
        out->Write(&rtti._strings.front(), rtti._strings.size());
    }

    // Finalize, write actual RTTI header
    const soff_t end_soff = out->GetPosition();
    out->Seek(rtti_soff, kSeekBegin);
    out->WriteInt32(0); // format
    out->WriteInt32((uint32_t)(typei_soff - rtti_soff)); // header size
    out->WriteInt32((uint32_t)(end_soff - rtti_soff)); // full size
    out->WriteInt32(RTTI::Location::FileSize); // location info size
    out->WriteInt32((uint32_t)(loc_soff - rtti_soff)); // locations table offset
    out->WriteInt32(rtti._locs.size()); // number of locations
    out->WriteInt32(RTTI::Type::FileSize); // type info size
    out->WriteInt32((uint32_t)(typei_soff - rtti_soff)); // types table offset
    out->WriteInt32(rtti._types.size()); // number of types
    out->WriteInt32(RTTI::Field::FileSize); // field info size
    out->WriteInt32((uint32_t)(fieldi_soff - rtti_soff)); // fields table offset
    out->WriteInt32(rtti._fields.size()); // number of fields
    out->WriteInt32((uint32_t)(str_soff - rtti_soff)); // strings table offset
    out->WriteInt32(rtti._strings.size()); // string table size
    out->Seek(end_soff, kSeekBegin);
}

void RTTI::CreateQuickRefs()
{
    // TODO: keep this map in the RTTI struct, or extended one?
    // TODO: optimize this out for joint collection, as it has typeid = table index
    std::unordered_map<uint32_t, size_t> typeid_to_type;
    for (size_t i = 0; i < _types.size(); ++i)
    {
        typeid_to_type[_types[i].this_id] = i;
    }

    for (auto &loc : _locs)
    {
        loc.name = &_strings[loc.name_stri];
    }

    for (auto &ti : _types)
    {
        ti.name = &_strings[ti.name_stri];
        ti.location = &_locs[ti.loc_id];
        if (ti.parent_id > 0u)
            ti.parent = &_types[typeid_to_type[ti.parent_id]];
        if (ti.field_num > 0u)
        {
            ti.first_field = &_fields[ti.field_index];
            for (uint32_t index = 0; index < ti.field_num; ++index)
            {
                auto &fi = _fields[ti.field_index + index];
                fi.name = &_strings[fi.name_stri];
                fi.type = &_types[typeid_to_type[fi.f_typeid]];
                fi.owner = &_types[typeid_to_type[ti.this_id]];
                fi.prev_field = (index > 0) ? &_fields[ti.field_index + index - 1] : nullptr;
                fi.next_field = (index + 1 < ti.field_num) ? &_fields[ti.field_index + index + 1] : nullptr;
            }
        }
    }
}

void RTTIBuilder::AddLocation(const std::string &name, uint32_t loc_id, uint32_t flags)
{
    RTTI::Location loc;
    loc.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    loc.id = loc_id;
    loc.flags = flags;
    _rtti._locs.push_back(loc);
}

void RTTIBuilder::AddType(const std::string &name, uint32_t type_id,
    uint32_t loc_id, uint32_t parent_id, uint32_t flags, uint32_t size)
{
    RTTI::Type ti;
    ti.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    ti.this_id = type_id;
    ti.loc_id = loc_id;
    ti.parent_id = parent_id;
    ti.flags = flags;
    ti.size = size;
    _rtti._types.push_back(ti);
}

void RTTIBuilder::AddField(uint32_t owner_id, const std::string &name,
    uint32_t offset, uint32_t f_typeid, uint32_t flags, uint32_t num_elems)
{
    RTTI::Field fi;
    fi.offset = offset;
    fi.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    fi.f_typeid = f_typeid;
    fi.flags = flags;
    fi.num_elems = num_elems;
    _fieldIdx.insert(std::make_pair(owner_id, fi)); // match field to owner type
}

RTTI RTTIBuilder::Finalize()
{
    // Save complete string data
    _rtti._strings.resize(_strpackedLen);
    for (const auto &s : _strtable)
    { // write strings at the precalculated offsets
        memcpy(&_rtti._strings.front() + s.second, s.first.c_str(), s.first.size() + 1);
    }

    // Save complete field data;
    for (auto &ti : _rtti._types)
    {
        auto fi_range = _fieldIdx.equal_range(ti.this_id);
        if (fi_range.first == _fieldIdx.end())
            continue; // no fields for this type
        ti.field_index = _rtti._fields.size(); // save first field index
        ti.field_num = std::distance(fi_range.first, fi_range.second);
        for (auto fi_it = fi_range.first; fi_it != fi_range.second; ++fi_it)
        {
            _rtti._fields.push_back(fi_it->second);
        }
    }

    _rtti.CreateQuickRefs();

    RTTI rtti = std::move(_rtti);
    _rtti = RTTI(); // reset, in case user will want to create a new collection
    return rtti;
}

uint32_t JointRTTI::JoinLocation(const Location &loc, uint32_t uid, const char *name,
    std::unordered_map<uint32_t, uint32_t> &loc_l2g)
{
    if (uid <= _locs.size())
        _locs.resize(uid + 1);
    Location new_loc = loc;
    new_loc.id = uid;
    new_loc.name_stri = StrTableCopy(_strings, name);
    _locs[uid] = new_loc;
    loc_l2g.insert(std::make_pair(loc.id, new_loc.id));
    return new_loc.id;
}

uint32_t JointRTTI::JoinType(const Type &type, uint32_t uid, const char *name,
    const std::vector<Field> &src_fields, const std::vector<char> &src_strings,
    std::unordered_map<uint32_t, uint32_t> &type_l2g)
{
    if (uid <= _types.size())
        _types.resize(uid + 1);
    auto &type_slot = _types[uid];
    Type new_type = type;
    new_type.this_id = uid;
    new_type.name_stri = StrTableCopy(_strings, name);
    // Add new type's fields
    if (type.field_num > 0)
    {
        // If replaced type already has fields, and there's enough space in them,
        // then reuse that space; otherwise add to the end of fields array.
        // (avoid shifting fields here, TODO: perhaps have a "squash" method in JointRTTI?)
        // FIXME: we also currently assume that "placeholder" fields do not have
        // any strings assigned; if they will do eventually, we'll need to extra check for dups
        // using some kind of a map?, otherwise the string table would grow indefinitely;
        // another option is to use the aforementioned "squash" method periodically.
        uint32_t joint_fields_idx = _fields.size();
        if (type_slot.field_num >= type.field_num)
            joint_fields_idx = type_slot.field_index;
        else
            _fields.resize(_fields.size() + type.field_num);

        for (uint32_t findex = 0; findex < type.field_num; ++findex)
        {
            Field field = src_fields[type.field_index + findex];
            field.name_stri = StrTableCopy(_strings, &src_strings[field.name_stri]);
            _fields[joint_fields_idx + findex] = field;
        }
        new_type.field_index = joint_fields_idx;
    }
    type_slot = new_type;
    type_l2g.insert(std::make_pair(type.this_id, new_type.this_id));
    return new_type.this_id;
}

void JointRTTI::Join(const RTTI &rtti,
    std::unordered_map<uint32_t, uint32_t> &loc_l2g,
    std::unordered_map<uint32_t, uint32_t> &type_l2g)
{
    struct CompareLocs : public std::unary_function<const Location&, bool>
    {
      explicit CompareLocs(const Location &loc) : _loc(loc) {}
      bool operator() (const Location &arg) const { return strcmp(arg.name, _loc.name) == 0; }
      const Location &_loc;
    };

    // Will gather new entries for the cross-references post-resolution
    std::vector<uint32_t> new_types;

    // Merge in new locations and assign new global IDs
    for (const auto &local_loc : rtti._locs)
    {
        // TODO: refactor adding new and replacing old entries
        auto global_it = std::find_if(_locs.begin(), _locs.end(), CompareLocs(local_loc));
        if (global_it != _locs.end())
        { // only override if the existing loc was marked as "generated"
            if ((global_it->flags & kLoc_Generated) == 0)
            { // add a local2global match for existing loc, and skip the rest
                loc_l2g.insert(std::make_pair(local_loc.id, global_it->id));
                continue;
            }
            // replace existing entry; keep existing name string
            JoinLocation(local_loc, global_it->id, &_strings[global_it->name_stri], loc_l2g);
        }
        else
        {
            // add a new location
            JoinLocation(local_loc, _locs.size(), &rtti._strings[local_loc.name_stri], loc_l2g);
        }
    }

    // Merge in new types and assign new global IDs
    for (const auto &local_type : rtti._types)
    {
        // For the type lookups, construct the "fully qualified name"
        // by combining the location's name, and the type's own name.
        const String fullname = String::FromFormat("%s::%s",
            rtti._locs[local_type.loc_id].name, local_type.name);
        // TODO: refactor adding new and replacing old entries
        auto global_it = _rttiLookup.find(fullname);
        if (global_it != _rttiLookup.end())
        { // only override if the existing loc was marked as "generated"
            if ((_types[global_it->second].flags & kType_Generated) == 0)
            { // add a local2global match for existing type, and skip the rest
                type_l2g.insert(std::make_pair(local_type.this_id, global_it->second));
                continue;
            }
            // replace existing entry; keep existing name string
            const Type &gl_type = _types[global_it->second];
            new_types.push_back(
                JoinType(local_type, gl_type.this_id, &_strings[gl_type.name_stri],
                    rtti._fields, rtti._strings, type_l2g));
        }
        else
        {
            // add a new type
            new_types.push_back(
                JoinType(local_type, _types.size(), &rtti._strings[local_type.name_stri],
                    rtti._fields, rtti._strings, type_l2g));
            // add new type to a global lookup
            _rttiLookup.insert(std::make_pair(fullname, new_types.back()));
        }
    }

    // Resolve (remap) ID refs in the newly merged types;
    // only do this after all the new items are merged, because the types may
    // go in any order (parent may be listed after the child)
    for (uint32_t index : new_types)
    {
        RTTI::Type &type = _types[index];
        type.loc_id = loc_l2g[type.loc_id];
        if (type.parent_id > 0)
            type.parent_id = type_l2g[type.parent_id];
        // Resolve fields too
        for (uint32_t findex = 0; findex < type.field_num; ++findex)
        {
            RTTI::Field &field = _fields[type.field_index + findex];
            field.f_typeid = type_l2g[field.f_typeid];
        }
    }

    CreateQuickRefs();
}


String PrintRTTI(const RTTI &rtti)
{
    const auto &locs = rtti.GetLocations();
    const auto &types = rtti.GetTypes();

    const char *hr =   "-------------------------------------------------------------------------------";
    const char *dbhr = "===============================================================================";
    String fullstr;
    fullstr.AppendFmt("%s\nRTTI\n%s\n", dbhr, dbhr);

    for (const auto &ti : types)
    {
        fullstr.AppendFmt("%-12s(%-3u) %s\n", "Type:", ti.this_id, ti.name);
        fullstr.AppendFmt("%-12s(%-3u) %s\n", "Location:", ti.location->id, ti.location->name);
        if (ti.parent_id > 0u)
        {
            fullstr.AppendFmt("%-12s(%-3u) %s\n", "Parent:", ti.parent->this_id, ti.parent->name);
        }
        else
        {
            fullstr.AppendFmt("%-12snone\n", "Parent:");
        }

        if (ti.field_num > 0u)
        {
            fullstr.Append("Fields:\n");
            uint32_t index = 0u;
            for (const auto *fi = ti.first_field; fi; fi = fi->next_field, ++index)
            {
                fullstr.AppendFmt("%+4u |%+4u: %-24s: (%-3u) %s\n", index, fi->offset,
                    fi->name, fi->type->this_id, fi->type->name);
            }
        }
        else
        {
            fullstr.AppendFmt("%-12snone\n", "Fields:");
        }
        fullstr.AppendFmt("%s\n", hr);
    }

    fullstr.Append(dbhr);
    return fullstr;
}

} // namespace AGS

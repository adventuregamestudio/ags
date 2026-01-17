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
    if (!str_table.empty() && (string >= str_table.data() && (string < str_table.data() + str_table.size())))
        return string - str_table.data();

    const size_t old_packsz = str_table.size();
    const size_t new_strsz = strlen(string) + 1; // count null-terminator
    str_table.resize(str_table.size() + new_strsz);
    memcpy(str_table.data() + old_packsz, string, new_strsz);
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
//  uint32 | type flags             | see RTTI::TypeFlags
//  uint32 | type size              | in bytes
//  uint32 | num fields             |
//  uint32 | field table index      | an index of a first field
//
// Type Field Info:
// ----------------
//  uint32 | offset                 | relative offset, in bytes
//  uint32 | name                   | an offset in a string table
//  uint32 | type (local id)        | local type ID of this field
//  uint32 | flags                  | see RTTI::FieldFlags
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
    const uint32_t loc_table_off = (uint32_t)in->ReadInt32();
    const uint32_t loc_table_len = (uint32_t)in->ReadInt32();
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

        if (format >= FormatVersion::kFmtver_400_22)
        {
            t.base_id = (uint32_t)in->ReadInt32();
            t.dim_num = (uint32_t)in->ReadInt32();
        }

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
        in->Read(rtti._strings.data(), str_table_sz);
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
        // kFmtver_400_22
        out->WriteInt32(t.base_id);
        out->WriteInt32(t.dim_num);
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
        out->Write(rtti._strings.data(), rtti._strings.size());
    }

    // Finalize, write actual RTTI header
    const soff_t end_soff = out->GetPosition();
    out->Seek(rtti_soff, kSeekBegin);
    out->WriteInt32((uint32_t)FormatVersion::kFmtver_Latest); // format
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

RTTI::RTTI(const RTTI &rtti)
{
    *this = rtti;
}

RTTI &RTTI::operator=(const RTTI &rtti)
{
    _locs = rtti._locs;
    _types = rtti._types;
    _fields = rtti._fields;
    _strings = rtti._strings;
    CreateQuickRefs();
    return *this;
}

const RTTI::Location *RTTI::FindLocationByLocalID(uint32_t loc_id) const
{
    if (loc_id >= _locs.size())
        return nullptr;
    return &_locs[loc_id];
}

const RTTI::Type *RTTI::FindTypeByLocalID(uint32_t type_id) const
{
    // TODO: this may be optimized if "typeid_to_type" map is kept after CreateQuickRefs?
    // TODO: optimize this out for joint collection, as it has typeid = table index
    auto it = std::find_if(_types.begin(), _types.end(),
        [type_id](const Type& type) { return type.this_id == type_id; });
    return it == _types.end() ? nullptr : &*it;
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
        if (ti.base_id > 0u)
            ti.base = &_types[typeid_to_type[ti.base_id]];
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
    uint32_t loc_id, uint32_t base_id, uint32_t parent_id, uint32_t flags, uint32_t size, uint32_t dim_num)
{
    RTTI::Type ti;
    ti.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    ti.this_id = type_id;
    ti.loc_id = loc_id;
    ti.parent_id = parent_id;
    ti.flags = flags;
    ti.size = size;
    ti.base_id = base_id;
    ti.dim_num = dim_num;
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
        memcpy(_rtti._strings.data() + s.second, s.first.c_str(), s.first.size() + 1);
    }

    // Save complete field data
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
        if (type.base_id > 0)
            type.base_id = type_l2g[type.base_id];
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

void JointRTTI::AddGlobalTypeLookupAlias(const String &type_name)
{
    for (const auto &type : _types)
    {
        if (type_name.Compare(type.name) == 0)
        {
            _rttiLookup.insert(std::make_pair(type_name, type.this_id));
        }
    }
}

void JointRTTI::AddGlobalTypeLookupAliasesForLocation(const String &location)
{
    for (const auto &type : _types)
    {
        if (type.location && (location.Compare(type.location->name) == 0))
        {
            _rttiLookup.insert(std::make_pair(type.name, type.this_id));
        }
    }
}

//*****************************************************************************
// ScriptTOC serialization
//
// TOC header:
// ----------------
//  uint32 | format                 | for expanding the format
//  uint32 | header size            | size in bytes (counting from "format")
//  uint32 | full data size         | size in bytes (counting from "format")
//  uint32 | global vars entry size | fixed size of a global var info in bytes
//  uint32 | global vars table off  | a relative pos of a global vars table
//  uint32 | num global vars        | number of global vars in table
//  uint32 | functions entry size   | fixed size of a function info in bytes
//  uint32 | functions table off    | a relative pos of a functions table
//  uint32 | num functions          | number of functions in table
//  uint32 | func param entry size  | fixed size of a func param info in bytes
//  uint32 | param table offset     | a relative pos of a func param table
//  uint32 | num param fields       | number of func params in table
//  uint32 | local vars entry size  | fixed size of a local var info in bytes
//  uint32 | local vars table off   | a relative pos of a local vars table
//  uint32 | num local vars         | number of local vars in table
//  uint32 | string table offset    | a relative pos of a strings table
//  uint32 | string table size      | size of a string table, in bytes
//
// Variable Info:
// ----------------
//   int32 | offset                 | offset in script data, in bytes
//  uint32 | name                   | an offset in a string table
//  uint32 | location (local id)    | local location's ID (ref to RTTI)
//  uint32 | scope begin            | valid var's scope start, in bytecode pos
//  uint32 | scope end              | valid var's scope end, in bytecode pos
//  uint32 | variable flags         | see VariableFlags
//  uint32 | type (local id)        | local type ID of this var (ref to RTTI)
//  uint32 | field flags            | see RTTI::FieldFlags
//  uint32 | number of elements     | for arrays, 0 = single var
//
// Function Info:
// ----------------
//  uint32 | name                   | an offset in a string table
//  uint32 | location (local id)    | local location's ID (ref to RTTI)
//  uint32 | scope begin            | function's scope start, in bytecode pos
//  uint32 | scope end              | function's scope end, in bytecode pos
//  uint32 | type (local id)        | local type ID of this func (ref to RTTI)
//  uint32 | function flags         | see FunctionFlags
//  uint32 | ret value type         | local type ID of return value
//  uint32 | ret value flags        | see RTTI::FieldFlags
//  uint32 | ret value elem count   | (reserved, unusable atm)
//  uint32 | num params             | number of function parameters
//  uint32 | param table index      | an index of a first parameter
//  uint32 | num local data entries | number of related local data entries
//  uint32 | param table index      | an index of a first local data entries
//
// Func Param Info (corresponds to RTTI's Type Field Info):
// ----------------
//  uint32 | offset                 | relative offset, in bytes
//  uint32 | name                   | an offset in a string table
//  uint32 | type (local id)        | local type ID of this param
//  uint32 | flags                  | see RTTI::FieldFlags
//  uint32 | number of elements     | (reserved, unusable atm)
//
//*****************************************************************************

ScriptTOC ScriptTOCSerializer::Read(Stream *in, const RTTI *rtti)
{
    ScriptTOC toc;

    // TOC Header
    const soff_t toc_soff = in->GetPosition();
    const uint32_t format = in->ReadInt32();
    const size_t head_sz = (uint32_t)in->ReadInt32();
    const size_t full_sz = (uint32_t)in->ReadInt32();
    const uint32_t glvar_sz = (uint32_t)in->ReadInt32();
    const uint32_t glvar_table_off = (uint32_t)in->ReadInt32();
    const uint32_t glvar_table_len = (uint32_t)in->ReadInt32();
    const size_t func_sz = (uint32_t)in->ReadInt32();
    const uint32_t func_table_off = (uint32_t)in->ReadInt32();
    const uint32_t func_table_len = (uint32_t)in->ReadInt32();
    const size_t func_param_sz = (uint32_t)in->ReadInt32();
    const uint32_t func_param_table_off = (uint32_t)in->ReadInt32();
    const uint32_t func_param_table_len = (uint32_t)in->ReadInt32();
    const size_t locvar_sz = (uint32_t)in->ReadInt32();
    const uint32_t locvar_table_off = (uint32_t)in->ReadInt32();
    const uint32_t locvar_table_len = (uint32_t)in->ReadInt32();
    const uint32_t str_table_off = (uint32_t)in->ReadInt32();
    const size_t str_table_sz = (uint32_t)in->ReadInt32();

    const soff_t glvar_soff = toc_soff + glvar_table_off;
    const soff_t func_soff = toc_soff + func_table_off;
    const soff_t func_param_soff = toc_soff + func_param_table_off;;
    const soff_t locvar_soff = toc_soff + locvar_table_off;
    const soff_t str_soff = toc_soff + str_table_off;
    const soff_t end_soff = toc_soff + full_sz;

    // Global variables
    in->Seek(glvar_soff, kSeekBegin);
    for (size_t i = 0; i < glvar_table_len; ++i)
    {
        ScriptTOC::Variable var;
        var.offset = in->ReadInt32();
        var.name_stri = (uint32_t)in->ReadInt32();
        var.loc_id = (uint32_t)in->ReadInt32();
        var.scope_begin = (uint32_t)in->ReadInt32();
        var.scope_end = (uint32_t)in->ReadInt32();
        var.v_flags = (uint32_t)in->ReadInt32();
        var.f_typeid = (uint32_t)in->ReadInt32();
        var.f_flags = (uint32_t)in->ReadInt32();
        var.num_elems = (uint32_t)in->ReadInt32();
        toc._glVariables.push_back(var);
    }

    // Functions
    in->Seek(func_soff, kSeekBegin);
    for (size_t i = 0; i < func_table_len; ++i)
    {
        ScriptTOC::Function func;
        func.name_stri = (uint32_t)in->ReadInt32();
        func.loc_id = (uint32_t)in->ReadInt32();
        func.scope_begin = (uint32_t)in->ReadInt32();
        func.scope_end = (uint32_t)in->ReadInt32();
        func.f_typeid = (uint32_t)in->ReadInt32();
        func.flags = (uint32_t)in->ReadInt32();
        func.rv_typeid = (uint32_t)in->ReadInt32();
        func.rv_flags = (uint32_t)in->ReadInt32();
        func.rv_num_elems = (uint32_t)in->ReadInt32();
        func.param_num = (uint32_t)in->ReadInt32();
        func.param_index = (uint32_t)in->ReadInt32();
        func.local_data_num = (uint32_t)in->ReadInt32();
        func.local_data_index = (uint32_t)in->ReadInt32();
        toc._functions.push_back(func);
    }

    // Function parameters
    in->Seek(func_param_soff, kSeekBegin);
    for (size_t i = 0; i < func_param_table_len; ++i)
    {
        ScriptTOC::FunctionParam fp;
        fp.offset = (uint32_t)in->ReadInt32();
        fp.name_stri = (uint32_t)in->ReadInt32();
        fp.f_typeid = (uint32_t)in->ReadInt32();
        fp.flags = (uint32_t)in->ReadInt32();
        fp.num_elems = (uint32_t)in->ReadInt32();
        toc._fparams.push_back(fp);
    }

    // Local variables
    in->Seek(locvar_soff, kSeekBegin);
    for (size_t i = 0; i < locvar_table_len; ++i)
    {
        ScriptTOC::Variable var;
        var.offset = in->ReadInt32();
        var.name_stri = (uint32_t)in->ReadInt32();
        var.loc_id = (uint32_t)in->ReadInt32();
        var.scope_begin = (uint32_t)in->ReadInt32();
        var.scope_end = (uint32_t)in->ReadInt32();
        var.v_flags = (uint32_t)in->ReadInt32();
        var.f_typeid = (uint32_t)in->ReadInt32();
        var.f_flags = (uint32_t)in->ReadInt32();
        var.num_elems = (uint32_t)in->ReadInt32();
        toc._locVariables.push_back(var);
    }

    // String Table
    in->Seek(str_soff, kSeekBegin);
    if (str_table_sz > 0)
    {
        toc._strings.resize(str_table_sz);
        in->Read(toc._strings.data(), str_table_sz);
    }

    // Finish
    in->Seek(end_soff, kSeekBegin);

    toc.CreateQuickRefs(rtti);
    return std::move(toc);
}

void ScriptTOCSerializer::Write(const ScriptTOC &toc, Stream *out)
{
    // TOC Header placeholder
    const soff_t toc_soff = out->GetPosition();
    out->WriteByteCount(0, 17 * sizeof(uint32_t));

    // Global variables
    uint32_t glvar_count = 0u;
    const soff_t glvar_soff = out->GetPosition();
    for (const auto &var : toc._glVariables)
    {
        out->WriteInt32(var.offset);
        out->WriteInt32(var.name_stri);
        out->WriteInt32(var.loc_id);
        out->WriteInt32(var.scope_begin);
        out->WriteInt32(var.scope_end);
        out->WriteInt32(var.v_flags);
        out->WriteInt32(var.f_typeid);
        out->WriteInt32(var.f_flags);
        out->WriteInt32(var.num_elems);
        glvar_count++;
    }

    // Functions
    const soff_t func_soff = out->GetPosition();
    for (const auto &func : toc._functions)
    {
        out->WriteInt32(func.name_stri);
        out->WriteInt32(func.loc_id);
        out->WriteInt32(func.scope_begin);
        out->WriteInt32(func.scope_end);
        out->WriteInt32(func.f_typeid);
        out->WriteInt32(func.flags);
        out->WriteInt32(func.rv_typeid);
        out->WriteInt32(func.rv_flags);
        out->WriteInt32(func.rv_num_elems);
        out->WriteInt32(func.param_num);
        out->WriteInt32(func.param_index);
        out->WriteInt32(func.local_data_num);
        out->WriteInt32(func.local_data_index);
    }

    // Function parameters
    const soff_t func_params_soff = out->GetPosition();
    for (const auto &fp : toc._fparams)
    {
        out->WriteInt32(fp.offset);
        out->WriteInt32(fp.name_stri);
        out->WriteInt32(fp.f_typeid);
        out->WriteInt32(fp.flags);
        out->WriteInt32(fp.num_elems);
    }

    // Local variables
    const soff_t locvar_soff = out->GetPosition();
    uint32_t locvar_count = 0u;
    for (const auto &var : toc._locVariables)
    {
        out->WriteInt32(var.offset);
        out->WriteInt32(var.name_stri);
        out->WriteInt32(var.loc_id);
        out->WriteInt32(var.scope_begin);
        out->WriteInt32(var.scope_end);
        out->WriteInt32(var.v_flags);
        out->WriteInt32(var.f_typeid);
        out->WriteInt32(var.f_flags);
        out->WriteInt32(var.num_elems);
        locvar_count++;
    }

    // String Table
    const soff_t str_soff = out->GetPosition();
    if (toc._strings.size() > 0)
    {
        out->Write(toc._strings.data(), toc._strings.size());
    }

    // Finalize, write actual TOC header
    const soff_t end_soff = out->GetPosition();
    out->Seek(toc_soff, kSeekBegin);
    out->WriteInt32(0); // format
    out->WriteInt32((uint32_t)(glvar_soff - toc_soff)); // header size
    out->WriteInt32((uint32_t)(end_soff - toc_soff)); // full size
    out->WriteInt32(ScriptTOC::Variable::FileSize); // global var size
    out->WriteInt32((uint32_t)(glvar_soff - toc_soff)); // global var table offset
    out->WriteInt32(glvar_count); // number of global variables
    out->WriteInt32(ScriptTOC::Function::FileSize); // function size
    out->WriteInt32((uint32_t)(func_soff - toc_soff)); // function table offset
    out->WriteInt32(toc._functions.size()); // number of functions
    out->WriteInt32(ScriptTOC::FunctionParam::FileSize); // function param size
    out->WriteInt32((uint32_t)(func_params_soff - toc_soff)); // function param table offset
    out->WriteInt32(toc._fparams.size()); // number of function params
    out->WriteInt32(ScriptTOC::Variable::FileSize); // local var size
    out->WriteInt32((uint32_t)(locvar_soff - toc_soff)); // local var table offset
    out->WriteInt32(locvar_count); // number of local variables
    out->WriteInt32((uint32_t)(str_soff - toc_soff)); // strings table offset
    out->WriteInt32(toc._strings.size()); // string table size
    out->Seek(end_soff, kSeekBegin);
}

ScriptTOC::ScriptTOC(const ScriptTOC &toc)
{
    *this = toc;
}

ScriptTOC &ScriptTOC::operator=(const ScriptTOC &toc)
{
    _glVariables = toc._glVariables;
    _locVariables = toc._locVariables;
    _functions = toc._functions;
    _fparams = toc._fparams;
    _strings = toc._strings;
    // FIXME: this is dangerous because of how RTTI is referenced using raw pointer
    CreateQuickRefs(toc._rtti);
    return *this;
}

void ScriptTOC::RebindRTTI(RTTI *rtti)
{
    CreateQuickRefs(rtti);
}

void ScriptTOC::CreateQuickRefs(const RTTI *rtti)
{
    for (auto &var : _glVariables)
    {
        var.name = &_strings[var.name_stri];
    }

    for (auto &var : _locVariables)
    {
        var.name = &_strings[var.name_stri];
    }

    for (auto &fn : _functions)
    {
        fn.name = &_strings[fn.name_stri];
        if (fn.param_num > 0u)
        {
            fn.first_param = &_fparams[fn.param_index];
            for (uint32_t index = 0; index < fn.param_num; ++index)
            {
                auto &fp = _fparams[fn.param_index + index];
                fp.name = &_strings[fp.name_stri];
                fp.owner = &fn;
                fp.prev_field = (index > 0) ? &_fparams[fn.param_index + index - 1] : nullptr;
                fp.next_field = (index + 1 < fn.param_num) ? &_fparams[fn.param_index + index + 1] : nullptr;
            }
        }
        if (fn.local_data_num > 0u)
        {
            fn.local_data = &_locVariables[fn.local_data_index];
            for (uint32_t index = 0; index < fn.local_data_num; ++index)
            {
                auto &var = _locVariables[fn.local_data_index + index];
                var.function = &fn;
                var.prev_local = (index > 0) ? &_locVariables[fn.local_data_index + index - 1] : nullptr;
                var.next_local = (index + 1 < fn.local_data_num) ? &_locVariables[fn.local_data_index + index + 1] : nullptr;
            }
        }
    }

    _rtti = rtti;
    if (rtti)
    {
        for (auto &var : _glVariables)
        {
            var.location = rtti->FindLocationByLocalID(var.loc_id);
            var.type = rtti->FindTypeByLocalID(var.f_typeid);
        }
        for (auto &var : _locVariables)
        {
            var.location = rtti->FindLocationByLocalID(var.loc_id);
            var.type = rtti->FindTypeByLocalID(var.f_typeid);
        }
        for (auto &fn : _functions)
        {
            fn.location = rtti->FindLocationByLocalID(fn.loc_id);
            fn.return_type = rtti->FindTypeByLocalID(fn.rv_typeid);
        }
        for (auto &fp : _fparams)
        {
            fp.type = rtti->FindTypeByLocalID(fp.f_typeid);
        }
    }
}

void ScriptTOCBuilder::AddGlobalVar(const std::string &name, uint32_t loc_id,
    uint32_t offset, uint32_t v_flags, uint32_t f_type_id, uint32_t f_flags,
    uint32_t num_elems)
{
    ScriptTOC::Variable var;
    var.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    var.loc_id = loc_id;
    var.offset = offset;
    var.v_flags = v_flags
        & ~(ScriptTOC::kVariable_Local | ScriptTOC::kVariable_Parameter);
    var.f_typeid = f_type_id;
    var.f_flags = f_flags;
    var.num_elems = num_elems;
    _toc._glVariables.push_back(var);
}

void ScriptTOCBuilder::AddLocalVar(const std::string &name, uint32_t loc_id,
    uint32_t offset, uint32_t scope_begin, uint32_t scope_end, uint32_t v_flags,
    uint32_t f_type_id, uint32_t f_flags, uint32_t num_elems)
{
    ScriptTOC::Variable var;
    var.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    var.loc_id = loc_id;
    var.offset = offset;
    var.scope_begin = scope_begin;
    var.scope_end = scope_end;
    var.v_flags = v_flags | (ScriptTOC::kVariable_Local);
    var.f_typeid = f_type_id;
    var.f_flags = f_flags;
    var.num_elems = num_elems;
    _toc._locVariables.push_back(var);
}

uint32_t ScriptTOCBuilder::AddFunction(const std::string &name, uint32_t loc_id,
    uint32_t scope_begin, uint32_t scope_end, uint32_t flags,
    uint32_t rv_typeid, uint32_t rv_flags)
{
    ScriptTOC::Function func;
    func.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    func.loc_id = loc_id;
    func.scope_begin = scope_begin;
    func.scope_end = scope_end;
    func.flags = flags;
    func.rv_typeid = rv_typeid;
    func.rv_flags = rv_flags;
    _toc._functions.push_back(func);
    return static_cast<uint32_t>(_toc._functions.size() - 1);
}

void ScriptTOCBuilder::AddFunctionParam(uint32_t func_id, const std::string &name, uint32_t offset,
    uint32_t f_typeid, uint32_t flags)
{
    ScriptTOC::FunctionParam fi;
    fi.offset = offset;
    fi.name_stri = StrTableAdd(_strtable, name, _strpackedLen);
    fi.f_typeid = f_typeid;
    fi.flags = flags;
    _paramIdx.insert(std::make_pair(func_id, fi)); // match field to owner type
}

ScriptTOC ScriptTOCBuilder::Finalize(const RTTI *rtti)
{
    // Save complete string data
    _toc._strings.resize(_strpackedLen);
    for (const auto &s : _strtable)
    { // write strings at the precalculated offsets
        memcpy(_toc._strings.data() + s.second, s.first.c_str(), s.first.size() + 1);
    }

    // Sort global vars by offset
    std::sort(_toc._glVariables.begin(), _toc._glVariables.end(),
        [](const ScriptTOC::Variable &first, const ScriptTOC::Variable &second) { return first.offset < second.offset; });

    // Sort functions by scope
    std::sort(_toc._functions.begin(), _toc._functions.end(),
        [](const ScriptTOC::Function &first, const ScriptTOC::Function &second) { return first.scope_begin < second.scope_begin; });

    // Sort local entries by scope and offset
    auto &local_vars = _toc._locVariables;
    std::sort(local_vars.begin(), local_vars.end(), ScriptTOC::ScopedVariableLess());

    // Save complete function data, with params and local vars refs
    for (uint32_t i = 0; i < _toc._functions.size(); ++i)
    {
        auto &fn = _toc._functions[i];
        // Find prepared parameters for this function
        auto fi_range = _paramIdx.equal_range(i);
        if (fi_range.first != _paramIdx.end())
        {
            fn.param_index = _toc._fparams.size(); // save first param index
            fn.param_num = std::distance(fi_range.first, fi_range.second);
            for (auto fi_it = fi_range.first; fi_it != fi_range.second; ++fi_it)
            {
                _toc._fparams.push_back(fi_it->second);
            }
        }
        // Scan sorted local data for anything that matches this function's scope
        ScriptTOC::Variable test; test.scope_begin = fn.scope_begin; test.scope_end = fn.scope_end;
        auto lv_range = std::equal_range(local_vars.begin(), local_vars.end(), test, ScriptTOC::ScopedVariableLessScope());
        if (lv_range.first != lv_range.second)
        {
            fn.local_data_index = lv_range.first - local_vars.begin(); // save first entry index
            fn.local_data_num = lv_range.second - lv_range.first; // local data number
            // Fixup local entries (CHECKME: perhaps do this upon loading instead? or both?)
            for (auto it = lv_range.first; it != lv_range.second; ++it)
            {
                auto &var = *it;
                var.loc_id = fn.loc_id;
                var.scope_begin = std::min(fn.scope_end, std::max(fn.scope_begin, var.scope_begin));
                var.scope_end = std::min(fn.scope_end, std::max(fn.scope_begin, var.scope_end));
            }
        }
    }

    _toc.CreateQuickRefs(rtti);

    ScriptTOC toc = std::move(_toc);
    _toc = ScriptTOC(); // reset, in case user will want to create a new collection
    return toc;
}

//*****************************************************************************
//
// Additional helpers.
//
//*****************************************************************************

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
        if (ti.base_id > 0u)
        {
            fullstr.AppendFmt("%-12s(%-3u) %s\n", "Base:", ti.base->this_id, ti.base->name);
        }
        if (ti.parent_id > 0u)
        {
            fullstr.AppendFmt("%-12s(%-3u) %s\n", "Parent:", ti.parent->this_id, ti.parent->name);
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

static void FmtField(String &s, const RTTI::Type *t, uint32_t f_flags, const char *f_name)
{
    s.AppendFmt("%s%s%s %s", t->name,
        (f_flags & RTTI::kField_ManagedPtr) ? "*" : "",
        (f_flags & RTTI::kField_Array) ? "[]" : "",
        f_name ? f_name : "");
}

String PrintScriptTOC(const ScriptTOC &toc, const char *scriptname)
{
    const auto glvars = toc.GetGlobalVariables();
    const auto funcs = toc.GetFunctions();
    const auto locvars = toc.GetLocalVariables();

    const char *hr =   "-------------------------------------------------------------------------------";
    const char *dbhr = "===============================================================================";
    String fullstr;
    fullstr.AppendFmt("%s\nScript '%s' Table of Contents\n%s\n", dbhr, scriptname, dbhr);

    // Exclude imported variables from this list
    uint32_t own_glvars = 0u;
    for (const auto &var : glvars)
        if ((var.v_flags & ScriptTOC::kVariable_Import) == 0)
            own_glvars++;

    fullstr.AppendFmt("Global variables (%d):\n%s\n", own_glvars, hr);
    for (const auto &var : glvars)
    {
        if ((var.v_flags & ScriptTOC::kVariable_Import) != 0)
            continue;
        fullstr.AppendFmt("%-8u: ", var.offset);
        FmtField(fullstr, var.type, var.f_flags, var.name);
        fullstr.AppendChar('\n');
    }

    // Exclude imported functions from this list
    uint32_t own_funcs = 0u;
    for (const auto &fn : funcs)
        if ((fn.flags & ScriptTOC::kFunction_Import) == 0)
            own_funcs++;

    fullstr.AppendFmt("%s\nFunctions (%d):\n%s\n", dbhr, own_funcs, hr);
    for (const auto &fn : funcs)
    {
        if ((fn.flags & ScriptTOC::kFunction_Import) != 0)
            continue;
        FmtField(fullstr, fn.return_type, fn.rv_flags, nullptr);
        fullstr.AppendFmt("%s ( ", fn.name);
        for (const auto *p = fn.first_param; p; p = p->next_field)
        {
            if (p != fn.first_param)
                fullstr.Append(", ");
            FmtField(fullstr, p->type, p->flags, p->name);
        }
        if (fn.flags & ScriptTOC::kFunction_Variadic)
            fullstr.Append(", ...");
        fullstr.AppendFmt(" )\n    Bytecode scope: %u - %u\n", fn.scope_begin, fn.scope_end);

        if (fn.local_data_num > 0u)
        {
            fullstr.AppendFmt("Local data (%d):\n", fn.local_data_num);
            for (const auto *var = fn.local_data; var; var = var->next_local)
            {
                fullstr.AppendFmt("%u - %u:%+4d: ", var->scope_begin, var->scope_end, var->offset);
                FmtField(fullstr, var->type, var->f_flags, var->name);
                fullstr.AppendChar('\n');
            }
        }
        fullstr.AppendFmt("%s\n", hr);
    }

    fullstr.Append(dbhr);
    return fullstr;
}

} // namespace AGS

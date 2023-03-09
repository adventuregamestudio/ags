//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "script/cc_reflect.h"
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

//*****************************************************************************
// RTTI serialization
//
// RTTI header:
// ----------------
//  uint32 | format                 | for expanding the rtti format
//  uint32 | header size            | size in bytes (counting from "format")
//  uint32 | full rtti data size    | size in bytes (counting from "format")
//  uint32 | type entry size        | fixed size of a type info in bytes
//  uint32 | types table offset     | a relative pos of a types table
//  uint32 | num types              | number of types in table
//  uint32 | field entry size       | fixed size of a type field info in bytes
//  uint32 | fields table offset    | a relative pos of a type fields table
//  uint32 | num type fields        | number of fields in table
//  uint32 | string table offset    | a relative pos of a strings table
//  uint32 | string table size      | size of a string table, in bytes
//
// Type Info:
// ----------------
//  uint32 | fully qualified name   | an offset in a string table
//  uint32 | local id               | local type ID
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

void RTTI::Read(Stream *in)
{
    // RTTI Header
    const soff_t rtti_soff = in->GetPosition();
    const uint32_t format = in->ReadInt32();
    const size_t head_sz = (uint32_t)in->ReadInt32();
    const size_t full_sz = (uint32_t)in->ReadInt32();
    const size_t typei_sz = (uint32_t)in->ReadInt32();
    const uint32_t typei_table_off = (uint32_t)in->ReadInt32();
    const uint32_t typei_table_len = (uint32_t)in->ReadInt32();
    const size_t fieldi_sz = (uint32_t)in->ReadInt32();
    const uint32_t fieldi_table_off = (uint32_t)in->ReadInt32();
    const uint32_t fieldi_table_len = (uint32_t)in->ReadInt32();
    const uint32_t str_table_off = (uint32_t)in->ReadInt32();
    const size_t str_table_sz = (uint32_t)in->ReadInt32();

    const soff_t typei_soff = rtti_soff + typei_table_off;
    const soff_t fieldi_soff = rtti_soff + fieldi_table_off;
    const soff_t str_soff = rtti_soff + str_table_off;
    const soff_t end_soff = rtti_soff + full_sz;

    // Type Infos
    in->Seek(typei_soff, kSeekBegin);
    for (size_t i = 0; i < typei_table_len; ++i)
    {
        RTTI::Type t;
        t.fullname_stri = (uint32_t)in->ReadInt32();
        t.this_id = (uint32_t)in->ReadInt32();
        t.parent_id = (uint32_t)in->ReadInt32();
        t.flags = (uint32_t)in->ReadInt32();
        t.size = (uint32_t)in->ReadInt32();
        t.field_num = (uint32_t)in->ReadInt32();
        t.field_index = (uint32_t)in->ReadInt32();
        _types.push_back(t);
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
        _fields.push_back(f);
    }

    // String Table
    in->Seek(str_soff, kSeekBegin);
    if (str_table_sz > 0)
    {
        _strings.resize(str_table_sz);
        in->Read(&_strings.front(), str_table_sz);
    }

    // Finish
    in->Seek(end_soff, kSeekBegin);

    CreateQuickRefs();
}

void RTTI::Write(Stream *out) const
{
    // RTTI Header placeholder
    const soff_t rtti_soff = out->GetPosition();
    out->WriteByteCount(0, 11 * sizeof(uint32_t));

    // Type Infos
    const soff_t typei_soff = out->GetPosition();
    for (const auto &t : _types)
    {
        out->WriteInt32(t.fullname_stri);
        out->WriteInt32(t.this_id);
        out->WriteInt32(t.parent_id);
        out->WriteInt32(t.flags);
        out->WriteInt32(t.size);
        out->WriteInt32(t.field_num);
        out->WriteInt32(t.field_index);
    }

    // Field Infos
    const soff_t fieldi_soff = out->GetPosition();
    for (const auto &f : _fields)
    {
        out->WriteInt32(f.offset);
        out->WriteInt32(f.name_stri);
        out->WriteInt32(f.f_typeid);
        out->WriteInt32(f.flags);
        out->WriteInt32(f.num_elems);
    }

    // String Table
    const soff_t str_soff = out->GetPosition();
    if (_strings.size() > 0)
    {
        out->Write(&_strings.front(), _strings.size());
    }

    // Finalize, write actual RTTI header
    const soff_t end_soff = out->GetPosition();
    out->Seek(rtti_soff, kSeekBegin);
    out->WriteInt32(0); // format
    out->WriteInt32((uint32_t)(typei_soff - rtti_soff)); // header size
    out->WriteInt32((uint32_t)(end_soff - rtti_soff)); // full size
    out->WriteInt32(7 * sizeof(uint32_t)); // type info size
    out->WriteInt32((uint32_t)(typei_soff - rtti_soff)); // types table offset
    out->WriteInt32(_types.size()); // number of types
    out->WriteInt32(5 * sizeof(uint32_t)); // field info size
    out->WriteInt32((uint32_t)(fieldi_soff - rtti_soff)); // fields table offset
    out->WriteInt32(_fields.size()); // number of fields
    out->WriteInt32((uint32_t)(str_soff - rtti_soff)); // strings table offset
    out->WriteInt32(_strings.size()); // string table size
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

    for (auto &ti : _types)
    {
        ti.fullname = &_strings[ti.fullname_stri];
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

void RTTIBuilder::AddType(const std::string &name, uint32_t type_id,
    uint32_t parent_id, uint32_t flags, uint32_t size)
{
    RTTI::Type ti;
    ti.fullname_stri = StrTableAdd(_strtable, name, _strpackedLen);
    ti.this_id = type_id;
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

RTTI &&RTTIBuilder::Finalize()
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

    return std::move(_rtti);
}

} // namespace AGS

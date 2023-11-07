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
//
// Script reflection helpers. Intended to analyze script memory.
//
//=============================================================================
#ifndef __CC_REFLECT_H
#define __CC_REFLECT_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/types.h"
#include "util/string.h"
#include "util/string_types.h"

namespace AGS
{

namespace Common { class Stream; }

class RTTIBuilder;
class RTTISerializer;
class JointRTTI;

// Runtime type information for the AGS script:
// contains tables of types and their inner fields.
// Type ids are arbitrary numbers that strictly correspond to the particular
// context (such as individual script, for instance), and not necessarily
// sequential (may have gaps). For a globally unique identifier -
// use a "fully qualified name" instead: in a format of "locname::typename",
// where "locname" is a name of location and "typename" is a name of type.
class RTTI
{
    friend RTTIBuilder;
    friend RTTISerializer;
    friend JointRTTI;
public:
    enum LocationFlags
    {
        // We use "generated" flag to mark locs that are created at runtime
        // and are intended to be replaced by "true" locs with the same id
        kLoc_Generated    = 0x80000000
    };

    enum TypeFlags
    {
        kType_Struct      = 0x0001,
        kType_Managed     = 0x0002,
        // We use "generated" flag to mark types that are created at runtime
        // and are intended to be replaced by "true" types with the same id
        kType_Generated   = 0x80000000
    };

    enum FieldFlags
    {
        kField_ManagedPtr = 0x0001,
        kField_Array      = 0x0002,
        // We use "generated" flag to mark fields that are created at runtime
        // and are intended to be replaced by "true" fields later
        kField_Generated  = 0x80000000
    };

    // An "undefined type" id value
    const static uint32_t NoType = 0u;
    // Size of a "pointer" in the script memory
    const static size_t PointerSize = sizeof(uint32_t);

    struct Field;

    // Location info: a context, in which a symbol
    // (type, function, variable) may be defined.
    struct Location
    {
        friend RTTI; friend RTTIBuilder; friend RTTISerializer; friend JointRTTI;
    public:
        const static size_t FileSize = 3 * sizeof(uint32_t);
        uint32_t id = 0u; // location's id
        uint32_t flags = 0u; // location flags
        // Quick-access links
        const char *name = nullptr;
    private:
        // Internal references
        uint32_t name_stri = 0u; // location's name (string table offset)
    };

    // Type's info
    struct Type
    {
        friend RTTI; friend RTTIBuilder; friend RTTISerializer; friend JointRTTI;
    public:
        const static size_t FileSize = 8 * sizeof(uint32_t);
        uint32_t this_id = 0u; // this type's id (local to current RTTI struct)
        uint32_t loc_id = 0u; // type location's id (script or header)
        uint32_t parent_id = 0u; // parent type's id
        uint32_t flags = 0u; // type flags
        uint32_t size = 0u; // type size in bytes
        uint32_t field_num = 0u; // number of fields, if any
        // Quick-access links
        // Type's name; along with location's name will create a
        // "fully qualified name" suitable for uniquely identify this type
        // in the global scope ("locationname::typename").
        const char *name = nullptr;
        const Location *location = nullptr;
        const Type *parent = nullptr;
        const Field *first_field = nullptr;
    private:
        // Internal references
        uint32_t name_stri = 0u; // type's name (string table offset)
        uint32_t field_index = 0u; // first field index in the fields table
    };

    // Type's field info
    struct Field
    {
        friend RTTI; friend RTTIBuilder; friend RTTISerializer; friend JointRTTI;
    public:
        const static size_t FileSize = 5 * sizeof(uint32_t);
        uint32_t offset = 0u; // relative offset of this field, in bytes
        uint32_t f_typeid = 0u; // field's type id
        uint32_t flags = 0u; // field flags
        uint32_t num_elems = 0u; // number of elements (for array)
        // Quick-access links
        const char *name = nullptr;
        const Type *type = nullptr;
        const Type *owner = nullptr;
        const Field *prev_field = nullptr;
        const Field *next_field = nullptr;
    private:
        // Internal references
        uint32_t name_stri = 0u; // field's name (string table offset)
    };

    RTTI()
    {
        _strings.push_back(0); // guarantee zero-len string at index 0
    }

    bool IsEmpty() const { return _types.empty(); }
    // Returns list of locations.
    const std::vector<Location> &GetLocations() const { return _locs; }
    // Returns list of types. Please be aware that the order of them
    // in collection is not defined, and an index in the list is not
    // guaranteed to match typeid at all.
    const std::vector<Type> &GetTypes() const { return _types; }

private:
    // Generates quick reference fields, binding table entries between each other
    void CreateQuickRefs();

    // Location (type context) definitions
    std::vector<Location> _locs;
    // Type descriptions
    std::vector<Type> _types;
    // Type fields' descriptions
    std::vector<Field> _fields;
    // All RTTI strings packed, separated by null-terminators
    std::vector<char> _strings;
};

// A helper class that implements RTTI serialization in the dedicated format.
class RTTISerializer
{
public:
    // Reads the RTTI collection from the stream
    static RTTI Read(AGS::Common::Stream *in);
    // Writes the RTTI collection to the stream
    static void Write(const RTTI &rtti, AGS::Common::Stream *out);
};

// A helper class that lets you generate RTTI collection.
// Use Add* methods to construct list of types and their members,
// then call Finalize which returns a constructed RTTI object.
class RTTIBuilder
{
public:
    RTTIBuilder() = default;
    // Adds a location entry
    void AddLocation(const std::string &name, uint32_t loc_id, uint32_t flags);
    // Adds a type entry
    void AddType(const std::string &name, uint32_t type_id, uint32_t loc_id,
        uint32_t parent_id, uint32_t flags, uint32_t size);
    // Adds a type's field entry
    void AddField(uint32_t owner_id, const std::string &name, uint32_t offset,
        uint32_t f_typeid, uint32_t flags, uint32_t num_elems);
    // Finalizes the RTTI, generates remaining data based on collected one
    RTTI Finalize();

private:
    // RTTI that is being built
    RTTI _rtti;
    // Helper fields
    std::multimap<uint32_t, RTTI::Field> _fieldIdx; // type id to fields list
    std::map<std::string, uint32_t> _strtable; // string to offset
    uint32_t _strpackedLen = 0u; // packed string table size
};

// A class which supports merging RTTI collections together.
// Internally remaps typeids from individual (aka local) rtti collection to
// a joint (aka global) one.
// Guarantees that the types' indexes in collection are matching their typeid
// (unlike common RTTI).
class JointRTTI : private RTTI
{
public:
    const RTTI &AsConstRTTI() const { return *this; }

    using RTTI::IsEmpty;
    using RTTI::GetLocations;
    using RTTI::GetTypes;

    // Merges one rtti into another; skips type duplicates using fully qualified names.
    // Writes location and type local-to-global maps, which may be used by the
    // external user to match local script's type with a global one.
    void Join(const RTTI &rtti,
        std::unordered_map<uint32_t, uint32_t> &loc_l2g,
        std::unordered_map<uint32_t, uint32_t> &type_l2g);

private:
    // Map fully-qualified type name to a joint (global) typeid
    std::unordered_map<AGS::Common::String, uint32_t> _rttiLookup;

    uint32_t JoinLocation(const Location &loc, uint32_t uid, const char *name,
        std::unordered_map<uint32_t, uint32_t> &loc_l2g);
    uint32_t JoinType(const Type &type, uint32_t uid, const char *name,
        const std::vector<Field> &src_fields, const std::vector<char> &src_strings,
        std::unordered_map<uint32_t, uint32_t> &type_l2g);
};



// Prints RTTI types and their fields into the string.
// TODO: provide TextWriter instead of returning a String,
// but need to implement a TextWriter that writes into the engine's log
AGS::Common::String PrintRTTI(const RTTI &rtti);

} // namespace AGS

#endif // __CC_REFLECT_H

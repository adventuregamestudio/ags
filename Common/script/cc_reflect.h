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
// Script reflection helpers. Intended to analyze script memory.
//
//-----------------------------------------------------------------------------
//
// NOTES on possible refactor:
//
// 1) Consider "de-optimizing" RTTI class and make it more user-friendly.
// A bit of explanation:
//    RTTI is serialized packed in a database-style format, where each entry
//    has fixed size in memory (on disk, and in RAM, if unseralized as-is).
//    Anything of dynamic size, such as strings, is gathered as large array
//    and appended as an extension, while table entries store reference
//    indexes to such array.
//    This is good for a file storage; this format is very easy to parse,
//    expand, and supports both forward and backward compatibility.
//    But it looks "off" when represented as a struct.
//    If desired, RTTI class and internal structs may be refactored into
//    something more convenient for a common usage in program code.
//    If that's done, then RTTIBuilder class should be likely merged with
//    RTTISerializer, as data has still be packed prior writing it to a file,
//    and unpacked when reading from a file.
//    This may also let to get rid of uses of "friend" classes here.
// EDIT: at the very least, Type, Field etc structs may be split into a
//    fully public struct with basic fields, and extended structs with extras,
//    such as "quick reference links".
// Same refers to ScriptTOC class.
//
// 2) Generic "table building" and serialization.
//    The code that constructs and (un)serializes RTTI may be factored into a
//    generic (un)packing and serialization algorithm for any similar format:
//    a series of tables of fixed-sized entries, that have anything of dynamic
//    size in a separate table or list, and where entries reference each other
//    by a integer indexes.
//
//-----------------------------------------------------------------------------
//
// TODOs and FIXMEs:
//
// * Because of a design oversight, multi-dimensional arrays are not described
//   correctly, there's no way to know number of dimensions, and therefore
//   get an actual type of an element of a jagged array (dyn array of arrs).
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
//
// TODO: support Function Types: that is a type of function pointer;
// such types might be required for function pointer variables (or delegates).
// Function Type should correspond to a particular function prototype.
// In this case Fields could represent return value and parameters.
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
        const static size_t FileSize = 3 * sizeof(uint32_t);
    public:
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
        const static size_t FileSize = 8 * sizeof(uint32_t);
    public:
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
        const static size_t FileSize = 5 * sizeof(uint32_t);
    public:
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

    RTTI(const RTTI &rtti) = default;
    RTTI(RTTI &&rtti) = default;

    RTTI &operator = (const RTTI &rtti) = default;
    RTTI &operator = (RTTI &&rtti) = default;

    bool IsEmpty() const { return _types.empty(); }
    // Returns list of locations.
    const std::vector<Location> &GetLocations() const { return _locs; }
    // Returns list of types. Please be aware that the order of them
    // in collection is not defined, and an index in the list is not
    // guaranteed to match typeid at all.
    const std::vector<Type> &GetTypes() const { return _types; }
    // Finds a location by its local id
    const Location *FindLocationByLocalID(uint32_t loc_id) const;
    // Finds a type by its local id
    const Type *FindTypeByLocalID(uint32_t type_id) const;

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


class ScriptTOCBuilder;
class ScriptTOCSerializer;

// Table of Contents for the script:
// contains tables of functions and variables, both global and local ones.
// Has strictly debugging and diagnostic purposes. Runtime script execution
// must not depend on this table.
//
// Function infos define function's scope in bytecode positions,
// return value and a list of parameters.
// Local variables are defined in their respective lifetime Scope, a range
// of script's bytecode positions in which this variable is valid.
// Local variables table also includes function's named parameters. This
// creates certain duplication of data, but makes it easier to cherry pick
// data when parsing a file, or scan local script data at runtime.
//
// IMPORTANT: ScriptDataTOC depends on RTTI, but may be still used without one
// to a certain extent (types and location ids won't be resolved).
class ScriptTOC
{
    friend ScriptTOCBuilder; friend ScriptTOCSerializer;
public:
    enum VariableFlags
    {
        // Variable is imported from elsewhere (only global variable);
        // location id tells location of import declaration (not variable),
        // offset value is unknown until runtime and must be ignored
        kVariable_Import     = 0x0001,
        // Local variable, allocated on stack;
        // scope has meaning only for local variables;
        // variable's offset is saved as a positive number,
        // where first non-parameter variable is at 0
        kVariable_Local      = 0x0002,
        // Is a function's parameter, allocated on stack;
        // variable's offset is saved as a negative number
        kVariable_Parameter  = 0x0004
    };

    enum FunctionFlags
    {
        // Function is imported from elsewhere;
        // location id tells location of import declaration (not function),
        // scope is unknown until runtime and must be ignored
        kFunction_Import    = 0x0001,
        // A variadic function; its param_num tells number of fixed params
        kFunction_Variadic  = 0x0002
    };

    struct Function;

    // Variable's info,
    // serves for both global and local vars (including function parameters).
    // Any variable may be uniquely identified by:
    //     script + type of mem + scope + offset
    // For global variables "script + offset" is enough.
    // Note that "location id" tells where in script this variable was defined,
    // but not where it is allocated. This has strictly diagnostic purposes.
    // For local variables' offset contains relative address of
    // this variable in function data on stack, where 0 is right after the last
    // function parameter (local variables have offset >= 0, while the function
    // parameters have offset < 0).
    // The variable's scope is in bytecode pos; this makes it possible to
    // know precisely whether variable is allocated at the given position
    // of execution. (Scope is ignored for global variables.)
    struct Variable
    {
        friend ScriptTOC; friend ScriptTOCBuilder; friend ScriptTOCSerializer;
        const static size_t FileSize = 9 * sizeof(uint32_t);
    public:
        int32_t  offset = 0;  // offset of this variable, in bytes;
            // this is either global or function data offset;
            // function parameters have negative offset values
        uint32_t loc_id = 0u; // variable's location's id (where defined)
        uint32_t scope_begin = 0u; // lifetime scope start (in bytecode pos)
        uint32_t scope_end = 0u; // lifetime scope end (in bytecode pos)
        uint32_t v_flags = 0u; // variable's flags
        uint32_t f_typeid = 0u; // field's type id
        uint32_t f_flags = 0u; // field flags
        uint32_t num_elems = 0u; // number of elements (for array)
        // Quick-access links
        const char *name = nullptr;
        const RTTI::Location *location = nullptr;
        const RTTI::Type *type = nullptr;
        // Quick links for local variables only
        const Function *function = nullptr; // for local vars
        const Variable *prev_local = nullptr;
        const Variable *next_local = nullptr;
    private:
        // Internal references
        uint32_t name_stri = 0u; // variable's name (string table offset)
    };

    struct FunctionParam;

    // Function's info
    // Any function may be uniquely identified by:
    //     script + scope begin
    // TODO: might we require an explicit "local function id"? check the function pointers idea.
    // Note that "location id" tells where in script this function was defined,
    // but not which script's bytecode it is in. This has strictly diagnostic
    // purposes.
    struct Function
    {
        friend ScriptTOC; friend ScriptTOCBuilder; friend ScriptTOCSerializer;
        const static size_t FileSize = 13 * sizeof(uint32_t);
    public:
        uint32_t loc_id = 0u; // variable's location's id (where defined)
        uint32_t scope_begin = 0u; // execution scope start (in bytecode pos)
        uint32_t scope_end = 0u; // execution scope end (in bytecode pos)
        uint32_t f_typeid = 0u; // function's type id (type of prototype, reserved)
        uint32_t flags = 0u; // function's flags
        uint32_t rv_typeid = 0u; // return value's type id
        uint32_t rv_flags = 0u; // return value's field flags
        uint32_t rv_num_elems = 0u; // return value number of elements (reserved)
        uint32_t param_num = 0u; // number of params, if any
        uint32_t local_data_num = 0u; // number of related entries in the local vars table
        // Quick-access links
        const char *name = nullptr;
        const RTTI::Location *location = nullptr;
        const RTTI::Type *return_type = nullptr;
        const FunctionParam *first_param = nullptr;
        const Variable *local_data = nullptr;

        inline uint32_t GetLocalDataIndex() const { return local_data_index; }
    private:
        // Internal references
        uint32_t name_stri = 0u; // variable's name (string table offset)
        uint32_t param_index = 0u; // first param index in the params table
        uint32_t local_data_index = 0u; // first index in the local variables table
    };

    // Function param's info
    // FIXME: practically copies RTTI::Field, the only difference
    // are "quick-access links". Refactor this, pick out base struct, etc...
    struct FunctionParam
    {
        friend ScriptTOC; friend ScriptTOCBuilder; friend ScriptTOCSerializer;
        const static size_t FileSize = 5 * sizeof(uint32_t);
    public:
        uint32_t offset = 0u; // relative offset of this field, in bytes
        uint32_t f_typeid = 0u; // field's type id
        uint32_t flags = 0u; // field flags
        uint32_t num_elems = 0u; // number of elements (for array)
        // Quick-access links
        const char *name = nullptr;
        const RTTI::Type *type = nullptr;
        const Function *owner = nullptr;
        const FunctionParam *prev_field = nullptr;
        const FunctionParam *next_field = nullptr;
    private:
        // Internal references
        uint32_t name_stri = 0u; // field's name (string table offset)
    };

    ScriptTOC()
    {
        _strings.push_back(0); // guarantee zero-len string at index 0
    }

    ScriptTOC(const ScriptTOC &rtti) = default;
    ScriptTOC(ScriptTOC &&rtti) = default;

    ScriptTOC &operator = (const ScriptTOC &rtti) = default;
    ScriptTOC &operator = (ScriptTOC &&rtti) = default;

    bool IsEmpty() const { return _glVariables.empty() && _functions.empty(); }
    // Returns list of global variables
    const std::vector<Variable> &GetGlobalVariables() const { return _glVariables; }
    // Returns list of local variables
    const std::vector<Variable> &GetLocalVariables() const { return _locVariables; }
    // Returns list of functions
    const std::vector<Function> &GetFunctions() const { return _functions; }

    //
    // Various helpers
    //
    // Predicate used to sort scoped variables by scope and offset
    struct ScopedVariableLess
    {
        bool operator() (const ScriptTOC::Variable &first, const ScriptTOC::Variable &second) const
        { return (first.scope_begin < second.scope_begin) ||
                 (first.scope_begin == second.scope_begin) && (first.offset < second.offset); }
    };
    // Predicate used to find whether a scoped variable lies inside particular parent scope
    struct ScopedVariableLessScope
    {
        bool operator() (const ScriptTOC::Variable &first, const ScriptTOC::Variable &second) const
        { return (first.scope_begin < second.scope_begin) && (first.scope_end <= second.scope_begin); }
    };

private:
    // Generates quick reference fields, binding table entries between each other;
    // optionally lets link to RTTI types and locations
    void CreateQuickRefs(const RTTI *rtti);

    // An optional reference to RTTI
    const RTTI *_rtti = nullptr;
    // Global variables' descriptions
    std::vector<Variable> _glVariables;
    // Local variables' descriptions (includes all allocated loc data, fn params too)
    std::vector<Variable> _locVariables;
    // Function descriptions
    std::vector<Function> _functions;
    // Function params' descriptions
    std::vector<FunctionParam> _fparams;
    // All TOC strings packed, separated by null-terminators
    std::vector<char> _strings;
};

// A helper class that implements ScriptTOC serialization in the dedicated format.
class ScriptTOCSerializer
{
public:
    // Reads the RTTI collection from the stream
    static ScriptTOC Read(AGS::Common::Stream *in, const RTTI *rtti);
    // Writes the RTTI collection to the stream
    static void Write(const ScriptTOC &toc, AGS::Common::Stream *out);
};

// A helper class that lets you generate ScriptDataTOC collection.
// Use Add* methods to construct list of types and their members,
// then call Finalize which returns a constructed ScriptDataTOC object.
class ScriptTOCBuilder
{
public:
    ScriptTOCBuilder() = default;
    // Add a global variable entry
    void AddGlobalVar(const std::string &name, uint32_t loc_id,
        uint32_t offset, uint32_t v_flags, uint32_t f_type_id, uint32_t f_flags,
        uint32_t num_elems);
    // Adds a function entry; returns assigned function's index
    uint32_t AddFunction(const std::string &name, uint32_t loc_id,
        uint32_t scope_begin, uint32_t scope_end, uint32_t flags,
        uint32_t rv_typeid, uint32_t rv_flags);
    // Add a function's parameter
    void AddFunctionParam(uint32_t func_id, const std::string &name, uint32_t offset,
        uint32_t f_typeid, uint32_t flags);
    // Add a local variable entry, defined by lifetime scope (inside a function)
    void AddLocalVar(const std::string &name, uint32_t loc_id,
        uint32_t offset, uint32_t scope_begin, uint32_t scope_end, uint32_t v_flags,
        uint32_t f_type_id, uint32_t f_flags, uint32_t num_elems);
    // Finalizes the ScriptTOC, generates remaining data based on collected one;
    // optionally lets link to RTTI types and locations
    ScriptTOC Finalize(const RTTI *rtti);

private:
    // ScriptTOC that is being built
    ScriptTOC _toc;
    // Helper fields
    std::multimap<uint32_t, ScriptTOC::FunctionParam> _paramIdx; // function id to param list
    std::map<std::string, uint32_t> _strtable; // string to offset
    uint32_t _strpackedLen = 0u; // packed string table size
};


// Prints RTTI types and their fields into the string.
// TODO: provide TextWriter instead of returning a String,
// but need to implement a TextWriter that writes into the engine's log
AGS::Common::String PrintRTTI(const RTTI &rtti);
// Prints Script TOC into the string
AGS::Common::String PrintScriptTOC(const ScriptTOC &toc, const char *scriptname);

} // namespace AGS

#endif // __CC_REFLECT_H

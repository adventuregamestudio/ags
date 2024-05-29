//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "debug/memory_inspect.h"
#include <algorithm>
#include "ac/dynobj/dynobj_manager.h"
#include "script/cc_instance.h"
#include "script/systemimports.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace AGS
{
namespace Engine
{
namespace MemoryInspect
{

// Resolves script memory using a memory pointer, and a instruction list ("mem_ref").
// Fills in memory value formatted according to its type.
// Returns whether was successful.
// NOTE: this function is run recursively, until reaching end of instruction list.
//
// Instruction format:
//   offset[,type[:offset,type[:...]]
//
//   offset is in bytes
//   type can be:
//     - c    - char (1 byte integer)
//     - iN   - integer of given size in bytes, e.g. i1, i2, i4
//     - fN   - float of given size in bytes, e.g. f4
//     - dN   - plain data (struct, plain array), optionally of given size in bytes
//     - s    - plain string (null-terminated sequences of chars)
//     - p    - pointer, reserved
//     - h    - handle (managed), an int32 that may be resolved to a pointer
//   each entry in this instruction list is resolved to a new memory ptr,
//   according to its type, and the following entry is read relative to that
//   new memory ptr.
//   "mem_sz" argument, as well as data sizes met in the instruction list are
//   used to prevent reading past valid memory range.
//
// TODO: value formatting option?
static bool ResolveMemory(const uint8_t *mem_ptr, size_t mem_sz,
    const String &mem_ref, const size_t parse_at, String &value)
{
    assert(mem_ptr);
    assert(!mem_ref.IsEmpty());
    if (!mem_ptr || mem_ref.IsEmpty())
        return false;

    int offset = atoi(&mem_ref[parse_at]);
    const size_t type_at = mem_ref.FindChar(',', parse_at);
    const size_t next_off_at = mem_ref.FindChar(':', parse_at);
    char type = 0;
    size_t size = 0u;
    
    if ((type_at != String::NoIndex) && (type_at < next_off_at))
    {
        type = mem_ref[type_at + 1];
        if (type_at + 2 < mem_ref.GetLength())
            size = atoi(&mem_ref[type_at + 2]);
    }
    else
    {
        // default to 'i4'
        type = 'i';
        size = 4u;
    }

    if (offset < 0 || ((mem_sz != 0) && (static_cast<uint32_t>(offset) >= mem_sz + size)))
        return false; // attempt to read beyond valid range

    mem_ptr = mem_ptr + offset;
    if (next_off_at != String::NoIndex)
    {
        // Resolve sub-entry, if possible
        switch (type)
        {
        case 'd': // plain data, keep same mem ptr
            mem_sz -= offset;
            break;
        case 'p': // value of a address, reserved but not supported atm
            return false;
        case 'h': // managed handle, resolve to managed object addr
        {
            const int32_t *value_ptr = reinterpret_cast<const int32_t*>(mem_ptr);
            const void *addr = ccGetObjectAddressFromHandle(*value_ptr);
            if (!addr)
                return false; // object not found
            mem_ptr = reinterpret_cast<const uint8_t*>(addr);
            mem_sz = 0u; // FIXME... how to get it? use dynamic manager? need to expand iface
            break;
        }
        default: // invalid type for sub-entries, fail
            return false;
        }

        return ResolveMemory(mem_ptr, mem_sz, mem_ref, next_off_at + 1, value);
    }

    // Resolve final entry
    switch (type)
    {
    case 'c': // byte printed as a character
    {
        const int8_t *value_ptr = reinterpret_cast<const int8_t*>(mem_ptr);
        value = String::FromFormat("%c", *value_ptr);
        break;
    }
    case 'i': // integer printed as a number
        switch (size)
        {
        case 1:
        {
            const int8_t *value_ptr = reinterpret_cast<const int8_t*>(mem_ptr);
            value = String::FromFormat("%d", *value_ptr);
            break;
        }
        case 2:
        {
            const int16_t *value_ptr = reinterpret_cast<const int16_t*>(mem_ptr);
            value = String::FromFormat("%d", *value_ptr);
            break;
        }
        case 4:
        {
            const int32_t *value_ptr = reinterpret_cast<const int32_t*>(mem_ptr);
            value = String::FromFormat("%d", *value_ptr);
            break;
        }
        default: // unknown type, fail
            return false;
        }
        break;
    case 'f': // float printed as a number
        switch (size)
        {
        case 4:
        {
            const float *value_ptr = reinterpret_cast<const float*>(mem_ptr);
            value = String::FromFormat("%f", *value_ptr);
            break;
        }
        default: // unknown type, fail
            return false;
        }
        break;
    case 'd': // plain data... not supported yet
        // TODO: convert requested size to base64?
        break;
    case 's': // string pointer, print as a string
        value = reinterpret_cast<const char*>(mem_ptr);
        break;
    case 'p': // value of a address, reserved
        break;
    case 'h': // value of a managed handle (int32)
    {
        const int32_t *value_ptr = reinterpret_cast<const int32_t*>(mem_ptr);
        value = String::FromFormat("%d", *value_ptr);
        break;
    }
    default: // unknown type, fail
        return false;
    }
    return true;
}

// Writes (appends) a memory read instruction into the provided string
static void WriteMemReadInstruction(String &mem_ref, const RTTI::Type &type, uint32_t f_flags, uint32_t f_offset)
{
    // Using hardcoded type names here, is there another way?...
    // TODO: perhaps generate a table of basic types, avoid testing name every time
    char typec;
    uint32_t typesz;
    if ((f_flags & RTTI::kField_ManagedPtr) != 0)
    {
        typec = 'h'; typesz = 4; // managed handle int32, must be resolved
    }
    else if ((f_flags & RTTI::kField_Array) != 0)
    {
        typec = 'd'; typesz = 0; // plain data; FIXME: full array size!
    }
    else if (strcmp(type.name, "char") == 0)
    {
        typec = 'i'; typesz = 1;
    }
    else if (strcmp(type.name, "short") == 0)
    {
        typec = 'i'; typesz = 2;
    }
    else if (strcmp(type.name, "int") == 0)
    {
        typec = 'i'; typesz = 4;
    }
    else if (strcmp(type.name, "float") == 0)
    {
        typec = 'f'; typesz = 4;
    }
    else if (strcmp(type.name, "string") == 0)
    {
        typec = 's'; typesz = 200; // old-style string of fixed size
    }
    else
    {
        typec = 'd'; typesz = type.size; // plain data or unknown type
    }

    mem_ref.AppendFmt("%u,%c%u", f_offset, typec, typesz);
}

// Writes (appends) a memory read instruction for array element into the provided string
static void WriteMemReadElemInstruction(String &mem_ref, const RTTI::Type &type, uint32_t arr_index)
{
    // Array of pointers or array of PODs?
    if ((type.flags & RTTI::kType_Managed) != 0)
    {
        WriteMemReadInstruction(mem_ref, type, RTTI::kField_ManagedPtr, RTTI::PointerSize * arr_index);
    }
    else
    {
        WriteMemReadInstruction(mem_ref, type, 0u, type.size * arr_index);
    }
}

static bool TryGetGlobalVariable(const String &field_ref, const ccInstance *inst,
    const ScriptTOC::Variable *&found_var, const void *&found_mem_ptr, size_t &found_mem_sz, size_t &use_var_offset)
{
    const auto &toc = *inst->instanceof->sctoc;
    if (toc.GetGlobalVariables().empty())
        return false; // no global data

    auto var_it = inst->GetGlobalVariableLookup().find(field_ref);
    if (var_it == inst->GetGlobalVariableLookup().end())
        return false; // cannot find a global variable

    const auto &var = toc.GetGlobalVariables()[var_it->second];
    // If this is a script's own global variable, then simply reference its global memory
    if ((var.v_flags & ScriptTOC::kVariable_Import) == 0)
    {
        found_var = &var;
        found_mem_ptr = inst->globaldata;
        found_mem_sz = inst->globaldatasize;
        use_var_offset = var.offset;
        return true;
    }
    // If it's an imported variable, then this may be memory from another script,
    // but also from the engine or plugin!
    else
    {
        // TODO: following may be simplified by recording variable ptr in runtime TOC
        const ScriptImport *import = simp.getByName(var.name);
        if (!import)
            return false;

        found_var = &var;
        if (import->InstancePtr)
        {
            // Import from another script
            use_var_offset = (static_cast<const uint8_t*>(import->Value.GetDirectPtr()) - reinterpret_cast<const uint8_t*>(import->InstancePtr->globaldata));
            found_mem_ptr = import->InstancePtr->globaldata;
            found_mem_sz = import->InstancePtr->globaldatasize;
        }
        else
        {
            // Import from the engine or plugin
            // FIXME: we might have to return whole RuntimeScriptValue along,
            // and then use IScriptObject (if present) for reading type's fields
            found_mem_ptr = import->Value.GetDirectPtr();
            found_mem_sz = 0u; // FIXME: get from var type?
            use_var_offset = 0u;
        }
        return true;
    }
}

static bool TryGetLocalVariable(const String &field_ref, const ccInstance *inst,
    const ScriptTOC::Variable *&found_var, const void *&found_mem_ptr, size_t &found_mem_sz, size_t &use_var_offset)
{
    const auto &toc = *inst->instanceof->sctoc;
    if (toc.GetLocalVariables().empty())
        return false; // no local data

    // TODO: use runtime TOC where we can guarantee sorted function list (or fixup TOC on load)
    ScriptTOC::Function test_func; test_func.scope_begin = inst->pc; test_func.scope_end = inst->pc + 1;
    auto func_it = std::lower_bound(toc.GetFunctions().begin(), toc.GetFunctions().end(), test_func,
        [](const ScriptTOC::Function &first, const ScriptTOC::Function &second)
        { return (first.scope_begin < second.scope_begin) && (first.scope_end <= second.scope_begin); });

    if (func_it == toc.GetFunctions().end())
        return false; // no matching function found (unexpected...)
    if (!func_it->local_data)
        return false; // no local data in this function

    const auto &func = *func_it;
    // Find the latest variable which scope begins prior to the current script pos
    const ScriptTOC::Variable *var = func.local_data;
    for (const ScriptTOC::Variable *next_var = var;
        next_var && next_var->scope_begin <= static_cast<uint32_t>(inst->pc);
        var = next_var, next_var = next_var->next_local);

    // FIXME: helper method returning stack? don't direct access!
    // Note this stack ptr is set *after* the last allocated local data
    const auto *stack_ptr = inst->registers[SREG_SP].RValue;
    const ScriptTOC::Variable *last_var = nullptr;
    // Scan local variable backwards before we find one that matches the name
    for (; var; var = var->prev_local)
    {
        assert(var->v_flags & ScriptTOC::kVariable_Local);
        // Skip if local variable's scope ends before current pos;
        // note we don't break, as this may be a nested scope inside a function
        if (var->scope_end <= static_cast<uint32_t>(inst->pc))
            continue;

        --stack_ptr;

        // HACK: detect when we reach function parameter list,
        // and skip 1 extra entry on stack, reserved for the return value;
        // FIXME: can we use variable's offset parameter?
        if ((var->v_flags & ScriptTOC::kVariable_Parameter) &&
            (!last_var || (last_var->v_flags & ScriptTOC::kVariable_Parameter) == 0))
        {
            --stack_ptr;
        }

        if (strcmp(var->name, field_ref.GetCStr()) == 0)
        {
            found_var = var;
            // FIXME: this is horrible...
            if (stack_ptr->Type < kScValStackPtr)
                found_mem_ptr = &stack_ptr->IValue;
            else
                found_mem_ptr = stack_ptr->GetDirectPtr();
            found_mem_sz = stack_ptr->Size;
            use_var_offset = 0u;
            return true;
        }

        last_var = var;
    }
    return false;
}

// Try getting a registered variable from the current script's global memory,
// or local memory (stack); or, if this is an imported variable, then lookup
// the import table for its real address.
// TODO: don't pass array_index, apply separately, outside of this func
static bool ParseScriptVariable(const String &field_ref, const ccInstance *inst,
    const RTTI &rtti,
    String &mem_ref, const uint8_t *&found_mem_ptr, size_t &found_mem_sz,
    const RTTI::Type *&next_field_type)
{
    const ScriptTOC::Variable *var_ptr = nullptr;
    const void *mem_ptr = nullptr;
    size_t mem_sz = 0u;
    // NOTE: we cannot use found variable's offset directly, because the some
    // are not stored in script memory, and also the script VM's stack has a special storage.
    size_t use_var_offset = 0u;

    // First try local data
    if (!TryGetLocalVariable(field_ref, inst, var_ptr, mem_ptr, mem_sz, use_var_offset))
    {
        // Then try script's global variable;
        // this includes imported symbols from other scripts, plugins or engine
        if (!TryGetGlobalVariable(field_ref, inst, var_ptr, mem_ptr, mem_sz, use_var_offset))
        {
            return false;
        }
    }

    // Add found variable to the instruction list
    const auto &var = *var_ptr;
    // resolve local script's type to a global type index
    // todo: this should be resolved after loading script, similar to RTTI!
    const auto *l2gtypes = &inst->GetLocal2GlobalTypeMap();
    auto type_it = l2gtypes->find(var.f_typeid);
    if (type_it == l2gtypes->end())
        return false; // cannot find global type
    uint32_t g_typeid = type_it->second;
    const RTTI::Type &field_type = rtti.GetTypes()[g_typeid];
    WriteMemReadInstruction(mem_ref, field_type, var.f_flags, use_var_offset);
    found_mem_ptr = static_cast<const uint8_t*>(mem_ptr);
    found_mem_sz = mem_sz;
    next_field_type = &field_type;
    return true;
}

// Parses a field name of a given type, writes memory read instruction into "mem_ref",
// assigns the found field's type into provided "field_type".
// Returns success if field was found, failure otherwise.
static bool ParseTypeField(const String &field_ref, const RTTI &rtti,
    const RTTI::Type &type, String &mem_ref, const RTTI::Type *&next_field_type)
{
    // TODO: is it possible to speed this up, making a field lookup,
    // or that would be too costly to do per type?
    const RTTI::Field *found_field = nullptr;
    for (const auto *field = type.first_field; field; field = field->next_field)
    {
        if (strcmp(field->name, field_ref.GetCStr()) == 0)
        {
            found_field = field;
            break;
        }
    }

    if (!found_field)
        return false; // couldn't resolve next symbol

    // "f_typeid" here is already resolved to global type id,
    // because we're using joint global RTTI, not local script's one
    const RTTI::Type &field_type = rtti.GetTypes()[found_field->f_typeid];
    mem_ref.AppendChar(':');
    WriteMemReadInstruction(mem_ref, field_type, found_field->flags, found_field->offset);
    next_field_type = &field_type;
    return true;
}

// TODO: support sub-expressions and parse this as VariableRefToMemoryRef?
static bool ParseArrayIndex(const String &field_ref, const RTTI::Type &arr_type, String &mem_ref)
{
    int arr_index = StrUtil::StringToInt(field_ref, -1);
    if (arr_index < 0)
        return false;
    mem_ref.AppendChar(':');
    WriteMemReadElemInstruction(mem_ref, arr_type, arr_index);
    return true;
}

inline bool IsKeywordChar(char c)
{
    return std::isalnum(c) || c == '_';
}

static String GetNextVarSection(const String &var_ref, size_t &index, char &access_type)
{
    if (index >= var_ref.GetLength())
        return {};

    for (; std::isspace(var_ref[index]); ++index); // skip any whitespace

    if (var_ref[index] == '.')
    {
        access_type = '.';
        for (++index; std::isspace(var_ref[index]); ++index); // skip any whitespace
    }
    else if (var_ref[index] == '[')
    {
        access_type = '[';
        for (++index; std::isspace(var_ref[index]); ++index); // skip any whitespace
    }
    else if (IsKeywordChar(var_ref[index]))
    {
        access_type = 0;
    }
    else
    {
        return {}; // end of string, or bad syntax
    }

    const size_t keyword_start = index;
    for (++index; IsKeywordChar(var_ref[index]); ++index); // scan for the keyword (variable/field name)
    const size_t keyword_end = index;

    if (access_type == '[')
    {
        for (; std::isspace(var_ref[index]); ++index); // skip any whitespace
        if (var_ref[index] != ']')
            return {}; // bad syntax, no closing bracket
        index++;
    }
    
    return var_ref.Mid(keyword_start, keyword_end - keyword_start);
}

// Parses the naming chain item by item, and build mem_ref string,
// containing set of instructions used to access and resolve actual memory
static bool VariableRefToMemoryRef(const String &var_ref, const ccInstance *inst,
    const uint8_t *&found_mem_ptr, size_t &found_mem_sz, String &mem_ref, String &last_type_name)
{
    String memory_ref;
    const auto &rtti = ccInstance::GetRTTI()->AsConstRTTI();
    const RTTI::Type *last_type = nullptr;
    const RTTI::Type *next_type = nullptr;

    String item;
    size_t index = 0;
    char access_type = 0;
    for (item = GetNextVarSection(var_ref, index, access_type);
        !item.IsEmpty(); item = GetNextVarSection(var_ref, index, access_type))
    {
        if (access_type == 0)
        {
            // Try getting a variable in the current script
            if (!ParseScriptVariable(item, inst, rtti, mem_ref,
                    found_mem_ptr, found_mem_sz, next_type))
                return false;
        }
        else if (access_type == '.')
        {
            if (!last_type)
                return false; // access member without type

            // Try getting a member of the last found type; save field's type into "next_type"
            if (!ParseTypeField(item, rtti, *last_type, mem_ref, next_type))
                return false;
        }
        else if (access_type == '[')
        {
            if (!last_type)
                return false; // access member without type

            // Try get the array index
            if (!ParseArrayIndex(item, *last_type, mem_ref))
                return false;
        }
        else
        {
            return false; // internal mistake? should not happen
        }

        last_type = next_type;
    }

    assert(last_type);
    if (!last_type)
        return false;

    // FIXME: this is a hack, force resolve managed "String" type
    // so that user receives a string value instead of a handle int32;
    // need to think how to deal with this situation in a generic way.
    if (((last_type->flags & RTTI::kType_Managed) != 0) &&
        (strcmp(last_type->name, "String") == 0))
    {
        mem_ref.Append(":0,s");
    }

    last_type_name = last_type->name;
    return true;
}

bool QueryScriptVariableInContext(const String &var_ref, String &type_str, String &value_str)
{
    if (var_ref.IsNullOrSpace())
        return false; // no name

    ccInstance *inst = ccInstance::GetCurrentInstance();
    if (!inst)
        return false; // not in running script
    
    const uint8_t *entry_mem_ptr = nullptr;
    size_t entry_mem_sz = 0u;
    String mem_ref;
    String last_type_name;
    if (!VariableRefToMemoryRef(var_ref, inst, entry_mem_ptr, entry_mem_sz, mem_ref, last_type_name))
        return false; // failed to parse variable name chain
    
    if (ResolveMemory(entry_mem_ptr, entry_mem_sz, mem_ref, 0u, value_str))
    {
        type_str = last_type_name;
        return true;
    }
    return false;
}

} // namespace MemoryInspect
} // namespace Engine
} // namespace AGS

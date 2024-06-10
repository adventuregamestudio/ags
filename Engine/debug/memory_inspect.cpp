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
#include "util/compress.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace AGS
{
namespace Engine
{
namespace MemoryInspect
{

// MemoryReference is a struct that defines a path to a data in memory,
// using a starting memory pointer and a list of instructions.
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
struct MemoryReference
{
    String Instruction;
    const uint8_t *MemPtr = nullptr;
    size_t MemSize = 0u;

    MemoryReference() = default;
    MemoryReference(const String &inst, const uint8_t *mem_ptr, size_t mem_sz)
        : Instruction(inst), MemPtr(mem_ptr), MemSize(mem_sz) {}
};

struct MemoryValue
{
    String Value;
    String TypeHint;

    MemoryValue() = default;
    MemoryValue(const String &value, const String &hint)
        : Value(value), TypeHint(hint) {}
};

// Resolves script memory using a MemoryReference instruction.
// Fills in memory value and a type hint (for interpreting value).
// Returns whether was successful.
// NOTE: this function is run recursively, until reaching end of instruction list.
static bool ResolveMemory(const MemoryReference &mem_ref, const size_t parse_at, MemoryValue &value)
{
    const String &mem_inst = mem_ref.Instruction;
    const uint8_t *mem_ptr = mem_ref.MemPtr;
    size_t mem_sz = mem_ref.MemSize;
    assert(mem_ptr);
    assert(!mem_inst.IsEmpty());
    if (!mem_ptr || mem_inst.IsEmpty())
        return false;

    int offset = atoi(&mem_inst[parse_at]);
    const size_t type_at = mem_inst.FindChar(',', parse_at);
    const size_t next_off_at = mem_inst.FindChar(':', parse_at);
    char type = 0;
    size_t size = 0u;
    
    if ((type_at != String::NoIndex) && (type_at < next_off_at))
    {
        type = mem_inst[type_at + 1];
        if (type_at + 2 < mem_inst.GetLength())
            size = atoi(&mem_inst[type_at + 2]);
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

        return ResolveMemory(MemoryReference(mem_inst, mem_ptr, mem_sz), next_off_at + 1, value);
    }

    // Resolve final entry
    switch (type)
    {
    case 'i': // integer (8, 16 or 32-bit)
        switch (size)
        {
        case 1:
        case 2:
        case 4:
            value = MemoryValue(base64_encode(mem_ptr, size), String::FromFormat("i%d", size));
            break;
        default: // unknown type, fail
            return false;
        }
        break;
    case 'f': // float (32-bit)
        switch (size)
        {
        case 4:
            value = MemoryValue(base64_encode(mem_ptr, size), "f4");
            break;
        default: // unknown type, fail
            return false;
        }
        break;
    case 'd': // plain data... not supported yet
        // TODO: convert requested size to base64?
        break;
    case 's': // string pointer, print as a string
        value = MemoryValue(reinterpret_cast<const char*>(mem_ptr), "s");
        break;
    case 'p': // value of a address, reserved
        break;
    case 'h': // value of a managed handle (int32)
    {
        value = MemoryValue(base64_encode(mem_ptr, 4), "h");
        break;
    }
    default: // unknown type, fail
        return false;
    }
    return true;
}

// Writes (appends) a memory read instruction into the provided string
static void WriteMemReadInstruction(String &mem_inst, const RTTI::Type &type, uint32_t f_flags, uint32_t f_offset)
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

    mem_inst.AppendFmt("%u,%c%u", f_offset, typec, typesz);
}

// Writes (appends) a memory read instruction for array element into the provided string
static void WriteMemReadElemInstruction(String &mem_inst, const RTTI::Type &type, uint32_t arr_index)
{
    // Array of pointers or array of PODs?
    if ((type.flags & RTTI::kType_Managed) != 0)
    {
        WriteMemReadInstruction(mem_inst, type, RTTI::kField_ManagedPtr, RTTI::PointerSize * arr_index);
    }
    else
    {
        WriteMemReadInstruction(mem_inst, type, 0u, type.size * arr_index);
    }
}

// MemoryVariable is a helper struct for grouping found variable def and memory address.
struct MemoryVariable
{
    const ScriptTOC::Variable *Variable = nullptr;
    const void *MemoryPtr = nullptr;
    size_t MemorySize = 0u;
    // NOTE: we cannot use found variable's offset directly, because some of them
    // are not stored in script memory, and also the script VM's stack has a special storage.
    size_t Offset = 0u;

    MemoryVariable() = default;
    MemoryVariable(const ScriptTOC::Variable *var, const void *mem_ptr, size_t mem_sz, size_t off)
        : Variable(var), MemoryPtr(mem_ptr), MemorySize(mem_sz), Offset(off) {}
};

static bool TryGetGlobalVariable(const String &field_ref, const ccInstance *inst,
    MemoryVariable &found_var)
{
    // Select the actual script at the top of the stack
    // (this could be a different script runnin on this instance, in case of far calls)
    const ccInstance *top_inst = inst->runningInst;
    const auto &toc = *top_inst->instanceof->sctoc;
    if (toc.GetGlobalVariables().empty())
        return false; // no global data

    auto var_it = top_inst->GetGlobalVariableLookup().find(field_ref);
    if (var_it == top_inst->GetGlobalVariableLookup().end())
        return false; // cannot find a global variable

    const auto &var = toc.GetGlobalVariables()[var_it->second];
    // If this is a script's own global variable, then simply reference its global memory
    if ((var.v_flags & ScriptTOC::kVariable_Import) == 0)
    {
        found_var = MemoryVariable(&var, top_inst->globaldata, top_inst->globaldatasize, var.offset);
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

        if (import->InstancePtr)
        {
            // Import from another script
            found_var = MemoryVariable(&var,
                import->InstancePtr->globaldata,
                import->InstancePtr->globaldatasize,
                static_cast<const uint8_t*>(import->Value.GetDirectPtr())
                    - reinterpret_cast<const uint8_t*>(import->InstancePtr->globaldata));
        }
        else
        {
            // Import from the engine or plugin
            // FIXME: we might have to return whole RuntimeScriptValue along,
            // and then use IScriptObject (if present) for reading type's fields
            // FIXME: get mem size from var type?
            found_var = MemoryVariable(&var, import->Value.GetDirectPtr(), 0u, 0u);
        }
        return true;
    }
}

static bool TryGetLocalVariable(const String &field_ref, const ccInstance *inst,
     MemoryVariable &found_var)
{
    // Select the actual script at the top of the stack
    // (this could be a different script runnin on this instance, in case of far calls)
    // NOTE: we will still use pc and stack of the current inst, since it's the one running!
    const ccScript *top_script = inst->runningInst->instanceof.get();
    const auto &toc = *top_script->sctoc;
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
            // FIXME: this is horrible...
            const void *mem_ptr = (stack_ptr->Type < kScValStackPtr) ?
                &stack_ptr->IValue :
                stack_ptr->GetDirectPtr();
            found_var = MemoryVariable(var, mem_ptr, stack_ptr->Size, 0u);
            return true;
        }

        last_var = var;
    }
    return false;
}

// TODO: pick out basic Field in RTTI (which does not have extra fields and quick links)?
struct FieldInfo
{
    const RTTI::Type *Type = nullptr;
    uint32_t FieldFlags = 0u; // RTTI::FieldFlags
    uint32_t NumElems = 0u;

    FieldInfo() = default;
    FieldInfo(const RTTI::Type *type, uint32_t f_flags, uint32_t num_elems)
        : Type(type), FieldFlags(f_flags), NumElems(num_elems) {}
};

// Try getting a registered variable from the current script's global memory,
// or local memory (stack); or, if this is an imported variable, then lookup
// the import table for its real address.
static HError ParseScriptVariable(const String &field_ref, const ccInstance *inst,
    const RTTI &rtti, MemoryReference &mem_ref, FieldInfo &next_field_info)
{
    MemoryVariable memvar;
    // First try local data
    if (!TryGetLocalVariable(field_ref, inst, memvar))
    {
        // Then try script's global variable;
        // this includes imported symbols from other scripts, plugins or engine
        if (!TryGetGlobalVariable(field_ref, inst, memvar))
        {
            return new Error(String::FromFormat("Variable not found in the current scope: '%s'", field_ref.GetCStr()));
        }
    }

    // Add found variable to the instruction list
    const auto &var = *memvar.Variable;
    // resolve local script's type to a global type index
    // TODO: this should be resolved after loading script, similar to RTTI!
    const ccInstance *top_inst = inst->runningInst;
    const auto *l2gtypes = &top_inst->GetLocal2GlobalTypeMap();
    auto type_it = l2gtypes->find(var.f_typeid);
    if (type_it == l2gtypes->end())
        return new Error(String::FromFormat("Type info not found for variable: '%s'", field_ref.GetCStr()));
    uint32_t g_typeid = type_it->second;
    const RTTI::Type &field_type = rtti.GetTypes()[g_typeid];
    WriteMemReadInstruction(mem_ref.Instruction, field_type, var.f_flags, memvar.Offset);
    mem_ref.MemPtr = static_cast<const uint8_t*>(memvar.MemoryPtr);
    mem_ref.MemSize = memvar.MemorySize;
    next_field_info = FieldInfo(&field_type, var.f_flags, var.num_elems);
    return HError::None();
}

// Parses a field name of a given type, writes memory read instruction into "mem_ref",
// assigns the found field's type into provided "field_type".
// Returns success if field was found, failure otherwise.
static HError ParseTypeField(const String &field_ref, const RTTI &rtti,
    const FieldInfo &field_info, MemoryReference &mem_ref, FieldInfo &next_field_info)
{
    // TODO: is it possible to speed this up, making a field lookup,
    // or that would be too costly to do per type?
    const RTTI::Type &type = *field_info.Type;
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
        return new Error(String::FromFormat("Field not found '%s'", field_ref.GetCStr())); // couldn't resolve next symbol

    // "f_typeid" here is already resolved to global type id,
    // because we're using joint global RTTI, not local script's one
    const RTTI::Type &field_type = rtti.GetTypes()[found_field->f_typeid];
    mem_ref.Instruction.AppendChar(':');
    WriteMemReadInstruction(mem_ref.Instruction, field_type, found_field->flags, found_field->offset);
    next_field_info = FieldInfo(&field_type, found_field->flags, found_field->num_elems);
    return HError::None();
}

// TODO: support sub-expressions and parse this as VariableRefToMemoryRef?
static HError ParseArrayIndex(const String &field_ref, const FieldInfo &field_info,
    MemoryReference &mem_ref, FieldInfo &next_field_info)
{
    int arr_index = StrUtil::StringToInt(field_ref, -1);
    if (arr_index < 0)
        return new Error(String::FromFormat("Invalid array index: '%s'", field_ref.GetCStr()));
    mem_ref.Instruction.AppendChar(':');
    WriteMemReadElemInstruction(mem_ref.Instruction, *field_info.Type, arr_index);
    // FIXME: this won't work for multidimensional arrays!
    // RTTI is missing necessary info for multidimensional arrays too.
    // FIXME: have a helper function that converts Type params to Field params of array element?
    uint32_t elem_f_flags =
        ((field_info.Type->flags & RTTI::kType_Managed) != 0) * RTTI::kField_ManagedPtr;
    next_field_info = FieldInfo(field_info.Type, elem_f_flags, 0u);
    return HError::None();
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
static HError VariableRefToMemoryRef(const String &var_ref, const ccInstance *inst,
    MemoryReference &mem_ref, FieldInfo &var_field_info)
{
    const auto &rtti = ccInstance::GetRTTI()->AsConstRTTI();
    FieldInfo last_field_info;
    FieldInfo next_field_info;

    String item;
    size_t index = 0;
    char access_type = 0;
    for (item = GetNextVarSection(var_ref, index, access_type);
        !item.IsEmpty(); item = GetNextVarSection(var_ref, index, access_type))
    {
        if (access_type == 0)
        {
            // Try getting a variable in the current script
            HError err = ParseScriptVariable(item, inst, rtti, mem_ref, next_field_info);
            if (!err)
                return err;
        }
        else if (access_type == '.')
        {
            if (!last_field_info.Type)
                return new Error("Parse error"); // access member without type

            // Try getting a member of the last found type; save field's type into "next_type"
            HError err = ParseTypeField(item, rtti, last_field_info, mem_ref, next_field_info);
            if (!err)
                return err;
        }
        else if (access_type == '[')
        {
            if (!last_field_info.Type)
                return new Error("Parse error"); // access member without type

            // Try get the array index
            HError err = ParseArrayIndex(item, last_field_info, mem_ref, next_field_info);
            if (!err)
                return err;
        }
        else
        {
            return new Error("Internal parse error"); // internal mistake? should not happen
        }

        last_field_info = next_field_info;
    }

    assert(last_field_info.Type);
    if (!last_field_info.Type)
        return new Error("Unknown field type: bad script or missing RTTI");

    // FIXME: this is a hack, force resolve managed "String" type
    // so that user receives a string value instead of a handle int32;
    // need to think how to deal with this situation in a generic way.
    if (((last_field_info.FieldFlags & RTTI::kField_Array) == 0) &&
        ((last_field_info.Type->flags & RTTI::kType_Managed) != 0) &&
        (strcmp(last_field_info.Type->name, "String") == 0))
    {
        mem_ref.Instruction.Append(":0,s");
    }

    var_field_info = last_field_info;
    return HError::None();
}

HError QueryScriptVariableInContext(const String &var_ref, VariableInfo &var_info)
{
    if (var_ref.IsNullOrSpace())
        return new Error("Bad input"); // no name

    ccInstance *inst = ccInstance::GetCurrentInstance();
    if (!inst)
        return new Error("No running script"); // not in running script
    
    MemoryReference mem_ref;
    FieldInfo var_field_info;
    HError err = VariableRefToMemoryRef(var_ref, inst, mem_ref, var_field_info);
    if (!err)
        return err; // failed to parse variable name chain
    
    MemoryValue mem_value;
    if (ResolveMemory(mem_ref, 0u, mem_value))
    {
        String type_name = var_field_info.Type->name;
        if (var_field_info.FieldFlags & RTTI::kField_Array)
        {
            if (var_field_info.NumElems > 0)
                type_name.AppendFmt("[%u]", var_field_info.NumElems);
            else
                type_name.Append("[]");
        }
        var_info = VariableInfo(mem_value.Value, type_name, mem_value.TypeHint);
        return HError::None();
    }
    return new Error("Failed to resolve script memory");
}

} // namespace MemoryInspect
} // namespace Engine
} // namespace AGS

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
// TODO: Some helper structs here may be merged or extend each other,
//       or extend RTTI / ScriptTOC structs (but latter would require
//       refactoring these structs first, see TODO comments to RTTI).
//
// TODO: Rewrite ParseScriptVariable part in VariableRefToMemoryRef.
//       Make a clear separation of writing instruction string, and
//       resolving memory: these operations should not be done simultaneously.
//       ParseScriptVariable should only find a variable definition,
//       and decide in which memory it is located.
//       Introduce instructions for retrieving global mem of particular script,
//       stack memory (with certain backwards offset), or an exported symbol.
//
// TODO: revise memory access instruction syntax.
//       there may be couple of things that it does not let do,
//       for instance: tell the size of an object resolved from a handle.
//
//       e.g. Instruction may contain global typeids of the accessed objects.
//
//=============================================================================
#include "debug/memory_inspect.h"
#include <algorithm>
#include "ac/dynobj/dynobj_manager.h"
#include "script/cc_common.h"
#include "script/cc_instance.h"
#include "script/runtimescript.h"
#include "script/script_runtime.h"
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
    const void *MemPtr = nullptr;
    IScriptObject *ObjMgr = nullptr;
    size_t MemSize = 0u;

    MemoryReference() = default;
    MemoryReference(const String &inst, const void *mem_ptr, IScriptObject *mgr, size_t mem_sz)
        : Instruction(inst), MemPtr(mem_ptr), ObjMgr(mgr), MemSize(mem_sz) {}
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
    const void *mem_ptr = mem_ref.MemPtr;
    IScriptObject *mem_mgr = mem_ref.ObjMgr;
    size_t mem_sz = mem_ref.MemSize;
    assert(mem_ptr);
    assert(!mem_inst.IsEmpty());
    if (!mem_ptr || mem_inst.IsEmpty())
        return false;

    const bool direct_obj = mem_inst[parse_at] == '>';
    const int offset = atoi(&mem_inst[parse_at]);
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

    // Read next value or mem ptr
    // Note that if a IScriptObject manager is present for this next value,
    // then we use its Read methods, otherwise we simply access mem_ptr + offset
    union
    {
        int8_t i1; int16_t i2; int32_t i4; float f4;
        const void *ptr;
    } next_value;

    const void *mem_ptr_off = static_cast<const uint8_t*>(mem_ptr) + offset;

    switch (type)
    {
    case 'i': // integer (8, 16 or 32-bit)
        switch (size)
        {
        case 1:
            next_value.i1 = mem_mgr ? mem_mgr->ReadInt8(mem_ptr, offset) : *static_cast<const int8_t*>(mem_ptr_off);
            break;
        case 2:
            next_value.i2 = mem_mgr ? mem_mgr->ReadInt16(mem_ptr, offset) : *static_cast<const int16_t*>(mem_ptr_off);
            break;
        case 4:
            next_value.i4 = mem_mgr ? mem_mgr->ReadInt32(mem_ptr, offset) : *static_cast<const int32_t*>(mem_ptr_off);
            break;
        default: // unknown type, fail
            return false;
        }
        break;
    case 'f': // float (32-bit)
        switch (size)
        {
        case 4:
            next_value.f4 = mem_mgr ? mem_mgr->ReadFloat(mem_ptr, offset) : *static_cast<const float*>(mem_ptr_off);
            break;
        default: // unknown type, fail
            return false;
        }
        break;
    case 'd': // plain data, advance ptr
    case 's': // string pointer
        if (direct_obj)
            next_value.ptr = mem_ptr;
        else
            next_value.ptr = mem_mgr ? mem_mgr->GetFieldPtr(mem_ptr, offset) : mem_ptr_off;
        break;
    case 'h': // managed handle, int32
        next_value.i4 = mem_mgr ? mem_mgr->ReadInt32(mem_ptr, offset) : *static_cast<const int32_t*>(mem_ptr_off);
        break;
    case 'p': // value of a address, reserved but not supported atm
        return false;
    default: // unknown type, fail
        return false;
    }

    // If there's a next entry in the instruction chain, then try to
    // resolve the next memory reference using current value, if possible
    if (next_off_at != String::NoIndex)
    {
        switch (type)
        {
        case 'd': // plain data, keep same mem ptr
        case 's':
            // If we have a direct object, keep everything for the next entry
            if (!direct_obj)
            {
                mem_ptr = static_cast<const uint8_t*>(next_value.ptr);
                mem_mgr = nullptr;
                mem_sz -= offset;
            }
            break;
        case 'h': // managed handle, resolve to managed object addr
        {
            void *addr;
            IScriptObject *mgr;
            if (ccGetObjectAddressAndManagerFromHandle(next_value.i4, addr, mgr) == kScValUndefined)
                return false; // object not found
            mem_ptr = reinterpret_cast<const uint8_t*>(addr);
            mem_mgr = mgr;
            mem_sz = 0u; // FIXME... how to get it? use typeinfo? but where to get typeinfo from?
            break;
        }
        default: // invalid type for sub-entries, fail
            return false;
        }

        return ResolveMemory(MemoryReference(mem_inst, mem_ptr, mem_mgr, mem_sz), next_off_at + 1, value);
    }

    // Resolve final entry, convert memory to base64
    switch (type)
    {
    case 'i': // integer (8, 16 or 32-bit)
        switch (size)
        {
        case 1:
            value = MemoryValue(base64_encode(&next_value.i1, size), "i1"); break;
        case 2:
            value = MemoryValue(base64_encode(&next_value.i2, size), "i2"); break;
        case 4:
            value = MemoryValue(base64_encode(&next_value.i4, size), "i4"); break;
        default: // unknown type, fail
            return false;
        }
        break;
    case 'f': // float (32-bit)
        switch (size)
        {
        case 4:
            value = MemoryValue(base64_encode(&next_value.f4, size), "f4"); break;
        default: // unknown type, fail
            return false;
        }
        break;
    case 'd': // plain data... not supported yet
        // TODO: convert requested size to base64?
        break;
    case 's': // string pointer, pass as a string (encoding matches game text format)
        value = MemoryValue(reinterpret_cast<const char*>(next_value.ptr), "s");
        break;
    case 'p': // value of a address, reserved
        break;
    case 'h': // value of a managed handle (int32)
        value = MemoryValue(base64_encode(&next_value.i4, size), "h");
        break;
    default: // unknown type, fail
        return false;
    }
    return true;
}

// Writes (appends) a memory read instruction into the provided string
static void WriteMemReadInstruction(String &mem_inst, const RTTI::Type &type, uint32_t f_flags, uint32_t f_elem_count, bool direct_obj, uint32_t f_offset)
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
        typec = 'd'; typesz = f_elem_count * type.size; // static array
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

    if (direct_obj)
        mem_inst.AppendFmt(">,%c%u", typec, typesz);
    else
        mem_inst.AppendFmt("%u,%c%u", f_offset, typec, typesz);
}

// Writes (appends) a memory read instruction for array element into the provided string
static void WriteMemReadElemInstruction(String &mem_inst, const RTTI::Type &type, uint32_t arr_index)
{
    // Array of pointers or array of PODs?
    if ((type.flags & RTTI::kType_Managed) != 0)
    {
        WriteMemReadInstruction(mem_inst, type, RTTI::kField_ManagedPtr, 0u, false, RTTI::PointerSize * arr_index);
    }
    else
    {
        WriteMemReadInstruction(mem_inst, type, 0u, 0u, false, type.size * arr_index);
    }
}

// MemoryVariable is a helper struct for grouping found variable def and memory address.
struct MemoryVariable
{
    const ScriptTOC::Variable *Variable = nullptr;
    const void *MemoryPtr = nullptr;
    IScriptObject *ObjMgr = nullptr;
    size_t MemorySize = 0u;

    MemoryVariable() = default;
    MemoryVariable(const ScriptTOC::Variable *var, const void *mem_ptr, IScriptObject *mgr, size_t mem_sz)
        : Variable(var), MemoryPtr(mem_ptr), ObjMgr(mgr), MemorySize(mem_sz) {}
};

static bool TryGetGlobalVariable(const String &field_ref, const ccInstance *inst,
    MemoryVariable &found_var)
{
    // Select the actual script at the top of the stack
    // (this could be a different script runnin on this instance, in case of far calls)
    const ccInstance *top_inst = inst->GetRunningInst();
    if (!top_inst->GetScript()->GetTOC())
        return false; // no TOC

    const auto &toc = *top_inst->GetScript()->GetTOC();
    if (toc.GetGlobalVariables().empty())
        return false; // no global data

    auto var_it = top_inst->GetGlobalVariableLookup().find(field_ref);
    if (var_it == top_inst->GetGlobalVariableLookup().end())
        return false; // cannot find a global variable

    const auto &var = toc.GetGlobalVariables()[var_it->second];
    // If this is a script's own global variable, then simply reference its global memory
    if ((var.v_flags & ScriptTOC::kVariable_Import) == 0)
    {
        found_var = MemoryVariable(&var, top_inst->GetGlobalData().data() + var.offset, nullptr, top_inst->GetGlobalData().size());
        return true;
    }
    // If it's an imported variable, then this may be memory from another script,
    // but also from the engine or plugin!
    else
    {
        // TODO: following may be simplified by recording variable ptr in runtime TOC
        const ScriptImport *import = simp.GetByName(var.name);
        if (!import)
            return false;

        if (import->ScriptPtr)
        {
            // Import from another script
            found_var = MemoryVariable(&var,
                import->Value.GetDirectPtr(), nullptr,
                import->ScriptPtr->GetGlobalData().size());
        }
        else
        {
            // Import from the engine or plugin
            // FIXME: get mem size from var type?
            // FIXME: this ptr retrieval is horrible...
            const void *mem_ptr = (import->Value.Type < kScValStackPtr) ?
                &import->Value.IValue : import->Value.GetDirectPtr();
            found_var = MemoryVariable(&var, import->Value.GetDirectPtr(), import->Value.ObjMgr, 0u);
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
    const RuntimeScript *top_script = inst->GetRunningInst()->GetScript();
    if (!top_script->GetTOC())
        return false; // no TOC

    const auto &toc = *top_script->GetTOC();
    if (toc.GetLocalVariables().empty())
        return false; // no local data

    // TODO: use runtime TOC where we can guarantee sorted function list (or fixup TOC on load)
    ScriptTOC::Function test_func; test_func.scope_begin = inst->GetPC(); test_func.scope_end = inst->GetPC() + 1;
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
        next_var && next_var->scope_begin <= static_cast<uint32_t>(inst->GetPC());
        var = next_var, next_var = next_var->next_local);

    // FIXME: helper method returning stack? don't direct access!
    // Note this stack ptr is set *after* the last allocated local data
    const auto *stack_ptr = inst->GetCurrentStack();
    const ScriptTOC::Variable *last_var = nullptr;
    // Scan local variable backwards before we find one that matches the name
    for (; var; var = var->prev_local)
    {
        assert(var->v_flags & ScriptTOC::kVariable_Local);
        // Skip if local variable's scope ends before current pos;
        // note we don't break, as this may be a nested scope inside a function
        if (var->scope_end <= static_cast<uint32_t>(inst->GetPC()))
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
            // FIXME: this ptr retrieval is horrible...
            const void *mem_ptr = (stack_ptr->Type < kScValStackPtr) ?
                &stack_ptr->IValue : stack_ptr->GetDirectPtr();
            found_var = MemoryVariable(var, mem_ptr, stack_ptr->ObjMgr, stack_ptr->Size);
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
    const ccInstance *top_inst = inst->GetRunningInst();
    const auto *l2gtypes = &top_inst->GetLocal2GlobalTypeMap();
    auto type_it = l2gtypes->find(var.f_typeid);
    if (type_it == l2gtypes->end())
        return new Error(String::FromFormat("Type info not found for variable: '%s'", field_ref.GetCStr()));
    uint32_t g_typeid = type_it->second;
    const RTTI::Type &field_type = rtti.GetTypes()[g_typeid];
    WriteMemReadInstruction(mem_ref.Instruction, field_type, var.f_flags, var.num_elems, true, 0u);
    mem_ref.MemPtr = memvar.MemoryPtr;
    mem_ref.ObjMgr = memvar.ObjMgr;
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
    if ((field_info.Type->flags & RTTI::kType_Struct) == 0)
        return new Error("Invalid struct member access");
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
    WriteMemReadInstruction(mem_ref.Instruction, field_type, found_field->flags, found_field->num_elems, false, found_field->offset);
    next_field_info = FieldInfo(&field_type, found_field->flags, found_field->num_elems);
    return HError::None();
}

// TODO: support sub-expressions and parse this as VariableRefToMemoryRef?
static HError ParseArrayIndex(const String &field_ref, const FieldInfo &field_info,
    MemoryReference &mem_ref, FieldInfo &next_field_info)
{
    if ((field_info.FieldFlags & RTTI::kField_Array) == 0)
        return new Error("Invalid array access");
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
    access_type = 0;
    if (index >= var_ref.GetLength())
    {
        return {};
    }

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
    else if (!IsKeywordChar(var_ref[index]))
    {
        return {}; // end of string, or bad syntax
    }

    const size_t keyword_start = index;
    for (; IsKeywordChar(var_ref[index]); ++index); // scan for the keyword (variable/field name)
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
    const auto &rtti = RuntimeScript::GetJointRTTI()->AsConstRTTI();
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

    if (access_type > 0)
        return new Error("Parse error"); // likely incomplete member/element access

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
    cc_clear_error(); // reset script error in case resolving memory went wrong
    return new Error("Failed to resolve script memory");
}

} // namespace MemoryInspect
} // namespace Engine
} // namespace AGS


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"       // macro definitions
#include "script/cc_error.h"            // error processing

AGS::SymbolTableEntry::SymbolTableEntry()
    : SName("")
    , SType(kSYM_NoType)
    , DeclSectionId(0)
    , DeclLine(0)
    , Flags(0)
    , TypeQualifiers(0)
    , SOffset(0)
    , Vartype(0)
    , SSize(0)
    , SScope(0)
    , VartypeType(kVTT_Atomic)
    , Dims({})
    , Extends(0)
    , FuncParamVartypes(std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , FuncParamDefaultValues(std::vector<ParamDefault>(1))
    , OperatorOpcode(0)
    , OperatorBinaryPrio(-1)
    , OperatorUnaryPrio(-1)
{ }


AGS::SymbolTableEntry::SymbolTableEntry(std::string const &name, SymbolType stype, size_t ssize)
    : SName(name)
    , SType(stype)
    , DeclSectionId(0)
    , DeclLine(0)
    , Flags(0)
    , TypeQualifiers(0)
    , SOffset(0)
    , Vartype(0)
    , SSize(ssize)
    , SScope(0)
    , VartypeType(kVTT_Atomic)
    , Dims({})
    , Extends(0)
    , FuncParamVartypes(std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , FuncParamDefaultValues(std::vector<ParamDefault>(1))
    , OperatorOpcode(0)
    , OperatorBinaryPrio(-1)
    , OperatorUnaryPrio(-1)
{ }

bool AGS::SymbolTableEntry::IsVTT(enum VartypeType vtt, SymbolTable const &symt) const
{
    if (kSYM_Vartype != SType)
        return symt.IsVTT(Vartype, vtt);

    // "Constant" always is the outermost vartype qualifier,
    // so if this is constant and we're checking for something else,
    // then look for the info one level down.
    if (kVTT_Const == VartypeType)
        return (kVTT_Const == vtt) ? true: symt.IsVTT(Vartype, vtt);
    return vtt == VartypeType;
}

size_t AGS::SymbolTableEntry::GetSize(SymbolTable const & symt) const
{
    return (kSYM_Vartype == SType) ? SSize : symt.GetSize(Vartype);
}

size_t AGS::SymbolTableEntry::NumArrayElements(SymbolTable const &symt) const
{
    if (kSYM_Vartype != SType)
        return symt.NumArrayElements(Vartype);

    if (0 == Dims.size())
        return 0;

    size_t num = 1;
    for (size_t dims_idx = 0; dims_idx < Dims.size(); ++dims_idx)
        num *= Dims[dims_idx];
    return num;
}

bool AGS::SymbolTableEntry::IsVTF(SymbolTableFlag flag, SymbolTable const &symt) const
{
    if (kSYM_Vartype != SType && kSYM_UndefinedStruct != SType)
        return symt.IsVTF(Vartype, flag);

    // Recursively get to the innermost symbol; read that symbol's flags
    if (kVTT_Atomic == VartypeType)
        return FlagIsSet(Flags, flag);
    return symt.IsVTF(Vartype, flag);
}

bool AGS::SymbolTableEntry::ParamDefault::operator==(const ParamDefault &other) const
{
    if (Type != other.Type)
        return false;
    switch (Type)
    {
    default:            return false;
    case kDT_None:      return true;
    case kDT_Int:       return IntDefault == other.IntDefault;
    case kDT_Float:     return FloatDefault == other.FloatDefault;
    case kDT_Dyn:       return DynDefault == other.DynDefault;
    }
}

std::string AGS::SymbolTableEntry::ParamDefault::ToString() const
{
    switch (Type)
    {
    default:            return "(Illegal default type)";
    case kDT_None:      return "(No default)";
    case kDT_Int:       return std::to_string(IntDefault);
    case kDT_Float:     return std::to_string(FloatDefault);
    case kDT_Dyn:       return (nullptr == DynDefault) ? "null" : "(A non-null value)";
    }
}

int32_t AGS::SymbolTableEntry::ParamDefault::ToInt32() const
{
    switch (Type)
    {
    default:            return 0;
    case kDT_Int:       return IntDefault;
    case kDT_Float:     return *((CodeCell *)&FloatDefault);
    }
}

AGS::SymbolTable::SymbolTable()
    : _stringStructSym(0)
{
    reset();
}

void AGS::SymbolTable::reset()
{
    _findCache.clear();
    _vartypesCache.clear();

    entries.clear();

    SetStringStructSym(0);

    Add("__dummy_sym_0__", kSYM_NoType);

    // Primitive types
    _charSym =
        Add("char", kSYM_Vartype, 1);
    _floatSym =
        Add("float", kSYM_Vartype, 4);
    _intSym =
        Add("int", kSYM_Vartype, SIZE_OF_INT);
    _longSym = 
        Add("long", kSYM_Vartype, SIZE_OF_INT);
    _shortSym = 
        Add("short", kSYM_Vartype, 2);
    _oldStringSym =
        Add("string", kSYM_Vartype, STRINGBUFFER_LENGTH);
    _voidSym =
        Add("void", kSYM_Vartype);

    // can be part of expression
    Add("]", kSYM_CloseBracket);
    Add(")", kSYM_CloseParenthesis);
    Add(".", kSYM_Dot);
    _nullSym =
        Add("null", kSYM_Null);
    Add("[", kSYM_OpenBracket);
    Add("(", kSYM_OpenParenthesis);

    AddOp("!", kSYM_Operator,  SCMD_NOTREG, -1, 101); // boolean NOT
    AddOp("~", kSYM_Operator,  SCMD_NOTREG, -1, 101); // bitwise NOT
    _dynpointerSym =
        AddOp("*", kSYM_Operator, SCMD_MULREG, 103);
    AddOp("/", kSYM_Operator, SCMD_DIVREG, 103);
    AddOp("%", kSYM_Operator, SCMD_MODREG, 103);
    AddOp("+", kSYM_Operator, SCMD_ADDREG, 105);
    AddOp("-", kSYM_Operator, SCMD_SUBREG, 105, 101);
    AddOp("<<", kSYM_Operator, SCMD_SHIFTLEFT, 107);
    AddOp(">>", kSYM_Operator, SCMD_SHIFTRIGHT, 107);
    AddOp("&", kSYM_Operator, SCMD_BITAND, 109);
    AddOp("|", kSYM_Operator, SCMD_BITOR, 110);
    AddOp("^", kSYM_Operator, SCMD_XORREG, 110);
    AddOp("==", kSYM_Operator, SCMD_ISEQUAL, 112);
    AddOp("!=", kSYM_Operator, SCMD_NOTEQUAL, 112);
    AddOp(">", kSYM_Operator, SCMD_GREATER, 112);
    AddOp("<", kSYM_Operator, SCMD_LESSTHAN, 112);
    AddOp(">=", kSYM_Operator, SCMD_GTE, 112);
    AddOp("<=", kSYM_Operator, SCMD_LTE, 112);
    AddOp("&&", kSYM_Operator, SCMD_AND, 118);
    AddOp("||", kSYM_Operator, SCMD_OR, 119);
    AddOp("?", kSYM_Tern, -1, 120);

    _thisSym =
        Add("this");

    // other keywords and symbols
    AddOp("=", kSYM_Assign, 0);
    AddOp("+=", kSYM_AssignMod, SCMD_ADDREG);
    AddOp("-=", kSYM_AssignMod, SCMD_SUBREG);
    AddOp("*=", kSYM_AssignMod, SCMD_MULREG);
    AddOp("/=", kSYM_AssignMod, SCMD_DIVREG);
    AddOp("&=", kSYM_AssignMod, SCMD_BITAND);
    AddOp("|=", kSYM_AssignMod, SCMD_BITOR);
    AddOp("^=", kSYM_AssignMod, SCMD_XORREG);
    AddOp("<<=", kSYM_AssignMod, SCMD_SHIFTLEFT);
    AddOp(">>=", kSYM_AssignMod, SCMD_SHIFTRIGHT);
    AddOp("++", kSYM_AssignSOp, SCMD_ADD);
    AddOp("--", kSYM_AssignSOp, SCMD_SUB);

    Add("attribute", kSYM_Attribute);
    Add("autoptr", kSYM_AutoPtr);
    Add("break", kSYM_Break);
    Add("builtin", kSYM_Builtin);
    Add("case", kSYM_Case);
    Add("}", kSYM_CloseBrace);
    Add(",", kSYM_Comma);
    Add("const", kSYM_Const);
    Add("continue", kSYM_Continue);
    Add("default", kSYM_Default);
    Add("do", kSYM_Do);
    Add("else", kSYM_Else);
    Add("enum", kSYM_Enum);
    Add("export", kSYM_Export);
    Add("extends", kSYM_Extends);
    Add("for", kSYM_For);
    Add("if", kSYM_If);
    Add("import", kSYM_Import);     // NOTE: Different keywords, same symbol
    Add("_tryimport", kSYM_Import); // NOTE: Different keywords, same symbol
    Add("internalstring", kSYM_InternalString);
    Add(":", kSYM_Label);
    Add("noloopcheck", kSYM_NoLoopCheck);
    Add("managed", kSYM_Managed);
    Add("::", kSYM_MemberAccess);
    AddOp("new", kSYM_New, -1, -1, 101); // note, can also be operator
    Add("{", kSYM_OpenBrace);
    Add("protected", kSYM_Protected);
    Add("readonly", kSYM_ReadOnly);
    Add("return", kSYM_Return);
    Add(";", kSYM_Semicolon);
    Add("static", kSYM_Static);
    Add("struct", kSYM_Struct);
    Add("switch", kSYM_Switch);
    Add("...", kSYM_Varargs);
    Add("while", kSYM_While);
    _lastPredefSym =
        Add("writeprotected", kSYM_WriteProtected); 
}

bool AGS::SymbolTable::IsAnyIntegerVartype(Symbol s) const
{
    if (s >= _charSym && s <= _shortSym && s != _floatSym)
        return true;
    if (!IsAtomic(s))
        return false;
    s = entries[s].Vartype;
    return (s >= _charSym && s <= _shortSym && s != _floatSym);
}

std::string const AGS::SymbolTable::GetName(AGS::Symbol symbl) const
{
    if (symbl < 0)
        return std::string("(end of input)");
    if (static_cast<size_t>(symbl) >= entries.size())
        return std::string("(invalid symbol)");
    return entries[symbl].SName;
}

void AGS::SymbolTable::SetDeclared(int idx, int section_id, int line)
{
    (*this)[idx].DeclSectionId = section_id;
    (*this)[idx].DeclLine = line;
}

AGS::Vartype AGS::SymbolTable::VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype)
{
    // Can't have classic arrays of classic arrays
    if (IsVTT(vartype, kVTT_Array))
        return vartype;

    std::string conv_name = entries[vartype].SName + "[";
    size_t const last_idx = dims.size() - 1;
    size_t num_elements = 1;
    for (size_t dims_idx = 0; dims_idx <= last_idx; ++dims_idx)
    {
        num_elements *= dims[dims_idx];
        conv_name += std::to_string(dims[dims_idx]);
        conv_name += (dims_idx == last_idx) ? "]" : ", ";
    }
    Vartype const array_vartype = FindOrAdd(conv_name);
    entries[array_vartype].SType = kSYM_Vartype;
    entries[array_vartype].VartypeType = kVTT_Array;
    entries[array_vartype].Vartype = vartype;
    entries[array_vartype].SSize = num_elements * GetSize(vartype);
    entries[array_vartype].Dims = dims;
    return array_vartype;
}

AGS::Vartype AGS::SymbolTable::VartypeWith(enum VartypeType vtt, Vartype vartype)
{
    // Return cached result if existent 
    std::pair<Vartype, enum VartypeType> const arg = { vartype, vtt };
    Vartype &valref(_vartypesCache[arg]);
    if (valref) 
        return valref;

    if (IsVTT(vartype, vtt))     
        return (valref = vartype); // Nothing to be done

    std::string pre = "";
    std::string post = "";
    switch (vtt)
    {
    default: pre = "QUAL" + std::to_string(vtt) + " "; break;
    case kVTT_Const: pre = "const "; break;
        // Note: Even if we have an autoptr and suppress the '*', we still need to add _something_ to the name.
        // The name for the pointered type must be different from the name of the unpointered type.
        // (If this turns out to be too ugly, then we need two fields for vartypes:
        // one field that is output to the user, another field that is guaranteed to have different values
        // for different vartypes.)
    case kVTT_Dynpointer: post = FlagIsSet(entries[vartype].Flags, kSFLG_StructAutoPtr)? " " : " *"; break;
    case kVTT_Dynarray: post = "[]"; break;
    }
    std::string const conv_name = (pre + entries[vartype].SName) + post;
    valref = FindOrAdd(conv_name);
    SymbolTableEntry &entry = entries[valref];
    entry.SType = kSYM_Vartype;
    entry.VartypeType = vtt;
    entry.Vartype = vartype;
    entry.SSize = (kVTT_Const == vtt) ? GetSize(vartype) : SIZE_OF_DYNPOINTER;
    return valref;
}

AGS::Vartype AGS::SymbolTable::VartypeWithout(long vtt, AGS::Vartype vartype) const
{
    while (
        IsInBounds(vartype) &&
        kSYM_Vartype == entries[vartype].SType &&
        FlagIsSet (entries[vartype].VartypeType, vtt))
        vartype = entries[vartype].Vartype;
    return vartype;
}

int AGS::SymbolTable::GetComponentsOfStruct(Symbol strct, std::vector<Symbol>& compo_list) const
{
    compo_list.clear();
    while(true)
    {
        for (size_t compo = 1; compo < entries.size(); compo++)
        {
            SymbolTableEntry const &entry = entries[compo];
            if (entry.Extends != strct || !FlagIsSet(entry.Flags, kSFLG_StructMember))
                continue;

            compo_list.push_back(compo);
        }
        if (entries[strct].Extends <= 0)
            return 0;
        strct = entries[strct].Extends;
    }
}

bool AGS::SymbolTable::IsAnyTypeOfString(Symbol s) const
{
    if (!IsInBounds(s))
        return false;

    // Convert a var to its vartype
    if (kSYM_LocalVar == entries[s].SType || kSYM_GlobalVar == entries[s].SType)
    {
        s = entries[s].Vartype;
        if (!IsInBounds(s))
            return false;
    }

    // Must be vartype at this point
    if (kSYM_Vartype != entries[s].SType)
        return false;

    // Oldstrings and String * are strings
    Vartype const s_without_const = VartypeWithout(kVTT_Const, s);

    return
        GetOldStringSym() == s_without_const ||
        GetStringStructSym() == VartypeWithout(kVTT_Dynpointer, s_without_const);
}

bool AGS::SymbolTable::IsOldstring(Symbol s) const
{
    if (!IsInBounds(s))
        return false;

    // Convert a var to its vartype
    if (kSYM_LocalVar == entries[s].SType || kSYM_GlobalVar == entries[s].SType)
    {
        s = entries[s].Vartype;
        if (!IsInBounds(s) || kSYM_Vartype != entries[s].SType)
            return false;
    }

    Vartype const s_without_const =
        VartypeWithout(kVTT_Const, s);
    // string and const string are oldstrings
    if (GetOldStringSym() == s_without_const)
        return true;
    
    // const char[..] and char[..] are considered oldstrings, too
    return (IsArray(s) && GetCharSym() == VartypeWithout(kVTT_Array, s_without_const));
}


AGS::Symbol AGS::SymbolTable::Add(std::string const &name, SymbolType stype, int ssize)
{
    if (0 != _findCache.count(name))
        return -1;

    SymbolTableEntry entry(name, stype, ssize);
    int const idx_of_new_entry = entries.size();
    entries.push_back(entry);
    _findCache[name] = idx_of_new_entry;
    return idx_of_new_entry;
}

AGS::Symbol AGS::SymbolTable::AddOp(std::string const &opname, SymbolType sty, CodeCell opcode, int binary_prio, int unary_prio)
{
    Symbol symbol_idx = Add(opname, sty);
    if (symbol_idx < 0)
        return symbol_idx;
    entries[symbol_idx].OperatorOpcode = opcode;
    entries[symbol_idx].OperatorBinaryPrio = binary_prio;
    entries[symbol_idx].OperatorUnaryPrio = unary_prio;
    return symbol_idx;
}

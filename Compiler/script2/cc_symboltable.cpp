
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"       // macro definitions
#include "script/cc_error.h"            // error processing

AGS::SymbolTableEntry::SymbolTableEntry()
    : SName("")
    , SType(SymT::kNoType)
    , Declared(kNoSrcLocation)
    , Flags(0)
    , TypeQualifiers(0)
    , SOffset(0)
    , Vartype(0)
    , SSize(0)
    , SScope(0u)
    , VartypeType(kVTT_Atomic)
    , Dims({})
    , Parent(0)
    , FuncParamVartypes(std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , FuncParamDefaultValues(std::vector<ParamDefault>(1))
    , OperatorOpcode(0)
    , OperatorBinaryPrio(-1)
    , OperatorUnaryPrio(-1)
{ }


AGS::SymbolTableEntry::SymbolTableEntry(std::string const &name, SymbolType stype, size_t ssize)
    : SName(name)
    , SType(stype)
    , Declared(kNoSrcLocation)
    , Flags(0)
    , TypeQualifiers(0)
    , SOffset(0)
    , Vartype(0)
    , SSize(ssize)
    , SScope(0u)
    , VartypeType(kVTT_Atomic)
    , Dims({})
    , Parent(0)
    , FuncParamVartypes(std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , FuncParamDefaultValues(std::vector<ParamDefault>(1))
    , OperatorOpcode(0)
    , OperatorBinaryPrio(-1)
    , OperatorUnaryPrio(-1)
{ }

bool AGS::SymbolTableEntry::IsVTT(enum VartypeType vtt, SymbolTable const &symt) const
{
    if (SymT::kVartype != SType)
        return symt.IsVTT(Vartype, vtt);

    // "Constant" always is the outermost vartype qualifier,
    // so if this is constant and we're checking for something else,
    // then look for the info one level down.
    if (kVTT_Const == VartypeType)
        return (kVTT_Const == vtt) ? true : symt.IsVTT(Vartype, vtt);
    return vtt == VartypeType;
}

size_t AGS::SymbolTableEntry::GetSize(SymbolTable const & symt) const
{
    return (SymT::kVartype == SType) ? SSize : symt.GetSize(Vartype);
}

size_t AGS::SymbolTableEntry::NumArrayElements(SymbolTable const &symt) const
{
    if (SymT::kVartype != SType)
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
    if (SymT::kVartype != SType && SymT::kUndefinedStruct != SType)
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
    entries.reserve(300);
    entries.resize(kKW_LastPredefined + 1);

    SetStringStructSym(0);

    Add(kKW_NoSymbol, "__dummy_sym_0__", SymT::kNoType);

    // Primitive types
    Add(kKW_Char, "char", SymT::kVartype, 1);
    Add(kKW_Float, "float", SymT::kVartype, 4);
    Add(kKW_Int, "int", SymT::kVartype, SIZE_OF_INT);
    Add(kKW_Long, "long", SymT::kVartype, SIZE_OF_INT);
    Add(kKW_Short, "short", SymT::kVartype, 2);
    Add(kKW_String, "string", SymT::kVartype, STRINGBUFFER_LENGTH);
    Add(kKW_Void, "void", SymT::kVartype, 0);

    Add(kKW_CloseBracket, "]", SymT::kDelimiter);
    Add(kKW_CloseParenthesis, ")", SymT::kDelimiter);
    Add(kKW_Dot, ".", SymT::kKeyword);
    Add(kKW_Null, "null", SymT::kKeyword);
    Add(kKW_OpenBracket, "[", SymT::kDelimiter);
    Add(kKW_OpenParenthesis, "(", SymT::kDelimiter);

    AddOp(kKW_Not, "!", SymT::kOperator, SCMD_NOTREG, -1, 101); // boolean NOT
    AddOp(kKW_BitNeg, "~", SymT::kOperator, SCMD_NOTREG, -1, 101); // bitwise NOT
    AddOp(kKW_Multiply, "*", SymT::kOperator, SCMD_MULREG, 103);
    AddOp(kKW_Divide, "/", SymT::kOperator, SCMD_DIVREG, 103);
    AddOp(kKW_Modulo, "%", SymT::kOperator, SCMD_MODREG, 103);
    AddOp(kKW_Plus, "+", SymT::kOperator, SCMD_ADDREG, 105);
    AddOp(kKW_Minus, "-", SymT::kOperator, SCMD_SUBREG, 105, 101);
    AddOp(kKW_ShiftLeft, "<<", SymT::kOperator, SCMD_SHIFTLEFT, 107);
    AddOp(kKW_ShiftRight, ">>", SymT::kOperator, SCMD_SHIFTRIGHT, 107);
    AddOp(kKW_BitAnd, "&", SymT::kOperator, SCMD_BITAND, 109);
    AddOp(kKW_BitOr, "|", SymT::kOperator, SCMD_BITOR, 110);
    AddOp(kKW_BitXor, "^", SymT::kOperator, SCMD_XORREG, 110);
    AddOp(kKW_Equal, "==", SymT::kOperator, SCMD_ISEQUAL, 112);
    AddOp(kKW_NotEqual, "!=", SymT::kOperator, SCMD_NOTEQUAL, 112);
    AddOp(kKW_Greater, ">", SymT::kOperator, SCMD_GREATER, 112);
    AddOp(kKW_Less, "<", SymT::kOperator, SCMD_LESSTHAN, 112);
    AddOp(kKW_GreaterEqual, ">=", SymT::kOperator, SCMD_GTE, 112);
    AddOp(kKW_LessEqual, "<=", SymT::kOperator, SCMD_LTE, 112);
    AddOp(kKW_And, "&&", SymT::kOperator, SCMD_AND, 118);
    AddOp(kKW_Or, "||", SymT::kOperator, SCMD_OR, 119);
    AddOp(kKW_Tern, "?", SymT::kKeyword, -1, 120);

    Add(kKW_This, "this", SymT::kKeyword);

    // Assignments and modifiers
    AddOp(kKW_Assign, "=", SymT::kAssign, 0);
    AddOp(kKW_AssignPlus, "+=", SymT::kAssignMod, SCMD_ADDREG);
    AddOp(kKW_AssignMinus, "-=", SymT::kAssignMod, SCMD_SUBREG);
    AddOp(kKW_AssignMultiply, "*=", SymT::kAssignMod, SCMD_MULREG);
    AddOp(kKW_AssignDivide, "/=", SymT::kAssignMod, SCMD_DIVREG);
    AddOp(kKW_AssignBitAnd, "&=", SymT::kAssignMod, SCMD_BITAND);
    AddOp(kKW_AssignBitOr, "|=", SymT::kAssignMod, SCMD_BITOR);
    AddOp(kKW_AssignBitXor, "^=", SymT::kAssignMod, SCMD_XORREG);
    AddOp(kKW_AssignShiftLeft, "<<=", SymT::kAssignMod, SCMD_SHIFTLEFT);
    AddOp(kKW_AssignShiftRight, ">>=", SymT::kAssignMod, SCMD_SHIFTRIGHT);
    AddOp(kKW_Increment, "++", SymT::kAssignSOp, SCMD_ADD);
    AddOp(kKW_Decrement, "--", SymT::kAssignSOp, SCMD_SUB);

    // other keywords and symbols
    Add(kKW_Attribute, "attribute", SymT::kKeyword);
    Add(kKW_Autoptr, "autoptr", SymT::kKeyword);
    Add(kKW_Break, "break", SymT::kKeyword);
    Add(kKW_Builtin, "builtin", SymT::kKeyword);
    Add(kKW_Case, "case", SymT::kKeyword);
    Add(kKW_CloseBrace, "}", SymT::kKeyword);
    Add(kKW_Comma, ",", SymT::kKeyword);
    Add(kKW_Const, "const", SymT::kKeyword);
    Add(kKW_Continue, "continue", SymT::kKeyword);
    Add(kKW_Default, "default", SymT::kKeyword);
    Add(kKW_Do, "do", SymT::kKeyword);
    Add(kKW_Else, "else", SymT::kKeyword);
    Add(kKW_Enum, "enum", SymT::kKeyword);
    Add(kKW_Export, "export", SymT::kKeyword);
    Add(kKW_Extends, "extends", SymT::kKeyword);
    Add(kKW_For, "for", SymT::kKeyword);
    Add(kKW_If, "if", SymT::kKeyword);
    Add(kKW_Import, "import", SymT::kImport);     // NOTE: Different keywords, same symbol
    Add(kKW_ImportTry, "_tryimport", SymT::kImport); // NOTE: Different keywords, same symbol
    Add(kKW_Internalstring, "internalstring", SymT::kKeyword);
    Add(kKW_Colon, ":", SymT::kKeyword);
    Add(kKW_Noloopcheck, "noloopcheck", SymT::kKeyword);
    Add(kKW_Managed, "managed", SymT::kKeyword);
    Add(kKW_ScopeRes, "::", SymT::kKeyword);
    AddOp(kKW_New, "new", SymT::kKeyword, -1, -1, 101); // note, can also be operator
    Add(kKW_OpenBrace, "{", SymT::kKeyword);
    Add(kKW_Protected, "protected", SymT::kKeyword);
    Add(kKW_Readonly, "readonly", SymT::kKeyword);
    Add(kKW_Return, "return", SymT::kKeyword);
    Add(kKW_Semicolon, ";", SymT::kKeyword);
    Add(kKW_Static, "static", SymT::kKeyword);
    Add(kKW_Struct, "struct", SymT::kKeyword);
    Add(kKW_Switch, "switch", SymT::kKeyword);
    Add(kKW_Varargs, "...", SymT::kKeyword);
    Add(kKW_While, "while", SymT::kKeyword);
    Add(kKW_Writeprotected, "writeprotected", SymT::kKeyword);
}

bool AGS::SymbolTable::IsAnyIntegerVartype(Symbol s) const
{
    if (s >= kKW_Char && s <= kKW_Short && s != kKW_Float)
        return true;
    if (!IsAtomic(s))
        return false;
    s = entries[s].Vartype;
    return (s >= kKW_Char && s <= kKW_Short && s != kKW_Float);
}

std::string const AGS::SymbolTable::GetName(AGS::Symbol symbl) const
{
    if (symbl < 0)
        return std::string("(end of input)");
    if (static_cast<size_t>(symbl) >= entries.size())
        return std::string("(invalid symbol)");
    return entries[symbl].SName;
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
    entries[array_vartype].SType = SymT::kVartype;
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
    case kVTT_Dynpointer: post = FlagIsSet(entries[vartype].Flags, kSFLG_StructAutoPtr) ? " " : " *"; break;
    case kVTT_Dynarray: post = "[]"; break;
    }
    std::string const conv_name = (pre + entries[vartype].SName) + post;
    valref = FindOrAdd(conv_name);
    SymbolTableEntry &entry = entries[valref];
    entry.SType = SymT::kVartype;
    entry.VartypeType = vtt;
    entry.Vartype = vartype;
    entry.SSize = (kVTT_Const == vtt) ? GetSize(vartype) : SIZE_OF_DYNPOINTER;
    return valref;
}

AGS::Vartype AGS::SymbolTable::VartypeWithout(long vtt, AGS::Vartype vartype) const
{
    while (
        IsInBounds(vartype) &&
        SymT::kVartype == entries[vartype].SType &&
        FlagIsSet(entries[vartype].VartypeType, vtt))
        vartype = entries[vartype].Vartype;
    return vartype;
}

int AGS::SymbolTable::GetComponentsOfStruct(Symbol strct, std::vector<Symbol>& compo_list) const
{
    compo_list.clear();
    while (true)
    {
        std::vector<Symbol> const &children = entries.at(strct).Children;
        for (auto children_it = children.begin(); children_it != children.end(); children_it++)
            compo_list.push_back(*children_it);
        if (entries[strct].Parent <= 0)
            return 0;
        strct = entries.at(strct).Parent;
    }
}

bool AGS::SymbolTable::IsAnyStringVartype(Symbol s) const
{
    if (!IsInBounds(s))
        return false;

    // Convert a var to its vartype
    if (SymT::kLocalVar == entries[s].SType || SymT::kGlobalVar == entries[s].SType)
    {
        s = entries[s].Vartype;
        if (!IsInBounds(s))
            return false;
    }

    // Must be vartype at this point
    if (SymT::kVartype != entries[s].SType)
        return false;

    // Oldstrings and String * are strings
    Vartype const s_without_const = VartypeWithout(kVTT_Const, s);

    return
        kKW_String == s_without_const ||
        GetStringStructSym() == VartypeWithout(kVTT_Dynpointer, s_without_const);
}

bool AGS::SymbolTable::IsOldstring(Symbol s) const
{
    if (!IsInBounds(s))
        return false;

    // Convert a var to its vartype
    if (SymT::kLocalVar == entries[s].SType || SymT::kGlobalVar == entries[s].SType)
    {
        s = entries[s].Vartype;
        if (!IsInBounds(s) || SymT::kVartype != entries[s].SType)
            return false;
    }

    Vartype const s_without_const =
        VartypeWithout(kVTT_Const, s);
    // string and const string are oldstrings
    if (kKW_String == s_without_const)
        return true;

    // const char[..] and char[..] are considered oldstrings, too
    return (IsArrayVartype(s) && kKW_Char == VartypeWithout(kVTT_Array, s_without_const));
}

AGS::Symbol AGS::SymbolTable::Add(std::string const &name, SymbolType stype, int ssize)
{
    // Note: A very significant portion of computing is spent in this function.
    if (0 != _findCache.count(name))
        return -1;

    if (entries.size() == entries.capacity())
    {
        size_t const new_size1 = entries.capacity() * 2;
        size_t const new_size2 = entries.capacity() + 1000;
        entries.reserve((new_size1 < new_size2) ? new_size1 : new_size2);
    }
    int const idx_of_new_entry = entries.size();
    entries.emplace_back(name, stype, ssize);
    _findCache[name] = idx_of_new_entry;
    return idx_of_new_entry;
}

AGS::Symbol AGS::SymbolTable::Add(Predefined kw, std::string const &name, SymbolType stype, int ssize)
{
    if (0 != _findCache.count(name))
        return -1;

    SymbolTableEntry &entry = entries[kw];
    entry.SName = name;
    entry.SType = stype;
    entry.SSize = ssize;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddOp(Predefined kw, std::string const &opname, SymbolType sty, CodeCell opcode, int binary_prio, int unary_prio)
{
    Symbol symbol_idx = Add(kw, opname, sty);
    if (symbol_idx >= 0)
    {
        entries[symbol_idx].OperatorOpcode = opcode;
        entries[symbol_idx].OperatorBinaryPrio = binary_prio;
        entries[symbol_idx].OperatorUnaryPrio = unary_prio;
    }
    return symbol_idx;
}

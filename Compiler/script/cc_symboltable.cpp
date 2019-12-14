
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"       // macro definitions
#include "script/cc_error.h"            // error processing

int AGS::SymbolTable::SectionMap::Section2Id(std::string const &sec)
{
    if (sec == _cacheSec)
        return _cacheId;
    _cacheSec = sec;
    size_t const section_size = _section.size();
    for (size_t section_idx = 0; section_idx < section_size; section_idx++)
        if (_section[section_idx] == sec)
            return ((_cacheId = section_idx));
    _section.push_back(sec);
    return ((_cacheId = section_size));
}

std::string const AGS::SymbolTable::SectionMap::Id2Section(int id) const
{
    return
        (id >= 0 && static_cast<size_t>(id) < _section.size()) ?
        _section[id] : "";
}

void AGS::SymbolTable::SectionMap::init()
{
    _cacheSec = "";
    _cacheId = -1;
    _section.clear();
}

AGS::SymbolTableEntry::SymbolTableEntry()
    : SName("")
    , SType(kSYM_NoType)
    , DeclSectionId(0)
    , DeclLine(0)
    , Flags(0)
    , SOffset(0)
    , vartype(0)
    , SSize(0)
    , SScope(0)
    , VartypeType(kVTT_Atomic)
    , Dims({})
    , Extends(0)
    , FuncParamTypes (std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , FuncParamDefaultValues(std::vector<int>(1))
    , FuncParamHasDefaultValues(std::vector<bool>(1))
{ }

AGS::SymbolTableEntry::SymbolTableEntry(const char *name, SymbolType stype, size_t sizee)
    : SName(std::string(name))
    , SType(stype)
    , DeclSectionId(0)
    , DeclLine(0)
    , Flags(0)
    , SOffset(0)
    , vartype(0)
    , SSize(sizee)
    , SScope(0)
    , VartypeType(kVTT_Atomic)
    , Dims({})
    , Extends(0)
    , FuncParamTypes(std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , FuncParamDefaultValues(std::vector<int>(1))
    , FuncParamHasDefaultValues(std::vector<bool>(1))
{ }

bool AGS::SymbolTableEntry::IsVTT(enum VartypeType vtt, SymbolTable const &symt) const
{
    if (kSYM_Vartype != SType)
        return symt.IsVTT(vartype, vtt);

    // "Constant" always is the outermost vartype qualifier,
    // so if this is constant and we're checking for something else,
    // then look for the info one level down.
    if (kVTT_Const == VartypeType)
        return (kVTT_Const == vtt) ? true: symt.IsVTT(vartype, vtt);
    return vtt == VartypeType;
}

size_t AGS::SymbolTableEntry::GetSize(SymbolTable const & symt) const
{
    return (kSYM_Vartype == SType) ? SSize : symt.GetSize(vartype);
}

size_t AGS::SymbolTableEntry::NumArrayElements(SymbolTable const &symt) const
{
    if (kSYM_Vartype != SType)
        return symt.NumArrayElements(vartype);

    if (0 == Dims.size())
        return 0;

    size_t num = 1;
    for (size_t dims_idx = 0; dims_idx < Dims.size(); ++dims_idx)
        num *= Dims[dims_idx];
    return num;
}

bool AGS::SymbolTableEntry::IsVTF(AGS::Flags f, SymbolTable const &symt) const
{
    if (kSYM_Vartype != SType && kSYM_UndefinedStruct != SType)
        return symt.IsVTF(vartype, f);

    // Recursively get to the innermost symbol; read that symbol's flags
    if (kVTT_Atomic == VartypeType)
        return FlagIsSet(Flags, f);
    return symt.IsVTF(vartype, f);
}

AGS::SymbolTable::SymbolTable()
    : _charSym(0)
    , _floatSym(0)
    , _intSym(0)
    , _nullSym(0)
    , _dynpointerSym(0)
    , _oldStringSym(0)
    , _stringStructSym(0)
    , _thisSym(0)
    , _voidSym(0)
    , _lastPredefSym(0)
{
    reset();
}

void AGS::SymbolTable::reset()
{
    _findCache.clear();
    _vartypesCache.clear();

    entries.clear();

    SetStringStructSym(0);

    AddWithTypeAndSize("___dummy__sym0", static_cast<SymbolType>(999), 0);

    // Primitive types
    _charSym =
        AddWithTypeAndSize("char", kSYM_Vartype, 1);
    _floatSym =
        AddWithTypeAndSize("float", kSYM_Vartype, 4);
    _intSym =
        AddWithTypeAndSize("int", kSYM_Vartype, SIZE_OF_INT);
    AddWithTypeAndSize("long", kSYM_Vartype, SIZE_OF_INT);
    AddWithTypeAndSize("short", kSYM_Vartype, 2);
    _oldStringSym =
        AddWithTypeAndSize("string", kSYM_Vartype, STRINGBUFFER_LENGTH);
    _voidSym =
        AddWithTypeAndSize("void", kSYM_Vartype, 0);

    // can be part of expression
    AddWithTypeAndSize("]", kSYM_CloseBracket, 0);
    AddWithTypeAndSize(")", kSYM_CloseParenthesis, 0);
    AddWithTypeAndSize(".", kSYM_Dot, 0);
    _nullSym = AddWithTypeAndSize("null", kSYM_Null, 0);
    AddWithTypeAndSize("[", kSYM_OpenBracket, 0);
    AddWithTypeAndSize("(", kSYM_OpenParenthesis, 0);
    AddWithTypeAndSize("?", kSYM_Tern, 20); // note, set operator prio
    // the second argument to the operators is their precedence: 1 is highest
    AddOp("!", 1, SCMD_NOTREG);
    _dynpointerSym =
        AddOp("*", 3, SCMD_MULREG);
    AddOp("/", 3, SCMD_DIVREG);
    AddOp("%", 3, SCMD_MODREG);
    AddOp("+", 5, SCMD_ADDREG);
    AddOp("-", 5, SCMD_SUBREG);
    AddOp("<<", 7, SCMD_SHIFTLEFT);
    AddOp(">>", 7, SCMD_SHIFTRIGHT);
    AddOp("&", 9, SCMD_BITAND);
    AddOp("|", 10, SCMD_BITOR);
    AddOp("^", 10, SCMD_XORREG);
    AddOp("==", 12, SCMD_ISEQUAL);
    AddOp("!=", 12, SCMD_NOTEQUAL);
    AddOp(">", 12, SCMD_GREATER);
    AddOp("<", 12, SCMD_LESSTHAN);
    AddOp(">=", 12, SCMD_GTE);
    AddOp("<=", 12, SCMD_LTE);
    AddOp("&&", 18, SCMD_AND);
    AddOp("||", 19, SCMD_OR);
    _thisSym =
        Add("this");

    // other keywords and symbols
    AddWithTypeAndSize("=", kSYM_Assign, 0);
    AddWithTypeAndSize("+=", kSYM_AssignMod, SCMD_ADDREG);
    AddWithTypeAndSize("-=", kSYM_AssignMod, SCMD_SUBREG);
    AddWithTypeAndSize("*=", kSYM_AssignMod, SCMD_MULREG);
    AddWithTypeAndSize("/=", kSYM_AssignMod, SCMD_DIVREG);
    AddWithTypeAndSize("&=", kSYM_AssignMod, SCMD_BITAND);
    AddWithTypeAndSize("|=", kSYM_AssignMod, SCMD_BITOR);
    AddWithTypeAndSize("^=", kSYM_AssignMod, SCMD_XORREG);
    AddWithTypeAndSize("<<=", kSYM_AssignMod, SCMD_SHIFTLEFT);
    AddWithTypeAndSize(">>=", kSYM_AssignMod, SCMD_SHIFTRIGHT);
    AddWithTypeAndSize("++", kSYM_AssignSOp, SCMD_ADD);
    AddWithTypeAndSize("--", kSYM_AssignSOp, SCMD_SUB);
    AddWithTypeAndSize("attribute", kSYM_Attribute, 0);
    AddWithTypeAndSize("autoptr", kSYM_AutoPtr, 0);
    AddWithTypeAndSize("break", kSYM_Break, 0);
    AddWithTypeAndSize("builtin", kSYM_Builtin, 0);
    AddWithTypeAndSize("case", kSYM_Case, 0);
    AddWithTypeAndSize("}", kSYM_CloseBrace, 0);
    AddWithTypeAndSize(",", kSYM_Comma, 0);
    AddWithTypeAndSize("const", kSYM_Const, 0);
    AddWithTypeAndSize("continue", kSYM_Continue, 0);
    AddWithTypeAndSize("default", kSYM_Default, 0);
    AddWithTypeAndSize("do", kSYM_Do, 0);
    AddWithTypeAndSize("else", kSYM_Else, 0);
    AddWithTypeAndSize("enum", kSYM_Enum, 0);
    AddWithTypeAndSize("export", kSYM_Export, 0);
    AddWithTypeAndSize("extends", kSYM_Extends, 0);
    AddWithTypeAndSize("for", kSYM_For, 0);
    AddWithTypeAndSize("if", kSYM_If, 0);
    AddWithTypeAndSize("import", kSYM_Import, 0);
    AddWithTypeAndSize("_tryimport", kSYM_Import, 0);
    AddWithTypeAndSize("internalstring", kSYM_InternalString, 0);
    AddWithTypeAndSize(":", kSYM_Label, 0);
    AddWithTypeAndSize("noloopcheck", kSYM_NoLoopCheck, 0);
    AddWithTypeAndSize("managed", kSYM_Managed, 0);
    AddWithTypeAndSize("::", kSYM_MemberAccess, 0);
    AddWithTypeAndSize("new", kSYM_New, 1); // note, set operator priority
    AddWithTypeAndSize("{", kSYM_OpenBrace, 0);
    AddWithTypeAndSize("protected", kSYM_Protected, 0);
    AddWithTypeAndSize("readonly", kSYM_ReadOnly, 0);
    AddWithTypeAndSize("return", kSYM_Return, 0);
    AddWithTypeAndSize(";", kSYM_Semicolon, 0);
    AddWithTypeAndSize("static", kSYM_Static, 0);
    AddWithTypeAndSize("struct", kSYM_Struct, 0);
    AddWithTypeAndSize("switch", kSYM_Switch, 0);
    AddWithTypeAndSize("...", kSYM_Varargs, 0);
    AddWithTypeAndSize("while", kSYM_While, 0);
    _lastPredefSym =
        AddWithTypeAndSize("writeprotected", kSYM_WriteProtected, 0); 
}

std::string const AGS::SymbolTable::GetName(AGS::Symbol symbl) const
{
    if (symbl < 0)
        return std::string("(end of input)");
    if (static_cast<size_t>(symbl) >= entries.size())
        return std::string("(invalid symbol)");
    return entries[symbl].SName;
}

void AGS::SymbolTable::SetDeclared(int idx, std::string const &section, int line)
{
    (*this)[idx].DeclSectionId = _sectionMap.Section2Id(section);
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
    Vartype const array_vartype = FindOrAdd(conv_name.c_str());
    entries[array_vartype].SType = kSYM_Vartype;
    entries[array_vartype].VartypeType = kVTT_Array;
    entries[array_vartype].vartype = vartype;
    entries[array_vartype].SSize = num_elements * GetSize(vartype);
    entries[array_vartype].Dims = dims;
    return array_vartype;
}

AGS::Vartype AGS::SymbolTable::VartypeWith(enum VartypeType vtt, AGS::Vartype vartype)
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
    case kVTT_Dynpointer: post = " *"; break;
    case kVTT_Dynarray: post = "[]"; break;
    }
    std::string const conv_name = (pre + entries[vartype].SName) + post;
    valref = FindOrAdd(conv_name.c_str());
    SymbolTableEntry &entry = entries[valref];
    entry.SType = kSYM_Vartype;
    entry.VartypeType = vtt;
    entry.vartype = vartype;
    entry.SSize = (kVTT_Const == vtt) ? GetSize(vartype) : SIZE_OF_DYNPOINTER;
    return valref;
}

AGS::Vartype AGS::SymbolTable::VartypeWithout(long vtt, AGS::Vartype vartype) const
{
    while (
        IsInBounds(vartype) &&
        kSYM_Vartype == entries[vartype].SType &&
        FlagIsSet (entries[vartype].VartypeType, vtt))
        vartype = entries[vartype].vartype;
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
        s = entries[s].vartype;
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
        s = entries[s].vartype;
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


AGS::Symbol AGS::SymbolTable::AddWithTypeAndSize(char const *name, SymbolType stype, int ssize)
{
    if (0 != _findCache.count(name))
        return -1;

    SymbolTableEntry entry(name, stype, ssize);
    int const idx_of_new_entry = entries.size();
    entries.push_back(entry);
    _findCache[name] = idx_of_new_entry;
    return idx_of_new_entry;
}

int AGS::SymbolTable::AddOp(const char *opname, int priority, int vcpucmd)
{
    AGS::Symbol symbol_idx = AddWithTypeAndSize(opname, kSYM_Operator, priority);
    if (symbol_idx >= 0)
        entries.at(symbol_idx).vartype = vcpucmd;
    return symbol_idx;
}

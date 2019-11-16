
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
    : sname("")
    , stype(kSYM_NoType)
    , decl_secid(0)
    , decl_line(0)
    , flags(0)
    , soffs(0)
    , vartype(0)
    , ssize(0)
    , sscope(0)
    , vartype_type(kVTT_Atomic)
    , dims({})
    , extends(0)
    , funcparamtypes (std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , funcParamDefaultValues(std::vector<int>(1))
    , funcParamHasDefaultValues(std::vector<bool>(1))
{ }

AGS::SymbolTableEntry::SymbolTableEntry(const char *name, SymbolType stype, size_t sizee)
    : sname(std::string(name))
    , stype(stype)
    , decl_secid(0)
    , decl_line(0)
    , flags(0)
    , soffs(0)
    , vartype(0)
    , ssize(sizee)
    , sscope(0)
    , vartype_type(kVTT_Atomic)
    , dims({})
    , extends(0)
    , funcparamtypes(std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , funcParamDefaultValues(std::vector<int>(1))
    , funcParamHasDefaultValues(std::vector<bool>(1))
{ }

bool AGS::SymbolTableEntry::IsVTT(VartypeType vtt, SymbolTable const &symt) const
{
    if (kSYM_Vartype != stype)
        return symt.IsVTT(vartype, vtt);

    // "Constant" always is the outermost vartype qualifier,
    // so if this is constant and we're checking for something else,
    // then look for the info one level down.
    if (kVTT_Const == vartype_type)
        return (kVTT_Const == vtt) ? true: symt.IsVTT(vartype, vtt);
    return vtt == vartype_type;
}

size_t AGS::SymbolTableEntry::GetSize(SymbolTable const & symt) const
{
    return (kSYM_Vartype == stype) ? ssize : symt.GetSize(vartype);
}

size_t AGS::SymbolTableEntry::NumArrayElements(SymbolTable const &symt) const
{
    if (kSYM_Vartype != stype)
        return symt.NumArrayElements(vartype);

    if (0 == dims.size())
        return 0;

    size_t num = 1;
    for (size_t dims_idx = 0; dims_idx < dims.size(); ++dims_idx)
        num *= dims[dims_idx];
    return num;
}

bool AGS::SymbolTableEntry::IsVTF(Flags f, SymbolTable const &symt) const
{
    if (kSYM_Vartype != stype && kSYM_UndefinedStruct != stype)
        return symt.IsVTF(vartype, f);

    // Recursively get to the innermost symbol; read that symbol's flags
    if (kVTT_Atomic == vartype_type)
        return FlagIsSet(flags, f);
    return symt.IsVTF(vartype, f);
}

AGS::SymbolTable::SymbolTable()
    : _charSym(0)
    , _floatSym(0)
    , _intSym(0)
    , _nullSym(0)
    , _pointerSym(0)
    , _stringSym(0)
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

    setStringStructSym(0);

    add_ex("___dummy__sym0", static_cast<SymbolType>(999), 0);

    // Primitive types
    _charSym =
        add_ex("char", kSYM_Vartype, 1);
    _floatSym =
        add_ex("float", kSYM_Vartype, 4);
    _intSym =
        add_ex("int", kSYM_Vartype, SIZE_OF_INT);
    add_ex("long", kSYM_Vartype, SIZE_OF_INT);
    add_ex("short", kSYM_Vartype, 2);
    _stringSym =
        add_ex("string", kSYM_Vartype, STRINGBUFFER_LENGTH);
    _voidSym =
        add_ex("void", kSYM_Vartype, 0);

    // can be part of expression
    add_ex("]", kSYM_CloseBracket, 0);
    add_ex(")", kSYM_CloseParenthesis, 0);
    add_ex(".", kSYM_Dot, 0);
    _nullSym = add_ex("null", kSYM_Null, 0);
    add_ex("[", kSYM_OpenBracket, 0);
    add_ex("(", kSYM_OpenParenthesis, 0);
    // the second argument to the operators is their precedence: 1 is highest
    add_operator("!", 1, SCMD_NOTREG);
    _pointerSym =
        add_operator("*", 2, SCMD_MULREG);
    add_operator("/", 3, SCMD_DIVREG);
    add_operator("%", 4, SCMD_MODREG);
    add_operator("+", 5, SCMD_ADDREG);
    add_operator("-", 5, SCMD_SUBREG);
    add_operator("<<", 7, SCMD_SHIFTLEFT);
    add_operator(">>", 8, SCMD_SHIFTRIGHT);
    add_operator("&", 9, SCMD_BITAND);
    add_operator("|", 10, SCMD_BITOR);
    add_operator("^", 11, SCMD_XORREG);
    add_operator("==", 12, SCMD_ISEQUAL);
    add_operator("!=", 13, SCMD_NOTEQUAL);
    add_operator(">", 14, SCMD_GREATER);
    add_operator("<", 15, SCMD_LESSTHAN);
    add_operator(">=", 16, SCMD_GTE);
    add_operator("<=", 17, SCMD_LTE);
    add_operator("&&", 18, SCMD_AND);
    add_operator("||", 19, SCMD_OR);
    _thisSym =
        add("this");

    // other keywords and symbols
    add_ex("=", kSYM_Assign, 0);
    add_ex("+=", kSYM_AssignMod, SCMD_ADDREG);
    add_ex("-=", kSYM_AssignMod, SCMD_SUBREG);
    add_ex("*=", kSYM_AssignMod, SCMD_MULREG);
    add_ex("/=", kSYM_AssignMod, SCMD_DIVREG);
    add_ex("&=", kSYM_AssignMod, SCMD_BITAND);
    add_ex("|=", kSYM_AssignMod, SCMD_BITOR);
    add_ex("^=", kSYM_AssignMod, SCMD_XORREG);
    add_ex("<<=", kSYM_AssignMod, SCMD_SHIFTLEFT);
    add_ex(">>=", kSYM_AssignMod, SCMD_SHIFTRIGHT);
    add_ex("++", kSYM_AssignSOp, SCMD_ADD);
    add_ex("--", kSYM_AssignSOp, SCMD_SUB);
    add_ex("attribute", kSYM_Attribute, 0);
    add_ex("autoptr", kSYM_AutoPtr, 0);
    add_ex("break", kSYM_Break, 0);
    add_ex("builtin", kSYM_Builtin, 0);
    add_ex("case", kSYM_Case, 0);
    add_ex("}", kSYM_CloseBrace, 0);
    add_ex(",", kSYM_Comma, 0);
    add_ex("const", kSYM_Const, 0);
    add_ex("continue", kSYM_Continue, 0);
    add_ex("default", kSYM_Default, 0);
    add_ex("do", kSYM_Do, 0);
    add_ex("else", kSYM_Else, 0);
    add_ex("enum", kSYM_Enum, 0);
    add_ex("export", kSYM_Export, 0);
    add_ex("extends", kSYM_Extends, 0);
    add_ex("for", kSYM_For, 0);
    add_ex("if", kSYM_If, 0);
    add_ex("import", kSYM_Import, 0);
    add_ex("_tryimport", kSYM_Import, 0);
    add_ex("internalstring", kSYM_InternalString, 0);
    add_ex(":", kSYM_Label, 0);
    add_ex("noloopcheck", kSYM_NoLoopCheck, 0);
    add_ex("managed", kSYM_Managed, 0);
    add_ex("::", kSYM_MemberAccess, 0);
    add_ex("new", kSYM_New, 1);
    add_ex("{", kSYM_OpenBrace, 0);
    add_ex("protected", kSYM_Protected, 0);
    add_ex("readonly", kSYM_ReadOnly, 0);
    add_ex("return", kSYM_Return, 0);
    add_ex(";", kSYM_Semicolon, 0);
    add_ex("static", kSYM_Static, 0);
    add_ex("struct", kSYM_Struct, 0);
    add_ex("switch", kSYM_Switch, 0);
    add_ex("...", kSYM_Varargs, 0);
    add_ex("while", kSYM_While, 0);
    _lastPredefSym =
        add_ex("writeprotected", kSYM_WriteProtected, 0); 
}

std::string const AGS::SymbolTable::get_name_string(AGS::Symbol symbl) const
{
    if (symbl < 0)
        return std::string("(end of input)");
    if (static_cast<size_t>(symbl) >= entries.size())
        return std::string("(invalid symbol)");
    return entries.at(symbl).sname;
}

std::string const AGS::SymbolTable::get_vartype_name_string(AGS::Vartype vartype) const
{
    if (!IsInBounds(vartype))
        return "(invalid vartype)";
    return entries.at(vartype).sname;
    }

void AGS::SymbolTable::set_declared(int idx, std::string const &section, int line)
{
    (*this)[idx].decl_secid = _sectionMap.Section2Id(section);
    (*this)[idx].decl_line = line;
}

AGS::Vartype AGS::SymbolTable::VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype)
{
    // Can't have classic arrays of classic arrays
    if (IsVTT(vartype, kVTT_Array))
        return vartype;

    std::string conv_name = entries[vartype].sname + "[";
    size_t const last_idx = dims.size() - 1;
    size_t num_elements = 1;
    for (size_t dims_idx = 0; dims_idx <= last_idx; ++dims_idx)
    {
        num_elements *= dims[dims_idx];
        conv_name += std::to_string(dims[dims_idx]);
        conv_name += (dims_idx == last_idx) ? "]" : ", ";
    }
    Vartype const array_vartype = find_or_add(conv_name.c_str());
    entries[array_vartype].stype = kSYM_Vartype;
    entries[array_vartype].vartype_type = kVTT_Array;
    entries[array_vartype].vartype = vartype;
    entries[array_vartype].ssize = num_elements * GetSize(vartype);
    entries[array_vartype].dims = dims;
    return array_vartype;
}

AGS::Vartype AGS::SymbolTable::VartypeWith(VartypeType vtt, AGS::Vartype vartype)
{
    // Return cached result if existent 
    std::pair<Vartype, VartypeType> const arg = { vartype, vtt };
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
    std::string const conv_name = (pre + entries[vartype].sname) + post;
    valref = find_or_add(conv_name.c_str());
    SymbolTableEntry &entry = entries[valref];
    entry.stype = kSYM_Vartype;
    entry.vartype_type = vtt;
    entry.vartype = vartype;
    entry.ssize = (kVTT_Const == vtt) ? GetSize(vartype) : SIZE_OF_DYNPOINTER;
    return valref;
}

AGS::Vartype AGS::SymbolTable::VartypeWithout(long vtt, AGS::Vartype vartype) const
{
    while (
        IsInBounds(vartype) &&
        kSYM_Vartype == entries[vartype].stype &&
        FlagIsSet (entries[vartype].vartype_type, vtt))
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
            if (entry.extends != strct || !FlagIsSet(entry.flags, kSFLG_StructMember))
                continue;

            compo_list.push_back(compo);
        }
        if (entries[strct].extends <= 0)
            return 0;
        strct = entries[strct].extends;
    }
}

bool AGS::SymbolTable::IsAnyTypeOfString(Symbol s) const
{
    if (!IsInBounds(s))
        return false;

    // Convert a var to its vartype
    if (kSYM_LocalVar == entries[s].stype || kSYM_GlobalVar == entries[s].stype)
    {
        s = entries[s].vartype;
        if (!IsInBounds(s))
            return false;
    }

    // Must be vartype at this point
    if (kSYM_Vartype != entries[s].stype)
        return false;

    // Oldstrings and String * are strings
    Vartype const s_without_const = VartypeWithout(kVTT_Const, s);

    return
        getOldStringSym() == s_without_const ||
        getStringStructSym() == VartypeWithout(kVTT_Dynpointer, s_without_const);
}

bool AGS::SymbolTable::IsOldstring(Symbol s) const
{
    if (!IsInBounds(s))
        return false;

    // Convert a var to its vartype
    if (kSYM_LocalVar == entries[s].stype || kSYM_GlobalVar == entries[s].stype)
    {
        s = entries[s].vartype;
        if (!IsInBounds(s) || kSYM_Vartype != entries[s].stype)
            return false;
    }

    Vartype const s_without_const =
        VartypeWithout(kVTT_Const, s);
    // string and const string are oldstrings
    if (getOldStringSym() == s_without_const)
        return true;
    
    // const char[..] and char[..] are considered oldstrings, too
    return (IsArray(s) && getCharSym() == VartypeWithout(kVTT_Array, s_without_const));
}


AGS::Symbol AGS::SymbolTable::add_ex(char const *name, SymbolType stype, int ssize)
{
    if (0 != _findCache.count(name))
        return -1;

    SymbolTableEntry entry(name, stype, ssize);
    int const idx_of_new_entry = entries.size();
    entries.push_back(entry);
    _findCache[name] = idx_of_new_entry;
    return idx_of_new_entry;
}

int AGS::SymbolTable::add_operator(const char *opname, int priority, int vcpucmd)
{
    AGS::Symbol symbol_idx = add_ex(opname, kSYM_Operator, priority);
    if (symbol_idx >= 0)
        entries.at(symbol_idx).vartype = vcpucmd;
    return symbol_idx;
}

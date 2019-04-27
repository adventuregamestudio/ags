
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"      // macro definitions

SymbolTableEntry::SymbolTableEntry()
    : sname("")
    , stype(kSYM_NoType)
    , flags(0)
    , vartype(0)
    , soffs(0)
    , ssize(0)
    , sscope(0)
    , arrsize(0)
    , extends(0)
    , funcparamtypes (std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , funcParamDefaultValues(std::vector<int>(1))
    , funcParamHasDefaultValues(std::vector<bool>(1))
{ }

SymbolTableEntry::SymbolTableEntry(const char *name, SymbolType stype, char sizee)
    : sname(std::string(name))
    , stype(stype)
    , flags(0)
    , vartype(0)
    , soffs(0)
    , ssize(sizee)
    , sscope(0)
    , arrsize(0)
    , extends(0)
    , funcparamtypes(std::vector<AGS::Vartype>(1)) // Function must have at least the return param
    , funcParamDefaultValues(std::vector<int>(1))
    , funcParamHasDefaultValues(std::vector<bool>(1))
{ }

int SymbolTableEntry::operatorToVCPUCmd()
{
    //return ssize + 8;
    return vartype;
}

int SymbolTableEntry::CopyTo(SymbolTableEntry &dest)
{
    dest.sname = this->sname;
    dest.stype = this->stype;
    dest.flags = this->flags;
    dest.vartype = this->vartype;
    dest.soffs = this->soffs;
    dest.ssize = this->ssize;
    dest.sscope = this->sscope;
    dest.arrsize = this->arrsize;
    dest.extends = this->extends;
    dest.funcparamtypes.assign(this->funcparamtypes.begin(), this->funcparamtypes.end());
    dest.funcParamDefaultValues.assign(this->funcParamDefaultValues.begin(), this->funcParamDefaultValues.end());
    dest.funcParamHasDefaultValues.assign(this->funcParamHasDefaultValues.begin(), this->funcParamHasDefaultValues.end());
    return 0;
}

SymbolTable::SymbolTable()
    : _charSym(0)
    , _floatSym(0)
    , _intSym(0)
    , _nullSym(0)
    , _pointerSym(0)
    , _stringSym(0)
    , _voidSym(0)
    , _stringStructSym(0)
    , _lastPredefSym(0)
{
    _findCache.clear();
}

SymbolType SymbolTable::get_type(AGS::Symbol symbol)  const
{
    symbol &= kVTY_FlagMask;

    if ((symbol < 0) || (symbol >= entries.size()))
        return kSYM_NoType;

    return entries[symbol].stype;
}

void SymbolTable::reset()
{
    _findCache.clear();

    entries.clear();

    setStringStructSym(0);

    add_ex("___dummy__sym0", static_cast<SymbolType>(999), 0);

    // Primitive types
    _charSym =
        add_ex("char", kSYM_Vartype, 1);
    _floatSym =
        add_ex("float", kSYM_Vartype, 4);
    _intSym =
        add_ex("int", kSYM_Vartype, 4);
    add_ex("long", kSYM_Vartype, 4);
    add_ex("short", kSYM_Vartype, 2);
    _stringSym =
        add_ex("string", kSYM_Vartype, 4);
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

std::string const SymbolTable::get_name_string(AGS::Symbol idx) const
{
    int const short_idx = (idx & kVTY_FlagMask);
    if (short_idx < 0)
        return std::string("(end of input)");
    if (short_idx >= entries.size())
        return std::string("(invalid symbol)");
    
    std::string result = entries[short_idx].sname;

    if (idx & kVTY_Pointer)
        result += "*";
    if (idx & (kVTY_Array|kVTY_DynArray))
        result += "[]";
    if (idx & kVTY_Const)
        result = "const " + result;

    return result;
}

std::string const SymbolTable::get_vartype_name_string(AGS::Vartype vartype) const
{
    AGS::Symbol const core_type = (vartype & kVTY_FlagMask);

    std::string result = (core_type >= 0 && core_type < entries.size()) ? entries[core_type].sname : "UNKNOWNTYPE";
    if ((vartype & kVTY_Pointer) && !(entries[core_type].flags & kSFLG_Autoptr))
        result += "*";
    if (vartype & (kVTY_Array|kVTY_DynArray))
        result += "[]";
    if (vartype & kVTY_Const)
        result = "const " + result;

    return result;
}

AGS::Symbol SymbolTable::add_ex(char const *name, SymbolType stype, int ssize)
{
    if (0 != _findCache.count(name))
        return -1;

    SymbolTableEntry entry(name, stype, ssize);
    int const idx_of_new_entry = entries.size();
    entries.push_back(entry);
    _findCache[name] = idx_of_new_entry;
    return idx_of_new_entry;
}

int SymbolTable::add_operator(const char *name, int priority, int vcpucmd)
{
    AGS::Symbol symbol_idx = add_ex(name, kSYM_Operator, priority);
    if (symbol_idx >= 0)
        entries[symbol_idx].vartype = vcpucmd;
    return symbol_idx;
}

SymbolTable sym;

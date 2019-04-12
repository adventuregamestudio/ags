
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"      // macro definitions

SymbolTableEntry::SymbolTableEntry()
    : sname("")
    , stype(0)
    , flags(0)
    , vartype(0)
    , soffs(0)
    , ssize(0)
    , sscope(0)
    , arrsize(0)
    , extends(0)
    , funcparamtypes (std::vector<unsigned long>(1)) // Function must have at least the return param
    , funcParamDefaultValues(std::vector<int>(1))
    , funcParamHasDefaultValues(std::vector<bool>(1))
{ }

SymbolTableEntry::SymbolTableEntry(const char *name, int typo, char sizee)
    : sname(std::string(name))
    , stype(typo)
    , flags(0)
    , vartype(0)
    , soffs(0)
    , ssize(sizee)
    , sscope(0)
    , arrsize(0)
    , extends(0)
    , funcparamtypes(std::vector<unsigned long>(1)) // Function must have at least the return param
    , funcParamDefaultValues(std::vector<int>(1))
    , funcParamHasDefaultValues(std::vector<bool>(1))
{ }

bool SymbolTableEntry::is_loadable_variable()
{
    return (stype == SYM_GLOBALVAR) || (stype == SYM_LOCALVAR) || (stype == SYM_CONSTANT);
}

void SymbolTableEntry::set_attrfuncs(int attrget, int attrset)
{
    // TODO check ranges and throw exception
    soffs = (attrget << 16) | attrset;
}

int SymbolTableEntry::get_attrget()
{
    int toret = (soffs >> 16) & 0x00ffff;
    if (toret == 0xffff) return -1;

    return toret;
}

int SymbolTableEntry::get_attrset()
{
    int toret = soffs & 0x00ffff;
    if (toret == 0xffff) return -1;

    return toret;
}

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
    : normalCharSym(0)
    , normalFloatSym(0)
    , normalIntSym(0)
    , normalNullSym(0)
    , normalPointerSym(0)
    , normalStringSym(0)
    , normalVoidSym(0)
    , stringStructSym(0)
    , lastPredefSym(0)
{
    _findCache.clear();
}

// Ignore the pointerness/constness of the symbol and get its type
AGS::Symbol SymbolTable::get_type(AGS::Symbol symbol)
{
    symbol &= STYPE_MASK;

    if ((symbol < 0) || (symbol >= entries.size()))
        return -1;

    return entries[symbol].stype;
}

void SymbolTable::reset()
{
    _findCache.clear();

    entries.clear();

    stringStructSym = 0;

    add_ex("___dummy__sym0", 999, 0);
    normalIntSym = add_ex("int", SYM_VARTYPE, 4);
    normalCharSym = add_ex("char", SYM_VARTYPE, 1);
    add_ex("long", SYM_VARTYPE, 4);
    add_ex("short", SYM_VARTYPE, 2);
    normalStringSym = add_ex("string", SYM_VARTYPE, 4);
    normalVoidSym = add_ex("void", SYM_VARTYPE, 0);
    normalFloatSym = add_ex("float", SYM_VARTYPE, 4);
    add_ex("=", SYM_ASSIGN, 0);
    add_ex(";", SYM_SEMICOLON, 0);
    add_ex(",", SYM_COMMA, 0);
    add_ex("(", SYM_OPENPARENTHESIS, 0);
    add_ex(")", SYM_CLOSEPARENTHESIS, 0);
    add_ex("{", SYM_OPENBRACE, 0);
    add_ex("}", SYM_CLOSEBRACE, 0);
    add_ex("+=", SYM_MASSIGN, SCMD_ADDREG);
    add_ex("-=", SYM_MASSIGN, SCMD_SUBREG);
    add_ex("*=", SYM_MASSIGN, SCMD_MULREG);
    add_ex("/=", SYM_MASSIGN, SCMD_DIVREG);
    add_ex("&=", SYM_MASSIGN, SCMD_BITAND);
    add_ex("|=", SYM_MASSIGN, SCMD_BITOR);
    add_ex("^=", SYM_MASSIGN, SCMD_XORREG);
    add_ex("<<=", SYM_MASSIGN, SCMD_SHIFTLEFT);
    add_ex(">>=", SYM_MASSIGN, SCMD_SHIFTRIGHT);
    add_ex("++", SYM_SASSIGN, SCMD_ADD);
    add_ex("--", SYM_SASSIGN, SCMD_SUB);
    // the second argument to the operators is their precedence: 1 is highest
    add_operator("!", 1, SCMD_NOTREG);
    normalPointerSym = add_operator("*", 2, SCMD_MULREG);
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
    add_ex("new", SYM_NEW, 1);
    add_ex("[", SYM_OPENBRACKET, 0);
    add_ex("]", SYM_CLOSEBRACKET, 0);
    add_ex(".", SYM_DOT, 0);
    add_ex("if", SYM_IF, 0);
    add_ex("else", SYM_ELSE, 0);
    add_ex("while", SYM_WHILE, 0);
    add_ex("for", SYM_FOR, 0);
    add_ex("break", SYM_BREAK, 0);
    add_ex("continue", SYM_CONTINUE, 0);
    add_ex("do", SYM_DO, 0);
    add_ex("switch", SYM_SWITCH, 0);
    add_ex("case", SYM_CASE, 0);
    add_ex("default", SYM_DEFAULT, 0);
    add_ex("...", SYM_VARARGS, 0);
    add_ex("struct", SYM_STRUCT, 0);
    add_ex("import", SYM_IMPORT, 0);
    add_ex("_tryimport", SYM_IMPORT, 0);
    add_ex("export", SYM_EXPORT, 0);
    add_ex("return", SYM_RETURN, 0);
    add_ex("readonly", SYM_READONLY, 0);
    add_ex("::", SYM_MEMBERACCESS, 0);
    add_ex(":", SYM_LABEL, 0);
    add_ex("attribute", SYM_ATTRIBUTE, 0);
    add_ex("enum", SYM_ENUM, 0);
    add_ex("managed", SYM_MANAGED, 0);
    normalNullSym = add_ex("null", SYM_NULL, 0);
    add_ex("extends", SYM_EXTENDS, 0);
    add_ex("static", SYM_STATIC, 0);
    add_ex("protected", SYM_PROTECTED, 0);
    add_ex("writeprotected", SYM_WRITEPROTECTED, 0);
    add_ex("const", SYM_CONST, 0);
    add_ex("internalstring", SYM_STRINGSTRUCT, 0);
    add_ex("autoptr", SYM_AUTOPTR, 0);
    add_ex("noloopcheck", SYM_LOOPCHECKOFF, 0);
    lastPredefSym = add_ex("builtin", SYM_BUILTIN, 0);
}

AGS::Symbol SymbolTable::find(const char *ntf)
{
    auto it = _findCache.find(ntf);
    if (it != _findCache.end())
        return it->second;
    return -1;
}

AGS::Symbol SymbolTable::find_or_add(const char *ntf)
{
    AGS::Symbol ret = find(ntf);
    if (ret >= 0)
        return ret;
    return add(ntf);
}


std::string const SymbolTable::get_name_string(int idx)
{
    int const short_idx = (idx & STYPE_MASK);
    if (short_idx < 0)
        return std::string("(end of input)");
    if (short_idx >= entries.size())
        return std::string("(invalid symbol)");
    if (short_idx == idx)
        return entries[short_idx].sname;

    std::string result = entries[short_idx].sname;

    if (idx & STYPE_POINTER)
        result += "*";
    if (idx & STYPE_DYNARRAY)
        result += "[]";
    if (idx & STYPE_CONST)
        result = "const " + result;

    return result;
}

char const *SymbolTable::get_name(int idx)
{
    static std::string str;
    str = get_name_string(idx);
    return str.c_str();
}

AGS::Symbol SymbolTable::add_ex(char const *name, AGS::Symbol stype, int ssize)
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
    AGS::Symbol symbol_idx = add_ex(name, SYM_OPERATOR, priority);
    if (symbol_idx >= 0)
        entries[symbol_idx].vartype = vcpucmd;
    return symbol_idx;
}

SymbolTable sym;

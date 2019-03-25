
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"      // macro definitions

SymbolTable::SymbolTable()
{
    stringStructSym = 0;
}

int SymbolTableEntry::get_num_args()
{
    return sscope % 100;
}

// Get the type of a symbol table entry, regardless of pointerness or constness
ags::Symbol SymbolTable::get_type(int ii) 
{
    // consider the real type, regardless of pointerness/constness
    ii &= ~(STYPE_POINTER | STYPE_CONST | STYPE_DYNARRAY);

    if ((ii < 0) || (ii >= entries.size()))
        return -1;

    return entries[ii].stype;
}

int SymbolTableEntry::is_loadable_variable()
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

void SymbolTable::reset() {
	for (std::map<int, char*>::iterator it = nameGenCache.begin(); it != nameGenCache.end(); ++it)
		free(it->second);

	nameGenCache.clear();

    entries.clear();

    stringStructSym = 0;
    symbolTree.clear();

    add_ex("___dummy__sym0", 999, 0);
    normalIntSym = add_ex("int", SYM_VARTYPE, 4);
    add_ex("char", SYM_VARTYPE, 1);
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
    nullSym = add_ex("null", SYM_NULL, 0);
    add_ex("extends", SYM_EXTENDS, 0);
    add_ex("static", SYM_STATIC, 0);
    add_ex("protected", SYM_PROTECTED, 0);
    add_ex("writeprotected", SYM_WRITEPROTECTED, 0);
    add_ex("const", SYM_CONST, 0);
    add_ex("internalstring", SYM_STRINGSTRUCT, 0);
    add_ex("autoptr", SYM_AUTOPTR, 0);
    add_ex("noloopcheck", SYM_LOOPCHECKOFF, 0);
    add_ex("builtin", SYM_BUILTIN, 0);
}
int SymbolTableEntry::operatorToVCPUCmd()
{
    //return ssize + 8;
    return vartype;
}

int SymbolTable::find(const char *ntf)
{
    return symbolTree.findValue(ntf);
}

std::string SymbolTable::get_friendly_name(int idx)
{
    int short_idx = (idx & STYPE_MASK);
    if (short_idx <= 0)
        return std::string("(end of input)");
    if (short_idx >= entries.size())
        return std::string("(invalid symbol)");

    std::string result = entries[short_idx].sname;

    if (idx & STYPE_POINTER)  result += std::string("*");
    if (idx & STYPE_DYNARRAY) result += std::string("[]");
    if (idx & STYPE_CONST)    result = std::string("const ") + result;

    return result;
}

std::string SymbolTable::get_name_string(int idx)
{
    if (idx < 0)
        return std::string("(end of input)");
    if (idx & STYPE_CONST)
    {
        idx &= ~STYPE_CONST;
        return "const " + get_name_string(idx);
    }

    if (idx & STYPE_DYNARRAY)
    {
        idx &= ~(STYPE_DYNARRAY | STYPE_POINTER);
        return get_name_string(idx) + "[]";
    }

    if (idx & STYPE_POINTER)
    {
        idx &= ~STYPE_POINTER;
        return get_name_string(idx) + "*";
    }

    return entries[idx].sname;
}

const char *SymbolTable::get_name(int idx)
{
    if (nameGenCache.count(idx) > 0)
    {
        return nameGenCache[idx];
    }

    std::size_t actualIdx = idx & STYPE_MASK;
    if (actualIdx < 0 || actualIdx >= entries.size())
        return NULL;

    std::string resultString = get_name_string(idx);
    char *result = (char *)malloc(resultString.length() + 1);
    strcpy(result, resultString.c_str());
    nameGenCache[idx] = result;
    return result;
}

int SymbolTable::add(const char *nta)
{
    return add_ex(nta, 0, 0);
}

int SymbolTable::add_ex(const char *nta, int typo, char sizee)
{
    if (find(nta) >= 0) return -1;

    int p_value = entries.size();

    SymbolTableEntry entry = {};
    entry.sname = std::string(nta);
    entry.stype = typo;
    entry.flags = 0;
    entry.vartype = 0;
    entry.soffs = 0;
    entry.ssize = sizee;
    entry.sscope = 0;
    entry.arrsize = 0;
    entry.extends = 0;
    entry.funcparamtypes = std::vector<unsigned long>(MAX_FUNCTION_PARAMETERS + 1);
    entry.funcParamDefaultValues = std::vector<int>(MAX_FUNCTION_PARAMETERS + 1);
    entry.funcParamHasDefaultValues = std::vector<bool>(MAX_FUNCTION_PARAMETERS + 1);
    entries.push_back(entry);

    symbolTree.addEntry(nta, p_value);
    return p_value;
}

int SymbolTable::add_operator(const char *nta, int priority, int vcpucmd)
{
    int nss = add_ex(nta, SYM_OPERATOR, priority);
    if (nss >= 0)
    {
        entries[nss].vartype = vcpucmd;
    }
    return nss;
}

SymbolTable sym;

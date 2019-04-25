#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions

#include <unordered_map>
#include <string>
#include <vector>

enum VartypeFlag : AGS::Vartype
{
    kVTYPE_Array = (0x10000000 << 0),
    kVTYPE_Const = (0x10000000 << 1),
    kVTYPE_DynArray = (0x10000000 << 2),
    kVTYPE_Pointer = (0x10000000 << 3),
    kVTYPE_FlagMask = (0x0FFFFFFF),
};

// There's another symbol definition in cc_symboldef.h which is deprecated
struct SymbolTableEntry {
    std::string sname;
    SymbolType stype; // e.g., kSYM_GlobalVar
    AGS::Flags flags;
    AGS::Vartype vartype; // may contain typeflags
    int soffs;
    long ssize; // or return type size for function
    short sscope; // or num arguments for function
    long arrsize; // num of bytes needed to contain the whole array (not num of elements)
    short extends; // inherits another class (classes) / owning class (member vars)
    // functions only, save types of return value and all parameters
    std::vector<AGS::Vartype> funcparamtypes;
    std::vector<int> funcParamDefaultValues;
    std::vector<bool> funcParamHasDefaultValues;

    SymbolTableEntry();
    SymbolTableEntry(const char *name, SymbolType stype, char ssize);

    inline int get_num_args() { return sscope % 100; }
    inline bool is_varargs() { return (sscope >= 100); }

    void set_attrfuncs(int attrget, int attrset);
    int get_attrget();
    int get_attrset();

    int operatorToVCPUCmd();

    int CopyTo(SymbolTableEntry &dest);
};

struct SymbolTable {
    // index for predefined symbols
    int normalCharSym;      // the symbol that corresponds to "char"
    int normalFloatSym;     // the symbol that corresponds to "float"
    int normalIntSym;       // the symbol that corresponds to "int"
    int normalNullSym;      // the symbol that corresponds to "null"
    int normalPointerSym;   // the symbol that corresponds to "*"
    int normalStringSym;    // the symbol that corresponds to "string"
    int normalThisSym;      // the symbol that corresponds to "this"
    int normalVoidSym;      // the symbol that corresponds to "void"
    int stringStructSym;    // the symbol that corresponds to "String" or whatever the stringstruct is
    int lastPredefSym;      // last predefined symbol

    std::vector<SymbolTableEntry> entries;

    SymbolTable();
    void reset();

    // Return the symbol to the name, or -1 if not found
    AGS::Symbol find(const char *name);  

    // Add to the symbol table if not in there already; in any case return the symbol
    AGS::Symbol find_or_add(const char *name); 

    // add the name to the symbol table, give it the type stype and the size ssize
    AGS::Symbol SymbolTable::add_ex(char const *name, SymbolType stype, int ssize);

    // add the name to the symbol table, empty type and size
    inline AGS::Symbol add(const char *name) { return add_ex(name, kSYM_NoType, 0); };

    // add the operator to the symbol table
    int  add_operator(const char *opname , int priority, int vcpucmd);

    // return name as char *, statically allocated
    char const *SymbolTable::get_name(int idx);

    // return the name to the symbol including "const" qualifier, including "*" or "[]"
    std::string const SymbolTable::get_name_string(int idx) const;

    // The symbol type, as given by the SYM_... constants
    SymbolType SymbolTable::get_type(AGS::Symbol symb);

    // the vartype of the symbol, i.e. "int" or "Dynarray *"
    inline AGS::Vartype SymbolTable::get_vartype(AGS::Symbol symb) { return (symb >= 0 && symb < entries.size()) ? entries[symb].vartype : -1; }

    // return the printable name of the vartype
    std::string const SymbolTable::get_vartype_name_string(long typ) const;



private:

    std::unordered_map<std::string, int> _findCache;
};


extern SymbolTable sym;

#endif //__CC_SYMBOLTABLE_H

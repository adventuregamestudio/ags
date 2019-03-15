#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions
#include "script/cc_treemap.h"

#include <map>
#include <string>
#include <vector>

#define STYPE_DYNARRAY  (0x10000000)
#define STYPE_CONST     (0x20000000)
#define STYPE_POINTER   (0x40000000)

#define STYPE_MASK       (0xFFFFFFF)

#define SYM_TEMPORARYTYPE -99

// There's another symbol definition in cc_symboldef.h which is deprecated
struct SymbolTableEntry {
    std::string sname;
    short stype;
    long flags;
    short vartype;
    int soffs;
    long ssize; // or return type size for function
    short sscope; // or num arguments for function
    long arrsize;
    short extends; // inherits another class (classes) / owning class (member vars)
    // functions only, save types of return value and all parameters
    std::vector<unsigned long> funcparamtypes;
    std::vector<int> funcParamDefaultValues;
    std::vector<bool> funcParamHasDefaultValues;

    SymbolTableEntry();
    SymbolTableEntry(const char *name, int stype, char ssize);

    inline int get_num_args() { return sscope % 100; }
    inline bool is_varargs() { return (sscope >= 100); }

    bool is_loadable_variable();

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
    AGS::Symbol SymbolTable::add_ex(char const *name, AGS::Symbol stype, int ssize);

    // add the name to the symbol table, empty type and size
    inline AGS::Symbol add(const char *name) { return add_ex(name, 0, 0); };

    // add the operator to the symbol table
    int  add_operator(const char *opname , int priority, int vcpucmd);

    // return the name to the symbol including "const" qualifier, including "*" or "[]"
    std::string const SymbolTable::get_name_string(int idx);

    // return name as char *, statically allocated
    char const *SymbolTable::get_name(int idx);

    // Ignore any type qualifiers that symb might contain, return its core type
    AGS::Symbol SymbolTable::get_type(AGS::Symbol symb);

private:

    std::map<std::string, int> _findCache;
};


extern SymbolTable sym;

#endif //__CC_SYMBOLTABLE_H

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

// So there's another symbol definition in cc_symboldef.h
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

    int get_num_args();

    int is_loadable_variable();

    void set_attrfuncs(int attrget, int attrset);
    int get_attrget();
    int get_attrset();

    int operatorToVCPUCmd();

    int CopyTo(SymbolTableEntry &dest);
};

struct SymbolTable {
    // index for predefined symbols
    int normalIntSym;
    int normalStringSym;
    int normalFloatSym;
    int normalVoidSym;
    int nullSym;
    int stringStructSym;  // can get overwritten with new String symbol defined in agsdefns.sh
    int lastPredefSym;

    // properties for symbols, size is numsymbols
    std::vector<SymbolTableEntry> entries;

    SymbolTable();
    void reset();    // clears table
    AGS::Symbol find(const char *);  // returns ID of symbol, or -1
    AGS::Symbol add_ex(const char *, AGS::Symbol, char);  // adds new symbol of type and size
    AGS::Symbol add(const char *);   // adds new symbol, returns -1 if already exists

    std::string SymbolTable::get_friendly_name(int idx);  // inclue ptr
    std::string SymbolTable::get_name_string(int idx);
    const char *get_name(int idx); // gets symbol name of index

    int  get_type(int ii);


private:

    std::map<int, char *> nameGenCache;

    ccTreeMap symbolTree;
    std::vector<char *> symbolTreeNames;

    int  add_operator(const char *, int priority, int vcpucmd); // adds new operator
};


extern SymbolTable sym;

#endif //__CC_SYMBOLTABLE_H

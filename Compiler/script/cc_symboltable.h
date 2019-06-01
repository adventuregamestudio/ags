#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions

#include <unordered_map>
#include <string>
#include <vector>

enum VartypeFlag : AGS::Vartype
{
    kVTY_Array = (0x10000000 << 0),
    kVTY_Const = (0x10000000 << 1),
    kVTY_DynArray = (0x10000000 << 2),
    kVTY_DynPointer = (0x10000000 << 3),
    kVTY_FlagMask = (0x0FFFFFFF),
};

struct SymbolTableEntry {
    std::string sname;
    SymbolType stype; // e.g., kSYM_GlobalVar
    AGS::Flags flags;
    AGS::Vartype vartype; // may contain typeflags
    AGS::CodeLoc soffs;
    int ssize; // or return type size for function
    int sscope; // or num arguments for function
    int arrsize; // num of elements in a standard (non-dynamic) array
    AGS::Vartype extends; // parent struct (for structs) / owning struct (for members)
    // functions only, save types of return value and all parameters
    std::vector<AGS::Vartype> funcparamtypes;
    std::vector<int> funcParamDefaultValues;
    std::vector<bool> funcParamHasDefaultValues;

    SymbolTableEntry();
    SymbolTableEntry(const char *name, SymbolType stype, char ssize);

    inline int get_num_args() { return sscope % 100; }
    inline bool is_varargs() { return (sscope >= 100); }

    int operatorToVCPUCmd();

    int CopyTo(SymbolTableEntry &dest);
};

struct SymbolTable {
private:
    // index for predefined symbols
    AGS::Symbol _charSym;      // the symbol that corresponds to "char"
    AGS::Symbol _floatSym;     // the symbol that corresponds to "float"
    AGS::Symbol _intSym;       // the symbol that corresponds to "int"
    AGS::Symbol _nullSym;      // the symbol that corresponds to "null"
    AGS::Symbol _pointerSym;   // the symbol that corresponds to "*"
    AGS::Symbol _stringSym;    // the symbol that corresponds to "string"
    AGS::Symbol _thisSym;      // the symbol that corresponds to "this"
    AGS::Symbol _voidSym;      // the symbol that corresponds to "void"
    AGS::Symbol _stringStructSym;    // the symbol that corresponds to "String" or whatever the stringstruct is
    AGS::Symbol _lastPredefSym;      // last predefined symbol
    std::unordered_map<std::string, int> _findCache;

public:
    std::vector<SymbolTableEntry> entries;

    SymbolTable();
    void reset();

    inline AGS::Symbol getCharSym() const { return _charSym; }
    inline AGS::Symbol getFloatSym() const { return _floatSym; }
    inline AGS::Symbol getIntSym() const { return _intSym; }
    inline AGS::Symbol getNullSym() const { return _nullSym; }
    inline AGS::Symbol getOldStringSym() const { return _stringSym; }
    inline AGS::Symbol getPointerSym() const { return _pointerSym; }
    inline AGS::Symbol getThisSym() const { return _thisSym; }
    inline AGS::Symbol getVoidSym() const { return _voidSym; }
    inline AGS::Symbol getStringStructSym() const { return _stringStructSym; }
    inline void setStringStructSym(AGS::Symbol s) { _stringStructSym = s; }
    inline AGS::Symbol getLastPredefSym() const { return _lastPredefSym; }

    // add the name to the symbol table, give it the type stype and the size ssize
    AGS::Symbol SymbolTable::add_ex(char const *name, SymbolType stype, int ssize);

    // add the name to the symbol table, empty type and size
    inline AGS::Symbol add(char const *name) { return add_ex(name, kSYM_NoType, 0); };

    // add the operator to the symbol table
    int add_operator(const char *opname , int priority, int vcpucmd);

    // Return the symbol to the name, or -1 if not found
    inline AGS::Symbol find(char const *name) { auto it = _findCache.find(name); return (_findCache.end() == it) ? -1 : it->second; }

    // Add to the symbol table if not in there already; in any case return the symbol
    inline AGS::Symbol find_or_add(char const *name) { AGS::Symbol ret = find(name); return (ret >= 0) ? ret : add(name); }

    // return name as char *, statically allocated
    inline char const *SymbolTable::get_name(AGS::Symbol sym) { static std::string str; str = get_name_string(sym); return str.c_str(); }

    // return the name to the symbol including "const" qualifier, including "*" or "[]"
    std::string const SymbolTable::get_name_string(AGS::Symbol sym) const;

    // The symbol type, as given by the kSYM_... constants
    SymbolType SymbolTable::get_type(AGS::Symbol symb) const;

    // the vartype of the symbol, i.e. "int" or "Dynarray *"
    inline AGS::Vartype SymbolTable::get_vartype(AGS::Symbol symb) { return (symb >= 0 && symb < static_cast<AGS::Symbol>(entries.size())) ? entries[symb].vartype : -1; }

    // the flags of a vartype, as given by the symbol table entry to its core type
    // -or- the flags of a symbol, as given by its symbol table entry
    inline AGS::Flags SymbolTable::get_flags(AGS::Vartype vt) { size_t idx = vt & kVTY_FlagMask; return (idx < entries.size()) ? entries[idx].flags : 0; }
    // return the printable name of the vartype
    std::string const SymbolTable::get_vartype_name_string(AGS::Vartype vartype) const;
   
};


extern SymbolTable sym;

#endif //__CC_SYMBOLTABLE_H

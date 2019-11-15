#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions          

#include <unordered_map>
#include <string>
#include <vector>

namespace AGS
{

enum VartypeFlag : AGS::Vartype
{
    kVTY_Array = (0x10000000 << 0),
    kVTY_Const = (0x10000000 << 1),
    kVTY_Dynarray = (0x10000000 << 2),
    kVTY_Managed = (0x10000000 << 3),
    kVTY_FlagMask = (0x0FFFFFFF),
};

enum VartypeType
{
    kVTT_Atomic = 0,
    kVTT_Array = 2 << 0,
    kVTT_Const = 2 << 1,
    kVTT_Dynarray = 2 << 2,
    kVTT_Dynpointer = 2 << 3,
};

struct SymbolTable;

struct SymbolTableEntry {
    friend SymbolTable;
protected:
    // Is (or has)  a vartype that can be recognized by a flag
    bool IsVTF(Flags f, SymbolTable const &symt) const;
    // Is (or has)  a vartype that can be recognized by a vartype type
    bool IsVTT(VartypeType vtt, SymbolTable const &symt) const;

public:
    std::string sname;
    SymbolType stype; // e.g., kSYM_GlobalVar
    int decl_secid, decl_line; // where this was declared
    AGS::Flags flags;
    AGS::CodeLoc soffs; // multiple use

    // Variables only
    AGS::Vartype vartype; // may contain typeflags

    // Variables, structs, struct members, and vartypes only
    size_t ssize;      // Size in bytes

    // Variables and functions only
    int sscope;     // for funcs, number of arguments + (is_variadic? 100 : 0)

    // Vartypes only
    VartypeType vartype_type;
    std::vector<size_t> dims; // number of elements in each dimension of static array
    
    // Vars or vartypes
    
    // Array or Dynarray
    size_t NumArrayElements(SymbolTable const &symt) const;
    size_t GetSize(SymbolTable const &symt) const;
    inline bool IsAnyArray(SymbolTable const &symt) const { return IsArray(symt) || IsDynarray(symt); };
    inline bool IsArray(SymbolTable const &symt) const { return IsVTT(kVTT_Array, symt); };
    inline bool IsAtomic(SymbolTable const &symt) const { return IsVTT(kVTT_Atomic, symt); };
    inline bool IsConst(SymbolTable const &symt) const { return IsVTT(kVTT_Const, symt); };
    inline bool IsDynarray(SymbolTable const &symt) const { return IsVTT(kVTT_Dynarray, symt); };
    inline bool IsDynpointer(SymbolTable const &symt) const { return IsVTT(kVTT_Dynpointer, symt); };
    inline bool IsDyn(SymbolTable const &symt) const { return IsVTT(kVTT_Dynarray, symt) || IsVTT(kVTT_Dynpointer, symt); };
    inline bool IsManaged(SymbolTable const &symt) const { return IsVTF(kSFLG_Managed, symt); };
    inline bool IsStruct(SymbolTable const &symt) const { return IsVTF(kSFLG_StructVartype, symt); };

    // Structs and struct members only
    AGS::Vartype extends; // parent struct (for structs) / owning struct (for members)

    // Functions only
    std::vector<AGS::Vartype> funcparamtypes;
    std::vector<int> funcParamDefaultValues;
    std::vector<bool> funcParamHasDefaultValues;
    inline size_t get_num_args() const { return sscope % 100; }
    inline bool is_varargs() const { return (sscope >= 100); }

    SymbolTableEntry();
    SymbolTableEntry(const char *name, SymbolType stype, size_t ssize);

    inline int operatorToVCPUCmd() const { return this->vartype; }

    inline int GetCPUOp() const { return ssize; };
};

struct SymbolTable {
    friend SymbolTableEntry;
private:
    // maps sections to IDs to save space
    class SectionMap
    {
    private:
        std::string _cacheSec;
        int _cacheId;
        std::vector <std::string> _section;
    public:
        int section2id(std::string const &sec);
        std::string const id2section(int id) const;
        void init();
        SectionMap() { init(); };
    } _sectionMap;

    // hashes pair<Vartype, VartypeType> for _vartypesCache
    struct VVTTHash
    {
        std::hash<Vartype> hash;
        size_t operator() (std::pair<Vartype, VartypeType> pair) const { return hash(pair.first ^ (1021 * pair.second)); };
    };

    // index for predefined symbols
    AGS::Symbol _charSym;      // the symbol that corresponds to "char"
    AGS::Symbol _floatSym;     // the symbol that corresponds to "float"
    AGS::Symbol _intSym;       // the symbol that corresponds to "int"
    AGS::Symbol _nullSym;      // the symbol that corresponds to "null"
    AGS::Symbol _pointerSym;   // the symbol that corresponds to "*"
    AGS::Symbol _stringSym;    // the symbol that corresponds to "string"
    AGS::Symbol _stringStructSym;    // the symbol that corresponds to "String" or whatever the stringstruct is
    AGS::Symbol _thisSym;      // the symbol that corresponds to "this"
    AGS::Symbol _voidSym;      // the symbol that corresponds to "void"
    AGS::Symbol _lastPredefSym;      // last predefined symbol

    AGS::Vartype _stringStructPtrVartype;

    mutable std::unordered_map<std::string, int> _findCache;
    mutable std::unordered_map<std::pair<Vartype, VartypeType>, Vartype, VVTTHash> _vartypesCache;

    inline bool IsVTT(Symbol s, VartypeType vtt) const { return IsInBounds(s) ? entries[s].IsVTT(vtt, *this) : false; }
    inline bool IsVTF(Symbol s, Flags f) const { return IsInBounds(s) ? entries[s].IsVTF(f, *this) : false; }

public:
    std::vector<SymbolTableEntry> entries;
    inline SymbolTableEntry &operator[](AGS::Symbol sym) { return entries.at(sym); };

    SymbolTable();
    inline void ResetCaches() const { _vartypesCache.clear(); };
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

    inline bool IsInBounds(Symbol s) const { return s > 0 && static_cast<size_t>(s) < entries.size(); }

    inline size_t GetSize(Symbol s) const { return IsInBounds(s) ? entries[s].GetSize(*this) : 0; };

    inline bool IsArray(Symbol s) const { return IsInBounds(s) ? entries[s].IsArray(*this) : false; };
    inline size_t NumArrayElements(Symbol s) const { return IsInBounds(s) ? entries[s].NumArrayElements(*this) : 0; };
    inline bool IsAtomic(Symbol s) const { return IsInBounds(s) ? entries[s].IsAtomic(*this) : false; };
    inline bool IsConst(Symbol s) const { return IsInBounds(s) ? entries[s].IsConst(*this) : false; };
    inline bool IsDynarray(Symbol s) const { return IsInBounds(s) ? entries[s].IsDynarray(*this) : false; };
    inline bool IsDynpointer(Symbol s) const { return IsInBounds(s) ? entries[s].IsDynpointer(*this) : false; };
    // Dynpointer or Dynarray
    inline bool IsDyn(Symbol s) const { return IsInBounds(s) ? entries[s].IsDyn(*this) : false; };
    inline bool IsManaged(Symbol s) const { return IsInBounds(s) ? entries[s].IsVTF(kSFLG_Managed, *this) : false; };
    inline bool IsStruct(Symbol s) const { return IsInBounds(s) ? entries[s].IsVTF(kSFLG_StructVartype, *this) : false; };
    // A predefined atomic vartype such as int and float.
    inline bool IsPrimitive(Symbol s) const { return (s > 0 && s <= getVoidSym()); };

    bool IsAnyTypeOfString(Symbol s) const;
    bool IsOldstring(Symbol s) const;

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
    inline char const *SymbolTable::get_name(AGS::Symbol sym) const { static std::string str; str = get_name_string(sym); return str.c_str(); }

    // return the name to the symbol including "const" qualifier, including "*" or "[]"
    std::string const SymbolTable::get_name_string(AGS::Symbol sym) const;

    // The symbol type, as given by the kSYM_... constants
    inline SymbolType get_type(AGS::Symbol symb) const { return IsInBounds(symb) ? entries[symb].stype : kSYM_NoType; };

    // the vartype of the symbol, i.e. "int" or "Dynarray *"
    inline AGS::Vartype SymbolTable::get_vartype(AGS::Symbol symb) const { return (symb >= 0 && symb < static_cast<AGS::Symbol>(entries.size())) ? entries.at(symb).vartype : -1; }

    // the flags of a vartype, as given by the symbol table entry to its core type
    // -or- the flags of a symbol, as given by its symbol table entry
    inline AGS::Flags SymbolTable::get_flags(AGS::Symbol vt) const { size_t idx = vt & kVTY_FlagMask; return (idx < entries.size()) ? entries.at(idx).flags : 0; }

    // return the printable name of the vartype
    std::string const SymbolTable::get_vartype_name_string(AGS::Vartype vartype) const;

    // Set/get section and line where the item is declared
    void set_declared(int idx, std::string const &section, int line);
    inline int get_declared_line(int idx) { return (*this)[idx].decl_line; };
    inline std::string const get_declared_section(int idx) const { return _sectionMap.id2section(entries.at(idx).decl_secid); };

    // The "Array[...] of vartype" vartype
    Vartype VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype);
    // The "Dynarray / Dynpointer/ Const ... of vartype" vartype
    Vartype VartypeWith(VartypeType vtt, Vartype vartype);
    // The vartype without the qualifiers given in vtt
    Vartype VartypeWithout(long vtt, Vartype vartype) const;

    // Fills compo_list with the indexes of all the strct components
    // Includes the ancesters' components
    int GetComponentsOfStruct(Symbol strct, std::vector<Symbol> &compo_list) const;

    // Unfortunately, a bit of a kludge. Expose the section to id mapping
    inline int section2id(std::string const &section) { return _sectionMap.section2id(section); };
    inline std::string const id2section(int id) const { return _sectionMap.id2section(id); };
};
} // namespace AGS
#endif //__CC_SYMBOLTABLE_H

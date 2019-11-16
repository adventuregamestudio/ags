#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions          

#include <unordered_map>
#include <string>
#include <vector>

namespace AGS
{

enum VartypeTypeT
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
    bool IsVTT(VartypeTypeT vtt, SymbolTable const &symt) const;

public:
    std::string SName;
    SymbolType SType; // e.g., kSYM_GlobalVar
    int DeclSectionId, DeclLine; // where this was declared
    Flags Flags;
    CodeLoc SOffset; // multiple use

    // Variables only
    Vartype vartype; // may contain typeflags

    // Variables, structs, struct members, and vartypes only
    size_t SSize;      // Size in bytes

    // Variables and functions only
    int SScope;     // for funcs, number of arguments + (is_variadic? 100 : 0)

    // Vartypes only
    VartypeTypeT VartypeType;
    std::vector<size_t> Dims; // number of elements in each dimension of static array
    
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
    AGS::Vartype Extends; // parent struct (for structs) / owning struct (for members)

    // Functions only
    std::vector<AGS::Vartype> FuncParamTypes;
    std::vector<int> FuncParamDefaultValues;
    std::vector<bool> FuncParamHasDefaultValues;
    inline size_t GetNumOfFuncArgs() const { return SScope % 100; }
    inline bool IsVarargsFunc() const { return (SScope >= 100); }

    SymbolTableEntry();
    SymbolTableEntry(const char *name, SymbolType stype, size_t ssize);

    inline int OpToVCPUCmd() const { return this->vartype; }

    inline int GetCPUOp() const { return SSize; };
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
        int Section2Id(std::string const &sec);
        std::string const Id2Section(int id) const;
        void init();
        SectionMap() { init(); };
    } _sectionMap;

    // hashes pair<Vartype, VartypeType> for _vartypesCache
    struct VVTTHash
    {
        std::hash<Vartype> hash;
        size_t operator() (std::pair<Vartype, VartypeTypeT> pair) const { return hash(pair.first ^ (1021 * pair.second)); };
    };

    // index for predefined symbols
    Symbol _charSym;      // the symbol that corresponds to "char"
    Symbol _floatSym;     // the symbol that corresponds to "float"
    Symbol _intSym;       // the symbol that corresponds to "int"
    Symbol _nullSym;      // the symbol that corresponds to "null"
    Symbol _pointerSym;   // the symbol that corresponds to "*"
    Symbol _stringSym;    // the symbol that corresponds to "string"
    Symbol _stringStructSym;    // the symbol that corresponds to "String" or whatever the stringstruct is
    Symbol _thisSym;      // the symbol that corresponds to "this"
    Symbol _voidSym;      // the symbol that corresponds to "void"
    Symbol _lastPredefSym;      // last predefined symbol

    AGS::Vartype _stringStructPtrVartype;

    mutable std::unordered_map<std::string, int> _findCache;
    mutable std::unordered_map<std::pair<Vartype, VartypeTypeT>, Vartype, VVTTHash> _vartypesCache;

    inline bool IsVTT(Symbol s, VartypeTypeT vtt) const { return IsInBounds(s) ? entries[s].IsVTT(vtt, *this) : false; }
    inline bool IsVTF(Symbol s, Flags f) const { return IsInBounds(s) ? entries[s].IsVTF(f, *this) : false; }

public:
    std::vector<SymbolTableEntry> entries;
    inline SymbolTableEntry &operator[](Symbol sym) { return entries.at(sym); };

    SymbolTable();
    inline void ResetCaches() const { _vartypesCache.clear(); };
    void reset();

    inline Symbol getCharSym() const { return _charSym; }
    inline Symbol getFloatSym() const { return _floatSym; }
    inline Symbol getIntSym() const { return _intSym; }
    inline Symbol getNullSym() const { return _nullSym; }
    inline Symbol getOldStringSym() const { return _stringSym; }
    inline Symbol getPointerSym() const { return _pointerSym; }
    inline Symbol getThisSym() const { return _thisSym; }
    inline Symbol getVoidSym() const { return _voidSym; }
    inline Symbol getStringStructSym() const { return _stringStructSym; }
    inline void setStringStructSym(Symbol s) { _stringStructSym = s; }
    inline Symbol getLastPredefSym() const { return _lastPredefSym; }

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
    Symbol SymbolTable::AddWithTypeAndSize(char const *name, SymbolType stype, int ssize);

    // add the name to the symbol table, empty type and size
    inline Symbol Add(char const *name) { return AddWithTypeAndSize(name, kSYM_NoType, 0); };

    // add the operator to the symbol table
    int AddOp(const char *opname, int priority, int vcpucmd);

    // Return the symbol to the name, or -1 if not found
    inline Symbol Find(char const *name) { auto it = _findCache.find(name); return (_findCache.end() == it) ? -1 : it->second; }

    // Add to the symbol table if not in there already; in any case return the symbol
    inline Symbol FindOrAdd(char const *name) { Symbol ret = Find(name); return (ret >= 0) ? ret : Add(name); }

    // return name as char *, statically allocated
    inline char const *SymbolTable::get_name(Symbol sym) const { static std::string str; str = get_name_string(sym); return str.c_str(); }

    // return the name to the symbol including "const" qualifier, including "*" or "[]"
    std::string const SymbolTable::get_name_string(Symbol sym) const;

    // The symbol type, as given by the kSYM_... constants
    inline SymbolType get_type(Symbol symb) const { return IsInBounds(symb) ? entries[symb].SType : kSYM_NoType; };

    // the vartype of the symbol, i.e. "int" or "Dynarray *"
    inline AGS::Vartype SymbolTable::get_vartype(Symbol symb) const { return (symb >= 0 && symb < static_cast<AGS::Symbol>(entries.size())) ? entries.at(symb).vartype : -1; }

    // the flags of a vartype, as given by the symbol table entry to its core type
    // -or- the flags of a symbol, as given by its symbol table entry
    inline Flags SymbolTable::get_flags(Symbol vt) const { return IsInBounds(vt) ? entries[vt].Flags : 0; }

    // return the printable name of the vartype
    std::string const SymbolTable::get_vartype_name_string(AGS::Vartype vartype) const;

    // Set/get section and line where the item is declared
    void set_declared(int idx, std::string const &section, int line);
    inline int get_declared_line(int idx) { return (*this)[idx].DeclLine; };
    inline std::string const get_declared_section(int idx) const { return _sectionMap.Id2Section(entries.at(idx).DeclSectionId); };

    // The "Array[...] of vartype" vartype
    Vartype VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype);
    // The "Dynarray / Dynpointer/ Const ... of vartype" vartype
    Vartype VartypeWith(VartypeTypeT vtt, Vartype vartype);
    // The vartype without the qualifiers given in vtt
    Vartype VartypeWithout(long vtt, Vartype vartype) const;

    // Fills compo_list with the indexes of all the strct components
    // Includes the ancesters' components
    int GetComponentsOfStruct(Symbol strct, std::vector<Symbol> &compo_list) const;

    // Unfortunately, a bit of a kludge. Expose the section to id mapping
    inline int Section2Id(std::string const &section) { return _sectionMap.Section2Id(section); };
    inline std::string const Id2Section(int id) const { return _sectionMap.Id2Section(id); };
};
} // namespace AGS
#endif //__CC_SYMBOLTABLE_H

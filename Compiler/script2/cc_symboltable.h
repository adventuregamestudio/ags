#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions          

#include <unordered_map>
#include <string>
#include <vector>

namespace AGS
{

enum VartypeType
{
    kVTT_Atomic = 0,
    kVTT_Array = 1 << 1,
    kVTT_Const = 1 << 2,
    kVTT_Dynarray = 1 << 3,
    kVTT_Dynpointer = 1 << 4,
};

struct SymbolTable;

struct SymbolTableEntry {
    friend SymbolTable;
protected:
    // Is (or has) a vartype that can be recognized by a flag
    bool IsVTF(SymbolTableFlag flag, SymbolTable const &symt) const;
    // Is (or has)  a vartype that can be recognized by a vartype type
    bool IsVTT(enum VartypeType vtt, SymbolTable const &symt) const;

public:

    enum ParamDefaultType
    {
        kDT_None = 0,
        kDT_Int,
        kDT_Float,
        kDT_Dyn,
    };

    static size_t const ParameterSScope = 1;

    struct ParamDefault
    {
        ParamDefaultType Type;
        union
        {
            int IntDefault;
            float FloatDefault;
            void *DynDefault;
        };

        inline ParamDefault() : Type(kDT_None) { }
        bool operator== (const ParamDefault &other) const;
        std::string ToString() const;
        int32_t ToInt32() const;
    };

    std::string SName;
    SymbolType SType; // e.g., kSYM_GlobalVar
    int DeclSectionId, DeclLine; // where this was declared
    FlagSet Flags;
    TypeQualifierSet TypeQualifiers;
    CodeLoc SOffset; // multiple use

    // Variables only
    AGS::Vartype Vartype; // may contain typeflags

    // Structs and vartypes only
    size_t SSize;      // Size in bytes

    // Variables and functions only
    size_t SScope;     // for funcs, whether the func is variadic

    // Vartypes only
    enum VartypeType VartypeType;
    std::vector<size_t> Dims; // number of elements in each dimension of static array
    
    // Structs and struct members only
    AGS::Vartype Extends; // parent struct (for structs) / owning struct (for members)

    // Functions only
    std::vector<AGS::Vartype> FuncParamVartypes;
    std::vector<ParamDefault> FuncParamDefaultValues;

    // Operator types only
    CodeCell OperatorOpcode;
    int OperatorBinaryPrio; // or 0 if not a binary operator
    int OperatorUnaryPrio;  // of 0 if not a unary operator

public:
    // General
    inline bool IsAnyArray(SymbolTable const &symt) const { return IsArray(symt) || IsDynarray(symt); }
    inline bool IsArray(SymbolTable const &symt) const { return IsVTT(kVTT_Array, symt); }
    inline bool IsAtomic(SymbolTable const &symt) const { return IsVTT(kVTT_Atomic, symt); }
    inline bool IsBuiltin(SymbolTable const &symt) const { return IsVTF(kSFLG_StructBuiltin, symt); }
    inline bool IsConst(SymbolTable const &symt) const { return IsVTT(kVTT_Const, symt); }
    inline bool IsDynarray(SymbolTable const &symt) const { return IsVTT(kVTT_Dynarray, symt); }
    inline bool IsDynpointer(SymbolTable const &symt) const { return IsVTT(kVTT_Dynpointer, symt); }
    inline bool IsDyn(SymbolTable const &symt) const { return IsVTT(kVTT_Dynarray, symt) || IsVTT(kVTT_Dynpointer, symt); }
    inline bool IsImport() const { return FlagIsSet(TypeQualifiers, kTQ_Import); }
    inline bool IsManaged(SymbolTable const &symt) const { return IsVTF(kSFLG_StructManaged, symt); }
    inline bool IsStruct(SymbolTable const &symt) const { return IsVTF(kSFLG_StructVartype, symt); }
    inline bool IsOperator() const { return (OperatorBinaryPrio >= 0) || (OperatorUnaryPrio >= 0); }
    inline bool IsParameter() const { return ParameterSScope == SScope; };

    // Array or Dynarray
    size_t NumArrayElements(SymbolTable const &symt) const;
    size_t GetSize(SymbolTable const &symt) const;
    
    // Functions
    inline size_t GetNumOfFuncParams() const { return FuncParamVartypes.size() - 1; }
    inline bool IsVarargsFunc() const { return (SScope > 0u); }
    inline bool HasParamDefault(size_t param) const { return kDT_None != FuncParamDefaultValues[param].Type; }

    SymbolTableEntry();
    SymbolTableEntry(std::string const &name, SymbolType stype = kSYM_NoType, size_t ssize = 0);
};

struct SymbolTable {
    friend SymbolTableEntry;

private:  
    // hashes pair<Vartype, VartypeType> for _vartypesCache
    struct VVTTHash
    {
        std::hash<Vartype> hash;
        size_t operator() (std::pair<Vartype, enum VartypeType> pair) const { return hash(pair.first ^ (1021 * pair.second)); };
    };

    // index for predefined symbols
    Symbol _charSym;      // the symbol that corresponds to "char"
    Symbol _floatSym;     // the symbol that corresponds to "float"
    Symbol _intSym;       // the symbol that corresponds to "int"
    Symbol _longSym;      // the symbol that corresponds to "long"
    Symbol _shortSym;     // the symbol that corresponds to "short"
    Symbol _nullSym;      // the symbol that corresponds to "null"
    Symbol _dynpointerSym;   // the symbol that corresponds to "*"
    Symbol _oldStringSym;    // the symbol that corresponds to "string"
    Symbol _stringStructSym; // the symbol that corresponds to "String" or whatever the stringstruct is
    Symbol _thisSym;      // the symbol that corresponds to "this"
    Symbol _voidSym;      // the symbol that corresponds to "void"
    Symbol _lastPredefSym;   // last predefined symbol

    AGS::Vartype _stringStructPtrVartype;

    mutable std::unordered_map<std::string, int> _findCache;
    mutable std::unordered_map<std::pair<Vartype, enum VartypeType>, Vartype, VVTTHash> _vartypesCache;

    inline bool IsVTT(Symbol s, enum VartypeType vtt) const { return IsInBounds(s) ? entries[s].IsVTT(vtt, *this) : false; }
    inline bool IsVTF(Symbol s, SymbolTableFlag f) const { return IsInBounds(s) ? entries[s].IsVTF(f, *this) : false; }

public:
    std::vector<SymbolTableEntry> entries;
    inline SymbolTableEntry &operator[](Symbol sym) { return entries.at(sym); };

    SymbolTable();
    inline void ResetCaches() const { _vartypesCache.clear(); };
    void reset();

    inline Symbol GetCharSym() const { return _charSym; }
    inline Symbol GetFloatSym() const { return _floatSym; }
    inline Symbol GetIntSym() const { return _intSym; }
    inline Symbol GetNullSym() const { return _nullSym; }
    inline Symbol GetOldStringSym() const { return _oldStringSym; }
    inline Symbol GetDynpointerSym() const { return _dynpointerSym; }
    inline Symbol GetThisSym() const { return _thisSym; }
    inline Symbol GetVoidSym() const { return _voidSym; }
    inline Symbol GetStringStructSym() const { return _stringStructSym; }
    inline void SetStringStructSym(Symbol s) { _stringStructSym = s; }
    inline Symbol GetLastPredefSym() const { return _lastPredefSym; }

    inline bool IsInBounds(Symbol s) const { return s > 0 && static_cast<size_t>(s) < entries.size(); }

    inline size_t GetSize(Symbol s) const { return IsInBounds(s) ? entries[s].GetSize(*this) : 0; };

    // int, long, char, an enum etc.
    bool IsAnyIntegerVartype(Symbol s) const;
    inline bool IsArray(Symbol s) const { return IsInBounds(s) ? entries[s].IsArray(*this) : false; }
    inline size_t NumArrayElements(Symbol s) const { return IsInBounds(s) ? entries[s].NumArrayElements(*this) : 0; }
    inline bool IsAtomic(Symbol s) const { return IsInBounds(s) ? entries[s].IsAtomic(*this) : false; }
    inline bool IsBuiltin(Symbol s) const { return IsInBounds(s) ? entries[s].IsBuiltin(*this) : false; }
    inline bool IsConst(Symbol s) const { return IsInBounds(s) ? entries[s].IsConst(*this) : false; }
    inline bool IsDynarray(Symbol s) const { return IsInBounds(s) ? entries[s].IsDynarray(*this) : false; }
    inline bool IsDynpointer(Symbol s) const { return IsInBounds(s) ? entries[s].IsDynpointer(*this) : false; }
    // Dynpointer or Dynarray
    inline bool IsDyn(Symbol s) const { return IsInBounds(s) ? entries[s].IsDyn(*this) : false; }
    inline bool IsImport(Symbol s) const { return IsInBounds(s) ? entries[s].IsImport() : false; }
    inline bool IsManaged(Symbol s) const { return IsInBounds(s) ? entries[s].IsManaged(*this) : false; }
    inline bool IsStruct(Symbol s) const { return IsInBounds(s) ? entries[s].IsStruct(*this) : false; }
    // A predefined atomic vartype such as int and float.
    inline bool IsPrimitive(Symbol s) const { return (s > 0 && s <= GetVoidSym()); };

    inline bool IsOperator(Symbol s) const { return entries[s].IsOperator(); }
    inline int BinaryOpPrio(Symbol s) const { return entries[s].OperatorBinaryPrio; }
    inline int UnaryOpPrio(Symbol s) const { return entries[s].OperatorUnaryPrio; }
    inline CodeCell GetOperatorOpcode(Symbol s) const { return entries[s].OperatorOpcode; }

    bool IsAnyTypeOfString(Symbol s) const;
    bool IsOldstring(Symbol s) const;

    // add the name to the symbol table, give it the type stype and the size ssize
    Symbol Add(std::string const &name, SymbolType stype = kSYM_NoType, int ssize = 0);

    // add the operator opname to the symbol table
    // Priorities: lower value = higher prio; negative value means no priority
    AGS::Symbol AddOp(std::string const &opname, SymbolType sty, CodeCell opcode, int binary_prio = -1, int unary_prio = -1);

    // Return the symbol to the name, or -1 if not found
    inline Symbol Find(std::string const &name) { auto it = _findCache.find(name); return (_findCache.end() == it) ? -1 : it->second; }
    inline Symbol FindCString(char *name) { return Find(name); } // for usage in the debugger

    // Add to the symbol table if not in there already; in any case return the symbol
    inline Symbol FindOrAdd(std::string const &name) { Symbol ret = Find(name); return (ret >= 0) ? ret : Add(name); }

    // return the name to the symbol including "const" qualifier, including "*" or "[]"
    std::string const SymbolTable::GetName(Symbol symbl) const;

    // The symbol type, as given by the kSYM_... constants
    inline SymbolType GetSymbolType(Symbol symb) const { return IsInBounds(symb) ? entries[symb].SType : kSYM_NoType; };

    // the vartype of the symbol, i.e. "int" or "Dynarray *"
    inline AGS::Vartype GetVartype(Symbol symb) const { return (symb >= 0 && symb < static_cast<AGS::Symbol>(entries.size())) ? entries.at(symb).Vartype : -1; }

    // Set/get section and line where the item is declared
    void SetDeclared(int idx, int section_id, int line);
    inline int GetDeclaredLine(int idx) { return (*this)[idx].DeclLine; };
    inline int GetDeclaredSectionId(int idx) { return (*this)[idx].DeclSectionId; };

    // The "Array[...] of vartype" vartype
    Vartype VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype);
    // The "Dynarray / Dynpointer/ Const ... of vartype" vartype
    Vartype VartypeWith(enum VartypeType vtt, Vartype vartype);
    // The vartype without the qualifiers given in vtt
    Vartype VartypeWithout(long vtt, Vartype vartype) const;

    // Fills compo_list with the indexes of all the strct components
    // Includes the ancesters' components
    int GetComponentsOfStruct(Symbol strct, std::vector<Symbol> &compo_list) const;

    };
} // namespace AGS
#endif //__CC_SYMBOLTABLE_H

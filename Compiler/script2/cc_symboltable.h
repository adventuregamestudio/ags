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

enum Predefined : Symbol
{
    kKW_NoSymbol = 0,
    // Primitive vartypes
    kKW_Char,       // "char"
    kKW_Float,      // "float"
    kKW_Int,        // "int"
    kKW_Long,       // "long"
    kKW_Short,      // "short"
    kKW_String,     // "string"
    kKW_Void,       // "void"

    kKW_CloseBracket,       // "]"
    kKW_CloseParenthesis,   // ")"
    kKW_Dot,        // "."
    kKW_Null,       // "null"
    kKW_OpenBracket,        // "["
    kKW_OpenParenthesis,    // (
    kKW_Not,        // "!"
    kKW_BitNeg,     // "~"
    kKW_Multiply,   // "*"
    kKW_Dynpointer = kKW_Multiply,
    kKW_Divide,     // "/"
    kKW_Modulo,     // "%"
    kKW_Plus,       // "+"
    kKW_Minus,      // "-"
    kKW_ShiftLeft,  // "<<"
    kKW_ShiftRight, // ">>"
    kKW_BitAnd,     // "&"
    kKW_BitOr,      // "|"
    kKW_BitXor,     // "^"
    kKW_Equal,      // "=="
    kKW_NotEqual,   // "!="
    kKW_Greater,    // ">"
    kKW_Less,       // "<"
    kKW_GreaterEqual,   // ">="
    kKW_LessEqual,      // "<="
    kKW_And,        // "&&"
    kKW_Or,         // "||"
    kKW_Tern,       // "?"

    kKW_This,       // "this"

    // Assignments
    kKW_Assign,     // "="
    kKW_AssignPlus, // "+="
    kKW_AssignMinus,// "-="
    kKW_AssignMultiply, // "*="
    kKW_AssignDivide,   // "/="
    kKW_AssignBitAnd,   // "&="
    kKW_AssignBitOr,    // "|="
    kKW_AssignBitXor,   // "^="
    kKW_AssignShiftLeft,// "<<="
    kKW_AssignShiftRight,   // ">>="
    kKW_Increment,      // "++"
    kKW_Decrement,      // "--"

    // Other keywords and symbols
    kKW_Attribute,  // "attribute"
    kKW_Autoptr,    // "autoptr"
    kKW_Break,      // "break"
    kKW_Builtin,    // "builtin"
    kKW_Case,       // "case"
    kKW_CloseBrace, // "}"
    kKW_Comma,      // ","
    kKW_Const,      // "const"
    kKW_Continue,   // "continue"
    kKW_Default,    // "cefault"
    kKW_Do,         // "do"
    kKW_Else,       // "else"
    kKW_Enum,       // "enum"
    kKW_Export,     // "export"
    kKW_Extends,    // "extends"
    kKW_For,        // "for"
    kKW_If,         // "if"
    kKW_Import,     // "import"
    kKW_ImportTry,  // "_tryimport"
    kKW_Internalstring,   // "internalstring"
    kKW_Colon,      // ":"
    kKW_Noloopcheck,// "noloopcheck"
    kKW_Managed,    // "managed"
    kKW_ScopeRes,   // "::"
    kKW_New,        // "new"
    kKW_OpenBrace,  // "{"
    kKW_Protected,  // "protected"
    kKW_Readonly,   // "readonly"
    kKW_Return,     // "return"
    kKW_Semicolon,  // ";"
    kKW_Static,     // "static"
    kKW_Struct,     // "struct"
    kKW_Switch,     // "switch"
    kKW_Varargs,    // "..."
    kKW_While,      // "while"
    kKW_Writeprotected,  // "writeprotected"
    kKW_LastPredefined = kKW_Writeprotected,
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
    static size_t const kParameterSScope = 1;
    static size_t const kFunctionSScope = 2;

    static size_t const kNoSrcLocation = INT_MAX;
    
    enum ParamDefaultType
    {
        kDT_None = 0,
        kDT_Int,
        kDT_Float,
        kDT_Dyn,
    };

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
    SymbolType SType; // e.g., SymT::kGlobalVar
    size_t Declared; // where this was declared
    FlagSet Flags;
    TypeQualifierSet TypeQualifiers;
    CodeLoc SOffset; // multiple use

    // Variables only
    AGS::Vartype Vartype;   
    AGS::Vartype Parent;    // struct and enum members

    // Variables and functions only
    size_t SScope;     // for funcs, whether the func is variadic

    // Vartypes only
    size_t SSize;      // Size in bytes
    enum VartypeType VartypeType;
    std::vector<size_t> Dims; // number of elements in each dimension of static array
    std::vector<Symbol> Children; // of structs and enums

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
    inline bool IsParameter() const { return kParameterSScope == SScope; };

    // Array or Dynarray
    size_t NumArrayElements(SymbolTable const &symt) const;
    size_t GetSize(SymbolTable const &symt) const;
    
    // Functions
    inline size_t GetNumOfFuncParams() const { return FuncParamVartypes.size() - 1; }
    inline bool IsVarargsFunc() const { return (SScope > 0u); }
    inline bool HasParamDefault(size_t param) const { return kDT_None != FuncParamDefaultValues[param].Type; }

    SymbolTableEntry();
    SymbolTableEntry(std::string const &name, SymbolType stype = SymT::kNoType, size_t ssize = 0);
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

    Symbol _stringStructSym; // the symbol that corresponds to "String" or whatever the stringstruct is
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

    inline Symbol GetStringStructSym() const { return _stringStructSym; }
    inline void SetStringStructSym(Symbol s) { _stringStructSym = s; }
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
    inline bool IsPrimitive(Symbol s) const { return (s > 0 && s <= kKW_Void); };

    inline bool IsOperator(Symbol s) const { return entries[s].IsOperator(); }
    inline bool IsVartype(Symbol s) const { return SymT::kVartype == GetSymbolType(s); }
    inline int BinaryOpPrio(Symbol s) const { return entries[s].OperatorBinaryPrio; }
    inline int UnaryOpPrio(Symbol s) const { return entries[s].OperatorUnaryPrio; }
    inline CodeCell GetOperatorOpcode(Symbol s) const { return entries[s].OperatorOpcode; }

    bool IsAnyTypeOfString(Symbol s) const;
    bool IsOldstring(Symbol s) const;

    // add the name to the symbol table, give it the type stype and the size ssize
    Symbol Add(std::string const &name, SymbolType stype = SymT::kNoType, int ssize = 0);

    // add the symbol to the symbol table at [kw]. Only use during initialization.
    Symbol Add(Predefined kw, std::string const &name, SymbolType stype, int ssize = 0);

    // add the operator opname to the symbol table
    // Priorities: lower value = higher prio; negative value means no priority
    Symbol AddOp(Predefined kw, std::string const &opname, SymbolType sty, CodeCell opcode, int binary_prio = -1, int unary_prio = -1);

    // Return the symbol to the name, or -1 if not found
    inline Symbol Find(std::string const &name) { auto it = _findCache.find(name); return (_findCache.end() == it) ? -1 : it->second; }
    inline Symbol FindCString(char *name) { return Find(name); } // for usage in the debugger

    // Add to the symbol table if not in there already; in any case return the symbol
    inline Symbol FindOrAdd(std::string const &name) { Symbol ret = Find(name); return (ret >= 0) ? ret : Add(name); }

    // return the name to the symbol including "const" qualifier, including "*" or "[]"
    std::string const SymbolTable::GetName(Symbol symbl) const;

    // The symbol type, as given by the SymT::k... constants
    inline SymbolType GetSymbolType(Symbol symb) const { return IsInBounds(symb) ? entries[symb].SType : SymT::kNoType; };

    // the vartype of the symbol, i.e. "int" or "Dynarray *"
    inline AGS::Vartype GetVartype(Symbol symb) const { return (symb >= 0 && symb < static_cast<AGS::Symbol>(entries.size())) ? entries.at(symb).Vartype : -1; }

    // Set/get the position in the source where the item is declared
    inline void SetDeclared(int idx, size_t declared) { (*this)[idx].Declared = declared; }
    inline int GetDeclared(int idx) { return (*this)[idx].Declared; };
    
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

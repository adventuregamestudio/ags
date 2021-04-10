#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions          

#include <unordered_map>
#include <map>
#include <bitset>
#include <string>
#include <vector>

namespace AGS
{

// In what type of memory the variable is allocated
enum class ScT  // Scope type
{
    kNone = 0,
    kGlobal,
    kImport,
    kLocal,
};
typedef ScT ScopeType;

enum class VTT  // Vartype type
{
    kAtomic = 0,
    kArray,
    kConst,
    kDynarray,
    kDynpointer,
};
typedef VTT VartypeType;

enum class VTF : size_t // Vartype flag
{
    kUndefined = 0,     // Undefined or only forward-defined
    kAutoptr,           // Will be displayed without the '*'
    kBuiltin,           // May not be instantiated with new (but new[] is okay)
    kEnum,              // Is an enum
    kIntegerVartype,    // Is any type of integer, e.g., int, 'enum's etc.
    kManaged,           // Must be instantiated with new or new[]
    kStruct,            // Is a struct
};
constexpr VTF kLastVartypeFlag = VTF::kStruct;
typedef VTF VartypeFlag;

// Wrapper around the bitset so that it is indexed by a vartype flag.
class VartypeFlags
{
    std::bitset<16u> _flags;

public:
    inline bool operator[](VartypeFlag f) const { return _flags[static_cast<size_t>(f)]; }
    // This ugly, ugly piece of C++ clumsiness is needed so that you can assign through the [] operator ))-:
    inline std::bitset<16u>::reference operator[](VartypeFlag f) { return _flags[static_cast<size_t>(f)]; }

    inline bool operator==(VartypeFlags const &other) const { return this->_flags == other._flags; }
    inline bool operator!=(VartypeFlags const &other) const { return !(*this == other); }
};

enum class TQ : size_t // Type qualifier
{
    kAttribute = 0,
    kAutoptr,
    kBuiltin,
    kConst,
    kImport,
    kManaged,
    kProtected,
    kReadonly,
    kStatic,
    kStringstruct,
    kWriteprotected,
};
typedef TQ TypeQualifier;

class TypeQualifierSet
{
    std::bitset<16u> _flags;

    // Map a type qualifier to a symbol so that you can use _sym.GetName() on it
    std::map<TypeQualifier, Symbol> const &TQToSymbolMap() const;

public:
    // This ugly, ugly piece of C++ clumsiness is needed so that you can assign through the [] operator ))-:
    inline std::bitset<16u>::reference operator[](TypeQualifier f) { return _flags[static_cast<size_t>(f)]; }
    inline bool operator[](TypeQualifier f) const { return _flags[static_cast<size_t>(f)]; }

    inline bool operator==(TypeQualifierSet const &other) const { return this->_flags == other._flags; }
    inline bool operator!=(TypeQualifierSet const &other) const { return !(*this == other); }

    inline Symbol TQ2Symbol(TypeQualifier tq) const { return TQToSymbolMap().at(tq); }

    // For iterating over type qualifiers; use it->first to get the qualifier
    // for (auto it = tqs.begin(); it != tqs.end(); it++)
    inline auto begin() const { return TQToSymbolMap().begin(); }
    inline auto end() const { return TQToSymbolMap().end(); }

    inline bool empty() { return _flags == std::bitset<16u>{}; }
};

// Note: Don't convert to enum class: Only the _start_ of the symbol table vector
// corresponds to the predefined symbols in a 1:1 way. A lot of other symbols follow.
// In consequence, we need to convert freely between Predefined and Symbol.
enum Predefined : Symbol
{
    kKW_NoSymbol = 0,

    kKW_Char,           // "char"
    kKW_Float,          // "float"
    kKW_Int,            // "int"
    kKW_Long,           // "long"
    kKW_Short,          // "short"
    kKW_String,         // "string"
    kKW_Void,           // "void"

    kKW_And,            // "&&"
    kKW_BitAnd,         // "&"
    kKW_BitNeg,         // "~"
    kKW_BitOr,          // "|"
    kKW_BitXor,         // "^"
    kKW_CloseBracket,       // "]"
    kKW_CloseParenthesis,   // ")"
    kKW_Divide,         // "/"
    kKW_Dot,            // "."
    kKW_Equal,          // "=="
    kKW_Greater,        // ">"
    kKW_GreaterEqual,   // ">="
    kKW_Less,           // "<"
    kKW_LessEqual,      // "<="
    kKW_Minus,          // "-"
    kKW_Modulo,         // "%"
    kKW_Multiply,       // "*"
    kKW_Not,            // "!"
    kKW_NotEqual,       // "!="
    kKW_Null,           // "null"
    kKW_OpenBracket,        // "["
    kKW_OpenParenthesis,    // "("
    kKW_Or,             // "||"
    kKW_Plus,           // "+"
    kKW_ShiftLeft,      // "<<"
    kKW_ShiftRight,     // ">>"
    kKW_Tern,           // "?"

    kKW_This,           // "this"

    kKW_Assign,         // "="
    kKW_AssignBitAnd,   // "&="
    kKW_AssignBitOr,    // "|="
    kKW_AssignBitXor,   // "^="
    kKW_AssignDivide,   // "/="
    kKW_AssignMinus,    // "-="
    kKW_AssignMultiply, // "*="
    kKW_AssignPlus,     // "+="
    kKW_AssignShiftLeft,    // "<<="
    kKW_AssignShiftRight,   // ">>="

    kKW_Decrement,      // "--"
    kKW_Increment,      // "++"

    kKW_Attribute,      // "attribute"
    kKW_Autoptr,        // "autoptr"
    kKW_Break,          // "break"
    kKW_Builtin,        // "builtin"
    kKW_Case,           // "case"
    kKW_CloseBrace,     // "}"
    kKW_Colon,          // ":"
    kKW_Comma,          // ","
    kKW_Const,          // "const"
    kKW_Continue,       // "continue"
    kKW_Default,        // "cefault"
    kKW_Do,             // "do"
    kKW_DotDotDot,      // "..."
    kKW_Else,           // "else"
    kKW_Enum,           // "enum"
    kKW_Export,         // "export"
    kKW_Extends,        // "extends"
    kKW_For,            // "for"
    kKW_If,             // "if"
    kKW_ImportStd,      // "import"
    kKW_ImportTry,      // "_tryimport"
    kKW_Internalstring, // "internalstring"
    kKW_Managed,        // "managed"
    kKW_New,            // "new"
    kKW_Noloopcheck,    // "noloopcheck"
    kKW_OpenBrace,      // "{"
    kKW_Protected,      // "protected"
    kKW_Readonly,       // "readonly"
    kKW_Return,         // "return"
    kKW_ScopeRes,       // "::"
    kKW_Semicolon,      // ";"
    kKW_Static,         // "static"
    kKW_Struct,         // "struct"
    kKW_Switch,         // "switch"
    kKW_While,          // "while"
    kKW_Writeprotected,  // "writeprotected"
};
constexpr Predefined kKW_LastPredefined = kKW_Writeprotected;
constexpr Predefined kKW_Dynpointer = kKW_Multiply;

// For function parameters, used in functions of the symbol table
struct FuncParameterDesc
{
    AGS::Vartype Vartype = kKW_NoSymbol;
    Symbol Name = kKW_NoSymbol;
    Symbol Default = kKW_NoSymbol;
};

struct SymbolTable;
struct SymbolTableEntry;

class SymbolTableConstant
{
public:
    static size_t const kParameterScope = 1;
    static size_t const kFunctionScope = 2;

    static size_t const kNoSrcLocation = INT_MAX;

    static size_t const kNoPrio = -1;
};

struct SymbolTableEntry : public SymbolTableConstant
{
    std::string Name = "";
    size_t Declared = kNoSrcLocation;    // where this was declared, pertains to _src
    size_t Scope = 0u;   
    bool Accessed = false;  // will be set to 'true' on first access

    // For const values; those must point to another symbol that is a literal
    struct ConstantDesc
    {
        Symbol ValueSym = kKW_NoSymbol;
    } *ConstantD = nullptr;

    // For components, e.g. of struct
    // This is the record of the _qualified_ struct component.
    // For example,  "A::B" has a ComponentD in which Vartype == A and Component == B
    // The _unqualified_ name does _not_ have a ComponentD. So "B" in the example above does _not_ have a ComponentD.
    struct ComponentDesc
    {
        Symbol Component = kKW_NoSymbol; // the naked symbol name (for a::b, it is b)
        Vartype Parent = kKW_NoSymbol; // The vartype of the component (for a::b, it is a)
        size_t Offset = 0u; // offset from the start of the struct of where the component is allocated
        bool IsFunction = false;
    } *ComponentD = nullptr;

    // For Delimeters
    struct DelimeterDesc
    {
        bool Opening = false;   // whether it is an opener, e.g., '('
        Symbol Partner = kKW_NoSymbol; // the corresponding closer to the opener and vice versa
        bool CanBePartOfAnExpression = false;
    } *DelimeterD = nullptr;

    // For functions
    struct FunctionDesc
    {
        TypeQualifierSet TypeQualifiers = {};
        // [0] describes the return type of the function
        std::vector<FuncParameterDesc> Parameters = {};
        CodeLoc Offset = 0;
        bool IsVariadic = false;
        bool NoLoopCheck = false;
    } *FunctionD = nullptr;

    // For literal values, e.g. 10, "foo", 2.0
    struct LiteralDesc
    {
        AGS::Vartype Vartype = kKW_NoSymbol;
        CodeCell Value = -77; // the value to use in the Bytecode
    } *LiteralD = nullptr;

    // For operators
    struct OperatorDesc
    {
        CodeCell Opcode = 0;
        int BinaryPrio = kNoPrio; 
        int UnaryPrio = kNoPrio;
        bool CanBePartOfAnExpression = false;
    } *OperatorD = nullptr;

    // For variables
    struct VariableDesc
    {
        TypeQualifierSet TypeQualifiers = {};
        AGS::Vartype Vartype = kKW_NoSymbol;
        size_t Offset = 0u;
    } *VariableD = nullptr;

    // For vartypes
    struct VartypeDesc
    {
        VartypeType Type = VTT::kAtomic;
        size_t Size = 0u;      // Size in bytes
        AGS::Vartype BaseVartype = kKW_NoSymbol;
        std::vector<size_t> Dims = {}; // For classic arrays: Number of elements in each dimension of static array
        std::map<Symbol, Symbol> Components = {}; // Maps the unqualified component to the qualified component
        Symbol Parent = kKW_NoSymbol; // For structs: this vartype extends the Parent
        VartypeFlags Flags;
    } *VartypeD = nullptr;

    SymbolTableEntry() = default;
    // Deep copy semantics for the pointers
    SymbolTableEntry(SymbolTableEntry const &orig);
    ~SymbolTableEntry();
    // Deep copy semantics for the pointers
    SymbolTableEntry &SymbolTableEntry::operator=(const SymbolTableEntry &);
    // Note, does not clear the Name field
    void Clear();
};

struct SymbolTable : public SymbolTableConstant
{

private:  
    // hashes pair<Vartype, VartypeType> for _vartypesCache
    struct VVTTHash
    {
        std::hash<Vartype> hash;
        size_t operator() (std::pair<Vartype, VartypeType> pair) const { return hash(pair.first ^ (1021 * static_cast<int>(pair.second))); };
    };

    Symbol _stringStructSym; // the symbol that corresponds to "String" or whatever the stringstruct is
    AGS::Vartype _stringStructPtrSym;
    Symbol _lastAllocated; // the last symbol that has been allocated at initialization; don't confuse with LastPredefined

    mutable std::unordered_map<std::string, int> _findCache;
    mutable std::unordered_map<std::pair<Vartype, VartypeType>, Vartype, VVTTHash> _vartypesCache;

    // add the "No Symbol" symbol to the symbol table at [kw].
    Symbol AddNoSymbol(Predefined kw, std::string const &name);

    // Add the assign symbol to the symbol table at [kw]. Priority as yet unused.
    Symbol AddAssign(Predefined kw, std::string const &name, size_t prio);

    // Add the modifying assign symbol to the symbol table at [kw]. Priority as yet unused.
    Symbol AddAssignMod(Predefined kw, std::string const &name, CodeCell opcode, size_t prio);

    // Add the delimeter symbol to the symbol table at [kw]. 'partner' is the closer to the opener or vice versa.
    Symbol AddDelimeter(Predefined kw, std::string const &name, bool is_opener, Symbol partner, bool can_be_expression);

    // Add the keyword symbol to the symbol table at [kw].
    Symbol AddKeyword(Predefined kw, std::string const &name);

    // Add the modifier symbol to the symbol table at [kw]. Priorities as yet unused.
    Symbol AddModifier(Predefined kw, std::string const &name, CodeCell opcode, size_t prefix_prio, size_t postfix_prio);

    // Add the operator symbol to the symbol table at [kw]. 
    // Priorities: lower value = higher prio.
    Symbol AddOperator(Predefined kw, std::string const &name, CodeCell opcode, size_t binary_prio, size_t unary_prio);

    // Add the vartype symbol to the symbol table at [kw].
    Symbol AddVartype(Predefined kw, std::string const &name, size_t size, bool is_integer_vartype = false);

    // Booleans that are based on a certain vartype type
    bool IsVTT(Symbol s, VartypeType vtt) const;
    // Booleans that are based on a certain vartype flag
    bool IsVTF(Symbol s, VartypeFlag flag) const;

    // A wrapper, only for usage in the debugger
    inline Symbol FindCString(char *name) { return Find(name); }

public:
    std::vector<SymbolTableEntry> entries;
    inline SymbolTableEntry &operator[](Symbol sym) { return entries.at(sym); };

    SymbolTable();

    // Don't reset _findCache: It isn't rebuilt automatically; Find() and FindOrAdd() will no longer work.
    inline void ResetCaches() const { _vartypesCache.clear(); };

    void SetStringStructSym(Symbol s);
    inline Symbol GetStringStructSym() const { return _stringStructSym; }
    inline Symbol GetStringStructPtrSym() const { return _stringStructPtrSym; }
    inline Symbol GetLastAllocated() const { return _lastAllocated; }

    // General
    inline bool IsInBounds(Symbol s) const { return s > 0 && static_cast<size_t>(s) < entries.size(); }
    bool IsInUse(Symbol s) const;

    bool IsIdentifier(Symbol s) const;

    // A constant that stands for a literal value. Don't confuse with ConstVartype
    inline bool IsConstant(Symbol s) const { return nullptr != entries.at(s).ConstantD; }
    inline void MakeEntryConstant(Symbol s) { if (!entries.at(s).ConstantD) entries.at(s).ConstantD = new SymbolTableEntry::ConstantDesc; }
    inline bool IsDelimeter(Symbol s) const { return nullptr != entries.at(s).DelimeterD; }
    inline void MakeEntryDelimeter(Symbol s) { if (!entries.at(s).DelimeterD) entries.at(s).DelimeterD = new SymbolTableEntry::DelimeterDesc; }
    inline bool IsFunction(Symbol s) const { return nullptr != entries.at(s).FunctionD; }
    inline void MakeEntryFunction(Symbol s) { if (!entries.at(s).FunctionD) entries.at(s).FunctionD = new SymbolTableEntry::FunctionDesc; }
    inline bool IsLiteral(Symbol s) const { return nullptr != entries.at(s).LiteralD; }
    inline void MakeEntryLiteral(Symbol s) { if (!entries.at(s).LiteralD) entries.at(s).LiteralD = new SymbolTableEntry::LiteralDesc; }
    inline bool IsOperator(Symbol s) const { return nullptr != entries.at(s).OperatorD; }
    inline void MakeEntryOperator(Symbol s) { if (!entries.at(s).OperatorD) entries.at(s).OperatorD = new SymbolTableEntry::OperatorDesc; }
    inline bool IsComponent(Symbol s) const { return nullptr != entries.at(s).ComponentD; }
    inline void MakeEntryComponent(Symbol s) { if (!entries.at(s).ComponentD) entries.at(s).ComponentD = new SymbolTableEntry::ComponentDesc; }
    inline bool IsVariable(Symbol s) const { return nullptr != entries.at(s).VariableD; }
    inline void MakeEntryVariable(Symbol s) { if (!entries.at(s).VariableD) entries.at(s).VariableD = new SymbolTableEntry::VariableDesc; }
    inline bool IsVartype(Symbol s) const { return nullptr != entries.at(s).VartypeD; }
    inline void MakeEntryVartype(Symbol s) { if (!entries.at(s).VartypeD) entries.at(s).VartypeD = new SymbolTableEntry::VartypeDesc; }

    inline bool IsPredefined(Symbol s) const { return s <= kKW_LastPredefined; }
    
    // The name to the symbol. Will also print vartype designations, e.g. ArrayFoo[5]
    std::string const SymbolTable::GetName(Symbol symbl) const;

    // Whether the symbol can be part of an expression.
    // Note: Whatever is within delimeters will be skipped completely
    // so it can be part of an expression no matter what is determined here
    bool CanBePartOfAnExpression(Symbol s);

    // Variables or vartypes
    // Size of a variable or vartype
    size_t GetSize(Symbol s) const;

    inline bool IsAtomicVartype(Symbol s) const { return IsVTT(s, VTT::kAtomic); }
    inline bool IsBuiltinVartype(Symbol s) const { return IsVTF(s, VTF::kBuiltin); }
    // Don't confuse with IsConstant() == is a constant that signifies a literal
    inline bool IsConstVartype(Symbol s) const { return IsVTT(s, VTT::kConst); }

    // int, long, char, an enum etc.
    bool IsAnyIntegerVartype(Symbol s) const;
    // A predefined atomic vartype such as int and float.
    bool IsPrimitiveVartype(Symbol s) const;

    // Structs
    inline bool IsStructVartype(Symbol s) const { return IsVTF(s, VTF::kStruct); }
    inline bool IsAutoptrVartype(Symbol s) const { return IsVTF(s, VTF::kAutoptr); }
    // Fills compo_list with the symbols of all the strct components. Includes the ancestors' components
    void GetComponentsOfStruct(Symbol strct, std::vector<Symbol> &compo_list) const;
    // Find the description of a component.
    // Return nullptr if not found. Otherwise, caller must 'delete' the result after being done with it
    // Start search with the components of ancestor.
    Symbol FindStructComponent(Symbol strct, Symbol component, Symbol ancestor) const;
    inline Symbol *FindStructComponent(Symbol strct, Symbol component) const { FindStructComponent(strct, component, strct); }

    // Arrays and variables that are arrays
    // The "Array[...] of vartype" vartype
    Vartype VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype);
    // The "Dynarray / Dynpointer/ Const ... of vartype" vartype
    Vartype VartypeWith(VartypeType vtt, Vartype vartype);
    // The vartype without the qualifier given in vtt
    Vartype VartypeWithout(VartypeType vtt, Vartype vartype) const;

    inline bool IsArrayVartype(Symbol s) const { return IsVTT(s, VTT::kArray); }
    size_t NumArrayElements(Symbol s) const;
    inline bool IsDynarrayVartype(Symbol s) const { return IsVTT(s, VTT::kDynarray); }
    inline bool IsAnyArrayVartype(Symbol s) const { return IsArrayVartype(s) || IsDynarrayVartype(s); }
    
    // Dyn and variables that are dyn
    inline bool IsDynpointerVartype(Symbol s) const { return IsVTT(s, VTT::kDynpointer); }
    inline bool IsDynVartype(Symbol s) const { return IsDynarrayVartype(s) || IsDynpointerVartype(s); }
    // true if the underlying base struct is managed
    bool IsManagedVartype(Symbol s) const;

    // Functions
    inline size_t NumOfFuncParams(Symbol func) const
        { return IsFunction(func) ? entries.at(func).FunctionD->Parameters.size() - 1 : 0; }
    inline bool IsVariadicFunc(Symbol func) const
        { return IsFunction(func) && entries.at(func).FunctionD->IsVariadic; }
    inline AGS::Vartype FuncReturnVartype(Symbol func) const
        { return IsFunction(func) ? entries.at(func).FunctionD->Parameters[0u].Vartype : kKW_NoSymbol; }

    // Variables
    inline bool IsImport(Symbol s) const
        { return IsVariable(s) ? entries.at(s).VariableD->TypeQualifiers[TQ::kImport] : false; }
    inline bool IsParameter(Symbol s) const { return kParameterScope == entries.at(s).Scope; };
    // The vartype of the variable, i.e. "int" or "Dynarray *"
    inline AGS::Vartype GetVartype(Symbol s) const
        { return IsVariable(s) ? entries.at(s).VariableD->Vartype : kKW_NoSymbol; }
    inline bool IsAttribute(Symbol s) const
        { return IsVariable(s) && entries.at(s).VariableD->TypeQualifiers[TQ::kAttribute]; }
    ScopeType GetScopeType(Symbol s) const;

    // Operators
    inline int BinaryOpPrio(Symbol s) const
        { return IsOperator(s) ? entries.at(s).OperatorD->BinaryPrio : 0; }
    inline int UnaryOpPrio(Symbol s) const
        { return IsOperator(s) ? entries.at(s).OperatorD->UnaryPrio : 0; }
    inline CodeCell OperatorOpcode(Symbol s) const
        { return IsOperator(s) ? entries.at(s).OperatorD->Opcode : 0; }

    // Strings
    bool IsAnyStringVartype(Symbol s) const;
    bool IsOldstring(Symbol s) const;

    // add the name to the symbol table
    Symbol Add(std::string const &name);
    
    // Return the symbol to the name, or kKW_NoSymbol if not found
    inline Symbol Find(std::string const &name) { auto it = _findCache.find(name); return (_findCache.end() == it) ? kKW_NoSymbol : it->second; }

    // Add to the symbol table if not present already; in any case return the symbol
    inline Symbol FindOrAdd(std::string const &name) { Symbol ret = Find(name); return (kKW_NoSymbol != ret ) ? ret : Add(name); }

    // Set/get the position in the source where the item is declared
    inline void SetDeclared(int idx, size_t declared) { entries[idx].Declared = declared; }
    inline int GetDeclared(int idx) { return entries[idx].Declared; };
    };
} // namespace AGS
#endif //__CC_SYMBOLTABLE_H

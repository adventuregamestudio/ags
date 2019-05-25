#ifndef __CS_PARSER_COMMON_H
#define __CS_PARSER_COMMON_H
#include <cstdint>

namespace AGS
{
typedef int Symbol; // A symbol (result of scanner preprocessing)
typedef Symbol *SymbolScript; // A buffer of symbols 
typedef long Flags; // Collection of bits that are set and reset TODO convert to std::bitset
typedef long Vartype; // e.g., "int"
typedef int Exporttype; // e.g., EXPORT_FUNCTION
typedef short SType; // e.g. kSYM_Global
typedef std::int32_t CodeCell; // A Bytecode cell (content) or an opcode
typedef std::int32_t CodeLoc; // An offset to code[0], may be negative
typedef int32_t StringsLoc; // An offset into the strings repository
typedef int32_t GlobalLoc; // An offset into the global space
typedef char FixupType; // the type of a fixup
} // namespace AGS

constexpr char const *NEW_SCRIPT_TOKEN_PREFIX = "\"__NEWSCRIPTSTART_";
constexpr size_t OLDSTRING_LENGTH = 200;   // how big to make oldstyle string buffers

constexpr size_t MAX_FUNCTION_PARAMETERS = 15;
constexpr size_t VARARGS_INDICATOR = 100;

enum SymbolType : AGS::SType
{
    kSYM_NoType,

    // Types below can appear in expressions
    kSYM_CloseBracket,
    kSYM_CloseParenthesis,
    kSYM_Constant,
    kSYM_Dot,
    kSYM_Function,
    kSYM_GlobalVar,
    kSYM_LiteralFloat,
    kSYM_LiteralInt,
    kSYM_LiteralString,
    kSYM_LocalVar,
    kSYM_Null,
    kSYM_OpenBracket,
    kSYM_OpenParenthesis,
    kSYM_Operator,
    kSYM_StructComponent,

    // Types below cannot appear in expressions
    kSYM_Assign,
    kSYM_AssignMod,         // Modifying assign, e.g. "+="
    kSYM_AssignSOp,         // single-op assignemnt, eg "++", "--"
    kSYM_Attribute,         // struct member as attribute
    kSYM_AutoPtr,           // automatic pointer
    kSYM_Break,
    kSYM_Builtin,           // can't be instantiated directly by the user
    kSYM_Case,
    kSYM_CloseBrace,
    kSYM_Comma,
    kSYM_Const,  
    kSYM_Continue,
    kSYM_Default,
    kSYM_Do,
    kSYM_Else,
    kSYM_Enum,
    kSYM_Export,
    kSYM_Extends,           // inheritance
    kSYM_For,
    kSYM_If,
    kSYM_Import,
    kSYM_InternalString,    // special string struct
    kSYM_Label,             // : appearing at the end of a label
    kSYM_NoLoopCheck,       // disable loop count checking
    kSYM_Managed,           // struct allocated on heap
    kSYM_MemberAccess,      // ::
    kSYM_New,  
    kSYM_OpenBrace,
    kSYM_Protected,  
    kSYM_ReadOnly,
    kSYM_Return,
    kSYM_Semicolon,
    kSYM_Static,            
    kSYM_Struct,
    kSYM_Switch,
    kSYM_UndefinedStruct,   // forward-declared struct
    kSYM_Varargs,
    kSYM_Vartype,
    kSYM_While,
    kSYM_WriteProtected,    // write-protected member
};
constexpr AGS::SType NOTEXPRESSION = kSYM_Assign; // STypes starting (numerically) with this aren't part of expressions

enum SymbolTableFlag : AGS::Flags
{
    kSFLG_Accessed     = 1 <<  0, // if not set, the variable is never used
    kSFLG_Attribute    = 1 <<  1, // is an attribute variable
    kSFLG_Autoptr      = 1 <<  2, // automatically convert definition to pointer
    kSFLG_Builtin      = 1 <<  3, // direct instantiation/extension not allowed
    // The flag below only exists because the variable path parser cannot yet handle
    // arrays within arrays (e.g.something[2].something[3].something = 17) 
    kSFLG_HasDynArray  = 1 <<  4, 
    kSFLG_Imported     = 1 <<  5, // this is an import variable
    kSFLG_Managed      = 1 <<  6, // managed struct (kSYM_Vartype)
    kSFLG_Parameter    = 1 <<  7,
    kSFLG_Protected    = 1 <<  8, // protected member func/var
    kSFLG_Readonly     = 1 <<  9, // user cannot change
    kSFLG_Static       = 1 << 10, // static member func/var
    kSFLG_StrBuffer    = 1 << 11, // was allocated a string buffer
    kSFLG_StructMember = 1 << 12, // set for member vars & funcs
    kSFLG_StructType   = 1 << 13, // is a struct type (type will be kSYM_Vartype)
    kSFLG_WriteProtected = 1 << 14,  // only the this pointer can write the var
};

constexpr size_t SIZE_OF_DYNPOINTER = 4;
constexpr size_t SIZE_OF_INT = 4;

extern int is_whitespace(char cht);
extern int is_digit(int chrac);
extern int is_alphanum(int chrac);

#endif // __CS_PARSER_COMMON_H

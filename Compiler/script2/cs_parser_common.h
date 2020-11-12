#ifndef __CS_PARSER_COMMON_H
#define __CS_PARSER_COMMON_H
#include <cstdint>
#include <cstddef>  // size_t
#include <string>
#include <vector>

namespace AGS
{
typedef int Symbol; // A symbol (result of scanner preprocessing)
typedef Symbol *SymbolScript; // A buffer of symbols 
typedef long FlagSet; // Collection of bits that are set and reset
typedef long Vartype; // e.g., "int"
typedef int Exporttype; // e.g., EXPORT_FUNCTION
typedef short SymbolTypeType;
typedef int32_t CodeCell; // A Bytecode cell (content) or an opcode
typedef int32_t CodeLoc; // An offset to code[0], may be negative
typedef int32_t StringsLoc; // An offset into the strings repository
typedef int32_t GlobalLoc; // An offset into the global space
typedef char FixupType; // the type of a fixup
typedef FlagSet TypeQualifierSet;

constexpr size_t STRINGBUFFER_LENGTH = 200;   // how big to make string buffers

constexpr size_t SIZE_OF_CHAR = 1;
constexpr size_t SIZE_OF_DYNPOINTER = 4;
constexpr size_t SIZE_OF_FLOAT = 4;
constexpr size_t SIZE_OF_INT = 4;
constexpr size_t SIZE_OF_LONG = 4;
constexpr size_t SIZE_OF_SHORT = 2;
constexpr size_t SIZE_OF_STACK_CELL = 4;
constexpr size_t STRUCT_ALIGNTO = 4;

constexpr size_t MAX_FUNCTION_PARAMETERS = 15;

inline static bool FlagIsSet(AGS::FlagSet fl_set, long flag) { return 0 != (fl_set & flag); }
inline static void SetFlag(AGS::FlagSet &fl_set, long flag, bool val) { if (val) fl_set |= flag; else fl_set &= ~flag; }

enum SymbolType : SymbolTypeType
{
    kSYM_NoType = 0,

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
    kSYM_Tern,              // the '?' in the a? b : c construct

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
    kSYM_Label,             // : at end of label, also used in ternary
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

// Types starting (numerically) with this aren't part of expressions
constexpr SymbolTypeType NOTEXPRESSION = kSYM_Assign; 

enum TypeQualifier
{
    kTQ_None = 0,
    kTQ_Attribute = 1 << 0,
    kTQ_Autoptr = 1 << 1,
    kTQ_Builtin = 1 << 2,
    kTQ_Const = 1 << 3,
    kTQ_ImportStd = 1 << 4,
    kTQ_ImportTry = 1 << 5,
    kTQ_Managed = 1 << 6,
    kTQ_Protected = 1 << 7,
    kTQ_Readonly = 1 << 8,
    kTQ_Static = 1 << 9,
    kTQ_Stringstruct = 1 << 10,
    kTQ_Writeprotected = 1 << 11,
    kTQ_Import = kTQ_ImportStd | kTQ_ImportTry,
};

enum SymbolTableFlag : FlagSet
{
    kSFLG_Accessed = 1 << 0, // If not set, the variable is never used
    kSFLG_NoLoopCheck = 1 << 1, // A function that does not check for long-running loops
    kSFLG_StructAutoPtr = 1 << 2, // "*" is implied
    kSFLG_StructBuiltin = 1 << 3, // is built in (can't use "new")
    kSFLG_StructMember = 1 << 4, // is a member 
    kSFLG_StructManaged = 1 << 5, // is managed
    kSFLG_StructVartype = 1 << 6, // is a struct 
};

// In what type of memory the variable is allocated
enum ScopeType
{
    kScT_None = 0,
    kScT_Global,
    kScT_Import,
    kScT_Local,
    kScT_Strings,
};

enum ErrorType
{
    kERR_None = 0,
    kERR_UserError = -1,
    kERR_InternalError = -99,
};

class MessageHandler
{
public:
    enum Severity
    {
        kSV_None,
        kSV_Info,
        kSV_Warning,
        kSV_Error,
    };

    struct Entry
    {
        Severity Severity;
        std::string Section;
        size_t Lineno;
        std::string Message;

        Entry(enum Severity sev, std::string const &section, size_t lineno, std::string const msg);
    };

    typedef std::vector<Entry> MessagesType;

private:
    MessagesType _entries;
    static Entry _noError;

public:
    inline void AddMessage(Severity sev, std::string const &sec, size_t line, std::string const &msg)
        {_entries.emplace_back(sev, sec, line, msg); }
    inline MessagesType GetMessages() const { return _entries; }
    inline void Clear() { _entries.clear(); }
    Entry const &GetError() const;
};

} // namespace AGS
#endif // __CS_PARSER_COMMON_H

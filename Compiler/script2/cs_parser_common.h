#ifndef __CS_PARSER_COMMON_H
#define __CS_PARSER_COMMON_H
#include <cstdint>
#include <cstddef>  // size_t
#include <string>
#include <vector>

namespace AGS
{
typedef int Symbol; // A symbol (result of scanner preprocessing)
typedef std::vector<Symbol> SymbolList;
typedef long FlagSet; // Collection of bits that are set and reset
typedef int Vartype; // e.g., "int"
typedef int Exporttype; // e.g., EXPORT_FUNCTION
typedef int32_t CodeCell; // A Bytecode cell (content) or an opcode
typedef int32_t CodeLoc; // An offset to code[0], may be negative
typedef int32_t StringsLoc; // An offset into the strings repository
typedef int32_t GlobalLoc; // An offset into the global space
typedef char FixupType; // the type of a fixup

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
        Severity Severity = kSV_Error;
        std::string Section = "";
        size_t Lineno = 0u;
        std::string Message = "";

        Entry() = default;
        Entry(enum Severity sev, std::string const &section, size_t lineno, std::string const &msg);
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

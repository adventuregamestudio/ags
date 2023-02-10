#ifndef __CS_PARSER_COMMON_H
#define __CS_PARSER_COMMON_H
#include <cstdint>
#include <cstddef>  // size_t
#include <string>
#include <vector>

#include "cs_message_handler.h"

namespace AGS
{
typedef int Symbol; // A symbol (result of scanner preprocessing)
typedef std::vector<Symbol> SymbolList;
typedef std::vector<size_t> RegisterList;
typedef uint64_t FlagSet; // Collection of bits that are set and reset
typedef int Vartype; // e.g., "int"
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

inline static bool FlagIsSet(AGS::FlagSet fl_set, uint64_t flag) { return 0 != (fl_set & flag); }
inline static void SetFlag(AGS::FlagSet &fl_set, uint64_t flag, bool val) { if (val) fl_set |= flag; else fl_set &= ~flag; }

enum ErrorType
{
    kERR_None = 0,
    kERR_UserError = -1,
    kERR_InternalError = -99,
};

} // namespace AGS
#endif // __CS_PARSER_COMMON_H

#ifndef __CS_PARSER_COMMON_H
#define __CS_PARSER_COMMON_H
#include <cstdint>

#define NEW_SCRIPT_TOKEN_PREFIX "\"__NEWSCRIPTSTART_"
#define STRING_LENGTH 200   // how big to make strings
#define MAX_NESTED_LEVEL 75
// [fw] NOTE: NEST_ macros are converted into an enum in cs_parser.h

#define MAX_FUNCTIONS 2000
#define MAX_FUNCTION_PARAMETERS 15
// This is the maximum length of a "static string" in the script
#define MAX_SYM_LEN 500
#define SYM_GLOBALVAR 1
#define SYM_LOCALVAR  2
#define SYM_OPERATOR  3
#define SYM_FUNCTION  4
#define SYM_OPENPARENTHESIS  5
#define SYM_CLOSEPARENTHESIS 6
#define SYM_OPENBRACKET   7
#define SYM_CLOSEBRACKET  8
#define SYM_DOT           9
#define SYM_STRUCTMEMBER 10   // variable names of struct members
#define SYM_STRING       18
#define SYM_LITERALVALUE 19   // it's just a number
#define SYM_CONSTANT     20   // constant variable (eg. literal "1")
#define SYM_NULL         21   // const variable
#define SYM_LITERALFLOAT 22   // it's just a floating point number
// types below are not parts of expressions
#define NOTEXPRESSION  23
#define SYM_OPENBRACE  23
#define SYM_CLOSEBRACE 24
#define SYM_ASSIGN     25
#define SYM_MASSIGN    26
#define SYM_VARARGS    27
#define SYM_IF         28
#define SYM_ELSE       29
#define SYM_STRUCT     30
#define SYM_WHILE      31
#define SYM_IMPORT     32
#define SYM_EXPORT     33
#define SYM_SASSIGN    34  // single-op assignemnt, eg "++", "--"
#define SYM_RETURN     35
#define SYM_READONLY   36
#define SYM_MEMBERACCESS 37  // ::
#define SYM_PROPERTY   38  // deprecated
#define SYM_ATTRIBUTE  38  // struct member as attribute
#define SYM_ENUM       39
#define SYM_SEMICOLON  40
#define SYM_MANAGED    41  // "managed" struct allocated on heap
#define SYM_COMMA      42
#define SYM_EXTENDS    43  // inheritance
#define SYM_STATIC     44  // static function
#define SYM_PROTECTED  45  // protected member
#define SYM_VARTYPE    46
#define SYM_UNDEFINEDSTRUCT 47  // forward-declared struct
#define SYM_WRITEPROTECTED  48  // write-protected member
#define SYM_CONST      49  // "const" keyword
#define SYM_STRINGSTRUCT 50  // special string struct
#define SYM_AUTOPTR    51  // automatic pointer
#define SYM_LOOPCHECKOFF 52  // disable loop count checking
#define SYM_NEW        53  // "new" keyword
#define SYM_FOR        54
#define SYM_BREAK      55
#define SYM_CONTINUE   56
#define SYM_DO         57
#define SYM_BUILTIN    58 // Used to indicate that a managed object can't be instantiated directly by the user
#define SYM_SWITCH     59
#define SYM_CASE       60
#define SYM_DEFAULT    61
#define SYM_LABEL      62 // : appearing at the end of a label

#define SFLG_PARAMETER  1
#define SFLG_ARRAY      2
#define SFLG_IMPORTED   4   // this is an import variable
#define SFLG_ACCESSED   8   // if not set, the variable is never used
#define SFLG_STRBUFFER    0x10  // was allocated a string buffer
#define SFLG_ISSTRING     0x20  // is a pointer
#define SFLG_READONLY     0x40  // user cannot change
#define SFLG_STRUCTMEMBER 0x80  // set for member vars & funcs
#define SFLG_POINTER     0x100  // pointer to object
#define SFLG_ATTRIBUTE   0x200  // is an attribute variable
#define SFLG_PROPERTY    0x200  // deprecated -- is an attribute variable
#define SFLG_STRUCTTYPE  0x400  // is a struct type (type will be SYM_VARTYPE)
#define SFLG_THISPTR     0x800  // is the "this" pointer
#define SFLG_MANAGED    0x1000  // managed struct (SYM_VARTYPE)
#define SFLG_STATIC     0x2000  // static member func/var
#define SFLG_PROTECTED  0x4000  // protected member func/var
#define SFLG_WRITEPROTECTED 0x8000  // only the this pointer can write the var
#define SFLG_CONST     0x10000  // const variable
#define SFLG_AUTOPTR   0x20000  // automatically convert definition to pointer
#define SFLG_DYNAMICARRAY 0x40000  // array allocated at runtime
#define SFLG_BUILTIN   0x80000  // direct instantiation/extension not allowed
/*
   The flag below is only present because the variable path parser
   (e.g. something[2].something[3].something = 17) cannot yet handle
   arrays within arrays
*/
#define SFLG_HASDYNAMICARRAY  0x100000
#define TEMP_SYMLIST_LENGTH 100

extern int is_whitespace(char cht);
extern void skip_whitespace(char **pttt);
extern int is_digit(int chrac);
extern int is_alphanum(int chrac);

namespace AGS
{
// A symbol (result of scanner preprocessing)
typedef int Symbol;

// A buffer of symbols 
typedef Symbol *SymbolScript;

// A code cell (content)
typedef std::intptr_t CodeCell;

// A code location, may be negative
typedef std::intptr_t CodeLoc;
} // namespace AGS

#endif // __CS_PARSER_COMMON_H

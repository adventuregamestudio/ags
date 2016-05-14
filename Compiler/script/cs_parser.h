/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones

SCOM is a script compiler for the 'C' language. The current version
implements:
* #define macros, definition of and use of
* "//" and "/*---* /" comments
* global and local variables; calling functions; assignments
* most of the standard 'C' operators
* structures and arrays
* import and export of variables and functions from parent program
* strings get allocated 200 bytes of storage automatically

It currently does NOT do:
* #define with parenthesis, eg. #define func(a) bar(a+3)
* typedefs
* optimize code generated - it could check if MAR already contains location
to read, for example

Some interesting points about how this works:
* while loops are stored internally as "else"-blocks, but with an extra
bit of data storing the start of the while test condition to go back to
* array index accesses are generated as code to allow anything inside
the brackets, whereas structure member accesses are hardcoded into the
offset in the code since the member has a fixed offset from the structure
start

*/

//-----------------------------------------------------------------------------
//  Should be used only internally by cs_compiler.cpp
//-----------------------------------------------------------------------------

#ifndef __CS_PARSER_H
#define __CS_PARSER_H

#include "cc_compiledscript.h"
#include <vector>

extern int cc_compile(const char*inpl, ccCompiledScript*scrip);

// A section of compiled code that needs to be moved or copied to a new location
struct ccChunk {
    std::vector<intptr_t> code;
    std::vector<int32_t> fixups;
    std::vector<char> fixuptypes;
    int codeoffset;
    int fixupoffset;
};

#endif // __CS_PARSER_H

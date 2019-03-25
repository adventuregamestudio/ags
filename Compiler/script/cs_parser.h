/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - INTERFACE AND HIGH-LEVEL STRUCTURE

The processing is done in the following layers:
* [Preprocessing - This has been done separately before the input arrives here.]
    Expand macros, delete comments

* Scanning
    Read the characters of the input and partition it in symbols (e.g., identifier, number literal).
* Tokenizing
    Enter all the symbols into a symbol table (thus recognizing keywords)
    Enter all literal strings into a strings table
    Recognize structs and prepend "." to struct component names
These two steps are piped. They are performed separately _before_ the Parsing (below) begins.
The result is:
    the symbol table, a global sym that is a struct symbolTable.
    the sequence of tokens, a parameter targ that is a struct ccInternalList *.
    the collected string literals that go into a struct ccCompiledScript.

* Parsing
    All the high-level logic.
The parsing functions get the input in a parameter targ that is a ccInternalList *.
The parser augments the symbol table sym as it goes along.
The result is in a parameter scrip that is a struct ccCompiledScript *
and has the following key components (amongst many other components):
    functions - all the functions defined (i.e. they have a body) in the current inputstring
    imports - all the functions declared (i.e. without body) in the current inputstring
        NOTE: This includes "forward" declarations where a func definition will arrive later after all.
    exports - all the functions that are made available to other entities
    code, fixups - the Bytecode that is generated.

(For an overview of the implementation details, see cs_parser.cpp)
*/

//-----------------------------------------------------------------------------
//  Should be used only internally by cs_compiler.cpp
//-----------------------------------------------------------------------------

#ifndef __CS_PARSER_H
#define __CS_PARSER_H

#include <vector>
#include <deque>
#include <string>
#include <sstream>

#include "cc_compiledscript.h"
#include "cc_internallist.h"
#include "cc_symboltable.h"
#include "cs_scanner.h"
#include "cs_tokenizer.h"


namespace AGS
{

// The stack of nesting statements 
class NestingStack
{
public:
    // A section of compiled code that needs to be moved or copied to a new location
    struct Chunk
    {
        std::vector<AGS::CodeCell> _code;
        std::vector<int32_t> _fixups;
        std::vector<char> _fixupTypes;
        int _codeOffset;
        int _fixupOffset;
    };

    // All data that is associated with a nesting level
    struct NestingInfo
    {
        int Type; // Type of the level, see AGS::NestingStack::NestingType
        AGS::CodeLoc _startLoc; // Index of the first byte generated for the level
        AGS::CodeLoc _info; // Various uses that differ by nesting type
        AGS::CodeLoc _defaultLabelLoc; // Location of default label
        std::vector<Chunk> _chunks; // Bytecode chunks that must be moved (FOR loops and SWITCH)
    };
    enum NestingType
    {
        kNT_Nothing = 0,  // {...} in the code without a particular purpose
        kNT_Function,     // A function
        kNT_BracedThen,   // THEN clause with braces
        kNT_UnbracedThen, // THEN clause without braces (i.e, it's a single simple statement)
        kNT_BracedElse,   // ELSE/inner FOR/WHILE clause with braces
        kNT_UnbracedElse, // ELSE/inner FOR/WHILE clause without braces
        kNT_BracedDo,     // DO clause with braces
        kNT_UnbracedDo,   // DO clause without braces 
        kNT_For,          // Outer FOR clause
        kNT_Switch,       // SWITCH clause
        kNT_Struct,       // Struct defn
    };
private:
    std::vector<NestingInfo> _stack;

public:
    NestingStack();

    // Depth of the nesting == index of the innermost nesting level
    inline size_t Depth() { return _stack.size(); };

    // Type of the innermost nesting
    inline NestingType Type() { return static_cast<NestingType>(_stack.back().Type); };
    inline void SetType(NestingType nt) { _stack.back().Type = nt; };
    // Type of the nesting at the given nesting level
    inline NestingType Type(size_t level) { return static_cast<NestingType>(_stack.at(level).Type); };

    // If the innermost nesting is a loop that has a jump back to the start,
    // then this gives the location to jump to; otherwise, it is 0
    inline std::int32_t _startLoc() { return _stack.back()._startLoc; };
    inline void SetStartLoc(std::int32_t start) { _stack.back()._startLoc = start; };
    // If the nesting at the given level has a jump back to the start,
    // then this gives the location to jump to; otherwise, it is 0
    inline std::int32_t _startLoc(size_t level) { return _stack.at(level)._startLoc; };

    // If the innermost nesting features a jump out instruction, then this is the location of it
    inline std::intptr_t JumpOutLoc() { return _stack.back()._info; };
    inline void SetJumpOutLoc(std::intptr_t loc) { _stack.back()._info = loc; };
    // If the nesting at the given level features a jump out, then this is the location of it
    inline std::intptr_t JumpOutLoc(size_t level) { return _stack.at(level)._info; };

    // If the innermost nesting is a SWITCH, the type of the switch expression
    int SwitchExprType() { return static_cast<int>(_stack.back()._info); };
    inline void SetSwitchExprType(int ty) { _stack.back()._info = ty; };

    // If the innermost nesting is a SWITCH, the location of the "default:" label
    inline std::int32_t _defaultLabelLoc() { return _stack.back()._defaultLabelLoc; };
    inline void SetDefaultLabelLoc(int32_t loc) { _stack.back()._defaultLabelLoc = loc; }

    // If the innermost nesting contains code chunks that must be moved around
    // (e.g., in FOR loops), then this is true, else false
    inline bool ChunksExist() { return !_stack.back()._chunks.empty(); }
    inline bool ChunksExist(size_t level) { return !_stack.at(level)._chunks.empty(); }

    // Code chunks that must be moved around (e.g., in FOR, DO loops)
    inline std::vector<Chunk> Chunks() { return _stack.back()._chunks; };
    inline std::vector<Chunk> Chunks(size_t level) { return _stack.at(level)._chunks; };

    // True iff the innermost nesting is unbraced
    inline bool IsUnbraced()
    {
        NestingType nt = Type();
        return (nt == kNT_UnbracedThen) || (nt == kNT_UnbracedElse) || (nt == kNT_UnbracedDo);
    }

    // Push a new nesting level (returns a  value < 0 on error)
    int Push(NestingType type, AGS::CodeLoc start, AGS::CodeLoc info);
    inline int Push(NestingType type) { return Push(type, 0, 0); };

    // Pop a nesting level
    inline void Pop() { _stack.pop_back(); };

    // Rip a generated chunk of code out of the codebase and stash it away for later 
    void YankChunk(::ccCompiledScript *scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset);

    // Write chunk of code back into the codebase that has been stashed in level given, at index
    void WriteChunk(::ccCompiledScript *scrip, size_t level, size_t index);
    // Write chunk of code back into the codebase stashed in the innermost level, at index
    inline void WriteChunk(::ccCompiledScript *scrip, size_t index) { WriteChunk(scrip, Depth() - 1, index); };


};

} // namespace AGS


extern int cc_tokenize(
    const char *inpl,         // preprocessed text to be tokenized
    ccInternalList *targ,     // store for the tokenized text
    ccCompiledScript *scrip); // store for the strings in the text

extern int cc_compile(
    const char *inpl,           // preprocessed text to be compiled
    ccCompiledScript *scrip);   // store for the compiled text

#endif // __CS_PARSER_H

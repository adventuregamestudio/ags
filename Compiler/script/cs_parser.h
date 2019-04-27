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
    the symbol table, a global sym that is a struct SymbolTable.
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
#include <string>
#include <map>

#include "cc_compiledscript.h"
#include "cc_internallist.h"
#include "cc_symboltable.h"


namespace AGS
{

// The stack of nesting statements 
class NestingStack
{
private:
    static int _chunkIdCtr; // for assigning unique IDs to chunks

    // A section of compiled code that needs to be moved or copied to a new location
    struct Chunk
    {
        std::vector<AGS::CodeCell> Code;
        std::vector<AGS::CodeLoc> Fixups;
        std::vector<char> FixupTypes;
        int CodeOffset;
        int FixupOffset;
        int Id;
    };

    // All data that is associated with a nesting level
    struct NestingInfo
    {
        int Type; // Type of the level, see AGS::NestingStack::NestingType
        AGS::CodeLoc StartLoc; // Index of the first byte generated for the level
        AGS::CodeLoc Info; // Various uses that differ by nesting type
        AGS::CodeLoc DefaultLabelLoc; // Location of default label
        std::vector<Chunk> Chunks; // Bytecode chunks that must be moved (FOR loops and SWITCH)
    };

    std::vector<NestingInfo> _stack;

public:
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
    inline AGS::CodeLoc StartLoc() { return _stack.back().StartLoc; };
    inline void SetStartLoc(AGS::CodeLoc start) { _stack.back().StartLoc = start; };
    // If the nesting at the given level has a jump back to the start,
    // then this gives the location to jump to; otherwise, it is 0
    inline AGS::CodeLoc StartLoc(size_t level) { return _stack.at(level).StartLoc; };

    // If the innermost nesting features a jump out instruction, 
    // then this is the location of the bytecode symbol that says where to jump
    inline std::intptr_t JumpOutLoc() { return _stack.back().Info; };
    inline void SetJumpOutLoc(std::intptr_t loc) { _stack.back().Info = loc; };
    // If the nesting at the given level features a jump out, then this is the location of it
    inline AGS::CodeLoc JumpOutLoc(size_t level) { return _stack.at(level).Info; };

    // If the innermost nesting is a SWITCH, the type of the switch expression
    int SwitchExprType() { return static_cast<int>(_stack.back().Info); };
    inline void SetSwitchExprType(int ty) { _stack.back().Info = ty; };

    // If the innermost nesting is a SWITCH, the location of the "default:" label
    inline AGS::CodeLoc DefaultLabelLoc() { return _stack.back().DefaultLabelLoc; };
    inline void SetDefaultLabelLoc(AGS::CodeLoc loc) { _stack.back().DefaultLabelLoc = loc; }

    // If the innermost nesting contains code chunks that must be moved around
    // (e.g., in FOR loops), then this is true, else false
    inline bool ChunksExist() { return !_stack.back().Chunks.empty(); }
    inline bool ChunksExist(size_t level) { return !_stack.at(level).Chunks.empty(); }

    // Code chunks that must be moved around (e.g., in FOR, DO loops)
    inline std::vector<Chunk> Chunks() { return _stack.back().Chunks; };
    inline std::vector<Chunk> Chunks(size_t level) { return _stack.at(level).Chunks; };

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
    // Returns the unique ID of this code in id
    void YankChunk(::ccCompiledScript *scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset, int &id);

    // Write chunk of code back into the codebase that has been stashed in level given, at index
    void WriteChunk(::ccCompiledScript *scrip, size_t level, size_t index, int &id);
    // Write chunk of code back into the codebase stashed in the innermost level, at index
    inline void WriteChunk(::ccCompiledScript *scrip, size_t index, int &id) { WriteChunk(scrip, Depth() - 1, index, id); };
};

class FuncCallpointMgr
{
private:
    int const CodeBaseId = 0;  // Magic number, means: This is in codebase, not in a yanked piece of code
    int const PatchedId = -1;  // Magic number, means: This is in codebase and has already been patched in

    struct PatchInfo
    {
        int ChunkId;
        AGS::CodeLoc Offset;
    };
    typedef std::vector<PatchInfo> PatchList;

    struct FuncInfo
    {
        CodeLoc Callpoint;
        PatchList List;
        FuncInfo();
    };

    typedef std::map<Symbol, FuncInfo> CallMap;
    CallMap _funcCallpointMap;


public:
    int Init();
    
    // Enter a code location where a function is called that hasn't been defined yet.
    int TrackForwardDeclFuncCall(::ccCompiledScript *scrip, Symbol func, CodeLoc idx);

    // When code is ripped out of the codebase: 
    // Update list of calls to forward declared functions 
    int UpdateCallListOnYanking(CodeLoc start, size_t len, int id);

    // When code is inserted into the codebase:
    // Update list of calls to forward declared functions
    int UpdateCallListOnWriting(CodeLoc start, int id);

    // Set the callpoint for a function. 
    // Patch all the function calls of the given function to point to dest
    int SetFuncCallpoint(::ccCompiledScript *scrip, Symbol func, AGS::CodeLoc dest);

    inline int HasFuncCallpoint(Symbol func) { return (_funcCallpointMap[func].Callpoint >= 0); }

    inline bool IsForwardDecl(AGS::Symbol func) { return (0 == _funcCallpointMap.count(func)); }

    // Gives an error message and returns a value < 0 iff there are still functions
    // without a location
    int CheckForUnresolvedFuncs();
};

typedef long TypeQualifierSet;

class ImportMgr
{
private:
    std::map<std::string, size_t> _importIdx;
    ccCompiledScript *_scrip;

public:
    void Init(ccCompiledScript *scrip);
    int FindOrAdd(std::string s);
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

int ParseFor_InitClauseVardecl(AGS::Symbol & cursym, ccInternalList * targ, const size_t &nested_level, ccCompiledScript * scrip);

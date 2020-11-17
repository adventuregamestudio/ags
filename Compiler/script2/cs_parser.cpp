/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - IMPLEMENTATION

General:
Functions have names of the form AaaAaa or AaaAaa_BbbBbb
where the component parts are camelcased. This means that function AaaAaa_BbbBbb is a
subfunction of function AaaAaa that is exclusively called by function AaaAaa.

The Parser does does NOT get the sequence of tokens in a pipe from the scanning step, i.e.,
it does NOT read the symbols one-by-one. To the contrary, the logic reads back and forth in
the token sequence.

(Nearly) All parser functions return an error code that is negative iff an error has been
encountered. In case of an error, they call Error() and return with a negative integer.

The Parser runs in two phases.
The first phase runs quickly through the tokenized source and collects the headers
of the local functions.

The second phase has the following main components:
    Declaration parsing
    Command parsing
        Functions that process the keyword Kkk are called ParseKkk()

    Code nesting and compound statements
        In ParseWhile() etc., DealWithEndOf..(), and class AGS::Parser::NestingStack.

    Expression parsing
        In ParseExpression()
        Note that "++" and "--" are treated as assignment symbols, not as operators.

    Memory access
        In AccessData()
        In order to read data or write to data, the respective piece of data must
        be located first. This also encompasses literals of the program code.
        Note that "." and "[]" are not treated as normal operators (operators like +).
        The memory offset of struct components in relation to the
        location of the respective struct is calculated at compile time, whereas array
        offsets are calculated at run time.

Notes on how nested statements are handled:
    When handling nested constructs, the parser sometimes generates and emits some code,
    then rips it out of the codebase and stores it internally, then later on, retrieves
    it and emits it into the codebase again.

Oldstyle strings, string literals, string buffers:
    If a "string" is declared, 200 bytes of memory are reserved on the stack (local) or in
    global memory (global). This is called a "string buffer". Whenever oldstyle strings or
    literal strings are used, they are referred to by the address of their first byte.
    The only way of modifying a string buffer is by functions. However, string buffer assignments
    are handled with inline code. The compiler doesn't attempt in any way to prevent buffer underruns or overruns.


MEMORY LAYOUT

Global variables go into their own dedicated memory block and are addressed relatively to the beginning of that block.
	This block is initialized with constant values at the start of the game. So it is possible to set the start value of
    globals to some constant value, but it is not possible to _calculate_ it at the start of the runtime.
    In particular, initial pointer values and initial String values can only be given as null because any
    other value would entail a runtime computation.

Literal strings go into their own, dedicated memory block and are also addressed relatively to the beginning of that block.
	The scanner populates this memory block; for the parser, the whole block is treated as constant and read-only.

Imported values are treated as if they were global values. However, their exact location is only computed at runtime by the
    linker. For the purposes of the parser, imported values are assigned an ordinal number #0, #1, #2 etc. and are referenced
    by their ordinal number.

Local variables go into a memory block, the "local memory block", that is reserved on the stack.
 	They are addressed relatively to the start of that block. The start of this block can always be determined at
 	compile time by subtracting a specific offset from the stack pointer, namely OffsetToLocalVarBlock.
 	This offset changes in the course of the compilation but can always be determined at compile time.
 	The space for local variables is reserved on the stack at runtime when the respective function is called.
    Therefore, local variables can be initialized to any value that can be computed,
    they aren't restricted to compile time constants as the global variables are.

A local variable is declared within a nesting of { ... } in the program code;
    It becomes valid at the point of declaration and it becomes invalid when the closing '}' to the
    innermost open '{' is encountered. In the course of reading the program from beginning to end,
    the open '{' that have not yet been closed form a stack called the "nesting stack".
    Whenever a '{' is encountered, the nesting stack gets an additional level; whenever a '}' is encountered,
    the topmost level is popped from the stack.
        Side Note: Compound statements can have a body that is NOT surrounded with braces, e.g.,
        "if (foo) i++;" instead of "if (foo) { i++; }". In this case the nesting stack is
        still extended by one level before the compound statement body is processed and
        reduced by one level afterwards.
  
    The depth of the stack plus 1 is called the nested depth or scope. Each local variable is assigned
 	the nested depth of its point of declaration.

    When program flow passes a closing '}' then all the variables with higher nested depth can be freed.
    This shortens the local memory block from the end; its start remains unchanged.
    "continue", "break" and "return" statements can break out of several '}' at once. In this case,
    all their respective variables must be freed.

    Class NestingStack keeps information on the nested level of code.
    For each nested level, the class keeps, amongst others, the location in the bytecode
    of the start of the construct and the location of a Bytecode jump to its end.

Parameters of a function are local variables; they are assigned the nested depth 1.
    Only parameters can have the nested depth 1.
    The first parameter of a function is also the first parameter in the local variable block. To make this happen,
    the parameters must be pushed back-to-front onto the stack when the function is called,
    i.e. the last function parameter must be pushed first.

    Global, imported variables, literal constants and strings are valid from the point of declaration onwards
    until the end of the compilation unit; their assigned nested depth is 0.

Whilst a function is running, its local variable block is as follows:
((lower memory addresses))
		parameter1					<- SP - OffsetToLocalVarBlock
		parameter2
		parameter3
		...
		(return address of the current function)
		variable21 with scope 2
		variable22 with scope 2
		variable23 with scope 2
		...
		variable31 with scope 3
		variable32 with scope 3
		...
		variable41 with scope 4
		...
		(pushed value1)
		(pushed value2)
		...
		(pushed valueN)				<- SP
((higher memory addresses))

Classic arrays and Dynarrays, pointers and managed structs:
    Memory that is allocated with "new" is allocated dynamically by the Engine. The compiler need not be concerned how it is done.
	The compiler also needn't concern itself with freeing the dynamic memory itself; this is the Engine's job, too.
    However, the compiler must declare that a memory cell shall hold a pointer to dynamic memory, by using the opcode MEMWRITEPTR.
    And when a memory cell is no longer reserved for pointers, this must be declared as well, using the opcode MEMZEROPTR.
		Side note: Before a function is called, all its parameters are "pushed" to the stack using the PUSHREG opcode.
		So when some parameters are pointers then the fact that the respective memory cells contain a pointer isn't declared yet.
		So first thing at the start of the function, all pointer parameters must be read with normal non-...PTR opcodes
        and then written into the same place as before using the special opcode MEMWRITEPTR.
		Side note: When a '}' is reached and local pointer variables are concerned, it isn't enough to just shorten the
		local memory block. On all such pointer variables, MEMZEROPTR must be applied first to declare that the respective memory
	    cells won't necessarily contain pointers any more.

    Any address that should hold a pointer must be manipulated using the SCMD_...PTR form of the opcodes

    There are only two kinds of dynamic memory blocks:
        memory blocks that do not contain any pointers to dynamic memory whatsoever
        memory blocks that completely consist of pointers to dynamic memory ONLY.
    (This is an engine restriction pertaining to memory serialization, not a compiler restriction.)

    A Dynarray of primitives (e.g., int[]) is represented in memory as a pointer to a memory
    block that comprises all the elements, one after the other.
    [*]->[][]...[]
    A Dynarray of structs must be a dynarray of managed structs. It is represented in
    memory as a pointer to a block of pointers, each of which points to one element.
    [*]->[*][*]...[*]
          |  |     |
          V  V ... V
         [] [] ... []
    In contrast to a dynamic array, a classic array is never managed.
    A classic array of primitives (e.g., int[12]) or of non-managed structs is represented
    in memory as a block of those elements.
    [][]...[]
    A classic array of managed structs is a classic array of pointers,
    each of which points to a memory block that contains one element.
    [*][*]...[*]
     |  |     |
     V  V ... V
    [] [] ... []

Pointers are exclusively used for managed memory. If managed structs are manipulated,
    pointers MUST ALWAYS be used; for un-managed structs, pointers MAY NEVER be used.
    However as an exception, in import statements non-pointed managed structs can be used, too.
    That means that the compiler can deduce whether a pointer is expected by
    looking at the keyword "managed" of the struct alone -- except in global import declarations.
    Blocks of primitive vartypes can be allocated as managed memory, in which case pointers
    MUST be used. Again, the compiler can deduce from the declaration that a pointer MUST be
    used in this case.
*/


#include <string>
#include <cstring>
#include <limits>
#include <fstream>
#include <cmath>

#include "util/string.h"

#include "script/cc_options.h"
#include "script/script_common.h"
#include "script/cc_error.h"

#include "cc_internallist.h"
#include "cc_symboltable.h"

#include "cs_parser_common.h"
#include "cs_scanner.h"
#include "cs_parser.h"


// Declared in Common/script/script_common.h 
// Defined in Common/script/script_common.cpp
extern int currentline;

char ccCopyright[] = "ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2020 others";

// Receives the section name in case of errors
static char SectionNameBuffer[256];

void AGS::Parser::AddToSymbolList(Symbol symb, SymbolList &list)
{
    size_t const ssize = list.size();
    for (size_t idx = 0; idx < ssize; ++idx)
        if (symb == list[ssize])
            return;
    list.push_back(symb);
}

std::string const AGS::Parser::TypeQualifierSet2String(TypeQualifierSet tqs) const
{
    std::string ret;
   
    for (auto tq_it = tqs.begin(); tqs.end() != tq_it; tq_it++)
        if (tqs[tq_it->first])
            ret += _sym.GetName(tq_it->second) + " ";
    if (ret.length() > 0)
        ret.pop_back(); // Get rid of the trailing space
    return ret;
}

AGS::Symbol AGS::Parser::MangleStructAndComponent(Symbol stname, Symbol component)
{
    std::string fullname_str = _sym.GetName(stname) + "::" + _sym.GetName(component);
    return _sym.FindOrAdd(fullname_str);
}

AGS::ErrorType AGS::Parser::SkipTo(SymbolList const &stoplist, SrcList &source)
{
    int delimeter_nesting_depth = 0;
    for (; !source.ReachedEOF(); source.GetNext())
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        Symbol const next_sym = _src.PeekNext();
        switch (next_sym)
        {
        case kKW_OpenBrace:
        case kKW_OpenBracket:
        case kKW_OpenParenthesis:
        {
            ++delimeter_nesting_depth;
            continue;
        }
        case kKW_CloseBrace:
        case kKW_CloseBracket:
        case kKW_CloseParenthesis:
        {
            if (--delimeter_nesting_depth < 0)
                return kERR_None;
            continue;
        }
        }
        if (0 < delimeter_nesting_depth)
            continue;

        for (auto it = stoplist.begin(); it != stoplist.end(); ++it)
            if (next_sym == *it)
                return kERR_None;
    }
    return kERR_UserError;
}

AGS::ErrorType AGS::Parser::SkipToClose(Predefined closer)
{
    SkipTo(SymbolList{}, _src);
    if (closer == _src.GetNext())
        return kERR_None;
    
    Error("!Unexpected closing symbol");
    return kERR_InternalError;
}

AGS::ErrorType AGS::Parser::Expect(Symbol expected, Symbol actual, std::string const &custom_msg)
{
    if (actual == expected)
        return kERR_None;

    if ("" != custom_msg)
        Error(
            (custom_msg + ", found %s instead").c_str(),
            _sym.GetName(actual).c_str());
    else
        Error(
            "Expected '%s', found '%s' instead",
            _sym.GetName(expected).c_str(),
            _sym.GetName(actual).c_str());
    return kERR_UserError;
}

AGS::ErrorType AGS::Parser::Expect(std::vector<Symbol> const &expected, Symbol actual)
{
    for (size_t expected_idx = 0; expected_idx < expected.size(); expected_idx++)
        if (actual == expected[expected_idx])
            return kERR_None;
    std::string errmsg = "Expected ";
    for (size_t expected_idx = 0; expected_idx < expected.size(); expected_idx++)
    {
        errmsg += "'" + _sym.GetName(expected[expected_idx]) + "'";
        if (expected_idx + 2 < expected.size())
            errmsg += ", ";
        else if (expected_idx + 2 == expected.size())
            errmsg += " or ";
    }
    errmsg += ", found '%s' instead";
    Error(errmsg.c_str(), _sym.GetName(actual).c_str());
    return kERR_UserError;
}
            

AGS::Parser::NestingStack::NestingInfo::NestingInfo(NSType stype, ccCompiledScript &scrip)
    : Type(stype)
    , OldDefinitions({})
    , Start(BackwardJumpDest{ scrip })
    , JumpOut(ForwardJump{ scrip })
    , SwitchExprVartype(0)
    , SwitchDefault({ scrip })
    , SwitchJumptable({ scrip })
    , Chunks({})
{
}

// For assigning unique IDs to chunks
int AGS::Parser::NestingStack::_chunkIdCtr = 0;

AGS::Parser::NestingStack::NestingStack(ccCompiledScript &scrip)
    :_scrip(scrip)
{
    // Push first record on stack so that it isn't empty
    Push(NSType::kNone);
}

bool AGS::Parser::NestingStack::AddOldDefinition(Symbol s, SymbolTableEntry const &entry)
{
    if (_stack.back().OldDefinitions.count(s) > 0)
        return true;

    _stack.back().OldDefinitions.emplace(s, entry);
    return false;
}

// Rip the code that has already been generated, starting from codeoffset, out of scrip
// and move it into the vector at list, instead.
void AGS::Parser::NestingStack::YankChunk(size_t src_line, CodeLoc code_start, size_t fixups_start, int &id)
{
    Chunk item;
    item.SrcLine = src_line;

    size_t const codesize = std::max<int>(0, _scrip.codesize);
    for (size_t code_idx = code_start; code_idx < codesize; code_idx++)
        item.Code.push_back(_scrip.code[code_idx]);

    size_t numfixups = std::max<int>(0, _scrip.numfixups);
    for (size_t fixups_idx = fixups_start; fixups_idx < numfixups; fixups_idx++)
    {
        CodeLoc const code_idx = _scrip.fixups[fixups_idx];
        item.Fixups.push_back(code_idx - code_start);
        item.FixupTypes.push_back(_scrip.fixuptypes[fixups_idx]);
    }
    item.Id = id = ++_chunkIdCtr;

    _stack.back().Chunks.push_back(item);

    // Cut out the code that has been pushed
    _scrip.codesize = code_start;
    _scrip.numfixups = static_cast<decltype(_scrip.numfixups)>(fixups_start);
}

// Copy the code in the chunk to the end of the bytecode vector 
void AGS::Parser::NestingStack::WriteChunk(size_t level, size_t chunk_idx, int &id)
{
    Chunk const item = Chunks(level).at(chunk_idx);
    id = item.Id;

    // Add a line number opcode so that runtime errors
    // can show the correct originating source line.
    if (0u < item.Code.size() && SCMD_LINENUM != item.Code[0u] && 0u < item.SrcLine)
        _scrip.WriteLineno(item.SrcLine);

    // The fixups are stored relative to the start of the insertion,
    // so remember what that is
    size_t const start_of_insert = _scrip.codesize;
    size_t const code_size = item.Code.size();
    for (size_t code_idx = 0u; code_idx < code_size; code_idx++)
        _scrip.WriteCode(item.Code[code_idx]);

    size_t const fixups_size = item.Fixups.size();
    for (size_t fixups_idx = 0u; fixups_idx < fixups_size; fixups_idx++)
        _scrip.AddFixup(
            item.Fixups[fixups_idx] + start_of_insert,
            item.FixupTypes[fixups_idx]);

    // Make the last emitted source line number invalid so that the next command will
    // generate a line number opcode first
    _scrip.LastEmittedLineno = INT_MAX;
}

AGS::Parser::FuncCallpointMgr::FuncCallpointMgr(Parser &parser)
    : _parser(parser)
{ }

void AGS::Parser::FuncCallpointMgr::Reset()
{
    _funcCallpointMap.clear();
}

AGS::ErrorType AGS::Parser::FuncCallpointMgr::TrackForwardDeclFuncCall(Symbol func, CodeLoc loc, size_t in_source)
{
    // Patch callpoint in when known
    CodeCell const callpoint = _funcCallpointMap[func].Callpoint;
    if (callpoint >= 0)
    {
        _parser._scrip.code[loc] = callpoint;
        return kERR_None;
    }

    // Callpoint not known, so remember this location
    PatchInfo pinfo;
    pinfo.ChunkId = kCodeBaseId;
    pinfo.Offset = loc;
    pinfo.InSource = in_source;
    _funcCallpointMap[func].List.push_back(pinfo);

    return kERR_None;
}

AGS::ErrorType AGS::Parser::FuncCallpointMgr::UpdateCallListOnYanking(CodeLoc chunk_start, size_t chunk_len, int id)
{
    size_t const chunk_end = chunk_start + chunk_len;

    for (CallMap::iterator func_it = _funcCallpointMap.begin();
        func_it != _funcCallpointMap.end();
        ++func_it)
    {
        PatchList &pl = func_it->second.List;
        size_t const pl_size = pl.size();
        for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        {
            PatchInfo &patch_info = pl[pl_idx];
            if (kCodeBaseId != patch_info.ChunkId)
                continue;
            if (patch_info.Offset < chunk_start || patch_info.Offset >= static_cast<decltype(patch_info.Offset)>(chunk_end))
                continue; // This address isn't yanked

            patch_info.ChunkId = id;
            patch_info.Offset -= chunk_start;
        }
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::FuncCallpointMgr::UpdateCallListOnWriting(CodeLoc start, int id)
{
    for (CallMap::iterator func_it = _funcCallpointMap.begin();
        func_it != _funcCallpointMap.end();
        ++func_it)
    {
        PatchList &pl = func_it->second.List;
        size_t const size = pl.size();
        for (size_t pl_idx = 0; pl_idx < size; ++pl_idx)
        {
            PatchInfo &patch_info = pl[pl_idx];
            if (patch_info.ChunkId != id)
                continue; // Not our concern this time

            // We cannot repurpose patch_info since it may be written multiple times.
            PatchInfo cb_patch_info;
            cb_patch_info.ChunkId = kCodeBaseId;
            cb_patch_info.Offset = patch_info.Offset + start;
            pl.push_back(cb_patch_info);
        }
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::FuncCallpointMgr::SetFuncCallpoint(Symbol func, CodeLoc dest)
{
    _funcCallpointMap[func].Callpoint = dest;
    PatchList &pl = _funcCallpointMap[func].List;
    size_t const pl_size = pl.size();
    bool yanked_patches_exist = false;
    for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        if (kCodeBaseId == pl[pl_idx].ChunkId)
        {
            _parser._scrip.code[pl[pl_idx].Offset] = dest;
            pl[pl_idx].ChunkId = kPatchedId;
        }
        else if (kPatchedId != pl[pl_idx].ChunkId)
        {
            yanked_patches_exist = true;
        }
    if (!yanked_patches_exist)
        pl.clear();
    return kERR_None;
}

AGS::ErrorType AGS::Parser::FuncCallpointMgr::CheckForUnresolvedFuncs()
{
    for (auto fcm_it = _funcCallpointMap.begin(); fcm_it != _funcCallpointMap.end(); ++fcm_it)
    {
        PatchList &pl = fcm_it->second.List;
        size_t const pl_size = pl.size();
        for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        {
            if (kCodeBaseId != pl[pl_idx].ChunkId)
                continue;
            _parser._src.SetCursor(pl[pl_idx].InSource);
            _parser.Error(
                _parser.ReferenceMsgSym("The called function '%s()' isn't defined with body nor imported", fcm_it->first).c_str(),
                _parser._sym.GetName(fcm_it->first).c_str());
            return kERR_InternalError;
        }
    }
    return kERR_None;
}

AGS::Parser::FuncCallpointMgr::CallpointInfo::CallpointInfo()
    : Callpoint(-1)
{ }

AGS::Parser::RestorePoint::RestorePoint(ccCompiledScript &scrip)
    : _scrip(scrip)
{
    _restoreLoc = _scrip.codesize;
    _lastEmittedSrcLineno = _scrip.LastEmittedLineno;
}

void AGS::Parser::RestorePoint::Restore()
{
    _scrip.codesize = _restoreLoc;
    _scrip.LastEmittedLineno = _lastEmittedSrcLineno;
}

AGS::Parser::BackwardJumpDest::BackwardJumpDest(ccCompiledScript &scrip)
    : _scrip(scrip)
    , _dest(-1)
    , _lastEmittedSrcLineno(INT_MAX)
{ }

void AGS::Parser::BackwardJumpDest::Set(CodeLoc cl)
{
    _dest = (cl >= 0) ? cl : _scrip.codesize;
    _lastEmittedSrcLineno = _scrip.LastEmittedLineno;
}

void AGS::Parser::BackwardJumpDest::WriteJump(CodeCell jump_op, size_t cur_line)
{
    if (SCMD_LINENUM != _scrip.code[_dest] &&
        _scrip.LastEmittedLineno != _lastEmittedSrcLineno)
    {
        _scrip.WriteLineno(cur_line);
    }
    _scrip.WriteCmd(jump_op, _scrip.RelativeJumpDist(_scrip.codesize + 1, _dest));
}

AGS::Parser::ForwardJump::ForwardJump(ccCompiledScript &scrip)
    : _scrip(scrip)
    , _lastEmittedSrcLineno(INT_MAX)
{ }

void AGS::Parser::ForwardJump::AddParam(int offset)
{
    // If the current value for the last emitted lineno doesn't match the
    // saved value then the saved value won't work for all jumps so it
    // must be set to invalid.
    if (_jumpDestParamLocs.empty())
        _lastEmittedSrcLineno = _scrip.LastEmittedLineno;
    else if (_lastEmittedSrcLineno != _scrip.LastEmittedLineno)
        _lastEmittedSrcLineno = INT_MAX;
    _jumpDestParamLocs.push_back(_scrip.codesize + offset);
}

void AGS::Parser::ForwardJump::Patch(size_t cur_line)
{
    if (!_jumpDestParamLocs.empty())
    {
        // There are two ways of reaching the bytecode that will be emitted next:
        // through the jump or from the previous bytecode command. If the source line
        // of both isn't identical then a line opcode must be emitted next.
        if (cur_line != _scrip.LastEmittedLineno || cur_line != _lastEmittedSrcLineno)
            _scrip.LastEmittedLineno = INT_MAX;
    }
    for (auto loc = _jumpDestParamLocs.cbegin(); loc != _jumpDestParamLocs.cend(); loc++)
        _scrip.code[*loc] = _scrip.RelativeJumpDist(*loc, _scrip.codesize);
    _jumpDestParamLocs.clear();
}

AGS::Parser::MemoryLocation::MemoryLocation(Parser &parser)
    : _parser(parser)
    , _ScType (ScT::kNone)
    , _startOffs(0u)
    , _componentOffs (0u)
{
}

AGS::ErrorType AGS::Parser::MemoryLocation::SetStart(ScopeType type, size_t offset)
{
    if (ScT::kNone != _ScType)
    {
        _parser.Error("!Memory location object doubly initialized ");
        return kERR_InternalError;
    }
    _ScType = type;
    _startOffs = offset;
    _componentOffs = 0;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::MemoryLocation::MakeMARCurrent(size_t lineno, ccCompiledScript &scrip)
{
    switch (_ScType)
    {
    default:
        // The start offset is already reached (e.g., when a Dynpointer chain is dereferenced) 
        // but the component offset may need to be processed.
        if (_componentOffs > 0)
            scrip.WriteCmd(SCMD_ADD, SREG_MAR, _componentOffs);
        break;

    case ScT::kGlobal:
        scrip.RefreshLineno(lineno);
        scrip.WriteCmd(SCMD_LITTOREG, SREG_MAR, _startOffs + _componentOffs);
        scrip.FixupPrevious(Parser::kFx_GlobalData);
        break;

    case ScT::kImport:
        // Have to convert the import number into a code offset first.
        // Can only then add the offset to it.
        scrip.RefreshLineno(lineno);
        scrip.WriteCmd(SCMD_LITTOREG, SREG_MAR, _startOffs);
        scrip.FixupPrevious(Parser::kFx_Import);
        if (_componentOffs != 0)
            scrip.WriteCmd(SCMD_ADD, SREG_MAR, _componentOffs);
        break;

    case ScT::kLocal:
        scrip.RefreshLineno(lineno);
        CodeCell const offset = scrip.OffsetToLocalVarBlock - _startOffs - _componentOffs;
        if (offset < 0)
        {   // Must be a bug: That memory is unused.
            _parser.Error("!Trying to emit the negative offset %d to the top-of-stack", (int) offset);
            return kERR_InternalError;
        }

        scrip.WriteCmd(SCMD_LOADSPOFFS, offset);
        break;
    }
    Reset();
    return kERR_None;
}

void AGS::Parser::MemoryLocation::Reset()
{
    _ScType = ScT::kNone;
    _startOffs = 0u;
    _componentOffs = 0u;
}

AGS::Parser::Parser(SrcList &src, FlagSet options, ccCompiledScript &scrip, SymbolTable &symt, MessageHandler &mh)
    : _nest(scrip)
    , _pp(PP::kPreAnalyze)
    , _sym(symt)
    , _src(src)
    , _options(options)
    , _scrip(scrip)
    , _msg_handler(mh)
    , _fcm(*this)
    , _fim(*this)
    , _structRefs({})
{
    _givm.clear();
    _lastEmittedSectionId = 0;
    _lastEmittedLineno = 0;
}

void AGS::Parser::SetDynpointerInManagedVartype(Vartype &vartype)
{
    if (_sym.IsManagedVartype(vartype))
        vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
}

size_t AGS::Parser::StacksizeOfLocals(size_t from_level)
{
    size_t total_size = 0;
    for (size_t level = from_level; level <= _nest.TopLevel(); level++)
    {
        // We only skim through the old definitions to get their indexes in _sym.
        // We don't use the definitions themselves here but what is current instead.
        std::map<Symbol, SymbolTableEntry> const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.cbegin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            if (_sym.IsVariable(s))
                total_size += _sym.GetSize(s);
        }
    }
    return total_size;
}

// Does vartype v contain releasable pointers?
bool AGS::Parser::ContainsReleasableDynpointers(Vartype vartype)
{
    if (_sym.IsDynVartype(vartype))
        return true;
    if (_sym.IsArrayVartype(vartype))
        return ContainsReleasableDynpointers(_sym[vartype].VartypeD->BaseVartype);
    if (!_sym.IsStructVartype(vartype))
        return false; // Atomic non-structs can't have pointers

    SymbolList compo_list;
    _sym.GetComponentsOfStruct(vartype, compo_list);
    for (size_t cl_idx = 0; cl_idx < compo_list.size(); cl_idx++)
    {
        Symbol const &var = compo_list[cl_idx];
        if (!_sym.IsVariable(var))
            continue;
        if (ContainsReleasableDynpointers(_sym[var].VariableD->Vartype))
            return true;
    }
    return false;
}

// We're at the end of a block and releasing a standard array of pointers.
// MAR points to the array start. Release each array element (pointer).
AGS::ErrorType AGS::Parser::FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements, bool &clobbers_ax)
{
    if (num_of_elements == 0)
        return kERR_None;

    if (num_of_elements < 4)
    {
        WriteCmd(SCMD_MEMZEROPTR);
        for (size_t loop = 1; loop < num_of_elements; ++loop)
        {
            WriteCmd(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
            WriteCmd(SCMD_MEMZEROPTR);
        }
        return kERR_None;
    }

    clobbers_ax = true;
    WriteCmd(SCMD_LITTOREG, SREG_AX, num_of_elements);

    BackwardJumpDest loop_start(_scrip);
    loop_start.Set();
    WriteCmd(SCMD_MEMZEROPTR);
    WriteCmd(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
    WriteCmd(SCMD_SUB, SREG_AX, 1);
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
    return kERR_None;
}

// We're at the end of a block and releasing all the pointers in a struct.
// MAR already points to the start of the struct.
void AGS::Parser::FreeDynpointersOfStruct(Vartype struct_vtype, bool &clobbers_ax)
{
    SymbolList compo_list;
    _sym.GetComponentsOfStruct(struct_vtype, compo_list);
    for (int cl_idx = 0; cl_idx < static_cast<int>(compo_list.size()); cl_idx++) // note "int"!
    {
        Symbol const component = compo_list[cl_idx];
        if (_sym.IsVariable(component) && ContainsReleasableDynpointers(_sym[component].VariableD->Vartype))
            continue;
        // Get rid of this component
        compo_list[cl_idx] = compo_list.back();
        compo_list.pop_back();
        cl_idx--; // this might make the var negative so it needs to be int
    }

    // Note, at this point, the components' offsets might no longer be ascending

    int offset_so_far = 0;
    for (auto compo_it = compo_list.begin(); compo_it != compo_list.end(); ++compo_it)
    {
        Symbol const &component = *compo_it;
        int const offset = static_cast<int>(_sym[component].ComponentD->Offset);
        Vartype const vartype = _sym[component].VariableD->Vartype;
        
        // Let MAR point to the component
        int const diff = offset - offset_so_far;
        if (diff != 0)
            WriteCmd(SCMD_ADD, SREG_MAR, diff);
        offset_so_far = offset;

        if (_sym.IsDynVartype(vartype))
        {
            WriteCmd(SCMD_MEMZEROPTR);
            continue;
        }

        if (compo_list.cend() != compo_it + 1)
            PushReg(SREG_MAR);
        if (_sym.IsArrayVartype(vartype))
            FreeDynpointersOfStdArray(vartype, clobbers_ax);
        else if (_sym.IsStructVartype(vartype))
            FreeDynpointersOfStruct(vartype, clobbers_ax);
        if (compo_list.cend() != compo_it + 1)
            PopReg(SREG_MAR);
    }
}

// We're at the end of a block and we're releasing a standard array of struct.
// MAR points to the start of the array. Release all the pointers in the array.
void AGS::Parser::FreeDynpointersOfStdArrayOfStruct(Vartype element_vtype, size_t num_of_elements, bool &clobbers_ax)
{
    clobbers_ax = true;

    // AX will be the index of the current element
    WriteCmd(SCMD_LITTOREG, SREG_AX, num_of_elements);

    BackwardJumpDest loop_start(_scrip);
    loop_start.Set();
    PushReg(SREG_MAR);
    PushReg(SREG_AX); // FreeDynpointersOfStruct might call funcs that clobber AX
    FreeDynpointersOfStruct(element_vtype, clobbers_ax);
    PopReg(SREG_AX);
    PopReg(SREG_MAR);
    WriteCmd(SCMD_ADD, SREG_MAR, _sym.GetSize(element_vtype));
    WriteCmd(SCMD_SUB, SREG_AX, 1);
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
    return;
}

// We're at the end of a block and releasing a standard array. MAR points to the start.
// Release the pointers that the array contains.
void AGS::Parser::FreeDynpointersOfStdArray(Symbol the_array, bool &clobbers_ax)
{
    Vartype const array_vartype =
        _sym.IsVartype(the_array) ? the_array : _sym.GetVartype(the_array);
    size_t const num_of_elements = _sym.NumArrayElements(array_vartype);
    if (num_of_elements < 1)
        return; // nothing to do
    Vartype const element_vartype =
        _sym[array_vartype].VartypeD->BaseVartype;
    if (_sym.IsDynpointerVartype(element_vartype))
    {
        FreeDynpointersOfStdArrayOfDynpointer(num_of_elements, clobbers_ax);
        return;
    }

    if (_sym.IsStructVartype(element_vartype))
        FreeDynpointersOfStdArrayOfStruct(element_vartype, num_of_elements, clobbers_ax);

    return;
}

// Note: Currently, the structs/arrays that are pointed to cannot contain
// pointers in their turn.
// If they do, we need a solution at runtime to chase the pointers to release;
// we can't do it at compile time. Also, the pointers might form "rings"
// (e.g., A contains a field that points to B; B contains a field that
// points to A), so we can't rely on reference counting for identifying
// _all_ the unreachable memory chunks. (If nothing else points to A or B,
// both are unreachable so _could_ be released, but they still point to each
// other and so have a reference count of 1; the reference count will never reach 0).

AGS::ErrorType AGS::Parser::FreeDynpointersOfLocals0(size_t from_level, bool &clobbers_ax, bool &clobbers_mar)
{
    for (size_t level = from_level; level <= _nest.TopLevel(); level++)
    {
        std::map<Symbol, SymbolTableEntry> const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.cbegin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            if (!_sym.IsVariable(s))
                continue; 
            Vartype const s_vartype = _sym.GetVartype(s);
            if (!ContainsReleasableDynpointers(s_vartype))
                continue;

            // Set MAR to the start of the construct that contains releasable pointers
            WriteCmd(SCMD_LOADSPOFFS, _scrip.OffsetToLocalVarBlock - _sym[s].VariableD->Offset);
            clobbers_mar = true;
            if (_sym.IsDynVartype(s_vartype))
                WriteCmd(SCMD_MEMZEROPTR);
            else if (_sym.IsArrayVartype(s_vartype))
                FreeDynpointersOfStdArray(s, clobbers_ax);
            else if (_sym.IsStructVartype(s_vartype))
                FreeDynpointersOfStruct(s_vartype, clobbers_ax);
        }
    }
    return kERR_None;
}

// Free the pointers of any locals that have a nesting depth higher than from_level
AGS::ErrorType AGS::Parser::FreeDynpointersOfLocals(size_t from_level)
{
    bool dummy_bool;
    return FreeDynpointersOfLocals0(from_level, dummy_bool, dummy_bool);
}

AGS::ErrorType AGS::Parser::FreeDynpointersOfAllLocals_DynResult(void)
{
    // The return value AX might point to a local dynamic object. So if we
    // now free the dynamic references and we don't take precautions,
    // this dynamic memory might drop its last reference and get
    // garbage collected in consequence. AX would have a dangling pointer.
    // We only need these precautions if there are local dynamic objects.
    RestorePoint rp_before_precautions(_scrip);

    // Allocate a local dynamic pointer to hold the return value.
    PushReg(SREG_AX);
    WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
    WriteCmd(SCMD_MEMINITPTR, SREG_AX);

    RestorePoint rp_before_freeing(_scrip);
    bool dummy_bool;
    bool mar_may_be_clobbered = false;
    ErrorType retval = FreeDynpointersOfLocals0(0u, dummy_bool, mar_may_be_clobbered);
    if (retval < 0) return retval;
    bool const no_precautions_were_necessary = rp_before_freeing.IsEmpty();

    // Now release the dynamic pointer with a special opcode that prevents 
    // memory de-allocation as long as AX still has this pointer, too
    if (mar_may_be_clobbered)
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
    WriteCmd(SCMD_MEMREADPTR, SREG_AX);
    WriteCmd(SCMD_MEMZEROPTRND); // special opcode
    PopReg(SREG_BX); // do NOT pop AX here
    if (no_precautions_were_necessary)
        rp_before_precautions.Restore();
    return kERR_None;
}

// Free all local Dynpointers taking care to not clobber AX
AGS::ErrorType AGS::Parser::FreeDynpointersOfAllLocals_KeepAX(void)
{
    RestorePoint rp_before_free(_scrip);
    bool clobbers_ax = false;
    bool dummy_bool;
    ErrorType retval = FreeDynpointersOfLocals0(0u, clobbers_ax, dummy_bool);
    if (retval < 0) return retval;
    if (!clobbers_ax)
        return kERR_None;

    // We should have saved AX, so redo this
    rp_before_free.Restore();
    PushReg(SREG_AX);
    retval = FreeDynpointersOfLocals0(0u, clobbers_ax, dummy_bool);
    if (retval < 0) return retval;
    PopReg(SREG_AX);

    return kERR_None;
}

AGS::ErrorType AGS::Parser::RestoreLocalsFromSymtable(size_t from_level)
{
    size_t const last_level = _nest.TopLevel();
    for (size_t level = from_level; level <= last_level; level++)
    {
        // Restore the old definition that we've stashed
        auto const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.begin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            // Note, it's important we use deep copy because the vector elements will be destroyed when the level is popped
            _sym[s] = symbols_it->second;
                continue;
        }
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::HandleEndOfDo()
{
    ErrorType retval = Expect(
        kKW_While,
        _src.GetNext(),
        "Expected the 'while' of a 'do ... while(...)' statement");
    if (retval < 0) return retval;

    retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // Jump back to the start of the loop while the condition is true
    _nest.Start().WriteJump(SCMD_JNZ, _src.GetLineno());
    // Jumps out of the loop should go here
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();

    return kERR_None;
}

AGS::ErrorType AGS::Parser::HandleEndOfElse()
{
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();
    return kERR_None;
}

AGS::ErrorType AGS::Parser::HandleEndOfSwitch()
{
    // If there was no terminating break at the last switch-case, 
    // write a jump to the jumpout point to prevent a fallthrough into the jumptable
    CodeLoc const lastcmd_loc = _scrip.codesize - 2;
    if (SCMD_JMP != _scrip.code[lastcmd_loc])
    {
        WriteCmd(SCMD_JMP, -77);
        _nest.JumpOut().AddParam();
    }

    // We begin the jump table
    _nest.SwitchJumptable().Patch(_src.GetLineno());

    // Get correct comparison operation: Don't compare strings as pointers but as strings
    CodeCell const eq_opcode =
        _sym.IsAnyStringVartype(_nest.SwitchExprVartype()) ? SCMD_STRINGSEQUAL : SCMD_ISEQUAL;

    const size_t number_of_cases = _nest.Chunks().size();
    for (size_t cases_idx = 0; cases_idx < number_of_cases; ++cases_idx)
    {
        int id;
        CodeLoc const codesize = _scrip.codesize;
        // Emit the code for the case expression of the current case. Result will be in AX
        _nest.WriteChunk(cases_idx, id);
        _fcm.UpdateCallListOnWriting(codesize, id);
        _fim.UpdateCallListOnWriting(codesize, id);
        
        WriteCmd(eq_opcode, SREG_AX, SREG_BX);
        _nest.SwitchCases().at(cases_idx).WriteJump(SCMD_JNZ, _src.GetLineno());
    }

    if (INT_MAX != _nest.SwitchDefault().Get())
        _nest.SwitchDefault().WriteJump(SCMD_JMP, _src.GetLineno());

    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();
    return kERR_None;
}

// Must return a symbol that is a literal.
AGS::ErrorType AGS::Parser::ParseParamlist_Param_DefaultValue(Vartype param_type, Symbol &default_value)
{
    if (kKW_Assign != _src.PeekNext())
    {
        default_value = kKW_NoSymbol; // No default value given
        return kERR_None;
    }

    _src.GetNext();   // Eat '='

    Symbol default_symbol = _src.GetNext(); 
    bool is_negative = false;
    if (kKW_Minus == default_symbol &&
        (kKW_Float == param_type || _sym.IsAnyIntegerVartype(param_type)))
    {
        is_negative = true;
        default_symbol = _src.GetNext();
    }

    while (_sym.IsConstant(default_symbol))
        default_symbol = _sym[default_symbol].ConstantD->ValueSym;

    if (_sym.IsDynVartype(param_type))
    {
        default_value = kKW_Null;
        if (kKW_Null == default_symbol)
            return kERR_None;
        if (_sym.Find("0") == default_symbol)
        {
            Warning("Found '0' as a parameter default for a dynamic object (prefer 'null')");
            return kERR_None;
        }
        Error("Expected the parameter default 'null', found '%s' instead", _sym.GetName(default_symbol).c_str());
        return kERR_UserError;
    }

    if (_sym.IsAnyStringVartype(param_type))
    {
        default_value = default_symbol;
        if (_sym.Find("0") == default_symbol)
        {
            Warning("Found '0' as a parameter default for a string (prefer '\"\"')");
            return kERR_None;
        }
        if (!_sym.IsLiteral(default_value) || kKW_String != _sym[default_value].LiteralD->Vartype)
        {
            Error (
                "Expected a constant or literal string as a parameter default, found '%s' instead",
                _sym.GetName(default_symbol).c_str());
            return kERR_UserError;
        }
    }   

    if (_sym.IsAnyIntegerVartype(param_type))
    {
        if (!_sym.IsLiteral(default_symbol) || kKW_Int != _sym[default_symbol].LiteralD->Vartype)
        {
            Error(
                "Expected a constant or literal integer as a parameter default, found '%s' instead",
                ((is_negative ? "-" : "") + _sym.GetName(default_symbol)).c_str());
            return kERR_UserError;
        }
        default_value = default_symbol;
        if (is_negative)
            return NegateLiteral(default_value);
        return kERR_None;
    }

    if (kKW_Float == param_type)
    {
        if (_sym.Find("0") == default_symbol)
        {
            Warning("Found '0' as a parameter default for a float (prefer '0.0')");
        }
        else if (!_sym.IsLiteral(default_symbol) || kKW_Float != _sym[default_symbol].LiteralD->Vartype)
        {
            Error(
                "Expected a constant or literal float as a parameter default, found '%s' instead",
                ((is_negative ? "-" : "") + _sym.GetName(default_symbol)).c_str());
            return kERR_UserError;
        }
        default_value = default_symbol;
        if (is_negative)
            return NegateLiteral(default_value);
        return kERR_None;
    }

    Error("Parameter cannot have any default value");
    return kERR_UserError;
}

AGS::ErrorType AGS::Parser::ParseDynArrayMarkerIfPresent(Vartype &vartype)
{
    if (kKW_OpenBracket != _src.PeekNext())
        return kERR_None;
    _src.GetNext(); // Eat '['
    ErrorType retval = Expect(kKW_CloseBracket, _src.GetNext());
    if (retval < 0) return retval;

    vartype = _sym.VartypeWith(VTT::kDynarray, vartype);
    return kERR_None;
}

// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
// We'll accept something like "this Character *"
AGS::ErrorType AGS::Parser::ParseFuncdecl_ExtenderPreparations(bool is_static_extender, Symbol &strct, Symbol &unqualified_name, TypeQualifierSet &tqs)
{
    if (is_static_extender)
        tqs[TQ::kStatic] = true;

    _src.GetNext(); // Eat "this" or "static"
    strct = _src.GetNext();
    if (!_sym.IsStructVartype(strct))
    {
        Error("Expected a struct type instead of '%s'", _sym.GetName(strct).c_str());
        return kERR_UserError;
    }

    Symbol const qualified_name = MangleStructAndComponent(strct, unqualified_name);

    if (kKW_Dynpointer == _src.PeekNext())
    {
        if (is_static_extender)
        {
            Error("Unexpected '*' after 'static' in static extender function");
            return kERR_UserError;
        }
        _src.GetNext(); // Eat '*'
    }

    // If a function is defined with the Extender mechanism, it needn't have a declaration
    // in the struct defn. So pretend that this declaration has happened.
    auto &components = _sym[strct].VartypeD->Components;
    if (0 == components.count(unqualified_name))
        components[unqualified_name] = qualified_name;
    _sym.MakeEntryComponent(qualified_name);
    _sym[qualified_name].ComponentD->Component = unqualified_name;
    _sym[qualified_name].ComponentD->Parent = strct;
    _sym[qualified_name].ComponentD->IsFunction = true;
    
    Symbol const punctuation = _src.PeekNext();
    ErrorType retval = Expect(SymbolList{ kKW_Comma, kKW_CloseParenthesis }, punctuation);
    if (retval < 0) return retval;
    if (kKW_Comma == punctuation)
        _src.GetNext(); // Eat ','

    unqualified_name = qualified_name;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVarname0(bool accept_member_access, Symbol &structname, Symbol &varname)
{
    structname = kKW_NoSymbol;
    varname = _src.GetNext();
    if (varname <= kKW_LastPredefined)
    {
        Error("Expected an identifier, found '%s' instead", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    // Note: A varname may be allowed although there already is a vartype with the same name.
    // For instance, as a component of a struct. (Room is a vartype; but Character.Room is allowed)
    if (kKW_ScopeRes != _src.PeekNext())
        return kERR_None;

    _src.GetNext(); // Eat '::'
    if (!accept_member_access)
    {
        Error("May not use '::' here");
        return kERR_UserError;
    }

    structname = varname;
    Symbol const unqualified_component = _src.GetNext();
    if (_sym.IsVartype(structname))
    {    
        auto const &components = _sym[structname].VartypeD->Components;
        if (0u == components.count(unqualified_component))
        {
            Error(
                ReferenceMsgSym(
                    "'%s' isn't a component of '%s'",
                    structname).c_str(),
                _sym.GetName(unqualified_component).c_str(),
                _sym.GetName(structname).c_str());
            return kERR_UserError;
        }

        varname = components.at(unqualified_component);
    }
    else
    {
        // This can happen and be legal for struct component functions
        varname = MangleStructAndComponent(structname, unqualified_component);
    }
    
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseParamlist_ParamType(Vartype &vartype)
{
    if (kKW_Void == vartype)
    {
        Error("A function parameter must not have the type 'void'");
        return kERR_UserError;
    }
    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    if (PP::kMain == _pp && !_sym.IsManagedVartype(vartype) && _sym.IsStructVartype(vartype))
    {
        Error("'%s' is non-managed; a non-managed struct cannot be passed as parameter", _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}


// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present
AGS::ErrorType AGS::Parser::ParseParamlist_Param_Name(bool body_follows, Symbol &param_name)
{
    param_name = kKW_NoSymbol;

    if (PP::kPreAnalyze == _pp || !body_follows)
    {
        // Ignore the parameter name when present, it won't be used later on (in this phase)
        Symbol const nextsym = _src.PeekNext();
        if (_sym.IsIdentifier(nextsym))
            _src.GetNext();
        return kERR_None;
    }

    ErrorType retval = ParseVarname(param_name);
    if (retval < 0) return retval;

    if (_sym.IsFunction(param_name))
    {
        Warning(
            ReferenceMsgSym("This hides the function '%s()'", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_None;
    }

    if (_sym.IsVariable(param_name))
    {
        if (ScT::kLocal != _sym.GetScopeType(param_name))
            return kERR_None;

        Error(
            ReferenceMsgSym("The name '%s' is already in use as a parameter", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_UserError;
    }

    if (_sym.IsVartype(param_name))
    {
        Warning(
            ReferenceMsgSym("This hides the type '%s'", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_None;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseParamlist_Param_AsVar2Sym(Symbol param_name, Vartype param_vartype, bool param_is_const, int param_idx)
{
    SymbolTableEntry &param_entry = _sym[param_name];
    
    if (param_is_const)
    {
        param_entry.VariableD->TypeQualifiers[TQ::kReadonly] = true;
        param_entry.VariableD->Vartype =
            _sym.VartypeWith(VTT::kConst, param_entry.VariableD->Vartype);
    }
    // the parameters are pushed backwards, so the top of the
    // stack has the first parameter. The + 1 is because the
    // call will push the return address onto the stack as well
    param_entry.VariableD->Offset =
        _scrip.OffsetToLocalVarBlock - (param_idx + 1) * SIZE_OF_STACK_CELL;
    _sym.SetDeclared(param_name, _src.GetCursor());
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseParamlist_Param(Symbol name_of_func, bool body_follows, Vartype param_vartype, bool param_is_const, size_t param_idx)
{
    ErrorType retval = ParseParamlist_ParamType(param_vartype);
    if (retval < 0) return retval;
    if (param_is_const)
        param_vartype = _sym.VartypeWith(VTT::kConst, param_vartype);

    Symbol param_name;
    retval = ParseParamlist_Param_Name(body_follows, param_name);
    if (retval < 0) return retval;

    retval = ParseDynArrayMarkerIfPresent(param_vartype);
    if (retval < 0) return retval;

    Symbol param_default;
    retval = ParseParamlist_Param_DefaultValue(param_vartype, param_default);
    if (retval < 0) return retval;


    _sym[name_of_func].FunctionD->Parameters.push_back({});
    _sym[name_of_func].FunctionD->Parameters.back().Vartype = param_vartype; 
    _sym[name_of_func].FunctionD->Parameters.back().Name = param_name;
    _sym[name_of_func].FunctionD->Parameters.back().Default = param_default;
    
    if (PP::kMain != _pp || !body_follows)
        return kERR_None;

    // All function parameters correspond to local variables.
    // A body will follow, so we need to enter this parameter as a variable into the symbol table
    ParseVardecl_CheckAndStashOldDefn(param_name);
    retval = ParseVardecl_Var2SymTable(param_name, param_vartype, ScT::kLocal);
    if (retval < 0) return retval;
    // Set the offset, make "const" if required
    return ParseParamlist_Param_AsVar2Sym(param_name, param_vartype, param_is_const, param_idx);
}

AGS::ErrorType AGS::Parser::ParseFuncdecl_Paramlist(Symbol funcsym, bool body_follows)
{
    _sym[funcsym].FunctionD->IsVariadic = false;
    _sym[funcsym].FunctionD->Parameters.resize(1u); // [0] is the return type; leave that
    bool param_is_const = false;
    size_t param_idx = 0;
    while (!_src.ReachedEOF())
    {
        Symbol const cursym = _src.GetNext();
        if (kKW_CloseParenthesis == cursym)
            return kERR_None;   // empty parameter list

        if (_sym.IsVartype(cursym))
        {
            if (param_idx == 0 && kKW_Void == cursym && kKW_CloseParenthesis == _src.PeekNext())
            {   // explicitly empty parameter list, "(void)"
                _src.GetNext(); // Eat ')'
                return kERR_None;
            }

            if ((++param_idx) >= MAX_FUNCTION_PARAMETERS)
            {
                Error("Too many parameters defined for function (max. allowed: %u)", MAX_FUNCTION_PARAMETERS - 1u);
                return kERR_UserError;
            }

            ErrorType retval = ParseParamlist_Param(funcsym, body_follows, cursym, param_is_const, _sym.NumOfFuncParams(funcsym) + 1);
            if (retval < 0) return retval;

            param_is_const = false; // modifier has been used up
            Symbol const nextsym = _src.GetNext();
            if (kKW_Comma != nextsym && kKW_CloseParenthesis != nextsym)
            {
                Error("Expected ',' or ')' or an identifier, found '%s' instead", _sym.GetName(nextsym).c_str());
                return kERR_UserError;
            }
            if (kKW_CloseParenthesis == nextsym)
                return kERR_None;
            continue;
        }

        if (kKW_Const == cursym)
        {
            // check in main compiler phase that type must follow
            if (PP::kMain == _pp && !_sym.IsVartype(_src.PeekNext()))
            {
                Error(
                    "Expected a type after 'const', found '%s' instead",
                    _sym.GetName(_src.PeekNext()).c_str());
                return kERR_UserError;
            }
            param_is_const = true;
            continue;
        }

        if (kKW_DotDotDot == cursym)
        {
            _sym[funcsym].FunctionD->IsVariadic = true;
            return Expect(kKW_CloseParenthesis, _src.GetNext(), "Expected ')' following the '...'");
        }
        
        Error("Unexpected '%s' in parameter list", _sym.GetName(cursym).c_str());
        return kERR_UserError;
    } // while
    // Can't happen
    Error("!End of input when processing parameter list");
    return kERR_InternalError;
}
void AGS::Parser::ParseFuncdecl_MasterData2Sym(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_function, Symbol name_of_function, bool body_follows)
{
    _sym.MakeEntryFunction(name_of_function);
    SymbolTableEntry &entry = _sym[name_of_function];
    
    entry.FunctionD->Parameters.resize(1u);
    entry.FunctionD->Parameters[0].Vartype = return_vartype;
    entry.FunctionD->Parameters[0].Name = kKW_NoSymbol;
    entry.FunctionD->Parameters[0].Default = kKW_NoSymbol;
    auto &entry_tqs = entry.FunctionD->TypeQualifiers;
    entry_tqs[TQ::kConst] = tqs[TQ::kConst];
    entry_tqs[TQ::kImport] = tqs[TQ::kImport];
    entry_tqs[TQ::kProtected] = tqs[TQ::kProtected];
    entry_tqs[TQ::kReadonly] = tqs[TQ::kReadonly];
    entry_tqs[TQ::kStatic] = tqs[TQ::kStatic];
    entry_tqs[TQ::kWriteprotected] = tqs[TQ::kWriteprotected];
    // Do not set Extends and the component flag here.
    // They are used to denote functions that were either declared in a struct defn or as extender

    if (PP::kPreAnalyze == _pp)
    {
        // Encode in entry.Offset the type of function declaration
        FunctionType ft = kFT_PureForward;
        if (tqs[TQ::kImport])
            ft = kFT_Import;
        if (body_follows)
            ft = kFT_LocalBody;
        if (_sym[name_of_function].FunctionD->Offset < ft)
            _sym[name_of_function].FunctionD->Offset = ft;
    }
    return;
}

// there was a forward declaration -- check that the real declaration matches it
AGS::ErrorType AGS::Parser::ParseFuncdecl_CheckThatKnownInfoMatches(std::string const &func_name, SymbolTableEntry::FunctionDesc const *this_entry, SymbolTableEntry::FunctionDesc const *known_info, size_t const known_declared, bool body_follows)
{
    if (!known_info)
        return kERR_None; // We don't have any known info
    if (!this_entry)
    {
        Error("!Function record missing");
        return kERR_InternalError;
    }

    auto known_tq = known_info->TypeQualifiers;
    known_tq[TQ::kImport] = false;
    auto this_tq = this_entry->TypeQualifiers;
    this_tq[TQ::kImport]  = false;
    if (known_tq != this_tq)
    {
        std::string const known_tq_str = TypeQualifierSet2String(known_tq);
        std::string const this_tq_str = TypeQualifierSet2String(this_tq);
        std::string const msg = ReferenceMsgLoc("'%s' has the qualifiers '%s' here but '%s' elsewhere", known_declared);
        Error(msg.c_str(), func_name.c_str(), this_tq_str.c_str(), known_tq_str.c_str());
        return kERR_UserError;
    }

    size_t const known_num_parameters = known_info->Parameters.size() - 1;
    size_t const this_num_parameters = this_entry->Parameters.size() - 1;
    if (known_num_parameters != this_num_parameters)
    {
        std::string const msg = ReferenceMsgLoc(
            "Function '%s' is declared with %d mandatory parameters here, %d mandatory parameters elswehere",
            known_declared);
        Error(msg.c_str(), func_name.c_str(), this_num_parameters, known_num_parameters);
        return kERR_UserError;
    }
    if (known_info->IsVariadic != this_entry->IsVariadic)
    {
        std::string const te =
            this_entry->IsVariadic ?
            "is declared to accept additional parameters here" :
            "is declared to not accept additional parameters here";
        std::string const ki =
            known_info->IsVariadic ?
            "to accepts additional parameters elsewhere" :
            "to not accept additional parameters elsewhere";
        std::string const msg =
            ReferenceMsgLoc("Function '%s' %s, %s", known_declared);
        Error(msg.c_str(), func_name.c_str(), te.c_str(), ki.c_str());
        return kERR_UserError;
    }

    Symbol const known_ret_type = known_info->Parameters[0u].Vartype;
    Symbol const this_ret_type = this_entry->Parameters[0u].Vartype;
    if (known_ret_type != this_ret_type)
    {
        std::string const msg = ReferenceMsgLoc(
            "Return type of '%s' is declared as '%s' here, as '%s' elsewhere",
            known_declared);
        Error(
            msg.c_str(),
            func_name.c_str(),
            _sym.GetName(this_ret_type).c_str(),
            _sym.GetName(known_ret_type).c_str());

        return kERR_UserError;
    }

    auto const &known_params = known_info->Parameters;
    auto const &this_params = this_entry->Parameters;
    for (size_t param_idx = 1; param_idx <= this_num_parameters; param_idx++)
    {
        Vartype const known_param_vartype = known_params[param_idx].Vartype;
        Vartype const this_param_vartype = this_params[param_idx].Vartype;
        if (known_param_vartype != this_param_vartype)
        {
            std::string const msg = ReferenceMsgLoc(
                "For function '%s': Type of parameter #%d is %s here, %s in a declaration elsewhere",
                known_declared);
            Error(
                msg.c_str(),
                func_name.c_str(),
                param_idx,
                _sym.GetName(this_param_vartype).c_str(),
                _sym.GetName(known_param_vartype).c_str());
            return kERR_UserError;
        }
    }

    if (body_follows)
    {
        // If none of the parameters have a default, we'll let this through.
        bool has_default = false;
        for (size_t param_idx = 1; param_idx < this_params.size(); ++param_idx)
            if (kKW_NoSymbol != this_params[param_idx].Default)
            {
                has_default = true;
                break;
            }
        if (!has_default)
            return kERR_None;
    }

    for (size_t param_idx = 1; param_idx < this_params.size(); ++param_idx)
    {
        auto const this_default = this_params[param_idx].Default;
        auto const known_default = known_params[param_idx].Default;
        if (this_default == known_default)
            continue;

        std::string errstr1 = "In this declaration, parameter #<1> <2>; ";
        errstr1.replace(errstr1.find("<1>"), 3, std::to_string(param_idx));
        if (kKW_NoSymbol == this_default)
            errstr1.replace(errstr1.find("<2>"), 3, "doesn't have a default value");
        else
            errstr1.replace(errstr1.find("<2>"), 3, "has the default " + _sym.GetName(this_default));

        std::string errstr2 = "in a declaration elsewhere, that parameter <2>";
        if (kKW_NoSymbol == known_default)
            errstr2.replace(errstr2.find("<2>"), 3, "doesn't have a default value");
        else
            errstr2.replace(errstr2.find("<2>"), 3, "has the default " + _sym.GetName(known_default));
        errstr1 += errstr2;
        Error(ReferenceMsgLoc(errstr1, known_declared).c_str());
        return kERR_UserError;
    }
    
    return kERR_None;
}

// Enter the function in the imports[] or functions[] array; get its index   
AGS::ErrorType AGS::Parser::ParseFuncdecl_EnterAsImportOrFunc(Symbol name_of_func, bool body_follows, bool func_is_import, size_t num_of_parameters, CodeLoc &function_soffs)
{
    if (body_follows)
    {
        // Index of the function in the ccCompiledScript::functions[] array
        function_soffs = _scrip.AddNewFunction(_sym.GetName(name_of_func), num_of_parameters);
        if (function_soffs < 0)
        {
            Error("Max. number of functions exceeded");
            return kERR_UserError;
        }
        _fcm.SetFuncCallpoint(name_of_func, function_soffs);
        return kERR_None;
    }

    if (!func_is_import)
    {
        function_soffs = -1; // forward decl; callpoint is unknown yet
        return kERR_None;
    }

    // Index of the function in the ccScript::imports[] array
    function_soffs = _scrip.FindOrAddImport(_sym.GetName(name_of_func));
    return kERR_None;
}


// We're at something like "int foo(", directly before the "("
// Get the symbol after the corresponding ")"
AGS::ErrorType AGS::Parser::ParseFuncdecl_DoesBodyFollow(bool &body_follows)
{
    int const cursor = _src.GetCursor();

    ErrorType retval = SkipToClose(kKW_CloseParenthesis);
    if (retval < 0) return retval;
    body_follows = (kKW_OpenBrace == _src.PeekNext());

    _src.SetCursor(cursor);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseFuncdecl_Checks(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, Vartype return_vartype, bool body_follows, bool no_loop_check)
{
    if (kKW_NoSymbol == struct_of_func && tqs[TQ::kProtected])
    {
        Error(
            "Function '%s' isn't a struct component and so cannot be 'protected'",
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (!body_follows && no_loop_check)
    {
        Error("Can only use 'noloopcheck' when a function body follows the definition");
        return kERR_UserError;
    }

    if(!_sym.IsFunction(name_of_func) && _sym.IsInUse(name_of_func))
    {
        Error(
            ReferenceMsgSym("'%s' is defined elsewhere as a non-function", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (!_sym.IsManagedVartype(return_vartype) && _sym.IsStructVartype(return_vartype))
    {
        Error("Can only return a struct when it is 'managed'");
        return kERR_UserError;
    }

    if (PP::kPreAnalyze == _pp &&
        body_follows &&
        _sym.IsFunction(name_of_func) &&
        kFT_LocalBody == _sym[name_of_func].FunctionD->Offset)
    {
        Error(
            ReferenceMsgSym("Function '%s' is already defined with body elsewhere", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (PP::kMain != _pp || kKW_NoSymbol == struct_of_func)
        return kERR_None;

    if (!_sym.IsComponent(name_of_func) ||
        struct_of_func != _sym[name_of_func].ComponentD->Parent)
    {
        // Functions only become struct components if they are declared in a struct or as extender
        std::string component = _sym.GetName(name_of_func);
        component.erase(0, component.rfind(':') + 1);
        Error(
            ReferenceMsgSym("Function '%s' has not been declared within struct '%s' as a component", struct_of_func).c_str(),
            component.c_str(), _sym.GetName(struct_of_func).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseFuncdecl_HandleFunctionOrImportIndex(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, bool body_follows)
{
    if (PP::kMain == _pp)
    {
        int func_startoffs;
        ErrorType retval = ParseFuncdecl_EnterAsImportOrFunc(name_of_func, body_follows, tqs[TQ::kImport], _sym.NumOfFuncParams(name_of_func), func_startoffs);
        if (retval < 0) return retval;
        _sym[name_of_func].FunctionD->Offset = func_startoffs;
    }

    if (!tqs[TQ::kImport])
        return kERR_None;

    // Imported functions
    _sym[name_of_func].FunctionD->TypeQualifiers[TQ::kImport] = true;
    // Import functions have an index into the imports[] array in lieu of a start offset.
    auto const imports_idx = _sym[name_of_func].FunctionD->Offset;

    _sym[name_of_func].FunctionD->TypeQualifiers[TQ::kImport] = true;

    if (PP::kPreAnalyze == _pp)
    {
        _sym[name_of_func].FunctionD->Offset = kFT_Import;
        return kERR_None;
    }

    if (struct_of_func > 0)
    {
        char appendage[10];
        
        sprintf(appendage, "^%d", _sym.NumOfFuncParams(name_of_func) + 100 * _sym[name_of_func].FunctionD->IsVariadic);
        strcat(_scrip.imports[imports_idx], appendage);
    }

    _fim.SetFuncCallpoint(name_of_func, imports_idx);
    return kERR_None;
}

// We're at something like "int foo(", directly before the "("
// This might or might not be within a struct defn
// An extender func param, if any, has already been resolved
AGS::ErrorType AGS::Parser::ParseFuncdecl(size_t declaration_start, TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_func, Symbol name_of_func, bool no_loop_check, bool &body_follows)
{
    ErrorType retval = ParseFuncdecl_DoesBodyFollow(body_follows);
    if (retval < 0) return retval;

    retval = ParseFuncdecl_Checks(tqs, struct_of_func, name_of_func, return_vartype, body_follows, no_loop_check);
    if (retval < 0) return retval;
   
    // A forward decl can be written with the
    // "import" keyword (when allowed in the options). This isn't an import
    // proper, so reset the "import" flag in this case.
    if (tqs[TQ::kImport] &&   // This declaration has 'import'
        _sym.IsFunction(name_of_func) &&
        !_sym[name_of_func].FunctionD->TypeQualifiers[TQ::kImport]) // but symbol table hasn't 'import'
    {
        if (FlagIsSet(_options, SCOPT_NOIMPORTOVERRIDE))
        {
            std::string const msg = ReferenceMsgSym(
                "In here, a function with a local body must not have an \"import\" declaration",
                name_of_func);
            Error(msg.c_str());
            return kERR_UserError;
        }
        tqs[TQ::kImport] = false;
    }

    if (PP::kMain == _pp && body_follows)
    {
        // All the parameters that will be defined as local variables go on nesting level 1.
        _nest.Push(NSType::kParameters);
        // When this function is called, first all the parameters are pushed on the stack
        // and then the address to which the function should return after it has finished.
        // So the first parameter isn't on top of the stack but one address below that
        _scrip.OffsetToLocalVarBlock += SIZE_OF_STACK_CELL;
    }

    // Stash away the known info about the function so that we can check whether this declaration is compatible
    SymbolTableEntry::FunctionDesc *known_info = _sym[name_of_func].FunctionD;
    _sym[name_of_func].FunctionD = nullptr;
    size_t const known_declared = _sym.GetDeclared(name_of_func);

    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, struct_of_func, name_of_func, body_follows);
    retval = ParseFuncdecl_Paramlist(name_of_func, body_follows);
    if (retval < 0) return retval;

    retval = ParseFuncdecl_CheckThatKnownInfoMatches(_sym.GetName(name_of_func), _sym[name_of_func].FunctionD, known_info, known_declared, body_follows);
    if (retval < 0) return retval;

    // copy the default values from the function prototype into the symbol table
    if (known_info)
    {
        auto &func_parameters = _sym[name_of_func].FunctionD->Parameters;
        auto const &known_parameters = known_info->Parameters;
        for (size_t parameters_idx = 0; parameters_idx < func_parameters.size(); ++parameters_idx)
            func_parameters[parameters_idx].Default = known_parameters[parameters_idx].Default;
    }

    delete known_info;

    retval = ParseFuncdecl_HandleFunctionOrImportIndex(tqs, struct_of_func, name_of_func, body_follows);
    if (retval < 0) return retval;

    _sym.SetDeclared(name_of_func, declaration_start);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::IndexOfLeastBondingOperator(SrcList &expression, int &idx)
{
    size_t nesting_depth = 0;

    int largest_prio_found = INT_MIN; // note: largest number == lowest priority
    bool largest_is_binary = true;
    int index_of_largest_prio = -1;
    bool encountered_operand = false;

    expression.StartRead();
    while (!expression.ReachedEOF())
    {
        Symbol const current_sym = expression.GetNext();

        if (kKW_CloseBracket == current_sym ||
            kKW_CloseParenthesis == current_sym)
        {
            encountered_operand = true;
            if (nesting_depth > 0)
                nesting_depth--;
            continue;
        }
        else if (kKW_OpenBracket == current_sym ||
            kKW_OpenParenthesis == current_sym)
        {
            nesting_depth++;
            continue;
        }
        else if (!_sym.IsOperator(current_sym))
        {
            encountered_operand = true;
        }

        // Continue if we aren't at zero nesting depth, since ()[] take priority
        if (nesting_depth > 0)
            continue;

        if (!_sym.IsOperator(current_sym))
            continue;

        // a binary operator has an operand to its left
        bool const is_binary = encountered_operand;
        encountered_operand = false;

        Symbol const current_op = current_sym;
        int const current_prio =
            is_binary ? _sym.BinaryOpPrio(current_op) : _sym.UnaryOpPrio(current_op);
        if (current_prio < 0)
        {
            Error(
                "'%s' cannot be used as %s operator",
                _sym.GetName(current_op).c_str(),
                is_binary ? "binary" : "unary");
            return kERR_UserError;
        }
        if (current_prio < largest_prio_found)
            continue; // can't be lowest priority

        // remember this and continue looking
        largest_prio_found = current_prio;
        // The cursor has already moved to the next symbol, so the index is one less
        index_of_largest_prio = expression.GetCursor() - 1;
        largest_is_binary = is_binary;
    }

    // unary operators are prefix, so if the least binding operator
    // turns out to be unary and not in first position, it must be
    // a chain of unary operators and the first should be evaluated
    // first
    idx = largest_is_binary ? index_of_largest_prio : 0;
    return kERR_None;
}

// Change the generic opcode to the one that is correct for the vartypes
// Also check whether the operator can handle the types at all
AGS::ErrorType AGS::Parser::GetOpcodeValidForVartype(Vartype vartype1, Vartype vartype2, CodeCell &opcode)
{
    if (kKW_Float == vartype1 || kKW_Float == vartype2)
    {
        switch (opcode)
        {
        default:
            Error("The operator cannot be applied to float values");
            return kERR_UserError;
        case SCMD_ADD:      opcode = SCMD_FADD; break;
        case SCMD_ADDREG:   opcode = SCMD_FADDREG; break;
        case SCMD_DIVREG:   opcode = SCMD_FDIVREG; break;
        case SCMD_GREATER:  opcode = SCMD_FGREATER; break;
        case SCMD_GTE:      opcode = SCMD_FGTE; break;
        case SCMD_ISEQUAL:  break;
        case SCMD_LESSTHAN: opcode = SCMD_FLESSTHAN; break;
        case SCMD_LTE:      opcode = SCMD_FLTE; break;
        case SCMD_MULREG:   opcode = SCMD_FMULREG; break;
        case SCMD_NOTEQUAL: break;
        case SCMD_SUB:      opcode = SCMD_FSUB; break;
        case SCMD_SUBREG:   opcode = SCMD_FSUBREG; break;
        }
        return kERR_None;
    }

    bool const iatos1 = _sym.IsAnyStringVartype(vartype1);
    bool const iatos2 = _sym.IsAnyStringVartype(vartype2);

    if (iatos1 || iatos2)
    {
        switch (opcode)
        {
        default:
            Error("Operator cannot be applied to string type values");
            return kERR_UserError;
        case SCMD_ISEQUAL:  opcode = SCMD_STRINGSEQUAL; break;
        case SCMD_NOTEQUAL: opcode = SCMD_STRINGSNOTEQ; break;
        }
        if (kKW_Null == vartype1 || kKW_Null == vartype2)
            return kERR_None;

        if (iatos1 != iatos2)
        {
            Error("A string type value cannot be compared to a value that isn't a string type");
            return kERR_UserError;
        }
        return kERR_None;
    }

    if (((_sym.IsDynpointerVartype(vartype1) || kKW_Null == vartype1) &&
        (_sym.IsDynpointerVartype(vartype2) || kKW_Null == vartype2)) ||
        ((_sym.IsDynarrayVartype(vartype1) || kKW_Null == vartype1) &&
        (_sym.IsDynarrayVartype(vartype2) || kKW_Null == vartype2)))
    {
        switch (opcode)
        {
        default:
            Error("The operator cannot be applied to managed types");
            return kERR_UserError;
        case SCMD_ISEQUAL:  return kERR_None;
        case SCMD_NOTEQUAL: return kERR_None;
        }
    }

    // Other combinations of managed types won't mingle
    if (_sym.IsDynpointerVartype(vartype1) || _sym.IsDynpointerVartype(vartype2))
    {
        Error("The operator cannot be applied to values of these types");
        return kERR_UserError;
    }

    ErrorType retval = IsVartypeMismatch(vartype1, kKW_Int, true);
    if (retval < 0) return retval;
    return IsVartypeMismatch(vartype2, kKW_Int, true);
}

// Check for a type mismatch in one direction only
bool AGS::Parser::IsVartypeMismatch_Oneway(Vartype vartype_is, Vartype vartype_wants_to_be) const
{
    // cannot convert 'void' to anything
    if (kKW_Void == vartype_is || kKW_Void == vartype_wants_to_be)
        return true;

    // Don't convert if no conversion is called for
    if (vartype_is == vartype_wants_to_be)
        return false;


    // Can convert null to dynpointer or dynarray
    if (kKW_Null == vartype_is)
        return
            !_sym.IsDynpointerVartype(vartype_wants_to_be) &&
            !_sym.IsDynarrayVartype(vartype_wants_to_be);

    // can convert String * to const string
    if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_is) &&
        kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be))
    {
        return false;
    }

    // can convert string or const string to String *
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_is) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be))
    {
        return false;
    }

    // Note: CanNOT convert String * or const string to string;
    // a function that has a string parameter may modify it, but a String or const string may not be modified.

    if (_sym.IsOldstring(vartype_is) != _sym.IsOldstring(vartype_wants_to_be))
        return true;

    // Note: the position of this test is important.
    // Don't "group" string tests "together" and move this test above or below them.
    // cannot convert const to non-const
    if (_sym.IsConstVartype(vartype_is) && !_sym.IsConstVartype(vartype_wants_to_be))
        return true;

    if (_sym.IsOldstring(vartype_is))
        return false;

    // From here on, don't mind constness or dynarray-ness
    vartype_is = _sym.VartypeWithout(VTT::kConst, vartype_is);
    vartype_is = _sym.VartypeWithout(VTT::kDynarray, vartype_is);
    vartype_wants_to_be = _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be);
    vartype_wants_to_be = _sym.VartypeWithout(VTT::kDynarray, vartype_wants_to_be);

    // floats cannot mingle with other types
    if ((vartype_is == kKW_Float) != (vartype_wants_to_be == kKW_Float))
        return true;

    // Can convert short, char etc. into int
    if (_sym.IsAnyIntegerVartype(vartype_is) && kKW_Int == vartype_wants_to_be)
        return false;

    // Checks to do if at least one is dynarray
    if (_sym.IsDynarrayVartype(vartype_is) || _sym.IsDynarrayVartype(vartype_wants_to_be))
    {
        // BOTH sides must be dynarray 
        if (_sym.IsDynarrayVartype(vartype_is) != _sym.IsDynarrayVartype(vartype_wants_to_be))
            return false;

        // The underlying core vartypes must be identical:
        // A dynarray contains a sequence of elements whose size are used
        // to index the individual element, so no extending elements
        Symbol const target_core_vartype = _sym.VartypeWithout(VTT::kDynarray, vartype_wants_to_be);
        Symbol const current_core_vartype = _sym.VartypeWithout(VTT::kDynarray, vartype_is);
        return current_core_vartype != target_core_vartype;
    }

    // Checks to do if at least one is dynpointer
    if (_sym.IsDynpointerVartype(vartype_is) || _sym.IsDynpointerVartype(vartype_wants_to_be))
    {
        // BOTH sides must be dynpointer
        if (_sym.IsDynpointerVartype(vartype_is) != _sym.IsDynpointerVartype(vartype_wants_to_be))
            return true;

        // Core vartypes need not be identical here: check against extensions
        Symbol const target_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be);
        Symbol current_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_is);
        while (current_core_vartype != target_core_vartype)
        {
            current_core_vartype = _sym[current_core_vartype].VartypeD->Parent;
            if (current_core_vartype == 0)
                return true;
        }
        return false;
    }

    // Checks to do if at least one is a struct or an array
    if (_sym.IsStructVartype(vartype_is) || _sym.IsStructVartype(vartype_wants_to_be) ||
        _sym.IsArrayVartype(vartype_is) || _sym.IsArrayVartype(vartype_wants_to_be))
        return (vartype_is != vartype_wants_to_be);

    return false;
}

// Check whether there is a type mismatch; if so, give an error
AGS::ErrorType AGS::Parser::IsVartypeMismatch(Vartype vartype_is, Vartype vartype_wants_to_be, bool orderMatters)
{
    if (!IsVartypeMismatch_Oneway(vartype_is, vartype_wants_to_be))
        return kERR_None;
    if (!orderMatters && !IsVartypeMismatch_Oneway(vartype_wants_to_be, vartype_is))
        return kERR_None;


    Error(
        "Type mismatch: cannot convert '%s' to '%s'",
        _sym.GetName(vartype_is).c_str(),
        _sym.GetName(vartype_wants_to_be).c_str());
    return kERR_UserError;
}

// returns whether the vartype of the opcode is always bool
bool AGS::Parser::IsBooleanOpcode(CodeCell opcode)
{
    if (opcode >= SCMD_ISEQUAL && opcode <= SCMD_OR)
        return true;

    if (opcode >= SCMD_FGREATER && opcode <= SCMD_FLTE)
        return true;

    if (opcode == SCMD_STRINGSNOTEQ || opcode == SCMD_STRINGSEQUAL)
        return true;

    return false;
}

// If we need a String but AX contains a string, 
// then convert AX into a String object and set its type accordingly
void AGS::Parser::ConvertAXStringToStringObject(Vartype wanted_vartype)
{
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, _scrip.AX_Vartype) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, wanted_vartype))
    {
        WriteCmd(SCMD_CREATESTRING, SREG_AX); // convert AX
        _scrip.AX_Vartype = _sym.VartypeWith(VTT::kDynpointer, _sym.GetStringStructSym());
    }
}

int AGS::Parser::GetReadCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMREAD;
    case 1:  return SCMD_MEMREADB;
    case 2:  return SCMD_MEMREADW;
    }
}

int AGS::Parser::GetWriteCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMWRITE;
    case 1:  return SCMD_MEMWRITEB;
    case 2:  return SCMD_MEMWRITEW;
    }
}

AGS::ErrorType AGS::Parser::HandleStructOrArrayResult(Vartype &vartype,Parser::ValueLocation &vloc)
{
    if (_sym.IsArrayVartype(vartype))
    {
        Error("Cannot access array as a whole (did you forget to add \"[0]\"?)");
        return kERR_UserError;
    }

    if (_sym.IsAtomicVartype(vartype) && _sym.IsStructVartype(vartype))
    {
        if (_sym.IsManagedVartype(vartype))
        {
            // Interpret the memory address as the result
            vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
            WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
            vloc = kVL_AX_is_value;
            _scrip.AX_Vartype = vartype;
            return kERR_None;
        }

        Error("Cannot access non-managed struct as a whole");
        return kERR_UserError;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ResultToAX(ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    if (kVL_MAR_pointsto_value != vloc)
        return kERR_None; // So it's already in AX 

    _scrip.AX_Vartype = vartype;
    _scrip.AX_ScopeType = scope_type;

    if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype))
        WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
    else
        WriteCmd(
            _sym.IsDynVartype(vartype) ? SCMD_MEMREADPTR : GetReadCommandForSize(_sym.GetSize(vartype)),
            SREG_AX);
    vloc = kVL_AX_is_value;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseExpression_CheckArgOfNew(Vartype argument_vartype)
{
    if (!_sym.IsVartype(argument_vartype))
    {
        Error("Expected a type after 'new', found '%s' instead", _sym.GetName(argument_vartype).c_str());
        return kERR_UserError;
    }

    if (_sym[argument_vartype].VartypeD->Flags[VTF::kUndefined])
    {
        Error(
            ReferenceMsgSym("The struct '%s' hasn't been completely defined yet", argument_vartype).c_str(),
            _sym.GetName(argument_vartype).c_str());
        return kERR_UserError;
    }

    if (!_sym.IsAnyIntegerVartype(argument_vartype) && kKW_Float != argument_vartype && !_sym.IsManagedVartype(argument_vartype))
    {
        Error("Can only use integer types or 'float' or managed types with 'new'");
        return kERR_UserError;
    }

    // Note: While it is an error to use a built-in type with new, it is
    // allowed to use a built-in type with new[].
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseExpression_New(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    expression.StartRead();
    expression.GetNext(); // Eat "new"

    if (expression.ReachedEOF())
    {
        Error("Expected a type after 'new' but didn't find any");
        return kERR_UserError;
    }
    Vartype const argument_vartype = expression.GetNext();

    ErrorType retval = ParseExpression_CheckArgOfNew(argument_vartype);
    if (retval < 0) return retval;

    bool const is_managed = _sym.IsManagedVartype(argument_vartype);
    bool const with_bracket_expr = !expression.ReachedEOF(); // "new FOO[BAR]"

    Vartype element_vartype = 0;
    if (with_bracket_expr)
    {
        // Note that in AGS, you can write "new Struct[]" but what you mean then is "new Struct*[]".
        retval = EatDynpointerSymbolIfPresent(argument_vartype);
        if (retval < 0) return retval;

        retval = AccessData_ReadBracketedIntExpression(expression);
        if (retval < 0) return retval;
        element_vartype = is_managed ? _sym.VartypeWith(VTT::kDynpointer, argument_vartype) : argument_vartype;
        vartype = _sym.VartypeWith(VTT::kDynarray, element_vartype);
    }
    else
    {
        if (_sym.IsBuiltinVartype(argument_vartype))
        {   
            Error("Expected '[' after the built-in type '%s'", _sym.GetName(argument_vartype).c_str());
            return kERR_UserError;
        }
        if (!is_managed)
        {
            Error("Expected '[' after the integer type '%s'", _sym.GetName(argument_vartype).c_str());
            return kERR_UserError;
        }
        element_vartype = argument_vartype;
        vartype = _sym.VartypeWith(VTT::kDynpointer, argument_vartype);
    }

    size_t const element_size = _sym.GetSize(element_vartype);
    if (0 == element_size)
    {   // The Engine really doesn't like that (division by zero error)
        Error("!Trying to emit allocation of zero dynamic memory");
        return kERR_InternalError;
    }

    if (with_bracket_expr)
        WriteCmd(SCMD_NEWARRAY, SREG_AX, element_size, is_managed);
    else
        WriteCmd(SCMD_NEWUSEROBJECT, SREG_AX, element_size);

    _scrip.AX_ScopeType = scope_type = ScT::kGlobal;
    _scrip.AX_Vartype = vartype;
    vloc = kVL_AX_is_value;
    return kERR_None;
}

// We're parsing an expression that starts with '-' (unary minus)
AGS::ErrorType AGS::Parser::ParseExpression_UnaryMinus(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    if (expression.Length() < 2)
    {
        Error(
            "Expected a term after '%s' but didn't find any",
            _sym.GetName(expression[0]).c_str());
        return kERR_UserError;
    }

    expression.EatFirstSymbol(); // Eat '-'
    if (expression.Length() == 1)
    {
        expression.StartRead();
        Symbol const peek = expression.PeekNext();
        if (_sym.IsConstant(peek) || _sym.IsLiteral(peek))
        {
            Symbol lit;
            ErrorType retval = ReadLiteralOrConst(expression, lit);
            if (retval < 0) return retval;
            scope_type = ScT::kGlobal;
            NegateLiteral(lit);
            return EmitLiteral(lit, vloc, vartype);
        }
    };

    // parse the rest of the expression into AX
    ErrorType retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    CodeCell opcode = SCMD_SUBREG; 
    retval = GetOpcodeValidForVartype(_scrip.AX_Vartype, _scrip.AX_Vartype, opcode);
    if (retval < 0) return retval;

    // Calculate 0 - AX
    // The binary representation of 0.0 is identical to the binary representation of 0
    // so this will work for floats as well as for ints.
    WriteCmd(SCMD_LITTOREG, SREG_BX, 0);
    WriteCmd(opcode, SREG_BX, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_AX_is_value;
    return kERR_None;
}

// We're parsing an expression that starts with '+' (unary minus)
AGS::ErrorType AGS::Parser::ParseExpression_UnaryPlus(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    if (expression.Length() < 2)
    {
        Error(
            "Expected a term after '%s' but didn't find any",
            _sym.GetName(expression[0]).c_str());
        return kERR_UserError;
    }

    expression.EatFirstSymbol(); // Eat '+'
    if (expression.Length() == 1)
    {
        expression.StartRead();
        Symbol const peek = expression.PeekNext();
        if (_sym.IsConstant(peek) || _sym.IsLiteral(peek))
        {
            Symbol lit;
            ErrorType retval = ReadLiteralOrConst(expression, lit);
            if (retval < 0) return retval;
            scope_type = ScT::kGlobal;
            return EmitLiteral(lit, vloc, vartype);
        }
    };

    ErrorType retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    if (_sym.IsAnyIntegerVartype(vartype) || kKW_Float == vartype)
        return kERR_None;

    Error("Cannot apply unary '+' to an expression of type '%s'", _sym.GetName(vartype));
    return kERR_UserError;
}

// We're parsing an expression that starts with '!' (boolean NOT) or '~' (bitwise Negate)
AGS::ErrorType AGS::Parser::ParseExpression_Negate(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    Symbol const op_sym = expression[0];
    if (expression.Length() < 2)
    {
        Error(
            "Expected a term after '%s' but didn't find any",
            _sym.GetName(op_sym).c_str());
        return kERR_UserError;
    }

    SrcList after_not = SrcList(expression, 1, expression.Length() - 1);
    ErrorType retval = ParseExpression_Term(after_not, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    if (!_sym.IsAnyIntegerVartype(_scrip.AX_Vartype))
    {
        Error(
            "Expected an integer expression after '%s' but found type %s",
            _sym.GetName(op_sym).c_str(),
            _sym.GetName(_scrip.AX_Vartype).c_str());
        return kERR_UserError;
    }
    
    bool const bitwise_negation = (0 != _sym.GetName(op_sym).compare("!"));
    if (bitwise_negation)
    {
        // There isn't any opcode for this, so calculate -1 - AX
        WriteCmd(SCMD_LITTOREG, SREG_BX, -1);
        WriteCmd(SCMD_SUBREG, SREG_BX, SREG_AX);
        WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    }
    else
    {
        WriteCmd(SCMD_NOTREG, SREG_AX);
    }

    vloc = kVL_AX_is_value;
    vartype = _scrip.AX_Vartype = kKW_Int;
    return kERR_None;
}

// The least binding operator is the first thing in the expression
// This means that the op must be an unary op.
AGS::ErrorType AGS::Parser::ParseExpression_Unary(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    switch (expression[0])
    {
    default:
        Error("Can't use '%s' as a prefix operator", _sym.GetName(expression[0]).c_str());
        return kERR_UserError;

    case kKW_BitNeg:
    case kKW_Not:
        return ParseExpression_Negate(expression, vloc, scope_type, vartype);

    case kKW_Minus:
        return ParseExpression_UnaryMinus(expression, vloc, scope_type, vartype);

    case kKW_New:
        return ParseExpression_New(expression, vloc, scope_type, vartype);

    case kKW_Plus:
        return ParseExpression_UnaryPlus(expression, vloc, scope_type, vartype);
    }
    // Can't reach.
    return kERR_None;

}

// The least binding operator is '?'
AGS::ErrorType AGS::Parser::ParseExpression_Ternary(size_t tern_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    // First term ends before the '?'
    SrcList term1 = SrcList(expression, 0, tern_idx);

    // Second term begins after the '?', we don't know how long it is yet
    SrcList after_term1 = SrcList(expression, tern_idx + 1, expression.Length() - (tern_idx + 1));

    // Find beginning of third term
    after_term1.StartRead();
    SkipTo(SymbolList{ kKW_Colon }, after_term1);
    if (after_term1.ReachedEOF() || kKW_Colon != after_term1.PeekNext())
    {
        expression.SetCursor(tern_idx);
        Error("Didn't find the matching ':' to '?'");
        return kERR_UserError;
    }
    size_t const term3_start = after_term1.GetCursor() + 1;
    SrcList term3 = SrcList(after_term1, term3_start, after_term1.Length() - term3_start);
    SrcList term2 = SrcList(after_term1, 0u, after_term1.GetCursor());

    Vartype term1_vartype, term2_vartype, term3_vartype;
    ScopeType term1_scope_type, term2_scope_type, term3_scope_type;

    // First term of ternary
    ErrorType retval = ParseExpression_Term(term1, vloc, term1_scope_type, term1_vartype);
    if (retval < 0) return retval;
    ResultToAX(vloc, term1_scope_type, term1_vartype);
    if (!term1.ReachedEOF())
    {
        Error("!Unexpected '%s' after 1st term of ternary", _sym.GetName(term1.GetNext()).c_str());
        return kERR_InternalError;
    }

    // We jump either to the start of the third term or to the end of the ternary
    // expression. We don't know where this is yet, thus -77. This is just a
    // random number that's easy to spot in debugging outputs (where it's a clue
    // that it probably hasn't been replaced by a proper value). Don't use for anything.
    WriteCmd(
        (term2.Length() > 0) ? SCMD_JZ : SCMD_JNZ,
        -77);
    ForwardJump test_jumpdest(_scrip);
    test_jumpdest.AddParam();

    // Second term of ternary
    bool const second_term_exists = (term2.Length() > 0);
    if (second_term_exists)
    {
        retval = ParseExpression_Term(term2, vloc, term2_scope_type, term2_vartype);
        if (retval < 0) return retval;
        if (!term2.ReachedEOF())
        {
            Error("!Unexpected '%s' after 1st term of ternary", _sym.GetName(term2.GetNext()).c_str());
            return kERR_InternalError;
        }
        ResultToAX(vloc, term2_scope_type, term2_vartype);
        if (_sym.IsAnyStringVartype(term2_vartype))
        {
            ConvertAXStringToStringObject(_sym.GetStringStructSym());
            term2_vartype = _scrip.AX_Vartype;
        }
        // Jump to the end of the ternary expression;
        // We don't know the dest yet, thus the placeholder value -77. Don't
        // test for this random magic number or use it in code
        WriteCmd(SCMD_JMP, -77);
    }
    else
    {
        // Take the first expression as the result of the missing second expression
        // No code is generated; instead, the conditional jump after the test goes
        // to the end of the expression if the test does NOT yield zero
        term2_vartype = term1_vartype;
        term2_scope_type = term1_scope_type;
        if (_sym.IsAnyStringVartype(term2_vartype))
        {
            ConvertAXStringToStringObject(_sym.GetStringStructSym());
            term2_vartype = _scrip.AX_Vartype;
        }
    }
    ForwardJump jumpdest_after_term2(_scrip); // only valid if second_term_exists
    jumpdest_after_term2.AddParam();

    // Third term of ternary
    if (0 == term3.Length())
    {
        expression.SetCursor(tern_idx);
        Error("The third expression of this ternary is empty");
        return kERR_UserError;
    }
    if (second_term_exists)
        test_jumpdest.Patch(_src.GetLineno());

    retval = ParseExpression_Term(term3, vloc, term3_scope_type, term3_vartype);
    if (retval < 0) return retval;
    ResultToAX(vloc, term3_scope_type, term3_vartype);
    if (_sym.IsAnyStringVartype(term3_vartype))
    {
        ConvertAXStringToStringObject(_sym.GetStringStructSym());
        term3_vartype = _scrip.AX_Vartype;
    }

    if (second_term_exists)
        jumpdest_after_term2.Patch(_src.GetLineno());
    else
        test_jumpdest.Patch(_src.GetLineno());

    scope_type =
        (ScT::kLocal == term2_scope_type || ScT::kLocal == term3_scope_type) ?
        ScT::kLocal : ScT::kGlobal;

    if (!IsVartypeMismatch_Oneway(term2_vartype, term3_vartype))
    {
        vartype = _scrip.AX_Vartype = term3_vartype;
        return kERR_None;
    }
    if (!IsVartypeMismatch_Oneway(term3_vartype, term2_vartype))
    {
        vartype = _scrip.AX_Vartype = term2_vartype;
        return kERR_None;
    }

    term3.SetCursor(0);
    Error("An expression of type '%s' is incompatible with an expression of type '%s'",
        _sym.GetName(term2_vartype).c_str(), _sym.GetName(term3_vartype).c_str());
    return kERR_UserError;
}

// The least binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
AGS::ErrorType AGS::Parser::ParseExpression_Binary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    // process the left hand side
    // This will be in vain if we find out later on that there isn't any right hand side,
    // but doing the left hand side first means that any errors will be generated from left to right
    Vartype vartype_lhs = 0;
    SrcList lhs = SrcList(expression, 0, op_idx);
    ErrorType retval = ParseExpression_Term(lhs, vloc, scope_type, vartype_lhs);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype_lhs);
    if (retval < 0) return retval;
    if (!lhs.ReachedEOF())
    {
        Error("!Unexpected '%s' after LHS of binary expression", _sym.GetName(lhs.GetNext()).c_str());
        return kERR_InternalError;
    }

    ForwardJump to_exit(_scrip);
    Symbol const operator_sym = expression[op_idx];
    int const opcode = _sym[operator_sym].OperatorD->Opcode;

    if (SCMD_AND == opcode)
    {
        // "&&" operator lazy evaluation: if AX is 0 then the AND has failed, 
        // so just jump directly past the AND instruction;
        // AX will still be 0 so that will do as the result of the calculation
        WriteCmd(SCMD_JZ, -77);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        to_exit.AddParam();
    }
    else if (SCMD_OR == opcode)
    {
        // "||" operator lazy evaluation: if AX is non-zero then the OR has succeeded, 
        // so just jump directly past the OR instruction; 
        // AX will still be non-zero so that will do as the result of the calculation
        WriteCmd(SCMD_JNZ, -77);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        to_exit.AddParam();
    }

    PushReg(SREG_AX);
    SrcList rhs = SrcList(expression, op_idx + 1, expression.Length());
    if (0 == rhs.Length())
    {
        // there is no right hand side for the expression
        Error("Binary operator '%s' doesn't have a right hand side", _sym.GetName(expression[op_idx]).c_str());
        return kERR_UserError;
    }

    retval = ParseExpression_Term(rhs, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    PopReg(SREG_BX); // Note, we pop to BX although we have pushed AX
    // now the result of the left side is in BX, of the right side is in AX

    // Check whether the left side type and right side type match either way
    retval = IsVartypeMismatch(vartype_lhs, vartype, false);
    if (retval < 0) return retval;

    int actual_opcode = opcode;
    retval = GetOpcodeValidForVartype(vartype_lhs, vartype, actual_opcode);
    if (retval < 0) return retval;

    WriteCmd(actual_opcode, SREG_BX, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_AX_is_value;

    to_exit.Patch(_src.GetLineno());

    // Operators like == return a bool (in our case, that's an int);
    // other operators like + return the type that they're operating on
    if (IsBooleanOpcode(actual_opcode))
        _scrip.AX_Vartype = vartype = kKW_Int;

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseExpression_BinaryOrTernary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    Symbol const operator_sym = expression[op_idx];
    if (kKW_Tern == operator_sym)
        return ParseExpression_Ternary(op_idx, expression, vloc, scope_type, vartype);
    return ParseExpression_Binary(op_idx, expression, vloc, scope_type, vartype);
}

AGS::ErrorType AGS::Parser::ParseExpression_InParens(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    // find the corresponding closing parenthesis
    size_t const bp_start = 1;
    expression.SetCursor(bp_start); // Skip the '('
    SkipTo(SymbolList{}, expression);
    size_t const bp_end = expression.GetCursor();
    
    SrcList between_parens = SrcList(expression, bp_start, bp_end - bp_start);
    ErrorType retval = ParseExpression_Term(between_parens, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    if (!between_parens.ReachedEOF())
    {
        Error("Expected ')', found '%s' instead.", _sym.GetName(between_parens.GetNext()).c_str());
        return kERR_UserError;
    }

    expression.GetNext(); // Eat ')'
    return kERR_None;
}

// We're in the parameter list of a function call, and we have less parameters than declared.
// Provide defaults for the missing values
AGS::ErrorType AGS::Parser::AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args,Symbol funcSymbol, bool func_is_import)
{
    for (size_t arg_idx = num_func_args; arg_idx > num_supplied_args; arg_idx--)
    {
        Symbol const param_default = _sym[funcSymbol].FunctionD->Parameters[arg_idx].Default;
        if (kKW_NoSymbol == param_default)
        {
            Error("Function call parameter #%d isn't provided and doesn't have any default value", arg_idx);
            return kERR_UserError;
        }
        if (!_sym.IsLiteral(param_default))
        {
            Error("!Parameter default symbol isn't literal");
            return kERR_InternalError;
        }

        // push the default value onto the stack
        WriteCmd(
            SCMD_LITTOREG,
            SREG_AX,
            _sym[param_default].LiteralD->Value);

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
    }
    return kERR_None;
}

void AGS::Parser::DoNullCheckOnStringInAXIfNecessary(Vartype valTypeTo)
{

    if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, _scrip.AX_Vartype) &&
        kKW_String == _sym.VartypeWithout(VTT::kConst, valTypeTo))
        WriteCmd(SCMD_CHECKNULLREG, SREG_AX);
}

std::string const AGS::Parser::ReferenceMsgLoc(std::string const &msg, size_t declared)
{
    if (SymbolTable::kNoSrcLocation == declared)
        return msg;

    int const section_id = _src.GetSectionIdAt(declared);
    std::string const &section = _src.SectionId2Section(section_id);

    int const line = _src.GetLinenoAt(declared);
    if (line <= 0 || (!section.empty() && '_' == section[0]))
        return msg;

    std::string tpl;
    if (_src.GetSectionId() != section_id)
        tpl = ". See <1> line <2>";
    else if (_src.GetLineno() != line)
        tpl = ". See line <2>";
    else
        tpl = ". See the current line";
    size_t const loc1 = tpl.find("<1>");
    if (std::string::npos != loc1)
        tpl.replace(tpl.find("<1>"), 3, section);
    size_t const loc2 = tpl.find("<2>");
    if (std::string::npos != loc2)
        tpl.replace(tpl.find("<2>"), 3, std::to_string(line));
    return msg + tpl;
}

std::string const AGS::Parser::ReferenceMsgSym(std::string const &msg,Symbol symb)
{
    return ReferenceMsgLoc(msg, _sym.GetDeclared(symb));
}

AGS::ErrorType AGS::Parser::AccessData_FunctionCall_PushParams(SrcList &parameters, size_t closed_paren_idx, size_t num_func_args, size_t num_supplied_args,Symbol funcSymbol, bool func_is_import)
{
    size_t param_num = num_supplied_args + 1;
    size_t start_of_current_param = 0;
    int end_of_current_param = closed_paren_idx;  // can become < 0, points to (last symbol of parameter + 1)
    // Go backwards through the parameters since they must be pushed that way
    do
    {
        // Find the start of the next parameter
        param_num--;
        int paren_nesting_depth = 0;
        for (size_t paramListIdx = end_of_current_param - 1; true; paramListIdx--)
        {
            // going backwards so ')' increases the depth level
            Symbol const &idx = parameters[paramListIdx];
            if (kKW_CloseParenthesis == idx)
                paren_nesting_depth++;
            if (kKW_OpenParenthesis == idx)
                paren_nesting_depth--;
            if ((paren_nesting_depth == 0 && kKW_Comma == idx) ||
                (paren_nesting_depth < 0 && kKW_OpenParenthesis == idx))
            {
                start_of_current_param = paramListIdx + 1;
                break;
            }
            if (paramListIdx == 0)
                break; // Don't put this into the for header!
        }

        if (end_of_current_param < 0 || static_cast<size_t>(end_of_current_param) < start_of_current_param)
        {   
            Error("!Parameter length is negative");
            return kERR_InternalError;
        }

        // Compile the parameter
        ValueLocation vloc;
        ScopeType scope_type;
        Vartype vartype;

        SrcList current_param = SrcList(parameters, start_of_current_param, end_of_current_param - start_of_current_param);
        ErrorType retval = ParseExpression_Term(current_param, vloc, scope_type, vartype);
        if (retval < 0) return retval;
        retval = ResultToAX(vloc, scope_type, vartype);
        if (retval < 0) return retval;

        if (param_num <= num_func_args) // we know what type to expect
        {
            // If we need a string object ptr but AX contains a normal string, convert AX
            Vartype const param_vartype = _sym[funcSymbol].FunctionD->Parameters[param_num].Vartype;
            ConvertAXStringToStringObject(param_vartype);
            vartype = _scrip.AX_Vartype;
            // If we need a normal string but AX contains a string object ptr, 
            // check that this ptr isn't null
            DoNullCheckOnStringInAXIfNecessary(param_vartype);

            if (IsVartypeMismatch(vartype, param_vartype, true))
                return kERR_UserError;
        }

        // Note: We push the parameters, which is tantamount to writing them
        // into memory with SCMD_MEMWRITE. The called function will use them
        // as local variables. However, if a parameter is managed, then its 
        // memory must be written with SCMD_MEMWRITEPTR, not SCMD_MEMWRITE 
        // as we do here. So to compensate, the called function will have to 
        // read each pointer variable with SCMD_MEMREAD and then write it
        // back with SCMD_MEMWRITEPTR.

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);

        end_of_current_param = start_of_current_param - 1;
    }
    while (end_of_current_param > 0);

    return kERR_None;
}


// Count parameters, check that all the parameters are non-empty; find closing paren
AGS::ErrorType AGS::Parser::AccessData_FunctionCall_CountAndCheckParm(SrcList &parameters,Symbol name_of_func, size_t &index_of_close_paren, size_t &num_supplied_args)
{
    size_t paren_nesting_depth = 1;
    num_supplied_args = 1;
    size_t param_idx;
    bool found_param_symbol = false;

    for (param_idx = 1; param_idx < parameters.Length(); param_idx++)
    {
        Symbol const &idx = parameters[param_idx];

        if (kKW_OpenParenthesis == idx)
            paren_nesting_depth++;
        if (kKW_CloseParenthesis == idx)
        {
            paren_nesting_depth--;
            if (paren_nesting_depth == 0)
                break;
        }

        if (paren_nesting_depth == 1 && kKW_Comma == idx)
        {
            num_supplied_args++;
            if (found_param_symbol)
                continue;

            Error("Argument %d in function call is empty", num_supplied_args - 1);
            return kERR_UserError;
        }
        found_param_symbol = true;
    }

    // Special case: "()" means 0 arguments
    if (num_supplied_args == 1 &&
        parameters.Length() > 1 &&
        kKW_CloseParenthesis == parameters[1])
    {
        num_supplied_args = 0;
    }

    index_of_close_paren = param_idx;

    if (kKW_CloseParenthesis != parameters[index_of_close_paren])
    {
        Error("!Missing ')' at the end of the parameter list");
        return kERR_InternalError;
    }

    if (index_of_close_paren > 0 &&
        kKW_Comma == parameters[index_of_close_paren - 1])
    {
        Error("Last argument in function call is empty");
        return kERR_UserError;
    }

    if (paren_nesting_depth > 0)
    {
        Error("!Parser confused near '%s'", _sym.GetName(name_of_func).c_str());
        return kERR_InternalError;
    }

    return kERR_None;
}

// We are processing a function call. General the actual function call
void AGS::Parser::AccessData_GenerateFunctionCall(Symbol name_of_func, size_t num_args, bool func_is_import)
{
    if (func_is_import)
    {
        // tell it how many args for this call (nested imported functions cause stack problems otherwise)
        WriteCmd(SCMD_NUMFUNCARGS, num_args);
    }

    // Load function address into AX
    WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[name_of_func].FunctionD->Offset);

    if (func_is_import)
    {   
        _scrip.FixupPrevious(kFx_Import);
        if (!_scrip.IsImport(_sym.GetName(name_of_func)))
            _fim.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1, _src.GetCursor());

        WriteCmd(SCMD_CALLEXT, SREG_AX); // Do the call
        // At runtime, we will arrive here when the function call has returned: Restore the stack
        if (num_args > 0)
            WriteCmd(SCMD_SUBREALSTACK, num_args);
        return;
    }

    // Func is non-import
    _scrip.FixupPrevious(kFx_Code);
    if (_fcm.IsForwardDecl(name_of_func))
        _fcm.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1, _src.GetCursor());

    WriteCmd(SCMD_CALL, SREG_AX);  // Do the call

    // At runtime, we will arrive here when the function call has returned: Restore the stack
    if (num_args > 0)
    {
        size_t const size_of_passed_args = num_args * SIZE_OF_STACK_CELL;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_passed_args);
        _scrip.OffsetToLocalVarBlock -= size_of_passed_args;
    }
}

// We are processing a function call.
// Get the parameters of the call and push them onto the stack.
AGS::ErrorType AGS::Parser::AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SrcList &parameters, size_t &actual_num_args)
{
    size_t const num_func_args = _sym.NumOfFuncParams(name_of_func);

    size_t num_supplied_args = 0;
    size_t closed_paren_idx;
    ErrorType retval = AccessData_FunctionCall_CountAndCheckParm(parameters, name_of_func, closed_paren_idx, num_supplied_args);
    if (retval < 0) return retval;

    // Push default parameters onto the stack when applicable
    // This will give an error if there aren't enough default parameters
    if (num_supplied_args < num_func_args)
    {
        retval = AccessData_FunctionCall_ProvideDefaults(num_func_args, num_supplied_args, name_of_func, func_is_import);
        if (retval < 0) return retval;
    }
    if (num_supplied_args > num_func_args && !_sym.IsVariadicFunc(name_of_func))
    {
        Error("Expected just %d parameters but found %d", num_func_args, num_supplied_args);
        return kERR_UserError;
    }
    // ASSERT at this point, the number of parameters is okay

    // Push the explicit arguments of the function
    if (num_supplied_args > 0)
    {
        retval = AccessData_FunctionCall_PushParams(parameters, closed_paren_idx, num_func_args, num_supplied_args, name_of_func, func_is_import);
        if (retval < 0) return retval;
    }

    actual_num_args = std::max(num_supplied_args, num_func_args);
    parameters.SetCursor(closed_paren_idx + 1); // Go to the end of the parameter list
    return kERR_None;
}

AGS::ErrorType AGS::Parser::AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, MemoryLocation &mloc, Vartype &rettype)
{
    if (kKW_OpenParenthesis != expression[1])
    {
        Error("Expected '('");
        return kERR_UserError;
    }

    expression.EatFirstSymbol();

    auto const function_tqs = _sym[name_of_func].FunctionD->TypeQualifiers;
    bool const func_is_import = function_tqs[TQ::kImport];
    // If function uses normal stack, we need to do stack calculations to get at certain elements
    bool const func_uses_normal_stack = !func_is_import;
    bool const called_func_uses_this =
        std::string::npos != _sym.GetName(name_of_func).find("::") &&
        !function_tqs[TQ::kStatic];
    bool const calling_func_uses_this = (kKW_NoSymbol != _sym.GetVartype(kKW_This));
    bool mar_pushed = false;
    bool op_pushed = false;

    if (calling_func_uses_this)
    {
        // Save OP since we need it after the func call
        // We must do this no matter whether the callED function itself uses "this"
        // because a called function that doesn't might call a function that does.
        PushReg(SREG_OP);
        op_pushed = true;
    }

    if (called_func_uses_this)
    {
        // MAR contains the address of "outer"; this is what will be used for "this" in the called function.
        ErrorType retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        if (retval < 0) return retval;

        // Parameter processing might entail calling yet other functions, e.g., in "f(...g(x)...)".
        // So we can't emit SCMD_CALLOBJ here, before parameters have been processed.
        // Save MAR because parameter processing might clobber it 
        PushReg(SREG_MAR);
        mar_pushed = true;
    }

    size_t num_args = 0;
    ErrorType retval = AccessData_PushFunctionCallParams(name_of_func, func_is_import, expression, num_args);
    if (retval < 0) return retval;

    if (called_func_uses_this)
    {
        if (0 == num_args)
        {   // MAR must still be current, so undo the unneeded PUSH above.
            _scrip.OffsetToLocalVarBlock -= SIZE_OF_STACK_CELL;
            _scrip.codesize -= 2;
            mar_pushed = false;
        }
        else
        {   // Recover the value of MAR from the stack. It's in front of the parameters.
            WriteCmd(
                SCMD_LOADSPOFFS,
                (1 + (func_uses_normal_stack ? num_args : 0)) * SIZE_OF_STACK_CELL);
            WriteCmd(SCMD_MEMREAD, SREG_MAR);
        }
        WriteCmd(SCMD_CALLOBJ, SREG_MAR);
    }

    AccessData_GenerateFunctionCall(name_of_func, num_args, func_is_import);

    // function return type
    rettype = _scrip.AX_Vartype = _sym.FuncReturnVartype(name_of_func);
    _scrip.AX_ScopeType = ScT::kLocal;

    if (mar_pushed)
        PopReg(SREG_MAR);
    if (op_pushed)
        PopReg(SREG_OP);

    MarkAcessed(name_of_func);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseExpression_NoOps(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    if (kKW_OpenParenthesis == expression[0])
        return ParseExpression_InParens(expression, vloc, scope_type, vartype);

    return AccessData(false, expression, vloc, scope_type, vartype);
}

AGS::ErrorType AGS::Parser::ParseExpression_Term(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    if (expression.Length() == 0)
    {
        Error("!Cannot parse empty subexpression");
        return kERR_InternalError;
    }

    Symbol const first_sym = expression[0];
    if (kKW_CloseParenthesis == first_sym || kKW_CloseBracket == first_sym || kKW_CloseBrace == first_sym)
    {   // Shouldn't happen: the scanner sees to it that nesting symbols match
        Error(
            "!Unexpected '%s' at start of expression",
            _sym.GetName(first_sym).c_str());
        return kERR_InternalError;
    }

    int least_binding_op_idx;
    ErrorType retval = IndexOfLeastBondingOperator(expression, least_binding_op_idx);  // can be < 0
    if (retval < 0) return retval;

    if (0 == least_binding_op_idx)
        retval = ParseExpression_Unary(expression, vloc, scope_type, vartype);
    else if (0 < least_binding_op_idx)
        retval = ParseExpression_BinaryOrTernary(static_cast<size_t>(least_binding_op_idx), expression, vloc, scope_type, vartype);
    else
        retval = ParseExpression_NoOps(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    if (!expression.ReachedEOF())
    {
        // e.g. "4 3" or "(5) 3".
        // This is most probably due to the user having forgotten an operator
        Error(
            "Expected an operator, found '%s' instead",
            _sym.GetName(expression.GetNext()).c_str());
        return kERR_UserError;
    }
    return HandleStructOrArrayResult(vartype, vloc);
}

// symlist starts a sequence of bracketed expressions; parse it
AGS::ErrorType AGS::Parser::AccessData_ReadIntExpression(SrcList &expression)
{
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;
    ErrorType retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    return IsVartypeMismatch(vartype, kKW_Int, true);
}

// We access a component of a struct in order to read or write it.
AGS::ErrorType AGS::Parser::AccessData_StructMember(Symbol component, bool writing, bool access_via_this, SrcList &expression,Parser::MemoryLocation &mloc, Vartype &vartype)
{
    expression.GetNext(); // Eat component
    SymbolTableEntry &entry = _sym[component];
    auto const compo_tqs = entry.VariableD->TypeQualifiers;

    if (writing && compo_tqs[TQ::kWriteprotected] && !access_via_this)
    {
        Error(
            "Writeprotected component '%s' must not be modified from outside",
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }
    if (compo_tqs[TQ::kProtected] && !access_via_this)
    {
        Error(
            "Protected component '%s' must not be accessed from outside",
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }

    mloc.AddComponentOffset(entry.ComponentD->Offset);
    vartype = _sym.GetVartype(component);
    return kERR_None;
}

// Get the symbol for the get or set function corresponding to the attribute given.
AGS::ErrorType AGS::Parser::ConstructAttributeFuncName(Symbol attribsym, bool writing, bool indexed,Symbol &func)
{
    std::string member_str = _sym.GetName(attribsym);
    // If "::" in the name, take the part after the last "::"
    size_t const m_access_position = member_str.rfind("::");
    if (std::string::npos != m_access_position)
        member_str = member_str.substr(m_access_position + 2);
    char const *stem_str = writing ? "set" : "get";
    char const *indx_str = indexed ? "i_" : "_";
    std::string func_str = stem_str + (indx_str + member_str);
    func = _sym.FindOrAdd(func_str);
    return kERR_None;
}

// We call the getter or setter of an attribute
AGS::ErrorType AGS::Parser::AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, Vartype &vartype)
{
    // Search for the attribute: It might be in an ancestor of 'vartype' instead of in 'vartype'.
    Symbol const unqualified_component = expression.GetNext();
    Symbol const struct_of_component =
        FindStructOfComponent(vartype, unqualified_component);
    if (kKW_NoSymbol == struct_of_component)
    {
        Error(
            ReferenceMsgSym(
                "Struct '%s' does not have an attribute named '%s'",
                struct_of_component).c_str(),
            _sym.GetName(vartype).c_str(),
            _sym.GetName(unqualified_component).c_str());
        return kERR_UserError;
    }
    auto const &struct_components = _sym[struct_of_component].VartypeD->Components;
    Symbol const name_of_attribute = struct_components.at(unqualified_component);

    bool const attrib_uses_this =
        !_sym[name_of_attribute].VariableD->TypeQualifiers[TQ::kStatic];
    bool const call_is_indexed =
        (kKW_OpenBracket == expression.PeekNext());
    bool const attrib_is_indexed =
        _sym.IsDynarrayVartype(name_of_attribute);

    if (call_is_indexed && !attrib_is_indexed)
    {
        Error("Unexpected '[' after non-indexed attribute %s", _sym.GetName(name_of_attribute).c_str());
        return kERR_UserError;
    }
    else if (!call_is_indexed && attrib_is_indexed)
    {
        Error("'[' expected after indexed attribute but not found", _sym.GetName(name_of_attribute).c_str());
        return kERR_UserError;
    }

    // Get the appropriate access function (as a symbol)
    Symbol unqualified_func_name = kKW_NoSymbol;
    ErrorType retval = ConstructAttributeFuncName(unqualified_component, is_setter, attrib_is_indexed, unqualified_func_name);
    if (retval < 0) return retval;
    if (0 == struct_components.count(unqualified_func_name))
    {
        Error(
            "!Attribute function '%s' not found in struct '%s'",
            _sym.GetName(unqualified_func_name).c_str(),
            _sym.GetName(struct_of_component).c_str());
        return kERR_InternalError;
    }
    Symbol const qualified_func_name = struct_components.at(unqualified_func_name);
    bool const func_is_import = _sym[qualified_func_name].FunctionD->TypeQualifiers[TQ::kImport];

    if (attrib_uses_this)
        PushReg(SREG_OP); // is the current this ptr, must be restored after call

    size_t num_of_args = 0;
    if (is_setter)
    {
        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
        ++num_of_args;
    }

    if (call_is_indexed)
    {
        // The index to be set is in the [...] clause; push it as the first parameter
        if (attrib_uses_this)
            PushReg(SREG_MAR); // must not be clobbered
        retval = AccessData_ReadBracketedIntExpression(expression);
        if (retval < 0) return retval;

        if (attrib_uses_this)
            PopReg(SREG_MAR);

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
        ++num_of_args;
    }

    if (attrib_uses_this)
        WriteCmd(SCMD_CALLOBJ, SREG_MAR); // make MAR the new this ptr

    AccessData_GenerateFunctionCall(qualified_func_name, num_of_args, func_is_import);

    if (attrib_uses_this)
        PopReg(SREG_OP); // restore old this ptr after the func call

    // attribute return type
    _scrip.AX_ScopeType = ScT::kLocal;
    _scrip.AX_Vartype = vartype = _sym.FuncReturnVartype(qualified_func_name);

    MarkAcessed(qualified_func_name);
    return kERR_None;
}


// Location contains a pointer to another address. Get that address.
AGS::ErrorType AGS::Parser::AccessData_Dereference(ValueLocation &vloc,Parser::MemoryLocation &mloc)
{
    if (kVL_AX_is_value == vloc)
    {
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        vloc = kVL_MAR_pointsto_value;
        mloc.Reset();
    }
    else
    {
        ErrorType retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        if (retval < 0) return retval;
        // Note: We need to check here whether m[MAR] == 0, but CHECKNULL
        // checks whether MAR == 0. So we need to do MAR := m[MAR] first.
        WriteCmd(SCMD_MEMREADPTR, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::AccessData_ProcessArrayIndexConstant(size_t idx, Symbol index_symbol, bool negate, size_t num_array_elements, size_t element_size, MemoryLocation &mloc)
{
    Symbol lit = index_symbol;
    while (_sym.IsConstant(lit))
        lit = _sym[lit].ConstantD->ValueSym;
    Vartype vartype = _sym[lit].LiteralD->Vartype;
    if (!_sym.IsAnyIntegerVartype(vartype))
    {
        Error(
            "Expected an integer expression in array index #%u but found '%s' instead which is type '%s'",
            idx + 1u,
            _sym.GetName(index_symbol),
            _sym.GetName(vartype));
        return kERR_UserError;
    }

    if (negate)
    {
        ErrorType retval = NegateLiteral(lit);
        if (retval < 0) return retval;
    }

    CodeCell const array_index = _sym[lit].LiteralD->Value;
    if (array_index < 0)
    {
        Error(
            "Array index #%u is %d, thus too low (minimum is 0)",
            idx + 1u,
            array_index);
        return kERR_UserError;
    }

    if (num_array_elements > 0 && static_cast<size_t>(array_index) >= num_array_elements)
    {
        Error(
            "Array index #%u is %d, thus too high (maximum is %u)",
            idx + 1u,
            array_index,
            num_array_elements - 1u);
        return kERR_UserError;
    }
    mloc.AddComponentOffset(array_index * element_size);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::AccessData_ProcessCurrentArrayIndex(size_t idx, size_t dim, size_t factor, bool is_dynarray, SrcList &expression, MemoryLocation &mloc)
{
    // Get the index
    size_t const index_start = expression.GetCursor();
    SkipTo(SymbolList{ kKW_Comma, kKW_CloseBracket }, expression);
    size_t const index_end = expression.GetCursor();
    SrcList current_index = SrcList(expression, index_start, index_end - index_start);
    if (0 == current_index.Length())
    {
        Error("Empty array index is not supported");
        return kERR_UserError;
    }

    // If the index is a literal or constant or a negation thereof, process it at compile time
    if (1 == current_index.Length())
    {
        Symbol const index_sym = current_index[0];
        if (_sym.IsLiteral(index_sym) || _sym.IsConstant(index_sym))
            return AccessData_ProcessArrayIndexConstant(idx, index_sym, false, dim, factor, mloc);
    }
    if (2 == current_index.Length() && kKW_Minus == current_index[0])
    {
        Symbol const index_sym = current_index[1];
        if (_sym.IsLiteral(index_sym) || _sym.IsConstant(index_sym))
            return AccessData_ProcessArrayIndexConstant(idx, index_sym, true, dim, factor, mloc);
    }

    ErrorType retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
    if (retval < 0) return retval;
    PushReg(SREG_MAR);
    
    retval = AccessData_ReadIntExpression(current_index);
    if (retval < 0) return retval;

    PopReg(SREG_MAR);
    

    // Note: DYNAMICBOUNDS compares the offset into the memory block;
    // it mustn't be larger than the size of the allocated memory. 
    // On the other hand, CHECKBOUNDS checks the index; it mustn't be
    //  larger than the maximum given. So dynamic bounds must be checked
    // after the multiplication; static bounds before the multiplication.
    // For better error messages at runtime, don't do CHECKBOUNDS after the multiplication.
    if (!is_dynarray)
        WriteCmd(SCMD_CHECKBOUNDS, SREG_AX, dim);
    if (factor != 1)
        WriteCmd(SCMD_MUL, SREG_AX, factor);
    if (is_dynarray)
        WriteCmd(SCMD_DYNAMICBOUNDS, SREG_AX);
    WriteCmd(SCMD_ADDREG, SREG_MAR, SREG_AX);
    return kERR_None;
}

// We're processing some struct component or global or local variable.
// If an array index follows, parse it and shorten symlist accordingly
AGS::ErrorType AGS::Parser::AccessData_ProcessAnyArrayIndex(ValueLocation vloc_of_array, SrcList &expression, ValueLocation &vloc,Parser::MemoryLocation &mloc, Vartype &vartype)
{
    if (kKW_OpenBracket != expression.PeekNext())
        return kERR_None;
    expression.GetNext(); // Eat '['

    bool const is_dynarray = _sym.IsDynarrayVartype(vartype);
    bool const is_array = _sym.IsArrayVartype(vartype);
    if (!is_dynarray && !is_array)
    {
        Error("Array index is only legal after an array expression");
        return kERR_UserError;
    }

    Vartype const element_vartype = _sym[vartype].VartypeD->BaseVartype;
    size_t const element_size = _sym.GetSize(element_vartype);
    std::vector<size_t> dim_sizes;
    std::vector<size_t> dynarray_dims = { 0, };
    std::vector<size_t> &dims = is_dynarray ? dynarray_dims : _sym[vartype].VartypeD->Dims;
    vartype = element_vartype;

    if (is_dynarray)
        AccessData_Dereference(vloc, mloc);

    // Number of dimensions and the the size of the dimension for each dimension
    size_t const num_of_dims = dims.size();
    dim_sizes.resize(num_of_dims);
    size_t factor = element_size;
    for (int dim_idx = num_of_dims - 1; dim_idx >= 0; dim_idx--) // yes, "int"
    {
        dim_sizes[dim_idx] = factor;
        factor *= dims[dim_idx];
    }

    for (size_t dim_idx = 0; dim_idx < num_of_dims; dim_idx++)
    {
        ErrorType retval = AccessData_ProcessCurrentArrayIndex(dim_idx, dims[dim_idx], dim_sizes[dim_idx], is_dynarray, expression, mloc);
        if (retval < 0) return retval;

        Symbol divider = expression.PeekNext();
        retval = Expect(SymbolList{ kKW_CloseBracket, kKW_Comma }, divider);
        if (retval < 0) return retval;

        if (kKW_CloseBracket == divider)
        {
            expression.GetNext(); // Eat ']'
            divider = expression.PeekNext();
        }
        if (kKW_Comma == divider || kKW_OpenBracket == divider)
        {
            if (num_of_dims == dim_idx + 1)
            {
                Error("Expected %d indexes, found more", num_of_dims);
                return kERR_UserError;
            }
            expression.GetNext(); // Eat ',' or '['
            continue;
        }
        if (num_of_dims != dim_idx + 1)
        {
            Error("Expected %d indexes, but only found %d", num_of_dims, dim_idx + 1);
            return kERR_UserError;
        }
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::AccessData_Variable(ScopeType scope_type, bool writing, SrcList &expression,Parser::MemoryLocation &mloc, Vartype &vartype)
{
    Symbol varname = expression.GetNext();
    if (ScT::kImport == scope_type)
        MarkAcessed(varname);
    SymbolTableEntry &entry = _sym[varname];
    CodeCell const soffs = entry.VariableD->Offset;
    auto const var_tqs = entry.VariableD->TypeQualifiers;

    if (writing && var_tqs[TQ::kReadonly])
    {
        Error("Cannot write to readonly '%s'", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    mloc.SetStart(scope_type, soffs);
    vartype = _sym.GetVartype(varname);

    // Process an array index if it follows
    ValueLocation vl_dummy = kVL_MAR_pointsto_value;
    return AccessData_ProcessAnyArrayIndex(kVL_MAR_pointsto_value, expression, vl_dummy, mloc, vartype);
}

AGS::ErrorType AGS::Parser::AccessData_FirstClause(bool writing, SrcList &expression, ValueLocation &vloc, ScopeType &return_scope_type,Parser::MemoryLocation &mloc, Vartype &vartype, bool &implied_this_dot, bool &static_access)
{
    if (expression.Length() < 1)
    {
        Error("!Empty first clause");
        return kERR_InternalError;
    }
    expression.StartRead();

    implied_this_dot = false;

    Symbol const first_sym = expression.PeekNext();

    do // exactly one time
    {
        if (kKW_This == first_sym)
        {
            expression.GetNext(); // Eat 'this'
            vartype = _sym.GetVartype(kKW_This);
            if (kKW_NoSymbol == vartype)
            {
                Error("'this' is only legal in non-static struct functions");
                return kERR_UserError;
            }
            vloc = kVL_MAR_pointsto_value;
            WriteCmd(SCMD_REGTOREG, SREG_OP, SREG_MAR);
            WriteCmd(SCMD_CHECKNULL);
            mloc.Reset();
            if (kKW_Dot == expression.PeekNext())
            {
                expression.GetNext(); // Eat '.'
                // Going forward, we must "imply" "this." since we've just gobbled it.
                implied_this_dot = true;
            }

            return kERR_None;
        }

        if (kKW_Null == first_sym)
        {
            expression.GetNext(); // Eat 'null'
            WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
            vloc = kVL_AX_is_value;
            return_scope_type = ScT::kGlobal;
            _scrip.AX_Vartype = vartype = kKW_Null;
            _scrip.AX_ScopeType = ScT::kGlobal;
            return kERR_None;
        }

        if (_sym.IsConstant(first_sym) ||
            _sym.IsLiteral(first_sym))
        {
            if (writing) break; // to error msg

            Symbol lit;
            ErrorType retval = ReadLiteralOrConst(lit);
            if (retval < 0) return retval;
            return_scope_type = ScT::kGlobal;
            return EmitLiteral(lit, vloc, vartype);
        }

        if (_sym.IsFunction(first_sym))
        {
            return_scope_type = ScT::kGlobal;
            vloc = kVL_AX_is_value;
            ErrorType retval = AccessData_FunctionCall(first_sym, expression, mloc, vartype);
            if (retval < 0) return retval;
            if (_sym.IsDynarrayVartype(vartype))
                return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
            return kERR_None;
        }

        if (_sym.IsVariable(first_sym))
        {
            ScopeType const scope_type = _sym.GetScopeType(first_sym);
            // Parameters may be 'return'ed even though they are local because they are allocated
            // outside of the function proper. Therefore return scope type for them is global.
            return_scope_type = _sym.IsParameter(first_sym) ? ScT::kGlobal : scope_type;
            vloc = kVL_MAR_pointsto_value;
            return AccessData_Variable(scope_type, writing, expression, mloc, vartype);
        }

        if (_sym.IsVartype(first_sym))
        {
            return_scope_type = ScT::kGlobal;
            static_access = true;
            vartype = expression.GetNext();
            mloc.Reset();
            return kERR_None;
        }
    
        // If this unknown symbol can be interpreted as a component of 'this',
        // treat it that way.
        vartype = _sym.GetVartype(kKW_This);
        if (_sym[vartype].VartypeD->Components.count(first_sym))
        {
            vloc = kVL_MAR_pointsto_value;
            WriteCmd(SCMD_REGTOREG, SREG_OP, SREG_MAR);
            WriteCmd(SCMD_CHECKNULL);
            mloc.Reset();

            // Going forward, the code should imply "this."
            // with the '.' already read in.
            implied_this_dot = true;
            // Then the component needs to be read again.
            expression.BackUp();
            return kERR_None;
        }

        Error("Unexpected '%s'", _sym.GetName(first_sym).c_str());
        return kERR_UserError;
    } while (false);

    Error("Cannot assign a value to '%s'", _sym.GetName(expression[0]).c_str());
    return kERR_UserError;
}

// We're processing a STRUCT.STRUCT. ... clause.
// We've already processed some structs, and the type of the last one is vartype.
// Now we process a component of vartype.
AGS::ErrorType AGS::Parser::AccessData_SubsequentClause(bool writing, bool access_via_this, bool static_access, SrcList &expression, ValueLocation &vloc, ScopeType &return_scope_type, MemoryLocation &mloc, Vartype &vartype)
{
    Symbol const unqualified_component = expression.PeekNext();
    Symbol const qualified_component = FindComponentInStruct(vartype, unqualified_component);

    if (kKW_NoSymbol == qualified_component)
    {
        Error(
            "Expected a component of '%s', found '%s' instead",
            _sym.GetName(vartype).c_str(),
            _sym.GetName(unqualified_component).c_str());
        return kERR_UserError;
    }

    if (_sym.IsFunction(qualified_component))
    {
        if (static_access && !_sym[qualified_component].FunctionD->TypeQualifiers[TQ::kStatic])
        {
            Error("Must specify a specific object for non-static function %s", _sym.GetName(qualified_component).c_str());
            return kERR_UserError;
        }

        vloc = kVL_AX_is_value;
        return_scope_type = ScT::kLocal;
        SrcList start_of_funccall = SrcList(expression, expression.GetCursor(), expression.Length());
        ErrorType retval = AccessData_FunctionCall(qualified_component, start_of_funccall, mloc, vartype);
        if (retval < 0) return retval;
        if (_sym.IsDynarrayVartype(vartype))
            return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
        return kERR_None;
    }

    if (!_sym.IsVariable(qualified_component))
    {
        Error(
            "Expected a variable, attribute, or function component of '%s', found '%s' instead",
            _sym.GetName(vartype).c_str(),
            _sym.GetName(unqualified_component).c_str());
        return kERR_UserError;
    }

    if (static_access && !_sym[qualified_component].VariableD->TypeQualifiers[TQ::kStatic])
    {
        Error("Must specify a specific object for non-static component %s", _sym.GetName(qualified_component).c_str());
        return kERR_UserError;
    }

    if (_sym.IsAttribute(qualified_component))
    {
        // make MAR point to the struct of the attribute
        ErrorType retval = mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        if (retval < 0) return retval;
        if (writing)
        {
            // We cannot process the attribute here so return to the assignment that
            // this attribute was originally called from
            vartype = _sym.GetVartype(qualified_component);
            vloc = kVL_Attribute;
            return kERR_None;
        }
        vloc = kVL_AX_is_value;
        bool const is_setter = false;
        return AccessData_CallAttributeFunc(is_setter, expression, vartype);
    }

    // So it is a non-attribute variable
    vloc = kVL_MAR_pointsto_value;
    ErrorType retval = AccessData_StructMember(qualified_component, writing, access_via_this, expression, mloc, vartype);
    if (retval < 0) return retval;
    return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
}

AGS::Symbol AGS::Parser::FindStructOfComponent(Vartype strct, Symbol unqualified_component)
{
    while (strct > 0 && _sym.IsVartype(strct))
    {
        auto const &components = _sym[strct].VartypeD->Components;
        if (0 < components.count(unqualified_component))
            return strct;
        strct = _sym[strct].VartypeD->Parent;
    }
    return kKW_NoSymbol;
}

AGS::Symbol AGS::Parser::FindComponentInStruct(Vartype strct, Symbol unqualified_component)
{
    while (strct > 0 && _sym.IsVartype(strct))
    {
        auto const &components = _sym[strct].VartypeD->Components;
        if (0 < components.count(unqualified_component))
            return components.at(unqualified_component);
        strct = _sym[strct].VartypeD->Parent;
    }
    return kKW_NoSymbol;
}

// We are in a STRUCT.STRUCT.STRUCT... cascade.
// Check whether we have passed the last dot
AGS::ErrorType AGS::Parser::AccessData_IsClauseLast(SrcList &expression, bool &is_last)
{
    size_t const cursor = expression.GetCursor();
    SkipTo(SymbolList{ kKW_Dot },  expression);
    is_last = (kKW_Dot != expression.PeekNext());
    expression.SetCursor(cursor);
    return kERR_None;
}

// Access a variable, constant, literal, func call, struct.component.component cascade, etc.
// Result is in AX or m[MAR], dependent on vloc. Type is in vartype.
// At end of function, symlist and symlist_len will point to the part of the symbol string
// that has not been processed yet
// NOTE: If this selects an attribute for writing, then the corresponding function will
// _not_ be called and symlist[0] will be the attribute.
AGS::ErrorType AGS::Parser::AccessData(bool writing, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype)
{
    if (0 == expression.Length())
    {
        Error("!empty expression");
        return kERR_InternalError;
    }
    
    // For memory accesses, we set the MAR register lazily so that we can
    // accumulate offsets at runtime instead of compile time.
    // This struct tracks what we will need to do to set the MAR register.
    MemoryLocation mloc = MemoryLocation(*this);

    bool clause_is_last = false;
    ErrorType retval = AccessData_IsClauseLast(expression, clause_is_last);
    if (retval < 0) return retval;

    bool implied_this_dot = false; // only true when "this." is implied
    bool static_access = false; // only true when a vartype has just been parsed

    // If we are reading, then all the accesses are for reading.
    // If we are writing, then all the accesses except for the last one
    // are for reading and the last one will be for writing.
    retval = AccessData_FirstClause((writing && clause_is_last), expression, vloc, scope_type, mloc, vartype, implied_this_dot, static_access);
    if (retval < 0) return retval;

    Vartype outer_vartype = kKW_NoSymbol;

    // If the previous function has assumed a "this." that isn't there,
    // then a '.' won't be coming up but the while body must be executed anyway.
    while (kKW_Dot == expression.PeekNext() || implied_this_dot)
    {
        if (!implied_this_dot)
            expression.GetNext(); // Eat '.'
        // Note: do not reset "implied_this_dot" here, it's still needed.

        // Here, if kVL_MAR_pointsto_value == vloc then the first byte of outer is at m[MAR + mar_offset].
        // We accumulate mar_offset at compile time as long as possible to save computing.
        outer_vartype = vartype;

        // Note: A DynArray can't be directly in front of a '.' (need a [...] first)
        if (_sym.IsDynpointerVartype(vartype))
        {
            retval = AccessData_Dereference(vloc, mloc);
            if (retval < 0) return retval;
            vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype);
        }

        if (!_sym.IsStructVartype(vartype) || !_sym.IsAtomicVartype(vartype))
        {
            if (_sym.IsArrayVartype(vartype) || _sym.IsDynarrayVartype(vartype))
                Error("Expected a struct in front of '.' but found an array instead");
            else        
                Error(
                    "Expected a struct in front of '.' but found an expression of type '%s' instead",
                    _sym.GetName(outer_vartype).c_str());
            return kERR_UserError;
        }

        if (expression.ReachedEOF())
        {
            Error("Expected struct component after '.' but did not find it");
            return kERR_UserError;
        }

        retval = AccessData_IsClauseLast(expression, clause_is_last);
        if (retval < 0) return retval;

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        retval = AccessData_SubsequentClause((clause_is_last && writing), implied_this_dot, static_access, expression, vloc, scope_type, mloc, vartype);
        if (retval < 0) return retval;

        // Next component access, if there is any, is dependent on
        // the current access, no longer on "this".
        implied_this_dot = false;
        // Next component access, if there is any, won't be static.
        static_access = false;
    }

    if (kVL_Attribute == vloc)
    {
        // Caller will do the assignment
        // For this to work, the caller must know the vartype of the struct
        // in which the attribute resides. Ignore "const" and "dynptr" to find the vartype
        vartype = _sym.VartypeWithout(VTT::kConst, outer_vartype);
        vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype);
        return kERR_None;
    }

    if (kVL_AX_is_value == vloc)
    {
        _scrip.AX_Vartype = vartype;
        _scrip.AX_ScopeType = scope_type;
        return kERR_None;
    }

    return mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
}

bool AGS::Parser::AccessData_MayAccessClobberAX(SrcList &expression)
{
    size_t const expression_length = expression.Length();
    if (0u == expression_length)
        return false;

    Symbol const first_sym = expression[0u];
    if (1u == expression_length)
        return !_sym.IsVariable(first_sym);

    Vartype outer_vartype = _sym.IsVartype(first_sym) ? first_sym : _sym.GetVartype(first_sym);
    for (size_t dot_idx = 1; dot_idx < expression.Length() - 2; dot_idx += 2)
    {
        if (kKW_Dot != expression[dot_idx])
            return true;
        Symbol const unqualified_component = expression[dot_idx + 1];
        auto const &outer_components = _sym[outer_vartype].VartypeD->Components;
        if (0 == outer_components.count(unqualified_component))
            return true;
        Symbol const qualified_component = outer_components.at(unqualified_component);
        if (!_sym.IsVariable(qualified_component))
            return true;
        outer_vartype = _sym.GetVartype(qualified_component);
    }
        
    return false;
}

// Insert Bytecode for:
// Copy at most OLDSTRING_LENGTH - 1 bytes from m[MAR...] to m[AX...]
// Stop when encountering a 0
void AGS::Parser::AccessData_StrCpy()
{
    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_CX); // CX = dest
    WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_BX); // BX = src
    WriteCmd(SCMD_LITTOREG, SREG_DX, STRINGBUFFER_LENGTH - 1); // DX = count
    CodeLoc const loop_start = _scrip.codesize; // Label LOOP_START
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_MAR); // AX = m[BX]
    WriteCmd(SCMD_MEMREAD, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_CX, SREG_MAR); // m[CX] = AX
    WriteCmd(SCMD_MEMWRITE, SREG_AX);
    WriteCmd(SCMD_JZ, -77);  // if (AX == 0) jumpto LOOP_END
    CodeLoc const jumpout_pos = _scrip.codesize - 1;
    WriteCmd(SCMD_ADD, SREG_BX, 1); // BX++, CX++, DX--
    WriteCmd(SCMD_ADD, SREG_CX, 1);
    WriteCmd(SCMD_SUB, SREG_DX, 1);
    WriteCmd(SCMD_REGTOREG, SREG_DX, SREG_AX); // if (DX != 0) jumpto LOOP_START
    WriteCmd(
        SCMD_JNZ,
        ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, loop_start));
    WriteCmd(SCMD_ADD, SREG_CX, 1); // Force a 0-terminated dest string
    WriteCmd(SCMD_REGTOREG, SREG_CX, SREG_MAR);
    WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    WriteCmd(SCMD_MEMWRITE, SREG_AX);
    CodeLoc const loop_end = _scrip.codesize; // Label LOOP_END
    _scrip.code[jumpout_pos] = ccCompiledScript::RelativeJumpDist(jumpout_pos, loop_end);
}

// We are typically in an assignment LHS = RHS; the RHS has already been
// evaluated, and the result of that evaluation is in AX.
// Store AX into the memory location that corresponds to LHS, or
// call the attribute function corresponding to LHS.
AGS::ErrorType AGS::Parser::AccessData_AssignTo(SrcList &expression)
{
    // We'll evaluate expression later on which moves the cursor,
    // so save it here and restore later on
    size_t const end_of_rhs_cursor = _src.GetCursor();

    // AX contains the result of evaluating the RHS of the assignment
    // Save on the stack so that it isn't clobbered
    Vartype rhsvartype = _scrip.AX_Vartype;
    ScopeType rhs_scope_type = _scrip.AX_ScopeType;
    // Save AX unless we are sure that it won't be clobbered
    bool const may_clobber = AccessData_MayAccessClobberAX(expression);
    if (may_clobber)
        PushReg(SREG_AX);

    bool const writing = true;
    ValueLocation vloc;
    Vartype lhsvartype;
    ScopeType lhs_scope_type;
    ErrorType retval = AccessData(writing, expression, vloc, lhs_scope_type, lhsvartype);
    if (retval < 0) return retval;

    if (kVL_AX_is_value == vloc)
    {
        if (!_sym.IsManagedVartype(lhsvartype))
        {
            Error("Cannot modify this value");
            return kERR_UserError;
        }
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        vloc = kVL_MAR_pointsto_value;
    }

    if (may_clobber)
        PopReg(SREG_AX);

    _scrip.AX_Vartype = rhsvartype;
    _scrip.AX_ScopeType = rhs_scope_type;

    if (kVL_Attribute == vloc)
    {
        // We need to call the attribute setter 
        Vartype struct_of_attribute = lhsvartype;

        bool const is_setter = true;
        retval =  AccessData_CallAttributeFunc(is_setter, expression, struct_of_attribute);
        if (retval < 0) return retval;
        _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
        return kERR_None;
    }

    // MAR points to the value

    if (kKW_String == lhsvartype && kKW_String == _sym.VartypeWithout(VTT::kConst, rhsvartype))
    {
        // copy the string contents over.
        AccessData_StrCpy();
        _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
        return kERR_None;
    }

    ConvertAXStringToStringObject(lhsvartype);
    rhsvartype = _scrip.AX_Vartype;
    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype))
    {
        Error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhsvartype).c_str(),
            _sym.GetName(lhsvartype).c_str());
        return kERR_UserError;
    }

    CodeCell const opcode =
        _sym.IsDynVartype(lhsvartype) ?
        SCMD_MEMWRITEPTR : GetWriteCommandForSize(_sym.GetSize(lhsvartype));
    WriteCmd(opcode, SREG_AX);
    _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
    return kERR_None;
}

AGS::ErrorType AGS::Parser::SkipToEndOfExpression()
{
    int nesting_depth = 0;

    Symbol const vartype_of_this = _sym[kKW_This].VariableD->Vartype;

    // The ':' in an "a ? b : c" construct can also be the end of a label, and in AGS,
    // expressions are allowed for labels. So we must take care that label ends aren't
    // mistaken for expression parts. For this, tern_depth counts the number of
    // unmatched '?' on the outer level. If this is non-zero, then any arriving 
    // ':' will be interpreted as part of a ternary.
    int tern_depth = 0;

    Symbol peeksym;
    while (0 <= (peeksym = _src.PeekNext())) // note assignment in while condition
    {
        // Skip over parts that are enclosed in braces, brackets, or parens
        if (kKW_OpenParenthesis == peeksym || kKW_OpenBracket == peeksym || kKW_OpenBrace == peeksym)
            ++nesting_depth;
        else if (kKW_CloseParenthesis == peeksym || kKW_CloseBracket == peeksym || kKW_CloseBrace == peeksym)
            if (--nesting_depth < 0)
                break; // this symbol can't be part of the current expression
        if (nesting_depth > 0)
        {
            _src.GetNext();
            continue;
        }

        if (kKW_Colon == peeksym)
        {
            // This is only allowed if it can be matched to an open tern
            if (--tern_depth < 0)
                break;

            _src.GetNext(); // Eat ':'
            continue;
        }

        if (kKW_Dot == peeksym)
        {
            _src.GetNext(); // Eat '.'
            _src.GetNext(); // Eat following symbol
            continue;
        }

        if (kKW_New == peeksym)
        {   // Only allowed if a type follows   
            _src.GetNext(); // Eat 'new'
            Symbol const sym_after_new = _src.PeekNext();
            if (_sym.IsVartype(sym_after_new))
            {
                _src.GetNext(); // Eat symbol after 'new'
                continue;
            }
            _src.BackUp(); // spit out 'new'
            break;
        }

        if (kKW_Null == peeksym)
        {   // Allowed.
            _src.GetNext(); // Eat 'null'
            continue;
        }

        if (kKW_Tern == peeksym)
        {
            tern_depth++;
            _src.GetNext(); // Eat '?'
            continue;
        }

        if (_sym.IsVartype(peeksym))
        {   // Only allowed if a dot follows
            _src.GetNext(); // Eat the vartype
            Symbol const nextsym = _src.PeekNext();
            if (kKW_Dot == nextsym)
                continue; // Do not eat the dot.
            _src.BackUp(); // spit out the vartype
            break;
        }

        // Let a symbol through if it can be considered a component of 'this'.
        if (kKW_NoSymbol != vartype_of_this &&
            0 < _sym[vartype_of_this].VartypeD->Components.count(peeksym))
        {
            _src.GetNext(); // Eat the peeked symbol
            continue;
        }

        if (!_sym.CanBePartOfAnExpression(peeksym))
            break;
        _src.GetNext(); // Eat the peeked symbol
    }

    if (nesting_depth > 0)
    {
        Error("Unexpected end of input");
        return kERR_UserError;
    }
    return kERR_None;
}

// evaluate the supplied expression, putting the result into AX
// returns 0 on success or -1 if compile error
// leaves targ pointing to last token in expression, so do getnext() to get the following ; or whatever
AGS::ErrorType AGS::Parser::ParseExpression()
{
    size_t const expr_start = _src.GetCursor();
    ErrorType retval = SkipToEndOfExpression();
    if (retval < 0) return retval;
    SrcList expression = SrcList(_src, expr_start, _src.GetCursor() - expr_start);
    if (0 == expression.Length())
    {
        Error("Expected an expression, found '%s' instead", _sym.GetName(_src.GetNext()).c_str());
        return kERR_UserError;
    }
    
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;

    retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    return ResultToAX(vloc, scope_type, vartype);
}

AGS::ErrorType AGS::Parser::AccessData_ReadBracketedIntExpression(SrcList &expression)
{
    ErrorType retval = Expect(kKW_OpenBracket, expression.GetNext());
    if (retval < 0) return retval;

    size_t start = expression.GetCursor();
    SkipTo(SymbolList{}, expression);
    SrcList in_brackets = SrcList(expression, start, expression.GetCursor() - start);

    retval = AccessData_ReadIntExpression(in_brackets);
    if (retval < 0) return retval;

    if (!in_brackets.ReachedEOF())
    {
        Error("Expected ']', found '%s' instead", _sym.GetName(in_brackets.GetNext()).c_str());
        return kERR_UserError;
    }
    return Expect(kKW_CloseBracket, expression.GetNext());
}

AGS::ErrorType AGS::Parser::ParseParenthesizedExpression()
{
    ErrorType retval = Expect(kKW_OpenParenthesis, _src.GetNext());
    if (retval < 0) return retval;
    
    retval = ParseExpression();
    if (retval < 0) return retval;

    return Expect(kKW_CloseParenthesis, _src.GetNext());
}

// We are parsing the left hand side of a += or similar statement.
AGS::ErrorType AGS::Parser::ParseAssignment_ReadLHSForModification(SrcList &lhs, ValueLocation &vloc, Vartype &lhstype)
{
    ScopeType scope_type;

    bool const writing = false; // reading access
    ErrorType retval = AccessData(writing, lhs, vloc, scope_type, lhstype);
    if (retval < 0) return retval;
    if (!lhs.ReachedEOF())
    {
        Error("!Unexpected symbols following expression");
        return kERR_InternalError;
    }

    if (kVL_MAR_pointsto_value == vloc)
    {
        // write memory to AX
        _scrip.AX_Vartype = lhstype;
        _scrip.AX_ScopeType = scope_type;
        WriteCmd(
            GetReadCommandForSize(_sym.GetSize(lhstype)),
            SREG_AX);
    }
    return kERR_None;
}

// "var = expression"; lhs is the variable
AGS::ErrorType AGS::Parser::ParseAssignment_Assign(SrcList &lhs)
{
    _src.GetNext(); // Eat '='
    ErrorType retval = ParseExpression(); // RHS of the assignment
    if (retval < 0) return retval;
    
    return AccessData_AssignTo(lhs);
}

// We compile something like "var += expression"
AGS::ErrorType AGS::Parser::ParseAssignment_MAssign(Symbol ass_symbol, SrcList &lhs)
{
    _src.GetNext(); // Eat assignment symbol

    // Parse RHS
    ErrorType retval = ParseExpression();
    if (retval < 0) return retval;

    PushReg(SREG_AX);
    Vartype rhsvartype = _scrip.AX_Vartype;

    // Parse LHS (moves the cursor to end of LHS, so save it and restore it afterwards)
    ValueLocation vloc;
    Vartype lhsvartype;
    size_t const end_of_rhs_cursor = _src.GetCursor();
    retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype); 
    if (retval < 0) return retval;
    _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS

    // Use the operator on LHS and RHS
    CodeCell opcode = _sym.OperatorOpcode(ass_symbol);
    retval = GetOpcodeValidForVartype(lhsvartype, rhsvartype, opcode);
    if (retval < 0) return retval;
    PopReg(SREG_BX);
    WriteCmd(opcode, SREG_AX, SREG_BX);

    if (kVL_MAR_pointsto_value == vloc)
    {
        // Shortcut: Write the result directly back to memory
        CodeCell memwrite = GetWriteCommandForSize(_sym.GetSize(lhsvartype));
        WriteCmd(memwrite, SREG_AX);
        return kERR_None;
    }

    // Do a conventional assignment
    return AccessData_AssignTo(lhs);
}

// "var++" or "var--"
AGS::ErrorType AGS::Parser::ParseAssignment_SAssign(Symbol ass_symbol, SrcList &lhs)
{
    ValueLocation vloc;
    Vartype lhsvartype;
    ErrorType retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype);
    if (retval < 0) return retval;

    // increment or decrement AX, using the correct opcode
    CodeCell opcode = _sym.OperatorOpcode(ass_symbol);
    retval = GetOpcodeValidForVartype(lhsvartype, lhsvartype, opcode);
    if (retval < 0) return retval;
    WriteCmd(opcode, SREG_AX, 1);

    if (kVL_MAR_pointsto_value == vloc)
    {
        _src.GetNext(); // Eat ++ or --
        // write AX back to memory
        Symbol memwrite = GetWriteCommandForSize(_sym.GetSize(lhsvartype));
        WriteCmd(memwrite, SREG_AX);
        return kERR_None;
    }

    retval = ParseAssignment_Assign(lhs); // moves cursor to end of LHS
    if (retval < 0) return retval; 
    _src.GetNext(); // Eat ++ or --
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_IntVartypeOrFloat(Vartype const wanted_vartype, void *&initial_val_ptr)
{
    bool const negate = (kKW_Minus == _src.PeekNext());
    if (negate)
        _src.GetNext(); // Eat '-'

    Symbol lit;
    ErrorType retval = ReadLiteralOrConst(lit);
    if (retval < 0) return retval;
    Vartype vartype = _sym[lit].LiteralD->Vartype;
    if ((kKW_Float == wanted_vartype) != (kKW_Float == vartype))
    {
        Error(
            "Expected a '%s' value after '=' but found a '%s' value instead",
            _sym.GetName(wanted_vartype).c_str(),
            _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }
    if (negate)
    {
        retval = NegateLiteral(lit);
        if (retval < 0) return retval;
    }

    size_t const wanted_size = _sym.GetSize(wanted_vartype);
    initial_val_ptr = malloc(sizeof(wanted_size));
    if (!initial_val_ptr)
    {
        Error("Out of memory");
        return kERR_UserError;
    }

    switch (wanted_size)
    {
    default:
        Error("Cannot give an initial value to a variable of type '%s' here", _sym.GetName(wanted_vartype));
        return kERR_None;
    case 1:
        (static_cast<int8_t *>(initial_val_ptr))[0] = _sym[lit].LiteralD->Value;
        return kERR_None;
    case 2:
        (static_cast<int16_t *>(initial_val_ptr))[0] = _sym[lit].LiteralD->Value;
        return kERR_None;
    case 4:
        (static_cast<int32_t *>(initial_val_ptr))[0] = _sym[lit].LiteralD->Value;
        return kERR_None;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_OldString(void *&initial_val_ptr)
{
    Symbol string_lit = _src.GetNext();
    if (_sym.IsConstant(string_lit))
        string_lit = _sym[string_lit].ConstantD->ValueSym;
    if (!_sym.IsLiteral(string_lit) ||
        _sym.VartypeWith(VTT::kConst, kKW_String) != _sym[string_lit].LiteralD->Vartype)
    {
        Error("Expected a string literal after '=', found '%s' instead", _sym.GetName(_src.PeekNext()).c_str());
        return kERR_UserError;
    }

    // The scanner has put the constant string into the strings table. That's where we must find and get it.
    std::string const lit_value = &(_scrip.strings[_sym[string_lit].LiteralD->Value]);

    if (lit_value.length() >= STRINGBUFFER_LENGTH)
    {
        Error(
            "Initializer string is too long (max. chars allowed: %d",
            STRINGBUFFER_LENGTH - 1);
        return kERR_UserError;
    }
    initial_val_ptr = malloc(STRINGBUFFER_LENGTH);
    if (!initial_val_ptr)
    {
        Error("Out of memory");
        return kERR_UserError;
    }
    char *init_ptr = static_cast<char *>(initial_val_ptr);
    // Unfortunately, vanilla C++ does not provide for strncpy_s(), so we can't use it.
    std::strncpy(init_ptr, lit_value.c_str(), STRINGBUFFER_LENGTH);
    init_ptr[STRINGBUFFER_LENGTH - 1] = '\0';

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl_InitialValAssignment(Symbol varname, void *&initial_val_ptr)
{
    initial_val_ptr = nullptr;
    _src.GetNext(); // Eat '='

    Vartype const vartype = _sym.GetVartype(varname);
    if (_sym.IsManagedVartype(vartype))
        return Expect(kKW_Null, _src.GetNext());

    if (_sym.IsStructVartype(vartype))
    {
        Error("'%s' is a struct and cannot be initialized here", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    if (_sym.IsArrayVartype(vartype))
    {
        Error("'%s' is an array and cannot be initialized here", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    if (kKW_String == vartype)
        return ParseVardecl_InitialValAssignment_OldString(initial_val_ptr);

    if (_sym.IsAnyIntegerVartype(vartype) || kKW_Float == vartype)
        return ParseVardecl_InitialValAssignment_IntVartypeOrFloat(vartype, initial_val_ptr);

    Error(
        "Variable '%s' has type '%s' and cannot be initialized here",
        _sym.GetName(varname).c_str(),
        _sym.GetName(vartype).c_str());
    return kERR_UserError;
}

AGS::ErrorType AGS::Parser::ParseVardecl_Var2SymTable(Symbol var_name, Vartype vartype, ScopeType scope_type)
{
    SymbolTableEntry &var_entry = _sym[var_name];
    _sym.MakeEntryVariable(var_name);
    var_entry.VariableD->Vartype = vartype;
    var_entry.Scope = _nest.TopLevel();
    _sym.SetDeclared(var_name, _src.GetCursor());
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl_CheckIllegalCombis(Vartype vartype, ScopeType scope_type)
{
    if (vartype == kKW_String && FlagIsSet(_options, SCOPT_OLDSTRINGS) == 0)
    {
        Error("Type 'string' is no longer supported; use String instead");
        return kERR_UserError;
    }

    if (vartype == kKW_String && ScT::kImport == scope_type)
    {
        // cannot import because string is really char *, and the pointer won't resolve properly
        Error("Cannot import string; use char[] instead");
        return kERR_UserError;
    }

    if (vartype == kKW_Void)
    {
        Error("'void' is not a valid type in this context");
        return kERR_UserError;
    }

    return kERR_None;
}

// there was a forward declaration -- check that the real declaration matches it
AGS::ErrorType AGS::Parser::ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info, bool body_follows)
{
    if (nullptr != known_info->ConstantD)
    {
        Error(
            ReferenceMsgLoc("The name '%s' is declared as a constant elsewhere, as a variable here", known_info->Declared).c_str(),
            known_info->Name.c_str());
        return kERR_UserError;
    }
    if (nullptr != known_info->FunctionD)
    {
        Error(
            ReferenceMsgLoc("The name '%s' is declared as a function elsewhere, as a variable here", known_info->Declared).c_str(),
            known_info->Name.c_str());
        return kERR_UserError;
    }
    if (nullptr != known_info->VartypeD)
    {
        Error(
            ReferenceMsgLoc("The name '%s' is declared as a type elsewhere, as a variable here", known_info->Declared).c_str(),
            known_info->Name.c_str());
        return kERR_UserError;
    }
    
    if (nullptr == known_info->VariableD)
        return kERR_None; // We don't have any known info

    TypeQualifierSet known_tq = known_info->VariableD->TypeQualifiers;
    known_tq[TQ::kImport] = false;
    TypeQualifierSet this_tq = this_entry->VariableD->TypeQualifiers;
    this_tq[TQ::kImport] = false;
    if (known_tq != this_tq)
    {
        std::string const ki_tq = TypeQualifierSet2String(known_tq);
        std::string const te_tq = TypeQualifierSet2String(this_tq);
        std::string msg = ReferenceMsgLoc(
            "The variable '%s' has the qualifiers '%s' here, but '%s' elsewhere",
            known_info->Declared);
        Error(msg.c_str(), te_tq.c_str(), ki_tq.c_str());
        return kERR_UserError;
    }

    if (known_info->VariableD->Vartype != this_entry->VariableD->Vartype)
    {
        // This will check the array lengths, too
        std::string msg = ReferenceMsgLoc(
            "This variable is declared as '%s' here, as '%s' elsewhere",
            known_info->Declared);
        Error(
            msg.c_str(),
            _sym.GetName(this_entry->VariableD->Vartype).c_str(),
            _sym.GetName(known_info->VariableD->Vartype).c_str());
        return kERR_UserError;
    }

    // Note, if the variables have the same vartype, they must also have the same size because size is a vartype property.

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl_Import(Symbol var_name)
{
    if (kKW_Assign == _src.PeekNext())
    {
        Error("Imported variables cannot have any initial assignment");
        return kERR_UserError;
    }

    if (_givm[var_name])
    {
        // This isn't really an import, so reset the flag and don't mark it for import
        _sym[var_name].VariableD->TypeQualifiers[TQ::kImport] = false;
        return kERR_None;
    }

    _sym[var_name].VariableD->TypeQualifiers[TQ::kImport] = true;
    int const import_offset = _scrip.FindOrAddImport(_sym.GetName(var_name));
    if (import_offset < 0)
    {
        Error("!Import table overflow");
        return kERR_InternalError;
    }
    _sym[var_name].VariableD->Offset = static_cast<size_t>(import_offset);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl_Global(Symbol var_name, Vartype vartype, void *&initial_val_ptr)
{

    if (kKW_Assign == _src.PeekNext())
    {
        ErrorType retval = ParseVardecl_InitialValAssignment(var_name, initial_val_ptr);
        if (retval < 0) return retval;
    }
    SymbolTableEntry &entry = _sym[var_name];
    entry.VariableD->Vartype = vartype;
    size_t const var_size = _sym.GetSize(vartype);
    int const global_offset = _scrip.AddGlobal(var_size, initial_val_ptr);
    if (global_offset < 0)
    {
        Error("!Cannot allocate global variable");
        return kERR_InternalError;
    }
    entry.VariableD->Offset = static_cast<size_t>(global_offset);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl_Local(Symbol var_name, Vartype vartype)
{
    size_t const var_size = _sym.GetSize(vartype);
    bool const is_dyn = _sym.IsDynVartype(vartype);

    _sym[var_name].VariableD->Offset = _scrip.OffsetToLocalVarBlock;

    if (kKW_Assign != _src.PeekNext())
    {
        // Initialize the variable with binary zeroes.
        WriteCmd(SCMD_LOADSPOFFS, 0);
        if (is_dyn)
            WriteCmd(SCMD_MEMZEROPTR);
        else
            WriteCmd(SCMD_ZEROMEMORY, var_size);
        WriteCmd(SCMD_ADD, SREG_SP, var_size);
        _scrip.OffsetToLocalVarBlock += var_size;
        return kERR_None;
    }

    // "readonly" vars can't be assigned to, so don't use standard assignment function here.
    _src.GetNext(); // Eat '='
    ErrorType retval = ParseExpression();
    if (retval < 0) return retval;

    // Vartypes must match. This is true even if the lhs is readonly.
    // As a special case, a string may be assigned a const string because the const string will be copied, not modified.
    Vartype const lhsvartype = vartype;
    Vartype const rhsvartype = _scrip.AX_Vartype;

    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype) &&
        !(kKW_String == _sym.VartypeWithout(VTT::kConst, rhsvartype) &&
          kKW_String == _sym.VartypeWithout(VTT::kConst, lhsvartype)))
    {
        Error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhsvartype).c_str(),
            _sym.GetName(lhsvartype).c_str());
        return kERR_UserError;
    }

    if (SIZE_OF_INT == var_size && !is_dyn)
    {
        // This PUSH moves the result of the initializing expression into the
        // new variable and reserves space for this variable on the stack.
        PushReg(SREG_AX);
        return kERR_None;
    }

    ConvertAXStringToStringObject(vartype);
    WriteCmd(SCMD_LOADSPOFFS, 0);
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, lhsvartype))
        AccessData_StrCpy();
    else
        WriteCmd(
            is_dyn ? SCMD_MEMWRITEPTR : GetWriteCommandForSize(var_size),
            SREG_AX);
    WriteCmd(SCMD_ADD, SREG_SP, var_size);
    _scrip.OffsetToLocalVarBlock += var_size;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl0(Symbol var_name, Vartype vartype, ScopeType scope_type, TypeQualifierSet tqs)
{
    if (kKW_OpenBracket == _src.PeekNext())
    {
        ErrorType retval = ParseArray(var_name, vartype);
        if (retval < 0) return retval;
    }

    // Enter the variable into the symbol table
    ErrorType retval = ParseVardecl_Var2SymTable(var_name, vartype, scope_type);
    if (retval < 0) return retval;
    _sym[var_name].VariableD->TypeQualifiers = tqs;

    switch (scope_type)
    {
    default:
        Error("!Wrong scope type");
        return kERR_InternalError;

    case ScT::kGlobal:
    {
        void *initial_val_ptr = nullptr;
        retval = ParseVardecl_Global(var_name, vartype, initial_val_ptr);
        if (initial_val_ptr) free(initial_val_ptr);
        return retval;
    }

    case ScT::kImport:
        return ParseVardecl_Import(var_name);

    case ScT::kLocal:
        return ParseVardecl_Local(var_name, vartype);
    }
}

AGS::ErrorType AGS::Parser::ParseVardecl_CheckAndStashOldDefn(Symbol var_name)
{
    do // exactly 1 times
    {
        if (_sym.IsFunction(var_name))
        {
            Warning(
                ReferenceMsgSym("This hides the function '%s()'", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            break;
        }

        if (_sym.IsPredefined(var_name))
        {
            Error("Mustn't redefine the predefined '%s'", _sym.GetName(var_name));
            return kERR_UserError;
        }

        if (_sym.IsVariable(var_name))
            break;

        if (_sym.IsVartype(var_name))
        {
            Error(
                ReferenceMsgSym("'%s' is in use as a type elsewhere", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }

        if (!_sym.IsInUse(var_name))
            break;

        Error(
            ReferenceMsgSym("'%s' is already in use elsewhere", var_name).c_str(),
            _sym.GetName(var_name).c_str());
        return kERR_UserError;
    }
    while (false);

    if (_nest.TopLevel() == _sym[var_name].Scope)
    {
        Error(
            ReferenceMsgSym("'%s' has already been defined in this scope", var_name).c_str(),
            _sym.GetName(var_name).c_str());
        return kERR_UserError;
    }

    if (SymbolTable::kParameterScope == _sym[var_name].Scope && SymbolTable::kFunctionScope == _nest.TopLevel())
    {
        Error(
            ReferenceMsgSym("'%s' has already been defined as a parameter", var_name).c_str(),
            _sym.GetName(var_name).c_str());
        return kERR_UserError;
    }

    if (_nest.AddOldDefinition(var_name, _sym[var_name]))
    {
        Error("!AddOldDefinition: Storage place occupied");
        return kERR_InternalError;
    }
    _sym[var_name].Clear();
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVardecl(Symbol var_name, Vartype vartype, ScopeType scope_type, TypeQualifierSet tqs)
{
    ErrorType retval = ParseVardecl_CheckIllegalCombis(vartype, scope_type);
    if (retval < 0) return retval;

    if (ScT::kLocal == scope_type)
    {
        retval = ParseVardecl_CheckAndStashOldDefn(var_name);
        if (retval < 0) return retval;
    }

    SymbolTableEntry known_info = _sym[var_name];
    retval = ParseVardecl0(var_name, vartype, scope_type, tqs);
    if (retval < 0) return retval;
    if (ScT::kLocal != scope_type)
        return ParseVardecl_CheckThatKnownInfoMatches(&_sym[var_name], &known_info, false);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseFuncBodyStart(Symbol struct_of_func,Symbol name_of_func)
{
    _nest.Push(NSType::kFunction);

    // write base address of function for any relocation needed later
    WriteCmd(SCMD_THISBASE, _scrip.codesize);
    SymbolTableEntry &entry = _sym[name_of_func];
    if (entry.FunctionD->NoLoopCheck)
        WriteCmd(SCMD_LOOPCHECKOFF);

    // If there are dynpointer parameters, then the caller has simply "pushed" them onto the stack.
    // We catch up here by reading each dynpointer and writing it again using MEMINITPTR
    // to declare that the respective cells will from now on be used for dynpointers.
    size_t const num_params = _sym.NumOfFuncParams(name_of_func);
    for (size_t param_idx = 1; param_idx <= num_params; param_idx++) // skip return value param_idx == 0
    {
        Vartype const param_vartype = _sym[name_of_func].FunctionD->Parameters[param_idx].Vartype;
        if (!_sym.IsDynVartype(param_vartype))
            continue;

        // The return address is on top of the stack, so the nth param is at (n+1)th position
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_STACK_CELL * (param_idx + 1));
        WriteCmd(SCMD_MEMREAD, SREG_AX); // Read the address that is stored there
        // Create a dynpointer that points to the same object as m[AX] and store it in m[MAR]
        WriteCmd(SCMD_MEMINITPTR, SREG_AX);
    }

    SymbolTableEntry &this_entry = _sym[kKW_This];
    this_entry.VariableD->Vartype = kKW_NoSymbol;
    if (struct_of_func > 0 && !_sym[name_of_func].FunctionD->TypeQualifiers[TQ::kStatic])
    {
        // Declare "this" but do not allocate memory for it
        this_entry.Scope = 0u;
        this_entry.Accessed = true; 
        this_entry.VariableD->Vartype = struct_of_func; // Don't declare this as dynpointer
        this_entry.VariableD->TypeQualifiers = {};
        this_entry.VariableD->TypeQualifiers[TQ::kReadonly] = true;
        this_entry.VariableD->Offset = 0u;
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::HandleEndOfFuncBody(Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    // Free all the dynpointers in parameters and locals. 
    FreeDynpointersOfLocals(1u);
    // Pop the local variables proper from the stack but leave the parameters.
    // This is important because the return address is directly above the parameters;
    // we need the return address to return. (The caller will pop the parameters later.)
    RemoveLocalsFromStack(_sym.kFunctionScope);
    // All the function variables, _including_ the parameters, become invalid.
    RestoreLocalsFromSymtable(_sym.kParameterScope);

    // Function has ended. Set AX to 0 unless the function doesn't return any value.
    if (kKW_Void != _sym[name_of_current_func].FunctionD->Parameters.at(0u).Vartype)
        WriteCmd(SCMD_LITTOREG, SREG_AX, 0);

    // We've just finished the body of the current function.
    name_of_current_func = kKW_NoSymbol;
    struct_of_current_func = _sym[kKW_This].VariableD->Vartype = kKW_NoSymbol;
    

    _nest.Pop();    // End function variables nesting
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();    // End function parameters nesting

    WriteCmd(SCMD_RET);
    // This has popped the return address from the stack, 
    // so adjust the offset to the start of the parameters.
    _scrip.OffsetToLocalVarBlock -= SIZE_OF_STACK_CELL;

    return kERR_None;
}

void AGS::Parser::ParseStruct_SetTypeInSymboltable(Symbol stname, TypeQualifierSet tqs)
{
    SymbolTableEntry &entry = _sym[stname];
    _sym.MakeEntryVartype(stname);

    entry.VartypeD->Parent = kKW_NoSymbol;
    entry.VartypeD->Size = 0u;
    entry.Declared = _src.GetCursor();

    auto &flags = entry.VartypeD->Flags;
    flags[VTF::kUndefined] = true; // Not completely defined yet
    flags[VTF::kStruct] = true;
    if (tqs[TQ::kManaged])
        flags[VTF::kManaged] = true;
    if (tqs[TQ::kBuiltin])
        flags[VTF::kBuiltin] = true;
    if (tqs[TQ::kAutoptr])
        flags[VTF::kAutoptr] = true;

    _sym.SetDeclared(stname, _src.GetCursor());
}

// We have accepted something like "struct foo" and are waiting for "extends"
AGS::ErrorType AGS::Parser::ParseStruct_ExtendsClause(Symbol stname)
{
    _src.GetNext(); // Eat "extends"
    Symbol const parent = _src.GetNext(); // name of the extended struct

    if (PP::kPreAnalyze == _pp)
        return kERR_None; // No further analysis necessary in first phase

    if (!_sym.IsStructVartype(parent))
    {
        Error(ReferenceMsgSym("Expected a struct type, found '%s' instead", parent).c_str(), _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    if (!_sym.IsManagedVartype(parent) && _sym.IsManagedVartype(stname))
    {
        Error("Managed struct cannot extend the unmanaged struct '%s'", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    if (_sym.IsManagedVartype(parent) && !_sym.IsManagedVartype(stname))
    {
        Error("Unmanaged struct cannot extend the managed struct '%s'", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    if (_sym.IsBuiltinVartype(parent) && !_sym.IsBuiltinVartype(stname))
    {
        Error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    _sym[stname].VartypeD->Size = _sym.GetSize(parent);
    _sym[stname].VartypeD->Parent = parent;
    return kERR_None;
}

// Check whether the qualifiers that accumulated for this decl go together
AGS::ErrorType AGS::Parser::Parse_CheckTQ(TypeQualifierSet tqs, bool in_func_body, bool in_struct_decl)
{
    if (in_struct_decl)
    {
        TypeQualifier error_tq;
        if (tqs[(error_tq = TQ::kBuiltin)] ||
            tqs[(error_tq = TQ::kStringstruct)])
        {
            Error("'%s' is illegal in a struct declaration", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
            return kERR_UserError;
        }
    }
    else // !in_struct_decl
    {
        TypeQualifier error_tq;
        if (tqs[(error_tq = TQ::kAttribute)] ||
            tqs[(error_tq = TQ::kProtected)] ||
            tqs[(error_tq = TQ::kWriteprotected)])
        {
            Error("'%s' is only legal in a struct declaration", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
            return kERR_UserError;
        }
    }

    if (in_func_body)
    {
        TypeQualifier error_tq;
        if (tqs[(error_tq = TQ::kAutoptr)] ||
            tqs[(error_tq = TQ::kBuiltin)] ||
            tqs[(error_tq = TQ::kImport)] ||
            tqs[(error_tq = TQ::kManaged)] ||
            tqs[(error_tq = TQ::kStatic)] ||
            tqs[(error_tq = TQ::kStringstruct)])
        {
            Error("'%s' is illegal in a function body", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
            return kERR_UserError;
        }
    }

    if (1 < tqs[TQ::kProtected] + tqs[TQ::kWriteprotected] + tqs[TQ::kReadonly])
    {
        Error("Can only use one out of 'protected', 'readonly', and 'writeprotected'");
        return kERR_UserError;
    }

    if (tqs[TQ::kAutoptr])
    {
        if (!tqs[TQ::kBuiltin] || !tqs[TQ::kManaged])
        {
            Error("'autoptr' must be combined with 'builtin' and 'managed'");
            return kERR_UserError;
        }
    }

    // Note: 'builtin' does not always presuppose 'managed'

    if (tqs[TQ::kStringstruct] && (!tqs[TQ::kAutoptr]))
    {
        Error("'stringstruct' must be combined with 'autoptr'");
        return kERR_UserError;
    }

    if (tqs[TQ::kConst])
    {
        Error("'const' can only be used for a function parameter (use 'readonly' instead)");
        return kERR_UserError;
    }

    if (tqs[TQ::kImport] && tqs[TQ::kStringstruct])
    {
        Error("Cannot combine 'import' and 'stringstruct'");
        return kERR_UserError;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::Parse_CheckTQSIsEmpty(TypeQualifierSet tqs)
{
    for (auto it = tqs.begin(); it != tqs.end(); it++)
    {
        if (!tqs[it->first])
            continue;
        Error("Unexpected '%s' before a command", _sym.GetName(it->second).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseQualifiers(TypeQualifierSet &tqs)
{
    bool istd_found = false;
    bool itry_found = false;
    tqs = {};
    while (!_src.ReachedEOF())
    {
        Symbol peeksym = _src.PeekNext();
        switch (peeksym)
        {
        default: return kERR_None;
        case kKW_Attribute:      tqs[TQ::kAttribute] = true; break;
        case kKW_Autoptr:        tqs[TQ::kAutoptr] = true; break;
        case kKW_Builtin:        tqs[TQ::kBuiltin] = true; break;
        case kKW_Const:          tqs[TQ::kConst] = true; break;
        case kKW_ImportStd:      tqs[TQ::kImport] = true; istd_found = true;  break;
        case kKW_ImportTry:      tqs[TQ::kImport] = true; itry_found = true;  break;
        case kKW_Internalstring: tqs[TQ::kStringstruct] = true; break;
        case kKW_Managed:        tqs[TQ::kManaged] = true; break;
        case kKW_Protected:      tqs[TQ::kProtected] = true; break;
        case kKW_Readonly:       tqs[TQ::kReadonly] = true; break;
        case kKW_Static:         tqs[TQ::kStatic] = true; break;
        case kKW_Writeprotected: tqs[TQ::kWriteprotected] = true; break;
        } // switch (_sym.GetSymbolType(peeksym))

        _src.GetNext();
        if (istd_found && itry_found)
        {
            Error("Cannot both use 'import' and '_tryimport'");
            return kERR_UserError;
        }
    };

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseStruct_CheckComponentVartype(Symbol stname, Vartype vartype)
{
    if (vartype == stname && !_sym.IsManagedVartype(vartype))
    {
        // cannot do "struct A { A varname; }", this struct would be infinitely large
        Error("Struct '%s' cannot be a member of itself", _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    if (!_sym.IsVartype(vartype))
    {
        Error(
            ReferenceMsgSym("Expected a type, found '%s' instead", vartype).c_str(),
             _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseStruct_FuncDecl(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, Vartype vartype)
{
    if (tqs[TQ::kWriteprotected])
    {
        Error("'writeprotected' does not apply to functions");
        return kERR_UserError;
    }

    size_t const declaration_start = _src.GetCursor();
    _src.GetNext(); // Eat '('

    bool body_follows;
    ErrorType retval = ParseFuncdecl(declaration_start, tqs, vartype, struct_of_func, name_of_func, false, body_follows);
    if (retval < 0) return retval;
    if (body_follows)
    {
        Error("Cannot code a function body within a struct definition");
        return kERR_UserError;
    }

    return Expect(kKW_Semicolon, _src.PeekNext());
}

AGS::ErrorType AGS::Parser::ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    SymbolTableEntry &entry = _sym[name_of_func];
    size_t const num_parameters_wanted = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);
    if (num_parameters_wanted != _sym.NumOfFuncParams(name_of_func))
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' should have %d parameter(s) but is declared with %d parameter(s) instead",
            name_of_func);
        Error(msg.c_str(), entry.Name.c_str(), num_parameters_wanted, _sym.NumOfFuncParams(name_of_func));
        return kERR_UserError;
    }

    Vartype const ret_vartype = is_setter ? kKW_Void : vartype;
    if (ret_vartype != _sym.FuncReturnVartype(name_of_func))
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' must return type '%s' but returns '%s' instead",
            name_of_func);
        Error(msg.c_str(),
            entry.Name.c_str(),
            _sym.GetName(ret_vartype).c_str(),
            _sym.GetName(_sym.FuncReturnVartype(name_of_func)).c_str());
        return kERR_UserError;
    }

    size_t p_idx = 1;
    if (is_indexed)
    {
        auto actual_vartype = entry.FunctionD->Parameters[p_idx].Vartype;
        if (kKW_Int != actual_vartype)
        {
            std::string const msg = ReferenceMsgSym(
                "Parameter #%d of attribute function '%s' must have type 'int' but has type '%s' instead",
                name_of_func);
            Error(msg.c_str(), p_idx, entry.Name.c_str(), _sym.GetName(actual_vartype).c_str());
            return kERR_UserError;
        }
        p_idx++;
    }

    if (!is_setter)
        return kERR_None;

    auto const actual_vartype = entry.FunctionD->Parameters[p_idx].Vartype;
    if (vartype != actual_vartype)
    {
        std::string const msg = ReferenceMsgSym(
            "Parameter #%d of attribute function '%s' must have type '%s' but has type '%s' instead",
            name_of_func);
        Error(msg.c_str(), p_idx, entry.Name.c_str(), _sym.GetName(vartype).c_str(), _sym.GetName(actual_vartype).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseStruct_Attribute_ParamList(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    auto &parameters = _sym[name_of_func].FunctionD->Parameters;
    FuncParameterDesc fpd = {};
    if (is_indexed)
    {
        fpd.Vartype = kKW_Int;
        parameters.push_back(fpd);
    }
    if (is_setter)
    {
        fpd.Vartype = vartype;
        parameters.push_back(fpd);
    }
    return kERR_None;
}

// We are processing an attribute.
// This corresponds to a getter func and a setter func, declare one of them
AGS::ErrorType AGS::Parser::ParseStruct_Attribute_DeclareFunc(TypeQualifierSet tqs, Symbol strct, Symbol qualified_name, Symbol unqualified_name, bool is_setter, bool is_indexed, Vartype vartype)
{
    // If this symbol has been defined before, check whether the definitions clash
    if (_sym.IsInUse(qualified_name) && !_sym.IsFunction(qualified_name))
    {
        std::string msg = ReferenceMsgSym(
            "Attribute uses '%s' as a function, this clashes with a declaration elsewhere",
            qualified_name);
        Error(msg.c_str(), _sym[qualified_name].Name.c_str());
        return kERR_UserError;
    }
    if (_sym.IsFunction(qualified_name))
    {
        ErrorType retval = ParseStruct_Attribute_CheckFunc(qualified_name, is_setter, is_indexed, vartype);
        if (retval < 0) return retval;
    }

    tqs[TQ::kImport] = true; // Assume that attribute functions are imported
    if (tqs[TQ::kImport] &&
        _sym.IsFunction(qualified_name) &&
        !_sym[qualified_name].FunctionD->TypeQualifiers[TQ::kImport])
    {
        if (FlagIsSet(_options, SCOPT_NOIMPORTOVERRIDE))
        {
            std::string const msg = ReferenceMsgSym(
                "In here, attribute functions may not be defined locally",
                qualified_name);
            Error(msg.c_str());
            return kERR_UserError;
        }
        tqs[TQ::kImport] = false;
    }

    // Store the fact that this function has been declared within the struct declaration
    _sym.MakeEntryComponent(qualified_name);
    _sym[qualified_name].ComponentD->Parent = strct;
    _sym[qualified_name].ComponentD->Component = unqualified_name;
    _sym[qualified_name].ComponentD->IsFunction = true;
    _sym[strct].VartypeD->Components[unqualified_name] = qualified_name;

    Vartype const return_vartype = is_setter ? kKW_Void : vartype;
    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, strct, qualified_name, false);

    ErrorType retval = ParseStruct_Attribute_ParamList(strct, qualified_name, is_setter, is_indexed, vartype);
    if (retval < 0) return retval;

    bool const body_follows = false; // we are within a struct definition
    return ParseFuncdecl_HandleFunctionOrImportIndex(tqs, strct, qualified_name, body_follows);
}

// We're in a struct declaration, parsing a struct attribute
AGS::ErrorType AGS::Parser::ParseStruct_Attribute(TypeQualifierSet tqs, Symbol stname, Symbol vname, Vartype vartype)
{
    size_t const declaration_start = _src.GetCursor();
    // "readonly" means that there isn't a setter function. The individual vartypes are not readonly.
    bool const attrib_is_readonly = tqs[TQ::kReadonly];
    tqs[TQ::kAttribute] = false;
    tqs[TQ::kReadonly] = false;

    bool attrib_is_indexed = false;
    if (kKW_OpenBracket == _src.PeekNext())
    {
        attrib_is_indexed = true;
        _src.GetNext();
        if (kKW_CloseBracket != _src.GetNext())
        {
            Error("Cannot specify array sizes for an attribute");
            return kERR_UserError;
        }
    }

    if (PP::kMain == _pp && attrib_is_indexed)
        _sym[vname].VariableD->Vartype = _sym.VartypeWith(VTT::kDynarray, vartype);

    // Declare attribute getter, e.g. get_ATTRIB()
    Symbol unqualified_func = kKW_NoSymbol;
    bool const get_func_is_setter = false;
    ErrorType retval = ConstructAttributeFuncName(vname, get_func_is_setter, attrib_is_indexed, unqualified_func);
    if (retval < 0) return retval;
    Symbol const get_func = MangleStructAndComponent(stname, unqualified_func);
    retval = ParseStruct_Attribute_DeclareFunc(tqs, stname, get_func, unqualified_func, get_func_is_setter, attrib_is_indexed, vartype);
    if (retval < 0) return retval;
    _sym.SetDeclared(get_func, declaration_start);

    if (attrib_is_readonly)
        return kERR_None;

    // Declare attribute setter, e.g. set_ATTRIB(value)
    bool const set_func_is_setter = true;
    retval = ConstructAttributeFuncName(vname, set_func_is_setter, attrib_is_indexed, unqualified_func);
    if (retval < 0) return retval;
    Symbol const set_func = MangleStructAndComponent(stname, unqualified_func);
    retval = ParseStruct_Attribute_DeclareFunc(tqs, stname, set_func, unqualified_func, set_func_is_setter, attrib_is_indexed, vartype);
    if (retval < 0) return retval;
    _sym.SetDeclared(set_func, declaration_start);

    return kERR_None;
}

// We're parsing an array var.
AGS::ErrorType AGS::Parser::ParseArray(Symbol vname, Vartype &vartype)
{
    _src.GetNext(); // Eat '['

    if (PP::kPreAnalyze == _pp)
    {
        // Skip the sequence of [...]
        while (true)
        {
            ErrorType retval = SkipToClose(kKW_CloseBracket);
            if (retval < 0) return retval;
            if (kKW_OpenBracket != _src.PeekNext())
                return kERR_None;
            _src.GetNext(); // Eat '['
        }
    }

    if (kKW_CloseBracket == _src.PeekNext())
    {
        // Dynamic array
        _src.GetNext(); // Eat ']'
        if (vartype == kKW_String)
        {
            Error("Dynamic arrays of old-style strings are not supported");
            return kERR_UserError;
        }
        if (!_sym.IsAnyIntegerVartype(vartype) && !_sym.IsManagedVartype(vartype) && kKW_Float != vartype)
        {
            Error("Can only have dynamic arrays of integer types, 'float', or managed structs. '%s' isn't any of this.", _sym.GetName(vartype).c_str());
            return kERR_UserError;
        }
        vartype = _sym.VartypeWith(VTT::kDynarray, vartype);
        return kERR_None;
    }

    std::vector<size_t> dims;

    // Static array
    while (true)
    {
        Symbol lit;
        std::string msg = "For dimension #<dim> of array '<arr>': ";
        msg.replace(msg.find("<dim>"), 5u, std::to_string(dims.size()));
        msg.replace(msg.find("<arr>"), 5u, _sym.GetName(vname).c_str());
        
        ErrorType retval = ReadIntLiteralOrConst(lit, msg);
        if (retval < 0) return retval;
        
        CodeCell const dimension_as_int = _sym[lit].LiteralD->Value;
        if (dimension_as_int < 1)
        {
            Error(
                "Array dimension #&u of array '%s' must be at least 1 but is %d instead",
                dims.size(),
                _sym.GetName(vname).c_str(),
                dimension_as_int);
            return kERR_UserError;
        }

        dims.push_back(dimension_as_int);

        Symbol const punctuation = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_CloseBracket }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Comma == punctuation)
            continue;
        if (kKW_OpenBracket != _src.PeekNext())
            break;
        _src.GetNext(); // Eat '['
    }
    vartype = _sym.VartypeWithArray(dims, vartype);
    return kERR_None;
}

// We're inside a struct decl, processing a member variable or a member attribute
AGS::ErrorType AGS::Parser::ParseStruct_VariableOrAttributeDefn(TypeQualifierSet tqs, Vartype vartype, Symbol name_of_struct, Symbol vname)
{
    if (_sym.IsDynarrayVartype(vartype)) // e.g., int [] zonk;
    {
        Error("Expected '('");
        return kERR_UserError;
    }

    if (PP::kMain == _pp)
    {
        if (tqs[TQ::kImport] && !tqs[TQ::kAttribute])
        {
            // member variable cannot be an import
            Error("Can't import struct component variables; import the whole struct instead");
            return kERR_UserError;
        }

        if (_sym.IsManagedVartype(vartype) && _sym.IsManagedVartype(name_of_struct) && !tqs[TQ::kAttribute])
        {
            // This is an Engine restriction
            Error("Cannot currently have managed variable components in managed struct");
            return kERR_UserError;
        }

        if (_sym.IsBuiltinVartype(vartype) && !_sym.IsManagedVartype(vartype))
        {
            // Non-managed built-in vartypes do exist
            Error("May not have a component variable of the non-managed built-in type '%s'", _sym.GetName(vartype).c_str());
            return kERR_UserError;
        }
        
        SymbolTableEntry &entry = _sym[vname];
        if (!tqs[TQ::kAttribute])
            entry.ComponentD->Offset = _sym[name_of_struct].VartypeD->Size;

        _sym.MakeEntryVariable(vname);
        entry.VariableD->Vartype = vartype;
        entry.VariableD->TypeQualifiers = tqs;
        // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the variable having the vartype
        entry.VariableD->TypeQualifiers[TQ::kAutoptr] = false;
        entry.VariableD->TypeQualifiers[TQ::kManaged] = false;
        entry.VariableD->TypeQualifiers[TQ::kBuiltin] = false;
    }

    if (tqs[TQ::kAttribute])
        return ParseStruct_Attribute(tqs, name_of_struct, vname, vartype);

    if (PP::kMain != _pp)
        return SkipTo(SymbolList{ kKW_Comma, kKW_Semicolon }, _src);

    if (_src.PeekNext() == kKW_OpenBracket)
    {
        Vartype vartype = _sym[vname].VariableD->Vartype;
        ErrorType retval = ParseArray(vname, vartype);
        if (retval < 0) return retval;
        _sym[vname].VariableD->Vartype = vartype;
    }

    _sym[name_of_struct].VartypeD->Size += _sym.GetSize(vname);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseStruct_MemberDefn(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype)
{
    size_t const declaration_start = _src.GetCursor();

    // Get the variable or function name.
    Symbol unqualified_component;
    ErrorType retval = ParseVarname(unqualified_component);
    if (retval < 0) return retval;
    Symbol const qualified_component = MangleStructAndComponent(name_of_struct, unqualified_component);

    bool const is_function = (kKW_OpenParenthesis == _src.PeekNext());

    if (PP::kMain == _pp)
    {
        if (!is_function && _sym.IsInUse(qualified_component))
        {
            std::string const msg = ReferenceMsgSym(
                "'%s' is already defined", qualified_component);
            Error(msg.c_str(), _sym.GetName(qualified_component).c_str());
            return kERR_UserError;
        }

        // Mustn't be in any ancester
        Symbol const parent = FindStructOfComponent(name_of_struct, unqualified_component);
        if (kKW_NoSymbol != parent)
        {
            Error(
                ReferenceMsgSym(
                    "The struct '%s' extends '%s', and '%s' is already defined",
                    parent).c_str(),
                _sym.GetName(name_of_struct).c_str(),
                _sym.GetName(parent).c_str(),
                _sym.GetName(qualified_component).c_str());
            return kERR_UserError;
        }
    }

    _sym.MakeEntryComponent(qualified_component);
    _sym[qualified_component].ComponentD->Component = unqualified_component;
    _sym[qualified_component].ComponentD->Parent = name_of_struct;
    _sym[qualified_component].ComponentD->IsFunction = is_function;
    _sym[name_of_struct].VartypeD->Components[unqualified_component] = qualified_component;
    _sym.SetDeclared(qualified_component, declaration_start);

    if (is_function)
        return ParseStruct_FuncDecl(name_of_struct, qualified_component, tqs, vartype);

    return ParseStruct_VariableOrAttributeDefn(tqs, vartype, name_of_struct, qualified_component);
 }

AGS::ErrorType AGS::Parser::EatDynpointerSymbolIfPresent(Vartype vartype)
{
    if (kKW_Dynpointer != _src.PeekNext())
        return kERR_None;

    if (PP::kPreAnalyze == _pp || _sym.IsManagedVartype(vartype))
    {
        _src.GetNext(); // Eat '*'
        return kERR_None;
    }

    Error("Cannot use '*' on the non-managed type '%s'", _sym.GetName(vartype).c_str());
    return kERR_UserError;
}

AGS::ErrorType AGS::Parser::ParseStruct_Vartype(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype)
{
    if (PP::kMain == _pp)
    {   // Check for illegal struct member types
        ErrorType retval = ParseStruct_CheckComponentVartype(name_of_struct, vartype);
        if (retval < 0) return retval;
    }

    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    // "int [] func(...)"
    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    // "TYPE noloopcheck foo(...)"
    if (kKW_Noloopcheck == _src.PeekNext())
    {
        Error("Cannot use 'noloopcheck' here");
        return kERR_UserError;
    }  

    // We've accepted a type expression and are now reading vars or one func that should have this type.
    while (true)
    {
        retval = ParseStruct_MemberDefn(name_of_struct, tqs, vartype);
        if (retval < 0) return retval;

        Symbol const punctuation = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Semicolon == punctuation)
            return kERR_None;
    }
}

// Handle a "struct" definition; we've already eaten the keyword "struct"
AGS::ErrorType AGS::Parser::ParseStruct(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    size_t const start_of_struct_decl = _src.GetCursor();

    // get token for name of struct
    Symbol const stname = _src.GetNext();

    if (!(_sym.IsVartype(stname) && _sym[stname].VartypeD->Flags[VTF::kUndefined]) &&
        _sym.IsInUse(stname))
    {
        std::string const msg = ReferenceMsgSym("'%s' is already defined", stname);
        Error(msg.c_str(), _sym.GetName(stname).c_str());
        return kERR_UserError;
    }

    ParseStruct_SetTypeInSymboltable(stname, tqs);

    // Declare the struct type that implements new strings
    if (tqs[TQ::kStringstruct])
    {
        if (_sym.GetStringStructSym() > 0 && stname != _sym.GetStringStructSym())
        {
            Error("The stringstruct type is already defined to be %s", _sym.GetName(_sym.GetStringStructSym()).c_str());
            return kERR_UserError;
        }
        _sym.SetStringStructSym(stname);
    }

    if (kKW_Extends == _src.PeekNext())
    {
        ErrorType retval = ParseStruct_ExtendsClause(stname);
        if (retval < 0) return retval;
    }

    // forward-declaration of struct type
    if (kKW_Semicolon == _src.PeekNext())
    {
        if (!tqs[TQ::kManaged])
        {
            Error("Forward-declared 'struct's must be 'managed'");
            return kERR_UserError;
        }
        _src.GetNext(); // Eat ';'
        return kERR_None;
    }

    ErrorType retval = Expect(kKW_OpenBrace, _src.GetNext());
    if (retval < 0) return retval;

    // Declaration of the components
    while (kKW_CloseBrace != _src.PeekNext())
    {
        currentline = _src.GetLinenoAt(_src.GetCursor());
        TypeQualifierSet tqs = {};
        retval = ParseQualifiers(tqs);
        if (retval < 0) return retval;
        bool const in_func_body = false;
        bool const in_struct_decl = true;
        retval = Parse_CheckTQ(tqs, in_func_body, in_struct_decl);
        if (retval < 0) return retval;

        Vartype vartype = _src.GetNext();

        retval = ParseStruct_Vartype(stname, tqs, vartype);
        if (retval < 0) return retval;
    }

    if (PP::kMain == _pp)
    {
        // round up size to nearest multiple of STRUCT_ALIGNTO
        size_t &struct_size = _sym[stname].VartypeD->Size;
        if (0 != (struct_size % STRUCT_ALIGNTO))
            struct_size += STRUCT_ALIGNTO - (struct_size % STRUCT_ALIGNTO);
    }

    _src.GetNext(); // Eat '}'

    // Struct has now been completely defined
    _sym[stname].VartypeD->Flags[VTF::kUndefined] = false;
    _structRefs.erase(stname);

    Symbol const nextsym = _src.PeekNext();
    if (kKW_Semicolon == nextsym)
    {
        if (tqs[TQ::kReadonly])
        {
            // Only now do we find out that there isn't any following declaration
            // so "readonly" was incorrect. 
            // Back up the cursor for the error message.
            _src.SetCursor(start_of_struct_decl);
            Error("'readonly' can only be used in a variable declaration");
            return kERR_UserError;
        }
        _src.GetNext(); // Eat ';'
        return kERR_None;
    }

    // If this doesn't seem to be a declaration at first glance,
    // warn that the user might have forgotten a ';'.
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input (did you forget a ';'?)");
        return kERR_UserError;
    }
    if (!(_sym.IsIdentifier(nextsym) && !_sym.IsVartype(nextsym)) &&
        kKW_Dynpointer != nextsym &&
        kKW_Noloopcheck != nextsym &&
        kKW_OpenBracket != nextsym)
    {
        Error("Unexpected '%s' (did you forget a ';'?)", _sym.GetName(nextsym).c_str());
        return kERR_UserError;
    }

    // Take struct that has just been defined as the vartype of a declaration
    return ParseVartype(stname, tqs, struct_of_current_func, name_of_current_func);
}

// We've accepted something like "enum foo { bar"; '=' follows
AGS::ErrorType AGS::Parser::ParseEnum_AssignedValue(Symbol vname, CodeCell &value)
{
    _src.GetNext(); // eat "="

    Symbol lit;
    std::string msg = "In the assignment to <name>: ";
    msg.replace(msg.find("<name>"), 6u, _sym.GetName(vname));
    ErrorType retval = ReadIntLiteralOrConst(lit, msg);
    if (retval < 0) return retval;

    value = _sym[lit].LiteralD->Value;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseEnum_Item2Symtable(Symbol enum_name,Symbol item_name, int value)
{
    Symbol value_sym;
    ErrorType retval = FindOrAddIntLiteral(value, value_sym);
    if (retval < 0) return retval;

    SymbolTableEntry &entry = _sym[item_name];
    _sym.MakeEntryConstant(item_name);

    entry.ConstantD->ValueSym = value_sym;
    entry.Scope = 0u;

    // AGS implements C-style enums, so their qualified name is identical to their unqualified name.
    _sym[enum_name].VartypeD->Components[item_name] = item_name;

    _sym.SetDeclared(item_name, _src.GetCursor());
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseEnum_Name2Symtable(Symbol enum_name)
{
    SymbolTableEntry &entry = _sym[enum_name];

    if (_sym.IsPredefined(enum_name))
    {
        Error("Expected an identifier, found the predefined symbol '%s' instead", _sym.GetName(enum_name).c_str());
        return kERR_UserError;
    }
    if (_sym.IsFunction(enum_name) || _sym.IsVartype(enum_name))
    {
        std::string msg = ReferenceMsgLoc("'%s' is already defined", entry.Declared);
        Error(msg.c_str(), _sym.GetName(enum_name).c_str());
        return kERR_UserError;
    }
    _sym.MakeEntryVartype(enum_name);

    entry.VartypeD->Size = SIZE_OF_INT;
    entry.VartypeD->BaseVartype = kKW_Int;
    entry.VartypeD->Flags[VTF::kEnum] = true;

    return kERR_None;
}

// enum eEnumName { value1, value2 };
// We've already eaten "enum"
AGS::ErrorType AGS::Parser::ParseEnum(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    size_t const start_of_enum_decl = _src.GetCursor();
    if (kKW_NoSymbol !=  name_of_current_func)
    {
        Error("Enum declaration is not allowed within a function body");
        return kERR_UserError;
    }
    if (tqs[TQ::kBuiltin])
    {
        Error("'builtin' can only be used in a struct declaration");
        return kERR_UserError;
    }

    // Get name of the enum, enter it into the symbol table
    Symbol enum_name = _src.GetNext();
    ErrorType retval = ParseEnum_Name2Symtable(enum_name);
    if (retval < 0) return retval;

    retval = Expect(kKW_OpenBrace, _src.GetNext());
    if (retval < 0) return retval;

    CodeCell current_constant_value = 0;

    while (true)
    {
        Symbol item_name = _src.GetNext();
        if (kKW_CloseBrace == item_name)
            break; // item list empty or ends with trailing ','

        if (PP::kMain == _pp)
        {
            if (_sym.IsConstant(item_name))
            {
                Error(
                    ReferenceMsgSym("'%s' is already defined as a constant or enum value", item_name).c_str(),
                    _sym.GetName(item_name).c_str());
                return kERR_UserError;
            }
            if (_sym.IsPredefined(item_name) || _sym.IsVariable(item_name) || _sym.IsFunction(item_name))
            {
                Error("Expected '}' or an unused identifier, found '%s' instead", _sym.GetName(item_name).c_str());
                return kERR_UserError;
            }
        }

        Symbol const punctuation = _src.PeekNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Assign, kKW_CloseBrace }, punctuation);
        if (retval < 0) return retval;

        if (kKW_Assign == punctuation)
        {
            // the value of this entry is specified explicitly
            retval = ParseEnum_AssignedValue(item_name, current_constant_value);
            if (retval < 0) return retval;
        }
        else
        {
            if (std::numeric_limits<CodeCell>::max() == current_constant_value)
            {
                Error(
                    "Cannot assign an enum value higher that %d to %s",
                    std::numeric_limits<CodeCell>::max(),
                    _sym.GetName(item_name).c_str());
                return kERR_UserError;
            }
            current_constant_value++;
        }

        // Enter this enum item as a constant int into the _sym table
        retval = ParseEnum_Item2Symtable(enum_name, item_name, current_constant_value);
        if (retval < 0) return retval;

        Symbol const comma_or_brace = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_CloseBrace }, comma_or_brace);
        if (retval < 0) return retval;
        if (kKW_Comma == comma_or_brace)
            continue;
        break;
    }

    Symbol const nextsym = _src.PeekNext();
    if (kKW_Semicolon == nextsym)
    {
        _src.GetNext(); // Eat ';'
        if (tqs[TQ::kReadonly])
        {
            // Only now do we find out that there isn't any following declaration
            // so "readonly" was incorrect. 
            // Back up the cursor for the error message.
            _src.SetCursor(start_of_enum_decl);
            Error("'readonly' can only be used in a variable declaration");
            return kERR_UserError;
        }
        return kERR_None;
    }

    // If this doesn't seem to be a declaration at first glance,
    // warn that the user might have forgotten a ';'.
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input (did you forget a ';'?)");
        return kERR_UserError;
    }
    if (!(_sym.IsIdentifier(nextsym) && !_sym.IsVartype(nextsym)) &&
        kKW_Dynpointer != nextsym &&
        kKW_Noloopcheck != nextsym &&
        kKW_OpenBracket != nextsym)
    {
        Error("Unexpected '%s' (did you forget a ';'?)", _sym.GetName(nextsym).c_str());
        return kERR_UserError;
    }

    // Take enum that has just been defined as the vartype of a declaration
    return ParseVartype(enum_name, tqs, struct_of_current_func, name_of_current_func);
}

AGS::ErrorType AGS::Parser::ParseExport_Function(Symbol func)
{
    // If all functions will be exported anyway, skip this here.
    if (FlagIsSet(_options, SCOPT_EXPORTALL))
        return kERR_None;

    if (_sym[func].FunctionD->TypeQualifiers[TQ::kImport])
    {
        Error(
            ReferenceMsgSym("Function '%s' is imported, so it cannot be exported", func).c_str(),
            _sym.GetName(func).c_str());
        return kERR_UserError;
    }

    return static_cast<ErrorType>(_scrip.AddExport(
        _sym.GetName(func).c_str(),
        _sym[func].FunctionD->Offset,
        _sym.NumOfFuncParams(func) + 100 * _sym[func].FunctionD->IsVariadic));
}

AGS::ErrorType AGS::Parser::ParseExport_Variable(Symbol var)
{
    ScopeType const var_sct =_sym.GetScopeType(var);
    if (ScT::kImport == var_sct)
    {
        Error(
            ReferenceMsgSym("The Variable '%s' is imported, so it cannot be exported", var).c_str(),
            _sym.GetName(var).c_str());
        return kERR_UserError;
    }
    if (ScT::kGlobal != var_sct)
    {
        Error(
            ReferenceMsgSym("The variable '%s' isn't global, so it cannot be exported", var).c_str(),
            _sym.GetName(var).c_str());
        return kERR_UserError;
    }

    // Note, if this is a string then the compiler keeps track of it by its first byte.
    // AFAICS, this _is_ exportable.
    
    return static_cast<ErrorType>(_scrip.AddExport(
        _sym.GetName(var).c_str(),
        _sym[var].VariableD->Offset));
}

AGS::ErrorType AGS::Parser::ParseExport()
{
    if (PP::kPreAnalyze == _pp)
    {
        SkipTo(SymbolList{ kKW_Semicolon }, _src);
        _src.GetNext(); // Eat ';'
        return kERR_None;
    }

    // export specified symbols
    while (true) 
    {
        Symbol const export_sym = _src.GetNext();
        if (_sym.IsFunction(export_sym))
        {
            ErrorType retval = ParseExport_Function(export_sym);
            if (retval < 0) return retval;
        }
        else if (_sym.IsVariable(export_sym))
        {
            ErrorType retval = ParseExport_Variable(export_sym);
            if (retval < 0) return retval;
        }
        else
        {
            Error("Expected a function or global variable but found '%s' instead", _sym.GetName(export_sym).c_str());
            return kERR_UserError;
        }

        Symbol const punctuation = _src.GetNext();
        ErrorType retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon, }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Semicolon == punctuation)
            break;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVartype_CheckForIllegalContext()
{
    NSType const ns_type = _nest.Type();
    if (NSType::kSwitch == ns_type)
    {
        Error("Cannot use declarations directly within a switch body. (Put \"{ ... }\" around the case statements)");
        return kERR_UserError;
    }

    if (NSType::kBraces == ns_type || NSType::kFunction == ns_type || NSType::kNone == ns_type)
        return kERR_None;

    Error("A declaration cannot be the sole body of an 'if', 'else' or loop clause");
    return kERR_UserError;
}

AGS::ErrorType AGS::Parser::ParseVartype_CheckIllegalCombis(bool is_function,TypeQualifierSet tqs)
{
    if (tqs[TQ::kStatic] && !is_function)
    {
        Error("'static' can only be applied to functions that are members of a struct");
        return kERR_UserError;
    }

    // Note: 'protected' is valid for struct functions; those can be defined directly,
    // as in int strct::function(){} or extender, as int function(this strct){}
    // We can't know at this point whether the function is extender, so we can't
    // check  at this point whether 'protected' is allowed.

    if (tqs[TQ::kReadonly] && is_function)
    {
        Error("Readonly cannot be applied to a function");
        return kERR_UserError;
    }

    if (tqs[TQ::kWriteprotected] && is_function)
    {
        Error("'writeprotected' cannot be applied to a function");
        return kERR_UserError;
    }

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVartype_FuncDecl(TypeQualifierSet tqs, Vartype vartype, Symbol struct_name, Symbol func_name, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func, bool &body_follows)
{
    size_t const declaration_start = _src.GetCursor();
    _src.GetNext(); // Eat '('

    if (0 >= struct_name)
    {
        bool const func_is_static_extender = (kKW_Static == _src.PeekNext());
        bool const func_is_extender = func_is_static_extender || (kKW_This == _src.PeekNext());

        if (func_is_extender)
        {
            // Rewrite extender function as a component function of the corresponding struct.
            ErrorType retval = ParseFuncdecl_ExtenderPreparations(func_is_static_extender, struct_name, func_name, tqs);
            if (retval < 0) return retval;
        }

    }

    // Do not set .Extends or the Component flag here. These denote that the
    // func has been either declared within the struct definition or as extender.

    ErrorType retval = ParseFuncdecl(declaration_start, tqs, vartype, struct_name, func_name, false, body_follows);
    if (retval < 0) return retval;
        
    if (!body_follows)
        return kERR_None;

    if (0 < name_of_current_func)
    {
        Error(
            ReferenceMsgSym("Function bodies cannot nest, but the body of function %s is still open. (Did you forget a '}'?)", func_name).c_str(),
            _sym.GetName(name_of_current_func).c_str());
        return kERR_UserError;
    }

    _sym[func_name].FunctionD->NoLoopCheck = no_loop_check;

    // We've started a function, remember what it is.
    name_of_current_func = func_name;
    struct_of_current_func = struct_name;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVartype_VarDecl_PreAnalyze(Symbol var_name, ScopeType scope_type)
{
    if (0 != _givm.count(var_name))
    {
        if (_givm[var_name])
        {
            Error("'%s' is already defined as a global non-import variable", _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
        else if (ScT::kGlobal == scope_type && FlagIsSet(_options, SCOPT_NOIMPORTOVERRIDE))
        {
            Error("'%s' is defined as an import variable; that can't be overridden here", _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
    }
    _givm[var_name] = (ScT::kGlobal == scope_type);

    // Apart from this, we aren't interested in var defns at this stage, so skip this defn
    SkipTo(SymbolList{ kKW_Comma, kKW_Semicolon }, _src);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseVartype_VarDecl(Symbol var_name, ScopeType scope_type, TypeQualifierSet tqs, Vartype vartype)
{
    if (PP::kPreAnalyze == _pp)
        return ParseVartype_VarDecl_PreAnalyze(var_name, scope_type);

    if (tqs[TQ::kStatic])
    {
        Error("'static' cannot be used in a variable declaration");
        return kERR_UserError;
    }
    ErrorType retval = Parse_CheckTQ(tqs, (_nest.TopLevel() > _sym.kParameterScope), _sym.IsComponent(var_name));
    if (retval < 0) return retval;

    // Note: Don't make a variable here yet; we haven't checked yet whether we may do so.

    TypeQualifierSet variable_tqs = tqs;
    // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the variable having the vartype.
    variable_tqs[TQ::kAutoptr] = false;
    variable_tqs[TQ::kManaged] = false;
    variable_tqs[TQ::kBuiltin] = false;
    return ParseVardecl(var_name, vartype, scope_type, variable_tqs);
}

// We accepted a variable type such as "int", so what follows is a function or variable declaration
AGS::ErrorType AGS::Parser::ParseVartype(Vartype vartype, TypeQualifierSet tqs, Symbol &struct_of_current_func,Symbol &name_of_current_func)
{
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input (did you forget ';'?)");
        return kERR_UserError;
    }
    if (tqs[TQ::kBuiltin])
    {
        Error("'builtin' can only be used in a struct declaration");
        return kERR_UserError;
    }

    // Don't define variable or function where illegal in context.
    ErrorType retval = ParseVartype_CheckForIllegalContext();
    if (retval < 0) return retval;

    if (_sym[vartype].VartypeD->Flags[VTF::kUndefined])
        _structRefs[vartype] = _src.GetCursor();

    ScopeType const scope_type = 
        (kKW_NoSymbol != name_of_current_func) ? ScT::kLocal :
        (tqs[TQ::kImport]) ? ScT::kImport : ScT::kGlobal;

    // Only imply a pointer for a managed entity if it isn't imported.
    if ((ScT::kImport == scope_type && kKW_Dynpointer == _src.PeekNext()) ||
        (ScT::kImport != scope_type && _sym.IsManagedVartype(vartype)))
    {
        vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
    }

    retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    // "int [] func(...)"
    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    bool const no_loop_check = (kKW_Noloopcheck == _src.PeekNext());
    if (no_loop_check)
        _src.GetNext();

    // We've accepted a vartype expression and are now reading vars or one func that should have this type.
    while(true)
    {
        // Get the variable or function name.
        Symbol var_or_func_name = kKW_NoSymbol;
        Symbol struct_name = kKW_NoSymbol;
        retval = ParseVarname(struct_name, var_or_func_name);
        if (retval < 0) return retval;

        bool const is_function = (kKW_OpenParenthesis == _src.PeekNext());

        // certain qualifiers, such as "static" only go with certain kinds of definitions.
        retval = ParseVartype_CheckIllegalCombis(is_function, tqs);
        if (retval < 0) return retval;

        if (is_function)
        {
            // Do not set .Extends or the Component flag here. These denote that the
            // func has been either declared within the struct definition or as extender,
            // so they are NOT set unconditionally
            bool body_follows = false;
            retval = ParseVartype_FuncDecl(tqs, vartype, struct_name, var_or_func_name, no_loop_check, struct_of_current_func, name_of_current_func, body_follows);
            if (retval < 0) return retval;
            if (body_follows)
                return kERR_None;
        }
        else if (_sym.IsDynarrayVartype(vartype) || no_loop_check) // e.g., int [] zonk;
        {
            Error("Expected '('");
            return kERR_UserError;
        }
        else
        {
            if (kKW_NoSymbol != struct_name)
            {
                Error("Variable may not contain '::'");
                return kERR_UserError;
            }
            retval = ParseVartype_VarDecl(var_or_func_name, scope_type, tqs, vartype);
            if (retval < 0) return retval;
        }

        Symbol const punctuation = _src.GetNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Semicolon == punctuation)
            return kERR_None;
    }
}

AGS::ErrorType AGS::Parser::HandleEndOfCompoundStmts()
{
    ErrorType retval;
    while (_nest.TopLevel() > _sym.kFunctionScope)
        switch (_nest.Type())
        {
        default:
            Error("!Nesting of unknown type ends");
            return kERR_InternalError;

        case NSType::kBraces:
        case NSType::kSwitch:
            // The body of those statements can only be closed by an explicit '}'.
            // So that means that there cannot be any more non-braced compound statements to close here.
            return kERR_None;

        case NSType::kDo:
            retval = HandleEndOfDo();
            if (retval < 0) return retval;
            break;

        case NSType::kElse:
            retval = HandleEndOfElse();
            if (retval < 0) return retval;
            break;

        case NSType::kIf:
        {
            bool else_follows;
            retval = HandleEndOfIf(else_follows);
            if (retval < 0 || else_follows)
                return retval;
            break;
        }

        case NSType::kWhile:
            retval = HandleEndOfWhile();
            if (retval < 0) return retval;
            break;
        } // switch (nesting_stack->Type())

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseReturn(Symbol name_of_current_func)
{
    Symbol const functionReturnType = _sym.FuncReturnVartype(name_of_current_func);

    if (kKW_Semicolon != _src.PeekNext())
    {
        if (functionReturnType == kKW_Void)
        {
            Error("Cannot return value from void function");
            return kERR_UserError;
        }

        // parse what is being returned
        ErrorType retval = ParseExpression();
        if (retval < 0) return retval;

        // If we need a string object ptr but AX contains a normal string, convert AX
        ConvertAXStringToStringObject(functionReturnType);

        // check return type is correct
        retval = IsVartypeMismatch(_scrip.AX_Vartype, functionReturnType, true);
        if (retval < 0) return retval;

        if (_sym.IsOldstring(_scrip.AX_Vartype) &&
            (ScT::kLocal == _scrip.AX_ScopeType))
        {
            Error("Cannot return local string from function");
            return kERR_UserError;
        }
    }
    else if (_sym.IsAnyIntegerVartype(functionReturnType))
    {
        WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    }
    else if (kKW_Void != functionReturnType)
    {
        Error("Must return a '%s' value from function", _sym.GetName(functionReturnType).c_str());
        return kERR_UserError;
    }

    ErrorType retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // If locals contain pointers, free them
    if (_sym.IsDynVartype(functionReturnType))
        FreeDynpointersOfAllLocals_DynResult(); // Special protection for result needed
    else if (kKW_Void != functionReturnType)
        FreeDynpointersOfAllLocals_KeepAX();
    else 
        FreeDynpointersOfLocals(0u);

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    // Pop the local variables proper from the stack but leave the parameters.
    // This is important because the return address is directly above the parameters;
    // we need the return address to return. (The caller will pop the parameters later.)
    RemoveLocalsFromStack(_sym.kFunctionScope);
  
    // Jump to the exit point of the function
    WriteCmd(SCMD_JMP, 0);
    _nest.JumpOut(_sym.kParameterScope).AddParam();

    // The locals only disappear if control flow actually follows the "return"
    // statement. Otherwise, below the statement, the locals remain on the stack.
    // So restore the OffsetToLocalVarBlock.
    _scrip.OffsetToLocalVarBlock = save_offset;
    return kERR_None;
}

// Evaluate the header of an "if" clause, e.g. "if (i < 0)".
AGS::ErrorType AGS::Parser::ParseIf()
{
    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    _nest.Push(NSType::kIf);

    // The code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    WriteCmd(SCMD_JZ, -77);
    _nest.JumpOut().AddParam();

    return kERR_None;
}

AGS::ErrorType AGS::Parser::HandleEndOfIf(bool &else_follows)
{
    if (kKW_Else != _src.PeekNext())
    {
        else_follows = false;
        _nest.JumpOut().Patch(_src.GetLineno());
        _nest.Pop();
        return kERR_None;
    }

    else_follows = true;
    _src.GetNext(); // Eat "else"
    // Match the 'else' clause that is following to this 'if' stmt:
    // So we're at the end of the "then" branch. Jump out.
    _scrip.WriteCmd(SCMD_JMP, -77);
    // So now, we're at the beginning of the "else" branch.
    // The jump after the "if" condition should go here.
    _nest.JumpOut().Patch(_src.GetLineno());
    // Mark the  out jump after the "then" branch, above, for patching.
    _nest.JumpOut().AddParam();
    // To prevent matching multiple else clauses to one if
    _nest.SetType(NSType::kElse);
    return kERR_None;
}

// Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
AGS::ErrorType AGS::Parser::ParseWhile()
{
    // point to the start of the code that evaluates the condition
    CodeLoc const condition_eval_loc = _scrip.codesize;

    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    _nest.Push(NSType::kWhile);

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    WriteCmd(SCMD_JZ, -77);
    _nest.JumpOut().AddParam();
    _nest.Start().Set(condition_eval_loc);

    return kERR_None;
}

AGS::ErrorType AGS::Parser::HandleEndOfWhile()
{
    // if it's the inner level of a 'for' loop,
    // drop the yanked chunk (loop increment) back in
    if (_nest.ChunksExist())
    {
        int id;
        CodeLoc const write_start = _scrip.codesize;
        _nest.WriteChunk(0u, id);
        _fcm.UpdateCallListOnWriting(write_start, id);
        _fim.UpdateCallListOnWriting(write_start, id);
        _nest.Chunks().clear();
    }

    // jump back to the start location
    _nest.Start().WriteJump(SCMD_JMP, _src.GetLineno());

    // This ends the loop
    _nest.JumpOut().Patch(_src.GetLineno());
    _nest.Pop();

    if (NSType::kFor != _nest.Type())
        return kERR_None;

    // This is the outer level of the FOR loop.
    // It can contain defns, e.g., "for (int i = 0;...)".
    // (as if it were surrounded in braces). Free these definitions
    return HandleEndOfBraceCommand();
}

AGS::ErrorType AGS::Parser::ParseDo()
{
    _nest.Push(NSType::kDo);
    _nest.Start().Set();
    return kERR_None;
}

AGS::ErrorType AGS::Parser::HandleEndOfBraceCommand()
{
    size_t const depth = _nest.TopLevel();
    FreeDynpointersOfLocals(depth);
    RemoveLocalsFromStack(depth);
    RestoreLocalsFromSymtable(depth);
    _nest.Pop();
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseAssignmentOrExpression(Symbol cursym)
{    
    // Get expression
    _src.BackUp(); // Expression starts with cursym: the symbol in front of the cursor.
    size_t const expr_start = _src.GetCursor();
    ErrorType retval = SkipToEndOfExpression();
    if (retval < 0) return retval;
    SrcList expression = SrcList(_src, expr_start, _src.GetCursor() - expr_start);

    if (expression.Length() == 0)
    {
        Error("Unexpected symbol '%s'", _sym.GetName(_src.GetNext()).c_str());
        return kERR_UserError;
    }

    Symbol const assignment_symbol = _src.PeekNext();
    switch (assignment_symbol)
    {
    default:
    {
        // No assignment symbol following: This is an isolated expression, e.g., a function call
        ValueLocation vloc;
        ScopeType scope_type;
        Vartype vartype;
        retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
        if (retval < 0) return retval;
        return ResultToAX(vloc, scope_type, vartype);
    }
    case kKW_Assign:
        return ParseAssignment_Assign(expression);

    case kKW_AssignBitAnd:
    case kKW_AssignBitOr:
    case kKW_AssignBitXor:
    case kKW_AssignDivide:
    case kKW_AssignMinus:
    case kKW_AssignMultiply:
    case kKW_AssignPlus:
    case kKW_AssignShiftLeft:
    case kKW_AssignShiftRight:
        return ParseAssignment_MAssign(assignment_symbol, expression);

    case kKW_Decrement:
    case kKW_Increment:
        return ParseAssignment_SAssign(assignment_symbol, expression);
    }
}

AGS::ErrorType AGS::Parser::ParseFor_InitClauseVardecl()
{
    Vartype vartype = _src.GetNext();
    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    while (true)
    {
        Symbol varname = _src.GetNext();
        Symbol const nextsym = _src.PeekNext();
        if (kKW_ScopeRes == nextsym || kKW_OpenParenthesis == nextsym)
        {
            Error("Function definition not allowed in for loop initialiser");
            return kERR_UserError;
        }
        retval = ParseVardecl(varname, vartype, ScT::kLocal, TypeQualifierSet{});
        if (retval < 0) return retval;

        Symbol const punctuation = _src.PeekNext();
        retval = Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (retval < 0) return retval;
        if (kKW_Comma == punctuation)
            _src.GetNext(); // Eat ','
        if (kKW_Semicolon == punctuation)
            return kERR_None;
    }
}

// The first clause of a 'for' header
AGS::ErrorType AGS::Parser::ParseFor_InitClause(Symbol peeksym)
{
    if (kKW_Semicolon == peeksym)
        return kERR_None; // Empty init clause
    if (_sym.IsVartype(peeksym))
        return ParseFor_InitClauseVardecl();
    return ParseAssignmentOrExpression(_src.GetNext());
}

AGS::ErrorType AGS::Parser::ParseFor_WhileClause()
{
    // Make the last emitted line number invalid so that a linenumber bytecode is emitted
    _scrip.LastEmittedLineno = INT_MAX;
    if (kKW_Semicolon == _src.PeekNext())
    {
        // Not having a while clause is tantamount to the while condition "true".
        // So let's write "true" to the AX register.
        WriteCmd(SCMD_LITTOREG, SREG_AX, 1);
        return kERR_None;
    }

    return ParseExpression();
}

AGS::ErrorType AGS::Parser::ParseFor_IterateClause()
{
    if (kKW_CloseParenthesis == _src.PeekNext())
        return kERR_None; // iterate clause is empty

    return ParseAssignmentOrExpression(_src.GetNext());
}

AGS::ErrorType AGS::Parser::ParseFor()
{
    // "for (I; E; C) {...}" is equivalent to "{ I; while (E) {...; C} }"
    // We implement this with TWO levels of the nesting stack.
    // The outer level contains "I"
    // The inner level contains "while (E) { ...; C}"

    // Outer level
    _nest.Push(NSType::kFor);

    ErrorType retval = Expect(kKW_OpenParenthesis, _src.GetNext());
    if (retval < 0) return retval;

    Symbol const peeksym = _src.PeekNext();
    if (kKW_CloseParenthesis == peeksym)
    {
        Error("Empty parentheses '()' aren't allowed after 'for' (write 'for(;;)' instead");
        return kERR_UserError;
    }

    // Initialization clause (I)
    retval = ParseFor_InitClause(peeksym);
    if (retval < 0) return retval;

    retval = Expect(kKW_Semicolon, _src.GetNext(), "Expected ';' after for loop initializer clause");
    if (retval < 0) return retval;

    // Remember where the code of the while condition starts.
    CodeLoc const while_cond_loc = _scrip.codesize;

    retval = ParseFor_WhileClause();
    if (retval < 0) return retval;

    retval = Expect(kKW_Semicolon, _src.GetNext(), "Expected ';' after for loop while clause");
    if (retval < 0) return retval;

    // Remember where the code of the iterate clause starts.
    CodeLoc const iterate_clause_loc = _scrip.codesize;
    size_t const iterate_clause_fixups_start = _scrip.numfixups;
    size_t const iterate_clause_lineno = _src.GetLineno();

    retval = ParseFor_IterateClause();
    if (retval < 0) return retval;

    retval = Expect(kKW_CloseParenthesis, _src.GetNext(), "Expected ')' after for loop iterate clause");
    if (retval < 0) return retval;

    // Inner nesting level
    _nest.Push(NSType::kWhile);
    _nest.Start().Set(while_cond_loc);

    // We've just generated code for getting to the next loop iteration.
     // But we don't need that code right here; we need it at the bottom of the loop.
     // So rip it out of the bytecode base and save it into our nesting stack.
    int id;
    size_t const yank_size = _scrip.codesize - iterate_clause_loc;
    _nest.YankChunk(iterate_clause_lineno, iterate_clause_loc, iterate_clause_fixups_start, id);
    _fcm.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);
    _fim.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);

    // Code for "If the expression we just evaluated is false, jump over the loop body."
    WriteCmd(SCMD_JZ, -77);
    _nest.JumpOut().AddParam();

    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseSwitch()
{
    // Get the switch expression
    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    // Remember the type of this expression to enforce it later
    Vartype const switch_expr_vartype = _scrip.AX_Vartype;

    // Copy the result to the BX register, ready for case statements
    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_BX);

    retval = Expect(kKW_OpenBrace, _src.GetNext());
    if (retval < 0) return retval;

    _nest.Push(NSType::kSwitch);
    _nest.SetSwitchExprVartype(switch_expr_vartype);
    _nest.SwitchDefault().Set(INT_MAX); // no default case encountered yet

    // Jump to the jump table
    _scrip.WriteCmd(SCMD_JMP, -77);
    _nest.SwitchJumptable().AddParam();

    // Check that "default" or "case" follows
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input");
        return kERR_UserError;
    }

    return Expect(SymbolList{ kKW_Default, kKW_Case, kKW_CloseBrace }, _src.PeekNext());
}

AGS::ErrorType AGS::Parser::ParseSwitchLabel(Symbol case_or_default)
{
    if (NSType::kSwitch != _nest.Type())
    {
        Error("'%s' is only allowed directly within a 'switch' block", _sym.GetName(case_or_default).c_str());
        return kERR_UserError;
    }

    if (kKW_Default == case_or_default)
    {
        if (INT_MAX != _nest.SwitchDefault().Get())
        {
            Error("This switch block already has a 'default' label");
            return kERR_UserError;
        }
        _nest.SwitchDefault().Set();
    }
    else // "case"
    {
        CodeLoc const start_of_code_loc = _scrip.codesize;
        size_t const start_of_fixups = _scrip.numfixups;
        size_t const start_of_code_lineno = _src.GetLineno();

        PushReg(SREG_BX);   // Result of the switch expression

        ErrorType retval = ParseExpression(); // case n: label expression
        if (retval < 0) return retval;

        // Vartypes of the "case" expression and the "switch" expression must match
        retval = IsVartypeMismatch(_scrip.AX_Vartype, _nest.SwitchExprVartype(), false);
        if (retval < 0) return retval;

        PopReg(SREG_BX);

        // rip out the already generated code for the case/switch and store it with the switch
        int id;
        size_t const yank_size = _scrip.codesize - start_of_code_loc;
        _nest.YankChunk(start_of_code_lineno, start_of_code_loc, start_of_fixups, id);
        _fcm.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
        _fim.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);

        BackwardJumpDest case_code_start(_scrip);
        case_code_start.Set();
        _nest.SwitchCases().push_back(case_code_start);
    }

    return Expect(kKW_Colon, _src.GetNext());
}

AGS::ErrorType AGS::Parser::RemoveLocalsFromStack(size_t nesting_level)
{
    size_t const size_of_local_vars = StacksizeOfLocals(nesting_level);
    if (size_of_local_vars > 0)
    {
        _scrip.OffsetToLocalVarBlock -= size_of_local_vars;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_local_vars);
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ReadLiteralOrConst(SrcList &src, Symbol &lit)
{
    Symbol const litconst = lit = src.GetNext();
    while (_sym.IsConstant(lit))
        lit = _sym[lit].ConstantD->ValueSym;

    if (!_sym.IsLiteral(lit))
    {
        Error("Expected a constant or a literal, found '%s' instead", _sym.GetName(litconst).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ReadIntLiteralOrConst(Symbol &lit, std::string const &msg)
{
    bool const negate = (kKW_Minus == _src.PeekNext());
    if (negate)
        _src.GetNext(); // Eat '-'

    ErrorType retval = ReadLiteralOrConst(lit);
    if (retval < 0) return retval;
    Vartype const vartype = _sym[lit].LiteralD->Vartype;
    if (_sym.IsAnyArrayVartype(vartype))
    {
        Error((msg + "Expected an integer, found an array instead").c_str());
        return kERR_UserError;
    }
    if (!_sym.IsAnyIntegerVartype(vartype))
    {
        Error((msg + "Expected an integer, found type '%d' instead").c_str(), _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    if (negate)
        return NegateLiteral(lit);
    return kERR_None;
}

AGS::ErrorType AGS::Parser::NegateLiteral(Symbol &symb)
{
    if (!_sym.IsLiteral(symb))
    {
        Error("!Expected literal");
        return kERR_InternalError;
    }
    Vartype vartype = _sym[symb].LiteralD->Vartype;

    CodeCell new_value;
    std::string new_value_string;
    if (_sym.IsAnyIntegerVartype(vartype))
    {
        new_value = -_sym[symb].LiteralD->Value;
        new_value_string = std::to_string(new_value);
    }
    else if (kKW_Float == vartype)
    {
        CodeCell const old_value = _sym[symb].LiteralD->Value;
        float const old_fvalue = *reinterpret_cast<float const *>(&old_value);
        float const new_fvalue = -old_fvalue;
        new_value = *reinterpret_cast<CodeCell const *>(&new_fvalue);
        new_value_string = std::to_string(new_fvalue);
    }
    else
    {
        Error("!Can't negate vartype '%s'", _sym.GetName(vartype).c_str());
        return kERR_InternalError;
    }

    symb = _sym.FindOrAdd(new_value_string);
    if (_sym.IsLiteral(symb))
        return kERR_None;
    _sym.MakeEntryLiteral(symb);
    _sym[symb].LiteralD->Vartype = vartype;
    _sym[symb].LiteralD->Value = new_value;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::EmitLiteral(Symbol lit, ValueLocation &vl, Vartype &vartype)
{
    if (!_sym.IsLiteral(lit))
    {
        Error("!'%s' isn't literal", _sym.GetName(lit).c_str());
        return kERR_InternalError;
    }
    vartype = _sym[lit].LiteralD->Vartype;

    WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[lit].LiteralD->Value);
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype))
        _scrip.FixupPrevious(kFx_String);
    _scrip.AX_Vartype = vartype;
    _scrip.AX_ScopeType = ScT::kGlobal;
    vl = kVL_AX_is_value;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::FindOrAddIntLiteral(CodeCell value, Symbol &symb)
{
    std::string const valstr = std::to_string(value);
    symb = _sym.Find(valstr);
    if (kKW_NoSymbol != symb)
    {
        if (_sym.IsLiteral(symb))
            return kERR_None;
        Error("!'%s' should be an integer literal but isn't.", valstr.c_str());
        return kERR_InternalError;
    }

    symb = _sym.Add(valstr);
    _sym.MakeEntryLiteral(symb);
    _sym[symb].LiteralD->Vartype = kKW_Int;
    _sym[symb].LiteralD->Value = value;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseBreak()
{
    ErrorType retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // Find the (level of the) looping construct to which the break applies
    // Note that this is similar, but _different_ from "continue".
    size_t nesting_level;
    for (nesting_level = _nest.TopLevel(); nesting_level > 0; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kSwitch == ltype || NSType::kWhile == ltype)
            break;
    }

    if (0u == nesting_level)
    {
        Error("'break' is only valid inside a loop or a switch statement block");
        return kERR_UserError;
    }

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    FreeDynpointersOfLocals(nesting_level + 1);
    RemoveLocalsFromStack(nesting_level + 1);
    
    // Jump out of the loop or switch
    WriteCmd(SCMD_JMP, -77);
    _nest.JumpOut(nesting_level).AddParam();

    // The locals only disappear if control flow actually follows the "break"
    // statement. Otherwise, below the statement, the locals remain on the stack.
    // So restore the OffsetToLocalVarBlock.
    _scrip.OffsetToLocalVarBlock = save_offset;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseContinue()
{
    ErrorType retval = Expect(kKW_Semicolon, _src.GetNext());
    if (retval < 0) return retval;

    // Find the level of the looping construct to which the break applies
    // Note that this is similar, but _different_ from "break".
    size_t nesting_level;
    for (nesting_level = _nest.TopLevel(); nesting_level > 0; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kWhile == ltype)
            break;
    }

    if (nesting_level == 0)
    {
        Error("'continue' is only valid inside a loop");
        return kERR_UserError;
    }

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    FreeDynpointersOfLocals(nesting_level + 1);
    RemoveLocalsFromStack(nesting_level + 1);

    // if it's a for loop, drop the yanked loop increment chunk in
    if (_nest.ChunksExist(nesting_level))
    {
        int id;
        CodeLoc const write_start = _scrip.codesize;
        _nest.WriteChunk(nesting_level, 0u, id);
        _fcm.UpdateCallListOnWriting(write_start, id);
        _fim.UpdateCallListOnWriting(write_start, id);
    }

    // Jump to the start of the loop
    _nest.Start(nesting_level).WriteJump(SCMD_JMP, _src.GetLineno());

    // The locals only disappear if control flow actually follows the "continue"
    // statement. Otherwise, below the statement, the locals remain on the stack.
     // So restore the OffsetToLocalVarBlock.
    _scrip.OffsetToLocalVarBlock = save_offset;
    return kERR_None;
}

AGS::ErrorType AGS::Parser::ParseCloseBrace()
{
    if (NSType::kSwitch == _nest.Type())
        return HandleEndOfSwitch();
    return HandleEndOfBraceCommand();
}

AGS::ErrorType AGS::Parser::ParseCommand(Symbol leading_sym, Symbol &struct_of_current_func,Symbol &name_of_current_func)
{
    ErrorType retval;

    // NOTE that some branches of this switch will leave
    // the whole function, others will continue after the switch.
    switch (leading_sym)
    {
    default:
    {
        // No keyword, so it should be an assignment or an isolated expression
        retval = ParseAssignmentOrExpression(leading_sym);
        if (retval < 0) return retval;
        retval = Expect(kKW_Semicolon, _src.GetNext());
        if (retval < 0) return retval;
        break;
    }

    case kKW_Break:
        retval = ParseBreak();
        if (retval < 0) return retval;
        break;

    case kKW_Case:
        retval = ParseSwitchLabel(leading_sym);
        if (retval < 0) return retval;
        break;

    case kKW_CloseBrace:
        // Note that the scanner has already made sure that every close brace has an open brace
        if (_sym.kFunctionScope >= _nest.TopLevel())
            return HandleEndOfFuncBody(struct_of_current_func, name_of_current_func);

        retval = ParseCloseBrace();
        if (retval < 0) return retval;
        break;

    case kKW_Continue:
        retval = ParseContinue();
        if (retval < 0) return retval;
        break;

    case kKW_Default:
        retval = ParseSwitchLabel(leading_sym);
        if (retval < 0) return retval;
        break;

    case kKW_Do:
        return ParseDo();

    case kKW_Else:
        Error("Cannot find any 'if' clause that matches this 'else'");
        return kERR_UserError;

    case kKW_For:
        return ParseFor();

    case kKW_If:
        return ParseIf();

    case kKW_OpenBrace:
        if (PP::kPreAnalyze == _pp)
        {
            name_of_current_func = struct_of_current_func = kKW_NoSymbol;
            return SkipToClose(kKW_CloseBrace);
        }
        if (_sym.kParameterScope == _nest.TopLevel())
             return ParseFuncBodyStart(struct_of_current_func, name_of_current_func);
        _nest.Push(NSType::kBraces);
        return kERR_None;

    case kKW_Return:
        retval = ParseReturn(name_of_current_func);
        if (retval < 0) return retval;
        break;

    case kKW_Switch:
        retval = ParseSwitch();
        if (retval < 0) return retval;
        break;

    case kKW_While:
        // This cannot be the end of a do...while() statement
        // because that would have been handled in HandleEndOfDo()
        return ParseWhile();
    }

    // This statement may be the end of some unbraced
    // compound statements, e.g. "while (...) if (...) i++";
    // Pop the nesting levels of such statements and handle
    // the associated jumps.
    return HandleEndOfCompoundStmts();
}

void AGS::Parser::HandleSrcSectionChangeAt(size_t pos)
{
    size_t const src_section_id = _src.GetSectionIdAt(pos);
    if (src_section_id == _lastEmittedSectionId)
        return;

    if (PP::kMain == _pp)
        _scrip.StartNewSection(_src.SectionId2Section(src_section_id));
    _lastEmittedSectionId = src_section_id;
}

AGS::ErrorType AGS::Parser::ParseInput()
{
    Parser::NestingStack nesting_stack(_scrip);
    size_t nesting_level = 0;

    // We start off in the global data part - no code is allowed until a function definition is started
    Symbol struct_of_current_func = kKW_NoSymbol; // non-zero only when a struct member function is open
    Symbol name_of_current_func = kKW_NoSymbol;

    // Collects vartype qualifiers such as 'readonly'
    TypeQualifierSet tqs = {};

    while (!_src.ReachedEOF())
    {
        size_t const next_pos = _src.GetCursor();
        HandleSrcSectionChangeAt(next_pos);
        currentline = _src.GetLinenoAt(next_pos);

        ErrorType retval = ParseQualifiers(tqs);
        if (retval < 0) return retval;

        Symbol const leading_sym = _src.GetNext();

        // Vartype clauses

        if (kKW_Enum == leading_sym)
        {
            retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false);
            if (retval < 0) return retval;
            retval = ParseEnum(tqs, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;
            continue;
        }

        if (kKW_Export == leading_sym)
        {
            retval = Parse_CheckTQSIsEmpty(tqs);
            if (retval < 0) return retval;
            retval = ParseExport();
            if (retval < 0) return retval;
            continue;
        }

        if (kKW_Struct == leading_sym)
        {
            retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false);
            if (retval < 0) return retval;
            retval = ParseStruct(tqs, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;
            continue;
        }

        if (_sym.IsVartype(leading_sym) && kKW_Dot != _src.PeekNext())
        {
            // Note: We can't check yet whether the TQS are legal because we don't know whether the
            // var / func names that will be defined will be composite.
            retval = ParseVartype(leading_sym, tqs, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;
            continue;
        }

        // Command clauses

        if (kKW_NoSymbol == name_of_current_func)
        {
            Error("'%s' is illegal outside a function", _sym.GetName(leading_sym).c_str());
            return kERR_UserError;
        }

        retval = Parse_CheckTQSIsEmpty(tqs);
        if (retval < 0) return retval;
        retval = ParseCommand(leading_sym, struct_of_current_func, name_of_current_func);
        if (retval < 0) return retval;
    } // while (!targ.reached_eof())

    return kERR_None;
}

AGS::ErrorType AGS::Parser::Parse_ReinitSymTable(size_t size_after_scanning)
{
    for (size_t sym_idx = _sym.GetLastAllocated() + 1; sym_idx < _sym.entries.size(); sym_idx++)
    {
        SymbolTableEntry &s_entry = _sym[sym_idx];

        if (_sym.IsFunction(sym_idx))
        {
            s_entry.FunctionD->TypeQualifiers[TQ::kImport] = (kFT_Import == s_entry.FunctionD->Offset);
            s_entry.FunctionD->Offset = 0;
            continue;
        }

        if (_sym.IsLiteral(sym_idx))
            continue; 

        s_entry.Clear(); // note, won't (and shouldn't) clear the Name field
    }

    // This has invalidated the symbol table caches, so kill them
    _sym.ResetCaches();

    return kERR_None;
}

AGS::ErrorType AGS::Parser::Parse_BlankOutUnusedImports()
{
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        if (_sym[entries_idx].Accessed)
            continue;

        // Don't "compact" the entries in the _scrip.imports[] array. They are referenced by index, and if you
        // change the indexes of the entries then you get dangling "references". So the only thing allowed is
        // setting unused import entries to "".
        if (_sym.IsFunction(entries_idx))
        {
            if(_sym[entries_idx].FunctionD->TypeQualifiers[TQ::kImport])
                _scrip.imports[_sym[entries_idx].FunctionD->Offset][0] = '\0';
            continue;
        }
        if (_sym.IsVariable(entries_idx))
        {
            // Don't mind attributes - they are shorthand for the respective getter
            // and setter funcs. If _those_ are unused, then they will be caught
            // in the same that way normal functions are.
            if (!_sym[entries_idx].VariableD->TypeQualifiers[TQ::kAttribute] &&
                _sym[entries_idx].VariableD->TypeQualifiers[TQ::kImport])
                _scrip.imports[_sym[entries_idx].VariableD->Offset][0] = '\0';
            continue;
        }
    }

    return kERR_None;
}

void AGS::Parser::MessageWithPosition(MessageHandler::Severity sev, int section_id, size_t lineno, char const *descr, ...)
{
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, descr, vlist1) + 1];
    vsprintf(message, descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msg_handler.AddMessage(
        sev,
        _src.SectionId2Section(section_id),
        lineno,
        message);

    delete[] message;
}

void AGS::Parser::Error(char const *descr, ...)
{
    // ErrorWithPosition() can't be called with a va_list and doesn't have a variadic variant,
    // so convert all the parameters into a single C string here
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, descr, vlist1) + 1];
    vsprintf(message, descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msg_handler.AddMessage(
        MessageHandler::kSV_Error,
        _src.SectionId2Section(_src.GetSectionId()),
        _src.GetLineno(),
        message);
    delete[] message;
}

void AGS::Parser::Warning(char const *descr, ...)
{
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, descr, vlist1) + 1];
    vsprintf(message, descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msg_handler.AddMessage(
        MessageHandler::kSV_Warning,
        _src.SectionId2Section(_src.GetSectionId()),
        _src.GetLineno(),
        message);
    delete[] message;
}

AGS::ErrorType AGS::Parser::Parse_PreAnalyzePhase()
{
    size_t const sym_size_after_scanning  = _sym.entries.size();

    _pp = PP::kPreAnalyze;
    ErrorType retval = ParseInput();
    if (retval < 0) return retval;

    _fcm.Reset();

    // Keep (just) the headers of functions that have a body to the main symbol table
    // Reset everything else in the symbol table,
    // but keep the entries so that they are guaranteed to have
    // the same index when parsed in phase 2
    return Parse_ReinitSymTable(sym_size_after_scanning);
}

AGS::ErrorType AGS::Parser::Parse_MainPhase()
{
    _pp = PP::kMain;
    return ParseInput();
}

AGS::ErrorType AGS::Parser::Parse_CheckForUnresolvedStructForwardDecls()
{
    for (auto it = _structRefs.cbegin(); it != _structRefs.cend(); ++it)
    {
        auto &stname = it->first;
        auto &src_location = it->second;
        if (_sym[stname].VartypeD->Flags[VTF::kUndefined])
        {
            _src.SetCursor(src_location);
            Error(
                ReferenceMsgSym("Struct '%s' is used but never completely defined", stname).c_str(),
                _sym.GetName(stname).c_str());
            return kERR_UserError;
        }
    }
    return kERR_None;
}

ErrorType AGS::Parser::Parse_CheckFixupSanity()
{
    for (size_t fixup_idx = 0; fixup_idx < static_cast<size_t>(_scrip.numfixups); fixup_idx++)
    {
        if (FIXUP_IMPORT != _scrip.fixuptypes[fixup_idx])
            continue;
        int const code_idx = _scrip.fixups[fixup_idx];
        if (code_idx < 0 || code_idx >= _scrip.codesize)
        {
            Error(
                "!Fixup #%d references non-existent code offset #%d",
                fixup_idx,
                code_idx);
            return kERR_InternalError;
        }
        int const cv = _scrip.code[code_idx];
        if (cv < 0 || cv >= _scrip.numimports ||
            '\0' == _scrip.imports[cv][0])
        {
            Error(
                "!Fixup #%d references non-existent import #%d",
                fixup_idx,
                cv);
            return kERR_InternalError;
        }
    }

    return kERR_None;
}

ErrorType AGS::Parser::Parse_ExportAllFunctions()
{
    for (size_t func_num = 0; func_num < _scrip.Functions.size(); func_num++)
    {
        if (0 > _scrip.AddExport(
            _scrip.Functions[func_num].Name,
            _scrip.Functions[func_num].CodeOffs,
            _scrip.Functions[func_num].NumOfParams))
        {
            Error("!Could not export function. Out of memory?");
            return kERR_InternalError;
        }
    }
    return kERR_None;
}

AGS::ErrorType AGS::Parser::Parse()
{
    try
    {
        CodeLoc const start_of_input = _src.GetCursor();

        ErrorType retval = Parse_PreAnalyzePhase();
        if (retval < 0) return retval;

        _src.SetCursor(start_of_input);
        retval = Parse_MainPhase();
        if (retval < 0) return retval;

        retval = _fcm.CheckForUnresolvedFuncs();
        if (retval < 0) return retval;
        retval = _fim.CheckForUnresolvedFuncs();
        if (retval < 0) return retval;
        retval = Parse_CheckForUnresolvedStructForwardDecls();
        if (retval < 0) return retval;
        if (FlagIsSet(_options, SCOPT_EXPORTALL))
        {
            retval = Parse_ExportAllFunctions();
            if (retval < 0) return retval;
        }
        retval = Parse_BlankOutUnusedImports();
        if (retval < 0) return retval;
        return Parse_CheckFixupSanity();
    }
    catch (std::exception const &e)
    {
        std::string const msg = std::string{ "!Exception encountered: currentline %d, " } + e.what();
        Error(msg.c_str(), currentline);
        return kERR_InternalError;
    }
}

// Scan inpl into scan tokens, build a symbol table
int cc_scan(std::string const &inpl, AGS::SrcList &src, AGS::ccCompiledScript &scrip, AGS::SymbolTable &symt, AGS::MessageHandler &mh)
{
    AGS::Scanner scanner = { inpl, src, scrip, symt, mh };
    return scanner.Scan();
}

int cc_parse(AGS::SrcList &src, AGS::FlagSet options, AGS::ccCompiledScript &scrip, AGS::SymbolTable &symt, AGS::MessageHandler &mh)
{
    AGS::Parser parser = { src, options, scrip, symt, mh };
    return parser.Parse();
}

int cc_compile(std::string const &inpl, AGS::FlagSet options, AGS::ccCompiledScript &scrip, AGS::MessageHandler &mh)
{
    std::vector<AGS::Symbol> symbols;
    AGS::LineHandler lh;
    size_t cursor = 0u;
    AGS::SrcList src = AGS::SrcList(symbols, lh, cursor);
    src.NewSection("UnnamedSection");
    src.NewLine(1u);

    AGS::SymbolTable symt;

    ccCurScriptName = nullptr;

    int error_code = cc_scan(inpl, src, scrip, symt, mh);
    if (error_code >= 0)
        error_code = cc_parse(src, options, scrip, symt, mh);
    return error_code;
}

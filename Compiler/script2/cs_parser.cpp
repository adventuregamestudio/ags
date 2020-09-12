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
 	compile time by subtracting a specific offset from the stack pointer, namely offset_to_local_var_block.
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
		parameter1					<- SP - offset_to_local_var_block
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

std::map<TypeQualifier, std::string> AGS::Parser::_tq2String;

bool AGS::Parser::IsIdentifier(AGS::Symbol symb)
{
    if (symb <= _sym.GetLastPredefSym() || symb > static_cast<int>(_sym.entries.size()))
        return false;
    std::string name = _sym.GetName(symb);
    if (name.size() == 0)
        return false;
    if ('0' <= name[0] && name[0] <= '9')
        return false;
    for (size_t idx = 0; idx < name.size(); ++idx)
    {
        char const &ch = name[idx];
        // don't use "is.." functions, these are locale dependent
        if ('0' <= ch && ch <= '9') continue;
        if ('A' <= ch && ch <= 'Z') continue;
        if ('a' <= ch && ch <= 'z') continue;
        if ('_' == ch) continue;
        return false;
    }
    return true;
}

std::string const AGS::Parser::TypeQualifierSet2String(TypeQualifierSet tqs) const
{
    std::string ret;

    for (auto tq_it = _tq2String.begin(); _tq2String.end() != tq_it; tq_it++)
        if (FlagIsSet(tqs, tq_it->first))
            ret += tq_it->second + " ";
    if (ret.length() > 0)
        ret.pop_back();
    return ret;
}

ErrorType AGS::Parser::String2Int(std::string const &str, int &val)
{
    const bool is_neg = (0 == str.length() || '-' == str.at(0));
    errno = 0;
    char *endptr = 0;
    const long longValue = strtol(str.c_str(), &endptr, 10);
    if ((longValue == LONG_MIN && errno == ERANGE) ||
        (is_neg && (endptr[0] != '\0')) ||
        (longValue < INT_MIN))
    {
        Error("Literal value '%s' is too low (min. is '%d')", str.c_str(), INT_MIN);
        return kERR_UserError;
    }

    if ((longValue == LONG_MAX && errno == ERANGE) ||
        ((!is_neg) && (endptr[0] != '\0')) ||
        (longValue > INT_MAX))
    {
        Error("Literal value %s is too high (max. is %d)", str.c_str(), INT_MAX);
        return kERR_UserError;
    }

    val = static_cast<int>(longValue);
    return kERR_None;
}

ErrorType AGS::Parser::String2Float(std::string const &float_as_string, float &f)
{
    char *endptr;
    char const *instring = float_as_string.c_str();
    double const d = strtod(instring, &endptr);
    if (endptr != instring + float_as_string.length())
    {   // The scanner ought to prevent that
        Error("!Illegal floating point literal '%s'", instring);
        return kERR_InternalError;
    }
    if (HUGE_VAL == d)
    {
        Error("Floating point literal '%s' is out of range", instring);
        return kERR_UserError;
    }
    f = static_cast<float>(d);
    return kERR_None;
}



AGS::Symbol AGS::Parser::MangleStructAndComponent(AGS::Symbol stname, AGS::Symbol component)
{
    std::string fullname_str = _sym.GetName(stname) + "::" + _sym.GetName(component);
    return _sym.FindOrAdd(fullname_str.c_str());
}

// Skim through source, ignoring delimited content completely.
// Stop in the following cases:
//   A symbol is encountered whose type is in stoplist[]
//   A closing symbol is encountered that hasn't been opened.
// Don't consume the symbol that stops the scan.
ErrorType AGS::Parser::SkipTo(SrcList &source, const AGS::SymbolType stoplist[], size_t stoplist_len)
{
    int delimeter_nesting_depth = 0;
    for (; !source.ReachedEOF(); source.GetNext())
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        SymbolType const curtype = _sym.GetSymbolType(_src.PeekNext());
        if (kSYM_OpenBrace == curtype ||
            kSYM_OpenBracket == curtype ||
            kSYM_OpenParenthesis == curtype)
        {
            ++delimeter_nesting_depth;
            continue;
        }
        if (kSYM_CloseBrace == curtype ||
            kSYM_CloseBracket == curtype ||
            kSYM_CloseParenthesis == curtype)
        {
            if (--delimeter_nesting_depth < 0)
                return kERR_None;
            continue;
        }
        if (0 < delimeter_nesting_depth)
            continue;

        for (size_t stoplist_idx = 0; stoplist_idx < stoplist_len; stoplist_idx++)
            if (curtype == stoplist[stoplist_idx])
                return kERR_None;
    }
    return kERR_UserError;
}

// For assigning unique IDs to chunks
int AGS::Parser::NestingStack::_chunkIdCtr = 0;

ErrorType AGS::Parser::NestingStack::Push(SymbolType type)
{
    NestingInfo const ni =
    {
        type,
        BackwardJumpDest{ _scrip },
        ForwardJump { _scrip },
        0,
        BackwardJumpDest{ _scrip },
        ForwardJump{ _scrip },
        std::vector<Chunk>{},
    };
    _stack.push_back(ni);
    return kERR_None;
}

AGS::Parser::NestingStack::NestingStack(::ccCompiledScript &scrip)
    :_scrip(scrip)
{
    // Push first record on stack so that it isn't empty
    Push(kSYM_NoType);
}

// Rip the code that has already been generated, starting from codeoffset, out of scrip
// and move it into the vector at list, instead.
void AGS::Parser::NestingStack::YankChunk(size_t src_line, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset, int &id)
{
    Chunk item;
    item.SrcLine = src_line;

    size_t const codesize = std::max(0, _scrip.codesize);
    for (size_t code_idx = codeoffset; code_idx < codesize; code_idx++)
        item.Code.push_back(_scrip.code[code_idx]);

    size_t numfixups = std::max(0, _scrip.numfixups);
    for (size_t fixups_idx = fixupoffset; fixups_idx < numfixups; fixups_idx++)
    {
        item.Fixups.push_back(_scrip.fixups[fixups_idx]);
        item.FixupTypes.push_back(_scrip.fixuptypes[fixups_idx]);
    }
    item.CodeOffset = codeoffset;
    item.FixupOffset = fixupoffset;
    item.Id = id = ++_chunkIdCtr;

    _stack.back().Chunks.push_back(item);

    // Cut out the code that has been pushed
    _scrip.codesize = codeoffset;
    _scrip.numfixups = fixupoffset;

}

// Copy the code in the chunk to the end of the bytecode vector 
void AGS::Parser::NestingStack::WriteChunk(size_t level, size_t index, int &id)
{
    Chunk const item = Chunks(level).at(index);
    id = item.Id;

    CodeLoc const adjust = _scrip.codesize - item.CodeOffset;

    size_t limit = item.Code.size();
    if (0 < limit && SCMD_LINENUM != item.Code[0])
        _scrip.write_lineno(item.SrcLine);
    for (size_t index = 0; index < limit; index++)
        _scrip.write_code(item.Code[index]);

    limit = item.Fixups.size();
    for (size_t index = 0; index < limit; index++)
        _scrip.add_fixup(item.Fixups[index] + adjust, item.FixupTypes[index]);

    // Make the last emitted source line number invalid so that the next command will
    // generate a line number opcode first
    _scrip.last_emitted_lineno = INT_MAX;
}

AGS::Parser::FuncCallpointMgr::FuncCallpointMgr(::SymbolTable &symt, ::ccCompiledScript &scrip)
    : _sym(symt)
    , _scrip(scrip)
{ }

void AGS::Parser::FuncCallpointMgr::Reset()
{
    _funcCallpointMap.clear();
}

ErrorType AGS::Parser::FuncCallpointMgr::TrackForwardDeclFuncCall(Symbol func, CodeLoc loc)
{
    // Patch callpoint in when known
    CodeCell const callpoint = _funcCallpointMap[func].Callpoint;
    if (callpoint >= 0)
    {
        _scrip.code[loc] = callpoint;
        return kERR_None;
    }

    // Callpoint not known, so remember this location
    PatchInfo pinfo;
    pinfo.ChunkId = CodeBaseId;
    pinfo.Offset = loc;
    _funcCallpointMap[func].List.push_back(pinfo);

    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::UpdateCallListOnYanking(AGS::CodeLoc chunk_start, size_t chunk_len, int id)
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
            if (patch_info.ChunkId != CodeBaseId)
                continue;
            if (patch_info.Offset < chunk_start || patch_info.Offset >= static_cast<int>(chunk_end))
                continue; // This address isn't yanked

            patch_info.ChunkId = id;
            patch_info.Offset -= chunk_start;
        }
    }

    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::UpdateCallListOnWriting(AGS::CodeLoc start, int id)
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
            cb_patch_info.ChunkId = CodeBaseId;
            cb_patch_info.Offset = patch_info.Offset + start;
            pl.push_back(cb_patch_info);
        }
    }

    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::SetFuncCallpoint(AGS::Symbol func, AGS::CodeLoc dest)
{
    _funcCallpointMap[func].Callpoint = dest;
    PatchList &pl = _funcCallpointMap[func].List;
    size_t const pl_size = pl.size();
    bool yanked_patches_exist = false;
    for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        if (pl[pl_idx].ChunkId == CodeBaseId)
        {
            _scrip.code[pl[pl_idx].Offset] = dest;
            pl[pl_idx].ChunkId = PatchedId;
        }
        else if (pl[pl_idx].ChunkId != PatchedId)
        {
            yanked_patches_exist = true;
        }
    if (!yanked_patches_exist)
        pl.clear();
    return kERR_None;
}

ErrorType AGS::Parser::FuncCallpointMgr::CheckForUnresolvedFuncs()
{
    for (CallMap::iterator fcm_it = _funcCallpointMap.begin(); fcm_it != _funcCallpointMap.end(); ++fcm_it)
    {
        PatchList &pl = fcm_it->second.List;
        size_t const pl_size = pl.size();
        for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        {
            if (pl[pl_idx].ChunkId != CodeBaseId)
                continue;
            // Is possible if there is a forward declaration but not an import
            cc_error("Function '%s()' has been called but not defined with body nor imported", _sym.GetName(fcm_it->first).c_str());
            return kERR_InternalError;
        }
    }
    return kERR_None;
}

AGS::Parser::FuncCallpointMgr::CallpointInfo::CallpointInfo()
    : Callpoint(-1)
{ }

AGS::Parser::RestorePoint::RestorePoint(::ccCompiledScript &scrip)
    : _scrip(scrip)
{
    _restoreLoc = _scrip.codesize;
    _lastEmittedSrcLineno = _scrip.last_emitted_lineno;
}

void AGS::Parser::RestorePoint::Restore()
{
    _scrip.codesize = _restoreLoc;
    _scrip.last_emitted_lineno = _lastEmittedSrcLineno;
}

AGS::Parser::BackwardJumpDest::BackwardJumpDest(::ccCompiledScript &scrip)
    : _scrip(scrip)
    , _dest(-1)
    , _lastEmittedSrcLineno(INT_MAX)
{ }

void AGS::Parser::BackwardJumpDest::Set(CodeLoc cl)
{
    _dest = (cl >= 0) ? cl : _scrip.codesize;
    _lastEmittedSrcLineno = _scrip.last_emitted_lineno;
}

void AGS::Parser::BackwardJumpDest::WriteJump(CodeCell jump_op, size_t cur_line)
{
    if (SCMD_LINENUM != _scrip.code[_dest] &&
        _scrip.last_emitted_lineno != _lastEmittedSrcLineno)
    {
        _scrip.write_lineno(cur_line);
    }
    _scrip.write_cmd(jump_op, _scrip.RelativeJumpDist(_scrip.codesize + 1, _dest));
}

AGS::Parser::ForwardJump::ForwardJump(::ccCompiledScript &scrip)
    : _scrip(scrip)
    , _lastEmittedSrcLineno(INT_MAX)
{ }

void AGS::Parser::ForwardJump::AddParam(int offset)
{
    // If the current value for the last emitted lineno doesn't match the
    // saved value then the saved value won't work for all jumps so it
    // must be set to invalid.
    if (_jumpDestParamLocs.empty())
        _lastEmittedSrcLineno = _scrip.last_emitted_lineno;
    else if (_lastEmittedSrcLineno != _scrip.last_emitted_lineno)
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
        if (cur_line != _scrip.last_emitted_lineno || cur_line != _lastEmittedSrcLineno)
            _scrip.last_emitted_lineno = INT_MAX;
    }
    for (auto loc = _jumpDestParamLocs.cbegin(); loc != _jumpDestParamLocs.cend(); loc++)
        _scrip.code[*loc] = _scrip.RelativeJumpDist(*loc, _scrip.codesize);
    _jumpDestParamLocs.clear();
}

AGS::Parser::ImportMgr::ImportMgr()
    : _scrip(nullptr)
{ }

void AGS::Parser::ImportMgr::Init(ccCompiledScript *scrip)
{
    _importIdx.clear();
    _scrip = scrip;
    for (int import_idx = 0; import_idx < scrip->numimports; import_idx++)
        _importIdx[scrip->imports[import_idx]] = import_idx;
}

bool AGS::Parser::ImportMgr::IsDeclaredImport(std::string s)
{
    return (_importIdx.end() != _importIdx.find(s));
}

int AGS::Parser::ImportMgr::FindOrAdd(std::string s)
{
    auto it = _importIdx.find(s);
    if (_importIdx.end() != it)
        return it->second;
    // Cache miss
    int idx = _scrip->add_new_import(s.c_str());
    _importIdx[s] = idx;
    return idx;
}

void AGS::Parser::MemoryLocation::SetStart(ScopeType type, size_t offset)
{
    ScType = type;
    _startOffs = offset;
    _componentOffs = 0;
}

void AGS::Parser::MemoryLocation::MakeMARCurrent(size_t lineno, ccCompiledScript &scrip)
{
    switch (_startOffsProcessed? kScT_None : ScType)
    {
    default: // The scope type and base address are up-to-date, but an offset might have accumulated 
        if (_componentOffs > 0)
        {
            scrip.refresh_lineno(lineno);
            scrip.write_cmd(SCMD_ADD, SREG_MAR, _componentOffs);
            _codeEmitted = true;
        }
        break;

    case kScT_Global:
        scrip.refresh_lineno(lineno);
        scrip.write_cmd(SCMD_LITTOREG, SREG_MAR, _startOffs + _componentOffs);
        scrip.fixup_previous(Parser::kFx_GlobalData);
        _codeEmitted = true;
        break;

    case kScT_Import:
        // Have to convert the import number into a code offset first.
        // Can only then add the offset to it.
        scrip.refresh_lineno(lineno);
        scrip.write_cmd(SCMD_LITTOREG, SREG_MAR, _startOffs);
        scrip.fixup_previous(Parser::kFx_Import);
        if (_componentOffs != 0)
            scrip.write_cmd(SCMD_ADD, SREG_MAR, _componentOffs);
        _codeEmitted = true;
        break;

    case kScT_Local:
        scrip.refresh_lineno(lineno);
        scrip.write_cmd(
            SCMD_LOADSPOFFS,
            scrip.offset_to_local_var_block - _startOffs - _componentOffs);
        _codeEmitted = true;
        break;
    }
    Reset();
    return;
}

void AGS::Parser::MemoryLocation::Reset()
{
    _startOffs = 0u;
    _componentOffs = 0u;
    _startOffsProcessed = true;
}

AGS::Parser::Parser(::SymbolTable &symt, SrcList &src, ::ccCompiledScript &scrip)
    : _fcm(symt, scrip)
    , _fim(symt, scrip)
    , _lastEmittedSectionId(0)
    , _lastEmittedLineno(0)
    , _scrip(scrip)
    , _warnings({})
    , _pp(kPP_PreAnalyze)
    , _sym(symt)
    , _src(src)
{
    _importMgr.Init(&scrip);
    _givm.clear();
    if (_tq2String.empty())
        _tq2String =
            {
                { kTQ_Attribute, "attribute", },
                { kTQ_Autoptr, "autoptr", },
                { kTQ_Builtin, "builtin", },
                { kTQ_Const, "const", },
                { kTQ_ImportStd, "import", },
                { kTQ_ImportTry, "_tryimport", },
                { kTQ_Managed, "managed", },
                { kTQ_Protected, "protected", },
                { kTQ_Readonly, "readonly", },
                { kTQ_Static, "static", },
                { kTQ_Stringstruct, "stringstruct", },
                { kTQ_Writeprotected, "writeprotected", },
            };
}

void AGS::Parser::SetDynpointerInManagedVartype(Vartype &vartype)
{
    if (_sym.IsManaged(vartype))
        vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
}

// Return size of local vars that have been allocated at a higher nesting depth than from_level
size_t AGS::Parser::StacksizeOfLocals(size_t from_level)
{
    size_t total_size = 0;
    int const from_scope = static_cast<int>(from_level);
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        if (_sym[entries_idx].SScope <= from_scope)
            continue;
        if (_sym[entries_idx].SType != kSYM_LocalVar)
            continue;

        total_size +=
            (_sym.GetThisSym() == entries_idx) ? SIZE_OF_DYNPOINTER : _sym.GetSize(entries_idx);
    }
    return total_size;
}

// Does vartype v contain releasable pointers?
// Also determines whether vartype contains standard (non-dynamic) arrays.
bool AGS::Parser::ContainsReleasableDynpointers(AGS::Vartype vartype)
{
    if (_sym.IsDyn(vartype))
        return true;
    if (_sym.IsArray(vartype))
        return ContainsReleasableDynpointers(_sym.GetVartype(vartype));
    if (!_sym.IsStruct(vartype))
        return false; // Atomic non-structs can't have pointers

    std::vector<Symbol> compo_list;
    _sym.GetComponentsOfStruct(vartype, compo_list);
    for (size_t cl_idx = 0; cl_idx < compo_list.size(); cl_idx++)
        if (ContainsReleasableDynpointers(_sym.GetVartype(compo_list[cl_idx])))
            return true;

    return false;
}

// We're at the end of a block and releasing a standard array of pointers.
// MAR points to the array start. Release each array element (pointer).
ErrorType AGS::Parser::FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements, bool &clobbers_ax)
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
void AGS::Parser::FreeDynpointersOfStruct(AGS::Symbol struct_vtype, bool &clobbers_ax)
{
    std::vector<Symbol> compo_list;
    _sym.GetComponentsOfStruct(struct_vtype, compo_list);
    for (int cl_idx = 0; cl_idx < static_cast<int>(compo_list.size()); cl_idx++) // note "int"!
    {
        if (ContainsReleasableDynpointers(_sym.GetVartype(compo_list[cl_idx])))
            continue;
        // Get rid of this component
        compo_list[cl_idx] = compo_list.back();
        compo_list.pop_back();
        cl_idx--; // this might make the var negative so it needs to be int
    }

    size_t offset_so_far = 0;
    for (auto compo_it = compo_list.cbegin(); compo_it != compo_list.cend(); ++compo_it)
    {
        SymbolTableEntry &entry = _sym[*compo_it];

        // Let MAR point to the component
        size_t const diff = entry.SOffset - offset_so_far;
        if (diff > 0)
            WriteCmd(SCMD_ADD, SREG_MAR, diff);
        offset_so_far = entry.SOffset;

        if (_sym.IsDyn(entry.Vartype))
        {
            WriteCmd(SCMD_MEMZEROPTR);
            continue;
        }

        if (compo_list.back() != *compo_it)
            _scrip.push_reg(SREG_MAR);
        if (entry.IsArray(_sym))
            FreeDynpointersOfStdArray(*compo_it, clobbers_ax);
        else if (entry.IsStruct(_sym))
            FreeDynpointersOfStruct(entry.Vartype, clobbers_ax);
        if (compo_list.back() != *compo_it)
            _scrip.pop_reg(SREG_MAR);
    }
}

// We're at the end of a block and we're releasing a standard array of struct.
// MAR points to the start of the array. Release all the pointers in the array.
void AGS::Parser::FreeDynpointersOfStdArrayOfStruct(AGS::Symbol struct_vtype, SymbolTableEntry &entry, bool &clobbers_ax)
{
    clobbers_ax = true;

    // AX will be the index of the current element
    WriteCmd(SCMD_LITTOREG, SREG_AX, entry.NumArrayElements(_sym));

    BackwardJumpDest loop_start(_scrip);
    loop_start.Set();
    _scrip.push_reg(SREG_MAR);
    _scrip.push_reg(SREG_AX); // FreeDynpointersOfStruct might call funcs that clobber AX
    FreeDynpointersOfStruct(struct_vtype, clobbers_ax);
    _scrip.pop_reg(SREG_AX);
    _scrip.pop_reg(SREG_MAR);
    WriteCmd(SCMD_ADD, SREG_MAR, _sym.GetSize(struct_vtype));
    WriteCmd(SCMD_SUB, SREG_AX, 1);
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
    return;
}

// We're at the end of a block and releasing a standard array. MAR points to the start.
// Release the pointers that the array contains.
void AGS::Parser::FreeDynpointersOfStdArray(Symbol the_array, bool &clobbers_ax)
{
    int const num_of_elements = _sym.NumArrayElements(the_array);
    if (num_of_elements < 1)
        return;
    Vartype const element_vartype =
        _sym.GetVartype(_sym.GetVartype(the_array));
    if (_sym.IsDynpointer(element_vartype))
    {
        FreeDynpointersOfStdArrayOfDynpointer(num_of_elements, clobbers_ax);
        return;
    }

    if (_sym.IsStruct(element_vartype))
        FreeDynpointersOfStdArrayOfStruct(element_vartype, _sym[the_array], clobbers_ax);

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

// Free the pointers of any locals that have a nesting depth higher than from_level
void AGS::Parser::FreeDynpointersOfLocals0(int from_level, bool &clobbers_ax, bool &clobbers_mar)
{
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        SymbolTableEntry &entry = _sym[entries_idx];
        if (entry.SScope <= from_level)
            continue;
        if (kSYM_LocalVar != entry.SType)
            continue;
        if (_sym.GetThisSym() == entries_idx)
            continue; // don't touch the this pointer
        if (!ContainsReleasableDynpointers(entry.Vartype))
            continue;

        
        // Set MAR to the start of the construct that contains releasable pointers
        int const sp_offset = _scrip.offset_to_local_var_block - entry.SOffset;
        WriteCmd(SCMD_LOADSPOFFS, sp_offset);
        clobbers_mar = true;
        if (_sym.IsDyn(entry.Vartype))
            WriteCmd(SCMD_MEMZEROPTR);
        else if (entry.IsArray(_sym))
            FreeDynpointersOfStdArray(entries_idx, clobbers_ax);
        else if (entry.IsStruct(_sym))
            FreeDynpointersOfStruct(entry.Vartype, clobbers_ax);
    }
}

// Free the pointers of any locals that have a nesting depth higher than from_level
ErrorType AGS::Parser::FreeDynpointersOfLocals(int from_level, AGS::Symbol name_of_current_func, bool ax_irrelevant)
{
    if (0 != from_level)
    {
        bool dummy_bool;
        FreeDynpointersOfLocals0(from_level, dummy_bool, dummy_bool);
        return kERR_None;
    }

    // We're ending the current function; AX is containing the result of the func call.
    Vartype const func_return_vartype = _sym[name_of_current_func].FuncParamVartypes.at(0);
    bool const function_returns_void = _sym.GetVoidSym() == func_return_vartype;

    if (_sym.IsDyn(func_return_vartype) && !ax_irrelevant)
    {
        // The return value AX might point to a local dynamic object. So if we
        // now free the dynamic references and we don't take precautions,
        // this dynamic memory will get killed so our AX value is useless.
        // We only need these precautions if there are local dynamic objects.
        RestorePoint rp_before_precautions(_scrip);

        // Allocate a local dynamic pointer to hold the return value.
        _scrip.push_reg(SREG_AX);
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
        WriteCmd(SCMD_MEMWRITEPTR, SREG_AX);

        RestorePoint rp_before_freeing(_scrip);
        bool dummy_bool;
        bool mar_may_be_clobbered = false;
        FreeDynpointersOfLocals0(from_level, dummy_bool, mar_may_be_clobbered);
        bool const no_precautions_were_necessary = rp_before_freeing.IsEmpty();

        // Now release the dynamic pointer with a special opcode that prevents 
        // memory de-allocation as long as AX still has this pointer, too
        if (mar_may_be_clobbered)
            WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
        WriteCmd(SCMD_MEMREADPTR, SREG_AX);
        WriteCmd(SCMD_MEMZEROPTRND); // special opcode
        _scrip.pop_reg(SREG_BX); // do NOT pop AX here
        if (no_precautions_were_necessary)
            rp_before_precautions.Restore();
        return kERR_None;
    }

    RestorePoint rp_before_free(_scrip);
    bool clobbers_ax = false;
    bool dummy_bool;
    FreeDynpointersOfLocals0(from_level, clobbers_ax, dummy_bool);
    if (!clobbers_ax || function_returns_void)
        return kERR_None;
    // Oops. AX was carrying our return value and shouldn't have been clobbered.
    // So we have to redo this and this time save AX before freeing.
    rp_before_free.Restore();
    _scrip.push_reg(SREG_AX);
    FreeDynpointersOfLocals0(from_level, clobbers_ax, dummy_bool);
    _scrip.pop_reg(SREG_AX);

    return kERR_None;
}

// Remove defns from the _sym table that have a nesting level higher than from_level
ErrorType AGS::Parser::RemoveLocalsFromSymtable(int from_level)
{

    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        if (_sym[entries_idx].SScope <= from_level)
            continue;
        if (_sym[entries_idx].SType != kSYM_LocalVar)
            continue;

        _sym[entries_idx].SType = kSYM_NoType;
        _sym[entries_idx].SScope = 0;
        _sym[entries_idx].Flags = 0;
        _sym[entries_idx].TypeQualifiers = 0;
    }
    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfDo(AGS::Parser::NestingStack *nesting_stack)
{
    Symbol const cursym = _src.GetNext();
    if (_sym.GetSymbolType(cursym) != kSYM_While)
    {
        Error("Expected the 'while' of a 'do ... while(...)' statement");
        return kERR_UserError;
    }

    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_Semicolon)
    {
        Error("Expected ';'");
        return kERR_UserError;
    }

    // Jump back to the start of the loop while the condition is true
    nesting_stack->Start().WriteJump(SCMD_JNZ, _src.GetLineno());
    // Jumps out of the loop should go here
    nesting_stack->JumpOut().Patch(_src.GetLineno());
    // The clause has ended, so pop the level off the stack
    nesting_stack->Pop();

    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfElse(NestingStack * nesting_stack)
{
    nesting_stack->JumpOut().Patch(_src.GetLineno());
    nesting_stack->Pop();
    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfSwitch(AGS::Parser::NestingStack *nesting_stack)
{
    // If there was no terminating break at the last switch-case, 
    // write a jump to the jumpout point to prevent a fallthrough into the jumptable
    CodeLoc const lastcmd_loc = _scrip.codesize - 2;
    if (SCMD_JMP != _scrip.code[lastcmd_loc])
    {
        // The bytecode int that contains the relative jump is in codesize+1
        WriteCmd(SCMD_JMP, -77);
        nesting_stack->JumpOut().AddParam();
    }

    // We begin the jump table
    nesting_stack->SwitchJumptable().Patch(_src.GetLineno());

    // Get correct comparison operation: Don't compare strings as pointers but as strings
    Vartype const noteq_op =
        _sym.IsAnyTypeOfString(nesting_stack->SwitchExprVartype()) ? SCMD_STRINGSNOTEQ : SCMD_NOTEQUAL;

    const size_t size_of_chunks = nesting_stack->Chunks().size();
    for (size_t index = 0; index < size_of_chunks; index++)
    {
        int id;
        CodeLoc const codesize = _scrip.codesize;
        // Put the result of the expression into AX
        nesting_stack->WriteChunk(index, id);
        _fcm.UpdateCallListOnWriting(codesize, id);
        _fim.UpdateCallListOnWriting(codesize, id);
        // Do the comparison
        WriteCmd(noteq_op, SREG_AX, SREG_BX);
        // This command will be written to code[codesize] and code[codesize]+1
        WriteCmd(
            SCMD_JZ,
            ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, nesting_stack->Chunks().at(index).CodeOffset));
    }

    // Write the default jump if necessary
    if (INT_MAX != nesting_stack->SwitchDefault().Get())
        nesting_stack->SwitchDefault().WriteJump(SCMD_JMP, _src.GetLineno());

    // Patch the jumps to the end
    nesting_stack->JumpOut().Patch(_src.GetLineno());

    nesting_stack->Chunks().clear();
    nesting_stack->Pop();

    return kERR_None;
}

ErrorType AGS::Parser::IntLiteralOrConst2Value(AGS::Symbol symb, bool is_negative, std::string const &errorMsg, int &the_value)
{
    SymbolType const stype = _sym.GetSymbolType(symb);
    if (kSYM_Constant == stype)
    {
        the_value = _sym[symb].SOffset;
        if (is_negative)
            the_value = -the_value;
        return kERR_None;
    }

    if (kSYM_LiteralInt == stype)
    {
        std::string literal = _sym.GetName(symb);
        if (is_negative)
            literal = '-' + literal;

        return String2Int(literal, the_value);
    }

    if (!errorMsg.empty())
        Error(errorMsg.c_str());
    return kERR_UserError;
}

ErrorType AGS::Parser::FloatLiteral2Value(Symbol symb, bool is_negative, std::string const &errorMsg, float &the_value)
{
    SymbolType const stype = _sym.GetSymbolType(symb);
    if (kSYM_LiteralFloat == stype)
    {
        std::string literal = _sym.GetName(symb);
        if (is_negative)
            literal = '-' + literal;

        return String2Float(literal, the_value);
    }

    if (!errorMsg.empty())
        Error(errorMsg.c_str());
    return kERR_UserError;
}

// We're parsing a parameter list and we have accepted something like "(...int i"
// We accept a default value clause like "= 15" if it follows at this point.
ErrorType AGS::Parser::ParseParamlist_Param_DefaultValue(AGS::Vartype param_type, SymbolTableEntry::ParamDefault &default_value)
{
    if (kSYM_Assign != _sym.GetSymbolType(_src.PeekNext()))
    {
        default_value.Type = SymbolTableEntry::kDT_None; // No default value given
        return kERR_None;
    }

    _src.GetNext();   // Eat '='

    Symbol default_value_symbol = _src.GetNext(); // can also be "-"
    bool default_is_negative = false;
    if (_sym.Find("-") == default_value_symbol)
    {
        default_is_negative = true;
        default_value_symbol = _src.GetNext();
    }

    if (_sym.IsDyn(param_type))
    {
        default_value.Type = SymbolTableEntry::kDT_Dyn;
        default_value.DynDefault = nullptr;

        if (!default_is_negative  && _sym.Find("0") == default_value_symbol || _sym.GetNullSym() == default_value_symbol)
            return kERR_None;
        Error("Expected the parameter default 'null'");
        return kERR_UserError;
    }

    if (_sym.IsAnyIntType(param_type))
    {
        default_value.Type = SymbolTableEntry::kDT_Int;
        return IntLiteralOrConst2Value(
            default_value_symbol,
            default_is_negative,
            "Expected an integer literal or constant as parameter default",
            default_value.IntDefault);
    }

    if (!_sym.GetFloatSym() == param_type)
    {
        Error("Parameter cannot have any default value");
        return kERR_UserError;
    }

    default_value.Type = SymbolTableEntry::kDT_Float;
    if (_sym.Find("0") == default_value_symbol)
    {
        default_value.FloatDefault = 0.0f;
        return kERR_None;
    }

    return FloatLiteral2Value(
        default_value_symbol,
        default_is_negative,
        "Expected a float literal as a parameter default",
        default_value.FloatDefault);
}

ErrorType AGS::Parser::ParseDynArrayMarkerIfPresent(AGS::Vartype &vartype)
{
    if (_sym.GetSymbolType(_src.PeekNext()) != kSYM_OpenBracket)
        return kERR_None;

    _src.GetNext(); // Eat '['
    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_CloseBracket)
    {
        Error("Fixed array size cannot be used here (use '[]' instead)");
        return kERR_UserError;
    }
    vartype = _sym.VartypeWith(kVTT_Dynarray, vartype);
    return kERR_None;
}

// Copy so that the forward decl can be compared afterwards to the real one     
ErrorType AGS::Parser::CopyKnownSymInfo(SymbolTableEntry &entry, SymbolTableEntry &known_info)
{
    known_info.SType = kSYM_NoType;
    if (kSYM_NoType == entry.SType)
        return kERR_None; // there is no info yet

    known_info = entry;

    // Kill the defaults so we can check whether this defn replicates them exactly.
    size_t const num_of_params = entry.GetNumOfFuncParams();

    SymbolTableEntry::ParamDefault deflt{};
    deflt.Type = SymbolTableEntry::kDT_None;
    entry.FuncParamDefaultValues.assign(num_of_params + 1, deflt);
    return kERR_None;
}


// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
// We'll accept something like "this Character *"
ErrorType AGS::Parser::ParseFuncdecl_ExtenderPreparations(bool is_static_extender, AGS::Symbol &struct_of_func, AGS::Symbol &name_of_func, TypeQualifierSet &tqs)
{
    if (is_static_extender)
        SetFlag(tqs, kTQ_Static, true);

    _src.GetNext(); // Eat "this" or "static"
    struct_of_func = _src.GetNext();
    if (!_sym.IsStruct(struct_of_func))
    {
        Error("Expected a struct type instead of '%s'", _sym.GetName(struct_of_func).c_str());
        return kERR_UserError;
    }

    name_of_func = MangleStructAndComponent(struct_of_func, name_of_func);

    if (_sym.GetDynpointerSym() == _src.PeekNext())
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
    _sym[name_of_func].Extends = struct_of_func;
    SetFlag(_sym[name_of_func].Flags, kSFLG_StructMember, true);

    SymbolType const punctuation = _sym.GetSymbolType(_src.PeekNext());
    if (kSYM_Comma != punctuation && kSYM_CloseParenthesis != punctuation)
    {
        Error("Expected ',' or ')' (cannot specify a parameter name for an extender parameter)");
        return kERR_UserError;
    }

    if (kSYM_Comma == punctuation)
        _src.GetNext();

    return kERR_None;
}

ErrorType AGS::Parser::ParseVarname(bool accept_member_access, Symbol &structname, Symbol &varname)
{
    varname = _src.GetNext();
    if (varname <= _sym.GetLastPredefSym())
    {
        Error("Unexpected '%s'", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    if (!accept_member_access)
    {
        if (0 != structname)
            return kERR_None;

        if (kSYM_Vartype == _sym.GetSymbolType(varname))
        {
            std::string msg =
                ReferenceMsgSym("'%s' is already in use as a type name", varname);
            Error(msg.c_str(), _sym.GetName(varname).c_str());
            return kERR_UserError;
        }
        return kERR_None;
    }

    if (_sym.GetSymbolType(_src.PeekNext()) != kSYM_MemberAccess)
        return kERR_None; // done

    if (!accept_member_access)
    {
        Error("Cannot use '::' here");
        return kERR_UserError;
    }

    // We are accepting "struct::member"; so varname isn't the var name yet: it's the struct name.
    structname = varname;
    _src.GetNext(); // Eat "::"
    Symbol membername = _src.GetNext();

    // change varname to be the full function name
    varname = MangleStructAndComponent(structname, membername);
    if (varname < 0)
    {
        Error("'%s' does not contain a function '%s'",
            _sym.GetName(structname).c_str(),
            _sym.GetName(membername).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseParamlist_ParamType(AGS::Vartype &vartype)
{
    if (_sym.GetVoidSym() == vartype)
    {
        Error("A function parameter must not have the type 'void'");
        return kERR_UserError;
    }
    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    if (kPP_Main == _pp && !_sym.IsManaged(vartype) && _sym.IsStruct(vartype))
    {
        Error("'%s' is non-managed; a non-managed struct cannot be passed as parameter", _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}


// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present
ErrorType AGS::Parser::ParseParamlist_Param_Name(bool body_follows, AGS::Symbol &param_name)
{
    param_name = -1;

    if (kPP_PreAnalyze == _pp || !body_follows)
    {
        // Ignore the parameter name when present, it won't be used later on (in this phase)
        Symbol const nextsym = _src.PeekNext();
        if (IsIdentifier(nextsym))
            _src.GetNext();
        return kERR_None;
    }

    Symbol no_struct = 0;
    ErrorType retval = ParseVarname(false, no_struct, param_name);
    if (retval < 0) return retval;
    if (kSYM_GlobalVar == _sym.GetSymbolType(param_name))
    {
        // This is a definition -- so the parameter name must not be a global variable
        Error(
            ReferenceMsgSym("The name '%s' is already used for a global variable", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_UserError;
    }

    if (kSYM_NoType != _sym.GetSymbolType(param_name))
    {
       Error(
           ReferenceMsgSym("Parameter '%s' is already in use", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}


void AGS::Parser::ParseParamlist_Param_AsVar2Sym(AGS::Symbol param_name, AGS::Vartype param_vartype, bool param_is_const, int param_idx)
{
    SymbolTableEntry &param_entry = _sym[param_name];
    param_entry.SType = kSYM_LocalVar;
    param_entry.Extends = 0;
    param_entry.Vartype = param_vartype;
    param_entry.SScope = SymbolTableEntry::ParameterSScope;
    if (param_is_const)
    {
        SetFlag(param_entry.TypeQualifiers, kTQ_Readonly, true);
        param_entry.Vartype = _sym.VartypeWith(kVTT_Const, param_entry.Vartype);
    }
    // the parameters are pushed backwards, so the top of the
    // stack has the first parameter. The + 1 is because the
    // call will push the return address onto the stack as well
    param_entry.SOffset = _scrip.offset_to_local_var_block - (param_idx + 1) * SIZE_OF_STACK_CELL;
    _sym.SetDeclared(param_name, _src.GetSectionId(), _src.GetLineno());
}

ErrorType AGS::Parser::ParseParamlist_Param(AGS::Symbol name_of_func, bool body_follows, AGS::Vartype param_vartype, bool param_is_const, size_t param_idx)
{
    ErrorType retval = ParseParamlist_ParamType(param_vartype);
    if (retval < 0) return retval;
    if (param_is_const)
        param_vartype = _sym.VartypeWith(kVTT_Const, param_vartype);

    Symbol param_name;
    retval = ParseParamlist_Param_Name(body_follows, param_name);
    if (retval < 0) return retval;

    retval = ParseDynArrayMarkerIfPresent(param_vartype);
    if (retval < 0) return retval;

    SymbolTableEntry::ParamDefault param_default;
    retval = ParseParamlist_Param_DefaultValue(param_vartype, param_default);
    if (retval < 0) return retval;

    _sym[name_of_func].FuncParamVartypes.push_back(param_vartype);
    _sym[name_of_func].FuncParamDefaultValues.push_back(param_default);
    
    if (kPP_Main != _pp || !body_follows)
        return kERR_None;

    // All function parameters correspond to local variables.
    // A body will follow, so we need to enter this parameter as a variable into the symbol table
    ParseParamlist_Param_AsVar2Sym(param_name, param_vartype, param_is_const, param_idx);

    return kERR_None;
}

ErrorType AGS::Parser::ParseFuncdecl_Paramlist(AGS::Symbol funcsym, bool body_follows)
{
    _sym[funcsym].SScope = false;
    _sym[funcsym].FuncParamVartypes.resize(1u); // [0] is the return type; leave that
    _sym[funcsym].FuncParamDefaultValues.resize(1u);
    bool param_is_const = false;
    size_t param_idx = 0;
    while (!_src.ReachedEOF())
    {
        Symbol const cursym = _src.GetNext();
        if (kSYM_CloseParenthesis == _sym.GetSymbolType(cursym))
            return kERR_None;   // empty parameter list

        switch (_sym.GetSymbolType(cursym))
        {
        default:
            Error("Unexpected '%s' in parameter list", _sym.GetName(cursym).c_str());
            return kERR_UserError;

        case kSYM_Const:
        {
            // check in main compiler phase that type must follow
            if (kPP_Main == _pp && kSYM_Vartype != _sym.GetSymbolType(_src.PeekNext()))
            {
                Error("Expected a type after 'const'");
                return kERR_UserError;
            }
            param_is_const = true;
            continue;
        }

        case kSYM_Varargs:
        {
            _sym[funcsym].SScope = true;
            if (kSYM_CloseParenthesis != _sym.GetSymbolType(_src.GetNext()))
            {
                Error("Expected ')' after '...'");
                return kERR_UserError;
            }
            return kERR_None;
        }

        case kSYM_Vartype:
        {
            if (param_idx == 0 && _sym.GetVoidSym() == cursym && kSYM_CloseParenthesis == _sym.GetSymbolType(_src.PeekNext()))
            {   // explicitly empty parameter list, "(void)"
                _src.GetNext(); // Eat ')'
                return kERR_None;
            }

            if ((++param_idx) >= MAX_FUNCTION_PARAMETERS)
            {
                Error("Too many parameters defined for function (max. allowed: %d)", static_cast<int>(MAX_FUNCTION_PARAMETERS) - 1);
                return kERR_UserError;
            }

            ErrorType retval = ParseParamlist_Param(funcsym, body_follows, cursym, param_is_const, _sym[funcsym].FuncParamVartypes.size());
            if (retval < 0) return retval;

            param_is_const = false; // modifier has been used up
            Symbol const nextsym = _src.GetNext();
            SymbolType const nexttype = _sym.GetSymbolType(nextsym);
            if (kSYM_Comma != nexttype && kSYM_CloseParenthesis != nexttype)
            {
                Error("Expected ',' or ')' or an identifier, found '%s' instead", _sym.GetName(nextsym).c_str());
                return kERR_UserError;
            }
            if (kSYM_CloseParenthesis == nexttype)
                return kERR_None;
            continue;            
        }
        } // switch
    } // while
    Error("!End of input when processing parameter list");
    return kERR_InternalError;
}
void AGS::Parser::ParseFuncdecl_MasterData2Sym(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_function, Symbol name_of_function, bool body_follows)
{
    SymbolTableEntry &entry = _sym[name_of_function];
    entry.SType = kSYM_Function;
    entry.FuncParamVartypes[0] = return_vartype;
    // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the entity returned.
    entry.TypeQualifiers = tqs &  ~kTQ_Autoptr & ~kTQ_Managed & ~kTQ_Builtin;

    // Do not set Extends and the component flag here.
    // They are used to denote functions that were either declared in a struct defn or as extender

    if (kPP_PreAnalyze == _pp)
    {
        // Encode in entry.SOffset the type of function declaration
        FunctionType ft = kFT_PureForward;
        if (FlagIsSet(tqs, kTQ_Import))
            ft = kFT_Import;
        if (body_follows)
            ft = kFT_LocalBody;
        if (_sym[name_of_function].SOffset < ft)
            _sym[name_of_function].SOffset = ft;
    }
    return;
}


ErrorType AGS::Parser::ParseFuncdecl_CheckThatKIM_CheckDefaults(SymbolTableEntry const &this_entry, SymbolTableEntry const &known_info, bool body_follows)
{
    if (body_follows)
    {
        // If none of the parameters have a default, we'll let this through.
        bool has_default = false;
        for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncParams(); ++param_idx)
            if (this_entry.HasParamDefault(param_idx))
            {
                has_default = true;
                break;
            }
        if (!has_default)
            return kERR_None;
    }

    // this is 1 .. GetNumOfFuncArgs(), INCLUSIVE, because param 0 is the return type
    for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncParams(); ++param_idx)
    {
        if ((this_entry.HasParamDefault(param_idx) == known_info.HasParamDefault(param_idx)) &&
            (this_entry.HasParamDefault(param_idx) == false ||
                this_entry.FuncParamDefaultValues[param_idx] ==
                known_info.FuncParamDefaultValues[param_idx]))
            continue;

        std::string errstr1 = "In this declaration, parameter #<1> <2>; ";
        errstr1.replace(errstr1.find("<1>"), 3, std::to_string(param_idx));
        if (!this_entry.HasParamDefault(param_idx))
            errstr1.replace(errstr1.find("<2>"), 3, "doesn't have a default value");
        else
            errstr1.replace(errstr1.find("<2>"), 3, "has the default "
                + this_entry.FuncParamDefaultValues[param_idx].ToString());

        std::string errstr2 = "in a declaration elsewhere, that parameter <2>";
        if (!known_info.HasParamDefault(param_idx))
            errstr2.replace(errstr2.find("<2>"), 3, "doesn't have a default value");
        else
            errstr2.replace(errstr2.find("<2>"), 3, "has the default "
                + known_info.FuncParamDefaultValues[param_idx].ToString());
        errstr1 += errstr2;
        Error(ReferenceMsg(errstr1, known_info.DeclSectionId, known_info.DeclLine).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}

// there was a forward declaration -- check that the real declaration matches it
ErrorType AGS::Parser::ParseFuncdecl_CheckThatKnownInfoMatches(SymbolTableEntry &this_entry, SymbolTableEntry const &known_info, bool body_follows)
{
    if (kSYM_NoType == known_info.SType)
        return kERR_None; // We don't have any known info

    if (known_info.SType != this_entry.SType)
    {
        std::string msg = ReferenceMsg(
            "'%s' is declared as a function here but differently elsewhere",
            known_info.DeclSectionId,
            known_info.DeclLine);
        Error(msg.c_str(), this_entry.SName.c_str());
        return kERR_UserError;
    }

    if ((known_info.TypeQualifiers & ~kTQ_Import) != (this_entry.TypeQualifiers & ~kTQ_Import))
    {
        std::string const ki_tq = TypeQualifierSet2String(known_info.TypeQualifiers & ~kTQ_Import);
        std::string const te_tq = TypeQualifierSet2String(this_entry.TypeQualifiers & ~kTQ_Import);
        std::string msg = ReferenceMsg(
            "'%s' has the qualifiers '%s' here but '%s' elsewhere",
            known_info.DeclSectionId,
            known_info.DeclLine);
        Error(msg.c_str(), this_entry.SName.c_str(), te_tq.c_str(), ki_tq.c_str());
        return kERR_UserError;
    }

    if (known_info.GetNumOfFuncParams() != this_entry.GetNumOfFuncParams())
    {
        std::string msg = ReferenceMsg(
            "Function '%s' is declared with %d mandatory parameters here, %d mandatory parameters elswehere",
            known_info.DeclSectionId,
            known_info.DeclLine);
        Error(msg.c_str(), this_entry.SName.c_str(), this_entry.GetNumOfFuncParams(), known_info.GetNumOfFuncParams());
        return kERR_UserError;
    }
    if (known_info.IsVarargsFunc() != this_entry.IsVarargsFunc())
    {
        std::string te =
            this_entry.IsVarargsFunc() ?
            "is declared to accept additional parameters here" :
            "is declared to not accept additional parameters here";
        std::string ki =
            known_info.IsVarargsFunc() ?
            "to accepts additional parameters elsewhere" :
            "to not accept additional parameters elsewhere";
        std::string const msg =
            ReferenceMsg(
                "Function '%s' %s, %s",
                known_info.DeclSectionId,
                known_info.DeclLine);
        Error(msg.c_str(), this_entry.SName.c_str(), te.c_str(), ki.c_str());
        return kERR_UserError;
    }

    if (known_info.FuncParamVartypes.at(0) != this_entry.FuncParamVartypes.at(0))
    {
        std::string msg = ReferenceMsg(
            "Return type of '%s' is declared as '%s' here, as '%s' elsewhere",
            known_info.DeclSectionId,
            known_info.DeclLine);
        Error(
            msg.c_str(),
            this_entry.SName.c_str(),
            _sym.GetName(this_entry.FuncParamVartypes.at(0)).c_str(),
            _sym.GetName(known_info.FuncParamVartypes.at(0)).c_str());

        return kERR_UserError;
    }

    for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncParams(); param_idx++)
    {
        if (known_info.FuncParamVartypes.at(param_idx) != this_entry.FuncParamVartypes.at(param_idx))
        {
            std::string msg = ReferenceMsg(
                "For function '%s': Type of parameter #%d is %s here, %s in a declaration elsewhere",
                known_info.DeclSectionId,
                known_info.DeclLine);
            Error(
                msg.c_str(),
                this_entry.SName.c_str(),
                param_idx,
                _sym.GetName(this_entry.FuncParamVartypes.at(param_idx)).c_str(),
                _sym.GetName(known_info.FuncParamVartypes.at(param_idx)).c_str());
            return kERR_UserError;
        }
    }

    // Check that the defaults match
    ErrorType retval = ParseFuncdecl_CheckThatKIM_CheckDefaults(this_entry, known_info, body_follows);
    if (retval < 0) return retval;

    return kERR_None;
}

// Enter the function in the imports[] or functions[] array; get its index   
ErrorType AGS::Parser::ParseFuncdecl_EnterAsImportOrFunc(AGS::Symbol name_of_func, bool body_follows, bool func_is_import, AGS::CodeLoc &function_soffs, int &function_idx)
{
    if (body_follows)
    {
        // Index of the function in the ccCompiledScript::functions[] array
        function_soffs = _scrip.add_new_function(_sym.GetName(name_of_func), &function_idx);
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
    function_soffs = _importMgr.FindOrAdd(_sym.GetName(name_of_func));
    return kERR_None;
}


// We're at something like "int foo(", directly before the "("
// Get the symbol after the corresponding ")"
ErrorType AGS::Parser::ParseFuncdecl_DoesBodyFollow(bool &body_follows)
{
    int const cursor = _src.GetCursor();

    SymbolType const stoplist[] = { kSYM_NoType };
    SkipTo(_src, stoplist, 0);

    if (kSYM_CloseParenthesis != _sym.GetSymbolType(_src.GetNext()))
    {
        Error("!Unclosed parameter list of function");
        return kERR_InternalError;
    }

    body_follows = kSYM_OpenBrace == _sym.GetSymbolType(_src.PeekNext());
    _src.SetCursor(cursor);
    return kERR_None;
}

ErrorType AGS::Parser::ParseFuncdecl_Checks(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, Vartype return_vartype, bool body_follows, bool no_loop_check)
{
    if (0 >= struct_of_func && FlagIsSet(tqs, kTQ_Protected))
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

    SymbolType const stype = _sym[name_of_func].SType;
    if (kSYM_Function != stype && kSYM_NoType != stype)
    {
        Error(
            ReferenceMsgSym("'%s' is defined elsewhere as a non-function", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (!_sym.IsManaged(return_vartype) && _sym.IsStruct(return_vartype))
    {
        Error("Can only return a struct when it is 'managed'");
        return kERR_UserError;
    }

    if (kPP_PreAnalyze == _pp && body_follows && kFT_LocalBody == _sym[name_of_func].SOffset)
    {
        Error(
            ReferenceMsgSym("Function '%s' has already been defined with body elsewhere", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
        return kERR_UserError;
    }

    if (kPP_Main == _pp && 0 < struct_of_func && struct_of_func != _sym[name_of_func].Extends)
    {
        // Functions only get this if they are declared in a struct or as extender
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
    if (kPP_Main == _pp)
    {
        // Get start offset and function index
        int function_idx = -1; // Index in the _scrip.functions[] array
        int func_startoffs;
        ErrorType retval = ParseFuncdecl_EnterAsImportOrFunc(name_of_func, body_follows, FlagIsSet(tqs, kTQ_Import), func_startoffs, function_idx);
        if (retval < 0) return retval;
        _sym[name_of_func].SOffset = func_startoffs;
        if (function_idx >= 0)
            _scrip.functions[function_idx].NumOfParams =
            _sym[name_of_func].GetNumOfFuncParams();
    }

    if (!FlagIsSet(tqs, kTQ_Import))
        return kERR_None;

    // Imported functions

    SetFlag(_sym[name_of_func].TypeQualifiers, kTQ_Import, true);

    if (kPP_PreAnalyze == _pp)
    {
        _sym[name_of_func].SOffset = kFT_Import;
        return kERR_None;
    }

    if (struct_of_func > 0)
    {
        char appendage[10];
        sprintf(appendage, "^%d", _sym[name_of_func].GetNumOfFuncParams() + 100 * _sym[name_of_func].SScope);
        strcat(_scrip.imports[_sym[name_of_func].SOffset], appendage);
    }

    _fim.SetFuncCallpoint(name_of_func, _sym[name_of_func].SOffset);
    return kERR_None;
}

// We're at something like "int foo(", directly before the "("
// This might or might not be within a struct defn
// An extender func param, if any, has already been resolved
ErrorType AGS::Parser::ParseFuncdecl(size_t declaration_start, TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_func, Symbol name_of_func, bool no_loop_check, bool &body_follows)
{
    ErrorType retval = ParseFuncdecl_DoesBodyFollow(body_follows);
    if (retval < 0) return retval;

    retval = ParseFuncdecl_Checks(tqs, struct_of_func, name_of_func, return_vartype, body_follows, no_loop_check);
    if (retval < 0) return retval;
   
    // A forward decl can be written with the
    // "import" keyword (when allowed in the options). This isn't an import
    // proper, so reset the "import" flag in this case.
    if (FlagIsSet(tqs, kTQ_Import) &&   // This declaration has 'import'
        kSYM_Function == _sym.GetSymbolType(name_of_func) &&
        !FlagIsSet(_sym[name_of_func].TypeQualifiers, kTQ_Import)) // but symbol table hasn't 'import'
    {
        if (0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            std::string const msg = ReferenceMsgSym(
                "In here, a function with a local body must not have an \"import\" declaration",
                name_of_func);
            Error(msg.c_str());
            return kERR_UserError;
        }
        SetFlag(tqs, kTQ_Import, false);
    }

    if (kPP_Main == _pp && body_follows)
    {
        // When this function is called, first all the parameters are pushed on the stack
        // and then the address to which the function should return after it has finished.
        // So the first parameter isn't on top of the stack but one address below that
        _scrip.offset_to_local_var_block += SIZE_OF_STACK_CELL;
    }

    // Copy all known info about the function so that we can check whether this declaration is compatible
    SymbolTableEntry known_info;
    retval = CopyKnownSymInfo(_sym[name_of_func], known_info);
    if (retval < 0) return retval;

    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, struct_of_func, name_of_func, body_follows);

   retval = ParseFuncdecl_Paramlist(name_of_func, body_follows);
    if (retval < 0) return retval;

   retval = ParseFuncdecl_CheckThatKnownInfoMatches(_sym[name_of_func], known_info, body_follows);
    if (retval < 0) return retval;

    // copy the default values from the function prototype into the symbol table
    if (known_info.SType != kSYM_NoType)
        _sym[name_of_func].FuncParamDefaultValues.assign(
            known_info.FuncParamDefaultValues.begin(),
            known_info.FuncParamDefaultValues.end());

    retval = ParseFuncdecl_HandleFunctionOrImportIndex(tqs, struct_of_func, name_of_func, body_follows);
    if (retval < 0) return retval;

    _sym.SetDeclared(name_of_func, _src.GetSectionIdAt(declaration_start), _src.GetLinenoAt(declaration_start));
    return kERR_None;
}


// interpret the float as if it were an int (without converting it really);
// return that int
int AGS::Parser::InterpretFloatAsInt(float floatval)
{
    float *floatptr = &floatval; // Get pointer to the float
    int *intptr = reinterpret_cast<int *>(floatptr); // pretend that it points to an int
    return *intptr; // return the int that the pointer points to
}

ErrorType AGS::Parser::IndexOfLeastBondingOperator(SrcList &expression, int &idx)
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
        SymbolType current_sym_type = _sym.GetSymbolType(current_sym);
        switch (current_sym_type)
        {
        default:
            encountered_operand = true;
            break;

        case kSYM_New:
        case kSYM_Operator:
        case kSYM_Tern:
            current_sym_type = kSYM_Operator;
            break;

        case kSYM_CloseBracket:
        case kSYM_CloseParenthesis:
            encountered_operand = true;
            if (nesting_depth > 0)
                nesting_depth--;
            continue;

        case kSYM_OpenBracket:
        case kSYM_OpenParenthesis:
            nesting_depth++;
            continue;
        }

        // Continue if we aren't at zero nesting depth, since ()[] take priority
        if (nesting_depth > 0)
            continue;

        if (current_sym_type != kSYM_Operator)
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
ErrorType AGS::Parser::GetOpcodeValidForVartype(Vartype vartype1, Vartype vartype2, CodeCell &opcode)
{
    if (_sym.GetFloatSym() == vartype1 || _sym.GetFloatSym() == vartype2)
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

    bool const iatos1 = _sym.IsAnyTypeOfString(vartype1);
    bool const iatos2 = _sym.IsAnyTypeOfString(vartype2);

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
        if (_sym.GetNullSym() == vartype1 || _sym.GetNullSym() == vartype2)
            return kERR_None;

        if (iatos1 != iatos2)
        {
            Error("A string type value cannot be compared to a value that isn't a string type");
            return kERR_UserError;
        }
        return kERR_None;
    }

    if (((_sym.IsDynpointer(vartype1) || vartype1 == _sym.GetNullSym()) &&
        (_sym.IsDynpointer(vartype2) || vartype2 == _sym.GetNullSym())) ||
        ((_sym.IsDynarray(vartype1) || vartype1 == _sym.GetNullSym()) &&
        (_sym.IsDynarray(vartype2) || vartype2 == _sym.GetNullSym())))
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
    if (_sym.IsDynpointer(vartype1) || _sym.IsDynpointer(vartype2))
    {
        Error("The operator cannot be applied to values of these types");
        return kERR_UserError;
    }

    ErrorType retval = IsVartypeMismatch(vartype1, _sym.GetIntSym(), true);
    if (retval < 0) return retval;
    return IsVartypeMismatch(vartype2, _sym.GetIntSym(), true);
}

// Check for a type mismatch in one direction only
bool AGS::Parser::IsVartypeMismatch_Oneway(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be) const
{
    // cannot convert 'void' to anything
    if (_sym.GetVoidSym() == vartype_is || _sym.GetVoidSym() == vartype_wants_to_be)
        return true;

    // Don't convert if no conversion is called for
    if (vartype_is == vartype_wants_to_be)
        return false;


    // Can convert null to dynpointer or dynarray
    if (_sym.GetNullSym() == vartype_is)
        return
        !_sym.IsDynpointer(vartype_wants_to_be) &&
        !_sym.IsDynarray(vartype_wants_to_be);

    // can convert String * to const string
    if (_sym.GetStringStructSym() == _sym.VartypeWithout(kVTT_Dynpointer, vartype_is) &&
        _sym.GetOldStringSym() == _sym.VartypeWithout(kVTT_Const, vartype_wants_to_be))
    {
        return false;
    }

    // can convert string or const string to String *
    if (_sym.GetOldStringSym() == _sym.VartypeWithout(kVTT_Const, vartype_is) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(kVTT_Dynpointer, vartype_wants_to_be))
    {
        return false;
    }

    if (_sym.IsOldstring(vartype_is) != _sym.IsOldstring(vartype_wants_to_be))
        return true;

    // Note: the position of this test is important.
    // Don't "group" string tests "together" and move this test above or below them.
    // cannot convert const to non-const
    if (_sym.IsConst(vartype_is) && !_sym.IsConst(vartype_wants_to_be))
        return true;

    if (_sym.IsOldstring(vartype_is))
        return false;

    // From here on, don't mind constness or dynarray-ness
    vartype_is = _sym.VartypeWithout(kVTT_Const | kVTT_Dynarray, vartype_is);
    vartype_wants_to_be = _sym.VartypeWithout(kVTT_Const | kVTT_Dynarray, vartype_wants_to_be);

    // floats cannot mingle with other types
    if ((vartype_is == _sym.GetFloatSym()) != (vartype_wants_to_be == _sym.GetFloatSym()))
        return true;

    // Can convert short, char etc. into int
    if (_sym.IsAnyIntType(vartype_is) && _sym.GetIntSym() == vartype_wants_to_be)
        return false;

    // Checks to do if at least one is dynarray
    if (_sym.IsDynarray(vartype_is) || _sym.IsDynarray(vartype_wants_to_be))
    {
        // BOTH sides must be dynarray 
        if (_sym.IsDynarray(vartype_is) != _sym.IsDynarray(vartype_wants_to_be))
            return false;

        // The underlying core vartypes must be identical:
        // A dynarray contains a sequence of elements whose size are used
        // to index the individual element, so no extending elements
        Symbol const target_core_vartype = _sym.VartypeWithout(kVTT_Dynarray, vartype_wants_to_be);
        Symbol const current_core_vartype = _sym.VartypeWithout(kVTT_Dynarray, vartype_is);
        return current_core_vartype != target_core_vartype;
    }

    // Checks to do if at least one is dynpointer
    if (_sym.IsDynpointer(vartype_is) || _sym.IsDynpointer(vartype_wants_to_be))
    {
        // BOTH sides must be dynpointer
        if (_sym.IsDynpointer(vartype_is) != _sym.IsDynpointer(vartype_wants_to_be))
            return true;

        // Core vartypes need not be identical here: check against extensions
        Symbol const target_core_vartype = _sym.VartypeWithout(kVTT_Dynpointer, vartype_wants_to_be);
        Symbol current_core_vartype = _sym.VartypeWithout(kVTT_Dynpointer, vartype_is);
        while (current_core_vartype != target_core_vartype)
        {
            current_core_vartype = _sym[current_core_vartype].Extends;
            if (current_core_vartype == 0)
                return true;
        }
        return false;
    }

    // Checks to do if at least one is a struct or an array
    if (_sym.IsStruct(vartype_is) || _sym.IsStruct(vartype_wants_to_be) ||
        _sym.IsArray(vartype_is) || _sym.IsArray(vartype_wants_to_be))
        return (vartype_is != vartype_wants_to_be);

    return false;
}

// Check whether there is a type mismatch; if so, give an error
ErrorType AGS::Parser::IsVartypeMismatch(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be, bool orderMatters)
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
void AGS::Parser::ConvertAXStringToStringObject(AGS::Vartype wanted_vartype)
{
    if (_sym.GetOldStringSym() == _sym.VartypeWithout(kVTT_Const, _scrip.ax_vartype) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(kVTT_Dynpointer, wanted_vartype))
    {
        WriteCmd(SCMD_CREATESTRING, SREG_AX); // convert AX
        _scrip.ax_vartype = _sym.VartypeWith(kVTT_Dynpointer, _sym.GetStringStructSym());
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

ErrorType AGS::Parser::HandleStructOrArrayResult(AGS::Vartype &vartype, AGS::Parser::ValueLocation &vloc)
{
    if (_sym.IsArray(vartype))
    {
        Error("Cannot access array as a whole (did you forget to add \"[0]\"?)");
        return kERR_UserError;
    }

    if (_sym.IsAtomic(vartype) && _sym.IsStruct(vartype))
    {
        if (_sym.IsManaged(vartype))
        {
            // Interpret the memory address as the result
            vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
            WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
            vloc = kVL_ax_is_value;
            _scrip.ax_vartype = vartype;
            return kERR_None;
        }

        Error("Cannot access non-managed struct as a whole");
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ResultToAX(ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    if (kVL_mar_pointsto_value != vloc)
        return kERR_None; // So it's already in AX 

    _scrip.ax_vartype = vartype;
    _scrip.ax_scope_type = scope_type;

    if (_sym.GetOldStringSym() == _sym.VartypeWithout(kVTT_Const, vartype))
        WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
    else
        WriteCmd(
            _sym.IsDyn(vartype) ? SCMD_MEMREADPTR : GetReadCommandForSize(_sym.GetSize(vartype)),
            SREG_AX);
    vloc = kVL_ax_is_value;
    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_CheckArgOfNew(Vartype new_vartype)
{
    if (kSYM_Vartype != _sym.GetSymbolType(new_vartype))
    {
        Error("Expected a type after 'new', found '%s' instead", _sym.GetName(new_vartype).c_str());
        return kERR_UserError;
    }

    if (kSYM_UndefinedStruct == _sym.GetSymbolType(new_vartype))
    {
        Error(
            "The struct '%s' hasn't been completely defined yet",
            _sym.GetName(new_vartype).c_str());
        return kERR_UserError;
    }

    if (!_sym.IsAnyIntType(new_vartype) && !_sym.IsManaged(new_vartype))
    {
        Error("Can only use integer or managed types with 'new'");
        return kERR_UserError;
    }

    // Note: While it is an error to use a built-in type with new, it is
    // allowed to use a built-in type with new[].
    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_New(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    expression.StartRead();
    expression.GetNext(); // Eat "new"
    if (expression.ReachedEOF())
    {
        Error("Expected a type after 'new' but didn't find any");
        return kERR_UserError;
    }
    Vartype const new_vartype = expression.GetNext();
    ErrorType retval = ParseExpression_CheckArgOfNew(new_vartype);
    if (retval < 0) return retval;       

    if (expression.ReachedEOF()) // "new VARTYPE", nothing following
    {
        if (_sym.IsBuiltin(new_vartype))
        {
            Error(
                "Cannot use 'new' with the built-in type '%s'",
                _sym.GetName(new_vartype).c_str());
            return kERR_UserError;
        }

        vartype = _sym.VartypeWith(kVTT_Dynpointer, new_vartype);
        const size_t size = _sym.GetSize(new_vartype);
        WriteCmd(SCMD_NEWUSEROBJECT, SREG_AX, size);
        _scrip.ax_scope_type = scope_type = kScT_Global;
        _scrip.ax_vartype = vartype;
        vloc = kVL_ax_is_value;
        return kERR_None;
    }

    // new VARTYPE[...]
    bool const is_managed = !_sym.IsAnyIntType(new_vartype);
    int const element_size = _sym.GetSize(new_vartype);

    retval = AccessData_ReadBracketedIntExpression(expression);
    if (retval < 0) return retval;

    WriteCmd(SCMD_NEWARRAY, SREG_AX, element_size, is_managed);

    vartype = new_vartype;
    if (is_managed)
        vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
    vartype = _sym.VartypeWith(kVTT_Dynarray, vartype);
    _scrip.ax_scope_type = scope_type = kScT_Global;
    _scrip.ax_vartype = vartype;
    vloc = kVL_ax_is_value;
    return kERR_None;
}

// We're parsing an expression that starts with '-' (unary minus)
ErrorType AGS::Parser::ParseExpression_UnaryMinus(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
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
        SymbolType const stype = _sym.GetSymbolType(expression.PeekNext());
        if (kSYM_Constant == stype || kSYM_LiteralInt == stype)
            return AccessData_IntLiteralOrConst(true, expression, vartype);
        if (kSYM_LiteralFloat == stype)
            return AccessData_FloatLiteral(true, expression, vartype);
    };

    // parse the rest of the expression into AX
    ErrorType retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    CodeCell opcode = SCMD_SUBREG; 
    retval = GetOpcodeValidForVartype(_scrip.ax_vartype, _scrip.ax_vartype, opcode);
    if (retval < 0) return retval;

    // Calculate 0 - AX
    // The binary representation of 0.0 is identical to the binary representation of 0
    // so this will work for floats as well as for ints.
    WriteCmd(SCMD_LITTOREG, SREG_BX, 0);
    WriteCmd(opcode, SREG_BX, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_ax_is_value;
    return kERR_None;
}

// We're parsing an expression that starts with '!' (boolean NOT) or '~' (bitwise Negate)
ErrorType AGS::Parser::ParseExpression_Negate(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
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

    if (!_sym.IsAnyIntType(_scrip.ax_vartype))
    {
        Error(
            "Expected an integer expression after '%s' but found type %s",
            _sym.GetName(op_sym).c_str(),
            _sym.GetName(_scrip.ax_vartype).c_str());
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

    vloc = kVL_ax_is_value;
    vartype = _scrip.ax_vartype = _sym.GetIntSym();
    return kERR_None;
}

// The least binding operator is the first thing in the expression
// This means that the op must be an unary op.
ErrorType AGS::Parser::ParseExpression_Unary(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    Symbol const first_op = expression[0];

    if (kSYM_New == _sym.GetSymbolType(first_op)) // we're parsing something like "new foo"
        return ParseExpression_New(expression, vloc, scope_type, vartype);

    CodeCell const opcode = _sym.GetOperatorOpcode(first_op);
    if (SCMD_SUBREG == opcode) // we're parsing something like "- foo"
        return ParseExpression_UnaryMinus(expression, vloc, scope_type, vartype);

    if (SCMD_NOTREG == opcode) // we're parsing something like "! foo"
        return ParseExpression_Negate(expression, vloc, scope_type, vartype);

    // All the other operators need a non-empty left hand side
    Error("Unexpected operator '%s' without a preceding expression", _sym.GetName(first_op).c_str());
    return kERR_UserError;
}

// The least binding operator is '?'
ErrorType AGS::Parser::ParseExpression_Ternary(size_t tern_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    // First term ends before the '?'
    SrcList term1 = SrcList(expression, 0, tern_idx);

    // Second term begins after the '?', we don't know how long it is yet
    SrcList after_term1 = SrcList(expression, tern_idx + 1, expression.Length() - (tern_idx + 1));

    // Find beginning of third term
    SymbolType const stoplist[] = { kSYM_Label, };
    size_t stoplist_len = 1;
    after_term1.StartRead();
    SkipTo(after_term1, stoplist, stoplist_len);
    if (after_term1.ReachedEOF() || kSYM_Label != _sym.GetSymbolType(after_term1.PeekNext()))
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
        if (_sym.IsAnyTypeOfString(term2_vartype))
        {
            ConvertAXStringToStringObject(_sym.GetStringStructSym());
            term2_vartype = _scrip.ax_vartype;
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
        if (_sym.IsAnyTypeOfString(term2_vartype))
        {
            ConvertAXStringToStringObject(_sym.GetStringStructSym());
            term2_vartype = _scrip.ax_vartype;
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
    if (_sym.IsAnyTypeOfString(term3_vartype))
    {
        ConvertAXStringToStringObject(_sym.GetStringStructSym());
        term3_vartype = _scrip.ax_vartype;
    }

    if (second_term_exists)
        jumpdest_after_term2.Patch(_src.GetLineno());
    else
        test_jumpdest.Patch(_src.GetLineno());

    scope_type =
        (kScT_Local == term2_scope_type || kScT_Local == term3_scope_type) ?
        kScT_Local : kScT_Global;

    if (!IsVartypeMismatch_Oneway(term2_vartype, term3_vartype))
    {
        vartype = _scrip.ax_vartype = term3_vartype;
        return kERR_None;
    }
    if (!IsVartypeMismatch_Oneway(term3_vartype, term2_vartype))
    {
        vartype = _scrip.ax_vartype = term2_vartype;
        return kERR_None;
    }

    term3.SetCursor(0);
    Error("An expression of type '%s' is incompatible with an expression of type '%s'",
        _sym.GetName(term2_vartype).c_str(), _sym.GetName(term3_vartype).c_str());
    return kERR_UserError;
}

// The least binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
ErrorType AGS::Parser::ParseExpression_Binary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
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
    int const opcode = _sym.GetOperatorOpcode(operator_sym);

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

    _scrip.push_reg(SREG_AX);
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

    _scrip.pop_reg(SREG_BX); // Note, we pop to BX although we have pushed AX
    // now the result of the left side is in BX, of the right side is in AX

    // Check whether the left side type and right side type match either way
    retval = IsVartypeMismatch(vartype_lhs, vartype, false);
    if (retval < 0) return retval;

    int actual_opcode = opcode;
    retval = GetOpcodeValidForVartype(vartype_lhs, vartype, actual_opcode);
    if (retval < 0) return retval;

    WriteCmd(actual_opcode, SREG_BX, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_ax_is_value;

    to_exit.Patch(_src.GetLineno());

    // Operators like == return a bool (in our case, that's an int);
    // other operators like + return the type that they're operating on
    if (IsBooleanOpcode(actual_opcode))
        _scrip.ax_vartype = vartype = _sym.GetIntSym();

    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_BinaryOrTernary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    Symbol const operator_sym = expression[op_idx];
    if (kSYM_Tern == _sym.GetSymbolType(operator_sym))
        return ParseExpression_Ternary(op_idx, expression, vloc, scope_type, vartype);
    return ParseExpression_Binary(op_idx, expression, vloc, scope_type, vartype);
}

ErrorType AGS::Parser::ParseExpression_InParens(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    // find the corresponding closing parenthesis
    size_t const bp_start = 1;
    expression.SetCursor(bp_start); // Skip the '('
    SymbolType stoplist[] = { kSYM_NoType };
    SkipTo(expression, stoplist, 0);
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
ErrorType AGS::Parser::AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import)
{
    for (size_t arg_idx = num_func_args; arg_idx > num_supplied_args; arg_idx--)
    {
        if (!_sym[funcSymbol].HasParamDefault(arg_idx))
        {
            Error("Function call parameter # %d isn't provided and doesn't have any default value", arg_idx);
            return kERR_UserError;
        }

        // push the default value onto the stack
        WriteCmd(
            SCMD_LITTOREG,
            SREG_AX,
            _sym[funcSymbol].FuncParamDefaultValues[arg_idx].ToInt32());

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            _scrip.push_reg(SREG_AX);
    }
    return kERR_None;
}

void AGS::Parser::DoNullCheckOnStringInAXIfNecessary(AGS::Vartype valTypeTo)
{

    if (_sym.GetStringStructSym() == _sym.VartypeWithout(kVTT_Dynpointer, _scrip.ax_vartype) &&
        _sym.GetOldStringSym() == _sym.VartypeWithout(kVTT_Const, valTypeTo))
        WriteCmd(SCMD_CHECKNULLREG, SREG_AX);
}

std::string AGS::Parser::ReferenceMsg(std::string const &msg, int section_id, int line)
{
    std::string const &section = _src.SectionId2Section(section_id);

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

std::string AGS::Parser::ReferenceMsgSym(std::string const &msg, AGS::Symbol symb)
{
    return ReferenceMsg(
        msg,
        _sym.GetDeclaredSectionId(symb),
        _sym.GetDeclaredLine(symb));
}

ErrorType AGS::Parser::AccessData_FunctionCall_PushParams(SrcList &parameters, size_t closed_paren_idx, size_t num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import)
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
            const SymbolType idx_type = _sym.GetSymbolType(parameters[paramListIdx]);
            if (idx_type == kSYM_CloseParenthesis)
                paren_nesting_depth++;
            if (idx_type == kSYM_OpenParenthesis)
                paren_nesting_depth--;
            if ((paren_nesting_depth == 0 && idx_type == kSYM_Comma) ||
                (paren_nesting_depth < 0 && idx_type == kSYM_OpenParenthesis))
            {
                start_of_current_param = paramListIdx + 1;
                break;
            }
            if (paramListIdx == 0)
                break; // Don't put this into the for header!
        }

        if (end_of_current_param < static_cast<int>(start_of_current_param))
        {
            Error("!parameter length is negative");
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
            Vartype const param_vartype = _sym[funcSymbol].FuncParamVartypes[param_num];
            ConvertAXStringToStringObject(param_vartype);
            vartype = _scrip.ax_vartype;
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
            _scrip.push_reg(SREG_AX);

        end_of_current_param = start_of_current_param - 1;
    }
    while (end_of_current_param > 0);

    return kERR_None;
}


// Count parameters, check that all the parameters are non-empty; find closing paren
ErrorType AGS::Parser::AccessData_FunctionCall_CountAndCheckParm(SrcList &parameters, AGS::Symbol name_of_func, size_t &index_of_close_paren, size_t &num_supplied_args)
{
    size_t paren_nesting_depth = 1;
    num_supplied_args = 1;
    size_t param_idx;
    bool found_param_symbol = false;

    for (param_idx = 1; param_idx < parameters.Length(); param_idx++)
    {
        const SymbolType idx_type = _sym.GetSymbolType(parameters[param_idx]);

        if (idx_type == kSYM_OpenParenthesis)
            paren_nesting_depth++;
        if (idx_type == kSYM_CloseParenthesis)
        {
            paren_nesting_depth--;
            if (paren_nesting_depth == 0)
                break;
        }

        if (paren_nesting_depth == 1 && idx_type == kSYM_Comma)
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
        _sym.GetSymbolType(parameters[1]) == kSYM_CloseParenthesis)
    {
        num_supplied_args = 0;
    }

    index_of_close_paren = param_idx;

    if (_sym.GetSymbolType(parameters[index_of_close_paren]) != kSYM_CloseParenthesis)
    {
        Error("!Missing ')' at the end of the parameter list");
        return kERR_InternalError;
    }

    if (index_of_close_paren > 0 &&
        _sym.GetSymbolType(parameters[index_of_close_paren - 1]) == kSYM_Comma)
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
    WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[name_of_func].SOffset);

    if (func_is_import)
    {
        _scrip.fixup_previous(kFx_Import); 
        if (!_importMgr.IsDeclaredImport(_sym.GetName(name_of_func)))
            _fim.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1);

        WriteCmd(SCMD_CALLEXT, SREG_AX); // Do the call
        // At runtime, we will arrive here when the function call has returned: Restore the stack
        if (num_args > 0)
            WriteCmd(SCMD_SUBREALSTACK, num_args);
        return;
    }

    // Func is non-import
    _scrip.fixup_previous(kFx_Code);
    if (_fcm.IsForwardDecl(name_of_func))
        _fcm.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1);

    WriteCmd(SCMD_CALL, SREG_AX);  // Do the call

    // At runtime, we will arrive here when the function call has returned: Restore the stack
    if (num_args > 0)
    {
        size_t const size_of_passed_args = num_args * SIZE_OF_STACK_CELL;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_passed_args);
        _scrip.offset_to_local_var_block -= size_of_passed_args;
    }
}

// We are processing a function call.
// Get the parameters of the call and push them onto the stack.
ErrorType AGS::Parser::AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SrcList &parameters, size_t &actual_num_args)
{
    size_t const num_func_args = _sym[name_of_func].GetNumOfFuncParams();

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
    if (num_supplied_args > num_func_args && !_sym[name_of_func].IsVarargsFunc())
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

ErrorType AGS::Parser::AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, MemoryLocation &mloc, Vartype &rettype)
{
    if (kSYM_OpenParenthesis != _sym.GetSymbolType(expression[1]))
    {
        Error("Expected '('");
        return kERR_UserError;
    }

    expression.EatFirstSymbol();

    bool const func_is_import = FlagIsSet(_sym[name_of_func].TypeQualifiers, kTQ_Import);
    // If function uses normal stack, we need to do stack calculations to get at certain elements
    bool const func_uses_normal_stack = !func_is_import;
    bool const func_uses_this =
        std::string::npos != _sym.GetName(name_of_func).find("::") &&
        !FlagIsSet(_sym[name_of_func].TypeQualifiers, kTQ_Static);
    bool mar_pushed = false;
    bool op_pushed = false;

    if (func_uses_this)
    {
        if (0 != _sym.GetVartype(_sym.GetThisSym())) // the calling function uses "this"
        {   // Save OP since we must restore it after the func call
            _scrip.push_reg(SREG_OP); 
            op_pushed = true;
        }

        // MAR contains the address of "outer"; this is what will be used for "this" in the called function.
        mloc.MakeMARCurrent(_src.GetLineno(), _scrip);

        // Parameter processing might entail calling yet other functions, e.g., in "f(...g(x)...)".
        // So we can't emit SCMD_CALLOBJ here, before parameters have been processed.
        // Save MAR because parameter processing might clobber it 
        _scrip.push_reg(SREG_MAR);
        mar_pushed = true;
    }

    size_t num_args = 0;
    ErrorType retval = AccessData_PushFunctionCallParams(name_of_func, func_is_import, expression, num_args);
    if (retval < 0) return retval;

    if (func_uses_this)
    {
        if (0 == num_args)
        {   // MAR is still current, so undo the unneeded PUSH above.
            _scrip.offset_to_local_var_block -= SIZE_OF_STACK_CELL;
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
    rettype = _scrip.ax_vartype = _sym[name_of_func].FuncParamVartypes[0];
    _scrip.ax_scope_type = kScT_Local;

    if (mar_pushed)
        _scrip.pop_reg(SREG_MAR);
    if (op_pushed)
        _scrip.pop_reg(SREG_OP); // Recover the current "this"

    MarkAcessed(name_of_func);
    return kERR_None;
}

ErrorType AGS::Parser::ParseExpression_NoOps(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    Symbol const first_sym = expression[0];
    SymbolType const first_sym_type = _sym.GetSymbolType(first_sym);
    if (kSYM_OpenParenthesis == first_sym_type)
        return ParseExpression_InParens(expression, vloc, scope_type, vartype);

    if (kSYM_Operator != first_sym_type)
        return AccessData(false, expression, vloc, scope_type, vartype);

    Error("Unexpected '%s'", _sym.GetName(first_sym).c_str());
    return kERR_UserError;
}

ErrorType AGS::Parser::ParseExpression_Term(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    if (expression.Length() == 0)
    {
        Error("!Cannot parse empty subexpression");
        return kERR_InternalError;
    }

    SymbolType const stype = _sym.GetSymbolType(expression[0]);
    if (kSYM_CloseParenthesis == stype || kSYM_CloseBracket == stype || kSYM_CloseBrace == stype)
    {   // Shouldn't happen: the scanner sees to it that nesting symbols match
        Error(
            "!Unexpected '%s' at start of expression",
            _sym.GetName(expression[0]).c_str());
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
ErrorType AGS::Parser::AccessData_ReadIntExpression(SrcList &expression)
{
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;
    ErrorType retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope_type, vartype);
    if (retval < 0) return retval;

    return IsVartypeMismatch(vartype, _sym.GetIntSym(), true);
}

// We access a variable or a component of a struct in order to read or write it.
// This is a simple member of the struct.
ErrorType AGS::Parser::AccessData_StructMember(AGS::Symbol component, bool writing, bool access_via_this, SrcList &expression, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    expression.GetNext(); // Eat component
    SymbolTableEntry &entry = _sym[component];

    if (writing && FlagIsSet(entry.TypeQualifiers, kTQ_Writeprotected) && !access_via_this)
    {
        Error(
            "Writeprotected component '%s' must not be modified from outside",
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }
    if (FlagIsSet(entry.TypeQualifiers, kTQ_Protected) && !access_via_this)
    {
        Error(
            "Protected component '%s' must not be accessed from outside",
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }

    mloc.AddComponentOffset(entry.SOffset);
    vartype = _sym.GetVartype(component);
    return kERR_None;
}

// Get the symbol for the get or set function corresponding to the attribute given.
ErrorType AGS::Parser::ConstructAttributeFuncName(AGS::Symbol attribsym, bool writing, bool indexed, AGS::Symbol &func)
{
    std::string member_str = _sym.GetName(attribsym);
    // If "::" in the name, take the part after the last "::"
    size_t const m_access_position = member_str.rfind("::");
    if (std::string::npos != m_access_position)
        member_str = member_str.substr(m_access_position + 2);
    char const *stem_str = writing ? "set" : "get";
    char const *indx_str = indexed ? "i_" : "_";
    std::string func_str = stem_str + (indx_str + member_str);
    func = _sym.FindOrAdd(func_str.c_str());
    return kERR_None;
}

// We call the getter or setter of an attribute
ErrorType AGS::Parser::AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, AGS::Vartype &vartype)
{
    Symbol const component = expression.GetNext();
    Symbol const struct_of_component =
        FindStructOfComponent(Vartype2Symbol(vartype), component);
    if (0 == struct_of_component)
    {
        Error(
            "Struct '%s' does not have an attribute named '%s'",
            _sym.GetName(Vartype2Symbol(vartype)).c_str(),
            _sym.GetName(component).c_str());
        return kERR_UserError;
    }
    Symbol const name_of_attribute = MangleStructAndComponent(struct_of_component, component);

    bool const attrib_uses_this =
        !FlagIsSet(_sym[name_of_attribute].TypeQualifiers, kTQ_Static);
    bool const call_is_indexed =
        (kSYM_OpenBracket == _sym.GetSymbolType(expression.PeekNext()));
    bool const attrib_is_indexed =
        _sym.IsDynarray(name_of_attribute);

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
    Symbol name_of_func = -1;
    ErrorType retval = ConstructAttributeFuncName(component, is_setter, attrib_is_indexed, name_of_func);
    if (retval < 0) return retval;
    name_of_func = MangleStructAndComponent(struct_of_component, name_of_func);
    if (name_of_func < 0) return retval;

    bool const func_is_import = FlagIsSet(_sym[name_of_func].TypeQualifiers, kTQ_Import);

    if (attrib_uses_this)
        _scrip.push_reg(SREG_OP); // is the current this ptr, must be restored after call

    size_t num_of_args = 0;
    if (is_setter)
    {
        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            _scrip.push_reg(SREG_AX);
        ++num_of_args;
    }

    if (call_is_indexed)
    {
        // The index to be set is in the [...] clause; push it as the first parameter
        if (attrib_uses_this)
            _scrip.push_reg(SREG_MAR); // must not be clobbered
        retval = AccessData_ReadBracketedIntExpression(expression);
        if (retval < 0) return retval;

        if (attrib_uses_this)
            _scrip.pop_reg(SREG_MAR);

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            _scrip.push_reg(SREG_AX);
        ++num_of_args;
    }

    if (attrib_uses_this)
        WriteCmd(SCMD_CALLOBJ, SREG_MAR); // make MAR the new this ptr

    AccessData_GenerateFunctionCall(name_of_func, num_of_args, func_is_import);

    if (attrib_uses_this)
        _scrip.pop_reg(SREG_OP); // restore old this ptr after the func call

    // attribute return type
    _scrip.ax_scope_type = kScT_Local;
    _scrip.ax_vartype = vartype = _sym[name_of_func].FuncParamVartypes[0];

    MarkAcessed(name_of_func);
    return kERR_None;
}


// Location contains a pointer to another address. Get that address.
ErrorType AGS::Parser::AccessData_Dereference(ValueLocation &vloc, AGS::Parser::MemoryLocation &mloc)
{
    if (kVL_ax_is_value == vloc)
    {
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        vloc = kVL_mar_pointsto_value;
        mloc.Reset();
    }
    else
    {
        mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        // Note: We need to check here whether m[MAR] == 0, but CHECKNULL
        // checks whether MAR == 0. So we need to do MAR := m[MAR] first.
        WriteCmd(SCMD_MEMREADPTR, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
    }
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_ProcessArrayIndexConstant(size_t idx, Symbol index_symbol, bool negate, size_t num_array_elements, size_t element_size, MemoryLocation &mloc)
{
    int array_index = -1;
    std::string msg = "Error parsing array index #<1>";
    msg.replace(msg.find("<1>"), 3, std::to_string(idx + 1));
    ErrorType retval = IntLiteralOrConst2Value(index_symbol, negate, msg.c_str(), array_index);
    if (retval < 0) return retval;
    if (array_index < 0)
    {
        Error(
            "Array index #%d is %d, thus out of bounds (minimum is 0)",
            static_cast<int>(idx + 1),
            array_index);
        return kERR_UserError;
    }
    if (num_array_elements > 0 && static_cast<size_t>(array_index) >= num_array_elements)
    {
        Error(
            "Array index #%d is %d, thus out of bounds (maximum is %d)",
            static_cast<int>(idx + 1),
            array_index,
            static_cast<int>(num_array_elements - 1));
        return kERR_UserError;
    }

    mloc.AddComponentOffset(array_index * element_size);
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_ProcessCurrentArrayIndex(size_t idx, size_t dim, size_t factor, bool is_dynarray, SrcList &expression, MemoryLocation &mloc)
{
    // Get the index
    size_t const index_start = expression.GetCursor();
    SymbolType const stoplist[] = { kSYM_Comma, kSYM_CloseBracket, };
    SkipTo(expression, stoplist, 2);
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
        SymbolType const index_sym_type = _sym.GetSymbolType(index_sym);
        if (kSYM_LiteralInt == index_sym_type || kSYM_Constant == index_sym_type)
            return AccessData_ProcessArrayIndexConstant(idx, index_sym, false, dim, factor, mloc);
    }
    if (2 == current_index.Length())
    {
        Symbol const op_sym = current_index[0];
        Symbol const index_sym = current_index[1];
        SymbolType const index_sym_type = _sym.GetSymbolType(index_sym);
        if (kSYM_Operator == _sym[op_sym].SType && SCMD_SUBREG ==_sym[op_sym].OperatorOpcode &&
            (kSYM_LiteralInt == index_sym_type || kSYM_Constant == index_sym_type))
            return AccessData_ProcessArrayIndexConstant(idx, index_sym, true, dim, factor, mloc);
    }

    // Save MAR from being clobbered if it may contain something important
    // If nothing has been done, MAR can't have been set yet, so no need to remember it
    bool mar_pushed = false;
    if (!mloc.NothingDoneYet())
    {
        mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
        _scrip.push_reg(SREG_MAR);
        mar_pushed = true;
    }

    ErrorType retval = AccessData_ReadIntExpression(current_index);
    if (retval < 0) return retval;

    if (mar_pushed)
        _scrip.pop_reg(SREG_MAR);
    mloc.MakeMARCurrent(_src.GetLineno(), _scrip);

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
ErrorType AGS::Parser::AccessData_ProcessAnyArrayIndex(ValueLocation vloc_of_array, SrcList &expression, ValueLocation &vloc, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    if (kSYM_OpenBracket != _sym.GetSymbolType(expression.PeekNext()))
        return kERR_None;
    expression.GetNext(); // Eat '['

    bool const is_dynarray = _sym.IsDynarray(vartype);
    bool const is_array = _sym.IsArray(vartype);
    if (!is_dynarray && !is_array)
    {
        Error("Array index is only legal after an array expression");
        return kERR_UserError;
    }

    Vartype const element_vartype = _sym[vartype].Vartype;
    size_t const element_size = _sym.GetSize(element_vartype);
    std::vector<size_t> dim_sizes;
    std::vector<size_t> dynarray_dims = { 0, };
    std::vector<size_t> &dims = is_dynarray ? dynarray_dims : _sym[vartype].Dims;
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

        SymbolType divider_sym_type = _sym.GetSymbolType(expression.PeekNext());
        if (kSYM_CloseBracket != divider_sym_type && kSYM_Comma != divider_sym_type)
        {
            Error("Expected ',' or '] after array index");
            return kERR_UserError;
        }
        if (kSYM_CloseBracket == divider_sym_type)
        {
            expression.GetNext(); // Eat ']'
            divider_sym_type = _sym.GetSymbolType(expression.PeekNext());
        }
        if (kSYM_Comma == divider_sym_type || kSYM_OpenBracket == divider_sym_type)
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

ErrorType AGS::Parser::AccessData_GlobalOrLocalVar(bool is_global, bool writing, SrcList &expression, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    Symbol varname = expression.GetNext();
    SymbolTableEntry &entry = _sym[varname];
    CodeCell const soffs = entry.SOffset;

    if (writing && FlagIsSet(entry.TypeQualifiers, kTQ_Readonly))
    {
        Error("Cannot write to readonly '%s'", _sym.GetName(varname).c_str());
        return kERR_UserError;
    }

    if (FlagIsSet(entry.TypeQualifiers, kTQ_Import))
        mloc.SetStart(kScT_Import, soffs);
    else
        mloc.SetStart(is_global ? kScT_Global : kScT_Local, soffs);

    vartype = _sym.GetVartype(varname);

    // Process an array index if it follows
    ValueLocation vl_dummy = kVL_mar_pointsto_value;
    return AccessData_ProcessAnyArrayIndex(kVL_mar_pointsto_value, expression, vl_dummy, mloc, vartype);
}

ErrorType AGS::Parser::AccessData_Static(SrcList &expression, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    vartype = expression[0];
    expression.EatFirstSymbol(); // Eat vartype
    mloc.Reset();
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_FloatLiteral(bool negate, SrcList &expression, AGS::Vartype &vartype)
{
    float f;
    ErrorType retval = String2Float(_sym.GetName(expression.GetNext()), f);
    if (retval < 0) return retval;

    if (negate)
        f = -f;
    int const i = InterpretFloatAsInt(f);

    WriteCmd(SCMD_LITTOREG, SREG_AX, i);
    _scrip.ax_vartype = vartype = _sym.GetFloatSym();
    _scrip.ax_scope_type = kScT_Global;
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_IntLiteralOrConst(bool negate, SrcList &expression, AGS::Vartype &vartype)
{
    int literal;
    
    ErrorType retval = IntLiteralOrConst2Value(expression.GetNext(), negate, "Error parsing integer value", literal);
    if (retval < 0) return retval;

    WriteCmd(SCMD_LITTOREG, SREG_AX, literal);
    _scrip.ax_vartype = vartype = _sym.GetIntSym();
    _scrip.ax_scope_type = kScT_Global;
    return kERR_None;
}

ErrorType AGS::Parser::AccessData_Null(SrcList &expression, AGS::Vartype &vartype)
{
    expression.GetNext(); // Eat 'null'

    WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    _scrip.ax_vartype = vartype = _sym.GetNullSym();
    _scrip.ax_scope_type = kScT_Global;

    return kERR_None;
}

ErrorType AGS::Parser::AccessData_StringLiteral(SrcList &expression, AGS::Vartype &vartype)
{
    WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[expression.GetNext()].SOffset);
    _scrip.fixup_previous(kFx_String);
    _scrip.ax_vartype = vartype = _sym.VartypeWith(kVTT_Const, _sym.GetOldStringSym());

    return kERR_None;
}

ErrorType AGS::Parser::AccessData_FirstClause(bool writing, SrcList &expression, ValueLocation &vloc, ScopeType &return_scope_type, AGS::Parser::MemoryLocation &mloc, AGS::Vartype &vartype, bool &access_via_this, bool &static_access)
{
    if (expression.Length() < 1)
    {
        Error("!Empty variable");
        return kERR_InternalError;
    }
    expression.StartRead();

    Symbol const first_sym = expression.PeekNext();

    if (_sym.GetThisSym() == first_sym)
    {
        expression.GetNext(); // Eat 'this'
        expression.GetNext(); // Eat '.'
        vartype = _sym.GetVartype(_sym.GetThisSym());
        if (0 == vartype)
        {
            Error("'this' is only legal in non-static struct functions");
            return kERR_UserError;
        }
        vloc = kVL_mar_pointsto_value;
        WriteCmd(SCMD_REGTOREG, SREG_OP, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        mloc.Reset();
        access_via_this = true;
        return kERR_None;
    }

    switch (_sym.GetSymbolType(first_sym))
    {
    default:
    {
        // If this unknown symbol can be interpreted as a component of this,
        // treat it that way.
        vartype = _sym.GetVartype(_sym.GetThisSym());
        Symbol const thiscomponent = MangleStructAndComponent(vartype, first_sym);
        if (0 != _sym[thiscomponent].SType)
        {
            vloc = kVL_mar_pointsto_value;
            WriteCmd(SCMD_REGTOREG, SREG_OP, SREG_MAR);
            WriteCmd(SCMD_CHECKNULL);
            mloc.Reset();
            access_via_this = true;

            // We _should_ prepend "this." to the expression here but can't do that.
            // So we don't
            return kERR_None;
        }

        Error("Unexpected '%s'", _sym.GetName(expression.GetNext()).c_str());
        return kERR_UserError;
    }

    case kSYM_Constant:
        if (writing) break; // to error msg
        return_scope_type = mloc.ScType = kScT_Global;
        vloc = kVL_ax_is_value;
        return AccessData_IntLiteralOrConst(false, expression, vartype);

    case kSYM_Function:
    {
        return_scope_type = mloc.ScType = kScT_Global;
        vloc = kVL_ax_is_value;
        ErrorType retval = AccessData_FunctionCall(first_sym, expression, mloc, vartype);
        if (retval < 0) return retval;
        if (_sym.IsDynarray(vartype))
            return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
        return kERR_None;
    }

    case kSYM_GlobalVar:
    {
        return_scope_type = mloc.ScType = kScT_Global;
        vloc = kVL_mar_pointsto_value;
        bool const is_global = true;
        MarkAcessed(first_sym);
        return AccessData_GlobalOrLocalVar(is_global, writing, expression, mloc, vartype);
    }

    case kSYM_LiteralFloat:
        if (writing) break; // to error msg
        return_scope_type = mloc.ScType = kScT_Global;
        vloc = kVL_ax_is_value;
        return AccessData_FloatLiteral(false, expression, vartype);

    case kSYM_LiteralInt:
        if (writing) break; // to error msg
        return_scope_type = mloc.ScType = kScT_Global;
        vloc = kVL_ax_is_value;
        return AccessData_IntLiteralOrConst(false, expression, vartype);

    case kSYM_LiteralString:
        if (writing) break; // to error msg
        return_scope_type = mloc.ScType = kScT_Global;
        vloc = kVL_ax_is_value;
        return AccessData_StringLiteral(expression, vartype);

    case kSYM_LocalVar:
    {
        // Parameters can be returned although they are local because they are allocated
        // outside of the function proper. The return scope type for them is global.
        return_scope_type = _sym[first_sym].IsParameter() ? kScT_Global : kScT_Local;
        vloc = kVL_mar_pointsto_value;
        bool const is_global = false;
        return AccessData_GlobalOrLocalVar(is_global, writing, expression, mloc, vartype);
    }

    case kSYM_Null:
        if (writing) break; // to error msg
        return_scope_type = mloc.ScType = kScT_Global;
        vloc = kVL_ax_is_value;
        return AccessData_Null(expression, vartype);

    case kSYM_Vartype:
        return_scope_type = mloc.ScType = kScT_Global;
        static_access = true;
        return AccessData_Static(expression, mloc, vartype);
    }

    Error("Cannot assign a value to '%s'", _sym.GetName(expression[0]).c_str());
    return kERR_UserError;
}

// We're processing a STRUCT.STRUCT. ... clause.
// We've already processed some structs, and the type of the last one is vartype.
// Now we process a component of vartype.
ErrorType AGS::Parser::AccessData_SubsequentClause(bool writing, bool access_via_this, bool static_access, SrcList &expression, ValueLocation &vloc, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    Symbol const next_sym = expression.PeekNext();

    Symbol const component = AccessData_FindComponent(Vartype2Symbol(vartype), next_sym);
    SymbolType const component_type = (component) ? _sym.GetSymbolType(component) : kSYM_NoType;

    if (static_access && !FlagIsSet(_sym[component].TypeQualifiers, kTQ_Static))
    {
        Error("Must specify a specific struct for non-static component %s", _sym.GetName(component).c_str());
        return kERR_UserError;
    }

    ErrorType retval;
    switch (component_type)
    {
    default:
        Error(
            "Expected a component of '%s', found '%s' instead",
            _sym.GetName(vartype).c_str(),
            _sym.GetName(next_sym).c_str());
        return kERR_UserError;

    case kSYM_Attribute:
    {
        mloc.MakeMARCurrent(_src.GetLineno(), _scrip); // make MAR point to the struct of the attribute
        if (writing)
        {
            // We cannot process the attribute here so return to the assignment that
            // this attribute was originally called from
            vartype = _sym.GetVartype(component);
            vloc = kVL_attribute;
            return kERR_None;
        }
        vloc = kVL_ax_is_value;
        bool const is_setter = false;
        return AccessData_CallAttributeFunc(is_setter, expression, vartype);
    }

    case kSYM_Function:
    {
        vloc = kVL_ax_is_value;
        mloc.ScType = kScT_Local;
        SrcList start_of_funccall = SrcList(expression, expression.GetCursor(), expression.Length());
        retval = AccessData_FunctionCall(component, start_of_funccall, mloc, vartype);
        if (retval < 0) return retval;
        if (_sym.IsDynarray(vartype))
            return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
        return kERR_None;
    }

    case kSYM_StructComponent:
        vloc = kVL_mar_pointsto_value;
        retval = AccessData_StructMember(component, writing, access_via_this, expression, mloc, vartype);
        if (retval < 0) return retval;
        return AccessData_ProcessAnyArrayIndex(vloc, expression, vloc, mloc, vartype);
    }

    // Can't reach
}

AGS::Symbol AGS::Parser::FindStructOfComponent(Vartype strct, Symbol component)
{
    do
    {
        Symbol symb = MangleStructAndComponent(strct, component);
        if (kSYM_NoType != _sym.GetSymbolType(symb))
            return strct;
        strct = _sym[strct].Extends;
    }
    while (strct > 0);
    return 0;
}

AGS::Symbol AGS::Parser::AccessData_FindComponent(AGS::Vartype strct, AGS::Symbol component)
{
    do
    {
        Symbol ret = MangleStructAndComponent(strct, component);
        if (kSYM_NoType != _sym.GetSymbolType(ret))
            return ret;
        strct = _sym[strct].Extends;
    }
    while (strct > 0);
    return 0;
}

// We are in a STRUCT.STRUCT.STRUCT... cascade.
// Check whether we have passed the last dot
ErrorType AGS::Parser::AccessData_IsClauseLast(SrcList &expression, bool &is_last)
{
    size_t const cursor = expression.GetCursor();
    SymbolType const stoplist[] = { kSYM_Dot };
    SkipTo(expression, stoplist, 1);
    is_last = (kSYM_Dot != _sym.GetSymbolType(expression.PeekNext()));
    expression.SetCursor(cursor);
    return kERR_None;
}

// Access a variable, constant, literal, func call, struct.component.component cascade, etc.
// Result is in AX or m[MAR], dependent on vloc. Type is in vartype.
// At end of function, symlist and symlist_len will point to the part of the symbol string
// that has not been processed yet
// NOTE: If this selects an attribute for writing, then the corresponding function will
// _not_ be called and symlist[0] will be the attribute.
ErrorType AGS::Parser::AccessData(bool writing, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, AGS::Vartype &vartype)
{
    if (0 == expression.Length())
    {
        Error("!empty expression");
        return kERR_InternalError;
    }
    
    // For memory accesses, we set the MAR register lazily so that we can
    // accumulate offsets at runtime instead of compile time.
    // This struct tracks what we will need to do to set the MAR register.
    MemoryLocation mloc = MemoryLocation();

    bool clause_is_last = false;
    ErrorType retval = AccessData_IsClauseLast(expression, clause_is_last);
    if (retval < 0) return retval;

    bool access_via_this = false; // only true when "this" has just been parsed
    bool static_access = false; // only true when a vartype has just been parsed

    // If we are reading, then all the accesses are for reading.
    // If we are writing, then all the accesses except for the last one
    // are for reading and the last one will be for writing.
    retval = AccessData_FirstClause((writing && clause_is_last), expression, vloc, scope_type, mloc, vartype, access_via_this, static_access);
    if (retval < 0) return retval;

    Vartype outer_vartype = 0;

    // If the previous function has assumed a "this." that isn't there,
    // then a '.' won't be coming up but the while body must be executed anyway.
    // This is why the while condition has "access_via_this" in it.
    while (kSYM_Dot == _sym.GetSymbolType(expression.PeekNext()) || access_via_this)
    {
        if (!access_via_this)
            expression.GetNext(); // Eat '.' or its replacement
        if (expression.ReachedEOF())
        {
            Error("Expected struct component after '.' but did not find it");
            return kERR_UserError;
        }
        // Here, if kVL_mar_pointsto_value == vloc then the first byte of outer is at m[MAR + mar_offset].
        // We accumulate mar_offset at compile time as long as possible to save computing.
        outer_vartype = vartype;
        if (!_sym.IsStruct(outer_vartype))
        {
            Error("Expected a struct before '.' but did not find it");
            return kERR_UserError;
        }

        // Note: A DynArray can't be directly in front of a '.' (need a [...] first)
        if (_sym.IsDynpointer(vartype))
        {
            retval = AccessData_Dereference(vloc, mloc);
            if (retval < 0) return retval;
            vartype = _sym.VartypeWithout(kVTT_Dynpointer, vartype);
        }

        retval = AccessData_IsClauseLast(expression, clause_is_last);
        if (retval < 0) return retval;

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        retval = AccessData_SubsequentClause((clause_is_last && writing), access_via_this, static_access, expression, vloc, mloc, vartype);
        if (retval < 0) return retval;

        // Only the _immediate_ access via 'this.' counts for this flag.
        // This has passed now, so reset the flag.
        access_via_this = false;
        static_access = false; // Same for 'vartype.'
    }

    if (kVL_attribute == vloc)
    {
        // Caller will do the assignment
        // For this to work, the caller must know the type of the struct
        // in which the attribute resides
        vartype = _sym.VartypeWithout(
            kVTT_Const | kVTT_Dynarray | kVTT_Dynpointer,
            outer_vartype);
        return kERR_None;
    }

    if (kVL_ax_is_value == vloc)
    {
        _scrip.ax_vartype = vartype;
        _scrip.ax_scope_type = scope_type;
        return kERR_None;
    }

    mloc.MakeMARCurrent(_src.GetLineno(), _scrip);
    return kERR_None;
}

// In order to avoid push AX/pop AX, find out common cases that don't clobber AX
bool AGS::Parser::AccessData_MayAccessClobberAX(SrcList &expression)
{
    SymbolType const type_of_first = _sym.GetSymbolType(expression[0]);
    if (kSYM_GlobalVar != type_of_first && kSYM_LocalVar != type_of_first)
        return true;

    if (1 == expression.Length())
        return false;

    for (size_t idx = 0; idx < expression.Length() - 3; idx += 2)
    {
        if (kSYM_Dot != _sym.GetSymbolType(expression[idx + 1]))
            return true;
        Symbol const compo = MangleStructAndComponent(expression[0], expression[2]);
        if (kSYM_StructComponent != _sym.GetSymbolType(compo))
            return true;
    }
    return false;
}

// Insert Bytecode for:
// Copy at most OLDSTRING_LENGTH-1 bytes from m[MAR...] to m[AX...]
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
    WriteCmd(SCMD_JZ, 0);  // if (AX == 0) jumpto LOOP_END
    CodeLoc const jumpout1_pos = _scrip.codesize - 1;
    WriteCmd(SCMD_ADD, SREG_BX, 1); // BX++, CX++, DX--
    WriteCmd(SCMD_ADD, SREG_CX, 1);
    WriteCmd(SCMD_SUB, SREG_DX, 1);
    WriteCmd(SCMD_REGTOREG, SREG_DX, SREG_AX); // if (DX == 0) jumpto LOOP_END
    WriteCmd(SCMD_JZ, 0);
    CodeLoc const jumpout2_pos = _scrip.codesize - 1;
    WriteCmd(
        SCMD_JMP,
        ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, loop_start)); // jumpto LOOP_START
    CodeLoc const loop_end = _scrip.codesize; // Label LOOP_END
    _scrip.code[jumpout1_pos] = ccCompiledScript::RelativeJumpDist(jumpout1_pos, loop_end);
    _scrip.code[jumpout2_pos] = ccCompiledScript::RelativeJumpDist(jumpout2_pos, loop_end);
}

// We are typically in an assignment LHS = RHS; the RHS has already been
// evaluated, and the result of that evaluation is in AX.
// Store AX into the memory location that corresponds to LHS, or
// call the attribute function corresponding to LHS.
ErrorType AGS::Parser::AccessData_AssignTo(SrcList &expression)
{
    // We'll evaluate expression later on which moves the cursor,
    // so save it here and restore later on
    size_t const end_of_rhs_cursor = _src.GetCursor();

    // AX contains the result of evaluating the RHS of the assignment
    // Save on the stack so that it isn't clobbered
    Vartype rhsvartype = _scrip.ax_vartype;
    ScopeType rhs_scope_type = _scrip.ax_scope_type;
    // Save AX unless we are sure that it won't be clobbered
    bool const may_clobber = AccessData_MayAccessClobberAX(expression);
    if (may_clobber)
        _scrip.push_reg(SREG_AX);

    bool const writing = true;
    ValueLocation vloc;
    Vartype lhsvartype;
    ScopeType lhs_scope_type;
    ErrorType retval = AccessData(writing, expression, vloc, lhs_scope_type, lhsvartype);
    if (retval < 0) return retval;

    if (kVL_ax_is_value == vloc)
    {
        if (!_sym.IsManaged(lhsvartype))
        {
            Error("Cannot modify this value");
            return kERR_UserError;
        }
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        vloc = kVL_mar_pointsto_value;
    }

    if (may_clobber)
        _scrip.pop_reg(SREG_AX);
    _scrip.ax_vartype = rhsvartype;
    _scrip.ax_scope_type = rhs_scope_type;

    if (kVL_attribute == vloc)
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

    if (_sym.GetOldStringSym() == lhsvartype && _sym.GetOldStringSym() == _sym.VartypeWithout(kVTT_Const, rhsvartype))
    {
        // copy the string contents over.
        AccessData_StrCpy();
        _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
        return kERR_None;
    }

    ConvertAXStringToStringObject(lhsvartype);
    rhsvartype = _scrip.ax_vartype;
    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype))
    {
        Error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhsvartype).c_str(),
            _sym.GetName(lhsvartype).c_str());
        return kERR_UserError;
    }

    CodeCell const opcode =
        _sym.IsDyn(lhsvartype) ?
        SCMD_MEMWRITEPTR : GetWriteCommandForSize(_sym.GetSize(lhsvartype));
    WriteCmd(opcode, SREG_AX);
    _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
    return kERR_None;
}

ErrorType AGS::Parser::SkipToEndOfExpression()
{
    int nesting_depth = 0;

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
        SymbolType const peektype = _sym.GetSymbolType(peeksym);
        if (kSYM_OpenParenthesis == peektype || kSYM_OpenBracket == peektype || kSYM_OpenBrace == peektype)
            ++nesting_depth;
        else if (kSYM_CloseParenthesis == peektype || kSYM_CloseBracket == peektype || kSYM_CloseBrace == peektype)
            if (--nesting_depth < 0)
                break; // this symbol can't be part of the current expression
        if (nesting_depth > 0)
        {
            _src.GetNext();
            continue;
        }

        if (kSYM_Dot == peektype)
        {
            _src.GetNext(); // Eat '.'
            _src.GetNext(); // Eat following symbol
            continue;
        }

        if (kSYM_Tern == peektype)
        {
            tern_depth++;
            _src.GetNext(); // Eat '?'
            continue;
        }

        if (kSYM_Label == peektype)
        {
            // This is only allowed if it can be matched to an open tern
            if (--tern_depth < 0)
                break;

            _src.GetNext(); // Eat ':'
            continue; 
        }

        if (kSYM_New == peektype)
        {
            // This is only allowed if a type follows
            _src.GetNext(); // Eat 'new'
            Symbol const sym_after_new = _src.PeekNext();
            SymbolType const type_of_sym_after = _sym.GetSymbolType(sym_after_new);
            if (kSYM_Vartype == type_of_sym_after || kSYM_UndefinedStruct == type_of_sym_after)
            {
                _src.GetNext(); // Eat symbol after 'new'
                continue;
            }
            _src.BackUp(); // spit out 'new'
            break;
        }

        if (kSYM_Vartype == peektype)
        {
            // This is only allowed if a dot follows
            _src.GetNext(); // Eat the vartype
            Symbol const nextsym = _src.PeekNext();
            if (kSYM_Dot == _sym.GetSymbolType(nextsym))
            {
                _src.GetNext(); // Eat '.'
                continue;
            }
            _src.BackUp(); // spit out the vartype
            break;
        }

        // Apart from the exceptions above, all symbols starting at NOTEXPRESSION can't
        // be part of an expression
        if (peektype >= NOTEXPRESSION)
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
ErrorType AGS::Parser::ParseExpression()
{
    size_t const expr_start = _src.GetCursor();
    ErrorType retval = SkipToEndOfExpression();
    if (retval < 0) return retval;
    SrcList expression = SrcList(_src, expr_start, _src.GetCursor() - expr_start);
    if (0 == expression.Length())
    {
        Error("!Empty expression");
        return kERR_InternalError;
    }
    
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;

    retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;

    return ResultToAX(vloc, scope_type, vartype);
}

ErrorType AGS::Parser::AccessData_ReadBracketedIntExpression(SrcList &expression)
{
    Symbol const open_bracket = expression.GetNext();
    if (kSYM_OpenBracket != _sym.GetSymbolType(open_bracket))
    {
        Error("Expected '[', found '%s' instead", _sym.GetName(open_bracket).c_str());
        return kERR_UserError;
    }

    size_t start = expression.GetCursor();
    SymbolType const stoplist[] = { kSYM_NoType, };
    SkipTo(expression, stoplist, 0);
    SrcList in_brackets = SrcList(expression, start, expression.GetCursor() - start);

    ErrorType retval = AccessData_ReadIntExpression(in_brackets);
    if (retval < 0) return retval;

    if (!in_brackets.ReachedEOF())
    {
        Error("Expected ']', found '%s' instead", _sym.GetName(in_brackets.GetNext()).c_str());
        return kERR_UserError;
    }
    expression.GetNext(); // Eat ']'
    return kERR_None;
}

ErrorType AGS::Parser::ParseParenthesizedExpression()
{
    Symbol next_sym = _src.GetNext();
    if (_sym.GetSymbolType(next_sym) != kSYM_OpenParenthesis)
    {
        Error("Expected '(', found '%s' instead", _sym.GetName(next_sym));
        return kERR_UserError;
    }
    ErrorType retval = ParseExpression();
    if (retval < 0) return retval;

    next_sym = _src.GetNext();
    if (_sym.GetSymbolType(next_sym) != kSYM_CloseParenthesis)
    {
        Error("Expected ')', found '%s' instead", _sym.GetName(next_sym));
        return kERR_UserError;
    }
    return kERR_None;
}

// We are parsing the left hand side of a += or similar statement.
ErrorType AGS::Parser::ParseAssignment_ReadLHSForModification(SrcList &lhs, ValueLocation &vloc, AGS::Vartype &lhstype)
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

    if (kVL_mar_pointsto_value == vloc)
    {
        // write memory to AX
        _scrip.ax_vartype = lhstype;
        _scrip.ax_scope_type = scope_type;
        WriteCmd(
            GetReadCommandForSize(_sym.GetSize(lhstype)),
            SREG_AX);
    }
    return kERR_None;
}

// "var = expression"; lhs is the variable
ErrorType AGS::Parser::ParseAssignment_Assign(SrcList &lhs)
{
    ErrorType retval = ParseExpression(); // RHS of the assignment
    if (retval < 0) return retval;
    
    return AccessData_AssignTo(lhs);
}

// We compile something like "var += expression"
ErrorType AGS::Parser::ParseAssignment_MAssign(AGS::Symbol ass_symbol, SrcList &lhs)
{
    // Parse RHS
    ErrorType retval = ParseExpression();
    if (retval < 0) return retval;

    _scrip.push_reg(SREG_AX);
    Vartype rhsvartype = _scrip.ax_vartype;

    // Parse LHS (moves the cursor to end of LHS, so save it and restore it afterwards)
    ValueLocation vloc;
    Vartype lhsvartype;
    size_t const end_of_rhs_cursor = _src.GetCursor();
    retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype); 
    if (retval < 0) return retval;
    _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS

    // Use the operator on LHS and RHS
    CodeCell opcode = _sym.GetOperatorOpcode(ass_symbol);
    retval = GetOpcodeValidForVartype(lhsvartype, rhsvartype, opcode);
    if (retval < 0) return retval;
    _scrip.pop_reg(SREG_BX);
    WriteCmd(opcode, SREG_AX, SREG_BX);

    if (kVL_mar_pointsto_value == vloc)
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
ErrorType AGS::Parser::ParseAssignment_SAssign(AGS::Symbol ass_symbol, SrcList &lhs)
{
    ValueLocation vloc;
    Vartype lhsvartype;
    ErrorType retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype);
    if (retval < 0) return retval;

    // increment or decrement AX, using the correct opcode
    CodeCell opcode = _sym.GetOperatorOpcode(ass_symbol);
    retval = GetOpcodeValidForVartype(lhsvartype, lhsvartype, opcode);
    if (retval < 0) return retval;
    WriteCmd(opcode, SREG_AX, 1);

    if (kVL_mar_pointsto_value == vloc)
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

// We've read a variable or selector of a struct into symlist[], the last identifying component is in cursym.
// An assignment symbol is following. Compile the assignment.
ErrorType AGS::Parser::ParseAssignment(AGS::Symbol ass_symbol, SrcList &lhs)
{
    switch (_sym.GetSymbolType(ass_symbol))
    {
    default: // can't happen
        Error("!Illegal assignment symbol found");
        return kERR_InternalError;

    case kSYM_Assign:
        return ParseAssignment_Assign(lhs);

    case kSYM_AssignMod:
        return ParseAssignment_MAssign(ass_symbol, lhs);

    case kSYM_AssignSOp:
        return ParseAssignment_SAssign(ass_symbol, lhs);
    }
}

ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_Float(bool is_neg, void *& initial_val_ptr)
{
    // initialize float
    if (_sym.GetSymbolType(_src.PeekNext()) != kSYM_LiteralFloat)
    {
        Error("Expected floating point value after '='");
        return kERR_UserError;
    }

    float float_init_val = static_cast<float>(atof(_sym.GetName(_src.GetNext()).c_str()));
    if (is_neg)
        float_init_val = -float_init_val;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        Error("Out of memory");
        return kERR_UserError;
    }

    // Interpret the float as an int; move that into the allocated space
    (static_cast<long *>(initial_val_ptr))[0] = InterpretFloatAsInt(float_init_val);

    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_OldString(void *&initial_val_ptr)
{
    Symbol literal_sym = _src.GetNext();
    if (kSYM_LiteralString != _sym.GetSymbolType(literal_sym))
    {
        Error("Expected a literal string");
        return kERR_UserError;
    }
    std::string literal = _sym.GetName(literal_sym);
    if (literal.length() >= STRINGBUFFER_LENGTH)
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
    std::strncpy(
        static_cast<char *>(initial_val_ptr), literal.c_str(),
        STRINGBUFFER_LENGTH);
    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_InitialValAssignment_Inttype(bool is_neg, void *&initial_val_ptr)
{
    // Initializer for an integer value
    int int_init_val;
    ErrorType retval = IntLiteralOrConst2Value(_src.GetNext(), is_neg, "Expected integer value after '='", int_init_val);
    if (retval < 0) return retval;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        Error("Out of memory");
        return kERR_UserError;
    }
    // Convert int to long; move that into the allocated space
    (reinterpret_cast<long *>(initial_val_ptr))[0] = int_init_val;

    return kERR_None;
}

// if initial_value is non-null, it returns malloc'd memory that must be free
ErrorType AGS::Parser::ParseVardecl_InitialValAssignment(AGS::Symbol varname, void *&initial_val_ptr)
{
    initial_val_ptr = nullptr;
    _src.GetNext(); // Eat '='

    if (_sym.IsManaged(varname))
    {
        Error("Cannot assign an initial value to a managed type or String");
        return kERR_UserError;
    }

    if (_sym.IsStruct(varname))
    {
        Error("Cannot initialize struct type");
        return kERR_UserError;
    }

    if (_sym.GetOldStringSym() == _sym.GetVartype(varname))
        return ParseVardecl_InitialValAssignment_OldString(initial_val_ptr);

    // accept leading '-' if present
    bool is_neg = false;
    if (_src.PeekNext() == _sym.Find("-"))
    {
        is_neg = true;
        _src.GetNext();
    }

    // Do actual assignment
    if (_sym.GetVartype(varname) == _sym.GetFloatSym())
        return ParseVardecl_InitialValAssignment_Float(is_neg, initial_val_ptr);
    return ParseVardecl_InitialValAssignment_Inttype(is_neg, initial_val_ptr);
}

// Move variable information into the symbol table
void AGS::Parser::ParseVardecl_Var2SymTable(Symbol var_name, AGS::Vartype vartype, ScopeType scope_type)
{
    SymbolTableEntry &entry = _sym[var_name];
    entry.SType = (scope_type == kScT_Local) ? kSYM_LocalVar : kSYM_GlobalVar;
    entry.Vartype = vartype;
    _sym.SetDeclared(var_name, _src.GetSectionId(), _src.GetLineno());
}

ErrorType AGS::Parser::ParseVardecl_CheckIllegalCombis(AGS::Vartype vartype, ScopeType scope_type)
{
    if (vartype == _sym.GetOldStringSym() && ccGetOption(SCOPT_OLDSTRINGS) == 0)
    {
        Error("Type 'string' is no longer supported; use String instead");
        return kERR_UserError;
    }

    if (vartype == _sym.GetOldStringSym() && kScT_Import == scope_type)
    {
        // cannot import because string is really char *, and the pointer won't resolve properly
        Error("Cannot import string; use char[] instead");
        return kERR_UserError;
    }

    if (vartype == _sym.GetVoidSym())
    {
        Error("'void' not a valid variable type");
        return kERR_UserError;
    }

    return kERR_None;
}

// there was a forward declaration -- check that the real declaration matches it
ErrorType AGS::Parser::ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info, bool body_follows)
{
    if (0 == known_info->SType)
        return kERR_None; // We don't have any known info

    if (known_info->SType != this_entry->SType)
    {
        Error(ReferenceMsg(
            "This variable is declared as %s elsewhere",
            known_info->DeclSectionId,
            known_info->DeclLine).c_str(),
            (kSYM_Function == known_info->SType)  ? "function" :
            (kSYM_GlobalVar == known_info->SType) ? "global variable" :
            (kSYM_LocalVar == known_info->SType)  ? "local variable" : "another entity");
        return kERR_UserError;
    }

    if ((known_info->TypeQualifiers & ~kTQ_Import) != (this_entry->TypeQualifiers & ~kTQ_Import))
    {
        std::string const ki_tq = TypeQualifierSet2String(known_info->TypeQualifiers & ~kTQ_Import);
        std::string const te_tq = TypeQualifierSet2String(this_entry->TypeQualifiers & ~kTQ_Import);
        std::string msg = ReferenceMsg(
            "The variable '%s' has the qualifiers '%s' here, but '%s' elsewhere",
            known_info->DeclSectionId,
            known_info->DeclLine);
        Error(msg.c_str(), te_tq.c_str(), ki_tq.c_str());
        return kERR_UserError;
    }

    if (known_info->Vartype != this_entry->Vartype)
    {
        // This will check the array lengths, too
        std::string msg = ReferenceMsg(
            "This variable is declared as %s here, as %s elsewhere",
            known_info->DeclSectionId,
            known_info->DeclLine);
        Error(
            msg.c_str(),
            _sym.GetName(this_entry->Vartype).c_str(),
            _sym.GetName(known_info->Vartype).c_str());
        return kERR_UserError;
    }

    if (known_info->GetSize(_sym) != this_entry->GetSize(_sym))
    {
        std::string msg = ReferenceMsg(
            "Size of this variable is %d here, %d declared elsewhere",
            known_info->DeclSectionId,
            known_info->DeclLine);
        Error(
            msg.c_str(),
            this_entry->GetSize(_sym), known_info->GetSize(_sym));
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_GlobalImport(AGS::Symbol var_name, bool has_initial_assignment)
{
    if (has_initial_assignment)
    {
        Error("Imported variables cannot have any initial assignment");
        return kERR_UserError;
    }

    if (_givm[var_name])
        return kERR_None; // Skip this since the global non-import decl will come later

    SetFlag(_sym[var_name].TypeQualifiers, kTQ_Import, true);
    _sym[var_name].SOffset = _scrip.add_new_import(_sym.GetName(var_name).c_str());
    if (_sym[var_name].SOffset == -1)
    {
        Error("!Import table overflow");
        return kERR_InternalError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_GlobalNoImport(AGS::Symbol var_name, AGS::Vartype vartype, bool has_initial_assignment, void *&initial_val_ptr)
{
    if (has_initial_assignment)
    {
        ErrorType retval = ParseVardecl_InitialValAssignment(var_name, initial_val_ptr);
        if (retval < 0) return retval;
    }
    SymbolTableEntry &entry = _sym[var_name];
    entry.Vartype = vartype;
    size_t const var_size = _sym.GetSize(vartype);
    entry.SOffset = _scrip.add_global(var_size, initial_val_ptr);
    if (entry.SOffset < 0)
    {
        Error("!Cannot allocate global variable");
        return kERR_InternalError;
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl_Local(AGS::Symbol var_name, AGS::Vartype vartype, bool has_initial_assignment)
{
    size_t const var_size = _sym.GetSize(vartype);
    bool const is_dyn = _sym.IsDyn(vartype);

    _sym[var_name].SOffset = _scrip.offset_to_local_var_block;

    if (!has_initial_assignment)
    {
        // Initialize the variable with binary zeroes.
        WriteCmd(SCMD_LOADSPOFFS, 0);
        if (is_dyn)
            WriteCmd(SCMD_MEMZEROPTR);
        else
            WriteCmd(SCMD_ZEROMEMORY, var_size);
        WriteCmd(SCMD_ADD, SREG_SP, var_size);
        _scrip.offset_to_local_var_block += var_size;
        return kERR_None;
    }

    // "readonly" vars can't be assigned to, so don't use standard assignment function here.
    _src.GetNext(); // Eat '='
    ErrorType retval = ParseExpression();
    if (retval < 0) return retval;

    // Vartypes must match. This is true even if the lhs is readonly.
    Vartype const lhsvartype = vartype;
    Vartype const rhsvartype = _scrip.ax_vartype;

    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype))
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
        _scrip.push_reg(SREG_AX);
        return kERR_None;
    }

    ConvertAXStringToStringObject(vartype);
    WriteCmd(SCMD_LOADSPOFFS, 0);
    WriteCmd(
        is_dyn ? SCMD_MEMWRITEPTR : GetWriteCommandForSize(var_size),
        SREG_AX);
    WriteCmd(SCMD_ADD, SREG_SP, var_size);
    _scrip.offset_to_local_var_block += var_size;
    return kERR_None;
}

ErrorType AGS::Parser::ParseVardecl0(AGS::Symbol var_name, AGS::Vartype vartype, ScopeType scope_type)
{
    SymbolType next_type = _sym.GetSymbolType(_src.PeekNext());
    if (kSYM_OpenBracket == next_type)
    {
        ErrorType retval = ParseArray(var_name, vartype);
        if (retval < 0) return retval;
        next_type = _sym.GetSymbolType(_src.PeekNext());
    }

    // Enter the variable into the symbol table
    ParseVardecl_Var2SymTable(var_name, vartype, scope_type);

    bool const has_initial_assignment = (kSYM_Assign == next_type);

    switch (scope_type)
    {
    default:
        Error("!Wrong scope type");
        return kERR_InternalError;

    case kScT_Global:
    {
        void *initial_val_ptr = nullptr;
        ErrorType retval = ParseVardecl_GlobalNoImport(var_name, vartype, has_initial_assignment, initial_val_ptr);
        if (initial_val_ptr) free(initial_val_ptr);
        return retval;
    }

    case kScT_Import:
        return ParseVardecl_GlobalImport(var_name, has_initial_assignment);

    case kScT_Local:
        return ParseVardecl_Local(var_name, vartype, has_initial_assignment);
    }
}

// wrapper around ParseVardecl0() 
ErrorType AGS::Parser::ParseVardecl(AGS::Symbol var_name, AGS::Vartype vartype, ScopeType scope_type)
{
    ErrorType retval = ParseVardecl_CheckIllegalCombis(vartype, scope_type);
    if (retval < 0) return retval;

    if (kScT_Local == scope_type && _sym[var_name].SType != 0)
    {
        std::string msg = ReferenceMsgSym(
            "Variable %s has already been declared", var_name);
        Error(msg.c_str(), _sym.GetName(var_name).c_str());
        return kERR_UserError;
    }

    SymbolTableEntry known_info;
    retval = CopyKnownSymInfo(_sym[var_name], known_info);
    if (retval < 0) return retval;

    retval = ParseVardecl0(var_name, vartype, scope_type);
    if (retval < 0) return retval;

    return ParseVardecl_CheckThatKnownInfoMatches(&_sym[var_name], &known_info, false);
}

ErrorType AGS::Parser::ParseFuncBody(AGS::Parser::NestingStack *nesting_stack, Symbol struct_of_func, AGS::Symbol name_of_func)
{
    nesting_stack->Push(kSYM_Function);

    // write base address of function for any relocation needed later
    WriteCmd(SCMD_THISBASE, _scrip.codesize);
    SymbolTableEntry &entry = _sym[name_of_func];
    if (FlagIsSet(entry.Flags, kSFLG_NoLoopCheck))
    {
        WriteCmd(SCMD_LOOPCHECKOFF);
        SetFlag(entry.Flags, kSFLG_NoLoopCheck, false);
    }

    // If there are dynpointer parameters, then the caller has simply "pushed" them onto the stack.
    // We catch up here by reading each dynpointer and writing it again using MEMWRITEPTR
    // to declare that the respective cells are used for dynpointers.
    size_t const num_params = _sym[name_of_func].GetNumOfFuncParams();
    for (size_t param_idx = 1; param_idx <= num_params; param_idx++) // skip return value pa == 0
    {
        Vartype const param_vartype = _sym[name_of_func].FuncParamVartypes[param_idx];
        if (!_sym.IsManaged(param_vartype))
            continue;

        // The return address is on top of the stack, so the nth param is at (n+1)th position
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_STACK_CELL * (param_idx + 1));
        WriteCmd(SCMD_MEMREAD, SREG_AX); // Read the address that is stored there
        // Create a dynpointer that points to the same object as m[AX] and store it in m[MAR]
        WriteCmd(SCMD_MEMWRITEPTR, SREG_AX);
    }

    SymbolTableEntry &this_entry = _sym[_sym.GetThisSym()];
    this_entry.Vartype = 0;
    if (struct_of_func > 0 && !FlagIsSet(_sym[name_of_func].TypeQualifiers, kTQ_Static))
    {
        // Declare "this" but do not allocate memory for it
        this_entry.SType = kSYM_LocalVar;
        this_entry.Vartype = struct_of_func; // Don't declare this as dynpointer
        this_entry.SScope = 0;
        this_entry.TypeQualifiers = kTQ_Readonly;
        this_entry.Flags = kSFLG_Accessed | kSFLG_StructVartype;
        this_entry.SOffset = 0;
    }
    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfFuncBody(NestingStack *nesting_stack, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    // Pop the local variables proper from the stack but leave the parameters.
    // This is important because the return address is directly above the parameters;
    // we need the return address to return. (The caller will pop the parameters later.)
    ExitNesting(SymbolTableEntry::ParameterSScope);
    // All the function variables, _including_ the parameters (!), become invalid.
    RemoveLocalsFromSymtable(0);

    // Function has ended. Set AX to 0 unless the function doesn't return any value.
    if (_sym.GetVoidSym() != _sym[name_of_current_func].FuncParamVartypes.at(0))
        WriteCmd(SCMD_LITTOREG, SREG_AX, 0);

    // We've just finished the body of the current function.
    name_of_current_func = -1;
    struct_of_current_func = -1;

    nesting_stack->JumpOut().Patch(_src.GetLineno());
    nesting_stack->Pop();

    WriteCmd(SCMD_RET);
    // This has popped the return address from the stack, 
    // so adjust the offset to the start of the parameters.
    _scrip.offset_to_local_var_block -= SIZE_OF_STACK_CELL;

    return kERR_None;
}

void AGS::Parser::ParseStruct_SetTypeInSymboltable(AGS::Symbol stname, TypeQualifierSet tqs)
{
    SymbolTableEntry &entry = _sym[stname];

    entry.Extends = 0;
    entry.SType = kSYM_Vartype;
    entry.SSize = 0;

    SetFlag(entry.Flags, kSFLG_StructVartype, true);
    if (FlagIsSet(tqs, kTQ_Managed))
        SetFlag(entry.Flags, kSFLG_StructManaged, true);
    if (FlagIsSet(tqs, kTQ_Builtin))
        SetFlag(entry.Flags, kSFLG_StructBuiltin, true);
    if (FlagIsSet(tqs, kTQ_Autoptr))
        SetFlag(entry.Flags, kSFLG_StructAutoPtr, true);

    _sym.SetDeclared(stname, _src.GetSectionId(), _src.GetLineno());
}

// We have accepted something like "struct foo" and are waiting for "extends"
ErrorType AGS::Parser::ParseStruct_ExtendsClause(AGS::Symbol stname, size_t &size_so_far)
{
    _src.GetNext(); // Eat "extends"
    Symbol const parent = _src.GetNext(); // name of the extended struct

    if (kPP_PreAnalyze == _pp)
        return kERR_None; // No further analysis necessary in first phase

    if (kSYM_Vartype != _sym.GetSymbolType(parent))
    {
        Error("Expected a struct type here");
        return kERR_UserError;
    }
    if (!_sym.IsStruct(parent))
    {
        Error("Must extend a struct type");
        return kERR_UserError;
    }
    if (!_sym.IsManaged(parent) && _sym.IsManaged(stname))
    {
        Error("Managed struct cannot extend the unmanaged struct '%s'", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    if (_sym.IsManaged(parent) && !_sym.IsManaged(stname))
    {
        Error("Unmanaged struct cannot extend the managed struct '%s'", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    if (_sym.IsBuiltin(parent) && !_sym.IsBuiltin(stname))
    {
        Error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", _sym.GetName(parent).c_str());
        return kERR_UserError;
    }
    size_so_far = _sym.GetSize(parent);
    _sym[stname].Extends = parent;
    return kERR_None;
}

// Check whether the qualifiers that accumulated for this decl go together
ErrorType AGS::Parser::Parse_CheckTQ(TypeQualifierSet tqs, bool in_func_body, bool in_struct_decl, AGS::Symbol decl_type)
{
    if (in_struct_decl)
    {
        TypeQualifier error_tq = kTQ_None;
        if (FlagIsSet(tqs, (error_tq = kTQ_Builtin)) ||
            FlagIsSet(tqs, (error_tq = kTQ_ImportTry)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Stringstruct)))
        {
            Error("'%s' is illegal in a struct declaration", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }
    else // !in_struct_decl
    {
        TypeQualifier error_tq = kTQ_None;
        if (FlagIsSet(tqs, (error_tq = kTQ_Attribute)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Protected)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Writeprotected)))
        {
            Error("'%s' is only legal in a struct declaration", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }

    if (in_func_body)
    {
        TypeQualifier error_tq = kTQ_None;
        if (FlagIsSet(tqs, (error_tq = kTQ_Autoptr)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Builtin)) ||
            FlagIsSet(tqs, (error_tq = kTQ_ImportStd)) ||
            FlagIsSet(tqs, (error_tq = kTQ_ImportTry)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Managed)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Static)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Stringstruct)))
        {
            Error("'%s' is illegal in a function body", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }

    // Keywords that never go together
    if (1 < FlagIsSet(tqs, kTQ_Protected) + FlagIsSet(tqs, kTQ_Writeprotected) + FlagIsSet(tqs, kTQ_Readonly))
    {
        Error("Can only use one out of 'protected', 'readonly', and 'writeprotected'");
        return kERR_UserError;
    }

    if (1 < FlagIsSet(tqs, kTQ_ImportStd) + FlagIsSet(tqs, kTQ_ImportTry))
    {
        Error("Cannot combine 'import' and '_tryimport'");
        return kERR_UserError;
    }

    // Will fire for all commands
    if (kSYM_OpenBrace == decl_type && 0 != tqs)
    {
        TypeQualifier error_tq = kTQ_None;
        if (FlagIsSet(tqs, (error_tq = kTQ_Autoptr)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Attribute)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Builtin)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Const)) ||
            FlagIsSet(tqs, (error_tq = kTQ_ImportStd)) ||
            FlagIsSet(tqs, (error_tq = kTQ_ImportTry)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Managed)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Protected)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Readonly)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Static)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Stringstruct)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Writeprotected)))
        {
            Error("Unexpected '%s' before a command", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }


    if (FlagIsSet(tqs, kTQ_Autoptr))
    {
        if (!FlagIsSet(tqs, kTQ_Builtin) || !FlagIsSet(tqs, kTQ_Managed))
        {
            Error("'autoptr' must be combined with 'builtin' and 'managed'");
            return kERR_UserError;
        }
    }

    if (FlagIsSet(tqs, kTQ_Builtin) && kSYM_Struct != decl_type)
    {
        Error("'builtin' can only be used in a struct definition");
        return kERR_UserError;
    }

    if (FlagIsSet(tqs, kTQ_Const))
    {
        Error("'const' can only be used for a function parameter (use 'readonly' instead)");
        return kERR_UserError;
    }

    if (kSYM_Export == decl_type && 0 != tqs)
    {
        TypeQualifier error_tq = kTQ_None;
        if (FlagIsSet(tqs, (error_tq = kTQ_Attribute)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Autoptr)) ||
            FlagIsSet(tqs, (error_tq = kTQ_ImportStd)) ||
            FlagIsSet(tqs, (error_tq = kTQ_ImportTry)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Managed)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Static)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Stringstruct)) ||
            FlagIsSet(tqs, (error_tq = kTQ_Writeprotected)))
        {
            Error("Cannot combine 'export' and '%s'", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }

    if (FlagIsSet(tqs, kTQ_Import))
    {
        TypeQualifier error_tq;
        if (FlagIsSet(tqs, (error_tq = kTQ_Stringstruct)))
        {
            Error("Cannot combine 'import' and '%s'", _tq2String[error_tq].c_str());
            return kERR_UserError;
        }
    }

    if (FlagIsSet(tqs, kTQ_Readonly) && kSYM_Vartype != decl_type)
    {
        Error("'readonly' can only be used in a variable declaration");
        return kERR_UserError;
    }

    if (FlagIsSet(tqs, kTQ_Static) && kSYM_Vartype != decl_type)
    {
        Error("'static' cannot be used in a variable declaration");
        return kERR_UserError;
    }

    if (FlagIsSet(tqs, kTQ_Stringstruct) && (!FlagIsSet(tqs, kTQ_Autoptr)))
    {
        Error("'stringstruct' must be combined with 'autoptr'");
        return kERR_UserError;
    }

    return kERR_None;
}

void AGS::Parser::ParseQualifiers(TypeQualifierSet &tqs)
{
    tqs = 0;
    while (!_src.ReachedEOF())
    {
        Symbol peeksym = _src.PeekNext();
        switch (_sym.GetSymbolType(peeksym))
        {
        default: return;
        case kSYM_Attribute:      SetFlag(tqs, kTQ_Attribute, true); break;
        case kSYM_AutoPtr:        SetFlag(tqs, kTQ_Autoptr, true); break;
        case kSYM_Builtin:        SetFlag(tqs, kTQ_Builtin, true); break;
        case kSYM_Const:          SetFlag(tqs, kTQ_Const, true); break;
        // for kSYM_Import, see below
        case kSYM_InternalString: SetFlag(tqs, kTQ_Stringstruct, true); break;
        case kSYM_Managed:        SetFlag(tqs, kTQ_Managed, true); break;
        case kSYM_Protected:      SetFlag(tqs, kTQ_Protected, true); break;
        case kSYM_ReadOnly:       SetFlag(tqs, kTQ_Readonly, true); break;
        case kSYM_Static:         SetFlag(tqs, kTQ_Static, true); break;
        case kSYM_WriteProtected: SetFlag(tqs, kTQ_Writeprotected, true); break;

        // Special case: Same symbol but different flag depending on how it is written
        case kSYM_Import:
                SetFlag(
                    tqs,
                    (0 ==_sym.GetName(peeksym).compare("_tryimport"))?
                        kTQ_ImportTry : kTQ_ImportStd,
                    true);
                break;
        } // switch (_sym.GetSymbolType(peeksym))

        _src.GetNext();
    };

    return;
}

ErrorType AGS::Parser::ParseStruct_CheckComponentVartype(Symbol stname, AGS::Vartype vartype, bool member_is_import)
{
    if (Vartype2Symbol(vartype) == stname && !_sym.IsManaged(vartype))
    {
        // cannot do "struct A { A varname; }", this struct would be infinitely large
        Error("Struct '%s' cannot be a member of itself", _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    SymbolType const vartype_type = _sym.GetSymbolType(vartype);
    if (vartype_type == kSYM_NoType)
    {
        Error(
            "Type '%s' is undefined",
            _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }
    if (kSYM_Vartype != vartype_type && kSYM_UndefinedStruct != vartype_type)
    {
        std::string msg = ReferenceMsgSym(
            "'%s' should be a typename but is already in use differently",
            Vartype2Symbol(vartype));
        Error(
            msg.c_str(),
            _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    if (vartype == _sym.GetOldStringSym()) // [fw] Where's the problem?
    {
        Error("'string' not allowed inside a struct");
        return kERR_UserError;
    }
    return kERR_None;
}

// check that we haven't extended a struct that already contains a member with the same name
ErrorType AGS::Parser::ParseStruct_CheckForCompoInAncester(AGS::Symbol orig, AGS::Symbol compo, AGS::Symbol current_struct)
{
    if (current_struct <= 0)
        return kERR_None;
    Symbol const member = MangleStructAndComponent(current_struct, compo);
    if (kSYM_NoType != _sym.GetSymbolType(member))
    {
        std::string msg = ReferenceMsgSym(
            "The struct '%s' extends '%s', and '%s' is already defined",
            member);
        Error(
            msg.c_str(),
            _sym.GetName(orig).c_str(),
            _sym.GetName(current_struct).c_str(),
            _sym.GetName(member).c_str());
        return kERR_UserError;
    }

    return ParseStruct_CheckForCompoInAncester(orig, compo, _sym[current_struct].Extends);
}

ErrorType AGS::Parser::ParseStruct_FuncDecl(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, Vartype vartype)
{
    if (FlagIsSet(tqs, kTQ_Writeprotected))
    {
        Error("'writeprotected' does not apply to functions");
        return kERR_UserError;
    }

    size_t const declaration_start = _src.GetCursor();
    _src.GetNext(); // Eat '('

    SetFlag(_sym[name_of_func].Flags, kSFLG_StructMember, true);
    _sym[name_of_func].Extends = struct_of_func;

    bool body_follows;
    ErrorType retval = ParseFuncdecl(declaration_start, tqs, vartype, struct_of_func, name_of_func, false, body_follows);
    if (retval < 0) return retval;
    if (body_follows)
    {
        Error("Cannot code a function body within a struct definition");
        return kERR_UserError;
    }
    if (kSYM_Semicolon != _sym.GetSymbolType(_src.PeekNext()))
    {
        Error("Expected ';'");
        return kERR_UserError;
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, AGS::Vartype vartype)
{
    SymbolTableEntry &entry = _sym[name_of_func];
    size_t const num_parameters_wanted = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);
    if (num_parameters_wanted != entry.GetNumOfFuncParams())
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' should have %d parameter(s) but is declared with %d parameter(s) instead",
            name_of_func);
        Error(msg.c_str(), entry.SName.c_str(), num_parameters_wanted, entry.GetNumOfFuncParams());
        return kERR_UserError;
    }

    Vartype const ret_vartype = is_setter ? _sym.GetVoidSym() : vartype;
    if (entry.FuncParamVartypes[0] != ret_vartype)
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' must return type '%s' but returns '%s' instead",
            name_of_func);
        Error(msg.c_str(), entry.SName.c_str(),
            _sym.GetName(ret_vartype).c_str(),
            _sym.GetName(entry.FuncParamVartypes[0]).c_str());
        return kERR_UserError;
    }

    size_t p_idx = 1;
    if (is_indexed)
    {
        if (entry.FuncParamVartypes[p_idx] != _sym.GetIntSym())
        {
            std::string const msg = ReferenceMsgSym(
                "Parameter #%d of attribute function '%s' must have type integer but doesn't.",
                name_of_func);
            Error(msg.c_str(), p_idx, entry.SName.c_str());
            return kERR_UserError;
        }
        p_idx++;
    }

    if (is_setter && entry.FuncParamVartypes[p_idx] != vartype)
    {
        std::string const msg = ReferenceMsgSym(
            "Parameter #d of attribute function '%s' must have type '%s'",
            name_of_func);
        Error(msg.c_str(), p_idx, entry.SName.c_str(), _sym.GetName(vartype).c_str());
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseStruct_Attribute_ParamList(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    SymbolTableEntry &entry = _sym[name_of_func];
    size_t const num_param = is_indexed + is_setter;

    entry.FuncParamVartypes.resize(num_param + 1);

    size_t p_idx = 1;
    if (is_indexed)
        entry.FuncParamVartypes[p_idx++] = _sym.GetIntSym();
    if (is_setter)
        entry.FuncParamVartypes[p_idx] = vartype;
    SymbolTableEntry::ParamDefault deflt = {};
    entry.FuncParamDefaultValues.assign(entry.FuncParamVartypes.size(), deflt);
    return kERR_None;
}

// We are processing an attribute.
// This corresponds to a getter func and a setter func, declare one of them
ErrorType AGS::Parser::ParseStruct_Attribute_DeclareFunc(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    // If this symbol has been defined before, check whether the definitions clash
    SymbolType const stype = _sym[name_of_func].SType;
    if (kSYM_Function != stype && kSYM_NoType != stype)
    {
        std::string msg = ReferenceMsgSym(
            "Attribute uses '%s' as a function, this clashes with a declaration elsewhere",
            name_of_func);
        Error(msg.c_str(), _sym[name_of_func].SName.c_str());
        return kERR_UserError;
    }
    if (kSYM_Function == stype)
    {
        ErrorType retval = ParseStruct_Attribute_CheckFunc(name_of_func, is_setter, is_indexed, vartype);
        if (retval < 0) return retval;
    }

    SetFlag(tqs, kTQ_Import, true); // Assume that attribute functions are imported
    if (FlagIsSet(tqs, kTQ_Import) &&
        kSYM_Function == _sym.GetSymbolType(name_of_func) &&
        !FlagIsSet(_sym[name_of_func].TypeQualifiers, kTQ_Import))
    {
        if (0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            std::string const msg = ReferenceMsgSym(
                "In here, attribute functions may not be defined locally",
                name_of_func);
            Error(msg.c_str());
            return kERR_UserError;
        }
        SetFlag(tqs, kTQ_Import, false);
    }

    // Store the fact that this function has been declared within the struct declaration
    _sym[name_of_func].Extends = struct_of_func;
    SetFlag(_sym[name_of_func].Flags, kSFLG_StructMember, true);

    Vartype const return_vartype = is_setter ? _sym.GetVoidSym() : vartype;
    ParseFuncdecl_MasterData2Sym(tqs & ~kTQ_Attribute, return_vartype, struct_of_func, name_of_func, false);

    ErrorType retval = ParseStruct_Attribute_ParamList(struct_of_func, name_of_func, is_setter, is_indexed, vartype);
    if (retval < 0) return retval;

    // When the function is defined, it won't have "attribute" set so don't set "attribute" here
   

    bool const body_follows = false; // we are within a struct definition
    return ParseFuncdecl_HandleFunctionOrImportIndex(tqs, struct_of_func, name_of_func, body_follows);
}

// We're in a struct declaration, parsing a struct attribute
ErrorType AGS::Parser::ParseStruct_Attribute(TypeQualifierSet tqs, Symbol stname, Symbol vname, Vartype vartype)
{
    size_t const declaration_start = _src.GetCursor();
    // "readonly" means that there isn't a setter function. The individual vartypes are not readonly.
    bool const attrib_is_readonly = FlagIsSet(tqs, kTQ_Readonly);
    SetFlag(tqs, kTQ_Readonly, false);

    bool attrib_is_indexed = false;

    if (kSYM_OpenBracket == _sym.GetSymbolType(_src.PeekNext()))
    {
        attrib_is_indexed = true;
        _src.GetNext();
        if (kSYM_CloseBracket != _sym.GetSymbolType(_src.GetNext()))
        {
            Error("Cannot specify array size for attribute");
            return kERR_UserError;
        }
    }

    _sym[vname].SType = kSYM_Attribute;
    if (attrib_is_indexed)
        _sym[vname].Vartype = _sym.VartypeWith(kVTT_Dynarray, _sym[vname].Vartype);

    // Declare attribute getter, e.g. get_ATTRIB()
    Symbol attrib_func = -1;
    bool const get_func_is_setter = false;
    ErrorType retval = ConstructAttributeFuncName(vname, get_func_is_setter, attrib_is_indexed, attrib_func);
    if (retval < 0) return retval;
    Symbol const get_func_name = MangleStructAndComponent(stname, attrib_func);
    retval = ParseStruct_Attribute_DeclareFunc(tqs, stname, get_func_name, get_func_is_setter, attrib_is_indexed, vartype);
    if (retval < 0) return retval;
    _sym.SetDeclared(
        get_func_name,
        _src.GetSectionIdAt(declaration_start),
        _src.GetLinenoAt(declaration_start));

    if (attrib_is_readonly)
        return kERR_None;

    // Declare attribute setter, e.g. set_ATTRIB(value)
    bool const set_func_is_setter = true;
    retval = ConstructAttributeFuncName(vname, set_func_is_setter, attrib_is_indexed, attrib_func);
    if (retval < 0) return retval;
    Symbol const set_func_name = MangleStructAndComponent(stname, attrib_func);
    retval = ParseStruct_Attribute_DeclareFunc(tqs, stname, set_func_name, set_func_is_setter, attrib_is_indexed, vartype);
    if (retval < 0) return retval;
    _sym.SetDeclared(
        set_func_name,
        _src.GetSectionIdAt(declaration_start),
        _src.GetLinenoAt(declaration_start));

    return kERR_None;
}

// We're parsing an array var.
ErrorType AGS::Parser::ParseArray(AGS::Symbol vname, AGS::Vartype &vartype)
{
    _src.GetNext(); // Eat '['

    if (kPP_PreAnalyze == _pp)
    {
        // Skip the sequence of [...]
        while (true)
        {
            const SymbolType stoplist[] = { kSYM_NoType, };
            SkipTo(_src, stoplist, 0);
            _src.GetNext(); // Eat ']'
            if (kSYM_OpenBracket != _src.PeekNext())
                return kERR_None;
            _src.GetNext(); // Eat '['
        }
    }

    if (kSYM_CloseBracket == _sym.GetSymbolType(_src.PeekNext()))
    {
        // Dynamic array
        _src.GetNext(); // Eat ']'
        if (vartype == _sym.GetOldStringSym())
        {
            Error("Dynamic arrays of old-style strings are not supported");
            return kERR_UserError;
        }
        if (!_sym.IsAnyIntType(vartype) && !_sym.IsManaged(vartype) && _sym.GetFloatSym() != vartype)
        {
            Error("Can only have dynamic arrays of integer types, float or managed structs. '%s' isn't any of this.", _sym.GetName(vartype).c_str());
            return kERR_UserError;
        }
        vartype = _sym.VartypeWith(kVTT_Dynarray, vartype);
        return kERR_None;
    }

    std::vector<size_t> dims;

    // Static array
    while (true)
    {
        Symbol const dim_symbol = _src.GetNext();

        int dimension_as_int;
        ErrorType retval = IntLiteralOrConst2Value(dim_symbol, false, "Expected a constant integer value for array dimension", dimension_as_int);
        if (retval < 0) return retval;

        if (dimension_as_int < 1)
        {
            Error("Array dimension must be at least 1, found %d instead", dimension_as_int);
            return kERR_UserError;
        }

        dims.push_back(dimension_as_int);

        SymbolType const next_symtype = _sym.GetSymbolType(_src.GetNext());
        if (kSYM_Comma == next_symtype)
            continue;
        if (kSYM_CloseBracket != next_symtype)
        {
            Error("Expected ']' or ',' after array dimension");
            return kERR_UserError;
        }
        SymbolType const peek_symtype = _sym.GetSymbolType(_src.PeekNext());
        if (kSYM_OpenBracket != peek_symtype)
            break;
        _src.GetNext(); // Eat '['
    }
    vartype = _sym.VartypeWithArray(dims, vartype);
    return kERR_None;
}

// We're inside a struct decl, processing a member variable or a member attribute
ErrorType AGS::Parser::ParseStruct_VariableOrAttributeDefn(TypeQualifierSet tqs, Vartype vartype, Symbol stname, Symbol vname, size_t &size_so_far)
{
    if (kPP_Main == _pp)
    {
        if (_sym.IsBuiltin(vartype) && !_sym.IsDyn(vartype))
        {
            Error("'%s' is a builtin non-managed struct; struct members of that type are not supported",
                _sym.GetName(vartype).c_str());
            return kERR_UserError;
        }

        if (FlagIsSet(tqs, kTQ_Import) && !FlagIsSet(tqs, kTQ_Attribute))
        {
            // member variable cannot be an import
            Error("Can't import struct component variables; import the whole struct instead");
            return kERR_UserError;
        }

        SymbolTableEntry &entry = _sym[vname];
        entry.SType = kSYM_StructComponent;
        entry.Extends = stname;  // save which struct it belongs to
        entry.SOffset = size_so_far;
        entry.Vartype = vartype;
        // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the variable having the vartype
        entry.TypeQualifiers = tqs & ~kTQ_Autoptr & ~kTQ_Managed & ~kTQ_Builtin;
    }

    if (FlagIsSet(tqs, kTQ_Attribute))
        return ParseStruct_Attribute(tqs, stname, vname, vartype);

    if (_sym.GetSymbolType(_src.PeekNext()) == kSYM_OpenBracket)
    {
        Vartype vartype = _sym[vname].Vartype;
        ErrorType retval = ParseArray(vname, vartype);
        if (retval < 0) return retval;
        _sym[vname].Vartype = vartype;
    }

    size_so_far += _sym.GetSize(vname);
    return kERR_None;
}

// We have accepted something like "struct foo extends bar { readonly int".
// We're waiting for the name of the member.
ErrorType AGS::Parser::ParseStruct_MemberDefn(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype, size_t &size_so_far)
{
    // Get the variable or function name.
    Symbol component;
    ErrorType retval = ParseVarname(false, name_of_struct, component);
    if (retval < 0) return retval;

    Symbol const var_or_func_name = MangleStructAndComponent(name_of_struct, component);
    bool const is_function = (kSYM_OpenParenthesis == _sym.GetSymbolType(_src.PeekNext()));

    // In here, all struct members get this flag, functions included
    // This flag shows that the respective member has been declared within a struct xx {  }
    SetFlag(_sym[var_or_func_name].Flags, kSFLG_StructMember, true);
    _sym[var_or_func_name].Extends = name_of_struct;

    if (is_function)
        return ParseStruct_FuncDecl(name_of_struct, var_or_func_name, tqs, vartype);
    
    size_t const declaration_start = _src.GetCursor();
    if (_sym.IsDynarray(vartype)) // e.g., int [] zonk;
    {
        Error("Expected '('");
        return kERR_UserError;
    }

    if (kPP_Main == _pp)
    {
        if (_sym.GetSymbolType(var_or_func_name) != 0)
        {
            std::string const msg = ReferenceMsgSym(
                "'%s' is already defined", var_or_func_name);
            Error(msg.c_str(), _sym.GetName(var_or_func_name).c_str());
            return kERR_UserError;
        }

        // Mustn't be in any ancester
        retval = ParseStruct_CheckForCompoInAncester(name_of_struct, component, _sym[name_of_struct].Extends);
        if (retval < 0) return retval;
    }

    retval =  ParseStruct_VariableOrAttributeDefn(tqs, vartype, name_of_struct, var_or_func_name, size_so_far);
    if (retval < 0) return retval;

    _sym.SetDeclared(var_or_func_name, _src.GetSectionIdAt(declaration_start), _src.GetLinenoAt(declaration_start));
    return kERR_None;
}

ErrorType AGS::Parser::EatDynpointerSymbolIfPresent(Vartype vartype)
{
    if (_sym.GetDynpointerSym() != _src.PeekNext())
        return kERR_None;

    if (kPP_PreAnalyze == _pp || _sym.IsManaged(vartype))
    {
        _src.GetNext(); // Eat '*'
        return kERR_None;
    }

    Error("Cannot use '*' on the non-managed type '%s'", _sym.GetName(vartype).c_str());
    return kERR_UserError;
}

ErrorType AGS::Parser::ParseStruct_Vartype(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype, size_t &size_so_far)
{
    // Check for illegal struct member types
    if (kPP_Main == _pp)
    {
        ErrorType retval = ParseStruct_CheckComponentVartype(name_of_struct, vartype, FlagIsSet(tqs, kTQ_Import));
        if (retval < 0) return retval;
    }

    SetDynpointerInManagedVartype(vartype);
    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    // "int [] func(...)"
    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    // "TYPE noloopcheck foo(...)"
    if (kSYM_NoLoopCheck == _sym.GetSymbolType(_src.PeekNext()))
    {
        Error("Cannot use 'noloopcheck' here");
        return kERR_UserError;
    }  

    // We've accepted a type expression and are now reading vars or one func that should have this type.
    while (true)
    {
        retval = ParseStruct_MemberDefn(name_of_struct, tqs, vartype, size_so_far);
        if (retval < 0) return retval;

        Symbol const punctuation = _src.GetNext();
        SymbolType const punct_type = _sym.GetSymbolType(punctuation);
        if (kSYM_Comma != punct_type && kSYM_Semicolon != punct_type)
        {
            Error("Expected ',' or ';', found '%s' instead", _sym.GetName(punctuation).c_str());
            return kERR_UserError;
        }
        if (kSYM_Semicolon == punct_type)
            return kERR_None;
    }
}

// Handle a "struct" definition; we've already eaten the keyword "struct"
ErrorType AGS::Parser::ParseStruct(TypeQualifierSet tqs, AGS::Parser::NestingStack &nesting_stack, AGS::Symbol struct_of_current_func, AGS::Symbol name_of_current_func)
{
    // get token for name of struct
    Symbol const stname = _src.GetNext();

    if ((_sym.GetSymbolType(stname) != 0) &&
        (_sym.GetSymbolType(stname) != kSYM_UndefinedStruct))
    {
        std::string const msg = ReferenceMsgSym("'%s' is already defined", stname);
        Error(msg.c_str(), _sym.GetName(stname).c_str());
        return kERR_UserError;
    }

    ParseStruct_SetTypeInSymboltable(stname, tqs);

    // Declare the struct type that implements new strings
    if (FlagIsSet(tqs, kTQ_Stringstruct))
    {
        if (_sym.GetStringStructSym() > 0 && stname != _sym.GetStringStructSym())
        {
            Error("The stringstruct type is already defined to be %s", _sym.GetName(_sym.GetStringStructSym()).c_str());
            return kERR_UserError;
        }
        _sym.SetStringStructSym(stname);
    }

    size_t size_so_far = 0; // Will sum up the size of the struct

    if (_sym.GetSymbolType(_src.PeekNext()) == kSYM_Extends)
    {
        ErrorType retval = ParseStruct_ExtendsClause(stname, size_so_far);
        if (retval < 0) return retval;
    }

    // forward-declaration of struct type
    if (_sym.GetSymbolType(_src.PeekNext()) == kSYM_Semicolon)
    {
        if (!FlagIsSet(tqs, kTQ_Managed))
        {
            Error("Forward-declared structs must be 'managed'");
            return kERR_UserError;
        }
        _src.GetNext(); // Eat ';'
        SymbolTableEntry &entry = _sym[stname];
        entry.SType = kSYM_UndefinedStruct;
        SetFlag(entry.Flags, kSFLG_StructManaged, true);
        entry.SSize = 0;
        return kERR_None;
    }

    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_OpenBrace)
    {
        Error("Expected '{'");
        return kERR_UserError;
    }

    // Declaration of the components
    while (_sym.GetSymbolType(_src.PeekNext()) != kSYM_CloseBrace)
    {
        currentline = _src.GetLinenoAt(_src.GetCursor());
        TypeQualifierSet tqs = 0;
        ParseQualifiers(tqs);
        bool const in_func_body = false;
        bool const in_struct_decl = true;
        ErrorType retval = Parse_CheckTQ(tqs, in_func_body, in_struct_decl, kSYM_Vartype);
        if (retval < 0) return retval;

        Vartype vartype = _src.GetNext();

        retval = ParseStruct_Vartype(stname, tqs, vartype, size_so_far);
        if (retval < 0) return retval;
    }

    if (kPP_Main == _pp)
    {
        // round up size to nearest multiple of STRUCT_ALIGNTO
        if (0 != (size_so_far % STRUCT_ALIGNTO))
            size_so_far += STRUCT_ALIGNTO - (size_so_far % STRUCT_ALIGNTO);
        _sym[stname].SSize = size_so_far;
    }

    _src.GetNext(); // Eat '}'

    Symbol const nextsym = _src.PeekNext();
    SymbolType const type_of_next = _sym.GetSymbolType(nextsym);
    if (kSYM_Semicolon == type_of_next)
    {
        _src.GetNext(); // Eat ';'
        return kERR_None;
    }

    // If this doesn't seem to be a declaration at first glance,
    // warn that the user might have forgotten a ';'.
    if (kSYM_NoType != type_of_next &&
        kSYM_Function != type_of_next &&
        kSYM_GlobalVar != type_of_next &&
        kSYM_LocalVar != type_of_next &&
        kSYM_NoLoopCheck != type_of_next && 
        _sym.GetDynpointerSym() != nextsym)
    {
        Error("Unexpected '%s' (did you forget a ';' ?)", _sym.GetName(nextsym).c_str());
        return kERR_UserError;
    }

    // Take struct that has just been defined as the vartype of a declaration
    return ParseVartype(stname, nesting_stack, tqs, struct_of_current_func, name_of_current_func);
}

// We've accepted something like "enum foo { bar"; '=' follows
ErrorType AGS::Parser::ParseEnum_AssignedValue(int &currentValue)
{
    _src.GetNext(); // eat "="

    // Get the value of the item
    Symbol item_value = _src.GetNext(); // may be '-', too
    bool is_neg = false;
    if (item_value == _sym.Find("-"))
    {
        is_neg = true;
        item_value = _src.GetNext();
    }

    return IntLiteralOrConst2Value(item_value, is_neg, "Expected integer or integer constant after '='", currentValue);
}

void AGS::Parser::ParseEnum_Item2Symtable(AGS::Symbol enum_name, AGS::Symbol item_name, int currentValue)
{
    SymbolTableEntry &entry = _sym[item_name];

    entry.SType = kSYM_Constant;
    entry.Vartype = enum_name;
    entry.SScope = 0;
    entry.TypeQualifiers = kTQ_Readonly;
    // soffs is unused for a constant, so in a gratuitous hack we use it to store the enum's value
    entry.SOffset = currentValue;
    if (kPP_Main == _pp)
        _sym.SetDeclared(item_name, _src.GetSectionId(), _src.GetLineno());
}

ErrorType AGS::Parser::ParseEnum_Name2Symtable(AGS::Symbol enumName)
{
    SymbolTableEntry &entry = _sym[enumName];

    if (0 != entry.SType)
    {
        std::string msg = ReferenceMsg(
            "'%s' is already defined",
            entry.DeclSectionId,
            entry.DeclLine);
        Error(msg.c_str(), _sym.GetName(enumName).c_str());
        return kERR_UserError;
    }

    entry.SType = kSYM_Vartype;
    entry.SSize = SIZE_OF_INT;
    entry.Vartype = _sym.GetIntSym();

    return kERR_None;
}

// enum EnumName { value1, value2 }
ErrorType AGS::Parser::ParseEnum0()
{
    // Get name of the enum, enter it into the symbol table
    Symbol enum_name = _src.GetNext();
    ErrorType retval = ParseEnum_Name2Symtable(enum_name);
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_OpenBrace)
    {
        Error("Expected '{'");
        return kERR_UserError;
    }

    int currentValue = 0;

    while (true)
    {
        Symbol item_name = _src.GetNext();
        if (_sym.GetSymbolType(item_name) == kSYM_CloseBrace)
            break; // item list empty or ends with trailing ','

        if (kPP_Main == _pp)
        {
            if (_sym.GetSymbolType(item_name) == kSYM_Const)
            {
                std::string msg =
                    ReferenceMsgSym("'%s' is already defined as a constant or enum value", item_name);
                Error(msg.c_str(), _sym.GetName(item_name).c_str());
                return kERR_UserError;
            }
            if (_sym.GetSymbolType(item_name) != 0)
            {
                Error("Expected '}' or an unused identifier, found '%s' instead", _sym.GetName(item_name).c_str());
                return kERR_UserError;
            }
        }

        // increment the value of the enum entry
        currentValue++;

        SymbolType type_of_next = _sym.GetSymbolType(_src.PeekNext());
        if (type_of_next != kSYM_Assign && type_of_next != kSYM_Comma && type_of_next != kSYM_CloseBrace)
        {
            Error("Expected '=' or ',' or '}'");
            return kERR_UserError;
        }

        if (type_of_next == kSYM_Assign)
        {
            // the value of this entry is specified explicitly
            retval = ParseEnum_AssignedValue(currentValue);
            if (retval < 0) return retval;
        }

        // Enter this enum item as a constant int into the _sym table
        ParseEnum_Item2Symtable(enum_name, item_name, currentValue);

        Symbol comma_or_brace = _src.GetNext();
        if (_sym.GetSymbolType(comma_or_brace) == kSYM_CloseBrace)
            break;
        if (_sym.GetSymbolType(comma_or_brace) == kSYM_Comma)
            continue;

        Error("Expected ',' or '}'");
        return kERR_UserError;
    }
    return kERR_None;
}

// enum eEnumName { value1, value2 };
// We've already eaten "enum"
ErrorType AGS::Parser::ParseEnum(AGS::Symbol name_of_current_function)
{
    if (name_of_current_function >= 0)
    {
        Error("Enum declaration not allowed within a function body");
        return kERR_UserError;
    }

    ErrorType retval = ParseEnum0();
    if (retval < 0) return retval;

    // Force a semicolon after the declaration
    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_Semicolon)
    {
        Error("Expected ';'");
        return kERR_UserError;
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseExport()
{
    if (kPP_PreAnalyze == _pp)
    {
        const SymbolType stoplist[] = { kSYM_Semicolon };
        SkipTo(_src, stoplist, 1);
        _src.GetNext(); // Eat ';'
        return kERR_None;
    }

    // export specified symbols
    while (true) 
    {
        Symbol const export_sym = _src.GetNext();
        SymbolType const export_type = _sym.GetSymbolType(export_sym);
        if ((export_type != kSYM_GlobalVar) && (export_type != kSYM_Function))
        {
            Error("Can only export global variables and functions, not '%s'", _sym.GetName(export_sym).c_str());
            return kERR_UserError;
        }
        if (_sym.IsImport(export_sym))
        {
            Error("Cannot export the imported '%s'", _sym.GetName(export_sym).c_str());
            return kERR_UserError;
        }
        if (_sym.GetOldStringSym() == _sym.GetVartype(export_sym))
        {
            Error("Cannot export 'string'; use char[200] instead");
            return kERR_UserError;
        }
        // if all functions are being exported anyway, don't bother doing it now
        if (!(0 != ccGetOption(SCOPT_EXPORTALL) && kSYM_Function == export_type))
        {
            ErrorType retval = static_cast<ErrorType>(_scrip.add_new_export(
                _sym.GetName(export_sym).c_str(),
                (kSYM_GlobalVar == export_type) ? EXPORT_DATA : EXPORT_FUNCTION,
                _sym[export_sym].SOffset,
                _sym[export_sym].SScope));
            if (retval < 0) return retval;
        }
        Symbol const next_sym = _src.GetNext();
        Symbol const type_of_next = _sym.GetSymbolType(next_sym);
        if (kSYM_Semicolon != type_of_next && kSYM_Comma != type_of_next)
        {
            Error("Expected ',' or ';' instead of '%s'", _sym.GetName(next_sym).c_str());
            return kERR_UserError;
        }
        if (kSYM_Semicolon == type_of_next)
            break;
       

    }

    return kERR_None;
}
ErrorType AGS::Parser::ParseVartype_CheckForIllegalContext(NestingStack const &nesting_stack)
{
    SymbolType const ns_type = nesting_stack.Type();
    if (kSYM_Switch == ns_type)
    {
        Error("Cannot use declarations directly within a switch body. (Put \"{ ... }\" around the case statements)");
        return kERR_UserError;
    }

    if (kSYM_OpenBrace == ns_type || kSYM_Function == ns_type || kSYM_NoType == ns_type)
        return kERR_None;

    Error("A declaration cannot be the sole body of an 'if', 'else' or loop clause");
    return kERR_UserError;
}

ErrorType AGS::Parser::ParseVartype_CheckIllegalCombis(bool is_function, AGS::TypeQualifierSet tqs)
{
    if (FlagIsSet(tqs, kTQ_Static) && !is_function)
    {
        Error("'static' can only be applied to functions that are members of a struct");
        return kERR_UserError;
    }

    // Note: 'protected' is valid for struct functions; those can be defined directly,
    // as in int strct::function(){} or extender, as int function(this strct){}
    // We can't know at this point whether the function is extender, so we can't
    // check  at this point whether 'protected' is allowed.

    if (FlagIsSet(tqs, kTQ_Readonly) && is_function)
    {
        Error("Readonly cannot be applied to a function");
        return kERR_UserError;
    }

    if (FlagIsSet(tqs, kTQ_Writeprotected) && is_function)
    {
        Error("'writeprotected' cannot be applied to a function");
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ParseVartype_FuncDecl(TypeQualifierSet tqs, Vartype vartype, Symbol struct_name, Symbol func_name, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func, bool &body_follows)
{
    size_t const declaration_start = _src.GetCursor();
    _src.GetNext(); // Eat '('

    if (0 >= struct_name)
    {
        bool const func_is_static_extender = (kSYM_Static == _sym.GetSymbolType(_src.PeekNext()));
        bool const func_is_extender = (func_is_static_extender) || (_sym.GetThisSym() == _src.PeekNext());

        if (func_is_extender)
        {
            // Rewrite extender function as a component function of the corresponding struct.
            ErrorType retval = ParseFuncdecl_ExtenderPreparations(func_is_static_extender, struct_name, func_name, tqs);
            if (retval < 0) return retval;
        }

    }

    // Do not set .Extends or the StructComponent flag here. These denote that the
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

    if (no_loop_check)
        SetFlag(_sym[func_name].Flags, kSFLG_NoLoopCheck, true);

    // We've started a function, remember what it is.
    name_of_current_func = func_name;
    struct_of_current_func = struct_name;
    return kERR_None;
}

ErrorType AGS::Parser::ParseVartype_VarDecl_PreAnalyze(AGS::Symbol var_name, ScopeType scope_type)
{
    if (0 != _givm.count(var_name))
    {
        if (_givm[var_name])
        {
            Error("'%s' is already defined as a global non-import variable", _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
        else if (kScT_Global == scope_type && 0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            Error("'%s' is defined as an import variable; that cannot be overridden here", _sym.GetName(var_name).c_str());
            return kERR_UserError;
        }
    }
    _givm[var_name] = (kScT_Global == scope_type);

    // Apart from this, we aren't interested in var defns at this stage, so skip this defn
    SymbolType const stoplist[] = { kSYM_Comma, kSYM_Semicolon, };
    SkipTo(_src, stoplist, 2);
    return kERR_None;
}

ErrorType AGS::Parser::ParseVartype_VarDecl(Symbol var_name, ScopeType scope_type, int nesting_level, TypeQualifierSet tqs, Vartype vartype)
{
    if (kPP_PreAnalyze == _pp)
        return ParseVartype_VarDecl_PreAnalyze(var_name, scope_type);

    if (kScT_Local == scope_type)
        _sym[var_name].SScope = nesting_level;
    // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the variable having the vartype.
    _sym[var_name].TypeQualifiers = tqs & ~kTQ_Autoptr & ~kTQ_Managed & ~kTQ_Builtin;

    ErrorType retval = Parse_CheckTQ(tqs, (nesting_level >= 2), std::string::npos != _sym.GetName(var_name).rfind(':'), kSYM_Vartype);
    if (retval < 0) return retval;

    // parse the definition
    return ParseVardecl(var_name, vartype, scope_type);
}

// We accepted a variable type such as "int", so what follows is a function or variable declaration
ErrorType AGS::Parser::ParseVartype(Vartype vartype, NestingStack const &nesting_stack, TypeQualifierSet tqs, Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func)
{
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input (did you forget ';'?)");
        return kERR_UserError;
    }

    // Don't define variable or function where illegal in context.
    ErrorType retval = ParseVartype_CheckForIllegalContext(nesting_stack);
    if (retval < 0) return retval;

    ScopeType const scope_type = 
        (name_of_current_func > 0) ? kScT_Local :
        (FlagIsSet(tqs, kTQ_Import)) ? kScT_Import : kScT_Global;

    // Only imply a pointer for a managed entity if it isn't imported.
    if ((kScT_Import == scope_type && _sym.GetDynpointerSym() == _src.PeekNext()) ||
        (kScT_Import != scope_type && _sym.IsManaged(vartype)))
    {
        vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
    }

    retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    // "int [] func(...)"
    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    bool const no_loop_check = (kSYM_NoLoopCheck == _sym.GetSymbolType(_src.PeekNext()));
    if (no_loop_check)
        _src.GetNext();

    // We've accepted a vartype expression and are now reading vars or one func that should have this type.
    while(true)
    {
        // Get the variable or function name.
        Symbol var_or_func_name = -1;
        Symbol struct_name = -1;
        retval = ParseVarname(true, struct_name, var_or_func_name);
        if (retval < 0) return retval;

        bool const is_function = (kSYM_OpenParenthesis == _sym.GetSymbolType(_src.PeekNext()));

        // certain qualifiers, such as "static" only go with certain kinds of definitions.
        retval = ParseVartype_CheckIllegalCombis(is_function, tqs);
        if (retval < 0) return retval;

        if (is_function)
        {
            // Do not set .Extends or the StructComponent flag here. These denote that the
            // func has been either declared within the struct definition or as extender,
            // so they are NOT set unconditionally
            bool body_follows = false;
            retval = ParseVartype_FuncDecl(tqs, vartype, struct_name, var_or_func_name, no_loop_check, struct_of_current_func, name_of_current_func, body_follows);
            if (retval < 0) return retval;
            if (body_follows)
                return kERR_None;
        }
        else if (_sym.IsDynarray(vartype) || no_loop_check) // e.g., int [] zonk;
        {
            Error("Expected '('");
            return kERR_UserError;
        }
        else
        {
            if (0 < struct_name)
            {
                Error("Variable may not contain '::'");
                return kERR_UserError;
            }
            retval = ParseVartype_VarDecl(var_or_func_name, scope_type, nesting_stack.Depth(), tqs, vartype);
            if (retval < 0) return retval;
        }

        SymbolType const punctuation = _sym.GetSymbolType(_src.PeekNext());
        if (kSYM_Comma != punctuation && kSYM_Semicolon != punctuation)
        {
            Error("Expected ',' or ';', found '%s' instead", _sym.GetName(_src.PeekNext()).c_str());
            return kERR_UserError;
        }
        _src.GetNext();  // Eat ',' or ';'
        if (kSYM_Semicolon == punctuation)
            return kERR_None;
    }
}

ErrorType AGS::Parser::HandleEndOfCompoundStmts(AGS::Parser::NestingStack *nesting_stack)
{
    ErrorType retval;
    while (nesting_stack->Depth() > 2)
        switch (nesting_stack->Type())
        {
        default:
            Error("!Nesting of unknown type ends");
            return kERR_InternalError;

        case kSYM_OpenBrace:
        case kSYM_Switch:
            // The body of those statements can only be closed by an explicit '}'.
            // So that means that there cannot be any more non-braced compound statements to close here.
            return kERR_None;

        case kSYM_Do:
            retval = HandleEndOfDo(nesting_stack);
            if (retval < 0) return retval;
            break;

        case kSYM_Else:
            retval = HandleEndOfElse(nesting_stack);
            if (retval < 0) return retval;
            break;

        case kSYM_If:
        {
            bool else_follows;
            retval = HandleEndOfIf(nesting_stack, else_follows);
            if (retval < 0 || else_follows)
                return retval;
            break;
        }

        case kSYM_While:
            retval = HandleEndOfWhile(nesting_stack);
            if (retval < 0) return retval;
            break;
        } // switch (nesting_stack->Type())

    return kERR_None;
}

ErrorType AGS::Parser::ParseReturn(NestingStack *nesting_stack, AGS::Symbol name_of_current_func)
{
    Symbol const functionReturnType = _sym[name_of_current_func].FuncParamVartypes[0];

    if (_sym.GetSymbolType(_src.PeekNext()) != kSYM_Semicolon)
    {
        if (functionReturnType == _sym.GetVoidSym())
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
        retval = IsVartypeMismatch(_scrip.ax_vartype, functionReturnType, true);
        if (retval < 0) return retval;

        if (_sym.IsOldstring(_scrip.ax_vartype) &&
            (kScT_Local == _scrip.ax_scope_type))
        {
            Error("Cannot return local string from function");
            return kERR_UserError;
        }
    }
    else if (_sym.IsAnyIntType(functionReturnType))
    {
        WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    }
    else if (_sym.GetVoidSym() != functionReturnType)
    {
        Error("Must return a '%s' value from function", _sym.GetName(functionReturnType).c_str());
        return kERR_UserError;
    }

    Symbol const cursym = _src.GetNext();
    if (kSYM_Semicolon != _sym.GetSymbolType(cursym))
    {
        Error("Expected ';' instead of '%s'", _sym.GetName(cursym).c_str());
        return kERR_UserError;
    }

    // If locals contain pointers, free them
    FreeDynpointersOfLocals(0, name_of_current_func);
    size_t const size_of_locals = StacksizeOfLocals(SymbolTableEntry::ParameterSScope);
    // Pop local variables from the stack, but don't adjust _scrip.offset_to_local_var_block
    // because that is done as part of the end-of-function bookkeeping (?)
    if (size_of_locals > 0)
        WriteCmd(SCMD_SUB, SREG_SP, size_of_locals);

    // Jump to the exit point of the function
    WriteCmd(SCMD_JMP, 0);
    nesting_stack->JumpOut(1).AddParam();

    return kERR_None;
}

// Evaluate the header of an "if" clause, e.g. "if (i < 0)".
ErrorType AGS::Parser::ParseIf(AGS::Parser::NestingStack *nesting_stack)
{
    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    retval = nesting_stack->Push(kSYM_If);
    if (retval < 0) return retval;

    // The code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    WriteCmd(SCMD_JZ, -77);
    nesting_stack->JumpOut().AddParam();

    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfIf(NestingStack *nesting_stack, bool &else_follows)
{
    if (kSYM_Else != _sym.GetSymbolType(_src.PeekNext()))
    {
        else_follows = false;
        nesting_stack->JumpOut().Patch(_src.GetLineno());
        nesting_stack->Pop();
        return kERR_None;
    }

    else_follows = true;
    _src.GetNext(); // Eat "else"
    // Match the 'else' clause that is following to this 'if' stmt:
    // So we're at the end of the "then" branch. Jump out.
    _scrip.write_cmd(SCMD_JMP, -77);
    // So now, we're at the beginning of the "else" branch.
    // The jump after the "if" condition should go here.
    nesting_stack->JumpOut().Patch(_src.GetLineno());
    // Mark the  out jump after the "then" branch, above, for patching.
    nesting_stack->JumpOut().AddParam();
    // To prevent matching multiple else clauses to one if
    nesting_stack->SetType(kSYM_Else);
    return kERR_None;
}

// Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
ErrorType AGS::Parser::ParseWhile(AGS::Parser::NestingStack *nesting_stack)
{
    // point to the start of the code that evaluates the condition
    CodeLoc const condition_eval_loc = _scrip.codesize;

    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    retval = nesting_stack->Push(kSYM_While);
    if (retval < 0) return retval;

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    WriteCmd(SCMD_JZ, -77);
    nesting_stack->JumpOut().AddParam();
    nesting_stack->Start().Set(condition_eval_loc);

    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfWhile(NestingStack * nesting_stack)
{
    // if it's the inner level of a 'for' loop,
    // drop the yanked chunk (loop increment) back in
    if (nesting_stack->ChunksExist())
    {
        int id;
        CodeLoc const write_start = _scrip.codesize;
        nesting_stack->WriteChunk(0, id);
        _fcm.UpdateCallListOnWriting(write_start, id);
        _fim.UpdateCallListOnWriting(write_start, id);
        nesting_stack->Chunks().clear();
    }

    // jump back to the start location
    nesting_stack->Start().WriteJump(SCMD_JMP, _src.GetLineno());

    // This ends the loop
    nesting_stack->JumpOut().Patch(_src.GetLineno());
    nesting_stack->Pop();

    if (kSYM_For != nesting_stack->Type())
        return kERR_None;

    // This is the outer level of the FOR loop.
    // It can contain defns, e.g., "for (int i = 0;...)".
    // (as if it were surrounded in braces). Free these definitions
    return HandleEndOfBraceCommand(nesting_stack);
}

ErrorType AGS::Parser::ParseDo(AGS::Parser::NestingStack *nesting_stack)
{
    ErrorType retval = nesting_stack->Push(kSYM_Do);
    if (retval < 0) return retval;
    nesting_stack->Start().Set();

    return kERR_None;
}

ErrorType AGS::Parser::HandleEndOfBraceCommand(NestingStack *nesting_stack)
{
    nesting_stack->Pop();
    ExitNesting(nesting_stack->Depth());
    RemoveLocalsFromSymtable(nesting_stack->Depth());

    return kERR_None;
}

ErrorType AGS::Parser::ParseAssignmentOrExpression(AGS::Symbol cursym)
{    
    // Get expression
    _src.BackUp(); // Expression starts with cursym: the symbol in front of the cursor.
    size_t const expr_start = _src.GetCursor();
    ErrorType retval = SkipToEndOfExpression();
    if (retval < 0) return retval;
    SrcList expression = SrcList(_src, expr_start, _src.GetCursor() - expr_start);

    if (expression.Length() == 0)
    {
        Error("Unexpected symbol '%s' at start of statement", _sym.GetName(_src.GetNext()).c_str());
        return kERR_UserError;
    }

    Symbol const nextsym = _src.PeekNext();
    SymbolType const nexttype = _sym.GetSymbolType(nextsym);
    if (kSYM_Assign == nexttype || kSYM_AssignMod == nexttype || kSYM_AssignSOp == nexttype)
    {
        _src.GetNext(); // Eat assignment symbol
        return ParseAssignment(nextsym, expression);
    }

    // So this must be an isolated expression such as a function call. 
    ValueLocation vloc;
    ScopeType scope_type;
    Vartype vartype;
    retval = ParseExpression_Term(expression, vloc, scope_type, vartype);
    if (retval < 0) return retval;
    return ResultToAX(vloc, scope_type, vartype);
}

ErrorType AGS::Parser::ParseFor_InitClauseVardecl(size_t nesting_level)
{
    Vartype vartype = _src.GetNext();
    SetDynpointerInManagedVartype(vartype);

    ErrorType retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_src.PeekNext()) == kSYM_NoLoopCheck)
    {
        Error("'noloopcheck' is not applicable in this context");
        return kERR_UserError;
    }

    while (true)
    {
        Symbol varname = _src.GetNext();
        if (_sym.GetSymbolType(varname) != 0)
        {
            std::string msg =
                ReferenceMsgSym("Variable '%s' is already defined", varname);
            Error(msg.c_str(), _sym.GetName(varname).c_str());
            return kERR_UserError;
        }

        SymbolType const next_type = _sym.GetSymbolType(_src.PeekNext());
        if (kSYM_MemberAccess == next_type || kSYM_OpenParenthesis == next_type)
        {
            Error("Function definition not allowed in for loop initialiser");
            return kERR_UserError;
        }

        _sym[varname].SScope = nesting_level;
        retval = ParseVardecl(varname, vartype, kScT_Local);
        if (retval < 0) return retval;

        Symbol const punctuation = _sym.GetSymbolType(_src.PeekNext());
        if (kSYM_Comma != punctuation && kSYM_Semicolon != punctuation)
        {
            Error("Unexpected '%s'", _sym.GetName(_src.PeekNext()).c_str());
            return kERR_UserError;
        }
        if (kSYM_Comma == punctuation)
            _src.GetNext();  // Eat ','
        if (kSYM_Semicolon == punctuation)
            return kERR_None;
    }
}

// The first clause of a for header
ErrorType AGS::Parser::ParseFor_InitClause(AGS::Symbol peeksym, size_t nesting_level)
{
    if (_sym.GetSymbolType(peeksym) == kSYM_Semicolon)
        return kERR_None; // Empty init clause
    if (_sym.GetSymbolType(peeksym) == kSYM_Vartype)
        return ParseFor_InitClauseVardecl(nesting_level);
    return ParseAssignmentOrExpression(_src.GetNext());
}

ErrorType AGS::Parser::ParseFor_WhileClause()
{
    // Make the last emitted line number invalid so that a linenumber bytecode is emitted
    _scrip.last_emitted_lineno = INT_MAX;
    if (_sym.GetSymbolType(_src.PeekNext()) == kSYM_Semicolon)
    {
        // Not having a while clause is tantamount to the while condition "true".
        // So let's write "true" to the AX register.
        WriteCmd(SCMD_LITTOREG, SREG_AX, 1);
        return kERR_None;
    }

    return ParseExpression();
}

ErrorType AGS::Parser::ParseFor_IterateClause()
{
    // Check for empty interate clause
    if (kSYM_CloseParenthesis == _sym.GetSymbolType(_src.PeekNext()))
        return kERR_None;

    return ParseAssignmentOrExpression(_src.GetNext());
}

ErrorType AGS::Parser::ParseFor(AGS::Parser::NestingStack *nesting_stack)
{
    // "for (I; E; C) {...}" is equivalent to "{ I; while (E) {...; C} }"
    // We implement this with TWO levels of the nesting stack.
    // The outer level contains "I"
    // The inner level contains "while (E) { ...; C}"

    // Outer level
    ErrorType retval = nesting_stack->Push(kSYM_For);
    if (retval < 0) return retval;

    Symbol const paren = _src.GetNext();
    if (_sym.GetSymbolType(paren) != kSYM_OpenParenthesis)
    {
        Error("Expected '(' after 'for', found '%s' instead", _sym.GetName(paren).c_str());
        return kERR_UserError;
    }

    Symbol peeksym = _src.PeekNext();
    if (_sym.GetSymbolType(peeksym) == kSYM_CloseParenthesis)
    {
        Error("Empty parentheses \"()\" aren't allowed after \"for\" (write \"for(;;)\" instead");
        return kERR_UserError;
    }

    // Generate the initialization clause (I)
    retval = ParseFor_InitClause(peeksym, nesting_stack->Depth());
    if (retval < 0) return retval;

    Symbol const semicolon1 = _src.GetNext();
    if (_sym.GetSymbolType(semicolon1) != kSYM_Semicolon)
    {
        Error(
            "Expected ';' after for loop initializer clause, found '%s' instead",
            _sym.GetName(semicolon1).c_str());
        return kERR_UserError;
    }

    // Remember where the code of the while condition starts.
    CodeLoc const while_cond_loc = _scrip.codesize;

    retval = ParseFor_WhileClause();
    if (retval < 0) return retval;

    Symbol const semicolon2 = _src.GetNext();
    if (_sym.GetSymbolType(semicolon2) != kSYM_Semicolon)
    {
        Error(
            "Expected ';' after for loop while clause, found '%s' instead",
            _sym.GetName(semicolon2).c_str());
        return kERR_UserError;
    }

    // Remember where the code of the iterate clause starts.
    CodeLoc const iterate_clause_loc = _scrip.codesize;
    size_t const iterate_clause_lineno = _src.GetLineno();
    size_t pre_fixup_count = _scrip.numfixups;

    retval = ParseFor_IterateClause();
    if (retval < 0) return retval;
    Symbol const close_paren = _src.GetNext();
    if (_sym.GetSymbolType(close_paren) != kSYM_CloseParenthesis)
    {
        Error("Expected ')' after for loop iterate clause, found '%s' instead",
            _sym.GetName(close_paren).c_str());
        return kERR_UserError;
    }

    // Inner nesting level
    retval = nesting_stack->Push(kSYM_While);
    if (retval < 0) return retval;
    nesting_stack->Start().Set(while_cond_loc);

    // We've just generated code for getting to the next loop iteration.
     // But we don't need that code right here; we need it at the bottom of the loop.
     // So rip it out of the bytecode base and save it into our nesting stack.
    int id;
    size_t const yank_size = _scrip.codesize - iterate_clause_loc;
    nesting_stack->YankChunk(iterate_clause_lineno, iterate_clause_loc, pre_fixup_count, id);
    _fcm.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);
    _fim.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);

    // Code for "If the expression we just evaluated is false, jump over the loop body."
    WriteCmd(SCMD_JZ, -77);
    nesting_stack->JumpOut().AddParam();

    return kERR_None;
}

ErrorType AGS::Parser::ParseSwitch(AGS::Parser::NestingStack *nesting_stack)
{
    // Get the switch expression
    ErrorType retval = ParseParenthesizedExpression();
    if (retval < 0) return retval;

    // Remember the type of this expression to enforce it later
    Vartype const switch_expr_vartype = _scrip.ax_vartype;

    // Copy the result to the BX register, ready for case statements
    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_BX);

    if (kSYM_OpenBrace != _sym.GetSymbolType(_src.GetNext()))
    {
        Error("Expected '{'");
        return kERR_UserError;
    }

    retval = nesting_stack->Push(kSYM_Switch);
    if (retval < 0) return retval;

    nesting_stack->SetSwitchExprVartype(switch_expr_vartype);
    nesting_stack->SwitchDefault().Set(INT_MAX);

    // Jump to the jump table
    _scrip.write_cmd(SCMD_JMP, -77);
    nesting_stack->SwitchJumptable().AddParam();

    // Check that "default" or "case" follows
    if (_src.ReachedEOF())
    {
        Error("Unexpected end of input");
        return kERR_UserError;
    }
    if (_sym.GetSymbolType(_src.PeekNext()) != kSYM_Case && _sym.GetSymbolType(_src.PeekNext()) != kSYM_Default && _sym.GetSymbolType(_src.PeekNext()) != kSYM_CloseBrace)
    {
        Error("Expected 'default' or 'case', found '%s' instead", _sym.GetName(_src.PeekNext()).c_str());
        return kERR_UserError;
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseSwitchLabel(AGS::Symbol cursym, AGS::Parser::NestingStack *nesting_stack)
{
    if (kSYM_Switch != nesting_stack->Type())
    {
        Error("'%s' is only allowed directly within a 'switch' block", _sym.GetName(cursym).c_str());
        return kERR_UserError;
    }

    if (kSYM_Default == _sym.GetSymbolType(cursym))
    {
        if (INT_MAX != nesting_stack->SwitchDefault().Get())
        {
            Error("This switch block already has a 'default' label");
            return kERR_UserError;
        }
        nesting_stack->SwitchDefault().Set();
    }
    else // "case"
    {
        CodeLoc const start_of_code_loc = _scrip.codesize;
        size_t const start_of_code_lineno = _src.GetLineno();
        int const numfixups_at_start_of_code = _scrip.numfixups;

        // Push the switch variable onto the stack
        _scrip.push_reg(SREG_BX);

        ErrorType retval = ParseExpression(); // case n: label expression
        if (retval < 0) return retval;

        // check that the types of the "case" expression and the "switch" expression match
        retval = IsVartypeMismatch(_scrip.ax_vartype, nesting_stack->SwitchExprVartype(), false);
        if (retval < 0) return retval;

        // Pop the switch variable, ready for comparison
        _scrip.pop_reg(SREG_BX);

        // rip out the already generated code for the case/switch and store it with the switch
        int id;
        size_t const yank_size = _scrip.codesize - start_of_code_loc;
        nesting_stack->YankChunk(start_of_code_lineno, start_of_code_loc, numfixups_at_start_of_code, id);
        _fcm.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
        _fim.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
    }

    // expect and gobble the ':'
    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_Label)
    {
        Error("Expected ':'");
        return kERR_UserError;
    }

    return kERR_None;
}

ErrorType AGS::Parser::ExitNesting(size_t nesting_level)
{
    // If locals contain pointers, free them
    FreeDynpointersOfLocals(nesting_level);

    // Pop local variables from the stack
    int const size_of_local_vars = StacksizeOfLocals(nesting_level);
    if (size_of_local_vars > 0)
    {
        _scrip.offset_to_local_var_block -= size_of_local_vars;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_local_vars);
    }
    return kERR_None;
}

ErrorType AGS::Parser::ParseBreak(AGS::Parser::NestingStack *nesting_stack)
{
    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_Semicolon)
    {
        Error("Expected ';'");
        return kERR_UserError;
    }

    size_t nesting_level;

    // Find the (level of the) looping construct to which the break applies
    // Note that this is similar, but _different_ from "continue"!
    for (nesting_level = nesting_stack->Depth() - 1; nesting_level > 0; nesting_level--)
    {
        SymbolType ltype = nesting_stack->Type(nesting_level);
        if (kSYM_Do == ltype || kSYM_Switch == ltype || kSYM_While == ltype)
            break;
    }

    if (nesting_level == 0)
    {
        Error("'break' only valid inside a loop or switch statement block");
        return kERR_UserError;
    }
    ExitNesting(nesting_level);

    // Jump out of the loop
    WriteCmd(SCMD_JMP, -77);
    nesting_stack->JumpOut(nesting_level).AddParam();

    return kERR_None;
}

ErrorType AGS::Parser::ParseContinue(AGS::Parser::NestingStack *nesting_stack)
{
    if (_sym.GetSymbolType(_src.GetNext()) != kSYM_Semicolon)
    {
        Error("Expected ';'");
        return kERR_UserError;
    }

    size_t nesting_level;

    // Find the (level of the) looping construct to which the break applies
    // Note that this is similar, but _different_ from "break"!
    for (nesting_level = nesting_stack->Depth() - 1; nesting_level > 0; nesting_level--)
    {
        SymbolType ltype = nesting_stack->Type(nesting_level);
        if (kSYM_Do == ltype || kSYM_While == ltype)
            break;
    }

    if (nesting_level == 0)
    {
        Error("'continue' is only valid inside a loop or switch statement block");
        return kERR_UserError;
    }

    ExitNesting(nesting_level);

    // if it's a for loop, drop the yanked chunk (loop increment) back in
    if (nesting_stack->ChunksExist(nesting_level))
    {
        int id;
        CodeLoc const write_start = _scrip.codesize;
        nesting_stack->WriteChunk(nesting_level, 0, id);
        _fcm.UpdateCallListOnWriting(write_start, id);
        _fim.UpdateCallListOnWriting(write_start, id);
    }

    // Jump to the start of the loop
    nesting_stack->Start(nesting_level).WriteJump(SCMD_JMP, _src.GetLineno());
    return kERR_None;
}

ErrorType AGS::Parser::ParseCloseBrace(AGS::Parser::NestingStack *nesting_stack)
{
    if (kSYM_Switch == nesting_stack->Type())
        return HandleEndOfSwitch(nesting_stack);
    return HandleEndOfBraceCommand(nesting_stack);
}

// We parse a command. The leading symbol has already been eaten
ErrorType AGS::Parser::ParseCommand(AGS::Symbol leading_sym, AGS::Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func, AGS::Parser::NestingStack *nesting_stack)
{
    ErrorType retval;

    // NOTE that some branches of this switch will leave
    // the whole function, others will continue after the switch.
    switch (_sym.GetSymbolType(leading_sym))
    {
    default:
    {
        // No keyword, so it should be an assignment or an isolated expression
        retval = ParseAssignmentOrExpression(leading_sym);
        if (retval < 0) return retval;
        Symbol const semicolon = _src.GetNext();
        if (_sym.GetSymbolType(semicolon) != kSYM_Semicolon)
        {
            Error("Expected ';', found '%s' instead.", _sym.GetName(semicolon).c_str());
            return kERR_UserError;
        }
        break;
    }

    case kSYM_Break:
        retval = ParseBreak(nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Case:
        retval = ParseSwitchLabel(leading_sym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case  kSYM_CloseBrace:
        // Note that the scanner has already made sure that every close brace has an open brace
        if (2 >= nesting_stack->Depth())
            return HandleEndOfFuncBody(nesting_stack, struct_of_current_func, name_of_current_func);

        retval = ParseCloseBrace(nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Continue:
        retval = ParseContinue(nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Default:
        retval = ParseSwitchLabel(leading_sym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Do:
        return ParseDo(nesting_stack);

    case kSYM_Else:
        Error("Cannot find any 'if' clause that matches this 'else'");
        return kERR_UserError;

    case kSYM_For:
        return ParseFor(nesting_stack);

    case kSYM_If:
        return ParseIf(nesting_stack);

    case kSYM_OpenBrace:
        if (2 > nesting_stack->Depth())
             return ParseFuncBody(nesting_stack, struct_of_current_func, name_of_current_func);
        return nesting_stack->Push(kSYM_OpenBrace);

    case kSYM_Return:
        retval = ParseReturn(nesting_stack, name_of_current_func);
        if (retval < 0) return retval;
        break;

    case kSYM_Switch:
        retval = ParseSwitch(nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_While:
        // This cannot be the end of a do...while() statement
        // because that would have been handled in HandleEndOfDo()
        return ParseWhile(nesting_stack);
    }

    // This statement may be the end of some unbraced
    // compound statements, e.g. "while (...) if (...) i++";
    // Pop the nesting levels of such statements and handle
    // the associated jumps.
    return HandleEndOfCompoundStmts(nesting_stack);
}

void AGS::Parser::HandleSrcSectionChangeAt(size_t pos)
{
    size_t const src_section_id = _src.GetSectionIdAt(pos);
    if (src_section_id == _lastEmittedSectionId)
        return;

    if (kPP_Main == _pp)
        _scrip.start_new_section(_src.SectionId2Section(src_section_id));
    _lastEmittedSectionId = src_section_id;
}

void AGS::Parser::Parse_SkipToEndingBrace()
{
    // Skip to matching '}'
    SymbolType const stoplist[] = { kSYM_NoType, };
    SkipTo(_src, stoplist, 0); // pass empty list
    _src.GetNext(); // Eat '}'
}

ErrorType AGS::Parser::ParseInput()
{
    Parser::NestingStack nesting_stack(_scrip);
    size_t nesting_level = 0;

    // We start off in the global data part - no code is allowed until a function definition is started
    Symbol struct_of_current_func = 0; // non-zero only when a struct member function is open
    Symbol name_of_current_func = -1;

    // Collects vartype qualifiers such as 'readonly'
    TypeQualifierSet tqs = 0;

    while (!_src.ReachedEOF())
    {
        size_t const next_pos = _src.GetCursor();
        HandleSrcSectionChangeAt(next_pos);
        currentline = _src.GetLinenoAt(next_pos);

        ParseQualifiers(tqs);

        Symbol const leading_sym = _src.GetNext();
        switch (_sym.GetSymbolType(leading_sym))
        {

        default:
            // Assume a command
            break;

        case 0:
            // let it through if "this" can be implied
            if (struct_of_current_func > 0)
            {
                Symbol const mangled = MangleStructAndComponent(struct_of_current_func, leading_sym);
                if (kSYM_NoType != _sym.GetSymbolType(mangled))
                    break;
            }
            Error("Unexpected token '%s'", _sym.GetName(leading_sym).c_str());
            return kERR_UserError;

        case kSYM_Enum:
        {
            ErrorType retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false, kSYM_Enum);
            if (retval < 0) return retval;
            retval = ParseEnum(name_of_current_func);
            if (retval < 0) return retval;
            continue;
        }

        case kSYM_Export:
        {
            ErrorType retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false, kSYM_Export);
            if (retval < 0) return retval;
            retval = ParseExport();
            if (retval < 0) return retval;
            continue;
        }

        case kSYM_OpenBrace:
        {
            if (kPP_Main == _pp)
                break; // treat as a command, below the switch

            Parse_SkipToEndingBrace();
            name_of_current_func = -1;
            struct_of_current_func = -1;
            continue;
        }

        case  kSYM_Struct:
        {
            ErrorType retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false, kSYM_Struct);
            if (retval < 0) return retval;
            retval = ParseStruct(tqs, nesting_stack, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;
            continue;
        }

        case kSYM_Vartype:
        {
            if (kSYM_Dot == _sym.GetSymbolType(_src.PeekNext()))
                break; // this is a static struct component function call, so a command

            // We can't check yet whether the TQS are legal because we don't know whether the
            // var / func names are composite.
            Vartype const vartype = leading_sym;
            ErrorType retval = ParseVartype(vartype, nesting_stack, tqs, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;

            continue;
        }

        } // switch (symType)

        // Commands are only allowed within a function
        if (name_of_current_func <= 0)
        {
            Error("'%s' is illegal outside a function", _sym.GetName(leading_sym).c_str());
            return kERR_UserError;
        }

        ErrorType retval = Parse_CheckTQ(tqs, (name_of_current_func > 0), false, kSYM_OpenBrace);
        if (retval < 0) return retval;

        retval = ParseCommand(leading_sym, struct_of_current_func, name_of_current_func, &nesting_stack);
        if (retval < 0) return retval;
    } // while (!targ.reached_eof())

    return kERR_None;
}

// Copy all the func headers from the PreAnalyse phase into the "real" symbol table
ErrorType AGS::Parser::Parse_ReinitSymTable(const ::SymbolTable &sym_after_scanning)
{
    size_t const sym_after_scanning_size = sym_after_scanning.entries.size();
    SymbolTableEntry empty;
    empty.SType = kSYM_NoType;

    for (size_t sym_idx = 0; sym_idx < _sym.entries.size(); sym_idx++)
    {
        SymbolTableEntry &s_entry = _sym[sym_idx];
        if (kSYM_Function == s_entry.SType)
        {
            SetFlag(s_entry.TypeQualifiers, kTQ_Import, (kFT_Import == s_entry.SOffset));
            s_entry.SOffset = 0;
            continue;
        }
        std::string const sname = s_entry.SName;
        s_entry =
            (sym_idx < sym_after_scanning_size) ? sym_after_scanning.entries[sym_idx] : empty;
        s_entry.SName = sname;
    }

    // This has invalidated the symbol table caches, so kill them
    _sym.ResetCaches();

    return kERR_None;
}

// blank out the name for imports that are not used, to save space
// in the output file       
ErrorType AGS::Parser::Parse_BlankOutUnusedImports()
{
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        SymbolType const stype = _sym.GetSymbolType(entries_idx);
        // Don't mind attributes - they are shorthand for the respective getter
        // and setter funcs. If _those_ are unused, then they will be caught
        // in the same that way normal functions are.
        if (kSYM_Function != stype && kSYM_GlobalVar != stype)
            continue;

        if (FlagIsSet(_sym[entries_idx].TypeQualifiers, kTQ_Import) &&
            !FlagIsSet(_sym[entries_idx].Flags, kSFLG_Accessed))
            _scrip.imports[_sym[entries_idx].SOffset][0] = '\0';
    }

    return kERR_None;
}

void AGS::Parser::ErrorWithPosition(int section_id, int lineno, char const *descr, ...)
{
    // cc_error() can't be called with a va_list and doesn't have a variadic variant,
    // so convert all the parameters into a single C string here
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, descr, vlist1) + 1];
    vsprintf(message, descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    // Set line; this is an implicit parameter of cc_error()
    currentline = lineno;

    // If an error occurs, then ccCurScriptName is supposed to point to a
    // static character array that contains the section of the error. 
    char const *current_section = _src.SectionId2Section(section_id).c_str();
    strncpy(SectionNameBuffer, current_section, sizeof(SectionNameBuffer) / sizeof(char) - 1);
    ccCurScriptName = SectionNameBuffer;
    cc_error("%s", message);
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

    ErrorWithPosition(_src.GetSectionId(), _src.GetLineno(), message);
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

    struct Warning const warning = { _src.GetLineno(), message };
    _warnings.push_back(warning);
    delete[] message;
}

ErrorType AGS::Parser::Parse_PreAnalyzePhase()
{
    // Needed to partially reset the symbol table later on
    SymbolTable const sym_after_scanning(_sym);

    _pp = kPP_PreAnalyze;
    ErrorType retval = ParseInput();
    if (retval < 0) return retval;

    _fcm.Reset();

    // Keep (just) the headers of functions that have a body to the main symbol table
    // Reset everything else in the symbol table,
    // but keep the entries so that they are guaranteed to have
    // the same index when parsed in phase 2
    return Parse_ReinitSymTable(sym_after_scanning);

}

ErrorType AGS::Parser::Parse_MainPhase()
{
    _pp = kPP_Main;
    return ParseInput();
}

ErrorType AGS::Parser::Parse()
{
    CodeLoc const start_of_input = _src.GetCursor();

    ErrorType retval = Parse_PreAnalyzePhase();
    if (retval < 0) return retval;

    _src.SetCursor(start_of_input);
    retval = Parse_MainPhase();
    if (retval < 0) return retval;

    // If the following functions generate errors, they pertain to the source
    // as a whole. So let's generate them for the last source char. 
    size_t const last_pos = _src.Length() - 1;
    char const *current_section = _src.SectionId2Section(_src.GetSectionIdAt(last_pos)).c_str();
    strncpy(SectionNameBuffer, current_section, sizeof(SectionNameBuffer) / sizeof(char) - 1);
    ccCurScriptName = SectionNameBuffer;
    currentline = _src.GetLinenoAt(last_pos);

    retval = _fcm.CheckForUnresolvedFuncs();
    if (retval < 0) return retval;
    retval = _fim.CheckForUnresolvedFuncs();
    if (retval < 0) return retval;
    return Parse_BlankOutUnusedImports();
}

// Scan inpl into scan tokens, build a symbol table
int cc_scan(char const *inpl, SrcList *src, ccCompiledScript *scrip, SymbolTable *sym)
{
    AGS::Scanner scanner = { inpl, *src, *scrip, *sym };
    bool error_encountered = false;
    scanner.Scan(error_encountered);
    if (!error_encountered)
        return kERR_None;

    // Scaffolding around cc_error()
    currentline = scanner.GetLineno();
    std::string const section_buf = scanner.GetSection();
    ccCurScriptName = section_buf.c_str();
    cc_error("%s", scanner.GetLastError().c_str());
    return kERR_UserError;
}

int cc_parse(AGS::SrcList *src, ccCompiledScript *scrip, SymbolTable *symt)
{
    AGS::Parser parser = { *symt, *src, *scrip };
    return parser.Parse();
}

int cc_compile(char const *inpl, ccCompiledScript *scrip)
{
    std::vector<Symbol> symbols;
    LineHandler lh;
    size_t cursor = 0;
    SrcList src = SrcList(symbols, lh, cursor);
    src.NewSection("UnnamedSection");
    src.NewLine(1);
    SymbolTable sym;

    int retval = cc_scan(inpl, &src, scrip, &sym);
    if (retval < 0) return retval;

    return cc_parse(&src, scrip, &sym);
}


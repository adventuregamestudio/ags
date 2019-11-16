/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - IMPLEMENTATION

General:
Functions have names of the form AaaAaa or AaaAaa_BbbBbb
where the component parts are camelcased. This means that function AaaAaa_BbbBbb is a
subfunction of function AaaAaa that is exclusively called by function AaaAaa.

The Parser does does NOT get the sequence of tokens in a pipe from the Tokenizing step, i.e.,
it does NOT read the symbols one-by-one. To the contrary, the logic reads back and forth in
the token sequence.

(Nearly) All parser functions return an error code that is negative iff an error has been
encountered. In case of an error, they call cc_error() and return with a negative integer.

The Parser runs in two phases.
The first phase runs quickly through the tokenized source and collects the headers
of the local functions.

The second phase has the following main components:
    Declaration parsing
    Command parsing
        Functions that process the keyword Kkk are called ParseKkk()

    Code nesting and compound statements
        In ParseWhile() etc., DealWithEndOf..(), and class AGS::NestingStack.

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
    Class NestingStack keeps information on the "nesting level" of code:
    in the statement "while (i > 0) i += 1", the nesting level "i += 1" is defined
    to be one higher than the nesting level of "while (i > 0)".
    For each nesting level, the class keeps, amongst others, the location in the bytecode
    of the start of the construct and the location of a Bytecode jump to its end.

    When handling nested constructs, the parser sometimes generates and emits some code,
    then rips it out of the codebase and stores it internally, then later on, retrieves
    it and emits it into the codebase again.

Pointers and managed structs:
    Bytecode: Any address that should hold a pointer must be manipulated using the
    SCMD_...PTR form of the commands and released by overwriting it with SCMD_MEMZEROPTR.

    Pointers are exclusively used for managed memory. If managed structs are manipulated,
    pointers MUST ALWAYS be used; for un-managed structs, pointers MAY NEVER be used. Blocks
    of primitive vartypes can be allocated as managed memory, in which case pointers MUST be
    used. That means that the compiler can deduce in about 99 % of the cases whether
    a pointer is used by looking at the managed keyword alone.

Classic arrays and Dynarrays:
    A variable type (vartype) consists of the symbol table index of the core type to which
    flags are added.
    A DynArray is always managed, so always also has the Managed flag.
    A Dynarray of primitives (e.g., int[]) is represented in memory as a pointer to a memory
    block that comprises all the elements, one after the other.
    [*]->[][]...[]
    A Dynarray of structs must be a dynarray of managed structs, so there is no additional
    indication that the structs are managed. It is represented in memory as a pointer to a
    block of pointers, each of which points to one element.
    [*]->[*][*]...[*]
          |  |     |
          V  V ... V
         [] [] ... []
    In contrast to a dynamic array, a classic array is never managed.
    A classic array of primitives (e.g., int[12]) or of non-managed structs is represented
    in memory as a block of those elements.
    [][]...[]
    A classic array of managed structs has both the Array and the Managed flag set. In this
    case, the Managed tag refers to the core type, i.e., it's a classic array of pointers,
    each of which points to a memory block that contains one element.
    [*][*]...[*]
     |  |     |
     V  V ... V
    [] [] ... []

Oldstyle strings, string literals, string buffers:
    If a "string" is declared, 200 bytes of memory are reserved on the stack (local) or in
    global memory (global). This is called a "string buffer". Whenever oldstyle strings or
    literal strings are used, they are referred to by the address of their first byte.
    The only way of modifying a string buffer is by functions. The compiler doesn't
    attempt in any way to prevent buffer underruns or overruns.  
*/


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <limits>
#include <algorithm>
#include <ctime>
#include <fstream>

#include "script/cc_options.h"
#include "script/script_common.h"
#include "script/cc_error.h"

#include "cc_internallist.h"    // ccInternalList
#include "cs_parser_common.h"
#include "cc_symboltable.h"

#include "cs_scanner.h"
#include "cs_tokenizer.h"
#include "cs_parser.h"


// Declared in Common/script/script_common.h 
// Defined in Common/script/script_common.cpp
extern int currentline;

char ccCopyright[] = "ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2019 others";

bool AGS::Parser::IsIdentifier(AGS::Symbol symb)
{
    if (symb <= _sym.getLastPredefSym() || symb > static_cast<int>(_sym.entries.size()))
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


int AGS::Parser::String2Int(std::string str, int &val, bool send_error)
{
    const bool is_neg = (0 == str.length() || '-' == str.at(0));
    errno = 0;
    char *endptr = 0;
    const long longValue = strtol(str.c_str(), &endptr, 10);
    if ((longValue == LONG_MIN && errno == ERANGE) ||
        (is_neg && (endptr[0] != '\0')) ||
        (longValue < INT_MIN))
    {
        if (send_error)
            cc_error("Literal value '%s' is too low (min. is '%d')", str.c_str(), INT_MIN);
        return -1;
    }

    if ((longValue == LONG_MAX && errno == ERANGE) ||
        ((!is_neg) && (endptr[0] != '\0')) ||
        (longValue > INT_MAX))
    {
        if (send_error)
            cc_error("Literal value %s is too high (max. is %d)", str.c_str(), INT_MAX);
        return -1;
    }

    val = static_cast<int>(longValue);
    return 0;
}

AGS::Symbol AGS::Parser::MangleStructAndComponent(AGS::Symbol stname, AGS::Symbol component)
{
    std::string fullname_str = _sym.GetName(stname) + "::" + _sym.GetName(component);
    return _sym.FindOrAdd(fullname_str.c_str());
}

// Skim through the input, ignoring delimited content completely.
// Stop in the following cases:
//   A symbol is encountered whose type is in stoplist[]
//   A closing symbol is encountered that hasn't been opened.
// Don't consume the symbol that stops the scan.
int AGS::Parser::SkipTo(const AGS::SymbolType stoplist[], size_t stoplist_len)
{
    int delimeter_nesting_depth = 0;
    while (!_targ.reached_eof())
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        AGS::Symbol const cursym = _targ.peeknext();
        SymbolType const curtype = _sym.GetSymbolType(cursym);
        if (curtype == kSYM_OpenBrace ||
            curtype == kSYM_OpenBracket ||
            curtype == kSYM_OpenParenthesis)
        {
            ++delimeter_nesting_depth;
        }
        if (curtype == kSYM_CloseBrace ||
            curtype == kSYM_CloseBracket ||
            curtype == kSYM_CloseParenthesis)
        {
            if (--delimeter_nesting_depth < 0)
                return 0;
        }
        for (size_t stoplist_idx = 0; stoplist_idx < stoplist_len; stoplist_idx++)
            if (curtype == stoplist[stoplist_idx])
                return 0;
        _targ.getnext();
    }
    return -1;
}

int AGS::Parser::SkipToScript0(AGS::Symbol *end_sym_ptr, const AGS::SymbolType stoplist[], size_t stoplist_len, AGS::Symbol *&act_sym_ptr)
{
    int delimeter_nesting_depth = 0;

    for (; act_sym_ptr != end_sym_ptr; act_sym_ptr++)
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        AGS::Symbol const cursym = *act_sym_ptr;
        SymbolType const curtype = _sym.GetSymbolType(cursym);
        if (curtype == kSYM_OpenBrace ||
            curtype == kSYM_OpenBracket ||
            curtype == kSYM_OpenParenthesis)
        {
            ++delimeter_nesting_depth;
        }
        if (curtype == kSYM_CloseBrace ||
            curtype == kSYM_CloseBracket ||
            curtype == kSYM_CloseParenthesis)
        {
            if (--delimeter_nesting_depth < 0)
                return 0;
        }
        for (size_t stoplist_idx = 0; stoplist_idx < stoplist_len; stoplist_idx++)
            if (curtype == stoplist[stoplist_idx])
                return 0;
    }
    return -1;
}

// Like SkipTo, but for symbol scripts
int AGS::Parser::SkipToScript(const AGS::SymbolType stoplist[], size_t stoplist_len, SymbolScript &symlist, size_t &symlist_len)
{
    SymbolScript const end_ptr = symlist + symlist_len;
    int retval = SkipToScript0(end_ptr, stoplist, stoplist_len, symlist);
    symlist_len = end_ptr - symlist; // Get new length of the symbol script

    return retval;
}

// For assigning unique IDs to chunks
int AGS::NestingStack::_chunkIdCtr = 0;

int AGS::NestingStack::Push(NestingType type, AGS::CodeLoc start, AGS::CodeLoc info)
{
    std::vector<AGS::NestingStack::Chunk> dummy_chunk;
    struct AGS::NestingStack::NestingInfo ni = { type, start, info, 0, dummy_chunk };
    try
    {
        _stack.push_back(ni);
    }
    catch (...)
    {
        cc_error("Out of memory");
        return -1;
    }
    return 0;
}

AGS::NestingStack::NestingStack()
{
    // Push first record on stack so that it isn't empty
    Push(AGS::NestingStack::kNT_Nothing);
}

// Rip the code that has already been generated, starting from codeoffset, out of scrip
// and move it into the vector at list, instead.
void AGS::NestingStack::YankChunk(ccCompiledScript &scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset, int &id)
{
    AGS::NestingStack::Chunk item;

    size_t const codesize = std::max(0, scrip.codesize);
    for (size_t code_idx = codeoffset; code_idx < codesize; code_idx++)
        item.Code.push_back(scrip.code[code_idx]);

    size_t numfixups = std::max(0, scrip.numfixups);
    for (size_t fixups_idx = fixupoffset; fixups_idx < numfixups; fixups_idx++)
    {
        item.Fixups.push_back(scrip.fixups[fixups_idx]);
        item.FixupTypes.push_back(scrip.fixuptypes[fixups_idx]);
    }
    item.CodeOffset = codeoffset;
    item.FixupOffset = fixupoffset;
    item.Id = id = ++_chunkIdCtr;

    _stack.back().Chunks.push_back(item);

    // Cut out the code that has been pushed
    scrip.codesize = codeoffset;
    scrip.numfixups = fixupoffset;
}

// Copy the code in the chunk to the end of the bytecode vector 
void AGS::NestingStack::WriteChunk(ccCompiledScript &scrip, size_t level, size_t index, int &id)
{
    AGS::NestingStack::Chunk const item = Chunks(level).at(index);
    id = item.Id;
    scrip.flush_line_numbers();
    AGS::CodeLoc adjust = scrip.codesize - item.CodeOffset;

    size_t limit = item.Code.size();
    for (size_t index = 0; index < limit; index++)
        scrip.write_code(item.Code[index]);

    limit = item.Fixups.size();
    for (size_t index = 0; index < limit; index++)
        scrip.add_fixup(item.Fixups[index] + adjust, item.FixupTypes[index]);
}

AGS::FuncCallpointMgr::FuncCallpointMgr(::SymbolTable &symt, ::ccCompiledScript &scrip)
    : _sym(symt)
    , _scrip(scrip)
{ }

void AGS::FuncCallpointMgr::Reset()
{
    _funcCallpointMap.clear();
}

int AGS::FuncCallpointMgr::TrackForwardDeclFuncCall(Symbol func, CodeLoc loc)
{
    // Patch callpoint in when known
    CodeCell const callpoint = _funcCallpointMap[func].Callpoint;
    if (callpoint >= 0)
    {
        _scrip.code[loc] = callpoint;
        return 0;
    }

    // Callpoint not known, so remember this location
    PatchInfo pinfo;
    pinfo.ChunkId = CodeBaseId;
    pinfo.Offset = loc;
    _funcCallpointMap[func].List.push_back(pinfo);

    return 0;
}

int AGS::FuncCallpointMgr::TrackExitJumppoint(Symbol func, CodeLoc loc)
{
    PatchInfo pinfo;
    pinfo.ChunkId = CodeBaseId;
    pinfo.Offset = loc;
    _funcCallpointMap[-func].List.push_back(pinfo);

    return 0;
}

int AGS::FuncCallpointMgr::UpdateCallListOnYanking(AGS::CodeLoc chunk_start, size_t chunk_len, int id)
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

    return 0;
}

int AGS::FuncCallpointMgr::UpdateCallListOnWriting(AGS::CodeLoc start, int id)
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

    return 0;
}

int AGS::FuncCallpointMgr::SetFuncCallpoint(AGS::Symbol func, AGS::CodeLoc dest)
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
    return 0;
}

int AGS::FuncCallpointMgr::SetFuncExitJumppoint(AGS::Symbol func, AGS::CodeLoc dest)
{
    // Exit points of functions are stored under the negative function number
    _funcCallpointMap[-func].Callpoint = dest;
    PatchList &pl = _funcCallpointMap[-func].List;
    for (auto pl_it = pl.begin(); pl_it != pl.end(); ++pl_it)
        if (pl_it->ChunkId == CodeBaseId)
        {
            size_t const here = pl_it->Offset;
            _scrip.code[here] = ccCompiledScript::RelativeJumpDist(here, dest);
            pl_it->ChunkId = PatchedId;
        }
    pl.clear();
    return 0;
}

int AGS::FuncCallpointMgr::CheckForUnresolvedFuncs()
{
    for (CallMap::iterator fcm_it = _funcCallpointMap.begin(); fcm_it != _funcCallpointMap.end(); ++fcm_it)
    {
        PatchList &pl = fcm_it->second.List;
        size_t const pl_size = pl.size();
        for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        {
            if (pl[pl_idx].ChunkId != CodeBaseId)
                continue;

            cc_error("Function '%s()' has been called but not defined with body nor imported", _sym.GetName(fcm_it->first).c_str());
            return -1;
        }
    }
    return 0;
}

AGS::FuncCallpointMgr::CallpointInfo::CallpointInfo()
    : Callpoint(-1)
{ }

AGS::ImportMgr::ImportMgr()
    : _scrip(nullptr)
{
}

void AGS::ImportMgr::Init(ccCompiledScript *scrip)
{
    _importIdx.clear();
    _scrip = scrip;
    for (int import_idx = 0; import_idx < scrip->numimports; import_idx++)
        _importIdx[scrip->imports[import_idx]] = import_idx;
}

bool AGS::ImportMgr::IsDeclaredImport(std::string s)
{
    return (_importIdx.end() != _importIdx.find(s));
}

int AGS::ImportMgr::FindOrAdd(std::string s)
{
    auto it = _importIdx.find(s);
    if (_importIdx.end() != it)
        return it->second;
    // Cache miss
    int idx = _scrip->add_new_import(s.c_str());
    _importIdx[s] = idx;
    return idx;
}

void AGS::MemoryLocation::SetStart(SymbolType type, size_t offset)
{
    _Type = type;
    _StartOffs = offset;
    _ComponentOffs = 0;
}

void AGS::MemoryLocation::MakeMARCurrent(ccCompiledScript &scrip)
{
    switch (_Type)
    {
    default: // The memory location of the struct is up-to-date, but an offset might have accumulated 
        if (_ComponentOffs > 0)
            scrip.write_cmd2(SCMD_ADD, SREG_MAR, _ComponentOffs);
        break;

    case kSYM_GlobalVar:
        scrip.write_cmd2(SCMD_LITTOREG, SREG_MAR, _StartOffs + _ComponentOffs);
        scrip.fixup_previous(Parser::kFx_GlobalData);
        break;

    case kSYM_Import:
        // Have to convert the import number into a code offset first.
        // Can only then add the offset to it.
        scrip.write_cmd2(SCMD_LITTOREG, SREG_MAR, _StartOffs);
        scrip.fixup_previous(Parser::kFx_Import);
        if (_ComponentOffs != 0)
            scrip.write_cmd2(SCMD_ADD, SREG_MAR, _ComponentOffs);
        break;

    case kSYM_LocalVar:
        scrip.write_cmd1(
            SCMD_LOADSPOFFS,
            scrip.cur_sp - _StartOffs - _ComponentOffs);
        break;
    }
    Reset();
    return;
}

AGS::Parser::Parser(::SymbolTable &symt, ::ccInternalList &targ, ::ccCompiledScript &scrip)
    : _fcm(symt, scrip)
    , _fim(symt, scrip)
    , _scrip(scrip)
    , _sym(symt)
    , _targ(targ)
{
    _importMgr.Init(&scrip);
    _givm.clear();
}

void AGS::Parser::SetDynpointerInManagedVartype(Vartype &vartype)
{
    if (_sym.IsManaged(vartype))
        vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
}

// Return number of bytes to remove from stack to unallocate local vars
// of level from_level or higher
int AGS::Parser::StacksizeOfLocals(size_t from_level)
{
    int totalsub = 0;
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        if (_sym[entries_idx].SScope <= static_cast<int>(from_level))
            continue;
        if (_sym[entries_idx].SType != kSYM_LocalVar)
            continue;

        // caller will sort out stack, so ignore parameters
        if (FlagIsSet(_sym.GetFlags(entries_idx), kSFLG_Parameter))
            continue;

        totalsub +=
            (_sym.getThisSym() == entries_idx)? SIZE_OF_DYNPOINTER : _sym.GetSize(entries_idx);
    }
    return totalsub;
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
int AGS::Parser::FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements, bool &clobbers_ax)
{
    if (num_of_elements == 0)
        return 0;

    if (num_of_elements < 4)
    {
        _scrip.write_cmd0(SCMD_MEMZEROPTR);
        for (size_t loop = 1; loop < num_of_elements; ++loop)
        {
            _scrip.write_cmd2(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
            _scrip.write_cmd0(SCMD_MEMZEROPTR);
        }
        return 0;
    }

    clobbers_ax = true;
    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, num_of_elements);

    AGS::CodeLoc const loop_start = _scrip.codesize;
    _scrip.write_cmd0(SCMD_MEMZEROPTR);
    _scrip.write_cmd2(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
    _scrip.write_cmd2(SCMD_SUB, SREG_AX, 1);
    _scrip.write_cmd1(SCMD_JNZ, ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, loop_start));
    return 0;
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
            _scrip.write_cmd2(SCMD_ADD, SREG_MAR, diff);
        offset_so_far = entry.SOffset;

        if (_sym.IsDyn(entry.vartype))
        {
            _scrip.write_cmd0(SCMD_MEMZEROPTR);
            continue;
        }

        if (compo_list.back() != *compo_it)
            _scrip.push_reg(SREG_MAR);
        if (entry.IsArray(_sym))
            FreeDynpointersOfStdArray(*compo_it, clobbers_ax);
        else if (entry.IsStruct(_sym))
            FreeDynpointersOfStruct(entry.vartype, clobbers_ax);
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
    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, entry.NumArrayElements(_sym));

    AGS::CodeLoc loop_start = _scrip.codesize;
    _scrip.push_reg(SREG_MAR);
    _scrip.push_reg(SREG_AX); // FreeDynpointersOfStruct might call funcs that clobber AX
    FreeDynpointersOfStruct(struct_vtype, clobbers_ax);
    _scrip.pop_reg(SREG_AX);
    _scrip.pop_reg(SREG_MAR);
    _scrip.write_cmd2(SCMD_ADD, SREG_MAR, _sym.GetSize(struct_vtype));
    _scrip.write_cmd2(SCMD_SUB, SREG_AX, 1);
    _scrip.write_cmd1(SCMD_JNZ, ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, loop_start));
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

void AGS::Parser::FreeDynpointersOfLocals0(int from_level, bool &clobbers_ax, bool &clobbers_mar)
{
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        SymbolTableEntry &entry = _sym[entries_idx];
        if (entry.SScope <= from_level)
            continue;
        if (kSYM_LocalVar != entry.SType)
            continue;
        if (_sym.getThisSym() == entries_idx)
            continue; // don't touch the this pointer
        if (!ContainsReleasableDynpointers(entry.vartype))
            continue;

        clobbers_mar = true;
        int const sp_offset = _scrip.cur_sp - entry.SOffset;
        if (_sym.IsDyn(entry.vartype))
        {
            _scrip.write_cmd1(SCMD_LOADSPOFFS, sp_offset);
            _scrip.write_cmd0(SCMD_MEMZEROPTR);
            continue;
        }

        // Set MAR to the start of the construct that contains releasable pointers
        _scrip.write_cmd1(SCMD_LOADSPOFFS, sp_offset);

        if (entry.IsArray(_sym))
            FreeDynpointersOfStdArray(entries_idx, clobbers_ax);
        else if (entry.IsStruct(_sym))
            FreeDynpointersOfStruct(entry.vartype, clobbers_ax);
    }
}

// Free the pointers of any locals in level from_level or higher
int AGS::Parser::FreeDynpointersOfLocals(int from_level, AGS::Symbol name_of_current_func, bool ax_irrelevant)
{
    if (0 != from_level)
    {
        bool dummy_bool;
        FreeDynpointersOfLocals0(from_level, dummy_bool, dummy_bool);
        return 0;
    }

    // We're ending the current function; AX is containing the result of the func call.
    AGS::Vartype const func_return_type = _sym[name_of_current_func].FuncParamTypes.at(0);
    bool const function_returns_void = _sym.getVoidSym() == func_return_type;

    if (_sym.IsDyn(func_return_type) && !ax_irrelevant)
    {
        // The return value AX might point to a local dynamic object. So if we
        // now free the dynamic references and we don't take precautions,
        // this dynamic memory will get killed so our AX value is useless.
        // We only need these precautions if there are local dynamic objects.
        AGS::CodeLoc const codesize_before_precautions = _scrip.codesize;

        // Allocate a local dynamic pointer to hold the return value.
        _scrip.push_reg(SREG_AX);
        _scrip.write_cmd1(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
        _scrip.write_cmd1(SCMD_MEMINITPTR, SREG_AX);

        AGS::CodeLoc const codesize_before_freeing = _scrip.codesize;
        bool dummy_bool;
        bool mar_may_be_clobbered = false;
        FreeDynpointersOfLocals0(from_level, dummy_bool, mar_may_be_clobbered);
        bool const no_precautions_were_necessary =
            (codesize_before_freeing == _scrip.codesize);

        // Now release the dynamic pointer with a special opcode that prevents 
        // memory de-allocation as long as AX still has this pointer, too
        if (mar_may_be_clobbered)
            _scrip.write_cmd1(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
        _scrip.write_cmd1(SCMD_MEMREADPTR, SREG_AX);
        _scrip.write_cmd0(SCMD_MEMZEROPTRND); // special opcode
        _scrip.pop_reg(SREG_BX); // do NOT pop AX here
        if (no_precautions_were_necessary)
            _scrip.codesize = codesize_before_precautions; // rip out unneeded code
        return 0;
    }

    size_t const codesize_before = _scrip.codesize;
    bool clobbers_ax = false;
    bool dummy_bool;
    FreeDynpointersOfLocals0(from_level, clobbers_ax, dummy_bool);
    if (!clobbers_ax || function_returns_void)
        return 0;
    // Oops. AX was carrying our return value and shouldn't have been clobbered.
    // So we have to redo this and this time save AX before freeing.
    _scrip.codesize = codesize_before;
    _scrip.push_reg(SREG_AX);
    FreeDynpointersOfLocals0(from_level, clobbers_ax, dummy_bool);
    _scrip.pop_reg(SREG_AX);

    return 0;
}

// Remove defns from the _sym table of vars defined on from_level or higher
int AGS::Parser::RemoveLocalsFromSymtable(int from_level)
{

    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        if (_sym[entries_idx].SScope < from_level)
            continue;
        if (_sym[entries_idx].SType != kSYM_LocalVar)
            continue;

        _sym[entries_idx].SType = kSYM_NoType;
        _sym[entries_idx].SScope = 0;
        _sym[entries_idx].Flags = 0;
    }
    return 0;
}

// The type NT(Un)bracedElse isn't only used for ELSE branches, but also for the
// body of WHILE and other statements. 
// If the symbol "else" follows a THEN clause of an IF, this is handled most easily
// by adding an unconditional jump out and then changing the THEN clause into an ELSE clause. 
int AGS::Parser::DealWithEndOfElse(AGS::NestingStack *nesting_stack, bool &else_after_then)
{
    // Check whether the symbol "else" follows a then branch
    else_after_then = false;
    if (nesting_stack->Type() == AGS::NestingStack::kNT_UnbracedElse);
    else if (nesting_stack->Type() == AGS::NestingStack::kNT_BracedElse);
    else if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_Else)
    {
        _targ.getnext();  // eat "else"
        _scrip.write_cmd1(SCMD_JMP, 0); // jump out, to be patched later
        else_after_then = true;
    }

    if (nesting_stack->StartLoc()) // a loop that features a jump back to the start
    {
        _scrip.flush_line_numbers();
        // if it's a for loop, drop the yanked chunk (loop increment) back in
        if (nesting_stack->ChunksExist())
        {
            int id;
            AGS::CodeLoc const write_start = _scrip.codesize;
            nesting_stack->WriteChunk(_scrip, 0, id);
            _fcm.UpdateCallListOnWriting(write_start, id);
            _fim.UpdateCallListOnWriting(write_start, id);
            nesting_stack->Chunks().clear();
        }

        // jump back to the start location
        // The bytecode byte with the relative dest is at code[codesize+1]
        _scrip.write_cmd1(
            SCMD_JMP,
            ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, nesting_stack->StartLoc()));
    }

    // Patch the jump out of the construct to jump to here
    _scrip.code[nesting_stack->JumpOutLoc()] =
        ccCompiledScript::RelativeJumpDist(nesting_stack->JumpOutLoc(), _scrip.codesize);

    if (else_after_then)
    {
        // convert the THEN branch into an ELSE, i.e., stay on the same Depth()
        nesting_stack->SetType(AGS::NestingStack::kNT_UnbracedElse);
        if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_OpenBrace)
        {
            nesting_stack->SetType(AGS::NestingStack::kNT_BracedElse);
            _targ.getnext();
        }

        nesting_stack->SetJumpOutLoc(_scrip.codesize - 1);

        // We're continuing the current nesting until the "else" branch has ended.
        // So we haven't ended the current nesting yet. So we leave at this point.
        return 0;
    }

    // Clause ends, so pop the level off the stack
    nesting_stack->Pop();

    if (nesting_stack->Type() == AGS::NestingStack::kNT_For)
    {
        // A FOR is represented by two nestings, so we need to pop another level
        nesting_stack->Pop();

        // The outer nesting of the FOR can contain defns, e.g., "for (int i = 0;...)"
        // defines i. Free these definitions
        FreeDynpointersOfLocals(nesting_stack->Depth() - 1);
        int totalsub = StacksizeOfLocals(nesting_stack->Depth() - 1);
        if (totalsub > 0)
        {
            _scrip.cur_sp -= totalsub;
            _scrip.write_cmd2(SCMD_SUB, SREG_SP, totalsub);
        }
        RemoveLocalsFromSymtable(nesting_stack->Depth());
    }

    return 0;
}


int AGS::Parser::DealWithEndOfDo(AGS::NestingStack *nesting_stack)
{
    _scrip.flush_line_numbers();

    AGS::Symbol cursym = _targ.getnext();
    if (_sym.GetSymbolType(cursym) != kSYM_While)
    {
        cc_error("Do without while");
        return -1;
    }
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    _scrip.flush_line_numbers();

    int retval = ParseExpression();
    if (retval < 0) return retval;
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // Jump back to the start of the loop while the condition is true
    // The bytecode byte with the relative dest is at code[codesize+1]
    _scrip.write_cmd1(
        SCMD_JNZ,
        ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, nesting_stack->StartLoc()));
    // Patch the jump out of the loop; it should point to here
    _scrip.code[nesting_stack->JumpOutLoc()] =
        ccCompiledScript::RelativeJumpDist(nesting_stack->JumpOutLoc(), _scrip.codesize);

    // The clause has ended, so pop the level off the stack
    nesting_stack->Pop();

    return 0;
}


int AGS::Parser::DealWithEndOfSwitch(AGS::NestingStack *nesting_stack)
{
    // If there was no terminating break at the last switch-case, 
    // write a jump to the jumpout point to prevent a fallthrough into the jumptable
    const AGS::CodeLoc lastcmd_loc = _scrip.codesize - 2;
    const AGS::CodeLoc jumpout_loc = nesting_stack->StartLoc() + 2;
    if (_scrip.code[lastcmd_loc] != SCMD_JMP ||
        _scrip.code[lastcmd_loc + 1] != ccCompiledScript::RelativeJumpDist(lastcmd_loc + 1, jumpout_loc))
    {
        // The bytecode int that contains the relative jump is in codesize+1
        _scrip.write_cmd1(SCMD_JMP, ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, jumpout_loc));
    }

    // We begin the jump table; remember this address
    AGS::CodeLoc jumptable_loc = _scrip.codesize;

    // Patch the instruction "Jump to the jump table" at the start of the switch
    // so that it points to the correct address, i.e., here
    _scrip.code[nesting_stack->StartLoc() + 1] =
        ccCompiledScript::RelativeJumpDist(nesting_stack->StartLoc() + 1, jumptable_loc);

    // Get correct comparison operation: Don't compare strings as pointers but as strings
    int noteq_op = _sym.IsAnyTypeOfString(nesting_stack->SwitchExprType()) ? SCMD_STRINGSNOTEQ : SCMD_NOTEQUAL;

    const size_t size_of_chunks = nesting_stack->Chunks().size();
    for (size_t index = 0; index < size_of_chunks; index++)
    {
        int id;
        AGS::CodeLoc codesize = _scrip.codesize;
        // Put the result of the expression into AX
        nesting_stack->WriteChunk(_scrip, index, id);
        _fcm.UpdateCallListOnWriting(codesize, id);
        _fim.UpdateCallListOnWriting(codesize, id);
        // Do the comparison
        _scrip.write_cmd2(noteq_op, SREG_AX, SREG_BX);
        // This command will be written to code[codesize] and code[codesize]+1
        _scrip.write_cmd1(
            SCMD_JZ,
            ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, nesting_stack->Chunks().at(index).CodeOffset));
    }

    // Write the default jump if necessary
    if (nesting_stack->DefaultLabelLoc() != -1)
        _scrip.write_cmd1(SCMD_JMP, nesting_stack->DefaultLabelLoc() - _scrip.codesize - 2);

    // Patch the jump to the end of the switch block 
    // to jump to here (for break statements)
    _scrip.code[nesting_stack->StartLoc() + 3] =
        ccCompiledScript::RelativeJumpDist(nesting_stack->StartLoc() + 3, _scrip.codesize);

    // Patch the jump at the end of the switch block
    // (it is directly in front of the jumptable)
    // to jump to here
    _scrip.code[jumptable_loc - 1] =
        ccCompiledScript::RelativeJumpDist(jumptable_loc - 1, _scrip.codesize);

    nesting_stack->Chunks().clear();
    nesting_stack->Pop();

    return 0;
}

int AGS::Parser::ParseLiteralOrConstvalue(AGS::Symbol fromSym, int &theValue, bool isNegative, std::string errorMsg)
{
    if (fromSym >= 0)
    {
        SymbolTableEntry &from_entry = _sym[fromSym];
        if (from_entry.SType == kSYM_Constant)
        {
            theValue = from_entry.SOffset;
            if (isNegative)
                theValue = -theValue;
            return 0;
        }

        if (_sym.GetSymbolType(fromSym) == kSYM_LiteralInt)
        {
            std::string literalStrValue = _sym.GetName(fromSym);
            if (isNegative)
                literalStrValue = '-' + literalStrValue;

            return String2Int(literalStrValue, theValue, true);
        }
    }

    if (!errorMsg.empty())
        cc_error(errorMsg.c_str());
    return -1;
}


// We're parsing a parameter list and we have accepted something like "(...int i"
// We accept a default value clause like "= 15" if it follows at this point.
int AGS::Parser::ParseParamlist_Param_DefaultValue(bool &has_default_int, int &default_int_value)
{
    if (_sym.GetSymbolType(_targ.peeknext()) != kSYM_Assign)
    {
        has_default_int = false;
        return 0;
    }

    has_default_int = true;

    // parameter has default value
    _targ.getnext();   // Eat '='

    bool default_is_negative = false;
    AGS::Symbol default_value_symbol = _targ.getnext(); // may be '-', too
    if (default_value_symbol == _sym.Find("-"))
    {
        default_is_negative = true;
        default_value_symbol = _targ.getnext();
    }

    // extract the default value
    int retval = ParseLiteralOrConstvalue(default_value_symbol, default_int_value, default_is_negative, "Parameter default value must be literal");
    if (retval < 0) return retval;

    return 0;
}

int AGS::Parser::ParseDynArrayMarkerIfPresent(AGS::Vartype &vartype)
{
    if (_sym.GetSymbolType(_targ.peeknext()) != kSYM_OpenBracket)
        return 0;

    _targ.getnext(); // Eat '['
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseBracket)
    {
        cc_error("Fixed array size cannot be used here (use '[]' instead)");
        return -1;
    }
    vartype = _sym.VartypeWith(kVTT_Dynarray, vartype);
    return 0;
}

// Copy so that the forward decl can be compared afterwards to the real one     
int AGS::Parser::CopyKnownSymInfo(SymbolTableEntry &entry, SymbolTableEntry &known_info)
{
    known_info.SType = kSYM_NoType;
    if (kSYM_NoType == entry.SType)
        return 0; // there is no info yet

    known_info = entry;
    
    // Kill the defaults so we can check whether this defn replicates them exactly.
    size_t const num_of_args = entry.GetNumOfFuncArgs();
    entry.FuncParamHasDefaultValues.assign(num_of_args + 1, false);
    // -77 is an arbitrary value that is easy to spot in the debugger; 
    // don't use for anything in code
    entry.FuncParamDefaultValues.assign(num_of_args + 1, -77);
    return 0;
}


// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
// We'll accept something like "this Character *"
int AGS::Parser::ParseFuncdecl_ExtenderPreparations(bool is_static_extender, AGS::Symbol &name_of_func, AGS::Symbol &struct_of_func)
{
    if (struct_of_func > 0)
    {
        cc_error("A struct component function cannot be an extender function");
        return -1;
    }

    _targ.getnext(); // Eat "this" or "static"
    struct_of_func = _targ.peeknext();
    SymbolTableEntry &struct_entry = _sym[struct_of_func];
    if (!struct_entry.IsStruct(_sym))
    {
        cc_error("Expected a struct type instead of '%s'", _sym.GetName(struct_of_func).c_str());
        return -1;
    }

    if (std::string::npos != _sym.GetName(name_of_func).find_first_of(':'))
    {   // [fw] Can't be reached IMO. 
        cc_error("Extender functions cannot be part of a struct");
        return -1;
    }

    name_of_func = MangleStructAndComponent(struct_of_func, name_of_func);
    SymbolTableEntry &entry = _sym[name_of_func];

    // Don't clobber the flags set in the Pre-Analyze phase
    SetFlag(entry.Flags, kSFLG_StructMember, true);
    if (is_static_extender)
        SetFlag(entry.Flags, kSFLG_Static, true);

    _targ.getnext();
    if (!is_static_extender && _targ.getnext() != _sym.Find("*"))
    {
        cc_error("Instance extender function must be pointer");
        return -1;
    }

    if ((_sym.GetSymbolType(_targ.peeknext()) != kSYM_Comma) &&
        (_sym.GetSymbolType(_targ.peeknext()) != kSYM_CloseParenthesis))
    {
        if (_targ.getnext() == _sym.getPointerSym())
            cc_error("Must not use '*' for defining static extender function");
        else
            cc_error("Parameter name cannot be defined for extender type");
        return -1;
    }

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_Comma)
        _targ.getnext();

    return 0;
}


int AGS::Parser::ParseParamlist_ParamType(AGS::Vartype &vartype)
{
    if (_sym.getVoidSym() == vartype)
    {
        cc_error("A function parameter must not have the type 'void'");
        return -1;
    }
    SetDynpointerInManagedVartype(vartype);
    int retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    if (kPP_Main == _pp && !_sym.IsManaged(vartype) && _sym.IsStruct(vartype))
    {
        cc_error("A non-managed struct cannot be passed as parameter");
        return -1;
    }
    return 0;
}


// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present
int AGS::Parser::ParseParamlist_Param_Name(bool body_follows, AGS::Symbol &param_name)
{
    param_name = -1;

    if (kPP_PreAnalyze == _pp || !body_follows)
    {
        // Ignore the parameter name when present, it won't be used later on (in this phase)
        param_name = -1;
        AGS::Symbol const nextsym = _targ.peeknext();
        if (IsIdentifier(nextsym))
            _targ.getnext();
        return 0;
    }

    AGS::Symbol const nextsym = _targ.peeknext();
    if (_sym.GetSymbolType(nextsym) == kSYM_GlobalVar)
    {
        // This is a definition -- so the parameter name must not be a global variable
        std::string msg =
            ReferenceMsgSym("The name '%s' is already used for a global variable", nextsym);
        cc_error(msg.c_str(), _sym.GetName(_targ.peeknext()).c_str());
        return -1;
    }

    if (_sym.GetSymbolType(_targ.peeknext()) != 0)
    {
        // We need to have a real parameter name here
        cc_error("Expected a parameter name here, found '%s' instead", _sym.GetName(_targ.peeknext()).c_str());
        return -1;
    }

    param_name = _targ.getnext(); // get and gobble the parameter name

    return 0;
}


void AGS::Parser::ParseParamlist_Param_AsVar2Sym(AGS::Symbol param_name, AGS::Vartype param_type, bool param_is_const, int param_idx)
{
    SymbolTableEntry &param_entry = _sym[param_name];
    param_entry.SType = kSYM_LocalVar;
    param_entry.Extends = false;
    param_entry.vartype = param_type;
    size_t const param_size = 4; // We can only deal with parameters of size 4
    param_entry.SScope = 1;
    SetFlag(param_entry.Flags, kSFLG_Parameter, true);
    if (param_is_const)
    {
        SetFlag(param_entry.Flags, kSFLG_Readonly, true);
        param_entry.vartype = _sym.VartypeWith(kVTT_Const, param_entry.vartype);
    }
    // the parameters are pushed backwards, so the top of the
    // stack has the first parameter. The +1 is because the
    // call will push the return address onto the stack as well
    param_entry.SOffset = _scrip.cur_sp - (param_idx + 1) * 4;
    _sym.SetDeclared(param_name, ccCurScriptName, currentline);
}

void AGS::Parser::ParseParamlist_Param_Add2Func(AGS::Symbol name_of_func, int param_idx, AGS::Symbol param_type, bool param_is_const, bool param_has_int_default, int param_int_default)
{
    SymbolTableEntry &func_entry = _sym[name_of_func];
    size_t const minsize = param_idx + 1;
    if (func_entry.FuncParamTypes.size() < minsize)
    {
        func_entry.FuncParamTypes.resize(minsize);
        func_entry.FuncParamHasDefaultValues.resize(minsize);
        func_entry.FuncParamDefaultValues.resize(minsize);
    }

    func_entry.FuncParamTypes[param_idx] = param_type;
    if (param_is_const)
        func_entry.FuncParamTypes[param_idx] =
            _sym.VartypeWith(kVTT_Const, func_entry.FuncParamTypes[param_idx]);    

    if (param_has_int_default)
    {
        func_entry.FuncParamHasDefaultValues[param_idx] = param_has_int_default;
        func_entry.FuncParamDefaultValues[param_idx] = param_int_default;
    }
}

// process a parameter decl in a function parameter list, something like int foo(INT BAR
int AGS::Parser::ParseParamlist_Param(AGS::Symbol name_of_func, bool body_follows, AGS::Vartype vartype, bool param_is_const, int param_idx)
{
    int retval = ParseParamlist_ParamType(vartype);
    if (retval < 0) return retval;

    // Parameter name (when present and meaningful) 
    AGS::Symbol param_name;
    retval = ParseParamlist_Param_Name(body_follows, param_name);
    if (retval < 0) return retval;

    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    bool param_has_int_default = false;
    int param_int_default;
    retval = ParseParamlist_Param_DefaultValue(param_has_int_default, param_int_default);
    if (retval < 0) return retval;

    // Augment the function type in the symbol table  
    ParseParamlist_Param_Add2Func(name_of_func, param_idx, vartype, param_is_const, param_has_int_default, param_int_default);

    if (kPP_Main != _pp || !body_follows)
        return 0;

    // All function parameters correspond to local variables.
    // A body will follow, so we need to enter this parameter as a variable into the symbol table
    ParseParamlist_Param_AsVar2Sym(param_name, vartype, param_is_const, param_idx);

    return 0;
}

int AGS::Parser::ParseFuncdecl_Paramlist(AGS::Symbol funcsym, bool body_follows, int &numparams)
{
    bool param_is_const = false;
    while (!_targ.reached_eof())
    {
        AGS::Symbol const cursym = _targ.getnext();

        switch (_sym.GetSymbolType(cursym))
        {
        default:
            cc_error("Unexpected %s in parameter list", _sym.GetName(cursym).c_str());
            return -1;

        case kSYM_CloseParenthesis:
            return 0;

        case kSYM_Const:
        {
            // check in main compiler phase that type must follow
            if (kPP_Main == _pp && _sym.GetSymbolType(_targ.peeknext()) != kSYM_Vartype)
            {
                cc_error("Expected a type after 'const'");
                return -1;
            }
            param_is_const = true;
            continue;
        }

        case kSYM_Varargs:
        {
            numparams += VARARGS_INDICATOR;
            if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseParenthesis)
            {
                cc_error("Expected ')' after '...'");
                return -1;
            }
            return 0;
        }

        case kSYM_Vartype:
        {
            if ((numparams % VARARGS_INDICATOR) >= MAX_FUNCTION_PARAMETERS)
            {
                cc_error("Too many parameters defined for function (max. allowed: %d)", MAX_FUNCTION_PARAMETERS - 1);
                return -1;
            }

            int retval = ParseParamlist_Param(funcsym, body_follows, cursym, param_is_const, numparams);
            if (retval < 0) return retval;

            ++numparams;
            param_is_const = false; // modifier has been used up
            SymbolType const nexttype = _sym.GetSymbolType(_targ.peeknext());
            if (nexttype == kSYM_Comma)
                _targ.getnext(); // Eat ','
            continue;
        }
        } // switch
    } // while
    cc_error("End of input when processing parameter list");
    return -1;
}

void AGS::Parser::ParseFuncdecl_SetFunctype(Symbol name_of_function, Vartype return_vartype, bool func_is_static, bool func_is_protected, int numparams)
{
    SymbolTableEntry &entry = _sym[name_of_function];
    entry.SType = kSYM_Function;
    entry.SScope = numparams - 1;

    entry.FuncParamTypes[0] = return_vartype;
    if (func_is_static)
        SetFlag(entry.Flags, kSFLG_Static, true);
    if (func_is_protected)
        SetFlag(entry.Flags, kSFLG_Protected, true);

    return;
}


int AGS::Parser::ParseFuncdecl_CheckThatFDM_CheckDefaults(SymbolTableEntry const &this_entry, bool body_follows, SymbolTableEntry const &known_info)
{
    if (body_follows)
    {
        // If none of the parameters have a default,
        // we'll let this through for backward compatibility.
        bool has_default = false;
        for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncArgs(); ++param_idx)
            if (this_entry.FuncParamHasDefaultValues[param_idx])
            {
                has_default = true;
                break;
            }
        if (!has_default)
            return 0;
    }

    // this is 1 .. GetNumOfFuncArgs(), INCLUSIVE, because param 0 is the return type
    for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncArgs(); ++param_idx)
    {
        if ((this_entry.FuncParamHasDefaultValues[param_idx] ==
            known_info.FuncParamHasDefaultValues[param_idx]) &&
            (this_entry.FuncParamHasDefaultValues[param_idx] == false ||
                this_entry.FuncParamDefaultValues[param_idx] ==
                known_info.FuncParamDefaultValues[param_idx]))
            continue;

        std::string errstr1 = "In this declaration, parameter #<1> <2>; ";
        errstr1.replace(errstr1.find("<1>"), 3, std::to_string(param_idx));
        if (!this_entry.FuncParamHasDefaultValues[param_idx])
            errstr1.replace(errstr1.find("<2>"), 3, "doesn't have a default value");
        else
            errstr1.replace(errstr1.find("<2>"), 3, "has the default "
                + std::to_string(this_entry.FuncParamDefaultValues[param_idx]));

        std::string errstr2 = "in a declaration elsewhere, that parameter <2>.";
        if (!known_info.FuncParamHasDefaultValues[param_idx])
            errstr2.replace(errstr2.find("<2>"), 3, "doesn't have a default value");
        else
            errstr2.replace(errstr2.find("<2>"), 3, "has the default "
                + std::to_string(known_info.FuncParamDefaultValues[param_idx]));
        errstr1 += errstr2;
        errstr1 = ReferenceMsg(
            errstr1,
            _sym.Id2Section(known_info.DeclSectionId),
            known_info.DeclLine);
        cc_error(errstr1.c_str());
        return -1;
    }
    return 0;
}

// there was a forward declaration -- check that the real declaration matches it
int AGS::Parser::ParseFuncdecl_CheckThatKnownInfoMatches(SymbolTableEntry &this_entry, bool body_follows, SymbolTableEntry const &known_info)
{
    if (kSYM_NoType == known_info.SType)
        return 0; // We don't have any known info

    if (known_info.SType != this_entry.SType)
    {
        std::string msg = ReferenceMsg(
            "This is declared as a function here but differently elsewhere",
            _sym.Id2Section(known_info.DeclSectionId),
            known_info.DeclLine);
        cc_error(msg.c_str());
        return -1;
    }

    if ((known_info.Flags & ~kSFLG_Imported) != (this_entry.Flags & ~kSFLG_Imported))
    {
        std::string msg = ReferenceMsg(
            "The qualifiers of this function are different here than elsewhere",
            _sym.Id2Section(known_info.DeclSectionId),
            known_info.DeclLine);
        cc_error(msg.c_str());
        return -1;
    }

    if (known_info.GetNumOfFuncArgs() != this_entry.GetNumOfFuncArgs())
    {
        std::string msg = ReferenceMsg(
            "Function is declared with %d mandatory parameters here, %d mandatory parameters elswehere",
            _sym.Id2Section(known_info.DeclSectionId),
            known_info.DeclLine);
        cc_error(msg.c_str(), this_entry.GetNumOfFuncArgs(), known_info.GetNumOfFuncArgs());
        return -1;
    }
    if (known_info.IsVarargsFunc() != this_entry.IsVarargsFunc())
    {
        std::string te =
            known_info.IsVarargsFunc() ?
            "is declared to accept additional arguments here" :
            "is declared to not accept additional arguments here";
        std::string ki =
            known_info.IsVarargsFunc() ?
            "to accepts additional arguments elsewhere" :
            "to not accept additional arguments elsewhere";
        std::string const msg =
            ReferenceMsg(
            "Function %s, %s.",
            _sym.Id2Section(known_info.DeclSectionId),
            known_info.DeclLine);
        cc_error(msg.c_str(), te.c_str(), ki.c_str());
        return -1;
    }

    if (known_info.FuncParamTypes.at(0) != this_entry.FuncParamTypes.at(0))
    {
        std::string msg = ReferenceMsg(
            "Return type is declared as %s here, as %s elsewhere",
            _sym.Id2Section(known_info.DeclSectionId),
            known_info.DeclLine);
        cc_error(
            msg.c_str(),
            _sym.GetName(this_entry.FuncParamTypes.at(0)).c_str(),
            _sym.GetName(known_info.FuncParamTypes.at(0)).c_str());

        return -1;
    }

        for (size_t param_idx = 1; param_idx <= this_entry.GetNumOfFuncArgs(); param_idx++)
    {
        if (known_info.FuncParamTypes.at(param_idx) != this_entry.FuncParamTypes.at(param_idx))
        {
            std::string msg = ReferenceMsg(
                "Type of parameter #%d is %s here, %s in a declaration elsewhere",
                _sym.Id2Section(known_info.DeclSectionId),
                known_info.DeclLine);
            cc_error(
                msg.c_str(),
                param_idx,
                _sym.GetName(this_entry.FuncParamTypes.at(param_idx)).c_str(),
                _sym.GetName(known_info.FuncParamTypes.at(param_idx)).c_str());
            return -1;
        }
    }

    // Check that the defaults match
    int retval = ParseFuncdecl_CheckThatFDM_CheckDefaults(this_entry, body_follows, known_info);
    if (retval < 0) return retval;

    return 0;
}

// Enter the function in the imports[] or functions[] array; get its index   
int AGS::Parser::ParseFuncdecl_EnterAsImportOrFunc(AGS::Symbol name_of_func, bool body_follows, bool func_is_import, AGS::CodeLoc &function_soffs, int &function_idx)
{
    if (body_follows)
    {
        if (func_is_import)
        {
            cc_error("Imported functions cannot have a body");
            return -1;
        }
        // Index of the function in the ccCompiledScript::functions[] array
        function_soffs = _scrip.add_new_function(_sym.GetName(name_of_func), &function_idx);
        if (function_soffs < 0)
        {
            cc_error("Max. number of functions exceeded");
            return -1;
        }
        _fcm.SetFuncCallpoint(name_of_func, function_soffs);
        return 0;
    }

    if (!func_is_import)
    {
        function_soffs = -1; // forward decl; callpoint is unknown yet
        return 0;
    }

    // Index of the function in the ccScript::imports[] array
    function_soffs = _importMgr.FindOrAdd(_sym.GetName(name_of_func));
    return 0;
}


// We're at something like "int foo(", directly before the "("
// Get the symbol after the corresponding ")"
int AGS::Parser::ParseFuncdecl_GetSymbolAfterParmlist(AGS::Symbol &symbol)
{
    int pos = _targ.pos;

    SymbolType const stoplist[] = { kSYM_NoType };
    SkipTo(stoplist, 0); // Skim to matching ')'

    if (kSYM_CloseParenthesis != _sym.GetSymbolType(_targ.getnext()))
    {
        cc_error("Internal error: Unclosed parameter list of function");
        return -99;
    }

    symbol = _targ.peeknext();
    _targ.pos = pos;
    return 0;
}

int AGS::Parser::ParseFuncdecl_CheckValidHere(AGS::Symbol name_of_func, AGS::Vartype return_vartype, bool body_follows)
{
    SymbolType const stype = _sym[name_of_func].SType;
    if (kSYM_Function != stype && kSYM_NoType != stype)
    {
        std::string msg = ReferenceMsg(
            "'%s' is already defined",
            _sym.Id2Section(_sym[name_of_func].DeclSectionId),
            _sym[name_of_func].DeclLine);
        cc_error(msg.c_str(), _sym.GetName(name_of_func).c_str());
        return -1;
    }

    if (!_sym.IsManaged(return_vartype) && _sym.IsStruct(return_vartype))
    {
        cc_error("Can only return a managed struct from function");
        return -1;
    }

    if (body_follows && kPP_Main == _pp)
    {
        if (_fcm.HasFuncCallpoint(name_of_func))
        {
            std::string msg =
                ReferenceMsgSym("This function has already been defined with body", name_of_func);
            cc_error(msg.c_str());
            return -1;
        }
    }
    return 0;
}

// We're at something like "int foo(", directly before the "("
// This might or might not be within a struct defn
int AGS::Parser::ParseFuncdecl(AGS::Symbol &name_of_func, AGS::Vartype return_vartype, TypeQualifierSet tqs, AGS::Symbol &struct_of_func, bool &body_follows)
{
    _targ.getnext(); // Eat '('
    {
        AGS::Symbol symbol;
        int retval = ParseFuncdecl_GetSymbolAfterParmlist(symbol);
        if (retval < 0) return retval;
        body_follows = (kSYM_OpenBrace == _sym.GetSymbolType(symbol));
    }

    bool const func_is_static_extender = (kSYM_Static == _sym.GetSymbolType(_targ.peeknext()));
    bool const func_is_extender = (func_is_static_extender) || (_sym.getThisSym() == _targ.peeknext());

    // Rewrite extender function as if it were a component function of the corresponding struct.
    if (func_is_extender)
    {
        int retval = ParseFuncdecl_ExtenderPreparations(func_is_static_extender, name_of_func, struct_of_func);
        if (retval < 0) return retval;
    }

    int retval = ParseFuncdecl_CheckValidHere(name_of_func, return_vartype, body_follows);
    if (retval < 0) return retval;
    
    // A forward decl can be written with the
    // "import" keyword (when allowed in the options). This isn't an import
    // proper, so reset the "import" flag in this case.
    if (FlagIsSet(tqs, kTQ_Import) && kSYM_Function == _sym.GetSymbolType(name_of_func) && !FlagIsSet(_sym.GetFlags(name_of_func), kSFLG_Imported))
    {
        if (0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            cc_error("In here, a function with a local body must not have an \"import\" declaration");
            return -1;
        }
        SetFlag(tqs, kTQ_Import, false);
    }

    if (body_follows && kPP_Main == _pp)
        _scrip.cur_sp += 4;  // the return address will be pushed

    // Copy all known info about the function so that we can check whether this declaration is compatible
    SymbolTableEntry known_info;
    retval = CopyKnownSymInfo(_sym[name_of_func], known_info);
    if (retval < 0) return retval;

    int numparams = 1; // Counts the number of parameters including the ret parameter, so start at 1
    retval = ParseFuncdecl_Paramlist(name_of_func, body_follows, numparams);
    if (retval < 0) return retval;

    ParseFuncdecl_SetFunctype(name_of_func, return_vartype, FlagIsSet(tqs, kTQ_Static), FlagIsSet(tqs, kTQ_Protected), numparams);

    retval = ParseFuncdecl_CheckThatKnownInfoMatches(_sym[name_of_func], body_follows, known_info);
    if (retval < 0) return retval;

    // copy the default values from the function prototype
    if (known_info.SType != kSYM_NoType)
    {
        _sym[name_of_func].FuncParamHasDefaultValues.assign(
            known_info.FuncParamHasDefaultValues.begin(),
            known_info.FuncParamHasDefaultValues.end());
        _sym[name_of_func].FuncParamDefaultValues.assign(
            known_info.FuncParamDefaultValues.begin(),
            known_info.FuncParamDefaultValues.end());
    }

    _sym.SetDeclared(name_of_func, ccCurScriptName, currentline);

    if (kPP_Main == _pp)
    {
        // Get start offset and function index
        int function_idx = -1; // Index in the _scrip.functions[] array
        int func_startoffs;
        retval = ParseFuncdecl_EnterAsImportOrFunc(name_of_func, body_follows, FlagIsSet(tqs, kTQ_Import), func_startoffs, function_idx);
        if (retval < 0) return retval;
        _sym[name_of_func].SOffset = func_startoffs;
        if (function_idx >= 0)
            _scrip.functions[function_idx].NumOfParams = (numparams - 1);
    }

    if (!FlagIsSet(tqs, kTQ_Import))
        return 0;

    // Imported functions

    SetFlag(_sym[name_of_func].Flags, kSFLG_Imported, true);

    if (kPP_PreAnalyze == _pp)
    {
        _sym[name_of_func].SOffset = kFT_Import;
        return 0;
    }

    if (struct_of_func > 0)
    {
        char appendage[10];
        sprintf(appendage, "^%d", _sym[name_of_func].SScope); // num of parameters + (is_variadic)? 100 : 0
        strcat(_scrip.imports[_sym[name_of_func].SOffset], appendage);
    }

    _fim.SetFuncCallpoint(name_of_func, _sym[name_of_func].SOffset);
    return 0;
}


// interpret the float as if it were an int (without converting it really);
// return that int
int AGS::Parser::InterpretFloatAsInt(float floatval)
{
    float *floatptr = &floatval; // Get pointer to the float
    int *intptr = reinterpret_cast<int *>(floatptr); // pretend that it points to an int
    return *intptr; // return the int that the pointer points to
}

// return the index of the lowest MATHEMATICAL priority operator in the list,
// so that either side of it can be evaluated first.
// returns -1 if no operator was found
int AGS::Parser::IndexOfLowestBondingOperator(AGS::SymbolScript slist, size_t slist_len)
{
    size_t bracket_nesting_depth = 0;
    size_t paren_nesting_depth = 0;

    int lowest_MathPrio_found = std::numeric_limits<int>::max(); // c++ STL lingo for MAXINT
    int index_of_lowest_MathPrio = -1;

    for (size_t slist_idx = 0; slist_idx < slist_len; slist_idx++)
    {
        SymbolType thisType = _sym.GetSymbolType(slist[slist_idx]);
        switch (thisType)
        {
        default:
            break;
        case kSYM_OpenBracket:
            bracket_nesting_depth++;
            continue;
        case kSYM_CloseBracket:
            if (bracket_nesting_depth > 0)
                bracket_nesting_depth--;
            continue;
        case kSYM_OpenParenthesis:
            paren_nesting_depth++;
            continue;
        case kSYM_CloseParenthesis:
            if (paren_nesting_depth > 0)
                paren_nesting_depth--;
            continue;
        }

        // Continue if we aren't at zero nesting depth, since ()[] take priority
        if (paren_nesting_depth > 0 || bracket_nesting_depth > 0)
            continue;

        if (thisType != kSYM_Operator && thisType != kSYM_New)
            continue;

        int this_MathPrio = MathPrio(slist[slist_idx]);
        if (this_MathPrio > lowest_MathPrio_found)
            continue; // can't be lowest priority

        // remember this and keep looking
        lowest_MathPrio_found = this_MathPrio;
        index_of_lowest_MathPrio = slist_idx;
    }
    return index_of_lowest_MathPrio;
}

// Change the generic operator vcpuOp to the one that is correct for the vartypes
// Also check whether the operator can handle the types at all
int AGS::Parser::GetOperatorValidForVartype(AGS::Vartype vartype1, AGS::Vartype vartype2, AGS::CodeCell &vcpuOp)
{
    if (_sym.getFloatSym() == vartype1 || _sym.getFloatSym() == vartype2)
    {
        if (vartype1 != vartype2)
        {
            cc_error("The operator cannot be applied to a float and a non-float value");
            return -1;
        }
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to float values");
            return -1;
        case SCMD_ADD:      vcpuOp = SCMD_FADD; break;
        case SCMD_ADDREG:   vcpuOp = SCMD_FADDREG; break;
        case SCMD_DIVREG:   vcpuOp = SCMD_FDIVREG; break;
        case SCMD_GREATER:  vcpuOp = SCMD_FGREATER; break;
        case SCMD_GTE:      vcpuOp = SCMD_FGTE; break;
        case SCMD_ISEQUAL:  break;
        case SCMD_LESSTHAN: vcpuOp = SCMD_FLESSTHAN; break;
        case SCMD_LTE:      vcpuOp = SCMD_FLTE; break;
        case SCMD_MULREG:   vcpuOp = SCMD_FMULREG; break;
        case SCMD_NOTEQUAL: break;
        case SCMD_SUB:      vcpuOp = SCMD_FSUB; break;
        case SCMD_SUBREG:   vcpuOp = SCMD_FSUBREG; break;
        }
        return 0;
    }

    bool const iatos1 = _sym.IsAnyTypeOfString(vartype1);
    bool const iatos2 = _sym.IsAnyTypeOfString(vartype2);

    if (iatos1 || iatos2)
    {
        if (_sym.getNullSym() == vartype1 || _sym.getNullSym() ==vartype2)
            return 0;

        if (iatos1 != iatos2)
        {
            cc_error("A string type value cannot be compared to a value that isn't a string type");
            return -1;
        }
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to string type values");
            return -1;
        case SCMD_ISEQUAL:  vcpuOp = SCMD_STRINGSEQUAL; return 0;
        case SCMD_NOTEQUAL: vcpuOp = SCMD_STRINGSNOTEQ; return 0;
        }
    }

    if (((_sym.IsDynpointer(vartype1) || vartype1 == _sym.getNullSym()) &&
        (_sym.IsDynpointer(vartype2) || vartype2 == _sym.getNullSym())) ||
        ((_sym.IsDynarray(vartype1) || vartype1 == _sym.getNullSym()) &&
        (_sym.IsDynarray(vartype2) || vartype2 == _sym.getNullSym())))
    {
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to managed types");
            return -1;
        case SCMD_ISEQUAL:  return 0;
        case SCMD_NOTEQUAL: return 0;
        }
    }

    // Other combinations of managed types won't mingle
    if (_sym.IsDynpointer(vartype1) || _sym.IsDynpointer(vartype2))
    {
        cc_error("The operator cannot be applied to values of these types");
        return -1;
    }

    return 0;
}

// Check for a type mismatch in one direction only
bool AGS::Parser::IsVartypeMismatch_Oneway(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be)
{
    // cannot convert 'void' to anything
    if (_sym.getVoidSym() == vartype_is)
        return true;

    // Don't convert if no conversion is called for
    if (vartype_is == vartype_wants_to_be)
        return false;

    // cannot convert const to non-const
    if (_sym.IsConst(vartype_is)  && !_sym.IsConst(vartype_wants_to_be))
        return true;

    // Can convert null to dynpointer or dynarray
    if (_sym.getNullSym() == vartype_is)
        return
            !_sym.IsDynpointer(vartype_wants_to_be) &&
            !_sym.IsDynarray(vartype_wants_to_be);

    // can convert String* to const string
    if (_sym.getStringStructSym() == _sym.VartypeWithout(kVTT_Dynpointer, vartype_is) &&
        _sym.getOldStringSym() == _sym.VartypeWithout(kVTT_Const, vartype_wants_to_be))
    {
        return false;
    }
    if (_sym.IsOldstring(vartype_is) != _sym.IsOldstring(vartype_wants_to_be))
        return true;
    if (_sym.IsOldstring(vartype_is))
        return false;

    // From here on, don't mind constness or dynarray-ness
    vartype_is = _sym.VartypeWithout(kVTT_Const | kVTT_Dynarray, vartype_is);
    vartype_wants_to_be = _sym.VartypeWithout(kVTT_Const | kVTT_Dynarray, vartype_wants_to_be);

    // floats cannot mingle with other types
    if ((vartype_is == _sym.getFloatSym()) != (vartype_wants_to_be == _sym.getFloatSym()))
        return true;

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
        Symbol const act_core_vartype = _sym.VartypeWithout(kVTT_Dynarray, vartype_is);
        return act_core_vartype != target_core_vartype;
    }

    // Checks to do if at least one is dynpointer
    if (_sym.IsDynpointer(vartype_is) || _sym.IsDynpointer(vartype_wants_to_be))
    {
        // BOTH sides must be dynpointer
        if (_sym.IsDynpointer(vartype_is) != _sym.IsDynpointer(vartype_wants_to_be))
            return true;

        // Core vartypes need not be identical here: check against extensions
        Symbol const target_core_vartype = _sym.VartypeWithout(kVTT_Dynpointer, vartype_wants_to_be);
        Symbol act_core_vartype = _sym.VartypeWithout(kVTT_Dynpointer, vartype_is);
        while (act_core_vartype != target_core_vartype)
        {
            act_core_vartype = _sym[act_core_vartype].Extends;
            if (act_core_vartype == 0)
                return true;
        }
        return false;
    }

    // Checks to do if at least one is a struct
    if (_sym.IsStruct(vartype_is) || _sym.IsStruct(vartype_wants_to_be))
        return (vartype_is != vartype_wants_to_be);

     return false;
}

// Check whether there is a type mismatch; if so, give an error
int AGS::Parser::IsVartypeMismatch(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be, bool orderMatters)
{
    if (!IsVartypeMismatch_Oneway(vartype_is, vartype_wants_to_be))
        return 0;
    if (!orderMatters && !IsVartypeMismatch_Oneway(vartype_wants_to_be, vartype_is))
        return 0;


    cc_error(
        "Type mismatch: cannot convert '%s' to '%s'",
        _sym.GetName(vartype_is).c_str(),
        _sym.GetName(vartype_wants_to_be).c_str());
    return -1;
}

// returns whether this operator's val type is always bool
bool AGS::Parser::IsBooleanVCPUOperator(int scmdtype)
{
    if ((scmdtype >= SCMD_ISEQUAL) &&
        (scmdtype <= SCMD_OR))
    {
        return true;
    }

    if ((scmdtype >= SCMD_FGREATER) &&
        (scmdtype <= SCMD_FLTE))
    {
        return true;
    }

    if ((scmdtype == SCMD_STRINGSNOTEQ) ||
        (scmdtype == SCMD_STRINGSEQUAL))
    {
        return true;
    }
    return false;
}

// If we need a String but AX contains a string, 
// then convert AX into a String object and set its type accordingly
void AGS::Parser::ConvertAXStringToStringObject(AGS::Vartype wanted_vartype)
{
    if (_sym.getOldStringSym() == _sym.VartypeWithout(kVTT_Const, _scrip.ax_vartype) &&
        _sym.getStringStructSym() == _sym.VartypeWithout(kVTT_Dynpointer, wanted_vartype))
    {
        _scrip.write_cmd1(SCMD_CREATESTRING, SREG_AX); // convert AX
        _scrip.ax_vartype = _sym.VartypeWith(kVTT_Dynpointer, _sym.getStringStructSym());
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

int AGS::Parser::HandleStructOrArrayResult(AGS::Vartype &vartype, AGS::Parser::ValueLocation &vloc)
{
    if (_sym.IsArray(vartype))
    {
        cc_error("Cannot access array as a whole (did you forget to add \"[0]\"?)");
        return -1;
    }
    
    if (_sym.IsAtomic(vartype) && _sym.IsStruct(vartype))
    {
        if (_sym.IsManaged(vartype))
        {
            // Interpret the memory address as the result
            vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
            _scrip.write_cmd2(SCMD_REGTOREG, SREG_MAR, SREG_AX);
            vloc = kVL_ax_is_value;
            _scrip.ax_vartype = vartype;
            return 0;
        }

        cc_error("Cannot access non-managed struct as a whole");
        return -1;
    }

    return 0;
}

int AGS::Parser::ResultToAX(ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (kVL_mar_pointsto_value != vloc)
        return 0; // So it's already in AX 

    _scrip.ax_vartype = vartype;
    _scrip.ax_val_scope = scope;

    if (_sym.getOldStringSym() == _sym.VartypeWithout(kVTT_Const, vartype))
        _scrip.write_cmd2(SCMD_REGTOREG, SREG_MAR, SREG_AX);
    else
        _scrip.write_cmd1(
            _sym.IsDyn(vartype) ? SCMD_MEMREADPTR : GetReadCommandForSize(_sym.GetSize(vartype)),
            SREG_AX);
    return 0;
}

int AGS::Parser::ParseExpression_CheckArgOfNew(const AGS::SymbolScript &symlist, size_t symlist_len)
{
    if (symlist_len >= 2)
    {
        Vartype const new_vartype = symlist[1];
        if (_sym.GetSymbolType(new_vartype) == kSYM_Vartype)
            return 0;

        if (_sym.GetSymbolType(new_vartype) == kSYM_UndefinedStruct)
        {
            cc_error(
                "The struct %s hasn't been completely defined yet",
                _sym.GetName(new_vartype).c_str());
            return -1;
        }

        if (!_sym.IsPrimitive(new_vartype) && !_sym.IsManaged(new_vartype))
        {
            cc_error("Can only use primitive or managed types with 'new'");
            return -1;
        }

        if (FlagIsSet(_sym.GetFlags(new_vartype), kSFLG_Builtin))
        {
            cc_error(
                "Built-in type '%s' cannot be instantiated directly",
                _sym.GetName(new_vartype).c_str());
            return -1;
        }

        return 0;
    }
   
    cc_error("Expected a type after 'new'");
    return -1;
}

int AGS::Parser::ParseExpression_NewIsFirst(const AGS::SymbolScript &symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    int retval = ParseExpression_CheckArgOfNew(symlist, symlist_len);
    if (retval < 0) return retval;

    Vartype const new_vartype = vartype = symlist[1];
    
    if (symlist_len <= 3) // "new VARTYPE", nothing following
    {
        vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
        const size_t size = _sym.GetSize(new_vartype);
        _scrip.write_cmd2(SCMD_NEWUSEROBJECT, SREG_AX, size);
        _scrip.ax_val_scope = scope = kSYM_GlobalVar;
        _scrip.ax_vartype = vartype;
        vloc = kVL_ax_is_value;
        return 0;
    }

    // new VARTYPE[...]
    if (kSYM_OpenBracket == _sym.GetSymbolType(symlist[2]) && kSYM_CloseBracket == _sym.GetSymbolType(symlist[symlist_len - 1]))
    {
        bool const is_managed = !_sym.IsPrimitive(new_vartype);
        int const element_size = _sym.GetSize(new_vartype);
        
        // Expression for length of array begins after "[", ends before "]"
        // So expression_length = whole_length - 3 - 1
        retval = AccessData_ArrayIndexIntoAX(&symlist[3], symlist_len - 4);
        if (retval < 0) return retval;

        _scrip.write_cmd3(SCMD_NEWARRAY, SREG_AX, element_size, is_managed);

        if (is_managed)
            vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
        vartype = _sym.VartypeWith(kVTT_Dynarray, vartype);
        _scrip.ax_val_scope = scope = kSYM_GlobalVar;
        _scrip.ax_vartype = vartype;
        vloc = kVL_ax_is_value;
        return 0;
    }

    cc_error("Unexpected characters following 'new %s'", _sym.GetName(symlist[1]).c_str());
    return -1;
}

// We're parsing an expression that starts with '-' (unary minus)
int AGS::Parser::ParseExpression_UnaryMinusIsFirst(const AGS::SymbolScript &symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (symlist_len < 2)
    {
        cc_error("Parse error at '-'");
        return -1;
    }
    // parse the rest of the expression into AX
    int retval = ParseExpression_Subexpr(&symlist[1], symlist_len - 1, vloc, scope, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope, vartype);
    if (retval < 0) return retval;

    // now, subtract the result from 0 (which negates it)
    int cpuOp = SCMD_SUBREG; // get correct bytecode for the subtraction
    retval = GetOperatorValidForVartype(_scrip.ax_vartype, _sym.getIntSym(), cpuOp);
    if (retval < 0) return retval;

    _scrip.write_cmd2(SCMD_LITTOREG, SREG_BX, 0);
    _scrip.write_cmd2(cpuOp, SREG_BX, SREG_AX);
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_ax_is_value;
    return 0;
}

// We're parsing an expression that starts with '!' (boolean NOT)
int AGS::Parser::ParseExpression_NotIsFirst(const AGS::SymbolScript & symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (symlist_len < 2)
    {
        cc_error("Parse error at '!'");
        return -1;
    }

    // parse the rest of the expression
    int retval = ParseExpression_Subexpr(&symlist[1], symlist_len - 1, vloc, scope, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope, vartype);
    if (retval < 0) return retval;

    // negate the result
    // First determine the correct bytecode for the negation
    int cpuOp = SCMD_NOTREG;
    retval = GetOperatorValidForVartype(_scrip.ax_vartype, 0, cpuOp);
    if (retval < 0) return retval;

    // now, NOT the result
    _scrip.write_cmd1(cpuOp, SREG_AX);
    vloc = kVL_ax_is_value;
    return 0;
}

// The lowest-binding operator is the first thing in the expression
// This means that the op must be an unary op.
int AGS::Parser::ParseExpression_OpIsFirst(const AGS::SymbolScript &symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (kSYM_New == _sym.GetSymbolType(symlist[0]))
    {
        // we're parsing something like "new foo"
        return ParseExpression_NewIsFirst(symlist, symlist_len, vloc, scope, vartype);
    }

    int const cmd = _sym[symlist[0]].OpToVCPUCmd();
    if (cmd == SCMD_SUBREG)
    {
        // we're parsing something like "- foo"
        return ParseExpression_UnaryMinusIsFirst(symlist, symlist_len, vloc, scope, vartype);
    }

    if (cmd == SCMD_NOTREG)
    {
        // we're parsing something like "! foo"
        return ParseExpression_NotIsFirst(symlist, symlist_len, vloc, scope, vartype);
    }

    // All the other operators need a non-empty left hand side
    cc_error("Unexpected operator '%s' without a preceding expression", _sym.GetName(symlist[0]).c_str());
    return -1;
}

// The lowest-binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
int AGS::Parser::ParseExpression_OpIsSecondOrLater(size_t op_idx, const AGS::SymbolScript &symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    int vcpuOperator = _sym[symlist[op_idx]].OpToVCPUCmd();

    if (vcpuOperator == SCMD_NOTREG)
    {
        // you can't do   a = b ! c;
        cc_error("Invalid use of operator '!'");
        return -1;
    }

    if ((vcpuOperator == SCMD_SUBREG) &&
        (op_idx > 1) &&
        (kSYM_Operator == _sym.GetSymbolType(symlist[op_idx - 1])))
    {
        // We aren't looking at a subtraction; instead, the '-' is the unary minus of a negative value
        // Thus, the "real" operator must be further to the right, find it.
        op_idx = IndexOfLowestBondingOperator(symlist, op_idx);
        vcpuOperator = _sym[symlist[op_idx]].OpToVCPUCmd();
    }

    // process the left hand side
    // This will be in vain if we find out later on that there isn't any right hand side,
    // but doing the left hand side first means that any errors will be generated from left to right
    AGS::Vartype vartype_lhs = 0;
    int retval = ParseExpression_Subexpr(&symlist[0], op_idx, vloc, scope, vartype_lhs);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope, vartype_lhs);
    if (retval < 0) return retval;

    if (op_idx + 1 >= symlist_len)
    {
        // there is no right hand side for the expression
        cc_error("Parse error: invalid use of operator '%s'", _sym.GetName(symlist[op_idx]).c_str());
        return -1;
    }

    AGS::CodeLoc jump_dest_loc_to_patch = -1;
    if (vcpuOperator == SCMD_AND)
    {
        // "&&" operator lazy evaluation ... 
        // if AX is 0 then the AND has failed, 
        // so just jump directly past the AND instruction;
        // AX will still be 0 so that will do as the result of the calculation
        _scrip.write_cmd1(SCMD_JZ, 0);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        jump_dest_loc_to_patch = _scrip.codesize - 1;
    }
    else if (vcpuOperator == SCMD_OR)
    {
        // "||" operator lazy evaluation ... 
        // if AX is non-zero then the OR has succeeded, 
        // so just jump directly past the OR instruction; 
        // AX will still be non-zero so that will do as the result of the calculation
        _scrip.write_cmd1(SCMD_JNZ, 0);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        jump_dest_loc_to_patch = _scrip.codesize - 1;
    }

    _scrip.push_reg(SREG_AX);
    retval = ParseExpression_Subexpr(&symlist[op_idx + 1], symlist_len - (op_idx + 1), vloc, scope, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope, vartype);
    if (retval < 0) return retval;
    _scrip.pop_reg(SREG_BX); // <-- note, we pop to BX although we have pushed AX
    // now the result of the left side is in BX, of the right side is in AX

    // Check whether the left side type and right side type match either way
    retval = IsVartypeMismatch(vartype_lhs, vartype, false);
    if (retval < 0) return retval;

    retval = GetOperatorValidForVartype(vartype_lhs, vartype, vcpuOperator);
    if (retval < 0) return retval;

    _scrip.write_cmd2(vcpuOperator, SREG_BX, SREG_AX);
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
    vloc = kVL_ax_is_value;

    if (jump_dest_loc_to_patch >= 0)
    {
        _scrip.code[jump_dest_loc_to_patch] =
            ccCompiledScript::RelativeJumpDist(jump_dest_loc_to_patch, _scrip.codesize);
    }

    // Operators like == return a bool (in our case, that's an int);
    // other operators like + return the type that they're operating on
    if (IsBooleanVCPUOperator(vcpuOperator))
        _scrip.ax_vartype = vartype = _sym.getIntSym();
    return 0;
}

int AGS::Parser::ParseExpression_OpenParenthesis(AGS::SymbolScript & symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    int matching_paren_idx = -1;
    size_t paren_nesting_depth = 1; // we've already read a '('
    // find the corresponding closing parenthesis
    for (size_t idx = 1; idx < symlist_len; idx++)
    {
        switch (_sym.GetSymbolType(symlist[idx]))
        {
        default:
            continue;

        case kSYM_OpenParenthesis:
            paren_nesting_depth++;
            continue;

        case kSYM_CloseParenthesis:
            if (--paren_nesting_depth > 0)
                continue;

            matching_paren_idx = idx; // found the index of the matching ')'
            break;
        }

        break;
    }

    if (matching_paren_idx < 0)
    {
        cc_error("Open parenthesis does not have a matching close parenthesis");
        return -1;
    }

    if (matching_paren_idx <= 1)
    {
        cc_error("Unexpected \"()\"");
        return -1;
    }

    // Recursively compile the subexpression
    int retval = ParseExpression_Subexpr(&symlist[1], matching_paren_idx - 1, vloc, scope, vartype);
    if (retval < 0) return retval;

    symlist += matching_paren_idx + 1;
    symlist_len -= matching_paren_idx + 1;
    if (symlist_len > 0)
    {
        // there is some code after the ')'
        // this should not be possible unless the user does
        // something like "if ((x) 1234)", i.e. with an operator missing
        cc_error("Parse error: operator expected");
        return -1;
    }
    return 0;
}

// We're in the parameter list of a function call, and we have less parameters than declared.
// Provide defaults for the missing values
int AGS::Parser::AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import)
{
    for (size_t arg_idx = num_func_args; arg_idx > num_supplied_args; arg_idx--)
    {
        if (!_sym[funcSymbol].FuncParamHasDefaultValues[arg_idx])
        {
            cc_error("Function call parameter # %d isn't provided and does not have a default value", arg_idx);
            return -1;
        }

        // push the default value onto the stack
        _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, _sym[funcSymbol].FuncParamDefaultValues[arg_idx]);

        if (func_is_import)
            _scrip.write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            _scrip.push_reg(SREG_AX);
    }
    return 0;
}

void AGS::Parser::DoNullCheckOnStringInAXIfNecessary(AGS::Vartype valTypeTo)
{

    if (_sym.getStringStructSym() == _sym.VartypeWithout(kVTT_Dynpointer, _scrip.ax_vartype) &&
        _sym.getOldStringSym() == _sym.VartypeWithout(kVTT_Const, valTypeTo) )
        _scrip.write_cmd1(SCMD_CHECKNULLREG, SREG_AX);
}

std::string AGS::Parser::ReferenceMsg(std::string const &msg, std::string const &section, int line)
{
    if (line <= 0 || (!section.empty() && section[0] == '_'))
        return msg;

    std::string tpl = ". See line <2>";
    if (section.compare(ccCurScriptName) != 0)
    {
        tpl = ". See <1> line <2>";
        tpl.replace(tpl.find("<1>"), 3, section);
    }
    return msg + tpl.replace(tpl.find("<2>"), 3, std::to_string(line));
}

std::string AGS::Parser::ReferenceMsgSym(std::string const &msg, AGS::Symbol symb)
{
    return ReferenceMsg(
        msg,
        _sym.GetDeclaredSection(symb),
        _sym.GetDeclaredLine(symb));
}

int AGS::Parser::AccessData_FunctionCall_PushParams(const AGS::SymbolScript &paramList, size_t closedParenIdx, size_t num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import)
{
    size_t param_num = num_supplied_args + 1;
    size_t start_of_this_param = 0;
    int end_of_this_param = closedParenIdx;  // can become < 0, points to (last byte of parameter + 1)
    // Go backwards through the parameters since they must be pushed that way
    do
    {
        // Find the start of the next parameter
        param_num--;
        int paren_nesting_depth = 0;
        for (size_t paramListIdx = end_of_this_param - 1; true; paramListIdx--)
        {
            // going backwards so ')' increases the depth level
            const SymbolType idx_type = _sym.GetSymbolType(paramList[paramListIdx]);
            if (idx_type == kSYM_CloseParenthesis)
                paren_nesting_depth++;
            if (idx_type == kSYM_OpenParenthesis)
                paren_nesting_depth--;
            if ((paren_nesting_depth == 0 && idx_type == kSYM_Comma) ||
                (paren_nesting_depth < 0 && idx_type == kSYM_OpenParenthesis))
            {
                start_of_this_param = paramListIdx + 1;
                break;
            }
            if (paramListIdx == 0)
                break; // Don't put this into the for header!
        }

        if (end_of_this_param < static_cast<int>(start_of_this_param))
        {
            cc_error("Internal error: parameter length is negative");
            return -99;
        }

        // Compile the parameter
        ValueLocation vloc;
        int scope;
        AGS::Vartype vartype;

        int retval = ParseExpression_Subexpr(&paramList[start_of_this_param], end_of_this_param - start_of_this_param, vloc, scope, vartype);
        if (retval < 0) return retval;
        retval = ResultToAX(vloc, scope, vartype);
        if (retval < 0) return retval;

        if (param_num <= num_func_args) // we know what type to expect
        {
            // If we need a string object ptr but AX contains a normal string, convert AX
            Vartype const param_vartype = _sym[funcSymbol].FuncParamTypes[param_num];
            ConvertAXStringToStringObject(param_vartype);
            vartype = _scrip.ax_vartype;
            // If we need a normal string but AX contains a string object ptr, 
            // check that this ptr isn't null
            DoNullCheckOnStringInAXIfNecessary(param_vartype);

            if (IsVartypeMismatch(vartype, param_vartype, true))
                return -1;
        }

        // Note: We push the parameters, which is tantamount to writing them
        // into memory with SCMD_MEMWRITE. The called function will use them
        // as local variables. However, if a parameter is managed, then its 
        // memory must be written with SCMD_MEMWRITEPTR, not SCMD_MEMWRITE 
        // as we do here. So to compensate, the called function will have to 
        // read each pointer variable with SCMD_MEMREAD and then write it
        // back with SCMD_MEMWRITEPTR.

        if (func_is_import)
            _scrip.write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            _scrip.push_reg(SREG_AX);

        end_of_this_param = start_of_this_param - 1;
    }
    while (end_of_this_param > 0);

    return 0;
}


// Count parameters, check that all the parameters are non-empty; find closing paren
int AGS::Parser::AccessData_FunctionCall_CountAndCheckParm(const AGS::SymbolScript &paramList, size_t paramListLen, AGS::Symbol funcSymbol, size_t &indexOfCloseParen, size_t &num_supplied_args)
{
    size_t paren_nesting_depth = 1;
    num_supplied_args = 1;
    size_t paramListIdx;
    bool found_param_symbol = false;

    for (paramListIdx = 1; paramListIdx < paramListLen; paramListIdx++)
    {
        const SymbolType idx_type = _sym.GetSymbolType(paramList[paramListIdx]);

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

            cc_error("Argument %d in function call is empty", num_supplied_args - 1);
            return -1;
        }
        found_param_symbol = true;
    }

    // Special case: "()" means 0 arguments
    if (num_supplied_args == 1 &&
        paramListLen > 1 &&
        _sym.GetSymbolType(paramList[1]) == kSYM_CloseParenthesis)
    {
        num_supplied_args = 0;
    }

    indexOfCloseParen = paramListIdx;

    if (_sym.GetSymbolType(paramList[indexOfCloseParen]) != kSYM_CloseParenthesis)
    {
        cc_error("Missing ')' at the end of the parameter list");
        return -1;
    }

    if (indexOfCloseParen > 0 &&
        _sym.GetSymbolType(paramList[indexOfCloseParen - 1]) == kSYM_Comma)
    {
        cc_error("Last argument in function call is empty");
        return -1;
    }

    if (indexOfCloseParen < paramListLen - 1 &&
        _sym.GetSymbolType(paramList[indexOfCloseParen + 1]) != kSYM_Semicolon)
    {
        cc_error("Internal error: Unexpected symbols trailing the parameter list");
        return -1;
    }

    if (paren_nesting_depth > 0)
    {
        cc_error("Internal error: Parser confused near '%s'", _sym.GetName(funcSymbol).c_str());
        return -1;
    }

    return 0;
}

// We are processing a function call. General the actual function call
void AGS::Parser::AccessData_GenerateFunctionCall(AGS::Symbol name_of_func, size_t num_args, bool func_is_import)
{
    if (func_is_import)
    {
        // tell it how many args for this call (nested imported functions
        // cause stack problems otherwise)
        _scrip.write_cmd1(SCMD_NUMFUNCARGS, num_args);
    }

    // Call the function: Get address into AX
    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, _sym[name_of_func].SOffset);

    if (func_is_import)
    {
        if (!_importMgr.IsDeclaredImport(_sym.GetName(name_of_func)))
            _fim.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1);
        _scrip.fixup_previous(kFx_Import);
        _scrip.write_cmd1(SCMD_CALLEXT, SREG_AX); // do the call
        // At runtime, we will arrive here when the function call has returned
        // restore the stack
        if (num_args > 0)
            _scrip.write_cmd1(SCMD_SUBREALSTACK, num_args);
        return;
    }

    // Func is non-import
    if (_fcm.IsForwardDecl(name_of_func))
        _fcm.TrackForwardDeclFuncCall(name_of_func, _scrip.codesize - 1);
    _scrip.fixup_previous(kFx_Code);
    _scrip.write_cmd1(SCMD_CALL, SREG_AX);  // do the call

    // At runtime, we will arrive here when the function call has returned
    // restore the stack
    if (num_args > 0)
    {
        _scrip.cur_sp -= num_args * 4;
        _scrip.write_cmd2(SCMD_SUB, SREG_SP, num_args * 4);
    }
}

// We are processing a function call.
// Get the parameters of the call and push them onto the stack.
// Return the number of the parameters pushed
int AGS::Parser::AccessData_PushFunctionCallParams(AGS::Symbol name_of_func, bool func_is_import, AGS::SymbolScript &paramList, size_t paramListLen, size_t &actual_num_args)
{
    // Expected number of arguments, or expected minimal number of arguments
    size_t const num_func_args = _sym[name_of_func].GetNumOfFuncArgs();
    bool const func_is_varargs = _sym[name_of_func].IsVarargsFunc();

    size_t num_supplied_args = 0;
    size_t indexOfClosedParen;
    int retval = AccessData_FunctionCall_CountAndCheckParm(paramList, paramListLen, name_of_func, indexOfClosedParen, num_supplied_args);
    if (retval < 0) return retval;

    // Push default parameters onto the stack when applicable
    // This will give an error if there aren't enough default parameters
    if (num_supplied_args < num_func_args)
    {
        retval = AccessData_FunctionCall_ProvideDefaults(num_func_args, num_supplied_args, name_of_func, func_is_import);
        if (retval < 0) return retval;
    }
    if (num_supplied_args > num_func_args && !func_is_varargs)
    {
        cc_error("Expected just %d parameters but found %d", num_func_args, num_supplied_args);
        return -1;
    }
    // ASSERT at this point, the number of parameters is okay

    // Push the explicit arguments of the function
    if (num_supplied_args > 0)
    {
        retval = AccessData_FunctionCall_PushParams(paramList, indexOfClosedParen, num_func_args, num_supplied_args, name_of_func, func_is_import);
        if (retval < 0) return retval;
    }

    actual_num_args = std::max(num_supplied_args, num_func_args);
    return 0;
}

int AGS::Parser::AccessData_FunctionCall(AGS::Symbol name_of_func, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::MemoryLocation &mloc, AGS::Vartype &rettype)
{
    if (_sym.GetSymbolType(symlist[1]) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }

    AGS::SymbolScript paramList = symlist + 1;
    size_t paramListLen = symlist_len - 1;

    bool const func_is_import = FlagIsSet(_sym.GetFlags(name_of_func), kSFLG_Imported);
    // If function uses normal stack, we need to do stack calculations to get at certain elements
    bool const func_uses_normal_stack = !func_is_import;
    bool func_uses_this = false;
    if (std::string::npos != _sym.GetName(name_of_func).find("::"))
        func_uses_this = !FlagIsSet(_sym.GetFlags(name_of_func), kSFLG_Static);

    if (func_uses_this)
    {
        if (0 != _sym.GetVartype(_sym.getThisSym()))
            _scrip.push_reg(SREG_OP); // Save OP since we must restore it after the func call

        // Get address of outer into MAR; that's what the func will use as its "this"
        mloc.MakeMARCurrent(_scrip);
        _scrip.push_reg(SREG_MAR);
    }

    size_t num_args = 0;
    int retval = AccessData_PushFunctionCallParams(name_of_func, func_is_import, paramList, paramListLen, num_args);
    if (retval < 0) return retval;

    if (func_uses_this)
    {
        if (0 == num_args)
        {   // Undo unneeded PUSH
            _scrip.cur_sp -= 4;
            _scrip.codesize -= 2; 
        }
        else
        {
            _scrip.write_cmd1(
                SCMD_LOADSPOFFS,
                (func_uses_normal_stack ? num_args : 0) * 4 + 4);
        }
        _scrip.write_cmd1(SCMD_CALLOBJ, SREG_MAR);
    }

    AccessData_GenerateFunctionCall(name_of_func, num_args, func_is_import);

    // function return type
    rettype = _scrip.ax_vartype = _sym[name_of_func].FuncParamTypes[0];
    _scrip.ax_val_scope = kSYM_LocalVar;

    // At runtime, we have returned from the func call,
    // so we must continue with the object pointer that was in use before the call
    if (func_uses_this)
    {
        if (0 < num_args)
            _scrip.pop_reg(SREG_MAR);
        if (0 != _sym.GetVartype(_sym.getThisSym()))
            _scrip.pop_reg(SREG_OP);
    }

    MarkAcessed(name_of_func);
    return 0;
}

int AGS::Parser::ParseExpression_NoOps(AGS::SymbolScript symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (kSYM_OpenParenthesis == _sym.GetSymbolType(symlist[0]))
        return ParseExpression_OpenParenthesis(symlist, symlist_len, vloc, scope, vartype);

    if (kSYM_Operator != _sym.GetSymbolType(symlist[0]))
        return AccessData(false, false, symlist, symlist_len, vloc, scope, vartype);

    // The operator at the beginning must be a unary minus
    if (SCMD_SUBREG == _sym[symlist[0]].OpToVCPUCmd())
    {
        size_t len_minus_1 = symlist_len - 1;
        SymbolScript symlist1 = symlist + 1;
        return AccessData(false, true, symlist1, len_minus_1, vloc, scope, vartype);
    }
    cc_error("Parse error: Unexpected '%s'", _sym.GetName(symlist[0]).c_str());
    return -1;
}

int AGS::Parser::ParseExpression_Subexpr(AGS::SymbolScript symlist, size_t symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (symlist_len == 0)
    {
        cc_error("Internal error: Cannot parse empty subexpression");
        return -1;
    }
    if (kSYM_CloseBracket == _sym.GetSymbolType(symlist[0]))
    {
        cc_error("Unexpected ')' at start of expression");
        return -1;
    }

    int lowest_op_idx = IndexOfLowestBondingOperator(symlist, symlist_len);  // can be < 0

    // If the lowest bonding operator is '-' and right in front,
    // then it has been misinterpreted so far: it's really a unary minus
    if ((lowest_op_idx == 0) &&
        (symlist_len > 1) &&
        (_sym[symlist[0]].OpToVCPUCmd() == SCMD_SUBREG))
    {
        lowest_op_idx = IndexOfLowestBondingOperator(&symlist[1], symlist_len - 1);
        if (lowest_op_idx >= 0)
            lowest_op_idx++;
    }

    int retval = 0;
    if (lowest_op_idx == 0)
        retval = ParseExpression_OpIsFirst(symlist, symlist_len, vloc, scope, vartype);
    else if (lowest_op_idx > 0)
        retval = ParseExpression_OpIsSecondOrLater(static_cast<size_t>(lowest_op_idx), symlist, symlist_len, vloc, scope, vartype);
    else
        retval = ParseExpression_NoOps(symlist, symlist_len, vloc, scope, vartype);
    if (retval < 0) return retval;
    return HandleStructOrArrayResult(vartype, vloc);
}

// symlist starts a bracketed expression; parse it
int AGS::Parser::AccessData_ArrayIndexIntoAX(SymbolScript symlist, size_t symlist_len)
{
    ValueLocation vloc;
    int scope;
    AGS::Vartype vartype;
    int retval = ParseExpression_Subexpr(symlist, symlist_len, vloc, scope, vartype);
    if (retval < 0) return retval;
    retval = ResultToAX(vloc, scope, vartype);
    if (retval < 0) return retval;

    return IsVartypeMismatch(vartype, _sym.getIntSym(), true);
}

// We access a variable or a component of a struct in order to read or write it.
// This is a simple member of the struct.
int AGS::Parser::AccessData_StructMember(AGS::Symbol component, bool writing, bool access_via_this, SymbolScript &symlist, size_t &symlist_len, AGS::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    SymbolTableEntry &entry = _sym[component];

    if (writing && FlagIsSet(entry.Flags, kSFLG_WriteProtected) && !access_via_this)
    {
        cc_error(
            "Writeprotected component '%s' must not be modified from outside",
            _sym.GetName(component).c_str());
        return -1;
    }
    if (FlagIsSet(entry.Flags, kSFLG_Protected) && !access_via_this)
    {
        cc_error(
            "Protected component '%s' must not be accessed from outside",
            _sym.GetName(component).c_str());
        return -1;
    }

    mloc.AddComponentOffset(entry.SOffset);
    vartype = _sym.GetVartype(component);
    symlist++;
    symlist_len--;
    return 0;
}

// Get the symbol for the get or set function corresponding to the attribute given.
int AGS::Parser::ConstructAttributeFuncName(AGS::Symbol attribsym, bool writing, bool indexed, AGS::Symbol &func)
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
    return 0;
}

// We call the getter or setter of an attribute
int AGS::Parser::AccessData_Attribute(bool is_attribute_set_func, SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    AGS::Symbol const component = symlist[0];
    symlist++;  // eat component
    symlist_len--; // eat component ct'd.
    AGS::Symbol const struct_of_component =
        AccessData_FindStructOfComponent(Vartype2Symbol(vartype), component);
    if (0 == struct_of_component)
    {
        cc_error(
            "Struct '%s' does not have an attribute named '%s'",
            _sym.GetName(Vartype2Symbol(vartype)).c_str(),
            _sym.GetName(component).c_str());
        return -1;
    }
    AGS::Symbol const name_of_attribute = MangleStructAndComponent(struct_of_component, component);

    bool const attrib_uses_this =
        !FlagIsSet(_sym.GetFlags(name_of_attribute), kSFLG_Static);
    bool const call_is_indexed =
        (symlist_len > 0 && kSYM_OpenBracket == _sym.GetSymbolType(symlist[0]));
    bool const attrib_is_indexed =
        _sym.IsDynarray(name_of_attribute);

    if (call_is_indexed != attrib_is_indexed)
    {
        cc_error(call_is_indexed ? "Unexpected '['" : "'[' expected but not found");
        return -1;
    }

    // Get the appropriate access function (as a symbol)
    AGS::Symbol name_of_func = -1;
    int retval = ConstructAttributeFuncName(component, is_attribute_set_func, attrib_is_indexed, name_of_func);
    if (retval < 0) return retval;
    name_of_func = MangleStructAndComponent(struct_of_component, name_of_func);
    if (name_of_func < 0) return retval;

    bool const func_is_import = FlagIsSet(_sym.GetFlags(name_of_func), kSYM_Import);

    if (attrib_uses_this)
        _scrip.push_reg(SREG_OP); // is the current this ptr, must be restored after call

    size_t num_of_args = 0;
    if (is_attribute_set_func)
    {
        if (func_is_import)
            _scrip.write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            _scrip.push_reg(SREG_AX);
        ++num_of_args;
    }

    if (call_is_indexed)
    {
        // The index to be set is in the [...] clause; push it as the first parameter
        if (attrib_uses_this)
            _scrip.push_reg(SREG_MAR); // must not be clobbered
        retval = AccessData_ArrayIndexIntoAX(&symlist[1], symlist_len - 2);
        if (retval < 0) return retval;
        symlist += symlist_len; // Eat the index
        symlist_len = 0; // Eat the index, ct'd
        if (attrib_uses_this)
            _scrip.pop_reg(SREG_MAR);

        if (func_is_import)
            _scrip.write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            _scrip.push_reg(SREG_AX);
        ++num_of_args;
    }

    if (attrib_uses_this)
        _scrip.write_cmd1(SCMD_CALLOBJ, SREG_MAR); // make MAR the new this ptr

    AccessData_GenerateFunctionCall(name_of_func, num_of_args, func_is_import);
    
    if (attrib_uses_this)
        _scrip.pop_reg(SREG_OP); // restore old this ptr after the func call

    // attribute return type
    _scrip.ax_val_scope = kSYM_LocalVar;
    _scrip.ax_vartype = vartype = _sym.GetVartype(name_of_func);

    MarkAcessed(name_of_attribute);
    MarkAcessed(name_of_func);
    return 0;
}


// Location contains a pointer to another address. Get that address.
int AGS::Parser::AccessData_Dereference(ValueLocation &vloc, AGS::MemoryLocation &mloc)
{
    if (kVL_ax_is_value == vloc)
    {
        _scrip.write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        _scrip.write_cmd0(SCMD_CHECKNULL);
        vloc = kVL_mar_pointsto_value;
        mloc.Reset();
    }
    else
    {
        mloc.MakeMARCurrent(_scrip);
        // Note: We need to check here whether m[MAR] == 0, but CHECKNULL
        // checks whether MAR == 0. So we need to do MAR := m[MAR] first.
        _scrip.write_cmd1(SCMD_MEMREADPTR, SREG_MAR);
        _scrip.write_cmd0(SCMD_CHECKNULL);
    }
    return 0;
}

int AGS::Parser::AccessData_ProcessArrayIndexConstant(AGS::Symbol index_symbol, size_t num_array_elements, size_t element_size, AGS::MemoryLocation &mloc)
{
    int array_index = -1;
    int retval = ParseLiteralOrConstvalue(index_symbol, array_index, false, "Error parsing integer");
    if (retval < 0) return retval;
    if (array_index < 0)
    {
        cc_error("Array index %d out of bounds (minimum is 0)", array_index);
        return -1;
    }
    if (num_array_elements > 0 && static_cast<size_t>(array_index) >= num_array_elements)
    {
        cc_error(
            "Array index %d out of bounds (maximum is %d)",
            array_index,
            static_cast<int>(num_array_elements - 1));
        return -1;
    }
    
    mloc.AddComponentOffset(array_index * element_size);
    return 0;
}

// We're processing some struct component or global or local variable.
// If an array index follows, parse it and shorten symlist accordingly
int AGS::Parser::AccessData_ProcessAnyArrayIndex(ValueLocation vloc_of_array, size_t num_array_elements, SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, AGS::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    if (0 == symlist_len || kSYM_OpenBracket != _sym.GetSymbolType(symlist[0]))
        return 0;

    // Find the location of the close bracket
    Symbol *close_brac_loc = symlist + 1;
    size_t len_starting_with_close_brac = symlist_len - 1;
    SymbolType stoplist[] = { kSYM_NoType, };
    SkipToScript(stoplist, 0, close_brac_loc, len_starting_with_close_brac);
    if (len_starting_with_close_brac == 0)
    {
        cc_error("Internal error: '[' has no matching ']");
        return -99;
    }
    size_t bracketed_expr_length = close_brac_loc + 1 - symlist;

    bool const is_dynarray = _sym.IsDynarray(vartype);
    bool const is_array = _sym.IsArray(vartype);
    if (!is_dynarray && !is_array)
    {
        cc_error("Array index is only legal after an array");
        return -1;
    }

    AGS::Vartype element_vartype = _sym[vartype].vartype;
    
    size_t const element_size = _sym.GetSize(element_vartype);
    vartype = element_vartype;

    if (is_dynarray)
        AccessData_Dereference(vloc, mloc);

    // Ideally, we would calculate compile time constants here. For now, only
    // process the special case [INT] where INT is a non-negative integer or constant.
    if (3 == bracketed_expr_length &&
        (kSYM_LiteralInt == _sym.GetSymbolType(symlist[1]) || kSYM_Const == _sym.GetSymbolType(symlist[1])))
    {
        int retval = AccessData_ProcessArrayIndexConstant(symlist[1], num_array_elements, element_size, mloc);
        if (retval < 0) return retval;
        symlist = close_brac_loc + 1;
        symlist_len = len_starting_with_close_brac - 1;
        return 0;
    }

    // Save MAR from being clobbered if it may contain something important
    bool mar_pushed = false;
    if (!mloc.NothingDoneYet()) // if nothing is done, it can't have been set yet
    {
        mloc.MakeMARCurrent(_scrip);
        _scrip.push_reg(SREG_MAR);
        mar_pushed = true;
    }

    int retval = AccessData_ArrayIndexIntoAX(symlist + 1, bracketed_expr_length - 2);
    if (retval < 0) return retval;

    if (mar_pushed)
        _scrip.pop_reg(SREG_MAR);
    mloc.MakeMARCurrent(_scrip);

    // Note: SCMD_DYNAMICBOUNDS checks that the register is between  0 and
    // the length of the memory chunk in bytes (NOT the number of elements
    // in the array!). SCMD_CHECKBOUNDS checks that the register is
    // between 0 and the number given in the command. So to keep them in
    // parallel, the check must be done in both cases on the length of
    // the memory chunk that makes up the array, and the number given in
    // the CHECKBOUNDS command must be this length of the memory chunk.
    // If array_size == 0, we don't know the array size and can't check.
    if (element_size != 1)
        _scrip.write_cmd2(SCMD_MUL, SREG_AX, element_size); // Multiply offset with length of one array entry
    if (is_dynarray)
        _scrip.write_cmd1(SCMD_DYNAMICBOUNDS, SREG_AX);
    else if (num_array_elements > 0)
        _scrip.write_cmd2(SCMD_CHECKBOUNDS, SREG_AX, num_array_elements * element_size);
    _scrip.write_cmd2(SCMD_ADDREG, SREG_MAR, SREG_AX); // Add offset

    symlist = close_brac_loc + 1;
    symlist_len = len_starting_with_close_brac - 1;
    return 0;
}

int AGS::Parser::AccessData_GlobalOrLocalVar(bool is_global, bool writing, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::MemoryLocation &mloc, AGS::Vartype &vartype)
{
    AGS::Symbol const varname = symlist[0];
    SymbolTableEntry &entry = _sym[varname];
    AGS::CodeCell const soffs = entry.SOffset;
    symlist++;
    symlist_len--;

    if (writing && FlagIsSet(entry.Flags, kSFLG_Readonly))
    {
        cc_error("Cannot write to readonly '%s'", _sym.GetName(varname).c_str());
        return -1;
    }

    if (FlagIsSet(_sym.GetFlags(varname), kSFLG_Imported))
        mloc.SetStart(kSYM_Import, soffs);
    else
        mloc.SetStart(is_global ? kSYM_GlobalVar : kSYM_LocalVar, soffs);

    vartype = _sym.GetVartype(varname);

    // Process an array index if it follows
    ValueLocation vl_dummy = kVL_mar_pointsto_value;
    return AccessData_ProcessAnyArrayIndex(kVL_mar_pointsto_value, _sym.NumArrayElements(varname), symlist, symlist_len, vl_dummy, mloc, vartype);
}

int AGS::Parser::AccessData_Static(AGS::SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    vartype = symlist[0];
    symlist++;
    symlist_len--;
    mloc.Reset();
    return 0;
}

int AGS::Parser::AccessData_LitFloat(bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    char *endptr;
    std::string float_as_string = _sym.GetName(symlist[0]);
    char const *instring = float_as_string.c_str();
    double const d = strtod(instring, &endptr);
    if (endptr != instring + _sym.GetName(symlist[0]).length())
    {
        cc_error("Illegal floating point literal '%s'", instring);
        return -1;
    }
    if (HUGE_VAL == d)
    {
        cc_error("Floating point literal '%s' is out of range", instring);
        return -1;
    }
    float const f = static_cast<float>(d * (negate ? -1 : 1));
    int const i = InterpretFloatAsInt(f);

    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, i);
    _scrip.ax_vartype = vartype = _sym.getFloatSym();
    _scrip.ax_val_scope = kSYM_GlobalVar;
    symlist++;
    symlist_len--;
    return 0;
}

int AGS::Parser::AccessData_LitOrConst(bool negateLiteral, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    int varSymValue;
    int retval = ParseLiteralOrConstvalue(symlist[0], varSymValue, negateLiteral, "Error parsing integer value");
    if (retval < 0) return retval;
    symlist++;
    symlist_len--;

    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, varSymValue);
    _scrip.ax_vartype = vartype = _sym.getIntSym();
    _scrip.ax_val_scope = kSYM_GlobalVar;

    return 0;
}

int AGS::Parser::AccessData_Null(bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    if (negate)
    {
        cc_error("'-null' is undefined");
        return -1;
    }

    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    _scrip.ax_vartype = vartype = _sym.getNullSym();
    _scrip.ax_val_scope = kSYM_GlobalVar;
    symlist++;
    symlist_len--;

    return 0;
}

int AGS::Parser::AccessData_String(bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    if (negate)
    {
        cc_error("The negative value of a string is undefined");
        return -1;
    }

    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, _sym[symlist[0]].SOffset);
    _scrip.fixup_previous(kFx_String);
    _scrip.ax_vartype = vartype = _sym.VartypeWith(kVTT_Const, _sym.getOldStringSym())
        ;
    symlist++;
    symlist_len--;
    return 0;
}

// Negates the value; this clobbers AX and BX
void AGS::Parser::AccessData_Negate(ValueLocation vloc)
{
    _scrip.write_cmd2(SCMD_LITTOREG, SREG_BX, 0);
    if (kVL_mar_pointsto_value == vloc)
        _scrip.write_cmd1(SCMD_MEMREAD, SREG_AX);
    _scrip.write_cmd2(SCMD_SUBREG, SREG_BX, SREG_AX);
    if (kVL_mar_pointsto_value == vloc)
        _scrip.write_cmd1(SCMD_MEMWRITE, SREG_BX);
    else
        _scrip.write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
}

// We're getting a variable, literal, constant, func call or the first element
// of a STRUCT.STRUCT.STRUCT... cascade.
// This moves symlist in all cases except for the cascade to the end of what is parsed,
// and in case of a cascade, to the end of the first element of the cascade, i.e.,
// to the position of the '.'. 
int AGS::Parser::AccessData_FirstClause(bool writing, AGS::SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, AGS::MemoryLocation &mloc, AGS::Vartype &vartype, bool &access_via_this, bool &static_access, bool &need_to_negate)
{
    if (symlist_len < 1)
    {
        cc_error("Internal error: Empty variable");
        return -99;
    }

    // In many cases, we can handle the negation here, so we reset the flag.
    // If we find that we _cannot_ handle negation later on, we'll undo that
    bool const input_negate = need_to_negate;
    need_to_negate = false;

    if (_sym.getThisSym() == symlist[0])
    {
        vartype = _sym.GetVartype(_sym.getThisSym());
        if (0 == vartype)
        {
            cc_error("'this' is only legal in non-static struct functions");
            return -1;
        }
        vloc = kVL_mar_pointsto_value;
        _scrip.write_cmd2(SCMD_REGTOREG, SREG_OP, SREG_MAR);
        _scrip.write_cmd0(SCMD_CHECKNULL);
        mloc.Reset();
        access_via_this = true;
        symlist++;
        symlist_len--;
        return 0;
    }

    switch (_sym.GetSymbolType(symlist[0]))
    {
    default:
    {
        // If this unknown symbol can be interpreted as a component of this,
        // treat it that way.
        vartype = _sym.GetVartype(_sym.getThisSym());
        AGS::Symbol const thiscomponent = MangleStructAndComponent(vartype, symlist[0]);
        if (0 != _sym[thiscomponent].SType)
        {
            vloc = kVL_mar_pointsto_value;
            _scrip.write_cmd2(SCMD_REGTOREG, SREG_OP, SREG_MAR);
            _scrip.write_cmd0(SCMD_CHECKNULL);
            mloc.Reset();
            access_via_this = true;

            // We _should_ prepend "this." to symlist here but can't do that (easily).
            // So we don't and the '.' that should be prepended doesn't exist.
            // To compensate for that, we back up symlist by one index
            symlist--;
            symlist_len++;
            return 0;
        }

        cc_error("Unexpected '%s'", _sym.GetName(symlist[0]).c_str());
        return -1;
    }

    case kSYM_Constant:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_LitOrConst(input_negate, symlist, symlist_len, vartype);

    case kSYM_Function:
    {
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        need_to_negate = input_negate;
        int retval = AccessData_FunctionCall(symlist[0], symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;
        int const zero_array_size_dummy = 0; // A function cannot return a static array.
        if (_sym.IsDynarray(vartype))
            return AccessData_ProcessAnyArrayIndex(vloc, zero_array_size_dummy, symlist, symlist_len, vloc, mloc, vartype);
        return 0;
    }

    case kSYM_GlobalVar:
    {
        scope = kSYM_GlobalVar;
        vloc = kVL_mar_pointsto_value;
        bool const is_global = true;
        need_to_negate = input_negate;
        MarkAcessed(symlist[0]);
        return AccessData_GlobalOrLocalVar(is_global, writing, symlist, symlist_len, mloc, vartype);
    }

    case kSYM_LiteralFloat:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_LitFloat(input_negate, symlist, symlist_len, vartype);

    case kSYM_LiteralInt:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_LitOrConst(input_negate, symlist, symlist_len, vartype);

    case kSYM_LiteralString:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_String(need_to_negate, symlist, symlist_len, vartype);

    case kSYM_LocalVar:
    {
        scope =
            (FlagIsSet(_sym.GetFlags(symlist[0]), kSFLG_Parameter)) ?
            kSYM_GlobalVar : kSYM_LocalVar;
        vloc = kVL_mar_pointsto_value;
        bool const is_global = false;
        need_to_negate = input_negate;
        return AccessData_GlobalOrLocalVar(is_global, writing, symlist, symlist_len, mloc, vartype);
    }

    case kSYM_Null:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_Null(need_to_negate, symlist, symlist_len, vartype);

    case kSYM_Vartype:
        scope = kSYM_GlobalVar;
        static_access = true;
        need_to_negate = input_negate;
        return AccessData_Static(symlist, symlist_len, mloc, vartype);
    }

    cc_error("Cannot assign a value to '%s'", _sym.GetName(symlist[0]).c_str());
    return -1;
}

// We're processing a STRUCT.STRUCT. ... clause.
// We've already processed some structs, and the type of the last one is vartype.
// Now we process a component of vartype.
int AGS::Parser::AccessData_SubsequentClause(bool writing, bool access_via_this, bool static_access, AGS::SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    AGS::Symbol const component = AccessData_FindComponent(Vartype2Symbol(vartype), symlist[0]);
    SymbolType const component_type = (component) ? _sym.GetSymbolType(component) : kSYM_NoType;

    if (static_access && !FlagIsSet(_sym.GetFlags(component), kSFLG_Static))
    {
        cc_error("Must specify a specific struct for this non-static component");
        return -1;
    }

    int retval = 0;
    switch (component_type)
    {
    default:
        cc_error(
            "Expected a component of '%s', found '%s' instead",
            _sym.GetName(vartype).c_str(),
            _sym.GetName(symlist[0]).c_str());
        return -1;

    case kSYM_Attribute:
    {
        mloc.MakeMARCurrent(_scrip); // make MAR point to the struct of the attribute
        if (writing)
        {
            // We cannot process the attribute here so return to the assignment that
            // this attribute was originally called from
            vartype = _sym.GetVartype(component);
            vloc = kVL_attribute;
            return 0;
        }
        vloc = kVL_ax_is_value;
        bool const is_attribute_set_func = false;
        return AccessData_Attribute(is_attribute_set_func, symlist, symlist_len, vartype);
    }

    case kSYM_Function:
    {
        vloc = kVL_ax_is_value;
        scope = kSYM_LocalVar;
        retval = AccessData_FunctionCall(component, symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;
        int const zero_array_size_dummy = 0; // A function cannot return a static array.
        if (_sym.IsDynarray(vartype))
            return AccessData_ProcessAnyArrayIndex(vloc, zero_array_size_dummy, symlist, symlist_len, vloc, mloc, vartype);
        return 0;
    }

    case kSYM_StructComponent:
        vloc = kVL_mar_pointsto_value;
        retval = AccessData_StructMember(component, writing, access_via_this, symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;
        return AccessData_ProcessAnyArrayIndex(vloc, _sym.NumArrayElements(component), symlist, symlist_len, vloc, mloc, vartype);
    }

    return 0; // Can't reach
}

AGS::Symbol AGS::Parser::AccessData_FindStructOfComponent(AGS::Vartype strct, AGS::Symbol component)
{
    do
    {
        AGS::Symbol symb = MangleStructAndComponent(strct, component);
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
        AGS::Symbol ret = MangleStructAndComponent(strct, component);
        if (kSYM_NoType != _sym.GetSymbolType(ret))
            return ret;
        strct = _sym[strct].Extends;
    }
    while (strct > 0);
    return 0;
}

// We are in a STRUCT.STRUCT.STRUCT... cascade.
// Check whether we have passed the last dot
int AGS::Parser::AccessData_IsClauseLast(AGS::SymbolScript symlist, size_t symlist_len, bool &is_last)
{
    SymbolType const stoplist[] = { kSYM_Dot };
    SkipToScript(stoplist, 1, symlist, symlist_len);
    is_last = (0 == symlist_len || kSYM_Dot != _sym.GetSymbolType(symlist[0]));
    return 0;
}

// Access a variable, constant, literal, func call, struct.component.component cascade, etc.
// Result is in AX or m[MAR], dependent on vloc. Type is in vartype.
// At end of function, symlist and symlist_len will point to the part of the symbol string
// that has not been processed yet
// NOTE: If this selects an attribute for writing, then the corresponding function will
// _not_ be called and symlist[0] will be the attribute.
int AGS::Parser::AccessData(bool writing, bool need_to_negate, AGS::SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (symlist_len == 0)
    {
        cc_error("Internal error: empty expression");
        return -99;
    }
    if (writing && need_to_negate)
    {
        cc_error("Can't apply unary minus to a value you are modifying");
        return -1;
    }

    // For memory accesses, we set the MAR register lazily so that we can
    // accumulate offsets at runtime instead of compile time.
    // This struct tracks what we will need to do to set the MAR register.
    AGS::MemoryLocation mloc = MemoryLocation();

    bool clause_is_last = false;
    int retval = AccessData_IsClauseLast(symlist, symlist_len, clause_is_last);
    if (retval < 0) return retval;

    bool access_via_this = false; // only true when "this" has just been parsed
    bool static_access = false; // only true when a vartype has just been parsed

    // If we are reading, then all the accesses are for reading.
    // If we are writing, then all the accesses except for the last one
    // are for reading and the last one will be for writing.
    retval = AccessData_FirstClause((writing && clause_is_last), symlist, symlist_len, vloc, scope, mloc, vartype, access_via_this, static_access, need_to_negate);
    if (retval < 0) return retval;

    AGS::Vartype outer_vartype = 0;

    // Unfortunately, the while condition is ugly:
    // Normally, the while body must be executed whenever symlist starts with a '.'.
    // However, if the previous function has assumed a "this." that isn't there,
    // then symlist won't start with a '.' but the while body must be executed anyway.
    // This is why the while condition has "access_via_this" in it.
    while (symlist_len > 0 && (kSYM_Dot == _sym.GetSymbolType(symlist[0]) || access_via_this))
    {
        ++symlist; // Eat '.'
        if (0 == --symlist_len) // Eat '.' cnt'd.
        {
            cc_error("Expected struct component after '.' but did not find it");
            return -1;
        }
        // Here, if kVL_mar_pointsto_value == vloc then the first byte of outer is at m[MAR + mar_offset].
        // We accumulate mar_offset at compile time as long as possible to save computing.
        outer_vartype = vartype;
        if (!_sym.IsStruct(outer_vartype))
        {
            cc_error("Expected a struct before '.' but did not find it");
            return -1;
        }

        // Note: A DynArray can't be directly in front of a '.' (need a [...] first)
        if (_sym.IsDynpointer(vartype))
        {
            retval = AccessData_Dereference(vloc, mloc);
            if (retval < 0) return retval;
            vartype = _sym.VartypeWithout(kVTT_Dynpointer, vartype);
        }

        retval = AccessData_IsClauseLast(symlist, symlist_len, clause_is_last);
        if (retval < 0) return retval;

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        retval = AccessData_SubsequentClause((clause_is_last && writing), access_via_this, static_access, symlist, symlist_len, vloc, scope, mloc, vartype);
        if (retval < 0) return retval;

        // Only the _immediate_ access via 'this.' counts for this flag.
        // This has passed now, so reset the flag.
        access_via_this = false;
        static_access = false; // Same for 'vartype.'
    }

    if (need_to_negate)
        AccessData_Negate(vloc);

    if (kVL_attribute == vloc)
    {
        // Caller will do the assignment
        // For this to work, the caller must know the type of the struct
        // in which the attribute resides
        vartype = _sym.VartypeWithout(
                kVTT_Const|kVTT_Dynarray|kVTT_Dynpointer,
                outer_vartype);
        return 0;
    }

    if (kVL_ax_is_value == vloc)
    {
        _scrip.ax_vartype = vartype;
        _scrip.ax_val_scope = scope;
        return 0;
    }

    mloc.MakeMARCurrent(_scrip);
    return 0;
}

// In order to avoid push AX/pop AX, find out common cases that don't clobber AX
bool AGS::Parser::AccessData_MayAccessClobberAX(SymbolScript symlist, size_t symlist_len)
{
    if (kSYM_GlobalVar != _sym.GetSymbolType(symlist[0]) && kSYM_LocalVar != _sym.GetSymbolType(symlist[0]))
        return true;

    if (symlist_len == 1)
        return false;

    for (size_t symlist_idx = 0; symlist_idx < symlist_len - 3; symlist_idx += 2)
    {
        if (kSYM_Dot != _sym.GetSymbolType(symlist[symlist_idx + 1]))
            return true;
        AGS::Symbol const compo = MangleStructAndComponent(symlist[0], symlist[2]);
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
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_CX); // CX = dest
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_MAR, SREG_BX); // BX = src
    _scrip.write_cmd2(SCMD_LITTOREG, SREG_DX, STRINGBUFFER_LENGTH - 1); // DX = count
    AGS::CodeLoc const loop_start = _scrip.codesize; // Label LOOP_START
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_MAR); // AX = m[BX]
    _scrip.write_cmd1(SCMD_MEMREAD, SREG_AX);
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_CX, SREG_MAR); // m[CX] = AX
    _scrip.write_cmd1(SCMD_MEMWRITE, SREG_AX);
    _scrip.write_cmd1(SCMD_JZ, 0);  // if (AX == 0) jumpto LOOP_END
    AGS::CodeLoc const jumpout1_pos = _scrip.codesize - 1;
    _scrip.write_cmd2(SCMD_ADD, SREG_BX, 1); // BX++, CX++, DX--
    _scrip.write_cmd2(SCMD_ADD, SREG_CX, 1);
    _scrip.write_cmd2(SCMD_SUB, SREG_DX, 1);
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_DX, SREG_AX); // if (DX == 0) jumpto LOOP_END
    _scrip.write_cmd1(SCMD_JZ, 0);
    AGS::CodeLoc const jumpout2_pos = _scrip.codesize - 1;
    _scrip.write_cmd1(
        SCMD_JMP,
        ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, loop_start)); // jumpto LOOP_START
    AGS::CodeLoc const loop_end = _scrip.codesize; // Label LOOP_END
    _scrip.code[jumpout1_pos] = ccCompiledScript::RelativeJumpDist(jumpout1_pos, loop_end);
    _scrip.code[jumpout2_pos] = ccCompiledScript::RelativeJumpDist(jumpout2_pos, loop_end);
}

// We are typically in an assignment LHS = RHS; the RHS has already been
// evaluated, and the result of that evaluation is in AX.
// Store AX into the memory location that corresponds to LHS, or
// call the attribute function corresponding to LHS.
int AGS::Parser::AccessData_Assign(SymbolScript symlist, size_t symlist_len)
{
    // AX contains the result of evaluating the RHS of the assignment
    // Save on the stack so that it isn't clobbered
    AGS::Vartype rhsvartype = _scrip.ax_vartype;
    int rhsscope = _scrip.ax_val_scope;
    // Save AX unless we are sure that it won't be clobbered
    bool const may_clobber = AccessData_MayAccessClobberAX(symlist, symlist_len);
    if (may_clobber)
        _scrip.push_reg(SREG_AX);

    bool const writing = true;
    bool const negate_dummy = false; // when writing, this parameter is pointless
    ValueLocation vloc;
    AGS::Vartype lhsvartype;
    int lhsscope;
    int retval = AccessData(writing, negate_dummy, symlist, symlist_len, vloc, lhsscope, lhsvartype);
    if (retval < 0) return retval;
    if (kVL_ax_is_value == vloc)
    {
        if (!_sym.IsManaged(lhsvartype))
        {
            cc_error("Cannot modify this value");
            return -1;
        }
        _scrip.write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        _scrip.write_cmd0(SCMD_CHECKNULL);
        vloc = kVL_mar_pointsto_value;
    }

    if (may_clobber)
        _scrip.pop_reg(SREG_AX);
    _scrip.ax_vartype = rhsvartype;
    _scrip.ax_val_scope = rhsscope;

    if (kVL_attribute == vloc)
    {
        // We need to call the attribute setter 
        AGS::Vartype struct_of_attribute = lhsvartype;

        bool const is_attribute_set_func = true;
        return AccessData_Attribute(is_attribute_set_func, symlist, symlist_len, struct_of_attribute);
    }

    // MAR points to the value

    if (_sym.getOldStringSym() == lhsvartype && _sym.getOldStringSym() == _sym.VartypeWithout(kVTT_Const, rhsvartype))
    {
        // copy the string contents over.
        AccessData_StrCpy();
        return 0;
    }

    ConvertAXStringToStringObject(lhsvartype);
    rhsvartype = _scrip.ax_vartype;
    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype))
    {
        cc_error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhsvartype).c_str(),
            _sym.GetName(lhsvartype).c_str());
        return -1;
    }

    if (_sym.IsDyn(lhsvartype))
        _scrip.write_cmd1(SCMD_MEMWRITEPTR, SREG_AX);
    else
        _scrip.write_cmd1(
            GetWriteCommandForSize(_sym.GetSize(lhsvartype)),
            SREG_AX);
    return 0;
}

// Read the symbols of an expression and buffer them into expr_script
// At end of routine, the cursor will be positioned in such a way
// that _targ.getnext() will get the symbol after the expression
int AGS::Parser::BufferExpression(ccInternalList &expr_script)
{
    int nesting_depth = 0;

    AGS::Symbol peeksym;
    while (0 <= (peeksym = _targ.peeknext())) // note assignment in while condition
    {
        size_t const pos = _targ.pos; // for backing up if necessary

        // Skip over parts that are enclosed in braces, brackets, or parens
        SymbolType const peektype = _sym.GetSymbolType(peeksym);
        if (kSYM_OpenParenthesis == peektype || kSYM_OpenBracket == peektype || kSYM_OpenBrace == peektype)
            ++nesting_depth;
        if (kSYM_CloseParenthesis == peektype || kSYM_CloseBracket == peektype || kSYM_CloseBrace == peektype)
            if (--nesting_depth < 0)
                break; // this symbol can't be part of the current expression
        if (nesting_depth == 0)
        {
            if (kSYM_Dot == peektype)
            {
                expr_script.write(_targ.getnext()); // '.'
                // Eat and write next symbol, is a component name
                if (_targ.peeknext() > 0)
                    expr_script.write(_targ.getnext());
                continue;
            }

            if (kSYM_New == peektype)
            {
                // This is only allowed if a type follows
                _targ.getnext(); // Eat 'new'
                AGS::Symbol nextnextsym = _targ.getnext();
                SymbolType const nextnexttype = _sym.GetSymbolType(nextnextsym);
                if (kSYM_Vartype == nextnexttype || kSYM_UndefinedStruct == nextnexttype)
                {
                    expr_script.write(peeksym);
                    expr_script.write(nextnextsym);
                    continue;
                }
                _targ.pos = pos; // Back up so that 'new' is still unread
                break;
            }

            if (kSYM_Vartype == peektype)
            {
                // This is only allowed if a dot follows
                _targ.getnext(); // Eat the vartype
                AGS::Symbol nextsym = _targ.getnext();
                if (kSYM_Dot == _sym.GetSymbolType(nextsym))
                {
                    expr_script.write(peeksym);
                    expr_script.write(nextsym);
                    continue;
                }
                _targ.pos = pos; // Back up so that the vartype is still unread
                break;
            }

            // Apart from the exceptions above, all symbols starting at NOTEXPRESSION can't
            // be part of an expression
            if (peektype >= NOTEXPRESSION)
                break;
        }

        expr_script.write(_targ.getnext());
    }

    if (expr_script.length <= 0)
    {
        cc_error("Internal error: Empty expression");
        return -1;
    }

    return 0;
}

// evaluate the supplied expression, putting the result into AX
// returns 0 on success or -1 if compile error
// leaves targ pointing to last token in expression, so do getnext() to get the following ; or whatever
int AGS::Parser::ParseExpression()
{
    ccInternalList expr_script;
    int retval = BufferExpression(expr_script);
    if (retval < 0) return retval;

    ValueLocation vloc;
    int scope;
    Vartype vartype;

    // we now have the expression in expr_script, parse it
    retval = ParseExpression_Subexpr(expr_script.script, expr_script.length, vloc, scope, vartype);
    if (retval < 0) return retval;

    return ResultToAX(vloc, scope, vartype);
}

// We are parsing the left hand side of a += or similar statement.
int AGS::Parser::ParseAssignment_ReadLHSForModification(ccInternalList const *lhs, ValueLocation &vloc, AGS::Vartype &lhstype)
{
    int scope;
    size_t lhs_length = (lhs->length < 0) ? 0 : lhs->length;
    AGS::SymbolScript lhs_script = lhs->script;
    bool const negative = false; // LHS can't start with a unary minus
    bool const writing = false; // reading access
    int retval = AccessData(writing, negative, lhs_script, lhs_length, vloc, scope, lhstype);
    if (retval < 0) return retval;
    if (lhs_length > 0)
    {
        cc_error("Internal error: Unexpected symbols following expression");
        return -99;
    }

    if (kVL_mar_pointsto_value == vloc)
    {
        // write memory to AX
        _scrip.ax_vartype = lhstype;
        _scrip.ax_val_scope = scope;
        _scrip.write_cmd1(
            GetReadCommandForSize(_sym.GetSize(lhstype)),
            SREG_AX);
    }
    return 0;
}

// "var = expression"; lhs is the variable
int AGS::Parser::ParseAssignment_Assign(ccInternalList const *lhs)
{
    int retval = ParseExpression(); // RHS of the assignment
    if (retval < 0) return retval;
    return AccessData_Assign(lhs->script, lhs->length);
}

// We compile something like "var += expression"
int AGS::Parser::ParseAssignment_MAssign(AGS::Symbol ass_symbol, ccInternalList const *lhs)
{
    // Parse RHS
    int retval = ParseExpression();
    if (retval < 0) return retval;
    _scrip.push_reg(SREG_AX);
    AGS::Vartype rhsvartype = _scrip.ax_vartype;

    // Parse LHS
    ValueLocation vloc;
    AGS::Vartype lhsvartype;
    retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype);
    if (retval < 0) return retval;

    // Use the operator on LHS and RHS
    int cpuOp = _sym[ass_symbol].GetCPUOp();
    retval = GetOperatorValidForVartype(lhsvartype, rhsvartype, cpuOp);
    if (retval < 0) return retval;
    _scrip.pop_reg(SREG_BX);
    _scrip.write_cmd2(cpuOp, SREG_AX, SREG_BX);

    if (kVL_mar_pointsto_value == vloc)
    {
        // write AX back to memory
        AGS::Symbol memwrite = GetWriteCommandForSize(_sym.GetSize(lhsvartype));
        _scrip.write_cmd1(memwrite, SREG_AX);
        return 0;
    }
    return AccessData_Assign(lhs->script, lhs->length);
}

// "var++" or "var--"
int AGS::Parser::ParseAssignment_SAssign(AGS::Symbol ass_symbol, ccInternalList const *lhs)
{
    ValueLocation vloc;
    AGS::Vartype lhsvartype;
    int retval = ParseAssignment_ReadLHSForModification(lhs, vloc, lhsvartype);
    if (retval < 0) return retval;

    // increment or decrement AX, using the correct bytecode
    int cpuOp = _sym[ass_symbol].GetCPUOp();
    retval = GetOperatorValidForVartype(lhsvartype, 0, cpuOp);
    if (retval < 0) return retval;
    _scrip.write_cmd2(cpuOp, SREG_AX, 1);

    if (kVL_mar_pointsto_value == vloc)
    {
        // write AX back to memory
        AGS::Symbol memwrite = GetWriteCommandForSize(_sym.GetSize(lhsvartype));
        _scrip.write_cmd1(memwrite, SREG_AX);
        return 0;
    }

    return ParseAssignment_Assign(lhs);
}

// We've read a variable or selector of a struct into symlist[], the last identifying component is in cursym.
// An assignment symbol is following. Compile the assignment.
int AGS::Parser::ParseAssignment(AGS::Symbol ass_symbol, ccInternalList const *lhs)
{
    switch (_sym.GetSymbolType(ass_symbol))
    {
    default: // can't happen
        cc_error("Internal error: Illegal assignment symbol found");
        return -99;

    case kSYM_Assign:
        return ParseAssignment_Assign(lhs);

    case kSYM_AssignMod:
        return ParseAssignment_MAssign(ass_symbol, lhs);

    case kSYM_AssignSOp:
        return ParseAssignment_SAssign(ass_symbol, lhs);
    }
}

int AGS::Parser::ParseVardecl_InitialValAssignment_Float(bool is_neg, void *& initial_val_ptr)
{
    // initialize float
    if (_sym.GetSymbolType(_targ.peeknext()) != kSYM_LiteralFloat)
    {
        cc_error("Expected floating point value after '='");
        return -1;
    }

    float float_init_val = static_cast<float>(atof(_sym.GetName(_targ.getnext()).c_str()));
    if (is_neg)
        float_init_val = -float_init_val;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        cc_error("Out of memory");
        return -1;
    }

    // Interpret the float as an int; move that into the allocated space
    (static_cast<long *>(initial_val_ptr))[0] = InterpretFloatAsInt(float_init_val);

    return 0;
}

int AGS::Parser::ParseVardecl_InitialValAssignment_OldString(void *&initial_val_ptr)
{
    AGS::Symbol literal_sym = _targ.getnext();
    if (kSYM_LiteralString != _sym.GetSymbolType(literal_sym))
    {
        cc_error("Expected a literal string");
        return -1;
    }
    std::string literal = _sym.GetName(literal_sym);
    if (literal.length() >= STRINGBUFFER_LENGTH)
    {
        cc_error(
            "Initializer string is too long (max. chars allowed: %d",
            STRINGBUFFER_LENGTH - 1);
        return -1;
    }
    initial_val_ptr = malloc(STRINGBUFFER_LENGTH);
    if (!initial_val_ptr)
    {
        cc_error("Out of memory");
        return -1;
    }
    std::strncpy(
        static_cast<char *>(initial_val_ptr), literal.c_str(),
        STRINGBUFFER_LENGTH);
    return 0;
}

int AGS::Parser::ParseVardecl_InitialValAssignment_Inttype(bool is_neg, void *&initial_val_ptr)
{
    // Initializer for an integer value
    int int_init_val;
    int retval = ParseLiteralOrConstvalue(_targ.getnext(), int_init_val, is_neg, "Expected integer value after '='");
    if (retval < 0) return retval;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        cc_error("Out of memory");
        return -1;
    }
    // Convert int to long; move that into the allocated space
    (reinterpret_cast<long *>(initial_val_ptr))[0] = int_init_val;

    return 0;
}

// if initial_value is non-null, it returns malloc'd memory that must be free
int AGS::Parser::ParseVardecl_InitialValAssignment(AGS::Symbol varname, void *&initial_val_ptr)
{
    initial_val_ptr = nullptr;
    _targ.getnext(); // Eat '='

    if (_sym.IsManaged(varname))
    {
        // TODO Initialize String
        cc_error("Cannot assign an initial value to a managed type or String");
        return -1;
    }

    if (_sym.IsStruct(varname))
    {
        cc_error("Cannot initialize struct type");
        return -1;
    }

    if (_sym.getOldStringSym() == _sym.GetVartype(varname))
        return ParseVardecl_InitialValAssignment_OldString(initial_val_ptr);

    // accept leading '-' if present
    bool is_neg = false;
    if (_targ.peeknext() == _sym.Find("-"))
    {
        is_neg = true;
        _targ.getnext();
    }

    // Do actual assignment
    if (_sym.GetVartype(varname) == _sym.getFloatSym())
        return ParseVardecl_InitialValAssignment_Float(is_neg, initial_val_ptr);
    return ParseVardecl_InitialValAssignment_Inttype(is_neg, initial_val_ptr);
}

// Move variable information into the symbol table
void AGS::Parser::ParseVardecl_Var2SymTable(Symbol var_name, AGS::Vartype vartype, Globalness globalness)
{
    SymbolTableEntry &entry = _sym[var_name];
    entry.SType = (globalness == kGl_Local) ? kSYM_LocalVar : kSYM_GlobalVar;
    entry.vartype = vartype;
    _sym.SetDeclared(var_name, ccCurScriptName, currentline);
}

// we have accepted something like "int a" and we're expecting "["
int AGS::Parser::ParseVardecl_Array(AGS::Symbol var_name, AGS::Vartype &vartype)
{
    _targ.getnext();  // skip the [

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_CloseBracket)
    {
        // Dynamic array
        if (vartype == _sym.getOldStringSym())
        {
            cc_error("Dynamic arrays of old-style strings are not supported");
            return -1;
        }
        vartype = _sym.VartypeWith(kVTT_Dynarray,vartype);
    }
    else
    {
        // Static array
        AGS::Symbol nextt = _targ.getnext();

        int arrsize_as_int;
        int retval = ParseLiteralOrConstvalue(nextt, arrsize_as_int, false, "Array size must be constant value");
        if (retval < 0) return retval;

        if (arrsize_as_int < 1)
        {
            cc_error("Array size must be at least 1");
            return -1;
        }

        std::vector<size_t> dims;
        dims.push_back(arrsize_as_int);
        vartype = _sym.VartypeWithArray(dims, vartype);
    }

    Symbol const nextsym = _targ.getnext();
    if (_sym.GetSymbolType(nextsym) != kSYM_CloseBracket)
    {
        cc_error(
            "Expected ']', found %s instead",
            _sym.GetName(nextsym).c_str());
        return -1;
    }

    return 0;
}

int AGS::Parser::ParseVardecl_CheckIllegalCombis(AGS::Vartype vartype, Globalness globalness)
{
    if (vartype == _sym.getOldStringSym() && ccGetOption(SCOPT_OLDSTRINGS) == 0)
    {
        cc_error("Type 'string' is no longer supported; use String instead");
        return -1;
    }

    if (vartype == _sym.getOldStringSym() && kGl_GlobalImport == globalness)
    {
        // cannot import, because string is really char *, and the pointer won't resolve properly
        cc_error("Cannot import string; use char[] instead");
        return -1;
    }

    if (vartype == _sym.getVoidSym())
    {
        cc_error("'void' not a valid variable type");
        return -1;
    }

    return 0;
}

// there was a forward declaration -- check that the real declaration matches it
int AGS::Parser::ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info)
{
    if (0 == known_info->SType)
        return 0; // We don't have any known info

    if ((known_info->Flags & ~kSFLG_Imported) != (this_entry->Flags & ~kSFLG_Imported))
    {
        std::string msg = ReferenceMsg(
            "Qualifiers of this variable do not match prototype",
            _sym.Id2Section(known_info->DeclSectionId),
            known_info->DeclLine);
        cc_error(msg.c_str());
        return -1;
    }

    if (known_info->vartype != this_entry->vartype)
    {
        // This will check the array lengths, too
        std::string msg = ReferenceMsg(
            "This variable is declared as %s here, as %s elsewhere",
            _sym.Id2Section(known_info->DeclSectionId),
            known_info->DeclLine);
        cc_error(
            msg.c_str(),
            _sym.GetName(this_entry->vartype).c_str(),
            _sym.GetName(known_info->vartype).c_str());
        return -1;
    }

    if (known_info->GetSize(_sym) != this_entry->GetSize(_sym))
    {
        std::string msg = ReferenceMsg(
            "Size of this variable is %d here, %d declared elsewhere",
            _sym.Id2Section(known_info->DeclSectionId),
            known_info->DeclLine);
        cc_error(
            msg.c_str(),
            this_entry->GetSize(_sym), known_info->GetSize(_sym));
        return -1;
    }

    return 0;
}

int AGS::Parser::ParseVardecl_GlobalImport(AGS::Symbol var_name, bool has_initial_assignment)
{
    if (has_initial_assignment)
    {
        cc_error("Imported variables cannot have an initial assignment");
        return -1;
    }

    if (_givm[var_name])
        return 0; // Skip this since the global non-import decl will come later

    SetFlag(_sym[var_name].Flags, kSFLG_Imported, true);
    _sym[var_name].SOffset = _scrip.add_new_import(_sym.GetName(var_name).c_str());
    if (_sym[var_name].SOffset == -1)
    {
        cc_error("Internal error: Import table overflow");
        return -1;
    }

    return 0;
}

int AGS::Parser::ParseVardecl_GlobalNoImport(AGS::Symbol var_name, AGS::Vartype vartype, bool has_initial_assignment, void *&initial_val_ptr)
{
    if (has_initial_assignment)
    {
        int retval = ParseVardecl_InitialValAssignment(var_name, initial_val_ptr);
        if (retval < 0) return retval;
    }
    SymbolTableEntry &entry = _sym[var_name];
    entry.vartype = vartype;
    size_t const var_size = _sym.GetSize(vartype);
    entry.SOffset = _scrip.add_global(var_size, initial_val_ptr);
    if (entry.SOffset < 0)
    {
        cc_error("Internal error: Cannot allocate global variable");
        return -1;
    }
    return 0;
}

int AGS::Parser::ParseVardecl_Local(AGS::Symbol var_name, AGS::Vartype vartype, bool has_initial_assignment)
{
    size_t const var_size = _sym.GetSize(vartype);
    _sym[var_name].SOffset = _scrip.cur_sp;

    if (!has_initial_assignment)
    {
        // Initialize the variable with binary zeroes.
        _scrip.write_cmd1(SCMD_LOADSPOFFS, 0);
        _scrip.write_cmd1(SCMD_ZEROMEMORY, var_size);
        _scrip.write_cmd2(SCMD_ADD, SREG_SP, var_size);
        _scrip.cur_sp += var_size;
        return 0;
    }

    // "readonly" vars can't be assigned to, so don't use standard assignment function here.
    _targ.getnext(); // Eat '='
    int retval = ParseExpression(); 
    if (retval < 0) return retval;

    bool const is_dyn = _sym.IsDyn(vartype);
    
    if (SIZE_OF_INT == var_size && !is_dyn)
    {
        // This PUSH moves the result of the initializing expression into the
        // new variable and reserves space for this variable on the stack.
        _scrip.push_reg(SREG_AX);
        return 0;
    }

    ConvertAXStringToStringObject(vartype);
    _scrip.write_cmd1(SCMD_LOADSPOFFS, 0);
    _scrip.write_cmd1(
        is_dyn ? SCMD_MEMINITPTR : GetWriteCommandForSize(var_size),
        SREG_AX);
    _scrip.write_cmd2(SCMD_ADD, SREG_SP, var_size);
    _scrip.cur_sp += var_size;
    return 0;
}

int AGS::Parser::ParseVardecl0(AGS::Symbol var_name, AGS::Vartype vartype, SymbolType next_type, Globalness globalness, bool &another_var_follows)
{
    if (kSYM_OpenBracket == next_type)
    {
        int retval = ParseVardecl_Array(var_name, vartype);
        if (retval < 0) return retval;
        next_type = _sym.GetSymbolType(_targ.peeknext());
    }

    // Enter the variable into the symbol table
    ParseVardecl_Var2SymTable(var_name, vartype, globalness);

    bool const has_initial_assignment = (kSYM_Assign == next_type);

    switch (globalness)
    {
    default:
        cc_error("Internal error: Wrong value of globalness");
        return -99;

    case kGl_GlobalImport:
        return ParseVardecl_GlobalImport(var_name, has_initial_assignment);

    case kGl_GlobalNoImport:
    {
        void *initial_val_ptr = nullptr;
        int retval = ParseVardecl_GlobalNoImport(var_name, vartype, has_initial_assignment, initial_val_ptr);
        if (initial_val_ptr) free(initial_val_ptr);
        return retval;
    }
    case kGl_Local:
        return ParseVardecl_Local(var_name, vartype, has_initial_assignment);
    }
}

// wrapper around ParseVardecl0() 
int AGS::Parser::ParseVardecl(AGS::Symbol var_name, AGS::Vartype vartype, SymbolType next_type, Globalness globalness, bool &another_var_follows)
{
    int retval = ParseVardecl_CheckIllegalCombis(vartype, globalness);
    if (retval < 0) return retval;

    if (kGl_Local == globalness && _sym[var_name].SType != 0)
    {
        std::string msg = ReferenceMsgSym(
            "Variable %s has already been declared", var_name);
        cc_error(msg.c_str(), _sym.GetName(var_name).c_str());
        return -1;
    }

    SymbolTableEntry known_info;
    retval = CopyKnownSymInfo(_sym[var_name], known_info);
    if (retval < 0) return retval;

    retval = ParseVardecl0(var_name, vartype, next_type, globalness, another_var_follows);
    if (retval < 0) return retval;

    retval = ParseVardecl_CheckThatKnownInfoMatches(&_sym[var_name], &known_info);
    if (retval < 0) return retval;

    if (_targ.reached_eof())
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    next_type = _sym.GetSymbolType(_targ.peeknext());
    if (next_type == kSYM_Comma)
    {
        _targ.getnext();  // Eat ','
        another_var_follows = true;
        return 0;
    }
    another_var_follows = false;
    if (next_type == kSYM_Semicolon)
        return 0;

    cc_error("Expected ',' or ';' or '=' instead of '%s'", _sym.GetName(_targ.peeknext()).c_str());
    return -1;
}

void AGS::Parser::ParseOpenbrace_FuncBody(AGS::Symbol name_of_func, int struct_of_func, AGS::NestingStack *nesting_stack)
{
    // write base address of function for any relocation needed later
    _scrip.write_cmd1(SCMD_THISBASE, _scrip.codesize);
    SymbolTableEntry &entry = _sym[name_of_func];
    if (FlagIsSet(entry.Flags, kSFLG_NoLoopCheck))
    {
        _scrip.write_cmd0(SCMD_LOOPCHECKOFF);
        SetFlag(entry.Flags, kSFLG_NoLoopCheck, false);
    }

    // loop through all parameters
    // the first entry is the return address, so skip that
    size_t const num_args = _sym[name_of_func].GetNumOfFuncArgs();
    for (size_t pa = 1; pa <= num_args; pa++)
    {
        AGS::Vartype const param_vartype = _sym[name_of_func].FuncParamTypes[pa];
        if (!_sym.IsManaged(param_vartype))
            continue;

        // For each managed parameter, an address is pushed where the parameter 
        // is stored, i.e. a value MAR (!!!)  where m[MAR] (!!!) contains the value. 
        // We need to convert this to the value itself.
        _scrip.write_cmd1(SCMD_LOADSPOFFS, 4 * (pa + 1)); // Set MAR to the pertinent memory address        
        _scrip.write_cmd1(SCMD_MEMREAD, SREG_AX); // Read the address that is stored there
        // Create a dynpointer that points to the same object as m[AX] and store it in m[MAR]
        _scrip.write_cmd1(SCMD_MEMINITPTR, SREG_AX);
    }

    SymbolTableEntry &this_entry = _sym[_sym.getThisSym()];
    this_entry.vartype = 0;
    if (struct_of_func > 0 && !FlagIsSet(_sym.GetFlags(name_of_func), kSFLG_Static))
    {
        // Declare the "this" pointer (allocated memory for it will never be used)
        this_entry.SType = kSYM_LocalVar;
        // Don't declare this as dynpointer to prevent it from being dereferenced twice.
        this_entry.vartype = struct_of_func;
        this_entry.SScope = nesting_stack->Depth() - 1;
        this_entry.Flags = kSFLG_Readonly | kSFLG_Accessed;
        // Allocate unused space on stack for the "this" pointer
        this_entry.SOffset = _scrip.cur_sp;
        _scrip.write_cmd1(SCMD_LOADSPOFFS, 0);
        _scrip.write_cmd2(SCMD_WRITELIT, SIZE_OF_DYNPOINTER, 0);
        _scrip.cur_sp += SIZE_OF_DYNPOINTER;
        _scrip.write_cmd2(SCMD_ADD, SREG_SP, SIZE_OF_DYNPOINTER);
    }
}

int AGS::Parser::ParseOpenbrace(AGS::NestingStack *nesting_stack, AGS::Symbol name_of_current_func, AGS::Symbol struct_of_current_func)
{
    if (nesting_stack->IsUnbraced())
    {
        cc_error("Internal compiler error in openbrace");
        return -1;
    }

    // Assume a brace without special reason as a default
    int retval = nesting_stack->Push(AGS::NestingStack::kNT_Nothing);
    if (retval < 0) return retval;

    if (nesting_stack->Depth() == 2)
    {
        // In this case, the braces are around a function body
        nesting_stack->SetType(AGS::NestingStack::kNT_Function);
        ParseOpenbrace_FuncBody(name_of_current_func, struct_of_current_func, nesting_stack);
    }

    return 0;
}


int AGS::Parser::ParseClosebrace(AGS::NestingStack *nesting_stack, AGS::Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func)
{
    size_t nesting_level = nesting_stack->Depth() - 1;

    if (nesting_level == 0 || nesting_stack->IsUnbraced())
    {
        cc_error("Unexpected '}'");
        return -1;
    }

    FreeDynpointersOfLocals(nesting_level - 1, name_of_current_func, (nesting_level == 1));
    int totalsub = StacksizeOfLocals(nesting_level - 1);
    if (totalsub > 0)
    {
        // Reduce the "high point" of the stack appropriately, 
        // write code for popping the bytes from the stack
        _scrip.cur_sp -= totalsub;
        _scrip.write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    }

    // All the local variables that were defined within the braces become invalid
    RemoveLocalsFromSymtable(nesting_level);

    if (nesting_level == 1)
    {
        // Emit code that returns 0
        if (_sym.getVoidSym() != _sym[name_of_current_func].FuncParamTypes.at(0))
            _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

        _fcm.SetFuncExitJumppoint(name_of_current_func, _scrip.codesize);
        // We've just finished the body of the current function.
        name_of_current_func = -1;
        struct_of_current_func = -1;

        // Write code to return from the function.
        // This pops the return address from the stack, 
        // so adjust the "high point" of stack allocation appropriately
        _scrip.write_cmd0(SCMD_RET);
        _scrip.cur_sp -= 4;

        nesting_stack->Pop();
        return 0;
    }

    // Deal with actions that need to be done at the end 
    int retval;
    switch (nesting_stack->Type())
    {
    default:
        nesting_stack->Pop();
        return 0;

    case AGS::NestingStack::kNT_BracedDo:
        retval = DealWithEndOfDo(nesting_stack);
        if (retval < 0) return retval;
        break;

    case AGS::NestingStack::kNT_BracedElse:
    case AGS::NestingStack::kNT_BracedThen:
    {
        bool if_turned_into_else;
        retval = DealWithEndOfElse(nesting_stack, if_turned_into_else);
        if (retval < 0) return retval;
        if (if_turned_into_else)
            return 0;
        break;
    }

    case AGS::NestingStack::kNT_Switch:
        retval = DealWithEndOfSwitch(nesting_stack);
        if (retval < 0) return retval;
        break;
    }

    // loop round doing all the end of elses, but break once an IF
    // has been turned into an ELSE
    while (nesting_stack->IsUnbraced())
    {
        if (nesting_stack->Type() == AGS::NestingStack::kNT_UnbracedDo)
        {
            retval = DealWithEndOfDo(nesting_stack);
            if (retval < 0) return retval;
        }

        bool if_turned_into_else;
        retval = DealWithEndOfElse(nesting_stack, if_turned_into_else);
        if (retval < 0) return retval;
        if (if_turned_into_else)
            break;
    }
    return 0;
}

void AGS::Parser::ParseStruct_SetTypeInSymboltable(AGS::Symbol stname, TypeQualifierSet tqs)
{
    SymbolTableEntry &entry = _sym[stname];

    entry.Extends = 0;
    entry.SType = kSYM_Vartype;
    SetFlag(entry.Flags, kSFLG_StructVartype, true);
    entry.SSize = 0;

    if (FlagIsSet(tqs, kTQ_Managed))
        SetFlag(entry.Flags, kSFLG_Managed, true);

    if (FlagIsSet(tqs, kTQ_Builtin))
        SetFlag(entry.Flags, kSFLG_Builtin, true);

    if (FlagIsSet(tqs, kTQ_Autoptr))
        SetFlag(entry.Flags, kSFLG_Autoptr, true);
    if (kPP_Main == _pp)
        _sym.SetDeclared(stname, ccCurScriptName, currentline);
}

// We have accepted something like "struct foo" and are waiting for "extends"
int AGS::Parser::ParseStruct_ExtendsClause(AGS::Symbol stname, AGS::Symbol &parent, size_t &size_so_far)
{
    _targ.getnext(); // Eat "extends"
    parent = _targ.getnext(); // name of the extended struct

    if (kPP_PreAnalyze == _pp)
        return 0; // No further analysis necessary in first phase

    if (kSYM_Vartype != _sym.GetSymbolType(parent))
    {
        cc_error("Expected a struct type here");
        return -1;
    }
    SymbolTableEntry &struct_entry = _sym[stname];
    SymbolTableEntry const &extends_entry = _sym[parent];

    if (!extends_entry.IsStruct(_sym))
    {
        cc_error("Must extend a struct type");
        return -1;
    }
    if (!FlagIsSet(extends_entry.Flags, kSFLG_Managed) && FlagIsSet(struct_entry.Flags, kSFLG_Managed))
    {
        cc_error("Managed struct cannot extend the unmanaged struct '%s'", _sym.GetName(parent).c_str());
        return -1;
    }
    if (FlagIsSet(extends_entry.Flags, kSFLG_Managed) && !FlagIsSet(struct_entry.Flags, kSFLG_Managed))
    {
        cc_error("Unmanaged struct cannot extend the managed struct '%s'", _sym.GetName(parent).c_str());
        return -1;
    }
    if (FlagIsSet(extends_entry.Flags, kSFLG_Builtin) && !FlagIsSet(struct_entry.Flags, kSFLG_Builtin))
    {
        cc_error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", _sym.GetName(parent).c_str());
        return -1;
    }
    size_so_far = _sym.GetSize(parent);
    struct_entry.Extends = parent;

    return 0;
}


void AGS::Parser::ParseStruct_MemberQualifiers(TypeQualifierSet &tqs)
{
    tqs = 0;
    while (true)
    {
        AGS::Symbol peeksym = _targ.peeknext();
        switch (_sym.GetSymbolType(peeksym))
        {
        default: return;
        case kSYM_Attribute:      SetFlag(tqs, kTQ_Attribute, true); break;
        case kSYM_Import:         SetFlag(tqs, kTQ_ImportStd, true); break;
        case kSYM_Protected:      SetFlag(tqs, kTQ_Protected, true); break;
        case kSYM_ReadOnly:       SetFlag(tqs, kTQ_Readonly, true); break;
        case kSYM_Static:         SetFlag(tqs, kTQ_Static, true); break;
        case kSYM_WriteProtected: SetFlag(tqs, kTQ_Writeprotected, true); break;
        }
        _targ.getnext();
    };

    return;
}

int AGS::Parser::ParseStruct_CheckComponentVartype(int stname, AGS::Vartype vartype, bool member_is_import)
{
    if (Vartype2Symbol(vartype) == stname && !_sym.IsManaged(vartype))
    {
        // cannot do "struct A { A a; }", this struct would be infinitely large
        cc_error("Struct '%s' can't be a member of itself", _sym.GetName(vartype).c_str());
        return -1;
    }

    SymbolType const vartype_type = _sym.GetSymbolType(vartype);
    if (vartype_type == kSYM_NoType)
    {
        cc_error(
            "Type '%s' is undefined",
            _sym.GetName(vartype).c_str());
        return -1;
    }
    if (kSYM_Vartype != vartype_type && kSYM_UndefinedStruct != vartype_type)
    {
        std::string msg = ReferenceMsgSym(
            "'%s' should be a typename but is already in use differently",
            Vartype2Symbol(vartype));
        cc_error(
            msg.c_str(),
            _sym.GetName(vartype).c_str());
        return -1;
    }

    if (vartype == _sym.getOldStringSym()) // [fw] Where's the problem?
    {
        cc_error("'string' not allowed inside a struct");
        return -1;
    }
    return 0;
}

// check that we haven't extended a struct that already contains a member with the same name
int AGS::Parser::ParseStruct_CheckForCompoInAncester(AGS::Symbol orig, AGS::Symbol compo, AGS::Symbol act_struct)
{
    if (act_struct <= 0)
        return 0;
    AGS::Symbol const member = MangleStructAndComponent(act_struct, compo);
    if (kSYM_NoType != _sym.GetSymbolType(member))
    {
        std::string msg = ReferenceMsgSym(
            "The struct '%s' extends '%s', and '%s' is already defined",
            member);
        cc_error(
            msg.c_str(),
            _sym.GetName(orig).c_str(),
            _sym.GetName(act_struct).c_str(),
            _sym.GetName(member).c_str());
        return -1;
    }

    return ParseStruct_CheckForCompoInAncester(orig, compo, _sym[act_struct].Extends);
}

int AGS::Parser::ParseStruct_Function(AGS::TypeQualifierSet tqs, AGS::Vartype vartype, AGS::Symbol stname, AGS::Symbol vname, AGS::Symbol name_of_current_func)
{
    if (FlagIsSet(tqs, kTQ_Writeprotected))
    {
        cc_error("'writeprotected' does not apply to functions");
        return -1;
    }

    bool body_follows;
    int retval = ParseFuncdecl(vname, vartype, tqs, stname, body_follows);
    if (retval < 0) return retval;
    if (body_follows)
    {
        cc_error("Cannot declare a function body within a struct definition");
        return -1;
    }
    if (kSYM_Semicolon != _sym.GetSymbolType(_targ.peeknext()))
    {
        cc_error("Expected ';'");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected))
        SetFlag(_sym[vname].Flags, kSFLG_Protected, true);
    return 0;
}

int AGS::Parser::ParseStruct_CheckAttributeFunc(SymbolTableEntry &entry, bool is_setter, bool is_indexed, AGS::Vartype vartype)
{
    size_t const sscope_wanted = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);
    if (entry.SScope != sscope_wanted)
    {
        cc_error(
            "The attribute function '%s' should have %d parameter(s) but is declared with %d parameter(s) instead",
            entry.SName.c_str(), sscope_wanted, entry.SScope);
        return -1;
    }
    AGS::Vartype const ret_vartype = is_setter ? _sym.getVoidSym() : vartype;
    if (entry.FuncParamTypes[0] != ret_vartype)
    {
        cc_error(
            "The attribute function '%s' must return type '%s' but returns '%s' instead",
            entry.SName.c_str(),
            _sym.GetName(ret_vartype).c_str(),
            _sym.GetName(entry.FuncParamTypes[0]).c_str());
        return -1;
    }
    size_t p_idx = 1;
    if (is_indexed)
    {
        if (entry.FuncParamTypes[p_idx] != _sym.getIntSym())
        {
            cc_error(
                "Parameter #%d of attribute function '%s' must have type integer but doesn't.",
                p_idx, entry.SName.c_str());
            return -1;
        }
        p_idx++;
    }
    if (is_setter && entry.FuncParamTypes[p_idx] != vartype)
    {
        cc_error(
            "Parameter #d of attribute function '%s' must have type '%s'",
            p_idx, entry.SName.c_str(), _sym.GetName(vartype).c_str());
        return -1;
    }

    return 0;
}

int AGS::Parser::ParseStruct_EnterAttributeFunc(bool is_setter, bool is_indexed, bool is_static, AGS::Vartype vartype, SymbolTableEntry &entry)
{
    entry.SType = kSYM_Function;
    SetFlag(entry.Flags, kSFLG_Imported, true);
    if (is_static)
        SetFlag(entry.Flags, kSFLG_Static, true);
    entry.SOffset = _importMgr.FindOrAdd(entry.SName);
    char  *num_param_suffix;
    if (is_setter)
        num_param_suffix = (is_indexed ? "^2" : "^1");
    else // getter
        num_param_suffix = (is_indexed ? "^1" : "^0");
    strcat(_scrip.imports[entry.SOffset], num_param_suffix);

    AGS::Vartype const retvartype = entry.FuncParamTypes[0] = entry.vartype = 
        is_setter ? _sym.getVoidSym() : vartype;
    entry.SScope = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);

    entry.FuncParamTypes.resize(entry.SScope + 1);

    size_t p_idx = 1;
    if (is_indexed)
        entry.FuncParamTypes[p_idx++] = _sym.getIntSym();
    if (is_setter)
        entry.FuncParamTypes[p_idx] = vartype;
    entry.FuncParamHasDefaultValues.assign(entry.FuncParamTypes.size(), false);
    entry.FuncParamDefaultValues.assign(entry.FuncParamTypes.size(), 0);
    return 0;
}

// We are processing an attribute.
// This corresponds to a getter func and a setter func, declare one of them
int AGS::Parser::ParseStruct_DeclareAttributeFunc(AGS::Symbol func, bool is_setter, bool is_indexed, bool is_static, AGS::Vartype vartype)
{
    SymbolTableEntry &entry = _sym[func];
    if (kSYM_Function != entry.SType && kSYM_NoType != entry.SType)
    {
        std::string msg = ReferenceMsgSym(
            "Attribute uses '%s' as a function, this clashes with a declaration elsewhere",
            func);
        cc_error(msg.c_str(), entry.SName.c_str());
        return -1;
    }

    if (kSYM_Function == entry.SType) // func has already been declared
        return ParseStruct_CheckAttributeFunc(entry, is_setter, is_indexed, vartype);

    int retval = ParseStruct_EnterAttributeFunc(is_setter, is_indexed, is_static, vartype, entry);
    if (retval < 0) return retval;
    return _fim.SetFuncCallpoint(func, entry.SOffset);
}

// We're in a struct declaration, parsing a struct attribute
int AGS::Parser::ParseStruct_Attribute(AGS::TypeQualifierSet tqs, AGS::Symbol stname, AGS::Symbol vname)
{
    bool attrib_is_indexed = false;

    if (kSYM_OpenBracket == _sym.GetSymbolType(_targ.peeknext()))
    {
        attrib_is_indexed = true;
        _targ.getnext();
        if (kSYM_CloseBracket != _sym.GetSymbolType(_targ.getnext()))
        {
            cc_error("Cannot specify array size for attribute");
            return -1;
        }
    }
    if (kPP_PreAnalyze == _pp)
        return 0;

    bool attrib_is_static = FlagIsSet(tqs, kTQ_Static);

    Vartype const coretype = _sym[vname].vartype;

    _sym[vname].SType = kSYM_Attribute;
    if (attrib_is_indexed)
        _sym[vname].vartype = _sym.VartypeWith(kVTT_Dynarray, _sym[vname].vartype);

    // Declare attribute get func, e.g. get_ATTRIB()
    AGS::Symbol attrib_func = -1;
    bool func_is_setter = false;
    int retval = ConstructAttributeFuncName(vname, func_is_setter, attrib_is_indexed, attrib_func);
    if (retval < 0) return retval;
    retval = ParseStruct_DeclareAttributeFunc(MangleStructAndComponent(stname, attrib_func), func_is_setter, attrib_is_indexed, attrib_is_static, coretype);
    if (retval < 0) return retval;

    if (FlagIsSet(tqs, kTQ_Readonly))
        return 0;

    // Declare attribute set func, e.g. set_ATTRIB(value)
    func_is_setter = true;
    retval = ConstructAttributeFuncName(vname, func_is_setter, attrib_is_indexed, attrib_func);
    if (retval < 0) return retval;
    return ParseStruct_DeclareAttributeFunc(MangleStructAndComponent(stname, attrib_func), func_is_setter, attrib_is_indexed, attrib_is_static, coretype);
}

// We're inside a struct decl, parsing an array var.
int AGS::Parser::ParseStruct_Array(AGS::Symbol stname, AGS::Symbol vname, size_t &size_so_far)
{
    _targ.getnext(); // Eat '['

    if (kPP_PreAnalyze == _pp)
    {
        // Skip the [...]
        const SymbolType stoplist[] = { kSYM_NoType, };
        SkipTo(stoplist, 0);
        _targ.getnext(); // Eat ']'
        return 0;
    }

    AGS::Symbol const nextt = _targ.getnext();
    if (_sym.GetSymbolType(nextt) == kSYM_CloseBracket)
    {
        if (FlagIsSet(_sym.GetFlags(stname), kSFLG_Managed))
        {
            cc_error("Member variable of managed struct cannot be dynamic array");
            return -1;
        }
        SetFlag(_sym[stname].Flags, kSFLG_HasDynArray, true);
        _sym[vname].vartype = _sym.VartypeWith(kVTT_Dynarray, _sym[vname].vartype);
        size_so_far += SIZE_OF_DYNPOINTER;
        return 0;
    }

    int array_size = 0;
    if (ParseLiteralOrConstvalue(nextt, array_size, false, "Array size must be constant value") < 0)
        return -1;
    if (array_size < 1)
    {
        cc_error("Array size cannot be less than 1");
        return -1;
    }

    std::vector<size_t> dims;
    dims.push_back(array_size);
    Vartype core_vartype = _sym[vname].vartype;
    SetDynpointerInManagedVartype(core_vartype);
    _sym[vname].vartype = _sym.VartypeWithArray(dims, core_vartype);
    size_so_far += _sym.GetSize(_sym[vname].vartype);

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseBracket)
    {
        cc_error("Expected ']'");
        return -1;
    }
    return 0;
}

// We're inside a struct decl, processing a member variable
int AGS::Parser::ParseStruct_VariableOrAttribute(AGS::TypeQualifierSet tqs, AGS::Vartype vartype, AGS::Symbol stname, AGS::Symbol vname, size_t &size_so_far)
{
    if (kPP_Main == _pp)
    {
        SymbolTableEntry &entry = _sym[vname];
        entry.SType = kSYM_StructComponent;
        entry.Extends = stname;  // save which struct it belongs to
        entry.SOffset = size_so_far;
        entry.vartype = vartype;
        if (FlagIsSet(tqs, kTQ_Readonly))
            SetFlag(entry.Flags, kSFLG_Readonly, true);
        if (FlagIsSet(tqs, kTQ_Attribute))
            SetFlag(entry.Flags, kSFLG_Attribute, true);
        if (FlagIsSet(tqs, kTQ_Static))
            SetFlag(entry.Flags, kSFLG_Static, true);
        if (FlagIsSet(tqs, kTQ_Protected))
            SetFlag(entry.Flags, kSFLG_Protected, true);
        if (FlagIsSet(tqs, kTQ_Writeprotected))
            SetFlag(entry.Flags, kSFLG_WriteProtected, true);
    }

    if (FlagIsSet(tqs, kTQ_Attribute))
        return ParseStruct_Attribute(tqs, stname, vname);

    if (FlagIsSet(tqs, kTQ_Import))
    {
        // member variable cannot be an import
        cc_error("Can't import struct component variables; import the whole struct instead");
        return -1;
    }

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_OpenBracket)
        return ParseStruct_Array(stname, vname, size_so_far);

    size_so_far += _sym.GetSize(vname);
    return 0;
}

// We have accepted something like "struct foo extends bar { const int".
// We're waiting for the name of the member.
int AGS::Parser::ParseStruct_MemberDefnVarOrFuncOrArray(AGS::Symbol parent, AGS::Symbol stname, AGS::Symbol current_func, TypeQualifierSet tqs, AGS::Vartype vartype, size_t &size_so_far)
{
    int retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    AGS::Symbol const component = _targ.getnext();
    AGS::Symbol const mangled_name = MangleStructAndComponent(stname, component);
    if (kSYM_Vartype == _sym.GetSymbolType(component) && _sym.IsPrimitive(component))
    {
        cc_error("Can't use primitive type '%s' as a struct component name");
        return -1;
    }

    bool const is_function = _sym.GetSymbolType(_targ.peeknext()) == kSYM_OpenParenthesis;
    if (!is_function && _sym.IsDynarray(vartype)) // e.g., int [] Foo;
    {
        cc_error("Expected '('");
        return -1;
    }

    if (_sym.IsManaged(vartype) &&
        FlagIsSet(_sym.GetFlags(stname), kSFLG_Managed) &&
        !FlagIsSet(tqs, kTQ_Attribute) &&
        !is_function)
    {
        cc_error("Member variable of a managed struct cannot be managed");
        return -1;
    }

    if (kPP_Main == _pp && !is_function)
    {
        if (_sym.GetSymbolType(mangled_name) != 0)
        {
            std::string msg = ReferenceMsgSym(
                "'%s' is already defined", mangled_name);
            cc_error(
                msg.c_str(),
                _sym.GetName(mangled_name).c_str());
            return -1;
        }

        // Mustn't be in any ancester
        retval = ParseStruct_CheckForCompoInAncester(stname, component, parent);
        if (retval < 0) return retval;
    }

    
    if (kPP_Main == _pp)
    {
        // All struct members get this flag, even functions
        SetFlag(_sym[mangled_name].Flags, kSFLG_StructMember, true);
        _sym.SetDeclared(mangled_name, ccCurScriptName, currentline);
    }

    if (is_function)
    {
        if (current_func > 0)
        {
            cc_error("Cannot declare struct member function inside a function body");
            return -1;
        }
        return ParseStruct_Function(tqs, vartype, stname, mangled_name, current_func);
    }

    return ParseStruct_VariableOrAttribute(tqs, vartype, stname, mangled_name, size_so_far);
}

int AGS::Parser::EatDynpointerSymbolIfPresent(Vartype vartype)
{
    if (_sym.getPointerSym() != _targ.peeknext())
        return 0;

    if (kPP_PreAnalyze == _pp || _sym.IsManaged(vartype))
    {
        _targ.getnext(); // Eat '*'
        return 0;
    }

    cc_error("Cannot use '*' for a non-managed type");
    return -1;
}

int AGS::Parser::ParseStruct_MemberStmt(AGS::Symbol stname, AGS::Symbol name_of_current_func, AGS::Symbol parent, size_t &size_so_far)
{
    // parse qualifiers of the member ("import" etc.), set booleans accordingly
    TypeQualifierSet tqs = 0;
    ParseStruct_MemberQualifiers(tqs);
    if (FlagIsSet(tqs, kTQ_Protected) && FlagIsSet(tqs, kTQ_Writeprotected))
    {
        cc_error("Field cannot be both protected and write-protected.");
        return -1;
    }
    AGS::Vartype vartype = _targ.getnext();

    SymbolTableEntry &entry = _sym[vartype];
    if (FlagIsSet(entry.Flags, kSFLG_Managed))
        vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
    int retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    if (kPP_Main == _pp)
    {
        // Certain types of members are not allowed in structs; check this
        retval = ParseStruct_CheckComponentVartype(stname, vartype, FlagIsSet(tqs, kTQ_Import));
        if (retval < 0) return retval;
    }

    // run through all variables declared on this member defn.
    while (true)
    {
        retval = ParseStruct_MemberDefnVarOrFuncOrArray(parent, stname, name_of_current_func, tqs, vartype, size_so_far);
        if (retval < 0) return retval;

        if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_Comma)
        {
            _targ.getnext(); // Eat ','
            continue;
        }
        break;
    }

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    return 0;
}

// Handle a "struct" definition clause
int AGS::Parser::ParseStruct(TypeQualifierSet tqs, AGS::NestingStack &nesting_stack, AGS::Symbol name_of_current_func, AGS::Symbol struct_of_current_func)
{
    // get token for name of struct
    AGS::Symbol const stname = _targ.getnext();

    if ((_sym.GetSymbolType(stname) != 0) &&
        (_sym.GetSymbolType(stname) != kSYM_UndefinedStruct))
    {
        cc_error("'%s' is already defined", _sym.GetName(stname).c_str());
        return -1;
    }

    ParseStruct_SetTypeInSymboltable(stname, tqs);

    // Declare the struct type that implements new strings
    if (FlagIsSet(tqs, kTQ_Stringstruct))
    {
        if (_sym.getStringStructSym() > 0 && stname != _sym.getStringStructSym())
        {
            cc_error("The stringstruct type is already defined to be %s", _sym.GetName(_sym.getStringStructSym()).c_str());
            return -1;
        }
        _sym.setStringStructSym(stname);
    }

    size_t size_so_far = 0; // Will sum up the size of the struct

    // If the struct extends another struct, the token of the other struct's name
    AGS::Symbol parent = 0;

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_Extends)
        ParseStruct_ExtendsClause(stname, parent, size_so_far);

    // forward-declaration of struct type
    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_Semicolon)
    {
        if (!FlagIsSet(tqs, kTQ_Managed))
        {
            cc_error("Forward-declared structs must be 'managed'");
            return -1;
        }
        _targ.getnext(); // Eat ';'
        SymbolTableEntry &entry = _sym[stname];
        entry.SType = kSYM_UndefinedStruct;
        SetFlag(entry.Flags, kSFLG_Managed, true);
        entry.SSize = 0;
        return 0;
    }

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_OpenBrace)
    {
        cc_error("Expected '{'");
        return -1;
    }

    // Declaration of the components
    while (_sym.GetSymbolType(_targ.peeknext()) != kSYM_CloseBrace)
    {
        int retval = ParseStruct_MemberStmt(stname, name_of_current_func, parent, size_so_far);
        if (retval < 0) return retval;
    }

    if (kPP_Main == _pp)
    {
        // align struct on 4-byte boundary in keeping with compiler
        if ((size_so_far % 4) != 0)
            size_so_far += 4 - (size_so_far % 4);
        _sym[stname].SSize = size_so_far;
    }

    _targ.getnext(); // Eat '}'

    Symbol const nextsym = _targ.peeknext();
    SymbolType const type_of_next = _sym.GetSymbolType(nextsym);
    if (kSYM_Semicolon == type_of_next)
    {
        _targ.getnext(); // Eat ';'
        return 0;
    }

    // Assume that this is a declaration
    return ParseVartype0(stname, &nesting_stack, tqs, name_of_current_func, struct_of_current_func);
}

// We've accepted something like "enum foo { bar"; '=' follows
int AGS::Parser::ParseEnum_AssignedValue(int &currentValue)
{
    _targ.getnext(); // eat "="

    // Get the value of the item
    AGS::Symbol item_value = _targ.getnext(); // may be '-', too
    bool is_neg = false;
    if (item_value == _sym.Find("-"))
    {
        is_neg = true;
        item_value = _targ.getnext();
    }

    return ParseLiteralOrConstvalue(item_value, currentValue, is_neg, "Expected integer or integer constant after '='");
}

void AGS::Parser::ParseEnum_Item2Symtable(AGS::Symbol enum_name, AGS::Symbol item_name, int currentValue)
{
    SymbolTableEntry &entry = _sym[item_name];

    entry.SType = kSYM_Constant;
    entry.vartype = enum_name;
    entry.SScope = 0;
    entry.Flags = kSFLG_Readonly;
    // soffs is unused for a constant, so in a gratuitous hack we use it to store the enum's value
    entry.SOffset = currentValue;
    if (kPP_Main == _pp)
        _sym.SetDeclared(item_name, ccCurScriptName, currentline);
}

int AGS::Parser::ParseEnum_Name2Symtable(AGS::Symbol enumName)
{
    SymbolTableEntry &entry = _sym[enumName];

    if (0 != entry.SType)
    {
        std::string msg = ReferenceMsg(
            "'%s' is already defined",
            _sym.Id2Section(entry.DeclSectionId),
            entry.DeclLine);
        cc_error(msg.c_str(), _sym.GetName(enumName).c_str());
        return -1;
    }

    entry.SType = kSYM_Vartype;
    entry.SSize = SIZE_OF_INT;
    entry.vartype = _sym.getIntSym();

    return 0;
}

// enum EnumName { value1, value2 }
int AGS::Parser::ParseEnum0()
{
    // Get name of the enum, enter it into the symbol table
    AGS::Symbol enum_name = _targ.getnext();
    int retval = ParseEnum_Name2Symtable(enum_name);
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_OpenBrace)
    {
        cc_error("Expected '{'");
        return -1;
    }

    int currentValue = 0;

    while (true)
    {
        AGS::Symbol item_name = _targ.getnext();
        if (_sym.GetSymbolType(item_name) == kSYM_CloseBrace)
            break; // item list empty or ends with trailing ','

        if (kPP_Main == _pp)
        {
            if (_sym.GetSymbolType(item_name) == kSYM_Const) 
            {
                std::string msg =
                    ReferenceMsgSym("'%s' is already defined as a constant or enum value", item_name);
                cc_error(msg.c_str(), _sym.GetName(item_name).c_str());
                return -1;
            }
            if (_sym.GetSymbolType(item_name) != 0) 
            {
                cc_error("Expected '}' or an unused identifier, found '%s' instead", _sym.GetName(item_name).c_str());
                return -1;
            }
        }

        // increment the value of the enum entry
        currentValue++;

        SymbolType type_of_next = _sym.GetSymbolType(_targ.peeknext());
        if (type_of_next != kSYM_Assign && type_of_next != kSYM_Comma && type_of_next != kSYM_CloseBrace)
        {
            cc_error("Expected '=' or ',' or '}'");
            return -1;
        }

        if (type_of_next == kSYM_Assign)
        {
            // the value of this entry is specified explicitly
            retval = ParseEnum_AssignedValue(currentValue);
            if (retval < 0) return retval;
        }

        // Enter this enum item as a constant int into the _sym table
        ParseEnum_Item2Symtable(enum_name, item_name, currentValue);

        AGS::Symbol comma_or_brace = _targ.getnext();
        if (_sym.GetSymbolType(comma_or_brace) == kSYM_CloseBrace)
            break;
        if (_sym.GetSymbolType(comma_or_brace) == kSYM_Comma)
            continue;

        cc_error("Expected ',' or '}'");
        return -1;
    }
    return 0;
}

// enum eEnumName { value1, value2 };
int AGS::Parser::ParseEnum(AGS::Symbol name_of_current_function)
{
    if (name_of_current_function >= 0)
    {
        cc_error("Enum declaration not allowed within a function body");
        return -1;
    }

    int retval = ParseEnum0();
    if (retval < 0) return retval;

    // Force a semicolon after the declaration
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}

int AGS::Parser::ParseExport()
{
    if (kPP_PreAnalyze == _pp)
    {
        const SymbolType stoplist[] = { kSYM_Semicolon };
        SkipTo(stoplist, 1);
        _targ.getnext(); // Eat ';'
        return 0;
    }

    // export specified symbols
    AGS::Symbol cursym = _targ.getnext();
    while (_sym.GetSymbolType(cursym) != kSYM_Semicolon)
    {
        SymbolType const curtype = _sym.GetSymbolType(cursym);
        if (curtype == 0)
        {
            cc_error("Can only export global variables and functions, not '%s'", _sym.GetName(cursym).c_str());
            return -1;
        }
        if ((curtype != kSYM_GlobalVar) && (curtype != kSYM_Function))
        {
            cc_error("Invalid export symbol '%s'", _sym.GetName(cursym).c_str());
            return -1;
        }
        if (FlagIsSet(_sym.GetFlags(cursym), kSFLG_Imported))
        {
            cc_error("Cannot export an import");
            return -1;
        }
        if (_sym.GetVartype(cursym) == _sym.getOldStringSym())
        {
            cc_error("Cannot export string; use char[200] instead");
            return -1;
        }
        // if all functions are being exported anyway, don't bother doing
        // it now
        if ((ccGetOption(SCOPT_EXPORTALL) != 0) && (curtype == kSYM_Function))
        {
        }
        else if (_scrip.add_new_export(_sym.GetName(cursym).c_str(),
            (curtype == kSYM_GlobalVar) ? EXPORT_DATA : EXPORT_FUNCTION,
            _sym[cursym].SOffset, _sym[cursym].SScope) == -1)
        {
            return -1;
        }
        if (_targ.reached_eof())
        {
            cc_error("Unexpected end of input");
            return -1;
        }
        cursym = _targ.getnext();
        if (_sym.GetSymbolType(cursym) == kSYM_Semicolon)
            break;
        if (_sym.GetSymbolType(cursym) != kSYM_Comma)
        {
            cc_error("Expected ',' instead of '%s'", _sym.GetName(cursym).c_str());
            return -1;
        }
        cursym = _targ.getnext();
    }

    return 0;
}

int AGS::Parser::ParseVartype_GetVarName(AGS::Symbol &varname, AGS::Symbol &struct_of_member_fct)
{
    struct_of_member_fct = 0;

    varname = _targ.getnext();
    SymbolType const vartype = _sym.GetSymbolType(varname);
    if (kSYM_NoType != vartype &&
        kSYM_Function != vartype &&
        kSYM_Vartype != vartype &&
        kSYM_GlobalVar != vartype &&
        kSYM_LocalVar != vartype)
    {
        cc_error("Unexpected '%s'", _sym.GetName(varname).c_str());
        return -1;
    }

    if (_sym.GetSymbolType(_targ.peeknext()) != kSYM_MemberAccess)
        return 0; // done

    // We are accepting "struct::member"; so varname isn't the var name yet: it's the struct name.
    struct_of_member_fct = varname;
    _targ.getnext(); // gobble "::"
    AGS::Symbol member_of_member_function = _targ.getnext();

    // change varname to be the full function name
    varname = MangleStructAndComponent(struct_of_member_fct, member_of_member_function);
    if (varname < 0)
    {
        cc_error("'%s' does not contain a function '%s'",
            _sym.GetName(struct_of_member_fct).c_str(),
            _sym.GetName(member_of_member_function).c_str());
        return -1;
    }

    return 0;
}

int AGS::Parser::ParseVartype_CheckForIllegalContext(AGS::NestingStack *nesting_stack)
{
    if (nesting_stack->IsUnbraced())
    {
        cc_error("A variable or function declaration cannot be the sole body of an 'if', 'else' or loop clause");
        return -1;
    }
    if (nesting_stack->Type() == AGS::NestingStack::kNT_Switch)
    {
        cc_error("This variable declaration may be skipped by case label. Use braces to limit its scope or move it outside the switch statement block");
        return -1;
    }
    return 0;
}

int AGS::Parser::ParseVartype_CheckIllegalCombis(bool is_function, AGS::TypeQualifierSet tqs)
{
    if (FlagIsSet(tqs, kTQ_Static) && !is_function)
    {
        cc_error("'static' only applies to member functions");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected) && is_function)
    {
        cc_error("'protected' not valid for functions");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Readonly) && is_function)
    {
        cc_error("Readonly cannot be applied to a function");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Writeprotected) && is_function)
    {
        cc_error("'writeprotected' not valid for functions");
        return -1;
    }

    return 0;
}

int AGS::Parser::ParseVartype_FuncDef(AGS::Symbol &func_name, AGS::Vartype vartype, TypeQualifierSet tqs, bool no_loop_check, AGS::Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func)
{
    bool body_follows;

    // In the case of extender functions, this will alter func_name
    int retval = ParseFuncdecl(func_name, vartype, tqs, struct_of_current_func, body_follows);
    if (retval < 0) return retval;

    SymbolTableEntry &entry = _sym[func_name];
    if (struct_of_current_func > 0)
        SetFlag(entry.Flags, kSFLG_StructMember, true);

    if (kPP_PreAnalyze == _pp)
    {
        if (body_follows && kFT_LocalBody == entry.SOffset)
        {
            cc_error("This function has already been defined with a body");
            return -1;
        }

        // Encode in entry.SOffset the type of function declaration
        FunctionType ft = kFT_PureForward;
        if (FlagIsSet(tqs, kTQ_Import))
            ft = kFT_Import;
        if (body_follows)
            ft = kFT_LocalBody;
        if (entry.SOffset < ft)
            entry.SOffset = ft;
    }

    if (!body_follows)
    {
        if (no_loop_check)
        {
            cc_error("Can only use 'noloopcheck' when function body follows definition");
            return -1;
        }

        if (kSYM_Semicolon != _sym.GetSymbolType(_targ.getnext()))
        {
            cc_error("Expected ';'");
            return -1;
        }
        return 0;
    }

    if (no_loop_check)
        SetFlag(entry.Flags, kSFLG_NoLoopCheck, true);
    // We've started a function, remember what it is.
    name_of_current_func = func_name;
    return 0;
}

int AGS::Parser::ParseVartype_VarDecl_PreAnalyze(AGS::Symbol var_name, Globalness globalness, bool &another_var_follows)
{
    if (0 != _givm.count(var_name))
    {
        if (_givm[var_name])
        {
            cc_error("'%s' is already defined as a global non-import variable", _sym.GetName(var_name).c_str());
            return -1;
        }
        else if (kGl_GlobalNoImport == globalness && 0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            cc_error("'%s' is defined as an import variable; that cannot be overridden here", _sym.GetName(var_name).c_str());
            return -1;
        }
    }
    _givm[var_name] = (kGl_GlobalNoImport == globalness);

    // Apart from this, we aren't interested in var defns at this stage, so skip this defn
    SymbolType const stoplist[] = { kSYM_Comma, kSYM_Semicolon, };
    SkipTo(stoplist, 2);
    another_var_follows = false;
    if (kSYM_Comma == _sym.GetSymbolType(_targ.peeknext()))
    {
        another_var_follows = true;
        _targ.getnext(); // Eat ','
    }
    return 0;
}

int AGS::Parser::ParseVartype_VarDecl(AGS::Symbol &var_name, Globalness globalness, int nested_level, bool is_readonly, AGS::Vartype vartype, SymbolType next_type, bool &another_var_follows)
{
    if (kPP_PreAnalyze == _pp)
        return ParseVartype_VarDecl_PreAnalyze(var_name, globalness, another_var_follows);

    if (kGl_Local == globalness)
        _sym[var_name].SScope = nested_level;
    if (is_readonly)
        SetFlag(_sym[var_name].Flags, kSFLG_Readonly, true);

    // parse the definition
    return ParseVardecl(var_name, vartype, next_type, globalness, another_var_follows);
}

// We accepted a variable type such as "int", so what follows is a function or variable definition
int AGS::Parser::ParseVartype0(AGS::Vartype vartype, AGS::NestingStack *nesting_stack, TypeQualifierSet tqs, AGS::Symbol &name_of_current_func, AGS::Symbol &struct_of_current_func)
{
    if (_targ.reached_eof())
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    // Don't define variable or function where illegal in context.
    int retval = ParseVartype_CheckForIllegalContext(nesting_stack);
    if (retval < 0) return retval;

    SymbolTableEntry &vartype_entry = _sym[Vartype2Symbol(vartype)];

    if (_sym.getPointerSym() == _targ.peeknext())
    {
        if (!_sym.IsPrimitive(vartype) && !_sym.IsManaged(vartype))
        {
            cc_error("Can only use '*' for a managed or primitive type here");
            return -1;
        }

        vartype = _sym.VartypeWith(kVTT_Dynpointer, vartype);
        _targ.getnext(); // Eat '*'
    }

    // "int [] func(...)"
    retval = ParseDynArrayMarkerIfPresent(vartype);
    if (retval < 0) return retval;

    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    bool const no_loop_check = (kSYM_NoLoopCheck == _sym.GetSymbolType(_targ.peeknext()));
    if (no_loop_check)
         _targ.getnext();

    Globalness globalness = kGl_Local;
    if (name_of_current_func <= 0)
        globalness = FlagIsSet(tqs, kTQ_Import) ? kGl_GlobalImport : kGl_GlobalNoImport;

    bool another_ident_follows = false; // will become true when we gobble a "," after a var defn
    // We've accepted a type expression and are now reading vars or one func that should have this type.
    do
    {
        if (_targ.reached_eof())
        {
            cc_error("Unexpected end of input");
            return -1;
        }

        // Get the variable or function name.
        AGS::Symbol var_or_func_name = -1;
        retval = ParseVartype_GetVarName(var_or_func_name, struct_of_current_func);
        if (retval < 0) return retval;

        if (kSYM_Vartype == _sym.GetSymbolType(var_or_func_name) || _sym.IsPrimitive(var_or_func_name))
        {
            std::string msg =
                ReferenceMsgSym("'%s' is already in use as a type name", var_or_func_name);
            cc_error(msg.c_str(), _sym.GetName(var_or_func_name).c_str());
            return -1;
        }

        // Check whether var or func is being defined
        SymbolType const next_type = _sym.GetSymbolType(_targ.peeknext());
        bool const is_function = (kSYM_OpenParenthesis == next_type);

        // certains modifiers, such as "static" only go with certain kinds of definitions.
        retval = ParseVartype_CheckIllegalCombis(is_function, tqs);
        if (retval < 0) return retval;

        if (is_function)
        {
            if ((name_of_current_func >= 0) || (nesting_stack->Depth() > 1))
            {
                cc_error("Nested functions not supported (you may have forgotten a closing brace)");
                return -1;
            }
            SetDynpointerInManagedVartype(vartype);
            return ParseVartype_FuncDef(var_or_func_name, vartype, tqs, no_loop_check, struct_of_current_func, name_of_current_func);
        }

        if (_sym.IsDynarray(vartype) || no_loop_check) // e.g., int [] Zonk;
        {
            cc_error("Expected '('");
            return -1;
        }

        // A variable with a managed type is automatically pointered. 
        // However, special exception for global import variables
        if (kGl_GlobalImport != globalness)
            SetDynpointerInManagedVartype(vartype);
        retval = ParseVartype_VarDecl(var_or_func_name, globalness, nesting_stack->Depth() - 1, FlagIsSet(tqs, kTQ_Readonly), vartype, next_type, another_ident_follows);
        if (retval < 0) return retval;
    }
    while (another_ident_follows);

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}

int AGS::Parser::ParseCommand_EndOfDoIfElse(AGS::NestingStack *nesting_stack)
{
    // Unravel else ... else ... chains
    while (nesting_stack->IsUnbraced())
    {
        if (nesting_stack->Type() == AGS::NestingStack::kNT_UnbracedDo)
        {
            int retval = DealWithEndOfDo(nesting_stack);
            if (retval < 0) return retval;
            continue;
        }

        bool else_after_then;
        int retval = DealWithEndOfElse(nesting_stack, else_after_then);
        if (retval < 0) return retval;
        // If an else follows a then clause, it has been changed into an
        // else clause that has just started so this clause has NOT ended yet.
        // So the else ... else chain is broken at this point, so we break out of the loop.
        if (else_after_then)
            break;
    }
    return 0;
}

int AGS::Parser::ParseReturn(AGS::Symbol name_of_current_func)
{
    AGS::Symbol const functionReturnType = _sym[name_of_current_func].FuncParamTypes[0];

    if (_sym.GetSymbolType(_targ.peeknext()) != kSYM_Semicolon)
    {
        if (functionReturnType == _sym.getVoidSym())
        {
            cc_error("Cannot return value from void function");
            return -1;
        }

        // parse what is being returned
        int retval = ParseExpression();
        if (retval < 0) return retval;

        // If we need a string object ptr but AX contains a normal string, convert AX
        ConvertAXStringToStringObject(functionReturnType);

        // check return type is correct
        retval = IsVartypeMismatch(_scrip.ax_vartype, functionReturnType, true);
        if (retval < 0) return retval;

        if (_sym.IsOldstring(_scrip.ax_vartype) &&
            (kSYM_LocalVar == _scrip.ax_val_scope))
        {
            cc_error("Cannot return local string from function");
            return -1;
        }
    }
    else if (_sym.getIntSym() == functionReturnType)
    {
        _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    }
    else if (_sym.getVoidSym() != functionReturnType)
    {
        cc_error("Must return a '%s' value from function", _sym.GetName(functionReturnType).c_str());
        return -1;
    }

    AGS::Symbol const cursym = _targ.getnext();
    if (kSYM_Semicolon != _sym.GetSymbolType(cursym))
    {
        cc_error("Expected ';' instead of '%s'", _sym.GetName(cursym).c_str());
        return -1;
    }

    FreeDynpointersOfLocals(0, name_of_current_func);
    int totalsub = StacksizeOfLocals(0);
    if (totalsub > 0)
        _scrip.write_cmd2(SCMD_SUB, SREG_SP, totalsub);

    // Jump to the exit point of the function
    _scrip.write_cmd1(SCMD_JMP, 0);
    _fcm.TrackExitJumppoint(name_of_current_func, _scrip.codesize - 1);

    return 0;
}

// Evaluate the head of an "if" clause, e.g. "if (i < 0)".
int AGS::Parser::ParseIf(AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    // Get expression, must be in parentheses
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression();
    if (retval < 0) return retval;
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    _scrip.write_cmd1(SCMD_JZ, 0);
    AGS::CodeLoc jump_dest_loc = _scrip.codesize - 1;

    // Assume unbraced as a default
    retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedThen, // Type
        0, // Start
        jump_dest_loc); // Info

    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_OpenBrace)
    {
        _targ.getnext();
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedThen); // change to braced
    }

    return 0;
}


// Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
int AGS::Parser::ParseWhile(AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    // point to the start of the code that evaluates the condition
    AGS::CodeLoc condition_eval_loc = _scrip.codesize;

    // Get expression, must be in parentheses
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression();
    if (retval < 0) return retval;
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    _scrip.write_cmd1(SCMD_JZ, 0);
    AGS::CodeLoc jump_dest_loc = _scrip.codesize - 1;

    // Assume unbraced as a default
    retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedThen, // Type
        condition_eval_loc, // Start
        jump_dest_loc); // Info
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_OpenBrace)
    {
        _targ.getnext();
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedElse); // change to braced
    }
    return 0;
}

int AGS::Parser::ParseDo(AGS::NestingStack *nesting_stack)
{
    // We need a jump at a known location for the break command to work:
    _scrip.write_cmd1(SCMD_JMP, 2); // Jump past the next jump :D
    _scrip.write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the end of the loop
    // This points to the address we have to patch with a jump past the end of the loop
    AGS::CodeLoc jump_dest_loc = _scrip.codesize - 1;

    // Assume an unbraced DO as a default
    int retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedDo, // Type
        _scrip.codesize, // Start
        jump_dest_loc);  // Info
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_OpenBrace)
    {
        _targ.getnext();
        // Change to braced DO
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedDo);
    }

    return 0;
}

// We're compiling function body code; the code does not start with a keyword or type.
// Thus, we should be at the start of an assignment or a funccall. Compile it.
int AGS::Parser::ParseAssignmentOrFunccall(AGS::Symbol cursym)
{
    ccInternalList expr_script;
    expr_script.write(cursym); // expression starts with this
    int retval = BufferExpression(expr_script);
    expr_script.startread();
    if (retval < 0) return retval;

    AGS::Symbol nextsym = _targ.peeknext();
    SymbolType const nexttype = _sym.GetSymbolType(nextsym);

    if (expr_script.length > 0)
    {
        if (nexttype == kSYM_Assign || nexttype == kSYM_AssignMod || nexttype == kSYM_AssignSOp)
        {
            _targ.getnext();
            return ParseAssignment(nextsym, &expr_script);
        }
        ValueLocation vloc;
        int scope;
        AGS::Vartype vartype;
        retval = ParseExpression_Subexpr(expr_script.script, expr_script.length, vloc, scope, vartype);
        if (retval < 0) return retval;
        return ResultToAX(vloc, scope, vartype);
    }
    cc_error("Unexpected symbol '%s'", _sym.GetName(nextsym).c_str());
    return -1;
}

int AGS::Parser::ParseFor_InitClauseVardecl(size_t nested_level)
{
    AGS::Vartype vartype = _targ.getnext();
    SetDynpointerInManagedVartype(vartype);

    int retval = EatDynpointerSymbolIfPresent(vartype);
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_NoLoopCheck)
    {
        cc_error("'noloopcheck' is not applicable in this context");
        return -1;
    }

    bool another_var_follows = false;
    do
    {
        AGS::Symbol varname = _targ.getnext();
        if (_sym.GetSymbolType(varname) != 0)
        {
            std::string msg =
                ReferenceMsgSym("Variable '%s' is already defined", varname);
            cc_error(msg.c_str(), _sym.GetName(varname).c_str());
            return -1;
        }

        SymbolType const next_type = _sym.GetSymbolType(_targ.peeknext());
        if (kSYM_MemberAccess == next_type || kSYM_OpenParenthesis == next_type)
        {
            cc_error("Function definition not allowed in for loop initialiser");
            return -1;
        }

        _sym[varname].SScope = static_cast<short>(nested_level);
        retval = ParseVardecl(varname, vartype, next_type, kGl_Local, another_var_follows);
        if (retval < 0) return retval;
    }
    while (another_var_follows);
    return 0;
}

// The first clause of a for header
int AGS::Parser::ParseFor_InitClause(AGS::Symbol peeksym, size_t nested_level)
{
    _scrip.flush_line_numbers();

    if (_sym.GetSymbolType(peeksym) == kSYM_Semicolon)
        return 0; // Empty init clause
    if (_sym.GetSymbolType(peeksym) == kSYM_Vartype)
        return ParseFor_InitClauseVardecl(nested_level);
    return ParseAssignmentOrFunccall(_targ.getnext());
}

int AGS::Parser::ParseFor_WhileClause()
{
    _scrip.flush_line_numbers();

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_Semicolon)
    {
        // Not having a while clause is tantamount to the while condition "true".
        // So let's write "true" to the AX register.
        _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, 1);
        return 0;
    }

    return ParseExpression();
}

int AGS::Parser::ParseFor_IterateClause()
{
    _scrip.flush_line_numbers();
    // Check for empty interate clause
    if (kSYM_CloseParenthesis == _sym.GetSymbolType(_targ.peeknext()))
        return 0;

    return ParseAssignmentOrFunccall(_targ.getnext());
}

int AGS::Parser::ParseFor(AGS::Symbol &cursym, AGS::NestingStack *nesting_stack)
{
    // "for (I; E; C) { ...}" is equivalent to "{ I; while (E) { ...; C} }"
    // We implement this with TWO levels of the nesting stack.
    // The outer level contains "I"
    // The inner level contains "while (E) { ...; C}"

    // Outer level
    int retval = nesting_stack->Push(AGS::NestingStack::kNT_For);
    if (retval < 0) return retval;

    cursym = _targ.getnext();
    if (_sym.GetSymbolType(cursym) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '(' after 'for'");
        return -1;
    }

    AGS::Symbol peeksym = _targ.peeknext();
    if (_sym.GetSymbolType(peeksym) == kSYM_CloseParenthesis)
    {
        cc_error("Empty parentheses \"()\" aren't allowed after \"for\" (write \"for(;;)\" instead");
        return -1;
    }

    // Generate the initialization clause (I)
    retval = ParseFor_InitClause(peeksym, nesting_stack->Depth() - 1);
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';' after for loop initializer clause");
        return -1;
    }

    // Remember where the code of the while condition starts.
    AGS::CodeLoc while_cond_loc = _scrip.codesize;

    retval = ParseFor_WhileClause();
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';' after for loop while clause");
        return -1;
    }

    // Remember where the code of the iterate clause starts.
    AGS::CodeLoc iterate_clause_loc = _scrip.codesize;
    size_t pre_fixup_count = _scrip.numfixups;

    retval = ParseFor_IterateClause();
    if (retval < 0) return retval;
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')' after for loop iterate clause");
        return -1;
    }

    // Inner nesting level - assume unbraced as a default
    retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedElse, // Type
        while_cond_loc, // Start
        0); // Info
    if (retval < 0) return retval;

    if (_sym.GetSymbolType(_targ.peeknext()) == kSYM_OpenBrace)
    {
        _targ.getnext();
        // Set type "braced" instead of "unbraced"
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedElse);
    }

    // We've just generated code for getting to the next loop iteration.
    // But we don't need that code right here; we need it at the bottom of the loop.
    // So rip it out of the bytecode base and save it into our nesting stack.
    int id;
    size_t const yank_size = _scrip.codesize - iterate_clause_loc;
    nesting_stack->YankChunk(_scrip, iterate_clause_loc, pre_fixup_count, id);
    _fcm.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);
    _fim.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);

    // Code for "If the expression we just evaluated is false, jump over the loop body."
    // the 0 will be fixed to a proper offset later
    _scrip.write_cmd1(SCMD_JZ, 0);
    nesting_stack->SetJumpOutLoc(_scrip.codesize - 1); // the address to fix

    return 0;
}

int AGS::Parser::ParseSwitch(AGS::NestingStack *nesting_stack)
{
    // Get the switch expression, must be in parentheses
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression();
    if (retval < 0) return retval;
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }

    // Remember the type of this expression to enforce it later
    int switch_expr_type = _scrip.ax_vartype;

    // Copy the result to the BX register, ready for case statements
    _scrip.write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
    _scrip.flush_line_numbers();

    // Remember the start of the lookup table
    AGS::CodeLoc lookup_table_start = _scrip.codesize;

    _scrip.write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the lookup table
    _scrip.write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to beyond the switch statement (for break)

    // There's no such thing as an unbraced SWITCH, so '{' must follow
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_OpenBrace)
    {
        cc_error("Expected '{'");
        return -1;
    }

    retval = nesting_stack->Push(
        AGS::NestingStack::kNT_Switch, // Type
        lookup_table_start, // Start
        switch_expr_type); // Info
    if (retval < 0) return retval;
    nesting_stack->SetDefaultLabelLoc(-1);

    // Check that "default" or "case" follows
    if (_targ.reached_eof())
    {
        currentline = _targ.lineAtEnd;
        cc_error("Unexpected end of input");
        return -1;
    }
    if (_sym.GetSymbolType(_targ.peeknext()) != kSYM_Case && _sym.GetSymbolType(_targ.peeknext()) != kSYM_Default && _sym.GetSymbolType(_targ.peeknext()) != kSYM_CloseBrace)
    {
        cc_error("Invalid keyword '%s' in switch statement block", _sym.GetName(_targ.peeknext()).c_str());
        return -1;
    }
    return 0;
}

int AGS::Parser::ParseSwitchLabel(AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    if (nesting_stack->Type() != AGS::NestingStack::kNT_Switch)
    {
        cc_error("Case label not valid outside switch statement block");
        return -1;
    }

    if (_sym.GetSymbolType(cursym) == kSYM_Default)
    {
        if (nesting_stack->DefaultLabelLoc() != -1)
        {
            cc_error("This switch statement block already has a \"default:\" label");
            return -1;
        }
        nesting_stack->SetDefaultLabelLoc(_scrip.codesize);
    }
    else // "case"
    {
        AGS::CodeLoc start_of_code_loc = _scrip.codesize;
        int numfixups_at_start_of_code = _scrip.numfixups;
        // Push the switch variable onto the stack
        _scrip.push_reg(SREG_BX);

        int retval = ParseExpression();
        if (retval < 0) return retval;  // case n: label expression, result is in AX

        // check that the types of the "case" expression and the "switch" expression match
        retval = IsVartypeMismatch(_scrip.ax_vartype, nesting_stack->SwitchExprType(), false);
        if (retval < 0) return retval;

        // Pop the switch variable, ready for comparison
        _scrip.pop_reg(SREG_BX);

        // get the right equality operator for the type
        int eq_op = SCMD_ISEQUAL;
        retval = GetOperatorValidForVartype(_scrip.ax_vartype, nesting_stack->SwitchExprType(), eq_op);
        if (retval < 0) return retval;

        // [fw] Comparison operation may be missing here.

    // rip out the already generated code for the case/switch and store it with the switch
        int id;
        size_t const yank_size = _scrip.codesize - start_of_code_loc;
        nesting_stack->YankChunk(_scrip, start_of_code_loc, numfixups_at_start_of_code, id);
        _fcm.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
        _fim.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
    }

    // expect and gobble the ':'
    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Label)
    {
        cc_error("Expected ':'");
        return -1;
    }

    return 0;
}

int AGS::Parser::ParseBreak(AGS::NestingStack *nesting_stack)
{
    // Find the (level of the) looping construct to which the break applies
    size_t loop_level = nesting_stack->Depth() - 1;
    while (loop_level > 0 && nesting_stack->StartLoc(loop_level) == 0)
        loop_level--;

    if (loop_level == 0)
    {
        cc_error("Break only valid inside a loop or switch statement block");
        return -1;
    }

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // If locals contain pointers, free them
    FreeDynpointersOfLocals(loop_level - 1);

    // Pop local variables from the stack
    int totalsub = StacksizeOfLocals(loop_level - 1);
    if (totalsub > 0)
        _scrip.write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    _scrip.flush_line_numbers();

    // The jump out of the loop, below, may be a conditional jump.
    // So clear AX to make sure that the jump is executed.
    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

    // Jump to a jump to the end of the loop
    // The bytecode byte with the relative dest is at code[codesize+1]
    if (nesting_stack->Type(loop_level) == AGS::NestingStack::kNT_Switch)
        _scrip.write_cmd1(SCMD_JMP,
            ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, nesting_stack->StartLoc(loop_level) + 2));
    else
        _scrip.write_cmd1(SCMD_JMP,
            ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, nesting_stack->JumpOutLoc(loop_level) - 1));
    return 0;
}

int AGS::Parser::ParseContinue(AGS::NestingStack *nesting_stack)
{
    // Find the (level of the) looping construct to which the break applies
    size_t loop_level = nesting_stack->Depth() - 1;
    while (loop_level > 0 && nesting_stack->StartLoc(loop_level) == 0)
        loop_level--;

    if (loop_level == 0)
    {
        cc_error("Continue not valid outside a loop");
        return -1;
    }

    if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // If locals contain pointers, free them
    FreeDynpointersOfLocals(loop_level - 1);

    // Pop local variables from the stack
    int totalsub = StacksizeOfLocals(loop_level - 1);
    if (totalsub > 0)
        _scrip.write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    _scrip.flush_line_numbers();

    // if it's a for loop, drop the yanked chunk (loop increment)back in
    if (nesting_stack->ChunksExist(loop_level))
    {
        int id;
        AGS::CodeLoc const write_start = _scrip.codesize;
        nesting_stack->WriteChunk(_scrip, loop_level, 0, id);
        _fcm.UpdateCallListOnWriting(write_start, id);
        _fim.UpdateCallListOnWriting(write_start, id);
    }
    _scrip.flush_line_numbers();

    // original comment "The jump below may be a conditional jump."
    //  [fw] Nooo? Leave it in, anyway, so that we have identical bytecode
    // original comment "So clear AX to make sure that the jump is executed."
    _scrip.write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

    // Jump to the start of the loop
    // The bytecode int with the relative dest is at code[codesize+1]
    _scrip.write_cmd1(
        SCMD_JMP,
        ccCompiledScript::RelativeJumpDist(_scrip.codesize + 1, nesting_stack->StartLoc(loop_level)));
    return 0;
}

int AGS::Parser::ParseCommand(AGS::Symbol cursym, AGS::Symbol &name_of_current_func, AGS::Symbol &struct_of_current_func, AGS::NestingStack *nesting_stack)
{
    int retval;

    switch (_sym.GetSymbolType(cursym))
    {
    default:
        // If it doesn't begin with a keyword, it should be an assignment
        // or a func call.
        retval = ParseAssignmentOrFunccall(cursym);
        if (retval < 0) return retval;
        if (_sym.GetSymbolType(_targ.getnext()) != kSYM_Semicolon)
        {
            cc_error("Expected ';'");
            return -1;
        }
        break;

    case kSYM_Break:
        retval = ParseBreak(nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Case:
        retval = ParseSwitchLabel(cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case  kSYM_CloseBrace:
        return ParseClosebrace(nesting_stack, struct_of_current_func, name_of_current_func);

    case kSYM_Continue:
        retval = ParseContinue(nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Default:
        retval = ParseSwitchLabel(cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Do:
        return ParseDo(nesting_stack);

    case kSYM_For:
        return ParseFor(cursym, nesting_stack);

    case kSYM_If:
        return ParseIf(cursym, nesting_stack);

    case kSYM_OpenBrace:
        return ParseOpenbrace(nesting_stack, name_of_current_func, struct_of_current_func);

    case kSYM_Return:
        retval = ParseReturn(name_of_current_func);
        if (retval < 0) return retval;
        break;

    case kSYM_Switch:
        retval = ParseSwitch(nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_While:
        return ParseWhile(cursym, nesting_stack);
    }

    // sort out jumps when a single-line if or else has finished
    return ParseCommand_EndOfDoIfElse(nesting_stack);
}

int AGS::Parser::Parse_HandleLines(int &currentlinewas)
{
    if (currentline == -10)
        return 1; // end of stream was reached


    if ((currentline != currentlinewas) && (ccGetOption(SCOPT_LINENUMBERS) != 0))
    {
        _scrip.set_line_number(currentline);
        currentlinewas = currentline;
    }

    return 0;
}

int AGS::Parser::Parse_TQCombiError(TypeQualifierSet tqs)
{
    std::map<TypeQualifier, std::string> const tq2String =
    {
        { kTQ_Autoptr, "autoptr" },
        { kTQ_Const, "const" },
        { kTQ_ImportStd, "import" },
        { kTQ_ImportTry, "_tryimport" },
        { kTQ_Managed, "managed" },
        { kTQ_Protected, "protected" },
        { kTQ_Readonly, "readonly" },
        { kTQ_Static, "static" },
        { kTQ_Stringstruct, "stringstruct" },
    };
    std::string kw2 = "[sentinel]", kw1;
    for (auto tq_it = tq2String.begin(); tq_it != tq2String.end(); ++tq_it)
    {
        if (!FlagIsSet(tqs, tq_it->first))
            continue;
        kw1 = kw2;
        kw2 = tq_it->second;
    }

    cc_error("Cannot use '%s' together with '%s'", kw1.c_str(), kw2.c_str());
    return -1;
}

// Check whether the qualifiers that accumulated for this decl go together
int AGS::Parser::Parse_CheckTQ(TypeQualifierSet tqs, AGS::Symbol decl_type)
{
    if (FlagIsSet(tqs, kTQ_Autoptr))
    {
        if (!FlagIsSet(tqs, kTQ_Managed) || !FlagIsSet(tqs, kTQ_Builtin))
        {
            cc_error("'autoptr' must be used with 'managed' and 'builtin'");
            return -1;
        }
    }

    if (FlagIsSet(tqs, kTQ_Builtin) && kSYM_Struct != decl_type)
    {
        cc_error("'builtin' can only be used with structs");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Const))
    {
        cc_error("'const' is only valid for function parameters (use 'readonly' instead)");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Import) && 0 != (tqs & ~kTQ_Readonly &~kTQ_Import))
    {
        Parse_TQCombiError((tqs & ~kTQ_Readonly));
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Managed) && kSYM_Struct != decl_type)
    {
        cc_error("'managed' can only be used with structs");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected) && 0 != (tqs & ~kTQ_Static & ~kTQ_Readonly))
    {
        Parse_TQCombiError((tqs & ~kTQ_Static & ~kTQ_Readonly));
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Readonly) && kSYM_Vartype != decl_type)
    {
        cc_error("'readonly' can only be used in a type declaration");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Static) && kSYM_Vartype != decl_type)
    {
        cc_error("'static' can only be used in a type declaration");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Static) && 0 != (tqs & ~kTQ_Static & ~kTQ_Protected & ~kTQ_Readonly))
    {
        Parse_TQCombiError((tqs & ~kTQ_Static & ~kTQ_Readonly));
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Stringstruct) && (!FlagIsSet(tqs, kTQ_Autoptr)))
    {
        cc_error("'stringstruct' must be used in combination with 'autoptr'");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Writeprotected))
    {
        cc_error("'writeprotected' is only valid for struct fields");
        return -1;
    }

    if (kSYM_Export == decl_type && 0 != tqs)
    {
        Parse_TQCombiError(tqs);
        return -1;
    }
    return 0;
}

int AGS::Parser::ParseVartype(AGS::Symbol cursym, TypeQualifierSet tqs, AGS::NestingStack &nesting_stack, AGS::Symbol &name_of_current_func, AGS::Symbol &struct_of_current_func)
{
    // func or variable definition
    int retval = Parse_CheckTQ(tqs, kSYM_Vartype);
    if (retval < 0) return retval;
    Vartype const vartype = cursym;
    return ParseVartype0(vartype, &nesting_stack, tqs, name_of_current_func, struct_of_current_func);
}

void AGS::Parser::Parse_SkipToEndingBrace()
{
    // Skip to matching '}'
    SymbolType const stoplist[] = { kSYM_NoType, };
    SkipTo(stoplist, 0); // pass empty list
    _targ.getnext(); // Eat '}'
}

// Buffer for the script name
std::string _ScriptNameBuffer;

void AGS::Parser::Parse_StartNewSection(AGS::Symbol mangled_section_name)
{
    _ScriptNameBuffer = _sym.GetName(mangled_section_name).substr(18);
    _ScriptNameBuffer.pop_back(); // strip closing speech mark
    ccCurScriptName = _ScriptNameBuffer.c_str();
    currentline = 0;
    if (kPP_Main == _pp)
        _scrip.start_new_section(_ScriptNameBuffer.c_str());
}

int AGS::Parser::ParseInput()
{
    AGS::NestingStack nesting_stack;
    size_t nested_level = 0;

    AGS::Symbol struct_of_current_func = 0; // non-zero only when a struct member function is open
    AGS::Symbol name_of_current_func = -1;

    // Go through the list of tokens one by one. We start off in the global data
    // part - no code is allowed until a function definition is started
    currentline = 1; // This is an externally referenced, global variable. cc_internallist.cpp, cs_internallist_test.cpp, cs_parser.cpp
    int currentlinewas = 0;

    // This collects the qualifiers ("static" etc.);
    // it is reset whenever the qualifiers are used.
    TypeQualifierSet tqs = 0;

    while (!_targ.reached_eof())
    {
        AGS::Symbol cursym = _targ.getnext();

        if (0 == _sym.GetName(cursym).compare(0, 18, NEW_SCRIPT_TOKEN_PREFIX))
        {
            Parse_StartNewSection(cursym);
            continue;
        }

        int retval = Parse_HandleLines(currentlinewas);
        if (retval > 0)
            break; // end of input

        switch (_sym.GetSymbolType(cursym))
        {

        default: break;

        case 0:
            // let it through if "this" can be implied
            if (struct_of_current_func > 0)
            {
                AGS::Symbol combined = MangleStructAndComponent(struct_of_current_func, cursym);
                if (kSYM_NoType != _sym.GetSymbolType(combined))
                    break;
            }
            cc_error("Unexpected token '%s'", _sym.GetName(cursym).c_str());
            return -1;

        case kSYM_AutoPtr:
        {
            SetFlag(tqs, kTQ_Autoptr, true);
            continue;
        }

        case kSYM_Builtin:
        {
            SetFlag(tqs, kTQ_Builtin, true);
            continue;
        }

        case kSYM_Const:
        {
            SetFlag(tqs, kTQ_Const, true);
            continue;
        }

        case kSYM_Enum:
        {
            retval = Parse_CheckTQ(tqs, kSYM_Export);
            if (retval < 0) return retval;
            retval = ParseEnum(name_of_current_func);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case kSYM_Export:
        {
            retval = Parse_CheckTQ(tqs, kSYM_Export);
            if (retval < 0) return retval;
            retval = ParseExport();
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case kSYM_Import:
        {
            if (std::string::npos != _sym.GetName(cursym).find("_tryimport"))
                SetFlag(tqs, kTQ_ImportTry, true);
            else
                SetFlag(tqs, kTQ_ImportStd, true);
            if (name_of_current_func > 0)
            {
                cc_error("'import' not allowed inside function body");
                return -1;
            }
            continue;
        }

        case kSYM_InternalString:
        {
            SetFlag(tqs, kTQ_Stringstruct, true);
            continue;
        }

        case  kSYM_Managed:
        {
            SetFlag(tqs, kTQ_Managed, true);
            continue;
        }

        case kSYM_OpenBrace:
        {
            if (kPP_Main == _pp)
                break; // treat as a command, below the switch

            Parse_SkipToEndingBrace();
            tqs = 0;
            name_of_current_func = -1;
            struct_of_current_func = -1;
            continue;
        }

        case kSYM_Protected:
        {
            SetFlag(tqs, kTQ_Protected, true);
            if (name_of_current_func > 0)
            {
                cc_error("'protected' not allowed inside a function body");
                return -1;
            }
            continue;
        }

        case kSYM_ReadOnly:
        {
            SetFlag(tqs, kTQ_Readonly, true);
            continue;
        }

        case kSYM_Static:
        {
            SetFlag(tqs, kTQ_Static, true);
            if (name_of_current_func >= 0)
            {
                cc_error("'static' not allowed inside function body");
                return -1;
            }
            continue;
        }

        case  kSYM_Struct:
        {
            retval = Parse_CheckTQ(tqs, kSYM_Struct);
            if (retval < 0) return retval;
            retval = ParseStruct(tqs, nesting_stack, name_of_current_func, struct_of_current_func);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case kSYM_Vartype:
        {
            if (kSYM_Dot == _sym.GetSymbolType(_targ.peeknext()))
                break; // this is a static struct component function call, so a command
            retval = ParseVartype(cursym, tqs, nesting_stack, name_of_current_func, struct_of_current_func);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        } // switch (symType)

        // Commands are only allowed within a function
        if (name_of_current_func <= 0)
        {
            cc_error("'%s' is illegal outside a function", _sym.GetName(cursym).c_str());
            return -1;
        }

        retval = ParseCommand(cursym, name_of_current_func, struct_of_current_func, &nesting_stack);
        if (retval < 0) return retval;
        tqs = 0;
    } // while (!targ.reached_eof())

    return 0;
}

// Copy all the func headers from the PreAnalyse phase into the "real" symbol table
int AGS::Parser::Parse_ReinitSymTable(const ::SymbolTable &tokenize_res)
{
    size_t const tokenize_res_size = tokenize_res.entries.size();
    SymbolTableEntry empty;
    empty.SType = kSYM_NoType;

    for (size_t sym_idx = 0; sym_idx < _sym.entries.size(); sym_idx++)
    {
        SymbolTableEntry &s_entry = _sym[sym_idx];
        if (s_entry.SType == kSYM_Function)
        {
            SetFlag(s_entry.Flags, kSFLG_Imported, (kFT_Import == s_entry.SOffset));
            s_entry.SOffset = 0;
            continue;
        }
        std::string const sname = s_entry.SName;
        s_entry =
            (sym_idx < tokenize_res_size) ? tokenize_res.entries[sym_idx] : empty;
        s_entry.SName = sname;
    }

    // This has invalidated the symbol table caches, so kill them
    _sym.ResetCaches();

    return 0;
}

// blank out the name for imports that are not used, to save space
// in the output file       
int AGS::Parser::Parse_BlankOutUnusedImports()
{
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        SymbolType const stype = _sym.GetSymbolType(entries_idx);
        // Don't mind attributes - they are shorthand for the respective getter
        // and setter funcs. If _those_ are unused, then they will be caught
        // in the same that way normal functions are.
        if (kSYM_Function != stype && kSYM_GlobalVar != stype)
            continue;
 
        if (FlagIsSet(_sym[entries_idx].Flags, kSFLG_Imported) &&
            !FlagIsSet(_sym[entries_idx].Flags, kSFLG_Accessed))
                _scrip.imports[_sym[entries_idx].SOffset][0] = '\0';
        }

    return 0;
}

int AGS::Parser::Parse_PreAnalyzePhase()
{
    // Needed to partially reset the symbol table later on
    SymbolTable const tokenize_res(_sym);

    _pp = kPP_PreAnalyze;
    int retval = ParseInput();
    if (retval < 0) return retval;

    _fcm.Reset();

    // Keep (just) the headers of functions that have a body to the main symbol table
    // Reset everything else in the symbol table,
    // but keep the entries so that they are guaranteed to have
    // the same index when parsed in phase 2
    return Parse_ReinitSymTable(tokenize_res);
    
}

int AGS::Parser::Parse_MainPhase()
{
    _pp = kPP_Main;
    return ParseInput();
}

int AGS::Parser::Parse()
{
    AGS::CodeLoc const start_of_input = _targ.pos;

    int retval = Parse_PreAnalyzePhase();
    if (retval < 0) return retval;

    _targ.pos = start_of_input;
    retval = Parse_MainPhase();
    if (retval < 0) return retval;

    retval = _fcm.CheckForUnresolvedFuncs();
    if (retval < 0) return retval;
    retval = _fim.CheckForUnresolvedFuncs();
    if (retval < 0) return retval;
    return Parse_BlankOutUnusedImports();
}

// Scan inpl into scan tokens, write line number opcodes, build a symbol table
int cc_tokenize(const char *inpl, ccInternalList *targ, ccCompiledScript *scrip, SymbolTable &sym)
{
    AGS::Scanner scanner(inpl, 1, targ);
    AGS::Tokenizer tokenizer(&scanner, targ, &sym, scrip);

    // Write pseudo opcode for "Starting line 1"
    targ->write_meta(SMETA_LINENUM, 1);

    bool eof_encountered = false;
    bool error_encountered = false;
    while (true)
    {
        AGS::Symbol token;
        tokenizer.GetNextToken(token, eof_encountered, error_encountered);
        if (error_encountered)
        {
            currentline = tokenizer.GetLineno();
            cc_error(tokenizer.GetLastError().c_str());
            return -1;
        }
        if (eof_encountered || error_encountered)
            break;
        targ->write(token);
    }

    // Write pseudo opcode for "This ends this tokenization"
    targ->write_meta(SMETA_END, 0);

    return 0;
}

int cc_parse(ccInternalList *targ, ccCompiledScript *scrip, SymbolTable &sym)
{
    targ->startread();
    Parser parser(sym, *targ, *scrip);
    return parser.Parse();
}

int cc_compile(const char *inpl, ccCompiledScript *scrip)
{
    ccInternalList targ;
    SymbolTable sym;

    int retval = cc_tokenize(inpl, &targ, scrip, sym);
    if (retval < 0) return retval;

    return cc_parse(&targ, scrip, sym);
}

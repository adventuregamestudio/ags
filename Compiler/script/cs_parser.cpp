/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - IMPLEMENTATION

General:
The origin of this module is C code, so there are lots of functions that haven't been converted
to classes (yet). These functions have names of the form AaaAaa or AaaAaa_BbbBbb
where the component parts are already camelcased. This means that function AaaAaa_BbbBbb is a
subfunction of function AaaAaa that is exclusively called by function AaaAaa.
In this way, we get a neatly grouped list of functions in the overview for the time being,
until function AaaAaa and its subfunctions have been converted into a proper class.

There shouldn't be any classes in the global namespace, but almost all the functions and
structs that have C code origin still are.

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
        In ParseExpression() and ParseExpression_Subexpr()
        Note that "++" and "--" are treated as assignment symbols, not as operators.

    Memory access
        In MemoryAccess() and AccessData()
        In order to read data or write to data, the respective piece of data must
        be located first. This also encompasses literals of the program code.
        Note that "." and "[]" are not treated as normal operators (operators like +).
        Rather, a whole term such as a.b.c[d].e(f, g, h) is read in all at once and
        processed together. The memory offset of struct components in relation to the
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

#include "cc_variablesymlist.h"
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

int ParseExpression(ccInternalList *targ, ccCompiledScript *script);
int ParseExpression_Subexpr(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len);
int ReadDataIntoAX(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len, bool negate = false);
void FreePointersOfStdArray(ccCompiledScript *scrip, SymbolTableEntry &entry, bool &clobbers_ax);

// [fw] This ought to replace the #defines in script_common.h
enum FxFixupType // see script_common.h
{
    kFx_NoFixup = 0,
    kFx_DataData = FIXUP_DATADATA,     // globaldata[fixup] += &globaldata[0]
    kFx_Function = FIXUP_FUNCTION,     // code[fixup] += &code[0]
    kFx_GlobalData = FIXUP_GLOBALDATA, //  code[fixup] += &globaldata[0]
    kFx_Import = FIXUP_IMPORT,         // code[fixup] = &imported_thing[code[fixup]]
    kFx_Stack = FIXUP_STACK,           // code[fixup] += &stack[0]
    kFx_String = FIXUP_STRING,         // code[fixup] += &strings[0]
    
};

enum Globalness
{
    kGl_Local = 0,            // Local
    kGl_GlobalNoImport = 1,   // Global, not imported
    kGl_GlobalImport = 2,     // Global, imported
};

enum Importness
{
    kIm_NoImport = 0,
    kIm_ImportType1 = 1,
    kIm_ImportType2 = 2,
};

enum TypeQualifier
{
    kTQ_Attribute = 1 << 0,
    kTQ_Autoptr = 1 << 1,
    kTQ_Builtin = 1 << 2,
    kTQ_Const = 1 << 3,
    kTQ_ImportStd = 1 << 4,
    kTQ_ImportTry = 1 << 5,
    kTQ_Noloopcheck = 1 << 6,
    kTQ_Managed = 1 << 7,
    kTQ_Protected = 1 << 8,
    kTQ_Readonly = 1 << 9,
    kTQ_Static = 1 << 10,
    kTQ_Stringstruct = 1 << 11,
    kTQ_Writeprotected = 1 << 12,
    kTQ_Import = kTQ_ImportStd | kTQ_ImportTry,
};

inline bool FlagIsSet(AGS::Flags fl_set, long flag) { return 0 != (fl_set & flag); }
inline void SetFlag(AGS::Flags &fl_set, long flag, bool val) { if (val) fl_set |= flag; else fl_set &= ~flag; }


bool IsIdentifier(AGS::Symbol symb)
{
    if (symb <= sym.getLastPredefSym() || symb > static_cast<int>(sym.entries.size()))
        return false;
    std::string name = sym.get_name_string(symb);
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

bool ReachedEOF(ccInternalList *targ)
{
    if (targ->peeknext() != SCODE_INVALID)
        return false;

    // We are past the last symbol in the file
    targ->getnext();
    currentline = targ->lineAtEnd;
    return true;
}

int String2Int(std::string str, int &val, bool send_error)
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

AGS::Symbol MangleStructAndComponent(AGS::Symbol stname, AGS::Symbol component)
{
    std::string fullname_str = sym.get_name_string(stname) + "::" + sym.get_name_string(component);
    return sym.find_or_add(fullname_str.c_str());
}

// Skim through the input, ignoring delimited content completely.
// Stop in the following cases:
//   A symbol is encountered whose type is in stoplist[]
//   A closing symbol is encountered that hasn't been opened.
// Don't consume the symbol that stops the scan.
int SkipTo(ccInternalList *targ, const AGS::Symbol stoplist[], size_t stoplist_len)
{
    int delimeter_nesting_depth = 0;
    while (!ReachedEOF(targ))
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        AGS::Symbol const cursym = targ->peeknext();
        SymbolType const curtype = sym.get_type(cursym);
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
        targ->getnext();
    }
    return -1;
}

int SkipToScript0(AGS::Symbol *end_sym_ptr, const AGS::Symbol stoplist[], size_t stoplist_len, AGS::Symbol *&act_sym_ptr)
{
    int delimeter_nesting_depth = 0;

    for (; act_sym_ptr != end_sym_ptr; act_sym_ptr++)
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        AGS::Symbol const cursym = *act_sym_ptr;
        SymbolType const curtype = sym.get_type(cursym);
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
int SkipToScript(const AGS::Symbol stoplist[], size_t stoplist_len, SymbolScript &symlist, size_t &symlist_len)
{
    SymbolScript const end_ptr = symlist + symlist_len;
    int retval = SkipToScript0(end_ptr, stoplist, stoplist_len, symlist);
    symlist_len = end_ptr - symlist; // Get new length of the symbol script

    return retval;
}

// Returns the relative distance in a jump instruction
// "here" is the location of the bytecode int that will contain
// the (relative) destination.It is not the location of the
// start of the command but the location of its first parameter
inline int RelativeJumpDist(AGS::CodeLoc here, AGS::CodeLoc dest)
{
    // JMP 0 jumps to the bytecode symbol directly behind the command.
    // So if dest == here, -1 must be returned.
    return static_cast<int>(dest) - static_cast<int>(here) - 1;
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
void AGS::NestingStack::YankChunk(ccCompiledScript *scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset, int &id)
{
    AGS::NestingStack::Chunk item;

    size_t const codesize = std::max(0, scrip->codesize);
    for (size_t code_idx = codeoffset; code_idx < codesize; code_idx++)
        item.Code.push_back(scrip->code[code_idx]);

    size_t numfixups = std::max(0, scrip->numfixups);
    for (size_t fixups_idx = fixupoffset; fixups_idx < numfixups; fixups_idx++)
    {
        item.Fixups.push_back(scrip->fixups[fixups_idx]);
        item.FixupTypes.push_back(scrip->fixuptypes[fixups_idx]);
    }
    item.CodeOffset = codeoffset;
    item.FixupOffset = fixupoffset;
    item.Id = id = ++_chunkIdCtr;

    _stack.back().Chunks.push_back(item);

    // Cut out the code that has been pushed
    scrip->codesize = codeoffset;
    scrip->numfixups = fixupoffset;
}

// Copy the code in the chunk to the end of the bytecode vector 
void AGS::NestingStack::WriteChunk(ccCompiledScript *scrip, size_t level, size_t index, int &id)
{
    AGS::NestingStack::Chunk const item = Chunks(level).at(index);
    id = item.Id;
    scrip->flush_line_numbers();
    AGS::CodeLoc adjust = scrip->codesize - item.CodeOffset;

    size_t limit = item.Code.size();
    for (size_t index = 0; index < limit; index++)
        scrip->write_code(item.Code[index]);

    limit = item.Fixups.size();
    for (size_t index = 0; index < limit; index++)
        scrip->add_fixup(item.Fixups[index] + adjust, item.FixupTypes[index]);
}

int AGS::FuncCallpointMgr::Init()
{
    _funcCallpointMap.clear();
    return 0;
}

int AGS::FuncCallpointMgr::TrackForwardDeclFuncCall(::ccCompiledScript *scrip, Symbol func, CodeLoc loc)
{
    // Patch callpoint in when known
    CodeCell const callpoint = _funcCallpointMap[func].Callpoint;
    if (callpoint >= 0)
    {
        scrip->code[loc] = callpoint;
        return 0;
    }

    // Callpoint not known, so remember this location
    PatchInfo pinfo;
    pinfo.ChunkId = CodeBaseId;
    pinfo.Offset = loc;
    _funcCallpointMap[func].List.push_back(pinfo);

    return 0;
}

int AGS::FuncCallpointMgr::TrackExitJumppoint(::ccCompiledScript *scrip, Symbol func, CodeLoc loc)
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
            if (patch_info.Offset < chunk_start || patch_info.Offset >= chunk_end)
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

int AGS::FuncCallpointMgr::SetFuncCallpoint(ccCompiledScript *scrip, AGS::Symbol func, AGS::CodeLoc dest)
{
    _funcCallpointMap[func].Callpoint = dest;
    PatchList &pl = _funcCallpointMap[func].List;
    size_t const pl_size = pl.size();
    bool yanked_patches_exist = false;
    for (size_t pl_idx = 0; pl_idx < pl_size; ++pl_idx)
        if (pl[pl_idx].ChunkId == CodeBaseId)
        {
            scrip->code[pl[pl_idx].Offset] = dest;
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

int AGS::FuncCallpointMgr::SetFuncExitJumppoint(ccCompiledScript *scrip, AGS::Symbol func, AGS::CodeLoc dest)
{
    // Exit points of functions are stored under the negative function number
    _funcCallpointMap[-func].Callpoint = dest;
    PatchList &pl = _funcCallpointMap[-func].List;
    size_t const pl_size = pl.size();
    for (auto pl_it = pl.begin(); pl_it != pl.end(); ++pl_it)
        if (pl_it->ChunkId == CodeBaseId)
        {
            size_t const here = pl_it->Offset;
            scrip->code[here] = RelativeJumpDist(here, dest);
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

            cc_error("Function '%s()' has been called but not defined with body nor imported", sym.get_name_string(fcm_it->first).c_str());
            return -1;
        }
    }
    return 0;
}

AGS::FuncCallpointMgr::CallpointInfo::CallpointInfo()
    : Callpoint(-1)
{ }

// Manage a map of all the functions that have bodies (in the current source).
// Nearly all the functions need this directly or indirectly, and so
// it is here.
// [TODO] Convert this into a class variable when the compiler has been
//        converted to classes
AGS::FuncCallpointMgr g_FCM;

void AGS::ImportMgr::Init(ccCompiledScript *scrip)
{
    _scrip = scrip;
    _importIdx.clear();
    for (size_t import_idx = 0; import_idx < scrip->numimports; import_idx++)
        _importIdx[scrip->imports[import_idx]] = import_idx;
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

// Measurements show that the checks whether imports already exist take up
// considerable time. The Import Manager speeds this up by caching the lookups.
// [TODO] When the compiler has been converted to classes, this can become
//        a class variable. Or it might be integrated into ccCompiledScript.
AGS::ImportMgr g_ImportMgr;

// Global Import Variable Manager
// Manage a list of all global import variables and track whether they are
// re-defined as non-import later on.
// Symbol maps to TRUE if it is global import, to FALSE if it is global non-import.
// Only a global import may have a repeated identical definition.
// Only a global import may be re-defined as a global non-import (identical except for the "import" declarator),
//    and this may only happen if the options don't forbid this.
// [TODO] Convert this into a class variable when the compiler has been
//        converted to classes

std::map<AGS::Symbol, bool> g_GIVM;

// Parsing Phase: Track the phase the parser is in.
// This is used ubiquitously, so defined as a global variable.
// [TODO] Convert this into a class variable when the compiler has been
//        converted to classes
enum ParsingPhases
{
    kPP_PreAnalyze = 0, // A pre-phase that finds out, amongst others, what functions have (local) bodies
    kPP_Main,           // The main phase that generates the bytecode.
} g_PP;

enum FunctionType
{
    kFT_PureForward = 0,
    kFT_Import = 1,
    kFT_LocalBody = 2,
};

// Auxiliary symbol table that is used in the first phase.
// This is used ubiquitously, so defined as a global variable.
// [TODO] Convert this into a class variable when the compiler has been
//        converted to classes
typedef std::map<AGS::Symbol, SymbolTableEntry> TSym1Table;
TSym1Table g_Sym1;

// Reference to the symbol table that works irrespective of the phase we are in
inline SymbolTableEntry &GetSymbolTableEntryAnyPhase(AGS::Symbol symb)
{
    if (kPP_Main == g_PP)
        return sym.entries[symb];
    return g_Sym1[symb];
}

// Get the type of symb; this will work irrespective of the phase we are in
inline SymbolType GetSymbolTypeAnyPhase(AGS::Symbol symb)
{
    if (symb < 0)
        return kSYM_NoType;
    return GetSymbolTableEntryAnyPhase(symb & kVTY_FlagMask).stype;
}

// Scan inpl into scan tokens, write line number opcodes, build a symbol table, mangle complex symbols
int cc_tokenize(const char *inpl, ccInternalList *targ, ccCompiledScript *scrip)
{
    AGS::Scanner scanner(inpl, 1, targ);
    AGS::Tokenizer tokenizer(&scanner, targ, &sym, scrip);

    // Write pseudo opcode for "Starting line 1"
    targ->write_meta(SMETA_LINENUM, 1);

    bool eof_encountered = false;
    bool error_encountered = false;
    int pgb_counter = 0;
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

bool is_any_type_of_string(AGS::Vartype symtype)
{
    SetFlag(symtype, kVTY_Const, false);
    if (symtype == sym.getOldStringSym())
        return true;
    SetFlag(symtype, kVTY_Pointer, false);
    if (symtype && symtype == sym.getStringStructSym())
        return true;
    return false;
}

// Return number of bytes to remove from stack to unallocate local vars
// of level from_level or higher
int StacksizeOfLocals(int from_level)
{
    int totalsub = 0;
    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].sscope <= from_level)
            continue;
        if (sym.entries[entries_idx].stype != kSYM_LocalVar)
            continue;

        // caller will sort out stack, so ignore parameters
        if (FlagIsSet(sym.entries[entries_idx].flags, kSFLG_Parameter))
            continue;

        if (FlagIsSet(sym.get_vartype(entries_idx), kVTY_DynArray))
        {
            totalsub += SIZE_OF_POINTER;
            continue;
        }

        // Calculate the size of one var of the given type
        size_t ssize = sym.entries[entries_idx].ssize;
        if (FlagIsSet(sym.entries[entries_idx].flags, kSFLG_StrBuffer))
            ssize += STRING_LENGTH;

        // Calculate the number of vars
        size_t number = 1;
        if (FlagIsSet(sym.entries[entries_idx].vartype, kVTY_Array))
            number = sym.entries[entries_idx].arrsize;

        totalsub += ssize * number;
    }
    return totalsub;
}

// Does vartype v contain releasable pointers?
// Also determines whether vartype contains standard (non-dynamic) arrays.
bool ContainsReleasablePointers(AGS::Vartype v)
{
    if (FlagIsSet(v, kVTY_DynArray) || FlagIsSet(v, kVTY_Pointer))
        return true;

    AGS::Vartype const coretype = (v & kVTY_FlagMask);
    if (!FlagIsSet(sym.entries[coretype].flags, kSFLG_StructType))
        return false; // primitive types can't have pointers
    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        SymbolTableEntry &entry = sym.entries[entries_idx];
        if (!FlagIsSet(entry.flags, kSFLG_StructMember))
            continue;
        if (coretype != entry.extends)
            continue;
        if (ContainsReleasablePointers(entry.vartype))
            return true;
    }
    return false;
}


// We're at the end of a block and releasing a standard array of pointers.
// MAR points to the array start. Release each array element (pointer).
int FreePointersOfStdArrayOfPointer(ccCompiledScript *scrip, size_t arrsize, bool &clobbers_ax)
{
    if (arrsize == 0)
        return 0;

    if (arrsize < 4)
    {
        scrip->write_cmd0(SCMD_MEMZEROPTR);
        for (size_t loop = 1; loop < arrsize; ++loop)
        {
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, SIZE_OF_POINTER);
            scrip->write_cmd0(SCMD_MEMZEROPTR);
        }
        return 0;
    }

    clobbers_ax = true;
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, arrsize);
    
    AGS::CodeLoc const loop_start = scrip->codesize;
    scrip->write_cmd0(SCMD_MEMZEROPTR);
    scrip->write_cmd2(SCMD_ADD, SREG_MAR, SIZE_OF_POINTER);
    scrip->write_cmd2(SCMD_SUB, SREG_AX, 1);
    scrip->write_cmd1(SCMD_JNZ, RelativeJumpDist(scrip->codesize + 1, loop_start));
    return 0;
}

// We're at the end of a block and releasing all the pointers in a struct.
// MAR already points to the start of the struct.
void FreePointersOfStruct(ccCompiledScript *scrip, AGS::Symbol struct_vtype, bool &clobbers_ax)
{
    std::vector <size_t> compo_list;
    for (size_t compo = 0; compo < sym.entries.size(); compo++)
    {
        SymbolTableEntry &entry = sym.entries[compo];
        if (!FlagIsSet(entry.flags, kSFLG_StructMember))
            continue;
        if (entry.extends != struct_vtype)
            continue;
        if (!ContainsReleasablePointers(entry.vartype))
            continue;
        compo_list.push_back(compo);
    }

    size_t offset_so_far = 0;
    for (auto compo_it = compo_list.cbegin(); compo_it != compo_list.cend(); ++compo_it)
    {
        SymbolTableEntry &entry = sym.entries[*compo_it];

        // Let MAR point to the component
        size_t const diff = entry.soffs - offset_so_far;
        if (diff > 0)
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, diff);
        offset_so_far = entry.soffs;

        if ((FlagIsSet(entry.vartype, kVTY_DynArray) || FlagIsSet(entry.vartype, kVTY_Pointer)) && entry.arrsize <= 1)
        {
            scrip->write_cmd0(SCMD_MEMZEROPTR);
            continue;
        }

        if (compo_list.back() != *compo_it)
            scrip->push_reg(SREG_MAR);
        if (FlagIsSet(entry.vartype, kVTY_Array))
            FreePointersOfStdArray(scrip, entry, clobbers_ax);
        else if (FlagIsSet(sym.entries[entry.vartype & kVTY_FlagMask].flags, kSFLG_StructType))
            FreePointersOfStruct(scrip, entry.vartype & kVTY_FlagMask, clobbers_ax);
        if (compo_list.back() != *compo_it)
            scrip->pop_reg(SREG_MAR);
    }
}

// We're at the end of a block and we're releasing a standard array of struct.
// MAR points to the start of the array. Release all the pointers in the array.
void FreePointersOfStdArrayOfStruct(ccCompiledScript *scrip, AGS::Symbol struct_vtype, SymbolTableEntry &entry, bool &clobbers_ax)
{
    clobbers_ax = true;

    // AX will be the index of the current element
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, entry.arrsize);

    AGS::CodeLoc loop_start = scrip->codesize;
    scrip->push_reg(SREG_MAR);
    scrip->push_reg(SREG_AX); // FreePointersOfStruct might call funcs that clobber AX
    FreePointersOfStruct(scrip, struct_vtype, clobbers_ax);
    scrip->pop_reg(SREG_AX);
    scrip->pop_reg(SREG_MAR);
    scrip->write_cmd2(SCMD_ADD, SREG_MAR, entry.ssize);
    scrip->write_cmd2(SCMD_SUB, SREG_AX, 1);
    scrip->write_cmd1(SCMD_JNZ, RelativeJumpDist(scrip->codesize + 1, loop_start));
    return;
}

// We're at the end of a block and releasing a standard array. MAR points to the start.
// Release the pointers that the array contains.
void FreePointersOfStdArray(ccCompiledScript *scrip, SymbolTableEntry &entry, bool &clobbers_ax)
{
    if (entry.arrsize < 1)
        return;

    if (FlagIsSet(entry.vartype, kVTY_Pointer) || FlagIsSet(entry.vartype, kVTY_DynArray))
    {
        FreePointersOfStdArrayOfPointer(scrip, entry.arrsize, clobbers_ax);
        return;
    }

    AGS::Symbol const coretype = entry.vartype & kVTY_FlagMask;
    if (!FlagIsSet(sym.entries[coretype].flags, kSFLG_StructType))
        return; // nothing to do

    FreePointersOfStdArrayOfStruct(scrip, coretype, entry, clobbers_ax);
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

void FreePointersOfLocals0(ccCompiledScript *scrip, int from_level, bool &clobbers_ax, bool &clobbers_mar)
{
    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        SymbolTableEntry &entry = sym.entries[entries_idx];
        if (entry.sscope <= from_level)
            continue;
        if (kSYM_LocalVar != entry.stype)
            continue;
        if (sym.getThisSym() == entries_idx)
            continue; // don't touch the this pointer
        if (!ContainsReleasablePointers(entry.vartype))
            continue;

        clobbers_mar = true;
        int const sp_offset = scrip->cur_sp - entry.soffs;
        if ((FlagIsSet(entry.vartype, kVTY_Pointer) || FlagIsSet(entry.vartype, kVTY_DynArray)) &&
            entry.arrsize <= 1)
        {
            // Simply release the pointer
            scrip->write_cmd1(SCMD_LOADSPOFFS, sp_offset);
            scrip->write_cmd0(SCMD_MEMZEROPTR);
            continue;
        }

        // Set MAR to the start of the construct that contains releasable pointers
        scrip->write_cmd1(SCMD_LOADSPOFFS, sp_offset);

        if (FlagIsSet(entry.vartype, kVTY_Array))
            FreePointersOfStdArray(scrip, entry, clobbers_ax);
        else if (FlagIsSet(sym.get_flags(entry.vartype), kSFLG_StructType))
            FreePointersOfStruct(scrip, entry.vartype & kVTY_FlagMask, clobbers_ax);
    }
}

// Free the pointers of any locals in level from_level or higher
int FreePointersOfLocals(ccCompiledScript *scrip, int from_level, AGS::Symbol name_of_current_func = 0)
{
    if (0 != from_level)
    {
        bool dummy_bool;
        FreePointersOfLocals0(scrip, from_level, dummy_bool, dummy_bool);
        return 0;
    }

    // We're ending the current function; AX is containing the result of the func call.
    AGS::Vartype const func_return_type = sym.entries[name_of_current_func].funcparamtypes.at(0);
    bool const function_returns_void = sym.getVoidSym() == func_return_type;
    bool const function_returns_a_pointer = FlagIsSet(func_return_type, kVTY_Pointer | kVTY_DynArray);
    if (function_returns_a_pointer)
    {
        // The dynamic object that AX points to mustn't be freed so that it can be returned
        // to the caller. So allocate a local variable that will point to the dynamic object.
        // This will prevent the reference counter of the dynamic object to drop to zero.
        scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR);
        scrip->write_cmd1(SCMD_MEMINITPTR, SREG_AX);
        size_t const codesize_before = scrip->codesize;
        scrip->push_reg(SREG_AX);
        bool dummy_bool;
        bool clobbers_mar = false;
        FreePointersOfLocals0(scrip, from_level, dummy_bool, clobbers_mar);
        scrip->pop_reg(SREG_AX);
        if (codesize_before + 4 == scrip->codesize)
            scrip->codesize = codesize_before; // rip out the unneeded push AX/pop AX
        // Now release the dynamic memory block with a special command that
        // prevents de-allocation as long as AX has the address, too
        if (clobbers_mar)
            scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR);
        scrip->write_cmd0(SCMD_MEMZEROPTRND);
        return 0;
    }

    size_t const codesize_before = scrip->codesize;
    bool clobbers_ax = false;
    bool dummy_bool;
    FreePointersOfLocals0(scrip, from_level, clobbers_ax, dummy_bool);
    if (!clobbers_ax || function_returns_void)
        return 0;
    // Oops. AX was carrying our return value and shouldn't have been clobbered.
    // So we have to redo this and this time save AX before freeing.
    scrip->codesize = codesize_before;
    scrip->push_reg(SREG_AX);
    FreePointersOfLocals0(scrip, from_level, clobbers_ax, dummy_bool);
    scrip->pop_reg(SREG_AX);
}

// Remove defns from the sym table of vars defined on from_level or higher
int RemoveLocalsFromSymtable(int from_level)
{

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].sscope < from_level)
            continue;
        if (sym.entries[entries_idx].stype != kSYM_LocalVar)
            continue;

        sym.entries[entries_idx].stype = kSYM_NoType;
        sym.entries[entries_idx].sscope = 0;
        sym.entries[entries_idx].flags = 0;
    }
    return 0;
}

// The type NT(Un)bracedElse isn't only used for ELSE branches, but also for the
// body of WHILE and other statements. 
// If the symbol "else" follows a THEN clause of an IF, this is handled most easily
// by adding an unconditional jump out and then changing the THEN clause into an ELSE clause. 
int DealWithEndOfElse(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack, bool &else_after_then)
{
    // Check whether the symbol "else" follows a then branch
    else_after_then = false;
    if (nesting_stack->Type() == AGS::NestingStack::kNT_UnbracedElse);
    else if (nesting_stack->Type() == AGS::NestingStack::kNT_BracedElse);
    else if (sym.get_type(targ->peeknext()) == kSYM_Else)
    {
        targ->getnext();  // eat "else"
        scrip->write_cmd1(SCMD_JMP, 0); // jump out, to be patched later
        else_after_then = true;
    }

    if (nesting_stack->StartLoc()) // a loop that features a jump back to the start
    {
        scrip->flush_line_numbers();
        // if it's a for loop, drop the yanked chunk (loop increment) back in
        if (nesting_stack->ChunksExist())
        {
            int id;
            AGS::CodeLoc const write_start = scrip->codesize;
            nesting_stack->WriteChunk(scrip, 0, id);
            g_FCM.UpdateCallListOnWriting(write_start, id);
            nesting_stack->Chunks().clear();
        }

        // jump back to the start location
        // The bytecode byte with the relative dest is at code[codesize+1]
        scrip->write_cmd1(
            SCMD_JMP,
            RelativeJumpDist(scrip->codesize + 1, nesting_stack->StartLoc()));
    }

    // Patch the jump out of the construct to jump to here
    scrip->code[nesting_stack->JumpOutLoc()] =
        RelativeJumpDist(nesting_stack->JumpOutLoc(), scrip->codesize);

    if (else_after_then)
    {
        // convert the THEN branch into an ELSE, i.e., stay on the same Depth()
        nesting_stack->SetType(AGS::NestingStack::kNT_UnbracedElse);
        if (sym.get_type(targ->peeknext()) == kSYM_OpenBrace)
        {
            nesting_stack->SetType(AGS::NestingStack::kNT_BracedElse);
            targ->getnext();
        }

        nesting_stack->SetJumpOutLoc(scrip->codesize - 1);

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
        FreePointersOfLocals(scrip, nesting_stack->Depth() - 1);
        int totalsub = StacksizeOfLocals(nesting_stack->Depth() - 1);
        if (totalsub > 0)
        {
            scrip->cur_sp -= totalsub;
            scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
        }
        RemoveLocalsFromSymtable(nesting_stack->Depth());
    }

    return 0;
}


int DealWithEndOfDo(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack)
{
    scrip->flush_line_numbers();

    AGS::Symbol cursym = targ->getnext();
    if (sym.get_type(cursym) != kSYM_While)
    {
        cc_error("Do without while");
        return -1;
    }
    if (sym.get_type(targ->getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    scrip->flush_line_numbers();

    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }
    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // Jump back to the start of the loop while the condition is true
    // The bytecode byte with the relative dest is at code[codesize+1]
    scrip->write_cmd1(
        SCMD_JNZ,
        RelativeJumpDist(scrip->codesize + 1, nesting_stack->StartLoc()));
    // Patch the jump out of the loop; it should point to here
    scrip->code[nesting_stack->JumpOutLoc()] =
        RelativeJumpDist(nesting_stack->JumpOutLoc(), scrip->codesize);

    // The clause has ended, so pop the level off the stack
    nesting_stack->Pop();

    return 0;
}


int DealWithEndOfSwitch(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack)
{
    const size_t ns_level = nesting_stack->Depth() - 1;

    // If there was no terminating break at the last switch-case, 
    // write a jump to the jumpout point to prevent a fallthrough into the jumptable
    const AGS::CodeLoc lastcmd_loc = scrip->codesize - 2;
    const AGS::CodeLoc jumpout_loc = nesting_stack->StartLoc() + 2;
    if (scrip->code[lastcmd_loc] != SCMD_JMP ||
        scrip->code[lastcmd_loc + 1] != RelativeJumpDist(lastcmd_loc + 1, jumpout_loc))
    {
        // The bytecode int that contains the relative jump is in codesize+1
        scrip->write_cmd1(SCMD_JMP, RelativeJumpDist(scrip->codesize + 1, jumpout_loc));
    }

    // We begin the jump table; remember this address
    AGS::CodeLoc jumptable_loc = scrip->codesize;

    // Patch the instruction "Jump to the jump table" at the start of the switch
    // so that it points to the correct address, i.e., here
    scrip->code[nesting_stack->StartLoc() + 1] =
        RelativeJumpDist(nesting_stack->StartLoc() + 1, jumptable_loc);

    // Get correct comparison operation: Don't compare strings as pointers but as strings
    int noteq_op = is_any_type_of_string(nesting_stack->SwitchExprType()) ? SCMD_STRINGSNOTEQ : SCMD_NOTEQUAL;

    const size_t size_of_chunks = nesting_stack->Chunks().size();
    for (size_t index = 0; index < size_of_chunks; index++)
    {
        int id;
        AGS::CodeLoc codesize = scrip->codesize;
        // Put the result of the expression into AX
        nesting_stack->WriteChunk(scrip, index, id);
        g_FCM.UpdateCallListOnWriting(codesize, id);
        // Do the comparison
        scrip->write_cmd2(noteq_op, SREG_AX, SREG_BX);
        // This command will be written to code[codesize] and code[codesize]+1
        scrip->write_cmd1(
            SCMD_JZ,
            RelativeJumpDist(scrip->codesize + 1, nesting_stack->Chunks().at(index).CodeOffset));
    }

    // Write the default jump if necessary
    if (nesting_stack->DefaultLabelLoc() != -1)
        scrip->write_cmd1(SCMD_JMP, nesting_stack->DefaultLabelLoc() - scrip->codesize - 2);

    // Patch the jump to the end of the switch block 
    // to jump to here (for break statements)
    scrip->code[nesting_stack->StartLoc() + 3] =
        RelativeJumpDist(nesting_stack->StartLoc() + 3, scrip->codesize);

    // Patch the jump at the end of the switch block
    // (it is directly in front of the jumptable)
    // to jump to here
    scrip->code[jumptable_loc - 1] =
        RelativeJumpDist(jumptable_loc - 1, scrip->codesize);

    nesting_stack->Chunks().clear();
    nesting_stack->Pop();

    return 0;
}

// We are looking for a component in a struct type or any of its ancesters.
// If it's in one of the ancesters, copy it into the struct before returning it.
int FindOrAddComponent(AGS::Symbol stname, AGS::Symbol &componame, bool errorOnFail = true)
{
    AGS::Symbol const fullname = MangleStructAndComponent(stname, componame);
    if (kSYM_NoType != sym.get_type(fullname))
    {
        componame = fullname;
        return 0;
    }

    // Look for it in the ancesters
    AGS::Symbol const parent = sym.entries[stname].extends;
    if (parent > 0)
    {
        if (0 == FindOrAddComponent(parent, componame, false))
        {
            if (kSYM_StructComponent == sym.get_type(componame))
            {
                // Copy it over
                std::string const save_sname = sym.entries[fullname].sname;
                sym.entries[componame].CopyTo(sym.entries[fullname]);
                sym.entries[fullname].sname = save_sname;
                componame = fullname;
            }
            return 0;
        }
    }

    if (errorOnFail)
        cc_error(
            "'%s' isn't a member of '%s'. Are you sure you spelt it correctly (remember, capital letters are important)?",
            sym.get_name_string(componame).c_str(),
            sym.get_name_string(stname).c_str());
    return -1;
}


int ParseLiteralOrConstvalue(AGS::Symbol fromSym, int &theValue, bool isNegative, std::string errorMsg)
{
    if (fromSym >= 0)
    {
        SymbolTableEntry &from_entry = GetSymbolTableEntryAnyPhase(fromSym); 
        if (from_entry.stype == kSYM_Constant)
        {
            theValue = from_entry.soffs;
            if (isNegative)
                theValue = -theValue;
            return 0;
        }

        if (sym.get_type(fromSym) == kSYM_LiteralInt)
        {
            std::string literalStrValue = sym.get_name_string(fromSym);
            if (isNegative)
                literalStrValue = '-' + literalStrValue;

            return String2Int(literalStrValue, theValue, true);
        }
    }

    cc_error(errorMsg.c_str());
    return -1;
}


// We're parsing a parameter list and we have accepted something like "(...int i"
// We accept a default value clause like "= 15" if it follows at this point.
int ParseParamlist_Param_DefaultValue(ccInternalList *targ, bool &has_default_int, int &default_int_value)
{
    if (sym.get_type(targ->peeknext()) != kSYM_Assign)
    {
        has_default_int = false;
        return 0;
    }

    has_default_int = true;

    // parameter has default value
    targ->getnext();   // Eat '='

    bool default_is_negative = false;
    AGS::Symbol default_value_symbol = targ->getnext(); // may be '-', too
    if (default_value_symbol == sym.find("-"))
    {
        default_is_negative = true;
        default_value_symbol = targ->getnext();
    }

    // extract the default value
    int retval = ParseLiteralOrConstvalue(default_value_symbol, default_int_value, default_is_negative, "Parameter default value must be literal");
    if (retval < 0) return retval;

    return 0;
}


// process a dynamic array declaration, when present
// We have accepted something like "int foo" and we might expect a trailing "[]" here
// Return values:  0 -- not an array, 1 -- an array, -1 -- error occurred
int ParseParamlist_Param_DynArrayMarker(ccInternalList *targ, AGS::Symbol typeSym, bool isPointer)
{
    if (sym.get_type(targ->peeknext()) != kSYM_OpenBracket)
        return 0;

    // Gobble the '[', expect and gobble ']'
    targ->getnext();
    if (sym.get_type(targ->getnext()) != kSYM_CloseBracket)
    {
        cc_error("Fixed array size cannot be used here (use '[]' instead)");
        return -1;
    }

    if (kPP_PreAnalyze == g_PP)
        return 1;

    if (FlagIsSet(sym.entries[typeSym].flags, kSFLG_StructType))
    {
        if (!FlagIsSet(sym.entries[typeSym].flags, kSFLG_Managed))
        {
            cc_error("Cannot pass non-managed struct array");
            return -1;
        }
        if (!isPointer)
        {
            cc_error("Cannot pass non-pointer struct array");
            return -1;
        }
    }
    return 1;
}

// Copy so that the forward decl can be compared afterwards to the real one     
int CopyKnownSymInfo(SymbolTableEntry &entry, SymbolTableEntry &known_info)
{
    known_info.stype = kSYM_NoType;
    if (0 == entry.stype)
        return 0; // there is no info yet

    int retval = entry.CopyTo(known_info);
    if (retval < 0) return retval;
    // If the return type is unset, deduce it
    if (0 == known_info.ssize && known_info.funcparamtypes.size() > 1)
    {
        AGS::Symbol const rettype = known_info.funcparamtypes.at(0) & ~(kVTY_Pointer | kVTY_DynArray);
        known_info.ssize = sym.entries[rettype].ssize;
    }

    // Kill the defaults so we can check whether this defn replicates them exactly.
    entry.funcParamHasDefaultValues.assign(entry.sscope + 1, false);
    // -77 is an arbitrary value that is easy to spot in the debugger; 
    // don't use for anything in code
    entry.funcParamDefaultValues.assign(entry.sscope + 1, -77);
    return 0;
}


// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
// We'll accept something like "this Character *"
int ParseFuncdecl_ExtenderPreparations(
    ccInternalList *targ,
    bool is_static_extender,
    AGS::Symbol &name_of_func,
    AGS::Symbol &struct_of_func)
{
    if (struct_of_func > 0)
    {
        cc_error("A struct component function cannot be an extender function");
        return -1;
    }

    targ->getnext(); // Eat "this" or "static"
    struct_of_func = targ->peeknext();
    SymbolTableEntry &struct_entry = GetSymbolTableEntryAnyPhase(struct_of_func);
    if (!FlagIsSet(struct_entry.flags, kSFLG_StructType))
    {
        cc_error("Expected a struct type instead of '%s'", sym.get_name_string(struct_of_func).c_str());
        return -1;
    }

    if (std::string::npos != sym.get_name_string(name_of_func).find_first_of(':'))
    {   // [fw] Can't be reached IMO. 
        cc_error("Extender functions cannot be part of a struct");
        return -1;
    }

    name_of_func = MangleStructAndComponent(struct_of_func, name_of_func);
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(name_of_func); 

    // Don't clobber the flags set in the Pre-Analyze phase
    SetFlag(entry.flags, kSFLG_StructMember, true);
    if (is_static_extender)
        SetFlag(entry.flags, kSFLG_Static, true);

    targ->getnext();
    if (!is_static_extender && targ->getnext() != sym.find("*"))
    {
        cc_error("Instance extender function must be pointer");
        return -1;
    }

    if ((sym.get_type(targ->peeknext()) != kSYM_Comma) &&
        (sym.get_type(targ->peeknext()) != kSYM_CloseParenthesis))
    {
        if (targ->getnext() == sym.getPointerSym())
            cc_error("Static extender function cannot be pointer");
        else
            cc_error("Parameter name cannot be defined for extender type");
        return -1;
    }

    if (sym.get_type(targ->peeknext()) == kSYM_Comma)
        targ->getnext();

    return 0;
}


int ParseParamlist_ParamType(ccInternalList *targ, AGS::Symbol param_vartype, bool &param_is_ptr)
{
    // Determine whether the type is a pointer
    bool const param_is_natural_ptr =
        (sym.getPointerSym() == targ->peeknext());
    if (param_is_natural_ptr)
        targ->getnext(); // gobble the '*'
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(param_vartype);
    bool const param_is_autoptr = FlagIsSet(entry.flags, kSFLG_Autoptr);
    param_is_ptr = param_is_natural_ptr || param_is_autoptr;

    if (kPP_PreAnalyze == g_PP)
        return 0;

    // Safety checks on the parameter type
    if (sym.getVoidSym() == param_vartype)
    {
        cc_error("A function parameter must not have the type 'void'");
        return -1;
    }
    if (param_is_ptr)
    {
        if (!FlagIsSet(sym.get_flags(param_vartype), kSFLG_Managed))
        {
            // can only point to managed structs
            cc_error("Cannot declare pointer to non-managed type");
            return -1;
        }
        if (param_is_natural_ptr && param_is_autoptr)
        {
            cc_error("Cannot use '*' with %s", sym.get_vartype_name_string(param_vartype).c_str());
            return -1;
        }
    }
    if (FlagIsSet(sym.get_flags(param_vartype), kSFLG_StructType) && (!param_is_ptr))
    {
        cc_error("A struct cannot be passed as parameter");
        return -1;
    }
    return 0;
}


// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present
int ParseParamlist_Param_Name(ccInternalList *targ, bool body_follows, AGS::Symbol &param_name)
{
    param_name = -1;

    if (kPP_PreAnalyze == g_PP || !body_follows)
    {
        // Ignore the parameter name when present, it won't be used later on (in this phase)
        param_name = -1;
        AGS::Symbol const nextsym = targ->peeknext();
        if (IsIdentifier(nextsym))
            targ->getnext();
        return 0;
    }

    if (sym.get_type(targ->peeknext()) == kSYM_GlobalVar)
    {
        // This is a definition -- so the parameter name must not be a global variable
        cc_error("The name '%s' is already used for a global variable", sym.get_name_string(targ->peeknext()).c_str());
        return -1;
    }

    if (sym.get_type(targ->peeknext()) != 0)
    {
        // We need to have a real parameter name here
        cc_error("Expected a parameter name here, found '%s' instead", sym.get_name_string(targ->peeknext()).c_str());
        return -1;
    }

    param_name = targ->getnext(); // get and gobble the parameter name

    return 0;
}


void ParseParamlist_Param_AsVar2Sym(ccCompiledScript *scrip, AGS::Symbol param_name, AGS::Vartype param_type, bool param_is_ptr, bool param_is_const, bool param_is_dynarray, int param_idx)
{
    SymbolTableEntry &param_entry = sym.entries[param_name];
    param_entry.stype = kSYM_LocalVar;
    param_entry.extends = false;
    param_entry.arrsize = 1;
    param_entry.vartype = param_type;
    if (param_is_dynarray)
        SetFlag(param_entry.vartype, kVTY_Array | kVTY_DynArray, true);
    if (param_is_ptr)
        SetFlag(param_entry.vartype, kVTY_Pointer, true);
    size_t const param_size = 4; // We can only deal with parameters of size 4
    param_entry.ssize = param_size;
    param_entry.sscope = 1;
    SetFlag(param_entry.flags, kSFLG_Parameter, true);
    if (param_is_const)
    {
        SetFlag(param_entry.flags, kSFLG_Readonly, true);
        SetFlag(param_entry.vartype, kVTY_Const, true);
    }
    // the parameters are pushed backwards, so the top of the
    // stack has the first parameter. The +1 is because the
    // call will push the return address onto the stack as well
    param_entry.soffs = scrip->cur_sp - (param_idx + 1) * 4;
}

void ParseParamlist_Param_Add2Func(AGS::Symbol name_of_func, int param_idx, AGS::Symbol param_type, bool param_is_ptr, bool param_is_const, bool param_is_dynarray, bool param_has_int_default, int param_int_default)
{
    SymbolTableEntry &func_entry = GetSymbolTableEntryAnyPhase(name_of_func);
    size_t const minsize = param_idx + 1;
    if (func_entry.funcparamtypes.size() < minsize)
    {
        func_entry.funcparamtypes.resize(minsize);
        func_entry.funcParamHasDefaultValues.resize(minsize);
        func_entry.funcParamDefaultValues.resize(minsize);
    }

    func_entry.funcparamtypes[param_idx] = param_type;
    if (param_is_ptr)
        func_entry.funcparamtypes[param_idx] |= kVTY_Pointer;
    if (param_is_const)
        func_entry.funcparamtypes[param_idx] |= kVTY_Const;
    if (param_is_dynarray)
        func_entry.funcparamtypes[param_idx] |= kVTY_DynArray;

    if (param_has_int_default)
    {
        func_entry.funcParamHasDefaultValues[param_idx] = param_has_int_default;
        func_entry.funcParamDefaultValues[param_idx] = param_int_default;
    }
}

// process a parameter decl in a function parameter list, something like int foo(INT BAR
int ParseParamlist_Param(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol name_of_func, bool body_follows, AGS::Symbol param_type, bool param_is_const, int param_idx)
{
    // Parse complete parameter type
    bool param_is_ptr = false;
    int retval = ParseParamlist_ParamType(targ, param_type, param_is_ptr);
    if (retval < 0) return retval;

    // Parameter name (when present and meaningful) 
    AGS::Symbol param_name;
    retval = ParseParamlist_Param_Name(targ, body_follows, param_name);
    if (retval < 0) return retval;

    // Dynamic array signifier (when present)
    retval = ParseParamlist_Param_DynArrayMarker(targ, param_type, param_is_ptr);
    if (retval < 0) return retval;
    bool param_is_dynarray = (retval == 1);

    // Default clause (when present)
    bool param_has_int_default = false;
    int param_int_default;
    retval = ParseParamlist_Param_DefaultValue(targ, param_has_int_default, param_int_default);
    if (retval < 0) return retval;

    // Augment the function type in the symbol table  
    ParseParamlist_Param_Add2Func(name_of_func, param_idx, param_type, param_is_ptr, param_is_const, param_is_dynarray, param_has_int_default, param_int_default);

    if (kPP_Main != g_PP || !body_follows)
        return 0;

    // All function parameters correspond to local variables.
    // A body will follow, so we need to enter this parameter as a variable into the symbol table
    ParseParamlist_Param_AsVar2Sym(scrip, param_name, param_type, param_is_ptr, param_is_const, param_is_dynarray, param_idx);

    return 0;
}


int ParseFuncdecl_Paramlist(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol funcsym, bool body_follows, int &numparams)
{
    bool param_is_const = false;
    while (!ReachedEOF(targ))
    {
        AGS::Symbol const cursym = targ->getnext();
        SymbolType curtype = (kPP_PreAnalyze == g_PP && g_Sym1.count(cursym) > 0) ? g_Sym1[cursym].stype : sym.get_type(cursym);

        switch (curtype)
        {
        default:
            cc_error("Unexpected %s in parameter list", sym.get_name_string(cursym).c_str());
            return -1;

        case kSYM_CloseParenthesis:
            return 0;

        case kSYM_Const:
        {
            // check in main compiler phase that type must follow
            if (kPP_Main == g_PP && sym.get_type(targ->peeknext()) != kSYM_Vartype)
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
            if (sym.get_type(targ->getnext()) != kSYM_CloseParenthesis)
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

            int retval = ParseParamlist_Param(targ, scrip, funcsym, body_follows, cursym, param_is_const, numparams);
            if (retval < 0) return retval;

            ++numparams;
            param_is_const = false; // modifier has been used up
            SymbolType const nexttype = sym.get_type(targ->peeknext());
            if (nexttype == kSYM_Comma)
                targ->getnext(); // Eat ','
            continue;
        }
        } // switch
    } // while
    cc_error("End of input when processing parameter list");
    return -1;
}


void ParseFuncdecl_SetFunctype(
    SymbolTableEntry &entry,
    int return_type,
    bool func_returns_ptr,
    bool func_returns_dynarray,
    bool func_is_static,
    bool func_is_protected,
    int numparams)
{
    int const size_of_a_pointer = 4;

    entry.stype = kSYM_Function;
    SymbolTableEntry &ret_type_entry = GetSymbolTableEntryAnyPhase(return_type);
    entry.ssize = ret_type_entry.ssize;
    if (func_returns_ptr)
        entry.ssize = size_of_a_pointer;
    entry.sscope = numparams - 1;

    entry.funcparamtypes[0] = return_type;
    if (func_returns_ptr)
        entry.funcparamtypes[0] |= kVTY_Pointer;
    if (func_returns_dynarray)
        entry.funcparamtypes[0] |= kVTY_DynArray;
    if (func_is_static)
        SetFlag(entry.flags, kSFLG_Static, true);
    if (func_is_protected)
        SetFlag(entry.flags, kSFLG_Protected, true);

    return;
}


int ParseFuncdecl_CheckThatFDM_CheckDefaults(SymbolTableEntry *this_entry, bool body_follows, SymbolTableEntry *known_info)
{
    if (body_follows)
    {
        // If none of the parameters have a default,
        // we'll let this through for backward compatibility.
        bool has_default = false;
        for (int param_idx = 1; param_idx <= this_entry->get_num_args(); ++param_idx)
            if (this_entry->funcParamHasDefaultValues[param_idx])
            {
                has_default = true;
                break;
            }
        if (!has_default)
            return 0;
    }

    // this is 1 .. get_num_args(), INCLUSIVE, because param 0 is the return type
    for (int param_idx = 1; param_idx <= this_entry->get_num_args(); ++param_idx)
    {
        if ((this_entry->funcParamHasDefaultValues[param_idx] ==
            known_info->funcParamHasDefaultValues[param_idx]) &&
            (this_entry->funcParamHasDefaultValues[param_idx] == false ||
                this_entry->funcParamDefaultValues[param_idx] ==
                known_info->funcParamDefaultValues[param_idx]))
        {
            continue;
        }
        std::string errstr1 = "In this declaration, parameter #<1> <2>; ";
        errstr1.replace(errstr1.find("<1>"), 3, std::to_string(param_idx));
        if (!this_entry->funcParamHasDefaultValues[param_idx])
            errstr1.replace(errstr1.find("<2>"), 3, "doesn't have a default value");
        else
            errstr1.replace(errstr1.find("<2>"), 3, "has the default "
                + std::to_string(this_entry->funcParamDefaultValues[param_idx]));

        std::string errstr2 = "in a declaration elsewhere, that parameter <2>.";
        if (!known_info->funcParamHasDefaultValues[param_idx])
            errstr2.replace(errstr2.find("<2>"), 3, "doesn't have a default value");
        else
            errstr2.replace(errstr2.find("<2>"), 3, "has the default "
                + std::to_string(known_info->funcParamDefaultValues[param_idx]));
        errstr1 += errstr2;
        cc_error(errstr1.c_str());
        return -1;
    }
    return 0;
}

// there was a forward declaration -- check that the real declaration matches it
int ParseFuncdecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, bool body_follows, SymbolTableEntry *known_info)
{
    if (0 == known_info->stype)
        return 0; // We don't have any known info

    if (known_info->stype != this_entry->stype)
    {
        cc_error(
            "Type of function is declared as %s here, as %s elsewhere",
            sym.get_name_string(this_entry->stype).c_str(),
            sym.get_name_string(known_info->stype).c_str());
        return -1;
    }

    if ((known_info->flags & ~kSFLG_Imported) != (this_entry->flags & ~kSFLG_Imported))
    {
        cc_error("Qualifiers of function do not match prototype");
        return -1;
    }

    if (FlagIsSet(this_entry->vartype, kVTY_Array) && (known_info->arrsize != this_entry->arrsize))
    {
        cc_error(
            "Function is declared to return an array size %d here, %d elsewhere",
            this_entry->arrsize, known_info->arrsize);
        return -1;
    }

    if (known_info->sscope != this_entry->sscope)
    {
        cc_error("Function is declared with %d parameters here, with %d parameters elswehere", this_entry->sscope, known_info->sscope);
        return -1;
    }

    if (known_info->funcparamtypes.at(0) != this_entry->funcparamtypes.at(0))
    {
        cc_error(
            "Return type is declared as %s here, as %s elsewhere",
            sym.get_vartype_name_string(this_entry->funcparamtypes.at(0)).c_str(),
            sym.get_vartype_name_string(known_info->funcparamtypes.at(0)).c_str());

        return -1;
    }
    if (known_info->ssize != this_entry->ssize)
    {
        cc_error(
            "Size of return value is %d here, %d declared elsewhere",
            this_entry->ssize, known_info->ssize);
        return -1;
    }

    size_t const num_args = this_entry->get_num_args();
    if (num_args != known_info->get_num_args())
    {
        cc_error(
            "Function has %d explicit arguments here, %d elsewhere",
            this_entry->get_num_args(), known_info->get_num_args());
        return -1;
    }

    for (int param_idx = 1; param_idx <= num_args; param_idx++)
    {
        if (known_info->funcparamtypes.at(param_idx) != this_entry->funcparamtypes.at(param_idx))
        {
            cc_error(
                "Type of parameter no. %d is %s here, %s in a declaration elsewhere",
                param_idx,
                sym.get_name_string(this_entry->funcparamtypes.at(param_idx)).c_str(),
                sym.get_name_string(known_info->funcparamtypes.at(param_idx)).c_str());
            return -1;
        }
    }

    // Check that the defaults match
    int retval = ParseFuncdecl_CheckThatFDM_CheckDefaults(this_entry, body_follows, known_info);
    if (retval < 0) return retval;

    // copy the default values from the function prototype
    this_entry->funcParamHasDefaultValues.assign(
        known_info->funcParamHasDefaultValues.begin(),
        known_info->funcParamHasDefaultValues.end());
    this_entry->funcParamDefaultValues.assign(
        known_info->funcParamDefaultValues.begin(),
        known_info->funcParamDefaultValues.end());

    return 0;
}

// Enter the function in the imports[] or functions[] array; get its index   
int ParseFuncdecl_EnterAsImportOrFunc(ccCompiledScript *scrip, AGS::Symbol name_of_func, bool body_follows, bool func_is_import, int &function_soffs, int &function_idx)
{
    if (body_follows)
    {
        if (func_is_import)
        {
            cc_error("Imported functions cannot have a body");
            return -1;
        }
        // Index of the function in the ccCompiledScript::functions[] array
        function_soffs = scrip->add_new_function(sym.get_name_string(name_of_func).c_str(), &function_idx);
        if (function_soffs < 0)
        {
            cc_error("Max. number of functions exceeded");
            return -1;
        }
        g_FCM.SetFuncCallpoint(scrip, name_of_func, function_soffs);
        return 0;
    }

    if (!func_is_import)
    {
        function_soffs = -1; // forward decl; callpoint is unknown yet
        return 0;
    }

    // Index of the function in the ccScript::imports[] array
    function_soffs = g_ImportMgr.FindOrAdd(sym.get_name_string(name_of_func));
    return 0;
}


// We're at something like "int foo(", directly before the "("
// Get the symbol after the corresponding ")"
int ParseFuncdecl_GetSymbolAfterParmlist(ccInternalList *targ, AGS::Symbol &symbol)
{
    int pos = targ->pos;

    AGS::Symbol const stoplist[] = { 0 };
    SkipTo(targ, stoplist, 0); // Skim to matching ')'
 
    if (kSYM_CloseParenthesis != sym.get_type(targ->getnext()))
    {
        cc_error("Internal error: Unclosed parameter list of function");
        return -99;
    }

    symbol = targ->peeknext();
    targ->pos = pos;
    return 0;
}

// We're at something like "int foo(", directly before the "("
// This might or might not be within a struct defn
int ParseFuncdecl(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol &name_of_func,
    int return_type,
    bool func_returns_ptr,
    bool func_returns_dynarray,
    TypeQualifierSet tqs,
    AGS::Symbol &struct_of_func,
    bool &body_follows)
{
    targ->getnext(); // Eat '('
    {
        AGS::Symbol symbol;
        int retval = ParseFuncdecl_GetSymbolAfterParmlist(targ, symbol);
        if (retval < 0) return retval;
        body_follows = (kSYM_OpenBrace == sym.get_type(symbol));
    }

    bool const func_is_static_extender = (kSYM_Static == sym.get_type(targ->peeknext()));
    bool const func_is_extender = (func_is_static_extender) || (sym.getThisSym() == targ->peeknext());

    // Rewrite extender function as if it were a component function of the corresponding struct.
    if (func_is_extender)
    {
        int retval = ParseFuncdecl_ExtenderPreparations(targ, func_is_static_extender, name_of_func, struct_of_func);
        if (retval < 0) return retval;
    }

    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(name_of_func);
    if (kSYM_Function != entry.stype && kSYM_NoType != entry.stype)
    {
        cc_error("'%s' is already defined", sym.get_name_string(name_of_func).c_str());
        return -1;
    }

    if ((!func_returns_ptr) && (!func_returns_dynarray) &&
        FlagIsSet(entry.flags, kSFLG_StructType))
    {
        cc_error("Cannot return entire struct from function");
        return -1;
    }

    if (body_follows && kPP_Main == g_PP)
    {
        scrip->cur_sp += 4;  // the return address will be pushed

        if (g_FCM.HasFuncCallpoint(name_of_func))
        {
            cc_error("This function has already been defined with body");
            return -1;
        }
    }

    // A forward decl can be written with the
    // "import" keyword (when allowed in the options). This isn't an import
    // proper, so reset the "import" flag in this case.
    if (FlagIsSet(tqs, kTQ_Import) && kSYM_Function == entry.stype  && !FlagIsSet(entry.flags, kSFLG_Imported))
    {
        if (0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE))
        {
            cc_error("In here, a function with a local body must not have an \"import\" declaration");
            return -1;
        }
        SetFlag(tqs, kTQ_Import, false);
    }

    // Copy all known info about the function so that we can check whether this declaration is compatible
    SymbolTableEntry known_info;
    int retval = CopyKnownSymInfo(entry, known_info);
    if (retval < 0) return retval;

    // process parameter list, get number of parameters
    int numparams = 1; // Counts the number of parameters including the ret parameter, so start at 1
    retval = ParseFuncdecl_Paramlist(targ, scrip, name_of_func, body_follows, numparams);
    if (retval < 0) return retval;

    // Type the function in the symbol table
    ParseFuncdecl_SetFunctype(entry, return_type, func_returns_ptr, func_returns_dynarray, FlagIsSet(tqs, kTQ_Static), FlagIsSet(tqs, kTQ_Protected), numparams);

    // Check whether this declaration is compatible with known info; 
    retval = ParseFuncdecl_CheckThatKnownInfoMatches(&entry, body_follows, &known_info);
    if (retval < 0) return retval;

    if (kPP_Main == g_PP)
    {
        // Get start offset and function index
        int function_idx = -1; // Index in the scrip->functions[] array
        int func_startoffs;
        retval = ParseFuncdecl_EnterAsImportOrFunc(scrip, name_of_func, body_follows, FlagIsSet(tqs, kTQ_Import), func_startoffs, function_idx);
        if (retval < 0) return retval;
        entry.soffs = func_startoffs;
        if (function_idx >= 0)
            scrip->funcnumparams[function_idx] = (numparams - 1);
    }

    if (!FlagIsSet(tqs, kTQ_Import))
        return 0;

    // Imported functions
    SetFlag(entry.flags, kSFLG_Imported, true);

    if (kPP_PreAnalyze == g_PP)
    {
        entry.soffs = kFT_Import;
        return 0;
    }

    if (struct_of_func > 0)
    {
        // Append the number of parameters to the name of the import
        char appendage[10];
        sprintf(appendage, "^%d", entry.sscope);
        strcat(scrip->imports[entry.soffs], appendage);
    }

    return 0;
}


// interpret the float as if it were an int (without converting it really);
// return that int
inline int InterpretFloatAsInt(float floatval)
{
    float *floatptr = &floatval; // Get pointer to the float
    int *intptr = reinterpret_cast<int *>(floatptr); // pretend that it points to an int
    return *intptr; // return the int that the pointer points to
}


// The higher the MATHEMATICAL priority of an operator, the MORE binding it is.
// For example, "*" has a higher mathematical priority than "-".
// In contrast to this, "size" gives the priority in the INVERSE way: 
// The higher sym.entries[op].ssize is, the LESS binding is the operator op.
// To convert, we must subtract this value from some suitable value 
// (any will do that doesn't cause underflow of the subtraction).
inline int MathPrio(AGS::Symbol op)
{
    return 100 - sym.entries[op].ssize;
}


// return the index of the lowest MATHEMATICAL priority operator in the list,
// so that either side of it can be evaluated first.
// returns -1 if no operator was found
int IndexOfLowestBondingOperator(AGS::SymbolScript slist, size_t slist_len)
{
    size_t bracket_nesting_depth = 0;
    size_t paren_nesting_depth = 0;

    int lowest_MathPrio_found = std::numeric_limits<int>::max(); // c++ STL lingo for MAXINT
    int index_of_lowest_MathPrio = -1;

    for (size_t slist_idx = 0; slist_idx < slist_len; slist_idx++)
    {
        SymbolType thisType = sym.get_type(slist[slist_idx]);
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


inline bool is_string(AGS::Vartype vartype)
{
    if (vartype == sym.getOldStringSym())
        return true;
    if (vartype == (sym.getOldStringSym() | kVTY_Const))
        return true;
    if (vartype == (sym.getCharSym() | kVTY_Const | kVTY_Array))
        return true;
    return false;
}


// Change the generic operator vcpuOp to the one that is correct for the vartypes
// Also check whether the operator can handle the types at all
int GetOperatorValidForVartype(AGS::Vartype type1, AGS::Vartype type2, AGS::CodeCell &vcpuOp)
{
    if (sym.getFloatSym() == type1 || sym.getFloatSym() == type2)
    {
        if (type1 != type2)
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

    bool const iatos1 = is_any_type_of_string(type1);
    bool const iatos2 = is_any_type_of_string(type2);

    if (iatos1 || iatos2)
    {
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

    if (FlagIsSet(type1, kVTY_Pointer | kVTY_DynArray) &&
        FlagIsSet(type2, kVTY_Pointer | kVTY_DynArray))
    {
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to pointers or dynamic arrays");
            return -1;
        case SCMD_ISEQUAL:  return 0;
        case SCMD_NOTEQUAL: return 0;
        }
    }

    // Other combinations of pointers and/or dynamic arrays won't mingle
    if ((type1 & (kVTY_Pointer | kVTY_DynArray)) != 0 ||
        (type2 & (kVTY_Pointer | kVTY_DynArray)) != 0)
    {
        cc_error("The operator cannot be applied to values of these types");
        return -1;
    }

    return 0;
}


// Check for a type mismatch in one direction only
bool IsVartypeMismatch_Oneway(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be)
{
    // cannot convert 'void' to anything
    if (sym.getVoidSym() == vartype_is)
        return true;

    // Don't convert if no conversion is called for
    if (vartype_is == vartype_wants_to_be)
        return false;

    // cannot convert const to non-const
    if (((vartype_is & kVTY_Const) != 0) && ((vartype_wants_to_be & kVTY_Const) == 0))
        return true;

    // can convert String* to const string
    if ((vartype_is == (kVTY_Pointer | sym.getStringStructSym())) &&
        (vartype_wants_to_be == (kVTY_Const | sym.getOldStringSym())))
    {
        return false;
    }
    if (is_string(vartype_is) != is_string(vartype_wants_to_be))
        return true;
    if (is_string(vartype_is))
        return false;

    // Can convert from NULL to pointer
    if ((vartype_is == (kVTY_Pointer | sym.getNullSym())) && ((vartype_wants_to_be & kVTY_DynArray) != 0))
        return false;

    // Cannot convert non-dynarray to dynarray or vice versa
    if ((vartype_is & kVTY_DynArray) != (vartype_wants_to_be & kVTY_DynArray))
        return true;

    // From here on, don't mind constness or dynarray-ness
    vartype_is &= ~(kVTY_Const | kVTY_DynArray);
    vartype_wants_to_be &= ~(kVTY_Const | kVTY_DynArray);

    // floats cannot mingle with other types
    if ((vartype_is == sym.getFloatSym()) != (vartype_wants_to_be == sym.getFloatSym()))
        return true;

    // Checks to do if at least one is a pointer
    if ((vartype_is & kVTY_Pointer) || (vartype_wants_to_be & kVTY_Pointer))
    {
        // null can be cast to any pointer type
        if (vartype_is == (kVTY_Pointer | sym.getNullSym()))
        {
            if (vartype_wants_to_be & kVTY_Pointer)
                return false;
        }

        // BOTH sides must be pointers
        if ((vartype_is & kVTY_Pointer) != (vartype_wants_to_be & kVTY_Pointer))
            return true;

        // Types need not be identical here, but check against inherited classes
        int isClass = vartype_is & ~kVTY_Pointer;
        while (sym.entries[isClass].extends > 0)
        {
            isClass = sym.entries[isClass].extends;
            if ((isClass | kVTY_Pointer) == vartype_wants_to_be)
                return false;
        }
        return true;
    }

    // Checks to do if at least one is a struct
    bool typeIsIsStruct = (FlagIsSet(sym.get_flags(vartype_is), kSFLG_StructType));
    bool typeWantsToBeIsStruct = (FlagIsSet(sym.get_flags(vartype_wants_to_be), kSFLG_StructType));
    if (typeIsIsStruct || typeWantsToBeIsStruct)
    {
        // The types must match exactly
        if (vartype_is != vartype_wants_to_be)
            return true;

        return false;
    }

    return false;
}

// Check whether there is a type mismatch; if so, give an error
int IsVartypeMismatch(AGS::Vartype vartype_is, AGS::Vartype vartype_wants_to_be, bool orderMatters)
{
    if (!IsVartypeMismatch_Oneway(vartype_is, vartype_wants_to_be))
        return 0;
    if (!orderMatters && !IsVartypeMismatch_Oneway(vartype_wants_to_be, vartype_is))
        return 0;

    cc_error(
        "Type mismatch: cannot convert '%s' to '%s'",
        sym.get_vartype_name_string(vartype_is).c_str(),
        sym.get_vartype_name_string(vartype_wants_to_be).c_str());
    return -1;
}

// returns whether this operator's val type is always bool
inline bool IsBooleanVCPUOperator(int scmdtype)
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


void DoNullCheckOnStringInAXIfNecessary(ccCompiledScript *scrip, int valTypeTo)
{

    if (((scrip->ax_vartype & (~kVTY_Pointer)) == sym.getStringStructSym()) &&
        ((valTypeTo & (~kVTY_Const)) == sym.getOldStringSym()))
    {
        scrip->write_cmd1(SCMD_CHECKNULLREG, SREG_AX);
    }

}


// If we need a String but AX contains a normal literal string, 
// then convert AX into a String object and set its type accordingly
void ConvertAXIntoStringObject(ccCompiledScript *scrip, int valTypeTo)
{
    if (((scrip->ax_vartype & (~kVTY_Const)) == sym.getOldStringSym()) &&
        ((valTypeTo & (~kVTY_Pointer)) == sym.getStringStructSym()))
    {
        scrip->write_cmd1(SCMD_CREATESTRING, SREG_AX); // convert AX
        scrip->ax_vartype = kVTY_Pointer | sym.getStringStructSym(); // set type of AX
    }
}

inline int GetReadCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMREAD;
    case 1:  return SCMD_MEMREADB;
    case 2:  return SCMD_MEMREADW;
    }
}

inline int GetWriteCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMWRITE;
    case 1:  return SCMD_MEMWRITEB;
    case 2:  return SCMD_MEMWRITEW;
    }
}

int ParseExpression_NewIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript &symlist, size_t symlist_len)
{
    if (symlist_len < 2 || sym.get_type(symlist[1]) != kSYM_Vartype)
    {
        cc_error("Expected a type after 'new'");
        return -1;
    }

    // "new TYPE", nothing following
    if (symlist_len <= 3)
    {
        if (FlagIsSet(sym.get_flags(symlist[1]), kSFLG_Builtin))
        {
            cc_error("Built-in type '%s' cannot be instantiated directly", sym.get_name_string(symlist[1]).c_str());
            return -1;
        }
        const size_t size = sym.entries[symlist[1]].ssize;
        scrip->write_cmd2(SCMD_NEWUSEROBJECT, SREG_AX, size);
        scrip->ax_vartype = symlist[1] | kVTY_Pointer;
        return 0;
    }

    // "new TYPE[EXPR]", nothing following
    if (kSYM_OpenBracket == sym.get_type(symlist[2]) && kSYM_CloseBracket == sym.get_type(symlist[symlist_len - 1]))
    {
        AGS::Symbol arrayType = symlist[1];

        // Expression for length of array begins after "[", ends before "]"
        // So expression_length = whole_length - 3 - 1
        int retval = ParseExpression_Subexpr(scrip, &symlist[3], symlist_len - 4);
        if (retval < 0) return retval;

        if (sym.getIntSym() != scrip->ax_vartype)
        {
            cc_error("Array size must be an int");
            return -1;
        }

        bool isManagedType = false;
        int size = sym.entries[arrayType].ssize;
        if (FlagIsSet(sym.get_flags(arrayType), kSFLG_Managed))
        {
            isManagedType = true;
            size = SIZE_OF_POINTER;
        }
        else if (FlagIsSet(sym.get_flags(arrayType), kSFLG_StructType))
        {
            cc_error("Cannot create a dynamic array of an unmanaged struct");
            return -1;
        }

        scrip->write_cmd3(SCMD_NEWARRAY, SREG_AX, size, isManagedType);
        scrip->ax_vartype = arrayType | kVTY_DynArray | kVTY_Array;

        if (isManagedType)
            scrip->ax_vartype |= kVTY_Pointer;

        return 0;
    }

    cc_error("Unexpected characters following 'new %s'", sym.get_name_string(symlist[1]).c_str());
    return -1;
}


// We're parsing an expression that starts with '-' (unary minus)
int ParseExpression_UnaryMinusIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript &symlist, size_t symlist_len)
{
    if (symlist_len < 2)
    {
        cc_error("Parse error at '-'");
        return -1;
    }
    // parse the rest of the expression into AX
    int retval = ParseExpression_Subexpr(scrip, &symlist[1], symlist_len - 1);
    if (retval < 0) return retval;

    // now, subtract the result from 0 (which negates it)
    int cpuOp = SCMD_SUBREG; // get correct bytecode for the subtraction
    retval = GetOperatorValidForVartype(scrip->ax_vartype, sym.getIntSym(), cpuOp);
    if (retval < 0) return retval;

    scrip->write_cmd2(SCMD_LITTOREG, SREG_BX, 0);
    scrip->write_cmd2(cpuOp, SREG_BX, SREG_AX);
    scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
    return 0;
}

// We're parsing an expression that starts with '!' (boolean NOT)
int ParseExpression_NotIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript & symlist, size_t symlist_len)
{

    if (symlist_len < 2)
    {
        cc_error("Parse error at '!'");
        return -1;
    }

    // parse the rest of the expression into AX
    int retval = ParseExpression_Subexpr(scrip, &symlist[1], symlist_len - 1);
    if (retval < 0) return retval;

    // negate the result
    // First determine the correct bytecode for the negation
    int cpuOp = SCMD_NOTREG;
    retval = GetOperatorValidForVartype(scrip->ax_vartype, 0, cpuOp);
    if (retval < 0) return retval;

    // now, NOT the result
    scrip->write_cmd1(SCMD_NOTREG, SREG_AX);
    return 0;
}

// The lowest-binding operator is the first thing in the expression
// This means that the op must be an unary op.
int ParseExpression_OpIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript &symlist, size_t symlist_len)
{
    if (kSYM_New == sym.get_type(symlist[0]))
    {
        // we're parsing something like "new foo"
        return ParseExpression_NewIsFirst(scrip, symlist, symlist_len);
    }

    if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG)
    {
        // we're parsing something like "- foo"
        return ParseExpression_UnaryMinusIsFirst(scrip, symlist, symlist_len);
    }

    if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_NOTREG)
    {
        // we're parsing something like "! foo"
        return ParseExpression_NotIsFirst(scrip, symlist, symlist_len);
    }

    // All the other operators need a non-empty left hand side
    cc_error("Unexpected operator '%s' without a preceding expression", sym.get_name_string(symlist[0]).c_str());
    return -1;
}


// The lowest-binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
int ParseExpression_OpIsSecondOrLater(ccCompiledScript *scrip, size_t op_idx, const AGS::SymbolScript &symlist, size_t symlist_len)
{

    int vcpuOperator = sym.entries[symlist[op_idx]].operatorToVCPUCmd();

    if (vcpuOperator == SCMD_NOTREG)
    {
        // you can't do   a = b ! c;
        cc_error("Invalid use of operator '!'");
        return -1;
    }

    if ((vcpuOperator == SCMD_SUBREG) &&
        (op_idx > 1) &&
        (kSYM_Operator == sym.get_type(symlist[op_idx - 1])))
    {
        // We aren't looking at a subtraction; instead, the '-' is the unary minus of a negative value
        // Thus, the "real" operator must be further to the right, find it.
        op_idx = IndexOfLowestBondingOperator(symlist, op_idx);
        vcpuOperator = sym.entries[symlist[op_idx]].operatorToVCPUCmd();
    }

    // process the left hand side and save result onto stack
    // This will be in vain if we find out later on that there isn't any right hand side,
    // but doing the left hand side first means that any errors will be generated from left to right
    int retval = ParseExpression_Subexpr(scrip, &symlist[0], op_idx);
    if (retval < 0) return retval;

    if (op_idx + 1 >= symlist_len)
    {
        // there is no right hand side for the expression
        cc_error("Parse error: invalid use of operator '%s'", sym.get_name_string(symlist[op_idx]).c_str());
        return -1;
    }

    AGS::CodeLoc jump_dest_loc_to_patch = -1;
    if (vcpuOperator == SCMD_AND)
    {
        // "&&" operator lazy evaluation ... 
        // if AX is 0 then the AND has failed, 
        // so just jump directly past the AND instruction;
        // AX will still be 0 so that will do as the result of the calculation
        scrip->write_cmd1(SCMD_JZ, 0);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        jump_dest_loc_to_patch = scrip->codesize - 1;
    }
    else if (vcpuOperator == SCMD_OR)
    {
        // "||" operator lazy evaluation ... 
        // if AX is non-zero then the OR has succeeded, 
        // so just jump directly past the OR instruction; 
        // AX will still be non-zero so that will do as the result of the calculation
        scrip->write_cmd1(SCMD_JNZ, 0);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        jump_dest_loc_to_patch = scrip->codesize - 1;
    }

    int vartype_leftsize = scrip->ax_vartype;

    scrip->push_reg(SREG_AX);
    retval = ParseExpression_Subexpr(scrip, &symlist[op_idx + 1], symlist_len - (op_idx + 1));
    if (retval < 0) return retval;
    scrip->pop_reg(SREG_BX); // <-- note, we pop to BX although we have pushed AX
    // now the result of the left side is in BX, of the right side is in AX

    // Check whether the left side type and right side type match either way
    retval = IsVartypeMismatch(scrip->ax_vartype, vartype_leftsize, false);
    if (retval < 0) return retval;

    retval = GetOperatorValidForVartype(scrip->ax_vartype, vartype_leftsize, vcpuOperator);
    if (retval < 0) return retval;

    scrip->write_cmd2(vcpuOperator, SREG_BX, SREG_AX);
    scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);

    if (jump_dest_loc_to_patch >= 0)
    {
        scrip->code[jump_dest_loc_to_patch] =
            RelativeJumpDist(jump_dest_loc_to_patch, scrip->codesize);
    }

    // Operators like == return a bool (in our case, that's an int);
    // other operators like + return the type that they're operating on
    if (IsBooleanVCPUOperator(vcpuOperator))
        scrip->ax_vartype = sym.getIntSym();

    return 0;
}


int ParseExpression_OpenParenthesis(ccCompiledScript *scrip, AGS::SymbolScript & symlist, size_t symlist_len)
{
    int matching_paren_idx = -1;
    size_t paren_nesting_depth = 1; // we've already read a '('
    // find the corresponding closing parenthesis
    for (size_t idx = 1; idx < symlist_len; idx++)
    {
        switch (sym.get_type(symlist[idx]))
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
    int retval = ParseExpression_Subexpr(scrip, &symlist[1], matching_paren_idx - 1);
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

struct MemoryLocation
{
    SymbolType LType; // kSYM_GlobalVar, kSYM_Import, kSYM_LocalVar, kSYM_NoType
    size_t StartOffs;
    size_t ComponentOffs;
};

// Set the MAR register now: We set it lazily at the last minute
// so that offsets can be accumulated at compile time instead of runtime.
void AccessData_MakeMARCurrent(ccCompiledScript *scrip, MemoryLocation &mloc)
{
    switch (mloc.LType)
    {
    default: // The memory location of the struct is up-to-date, but an offset might have accumulated 
        if (mloc.ComponentOffs > 0)
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, mloc.ComponentOffs);
        break;

    case kSYM_GlobalVar:
        scrip->write_cmd2(
            SCMD_LITTOREG,
            SREG_MAR, mloc.StartOffs + mloc.ComponentOffs);
        scrip->fixup_previous(kFx_GlobalData);
        break;

    case kSYM_Import:
        scrip->write_cmd2(
            SCMD_LITTOREG,
            SREG_MAR, mloc.StartOffs + mloc.ComponentOffs);
        scrip->fixup_previous(kFx_Import);
        break;

    case kSYM_LocalVar:
        scrip->write_cmd1(
            SCMD_LOADSPOFFS,
            scrip->cur_sp - mloc.StartOffs - mloc.ComponentOffs);
        break;
    }
    mloc.LType = kSYM_NoType;
    mloc.ComponentOffs = 0;
    return;
}


// We're in the parameter list of a function call, and we have less parameters than declared.
// Provide defaults for the missing values
int AccessData_FunctionCall_ProvideDefaults(ccCompiledScript *scrip, int num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import)
{
    for (size_t arg_idx = num_func_args; arg_idx > num_supplied_args; arg_idx--)
    {
        if (!sym.entries[funcSymbol].funcParamHasDefaultValues[arg_idx])
        {
            cc_error("Function call parameter # %d isn't provided and does not have a default value", arg_idx);
            return -1;
        }

        // push the default value onto the stack
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, sym.entries[funcSymbol].funcParamDefaultValues[arg_idx]);

        if (func_is_import)
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            scrip->push_reg(SREG_AX);
    }
    return 0;
}


int AccessData_FunctionCall_PushParams(ccCompiledScript *scrip, const AGS::SymbolScript &paramList, size_t closedParenIdx, size_t num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol, bool func_is_import, bool keep_mar)
{
    size_t param_num = num_supplied_args + 1;
    size_t start_of_this_param = 0;
    int end_of_this_param = closedParenIdx;  // can become < 0, points to (last byte of parameter + 1)
    // Go backwards through the parameters, since they must be pushed that way
    do
    {
        // Find the start of the next parameter
        param_num--;
        int paren_nesting_depth = 0;
        for (size_t paramListIdx = end_of_this_param - 1; true; paramListIdx--)
        {
            // going backwards so ')' increases the depth level
            const SymbolType idx_type = sym.get_type(paramList[paramListIdx]);
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

        // Compile the parameter
        if (end_of_this_param - start_of_this_param < 0)
        {
            cc_error("Internal error: parameter length is negative");
            return -99;
        }

        if (keep_mar) // mustn't clobber MAR
            scrip->push_reg(SREG_MAR);
        int retval = ParseExpression_Subexpr(scrip, &paramList[start_of_this_param], end_of_this_param - start_of_this_param);
        if (retval < 0) return retval;
        if (keep_mar)
            scrip->pop_reg(SREG_MAR);

        if (param_num <= num_func_args) // we know what type to expect
        {
            // If we need a string object ptr but AX contains a normal string, convert AX
            int parameterType = sym.entries[funcSymbol].funcparamtypes[param_num];
            ConvertAXIntoStringObject(scrip, parameterType);

            if (IsVartypeMismatch(scrip->ax_vartype, parameterType, true))
                return -1;

            // If we need a normal string but AX contains a string object ptr, 
            // check that this ptr isn't null
            DoNullCheckOnStringInAXIfNecessary(scrip, parameterType);
        }

        if (func_is_import)
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            scrip->push_reg(SREG_AX);

        end_of_this_param = start_of_this_param - 1;

    }
    while (end_of_this_param > 0);

    return 0;
}


// Count parameters, check that all the parameters are non-empty; find closing paren
int AccessData_FunctionCall_CountAndCheckParm(const AGS::SymbolScript &paramList, size_t paramListLen, AGS::Symbol funcSymbol, size_t &indexOfCloseParen, size_t &num_supplied_args)
{
    size_t paren_nesting_depth = 1;
    num_supplied_args = 1;
    size_t paramListIdx;
    bool found_param_symbol = false;

    for (paramListIdx = 1; paramListIdx < paramListLen; paramListIdx++)
    {
        const SymbolType idx_type = sym.get_type(paramList[paramListIdx]);

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
        sym.get_type(paramList[1]) == kSYM_CloseParenthesis)
    {
        num_supplied_args = 0;
    }

    indexOfCloseParen = paramListIdx;

    if (sym.get_type(paramList[indexOfCloseParen]) != kSYM_CloseParenthesis)
    {
        cc_error("Missing ')' at the end of the parameter list");
        return -1;
    }

    if (indexOfCloseParen > 0 &&
        sym.get_type(paramList[indexOfCloseParen - 1]) == kSYM_Comma)
    {
        cc_error("Last argument in function call is empty");
        return -1;
    }

    if (indexOfCloseParen < paramListLen - 1 &&
        sym.get_type(paramList[indexOfCloseParen + 1]) != kSYM_Semicolon)
    {
        cc_error("Internal error: Unexpected symbols trailing the parameter list");
        return -1;
    }

    if (paren_nesting_depth > 0)
    {
        cc_error("Internal error: Parser confused near '%s'", sym.get_name_string(funcSymbol).c_str());
        return -1;
    }

    return 0;
}

// We are processing a function call. General the actual function call
void AccessData_GenerateFunctionCall(ccCompiledScript *scrip, AGS::Symbol name_of_func, size_t num_args, bool func_is_import)
{
    if (func_is_import)
    {
        // tell it how many args for this call (nested imported functions
        // cause stack problems otherwise)
        scrip->write_cmd1(SCMD_NUMFUNCARGS, num_args);
    }

    // Call the function: Get address into AX
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, sym.entries[name_of_func].soffs);

    if (func_is_import)
    {
        scrip->fixup_previous(kFx_Import);
        scrip->write_cmd1(SCMD_CALLEXT, SREG_AX); // do the call
        // At runtime, we will arrive here when the function call has returned
        // restore the stack
        if (num_args > 0)
            scrip->write_cmd1(SCMD_SUBREALSTACK, num_args);
        return;
    }

    // Func is non-import
    if (g_FCM.IsForwardDecl(name_of_func))
        g_FCM.TrackForwardDeclFuncCall(scrip, name_of_func, scrip->codesize - 1);
    scrip->fixup_previous(kFx_Function);
    scrip->write_cmd1(SCMD_CALL, SREG_AX);  // do the call

    // At runtime, we will arrive here when the function call has returned
    // restore the stack
    if (num_args > 0)
    {
        scrip->cur_sp -= num_args * 4;
        scrip->write_cmd2(SCMD_SUB, SREG_SP, num_args * 4);
    }
}

// We are processing a function call.
// Get the parameters of the call and push them onto the stack.
// Return the number of the parameters pushed
// NOTE: If keep_mar, we must be careful not to clobber the MAR register
int AccessData_PushFunctionCallParams(ccCompiledScript *scrip, AGS::Symbol name_of_func, bool func_is_import, AGS::SymbolScript &paramList, size_t paramListLen, bool keep_mar, size_t &actual_num_args)
{
    // Expected number of arguments, or expected minimal number of arguments
    size_t const num_func_args = sym.entries[name_of_func].get_num_args();
    bool const func_is_varargs = sym.entries[name_of_func].is_varargs();

    size_t num_supplied_args = 0;
    size_t indexOfClosedParen;
    int retval = AccessData_FunctionCall_CountAndCheckParm(paramList, paramListLen, name_of_func, indexOfClosedParen, num_supplied_args);
    if (retval < 0) return retval;

    // Push default parameters onto the stack when applicable
    // This will give an error if there aren't enough default parameters
    if (num_supplied_args < num_func_args)
    {
        int retval = AccessData_FunctionCall_ProvideDefaults(scrip, num_func_args, num_supplied_args, name_of_func, func_is_import);
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
        int retval = AccessData_FunctionCall_PushParams(scrip, paramList, indexOfClosedParen, num_func_args, num_supplied_args, name_of_func, func_is_import, keep_mar);
        if (retval < 0) return retval;
    }

    actual_num_args = std::max(num_supplied_args, num_func_args);
    return 0;
}

int AccessData_FunctionCall(ccCompiledScript *scrip, AGS::Symbol name_of_func, AGS::SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, AGS::Vartype &rettype)
{
    bool const func_is_import = FlagIsSet(sym.get_flags(name_of_func), kSFLG_Imported);

    if (sym.get_type(symlist[1]) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }

    AGS::SymbolScript paramList = symlist + 1;
    size_t paramListLen = symlist_len - 1;

    // Find out whether the function uses "this" (non-static method)
    bool func_uses_this = false;
    if (std::string::npos != sym.get_name_string(name_of_func).find("::"))
    {
        func_uses_this = true;
        // static functions don't have an object instance, so no "this"
        if (FlagIsSet(sym.get_flags(name_of_func), kSFLG_Static))
            func_uses_this = false;
    }

    if (func_uses_this)
    {
        // Get address of outer into MAR; this is the object that the func will use
        AccessData_MakeMARCurrent(scrip, mloc);
        // Save OP since we must restore it after the func call
        scrip->push_reg(SREG_OP);
    }

    // Process parameter list of call; get the number of parameters used
    size_t num_args = 0;
    int retval = AccessData_PushFunctionCallParams(scrip, name_of_func, func_is_import, paramList, paramListLen, func_uses_this, num_args);
    if (retval < 0) return retval;

    if (func_uses_this)
    {
        // write the address of the function's "this" object to the OP reg
        scrip->write_cmd1(SCMD_CALLOBJ, SREG_MAR);
    }

    // Generate the function call proper
    AccessData_GenerateFunctionCall(scrip, name_of_func, num_args, func_is_import);

    // function return type
    rettype = scrip->ax_vartype = sym.entries[name_of_func].funcparamtypes[0];
    scrip->ax_val_scope = kSYM_LocalVar;

    // At runtime, we have returned from the func call,
    // so we must continue with the object pointer that was in use before the call
    if (func_uses_this)
        scrip->pop_reg(SREG_OP);

    // Note that this function has been accessed at least once
    SetFlag(sym.entries[name_of_func].flags, kSFLG_Accessed, true);
    return 0;
}

int ParseExpression_NoOps(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len)
{
    if (kSYM_OpenParenthesis == sym.get_type(symlist[0]))
        return ParseExpression_OpenParenthesis(scrip, symlist, symlist_len);

    if (kSYM_Operator != sym.get_type(symlist[0]))
        return ReadDataIntoAX(scrip, symlist, symlist_len);

    // The operator at the beginning must be a unary minus
    if (SCMD_SUBREG == sym.entries[symlist[0]].operatorToVCPUCmd())
        return ReadDataIntoAX(scrip, &symlist[1], symlist_len - 1, true);

    cc_error("Parse error: Unexpected '%s'", sym.get_name_string(symlist[0]).c_str());
    return -1;
}

int ParseExpression_Subexpr(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len)
{
    if (symlist_len == 0)
    {
        cc_error("Internal error: Cannot parse empty subexpression");
        return -1;
    }
    if (kSYM_CloseBracket == sym.get_type(symlist[0]))
    {
        cc_error("Unexpected ')' at start of expression");
        return -1;
    }

    int lowest_op_idx = IndexOfLowestBondingOperator(symlist, symlist_len);  // can be < 0

    // If the lowest bonding operator is '-' and right in front,
    // then it has been misinterpreted so far: it's really a unary minus
    if ((lowest_op_idx == 0) &&
        (symlist_len > 1) &&
        (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG))
    {
        lowest_op_idx = IndexOfLowestBondingOperator(&symlist[1], symlist_len - 1);
        if (lowest_op_idx >= 0)
            lowest_op_idx++;
    }

    if (lowest_op_idx == 0)
        return ParseExpression_OpIsFirst(scrip, symlist, symlist_len);

    if (lowest_op_idx > 0)
        return ParseExpression_OpIsSecondOrLater(scrip, static_cast<size_t>(lowest_op_idx), symlist, symlist_len);

    // There is no operator in the expression -- therefore, there will
    // just be a variable name or function call or a parenthesized expression

    return ParseExpression_NoOps(scrip, symlist, symlist_len);
}

// symlist starts a bracketed expression; parse it
int AccessData_ArrayIndexIntoAX(ccCompiledScript *scrip, SymbolScript symlist, size_t symlist_len)
{
    int retval = ParseExpression_Subexpr(scrip, symlist, symlist_len);
    if (retval < 0) return retval;

    symlist++;
    symlist_len--;

    // array index must be convertible to an int
    return IsVartypeMismatch(scrip->ax_vartype, sym.getIntSym(), true);
}


// We access a variable or a component of a struct in order to read or write it.
// This is a simple member of the struct.
int AccessData_StructMember(AGS::Symbol component, bool writing, bool access_via_this, SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    SymbolTableEntry &entry = sym.entries[component];

    if (writing && FlagIsSet(entry.flags, kSFLG_WriteProtected) && !access_via_this)
    {
        cc_error(
            "Writeprotected component '%s' must not be modified from outside",
            sym.get_name_string(component).c_str());
        return -1;
    }
    if (FlagIsSet(entry.flags, kSFLG_Protected) && !access_via_this)
    {
        cc_error(
            "Protected component '%s' must not be accessed from outside",
            sym.get_name_string(component).c_str());
        return -1;
    }

    mloc.ComponentOffs += entry.soffs;
    vartype = sym.get_vartype(component);
    symlist++;
    symlist_len--;
    return 0;
}

// Get the symbol for the get or set function corresponding to the attribute given.
int ConstructAttributeFuncName(AGS::Symbol attribsym, bool writing, bool indexed, AGS::Symbol &func)
{
    std::string member_str = sym.get_name_string(attribsym);
    // If "::" in the name, take the part after the last "::"
    size_t const m_access_position = member_str.rfind("::");
    if (std::string::npos != m_access_position)
        member_str = member_str.substr(m_access_position + 2);
    char const *stem_str = writing ? "set" : "get";
    char const *indx_str = indexed ? "i_" : "_";
    std::string func_str = stem_str + (indx_str + member_str);
    func = sym.find_or_add(func_str.c_str());
    return 0;
}

// We call the getter or setter of an attribute
int AccessData_Attribute(ccCompiledScript *scrip, SymbolScript symlist, size_t symlist_len, bool is_attribute_set_func, AGS::Vartype &vartype)
{
    AGS::Symbol const component_of_attribute = symlist[0];
    AGS::Symbol const struct_of_attribute = vartype & kVTY_FlagMask;
    AGS::Symbol name_of_attribute = component_of_attribute;
    int retval = FindOrAddComponent(struct_of_attribute, name_of_attribute);
    if (retval < 0) return retval;
        
    bool const attrib_uses_this =
        !FlagIsSet(sym.get_flags(name_of_attribute), kSFLG_Static);
    bool const is_indexed =
        (symlist_len > 1 && kSYM_OpenBracket == sym.get_type(symlist[1]));

    // Get the appropriate access function (as a symbol)
    AGS::Symbol name_of_func = -1;
    retval = ConstructAttributeFuncName(component_of_attribute, is_attribute_set_func, is_indexed, name_of_func);
    if (retval < 0) return retval;
    retval = FindOrAddComponent(struct_of_attribute, name_of_func);
    if (retval < 0) return retval;

    bool const func_is_import = FlagIsSet(sym.get_flags(name_of_func), kSYM_Import);

    if (attrib_uses_this)
        scrip->push_reg(SREG_OP); // is the current this ptr, must be restored after call

    size_t num_of_args = 0;
    if (is_attribute_set_func)
    {
        // The value to be set is in the AX register; push it as the last parameter
        if (func_is_import)
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            scrip->push_reg(SREG_AX);
        ++num_of_args;
    }

    if (is_indexed)
    {
        if (kSYM_CloseBracket != sym.get_type(symlist[symlist_len - 1]))
        {
            cc_error("Internal error: '[' has no matching ']");
            return -99;
        }
        // The index to be set is in the [...] clause;
        // get it and push it as the first parameter
        if (attrib_uses_this)
            scrip->push_reg(SREG_MAR); // must not be clobbered
        int retval = AccessData_ArrayIndexIntoAX(scrip, &symlist[2], symlist_len - 3);
        if (retval < 0) return retval;
        if (attrib_uses_this)
            scrip->pop_reg(SREG_MAR);

        if (func_is_import)
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            scrip->push_reg(SREG_AX);
        ++num_of_args;
    }

    if (attrib_uses_this)
        scrip->write_cmd1(SCMD_CALLOBJ, SREG_MAR); // make MAR the new this ptr

    // Generate the function call proper
    AccessData_GenerateFunctionCall(scrip, name_of_func, num_of_args, func_is_import);
    if (attrib_uses_this)
        scrip->pop_reg(SREG_OP); // restore old this ptr after the func call

    // attribute return type
    scrip->ax_val_scope = kSYM_LocalVar;
    scrip->ax_vartype = vartype =
        (is_attribute_set_func) ? sym.getVoidSym() : sym.get_vartype(name_of_attribute);

    // Attribute has been accessed
    SetFlag(sym.entries[name_of_attribute].flags, kSFLG_Accessed, true);
    SetFlag(sym.entries[name_of_func].flags, kSFLG_Accessed, true);
    return 0;
}

// This indicates where a value is stored.
// When reading, we need the value itself (but see below for arrays and structs)
// - It can be in AX (kVL_ax_is_value)
// - or in m(MAR) (kVL_mar_pointsto_value).
// - Attributes must be read through their getter function, but afterwards,
//   the value can be put into AX, so we don't need any special case for this.
// When writing, we can't use a value itself. Instead, we need a pointer
// to the adress that has to be modified.
// - This can be MAR, i.e., the value to modify is in m(MAR) (kVL_mar_pointsto_value).
// - or AX, i.e., the value to modify is in m(AX) (kVL_ax_pointsto_value)
// - attributes must be modified by calling their setter function (kVL_attribute)
// Arrays and structs can't fit into AX, so they are usually referenced
// as kVL_mar_pointsto_value or kVL_ax_pointsto_value
// Since this is a pointer in both cases, it can be used for reading and writing
enum ValueLocation
{
    kVL_ax_is_value,         // The value is in register AX
    kVL_ax_pointsto_value,   // The value is in m(AX)
    kVL_mar_pointsto_value,  // The value is in m(MAR)
    kVL_attribute            // The value must be modified by function call
};

int AccessData_Dereference(ccCompiledScript *scrip, ValueLocation &vloc, MemoryLocation &mloc)
{
    if (kVL_ax_pointsto_value == vloc)
    {
        scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        vloc = kVL_mar_pointsto_value;
    }
    if (kVL_mar_pointsto_value == vloc)
    {
        AccessData_MakeMARCurrent(scrip, mloc);
        scrip->write_cmd0(SCMD_CHECKNULL);
        scrip->write_cmd1(SCMD_MEMREADPTR, SREG_MAR);
    }
    else
    {
        cc_error("Cannot access value");
        return -1;
    }
    mloc.LType = kSYM_NoType;
    mloc.ComponentOffs = 0;
    return 0;
}

int AccessData_ProcessArrayIndexConstant(AGS::Symbol index_symbol, int array_size, size_t element_size, MemoryLocation &mloc)
{
    int array_index = -1;
    int retval = ParseLiteralOrConstvalue(index_symbol, array_index, false, "Error parsing integer");
    if (retval < 0) return retval;
    if (array_size > 0 && array_index >= array_size)
    {
        cc_error(
            "Array index %d out of bounds (maximum is %d)",
            array_index,
            array_size - 1);
        return -1;
    }
    if (array_index < 0)
    {
        cc_error("Array index %d out of bounds (minimum is 0)", array_index);
        return -1;
    }
    mloc.ComponentOffs += array_index * element_size;
    return 0;
}

// We're processing some struct component or global or local variable.
// If an array index follows, parse it and shorten symlist accordingly
int AccessData_ProcessAnyArrayIndex(ccCompiledScript *scrip, ValueLocation vloc_of_array, int array_size, SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    if (0 == symlist_len || kSYM_OpenBracket != sym.get_type(symlist[0]))
        return 0;

    // Find the location of the close bracket
    AGS::Symbol *close_brac_loc = symlist + 1;
    size_t len_starting_with_close_brac = symlist_len - 1;
    AGS::Symbol stoplist[] = { 0 };
    SkipToScript(stoplist, 0, close_brac_loc, len_starting_with_close_brac);
    if (len_starting_with_close_brac == 0)
    {
        cc_error("Internal error: '[' has no matching ']");
        return -99;
    }
    size_t bracketed_expr_length = close_brac_loc + 1 - symlist;
    
    // Must be any sort of array; if AX points to it, must be a dynarray
    bool const is_dynarray = FlagIsSet(vartype, kVTY_DynArray);
    bool const is_array = FlagIsSet(vartype, kVTY_Array);
    AGS::Vartype const core_vartype = vartype & kVTY_FlagMask;
    if (!is_dynarray && (!is_array || kVL_ax_pointsto_value == vloc_of_array))
    {
        cc_error("Array index is only legal after an array");
        return -1;
    }

    size_t const element_coresize = sym.entries[core_vartype].ssize;
    AGS::Vartype const element_type = vartype & ~kVTY_Array & ~kVTY_DynArray;
    size_t const element_size = FlagIsSet(element_type, kVTY_Pointer) ? SIZE_OF_POINTER : element_coresize;
    vartype = element_type;

    if (is_dynarray)
        AccessData_Dereference(scrip, vloc, mloc);

    if (kVL_ax_pointsto_value == vloc)
    {
        scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        vloc = kVL_mar_pointsto_value;
        mloc = { kSYM_NoType, 0, 0 };
    }

    // Ideally, we would calculate compile time constants here. For now, only
    // process the special case [INT] where INT is a non-negative integer or constant.
    if (1 == bracketed_expr_length &&
        (kSYM_LiteralInt == sym.get_type(symlist[1]) || kSYM_Const == sym.get_type(symlist[1])))
    {
        int retval = AccessData_ProcessArrayIndexConstant(symlist[1], array_size, element_size, mloc);
        if (retval < 0) return retval;
        symlist = close_brac_loc + 1;
        symlist_len = len_starting_with_close_brac - 1;
        return 0;
    }

    // Save MAR from being clobbered if it may have been (partially) set
    if (mloc.LType == kSYM_NoType)
        scrip->push_reg(SREG_MAR);

    int retval = AccessData_ArrayIndexIntoAX(scrip, symlist + 1, bracketed_expr_length - 2);
    if (retval < 0) return retval;
    if (is_dynarray)
        scrip->write_cmd1(SCMD_DYNAMICBOUNDS, SREG_AX);
    else if (array_size > 0)
        scrip->write_cmd2(SCMD_CHECKBOUNDS, SREG_AX, array_size);
    else // it's a static array, and we don't know the size
        ;   // don't check the bounds

    if (mloc.LType == kSYM_NoType)
        scrip->pop_reg(SREG_MAR);

    AccessData_MakeMARCurrent(scrip, mloc);

    if (element_size != 1)
        scrip->write_cmd2(SCMD_MUL, SREG_AX, element_size); // Multiply offset with length of one array entry
    scrip->write_cmd2(SCMD_ADDREG, SREG_MAR, SREG_AX); // Add offset

    symlist = close_brac_loc + 1;
    symlist_len = len_starting_with_close_brac - 1;
    return 0;
}

int AccessData_GlobalOrLocalVar(ccCompiledScript *scrip, bool is_global, bool writing, AGS::SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    AGS::Symbol const varname = symlist[0];
    SymbolTableEntry &entry = sym.entries[varname];
    AGS::CodeCell const soffs = entry.soffs;
    symlist++;
    symlist_len--;

    if (writing && FlagIsSet(entry.flags, kSFLG_Readonly))
    {
        cc_error("Cannot write to readonly '%s'", sym.get_name_string(varname).c_str());
        return -1;
    }

    if (FlagIsSet(sym.entries[varname].flags, kSFLG_Imported))
        mloc.LType = kSYM_Import;
    else
        mloc.LType = is_global ? kSYM_GlobalVar : kSYM_LocalVar;
    mloc.StartOffs = soffs;
    mloc.ComponentOffs = 0;
    
    vartype = sym.get_vartype(varname);

    // Process an array index if it follows
    ValueLocation vl_dummy = kVL_mar_pointsto_value;
    return AccessData_ProcessAnyArrayIndex(scrip, kVL_mar_pointsto_value, sym.entries[varname].arrsize, symlist, symlist_len, vl_dummy, mloc, vartype);
}

int AccessData_LitFloat(ccCompiledScript *scrip, bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    char *endptr;
    std::string float_as_string = sym.get_name_string(symlist[0]);
    char const *instring = float_as_string.c_str();
    double const d = strtod(instring, &endptr);
    if (endptr != instring + sym.get_name_string(symlist[0]).length())
    {
        cc_error("Illegal floating point literal '%s'", instring);
        return -1;
    }
    if (HUGE_VAL == d)
    {
        cc_error("Floating point literal '%s' is out of range", instring);
        return -1;
    }
    float const f = static_cast<float>(d * (negate? -1 : 1));
    int const i = InterpretFloatAsInt(f);

    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, i);
    scrip->ax_vartype = vartype = sym.getFloatSym();
    scrip->ax_val_scope = kSYM_GlobalVar;
    symlist++;
    symlist_len--;
    return 0;
}

int AccessData_LitOrConst(ccCompiledScript *scrip, bool negateLiteral, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    int varSymValue;
    int retval = ParseLiteralOrConstvalue(symlist[0], varSymValue, negateLiteral, "Error parsing integer value");
    if (retval < 0) return retval;
    symlist++;
    symlist_len--;

    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, varSymValue);
    scrip->ax_vartype = vartype = sym.getIntSym();
    scrip->ax_val_scope = kSYM_GlobalVar;

    return 0;
}

int AccessData_Null(ccCompiledScript *scrip, bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    if (negate)
    {
        cc_error("'-null' is undefined");
        return -1;
    }

    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    scrip->ax_vartype = vartype = sym.getNullSym() | kVTY_Pointer;
    scrip->ax_val_scope = kSYM_GlobalVar;
    symlist++;
    symlist_len--;

    return 0;
}

int AccessData_String(ccCompiledScript *scrip, bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, AGS::Vartype &vartype)
{
    if (negate)
    {
        cc_error("Can't calculate the negative value of a string");
        return -1;
    }

    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, sym.entries[symlist[0]].soffs);
    scrip->fixup_previous(kFx_String);
    scrip->ax_vartype = vartype = sym.getOldStringSym() | kVTY_Const;
    symlist++;
    symlist_len--;
    return 0;
}

// We are parsing the first part of a STRUCT.STRUCT.STRUCT... clause.
// This first part can also be a literal or constant instead of a struct.

int AccessData_StaticFunctionCallOrAttribute(ccCompiledScript *scrip, bool writing, AGS::SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, AGS::Vartype &vartype)
{
    if (symlist_len < 2 || kSYM_Dot != sym.get_type(symlist[1]))
    {
        cc_error("Expected '.' after '%s'", sym.get_name_string(symlist[0]));
        return -1;
    }
    if (symlist_len < 3)
    {
        cc_error("Expected component after '%s.'", sym.get_name_string(symlist[0]));
        return -1;
    }
    AGS::Symbol const staticname = MangleStructAndComponent(symlist[0], symlist[2]);
    symlist += 2; // skip '.' and component
    symlist_len -= 2;

    vloc = kVL_ax_is_value;
    MemoryLocation mloc = { kSYM_NoType, 0, 0 };
    if (kSYM_Function == sym.get_type(staticname))       
        return AccessData_FunctionCall(scrip, staticname, symlist, symlist_len, mloc, vartype);
    if (kSYM_Attribute == sym.get_type(staticname))
    {
        if (writing)
        {
            // We can't process that here, so return to the assignment we came from
            vloc = kVL_attribute;
            return 0;
        }
        return AccessData_Attribute(scrip, symlist, symlist_len, writing, vartype);
    }

    cc_error("Function or attribute '%s' unknown", sym.get_name_string(staticname).c_str());
    return -1;
}

// Negates the value; this clobbers AX and BX
void AccessData_Negate(ccCompiledScript *scrip, ValueLocation vloc)
{
    scrip->write_cmd2(SCMD_LITTOREG, SREG_BX, 0);
    if (kVL_mar_pointsto_value == vloc)
        scrip->write_cmd1(SCMD_MEMREAD, SREG_AX);
    scrip->write_cmd2(SCMD_SUBREG, SREG_BX, SREG_AX);
    if (kVL_mar_pointsto_value == vloc)
        scrip->write_cmd1(SCMD_MEMWRITE, SREG_AX);
}

// We're getting a variable, literal, constant, func call or the first element
// of a STRUCT.STRUCT.STRUCT... cascade.
// This moves symlist in all cases except for the cascade to the end of what is parsed,
// and in case of a cascade, to the end of the first element of the cascade, i.e.,
// to the position of the '.'. 
int AccessData_FirstClause(ccCompiledScript *scrip, bool writing, bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, MemoryLocation &mloc, AGS::Vartype &vartype, bool &access_via_this)
{
    if (symlist_len < 1)
    {
        cc_error("Internal error: Empty variable");
        return -99;
    }

    if (sym.getThisSym() == symlist[0])
    {
        vartype = sym.get_vartype(sym.getThisSym());
        if (0 == vartype)
        {
            cc_error("'this' is only legal in non-static struct functions");
            return -1;
        }
        vloc = kVL_mar_pointsto_value;
        scrip->write_cmd2(SCMD_REGTOREG, SREG_OP, SREG_MAR);
        scrip->write_cmd0(SCMD_CHECKNULL);
        mloc.LType = kSYM_NoType;
        access_via_this = true;
        symlist++;
        symlist_len--;
        return 0;
    }

    switch (sym.get_type(symlist[0]))
    {
    default:
    {
        // If this unknown symbol can be interpreted as a component of this,
        // treat it that way.
        vartype = sym.get_vartype(sym.getThisSym());
        AGS::Symbol const thiscomponent = MangleStructAndComponent(vartype, symlist[0]);
        if (0 != sym.entries[thiscomponent].stype)
        {
            vloc = kVL_mar_pointsto_value;
            scrip->write_cmd2(SCMD_REGTOREG, SREG_OP, SREG_MAR);
            scrip->write_cmd0(SCMD_CHECKNULL);
            access_via_this = true;
            // We _should_ prepend "this." to symlist here but can't do that (easily).
            // So we don't and the '.' that should be prepended doesn't exist.
            // To compensate for that, we back up symlist by one index
            mloc.LType = kSYM_NoType;
            symlist--; 
            symlist_len++;
            return 0;
        }
        
        cc_error("Unexpected '%s'", sym.get_name_string(symlist[0]).c_str());
        return -1;
    }

    case kSYM_Constant:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_LitOrConst(scrip, negate, symlist, symlist_len, vartype);

    case kSYM_Function:
    {
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        int retval = AccessData_FunctionCall(scrip, symlist[0], symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;
        if (negate)
            AccessData_Negate(scrip, vloc);
        return 0;
    }

    case kSYM_GlobalVar:
    {
        scope = kSYM_GlobalVar;
        vloc = kVL_mar_pointsto_value;
        bool const is_global = true;
        int retval = AccessData_GlobalOrLocalVar(scrip, is_global, writing, symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;
        if (negate)
            AccessData_Negate(scrip, vloc);
        return 0;
    }

    case kSYM_LiteralFloat:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_LitFloat(scrip, negate, symlist, symlist_len, vartype);

    case kSYM_LiteralInt:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_LitOrConst(scrip, negate, symlist, symlist_len, vartype);

    case kSYM_LocalVar:
    {
        scope =
            (FlagIsSet(sym.get_flags(symlist[0]), kSFLG_Parameter)) ?
            kSYM_GlobalVar : kSYM_LocalVar;
        vloc = kVL_mar_pointsto_value;
        bool const is_global = false;
        int retval = AccessData_GlobalOrLocalVar(scrip, is_global, writing, symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;
        if (negate)
            AccessData_Negate(scrip, vloc);
        return 0;
    }

    case kSYM_Null:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_Null(scrip, negate, symlist, symlist_len, vartype);

    case kSYM_LiteralString:
        if (writing) break; // to error msg
        scope = kSYM_GlobalVar;
        vloc = kVL_ax_is_value;
        return AccessData_String(scrip, negate, symlist, symlist_len, vartype);

    case kSYM_Vartype:
    {
        scope = kSYM_GlobalVar;
        int retval = AccessData_StaticFunctionCallOrAttribute(scrip, writing, symlist, symlist_len, vloc, vartype);
        if (retval < 0) return retval;
        if (negate)
            AccessData_Negate(scrip, vloc);
        return 0;
    }
    }

    cc_error("Cannot assign a value to '%s'", sym.get_name_string(symlist[0]).c_str());
    return -1;
}

// We're processing a STRUCT.STRUCT. ... clause.
// We've already processed some structs, and the type of the last one is vartype.
// Now we process a component of vartype.
int AccessData_SubsequentClause(ccCompiledScript *scrip, bool writing, bool access_via_this, AGS::SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, MemoryLocation &mloc, AGS::Vartype &vartype)
{
    AGS::Symbol component = symlist[0];
    SymbolType component_type = sym.get_type(component);
    if (kSYM_Attribute != component_type &&
        kSYM_Function != component_type &&
        kSYM_StructComponent != component_type)
    {
        int retval = FindOrAddComponent(vartype & kVTY_FlagMask, component);
        if (retval < 0) return retval;
        component_type = sym.get_type(component);
    }

    int retval = 0;
    switch (component_type)
    {
    default:
        cc_error(
            "Unexpected '%s'",
            sym.get_name_string(symlist[0]).c_str());
        return -1;

    case kSYM_Attribute:
    {
        if (writing)
        {
            // We cannot process this here so return to the assignment that
            // this attribute was originally called from
            vartype = sym.get_vartype(component);
            vloc = kVL_attribute;
            return 0;
        }
        vloc = kVL_ax_is_value;
        bool const is_attribute_set_func = false;
        return AccessData_Attribute(scrip, symlist, symlist_len, is_attribute_set_func, vartype);
    }

    case kSYM_Function:
    {
        vloc = kVL_ax_is_value;
        scope = kSYM_LocalVar;
        retval = AccessData_FunctionCall(scrip, component, symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;        
        int const zero_array_size_dummy = 0; // A function cannot return a static array.
        // We've just called a function, so its ret value will be in AX.
        // Thus _if_ an array index follows, it must be AX that points to that array.
        return AccessData_ProcessAnyArrayIndex(scrip, kVL_ax_pointsto_value, zero_array_size_dummy, symlist, symlist_len, vloc, mloc, vartype);
    }

    case kSYM_StructComponent:
        vloc = kVL_mar_pointsto_value;
        retval = AccessData_StructMember(component, writing, access_via_this, symlist, symlist_len, mloc, vartype);
        if (retval < 0) return retval;
        return AccessData_ProcessAnyArrayIndex(scrip, vloc, sym.entries[component].arrsize, symlist, symlist_len, vloc, mloc, vartype);
    }
    
    return 0; // Can't reach
}

// We are in a STRUCT.STRUCT.STRUCT... cascade.
// Check whether we have passed the last dot
int AccessData_IsClauseLast(AGS::SymbolScript symlist, size_t symlist_len, bool &is_last)
{
    AGS::Symbol const stoplist[] = { kSYM_Dot };
    SkipToScript(stoplist, 1, symlist, symlist_len);
    is_last = (0 == symlist_len || kSYM_Dot != sym.get_type(symlist[0]));
    return 0;
}

// Access a variable, constant, literal, func call, struct.component.component cascade, etc.
// Result is in AX or m[MAR], dependent on vloc. Type is in vartype.
// At end of function, symlist and symlist_len will point to the part of the symbol string
// that has not been processed yet
// NOTE: If this selects an attribute for writing, then the corresponding function will
// _not_ be called and symlist[0] will be the attribute.
int AccessData(ccCompiledScript *scrip, bool writing, bool negate, AGS::SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, AGS::Vartype &vartype)
{
    if (symlist_len == 0)
    {
        cc_error("Internal error: empty expression");
        return -99;
    }
    // For memory accesses, we set the MAR register lazily so that we can
    // accumulate offsets at runtime instead of compile time.
    // This struct tracks what we will need to do to set the MAR register.
    MemoryLocation mloc = { kSYM_NoType, 0, 0 };

    bool clause_is_last = false;
    int retval = AccessData_IsClauseLast(symlist, symlist_len, clause_is_last);
    if (retval < 0) return retval;

    bool access_via_this = false; // only true when "this" has just been parsed

    // If we are reading, then all the accesses are for reading.
    // If we are writing, then all the accesses except for the last one
    // are for reading and the last one will be for writing.
    retval = AccessData_FirstClause(scrip, (writing && clause_is_last), negate, symlist, symlist_len,  vloc, scope, mloc, vartype, access_via_this);
    if (retval < 0) return retval;

    AGS::Vartype outer_vartype = 0;

    // Unfortunately, the while condition is ugly:
    // Normally, the while body must be executed whenever symlist starts with a '.'.
    // However, if the previous function has assumed a "this." that isn't there,
    // then symlist won't start with a '.' but the while body must be executed anyway.
    // This is why the while condition has "access_via_this" in it.
    while (symlist_len > 0 && (kSYM_Dot == sym.get_type(symlist[0]) || access_via_this))
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
        AGS::Flags const outer_vartype_flags = sym.get_flags(outer_vartype);
        bool outer_is_dynarray = false;

        if (!FlagIsSet(outer_vartype_flags, kSFLG_StructType))
        {
            cc_error("Expected a struct before '.' but did not find it");
            return -1;
        }

        if (FlagIsSet(vartype, kVTY_DynArray))
        {
            // Dereference the dynarray
            int retval = AccessData_Dereference(scrip, vloc, mloc);
            if (retval < 0) return retval;
            SetFlag(vartype, kVTY_DynArray, false);
            outer_is_dynarray = true;
        }

        // Note: If _both_ kVTY_Pointer and kVTY_Array is set,
        // then this is an array of pointers instead of a pointer
        // and this array must NOT be dereferenced.
        if (FlagIsSet(vartype, kVTY_Pointer) && !FlagIsSet(vartype, kVTY_Array))
        {
            // Dereference the pointer
            int retval = AccessData_Dereference(scrip, vloc, mloc);
            if (retval < 0) return retval;
            SetFlag(vartype, kVTY_Pointer, false);
        }

        retval = AccessData_IsClauseLast(symlist, symlist_len, clause_is_last);
        if (retval < 0) return retval;

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        retval = AccessData_SubsequentClause(scrip, (clause_is_last && writing), access_via_this, symlist, symlist_len, vloc, scope, mloc, vartype);
        if (retval < 0) return retval;

        // Only the _immediate_ access via 'this.' counts for this flag.
        // This has passed now, so reset the flag.
        access_via_this = false;
    }

    if (kVL_attribute == vloc)
    {
        // Caller will do the assignment
        // For this to work, the caller must know the type of the struct
        // in which the attribute resides
        vartype = outer_vartype;
        return 0;
    }

    if (kVL_ax_is_value == vloc)
    {
        scrip->ax_vartype = vartype;
        scrip->ax_val_scope = scope;
        return 0;
    }

    AccessData_MakeMARCurrent(scrip, mloc);
    return 0;
}

// In order to avoid push AX/pop AX, find out common cases that don't clobber AX
bool AccessData_MayAccessClobberAX(ccCompiledScript *scrip, SymbolScript symlist, size_t symlist_len)
{
    if (kSYM_GlobalVar != sym.get_type(symlist[0]) && kSYM_LocalVar != sym.get_type(symlist[0]))
        return true;

    if (symlist_len == 1)
        return false;

    for (size_t symlist_idx = 0; symlist_idx < symlist_len - 3; symlist_idx += 2)
    {
        if (kSYM_Dot != sym.get_type(symlist[symlist_idx + 1]))
            return true;
        AGS::Symbol const compo = MangleStructAndComponent(symlist[0], symlist[2]);
        if (kSYM_StructComponent != sym.get_type(compo))
            return true;
    }
    return false;
}

// We are typically in an assignment LHS = RHS; the RHS has already been
// evaluated, and the result of that evaluation is in AX.
// Store AX into the memory location that corresponds to LHS, or
// call the attribute function corresponding to LHS.
int AccessData_Assign(ccCompiledScript *scrip, SymbolScript symlist, size_t symlist_len)
{
    // AX contains the result of evaluating the RHS of the assignment
    // Save on the stack so that it isn't clobbered
    AGS::Vartype rhsvartype = scrip->ax_vartype;
    int rhsscope = scrip->ax_val_scope;
    bool const may_clobber = AccessData_MayAccessClobberAX(scrip, symlist, symlist_len);
    if (may_clobber)
        scrip->push_reg(SREG_AX);

    bool const writing = true;
    bool const negate_dummy = false; // when writing, this parameter is pointless
    ValueLocation vloc;
    AGS::Vartype lhsvartype;
    int lhsscope;
    int retval = AccessData(scrip, writing, negate_dummy, symlist, symlist_len, vloc, lhsscope, lhsvartype);
    if (retval < 0) return retval;

    if (may_clobber)
        scrip->pop_reg(SREG_AX);
    scrip->ax_vartype = rhsvartype;
    scrip->ax_val_scope = rhsscope;

    if (kVL_attribute == vloc)
    {
        // We need to call the attribute setter 
        AGS::Vartype struct_of_attribute = lhsvartype;

        bool const is_attribute_set_func = true;
        return AccessData_Attribute(scrip, symlist, symlist_len, is_attribute_set_func, struct_of_attribute);
    }

    ConvertAXIntoStringObject(scrip, lhsvartype);
    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype))
    {
        cc_error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            sym.get_name_string(rhsvartype).c_str(),
            sym.get_name_string(lhsvartype).c_str());
        return -1;
    }

    if (kVL_ax_is_value == vloc)
    {
        // This is a non-writable value (e.g., a literal)
        cc_error("Cannot assign to this value");
        return -1;
    }

    // MAR points to the value
    if (FlagIsSet(rhsvartype, kVTY_DynArray | kVTY_Pointer))
        scrip->write_cmd1(SCMD_MEMWRITEPTR, SREG_AX);
    else
        scrip->write_cmd1(
            GetWriteCommandForSize(sym.entries[lhsvartype & kVTY_FlagMask].ssize),
            SREG_AX);
    return 0;
}


int ReadDataIntoAX(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len, bool negate)
{
    ValueLocation vloc;
    int scope;
    AGS::Vartype vartype;
    int retval = AccessData(scrip, false, negate, symlist, symlist_len, vloc, scope, vartype);
    if (retval < 0) return retval;
    if (kVL_mar_pointsto_value != vloc)
        return 0;

    // Get the result into AX
    if (FlagIsSet(vartype, kVTY_DynArray | kVTY_Pointer))
        scrip->write_cmd1(SCMD_MEMREADPTR, SREG_AX);
    else
        scrip->write_cmd1(
            GetReadCommandForSize(sym.entries[vartype & kVTY_FlagMask].ssize),
            SREG_AX);
    scrip->ax_vartype = vartype;
    scrip->ax_val_scope = scope;
    return 0;
}

// Read the symbols of an expression and buffer them into expr_script
// At end of routine, the cursor will be positioned in such a way
// that targ->getnext() will get the symbol after the expression
int BufferExpression(ccInternalList *targ, ccInternalList &expr_script)
{
    int nesting_depth = 0;

    for (AGS::Symbol nextsym = targ->peeknext(); 
        nextsym >= 0;
        nextsym = targ->peeknext())
    {
        size_t const pos = targ->pos; // for backing up if necessary

        // Skip over parts that are enclosed in braces, brackets, or parens
        SymbolType const nexttype = sym.get_type(nextsym);
        if (kSYM_OpenParenthesis == nexttype || kSYM_OpenBracket == nexttype || kSYM_OpenBrace == nexttype)
            ++nesting_depth;
        if (kSYM_CloseParenthesis == nexttype || kSYM_CloseBracket == nexttype || kSYM_CloseBrace == nexttype)
            if (--nesting_depth < 0)
                break; // this symbol can't be part of the current expression
        if (nesting_depth == 0)
        {

            if (kSYM_New == nexttype)
            {
                // This is only allowed if a type follows
                targ->getnext(); // eat the nexttype
                AGS::Symbol nextnextsym = targ->getnext();
                if (kSYM_Vartype == sym.get_type(nextnextsym))
                {
                    expr_script.write(nextsym);
                    expr_script.write(nextnextsym);
                    continue;
                }
                targ->pos = pos; // Back up so that the nexttype is still unread
                break;
            }

            if (kSYM_Vartype == nexttype)
            {
                // This is only allowed if a dot follows
                targ->getnext(); // eat the nexttype
                AGS::Symbol nextnextsym = targ->getnext();
                if (kSYM_Dot == sym.get_type(nextnextsym))
                {
                    expr_script.write(nextsym);
                    expr_script.write(nextnextsym);
                    continue;
                }
                targ->pos = pos; // Back up so that the nexttype is still unread
                break;
            }
        

            // Apart from the exceptions above, all symbols starting at NOTEXPRESSION can't
            // be part of an expression
            if (nexttype >= NOTEXPRESSION)
                break;
        }

        targ->getnext(); // Eat the nextsym
        expr_script.write(nextsym);
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
int ParseExpression(ccInternalList *targ, ccCompiledScript *scrip)
{
    ccInternalList expr_script;
    int retval = BufferExpression(targ, expr_script);
    if (retval < 0) return retval;

    // we now have the expression in expr_script, parse it
    return ParseExpression_Subexpr(scrip, expr_script.script, expr_script.length);
}

// We are parsing the left hand side of a += or similar statement.
int ParseAssignment_ReadLHSForModification(ccCompiledScript *scrip, ccInternalList const *lhs, ValueLocation &vloc, AGS::Vartype &lhstype)
{
    int scope;
    size_t lhs_length = (lhs->length < 0) ? 0 : lhs->length;
    AGS::SymbolScript lhs_script = lhs->script;
    bool const negative = false; // LHS can't start with a unary minus
    bool const writing = false; // reading access
    int retval = AccessData(scrip, writing, negative, lhs_script, lhs_length, vloc, scope, lhstype);
    if (retval < 0) return retval;
    if (lhs_length > 0)
    {
        cc_error("Internal error: Unexpected symbols following expression");
        return -99;
    }
    if (kVL_ax_is_value == vloc)
    {
        cc_error("This operator cannot be used here");
        return -1;
    }
    if (kVL_mar_pointsto_value == vloc)
    {
        // write memory to AX
        scrip->ax_vartype = lhstype;
        scrip->ax_val_scope = scope;
        scrip->write_cmd1(
            GetReadCommandForSize(sym.entries[lhstype].ssize),
            SREG_AX);
    }
    return 0;
}

// "var = expression"; lhs is the variable
int ParseAssignment_Assign(ccInternalList *targ, ccCompiledScript *scrip, ccInternalList const *lhs)
{
    int retval = ParseExpression(targ, scrip); // RHS of the assignment
    if (retval < 0) return retval;
    return AccessData_Assign(scrip, lhs->script, lhs->length);
}

// We compile something like "var += expression"
int ParseAssignment_MAssign(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol ass_symbol, ccInternalList const *lhs)
{
    // Parse RHS
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    scrip->push_reg(SREG_AX);
    AGS::Vartype rhsvartype = scrip->ax_vartype;

    // Parse LHS
    ValueLocation vloc;
    AGS::Vartype lhsvartype;
    retval = ParseAssignment_ReadLHSForModification(scrip, lhs, vloc, lhsvartype);
    if (retval < 0) return retval;

    // Use the operator on LHS and RHS
    int cpuOp = sym.entries[ass_symbol].ssize;
    retval = GetOperatorValidForVartype(lhsvartype, rhsvartype, cpuOp);
    if (retval < 0) return retval;
    scrip->pop_reg(SREG_BX);
    scrip->write_cmd2(cpuOp, SREG_AX, SREG_BX);

    if (kVL_mar_pointsto_value == vloc)
    {
        // write AX back to memory
        AGS::Symbol memwrite = GetWriteCommandForSize(sym.entries[lhsvartype].ssize);
        scrip->write_cmd1(memwrite, SREG_AX);
        return 0;
    }

    return ParseAssignment_Assign(targ, scrip, lhs);
}

// "var++" or "var--"
int ParseAssignment_SAssign(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol ass_symbol, ccInternalList const *lhs)
{
    ValueLocation vloc;
    AGS::Vartype lhsvartype;
    int retval = ParseAssignment_ReadLHSForModification(scrip, lhs, vloc, lhsvartype);
    if (retval < 0) return retval;

    // increment or decrement AX, using the correct bytecode
    int cpuOp = sym.entries[ass_symbol].ssize;
    retval = GetOperatorValidForVartype(lhsvartype, 0, cpuOp);
    if (retval < 0) return retval;
    scrip->write_cmd2(cpuOp, SREG_AX, 1);

    if (kVL_mar_pointsto_value == vloc)
    {
        // write AX back to memory
        AGS::Symbol memwrite = GetWriteCommandForSize(sym.entries[lhsvartype].ssize);
        scrip->write_cmd1(memwrite, SREG_AX);
        return 0;
    }
    
    return ParseAssignment_Assign(targ, scrip, lhs);
}

// We've read a variable or selector of a struct into symlist[], the last identifying component is in cursym.
// An assignment symbol is following. Compile the assignment.
int ParseAssignment(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol ass_symbol, ccInternalList const *lhs)
{
    switch (sym.get_type(ass_symbol))
    {
    default: // can't happen
        cc_error("Internal error: Illegal assignment symbol found");
        return -99;

    case kSYM_Assign:
        return ParseAssignment_Assign(targ, scrip, lhs);

    case kSYM_AssignMod:
        return ParseAssignment_MAssign(targ, scrip, ass_symbol, lhs);

    case kSYM_AssignSOp:
        return ParseAssignment_SAssign(targ, scrip, ass_symbol, lhs);
    }
}

// true if the symbol is "int" and the like.
inline bool is_primitive_vartype(AGS::Symbol symbl)
{
    return (symbl > 0 && symbl <= sym.getVoidSym());
}

int ParseVardecl_InitialValAssignment_ToLocal(ccInternalList *targ, ccCompiledScript *scrip, AGS::Vartype vartype)
{
    // Parse and compile the expression
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;

    // If we need a string object ptr but AX contains a normal string, convert AX
    ConvertAXIntoStringObject(scrip, vartype);

    // Check whether the types match
    retval = IsVartypeMismatch(scrip->ax_vartype, vartype, true);
    if (retval < 0) return retval;
    return 0;
}

int ParseVardecl_InitialValAssignment_ToGlobalFloat(ccInternalList *targ, bool is_neg, void *& initial_val_ptr)
{
    // initialize float
    if (sym.get_type(targ->peeknext()) != kSYM_LiteralFloat)
    {
        cc_error("Expected floating point value after '='");
        return -1;
    }

    float float_init_val = static_cast<float>(atof(sym.get_name_string(targ->getnext()).c_str()));
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

int ParseVardecl_InitialValAssignment_ToGlobalNonFloat(ccInternalList *targ, bool is_neg, void *& initial_val_ptr)
{
    // Initializer for an integer value
    int int_init_val;
    int retval = ParseLiteralOrConstvalue(targ->getnext(), int_init_val, is_neg, "Expected integer value after '='");
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
int ParseVardecl_InitialValAssignment_ToGlobal(ccInternalList *targ, AGS::Symbol varname, void *&initial_val_ptr)
{
    initial_val_ptr = nullptr;

    if (FlagIsSet(sym.get_flags(varname), kVTY_Pointer))
    {
        cc_error("Cannot assign an initial value to a global pointer");
        return -1;
    }

    if (FlagIsSet(sym.get_flags(varname), kVTY_DynArray))
    {
        cc_error("Cannot assign an initial value to a dynamic array");
        return -1;
    }

    // [fw] This check will probably fail to work for structs that contain just 1 int.
    if (sym.entries[varname].ssize > 4)
    {
        cc_error("Cannot initialize struct type");
        return -1;
    }

    // accept leading '-' if present
    bool is_neg = false;
    if (targ->peeknext() == sym.find("-"))
    {
        is_neg = true;
        targ->getnext();
    }

    // Do actual assignment
    if (sym.get_vartype(varname) == sym.getFloatSym())
        return ParseVardecl_InitialValAssignment_ToGlobalFloat(targ, is_neg, initial_val_ptr);
    return ParseVardecl_InitialValAssignment_ToGlobalNonFloat(targ, is_neg, initial_val_ptr);
}

// We have accepted something like "int var" and we are reading "= val"
int ParseVardecl_InitialValAssignment(ccInternalList *targ, ccCompiledScript *scrip, int next_type, Globalness isglobal, AGS::Symbol varname, void *&initial_val_ptr, FxFixupType &need_fixup)
{
    targ->getnext();  // skip the '='

    initial_val_ptr = nullptr; // there is no initial value
    if (isglobal == kGl_GlobalImport)
    {
        cc_error("Cannot set initial value of imported variables");
        return -1;
    }
    if (FlagIsSet(sym.entries[varname].vartype, kVTY_Array) &&
        !FlagIsSet(sym.entries[varname].vartype, kVTY_DynArray))
    {
        cc_error("Cannot assign a value to an array");
        return -1;
    }
    if (sym.entries[varname].vartype == sym.getOldStringSym())
    {
        cc_error("Cannot assign a value to a string, use StrCopy");
        return -1;
    }

    if (kGl_Local == isglobal)
    {
        // accept an expression of the appropriate type
        // This is compiled as an assignment, so there is no initial value to return here
        initial_val_ptr = nullptr;
        int retval = ParseVardecl_InitialValAssignment_ToLocal(targ, scrip, sym.get_vartype(varname));
        if (retval < 0) return retval;

        need_fixup = kFx_Function;   // code[fixup] += &code[0]
    }
    else // global var
    {
        // accept a literal or constant of the appropriate type
        int retval = ParseVardecl_InitialValAssignment_ToGlobal(targ, varname, initial_val_ptr);
        if (retval < 0) return retval;
    }

    return 0;
}

// Move variable information into the symbol table
void ParseVardecl_Var2SymTable(int var_name, Globalness is_global, bool is_pointer, int size_of_defn, AGS::Vartype vartype)
{
    SymbolTableEntry &entry = sym.entries[var_name];
    entry.extends = 0;
    entry.stype = (is_global == kGl_Local) ? kSYM_LocalVar : kSYM_GlobalVar;
    entry.ssize = size_of_defn;
    entry.arrsize = 0;
    entry.vartype = vartype;
    if (is_pointer)
        SetFlag(entry.vartype, kVTY_Pointer, true);
}

// we have accepted something like "int a" and we're expecting "["
int ParseVardecl_ArrayDecl(ccInternalList *targ, int var_name, int type_of_defn, int &size_of_defn, void *&initial_value_ptr)
{

    targ->getnext();  // skip the [
    SetFlag(sym.entries[var_name].vartype, kVTY_Array, true);

    if (sym.get_type(targ->peeknext()) == kSYM_CloseBracket)
    {
        // Dynamic array
        // this means var_name does not contain the first element of the array,
        // but points to it, instead.
        SetFlag(sym.entries[var_name].vartype, kVTY_DynArray, true);
        size_of_defn = SIZE_OF_POINTER;
        sym.entries[var_name].arrsize = 0;
        initial_value_ptr = calloc(1, size_of_defn);
        if (type_of_defn == sym.getOldStringSym())
        {
            cc_error("Dynamic arrays of old-style strings are not supported");
            return -1;
        }
    }
    else
    {
        int array_size;
        if (FlagIsSet(sym.get_flags(type_of_defn), kSFLG_HasDynArray))
        {
            cc_error("Cannot declare an array of a type containing dynamic array(s)");
            return -1;
        }

        AGS::Symbol nextt = targ->getnext();

        int retval = ParseLiteralOrConstvalue(nextt, array_size, false, "Array size must be constant value");
        if (retval < 0) return retval;

        if (array_size < 1)
        {
            cc_error("Array size must be >= 1");
            return -1;
        }

        size_of_defn = size_of_defn * array_size;
        initial_value_ptr = calloc(1, size_of_defn + 4);
        static_cast<int *>(initial_value_ptr)[0] = size_of_defn;
        sym.entries[var_name].arrsize = array_size;
    }

    if (sym.get_type(targ->getnext()) != kSYM_CloseBracket)
    {
        cc_error("Expected ']'");
        return -1;
    }

    return 0;
}

int ParseVardecl_StringDecl_GlobalNoImport(ccCompiledScript *scrip, void *&initial_value_ptr, FxFixupType &fixup_needed)
{
    // Reserve space for the string in globaldata; 
    // the initial value is the offset to the newly reserved space

    int offset_of_init_string = scrip->add_global(STRING_LENGTH, NULL);
    if (offset_of_init_string < 0)
    {
        cc_error("Out of memory");
        return -1;
    }

    initial_value_ptr = malloc(sizeof(int));
    if (!initial_value_ptr)
    {
        cc_error("Out of memory");
        return -1;
    }

    reinterpret_cast<int *>(initial_value_ptr)[0] = offset_of_init_string;
    fixup_needed = kFx_DataData;
    return 0;
}

int ParseVardecl_StringDecl_Local(ccCompiledScript *scrip, AGS::Symbol var_name, void *&initial_value_ptr)
{
    // Note: We can't use scrip->cur_sp since we don't know if we'll be in a nested function call at the time
    initial_value_ptr = nullptr;

    SetFlag(sym.entries[var_name].flags, kSFLG_StrBuffer, true); // Note in the symbol table that this var is a stringbuffer
    scrip->cur_sp += STRING_LENGTH; // reserve STRING_LENGTH bytes for the var on the stack

                                        // CX will contain the address of the new memory, which will be added to the stack
    scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_CX); // Copy current stack pointer to CX
    scrip->write_cmd2(SCMD_ADD, SREG_SP, STRING_LENGTH); // write code for reserving STRING LENGTH bytes 
    return 0;
}

int ParseVardecl_StringDecl(ccCompiledScript *scrip, AGS::Symbol var_name, Globalness is_global, void *&initial_value_ptr, FxFixupType &fixup_needed)
{
    
    initial_value_ptr = nullptr;
    fixup_needed = kFx_NoFixup;


    switch (is_global)
    {
    default: // This cannot happen
    {
        cc_error("Internal error: Wrong value for globalness");
        return 99;
    }

    case kGl_GlobalNoImport:
        return ParseVardecl_StringDecl_GlobalNoImport(scrip, initial_value_ptr, fixup_needed);

    case kGl_Local:
        return ParseVardecl_StringDecl_Local(scrip, var_name, initial_value_ptr);
    }
}

// We've parsed a definition of a local variable, provide the code for it
void ParseVardecl_CodeForDefnOfLocal(ccCompiledScript *scrip, int var_name, FxFixupType fixup_needed, int size_of_defn, void *initial_value)
{
    scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR); // MAR = SP

    // code for the initial assignment or the initialization to zeros
    if (fixup_needed == kFx_Function)
    {
        // expression worked out into ax
        if (FlagIsSet(sym.get_vartype(var_name), (kVTY_Pointer | kVTY_DynArray)))
            scrip->write_cmd1(SCMD_MEMINITPTR, SREG_AX);
        else
            scrip->write_cmd1(GetWriteCommandForSize(size_of_defn), SREG_AX);
    }
    else if (initial_value == NULL)
    {
        // local string, so the memory chunk pointer needs to be written
        scrip->write_cmd1(SCMD_MEMWRITE, SREG_CX); // memory[MAR] = CX
    }
    else
    {
        // local variable without initial value -- zero it
        scrip->write_cmd1(SCMD_ZEROMEMORY, size_of_defn); // memory[MAR+0, 1... ] = 0;
    }

    if (fixup_needed == kFx_DataData)
    {
        SetFlag(sym.entries[var_name].flags, kSFLG_StrBuffer, true);
        scrip->fixup_previous(kFx_Stack);
    }

    // Allocate space on the stack
    if (size_of_defn > 0)
    {
        scrip->cur_sp += size_of_defn;
        scrip->write_cmd2(SCMD_ADD, SREG_SP, size_of_defn);
    }
}


int ParseVardecl_CheckIllegalCombis(AGS::Vartype vartype, bool is_pointer, Globalness is_global)
{
    if (vartype == sym.getOldStringSym() && ccGetOption(SCOPT_OLDSTRINGS) == 0)
    {
        cc_error("Type 'string' is no longer supported; use String instead");
        return -1;
    }

    if (vartype == sym.getOldStringSym() && is_global == kGl_GlobalImport)
    {
        // cannot import, because string is really char *, and the pointer won't resolve properly
        cc_error("Cannot import string; use char[] instead");
        return -1;
    }


    if (FlagIsSet(sym.get_flags(vartype), kSFLG_Managed) && (!is_pointer) && (is_global != kGl_GlobalImport))
    {
        // managed structs must be allocated via ccRegisterObject,
        // and cannot be declared normally in the script (unless imported)
        cc_error("Cannot declare local instance of managed type");
        return -1;
    }

    if (vartype == sym.getVoidSym())
    {
        cc_error("'void' not a valid variable type");
        return -1;
    }

    if (is_pointer && !FlagIsSet(sym.get_flags(vartype), kSFLG_Managed))
    {
        // can only point to managed structs
        cc_error("Cannot declare pointer to non-managed type");
        return -1;
    }

    return 0;
}

// there was a forward declaration -- check that the real declaration matches it
int ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info)
{
    if (0 == known_info->stype)
        return 0; // We don't have any known info

    if (known_info->stype != this_entry->stype)
    {
        cc_error(
            "Type of this variable is declared as %s here, as %s elsewhere",
            sym.get_name_string(this_entry->stype).c_str(),
            sym.get_name_string(known_info->stype).c_str());
        return -1;
    }

    if ((known_info->flags & ~kSFLG_Imported) != (this_entry->flags & ~kSFLG_Imported))
    {
        cc_error("Qualifiers of this variable do not match prototype");
        return -1;
    }

    if (FlagIsSet(this_entry->vartype, kVTY_Array) && (known_info->arrsize != this_entry->arrsize))
    {
        cc_error(
            "Variable is declared as an array of size %d here, of size %d elsewhere",
            this_entry->arrsize, known_info->arrsize);
        return -1;
    }

    if (known_info->ssize != this_entry->ssize)
    {
        cc_error(
            "Size of this variable is %d here, %d declared elsewhere",
            this_entry->ssize, known_info->ssize);
        return -1;
    }

    return 0;
}

int ParseVardecl0(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol var_name,
    AGS::Vartype vartype,  // i.e. "void" or "int"
    SymbolType next_type, // type of the following symbol
    Globalness is_global,
    bool is_pointer,
    bool &another_var_follows,
    void *&initial_value_ptr)
{
    int size_of_defn = (is_pointer)? SIZE_OF_POINTER : sym.entries[vartype].ssize;

    // Enter the variable into the symbol table
    ParseVardecl_Var2SymTable(var_name, is_global, is_pointer, size_of_defn, vartype);

    if (kSYM_OpenBracket == next_type)
    {
        // Parse the bracketed expression; determine whether it is dynamic; if not, determine the size
        int retval = ParseVardecl_ArrayDecl(targ, var_name, vartype, size_of_defn, initial_value_ptr);
        if (retval < 0) return retval;

        next_type = sym.get_type(targ->peeknext());
    }
    else if (sym.getOldStringSym() != vartype)
    {
        initial_value_ptr = calloc(1, size_of_defn);
    }

    // initial assignment, i.e. a clause "= value" following the definition
    FxFixupType fixup_needed = kFx_NoFixup;
    if (kSYM_Assign == next_type)
    {
        if (initial_value_ptr)
            free(initial_value_ptr);
        int retval = ParseVardecl_InitialValAssignment(targ, scrip, next_type, is_global, var_name, initial_value_ptr, fixup_needed);
        if (retval < 0) return retval;
        next_type = sym.get_type(targ->peeknext());
    }

    switch (is_global)
    {
    default: // can't happen
        cc_error("Internal error: Wrong value of globalness");
        return -99;

    case kGl_GlobalImport:
        if (g_GIVM[var_name])
            break; // Skip this since the global non-import decl will come later
        if (vartype == sym.getOldStringSym())
        {
            cc_error("Cannot import string; use char[200] instead");
            return -1;
        }
        sym.entries[var_name].soffs = scrip->add_new_import(sym.get_name_string(var_name).c_str());
        SetFlag(sym.entries[var_name].flags, kSFLG_Imported, true);
        if (sym.entries[var_name].soffs == -1)
        {
            cc_error("Internal error: Import table overflow");
            return -1;
        }
        break;

    case kGl_GlobalNoImport:
        sym.entries[var_name].soffs = scrip->add_global(size_of_defn, reinterpret_cast<const char *>(initial_value_ptr));
        if (sym.entries[var_name].soffs < 0)
            return -1;
        if (fixup_needed == kFx_DataData)
            scrip->add_fixup(sym.entries[var_name].soffs, kFx_DataData);
        break;

    case kGl_Local:
        sym.entries[var_name].soffs = scrip->cur_sp;

        // Output the code for defining the local and initializing it
        ParseVardecl_CodeForDefnOfLocal(scrip, var_name, fixup_needed, size_of_defn, initial_value_ptr);
        break;
    }
    return 0;
}

// wrapper around ParseVardecl0() to prevent memory leakage
inline int ParseVardecl(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol var_name,
    AGS::Vartype vartype,  // i.e. "void" or "int"
    SymbolType next_type, // type of the following symbol
    Globalness is_global,
    bool is_pointer,
    bool &another_var_follows)
{
    int retval = ParseVardecl_CheckIllegalCombis(vartype, is_pointer, is_global);
    if (retval < 0) return retval;


    SymbolTableEntry known_info;
    if (sym.get_type(var_name) != 0)
        CopyKnownSymInfo(sym.entries[var_name], known_info);

    void *initial_value_ptr = nullptr;

    retval = ParseVardecl0(targ, scrip, var_name, vartype, next_type, is_global, is_pointer, another_var_follows, initial_value_ptr);

    if (initial_value_ptr != nullptr)
        free(initial_value_ptr);

    if (retval < 0) return retval;

    retval = ParseVardecl_CheckThatKnownInfoMatches(&sym.entries[var_name], &known_info);
    if (retval < 0) return retval;

    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    another_var_follows = false;
    next_type = sym.get_type(targ->peeknext());
    if (next_type == kSYM_Comma)
    {
        targ->getnext();  // Eat ','
        another_var_follows = true;
        return 0;
    }
    if (next_type == kSYM_Semicolon)
        return 0;

    cc_error("Expected ',' or ';' instead of '%s'", sym.get_name_string(targ->peeknext()).c_str());
    return -1;
}

void ParseOpenbrace_FuncBody(ccCompiledScript *scrip, AGS::Symbol name_of_func, int struct_of_func, bool is_noloopcheck, AGS::NestingStack *nesting_stack)
{
    // write base address of function for any relocation needed later
    scrip->write_cmd1(SCMD_THISBASE, scrip->codesize);
    if (is_noloopcheck)
        scrip->write_cmd0(SCMD_LOOPCHECKOFF);

    // loop through all parameters and check whether they are pointers
    // the first entry is the return value, so skip that
    const size_t num_args = sym.entries[name_of_func].get_num_args();
    for (size_t pa = 1; pa <= num_args; pa++)
    {
        if (sym.entries[name_of_func].funcparamtypes[pa] & (kVTY_Pointer | kVTY_DynArray))
        {
            // pointers are passed in on the stack with the real
            // memory address -- convert this to the mem handle
            // since params are pushed backwards, this works
            // the +1 is to deal with the return address
            scrip->write_cmd1(SCMD_LOADSPOFFS, 4 * (pa + 1));
            scrip->write_cmd1(SCMD_MEMREAD, SREG_AX);
            scrip->write_cmd1(SCMD_MEMINITPTR, SREG_AX);
        }
    }

    SymbolTableEntry &this_entry = sym.entries[sym.getThisSym()];
    this_entry.vartype = 0;
    if (struct_of_func > 0 && !FlagIsSet(sym.get_flags(name_of_func), kSFLG_Static))
    {
        // Declare the "this" pointer (allocated memory for it will never be used)
        this_entry.stype = kSYM_LocalVar;
        this_entry.ssize = SIZE_OF_POINTER;
        // Don't declare this as a kVTY_Pointer to prevent it being dereferenced twice
        this_entry.vartype = struct_of_func;
        this_entry.sscope = nesting_stack->Depth() - 1;
        this_entry.flags = kSFLG_Readonly | kSFLG_Accessed;
        // Allocate 4 unused empty bytes on stack for the "this" pointer
        this_entry.soffs = scrip->cur_sp;
        scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR);
        scrip->write_cmd2(SCMD_WRITELIT, SIZE_OF_POINTER, 0); 
        scrip->cur_sp += SIZE_OF_POINTER;
        scrip->write_cmd2(SCMD_ADD, SREG_SP, SIZE_OF_POINTER);
    }
}

int ParseOpenbrace(
    ccCompiledScript *scrip,
    AGS::NestingStack *nesting_stack,
    AGS::Symbol name_of_current_func,
    AGS::Symbol struct_of_current_func,
    bool is_noloopcheck)
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
        ParseOpenbrace_FuncBody(scrip, name_of_current_func, struct_of_current_func, is_noloopcheck, nesting_stack);
    }

    return 0;
}


int ParseClosebrace(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack, AGS::Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func)
{
    size_t nesting_level = nesting_stack->Depth() - 1;

    if (nesting_level == 0 || nesting_stack->IsUnbraced())
    {
        cc_error("Unexpected '}'");
        return -1;
    }

    if (nesting_level == 1) // Code exec reaches end of a function

    {
        // Emit code that returns 0
        if(sym.getVoidSym() != sym.entries[name_of_current_func].funcparamtypes.at(0) )
            scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    }

    FreePointersOfLocals(scrip, nesting_level - 1, name_of_current_func);
    int totalsub = StacksizeOfLocals(nesting_level - 1);
    if (totalsub > 0)
    {
        // Reduce the "high point" of the stack appropriately, 
        // write code for popping the bytes from the stack
        scrip->cur_sp -= totalsub;
        scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    }

    // All the local variables that were defined within the braces become invalid
    RemoveLocalsFromSymtable(nesting_level);

    if (nesting_level == 1)
    {
        g_FCM.SetFuncExitJumppoint(scrip, name_of_current_func, scrip->codesize);
        // We've just finished the body of the current function.
        name_of_current_func = -1;
        struct_of_current_func = -1;

        // Write code to return from the function.
        // This pops the return address from the stack, 
        // so adjust the "high point" of stack allocation appropriately
        scrip->write_cmd0(SCMD_RET);
        scrip->cur_sp -= 4;

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
        retval = DealWithEndOfDo(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case AGS::NestingStack::kNT_BracedElse:
    case AGS::NestingStack::kNT_BracedThen:
    {
        bool if_turned_into_else;
        int retval = DealWithEndOfElse(targ, scrip, nesting_stack, if_turned_into_else);
        if (retval < 0) return retval;
        if (if_turned_into_else)
            return 0;
        break;
    }
    
    case AGS::NestingStack::kNT_Switch:
        retval = DealWithEndOfSwitch(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;
    }

    // loop round doing all the end of elses, but break once an IF
    // has been turned into an ELSE
    while (nesting_stack->IsUnbraced())
    {
        if (nesting_stack->Type() == AGS::NestingStack::kNT_UnbracedDo)
        {
            int retval = DealWithEndOfDo(targ, scrip, nesting_stack);
            if (retval < 0) return retval;
        }

        bool if_turned_into_else;
        int retval = DealWithEndOfElse(targ, scrip, nesting_stack, if_turned_into_else);
        if (retval < 0) return retval;
        if (if_turned_into_else)
            break;
    }
    return 0;
}

void ParseStruct_SetTypeInSymboltable(AGS::Symbol stname, TypeQualifierSet tqs)
{
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(stname);

    entry.extends = 0;
    entry.stype = kSYM_Vartype;
    SetFlag(entry.flags, kSFLG_StructType, true);
    entry.ssize = 0;

    if (FlagIsSet(tqs, kTQ_Managed))
        SetFlag(entry.flags, kSFLG_Managed, true);

    if (FlagIsSet(tqs, kTQ_Builtin))
        SetFlag(entry.flags, kSFLG_Builtin, true);

    if (FlagIsSet(tqs, kTQ_Autoptr))
        SetFlag(entry.flags, kSFLG_Autoptr, true);
}


// We're processing the extends clause of a struct. Copy over all the parent elements
// except for functions and attributes into the current struct.
int ParseStruct_Extends_CopyParentComponents(AGS::Symbol parent, AGS::Symbol stname)
{
    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        SymbolTableEntry &entry = sym.entries[entries_idx];
        if (!FlagIsSet(entry.flags, kSFLG_StructMember))
            continue;
        if (entry.extends != parent)
            continue;
        if (kSYM_Function == entry.stype || kSYM_Attribute == entry.stype)
            continue;
        int const compo_start = entry.sname.rfind(':');
        if (std::string::npos == compo_start)
        {
            cc_error("Internal error: Component '%s' of '%s' does not have a struct prefix",
                entry.sname.c_str(), sym.get_name_string(parent).c_str());
            return -1;
        }
        std::string const compo_name_str = entry.sname.substr(compo_start + 1);
        AGS::Symbol const compocorename = sym.find_or_add(compo_name_str.c_str());
        AGS::Symbol const compo = MangleStructAndComponent(stname, compocorename);
        std::string const sname = sym.entries[compo].sname;
        entry.CopyTo(sym.entries[compo]);
        sym.entries[compo].sname = sname;
    }
    return 0;
}

// We have accepted something like "struct foo" and are waiting for "extends"
int ParseStruct_ExtendsClause(ccInternalList *targ, AGS::Symbol stname, AGS::Symbol &parent, size_t &size_so_far)
{
    targ->getnext(); // Eat "extends"
    parent = targ->getnext(); // name of the extended struct

    if (kPP_PreAnalyze == g_PP)
        return 0; // No further analysis necessary in first phase

    if (kSYM_Vartype != sym.get_type(parent))
    {
        cc_error("Expected a struct type here");
        return -1;
    }
    SymbolTableEntry &struct_entry = sym.entries[stname];
    SymbolTableEntry const &extends_entry = sym.entries[parent];

    if (!FlagIsSet(extends_entry.flags, kSFLG_StructType))
    {
        cc_error("Must extend a struct type");
        return -1;
    }
    if (!FlagIsSet(extends_entry.flags, kSFLG_Managed) && FlagIsSet(struct_entry.flags, kSFLG_Managed))
    {
        cc_error("Managed struct cannot extend the unmanaged struct '%s'", sym.get_name_string(parent).c_str());
        return -1;
    }
    if (FlagIsSet(extends_entry.flags, kSFLG_Managed) && !FlagIsSet(struct_entry.flags, kSFLG_Managed))
    {
        cc_error("Unmanaged struct cannot extend the managed struct '%s'", sym.get_name_string(parent).c_str());
        return -1;
    }
    if (FlagIsSet(extends_entry.flags, kSFLG_Builtin) && !FlagIsSet(struct_entry.flags, kSFLG_Builtin))
    {
        cc_error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", sym.get_name_string(parent).c_str());
        return -1;
    }
    size_so_far = static_cast<size_t>(extends_entry.ssize);
    struct_entry.extends = parent;

    return ParseStruct_Extends_CopyParentComponents(parent, stname);
}


void ParseStruct_MemberQualifiers(ccInternalList *targ, AGS::Symbol &cursym, TypeQualifierSet &tqs)
{
    tqs = 0;
    while (true)
    {
        cursym = targ->getnext();

        switch (sym.get_type(cursym))
        {
        default: break;
        case kSYM_Attribute:      SetFlag(tqs, kTQ_Attribute, true); continue;
        case kSYM_Import:         SetFlag(tqs, kTQ_ImportStd, true);   continue;
        case kSYM_Protected:      SetFlag(tqs, kTQ_Protected, true); continue;
        case kSYM_ReadOnly:       SetFlag(tqs, kTQ_Readonly, true);  continue;
        case kSYM_Static:         SetFlag(tqs, kTQ_Static, true);    continue;
        case kSYM_WriteProtected: SetFlag(tqs, kTQ_Writeprotected, true);  continue;
        }
        break;
    };

    return;
}

int ParseStruct_CheckComponentVartype(ccInternalList *targ, int stname, AGS::Symbol vartype, bool member_is_pointer, bool member_is_import)
{
    if ((vartype == stname) && (!member_is_pointer))
    {
        // cannot do "struct A { A a; }", this struct would be infinitely large
        cc_error("Struct '%s' can't be a member of itself", sym.get_name_string(vartype).c_str());
        return -1;
    }
    
    SymbolType const vartype_type = sym.get_type(vartype);
    if (vartype_type == kSYM_NoType)
    {
        cc_error(
            "Type '%s' is undefined",
            sym.get_vartype_name_string(vartype).c_str());
        return -1;
    }
    if (kSYM_Vartype != vartype_type && kSYM_UndefinedStruct != vartype_type)
    {
        cc_error(
            "'%s' should be a typename but is already in use differently",
            sym.get_vartype_name_string(vartype).c_str());
        return -1;
    }
    if (kSYM_UndefinedStruct == vartype_type && !member_is_pointer)
    {
        cc_error("You can only declare a pointer to a struct that hasn't been completely defined yet");
        return -1;
    }
    
    if (vartype == sym.getOldStringSym()) // [fw] Where's the problem?
    {
        cc_error("'string' not allowed inside a struct");
        return -1;
    }

    AGS::Flags const vartype_flags = sym.get_flags(vartype);

    if (FlagIsSet(vartype_flags, kSFLG_StructType) && !member_is_pointer)
    {
        if (FlagIsSet(vartype_flags, kSFLG_Builtin))
        {
            cc_error("You can only declare a pointer to a builtin type");
            return -1;
        }
        if (FlagIsSet(vartype_flags, kSFLG_Managed))
        {
            cc_error("You can only declare a pointer to a managed type");
            return -1;
        }
    }

    if (!FlagIsSet(vartype_flags, kSFLG_Managed) && member_is_pointer)
    {
        cc_error("Cannot declare a pointer to non-managed type");
        return -1;
    }

    return 0;
}

// check that we haven't extended a struct that already contains a member with the same name
int ParseStruct_CheckForCompoInAncester(AGS::Symbol orig, AGS::Symbol compo, AGS::Symbol act_struct)
{
    if (act_struct <= 0)
        return 0;
    AGS::Symbol const member = MangleStructAndComponent(act_struct, compo);
    if (kSYM_NoType != sym.get_type(member))
    {
        cc_error(
            "The struct '%s' extends '%s', and '%s::%s' is already defined",
            sym.get_name_string(orig).c_str(),
            sym.get_name_string(act_struct).c_str(),
            sym.get_name_string(act_struct).c_str(),
            sym.get_name_string(compo).c_str());
        return -1;
    }

    return ParseStruct_CheckForCompoInAncester(orig, compo, sym.entries[act_struct].extends);
}

int ParseStruct_Function(ccInternalList *targ, ccCompiledScript *scrip, AGS::TypeQualifierSet tqs, AGS::Vartype curtype, AGS::Symbol stname, AGS::Symbol vname, AGS::Symbol name_of_current_func, bool type_is_pointer, bool isDynamicArray)
{
    if (FlagIsSet(tqs, kTQ_Writeprotected))
    {
        cc_error("'writeprotected' does not apply to functions");
        return -1;
    }

    bool body_follows;
    int retval = ParseFuncdecl(
        targ, scrip, vname, curtype, type_is_pointer, isDynamicArray, tqs, stname, body_follows);
    if (retval < 0) return retval;
    if (body_follows)
    {
        cc_error("Cannot declare a function body within a struct definition");
        return -1;
    }
    if (kSYM_Semicolon != sym.get_type(targ->peeknext()))
    {
        cc_error("Expected ';'");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected))
        SetFlag(sym.entries[vname].flags, kSFLG_Protected, true);
    return 0;
}

int ParseStruct_CheckAttributeFunc(SymbolTableEntry &entry, bool is_setter, bool is_indexed, AGS::Vartype vartype)
{
    size_t const sscope_wanted = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);
    if (entry.sscope != sscope_wanted)
    {
        cc_error(
            "The attribute function '%s' should have %d parameter(s) but is declared with %d parameter(s) instead",
            entry.sname.c_str(), sscope_wanted, entry.sscope);
        return -1;
    }
    AGS::Vartype const ret_vartype = is_setter ? sym.getVoidSym() : vartype;
    if (!entry.funcparamtypes[0] != ret_vartype)
    {
        cc_error(
            "The attribute function '%s' must return type '%s' but returns '%s' instead",
            entry.sname.c_str(),
            sym.get_name_string(ret_vartype).c_str(),
            sym.get_vartype_name_string(entry.funcparamtypes[0]).c_str());
        return -1;
    }
    size_t p_idx = 1;
    if (is_indexed && entry.funcparamtypes[p_idx] != sym.getIntSym())
    {
        cc_error(
            "Parameter #%d of attribute function '%s' must have type integer but doesn't.",
            p_idx, entry.sname.c_str());
        return -1;
    }
    p_idx++;
    if (is_setter && entry.funcparamtypes[p_idx] != vartype)
    {
        cc_error(
            "Parameter #d of attribute function '%s' must have type '%s'",
            p_idx, entry.sname.c_str(), sym.get_name_string(vartype).c_str());
        return -1;
    }
    
    return 0;
}

int ParseStruct_EnterAttributeFunc(ccCompiledScript *scrip, SymbolTableEntry &entry, bool is_setter, bool is_indexed, AGS::Vartype vartype)
{
    entry.stype = kSYM_Function;
    SetFlag(entry.flags, kSFLG_Imported, true);
    entry.soffs = g_ImportMgr.FindOrAdd(entry.sname);
    char  *num_param_suffix;
    if (is_setter)
        num_param_suffix = (is_indexed ? "^2" : "^1");
    else // getter
        num_param_suffix = (is_indexed ? "^1" : "^0");
    strcat(scrip->imports[entry.soffs], num_param_suffix);

    AGS::Vartype const retvartype = entry.funcparamtypes[0] =
        is_setter ? sym.getVoidSym() : vartype;
    entry.ssize = sym.entries[retvartype & kVTY_FlagMask].ssize;
    entry.sscope = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);

    entry.funcparamtypes.resize(entry.sscope + 1);

    size_t p_idx = 1;
    if (is_indexed)
        entry.funcparamtypes[p_idx++] = sym.getIntSym();
    if (is_setter)
        entry.funcparamtypes[p_idx] = vartype;
    entry.funcParamHasDefaultValues.assign(entry.funcparamtypes.size(), false);
    entry.funcParamDefaultValues.assign(entry.funcparamtypes.size(), 0);

    return 0;
}

// We are processing an attribute.
// This corresponds to a getter func and a setter func, declare one of them
int ParseStruct_DeclareAttributeFunc(ccCompiledScript *scrip, AGS::Symbol func, bool is_setter, bool is_indexed, AGS::Vartype vartype)
{
    SymbolTableEntry &entry = sym.entries[func];
    if (kSYM_Function != entry.stype && kSYM_NoType != entry.stype)
    {
        cc_error(
            "Attribute uses '%s' as a function, this clashes with a declaration elsewhere",
            entry.sname.c_str());
        return -1;
    }

    if (kSYM_Function == entry.stype) // func has already been declared
        return ParseStruct_CheckAttributeFunc(entry, is_setter, is_indexed, vartype);

    return ParseStruct_EnterAttributeFunc(scrip, entry, is_setter, is_indexed, vartype);
}

// We're in a struct declaration, parsing a struct attribute
int ParseStruct_Attribute(ccInternalList *targ, ccCompiledScript *scrip, AGS::TypeQualifierSet tqs, AGS::Symbol stname, AGS::Symbol vname)
{
    bool attribute_is_indexed = false;

    if (kSYM_OpenBracket == sym.get_type(targ->peeknext()))
    {
        targ->getnext();
        if (kSYM_CloseBracket != sym.get_type(targ->getnext()))
        {
            cc_error("Cannot specify array size for attribute");
            return -1;
        }
    }
    if (kPP_PreAnalyze == g_PP)
        return 0;

    sym.entries[vname].stype = kSYM_Attribute;
    if (attribute_is_indexed)
    {
        SetFlag(sym.entries[vname].vartype, kVTY_Array, true);
        sym.entries[vname].arrsize = 0;
    }

    // Declare attribute get func, e.g. get_ATTRIB()
    AGS::Symbol attrib_func = -1;
    bool func_is_setter = false;
    int retval = ConstructAttributeFuncName(vname, func_is_setter, attribute_is_indexed, attrib_func);
    if (retval < 0) return retval;
    retval = ParseStruct_DeclareAttributeFunc(scrip, MangleStructAndComponent(stname, attrib_func), func_is_setter, attribute_is_indexed, sym.get_vartype(vname));
    if (retval < 0) return retval;

    if (FlagIsSet(tqs, kTQ_Readonly))
        return 0;

    // Declare attribute set func, e.g. set_ATTRIB(value)
    func_is_setter = true;
    retval = ConstructAttributeFuncName(vname, func_is_setter, attribute_is_indexed, attrib_func);
    if (retval < 0) return retval;
    return ParseStruct_DeclareAttributeFunc(scrip, MangleStructAndComponent(stname, attrib_func), func_is_setter, attribute_is_indexed, sym.get_vartype(vname));
}

// We're inside a struct decl, parsing an array var.
int ParseStruct_Array(ccInternalList *targ, AGS::Symbol stname, AGS::Symbol vname, size_t &size_so_far)
{
    targ->getnext(); // Eat '['

    if (kPP_PreAnalyze == g_PP)
    {
        // Skip the [...]
        const AGS::Symbol stoplist[] = { 0 };
        SkipTo(targ, stoplist, 0);
        targ->getnext(); // Eat ']'
        return 0;
    }

    int array_size;

    AGS::Symbol const nextt = targ->getnext();
    if (sym.get_type(nextt) == kSYM_CloseBracket)
    {
        if (FlagIsSet(sym.get_flags(stname), kSFLG_Managed))
        {
            cc_error("Member variable of managed struct cannot be dynamic array");
            return -1;
        }
        SetFlag(sym.entries[stname].flags, kSFLG_HasDynArray, true);
        SetFlag(sym.entries[vname].vartype, kVTY_DynArray, true);
        array_size = 0;
        size_so_far += SIZE_OF_POINTER;
    }
    else
    {
        if (ParseLiteralOrConstvalue(nextt, array_size, false, "Array size must be constant value") < 0)
            return -1;

        if (array_size < 1)
        {
            cc_error("Array size cannot be less than 1");
            return -1;
        }

        size_so_far += array_size * sym.entries[vname].ssize;

        if (sym.get_type(targ->getnext()) != kSYM_CloseBracket)
        {
            cc_error("Expected ']'");
            return -1;
        }
    }
    SetFlag(sym.entries[vname].vartype, kVTY_Array, true);
    sym.entries[vname].arrsize = array_size;
    return 0;
}

// We're inside a struct decl, processing a member variable
int ParseStruct_VariableOrAttribute(ccInternalList *targ, ccCompiledScript *scrip, AGS::TypeQualifierSet tqs, AGS::Vartype curtype, bool type_is_pointer, AGS::Symbol stname, AGS::Symbol vname, size_t &size_so_far)
{
    if (kPP_Main == g_PP)
    {
        SymbolTableEntry &entry = sym.entries[vname];
        entry.stype = kSYM_StructComponent;
        entry.extends = stname;  // save which struct it belongs to
        entry.ssize = sym.entries[curtype].ssize;
        entry.soffs = size_so_far;
        entry.vartype = curtype;
        if (type_is_pointer)
        {
            SetFlag(entry.vartype, kVTY_Pointer, true);
            sym.entries[vname].ssize = 4;
        }
        if (FlagIsSet(tqs, kTQ_Readonly))
            SetFlag(entry.flags, kSFLG_Readonly, true);
        if (FlagIsSet(tqs, kTQ_Attribute))
            SetFlag(entry.flags, kSFLG_Attribute, true);
        
        if (FlagIsSet(tqs, kTQ_Static))
            SetFlag(sym.entries[vname].flags, kSFLG_Static, true);
        if (FlagIsSet(tqs, kTQ_Protected))
            SetFlag(sym.entries[vname].flags, kSFLG_Protected, true);
        if (FlagIsSet(tqs, kTQ_Writeprotected))
            SetFlag(sym.entries[vname].flags, kSFLG_WriteProtected, true);
    }

    if (FlagIsSet(tqs, kTQ_Attribute))
        return ParseStruct_Attribute(targ, scrip, tqs, stname, vname);

    if (FlagIsSet(tqs, kTQ_Import))
    {
        // member variable cannot be an import
        cc_error("Can't import struct component variables; import the whole struct instead");
        return -1;
    }

    if (sym.get_type(targ->peeknext()) == kSYM_OpenBracket)
        return ParseStruct_Array(targ, stname, vname, size_so_far);

    size_so_far += sym.entries[vname].ssize;
    return 0;
}

// We have accepted something like "struct foo extends bar { const int".
// We're waiting for the name of the member.
int ParseStruct_MemberDefnVarOrFuncOrArray(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol parent,
    AGS::Symbol stname,
    AGS::Symbol current_func,
    TypeQualifierSet tqs,
    AGS::Vartype curtype,
    bool type_is_pointer,
    size_t &size_so_far)
{
    AGS::Symbol cursym = targ->getnext(); // normally variable name, array name, or function name, but can be [ too
    
    // Check whether "[]" is behind the type.  "struct foo { int [] bar; }" 
    bool isDynamicArray = false;
    if (sym.get_type(cursym) == kSYM_OpenBracket && sym.get_type(targ->peeknext()) == kSYM_CloseBracket)
    {
        isDynamicArray = true;
        if (kSYM_CloseBracket != sym.get_type(targ->getnext()))
        {
            cc_error("Can only use '[]' directly after a type");
            return -1;
        }
        cursym = targ->getnext();
    }

    AGS::Symbol const component = cursym;
    AGS::Symbol const mangled_name = MangleStructAndComponent(stname, component);
    if (kSYM_Vartype == sym.get_type(component) && is_primitive_vartype(component))
    {
        cc_error("Can't use primitive type '%s' as a struct component");
        return -1;
    }

    bool const is_function = sym.get_type(targ->peeknext()) == kSYM_OpenParenthesis;

    if (type_is_pointer && FlagIsSet(sym.get_flags(stname), kSFLG_Managed) && !is_function)
    {
        cc_error("Member variable of a managed struct cannot be a pointer");
        return -1;
    }

    if (kPP_Main == g_PP && !is_function)
    {
        if (sym.get_type(mangled_name) != 0)
        {
            cc_error(
                "'%s' is already defined",
                sym.get_name_string(mangled_name).c_str());
            return -1;
        }
    
        // Mustn't be in any ancester
        int retval = ParseStruct_CheckForCompoInAncester(stname, component, parent);
        if (retval < 0) return retval;
    }

    // All struct members get this flag, even functions
    if (kPP_Main == g_PP)
        SetFlag(sym.entries[mangled_name].flags, kSFLG_StructMember, true);

    if (is_function)
    {
        if (current_func > 0)
        {
            cc_error("Cannot declare struct member function inside a function body");
            return -1;
        }
        return ParseStruct_Function(targ, scrip, tqs, curtype, stname, mangled_name, current_func, type_is_pointer, isDynamicArray);
    }

    if (isDynamicArray)
    {
        // "int []" etc. only allowed as a function return declaration
        cc_error("Expected '('");
        return -1;
    }

    return ParseStruct_VariableOrAttribute(targ, scrip, tqs, curtype, type_is_pointer, stname, mangled_name, size_so_far);
}

int ParseStruct_MemberStmt(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol stname,
    AGS::Symbol name_of_current_func,
    AGS::Symbol parent,
    size_t &size_so_far)
{
    AGS::Symbol vartype; // the type of the current members being defined, given as a symbol

    // parse qualifiers of the member ("import" etc.), set booleans accordingly
    TypeQualifierSet tqs = 0;
    ParseStruct_MemberQualifiers(targ, vartype, tqs);
    if (FlagIsSet(tqs, kTQ_Protected) && FlagIsSet(tqs, kTQ_Writeprotected))
    {
        cc_error("Field cannot be both protected and write-protected.");
        return -1;
    }

    // vartype can now be: vartypename or vartypename *

    // A member defn. is a pointer if it is AUTOPOINTER or it has an explicit "*"
    bool type_is_pointer = false;
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(vartype);
    if (FlagIsSet(entry.flags, kSFLG_Autoptr))
    {
        type_is_pointer = true;
    }
    else if (sym.getPointerSym() == targ->peeknext())
    {
        type_is_pointer = true;
        targ->getnext();
    }

    // Certain types of members are not allowed in structs; check this
    if (kPP_Main == g_PP)
    {
        int retval = ParseStruct_CheckComponentVartype(targ, stname, vartype, type_is_pointer, FlagIsSet(tqs, kTQ_Import));
        if (retval < 0) return retval;
    }

    // run through all variables declared on this member defn.
    while (true)
    {
        int retval = ParseStruct_MemberDefnVarOrFuncOrArray(
            targ, scrip, parent, stname, name_of_current_func, tqs, vartype, type_is_pointer, size_so_far);
        if (retval < 0) return retval;

        if (sym.get_type(targ->peeknext()) == kSYM_Comma)
        {
            targ->getnext(); // Eat ','
            continue;
        }
        break;
    }

    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    return 0;
}

int ParseVartype0(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol type_of_defn, AGS::NestingStack *nesting_stack, TypeQualifierSet tqs, AGS::Symbol &name_of_current_func, AGS::Symbol &struct_of_current_func, bool &noloopcheck_is_set);

// Handle a "struct" definition clause
int ParseStruct(ccInternalList *targ, ccCompiledScript *scrip, TypeQualifierSet tqs, AGS::NestingStack &nesting_stack, AGS::Symbol name_of_current_func, AGS::Symbol struct_of_current_func)
{
    // get token for name of struct
    AGS::Symbol const stname = targ->getnext();

    if ((sym.get_type(stname) != 0) &&
        (sym.get_type(stname) != kSYM_UndefinedStruct))
    {
        cc_error("'%s' is already defined", sym.get_name_string(stname).c_str());
        return -1;
    }

    ParseStruct_SetTypeInSymboltable(stname, tqs);

    // Declare the struct type that implements new strings
    if (FlagIsSet(tqs, kTQ_Stringstruct))
    {
        if (sym.getStringStructSym() > 0 && stname != sym.getStringStructSym())
        {
            cc_error("The stringstruct type is already defined to be %s", sym.get_name_string(sym.getStringStructSym()).c_str());
            return -1;
        }
        sym.setStringStructSym(stname);
    }

    size_t size_so_far = 0; // Will sum up the size of the struct

    // If the struct extends another struct, the token of the other struct's name
    AGS::Symbol parent = 0;

    // optional "extends" clause
    if (sym.get_type(targ->peeknext()) == kSYM_Extends)
        ParseStruct_ExtendsClause(targ, stname, parent, size_so_far);

    // forward-declaration of struct type
    if (sym.get_type(targ->peeknext()) == kSYM_Semicolon)
    {
        targ->getnext(); // Eat ';'
        SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(stname);
        entry.stype = kSYM_UndefinedStruct;
        entry.ssize = 0;
        return 0;
    }

    // So we are in the declaration of the components
    if (sym.get_type(targ->getnext()) != kSYM_OpenBrace)
    {
        cc_error("Expected '{'");
        return -1;
    }

    // Process every member of the struct in turn
    while (sym.get_type(targ->peeknext()) != kSYM_CloseBrace)
    {
        int retval = ParseStruct_MemberStmt(targ, scrip, stname, name_of_current_func, parent, size_so_far);
        if (retval < 0) return retval;
    }

    if (kPP_Main == g_PP)
    {
        // align struct on 4-byte boundary in keeping with compiler
        if ((size_so_far % 4) != 0)
            size_so_far += 4 - (size_so_far % 4);
        sym.entries[stname].ssize = size_so_far;
    }

    targ->getnext(); // Eat '}'

    SymbolType const type_of_next = sym.get_type(targ->peeknext());
    if (kSYM_Semicolon == type_of_next)
    {
        targ->getnext(); // Eat ';'
        return 0;
    }
    if (0 != type_of_next)
    {
        cc_error("Expected ';' or a variable name (did you forget ';' after the last struct definition?)");
        return -1;
    }

    bool dummy;
    return ParseVartype0(targ, scrip, stname, &nesting_stack, tqs, name_of_current_func, struct_of_current_func, dummy);
}

// We've accepted something like "enum foo { bar"; '=' follows
int ParseEnum_AssignedValue(ccInternalList *targ, int &currentValue)
{
    targ->getnext(); // eat "="

    // Get the value of the item
    AGS::Symbol item_value = targ->getnext(); // may be '-', too
    bool is_neg = false;
    if (item_value == sym.find("-"))
    {
        is_neg = true;
        item_value = targ->getnext();
    }

    return ParseLiteralOrConstvalue(item_value, currentValue, is_neg, "Expected integer or integer constant after '='");
}

void ParseEnum_Item2Symtable(AGS::Symbol enum_name, AGS::Symbol item_name, int currentValue)
{
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(item_name);

    entry.stype = kSYM_Constant;
    entry.ssize = SIZE_OF_INT;
    entry.arrsize = 0;
    entry.vartype = enum_name;
    entry.sscope = 0;
    entry.flags = kSFLG_Readonly;
    // soffs is unused for a constant, so in a gratuitous hack we use it to store the enum's value
    entry.soffs = currentValue;
}

int ParseEnum_Name2Symtable(AGS::Symbol enumName)
{
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(enumName);

    if (0 != entry.stype)
    {
        cc_error("'%s' is already defined", sym.get_name_string(enumName).c_str());
        return -1;
    }

    entry.stype = kSYM_Vartype;
    entry.ssize = SIZE_OF_INT;
    entry.vartype = sym.getIntSym();

    return 0;
}

// enum EnumName { value1, value2 }
int ParseEnum0(ccInternalList *targ)
{
    // Get name of the enum, enter it into the symbol table
    AGS::Symbol enum_name = targ->getnext();
    int retval = ParseEnum_Name2Symtable(enum_name);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != kSYM_OpenBrace)
    {
        cc_error("Expected '{'");
        return -1;
    }

    int currentValue = 0;

    while (true)
    {
        AGS::Symbol item_name = targ->getnext();
        if (sym.get_type(item_name) == kSYM_CloseBrace)
            break; // item list empty or ends with trailing ','

        if (sym.get_type(item_name) == kSYM_Const)  // will only test properly in main phase, but that's OK
        {
            cc_error("'%s' is already defined as a constant or enum value", sym.get_name_string(item_name).c_str());
            return -1;
        }
        if (sym.get_type(item_name) != 0)  // will only test properly in main phase, but that's OK
        {
            cc_error("Expected '}' or an unused identifier, found '%s' instead", sym.get_name_string(item_name).c_str());
            return -1;
        }

        // increment the value of the enum entry
        currentValue++;

        SymbolType type_of_next = sym.get_type(targ->peeknext());
        if (type_of_next != kSYM_Assign && type_of_next != kSYM_Comma && type_of_next != kSYM_CloseBrace)
        {
            cc_error("Expected '=' or ',' or '}'");
            return -1;
        }

        if (type_of_next == kSYM_Assign)
        {
            // the value of this entry is specified explicitly
            int retval = ParseEnum_AssignedValue(targ, currentValue);
            if (retval < 0) return retval;
        }

        // Enter this enum item as a constant int into the sym table
        ParseEnum_Item2Symtable(enum_name, item_name, currentValue);

        AGS::Symbol comma_or_brace = targ->getnext();
        if (sym.get_type(comma_or_brace) == kSYM_CloseBrace)
            break;
        if (sym.get_type(comma_or_brace) == kSYM_Comma)
            continue;

        cc_error("Expected ',' or '}'");
        return -1;
    }
    return 0;
}

// enum eEnumName { value1, value2 };
int ParseEnum(ccInternalList *targ, AGS::Symbol name_of_current_function)
{
    if (name_of_current_function >= 0)
    {
        cc_error("Enum declaration not allowed within a function body");
        return -1;
    }

    int retval = ParseEnum0(targ);
    if (retval < 0) return retval;

    // Force a semicolon after the declaration
    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}

int ParseExport(ccInternalList *targ, ccCompiledScript *scrip)
{
    if (kPP_PreAnalyze == g_PP)
    {
        const AGS::Symbol stoplist[] = { kSYM_Semicolon };
        SkipTo(targ, stoplist, 1);
        targ->getnext(); // Eat ';'
        return 0;
    }

    // export specified symbols
    AGS::Symbol cursym = targ->getnext();
    while (sym.get_type(cursym) != kSYM_Semicolon)
    {
        SymbolType const curtype = sym.get_type(cursym);
        if (curtype == 0)
        {
            cc_error("Can only export global variables and functions, not '%s'", sym.get_name_string(cursym).c_str());
            return -1;
        }
        if ((curtype != kSYM_GlobalVar) && (curtype != kSYM_Function))
        {
            cc_error("Invalid export symbol '%s'", sym.get_name_string(cursym).c_str());
            return -1;
        }
        if (FlagIsSet(sym.get_flags(cursym), kSFLG_Imported))
        {
            cc_error("Cannot export an import");
            return -1;
        }
        if (sym.get_vartype(cursym) == sym.getOldStringSym())
        {
            cc_error("Cannot export string; use char[200] instead");
            return -1;
        }
        // if all functions are being exported anyway, don't bother doing
        // it now
        if ((ccGetOption(SCOPT_EXPORTALL) != 0) && (curtype == kSYM_Function))
        { }
        else if (scrip->add_new_export(sym.get_name_string(cursym).c_str(),
            (curtype == kSYM_GlobalVar) ? EXPORT_DATA : EXPORT_FUNCTION,
            sym.entries[cursym].soffs, sym.entries[cursym].sscope) == -1)
        {
            return -1;
        }
        if (ReachedEOF(targ))
        {
            cc_error("Unexpected end of input");
            return -1;
        }
        cursym = targ->getnext();
        if (sym.get_type(cursym) == kSYM_Semicolon)
            break;
        if (sym.get_type(cursym) != kSYM_Comma)
        {
            cc_error("Expected ',' instead of '%s'", sym.get_name_string(cursym).c_str());
            return -1;
        }
        cursym = targ->getnext();
    }

    return 0;
}

int ParseVartype_GetVarName(ccInternalList *targ, AGS::Symbol &varname, AGS::Symbol &struct_of_member_fct)
{
    struct_of_member_fct = 0;

    varname = targ->getnext();

    if (sym.get_type(targ->peeknext()) != kSYM_MemberAccess)
        return 0; // done

    // We are accepting "struct::member"; so varname isn't the var name yet: it's the struct name.
    struct_of_member_fct = varname;
    targ->getnext(); // gobble "::"
    AGS::Symbol member_of_member_function = targ->getnext();

    // change varname to be the full function name
    varname = MangleStructAndComponent(struct_of_member_fct, member_of_member_function);
    if (varname < 0)
    {
        cc_error("'%s' does not contain a function '%s'",
            sym.get_name_string(struct_of_member_fct).c_str(),
            sym.get_name_string(member_of_member_function).c_str());
        return -1;
    }

    return 0;
}

int ParseVartype_CheckForIllegalContext(AGS::NestingStack *nesting_stack)
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

int ParseVartype_GetPointerStatus(ccInternalList *targ, int type_of_defn, bool &isPointer)
{
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(type_of_defn);
    isPointer = false;
    if (targ->peeknext() == sym.find("*"))
    {
        // only allow pointers to structs
        
        if (!FlagIsSet(entry.flags, kSFLG_StructType))
        {
            cc_error("Cannot create pointer to basic type");
            return -1;
        }
        if (FlagIsSet(entry.flags, kSFLG_Autoptr))
        {
            cc_error("Invalid use of '*'");
            return -1;
        }
        isPointer = true;
        targ->getnext();
    }

    if (FlagIsSet(entry.flags, kSFLG_Autoptr))
        isPointer = true;

    return 0;
}


int ParseVartype_CheckIllegalCombis(bool is_function, bool is_member_definition, TypeQualifierSet tqs)
{
    if (FlagIsSet(tqs, kTQ_Static) && (!is_function || !is_member_definition))
    {
        cc_error("'static' only applies to member functions");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected) && is_function)
    {
        cc_error("'protected' not valid for functions");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Noloopcheck) && !is_function)
    {
        cc_error("'noloopcheck' only valid with functions");
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

    if (is_function && FlagIsSet(tqs, kTQ_Noloopcheck) && FlagIsSet(tqs, kTQ_Import))
    {
        cc_error("'noloopcheck' cannot be applied to imported functions");
        return -1;
    }

    return 0;
}

int ParseVartype_FuncDef(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &func_name, int type_of_defn, bool isPointer, bool isDynamicArray, TypeQualifierSet tqs, AGS::Symbol &struct_of_current_func, AGS::Symbol &name_of_current_func)
{
    bool body_follows;

    // In the case of extender functions, this will alter func_name
    int retval = ParseFuncdecl(
        targ, scrip, func_name, type_of_defn, isPointer, isDynamicArray,
        tqs, struct_of_current_func, body_follows);
    if (retval < 0) return retval;

    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(func_name);
    if (struct_of_current_func > 0)
        SetFlag(entry.flags, kSFLG_StructMember, true);

    if (kPP_PreAnalyze == g_PP)
    {
        if (body_follows && kFT_LocalBody == entry.soffs)
        {
            cc_error("This function has already been defined with a body");
            return -1;
        }

        // Encode in entry.soffs the type of function declaration
        FunctionType ft = kFT_PureForward;
        if (FlagIsSet(tqs, kTQ_Import))
            ft = kFT_Import;
        if (body_follows)
            ft = kFT_LocalBody;
        if (entry.soffs < ft)
            entry.soffs = ft;
    }

    if (!body_follows)
    {
        if (kSYM_Semicolon != sym.get_type(targ->getnext()))
        {
            cc_error("Expected ';'");
            return -1;
        }
        return 0;
    }

    // We've started a function, remember what it is.
    name_of_current_func = func_name;
    return 0;
}


int ParseVartype_VarDef(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &var_name, Globalness is_global, int nested_level, bool is_readonly, int type_of_defn, SymbolType next_type, bool isPointer, bool &another_var_follows)
{
    if (kPP_PreAnalyze == g_PP)
    {
        if (0 != g_GIVM.count(var_name))
        {
            if(g_GIVM[var_name])
            {
                cc_error("'%s' is already defined as a global non-import variable", sym.get_name_string(var_name).c_str());
                return -1;
            }
            else if (kGl_GlobalNoImport == is_global && 0 !=ccGetOption(SCOPT_NOIMPORTOVERRIDE))
            {
                cc_error("'%s' is defined as an import variable; that cannot be overridden here", sym.get_name_string(var_name).c_str());
                return -1;
            }
            
        }
        g_GIVM[var_name] = (kGl_GlobalNoImport == is_global);
            
        // Apart from this, we aren't interested in var defns at this stage, so skip this defn
        AGS::Symbol const stoplist[] = { kSYM_Comma, kSYM_Semicolon };
        SkipTo(targ, stoplist, 2);
        another_var_follows = false;
        if (kSYM_Comma == sym.get_type(targ->peeknext()))
        {
            another_var_follows = true;
            targ->getnext(); // Eat ','
        }
        return 0;
    }

    if (is_global == kGl_Local)
        sym.entries[var_name].sscope = nested_level;
    if (is_readonly)
        SetFlag(sym.entries[var_name].flags, kSFLG_Readonly, true);

    // parse the definition
    return ParseVardecl(targ, scrip, var_name, type_of_defn, next_type, is_global, isPointer, another_var_follows);
}

// We accepted a variable type such as "int", so what follows is a function or variable definition
int ParseVartype0(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol vartype,           // e.g., "int"
    AGS::NestingStack *nesting_stack,
    TypeQualifierSet tqs,
    AGS::Symbol &name_of_current_func,
    AGS::Symbol &struct_of_current_func, // 0 if _not_ a member function
    bool &noloopcheck_is_set)
{
    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    // Don't define variable or function where illegal in context.
    int retval = ParseVartype_CheckForIllegalContext(nesting_stack);
    if (retval < 0) return retval;

    // Calculate whether this is a pointer definition, gobbling "*" if present
    bool isPointer = false;
    retval = ParseVartype_GetPointerStatus(targ, vartype, isPointer);
    if (retval < 0) return retval;

    // Look for "[]"; if present, gobble it and call this a dynamic array.
    // "int [] func(...)"
    int dynArrayStatus = ParseParamlist_Param_DynArrayMarker(targ, vartype, isPointer);
    if (dynArrayStatus < 0) return dynArrayStatus;
    bool isDynamicArray = (dynArrayStatus > 0);

    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    noloopcheck_is_set = false;
    if (kSYM_NoLoopCheck == sym.get_type(targ->peeknext()))
    {
        targ->getnext();
        noloopcheck_is_set = true;
    }

    Globalness is_global = kGl_Local;
    if (name_of_current_func <= 0)
        is_global = FlagIsSet(tqs, kTQ_Import) ? kGl_GlobalImport : kGl_GlobalNoImport;

    bool another_ident_follows = false; // will become true when we gobble a "," after a var defn
    // We've accepted a type expression and are now reading vars or one func that should have this type.
    do
    {
        if (ReachedEOF(targ))
        {
            cc_error("Unexpected end of input");
            return -1;
        }

        // Get the variable or function name.
        AGS::Symbol var_or_func_name = -1;
        retval = ParseVartype_GetVarName(targ, var_or_func_name, struct_of_current_func);
        if (retval < 0) return retval;

        if (kSYM_Vartype == sym.get_type(var_or_func_name) || is_primitive_vartype(var_or_func_name))
        {
            cc_error("'%s' is already in use as a type name", sym.get_name_string(var_or_func_name).c_str());
            return -1;
        }

        // Check whether var or func is being defined
        SymbolType next_type = sym.get_type(targ->peeknext());
        bool is_function = (kSYM_OpenParenthesis == sym.get_type(targ->peeknext()));
        bool is_member_definition = (struct_of_current_func > 0);

        // certains modifiers, such as "static" only go with certain kinds of definitions.
        retval = ParseVartype_CheckIllegalCombis(is_function, is_member_definition, tqs);
        if (retval < 0) return retval;

        if (is_function) // function defn
        {
            if ((name_of_current_func >= 0) || (nesting_stack->Depth() > 1))
            {
                cc_error("Nested functions not supported (you may have forgotten a closing brace)");
                return -1;
            }

            return ParseVartype_FuncDef(targ, scrip, var_or_func_name, vartype, isPointer, isDynamicArray, tqs, struct_of_current_func, name_of_current_func);
        }

        retval = ParseVartype_VarDef(targ, scrip, var_or_func_name, is_global, nesting_stack->Depth() - 1, FlagIsSet(tqs, kTQ_Readonly), vartype, next_type, isPointer, another_ident_follows);
        if (retval < 0) return retval;
    }
    while (another_ident_follows);

    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}

int ParseCommand_EndOfDoIfElse(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack)
{
    // Unravel else ... else ... chains
    while (nesting_stack->IsUnbraced())
    {
        if (nesting_stack->Type() == AGS::NestingStack::kNT_UnbracedDo)
        {
            int retval = DealWithEndOfDo(targ, scrip, nesting_stack);
            if (retval < 0) return retval;
            continue;
        }

        bool else_after_then;
        int retval = DealWithEndOfElse(targ, scrip, nesting_stack, else_after_then);
        if (retval < 0) return retval;
        // If an else follows a then clause, it has been changed into an
        // else clause that has just started so this clause has NOT ended yet.
        // So the else ... else chain is broken at this point, so we break out of the loop.
        if (else_after_then)
            break;
    }
    return 0;
}

int ParseReturn(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol name_of_current_func)
{
    AGS::Symbol const functionReturnType = sym.entries[name_of_current_func].funcparamtypes[0];

    if (sym.get_type(targ->peeknext()) != kSYM_Semicolon)
    {
        if (functionReturnType == sym.getVoidSym())
        {
            cc_error("Cannot return value from void function");
            return -1;
        }

        // parse what is being returned
        int retval = ParseExpression(targ, scrip);
        if (retval < 0) return retval;

        // If we need a string object ptr but AX contains a normal string, convert AX
        ConvertAXIntoStringObject(scrip, functionReturnType);

        // check return type is correct
        retval = IsVartypeMismatch(scrip->ax_vartype, functionReturnType, true);
        if (retval < 0) return retval;

        if ((is_string(scrip->ax_vartype)) &&
            (scrip->ax_val_scope == kSYM_LocalVar))
        {
            cc_error("Cannot return local string from function");
            return -1;
        }
    }
    else if (sym.getIntSym() == functionReturnType)
    {
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    }
    else if (sym.getVoidSym() != functionReturnType)
    {
        cc_error("Must return a '%s' value from function", sym.get_name_string(functionReturnType).c_str());
        return -1;
    }

    AGS::Symbol const cursym = targ->getnext();
    if (kSYM_Semicolon != sym.get_type(cursym))
    {
        cc_error("Expected ';' instead of '%s'", sym.get_name_string(cursym).c_str());
        return -1;
    }

    FreePointersOfLocals(scrip, 0, name_of_current_func);
    int totalsub = StacksizeOfLocals(0);
    if (totalsub > 0)
        scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);

    // Jump to the exit point of the function
    scrip->write_cmd1(SCMD_JMP, 0);
    g_FCM.TrackExitJumppoint(scrip, name_of_current_func, scrip->codesize - 1);

    return 0;
}

// Evaluate the head of an "if" clause, e.g. "if (i < 0)".
int ParseIf(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    // Get expression, must be in parentheses
    if (sym.get_type(targ->getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    scrip->write_cmd1(SCMD_JZ, 0);
    AGS::CodeLoc jump_dest_loc = scrip->codesize - 1;

    // Assume unbraced as a default
    retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedThen, // Type
        0, // Start
        jump_dest_loc); // Info

    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == kSYM_OpenBrace)
    {
        targ->getnext();
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedThen); // change to braced
    }

    return 0;
}


// Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
int ParseWhile(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    // point to the start of the code that evaluates the condition
    AGS::CodeLoc condition_eval_loc = scrip->codesize;

    // Get expression, must be in parentheses
    if (sym.get_type(targ->getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    scrip->write_cmd1(SCMD_JZ, 0);
    AGS::CodeLoc jump_dest_loc = scrip->codesize - 1;

    // Assume unbraced as a default
    retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedThen, // Type
        condition_eval_loc, // Start
        jump_dest_loc); // Info
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == kSYM_OpenBrace)
    {
        targ->getnext();
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedElse); // change to braced
    }
    return 0;
}

int ParseDo(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack)
{
    // We need a jump at a known location for the break command to work:
    scrip->write_cmd1(SCMD_JMP, 2); // Jump past the next jump :D
    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the end of the loop
    // This points to the address we have to patch with a jump past the end of the loop
    AGS::CodeLoc jump_dest_loc = scrip->codesize - 1;

    // Assume an unbraced DO as a default
    int retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedDo, // Type
        scrip->codesize, // Start
        jump_dest_loc);  // Info
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == kSYM_OpenBrace)
    {
        targ->getnext();
        // Change to braced DO
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedDo);
    }

    return 0;
}

// We're compiling function body code; the code does not start with a keyword or type.
// Thus, we should be at the start of an assignment or a funccall. Compile it.
int ParseAssignmentOrFunccall(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym)
{
    ccInternalList expr_script;
    expr_script.write(cursym); // expression starts with this
    int retval = BufferExpression(targ, expr_script);
    if (retval < 0) return retval;

    AGS::Symbol nextsym = targ->peeknext();
    SymbolType const nexttype = sym.get_type(nextsym);

    if (expr_script.length > 0)
    {
        if (nexttype == kSYM_Assign || nexttype == kSYM_AssignMod || nexttype == kSYM_AssignSOp)
        {
            targ->getnext();
            return ParseAssignment(targ, scrip, nextsym, &expr_script);
        }
        return ParseExpression_Subexpr(scrip, expr_script.script, expr_script.length);
    }
    cc_error("Unexpected symbol '%s'", sym.get_name_string(nextsym).c_str());
    return -1;
}

int ParseFor_InitClauseVardecl(ccInternalList *targ, ccCompiledScript *scrip, size_t nested_level)
{
    AGS::Symbol const vartype = targ->getnext();
    AGS::Flags const vartype_flags = sym.get_flags(vartype);
    bool isPointer = false;

    if (sym.getPointerSym() == targ->peeknext())
    {
        // only allow pointers to structs
        if (!FlagIsSet(vartype_flags, kSFLG_StructType))
        {
            cc_error("Cannot create pointer to basic type");
            return -1;
        }
        if (FlagIsSet(vartype_flags, kSFLG_Autoptr))
        {
            cc_error("Invalid use of '*'");
            return -1;
        }
        isPointer = true;
        targ->getnext();
    }

    if (FlagIsSet(vartype_flags, kSFLG_Autoptr))
        isPointer = true;

    if (sym.get_type(targ->peeknext()) == kSYM_NoLoopCheck)
    {
        cc_error("'noloopcheck' is not applicable in this context");
        return -1;
    }

    bool another_var_follows = false;
    do
    {
        AGS::Symbol varname = targ->getnext();
        if (sym.get_type(varname) != 0)
        {
            cc_error("Variable '%s' is already defined", sym.get_name_string(varname).c_str());
            return -1;
        }

        SymbolType const next_type = sym.get_type(targ->peeknext());
        if (kSYM_MemberAccess == next_type || kSYM_OpenParenthesis == next_type)
        {
            cc_error("Function definition not allowed in for loop initialiser");
            return -1;
        }

        sym.entries[varname].sscope = static_cast<short>(nested_level);

        // parse the declaration
        int varsize = sym.entries[vartype].ssize;
        int retval = ParseVardecl(targ, scrip, varname, vartype, next_type, kGl_Local, isPointer, another_var_follows);
        if (retval < 0) return retval;
    }
    while (another_var_follows);
    return 0;
}

// The first clause of a for header
int ParseFor_InitClause(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol peeksym, size_t nested_level)
{
    scrip->flush_line_numbers();

    if (sym.get_type(peeksym) == kSYM_Semicolon)
        return 0; // Empty init clause
    if (sym.get_type(peeksym) == kSYM_Vartype)
        return ParseFor_InitClauseVardecl(targ, scrip, nested_level);
    return ParseAssignmentOrFunccall(targ, scrip, targ->getnext());
}

int ParseFor_WhileClause(ccInternalList *targ, ccCompiledScript *scrip)
{
    scrip->flush_line_numbers();

    if (sym.get_type(targ->peeknext()) == kSYM_Semicolon)
    {
        // Not having a while clause is tantamount to the while condition "true".
        // So let's write "true" to the AX register.
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 1);
        return 0;
    }

    return ParseExpression(targ, scrip);
}

int ParseFor_IterateClause(ccInternalList *targ, ccCompiledScript *scrip)
{
    scrip->flush_line_numbers();
    // Check for empty interate clause
    if (kSYM_CloseParenthesis == sym.get_type(targ->peeknext()))
        return 0;

    return ParseAssignmentOrFunccall(targ, scrip, targ->getnext());
}

int ParseFor(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &cursym, AGS::NestingStack *nesting_stack)
{
    // "for (I; E; C) { ...}" is equivalent to "{ I; while (E) { ...; C} }"
    // We implement this with TWO levels of the nesting stack.
    // The outer level contains "I"
    // The inner level contains "while (E) { ...; C}"

    // Outer level
    int retval = nesting_stack->Push(AGS::NestingStack::kNT_For);
    if (retval < 0) return retval;

    cursym = targ->getnext();
    if (sym.get_type(cursym) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '(' after 'for'");
        return -1;
    }

    AGS::Symbol peeksym = targ->peeknext();
    if (sym.get_type(peeksym) == kSYM_CloseParenthesis)
    {
        cc_error("Empty parentheses \"()\" aren't allowed after \"for\" (write \"for(;;)\" instead");
        return -1;
    }

    // Generate the initialization clause (I)
    retval = ParseFor_InitClause(targ, scrip, peeksym, nesting_stack->Depth() - 1);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';' after for loop initializer clause");
        return -1;
    }

    // Remember where the code of the while condition starts.
    AGS::CodeLoc while_cond_loc = scrip->codesize;

    retval = ParseFor_WhileClause(targ, scrip);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';' after for loop while clause");
        return -1;
    }

    // Remember where the code of the iterate clause starts.
    AGS::CodeLoc iterate_clause_loc = scrip->codesize;
    size_t pre_fixup_count = scrip->numfixups;

    retval = ParseFor_IterateClause(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != kSYM_CloseParenthesis)
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

    if (sym.get_type(targ->peeknext()) == kSYM_OpenBrace)
    {
        targ->getnext();
        // Set type "braced" instead of "unbraced"
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedElse);
    }

    // We've just generated code for getting to the next loop iteration.
    // But we don't need that code right here; we need it at the bottom of the loop.
    // So rip it out of the bytecode base and save it into our nesting stack.
    int id;
    size_t const yank_size = scrip->codesize - iterate_clause_loc;
    nesting_stack->YankChunk(scrip, iterate_clause_loc, pre_fixup_count, id);
    g_FCM.UpdateCallListOnYanking(iterate_clause_loc, yank_size, id);

    // Code for "If the expression we just evaluated is false, jump over the loop body."
    // the 0 will be fixed to a proper offset later
    scrip->write_cmd1(SCMD_JZ, 0);
    nesting_stack->SetJumpOutLoc(scrip->codesize - 1); // the address to fix

    return 0;
}

int ParseSwitch(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack)
{
    // Get the switch expression, must be in parentheses
    if (sym.get_type(targ->getnext()) != kSYM_OpenParenthesis)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != kSYM_CloseParenthesis)
    {
        cc_error("Expected ')'");
        return -1;
    }

    // Remember the type of this expression to enforce it later
    int switch_expr_type = scrip->ax_vartype;

    // Copy the result to the BX register, ready for case statements
    scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
    scrip->flush_line_numbers();

    // Remember the start of the lookup table
    AGS::CodeLoc lookup_table_start = scrip->codesize;

    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the lookup table
    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to beyond the switch statement (for break)

    // There's no such thing as an unbraced SWITCH, so '{' must follow
    if (sym.get_type(targ->getnext()) != kSYM_OpenBrace)
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
    if (ReachedEOF(targ))
    {
        currentline = targ->lineAtEnd;
        cc_error("Unexpected end of input");
        return -1;
    }
    if (sym.get_type(targ->peeknext()) != kSYM_Case && sym.get_type(targ->peeknext()) != kSYM_Default && sym.get_type(targ->peeknext()) != kSYM_CloseBrace)
    {
        cc_error("Invalid keyword '%s' in switch statement block", sym.get_name_string(targ->peeknext()).c_str());
        return -1;
    }
    return 0;
}

int ParseSwitchLabel(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    if (nesting_stack->Type() != AGS::NestingStack::kNT_Switch)
    {
        cc_error("Case label not valid outside switch statement block");
        return -1;
    }

    if (sym.get_type(cursym) == kSYM_Default)
    {
        if (nesting_stack->DefaultLabelLoc() != -1)
        {
            cc_error("This switch statement block already has a \"default:\" label");
            return -1;
        }
        nesting_stack->SetDefaultLabelLoc(scrip->codesize);
    }
    else // "case"
    {
        AGS::CodeLoc start_of_code_loc = scrip->codesize;
        int numfixups_at_start_of_code = scrip->numfixups;
        // Push the switch variable onto the stack
        scrip->push_reg(SREG_BX);

        // get an expression
        int retval = ParseExpression(targ, scrip);
        if (retval < 0) return retval;  // case n: label expression, result is in AX

        // check that the types of the "case" expression and the "switch" expression match
        retval = IsVartypeMismatch(scrip->ax_vartype, nesting_stack->SwitchExprType(), false);
        if (retval < 0) return retval;

        // Pop the switch variable, ready for comparison
        scrip->pop_reg(SREG_BX);

        // get the right equality operator for the type
        int eq_op = SCMD_ISEQUAL;
        retval = GetOperatorValidForVartype(scrip->ax_vartype, nesting_stack->SwitchExprType(), eq_op);
        if (retval < 0) return retval;

        // [fw] Comparison operation may be missing here.

    // rip out the already generated code for the case/switch and store it with the switch
        int id;
        size_t const yank_size = scrip->codesize - start_of_code_loc;
        nesting_stack->YankChunk(scrip, start_of_code_loc, numfixups_at_start_of_code, id);
        g_FCM.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
    }

    // expect and gobble the ':'
    if (sym.get_type(targ->getnext()) != kSYM_Label)
    {
        cc_error("Expected ':'");
        return -1;
    }

    return 0;
}

int ParseBreak(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack)
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

    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // If locals contain pointers, free them
    FreePointersOfLocals(scrip, loop_level - 1);

    // Pop local variables from the stack
    int totalsub = StacksizeOfLocals(loop_level - 1);
    if (totalsub > 0)
        scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    scrip->flush_line_numbers();

    // The jump out of the loop, below, may be a conditional jump.
    // So clear AX to make sure that the jump is executed.
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

    // Jump to a jump to the end of the loop
    // The bytecode byte with the relative dest is at code[codesize+1]
    if (nesting_stack->Type(loop_level) == AGS::NestingStack::kNT_Switch)
        scrip->write_cmd1(SCMD_JMP,
            RelativeJumpDist(scrip->codesize + 1, nesting_stack->StartLoc(loop_level) + 2));
    else
        scrip->write_cmd1(SCMD_JMP,
            RelativeJumpDist(scrip->codesize + 1, nesting_stack->JumpOutLoc(loop_level) - 1));
    return 0;
}

int ParseContinue(ccInternalList *targ, ccCompiledScript *scrip, AGS::NestingStack *nesting_stack)
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

    if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // If locals contain pointers, free them
    FreePointersOfLocals(scrip, loop_level - 1);

    // Pop local variables from the stack
    int totalsub = StacksizeOfLocals(loop_level - 1);
    if (totalsub > 0)
        scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    scrip->flush_line_numbers();

    // if it's a for loop, drop the yanked chunk (loop increment)back in
    if (nesting_stack->ChunksExist(loop_level))
    {
        int id;
        AGS::CodeLoc const write_start = scrip->codesize;
        nesting_stack->WriteChunk(scrip, loop_level, 0, id);
        g_FCM.UpdateCallListOnWriting(write_start, id);
    }
    scrip->flush_line_numbers();

    // original comment "The jump below may be a conditional jump."
    //  [fw] Nooo? Leave it in, anyway, so that we have identical bytecode
    // original comment "So clear AX to make sure that the jump is executed."
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

    // Jump to the start of the loop
    // The bytecode int with the relative dest is at code[codesize+1]
    scrip->write_cmd1(
        SCMD_JMP,
        RelativeJumpDist(scrip->codesize + 1, nesting_stack->StartLoc(loop_level)));
    return 0;
}

int ParseCommand(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol cursym,
    AGS::Symbol &name_of_current_func,
    AGS::Symbol &struct_of_current_func,
    AGS::NestingStack *nesting_stack,
    bool next_is_noloopcheck)
{
    int retval;

    switch (sym.get_type(cursym))
    {
    default:
        // If it doesn't begin with a keyword, it should be an assignment
        // or a func call.
        retval = ParseAssignmentOrFunccall(targ, scrip, cursym);
        if (retval < 0) return retval;
        if (sym.get_type(targ->getnext()) != kSYM_Semicolon)
        {
            cc_error("Expected ';'");
            return -1;
        }
        break;

    case kSYM_Break:
        retval = ParseBreak(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Case:
        retval = ParseSwitchLabel(targ, scrip, cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case  kSYM_CloseBrace:
        return ParseClosebrace(targ, scrip, nesting_stack, struct_of_current_func, name_of_current_func);

    case kSYM_Continue:
        retval = ParseContinue(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Default:
        retval = ParseSwitchLabel(targ, scrip, cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_Do:
        return ParseDo(targ, scrip, nesting_stack);

    case kSYM_For:
        return ParseFor(targ, scrip, cursym, nesting_stack);

    case kSYM_If:
        return ParseIf(targ, scrip, cursym, nesting_stack);

    case kSYM_OpenBrace:
        return ParseOpenbrace(scrip, nesting_stack, name_of_current_func, struct_of_current_func, next_is_noloopcheck);

    case kSYM_Return:
        retval = ParseReturn(targ, scrip, name_of_current_func);
        if (retval < 0) return retval;
        break;

    case kSYM_Switch:
        retval = ParseSwitch(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case kSYM_While:
        return ParseWhile(targ, scrip, cursym, nesting_stack);
    }

    // sort out jumps when a single-line if or else has finished
    return ParseCommand_EndOfDoIfElse(targ, scrip, nesting_stack);
}

int cc_parse_HandleLines(ccInternalList *targ, ccCompiledScript *scrip, int &currentlinewas)
{
    if (currentline == -10)
        return 1; // end of stream was reached


    if ((currentline != currentlinewas) && (ccGetOption(SCOPT_LINENUMBERS) != 0))
    {
        scrip->set_line_number(currentline);
        currentlinewas = currentline;
    }

    return 0;
}

int cc_parse_TQCombiError(TypeQualifierSet tqs)
{
    std::map<TypeQualifier, std::string> const tq2String =
    {
        {kTQ_Autoptr, "autoptr"},
        {kTQ_Const, "const"},
        {kTQ_ImportStd, "import"},
        {kTQ_ImportTry, "_tryimport"},
        {kTQ_Noloopcheck, "noloopcheck"},
        {kTQ_Managed, "managed"},
        {kTQ_Protected, "protected"},
        {kTQ_Readonly, "readonly"},
        {kTQ_Static, "static"},
        {kTQ_Stringstruct, "stringstruct"},
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
int cc_parse_CheckTQ(TypeQualifierSet tqs, AGS::Symbol decl_type)
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
        cc_parse_TQCombiError((tqs & ~kTQ_Readonly));
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Managed) && kSYM_Struct != decl_type)
    {
        cc_error("'managed' can only be used with structs");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected) && 0 != (tqs & ~kTQ_Static & ~kTQ_Readonly))
    {
        cc_parse_TQCombiError((tqs & ~kTQ_Static & ~kTQ_Readonly));
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
        cc_parse_TQCombiError((tqs & ~kTQ_Static & ~kTQ_Readonly));
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
        cc_parse_TQCombiError(tqs);
        return -1;
    }
    return 0;
}

int ParseVartype(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, TypeQualifierSet tqs, AGS::NestingStack &nesting_stack, AGS::Symbol &name_of_current_func, AGS::Symbol &struct_of_current_func, bool &set_nlc_flag)
{
    // func or variable definition
    int retval = cc_parse_CheckTQ(tqs, kSYM_Vartype);
    if (retval < 0) return retval;
    return ParseVartype0(targ, scrip, cursym, &nesting_stack, tqs, name_of_current_func, struct_of_current_func, set_nlc_flag);
}

void cc_parse_SkipToEndingBrace(ccInternalList *targ)
{
    // Skip to matching '}'
    AGS::Symbol const stoplist[] = { 0 };
    SkipTo(targ, stoplist, 0); // pass empty list
    targ->getnext(); // Eat '}'
}

// Buffer for the script name
std::string g_ScriptNameBuffer;

void cc_parse_StartNewSection(ccCompiledScript *scrip, AGS::Symbol mangled_section_name)
{
    g_ScriptNameBuffer = sym.get_name_string(mangled_section_name).substr(18);
    g_ScriptNameBuffer.pop_back(); // strip closing speech mark
    ccCurScriptName = g_ScriptNameBuffer.c_str();
    currentline = 0;
    // The Pre-Compile phase shouldn't generate any code,
    // so only do it in the Main phase
    if (kPP_Main == g_PP)
        scrip->start_new_section(g_ScriptNameBuffer.c_str());
}

int cc_parse_ParseInput(ccInternalList *targ, ccCompiledScript *scrip)
{
    AGS::NestingStack nesting_stack = AGS::NestingStack();
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

    while (!ReachedEOF(targ))
    {
        AGS::Symbol cursym = targ->getnext();

        if (0 == sym.get_name_string(cursym).compare(0, 18, NEW_SCRIPT_TOKEN_PREFIX))
        {
            cc_parse_StartNewSection(scrip, cursym);           
            continue;
        }

        int retval = cc_parse_HandleLines(targ, scrip, currentlinewas);
        if (retval > 0)
            break; // end of input

        SymbolType const symType = GetSymbolTypeAnyPhase(cursym);
        switch (symType)
        {

        default: break;

        case 0:
            // let it through if "this" can be implied
            if (struct_of_current_func > 0)
            {
                AGS::Symbol combined = MangleStructAndComponent(struct_of_current_func, cursym);
                if (kSYM_NoType != sym.get_type(combined))
                    break;
            }
            cc_error("Unexpected token '%s'", sym.get_name_string(cursym).c_str());
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
            int retval = cc_parse_CheckTQ(tqs, kSYM_Export);
            if (retval < 0) return retval;
            retval = ParseEnum(targ, name_of_current_func);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case kSYM_Export:
        {
            int retval = cc_parse_CheckTQ(tqs, kSYM_Export);
            if (retval < 0) return retval;
            retval = ParseExport(targ, scrip);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case kSYM_Import:
        {
            if (std::string::npos != sym.get_name_string(cursym).find("_tryimport"))
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
            if (kPP_Main == g_PP)
                break; // treat as a command, below the switch

            cc_parse_SkipToEndingBrace(targ);
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
            int retval = cc_parse_CheckTQ(tqs, kSYM_Struct);
            if (retval < 0) return retval;
            retval = ParseStruct(targ, scrip, tqs, nesting_stack, name_of_current_func, struct_of_current_func);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case kSYM_Vartype:
        {
            if (kSYM_Dot == sym.get_type(targ->peeknext()))
                break; // this is a static struct component function call, so a command
            bool set_nlc_flag = false;
            int retval = ParseVartype(targ, scrip, cursym, tqs, nesting_stack, name_of_current_func, struct_of_current_func, set_nlc_flag);
            if (retval < 0) return retval;
            tqs = 0;
            if (set_nlc_flag)
                SetFlag(tqs, kTQ_Noloopcheck, true);
            continue;
        }

        } // switch (symType)

        // Commands are only allowed within a function
        if (name_of_current_func <= 0)
        {
            cc_error("'%s' is illegal outside a function", sym.get_name_string(cursym).c_str());
            return -1;
        }

        retval = ParseCommand(targ, scrip, cursym, name_of_current_func, struct_of_current_func, &nesting_stack, FlagIsSet(tqs, kTQ_Noloopcheck));
        if (retval < 0) return retval;
        tqs = 0;
    } // while (!ReachedEOF(targ))

    return 0;
}

// Copy all the func headers from the PreAnalyse phase into the "real" symbol table
int cc_parse_FuncHeaders2Sym()
{
    for (TSym1Table::iterator sym_it = g_Sym1.begin(); sym_it != g_Sym1.end(); ++sym_it)
    {
        SymbolTableEntry &s_entry = sym_it->second;
        if (s_entry.stype != kSYM_Function)
            continue;
        SymbolTableEntry &f_entry = sym.entries[sym_it->first];
        SetFlag(s_entry.flags, kSFLG_Imported, (kFT_Import == s_entry.soffs));
        s_entry.soffs = 0;
        s_entry.sname = f_entry.sname;
        s_entry.CopyTo(f_entry);
    }
    return 0;
}

int cc_parse(ccInternalList *targ, ccCompiledScript *scrip)
{
    g_GIVM.clear();
    g_ImportMgr.Init(scrip);

    // Skim through the code and collect the headers of functions defined locally
    int const start_of_input = targ->pos;
    g_Sym1.clear();
    for (size_t idx = 0; idx < sym.entries.size(); idx++)
        g_Sym1[idx] = sym.entries[idx];

    g_PP = kPP_PreAnalyze;
    int retval = cc_parse_ParseInput(targ, scrip);
    if (retval < 0) return retval;

    // Copy (just) the headers of functions that have a body to the main symbol table
    retval = cc_parse_FuncHeaders2Sym();
    if (retval < 0) return retval;
    g_Sym1.clear();

    // Back up, parse the source in earnest and generate code for it
    targ->pos = start_of_input;
    g_PP = kPP_Main;
    g_FCM.Init();
    retval = cc_parse_ParseInput(targ, scrip);
    if (retval < 0) return retval;

    return g_FCM.CheckForUnresolvedFuncs();
}

// compile the code in the INPL parameter into code in the scrip structure,
// but don't reset anything because more files could follow
int cc_compile(const char *inpl, ccCompiledScript *scrip)
{
    ccInternalList targ;

    // Scan & tokenize the program code.
    int retval = cc_tokenize(inpl, &targ, scrip);
    if (retval < 0) return retval;

    targ.startread();
    return cc_parse(&targ, scrip);
}

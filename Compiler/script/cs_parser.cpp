﻿/*
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
        In ParseExpression() and ParseSubexpr()
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
int ParseSubexpr(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len);
int ReadDataIntoAX(ccCompiledScript *scrip, AGS::SymbolScript syml, size_t syml_len, bool negate = false);

enum FxFixupType // see script_common.h
{
    kFx_NoFixup = 0,
    kFx_FixupDataData = FIXUP_DATADATA, // globaldata[fixup] += &globaldata[0]
    kFx_FixupFunction = FIXUP_FUNCTION,  // code[fixup] += &code[0]
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
    kTQ_Import1 = 1 << 4,
    kTQ_Import2 = 1 << 5,
    kTQ_Noloopcheck = 1 << 6,
    kTQ_Managed = 1 << 7,
    kTQ_Protected = 1 << 8,
    kTQ_Readonly = 1 << 9,
    kTQ_Static = 1 << 10,
    kTQ_Stringstruct = 1 << 11,
    kTQ_Writeprotected = 1 << 12,
    kTQ_Import = kTQ_Import1 | kTQ_Import2,
};

inline bool FlagIsSet(long fl_set, long flag) { return 0 != (fl_set & flag); }
inline void SetFlag(long &fl_set, long flag, bool val) { fl_set &= ~flag; if (val) fl_set |= flag; }


bool IsIdentifier(AGS::Symbol symb)
{
    if (symb <= sym.lastPredefSym || symb > sym.entries.size())
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

AGS::Symbol MangleStructFunc(AGS::Symbol stname, AGS::Symbol funcname)
{
    std::string func_str = "::";
    func_str = sym.get_name_string(stname) + func_str + sym.get_name_string(funcname);
    return sym.find_or_add(func_str.c_str());
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
        AGS::Symbol const curtype = sym.get_type(cursym);
        if (curtype == SYM_OPENBRACE ||
            curtype == SYM_OPENBRACKET ||
            curtype == SYM_OPENPARENTHESIS)
        {
            ++delimeter_nesting_depth;
        }
        if (curtype == SYM_CLOSEBRACE ||
            curtype == SYM_CLOSEBRACKET ||
            curtype == SYM_CLOSEPARENTHESIS)
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

int SkipToScript0(AGS::Symbol *start_sym_ptr, AGS::Symbol *end_sym_ptr, const AGS::Symbol stoplist[], size_t stoplist_len, AGS::Symbol *&act_sym_ptr)
{
    int delimeter_nesting_depth = 0;

    for (act_sym_ptr = start_sym_ptr; act_sym_ptr != end_sym_ptr; act_sym_ptr++)
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        AGS::Symbol const cursym = *act_sym_ptr;
        AGS::Symbol const curtype = sym.get_type(cursym);
        if (curtype == SYM_OPENBRACE ||
            curtype == SYM_OPENBRACKET ||
            curtype == SYM_OPENPARENTHESIS)
        {
            ++delimeter_nesting_depth;
        }
        if (curtype == SYM_CLOSEBRACE ||
            curtype == SYM_CLOSEBRACKET ||
            curtype == SYM_CLOSEPARENTHESIS)
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
int SkipToScript(const AGS::Symbol stoplist[], size_t stoplist_len, SymbolScript &syml, size_t &syml_len)
{
    SymbolScript new_syml_start_ptr;
    int retval = SkipToScript0(syml, syml + syml_len, stoplist, stoplist_len, new_syml_start_ptr);
    syml_len = new_syml_start_ptr - syml; // Get new length of the symbol script
    syml = new_syml_start_ptr;

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
void AGS::NestingStack::YankChunk(ccCompiledScript *scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset, int &id)
{
    AGS::NestingStack::Chunk item;

    for (size_t code_idx = codeoffset; code_idx < scrip->codesize; code_idx++)
        item.Code.push_back(scrip->code[code_idx]);

    for (size_t fixups_idx = fixupoffset; fixups_idx < scrip->numfixups; fixups_idx++)
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

AGS::FuncCallpointMgr::FuncInfo::FuncInfo()
    : Callpoint(-1)
{ }

// Manage a map of all the functions that have bodies defined.
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
// Only a global import may be re-defined as a global non-import (identical except for the import),
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
    kPP_PreAnalyze = 0,
    kPP_Main,
} g_PP;

enum FunctionType
{
    kFT_PureForward = 0,
    kFT_Import = 1,
    kFT_LocalBody = 2,
};

// Auxiliary symbol table for the first phase.
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
inline AGS::Symbol GetSymbolTypeAnyPhase(AGS::Symbol symb)
{
    if (symb < 0)
        return -1;
    return GetSymbolTableEntryAnyPhase(symb & STYPE_MASK).stype;
}

std::string GetFullNameOfMember(AGS::Symbol structSym, AGS::Symbol memberSym)
{

    // Get C-string of member, de-mangle it if appropriate
    std::string memberNameStr = sym.get_name_string(memberSym);
    if (memberNameStr[0] == '.')
        memberNameStr.erase(0, 1);
    std::string ret = sym.get_name_string(structSym) + "::" + memberNameStr;
    return ret;
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
    while (true)
    {
        int token;
        tokenizer.GetNextToken(token, eof_encountered, error_encountered);
        if (error_encountered)
        {
            cc_error(tokenizer.GetLastError().c_str());
            return -1;
        }
        if (eof_encountered || error_encountered)
            break;
        targ->write(static_cast<::AGS::Symbol>(token));
    }

    // Write pseudo opcode for "This ends this tokenization"
    targ->write_meta(SMETA_END, 0);

    tokenizer.ResetTemporaryTypesInSymbolTable();

    return 0;
}


void FreePointer(ccCompiledScript *scrip, int spOffset, int zeroCmd, AGS::Symbol arraySym)
{
    scrip->write_cmd1(SCMD_LOADSPOFFS, spOffset);
    scrip->write_cmd(zeroCmd);

    if ((sym.entries[arraySym].flags & (SFLG_ARRAY | SFLG_DYNAMICARRAY)) == SFLG_ARRAY)
    {
        // array of pointers -- release each one
        const size_t arrsize = sym.entries[arraySym].arrsize;
        for (size_t entry = 1; entry < arrsize; entry++)
        {
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, 4);
            scrip->write_cmd(zeroCmd);
        }
    }
}


void FreePointersOfStruct(ccCompiledScript *scrip, AGS::Symbol structVarSym)
{
    size_t structType = sym.entries[structVarSym].vartype;

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].stype != SYM_STRUCTMEMBER)
            continue;
        if (sym.entries[entries_idx].extends != structType)
            continue;
        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_IMPORTED))
            continue;
        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_ATTRIBUTE))
            continue;

        if (!FlagIsSet(sym.entries[entries_idx].flags, SFLG_POINTER))
        {
            // original comment: "if non-pointer struct, need to process its members
            // **** TODO"
            // [fw] depends on how that struct of struct is in the symbol table;
            //      STRUCTSYM.SUBSTRUCTSYM perhaps?
            //      AGS:Symbol substruct = find(....);
            //      if (substruct is not in the symtable) continue;

            //      FreePointersOfStruct(scrip, substruct);

            continue;
        }

        // Locate where the pointer is on the stack
        int spOffs = (scrip->cur_sp - sym.entries[structVarSym].soffs) - sym.entries[entries_idx].soffs;

        FreePointer(scrip, spOffs, SCMD_MEMZEROPTR, static_cast<AGS::Symbol>(entries_idx));

        if (FlagIsSet(sym.entries[structVarSym].flags, SFLG_ARRAY))
        {
            // an array of structs, free any pointers in them
            const size_t arrsize = sym.entries[structVarSym].arrsize;
            for (size_t entry = 1; entry < arrsize; entry++)
            {
                spOffs -= sym.entries[structType].ssize;
                FreePointer(scrip, spOffs, SCMD_MEMZEROPTR, static_cast<AGS::Symbol>(entries_idx));
            }
        }
    }
}


inline bool is_any_type_of_string(AGS::Symbol symtype)
{
    symtype &= ~(STYPE_CONST | STYPE_POINTER);
    if ((symtype == sym.normalStringSym) || (symtype == sym.stringStructSym))
        return true;
    return false;
}


// Returns the relative distance in a jump instruction
// "here" is the location of the bytecode int that will contain
// the (relative) destination.It is not the location of the
// start of the command.
inline int RelativeJumpDist(AGS::CodeLoc here, AGS::CodeLoc dest)
{
    // JMP 0 jumps to the bytecode symbol directly behind the command.
    // So if dest == here, -1 must be returned.
    return static_cast<int>(dest - here - 1);
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
        if (sym.entries[entries_idx].stype != SYM_LOCALVAR)
            continue;

        // caller will sort out stack, so ignore parameters
        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_PARAMETER))
            continue;

        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_DYNAMICARRAY))
        {
            totalsub += 4; // size of an implicit pointer
            continue;
        }

        // Calculate the size of one var of the given type
        size_t ssize = sym.entries[entries_idx].ssize;
        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_STRBUFFER))
            ssize += STRING_LENGTH;

        // Calculate the number of vars
        size_t number = 1;
        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_ARRAY))
            number = sym.entries[entries_idx].arrsize;

        totalsub += ssize * number;
    }
    return totalsub;
}


// Free the pointers of any locals in level from_level or higher
int FreePointersOfLocals(ccCompiledScript *scrip, int from_level)
{
    int totalsub = 0;
    int zeroPtrCmd = SCMD_MEMZEROPTR;
    if (from_level == 0)
        zeroPtrCmd = SCMD_MEMZEROPTRND;

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].sscope <= from_level)
            continue;
        if (sym.entries[entries_idx].stype != SYM_LOCALVAR)
            continue;

        // don't touch the this pointer
        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_THISPTR))
            continue;

        if (FlagIsSet(sym.entries[entries_idx].flags, SFLG_POINTER) ||
            FlagIsSet(sym.entries[entries_idx].flags, SFLG_DYNAMICARRAY))
        {
            FreePointer(scrip, scrip->cur_sp - sym.entries[entries_idx].soffs, zeroPtrCmd, static_cast<AGS::Symbol>(entries_idx));
            continue;
        }

        if (FlagIsSet(sym.entries[sym.entries[entries_idx].vartype].flags, SFLG_STRUCTTYPE))
        {
            // free any pointers that this struct contains
            FreePointersOfStruct(scrip, static_cast<AGS::Symbol>(entries_idx));
            continue;
        }
    }
    return 0;
}

// Remove defns from the sym table of vars defined on from_level or higher
int RemoveLocalsFromSymtable(int from_level)
{

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].sscope < from_level)
            continue;
        if (sym.entries[entries_idx].stype != SYM_LOCALVAR)
            continue;

        sym.entries[entries_idx].stype = 0;
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
    else if (sym.get_type(targ->peeknext()) == SYM_ELSE)
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
        if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
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
    if (sym.get_type(cursym) != SYM_WHILE)
    {
        cc_error("Do without while");
        return -1;
    }
    if (sym.get_type(targ->getnext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    scrip->flush_line_numbers();

    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != SYM_CLOSEPARENTHESIS)
    {
        cc_error("Expected ')'");
        return -1;
    }
    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
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

// Given a struct STRUCT and a member MEMBER, 
// finds the symbol of the fully qualified name STRUCT::MEMBER and returns it in memSym.
// Gives error if the fully qualified symbol is protected.
int FindMemberSym(AGS::Symbol structSym, AGS::Symbol &memSym, bool allowProtected, bool errorOnFail = true)
{

    AGS::Symbol full_name = sym.find(GetFullNameOfMember(structSym, memSym).c_str());

    if (full_name < 0)
    {
        if (sym.entries[structSym].extends > 0)
        {
            // look for the member in the ancesters recursively
            if (FindMemberSym(sym.entries[structSym].extends, memSym, allowProtected) == 0)
                return 0;
        }
        if (errorOnFail)
            cc_error(
                "'%s' is not a public member of '%s'. Are you sure you spelt it correctly (remember, capital letters are important)?",
                sym.get_name_string(memSym).c_str(),
                sym.get_name_string(structSym).c_str());
        return -1;
    }

    if ((!allowProtected) && FlagIsSet(sym.entries[full_name].flags, SFLG_PROTECTED))
    {
        if (errorOnFail)
            cc_error(
                "Cannot access protected member '%s'",
                sym.get_name_string(full_name).c_str());
        return -1;
    }
    memSym = full_name;
    return 0;
}


int ParseLiteralOrConstvalue(AGS::Symbol fromSym, int &theValue, bool isNegative, std::string errorMsg)
{
    if (fromSym >= 0)
    {
        // sym.get_type() won't work in the Pre-Analyse phase for compile-time constants
        SymbolTableEntry &from_entry = GetSymbolTableEntryAnyPhase(fromSym); 
        if (from_entry.stype == SYM_CONSTANT)
        {
            theValue = from_entry.soffs;
            if (isNegative)
                theValue = -theValue;
            return 0;
        }

        if (sym.get_type(fromSym) == SYM_LITERALVALUE)
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
    if (sym.get_type(targ->peeknext()) != SYM_ASSIGN)
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
    if (sym.get_type(targ->peeknext()) != SYM_OPENBRACKET)
        return 0;

    // Gobble the '[', expect and gobble ']'
    targ->getnext();
    if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
    {
        cc_error("Fixed array size cannot be used here (use '[]' instead)");
        return -1;
    }

    if (kPP_PreAnalyze == g_PP)
        return 1;

    if (FlagIsSet(sym.entries[typeSym].flags, SFLG_STRUCTTYPE))
    {
        if (!FlagIsSet(sym.entries[typeSym].flags, SFLG_MANAGED))
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
    known_info.stype = 0;
    if (0 == entry.stype)
        return 0; // there is no info yet

    int retval = entry.CopyTo(known_info);
    if (retval < 0) return retval;
    // If the return type is unset, deduce it
    if (0 == known_info.ssize && known_info.funcparamtypes.size() > 1)
    {
        AGS::Symbol const rettype = known_info.funcparamtypes.at(0) & ~(STYPE_POINTER | STYPE_DYNARRAY);
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
    if (!FlagIsSet(struct_entry.flags, SFLG_STRUCTTYPE))
    {
        cc_error("Expected a struct type instead of '%s'", sym.get_name_string(struct_of_func).c_str());
        return -1;
    }

    if (std::string::npos != sym.get_name_string(name_of_func).find_first_of(':'))
    {   // [fw] Can't be reached IMO. 
        cc_error("Extender functions cannot be part of a struct");
        return -1;
    }

    name_of_func = MangleStructFunc(struct_of_func, name_of_func);
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(name_of_func); 

    // Don't clobber the flags set in the Pre-Analyze phase
    SetFlag(entry.flags, SFLG_STRUCTMEMBER, true);
    if (is_static_extender)
        SetFlag(entry.flags, SFLG_STATIC, true);

    targ->getnext();
    if (!is_static_extender && targ->getnext() != sym.find("*"))
    {
        cc_error("Instance extender function must be pointer");
        return -1;
    }

    if ((sym.get_type(targ->peeknext()) != SYM_COMMA) &&
        (sym.get_type(targ->peeknext()) != SYM_CLOSEPARENTHESIS))
    {
        if (targ->getnext() == sym.normalPointerSym)
            cc_error("Static extender function cannot be pointer");
        else
            cc_error("Parameter name cannot be defined for extender type");
        return -1;
    }

    if (sym.get_type(targ->peeknext()) == SYM_COMMA)
        targ->getnext();

    return 0;
}


int ParseParamlist_ParamType(ccInternalList *targ, AGS::Symbol param_type, bool &param_is_ptr)
{
    // Determine whether the type is a pointer
    bool const param_is_natural_ptr =
        (sym.normalPointerSym == targ->peeknext());
    if (param_is_natural_ptr)
        targ->getnext(); // gobble the '*'
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(param_type);
    bool const param_is_autoptr = FlagIsSet(entry.flags, SFLG_AUTOPTR);
    param_is_ptr = param_is_natural_ptr || param_is_autoptr;

    if (kPP_PreAnalyze == g_PP)
        return 0;

    // Safety checks on the parameter type
    if (param_type == sym.normalVoidSym)
    {
        cc_error("A function parameter must not have the type 'void'");
        return -1;
    }
    if (param_is_ptr)
    {
        if (!FlagIsSet(sym.entries[param_type].flags, SFLG_MANAGED))
        {
            // can only point to managed structs
            cc_error("Cannot declare pointer to non-managed type");
            return -1;
        }
        if (param_is_natural_ptr && param_is_autoptr)
        {
            cc_error("Cannot use '*' with %s", sym.get_name_string(param_type).c_str());
            return -1;
        }
    }
    if (FlagIsSet(sym.entries[param_type].flags, SFLG_STRUCTTYPE) && (!param_is_ptr))
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

    if (sym.get_type(targ->peeknext()) == SYM_GLOBALVAR)
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


void ParseParamlist_Param_AsVar2Sym(ccCompiledScript *scrip, AGS::Symbol param_name, AGS::Symbol param_type, bool param_is_ptr, bool param_is_const, bool param_is_dynarray, int param_idx)
{
    SymbolTableEntry &param_entry = sym.entries[param_name];
    param_entry.stype = SYM_LOCALVAR;
    param_entry.extends = false;
    param_entry.arrsize = 1;
    param_entry.vartype = param_type;
    param_entry.ssize = 4;
    param_entry.sscope = 1;
    SetFlag(param_entry.flags, SFLG_PARAMETER, true);
    if (param_is_ptr)
        SetFlag(param_entry.flags, SFLG_POINTER, true);
    if (param_is_const)
        SetFlag(param_entry.flags, SFLG_CONST | SFLG_READONLY, true);
    if (param_is_dynarray)
        SetFlag(param_entry.flags, SFLG_DYNAMICARRAY | SFLG_ARRAY, true);
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
        func_entry.funcparamtypes[param_idx] |= STYPE_POINTER;
    if (param_is_const)
        func_entry.funcparamtypes[param_idx] |= STYPE_CONST;
    if (param_is_dynarray)
        func_entry.funcparamtypes[param_idx] |= STYPE_DYNARRAY;

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
        AGS::Symbol curtype = sym.get_type(cursym);
        if (kPP_PreAnalyze == g_PP && g_Sym1.count(cursym) > 0)
            curtype = g_Sym1[cursym].stype;

        switch (curtype)
        {
        default:
            cc_error("Unexpected %s in parameter list", sym.get_name_string(cursym).c_str());
            return -1;

        case SYM_CLOSEPARENTHESIS:
            return 0;

        case SYM_CONST:
        {
            // check in main compiler phase that type must follow
            if (kPP_Main == g_PP && sym.get_type(targ->peeknext()) != SYM_VARTYPE)
            {
                cc_error("Expected a type after 'const'");
                return -1;
            }
            param_is_const = true;
            continue;
        }

        case SYM_VARARGS:
        {
            numparams += VARARGS_INDICATOR;
            if (sym.get_type(targ->getnext()) != SYM_CLOSEPARENTHESIS)
            {
                cc_error("Expected ')' after '...'");
                return -1;
            }
            return 0;
        }

        case SYM_VARTYPE:
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
            int const nexttype = sym.get_type(targ->peeknext());
            if (nexttype == SYM_COMMA)
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

    entry.stype = SYM_FUNCTION;
    SymbolTableEntry &ret_type_entry = GetSymbolTableEntryAnyPhase(return_type);
    entry.ssize = ret_type_entry.ssize;
    if (func_returns_ptr)
        entry.ssize = size_of_a_pointer;
    entry.sscope = numparams - 1;

    entry.funcparamtypes[0] = return_type;
    if (func_returns_ptr)
        entry.funcparamtypes[0] |= STYPE_POINTER;
    if (func_returns_dynarray)
        entry.funcparamtypes[0] |= STYPE_DYNARRAY;
    if (func_is_static)
        SetFlag(entry.flags, SFLG_STATIC, true);
    if (func_is_protected)
        SetFlag(entry.flags, SFLG_PROTECTED, true);

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

    if ((known_info->flags & ~SFLG_IMPORTED) != (this_entry->flags & ~SFLG_IMPORTED))
    {
        cc_error("Qualifiers of function do not match prototype");
        return -1;
    }

    if (FlagIsSet(this_entry->flags, SFLG_ARRAY) && (known_info->arrsize != this_entry->arrsize))
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
            sym.get_name_string(this_entry->funcparamtypes.at(0)).c_str(),
            sym.get_name_string(known_info->funcparamtypes.at(0)).c_str());

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
}


// We're at something like "int foo(", directly before the "("
// Get the symbol after the corresponding ")"
int ParseFuncdecl_GetSymbolAfterParmlist(ccInternalList *targ, AGS::Symbol &symbol)
{
    int pos = targ->pos;

    targ->getnext(); // Eat '('
    AGS::Symbol const stoplist[] = { 0 };
    SkipTo(targ, stoplist, 0); // Skim to matching ')'
 
    if (SYM_CLOSEPARENTHESIS != sym.get_type(targ->getnext()))
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
        body_follows = (SYM_OPENBRACE == sym.get_type(symbol));
    }

    bool const func_is_static_extender = (sym.get_type(targ->peeknext()) == SYM_STATIC);
    bool const func_is_extender = (func_is_static_extender) || (targ->peeknext() == sym.find("this"));

    // Rewrite extender function as if it were a component function of the corresponding struct.
    if (func_is_extender)
    {
        int retval = ParseFuncdecl_ExtenderPreparations(targ, func_is_static_extender, name_of_func, struct_of_func);
        if (retval < 0) return retval;
    }

    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(name_of_func);
    if (entry.stype != SYM_FUNCTION && entry.stype != 0)
    {
        cc_error("'%s' is already defined", sym.get_name_string(name_of_func).c_str());
        return -1;
    }
    if ((!func_returns_ptr) && (!func_returns_dynarray) &&
        FlagIsSet(entry.flags, SFLG_STRUCTTYPE))
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
    if (FlagIsSet(tqs, kTQ_Import) && entry.stype == SYM_FUNCTION && !FlagIsSet(entry.flags, SFLG_IMPORTED))
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
    SetFlag(entry.flags, SFLG_IMPORTED, true);

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
        int thisType = sym.get_type(slist[slist_idx]);
        switch (thisType)
        {
        default:
            break;
        case SYM_OPENBRACKET:
            bracket_nesting_depth++;
            continue;
        case SYM_CLOSEBRACKET:
            if (bracket_nesting_depth > 0)
                bracket_nesting_depth--;
            continue;
        case SYM_OPENPARENTHESIS:
            paren_nesting_depth++;
            continue;
        case SYM_CLOSEPARENTHESIS:
            if (paren_nesting_depth > 0)
                paren_nesting_depth--;
            continue;
        }

        // Continue if we aren't at zero nesting depth, since ()[] take priority
        if (paren_nesting_depth > 0 || bracket_nesting_depth > 0)
            continue;

        if (thisType != SYM_OPERATOR && thisType != SYM_NEW)
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


inline bool is_string(int valtype)
{
    if (valtype == sym.normalStringSym)
        return true;
    if (valtype == (sym.normalStringSym | STYPE_CONST))
        return true;
    if (valtype == (sym.normalCharSym | STYPE_CONST))
        return true;
    return false;
}


// Change the generic operator vcpuOp to the one that is correct for the types
// Also check whether the operator can handle the types at all
int GetOperatorValidForType(int type1, int type2, int &vcpuOp)
{
    if ((type1 == sym.normalFloatSym) || (type2 == sym.normalFloatSym))
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

    if (is_any_type_of_string(type1) || is_any_type_of_string(type2))
    {
        if (type1 != type2)
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

    if ((type1 & (STYPE_POINTER | STYPE_DYNARRAY)) != 0 &&
        (type2 & (STYPE_POINTER | STYPE_DYNARRAY)) != 0)
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
    if ((type1 & (STYPE_POINTER | STYPE_DYNARRAY)) != 0 ||
        (type2 & (STYPE_POINTER | STYPE_DYNARRAY)) != 0)
    {
        cc_error("The operator cannot be applied to values of these types");
        return -1;
    }

    return 0;
}


// Check for a type mismatch in one direction only
bool IsTypeMismatch_Oneway(int typeIs, int typeWantsToBe)
{
    // cannot convert 'void' to anything
    if (typeIs == sym.normalVoidSym)
        return true;

    // Don't convert if no conversion is called for
    if (typeIs == typeWantsToBe)
        return false;

    // cannot convert const to non-const
    if (((typeIs & STYPE_CONST) != 0) && ((typeWantsToBe & STYPE_CONST) == 0))
        return true;

    // can convert String* to const string
    if ((typeIs == (STYPE_POINTER | sym.stringStructSym)) &&
        (typeWantsToBe == (STYPE_CONST | sym.normalStringSym)))
    {
        return false;
    }
    if (is_string(typeIs) != is_string(typeWantsToBe))
        return true;
    if (is_string(typeIs))
        return false;

    // Can convert from NULL to pointer
    if ((typeIs == (STYPE_POINTER | sym.normalNullSym)) && ((typeWantsToBe & STYPE_DYNARRAY) != 0))
        return false;

    // Cannot convert non-dynarray to dynarray or vice versa
    if ((typeIs & STYPE_DYNARRAY) != (typeWantsToBe & STYPE_DYNARRAY))
        return true;

    // From here on, don't mind constness or dynarray-ness
    typeIs &= ~(STYPE_CONST | STYPE_DYNARRAY);
    typeWantsToBe &= ~(STYPE_CONST | STYPE_DYNARRAY);

    // floats cannot mingle with other types
    if ((typeIs == sym.normalFloatSym) != (typeWantsToBe == sym.normalFloatSym))
        return true;

    // Checks to do if at least one is a pointer
    if ((typeIs & STYPE_POINTER) || (typeWantsToBe & STYPE_POINTER))
    {
        // null can be cast to any pointer type
        if (typeIs == (STYPE_POINTER | sym.normalNullSym))
        {
            if (typeWantsToBe & STYPE_POINTER)
                return false;
        }

        // BOTH sides must be pointers
        if ((typeIs & STYPE_POINTER) != (typeWantsToBe & STYPE_POINTER))
            return true;

        // Types need not be identical here, but check against inherited classes
        int isClass = typeIs & ~STYPE_POINTER;
        while (sym.entries[isClass].extends > 0)
        {
            isClass = sym.entries[isClass].extends;
            if ((isClass | STYPE_POINTER) == typeWantsToBe)
                return false;
        }
        return true;
    }

    // Checks to do if at least one is a struct
    bool typeIsIsStruct = (FlagIsSet(sym.entries[typeIs].flags, SFLG_STRUCTTYPE));
    bool typeWantsToBeIsStruct = (FlagIsSet(sym.entries[typeWantsToBe].flags, SFLG_STRUCTTYPE));
    if (typeIsIsStruct || typeWantsToBeIsStruct)
    {
        // The types must match exactly
        if (typeIs != typeWantsToBe)
            return true;

        return false;
    }

    return false;
}

// Check whether there is a type mismatch; if so, give an error
int IsTypeMismatch(int typeIs, int typeWantsToBe, bool orderMatters)
{
    if (!IsTypeMismatch_Oneway(typeIs, typeWantsToBe))
        return 0;
    if (!orderMatters && !IsTypeMismatch_Oneway(typeWantsToBe, typeIs))
        return 0;

    cc_error(
        "Type mismatch: cannot convert '%s' to '%s'",
        sym.get_name_string(typeIs).c_str(),
        sym.get_name_string(typeWantsToBe).c_str());
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

    if (((scrip->ax_val_type & (~STYPE_POINTER)) == sym.stringStructSym) &&
        ((valTypeTo & (~STYPE_CONST)) == sym.normalStringSym))
    {
        scrip->write_cmd1(SCMD_CHECKNULLREG, SREG_AX);
    }

}


// If we need a String but AX contains a normal literal string, 
// then convert AX into a String object and set its type accordingly
void ConvertAXIntoStringObject(ccCompiledScript *scrip, int valTypeTo)
{
    if (((scrip->ax_val_type & (~STYPE_CONST)) == sym.normalStringSym) &&
        ((valTypeTo & (~STYPE_POINTER)) == sym.stringStructSym))
    {
        scrip->write_cmd1(SCMD_CREATESTRING, SREG_AX); // convert AX
        scrip->ax_val_type = STYPE_POINTER | sym.stringStructSym; // set type of AX
    }
}


void AccessData_SetAXScope(ccCompiledScript *scrip, int scrip_idx)
{
    scrip->ax_val_scope = SYM_GLOBALVAR;

    // "null" is considered to be a global var
    if (sym.get_type(scrip_idx) == SYM_NULL)
        return;

    // if it's a parameter, pretend it's a global var
    // this allows it to be returned back from the function
    if (FlagIsSet(sym.entries[scrip_idx].flags, SFLG_PARAMETER))
        return;

    scrip->ax_val_scope = sym.entries[scrip_idx].stype;
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


// [fw] Passing info around through a global variable: That is a HUGE code smell.
int SizeUsedInLastReadCommand = 0;


// Get the bytecode for reading or writing memory of size the_size
inline int GetReadWriteCmdForSize(int the_size, bool write_operation)
{
    // [fw] Passing info around through a global variable: That is a HUGE code smell.
    if (the_size != 0)
        SizeUsedInLastReadCommand = the_size;

    return (write_operation) ? GetWriteCommandForSize(the_size) : GetReadCommandForSize(the_size);
}


int ParseSubexpr_NewIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript &symlist, size_t symlist_len)
{
    if (symlist_len < 2 || sym.get_type(symlist[1]) != SYM_VARTYPE)
    {
        cc_error("Expected a type after 'new'");
        return -1;
    }

    // "new TYPE", nothing following
    if (symlist_len <= 3)
    {
        if (FlagIsSet(sym.entries[symlist[1]].flags, SFLG_BUILTIN))
        {
            cc_error("Built-in type '%s' cannot be instantiated directly", sym.get_name_string(symlist[1]).c_str());
            return -1;
        }
        const size_t size = sym.entries[symlist[1]].ssize;
        scrip->write_cmd2(SCMD_NEWUSEROBJECT, SREG_AX, size);
        scrip->ax_val_type = symlist[1] | STYPE_POINTER;
        return 0;
    }

    // "new TYPE[EXPR]", nothing following
    if (sym.get_type(symlist[2]) == SYM_OPENBRACKET && sym.get_type(symlist[symlist_len - 1]) == SYM_CLOSEBRACKET)
    {
        AGS::Symbol arrayType = symlist[1];

        // Expression for length of array begins after "[", ends before "]"
        // So expression_length = whole_length - 3 - 1
        int retval = ParseSubexpr(scrip, &symlist[3], symlist_len - 4);
        if (retval < 0) return retval;

        if (scrip->ax_val_type != sym.normalIntSym)
        {
            cc_error("Array size must be an int");
            return -1;
        }

        bool isManagedType = false;
        int size = sym.entries[arrayType].ssize;
        if (FlagIsSet(sym.entries[arrayType].flags, SFLG_MANAGED))
        {
            isManagedType = true;
            size = 4;
        }
        else if (FlagIsSet(sym.entries[arrayType].flags, SFLG_STRUCTTYPE))
        {
            cc_error("Cannot create a dynamic array of an unmanaged struct");
            return -1;
        }

        scrip->write_cmd3(SCMD_NEWARRAY, SREG_AX, size, isManagedType);
        scrip->ax_val_type = arrayType | STYPE_DYNARRAY;

        if (isManagedType)
            scrip->ax_val_type |= STYPE_POINTER;

        return 0;
    }

    cc_error("Unexpected characters following 'new %s'", sym.get_name_string(symlist[1]).c_str());
    return -1;
}


// We're parsing an expression that starts with '-' (unary minus)
int ParseSubexpr_UnaryMinusIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript &symlist, size_t symlist_len)
{
    if (symlist_len < 2)
    {
        cc_error("Parse error at '-'");
        return -1;
    }
    // parse the rest of the expression into AX
    int retval = ParseSubexpr(scrip, &symlist[1], symlist_len - 1);
    if (retval < 0) return retval;

    // now, subtract the result from 0 (which negates it)
    int cpuOp = SCMD_SUBREG; // get correct bytecode for the subtraction
    retval = GetOperatorValidForType(scrip->ax_val_type, 0, cpuOp);
    if (retval < 0) return retval;

    scrip->write_cmd2(SCMD_LITTOREG, SREG_BX, 0);
    scrip->write_cmd2(cpuOp, SREG_BX, SREG_AX);
    scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
    return 0;
}


// We're parsing an expression that starts with '!' (boolean NOT)
int ParseSubexpr_NotIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript & symlist, size_t symlist_len)
{

    if (symlist_len < 2)
    {
        cc_error("Parse error at '!'");
        return -1;
    }

    // parse the rest of the expression into AX
    int retval = ParseSubexpr(scrip, &symlist[1], symlist_len - 1);
    if (retval < 0) return retval;

    // negate the result
    // First determine the correct bytecode for the negation
    int cpuOp = SCMD_NOTREG;
    retval = GetOperatorValidForType(scrip->ax_val_type, 0, cpuOp);
    if (retval < 0) return retval;

    // now, NOT the result
    scrip->write_cmd1(SCMD_NOTREG, SREG_AX);
    return 0;
}


// The lowest-binding operator is the first thing in the expression
// This means that the op must be an unary op.
int ParseSubexpr_OpIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript &symlist, size_t symlist_len)
{
    if (sym.get_type(symlist[0]) == SYM_NEW)
    {
        // we're parsing something like "new foo"
        return ParseSubexpr_NewIsFirst(scrip, symlist, symlist_len);
    }

    if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG)
    {
        // we're parsing something like "- foo"
        return ParseSubexpr_UnaryMinusIsFirst(scrip, symlist, symlist_len);
    }

    if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_NOTREG)
    {
        // we're parsing something like "! foo"
        return ParseSubexpr_NotIsFirst(scrip, symlist, symlist_len);
    }

    // All the other operators need a non-empty left hand side
    cc_error("Unexpected operator '%s' without a preceding expression", sym.get_name_string(symlist[0]).c_str());
    return -1;
}


// The lowest-binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
int ParseSubexpr_OpIsSecondOrLater(ccCompiledScript *scrip, size_t op_idx, const AGS::SymbolScript &symlist, size_t symlist_len)
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
        (sym.get_type(symlist[op_idx - 1]) == SYM_OPERATOR))
    {
        // We aren't looking at a subtraction; instead, the '-' is the unary minus of a negative value
        // Thus, the "real" operator must be further to the right, find it.
        op_idx = IndexOfLowestBondingOperator(symlist, op_idx);
        vcpuOperator = sym.entries[symlist[op_idx]].operatorToVCPUCmd();
    }

    // process the left hand side and save result onto stack
    // This will be in vain if we find out later on that there isn't any right hand side,
    // but doing the left hand side first means that any errors will be generated from left to right
    int retval = ParseSubexpr(scrip, &symlist[0], op_idx);
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

    int valtype_leftsize = scrip->ax_val_type;

    scrip->push_reg(SREG_AX);
    retval = ParseSubexpr(scrip, &symlist[op_idx + 1], symlist_len - (op_idx + 1));
    if (retval < 0) return retval;
    scrip->pop_reg(SREG_BX); // <-- note, we pop to BX although we have pushed AX
    // now the result of the left side is in BX, of the right side is in AX

    // Check whether the left side type and right side type match either way
    retval = IsTypeMismatch(scrip->ax_val_type, valtype_leftsize, false);
    if (retval < 0) return retval;

    retval = GetOperatorValidForType(scrip->ax_val_type, valtype_leftsize, vcpuOperator);
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
        scrip->ax_val_type = sym.normalIntSym;

    return 0;
}


int ParseSubexpr_OpenParenthesis(ccCompiledScript *scrip, AGS::SymbolScript & symlist, size_t symlist_len)
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

        case SYM_OPENPARENTHESIS:
            paren_nesting_depth++;
            continue;

        case SYM_CLOSEPARENTHESIS:
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
    int retval = ParseSubexpr(scrip, &symlist[1], matching_paren_idx - 1);
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

// At compile time, we accumulate an offset that is to be added to MAR.
// Make MAR current by adding this offset to MAR now.
inline void AccessData_MakeMARCurrent(ccCompiledScript *scrip, size_t &mar_offset)
{
    if (mar_offset == 0)
        return;
    scrip->write_cmd2(SCMD_ADD, SREG_MAR, mar_offset);
    mar_offset = 0;
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
            const int idx_type = sym.get_type(paramList[paramListIdx]);
            if (idx_type == SYM_CLOSEPARENTHESIS)
                paren_nesting_depth++;
            if (idx_type == SYM_OPENPARENTHESIS)
                paren_nesting_depth--;
            if ((paren_nesting_depth == 0 && idx_type == SYM_COMMA) ||
                (paren_nesting_depth < 0 && idx_type == SYM_OPENPARENTHESIS))
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
        int retval = ParseSubexpr(scrip, &paramList[start_of_this_param], end_of_this_param - start_of_this_param);
        if (retval < 0) return retval;
        if (keep_mar)
            scrip->pop_reg(SREG_MAR);

        if (param_num <= num_func_args) // we know what type to expect
        {
            // If we need a string object ptr but AX contains a normal string, convert AX
            int parameterType = sym.entries[funcSymbol].funcparamtypes[param_num];
            ConvertAXIntoStringObject(scrip, parameterType);

            if (IsTypeMismatch(scrip->ax_val_type, parameterType, true))
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
        const int idx_type = sym.get_type(paramList[paramListIdx]);

        if (idx_type == SYM_OPENPARENTHESIS)
            paren_nesting_depth++;
        if (idx_type == SYM_CLOSEPARENTHESIS)
        {
            paren_nesting_depth--;
            if (paren_nesting_depth == 0)
                break;
        }

        if (paren_nesting_depth == 1 && idx_type == SYM_COMMA)
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
        sym.get_type(paramList[1]) == SYM_CLOSEPARENTHESIS)
    {
        num_supplied_args = 0;
    }

    indexOfCloseParen = paramListIdx;

    if (sym.get_type(paramList[indexOfCloseParen]) != SYM_CLOSEPARENTHESIS)
    {
        cc_error("Missing ')' at the end of the parameter list");
        return -1;
    }

    if (indexOfCloseParen > 0 &&
        sym.get_type(paramList[indexOfCloseParen - 1]) == SYM_COMMA)
    {
        cc_error("Last argument in function call is empty");
        return -1;
    }

    if (indexOfCloseParen < paramListLen - 1 &&
        sym.get_type(paramList[indexOfCloseParen + 1]) != SYM_SEMICOLON)
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
        scrip->fixup_previous(FIXUP_IMPORT);
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
    scrip->fixup_previous(FIXUP_FUNCTION);
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

int AccessData_FunctionCall(ccCompiledScript *scrip, AGS::SymbolScript &symlist, size_t &symlist_len, size_t &mar_offset, AGS::Symbol &rettype)
{
    AGS::Symbol const name_of_func = symlist[0];
    bool const func_is_import = FlagIsSet(sym.entries[name_of_func].flags, SFLG_IMPORTED);

    if (sym.get_type(symlist[1]) != SYM_OPENPARENTHESIS)
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
        if (FlagIsSet(sym.entries[name_of_func].flags, SFLG_STATIC))
            func_uses_this = false;
    }

    if (func_uses_this)
    {
        // Get address of outer into MAR; this is the object that the func will use
        AccessData_MakeMARCurrent(scrip, mar_offset);
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
    rettype = scrip->ax_val_type = sym.entries[name_of_func].funcparamtypes[0];
    scrip->ax_val_scope = SYM_LOCALVAR;

    // At runtime, we have returned from the func call,
    // so we must continue with the object pointer that was in use before the call
    if (func_uses_this)
        scrip->pop_reg(SREG_OP);

    // Note that this function has been accessed at least once
    SetFlag(sym.entries[name_of_func].flags, SFLG_ACCESSED, true);
    return 0;
}

int ParseSubexpr_NoOps(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len)
{

    // Can't check whether type is 0, because e.g. "this" doesn't have a type

    if (sym.get_type(symlist[0]) == SYM_OPENPARENTHESIS)
        return ParseSubexpr_OpenParenthesis(scrip, symlist, symlist_len);

    if (sym.get_type(symlist[0]) == SYM_OPERATOR)
    {
        // check for unary minus
        if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG)
        {
            if (symlist_len == 2) // negative literal
            {
                int retval = ReadDataIntoAX(scrip, &symlist[1], 1);
                if (retval < 0) return retval;
                scrip->write_cmd2(SCMD_SUBREG, SREG_BX, SREG_BX); // 0 -> BX
                scrip->write_cmd2(SCMD_SUBREG, SREG_BX, SREG_AX); // BX - AX -> BX
                scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX); // BX -> AX 
                return 0;
            }

            // If there are bogus tokens after a term that begins with unary minus, 
            // then the problem is the bogus tokens, beginning at index 2. 
            // Otherwise, the problem is the unary minus itself, at index 0. 
            cc_error(
                "Parse error: unexpected '%s'",
                sym.get_name_string(symlist[(symlist_len > 2) ? 2 : 0]).c_str());
            return -1;
        }

        // We don't know this unary operator. "new", perhaps?
        cc_error("Parse error: Unexpected '%s'", sym.get_name_string(symlist[0]).c_str());
        return -1;
    }

    // So this is a literal, variable or func call, simply read it.
    return ReadDataIntoAX(scrip, symlist, symlist_len);
}

int ParseSubexpr(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len)
{
    if (symlist_len == 0)
    {
        cc_error("Internal error: Cannot parse empty subexpression");
        return -1;
    }
    if (sym.get_type(symlist[0]) == SYM_CLOSEBRACKET)
    {
        cc_error("Unexpected ')' at start of expression");
        return -1;
    }

    int lowest_op_idx = IndexOfLowestBondingOperator(symlist, symlist_len);  // can be < 0

    // If the lowest bonding operator is right in front and an integer follows,
    // then it has been misinterpreted so far: 
    // it's really a unary minus. So let's try that.
    // [fw] Why don't we treat literal floats in the same way?
    if ((lowest_op_idx == 0) &&
        (symlist_len > 1) &&
        (sym.get_type(symlist[1]) == SYM_LITERALVALUE) &&
        (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG))
    {
        lowest_op_idx = IndexOfLowestBondingOperator(&symlist[1], symlist_len - 1);
        if (lowest_op_idx >= 0)
            lowest_op_idx++;
    }

    if (lowest_op_idx == 0)
        return ParseSubexpr_OpIsFirst(scrip, symlist, symlist_len);

    if (lowest_op_idx > 0)
        return ParseSubexpr_OpIsSecondOrLater(scrip, static_cast<size_t>(lowest_op_idx), symlist, symlist_len);

    // There is no operator in the expression -- therefore, there will
    // just be a variable name or function call or a parenthesized expression

    return ParseSubexpr_NoOps(scrip, symlist, symlist_len);
}

// symlist starts a bracketed expression; parse it
int AccessData_ArrayIndexIntoAX(ccCompiledScript *scrip, SymbolScript symlist, size_t symlist_len)
{
    if (symlist_len == 0 || SYM_OPENBRACKET != sym.get_type(symlist[0]))
    {
        cc_error("Internal error: '[' expected but not found");
        return -99;
    }
    if(sym.get_type(SYM_CLOSEBRACKET != symlist[symlist_len - 1]))
    {
        cc_error("Internal error: ']' expected but not found");
        return -99;
    }
    int retval = ParseSubexpr(scrip, symlist, symlist_len);
    if (retval < 0) return retval;

    // array index must be convertible to an int
    retval = IsTypeMismatch(scrip->ax_val_type, sym.normalIntSym, true);
    if (retval < 0) return retval;

    return 0;
}


// We access a variable or a component of a struct in order to read or write it.
// This is a simple member of the struct.
int AccessData_StructMember(AGS::Symbol component, size_t &mar_offset, AGS::Symbol &vartype)
{
    SymbolTableEntry &entry = sym.entries[component];
    // since the member has a fixed offset into the structure, don't
    // write out any code to calculate the offset - instead, modify
    // the offset value which will be written to MAR
    mar_offset += entry.soffs;
    vartype = entry.vartype;
    return 0;
}

// Get the symbol for the get or set function corresponding to the attribute given.
int GetAttributeFunction(AGS::Symbol attrib, bool writing, bool indexed, AGS::Symbol &func)
{
    std::string attrib_str = sym.get_name_string(attrib);
    size_t member_part_idx = attrib_str.rfind("::");
    if (std::string::npos == member_part_idx || 0 == member_part_idx)
    {
        cc_error("Internal error: Attribute has no struct name");
        return -99;
    }
    std::string struct_str = attrib_str.substr(0, member_part_idx);
    std::string member_str = attrib_str.substr(member_part_idx + 2);
    if (member_str.length() == 0)
    {
        cc_error("Internal error: Attribute has no component name");
        return -99;
    }

    char const *stem_str = writing ? "::set" : "::get";
    char const *indx_str = indexed ? "i_" : "_";
    
    std::string attr_func_str = struct_str + stem_str + indx_str + member_str;
    func = sym.find_or_add(attr_func_str.c_str());
    return 0;
}

// We call the getter or setter of an attribute
int AccessData_Attribute(ccCompiledScript *scrip, SymbolScript syml, size_t syml_len, bool is_attribute_set_func)
{
    AGS::Symbol const name_of_attribute = syml[0];
    // static attributes generate func calls, the others method calls
    bool const attrib_uses_this = !FlagIsSet(sym.entries[name_of_attribute].flags, SFLG_STATIC);
    // Indexed attributes have a [...] clause
    bool const indexed = (syml_len > 1 && SYM_OPENBRACKET == sym.get_type(syml[1]));

    // Get the appropriate access function (as a symbol)
    AGS::Symbol name_of_func = -1;
    int retval = GetAttributeFunction(name_of_attribute, is_attribute_set_func, indexed, name_of_func);
    if (retval < 0) return retval;
    bool const func_is_import = FlagIsSet(sym.entries[name_of_func].flags, SYM_IMPORT);

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

    if (indexed)
    {
        // The index to be set is in the [...] clause;
        // get it and push it as the first parameter
        if (attrib_uses_this)
            scrip->push_reg(SREG_MAR); // must not be clobbered
        AccessData_ArrayIndexIntoAX(scrip, &syml[1], syml_len - 1);
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
    scrip->ax_val_scope = SYM_LOCALVAR;
    if (is_attribute_set_func)
    {
        scrip->ax_val_type = sym.normalVoidSym;
    }
    else
    {
        scrip->ax_val_type = sym.entries[name_of_attribute].vartype;
        if (FlagIsSet(sym.entries[name_of_attribute].flags, SFLG_DYNAMICARRAY))
            scrip->ax_val_type |= STYPE_DYNARRAY;
        if (FlagIsSet(sym.entries[name_of_attribute].flags, SFLG_POINTER))
            scrip->ax_val_type |= STYPE_POINTER;
        if (FlagIsSet(sym.entries[name_of_attribute].flags, SFLG_CONST))
            scrip->ax_val_type |= STYPE_CONST;
    }

    // Attribute has been accessed
    SetFlag(sym.entries[name_of_attribute].flags, SFLG_ACCESSED, true);
    SetFlag(sym.entries[name_of_func].flags, SFLG_ACCESSED, true);
    return 0;
}

int AccessData_CheckAccess(AGS::Symbol variableSym, VariableSymlist variablePath[], bool writing, bool mustBeWritable, bool write_same_as_read_access, bool isLastClause, size_t vp_idx, bool cannotAssign)
{
    // if one of the struct members in the path is read-only, don't allow it
    if ((writing || mustBeWritable) && write_same_as_read_access)
    {
        // allow writing to read-only pointers if it's actually
        // an attribute being accessed
        if (FlagIsSet(sym.entries[variableSym].flags, SFLG_POINTER) && (!isLastClause))
        { }
        else if (FlagIsSet(sym.entries[variableSym].flags, SFLG_READONLY))
        {
            cc_error("Variable '%s' is read-only", sym.get_name_string(variableSym).c_str());
            return -1;
        }
        else if (FlagIsSet(sym.entries[variableSym].flags, SFLG_WRITEPROTECTED))
        {
            // write-protected variables can only be written by
            // the this ptr
            if ((vp_idx > 0) &&
                FlagIsSet(sym.entries[variablePath[vp_idx - 1].syml[0]].flags, SFLG_THISPTR))
            { }
            else
            {
                cc_error("Variable '%s' is write-protected", sym.get_name_string(variableSym).c_str());
                return -1;
            }
        }
    }

    if (writing && cannotAssign)
    {
        // an entire array or struct cannot be assigned to
        cc_error("Cannot assign to '%s'", sym.get_name_string(variableSym).c_str());
        return -1;
    }

    return 0;
}

enum ResultLocation
{
    kRL_memory,         // value to set/read is in m[MAR]
    kRL_ax_non_attrib,  // value to set/read is in AX, has not been provided by attribute
    kRL_attribute       // value to set is in AX, must be set by attribute setter
};

void AccessData_GetTypeSizeNumOfArrayElement(AGS::Symbol array_type, AGS::Symbol &element_type, size_t &element_size, size_t &element_num)
{
    size_t const size_of_pointer = 4;

    AGS::Symbol const coretype = (array_type & STYPE_MASK);
        
    if (array_type & STYPE_DYNARRAY)
    {
        element_size = (array_type & STYPE_POINTER) ? size_of_pointer : sym.entries[coretype].ssize;
        element_type = (array_type & ~STYPE_DYNARRAY);
        element_num = 0;
        return;
    }

    // Static array
    element_size = sym.entries[coretype].ssize;
    element_type = sym.entries[coretype].vartype;
    element_num = sym.entries[coretype].arrsize;
}

// We're processing some struct component or global or local variable. If an array index follows, process it. 
int AccessData_ProcessAnyArrayIndex(ccCompiledScript *scrip, SymbolScript syml, size_t syml_len, bool ax_pointsto_array, ResultLocation &rl, size_t &mar_offset, AGS::Symbol vartype)
{
    if (0 == syml_len || SYM_OPENBRACKET != sym.get_type(syml[0]))
        return 0;

    // Must be any sort of array; if AX points to it, must be a dynarray
    AGS::Symbol const coretype = vartype & STYPE_MASK;
    bool const is_dynarray = (vartype & STYPE_DYNARRAY);
    if (!is_dynarray && (ax_pointsto_array || sym.entries[vartype].arrsize <= 1))
    {
        cc_error("Array index is only legal after an array");
        return -1;
    }

    AGS::Symbol element_type = 0;
    size_t element_size = 0;
    size_t element_num = 0;
    AccessData_GetTypeSizeNumOfArrayElement(vartype, element_type, element_size, element_num);
    vartype = element_type;
    mar_offset = 0;

    if (ax_pointsto_array)
    {
        scrip->push_reg(SREG_AX); // push memory location of dynarray
    }
    else
    {
        if (is_dynarray)
            scrip->write_cmd1(SCMD_MEMREAD, SREG_MAR); // m[MAR] -> MAR (dereference)
        scrip->push_reg(SREG_MAR); // MAR contains memory location of the array, save it
    }
    AccessData_ArrayIndexIntoAX(scrip, syml, syml_len); // Get array index
    scrip->pop_reg(SREG_MAR); // Get array memory location into MAR
    if (is_dynarray)
        scrip->write_cmd1(SCMD_DYNAMICBOUNDS, SREG_AX);
    else
        scrip->write_cmd2(SCMD_CHECKBOUNDS, SREG_AX, element_num);
    scrip->write_cmd2(SCMD_MUL, SREG_AX, element_size); // Multiply offset with length of one array entry
    scrip->write_cmd2(SCMD_ADDREG, SREG_MAR, SREG_AX); // Add offset
}

int AccessData_GlobalOrLocalVar(ccCompiledScript *scrip, bool is_global, AGS::SymbolScript &syml, size_t &syml_len, ResultLocation rl, AGS::Symbol &vartype)
{
    AGS::Symbol const varname = syml[0];
    SymbolTableEntry &entry = sym.entries[varname];
    AGS::CodeCell const soffs = entry.soffs;
    syml++;
    syml_len--;
    if (is_global)
    {
        scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, soffs);
        scrip->fixup_previous(
            FlagIsSet(entry.flags, SFLG_IMPORTED) ? FIXUP_IMPORT : FIXUP_GLOBALDATA);
    }
    else
    {
        scrip->write_cmd1(SCMD_LOADSPOFFS, scrip->cur_sp - soffs);
    }
    rl = kRL_memory;
    vartype = sym.entries[varname].vartype;

    // Process an array index if it follows
    size_t mar_offset_dummy;
    return AccessData_ProcessAnyArrayIndex(scrip, syml, syml_len, false, rl, mar_offset_dummy, vartype);
}

int AccessData_LitFloat(ccCompiledScript *scrip, AGS::Symbol variableSym, AGS::Symbol &gotValType)
{
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, InterpretFloatAsInt((float)atof(sym.get_name_string(variableSym).c_str())));
    gotValType = sym.normalFloatSym;
    return 0;
}

int AccessData_LitOrConst(ccCompiledScript *scrip, AGS::Symbol variableSym, bool negateLiteral, int &gotValType)
{
    int varSymValue;
    int retval = ParseLiteralOrConstvalue(variableSym, varSymValue, negateLiteral, "Error parsing integer value");
    if (retval < 0) return retval;

    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, varSymValue);
    gotValType = sym.normalIntSym;

    return 0;
}

int AccessData_Null(ccCompiledScript *scrip, AGS::Symbol &gotValType)
{
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    gotValType = sym.normalNullSym | STYPE_POINTER;

    return 0;
}

int AccessData_String(ccCompiledScript *scrip, AGS::Symbol symbol, int &gotValType)
{
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, sym.entries[symbol].soffs);
    scrip->fixup_previous(FIXUP_STRING);
    gotValType = sym.normalStringSym | STYPE_CONST;

    return 0;
}

// We are parsing the first part of a STRUCT.STRUCT.STRUCT... clause.
// This first part can also be a literal or constant
int AccessData_FirstClause(ccCompiledScript *scrip, bool writing, AGS::SymbolScript &syml, size_t &syml_len, ResultLocation &rl, int &scope, AGS::Symbol &vartype)
{
    if (syml_len < 1)
    {
        cc_error("Internal error: Empty variable");
        return -99;
    }

    AGS::Symbol const symbol = syml[0];

    switch (sym.get_type(symbol))
    {
    default:
        cc_error("Unexpected '%s'", sym.get_name_string(symbol).c_str());
        return -1;

    case SYM_CONSTANT:
        scope = SYM_GLOBALVAR;
        rl = kRL_ax_non_attrib;
        return AccessData_LitOrConst(scrip, symbol, false, vartype);

    case SYM_FUNCTION:
    {
        scope = SYM_GLOBALVAR;
        rl = kRL_ax_non_attrib;
        size_t zero_mar_offset = 0;
        return AccessData_FunctionCall(scrip, syml, syml_len, zero_mar_offset, vartype);
    }
            
    case SYM_GLOBALVAR:
        scope = SYM_GLOBALVAR;
        return AccessData_GlobalOrLocalVar(scrip, true, syml, syml_len, rl, vartype);

    case SYM_LITERALFLOAT:
        scope = SYM_GLOBALVAR;
        rl = kRL_ax_non_attrib;
        return AccessData_LitFloat(scrip, symbol, vartype);

    case SYM_LITERALVALUE:
        scope = SYM_GLOBALVAR;
        rl = kRL_ax_non_attrib;
        return AccessData_LitOrConst(scrip, symbol, false, vartype);

    case SYM_LOCALVAR:
        scope = SYM_LOCALVAR;
        if (FlagIsSet(sym.entries[symbol].flags, SFLG_PARAMETER))
            scope = SYM_GLOBALVAR; 
        return AccessData_GlobalOrLocalVar(scrip, false, syml, syml_len, rl, vartype);

    case SYM_NULL:
        scope = SYM_GLOBALVAR;
        rl = kRL_ax_non_attrib;
        return AccessData_Null(scrip, vartype);

    case SYM_STRING:
        scope = SYM_GLOBALVAR;
        rl = kRL_ax_non_attrib;
        return AccessData_String(scrip, symbol, vartype);
    }

    return 0; // Can't reach
}

// We're in a STRUCT.STRUCT.STRUCT... clause.
// Split off the part directly before the first "."; put the length of the first part in syml.
// Let the rest begin immediately after the first "."; put its length into rest_len.
// If the sequence doesn't have any dots, don't modify syml_len and make the second part empty.
int AccessData_GetNextClause(SymbolScript syml, size_t &syml_len, SymbolScript &rest, size_t &rest_len)
{
    AGS::Symbol const stoplist[] = { SYM_DOT };
    rest = syml;
    rest_len = syml_len;
    int retval = SkipToScript(stoplist, 1, rest, rest_len);

    if (0 == rest_len)
        return 0; // Nothing following

    if (SYM_DOT != sym.get_type(rest[0]))
    {
        cc_error("Unexpected '%s'", sym.get_name_string(rest[0]).c_str());
        return -1;
    }

    // Skip the dot itself
    rest++;
    rest_len--;
    return 0;
}

// We're processing a STRUCT.STRUCT. ... clause.
// We've already processed some structs, and the type of the last one is outer.
// Now we process the component of outer.
int AccessData_DoComponent(ccCompiledScript *scrip, bool writing, AGS::SymbolScript &syml, size_t &syml_len, ResultLocation &rl, size_t &mar_offset, AGS::Symbol &vartype, int &scope)
{
    if (syml_len < 1)
    {
        cc_error("Internal error: Empty variable");
        return -99;
    }

    AGS::Symbol const component = syml[0];
    AGS::Symbol component_type = sym.get_type(component);
    if (FlagIsSet(sym.entries[component].flags, SFLG_ATTRIBUTE)) // Can only be recognized by flags
        component_type = SYM_ATTRIBUTE;

    int retval;
    switch (component_type)
    {
    default:
        cc_error("Unexpected '%s'", sym.get_name_string(component).c_str());
        return -1;

    case SYM_ATTRIBUTE:
    {
        if (writing)
        {
            // We cannot process this here so return to the assignment that
            // this attribute was originally called from
            rl = kRL_attribute;
            return 0;
        }
        rl = kRL_ax_non_attrib;
        AccessData_MakeMARCurrent(scrip, mar_offset);
        bool const is_attribute_get_func = false;
        return AccessData_Attribute(scrip, syml, syml_len, is_attribute_get_func);
    }

    case SYM_FUNCTION:
        rl = kRL_ax_non_attrib;
        scope = SYM_GLOBALVAR;
        retval = AccessData_FunctionCall(scrip, syml, syml_len, mar_offset, vartype);
        if (retval < 0) return retval;
        return AccessData_ProcessAnyArrayIndex(scrip, syml, syml_len, true, rl,  mar_offset, vartype);

    case SYM_STRUCTMEMBER:
        rl = kRL_memory;
        retval = AccessData_StructMember(component, mar_offset, vartype);
        if (retval < 0) return retval;
        return AccessData_ProcessAnyArrayIndex(scrip, syml, syml_len, false, rl, mar_offset, vartype);
    }
    
    return 0; // Can't reach
}

// Access a variable, constant, literal, struct.component.component sequence, func call etc.
// Result is in AX or m[MAR], dependent on rl. Type is in vartype.
// At end of function, syml and syml_len will point to the part of the symbol string
// that has not been processed yet
// NOTE: If this selects an attribute for writing, then the corresponding function will
// _not_ be called and syml[0] will be the attribute.
int AccessData(ccCompiledScript *scrip, bool writing, AGS::SymbolScript &syml, size_t &syml_len, ResultLocation &rl, int &scope, AGS::Symbol &vartype)
{
    if (syml_len <= 0)
    {
        cc_error("Internal error: empty expression");
        return -99;
    }

    AGS::Symbol valtype;
    int retval = AccessData_FirstClause(scrip, writing, syml, syml_len,  rl, scope, valtype);
    if (retval < 0) return retval;

    size_t mar_offset = 0;
    while (syml_len > 0 && SYM_DOT == sym.get_type(syml[0]))
    {
        // Invariant: Here, the outer structure begins at m[MAR + mar_offset]
        // We accumulate the offsets into mar_offset as far as possible and only
        // then add them to MAR, for efficiency.
        long const flags = sym.entries[valtype & STYPE_MASK].flags;
        if (!FlagIsSet(flags, SFLG_STRUCTTYPE))
        {
            cc_error("A dot must follow a struct type");
            return -1;
        }
        if (rl != kRL_memory)
        {
            if (0 == (valtype & SFLG_POINTER))
            {
                cc_error("Cannot access a component here");
                return -1;
            }
            scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        }

        if (valtype & SFLG_POINTER)
        {
            // Dereference the pointer in m[MAR + mar_offset].
            scrip->write_cmd(SCMD_CHECKNULL);
            AccessData_MakeMARCurrent(scrip, mar_offset);
            scrip->write_cmd1(SCMD_MEMREAD, SREG_MAR);
            valtype &= ~SFLG_POINTER;
        }

        SymbolScript rest;
        size_t rest_len;
        int retval = AccessData_GetNextClause(syml, syml_len, rest, rest_len);
        if (retval < 0) return retval;

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        bool const clause_is_last = (rest_len == 0);
        retval = AccessData_DoComponent(scrip, clause_is_last && writing, syml, syml_len, rl, mar_offset, valtype, scope);
        if (retval < 0) return retval;
        if (syml_len > 0)
        {
            cc_error("Internal error: Unexpected symbols in expression");
            return -99;
        }
        syml = rest;
        syml_len = rest_len;
    }

    if (rl != kRL_memory)
        return 0;

    if (mar_offset != 0)
    {
        scrip->write_cmd(SCMD_CHECKNULL);
        AccessData_MakeMARCurrent(scrip, mar_offset);
    }

    return 0;
}

// location to modify is m[MAR]; write AX to it
int AccessData_WriteMemory(ccCompiledScript * scrip, const size_t &syml_len)
{
    scrip->write_cmd1(SCMD_MEMWRITE, SREG_AX);

    if (syml_len > 0)
    {
        cc_error("Internal error: Unexpected symbols at end of expression");
        return -99;
    }
    return 0;
}

// We are typically in an assignment LHS = RHS; the RHS has already been
// evaluated, and the result of that evaluation is in AX.
// Store AX into the memory location that corresponds to LHS, or
// call the attribute function corresponding to LHS.
int AccessDataForWriting(ccCompiledScript *scrip, SymbolScript syml, size_t syml_len)
{
    // AX contains the result of evaluating the RHS of the assignment
    // Save on the stack so that it isn't clobbered
    AGS::Symbol rhstype = scrip->ax_val_type;
    int rhsscope = scrip->ax_val_scope;
    scrip->write_cmd1(SCMD_PUSHREG, SREG_AX);

    ResultLocation rl;
    AGS::Symbol lhstype;
    int lhsscope;
    AGS::Symbol rattrib;
    int retval = AccessData(scrip, true, syml, syml_len, rl, lhsscope, lhstype);
    if (retval < 0) return retval;

    if (kRL_ax_non_attrib == rl)
    {
        // This is a non-writable value (e.g., a literal)
        cc_error("Cannot assign to this value");
        return -1;
    }
        
    scrip->write_cmd1(SCMD_POPREG, SREG_AX);
    scrip->ax_val_type = rhstype;
    scrip->ax_val_scope = rhsscope;

    ConvertAXIntoStringObject(scrip, lhstype);
    if (IsTypeMismatch_Oneway(rhstype, lhstype))
    {
        cc_error(
            "Cannot assign a type '%s' value to a type '%s' variable",
            sym.get_name_string(rhstype).c_str(),
            sym.get_name_string(lhstype).c_str());
        return -1;
    }

    if (kRL_memory == rl)
        return AccessData_WriteMemory(scrip, syml_len);

    bool const is_attribute_set_func = true;
    return AccessData_Attribute(scrip, syml, syml_len, is_attribute_set_func);
}

// Expect an expression that will return
// either a memory location where the result is stored,
// or the result in AX
int AccessDataForReading(ccCompiledScript *scrip, SymbolScript syml, size_t syml_len)
{
    ResultLocation rl;
    AGS::Symbol vartype;

    int scope;
    int retval = AccessData(scrip, false, syml, syml_len, rl, scope, vartype);
    if (retval < 0) return retval;

    if (kRL_ax_non_attrib == rl || kRL_attribute == rl)
        return 0;

    // Result is in m[MAR], so put it into AX
    scrip->write_cmd1(SCMD_MEMREAD, SREG_AX);
    scrip->ax_val_type = vartype;
    scrip->ax_val_scope = scope;
    return 0;
}

int ReadDataIntoAX(ccCompiledScript *scrip, AGS::SymbolScript syml, size_t syml_len, bool negate)
{
    ResultLocation rl;
    int scope;
    AGS::Symbol vartype;
    int retval = AccessData(scrip, false, syml, syml_len, rl, scope, vartype);
    if (retval < 0) return retval;
    if (kRL_memory != rl)
        return 0;

    // Get the result into AX
    scrip->write_cmd1(SCMD_MEMREAD, SREG_AX);
    scrip->ax_val_type = vartype;
    scrip->ax_val_scope = scope;
    return 0;
}

int BufferExpression(ccInternalList *source, size_t script_idx, ccInternalList *dest)
{
    size_t source_len = script_idx - source->pos;

    // Reserve memory for destination script and copy source into destination
    dest->script = static_cast<AGS::SymbolScript>(malloc(source_len * sizeof(AGS::Symbol)));
    if (!dest->script)
    {
        cc_error("Out of memory");
        return -1;
    }

    // Copy the content over, skipping METAs
    size_t dest_idx = 0;
    for (size_t source_idx = source->pos; source_idx < script_idx; source_idx++)
    {
        if (source->script[source_idx] == SCODE_META)
        {
            source_idx += 2;
            continue;
        }
        dest->script[dest_idx++] = source->script[source_idx];
    }
    dest->length = dest_idx;
    dest->pos = 0;
    dest->allocated = source_len;

    return 0;
}

// Read the symbols of an expression and buffer them into expr_script
// At end of routine, the cursor will be positioned in such a way
// that targ->getnext() will get the symbol after the expression
int BufferExpression(ccInternalList *targ, ccInternalList &expr_script)
{
    size_t paren_nesting_depth = 0;

    for (AGS::Symbol nextsym = targ->peeknext(); 
        nextsym >= 0;
        nextsym = targ->peeknext())
    {
        size_t const pos = targ->pos; // for backing up if necessary

        AGS::Symbol const nexttype = sym.get_type(nextsym);
        if (SYM_OPENPARENTHESIS == nexttype || SYM_OPENBRACKET == nexttype || SYM_OPENBRACE == nexttype)
            ++paren_nesting_depth;
        if (SYM_CLOSEPARENTHESIS == nexttype || SYM_CLOSEBRACKET == nexttype || SYM_CLOSEBRACE == nexttype)
            if (--paren_nesting_depth < 0)
                break; // this symbol can't be part of the current expression

        if (SYM_NEW == nexttype)
        {
            // This is only allowed if a type follows
            targ->getnext(); // eat the nexttype
            AGS::Symbol nextnextsym = targ->getnext();
            if (SYM_VARTYPE == sym.get_type(nextnextsym))
            {
                expr_script.write(nextsym);
                expr_script.write(nextnextsym);
                continue;
            }
            targ->pos = pos; // Back up so that the nexttype is still unread
            break;
        }

        if (SYM_VARTYPE == nexttype)
        {
            // This is only allowed if a dot follows
            targ->getnext(); // eat the nexttype
            AGS::Symbol nextnextsym = targ->getnext();
            if (SYM_DOT == sym.get_type(nextnextsym))
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
    return ParseSubexpr(scrip, expr_script.script, expr_script.length);
}

int ParseAssignment_LHS(ccCompiledScript *scrip, ccInternalList *lhs, ResultLocation &rl, AGS::Symbol &lhstype)
{
    int scope;
    size_t lhslength = (lhs->length < 0) ? 0 : lhs->length;
    int retval = AccessData(scrip, false, lhs->script, lhslength, rl, scope, lhstype);
    if (retval < 0) return retval;
    if (lhslength > 0)
    {
        cc_error("Internal error: Unexpected symbols following expression");
        return -99;
    }
    if (kRL_ax_non_attrib == rl)
    {
        cc_error("This operator cannot be used here");
        return -1;
    }
    if (kRL_memory == rl)
    {
        // write memory to AX
        scrip->ax_val_type = lhstype;
        scrip->ax_val_scope = scope;
        AGS::Symbol memread = GetReadCommandForSize(sym.entries[lhstype].ssize);
        scrip->write_cmd1(memread, SREG_AX);
    }
    return 0;
}

// "var = expression"; lhs is the variable
int ParseAssignment_Assign(ccInternalList *targ, ccCompiledScript *scrip, ccInternalList *lhs)
{
    ParseExpression(targ, scrip); // RHS of the assignment
    return AccessDataForWriting(scrip, lhs->script, static_cast<size_t>(lhs->length));
}

// We compile something like "var += expression"
int ParseAssignment_MAssign(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol ass_symbol, ccInternalList *lhs)
{
    // Parse RHS
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    scrip->write_cmd1(SCMD_PUSHREG, SREG_AX);
    AGS::Symbol rhstype = scrip->ax_val_type;

    // Parse LHS
    ResultLocation rl;
    AGS::Symbol lhstype;
    retval = ParseAssignment_LHS(scrip, lhs, rl, lhstype);
    if (retval < 0) return retval;

    // Use the operator on LHS and RHS
    int cpuOp = sym.entries[ass_symbol].ssize;
    retval = GetOperatorValidForType(lhstype, rhstype, cpuOp);
    if (retval < 0) return retval;
    scrip->write_cmd1(SCMD_POPREG, SREG_BX); 
    scrip->write_cmd2(cpuOp, SREG_AX, SREG_BX);

    if (kRL_memory == rl)
    {
        // write AX back to memory
        AGS::Symbol memwrite = GetWriteCommandForSize(sym.entries[lhstype].ssize);
        scrip->write_cmd1(memwrite, SREG_AX);
        return 0;
    }

    return ParseAssignment_Assign(targ, scrip, lhs);
}

// "var++" or "var--"
int ParseAssignment_SAssign(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol ass_symbol, ccInternalList *lhs)
{
    ResultLocation rl;
    AGS::Symbol lhstype;
    int retval = ParseAssignment_LHS(scrip, lhs, rl, lhstype);
    if (retval < 0) return retval;

    // increment or decrement AX, using the correct bytecode
    int cpuOp = sym.entries[ass_symbol].ssize;
    retval = GetOperatorValidForType(lhstype, 0, cpuOp);
    if (retval < 0) return retval;
    scrip->write_cmd2(cpuOp, SREG_AX, 1);

    if (kRL_memory == rl)
    {
        // write AX back to memory
        AGS::Symbol memwrite = GetWriteCommandForSize(sym.entries[lhstype].ssize);
        scrip->write_cmd1(memwrite, SREG_AX);
        return 0;
    }
    
    return ParseAssignment_Assign(targ, scrip, lhs);
}

int ParseAssignment0(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol ass_symbol, ccInternalList *lhs)
{
    switch (sym.get_type(ass_symbol))
    {
    default: // can't happen
        cc_error("Internal error: Illegal assignment symbol found");
        return -99;

    case SYM_ASSIGN:
        return ParseAssignment_Assign(targ, scrip, lhs);

    case SYM_MASSIGN:
        return ParseAssignment_MAssign(targ, scrip, ass_symbol, lhs);

    case SYM_SASSIGN:
        return ParseAssignment_SAssign(targ, scrip, ass_symbol, lhs);
    }
}

// We've read a variable or selector of a struct into symlist[], the last identifying component is in cursym.
// An assignment symbol is following. Compile the assignment.
int ParseAssignment(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol ass_symbol, AGS::Symbol statementEndSymbol, ccInternalList *lhs)
{
    // Do the assignment
    int retval = ParseAssignment0(targ, scrip, ass_symbol, lhs);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) == statementEndSymbol)
        return 0;

    cc_error("Expected '%s'", sym.get_name_string(statementEndSymbol).c_str());
    return -1;
}

// true if the symbol is "int" and the like.
inline bool sym_is_predef_typename(AGS::Symbol symbl)
{
    return (symbl >= 0 && symbl <= sym.normalFloatSym);
}

int ParseVardecl_InitialValAssignment_ToLocal(ccInternalList *targ, ccCompiledScript *scrip, int completeVarType)
{
    // Parse and compile the expression
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;

    // If we need a string object ptr but AX contains a normal string, convert AX
    ConvertAXIntoStringObject(scrip, completeVarType);

    // Check whether the types match
    retval = IsTypeMismatch(scrip->ax_val_type, completeVarType, true);
    if (retval < 0) return retval;
    return 0;
}

int ParseVardecl_InitialValAssignment_ToGlobalFloat(ccInternalList *targ, bool is_neg, void *& initial_val_ptr)
{
    // initialize float
    if (sym.get_type(targ->peeknext()) != SYM_LITERALFLOAT)
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
int ParseVardecl_InitialValAssignment_ToGlobal(ccInternalList *targ, long varname, void *&initial_val_ptr)
{
    initial_val_ptr = nullptr;

    if (FlagIsSet(sym.entries[varname].flags, SFLG_POINTER))
    {
        cc_error("Cannot assign an initial value to a global pointer");
        return -1;
    }

    if (FlagIsSet(sym.entries[varname].flags, SFLG_DYNAMICARRAY))
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
    if (sym.entries[varname].vartype == sym.normalFloatSym)
        return ParseVardecl_InitialValAssignment_ToGlobalFloat(targ, is_neg, initial_val_ptr);
    return ParseVardecl_InitialValAssignment_ToGlobalNonFloat(targ, is_neg, initial_val_ptr);
}

// We have accepted something like "int var" and we are reading "= val"
int ParseVardecl_InitialValAssignment(ccInternalList *targ, ccCompiledScript *scrip, int next_type, Globalness isglobal, long varname, int type_of_defn, void *&initial_val_ptr, FxFixupType &need_fixup)
{
    targ->getnext();  // skip the '='

    initial_val_ptr = nullptr; // there is no initial value
    if (isglobal == kGl_GlobalImport)
    {
        cc_error("Cannot set initial value of imported variables");
        return -1;
    }
    if ((sym.entries[varname].flags & (SFLG_ARRAY | SFLG_DYNAMICARRAY)) == SFLG_ARRAY)
    {
        cc_error("Cannot assign a value to an array");
        return -1;
    }
    if (FlagIsSet(sym.entries[varname].flags, SFLG_ISSTRING))
    {
        cc_error("Cannot assign a value to a string, use StrCopy");
        return -1;
    }

    int completeVarType = type_of_defn;
    if (FlagIsSet(sym.entries[varname].flags, SFLG_POINTER))
        completeVarType |= STYPE_POINTER;
    if (FlagIsSet(sym.entries[varname].flags, SFLG_DYNAMICARRAY))
        completeVarType |= STYPE_DYNARRAY;

    if (isglobal == kGl_Local)
    {
        // accept an expression of the appropriate type
        // This is compiled as an assignment, so there is no initial value to return here
        initial_val_ptr = nullptr;
        int retval = ParseVardecl_InitialValAssignment_ToLocal(targ, scrip, completeVarType);
        if (retval < 0) return retval;

        need_fixup = kFx_FixupFunction;   // code[fixup] += &code[0]
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
void ParseVardecl_Var2SymTable(int var_name, Globalness is_global, bool is_pointer, int size_of_defn, int type_of_defn)
{
    sym.entries[var_name].extends = 0;
    sym.entries[var_name].stype = (is_global == kGl_Local) ? SYM_LOCALVAR : SYM_GLOBALVAR;
    sym.entries[var_name].ssize = size_of_defn;
    sym.entries[var_name].arrsize = 1;
    sym.entries[var_name].vartype = type_of_defn;
    if (is_pointer)
        SetFlag(sym.entries[var_name].flags, SFLG_POINTER, true);
}

// we have accepted something like "int a" and we're expecting "["
int ParseVardecl_ArrayDecl(ccInternalList *targ, int var_name, int type_of_defn, int &array_size, int &size_of_defn)
{
    // an array
    targ->getnext();  // skip the [

    if (sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
    {
        SetFlag(sym.entries[var_name].flags, SFLG_DYNAMICARRAY, true);
        array_size = 0;
        size_of_defn = 4;
    }
    else
    {
        if (FlagIsSet(sym.entries[type_of_defn].flags, SFLG_HASDYNAMICARRAY))
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

        size_of_defn *= array_size;
    }
    SetFlag(sym.entries[var_name].flags, SFLG_ARRAY, true);
    sym.entries[var_name].arrsize = array_size;

    if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
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
    fixup_needed = kFx_FixupDataData;
    return 0;
}

int ParseVardecl_StringDecl_Local(ccCompiledScript *scrip, AGS::Symbol var_name, void *&initial_value_ptr)
{
    // Note: We can't use scrip->cur_sp since we don't know if we'll be in a nested function call at the time
    initial_value_ptr = nullptr;

    SetFlag(sym.entries[var_name].flags, SFLG_STRBUFFER, true); // Note in the symbol table that this var is a stringbuffer
    scrip->cur_sp += STRING_LENGTH; // reserve STRING_LENGTH bytes for the var on the stack

                                        // CX will contain the address of the new memory, which will be added to the stack
    scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_CX); // Copy current stack pointer to CX
    scrip->write_cmd2(SCMD_ADD, SREG_SP, STRING_LENGTH); // write code for reserving STRING LENGTH bytes 
    return 0;
}

int ParseVardecl_StringDecl(ccCompiledScript *scrip, AGS::Symbol var_name, Globalness is_global, void *&initial_value_ptr, FxFixupType &fixup_needed)
{
    if (ccGetOption(SCOPT_OLDSTRINGS) == 0)
    {
        cc_error("Type 'string' is no longer supported; use String instead");
        return -1;
    }

    if (FlagIsSet(sym.entries[var_name].flags, SFLG_DYNAMICARRAY))
    {
        cc_error("Arrays of old-style strings are not supported");
        return -1;
    }

    if (is_global == kGl_GlobalImport)
    {
        // cannot import, because string is really char *, and the pointer won't resolve properly
        cc_error("Cannot import string; use char[] instead");
        return -1;
    }

    initial_value_ptr = nullptr;
    fixup_needed = kFx_NoFixup;

    SetFlag(sym.entries[var_name].flags, SFLG_ISSTRING, true);

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
    if (fixup_needed == kFx_FixupFunction)
    {
        // expression worked out into ax
        if (FlagIsSet(sym.entries[var_name].flags, (SFLG_POINTER | SFLG_DYNAMICARRAY)))
            scrip->write_cmd1(SCMD_MEMINITPTR, SREG_AX);
        else
            scrip->write_cmd1(GetReadWriteCmdForSize(size_of_defn, true), SREG_AX);
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

    if (fixup_needed == kFx_FixupDataData)
    {
        SetFlag(sym.entries[var_name].flags, SFLG_STRBUFFER, true);
        scrip->fixup_previous(FIXUP_STACK);
    }

    // Allocate space on the stack
    if (size_of_defn > 0)
    {
        scrip->cur_sp += size_of_defn;
        scrip->write_cmd2(SCMD_ADD, SREG_SP, size_of_defn);
    }
}


int ParseVardecl_CheckIllegalCombis(int type_of_defn, bool is_pointer, Globalness is_global)
{
    if (FlagIsSet(sym.entries[type_of_defn].flags, SFLG_MANAGED) && (!is_pointer) && (is_global != kGl_GlobalImport))
    {
        // managed structs must be allocated via ccRegisterObject,
        // and cannot be declared normally in the script (unless imported)
        cc_error("Cannot declare local instance of managed type");
        return -1;
    }

    if (type_of_defn == sym.normalVoidSym)
    {
        cc_error("'void' not a valid variable type");
        return -1;
    }

    if (!FlagIsSet(sym.entries[type_of_defn].flags, SFLG_MANAGED) && (is_pointer) && (kGl_GlobalImport != is_global))
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

    if ((known_info->flags & ~SFLG_IMPORTED) != (this_entry->flags & ~SFLG_IMPORTED))
    {
        cc_error("Qualifiers of this variable do not match prototype");
        return -1;
    }

    if (FlagIsSet(this_entry->flags, SFLG_ARRAY) && (known_info->arrsize != this_entry->arrsize))
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
    int var_name,
    int type_of_defn,  // i.e. "void" or "int"
    int next_type, // type of the following symbol
    Globalness is_global,
    bool is_pointer,
    bool &another_var_follows,
    void *initial_value_ptr)
{
    SymbolTableEntry known_info;
    if (sym.get_type(var_name) != 0)
        CopyKnownSymInfo(sym.entries[var_name], known_info);

    int retval = ParseVardecl_CheckIllegalCombis(type_of_defn, is_pointer, is_global);
    if (retval < 0) return retval;

    // this will become true iff we gobble a "," after the defn and expect another var of the same type
    another_var_follows = false;

    // will contain the initial value of the var being declared
    initial_value_ptr = nullptr;
    FxFixupType fixup_needed = kFx_NoFixup;
    int array_size = 1;

    int size_of_defn = sym.entries[type_of_defn].ssize;
    if (is_pointer)
        size_of_defn = 4;

    // Enter the variable into the symbol table
    ParseVardecl_Var2SymTable(var_name, is_global, is_pointer, size_of_defn, type_of_defn);

    if (next_type == SYM_OPENBRACKET)
    {
        // Parse the bracketed expression; determine whether it is dynamic; if not, determine the size
        int retval = ParseVardecl_ArrayDecl(targ, var_name, type_of_defn, array_size, size_of_defn);
        if (retval < 0) return retval;

        next_type = sym.get_type(targ->peeknext());
        initial_value_ptr = calloc(1, size_of_defn + 1);
    }
    else if (size_of_defn > 4)
    {
        // initialize the struct to all zeros
        initial_value_ptr = calloc(1, size_of_defn + 1);
    }
    else if (sym.get_name_string(type_of_defn).compare("string") == 0)
    {
        int retval = ParseVardecl_StringDecl(scrip, var_name, is_global, initial_value_ptr, fixup_needed);
        if (retval < 0) return retval;
    }
    else
    {
        // a kind of atomic value (char, int, long, float, pointer) -- initialize to 0
        initial_value_ptr = calloc(1, sizeof(long));
    }

    retval = ParseVardecl_CheckThatKnownInfoMatches(&sym.entries[var_name], &known_info);
    if (retval < 0) return retval;

    // initial assignment, i.e. a clause "= value" following the definition
    if (next_type == SYM_ASSIGN)
    {
        if (initial_value_ptr)
            free(initial_value_ptr);
        int retval = ParseVardecl_InitialValAssignment(targ, scrip, next_type, is_global, var_name, type_of_defn, initial_value_ptr, fixup_needed);
        if (retval < 0) return retval;
        next_type = sym.get_type(targ->peeknext());
    }

    switch (is_global)
    {
    default:
        cc_error("Internal error: Wrong value '%d' of is_global", is_global);
        return -99;

    case kGl_GlobalImport:
        if (g_GIVM[var_name])
            break; // Skip this since the global non-import decl will come later
        sym.entries[var_name].soffs = scrip->add_new_import(sym.get_name_string(var_name).c_str());
        SetFlag(sym.entries[var_name].flags, SFLG_IMPORTED, true);
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
        if (fixup_needed == kFx_FixupDataData)
            scrip->add_fixup(sym.entries[var_name].soffs, FIXUP_DATADATA);
        break;

    case kGl_Local:
        sym.entries[var_name].soffs = scrip->cur_sp;

        // Output the code for defining the local and initializing it
        ParseVardecl_CodeForDefnOfLocal(scrip, var_name, fixup_needed, size_of_defn, initial_value_ptr);
    }

    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }
    if (next_type == SYM_COMMA || next_type == SYM_SEMICOLON)
    {
        targ->getnext();  // skip the comma or semicolon
        another_var_follows = (next_type == SYM_COMMA);
        return 0;
    }

    cc_error("Expected ',' or ';' instead of '%s'", sym.get_name_string(targ->peeknext()).c_str());
    return -1;
}

// wrapper around ParseVardecl0() to prevent memory leakage
inline int ParseVardecl(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    int var_name,
    int type_of_defn,  // i.e. "void" or "int"
    int next_type, // type of the following symbol
    Globalness is_global,
    bool is_pointer,
    bool &another_var_follows)
{
    void *initial_value_ptr = nullptr;

    int retval = ParseVardecl0(targ, scrip, var_name, type_of_defn, next_type, is_global, is_pointer, another_var_follows, initial_value_ptr);

    if (initial_value_ptr != nullptr)
        free(initial_value_ptr);

    return retval;
}


void ParseOpenbrace_FuncBody(ccCompiledScript *scrip, AGS::Symbol name_of_func, int struct_of_func, bool is_noloopcheck, AGS::NestingStack *nesting_stack)
{
    // write base address of function for any relocation needed later
    scrip->write_cmd1(SCMD_THISBASE, scrip->codesize);
    if (is_noloopcheck)
        scrip->write_cmd(SCMD_LOOPCHECKOFF);

    // loop through all parameters and check whether they are pointers
    // the first entry is the return value, so skip that
    const size_t num_args = sym.entries[name_of_func].get_num_args();
    for (size_t pa = 1; pa <= num_args; pa++)
    {
        if (sym.entries[name_of_func].funcparamtypes[pa] & (STYPE_POINTER | STYPE_DYNARRAY))
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

    // non-static member function -- declare "this" ptr
    if ((struct_of_func) && (!FlagIsSet(sym.entries[name_of_func].flags, SFLG_STATIC)))
    {
        const AGS::Symbol thisSym = sym.find("this");
        if (thisSym > 0)
        {
            int varsize = 4;
            // declare "this" inside member functions
            sym.entries[thisSym].stype = SYM_LOCALVAR;
            sym.entries[thisSym].vartype = struct_of_func;
            sym.entries[thisSym].ssize = varsize; // pointer to struct
            sym.entries[thisSym].sscope = static_cast<short>(nesting_stack->Depth() - 1);
            sym.entries[thisSym].flags = SFLG_READONLY | SFLG_ACCESSED | SFLG_POINTER | SFLG_THISPTR;
            // declare as local variable
            sym.entries[thisSym].soffs = scrip->cur_sp;
            scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR);
            // first of all, write NULL to the pointer so that
            // it doesn't try and free it in the following call
            scrip->write_cmd2(SCMD_WRITELIT, varsize, 0);
            // write the OP location into the variable
            //scrip->write_cmd1(SCMD_MEMINITPTR, SREG_OP);
            // the "this" ptr is allocated a space on the stack,
            // even though it's not used (since accesses go directly
            // via the OP)
            scrip->cur_sp += varsize;
            scrip->write_cmd2(SCMD_ADD, SREG_SP, varsize);
        }
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

    if (nesting_level == 1)
    {
        // Code  trace reaches end of a function
        // Emit code that returns 0
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    }

    FreePointersOfLocals(scrip, nesting_level - 1);
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
        // We've just finished the body of the current function.
        name_of_current_func = -1;
        struct_of_current_func = -1;

        // Write code to return from the function.
        // This pops the return address from the stack, 
        // so adjust the "high point" of stack allocation appropriately
        scrip->write_cmd(SCMD_RET);
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
    }
    break;

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
    entry.stype = SYM_VARTYPE;
    SetFlag(entry.flags, SFLG_STRUCTTYPE, true);
    entry.ssize = 0;

    if (FlagIsSet(tqs, kTQ_Managed))
        SetFlag(entry.flags, SFLG_MANAGED, true);

    if (FlagIsSet(tqs, kTQ_Builtin))
        SetFlag(entry.flags, SFLG_BUILTIN, true);

    if (FlagIsSet(tqs, kTQ_Autoptr))
        SetFlag(entry.flags, SFLG_AUTOPTR, true);
}


// We have accepted something like "struct foo" and are waiting for "extends"
int ParseStruct_ExtendsClause(ccInternalList *targ, AGS::Symbol stname, AGS::Symbol &extendsWhat, size_t &size_so_far)
{
    targ->getnext(); // gobble "extends"
    extendsWhat = targ->getnext(); // name of the extended struct

    if (kPP_PreAnalyze == g_PP)
        return 0; // No further analysis necessary in first phase

    if (sym.get_type(extendsWhat) != SYM_VARTYPE)
    {
        cc_error("Expected a struct type here");
        return -1;
    }
    SymbolTableEntry &struct_entry = sym.entries[stname];
    SymbolTableEntry const &extends_entry = sym.entries[extendsWhat];

    if (!FlagIsSet(extends_entry.flags, SFLG_STRUCTTYPE))
    {
        cc_error("Must extend a struct type");
        return -1;
    }
    if (!FlagIsSet(extends_entry.flags, SFLG_MANAGED) && FlagIsSet(struct_entry.flags, SFLG_MANAGED))
    {
        cc_error("Managed struct cannot extend the unmanaged struct '%s'", sym.get_name_string(extendsWhat).c_str());
        return -1;
    }
    if (FlagIsSet(extends_entry.flags, SFLG_MANAGED) && !FlagIsSet(struct_entry.flags, SFLG_MANAGED))
    {
        cc_error("Unmanaged struct cannot extend the managed struct '%s'", sym.get_name_string(extendsWhat).c_str());
        return -1;
    }
    if (FlagIsSet(extends_entry.flags, SFLG_BUILTIN) && !FlagIsSet(struct_entry.flags, SFLG_BUILTIN))
    {
        cc_error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", sym.get_name_string(extendsWhat).c_str());
        return -1;
    }
    size_so_far = static_cast<size_t>(extends_entry.ssize);
    struct_entry.extends = extendsWhat;

    return 0;
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
        case SYM_ATTRIBUTE:      SetFlag(tqs, kTQ_Attribute, true); continue;
        case SYM_IMPORT:         SetFlag(tqs, kTQ_Import1, true);   continue;
        case SYM_PROTECTED:      SetFlag(tqs, kTQ_Protected, true); continue;
        case SYM_READONLY:       SetFlag(tqs, kTQ_Readonly, true);  continue;
        case SYM_STATIC:         SetFlag(tqs, kTQ_Static, true);    continue;
        case SYM_WRITEPROTECTED: SetFlag(tqs, kTQ_Writeprotected, true);  continue;
        }
        break;
    };

    return;
}

int ParseStruct_IsMemberTypeIllegal(ccInternalList *targ, int stname, AGS::Symbol cursym, bool member_is_pointer, bool member_is_import)
{
    const AGS::Symbol curtype = sym.get_type(cursym);
    // must either have a type of a struct here.
    if ((curtype != SYM_VARTYPE) &&
        (curtype != SYM_UNDEFINEDSTRUCT))
    {
        // Complain about non-type
        std::string type_name = sym.get_name_string(cursym).c_str();
        std::string prefix = sym.get_name_string(stname).c_str();
        prefix += "::";
        if (type_name.substr(0, prefix.length()) == prefix)
        {
            // The tokenizer has mangled the symbol, undo that.
            type_name = type_name.substr(prefix.length(), type_name.length());
        }
        cc_error("Expected a variable type instead of '%s'", type_name.c_str());
        return -1;
    }

    // [fw] Where's the problem?
    if (cursym == sym.normalStringSym)
    {
        cc_error("'string' not allowed inside struct");
        return -1;
    }

    if (targ->peeknext() < 0)
    {
        cc_error("Invalid syntax near '%s'", sym.get_name_string(cursym).c_str());
        return -1;
    }

    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    if ((curtype == SYM_UNDEFINEDSTRUCT) && !member_is_pointer)
    {
        cc_error("You can only declare a pointer to a struct that hasn't been completely defined yet");
        return -1;
    }

    const long curflags = sym.entries[cursym].flags;

    // [fw] Where is the problem?
    if (FlagIsSet(curflags, SFLG_STRUCTTYPE) && !member_is_pointer)
    {
        cc_error("Member variable cannot be struct");
        return -1;
    }
    if (member_is_pointer && FlagIsSet(sym.entries[stname].flags, SFLG_MANAGED) && !member_is_import)
    {
        cc_error("Member variable of managed struct cannot be pointer");
        return -1;
    }
    else if (FlagIsSet(curflags, SFLG_MANAGED) && !member_is_pointer)
    {
        cc_error("Cannot declare non-pointer of managed type");
        return -1;
    }
    else if (!FlagIsSet(curflags, SFLG_MANAGED) && member_is_pointer)
    {
        cc_error("Cannot declare pointer to non-managed type");
        return -1;
    }
    return 0;
}

// check that we haven't extended a struct that already contains a member with the same name
int ParseStruct_CheckMemberNotInASuperStruct(
    std::string nakedComponentNameStr,
    AGS::Symbol super_struct)
{
    AGS::Symbol member = sym.find_or_add(nakedComponentNameStr.c_str());

    if (FindMemberSym(super_struct, member, true, false) >= 0)
    {
        // Calculate name of the struct that contains the variable
        std::string super_name_str = sym.get_name_string(member);
        int struct_end_idx = super_name_str.rfind("::");
        if (struct_end_idx == std::string::npos) struct_end_idx = super_name_str.length();
        super_name_str.resize(struct_end_idx);
        cc_error(
            "This struct extends '%s', and '%s' is already defined",
            super_name_str.c_str(),
            sym.get_name_string(member).c_str());
        return -1;
    }
    return 0;
}

int ParseStruct_Function(ccInternalList *targ, ccCompiledScript *scrip, AGS::TypeQualifierSet tqs, AGS::Symbol curtype, AGS::Symbol stname, AGS::Symbol vname, AGS::Symbol name_of_current_func, bool type_is_pointer, bool isDynamicArray)
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
    if (SYM_SEMICOLON != sym.get_type(targ->peeknext()))
    {
        cc_error("Expected ';'");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected))
        SetFlag(sym.entries[vname].flags, SFLG_PROTECTED, true);
    return 0;
}

int ParseStruct_CheckAttributeFunc(AGS::Symbol func, bool setter, bool indexed, AGS::Symbol attrib_type)
{
    SymbolTableEntry &entry = sym.entries[func];
    bool const sscope_wanted = (indexed ? 1 : 0) + (setter ? 1 : 0);
    if (entry.sscope != sscope_wanted)
    {
        cc_error(
            "The attribute function '%s' should have %d parameters but has %d parameters instead",
            entry.sname.c_str(), sscope_wanted, entry.sscope);
        return -1;
    }
    size_t const index_param_num = 1 + (setter ? 1 : 0);
    if (indexed && entry.funcparamtypes[index_param_num] != sym.normalIntSym)
    {
        cc_error(
            "Parameter #%d of attribute function '%s' must have type integer but doesn't.",
            index_param_num, entry.sname.c_str());
        return -1;
    }
    if (setter && entry.funcparamtypes[2] != attrib_type)
    {
        cc_error(
            "Parameter #2 of attribute function '%s' must have type '%s' but doesn't",
            entry.sname.c_str(), sym.get_name_string(attrib_type).c_str());
        return -1;
    }
    if (!setter && entry.funcparamtypes[0] != attrib_type)
    {
        cc_error(
            "Return of attribute function '%s' must have type '%s' but doesn't",
            entry.sname.c_str(), sym.get_name_string(attrib_type).c_str());
        return -1;
    }
    return 0;
}

int ParseStruct_EnterAttributeFunc(ccCompiledScript *scrip, AGS::Symbol func, bool setter, bool indexed, AGS::Symbol attrib_type)
{
    SymbolTableEntry &entry = sym.entries[func];
    if (SYM_FUNCTION != entry.stype && 0 != entry.stype)
    {
        cc_error(
            "Attribute uses '%s' as a function, this symbol is used differently elsewhere",
            entry.sname.c_str());
        return -1;
    }

    if ((0 == entry.stype) || FlagIsSet(entry.flags, SFLG_IMPORTED))
        g_ImportMgr.FindOrAdd(entry.sname);

    // If function has been definied, check that it matches the prescribed definition
    if (SYM_FUNCTION == entry.stype)
    {
        // Handle func when it is defined, but check its parameter and return types
        int retval = ParseStruct_CheckAttributeFunc(func, setter, indexed, attrib_type);
        if (retval < 0) return retval;
    }
    else
    {
        SetFlag(entry.flags, SFLG_IMPORTED, true);
        int idx = g_ImportMgr.FindOrAdd(entry.sname);
        char  *num_param_suffix;
        if (setter)
            num_param_suffix = (indexed ? "^2" : "^1");
        else // getter
            num_param_suffix = (indexed ? "^1" : "^0");
        strcat(scrip->imports[idx], num_param_suffix);
    }
    return 0;
}

int ParseStruct_Attribute(ccInternalList *targ, ccCompiledScript *scrip, AGS::TypeQualifierSet tqs, AGS::Symbol stname, AGS::Symbol vname)
{
    if (kPP_PreAnalyze == g_PP)
    {
        if (SYM_OPENBRACKET == sym.get_type(targ->peeknext()))
        {
            targ->getnext();
            if (SYM_CLOSEBRACKET != sym.get_type(targ->getnext()))
            {
                cc_error("Cannot specify array size for attribute");
                return -1;
            }
        }
        return 0;
    }

    if (!FlagIsSet(tqs, kTQ_Import))
    {
        cc_error("Attribute must be import");
        return -1;
    }

    SetFlag(sym.entries[vname].flags, SFLG_IMPORTED, true);

    bool attribute_is_indexed = false;

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACKET)
    {
        attribute_is_indexed = true;
        targ->getnext();  // skip the [
        if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
        {
            cc_error("Cannot specify array size for attribute");
            return -1;
        }

        SetFlag(sym.entries[vname].flags, SFLG_ARRAY, true);
        sym.entries[vname].arrsize = 0;
    }

    // Enter attribute get func, e.g. get_ATTRIB()
    AGS::Symbol attrib_func;
    bool func_is_setter = false;
    int retval = GetAttributeFunction(vname, func_is_setter, attribute_is_indexed, attrib_func);
    if (retval < 0) return retval;
    retval = ParseStruct_EnterAttributeFunc(scrip, attrib_func, func_is_setter, attribute_is_indexed, static_cast<AGS::Symbol>(sym.entries[vname].vartype));
    if (retval < 0) return retval;

    if (FlagIsSet(tqs, kTQ_Readonly))
        return 0;

    // Enter attribute set func, e.g. set_ATTRIB(value)
    func_is_setter = true;
    retval = GetAttributeFunction(vname, func_is_setter, attribute_is_indexed, attrib_func);
    if (retval < 0) return retval;
    retval = ParseStruct_EnterAttributeFunc(scrip, attrib_func, func_is_setter, attribute_is_indexed, static_cast<AGS::Symbol>(sym.entries[vname].vartype));
    if (retval < 0) return retval;

    return 0;
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

    AGS::Symbol nextt = targ->getnext();
    if (sym.get_type(nextt) == SYM_CLOSEBRACKET)
    {
        if (FlagIsSet(sym.entries[stname].flags, SFLG_MANAGED))
        {
            cc_error("Member variable of managed struct cannot be dynamic array");
            return -1;
        }
        SetFlag(sym.entries[stname].flags, SFLG_HASDYNAMICARRAY, true);
        SetFlag(sym.entries[vname].flags, SFLG_DYNAMICARRAY, true);
        array_size = 0;
        size_so_far += 4;
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

        if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
        {
            cc_error("Expected ']'");
            return -1;
        }
    }
    SetFlag(sym.entries[vname].flags, SFLG_ARRAY, true);
    sym.entries[vname].arrsize = array_size;
    return 0;
}

// We're inside a struct decl, processing a member variable
int ParseStruct_Variable(ccInternalList *targ, ccCompiledScript *scrip, AGS::TypeQualifierSet tqs, AGS::Symbol curtype, bool type_is_pointer, AGS::Symbol stname, AGS::Symbol vname, size_t &size_so_far)
{
    if (kPP_Main == g_PP)
    {
        sym.entries[vname].stype = SYM_STRUCTMEMBER;
        sym.entries[vname].extends = stname;  // save which struct it belongs to
        sym.entries[vname].ssize = sym.entries[curtype].ssize;
        sym.entries[vname].soffs = size_so_far;
        sym.entries[vname].vartype = (short)curtype;
        if (FlagIsSet(tqs, kTQ_Readonly))
            SetFlag(sym.entries[vname].flags, SFLG_READONLY, true);
        if (FlagIsSet(tqs, kTQ_Attribute))
            SetFlag(sym.entries[vname].flags, SFLG_ATTRIBUTE, true);
        if (type_is_pointer)
        {
            SetFlag(sym.entries[vname].flags, SFLG_POINTER, true);
            sym.entries[vname].ssize = 4;
        }
        if (FlagIsSet(tqs, kTQ_Static))
            SetFlag(sym.entries[vname].flags, SFLG_STATIC, true);
        if (FlagIsSet(tqs, kTQ_Protected))
            SetFlag(sym.entries[vname].flags, SFLG_PROTECTED, true);
        if (FlagIsSet(tqs, kTQ_Writeprotected))
            SetFlag(sym.entries[vname].flags, SFLG_WRITEPROTECTED, true);
    }

    if (FlagIsSet(tqs, kTQ_Attribute))
        return ParseStruct_Attribute(targ, scrip, tqs, stname, vname);

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACKET)
        return ParseStruct_Array(targ, stname, vname, size_so_far);

    size_so_far += sym.entries[vname].ssize;
    return 0;
}

// We have accepted something like "struct foo extends bar { const int".
// We're waiting for the name of the member.
int ParseStruct_MemberDefnVarOrFuncOrArray(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol super_struct,
    AGS::Symbol stname,
    AGS::Symbol name_of_current_func, // ONLY used for funcs in structs
    TypeQualifierSet tqs,
    int curtype,
    bool type_is_pointer,
    size_t &size_so_far)
{
    AGS::Symbol vname = targ->getnext(); // normally variable name, array name, or function name, but can be [ too
    bool isDynamicArray = false;

    // Check whether "[]" is behind the type.  "struct foo { int [] bar; }" 
    if (sym.get_type(vname) == SYM_OPENBRACKET && sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
    {
        isDynamicArray = true;
        if (SYM_CLOSEBRACKET != sym.get_type(targ->getnext()))
        {
            cc_error("Expected ']'");
            return -1;
        }
        vname = targ->getnext();
    }

    // The Tokenizer prefixes all unknown symbols within a struct declaration with "STRUCT::".
    // Get the name without this prefix.
    std::string nakedComponentNameStr = sym.get_name_string(vname);
    size_t component_index = nakedComponentNameStr.find("::");
    if (component_index != std::string::npos)
        nakedComponentNameStr = nakedComponentNameStr.substr(component_index + 2);

    bool const isFunction = sym.get_type(targ->peeknext()) == SYM_OPENPARENTHESIS;

    if (!isFunction)
    {
        // Allow "struct ... { int S2 }" when S2 is a non-predefined type
        if (sym.get_type(vname) == SYM_VARTYPE && !sym_is_predef_typename(vname))
            vname = sym.find_or_add(GetFullNameOfMember(stname, vname).c_str());

        // Detect multiple declarations
        if (kPP_Main == g_PP &&
            sym.get_type(vname) != 0 &&
            (sym.get_type(vname) != SYM_VARTYPE || sym_is_predef_typename(vname)))
        {
            cc_error("'%s' is already defined", sym.get_name_string(vname).c_str());
            return -1;
        }
    }

    if (kPP_Main == g_PP && super_struct > 0)
    {
        // If this is an extension of another type, then we must make sure that names don't clash. 
        int retval = ParseStruct_CheckMemberNotInASuperStruct(nakedComponentNameStr, super_struct);
        if (retval < 0) return retval;
    }

    // All struct members get this flag, even functions
    if (kPP_Main == g_PP)
        SetFlag(sym.entries[vname].flags, SFLG_STRUCTMEMBER, true);

    if (isFunction)
    {
        if (name_of_current_func > 0)
        {
            cc_error("Cannot declare struct member function inside a function body");
            return -1;
        }
        return ParseStruct_Function(targ, scrip, tqs, curtype, stname, vname, name_of_current_func, type_is_pointer, isDynamicArray);
    }

    if (isDynamicArray)
    {
        // Someone tried to declare the function syntax for a dynamic array
        // But there was no function declaration
        cc_error("Expected '('");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Import) && (!FlagIsSet(tqs, kTQ_Attribute)))
    {
        // member variable cannot be an import
        cc_error("Only struct member functions may be declared with 'import'");
        return -1;
    }

    if ((curtype == stname) && (!type_is_pointer))
    {
        // cannot do  struct A { A a; }
        // since we don't know the size of A, recursiveness
        cc_error("Struct '%s' cannot be a member of itself", sym.get_name_string(curtype).c_str());
        return -1;
    }
    
    return ParseStruct_Variable(targ, scrip, tqs, curtype, type_is_pointer, stname, vname, size_so_far);
}

int ParseStruct_MemberStmt(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol stname,
    AGS::Symbol name_of_current_func,
    AGS::Symbol extendsWhat,
    size_t &size_so_far)
{
    AGS::Symbol curtype; // the type of the current members being defined, given as a symbol

    // parse qualifiers of the member ("import" etc.), set booleans accordingly
    TypeQualifierSet tqs = 0;
    ParseStruct_MemberQualifiers(targ, curtype, tqs);
    if (FlagIsSet(tqs, kTQ_Protected) && FlagIsSet(tqs, kTQ_Writeprotected))
    {
        cc_error("Field cannot be both protected and write-protected.");
        return -1;
    }

    // curtype can now be: typename or typename *

    // A member defn. is a pointer if it is AUTOPOINTER or it has an explicit "*"
    bool type_is_pointer = false;
    SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(curtype);
    if (FlagIsSet(entry.flags, SFLG_AUTOPTR))
    {
        type_is_pointer = true;
    }
    else if (sym.normalPointerSym == targ->peeknext())
    {
        type_is_pointer = true;
        targ->getnext();
    }

    // Certain types of members are not allowed in structs; check this
    if (kPP_Main == g_PP)
    {
        int retval = ParseStruct_IsMemberTypeIllegal(targ, stname, curtype, type_is_pointer, FlagIsSet(tqs, kTQ_Import));
        if (retval < 0) return retval;
    }

    // [fw] "struct foo { int * a, b, c;}"
    //      This declares b to be an int pointer; but in C or C++, b would be an int
    //      Bug?

    // run through all variables declared on this member defn.
    while (true)
    {
        int retval = ParseStruct_MemberDefnVarOrFuncOrArray(
            targ,
            scrip,
            extendsWhat,        // Parent struct
            stname,             // struct
            name_of_current_func,
            tqs,
            curtype,             // core type
            type_is_pointer,
            size_so_far);
        if (retval < 0) return retval;

        if (sym.get_type(targ->peeknext()) == SYM_COMMA)
        {
            targ->getnext(); // gobble the comma
            continue;
        }
        break;
    }

    // line must end with semicolon
    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
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
        (sym.get_type(stname) != SYM_UNDEFINEDSTRUCT))
    {
        cc_error("'%s' is already defined", sym.get_name_string(stname).c_str());
        return -1;
    }

    // Write the type of stname into the symbol table
    ParseStruct_SetTypeInSymboltable(stname, tqs);

    // Declare the struct type that implements new strings
    if (FlagIsSet(tqs, kTQ_Stringstruct))
    {
        if (sym.stringStructSym > 0 && stname != sym.stringStructSym)
        {
            cc_error("The stringstruct type is already defined to be %s", sym.get_name_string(sym.stringStructSym).c_str());
            return -1;
        }
        sym.stringStructSym = stname;
    }

    // Sums up the size of the struct
    size_t size_so_far = 0;

    // If the struct extends another struct, the token of the other struct's name
    AGS::Symbol extendsWhat = 0;

    // optional "extends" clause
    if (sym.get_type(targ->peeknext()) == SYM_EXTENDS)
        ParseStruct_ExtendsClause(targ, stname, extendsWhat, size_so_far);

    // forward-declaration of struct type
    if (sym.get_type(targ->peeknext()) == SYM_SEMICOLON)
    {
        targ->getnext(); // gobble the ";"
        SymbolTableEntry &entry = GetSymbolTableEntryAnyPhase(stname);
        entry.stype = SYM_UNDEFINEDSTRUCT;
        entry.ssize = 0;
        return 0;
    }

    // So we are in the declaration of the components
    // mandatory "{"
    if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
    {
        cc_error("Expected '{'");
        return -1;
    }

    // Process every member of the struct in turn
    while (sym.get_type(targ->peeknext()) != SYM_CLOSEBRACE)
    {
        int retval = ParseStruct_MemberStmt(targ, scrip, stname, name_of_current_func, extendsWhat, size_so_far);
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

    AGS::Symbol const next_sym = sym.get_type(targ->peeknext());
    if (SYM_SEMICOLON == next_sym)
    {
        targ->getnext(); // Eat ';'
        return 0;
    }
    if (0 != next_sym)
    {
        cc_error("Expected ';' or a variable name (did you forget ';' after the last struct definition?)");
        return -1;
    }

    bool dummy;
    ParseVartype0(targ, scrip, stname, &nesting_stack, tqs, name_of_current_func, struct_of_current_func, dummy);
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

    entry.stype = SYM_CONSTANT;
    entry.ssize = 4;
    entry.arrsize = 1;
    entry.vartype = enum_name;
    entry.sscope = 0;
    entry.flags = SFLG_READONLY;
    // soffs is unused for a constant, so in a gratuitous
    // hack we use it to store the enum's value
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
    entry.stype = SYM_VARTYPE;
    entry.ssize = 4; // standard int size
    entry.vartype = sym.normalIntSym;

    return 0;
}

// enum EnumName { value1, value2 }
int ParseEnum0(ccInternalList *targ)
{
    // Get name of the enum, enter it into the symbol table
    AGS::Symbol enum_name = targ->getnext();
    int retval = ParseEnum_Name2Symtable(enum_name);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
    {
        cc_error("Expected '{'");
        return -1;
    }

    int currentValue = 0;

    while (true)
    {
        AGS::Symbol item_name = targ->getnext();
        if (sym.get_type(item_name) == SYM_CLOSEBRACE)
            break; // item list empty or ends with trailing ','

        if (sym.get_type(item_name) == SYM_CONST)  // will only test properly in main phase, but that's OK
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

        AGS::Symbol tnext = sym.get_type(targ->peeknext());
        if (tnext != SYM_ASSIGN && tnext != SYM_COMMA && tnext != SYM_CLOSEBRACE)
        {
            cc_error("Expected '=' or ',' or '}'");
            return -1;
        }

        if (tnext == SYM_ASSIGN)
        {
            // the value of this entry is specified explicitly
            int retval = ParseEnum_AssignedValue(targ, currentValue);
            if (retval < 0) return retval;
        }

        // Enter this enum item as a constant int into the sym table
        ParseEnum_Item2Symtable(enum_name, item_name, currentValue);

        AGS::Symbol comma_or_brace = targ->getnext();
        if (sym.get_type(comma_or_brace) == SYM_CLOSEBRACE)
            break;
        if (sym.get_type(comma_or_brace) == SYM_COMMA)
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
    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
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
        const AGS::Symbol stoplist[] = { SYM_SEMICOLON };
        SkipTo(targ, stoplist, 1);
        targ->getnext(); // Eat ';'
        return 0;
    }

    // export specified symbols
    AGS::Symbol cursym = targ->getnext();
    while (sym.get_type(cursym) != SYM_SEMICOLON)
    {
        int nextype = sym.get_type(cursym);
        if (nextype == 0)
        {
            cc_error("Can only export global variables and functions, not '%s'", sym.get_name_string(cursym).c_str());
            return -1;
        }
        if ((nextype != SYM_GLOBALVAR) && (nextype != SYM_FUNCTION))
        {
            cc_error("Invalid export symbol '%s'", sym.get_name_string(cursym).c_str());
            return -1;
        }
        if (FlagIsSet(sym.entries[cursym].flags, SFLG_IMPORTED))
        {
            cc_error("Cannot export an import");
            return -1;
        }
        if (FlagIsSet(sym.entries[cursym].flags, SFLG_ISSTRING))
        {
            cc_error("Cannot export string; use char[200] instead");
            return -1;
        }
        // if all functions are being exported anyway, don't bother doing
        // it now
        if ((ccGetOption(SCOPT_EXPORTALL) != 0) && (nextype == SYM_FUNCTION))
        { }
        else if (scrip->add_new_export(sym.get_name_string(cursym).c_str(),
            (nextype == SYM_GLOBALVAR) ? EXPORT_DATA : EXPORT_FUNCTION,
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
        if (sym.get_type(cursym) == SYM_SEMICOLON)
            break;
        if (sym.get_type(cursym) != SYM_COMMA)
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

    if (sym.get_type(targ->peeknext()) != SYM_MEMBERACCESS)
        return 0; // done

    // We are accepting "struct::member"; so varname isn't the var name yet: it's the struct name.
    struct_of_member_fct = varname;
    targ->getnext(); // gobble "::"
    AGS::Symbol member_of_member_function = targ->getnext();

    // change varname to be the full function name
    varname = sym.find(GetFullNameOfMember(struct_of_member_fct, member_of_member_function).c_str());
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
        
        if (!FlagIsSet(entry.flags, SFLG_STRUCTTYPE))
        {
            cc_error("Cannot create pointer to basic type");
            return -1;
        }
        if (FlagIsSet(entry.flags, SFLG_AUTOPTR))
        {
            cc_error("Invalid use of '*'");
            return -1;
        }
        isPointer = true;
        targ->getnext();
    }

    if (FlagIsSet(entry.flags, SFLG_AUTOPTR))
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
        SetFlag(entry.flags, SFLG_STRUCTMEMBER, true);

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
        if (SYM_SEMICOLON != sym.get_type(targ->getnext()))
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


int ParseVartype_VarDef(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &var_name, Globalness is_global, int nested_level, bool is_readonly, int type_of_defn, int next_type, bool isPointer, bool &another_var_follows)
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
        AGS::Symbol const stoplist[] = { SYM_COMMA, SYM_SEMICOLON };
        SkipTo(targ, stoplist, 2);
        another_var_follows = (targ->getnext() == SYM_COMMA);
        return 0;
    }

    if (is_global == kGl_Local)
        sym.entries[var_name].sscope = nested_level;
    if (is_readonly)
        SetFlag(sym.entries[var_name].flags, SFLG_READONLY, true);

    // parse the definition
    int retval = ParseVardecl(targ, scrip, var_name, type_of_defn, next_type, is_global, isPointer, another_var_follows);
    if (retval < 0) return retval;
    return 0;
}

// We accepted a variable type such as "int", so what follows is a function or variable definition
int ParseVartype0(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol type_of_defn,           // e.g., "int"
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
    retval = ParseVartype_GetPointerStatus(targ, type_of_defn, isPointer);
    if (retval < 0) return retval;

    // Look for "[]"; if present, gobble it and call this a dynamic array.
    // "int [] func(...)"
    int dynArrayStatus = ParseParamlist_Param_DynArrayMarker(targ, type_of_defn, isPointer);
    if (dynArrayStatus < 0) return dynArrayStatus;
    bool isDynamicArray = (dynArrayStatus > 0);

    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    noloopcheck_is_set = false;
    if (sym.get_type(targ->peeknext()) == SYM_LOOPCHECKOFF)
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

        // Refuse to process it if it's a type name
        if (sym.get_type(var_or_func_name) == SYM_VARTYPE || sym_is_predef_typename(var_or_func_name))
        {
            cc_error("'%s' is already in use as a type name", sym.get_name_string(var_or_func_name).c_str());
            return -1;
        }

        // Check whether var or func is being defined
        int next_type = sym.get_type(targ->peeknext());
        bool is_function = (sym.get_type(targ->peeknext()) == SYM_OPENPARENTHESIS);
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

            retval = ParseVartype_FuncDef(targ, scrip, var_or_func_name, type_of_defn, isPointer, isDynamicArray, tqs, struct_of_current_func, name_of_current_func);
            if (retval < 0) return retval;
            another_ident_follows = false; // Can't join another func or var with ','
            break;
        }

        retval = ParseVartype_VarDef(targ, scrip, var_or_func_name, is_global, nesting_stack->Depth() - 1, FlagIsSet(tqs, kTQ_Readonly), type_of_defn, next_type, isPointer, another_ident_follows);
        if (retval < 0) return retval;
    }
    while (another_ident_follows);

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

int ParseReturn(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol inFuncSym)
{
    AGS::Symbol const functionReturnType = sym.entries[inFuncSym].funcparamtypes[0];

    if (sym.get_type(targ->peeknext()) != SYM_SEMICOLON)
    {
        if (functionReturnType == sym.normalVoidSym)
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
        retval = IsTypeMismatch(scrip->ax_val_type, functionReturnType, true);
        if (retval < 0) return retval;

        if ((is_string(scrip->ax_val_type)) &&
            (scrip->ax_val_scope == SYM_LOCALVAR))
        {
            cc_error("Cannot return local string from function");
            return -1;
        }
    }
    else if ((functionReturnType != sym.normalIntSym) && (functionReturnType != sym.normalVoidSym))
    {
        cc_error("Must return a '%s' value from function", sym.get_name_string(functionReturnType).c_str());
        return -1;
    }
    else
    {
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    }

    int cursym = sym.get_type(targ->getnext());
    if (cursym != SYM_SEMICOLON)
    {
        cc_error("Expected ';' instead of '%s'", sym.get_name_string(cursym).c_str());
        return -1;
    }

    // count total space taken by all local variables
    FreePointersOfLocals(scrip, 0);

    int totalsub = StacksizeOfLocals(0);
    if (totalsub > 0)
        scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    scrip->write_cmd(SCMD_RET);
    // We don't alter cur_sp since there can be code after the RETURN

    return 0;
}

// Evaluate the head of an "if" clause, e.g. "if (i < 0)".
int ParseIf(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    // Get expression, must be in parentheses
    if (sym.get_type(targ->getnext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != SYM_CLOSEPARENTHESIS)
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

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
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
    if (sym.get_type(targ->getnext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != SYM_CLOSEPARENTHESIS)
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

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
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

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
    {
        targ->getnext();
        // Change to braced DO
        nesting_stack->SetType(AGS::NestingStack::kNT_BracedDo);
    }

    return 0;
}

// TODO: This is either assignment_or_funccall or a vardef.
// If it is a vardef, it should begin with VARTYPE.
int ParseFor_InitClause(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &cursym, size_t nested_level)
{
    scrip->flush_line_numbers();

    // Check for empty init clause
    if (sym.get_type(cursym) == SYM_SEMICOLON)
        return 0;

    if (sym.get_type(cursym) == SYM_VARTYPE)
    {
        int vtwas = cursym;

        bool isPointer = false;

        if (sym.normalPointerSym == targ->peeknext())
        {
            // only allow pointers to structs
            if (!FlagIsSet(sym.entries[vtwas].flags, SFLG_STRUCTTYPE))
            {
                cc_error("Cannot create pointer to basic type");
                return -1;
            }
            if (FlagIsSet(sym.entries[vtwas].flags, SFLG_AUTOPTR))
            {
                cc_error("Invalid use of '*'");
                return -1;
            }
            isPointer = true;
            targ->getnext();
        }

        if (FlagIsSet(sym.entries[vtwas].flags, SFLG_AUTOPTR))
            isPointer = true;

        if (sym.get_type(targ->peeknext()) == SYM_LOOPCHECKOFF)
        {
            cc_error("'noloopcheck' is not applicable in this context");
            return -1;
        }

        // FIXME: This duplicates common variable declaration parsing at/around the "startvarbit" label
        bool another_var_follows = false;
        do
        {
            cursym = targ->getnext();
            if (cursym < 0)
            {
                // eg. "int" was the last word in the file
                currentline = targ->lineAtEnd;
                cc_error("Unexpected end of file");
                return -1;
            }

            int next_type = sym.get_type(targ->peeknext());
            if (next_type == SYM_MEMBERACCESS || next_type == SYM_OPENPARENTHESIS)
            {
                cc_error("Function definition not allowed in for loop initialiser");
                return -1;
            }
            else if (sym.get_type(cursym) != 0)
            {
                cc_error("Variable '%s' is already defined", sym.get_name_string(cursym).c_str());
                return -1;
            }
            else
            {
                // variable declaration
                sym.entries[cursym].sscope = static_cast<short>(nested_level);

                // parse the declaration
                int varsize = sym.entries[vtwas].ssize;
                int retval = ParseVardecl(targ, scrip, cursym, vtwas, next_type, kGl_Local, isPointer, another_var_follows);
                if (retval < 0) return retval;
            }
        }
        while (another_var_follows);
    }
    else
    {
        ccInternalList expr_script;
        int retval = BufferExpression(targ, expr_script);
        if (retval < 0) return retval;
        retval = ParseAssignment(targ, scrip, cursym, SYM_SEMICOLON, &expr_script);
        if (retval < 0) return retval;
    }
    return 0;
}

int ParseFor_WhileClause(ccInternalList *targ, ccCompiledScript *scrip)
{
    scrip->flush_line_numbers();

    // Check for empty while clause
    if (sym.get_type(targ->peeknext()) == SYM_SEMICOLON)
    {
        // Not having a while clause is tantamount to the while condition "true".
        // So let's write "true" to the AX register.
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 1);
        targ->getnext();
        return 0;
    }

    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}

// TODO: This should be ParseAssignmentOrFunccall
int ParseFor_IterateClause(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &cursym)
{
    scrip->flush_line_numbers();
    // Check for empty interate clause
    if (sym.get_type(cursym) == SYM_CLOSEPARENTHESIS)
        return 0;

    ccInternalList expr_script;
    int retval = BufferExpression(targ, expr_script);
    if (retval < 0) return retval;
    retval = ParseAssignment(targ, scrip, cursym, SYM_CLOSEPARENTHESIS, &expr_script);
    if (retval < 0) return retval;

    return 0;
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

    // '(' must follow
    cursym = targ->getnext();
    if (sym.get_type(cursym) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    // Even if clauses are empty, we still need their ';'
    cursym = targ->getnext();
    if (sym.get_type(cursym) == SYM_CLOSEPARENTHESIS)
    {
        cc_error("Empty parentheses \"()\" aren't allowed after \"for\" (write \"for(;;)\" instead");
        return -1;
    }

    // Generate the initialization clause (I)
    retval = ParseFor_InitClause(targ, scrip, cursym, nesting_stack->Depth() - 1);
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == SYM_CLOSEPARENTHESIS)
    {
        cc_error("Missing ';' inside for loop declaration");
        return -1;
    }

    // Remember where the code of the while condition starts.
    AGS::CodeLoc while_cond_loc = scrip->codesize;

    retval = ParseFor_WhileClause(targ, scrip);
    if (retval < 0) return retval;

    // Remember where the code of the iterate clause starts.
    AGS::CodeLoc iterate_clause_loc = scrip->codesize;
    size_t pre_fixup_count = scrip->numfixups;
    cursym = targ->getnext();

    retval = ParseFor_IterateClause(targ, scrip, cursym);
    if (retval < 0) return retval;

    // Inner nesting level - assume unbraced as a default
    retval = nesting_stack->Push(
        AGS::NestingStack::kNT_UnbracedElse, // Type
        while_cond_loc, // Start
        0); // Info
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
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
    if (sym.get_type(targ->getnext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    int retval = ParseExpression(targ, scrip);
    if (retval < 0) return retval;
    if (sym.get_type(targ->getnext()) != SYM_CLOSEPARENTHESIS)
    {
        cc_error("Expected ')'");
        return -1;
    }

    // Remember the type of this expression to enforce it later
    int switch_expr_type = scrip->ax_val_type;

    // Copy the result to the BX register, ready for case statements
    scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
    scrip->flush_line_numbers();

    // Remember the start of the lookup table
    AGS::CodeLoc lookup_table_start = scrip->codesize;

    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the lookup table
    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to beyond the switch statement (for break)

    // There's no such thing as an unbraced SWITCH, so '{' must follow
    if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
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
    if (sym.get_type(targ->peeknext()) != SYM_CASE && sym.get_type(targ->peeknext()) != SYM_DEFAULT && sym.get_type(targ->peeknext()) != SYM_CLOSEBRACE)
    {
        cc_error("Invalid keyword '%s' in switch statement block", sym.get_name_string(targ->peeknext()).c_str());
        return -1;
    }
    return 0;
}

int ParseCasedefault(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
{
    if (nesting_stack->Type() != AGS::NestingStack::kNT_Switch)
    {
        cc_error("Case label not valid outside switch statement block");
        return -1;
    }

    if (sym.get_type(cursym) == SYM_DEFAULT)
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
        retval = IsTypeMismatch(scrip->ax_val_type, nesting_stack->SwitchExprType(), false);
        if (retval < 0) return retval;

        // Pop the switch variable, ready for comparison
        scrip->pop_reg(SREG_BX);

        // get the right equality operator for the type
        int eq_op = SCMD_ISEQUAL;
        retval = GetOperatorValidForType(scrip->ax_val_type, nesting_stack->SwitchExprType(), eq_op);
        if (retval < 0) return retval;

        // [fw] Comparison operation may be missing here.

    // rip out the already generated code for the case/switch and store it with the switch
        int id;
        size_t const yank_size = scrip->codesize - start_of_code_loc;
        nesting_stack->YankChunk(scrip, start_of_code_loc, numfixups_at_start_of_code, id);
        g_FCM.UpdateCallListOnYanking(start_of_code_loc, yank_size, id);
    }

    // expect and gobble the ':'
    if (sym.get_type(targ->getnext()) != SYM_LABEL)
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

    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
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

    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
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

// We're compiling function body code; the code does not start with a keyword or type.
// Thus, we should be at the start of an assignment or a funccall. Compile it.
int ParseAssignmentOrFunccall(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym)
{
    ccInternalList expr_script;
    int retval = BufferExpression(targ, expr_script);
    if (retval < 0) return retval;

    AGS::Symbol nextsym = targ->getnext();
    AGS::Symbol const nexttype = sym.get_type(nextsym);

    if (expr_script.length > 0)
    {
        if (nexttype == SYM_SEMICOLON)
            return ParseSubexpr(scrip, expr_script.script, expr_script.length);

        if (nexttype == SYM_ASSIGN || nexttype == SYM_MASSIGN || nexttype == SYM_SASSIGN)
            return ParseAssignment(targ, scrip, nextsym, SYM_SEMICOLON, &expr_script);
    }
    cc_error("Unexpected symbol '%s'", sym.get_name_string(nextsym).c_str());
    return -1;
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
    int curtype = sym.get_type(cursym);

    switch (curtype)
    {
    default:
        // If it doesn't begin with a keyword, it should be an assignment
        // or a func call.
        retval = ParseAssignmentOrFunccall(targ, scrip, cursym);
        if (retval < 0) return retval;
        break;

    case SYM_BREAK:
        retval = ParseBreak(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_CASE:
        retval = ParseCasedefault(targ, scrip, cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case  SYM_CLOSEBRACE:
        return ParseClosebrace(targ, scrip, nesting_stack, struct_of_current_func, name_of_current_func);

    case SYM_CONTINUE:
        retval = ParseContinue(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_DEFAULT:
        retval = ParseCasedefault(targ, scrip, cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_DO:
        return ParseDo(targ, scrip, nesting_stack);

    case SYM_FOR:
        return ParseFor(targ, scrip, cursym, nesting_stack);

    case SYM_IF:
        return ParseIf(targ, scrip, cursym, nesting_stack);

    case SYM_OPENBRACE:
        return ParseOpenbrace(scrip, nesting_stack, name_of_current_func, struct_of_current_func, next_is_noloopcheck);

    case SYM_RETURN:
        retval = ParseReturn(targ, scrip, name_of_current_func);
        if (retval < 0) return retval;
        break;

    case SYM_SWITCH:
        retval = ParseSwitch(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_WHILE:
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
        {kTQ_Import1, "import"},
        {kTQ_Import2, "_tryimport"},
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

    if (FlagIsSet(tqs, kTQ_Builtin) && SYM_STRUCT != decl_type)
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

    if (FlagIsSet(tqs, kTQ_Managed) && SYM_STRUCT != decl_type)
    {
        cc_error("'managed' can only be used with structs");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Protected) && 0 != (tqs & ~kTQ_Static & ~kTQ_Readonly))
    {
        cc_parse_TQCombiError((tqs & ~kTQ_Static & ~kTQ_Readonly));
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Readonly) && SYM_VARTYPE != decl_type)
    {
        cc_error("'readonly' can only be used in a type declaration");
        return -1;
    }

    if (FlagIsSet(tqs, kTQ_Static) && SYM_VARTYPE != decl_type)
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

    if (SYM_EXPORT == decl_type && 0 != tqs)
    {
        cc_parse_TQCombiError(tqs);
        return -1;
    }
    return 0;
}

int ParseVartype(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, TypeQualifierSet tqs, AGS::NestingStack &nesting_stack, AGS::Symbol &name_of_current_func, AGS::Symbol &struct_of_current_func, bool &set_nlc_flag)
{
    // func or variable definition
    int retval = cc_parse_CheckTQ(tqs, SYM_VARTYPE);
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

    // This collects the qualifiers ("static" etc.); it is reset whenever the 
    // qualifiers are used.
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

        int const symType = GetSymbolTypeAnyPhase(cursym);
        switch (symType)
        {

        default: break;

        case 0:
            cc_error("Unexpected token '%s'", sym.get_name_string(cursym).c_str());
            return -1;

        case  SYM_AUTOPTR:
        {
            SetFlag(tqs, kTQ_Autoptr, true);
            continue;
        }

        case SYM_BUILTIN:
        {
            SetFlag(tqs, kTQ_Builtin, true);
            continue;
        }

        case SYM_CONST:
        {
            SetFlag(tqs, kTQ_Const, true);
            continue;
        }

        case SYM_ENUM:
        {
            int retval = cc_parse_CheckTQ(tqs, SYM_EXPORT);
            if (retval < 0) return retval;
            retval = ParseEnum(targ, name_of_current_func);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case SYM_EXPORT:
        {
            int retval = cc_parse_CheckTQ(tqs, SYM_EXPORT);
            if (retval < 0) return retval;
            retval = ParseExport(targ, scrip);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case SYM_IMPORT:
        {
            if (std::string::npos != sym.get_name_string(cursym).find("_tryimport"))
                SetFlag(tqs, kTQ_Import2, true);
            else
                SetFlag(tqs, kTQ_Import1, true);
            if (name_of_current_func > 0)
            {
                cc_error("'import' not allowed inside function body");
                return -1;
            }
            continue;
        }

        case  SYM_MANAGED:
        {
            SetFlag(tqs, kTQ_Managed, true);
            continue;
        }

        case SYM_OPENBRACE:
        {
            if (kPP_Main == g_PP)
                break; // treat as a command, below the switch

            cc_parse_SkipToEndingBrace(targ);
            tqs = 0;
            name_of_current_func = -1;
            struct_of_current_func = -1;
            continue;
        }

        case SYM_PROTECTED:
        {
            SetFlag(tqs, kTQ_Protected, true);
            if (name_of_current_func > 0)
            {
                cc_error("'protected' not allowed inside a function body");
                return -1;
            }
            continue;
        }

        case SYM_READONLY:
        {
            SetFlag(tqs, kTQ_Readonly, true);
            continue;
        }

        case SYM_STATIC:
        {
            SetFlag(tqs, kTQ_Static, true);
            if (name_of_current_func >= 0)
            {
                cc_error("'static' not allowed inside function body");
                return -1;
            }
            continue;
        }

        case SYM_STRINGSTRUCT:
        {
            SetFlag(tqs, kTQ_Stringstruct, true);
            continue;
        }

        case  SYM_STRUCT:
        {
            int retval = cc_parse_CheckTQ(tqs, SYM_STRUCT);
            if (retval < 0) return retval;
            retval = ParseStruct(targ, scrip, tqs, nesting_stack, name_of_current_func, struct_of_current_func);
            if (retval < 0) return retval;
            tqs = 0;
            continue;
        }

        case SYM_VARTYPE:
        {
            if (SYM_DOT == sym.get_type(targ->peeknext()))
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
    } // for

    return 0;
}

// Copy all the func headers from the PreAnalyse phase into the "real" symbol table
int cc_parse_FuncHeaders2Sym()
{
    for (TSym1Table::iterator sym_it = g_Sym1.begin(); sym_it != g_Sym1.end(); ++sym_it)
    {
        SymbolTableEntry &s_entry = sym_it->second;
        if (s_entry.stype != SYM_FUNCTION)
            continue;
        SymbolTableEntry &f_entry = sym.entries[sym_it->first];
        SetFlag(s_entry.flags, SFLG_IMPORTED, (kFT_Import == s_entry.soffs));
        s_entry.soffs = 0;
        s_entry.sname = f_entry.sname;
        s_entry.CopyTo(f_entry);
    }
    return 0;
}

int cc_parse(ccInternalList *targ, ccCompiledScript *scrip)
{
    // Skim through the code and collect the headers of functions defined locally
    int const start_of_input = targ->pos;

    g_GIVM.clear();
    g_ImportMgr.Init(scrip);
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

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

The Parser has the following main components:
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

char ccCopyright[] = "ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2018 others";
static char ScriptNameBuffer[256];

int  ParseExpression(ccInternalList *targ, ccCompiledScript *script, bool consider_paren_nesting);

int ReadDataIntoAX(ccCompiledScript*scrip, AGS::SymbolScript syml, int syml_len, bool negateLiteral);
int ReadDataIntoAX(ccCompiledScript*scrip, AGS::SymbolScript syml, int syml_len, bool negateLiteral, bool mustBeWritable, bool &write_same_as_read_access);

int MemoryAccess(
    ccCompiledScript *scrip, AGS::Symbol variableSym,
    int variableSymType, bool isAttribute,
    bool writing, bool mustBeWritable,
    bool addressof, bool isArrayOffset,
    int soffset, bool pointerIsInMAR,
    bool wholePointerAccess,
    AGS::Symbol mainVariableSym, int mainVariableType,
    bool isDynamicArray, bool negateLiteral);
int ParseSubexpr(ccCompiledScript *scrip, AGS::SymbolScript symlist, size_t symlist_len);


enum FxFixupType // see script_common.h
{
    kFx_NoFixup = 0,
    kFx_FixupDataData = 1, // globaldata[fixup] += &globaldata[0]
    kFx_FixupFunction = 2, // code[fixup] += &code[0]
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

// [fw] This should probably move to "cs_symboltable.h"
int sym_find_or_add(const char *sname)
{
    int sym_index = sym.find(sname);
    if (sym_index < 0)
        sym_index = sym.add(sname);
    return sym_index;
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
void AGS::NestingStack::YankChunk(ccCompiledScript *scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset)
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

    _stack.back().Chunks.push_back(item);

    // Cut out the code that has been pushed
    scrip->codesize = codeoffset;
    scrip->numfixups = fixupoffset;
}


// Copy the code in the chunk to the end of the bytecode vector 
void AGS::NestingStack::WriteChunk(ccCompiledScript *scrip, size_t level, size_t index)
{
    const AGS::NestingStack::Chunk item = Chunks(level).at(index);
    scrip->flush_line_numbers();
    AGS::CodeLoc adjust = scrip->codesize - item.CodeOffset;

    size_t limit = item.Code.size();
    for (size_t index = 0; index < limit; index++)
        scrip->write_code(item.Code[index]);

    limit = item.Fixups.size();
    for (size_t index = 0; index < limit; index++)
        scrip->add_fixup(item.Fixups[index] + adjust, item.FixupTypes[index]);
}


std::string ConstructedMemberName; // size limitation removed

const char *GetFullNameOfMember(AGS::Symbol structSym, AGS::Symbol memberSym)
{

    // Get C-string of member, de-mangle it if appropriate
    const char* memberNameStr = sym.get_name(memberSym);
    if (memberNameStr[0] == '.')
        memberNameStr = &memberNameStr[1];

    ConstructedMemberName = sym.get_name(structSym);
    ConstructedMemberName += "::";
    ConstructedMemberName += memberNameStr;

    return ConstructedMemberName.c_str();
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
        if ((sym.entries[entries_idx].flags & SFLG_IMPORTED) != 0)
            continue;
        if ((sym.entries[entries_idx].flags & SFLG_ATTRIBUTE) != 0)
            continue;

        if ((sym.entries[entries_idx].flags & SFLG_POINTER) == 0)
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

        if (sym.entries[structVarSym].flags & SFLG_ARRAY)
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
        if ((sym.entries[entries_idx].flags & SFLG_PARAMETER) != 0)
            continue;

        if ((sym.entries[entries_idx].flags & SFLG_DYNAMICARRAY) != 0)
        {
            totalsub += 4; // size of an implicit pointer
            continue;
        }

        // Calculate the size of one var of the given type
        size_t ssize = sym.entries[entries_idx].ssize;
        if (sym.entries[entries_idx].flags & SFLG_STRBUFFER)
            ssize += STRING_LENGTH;

        // Calculate the number of vars
        size_t number = 1;
        if (sym.entries[entries_idx].flags & SFLG_ARRAY)
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
        if (sym.entries[entries_idx].flags & SFLG_THISPTR)
            continue;

        if (((sym.entries[entries_idx].flags & SFLG_POINTER) != 0) ||
            ((sym.entries[entries_idx].flags & SFLG_DYNAMICARRAY) != 0))
        {
            FreePointer(scrip, scrip->cur_sp - sym.entries[entries_idx].soffs, zeroPtrCmd, static_cast<AGS::Symbol>(entries_idx));
            continue;
        }

        if (sym.entries[sym.entries[entries_idx].vartype].flags & SFLG_STRUCTTYPE)
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
int DealWithEndOfElse(ccInternalList *targ, ccCompiledScript*scrip, AGS::NestingStack *nesting_stack, bool &else_after_then)
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
            nesting_stack->WriteChunk(scrip, 0);
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
    if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    scrip->flush_line_numbers();

    int retval = ParseExpression(targ, scrip, true);
    if (retval < 0) return retval;

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
        // Put the result of the expression into AX
        nesting_stack->WriteChunk(scrip, index);
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
int FindMemberSym(AGS::Symbol structSym, AGS::Symbol &memSym, bool allowProtected) {

    // Construct a string out of struct and member, look it up in the symbol table
    const char *name_as_cstring = GetFullNameOfMember(structSym, memSym);
    AGS::Symbol full_name = sym.find(name_as_cstring);

    if (full_name < 0)
    {
        if (sym.entries[structSym].extends > 0)
        {
            // look for the member in the ancesters recursively
            if (FindMemberSym(sym.entries[structSym].extends, memSym, allowProtected) == 0)
                return 0;
            // the inherited member was not found, so fall through to the error message
        }
        cc_error("'%s' is not a public member of '%s'. Are you sure you spelt it correctly (remember, capital letters are important)?",
            sym.get_friendly_name(memSym).c_str(),
            sym.get_friendly_name(structSym).c_str());
        return -1;
    }

    if ((!allowProtected) && (sym.entries[full_name].flags & SFLG_PROTECTED))
    {
        cc_error("Cannot access protected member '%s'", sym.get_friendly_name(full_name).c_str());
        return -1;
    }
    memSym = full_name;
    return 0;
}


int ParseLiteralOrConstvalue(AGS::Symbol fromSym, int &theValue, bool isNegative, const char *errorMsg) {
    if (sym.get_type(fromSym) == SYM_CONSTANT)
    {
        theValue = sym.entries[fromSym].soffs;
        if (isNegative)
            theValue = -theValue;
        return 0;
    }

    if (sym.get_type(fromSym) == SYM_LITERALVALUE)
    {
        std::string literalStrValue = std::string(sym.get_name(fromSym));
        if (isNegative)
            literalStrValue = '-' + literalStrValue;

        // convert to LONG, but reject the result if out ouf INT range.
        errno = 0;
        char *endptr = 0;
        const long longValue = strtol(literalStrValue.c_str(), &endptr, 10);
        if ((longValue == LONG_MIN && errno == ERANGE) ||
            (isNegative && (endptr[0] != '\0')) ||
            (longValue < INT_MIN))
        {
            cc_error("Literal value '%s' is too low (min. is '%d')", literalStrValue.c_str(), INT_MIN);
            return -1;
        }

        if ((longValue == LONG_MAX && errno == ERANGE) ||
            ((!isNegative) && (endptr[0] != '\0')) ||
            (longValue > INT_MAX))
        {
            cc_error("Literal value '%s' is too high (max. is '%d')", literalStrValue.c_str(), INT_MAX);
            return -1;
        }

        theValue = static_cast<int>(longValue);
        return 0;
    }

    cc_error((char*)errorMsg);
    return -1;
}


// We're parsing a parameter list and we have accepted something like "(...int i"
// We accept a default value clause like "= 15" if it follows at this point.
int ParseParamlist_Param_DefaultValue(
    ccInternalList *targ,
    bool &has_default_int,
    int &default_int_value)
{

    if (sym.get_type(targ->peeknext()) != SYM_ASSIGN)
    {
        has_default_int = false;
        return 0;
    }

    has_default_int = true;

    // parameter has default value
    targ->getnext();   // eat "="

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
int ParseParamlist_Param_DynArrayMarker(
    ccInternalList *targ,
    AGS::Symbol typeSym,
    bool isPointer)
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

    if (sym.entries[typeSym].flags & SFLG_STRUCTTYPE)
    {
        if (!(sym.entries[typeSym].flags & SFLG_MANAGED))
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


int ParseFuncdecl_CopyForwardDecl(ccCompiledScript * scrip, AGS::Symbol & funcsym, SymbolTableEntry *oldDefinition)
{
    // Copy so that the forward definition can be compared afterwards to the real one 
    int retval = scrip->copy_import_symbol_table_entry(funcsym, oldDefinition);
    if (retval < 0) return retval;
    // Strip import flag since the real defn won't be exported
    oldDefinition->flags &= ~SFLG_IMPORTED;

    // Check whether the import has been referenced or whether imports may not be overridden;
    // if so, complain; remove the import flags
    // This ruins the flags, so save them beforehand and save them afterwards
    const int funcsym_flags = sym.entries[funcsym].flags;
    retval = scrip->just_remove_any_import(funcsym);
    sym.entries[funcsym].flags = funcsym_flags & ~SFLG_IMPORTED;
    return retval;
}


// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
// We'll accept something like "this Character *"
int ParseFuncdecl_ExtenderPreparations(
    ccInternalList * targ,
    ccCompiledScript * scrip,
    bool func_is_static_extender,
    Importness func_is_import,
    std::string & functionName,
    AGS::Symbol &funcsym,
    AGS::Symbol &struct_extended_by_the_func,
    SymbolTableEntry * oldDefinition)
{
    targ->getnext(); // accept "this" or "static"
    if (sym.get_type(targ->peeknext()) != SYM_VARTYPE)
    {
        cc_error("'%s' must be followed by a struct name",
            (func_is_static_extender ? "static" : "this"));
        return -1;
    }

    if ((sym.entries[targ->peeknext()].flags & SFLG_STRUCTTYPE) == 0)
    {
        cc_error("'%s' cannot be used with primitive types",
            (func_is_static_extender ? "static" : "this"));
        return -1;
    }

    if (functionName.find_first_of(':', 0) != std::string::npos)
    {   // [fw] Can't be reached IMO.
        cc_error("Extender functions cannot be part of a struct");
        return -1;
    }

    struct_extended_by_the_func = targ->peeknext();
    functionName = sym.get_name(struct_extended_by_the_func);
    functionName += "::";
    functionName += sym.get_name(funcsym);

    funcsym = sym_find_or_add(functionName.c_str());

    if (func_is_import == kIm_NoImport && (sym.entries[funcsym].flags & SFLG_IMPORTED) != 0)
    {
        int retval = ParseFuncdecl_CopyForwardDecl(scrip, funcsym, oldDefinition);
        if (retval < 0) return retval;
    }
    else if (sym.entries[funcsym].stype != 0)
    {
        cc_error("Function '%s' is already defined", functionName);
        return -1;
    }

    sym.entries[funcsym].flags = SFLG_STRUCTMEMBER;
    if (func_is_static_extender)
        sym.entries[funcsym].flags |= SFLG_STATIC;

    targ->getnext();
    if (!func_is_static_extender && targ->getnext() != sym.find("*"))
    {
        cc_error("Instance extender function must be pointer");
        return -1;
    }

    if ((sym.get_type(targ->peeknext()) != SYM_COMMA) &&
        (sym.get_type(targ->peeknext()) != SYM_CLOSEPARENTHESIS))
    {
        if (strcmp(sym.get_name(targ->getnext()), "*") == 0)
            cc_error("Static extender function cannot be pointer");
        else
            cc_error("Parameter name cannot be defined for extender type");
        return -1;
    }

    if (sym.get_type(targ->peeknext()) == SYM_COMMA)
        targ->getnext();

    return 0;
}


int ParseFuncdecl_NonExtenderPreparations(ccCompiledScript *scrip, Importness func_is_import, int struct_containing_the_func, std::string &functionName, AGS::Symbol &funcsym, SymbolTableEntry *oldDefinition)
{
    if (func_is_import == kIm_NoImport && (sym.entries[funcsym].flags & SFLG_IMPORTED) != 0)
    {
        // Copy the previous definition to oldDefinition, 
        // so that the current definition can be compared with it
        int retval = ParseFuncdecl_CopyForwardDecl(scrip, funcsym, oldDefinition);
        if (retval < 0) return retval;
    }
    else if (sym.entries[funcsym].stype != 0)
    {
        cc_error("Function '%s' is already defined", functionName);
        return -1;
    }
    return 0;
}


int ParseFunc_Paramlist_ParamType(ccInternalList *targ, AGS::Symbol param_type, bool &param_is_ptr)
{
    const bool param_is_natural_ptr =
        (0 == strcmp(sym.get_name(targ->peeknext()), "*"));
    // Determine whether the type is a pointer
    if (param_is_natural_ptr)
        targ->getnext(); // gobble the '*'

    const bool param_is_autoptr =
        (0 != (sym.entries[param_type].flags & SFLG_AUTOPTR));

    param_is_ptr = param_is_natural_ptr || param_is_autoptr;

    // Safety checks on the parameter type
    if (param_type == sym.normalVoidSym)
    {
        cc_error("A function parameter must not have the type 'void'");
        return -1;
    }
    if (param_is_ptr)
    {
        if (0 == (sym.entries[param_type].flags & SFLG_MANAGED))
        {
            // can only point to managed structs
            cc_error("Cannot declare pointer to non-managed type");
            return -1;
        }
        if (param_is_natural_ptr && param_is_autoptr)
        {
            cc_error("Cannot use '*' with %s", sym.get_name(param_type));
            return -1;
        }
    }
    if ((sym.entries[param_type].flags & SFLG_STRUCTTYPE) && (!param_is_ptr))
    {
        cc_error("A struct cannot be passed as parameter");
        return -1;
    }
    return 0;
}


// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present and a default clause such as "= 5" if present.
int ParseFunc_Paramlist_Param_NameAndDefault(
    ccInternalList *targ,
    bool paramlist_is_declaration,
    AGS::Symbol &param_name,
    bool &param_has_int_default,
    int &param_int_default)
{
    param_name = -1;
    param_has_int_default = false;
    param_int_default = 0;

    if (paramlist_is_declaration)
    {
        // Ignore the parameter name when present, it won't be used later on
        param_name = -1;
        const int next_type = sym.get_type(targ->peeknext());
        if ((next_type == 0) || next_type == SYM_GLOBALVAR)
            targ->getnext();

        // but Get the value of a default assignment, must be returned out of the function
        int retval = ParseParamlist_Param_DefaultValue(targ, param_has_int_default, param_int_default);
        if (retval < 0) return retval;

        return 0;
    }

    // parameter list is a definition
    if (sym.get_type(targ->peeknext()) == SYM_GLOBALVAR)
    {
        // This is a definition -- so the parameter name must not be a global variable
        cc_error("The name '%s' is already used for a global variable", sym.get_name(targ->peeknext()));
        return -1;
    }

    if (sym.get_type(targ->peeknext()) != 0)
    {
        // We need to have a real parameter name here
        cc_error("Expected a parameter name here, found '%s' instead", sym.get_name(targ->peeknext()));
        return -1;
    }

    param_name = targ->getnext(); // get and gobble the parameter name

    return 0;
}


// process a parameter decl in a function parameter list, something like int foo(INT BAR
int ParseFunc_Paramlist_Param(
    ccInternalList * targ,
    ccCompiledScript * scrip,
    AGS::Symbol funcsym,
    Importness func_is_import,
    AGS::Symbol cursym,
    bool param_is_const,
    int param_idx)
{
    // Determine the parameter type and gobble it completely
    // (Note: Later on, the type might turn out to be dynamic array)
    int param_type = cursym;
    bool param_is_ptr = false;

    int retval = ParseFunc_Paramlist_ParamType(targ, param_type, param_is_ptr);
    if (retval < 0) return retval;

    if (ReachedEOF(targ))
    {
        cc_error("Reached end of input within an open parameter list");
        return -1;
    }

    // Currently, a parameter list is seen as a declaration (instead of a definition) 
    // when the function statement has the "import" keyword. 
    // A declaration doesn't have a function body following. 
    // We create local variables iff we are within a definition
    bool paramlist_is_declaration = (func_is_import != kIm_NoImport);
    bool createdLocalVar = !paramlist_is_declaration;

    // Process the parameter name (when present and meaningful) and the default (when present)
    AGS::Symbol param_name;
    bool param_has_int_default = false;
    int param_int_default;
    retval = ParseFunc_Paramlist_Param_NameAndDefault(
        targ, paramlist_is_declaration, param_name, param_has_int_default, param_int_default);
    if (retval < 0) return retval;

    // Process a dynamic array signifier (when present)
    retval = ParseParamlist_Param_DynArrayMarker(targ, cursym, param_is_ptr);
    if (retval < 0) return retval;
    bool param_is_dynarray = (retval == 1);

    // If a local variable has been created, enter this into the symbol table
    if (createdLocalVar)
    {
        sym.entries[param_name].stype = SYM_LOCALVAR;
        sym.entries[param_name].extends = false;
        sym.entries[param_name].arrsize = 1;
        sym.entries[param_name].vartype = param_type;
        sym.entries[param_name].ssize = 4;
        sym.entries[param_name].sscope = 1;
        sym.entries[param_name].flags |= SFLG_PARAMETER;
        if (param_is_ptr)
            sym.entries[param_name].flags |= SFLG_POINTER;
        if (param_is_const)
            sym.entries[param_name].flags |= SFLG_CONST | SFLG_READONLY;
        if (param_is_dynarray)
            sym.entries[param_name].flags |= SFLG_DYNAMICARRAY | SFLG_ARRAY;
        // the parameters are pushed backwards, so the top of the
        // stack has the first parameter. The +1 is because the
        // call will push the return address onto the stack as well
        sym.entries[param_name].soffs = scrip->cur_sp - (param_idx + 1) * 4;
    }

    // Augment the function type in the symbol table  
    sym.entries[funcsym].funcparamtypes[param_idx] = param_type;
    if (param_has_int_default)
    {
        sym.entries[funcsym].funcParamHasDefaultValues[param_idx] = param_has_int_default;
        sym.entries[funcsym].funcParamDefaultValues[param_idx] = param_int_default;
    }

    if (param_is_ptr)
        sym.entries[funcsym].funcparamtypes[param_idx] |= STYPE_POINTER;
    if (param_is_const)
        sym.entries[funcsym].funcparamtypes[param_idx] |= STYPE_CONST;
    if (param_is_dynarray)
        sym.entries[funcsym].funcparamtypes[param_idx] |= STYPE_DYNARRAY;

    return 0;
}


int ParseFuncdecl_Paramlist(
    ccInternalList * targ,
    ccCompiledScript * scrip,
    AGS::Symbol funcsym,
    Importness func_is_import,
    int &numparams)
{
    bool param_is_const = false;
    while (true)
    {
        const AGS::Symbol cursym = targ->getnext();
        const AGS::Symbol curtype = sym.get_type(cursym);

        if (curtype == SYM_CLOSEPARENTHESIS)
            return 0;

        switch (curtype)
        {
        default:
            cc_error("Unexpected %s in parameter list", sym.get_name(cursym));
            return -1;

        case SYM_CONST:
        {
            // type must follow
            if (sym.get_type(targ->peeknext()) != SYM_VARTYPE)
            {
                cc_error("Expected a type after 'const'");
                return -1;
            }
            param_is_const = true;
            break;
        }

        case SYM_VARARGS:
        {
            numparams += VARARGS_INDICATOR;
            if (sym.get_type(targ->peeknext()) != SYM_CLOSEPARENTHESIS)
            {
                cc_error("Expected ')' after '...'");
                return -1;
            }
            break;
        }

        case SYM_VARTYPE:
        {
            if ((numparams % VARARGS_INDICATOR) >= MAX_FUNCTION_PARAMETERS)
            {
                cc_error("Too many parameters defined for function (max. allowed: %d)", MAX_FUNCTION_PARAMETERS - 1);
                return -1;
            }

            int retval = ParseFunc_Paramlist_Param(targ, scrip, funcsym, func_is_import, cursym, param_is_const, numparams);
            if (retval < 0) return retval;
            numparams++;
            param_is_const = false; // modifier has been used up
            const int nexttype = sym.get_type(targ->peeknext());
            if (nexttype != SYM_COMMA && nexttype != SYM_CLOSEPARENTHESIS)
            {
                cc_error("Expected ',' or ')' but found '%s'", sym.get_name(cursym));
            }

            if (nexttype == SYM_COMMA)
                targ->getnext();
            break;
        }
        } // switch
    } // while 
    return 0;
}


int ParseFuncdecl_CheckForIllegalCombis(bool func_is_readonly, AGS::Symbol name_of_current_func, size_t stack_depth)
{
    if (func_is_readonly)
    {
        cc_error("Readonly cannot be applied to a function");
        return -1;
    }

    if ((name_of_current_func >= 0) || (stack_depth > 1))
    {
        cc_error("Nested functions not supported (you may have forgotten a closing brace)");
        return -1;
    }
    return 0;
}


int ParseFuncdecl_SetFunctype(
    AGS::Symbol funcsym,
    int ret_parameter_size,
    int return_type,
    bool func_returns_ptr,
    bool func_returns_dynarray,
    bool func_is_static)
{
    sym.entries[funcsym].stype = SYM_FUNCTION;
    sym.entries[funcsym].ssize = ret_parameter_size;
    sym.entries[funcsym].funcparamtypes[0] = return_type;

    if (func_returns_ptr)
        sym.entries[funcsym].funcparamtypes[0] |= STYPE_POINTER;
    if (func_returns_dynarray)
        sym.entries[funcsym].funcparamtypes[0] |= STYPE_DYNARRAY;

    if ((!func_returns_ptr) && (!func_returns_dynarray) &&
        ((sym.entries[return_type].flags & SFLG_STRUCTTYPE) != 0))
    {
        cc_error("Cannot return entire struct from function");
        return -1;
    }
    if (func_is_static)
        sym.entries[funcsym].flags |= SFLG_STATIC;

    return 0;
}


// there was a forward declaration -- check that the real declaration matches it
int ParseFuncdecl_CheckThatForwardDeclMatches(AGS::Symbol funcname, SymbolTableEntry *prototype)
{
    if (prototype->stype != sym.entries[funcname].stype)
    {
        cc_error("Type of identifier differs from original declaration");
        return -1;
    }

    if (prototype->flags != (sym.entries[funcname].flags & ~SFLG_IMPORTED))
    {
        cc_error("Attributes of identifier do not match prototype");
        return -1;
    }

    if (prototype->ssize != sym.entries[funcname].ssize)
    {
        cc_error("Size of identifier does not match prototype");
        return -1;
    }

    if ((sym.entries[funcname].flags & SFLG_ARRAY) && (prototype->arrsize != sym.entries[funcname].arrsize))
    {
        cc_error("Array size '%d' of identifier does not match prototype which is '%d'", sym.entries[funcname].arrsize, prototype->arrsize);
        return -1;
    }

    if (prototype->stype != SYM_FUNCTION)
        return 0;

    // Following checks pertain to functions
    if (prototype->sscope != sym.entries[funcname].sscope)
    {
        cc_error("Function declaration has wrong number of arguments to prototype");
        return -1;
    }

    if (prototype->funcparamtypes.at(0) != sym.entries[funcname].funcparamtypes.at(0))
    {
        cc_error("Return type does not match prototype");
        return -1;
    }

    // this is 1 .. get_num_args(), INCLUSIVE, because param 0 is the return type
    for (int param_idx = 1; param_idx <= sym.entries[funcname].get_num_args(); param_idx++)
    {
        if (prototype->funcparamtypes.at(param_idx) != sym.entries[funcname].funcparamtypes.at(param_idx))
        {
            cc_error("Type of parameter no. %d does not match prototype", param_idx);
            return -1;
        }

        // copy the default values from the function prototype
        sym.entries[funcname].funcParamDefaultValues.push_back(prototype->funcParamDefaultValues.at(param_idx));
        sym.entries[funcname].funcParamHasDefaultValues.push_back(prototype->funcParamHasDefaultValues.at(param_idx));
    }

    return 0;
}

// We're at something like "int foo(", directly before the "("
// This might or might not be within a struct defn
int ParseFuncdecl(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    AGS::Symbol &name_of_func,
    int return_type, // from "vtwas" in the caller - is the return type in here
    bool func_returns_ptr,
    bool func_returns_dynarray,
    bool func_is_static,
    Importness func_is_import,  // NOT a bool: it can contain 0 .. 2
    AGS::Symbol struct_containing_the_func,
    AGS::Symbol &struct_extended_by_the_func, // the BAR in "int FOO(this BAR *," 
    bool &body_follows)
{
    int numparams = 1; // Counts the number of parameters including the ret parameter, so start at 1
    int ret_parameter_size = sym.entries[return_type].ssize;
    int idx_of_func;

    body_follows = false; // assume by default that a body won't follow

    // Internal name of the function being defined, may be re-written (mangled) later on
    std::string functionNameStr = sym.get_name(name_of_func);

    // skip the opening "("
    targ->getnext();
    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    bool func_is_static_extender = (sym.get_type(targ->peeknext()) == SYM_STATIC);
    bool func_is_extender = (func_is_static_extender) || (targ->peeknext() == sym.find("this"));

    // If the function had a forward declaration, it will go here.
    SymbolTableEntry forward_decl;
    forward_decl.stype = 0;

    // Set up the function
    if (func_is_extender)
    {
        int retval = ParseFuncdecl_ExtenderPreparations(
            targ, scrip, func_is_static_extender,
            func_is_import, functionNameStr,
            name_of_func, struct_extended_by_the_func,
            &forward_decl);
        if (retval < 0) return retval;
    }
    else // !func_is_extender
    {
        int retval = ParseFuncdecl_NonExtenderPreparations(
            scrip,
            func_is_import, struct_containing_the_func, functionNameStr,
            name_of_func, &forward_decl);
        if (retval < 0) return retval;
    }

    // Type the function in the symbol table
    int retval = ParseFuncdecl_SetFunctype(
        name_of_func, ret_parameter_size, return_type, func_returns_ptr,
        func_returns_dynarray, func_is_static);
    if (retval < 0) return retval;

    // Get function number and function index
    int funcNum = -1;

    // Enter the function in the imports[] or functions[] array; get its index
    if (func_is_import != kIm_NoImport)
    {
        // Index of the function in the ccScript::imports[] array
        // Note: currently, add_new_import() cannot return values < 0, so idx_of_func must be >= 0 here
        idx_of_func = scrip->add_new_import(functionNameStr.c_str());
    }
    else
    {
        // Index of the function in the ccCompiledScript::functions[] array
        idx_of_func = scrip->add_new_function(functionNameStr.c_str(), &funcNum);
        if (idx_of_func < 0)
        {
            cc_error("Max. number of functions exceeded");
            return -1;
        }
    }

    sym.entries[name_of_func].soffs = idx_of_func;  // save index of the function
    scrip->cur_sp += 4;  // the return address will be pushed

    // process parameter list, get number of parameters
    retval = ParseFuncdecl_Paramlist(targ, scrip, name_of_func, func_is_import, numparams);
    if (retval < 0) return retval;

    // save the number of parameters (not counting the ret parameter)
    sym.entries[name_of_func].sscope = (numparams - 1);
    if (funcNum >= 0)
        scrip->funcnumparams[funcNum] = sym.entries[name_of_func].sscope;

    // If there was a forward decl, it has to match the current decl.
    if (forward_decl.stype != 0)
    {
        retval = ParseFuncdecl_CheckThatForwardDeclMatches(name_of_func, &forward_decl);
        if (retval < 0) return retval;
    }

    // Non-imported functions must be followed by a body
    if (func_is_import == kIm_NoImport)
    {
        if (sym.get_type(targ->peeknext()) != SYM_OPENBRACE)
        {
            cc_error("Expected '{'");
            return -1;
        }
        body_follows = true;
        return 0;
    }

    // import functions
    sym.entries[name_of_func].flags |= SFLG_IMPORTED;

    if (struct_containing_the_func > 0)
    {
        // Append the number of parameters to the name of the import
        char appendage[10];
        sprintf(appendage, "^%d", sym.entries[name_of_func].sscope);
        strcat(scrip->imports[idx_of_func], appendage);
    }

    // member function expects the ';' to still be there whereas normal function does not
    AGS::Symbol nextvar = targ->peeknext();
    if (struct_containing_the_func == 0)
        nextvar = targ->getnext();

    if (sym.get_type(nextvar) != SYM_SEMICOLON)
    {
        cc_error("';' expected (cannot define body of imported function)");
        return -1;
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


// Whether the current symbol can still be part of the expression
bool CanBePartOfExpression(ccInternalList *targ, size_t script_idx)
{
    // NOTEXPRESSION is so defined that all lower symbols can be part of an expression
    if (sym.get_type(targ->script[script_idx]) < NOTEXPRESSION)
        return true;

    // "new TYPE" is legal, too (reserving dynamic space)
    if (sym.get_type(targ->script[script_idx]) == SYM_NEW)
        return true;
    if ((sym.get_type(targ->script[script_idx]) == SYM_VARTYPE) &&
        (script_idx > 0) &&
        (sym.get_type(targ->script[script_idx - 1]) == SYM_NEW))
    {
        return true;
    }

    // "TYPE ." is legal, too (accessing a component of a static class)
    if (((int)script_idx < targ->length - 1) &&
        (sym.get_type(targ->script[script_idx + 1]) == SYM_DOT) &&
        (sym.get_type(targ->script[script_idx]) == SYM_VARTYPE))
    {
        return true;
    }

    return false;
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
    if (valtype == 0)
        return false;
    const char *type_string = sym.get_name(valtype);
    if (!type_string)
        return false;

    if (strcmp(type_string, "const string") == 0)
        return true;
    if (strcmp(type_string, "string") == 0)
        return true;
    if (strcmp(type_string, "char*") == 0)
        return true;

    return false;
}


// Change the generic operator vcpuOp to the one that is correct for the types
int GetOperatorValidForType(int type1, int type2, int &vcpuOp)
{
    if ((type1 == sym.normalFloatSym) || (type2 == sym.normalFloatSym))
    {
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
    }

    if (is_any_type_of_string(type1) && is_any_type_of_string(type2))
    {
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to string values");
            return -1;
        case SCMD_ISEQUAL:  vcpuOp = SCMD_STRINGSEQUAL; return 0;
        case SCMD_NOTEQUAL: vcpuOp = SCMD_STRINGSNOTEQ; return 0;
        }
    }

    if ((type1 & STYPE_POINTER) != 0 && (type2 & STYPE_POINTER) != 0)
    {
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to pointers");
            return -1;
        case SCMD_ISEQUAL:  return 0;
        case SCMD_NOTEQUAL: return 0;
        }
    }

    // Other combinations of pointers and/or strings won't mingle
    if (is_string(type1) ||
        is_string(type2) ||
        (type1 & STYPE_POINTER) != 0 ||
        (type2 & STYPE_POINTER) != 0)
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
    if ((typeIs == (STYPE_POINTER | sym.nullSym)) && ((typeWantsToBe & STYPE_DYNARRAY) != 0))
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
        if (typeIs == (STYPE_POINTER | sym.nullSym))
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
    bool typeIsIsStruct = (0 != (sym.entries[typeIs].flags & SFLG_STRUCTTYPE));
    bool typeWantsToBeIsStruct = (0 != (sym.entries[typeWantsToBe].flags & SFLG_STRUCTTYPE));
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
        sym.get_friendly_name(typeIs).c_str(),
        sym.get_friendly_name(typeWantsToBe).c_str());
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


int BufferVarOrFunccall_HandleFuncCall(ccInternalList *targ, int & funcAtOffs, const AGS::SymbolScript &slist, size_t & slist_len)
{
    int paren_expr_startline = currentline;
    funcAtOffs = slist_len - 1;
    slist[slist_len++] = targ->getnext();

    if (sym.get_type(slist[slist_len - 1]) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }

    // include the member function params in the returned value
    size_t paren_nesting_depth = 1; // No. of '(' that wait for their matching ')'
    while (paren_nesting_depth > 0)
    {
        if (ReachedEOF(targ))
        {   // [fw] Can't be reached IMO. The tokenizer sees to it that delimiters match
            currentline = paren_expr_startline;
            cc_error("The '(' on line %d does not have a matching ')'", paren_expr_startline);
            return -1;
        }

        if (slist_len >= TEMP_SYMLIST_LENGTH - 1)
        {
            cc_error("Buffer exceeded: The '(' on line %d probably does not have a matching ')'", paren_expr_startline);
            return -1;
        }

        slist[slist_len++] = targ->getnext();
        if (sym.get_type(slist[slist_len - 1]) == SYM_CLOSEPARENTHESIS)
            paren_nesting_depth--;
        if (sym.get_type(slist[slist_len - 1]) == SYM_OPENPARENTHESIS)
            paren_nesting_depth++;
    }

    return 0;
}

// We have accepted something like "m.a."
int BufferVarOrFunccall_GotDot(ccInternalList *targ, bool justHadBrackets, AGS::SymbolScript slist, size_t &slist_len, AGS::Symbol &current_member, int &funcAtOffs)
{
    bool mustBeStaticMember = false;

    // Get the type of the component before the dot.
    AGS::Symbol current_member_type = 0;
    if (sym.get_type(current_member) == SYM_VARTYPE)
    {
        // static member access, eg. "Math.Func()"
        mustBeStaticMember = true;
        current_member_type = current_member;
    }
    else
    {
        current_member_type = sym.entries[current_member].vartype;
        if (current_member_type < 1)
        {
            cc_error("structure required on left side of '.'");
            return -1;
        }
    }

    if (((sym.entries[current_member].flags & SFLG_ARRAY) != 0) && (!justHadBrackets))
    {
        cc_error("'[' expected");
        return -1;
    }

    // allow protected member access with the "this" ptr only
    bool allowProtectedMembers = false;
    if (sym.entries[current_member].flags & SFLG_THISPTR)
        allowProtectedMembers = true;

    // Get the symbol to the right of the dot
    slist[slist_len++] = targ->getnext();

    // convert the member's sym to the structmember version
    int retval = FindMemberSym(current_member_type, slist[slist_len - 1], allowProtectedMembers);
    if (retval < 0) return retval;

    if ((sym.entries[slist[slist_len - 1]].flags & SFLG_STRUCTMEMBER) == 0)
    {
        cc_error("structure member required after '.'");
        return -1;
    }

    if ((mustBeStaticMember) && (0 == (sym.entries[slist[slist_len - 1]].flags & SFLG_STATIC)))
    {
        cc_error("Must have an instance of the struct to access a non-static member");
        return -1;
    }

    current_member = slist[slist_len - 1];

    if (sym.get_type(current_member) == SYM_FUNCTION)
    {
        // The symbol after the dot was a function name. This function is called.
        // We encountered something like s.m; we're waiting for '('
        int retval = BufferVarOrFunccall_HandleFuncCall(targ, funcAtOffs, slist, slist_len);
        if (retval < 0) return retval;
    }

    return 0;
}


// We've read something like "a.b.c[", we've already stored the '['
int BufferVarOrFunccall_GotOpenBracket(ccInternalList *targ, AGS::SymbolScript slist, size_t &slist_len)
{

    if ((sym.entries[slist[slist_len - 2]].flags & SFLG_ARRAY) == 0)
    {
        cc_error("%s is not an array", sym.get_friendly_name(slist[slist_len - 2]).c_str());
        return -1;
    }

    if (sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
    {
        cc_error("Array index not specified");
        return -1;
    }

    size_t bracket_nesting_depth = 1;
    int bracket_expr_startline = currentline;

    // accept and buffer the contents of the brackets and the closing bracket
    // comma is allowed because you can have e.g. array[func(a,b)]
    // vartype is allowed to permit access to static members, e.g. array[Game.GetColorFromRGB(0, 0, 0)]
    while (bracket_nesting_depth > 0)
    {
        AGS::Symbol next_symbol = targ->getnext();
        if (next_symbol == SCODE_INVALID)
        {
            currentline = bracket_expr_startline;
            cc_error("The '[' on line %d does not have a matching ']'", bracket_expr_startline);
            return -1;
        }

        if ((sym.get_type(next_symbol) >= NOTEXPRESSION) &&
            (sym.get_type(next_symbol) != SYM_COMMA) &&
            !((sym.get_type(next_symbol) == SYM_VARTYPE) && (sym.entries[slist[next_symbol]].flags & SFLG_STRUCTTYPE)))
        {
            cc_error("Unexpected symbol '%s'", sym.get_friendly_name(next_symbol).c_str());
            return -1;
        }

        slist[slist_len++] = next_symbol;
        if (sym.get_type(next_symbol) == SYM_CLOSEBRACKET)
            bracket_nesting_depth--;
        if (sym.get_type(next_symbol) == SYM_OPENBRACKET)
            bracket_nesting_depth++;
        if (slist_len >= TEMP_SYMLIST_LENGTH - 1)
        {
            cc_error("Buffer exceeded, the '[' on line %d probably does not have a matching ']'", bracket_expr_startline);
            return -1;
        }
    }

    return 0;
}


// Copies the parts of a  variable name or array expression or function call into slist[]
// If there isn't any of this here, this will return without error
int BufferVarOrFunccall(ccInternalList *targ, AGS::Symbol fsym, AGS::SymbolScript slist, size_t &slist_len, int &funcAtOffs)
{
    funcAtOffs = -1;

    bool mustBeStaticMember = false;

    // We read and buffer one of the following cases:
    // - A function call
    // - A loadable var
    // - A struct, then '.', then a struct member
    // - A type, then '.', then a static member
    // If that member is a function call, we read and buffer that function call
    // If that member is an array and we have [, we read the array expression

    slist_len = 0;
    slist[slist_len++] = fsym;


    if (sym.get_type(fsym) == SYM_FUNCTION)
        return BufferVarOrFunccall_HandleFuncCall(targ, funcAtOffs, slist, slist_len);

    // Must be variable or type
    if (!sym.entries[fsym].is_loadable_variable() && sym.get_type(fsym) != SYM_VARTYPE)
            return 0;

    bool justHadBrackets = false;

    for (int nexttype = sym.get_type(targ->peeknext());
        nexttype == SYM_DOT || nexttype == SYM_OPENBRACKET;
        nexttype = sym.get_type(targ->peeknext()))
    {
        if (ReachedEOF(targ))
        {
            cc_error("Dot operator must be followed by member function or attribute");
            return -1;
        }

        if (slist_len >= TEMP_SYMLIST_LENGTH - 5)
        {
            cc_error("Name expression too long: Probably a ']' was missing above.");
            return -1;
        }

        // store the '.' or '['
        slist[slist_len++] = targ->getnext();

        switch (nexttype)
        {
        default: // This can't happen
            cc_error("Internal error: '.' or '[' expected");
            return -99;

        case SYM_DOT:
        {
            int retval = BufferVarOrFunccall_GotDot(targ, justHadBrackets, slist, slist_len, fsym, funcAtOffs);
            if (retval < 0) return retval;
            justHadBrackets = false;
            break;
        }

        case SYM_OPENBRACKET:
        {
            int retval = BufferVarOrFunccall_GotOpenBracket(targ, slist, slist_len);
            if (retval < 0) return retval;
            justHadBrackets = true;
            break;
        }
        } // switch (nexttype)
    }
    return 0;
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


    void SetAXScope(ccCompiledScript *scrip, int scrip_idx)
    {
        scrip->ax_val_scope = SYM_GLOBALVAR;

        // "null" is considered to be a global var
        if (sym.get_type(scrip_idx) == SYM_NULL)
            return;

        // if it's a parameter, pretend it's a global var
        // this allows it to be returned back from the function
        if (sym.entries[scrip_idx].flags & SFLG_PARAMETER)
            return;

        scrip->ax_val_scope = sym.entries[scrip_idx].stype;
    }


    int FindClosingBracketOffs(size_t openBracketOffs, AGS::SymbolScript symlist, size_t symlist_len, size_t &brac_idx)
    {
        int nesting_depth = 0;
        for (brac_idx = openBracketOffs + 1; brac_idx < symlist_len; brac_idx++)
        {
            int symtype = sym.get_type(symlist[brac_idx]);
            if ((symtype == SYM_OPENBRACKET) || (symtype == SYM_OPENPARENTHESIS))
                nesting_depth++;
            if ((symtype == SYM_CLOSEBRACKET) || (symtype == SYM_CLOSEPARENTHESIS))
            {
                nesting_depth--;
                if (nesting_depth < 0)
                    return 0;
            }
        }
        // Did not find it
        return -1;
    }


    int FindOpeningBracketOffs(size_t closeBracketOffs, AGS::SymbolScript symlist, size_t &brac_idx)
    {
        int nesting_depth = 0;

        // don't convert to for loop, "for(..., brac_idx >= 0,...)" will NOT work
        brac_idx = closeBracketOffs;
        do
        {
            brac_idx--;
            int symtype = sym.get_type(symlist[brac_idx]);
            if ((symtype == SYM_OPENBRACKET) || (symtype == SYM_OPENPARENTHESIS))
            {
                nesting_depth--;
                if (nesting_depth < 0)
                    return 0;
            }
            if ((symtype == SYM_CLOSEBRACKET) || (symtype == SYM_CLOSEPARENTHESIS))
                nesting_depth++;
        }
        while (brac_idx > 0);
        // Didn't find it
        return -1;
    }


    int AccessData_SplitPathIntoParts(VariableSymlist *variablePath, AGS::SymbolScript syml, size_t syml_len, size_t &variablePathSize)
    {
        variablePathSize = 0;
        int lastOffs = 0;
        size_t syml_idx;

        // Separate out syml into a VariablePath for the clause
        // between each dot. If it's just a simple variable access,
        // we will only create one.
        for (syml_idx = 0; syml_idx < syml_len; syml_idx++)
        {
            if ((sym.get_type(syml[syml_idx]) == SYM_OPENBRACKET) ||
                (sym.get_type(syml[syml_idx]) == SYM_OPENPARENTHESIS))
            {
                // an array index, skip it
                FindClosingBracketOffs(syml_idx, syml, syml_len, syml_idx);
            }

            bool createPath = false;

            if (sym.get_type(syml[syml_idx]) == SYM_DOT)
            {
                createPath = true;
            }
            else if (syml_idx >= syml_len - 1)
            {
                // end of data stream, store the last bit
                syml_idx++;
                createPath = true;
            }

            if (!createPath)
                continue;

            if (variablePathSize >= MAX_VARIABLE_PATH)
            {
                cc_error("Variable path too long");
                return -1;
            }

            VariableSymlist *vpp = &variablePath[variablePathSize];
            vpp->init(syml_idx - lastOffs);
            for (int vsyml_idx = 0; vsyml_idx < vpp->len; vsyml_idx++)
                vpp->syml[vsyml_idx] = syml[lastOffs + vsyml_idx];
            lastOffs = syml_idx + 1;
            variablePathSize++;
        }

        return 0;
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


    int ParseSubexpr_NewIsFirst(ccCompiledScript * scrip, const AGS::SymbolScript & symlist, size_t symlist_len)
    {
        if (symlist_len < 2 || sym.get_type(symlist[1]) != SYM_VARTYPE)
        {
            cc_error("Expected a type after 'new'");
            return -1;
        }

        // "new TYPE", nothing following
        if (symlist_len <= 3)
        {
            if (sym.entries[symlist[1]].flags & SFLG_BUILTIN)
            {
                cc_error("Built-in type '%s' cannot be instantiated directly", sym.get_name(symlist[1]));
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
            if (sym.entries[arrayType].flags & SFLG_MANAGED)
            {
                isManagedType = true;
                size = 4;
            }
            else if (sym.entries[arrayType].flags & SFLG_STRUCTTYPE)
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

        cc_error("Unexpected characters following 'new %s'", sym.get_name(symlist[1]));
        return -1;
    }


    // We're parsing an expression that starts with '-' (unary minus)
    int ParseSubexpr_UnaryMinusIsFirst(ccCompiledScript *scrip, const AGS::SymbolScript & symlist, size_t symlist_len)
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
        cc_error("Unexpected operator '%s' without a preceding expression", sym.get_friendly_name(symlist[0]).c_str());
        return -1;
    }


    // The lowest-binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
    int ParseSubexpr_OpIsSecondOrLater(ccCompiledScript * scrip, size_t op_idx, const AGS::SymbolScript &symlist, size_t symlist_len)
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
            cc_error("Parse error: invalid use of operator '%s'", sym.get_friendly_name(symlist[op_idx]).c_str());
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


    int ParseSubexpr_OpenParenthesis(ccCompiledScript * scrip, AGS::SymbolScript & symlist, size_t symlist_len)
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


    // We're in the parameter list of a function call, and we have less parameters than declared.
    // Provide defaults for the missing values
    int ParseSubexpr_FunctionCall_ProvideDefaults(ccCompiledScript * scrip, int num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol)
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

            if (sym.entries[funcSymbol].flags & SFLG_IMPORTED)
                scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
            else
                scrip->push_reg(SREG_AX);
        }
        return 0;
    }


    int ParseSubexpr_FunctionCall_PushParams(ccCompiledScript * scrip, const AGS::SymbolScript &paramList, size_t closedParenIdx, size_t num_func_args, size_t num_supplied_args, AGS::Symbol funcSymbol)
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
            if (ParseSubexpr(scrip, &paramList[start_of_this_param], end_of_this_param - start_of_this_param))
                return -1;

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

            if (sym.entries[funcSymbol].flags & SFLG_IMPORTED)
                scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
            else
                scrip->push_reg(SREG_AX);

            end_of_this_param = start_of_this_param - 1;

        }
        while (end_of_this_param > 0);

        return 0;
    }


    // Count parameters, check that all the parameters are non-empty; find closing paren
    int ParseSubexpr_FunctionCall_CountAndCheckParm(const AGS::SymbolScript &paramList, size_t paramListLen, AGS::Symbol funcSymbol, size_t &indexOfCloseParen, size_t &num_supplied_args)
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
            cc_error("Internal error: Parser confused near '%s'", sym.get_friendly_name(funcSymbol).c_str());
            return -1;
        }

        return 0;
    }


    int ParseSubexpr_FunctionCall(ccCompiledScript * scrip, int funcSymbolIdx, AGS::SymbolScript vnlist, AGS::SymbolScript & symlist, size_t & symlist_len)
    {
        // workList is the function call beginning at the func symbol proper
        AGS::SymbolScript workList = symlist;
        int workListLen = symlist_len;
        if (funcSymbolIdx > 0)
        {
            workList = &vnlist[funcSymbolIdx];
            workListLen = symlist_len - funcSymbolIdx;
        }

        AGS::Symbol funcSymbol = workList[0];

        // Make sure that a '(' follows the funcname of the function call 
        if (sym.get_type(workList[1]) != SYM_OPENPARENTHESIS)
        {
            cc_error("Expected '('");
            return -1;
        }

        // paramList begins at the parameters, at the leading '('
        AGS::SymbolScript paramList = workList + 1;
        size_t paramListLen = workListLen - 1;


        // Generate code so that the runtime stack contains, bottom-to-top:
        //      a pointer to "this" if applicable
        //      the parameters in reverse sequence, so that the first parameter will pop off first 

        // Find out whether we use "this"; in this case, generate a push to the stack
        // This is supposed to push a pointer to "this" onto the stack as hidden first argument
        bool using_op = false;
        if (funcSymbolIdx > 0)
        {
            // functions in struct usually use "this" (method calls)
            using_op = true;
            // but static functions don't have an object instance, so no "this"
            if (sym.entries[funcSymbol].flags & SFLG_STATIC)
                using_op = false;
        }

        // push a pointer to the current object onto the stack before the parameters if applicable
        if (using_op)
            scrip->push_reg(SREG_OP);

        // Expected number of arguments, or expected minimal number of arguments
        size_t num_func_args = sym.entries[funcSymbol].get_num_args();
        bool func_is_varargs = (num_func_args >= VARARGS_INDICATOR);

        // Count the parameters and check them
        size_t indexOfClosedParen;
        size_t num_supplied_args;
        int retval = ParseSubexpr_FunctionCall_CountAndCheckParm(paramList, paramListLen, funcSymbol, indexOfClosedParen, num_supplied_args);
        if (retval < 0) return retval;

        // Push default parameters onto the stack when applicable
        // This will give an error if there aren't enough default parameters
        if (num_supplied_args < num_func_args)
        {
            int retval = ParseSubexpr_FunctionCall_ProvideDefaults(scrip, num_func_args, num_supplied_args, funcSymbol);
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
            int retval = ParseSubexpr_FunctionCall_PushParams(scrip, paramList, indexOfClosedParen, num_func_args, num_supplied_args, funcSymbol);
            if (retval < 0) return retval;
        }

        if (using_op)
        {
            // write the address of the function's object to the OP reg
            ReadDataIntoAX(scrip, vnlist, funcSymbolIdx, false);
            scrip->write_cmd1(SCMD_CALLOBJ, SREG_AX);
        }

        size_t actual_num_args = std::max(num_supplied_args, num_func_args);
        if (sym.entries[funcSymbol].flags & SFLG_IMPORTED)
        {
            // tell it how many args for this call (nested imported functions
            // causes stack problems otherwise)
            scrip->write_cmd1(SCMD_NUMFUNCARGS, actual_num_args);
        }

        // Call the function
        AGS::CodeLoc callpoint = sym.entries[funcSymbol].soffs;
        const bool is_import_function = (0 != (sym.entries[funcSymbol].flags & SFLG_IMPORTED));
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, callpoint);

        if (is_import_function)
        {
            scrip->fixup_previous(FIXUP_IMPORT);
            // do the call
            scrip->write_cmd1(SCMD_CALLEXT, SREG_AX);
            if (actual_num_args > 0)
                scrip->write_cmd1(SCMD_SUBREALSTACK, actual_num_args);
        }
        else
        {
            scrip->fixup_previous(FIXUP_FUNCTION);
            scrip->write_cmd1(SCMD_CALL, SREG_AX);

            // We will arrive here when the function call has returned
            // restore the stack
            if (actual_num_args > 0)
            {
                scrip->cur_sp -= actual_num_args * 4;
                scrip->write_cmd2(SCMD_SUB, SREG_SP, actual_num_args * 4);
            }
        }

        // function return type
        // This is an alias for "type of the current expression". 
        scrip->ax_val_type = sym.entries[funcSymbol].funcparamtypes[0];
        scrip->ax_val_scope = SYM_LOCALVAR;

        if (using_op)
            scrip->pop_reg(SREG_OP);

        // Note that this function has been accessed at least once
        sym.entries[funcSymbol].flags |= SFLG_ACCESSED;
        return 0;
    }


    int ParseSubexpr_NoOps(ccCompiledScript * scrip, AGS::SymbolScript symlist, size_t symlist_len)
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
                    int retval = ReadDataIntoAX(scrip, &symlist[1], 1, true);
                    if (retval < 0) return retval;
                    return 0;
                }

                // If there are bogus tokens after a term that begins with unary minus, 
                // then the problem is the bogus tokens, beginning at index 2. 
                // Otherwise, the problem is the unary minus itself, at index 0. 
                cc_error(
                    "Parse error: unexpected '%s'",
                    sym.get_friendly_name(symlist[(symlist_len > 2) ? 2 : 0]).c_str());
                return -1;
            }

            // We don't know this unary operator. "new", perhaps?
            cc_error("Parse error: Unexpected '%s'", sym.get_friendly_name(symlist[0]).c_str());
            return -1;
        }

        // Find out whether this is a variable or function call; if so, copy it to vnlist
        // We need to copy the bytes because a lot of code depends on the fact that there
        // are no METAs interspersed between the symbols. 
        AGS::Symbol vnlist[TEMP_SYMLIST_LENGTH];
        size_t vnlist_len;
        int funcAtOffs = 0;

        // Cast an internal list around symlist
        ccInternalList tlist;
        tlist.pos = 0;
        tlist.script = symlist;
        tlist.length = symlist_len;
        tlist.cancelCurrentLine = 0;

        int retval = BufferVarOrFunccall(&tlist, tlist.getnext(), vnlist, vnlist_len, funcAtOffs);
        // stop tlist trying to free the memory
        tlist.script = NULL;
        tlist.length = 0;
        if (retval < 0) return retval;

        if ((sym.get_type(symlist[0]) == SYM_FUNCTION) || (funcAtOffs > 0))
            return ParseSubexpr_FunctionCall(scrip, funcAtOffs, vnlist, symlist, symlist_len);

        if (symlist_len == 1)
        {
            // Must be a variable or literal, otherwise it's invalid
            int retval = ReadDataIntoAX(scrip, symlist, symlist_len, false);
            if (retval < 0) return retval;
            return 0;
        }

        if (symlist_len == vnlist_len)
        {
            int retval = ReadDataIntoAX(scrip, vnlist, vnlist_len, false);
            if (retval < 0) return retval;
            return 0;
        }

        cc_error("Parse error in expr near '%s'", sym.get_friendly_name(symlist[0]).c_str());
        return -1;
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


    int GetArrayIndexIntoAX(ccCompiledScript *scrip, AGS::SymbolScript symlist, int openBracketOffs, int closeBracketOffs, bool checkBounds, bool multiplySize) {

        // "push" the ax val type (because this is just an array index,
        // we're actually interested in the type of the variable being read)
        int axValTypeWas = scrip->ax_val_type;

        // save the size of the array element, so it doesn't get
        // overwritten by the size of the array index variable
        // [fw] Passing info around through a global variable: That is a HUGE code smell.
        int saveOldReadcmd = SizeUsedInLastReadCommand;
        // parse expression inside brackets to return the array index in AX
        int retval = ParseSubexpr(scrip, &symlist[openBracketOffs + 1], closeBracketOffs - (openBracketOffs + 1));
        if (retval < 0) return retval;
        SizeUsedInLastReadCommand = saveOldReadcmd;

        // array index must be an int
        retval = IsTypeMismatch(scrip->ax_val_type, sym.normalIntSym, true);
        if (retval < 0) return retval;

        // "pop" the ax val type
        scrip->ax_val_type = axValTypeWas;

        AGS::Symbol arrSym = symlist[openBracketOffs - 1];

        if ((sym.entries[arrSym].flags & SFLG_ARRAY) == 0)
        {
            cc_error("Internal error: '%s' is not an array", sym.get_friendly_name(arrSym).c_str());
            return -1;
        }

        if (checkBounds)
        {
            // check the array bounds that have been calculated in AX,
            // before they are added to the overall offset
            if ((sym.entries[arrSym].flags & SFLG_DYNAMICARRAY) == 0)
                scrip->write_cmd2(SCMD_CHECKBOUNDS, SREG_AX, sym.entries[arrSym].arrsize);
        }

        if (multiplySize)
        {
            // multiply up array index (in AX) by size of array element
            // to get memory offset
            scrip->write_cmd2(SCMD_MUL, SREG_AX, sym.entries[arrSym].ssize);
        }

        return 0;
    }


    // parse array brackets
    int AccessData_ParseArrayIndexPresent(ccCompiledScript *scrip, VariableSymlist *thisClause, bool writingOperation, bool &isArrayOffset)
    {

        if ((thisClause->len <= 1) || (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET))
        {
            // No '[', so no array index clause. Return without error.
            return 0;
        }

        // find where the brackets end
        size_t arrIndexEnd;
        FindClosingBracketOffs(1, thisClause->syml, thisClause->len, arrIndexEnd);
        if (arrIndexEnd != thisClause->len - 1)
        {
            cc_error("Unexpected token after array index");
            return -1;
        }

        bool attribute_indexer = false;
        bool checkBounds = true, multiplySize = true;

        if ((sym.entries[thisClause->syml[0]].flags & SFLG_ATTRIBUTE) ||
            (sym.entries[thisClause->syml[0]].flags & SFLG_POINTER))
        {
            // an array attribute or array of pointers; in this case,
            // don't touch CX but just calculate the index value into DX
            attribute_indexer = true;
            multiplySize = false;
            // don't check bounds, the attribute getter will do that
            if (sym.entries[thisClause->syml[0]].flags & SFLG_ATTRIBUTE)
                checkBounds = false;
        }

        // the value to write is in AX; preserve it
        if (writingOperation)
            scrip->push_reg(SREG_AX);

        // save the current offset in CX if there is one,
        // because ParseSubexpr might destroy it
        if (isArrayOffset)
            scrip->push_reg(SREG_CX);

        // get the byte offset of the array index into AX
        int retval = GetArrayIndexIntoAX(scrip, thisClause->syml, 1, arrIndexEnd, checkBounds, multiplySize);
        if (retval < 0) return retval;

        // if there is a current offset saved in CX, restore it
        // then add the result to CX (which is counting the overall offset)
        if (isArrayOffset)
        {
            scrip->pop_reg(SREG_CX);
            if (attribute_indexer)
                scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_DX);
            else
                scrip->write_cmd2(SCMD_ADDREG, SREG_CX, SREG_AX);
        }
        else
        {
            scrip->write_cmd2(SCMD_REGTOREG,
                SREG_AX,
                (attribute_indexer ? SREG_DX : SREG_CX));
        }

        if (!attribute_indexer)
            isArrayOffset = true;

        if (writingOperation)
            scrip->pop_reg(SREG_AX);

        // the array offset has now been added to CX (normal array)
        // or put into DX (attribute)

        return 0;
    }


    // We access a variable or a component of a struct in order to read or write it.
    // This is a simple member of the struct.
    inline void AccessData_PrepareComponentAccess_Elementary(AGS::Symbol variableSym, int & currentComponentOffset)
    {

        // since the member has a fixed offset into the structure, don't
        // write out any code to calculate the offset - instead, modify
        // the hard offset value which will be written to MAR
        currentComponentOffset += sym.entries[variableSym].soffs;
    }


    // We access a component of a struct in order to read or write it. 
    // This is a function that is a member of a struct.
    inline int AccessData_PrepareComponentAccess_MemberFunction(bool isLastClause, bool & getJustTheAddressIntoAX, bool & doMemoryAccessNow)
    {
        // This is only possible if it is last in the clause
        if (!isLastClause)
        {
            cc_error("Function().Member not supported");
            return -1;
        }
        // A function isn't _really_ part of a struct. In reality, it's just a 
        // "normal" function that takes the struct as an (implicit) first parameter.
        // So what we need is the address of the struct itself to be able to process the function call
        getJustTheAddressIntoAX = true;
        doMemoryAccessNow = true;
        return 0;
    }


    // We access a component of a struct in order to read or write it. 
    // This is an attribute.
    int AccessData_PrepareComponentAccess_Attribute(ccCompiledScript *scrip, AGS::Symbol variableSym, VariableSymlist *thisClause, bool writing, bool writingThisTime, bool mustBeWritable, bool &getJustTheAddressIntoAX, bool &doMemoryAccessNow, bool &isArrayOffset)
    {
        // since an attribute is effectively a function call, load the address of the object
        getJustTheAddressIntoAX = true;
        doMemoryAccessNow = true;

        int retval = AccessData_ParseArrayIndexPresent(scrip, thisClause, writing != 0, isArrayOffset);
        if (retval < 0) return retval;

        if (writing)
        {
            if ((writingThisTime) && (sym.entries[variableSym].flags & SFLG_READONLY))
            {
                cc_error("Attribute '%s' is read-only", sym.get_friendly_name(variableSym).c_str());
                return -1;
            }

            // Attribute Set -- move the new value into BX, so
            // that the object address can be retrieved into AX
            scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
        }

        return 0;
    }


    // We access a variable or a component of a struct in order to read or write it.
    // This is a pointer
    int AccessData_PrepareComponentAccess_Pointer(ccCompiledScript * scrip, AGS::Symbol variableSym, VariableSymlist * thisClause, int currentByteOffset, bool & isDynamicArray, bool writing, AGS::Symbol firstVariableType, AGS::Symbol firstVariableSym, bool isLastClause, bool pointerIsOnStack, bool & isArrayOffset, bool & getJustTheAddressIntoAX, int & currentComponentOffset, bool & accessActualPointer, bool & doMemoryAccessNow)
    {
        bool isArrayOfPointers = false;

        if (sym.entries[variableSym].flags & SFLG_ARRAY)
        {
            // array of pointers

            if ((thisClause->len <= 1) ||
                (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET))
            {
                // normally, the whole array can be used as a pointer.
                // this is not the case with an pointer array, so catch
                // it here and give an error
                if ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0)
                {
                    cc_error("Expected array index after '%s'", sym.get_friendly_name(variableSym).c_str());
                    return -1;
                }
                isDynamicArray = true;
            }
            else
            {
                // put array index into DX
                int retval = AccessData_ParseArrayIndexPresent(scrip, thisClause, writing, isArrayOffset);
                if (retval < 0) return retval;

                isArrayOfPointers = true;
            }
        }

        // if they are just saying "ptr" (or doing a "ptr.Func" call)
        // then move the address being pointed to into AX
        // (member function call passes in "ptr.")
        if (isLastClause)
            getJustTheAddressIntoAX = true;

        // Push the pointer address onto the stack, where it can be
        // retrieved by MemoryAccess later on
        if (sym.entries[variableSym].flags & SFLG_THISPTR)
        {
            if (pointerIsOnStack)
            {
                // already a pointer on the stack
                cc_error("Internal error: Found nested this pointers");
                return -1;
            }

            // for the "this" pointer, just use the Object Pointer
            scrip->push_reg(SREG_OP);
            currentComponentOffset = 0;
            return 0;
        }


        if (pointerIsOnStack)
        {
            // already a pointer on the stack
            scrip->pop_reg(SREG_MAR);
            scrip->write_cmd(SCMD_CHECKNULL);
            if (currentComponentOffset > 0)
                scrip->write_cmd2(SCMD_ADD, SREG_MAR, currentComponentOffset);
        }
        else if (firstVariableType == SYM_LOCALVAR)
        {
            scrip->write_cmd1(SCMD_LOADSPOFFS, scrip->cur_sp - currentComponentOffset);
        }
        else if (firstVariableType == SYM_GLOBALVAR)
        {
            if (sym.entries[firstVariableSym].flags & SFLG_IMPORTED)
            {
                scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, sym.entries[firstVariableSym].soffs);
                scrip->fixup_previous(FIXUP_IMPORT);
                if (currentComponentOffset != 0)
                    scrip->write_cmd2(SCMD_ADD, SREG_MAR, currentByteOffset);
            }
            else
            {
                scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, currentByteOffset);
                scrip->fixup_previous(FIXUP_GLOBALDATA);
            }
        }
        else
        {
            cc_error("Invalid type for pointer");
            return -1;
        }

        // if an array, the array indexer was put into DX
        if (isArrayOfPointers)
        {
            scrip->write_cmd2(SCMD_MUL, SREG_DX, 4);

            if (sym.entries[variableSym].flags & SFLG_DYNAMICARRAY)
            {
                // pointer to an array -- dereference the pointer
                scrip->write_cmd1(SCMD_MEMREADPTR, SREG_MAR);
                scrip->write_cmd(SCMD_CHECKNULL);
                scrip->write_cmd1(SCMD_DYNAMICBOUNDS, SREG_DX);
            }

            scrip->write_cmd2(SCMD_ADDREG, SREG_MAR, SREG_DX);
        }

        // push the pointer's address
        scrip->push_reg(SREG_MAR);
        getJustTheAddressIntoAX = true;
        accessActualPointer = true;
        doMemoryAccessNow = true;

        currentComponentOffset = 0;
        return 0;
    }


    int AccessData_PrepareComponentAccess_JustTheAddressCases(AGS::Symbol variableSym, VariableSymlist * thisClause, bool isLastClause, bool & getJustTheAddressIntoAX, bool & cannotAssign)
    {
        // array without index specified
        if ((sym.entries[variableSym].flags & SFLG_ARRAY) &&
            ((thisClause->len == 1) || (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET)) &&
            ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0))
        {
            if (sym.entries[variableSym].flags & SFLG_ATTRIBUTE)
            {
                // Returning an array attribute as a whole is not supported
                cc_error("Expected array index after '%s'", sym.get_friendly_name(variableSym).c_str());
                return -1;
            }
            getJustTheAddressIntoAX = true;
            cannotAssign = true;
            return 0;
        }

        // struct variable without member access
        if (((sym.entries[variableSym].flags & SFLG_POINTER) == 0) &&
            ((sym.entries[sym.entries[variableSym].vartype].flags & SFLG_STRUCTTYPE) != 0) &&
            ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0) &&
            isLastClause)
        {
            getJustTheAddressIntoAX = true;
            cannotAssign = true;
        }
        return 0;
    }


    // We access the a variable or a component of a struct in order to read or write it. 
    int AccessData_PrepareComponentAccess(ccCompiledScript * scrip, AGS::Symbol variableSym, int variableSymType, bool isLastClause, VariableSymlist * thisClause, bool writing, bool mustBeWritable, bool writingThisTime, AGS::Symbol firstVariableType, AGS::Symbol firstVariableSym, int &currentComponentOffset, bool &getJustTheAddressIntoAX, bool &doMemoryAccessNow, bool &is_attribute, bool &isArrayOffset, bool &write_same_as_read_access, bool &isDynamicArray, bool &pointerIsOnStack, bool &accessActualPointer, bool &cannotAssign)
    {
        write_same_as_read_access = true;
        getJustTheAddressIntoAX = false;
        doMemoryAccessNow = false;
        accessActualPointer = false;
        cannotAssign = false;

        is_attribute = (0 != (sym.entries[variableSym].flags & SFLG_ATTRIBUTE));
        isDynamicArray = (0 != (sym.entries[variableSym].flags & SFLG_DYNAMICARRAY));
        bool isPointer = (0 != (sym.entries[variableSym].flags & (SFLG_POINTER | SFLG_AUTOPTR)));
        bool isImported = (0 != (sym.entries[variableSym].flags & SFLG_IMPORTED));

        // Simple component access - increment the offset from the start of the structure,
        // which is known at compile time
        if (((variableSymType == SYM_GLOBALVAR) ||
            (variableSymType == SYM_LOCALVAR) ||
            (variableSymType == SYM_STRUCTMEMBER) ||
            (variableSymType == SYM_STRING)) &&
            (!is_attribute) &&
            (!isImported))
        {
            AccessData_PrepareComponentAccess_Elementary(variableSym, currentComponentOffset);
        }

        if (variableSymType == SYM_FUNCTION)
        {
            int retval = AccessData_PrepareComponentAccess_MemberFunction(isLastClause, getJustTheAddressIntoAX, doMemoryAccessNow);
            if (retval < 0) return retval;
        }
        else if (is_attribute)
        {
            // Writing an attribute calls a function, reading it calls another function.
            // Avert the caller so that it doesn't try, e.g., to do "++" in-memory.
            // When setting the attribute, they must always go the long way and first
            // evaluate the new value in AX, then set the attribute explicitly.
            write_same_as_read_access = false;

            int retval = AccessData_PrepareComponentAccess_Attribute(scrip, variableSym, thisClause, writing, writingThisTime, mustBeWritable, getJustTheAddressIntoAX, doMemoryAccessNow, isArrayOffset);
            if (retval < 0) return retval;
        }
        else if (isPointer)
        {
            // currentComponentOffset has been set at the start of this loop so it is safe to use
            int retval = AccessData_PrepareComponentAccess_Pointer(scrip, variableSym, thisClause, currentComponentOffset, isDynamicArray, writing, firstVariableType, firstVariableSym, isLastClause, pointerIsOnStack, isArrayOffset, getJustTheAddressIntoAX, currentComponentOffset, accessActualPointer, doMemoryAccessNow);
            if (retval < 0) return retval;

            pointerIsOnStack = true;
        }
        else
        {
            int retval = AccessData_ParseArrayIndexPresent(scrip, thisClause, writing, isArrayOffset);
            if (retval < 0) return retval;
        }

        int retval = AccessData_PrepareComponentAccess_JustTheAddressCases(variableSym, thisClause, isLastClause, getJustTheAddressIntoAX, cannotAssign);
        if (retval < 0) return retval;
        return 0;
    }


    int AccessData_ActualMemoryAccess(ccCompiledScript * scrip, AGS::Symbol variableSym, int variableSymType, bool pointerIsOnStack, bool writing, bool writingThisTime, bool is_attribute, bool mustBeWritable, bool getJustTheAddressIntoAX, bool isArrayOffset, int currentComponentOffset, bool accessActualPointer, AGS::Symbol firstVariableSym, AGS::Symbol firstVariableType, bool isDynamicArray, bool negateLiteral, bool isLastClause, VariableSymlist  variablePath[], size_t vp_idx)
    {
        int cachedAxValType = scrip->ax_val_type;

        // if a pointer in use, then its address was pushed on the
        // stack, so restore it here
        if (pointerIsOnStack)
            scrip->pop_reg(SREG_MAR);

        // in a writing operation, but not doing it just yet -- push
        // AX to save the value to write
        if ((writing) && (!writingThisTime))
            scrip->push_reg(SREG_AX);

        int retval = MemoryAccess(
            scrip, variableSym, variableSymType,
            is_attribute, writingThisTime, mustBeWritable,
            getJustTheAddressIntoAX, isArrayOffset,
            currentComponentOffset, pointerIsOnStack /* means that it's in MAR now*/,
            accessActualPointer,
            firstVariableSym, firstVariableType,
            isDynamicArray, negateLiteral);
        if (retval < 0) return retval;

        pointerIsOnStack = false;
        currentComponentOffset = 0;
        isDynamicArray = false;
        firstVariableType = SYM_GLOBALVAR;

        if (!isLastClause)
        {
            if (!is_attribute && !getJustTheAddressIntoAX)
            {
                cc_error("Unexpected '%s' in variable path", sym.get_friendly_name(variablePath[vp_idx + 1].syml[0]).c_str());
                return -1;
            }

            // pathing, eg. lstItems.OwningGUI.ID
            // we just read a pointer address, so re-push it for use
            // next time round
            if (writing)
            {
                scrip->ax_val_type = cachedAxValType;
                // the value to write was pushed onto the stack,
                // pop it back into BX
                scrip->pop_reg(SREG_BX);
                // meanwhile push the pointer
                // that was just read into AX onto the stack in its place
                scrip->push_reg(SREG_AX);
                // and then copy the value back into AX
                scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
            }
            else
            {
                scrip->push_reg(SREG_AX);
            }
        }
        return 0;
    }


    int AccessData_CheckAccess(AGS::Symbol variableSym, VariableSymlist variablePath[], bool writing, bool mustBeWritable, bool write_same_as_read_access, bool isLastClause, size_t vp_idx, bool cannotAssign)
    {
        // if one of the struct members in the path is read-only, don't allow it
        if (((writing) || (mustBeWritable)) && (write_same_as_read_access))
        {
            // allow writing to read-only pointers if it's actually
            // an attribute being accessed
            if ((sym.entries[variableSym].flags & SFLG_POINTER) && (!isLastClause)) {}
            else if (sym.entries[variableSym].flags & SFLG_READONLY)
            {
                cc_error("Variable '%s' is read-only", sym.get_friendly_name(variableSym).c_str());
                return -1;
            }
            else if (sym.entries[variableSym].flags & SFLG_WRITEPROTECTED)
            {
                // write-protected variables can only be written by
                // the this ptr
                if ((vp_idx > 0) && (sym.entries[variablePath[vp_idx - 1].syml[0]].flags & SFLG_THISPTR)) {}
                else
                {
                    cc_error("Variable '%s' is write-protected", sym.get_friendly_name(variableSym).c_str());
                    return -1;
                }
            }
        }

        if ((writing) && (cannotAssign))
        {
            // an entire array or struct cannot be assigned to
            cc_error("Cannot assign to '%s'", sym.get_friendly_name(variableSym).c_str());
            return -1;
        }

        return 0;
    }


    // read the various types of values into AX
    int AccessData(ccCompiledScript*scrip, AGS::SymbolScript syml, int syml_len, bool writing, bool mustBeWritable, bool negateLiteral, bool &write_same_as_read_access)
    {
        // If this is a reading access, then the scope of AX will be the scope of the thing read
        if (!writing)
            SetAXScope(scrip, syml[0]);

        // separate out the variable path, into a variablePath
        // for the bit between each dot
        VariableSymlist variablePath[MAX_VARIABLE_PATH];
        size_t variablePathSize;

        int retval = AccessData_SplitPathIntoParts(variablePath, syml, syml_len, variablePathSize);
        if (retval < 0) return retval;
        if (variablePathSize < 1)
            return 0;

        // start of the component that is looked up
        // given as an offset from the beginning of the overall structure
        int currentComponentOffset = 0;

        // Symbol and type of the first variable in the list 
        // (since that determines whether this is global/local)
        AGS::Symbol firstVariableSym = variablePath[0].syml[0];
        AGS::Symbol firstVariableType = sym.get_type(firstVariableSym);
        bool isArrayOffset = false;
        bool isDynamicArray = false;

        bool pointerIsOnStack = false;

        for (size_t vp_idx = 0; vp_idx < variablePathSize; vp_idx++)
        {
            VariableSymlist *thisClause = &variablePath[vp_idx];
            bool isLastClause = (vp_idx == variablePathSize - 1);

            AGS::Symbol variableSym = thisClause->syml[0];
            int variableSymType = sym.get_type(variableSym);

            bool getJustTheAddressIntoAX = false;
            bool doMemoryAccessNow = false;

            bool cannotAssign = false;
            bool is_attribute = false;
            bool accessActualPointer = false;

            // the memory access only wants to write if this is the
            // end of the path, not an intermediate pathing attribute
            bool writingThisTime = isLastClause && writing;

            // Mark the component as accessed
            sym.entries[variableSym].flags |= SFLG_ACCESSED;

            int retval = AccessData_PrepareComponentAccess(scrip, variableSym, variableSymType, isLastClause, thisClause, writing, mustBeWritable, writingThisTime, firstVariableType, firstVariableSym, currentComponentOffset, getJustTheAddressIntoAX, doMemoryAccessNow, is_attribute, isArrayOffset, write_same_as_read_access, isDynamicArray, pointerIsOnStack, accessActualPointer, cannotAssign);
            if (retval < 0) return retval;

            retval = AccessData_CheckAccess(variableSym, variablePath, writing, mustBeWritable, write_same_as_read_access, isLastClause, vp_idx, cannotAssign);
            if (retval < 0) return retval;

            if (!doMemoryAccessNow && !isLastClause)
                continue;

            retval = AccessData_ActualMemoryAccess(scrip, variableSym, variableSymType, pointerIsOnStack, writing, writingThisTime, is_attribute, mustBeWritable, getJustTheAddressIntoAX, isArrayOffset, currentComponentOffset, accessActualPointer, firstVariableSym, firstVariableType, isDynamicArray, negateLiteral, isLastClause, variablePath, vp_idx);
            if (retval < 0) return retval;


        }

        // free the VariablePaths
        for (size_t vp_idx = 0; vp_idx < variablePathSize; vp_idx++)
            variablePath[vp_idx].destroy();

        return 0;
    }


    int ReadDataIntoAX(ccCompiledScript*scrip, AGS::SymbolScript syml, int syml_len, bool negateLiteral)
    {
        bool dummy; // ignored parameter
        return AccessData(scrip, syml, syml_len, false, false, negateLiteral, dummy);
    }

    int ReadDataIntoAX(ccCompiledScript*scrip, AGS::SymbolScript syml, int syml_len, bool negateLiteral, bool mustBeWritable, bool &write_same_as_read_access)
    {
        return AccessData(scrip, syml, syml_len, false, mustBeWritable, negateLiteral, write_same_as_read_access);
    }

    // Get or set an attribute
    int CallAttributeFunc(ccCompiledScript *scrip, AGS::Symbol attrib_sym, int isWrite)
    {
        int numargs = 0;

        // AX contains the struct address

        // Always a struct member -- set OP = AX
        if ((sym.entries[attrib_sym].flags & SFLG_STATIC) == 0)
        {
            scrip->push_reg(SREG_OP);
            scrip->write_cmd1(SCMD_CALLOBJ, SREG_AX);
        }

        if (isWrite)
        {
            if (0 == (sym.entries[attrib_sym].flags & SFLG_IMPORTED))
            {
                cc_error("Internal error: Attribute is not import");
                return -1;
            }

            // BX contains the new value
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_BX);
            numargs++;
        }

        if (sym.entries[attrib_sym].flags & SFLG_ARRAY)
        {
            if (0 == (sym.entries[attrib_sym].flags & SFLG_IMPORTED))
            {
                cc_error("Internal error: Attribute is not import");
                return -1;
            }

            // array indexer is in DX
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_DX);
            numargs++;
        }

        if (sym.entries[attrib_sym].flags & SFLG_IMPORTED)
        {
            // tell it how many args for this call (nested imported functions
            // causes stack problems otherwise)
            scrip->write_cmd1(SCMD_NUMFUNCARGS, numargs);
        }

        int attribute_func;
        if (isWrite)
            attribute_func = sym.entries[attrib_sym].get_attrset();
        else
            attribute_func = sym.entries[attrib_sym].get_attrget();

        if (attribute_func == 0)
        {
            cc_error("Internal error: Attribute is in use but not set");
            return -1;
        }

        // AX = Func Address
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, attribute_func);

        if (sym.entries[attrib_sym].flags & SFLG_IMPORTED)
        {
            scrip->fixup_previous(FIXUP_IMPORT);
            // do the call
            scrip->write_cmd1(SCMD_CALLEXT, SREG_AX);
            if (numargs > 0)
                scrip->write_cmd1(SCMD_SUBREALSTACK, numargs);
        }
        else
        {
            scrip->fixup_previous(FIXUP_FUNCTION);
            scrip->write_cmd1(SCMD_CALL, SREG_AX);

            // restore the stack
            if (numargs > 0)
            {
                scrip->cur_sp -= numargs * 4;
                scrip->write_cmd2(SCMD_SUB, SREG_SP, numargs * 4);
            }
        }

        if (!isWrite)
        {
            // function return type
            scrip->ax_val_type = sym.entries[attrib_sym].vartype;
            scrip->ax_val_scope = SYM_LOCALVAR;
            if (sym.entries[attrib_sym].flags & SFLG_DYNAMICARRAY)
                scrip->ax_val_type |= STYPE_DYNARRAY;
            if (sym.entries[attrib_sym].flags & SFLG_POINTER)
                scrip->ax_val_type |= STYPE_POINTER;
            if (sym.entries[attrib_sym].flags & SFLG_CONST)
                scrip->ax_val_type |= STYPE_CONST;
        }

        if ((sym.entries[attrib_sym].flags & SFLG_STATIC) == 0)
            scrip->pop_reg(SREG_OP);

        return 0;
    }


    int MemoryAccess_Vartype(ccCompiledScript * scrip, AGS::Symbol variableSym, bool is_attribute, int &gotValType)
    {
        // it's a static member attribute
        if (!is_attribute)
        {
            cc_error("Internal error: Static non-attribute access");
            return -1;
        }
        // just write 0 to AX for ease of debugging if anything
        // goes wrong
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

        gotValType = sym.entries[variableSym].vartype;
        if (sym.entries[variableSym].flags & SFLG_CONST)
            gotValType |= STYPE_CONST;

        return 0;
    }


    int MemoryAccess_LitOrConst(ccCompiledScript * scrip, int mainVariableType, AGS::Symbol variableSym, bool writing, bool mustBeWritable, bool negateLiteral, int &gotValType)
    {
        if ((writing) || (mustBeWritable))
        {
            if (mainVariableType == SYM_LITERALVALUE)
                cc_error("Cannot write to a literal value");
            else
                cc_error("Cannot write to a constant");

            return -1;
        }

        int varSymValue;
        int retval = ParseLiteralOrConstvalue(variableSym, varSymValue, negateLiteral, "Error parsing integer value");
        if (retval < 0) return retval;

        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, varSymValue);
        gotValType = sym.normalIntSym;

        return 0;
    }


    int MemoryAccess_LitFloat(ccCompiledScript * scrip, AGS::Symbol variableSym, bool writing, bool mustBeWritable, int &gotValType)
    {
        if ((writing) || (mustBeWritable))
        {
            cc_error("Cannot write to a literal value");
            return -1;
        }
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, InterpretFloatAsInt((float)atof(sym.get_name(variableSym))));
        gotValType = sym.normalFloatSym;
        return 0;
    }


    // a "normal" variable or a pointer
    int MemoryAccess_Variable(ccCompiledScript * scrip, AGS::Symbol mainVariableSym, int mainVariableType, AGS::Symbol variableSym, bool pointerIsInMAR, bool &wholePointerAccess, bool addressof, int soffset, bool isArrayOffset, bool isDynamicArray, bool writing, int &gotValType)
    {
        int readwritecmd = GetReadWriteCmdForSize(sym.entries[variableSym].ssize, writing);

        gotValType = sym.entries[variableSym].vartype;
        if (sym.entries[variableSym].flags & SFLG_CONST)
            gotValType |= STYPE_CONST;

        if (pointerIsInMAR)
        {
            // the address is already in MAR by the caller
            if ((!wholePointerAccess) && ((!addressof) || (soffset) || (isArrayOffset)))
                scrip->write_cmd(SCMD_CHECKNULL);
            if (soffset != 0)
                scrip->write_cmd2(SCMD_ADD, SREG_MAR, soffset);
        }
        else if (mainVariableType == SYM_LOCALVAR)
        {
            // a local one
            scrip->write_cmd1(SCMD_LOADSPOFFS, scrip->cur_sp - soffset);
        }
        else // global variable
        {

            if (sym.entries[mainVariableSym].flags & SFLG_IMPORTED)
            {
                // imported variable, so get the import address and then add any offset
                scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, sym.entries[mainVariableSym].soffs);
                scrip->fixup_previous(FIXUP_IMPORT);
                if (soffset != 0)
                    scrip->write_cmd2(SCMD_ADD, SREG_MAR, soffset);
            }
            else
            {
                scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, soffset);
                scrip->fixup_previous(FIXUP_GLOBALDATA);
            }
        }

        if (isArrayOffset)
        {
            if (isDynamicArray)
            {
                scrip->write_cmd1(SCMD_MEMREADPTR, SREG_MAR);
                scrip->write_cmd(SCMD_CHECKNULL);
                scrip->write_cmd1(SCMD_DYNAMICBOUNDS, SREG_CX);
            }

            scrip->write_cmd2(SCMD_ADDREG, SREG_MAR, SREG_CX);
        }
        else if (isDynamicArray)
        {
            // not accessing an element of it, must be whole thing
            wholePointerAccess = true;
            gotValType |= STYPE_DYNARRAY;
        }

        if (wholePointerAccess)
        {
            scrip->write_cmd1((writing ? SCMD_MEMWRITEPTR : SCMD_MEMREADPTR), SREG_AX);
        }
        else if (addressof)
        {
            scrip->write_cmd2(SCMD_REGTOREG, SREG_MAR, SREG_AX);
        }
        else
        {
            scrip->write_cmd1(readwritecmd, SREG_AX);
        }
        return 0;
    }


    int MemoryAccess_String(ccCompiledScript * scrip, bool writing, int soffset, int &gotValType)
    {
        if (writing)
        {
            cc_error("Cannot write to a literal string");
            return -1;
        }

        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, soffset);
        scrip->fixup_previous(FIXUP_STRING);
        gotValType = sym.normalStringSym | STYPE_CONST;

        return 0;
    }


    int MemoryAccess_StructMember(AGS::Symbol mainVariableSym)
    {
        cc_error("Must include parent structure of member '%s'", sym.get_friendly_name(mainVariableSym).c_str());
        return -1;
    }


    int MemoryAccess_Null(ccCompiledScript * scrip, bool writing, int &gotValType)
    {
        if (writing)
        {
            cc_error("Invalid use of null");
            return -1;
        }
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
        gotValType = sym.nullSym | STYPE_POINTER;

        return 0;
    }


    int MemoryAccess_ActualAccess(ccCompiledScript * scrip, AGS::Symbol mainVariableSym, int mainVariableType, AGS::Symbol variableSym, bool writing, bool mustBeWritable, bool negateLiteral, bool pointerIsInMAR, bool addressof, int soffset, bool isArrayOffset, bool isDynamicArray, bool is_attribute, bool &wholePointerAccess, int &gotValType)
    {
        switch (mainVariableType)
        {
        default:
            break;

        case SYM_CONSTANT:
            return MemoryAccess_LitOrConst(scrip, mainVariableType, variableSym, writing, mustBeWritable, negateLiteral, gotValType);

        case SYM_GLOBALVAR:
            return MemoryAccess_Variable(scrip, mainVariableSym, mainVariableType, variableSym, pointerIsInMAR, wholePointerAccess, addressof, soffset, isArrayOffset, isDynamicArray, writing, gotValType);

        case SYM_LITERALFLOAT:
            return MemoryAccess_LitFloat(scrip, variableSym, writing, mustBeWritable, gotValType);

        case SYM_LITERALVALUE:
            return MemoryAccess_LitOrConst(scrip, mainVariableType, variableSym, writing, mustBeWritable, negateLiteral, gotValType);

        case SYM_LOCALVAR:
            return MemoryAccess_Variable(scrip, mainVariableSym, mainVariableType, variableSym, pointerIsInMAR, wholePointerAccess, addressof, soffset, isArrayOffset, isDynamicArray, writing, gotValType);

        case SYM_NULL:
            return MemoryAccess_Null(scrip, writing, gotValType);

        case SYM_STRING:
            return MemoryAccess_String(scrip, writing, soffset, gotValType);

        case SYM_STRUCTMEMBER:
            return MemoryAccess_StructMember(mainVariableSym);

        case SYM_VARTYPE:
            return MemoryAccess_Vartype(scrip, variableSym, is_attribute, gotValType);
        }

        // Can't reach this
        cc_error("Internal error: Read/write ax called with non-variable parameter '%s'", sym.get_friendly_name(variableSym).c_str());
        return -99;
    }


    int MemoryAccess(
        ccCompiledScript *scrip, AGS::Symbol variableSym,
        int variableSymType, bool is_attribute,
        bool writing, bool mustBeWritable,
        bool addressof, bool isArrayOffset,
        int soffset, bool pointerIsInMAR,
        bool wholePointerAccess,
        AGS::Symbol mainVariableSym, int mainVariableType,
        bool isDynamicArray, bool negateLiteral)
    {
        int gotValType;
        int retval = MemoryAccess_ActualAccess(scrip, mainVariableSym, mainVariableType, variableSym, writing, mustBeWritable, negateLiteral, pointerIsInMAR, addressof, soffset, isArrayOffset, isDynamicArray, is_attribute, wholePointerAccess, gotValType);
        if (retval < 0) return retval;

        if ((!is_attribute && addressof) ||
            (is_attribute && ((sym.entries[variableSym].flags & SFLG_POINTER) != 0)))
        {
            gotValType |= STYPE_POINTER;
        }

        if (writing)
        {
            retval = IsTypeMismatch(scrip->ax_val_type, gotValType, true);
            if (retval < 0) return retval;
        }

        // Must be set both when reading OR writing
        scrip->ax_val_type = gotValType;

        if (is_attribute)
        {
            // ParseArrays_and_members will have set addressOf to true,
            // so AX now contains the struct address, and BX
            // contains the new value if this is a Set
            retval = CallAttributeFunc(scrip, variableSym, writing);
            if (retval < 0) return retval;
        }

        return 0;
    }


    int WriteAXToData(ccCompiledScript*scrip, AGS::SymbolScript syml, int syml_len)
    {
        bool dummy; // ignored parameter
        return AccessData(scrip, syml, syml_len, true, false, false, dummy);
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


    // evaluate the supplied expression, putting the result into AX
    // returns 0 on success or -1 if compile error
    // leaves targ pointing to last token in expression, so do getnext() to get the following ; or whatever
    int ParseExpression(ccInternalList *targ, ccCompiledScript*scrip, bool consider_paren_nesting)
    {
        ccInternalList expr_script;
        size_t script_idx = 0;
        size_t paren_nesting_depth = 0;
        bool hadMetaOnly = true;

        // "Peek" into the symbols and find the first that is NOT part of the expression
        // [fw] This code can be rewritten with peeknext() -- this would skip METAs automatically
        for (script_idx = targ->pos; (int)script_idx < targ->length; script_idx++)
        {
            // Skip meta commands
            if (targ->script[script_idx] == SCODE_META)
            {
                script_idx += 2;
                continue;
            }

            // parenthesis depth counting
            if (sym.get_type(targ->script[script_idx]) == SYM_OPENPARENTHESIS)
            {
                paren_nesting_depth++;
            }
            else if (sym.get_type(targ->script[script_idx]) == SYM_CLOSEPARENTHESIS && paren_nesting_depth > 0)
            {
                paren_nesting_depth--;
                continue; // This means that the outermost ')' will be part of the expression
            }

            if (((paren_nesting_depth == 0) && !CanBePartOfExpression(targ, script_idx)) ||   // The parens are all closed and the expression can't continue
                ((paren_nesting_depth == 0) && consider_paren_nesting) || // the last paren has JUST been closed and this is the deciding factor
                sym.get_type(targ->script[script_idx]) == SYM_CLOSEPARENTHESIS) // all parens had been closed beforehand and there is another ')' pending
            {
                // Here, script_idx is the first symbol that is NOT part of the expression
                if ((script_idx == targ->pos) || hadMetaOnly)
                {
                    cc_error("Expression expected and not found at '%s'", sym.get_friendly_name(targ->script[script_idx]).c_str());
                    return -1;
                }

                // Copy the expression into expr_script (in order to skip the METAs)
                int retval = BufferExpression(targ, script_idx, &expr_script);
                if (retval < 0) return retval;
                break;
            }

            // found a real token, not just metadata
            hadMetaOnly = false;
        }

        if ((int)script_idx >= targ->length)
        {
            cc_error("End of input reached in middle of expression");
            return -1;
        }

        // move the cursor of targ to the symbol after the expression, so that getnext will find it.
        targ->pos = script_idx;

        // we now have the expression in expr_script, parse it
        return ParseSubexpr(scrip, expr_script.script, expr_script.length);
    }


    // We're in an assignment, cursym points to the LHS. Check that the LHS is assignable.
    int ParseAssignment_CheckLHSIsAssignable(AGS::Symbol cursym, const AGS::SymbolScript &vnlist, int vnlist_len)
    {
        if (sym.entries[cursym].is_loadable_variable())
            return 0;

        // Static attribute
        if ((sym.get_type(cursym) == SYM_VARTYPE) &&
            (vnlist_len > 2) &&
            (sym.entries[vnlist[2]].flags & SFLG_STATIC) > 0)
        {
            return 0;
        }

        cc_error("Variable or constant attribute required on left of \"%s\" assignment", sym.get_name(cursym));
        return -1;
    }


    // We are processing an assignment. vn_list[] contains a variable or a struct selector. 
    // If it is a (static or dynamic) array, then check whether the assignment is allowed.
    int ParseAssignment_ArrayChecks(AGS::Symbol cursym, AGS::Symbol nextsym, size_t vnlist_len)
    {
        // [fw] This may not be good enough. What if the array is in a struct? 
        //      Then the checks won't run.
        if ((sym.entries[cursym].flags & SFLG_DYNAMICARRAY) != 0)
        {
            // Can only assign to entire dynamic arrays, e.g., allocate the memory
            if ((vnlist_len < 2) && (sym.get_type(nextsym) != SYM_ASSIGN))
            {
                cc_error("Cannot use operator \"%s\" with an entire dynamic array", sym.get_name(nextsym));
                return -1;
            }
            return 0;
        }

        if ((sym.entries[cursym].flags & SFLG_ARRAY) != 0)
        {
            if (vnlist_len < 2)
            {
                cc_error("Cannot assign a value to an entire static array");
                return -1;
            }
        }
        return 0;
    }


    // We compile something like "a += b"
    int ParseAssignment_MAssign(ccCompiledScript * scrip, AGS::Symbol ass_symbol, const AGS::SymbolScript & vnlist, int vnlist_len)
    {
        // Read in and adjust the result
        scrip->push_reg(SREG_AX);
        int varTypeRHS = scrip->ax_val_type;

        int retval = ReadDataIntoAX(scrip, vnlist, vnlist_len, false);
        if (retval < 0) return retval;

        retval = IsTypeMismatch(varTypeRHS, scrip->ax_val_type, true);
        if (retval < 0) return retval;

        int cpuOp = sym.entries[ass_symbol].ssize;
        if (GetOperatorValidForType(varTypeRHS, scrip->ax_val_type, cpuOp))
            return -1;

        scrip->pop_reg(SREG_BX);
        scrip->write_cmd2(cpuOp, SREG_AX, SREG_BX);

        retval = WriteAXToData(scrip, &vnlist[0], vnlist_len);
        if (retval < 0) return retval;
        return 0;
    }


    int ParseAssignment_Assign(ccCompiledScript * scrip, int vnlist_len, const AGS::SymbolScript & vnlist)
    {
        // Convert normal literal string into String object
        size_t finalPartOfLHS = vnlist_len - 1;
        if (sym.get_type(vnlist[vnlist_len - 1]) == SYM_CLOSEBRACKET)
        {
            // deal with  a[1] = b
            FindOpeningBracketOffs(vnlist_len - 1, vnlist, finalPartOfLHS);
            if (--finalPartOfLHS < 0)
            {
                cc_error("No matching '[' for ']'");
                return -1;
            }
        }
        // If we need a string object ptr but AX contains a normal string, convert AX
        ConvertAXIntoStringObject(scrip, sym.entries[vnlist[finalPartOfLHS]].vartype);

        int retval = WriteAXToData(scrip, &vnlist[0], vnlist_len);
        if (retval < 0) return retval;

        return 0;
    }


    int ParseAssignment_SAssign(ccCompiledScript * scrip, AGS::Symbol ass_symbol, const AGS::SymbolScript & vnlist, int vnlist_len)
    {
        bool write_same_as_read_access;
        int retval = ReadDataIntoAX(scrip, &vnlist[0], vnlist_len, false, true, write_same_as_read_access);
        if (retval < 0) return retval;

        // Get the bytecode operator that corresponds to the assignment symbol and type
        int cpuOp = sym.entries[ass_symbol].ssize;
        retval = GetOperatorValidForType(scrip->ax_val_type, 0, cpuOp);
        if (retval < 0) return retval;

        scrip->write_cmd2(cpuOp, SREG_AX, 1);

        if (write_same_as_read_access)
        {
            // since the MAR won't have changed, we can directly write
            // the value back to it without re-calculating the offset
            // [fw] Passing info around through a global variable: That is a HUGE code smell.
            scrip->write_cmd1(GetReadWriteCmdForSize(SizeUsedInLastReadCommand, true), SREG_AX);
            return 0;
        }

        // copy the result (currently in AX) into the variable
        retval = WriteAXToData(scrip, &vnlist[0], vnlist_len);
        if (retval < 0) return retval;

        return 0;
    }


    int ParseAssignment_DoAssignment(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol ass_symbol, const AGS::SymbolScript &vnlist, int vnlist_len)
    {
        switch (sym.get_type(ass_symbol))
        {
        default: // can't happen
        {
            cc_error("Internal error: Illegal assignment symbol found");
            return -99;
        }

        case SYM_ASSIGN:
        {
            // Get RHS
            int retval = ParseExpression(targ, scrip, false);
            if (retval < 0) return retval;

            // Do assignment
            return ParseAssignment_Assign(scrip, vnlist_len, vnlist);
        }

        case SYM_MASSIGN:
        {
            // Get RHS
            int retval = ParseExpression(targ, scrip, false);
            if (retval < 0) return retval;

            // Do assignment
            return ParseAssignment_MAssign(scrip, ass_symbol, vnlist, vnlist_len);
        }

        case SYM_SASSIGN:
        {
            // "++" or "--". There isn't any RHS to read in. Do assignment.
            return ParseAssignment_SAssign(scrip, ass_symbol, vnlist, vnlist_len);
        }
        }

        return 0;
    }


    // We've read a variable or selector of a struct into vn_list[], the last identifying component is in cursym.
    // An assignment symbol is following. Compile the assignment.
    int ParseAssignment(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol cursym, AGS::Symbol statementEndSymbol, AGS::SymbolScript vnlist, int vnlist_len)
    {
        // Check that the LHS is a loadable variable or a static attribute. 
        int retval = ParseAssignment_CheckLHSIsAssignable(cursym, vnlist, vnlist_len);
        if (retval < 0) return retval;

        // Checks to do for the LHS if it is a (dynamic or static) array
        retval = ParseAssignment_ArrayChecks(cursym, targ->peeknext(), vnlist_len);
        if (retval < 0) return retval;

        if (sym.entries[cursym].flags & SFLG_ISSTRING)
        {
            cc_error("cannot assign to string; Use Str* functions instead");
            return -1;
        }

        // Do the assignment
        retval = ParseAssignment_DoAssignment(targ, scrip, targ->getnext(), vnlist, vnlist_len);
        if (retval < 0) return retval;

        // Gobble the statement end symbol (usually ';', can be ')')
        if (sym.get_type(targ->getnext()) != statementEndSymbol)
        {
            cc_error("Expected '%s'", sym.get_name(statementEndSymbol));
            return -1;
        }

        return 0;
    }


    // true if the symbol is "int" and the like.
    inline bool sym_is_predef_typename(AGS::Symbol symbl)
    {
        return (symbl >= 0 && symbl <= sym.normalFloatSym);
    }


    int ParseVardecl_InitialValAssignment_ToLocal(ccInternalList *targ, ccCompiledScript * scrip, int completeVarType)
    {
        // Parse and compile the expression
        int retval = ParseExpression(targ, scrip, false);
        if (retval < 0) return retval;

        // If we need a string object ptr but AX contains a normal string, convert AX
        ConvertAXIntoStringObject(scrip, completeVarType);

        // Check whether the types match
        retval = IsTypeMismatch(scrip->ax_val_type, completeVarType, true);
        if (retval < 0) return retval;
        return 0;
    }


    int ParseVardecl_InitialValAssignment_ToGlobalFloat(ccInternalList * targ, bool is_neg, void *& initial_val_ptr)
    {
        // initialize float
        if (sym.get_type(targ->peeknext()) != SYM_LITERALFLOAT)
        {
            cc_error("Expected floating point value after '='");
            return -1;
        }

        float float_init_val = static_cast<float>(atof(sym.get_name(targ->getnext())));
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


    int ParseVardecl_InitialValAssignment_ToGlobalNonFloat(ccInternalList * targ, bool is_neg, void *& initial_val_ptr)
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
    int ParseVardecl_InitialValAssignment_ToGlobal(ccInternalList *targ, long varname, void * &initial_val_ptr)
    {
        initial_val_ptr = nullptr;

        if ((sym.entries[varname].flags & SFLG_POINTER) != 0)
        {
            cc_error("Cannot assign an initial value to a global pointer");
            return -1;
        }

        if ((sym.entries[varname].flags & SFLG_DYNAMICARRAY) != 0)
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
        if (sym.get_name(targ->peeknext())[0] == '-')
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
    int ParseVardecl_InitialValAssignment(ccInternalList *targ, ccCompiledScript * scrip, int next_type, Globalness isglobal, long varname, int type_of_defn, void * &initial_val_ptr, FxFixupType &need_fixup)
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
        if (sym.entries[varname].flags & SFLG_ISSTRING)
        {
            cc_error("Cannot assign a value to a string, use StrCopy");
            return -1;
        }

        int completeVarType = type_of_defn;
        if (sym.entries[varname].flags & SFLG_POINTER)
            completeVarType |= STYPE_POINTER;
        if (sym.entries[varname].flags & SFLG_DYNAMICARRAY)
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
            sym.entries[var_name].flags |= SFLG_POINTER;
    }


    // we have accepted something like "int a" and we're expecting "["
    int ParseVardecl_ArrayDecl(ccInternalList *targ, int var_name, int type_of_defn, int &array_size, int &size_of_defn)
    {
        // an array
        targ->getnext();  // skip the [

        if (sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
        {
            sym.entries[var_name].flags |= SFLG_DYNAMICARRAY;
            array_size = 0;
            size_of_defn = 4;
        }
        else
        {
            if (sym.entries[type_of_defn].flags & SFLG_HASDYNAMICARRAY)
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
        sym.entries[var_name].flags |= SFLG_ARRAY;
        sym.entries[var_name].arrsize = array_size;

        if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
        {
            cc_error("Expected ']'");
            return -1;
        }

        return 0;
    }


    int ParseVardecl_StringDecl_GlobalNoImport(ccCompiledScript * scrip, void *&initial_value_ptr, FxFixupType &fixup_needed)
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


    int ParseVardecl_StringDecl_Local(ccCompiledScript * scrip, AGS::Symbol var_name, void * &initial_value_ptr)
    {
        // Note: We can't use scrip->cur_sp since we don't know if we'll be in a nested function call at the time
        initial_value_ptr = nullptr;

        sym.entries[var_name].flags |= SFLG_STRBUFFER; // Note in the symbol table that this var is a stringbuffer
        scrip->cur_sp += STRING_LENGTH; // reserve STRING_LENGTH bytes for the var on the stack

                                        // CX will contain the address of the new memory, which will be added to the stack
        scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_CX); // Copy current stack pointer to CX
        scrip->write_cmd2(SCMD_ADD, SREG_SP, STRING_LENGTH); // write code for reserving STRING LENGTH bytes 
        return 0;
    }


    int ParseVardecl_StringDecl(ccCompiledScript * scrip, AGS::Symbol var_name, Globalness is_global, void * &initial_value_ptr, FxFixupType &fixup_needed)
    {
        if (ccGetOption(SCOPT_OLDSTRINGS) == 0)
        {
            cc_error("Type 'string' is no longer supported; use String instead");
            return -1;
        }

        if (sym.entries[var_name].flags & SFLG_DYNAMICARRAY)
        {
            cc_error("Arrays of old-style strings are not supported");
            return -1;
        }

        if (is_global == kGl_GlobalImport)
        {
            // cannot import, because string is really char*, and the pointer won't resolve properly
            cc_error("Cannot import string; use char[] instead");
            return -1;
        }

        initial_value_ptr = nullptr;
        fixup_needed = kFx_NoFixup;

        sym.entries[var_name].flags |= SFLG_ISSTRING;

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
    void ParseVardecl_CodeForDefnOfLocal(ccCompiledScript * scrip, int var_name, FxFixupType fixup_needed, int size_of_defn, void * initial_value)
    {
        scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR); // MAR = SP

        // code for the initial assignment or the initialization to zeros
        if (fixup_needed == kFx_FixupFunction)
        {
            // expression worked out into ax
            if ((sym.entries[var_name].flags & (SFLG_POINTER | SFLG_DYNAMICARRAY)) != 0)
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
            sym.entries[var_name].flags |= SFLG_STRBUFFER;
            scrip->fixup_previous(FIXUP_STACK);
        }

        // Allocate space on the stack
        if (size_of_defn > 0)
        {
            scrip->cur_sp += size_of_defn;
            scrip->write_cmd2(SCMD_ADD, SREG_SP, size_of_defn);
        }
    }


    int ParseVardecl_CheckIllegalCombis(int var_name, int type_of_defn, bool is_pointer, Globalness is_global)
    {
        if (sym.get_type(var_name) != 0)
        {
            cc_error("Symbol '%s' is already defined");
            return -1;
        }

        if ((sym.entries[type_of_defn].flags & SFLG_MANAGED) && (!is_pointer) && (is_global != kGl_GlobalImport))
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

        if (((sym.entries[type_of_defn].flags & SFLG_MANAGED) == 0) && (is_pointer) && (is_global != kGl_GlobalImport))
        {
            // can only point to managed structs
            cc_error("Cannot declare pointer to non-managed type");
            return -1;
        }

        return 0;
    }


    int ParseVardecl0(
        ccInternalList *targ,
        ccCompiledScript * scrip,
        int var_name,
        int type_of_defn,  // i.e. "void" or "int"
        int next_type, // type of the following symbol
        Globalness is_global,
        bool is_pointer,
        bool &another_var_follows,
        void *initial_value_ptr)
    {
        int retval = ParseVardecl_CheckIllegalCombis(var_name, type_of_defn, is_pointer, is_global);
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

        // Default assignment
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
        else if (strcmp(sym.get_name(type_of_defn), "string") == 0)
        {
            int retval = ParseVardecl_StringDecl(scrip, var_name, is_global, initial_value_ptr, fixup_needed);
            if (retval < 0) return retval;
        }
        else
        {
            // a kind of atomic value (char, int, long, float, pointer) -- initialize to 0
            initial_value_ptr = calloc(1, sizeof(long));
        }

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
            sym.entries[var_name].soffs = scrip->add_new_import(sym.get_name(var_name));
            sym.entries[var_name].flags |= SFLG_IMPORTED;
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

        cc_error("Expected ',' or ';' instead of '%s'", sym.get_friendly_name(targ->peeknext()).c_str());
        return -1;
    }

    // wrapper around ParseVardecl0() to prevent memory leakage
    inline int ParseVardecl(
        ccInternalList *targ,
        ccCompiledScript * scrip,
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


    void ParseOpenbrace_FuncBody(ccCompiledScript * scrip, AGS::Symbol name_of_func, int struct_of_func, bool is_noloopcheck, AGS::NestingStack * nesting_stack)
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
        if ((struct_of_func) && (0 == (sym.entries[name_of_func].flags & SFLG_STATIC)))
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
        ccCompiledScript * scrip,
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

    void ParseStruct_SetTypeInSymboltable(SymbolTableEntry &entry, bool struct_is_managed, bool struct_is_builtin, bool struct_is_autoptr)
    {
        entry.extends = 0;
        entry.stype = SYM_VARTYPE;
        entry.flags |= SFLG_STRUCTTYPE;
        entry.ssize = 0;

        if (struct_is_managed)
            entry.flags |= SFLG_MANAGED;

        if (struct_is_builtin)
            entry.flags |= SFLG_BUILTIN;

        if (struct_is_autoptr)
            entry.flags |= SFLG_AUTOPTR;
    }


    // We have accepted something like "struct foo" and are waiting for "extends"
    int ParseStruct_ExtendsClause(ccInternalList *targ, int stname, AGS::Symbol &extendsWhat, int &size_so_far)
    {
        targ->getnext(); // gobble "extends"
        extendsWhat = targ->getnext(); // name of the extended struct
        if (sym.get_type(extendsWhat) != SYM_VARTYPE)
        {
            cc_error("Expected a struct type here");
            return -1;
        }
        SymbolTableEntry & struct_entry = sym.entries[stname];
        SymbolTableEntry & extends_entry = sym.entries[extendsWhat];

        if ((extends_entry.flags & SFLG_STRUCTTYPE) == 0)
        {
            cc_error("Must extend a struct type");
            return -1;
        }
        if ((extends_entry.flags & SFLG_MANAGED) == 0 && (struct_entry.flags & SFLG_MANAGED))
        {
            cc_error("Managed struct cannot extend the unmanaged struct '%s'", sym.get_name(extendsWhat));
            return -1;
        }
        if ((extends_entry.flags & SFLG_MANAGED) && (struct_entry.flags & SFLG_MANAGED) == 0)
        {
            cc_error("Unmanaged struct cannot extend the managed struct '%s'", sym.get_name(extendsWhat));
            return -1;
        }
        if ((extends_entry.flags & SFLG_BUILTIN) && (struct_entry.flags & SFLG_BUILTIN) == 0)
        {
            cc_error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", sym.get_name(extendsWhat));
            return -1;
        }
        size_so_far = extends_entry.ssize;
        struct_entry.extends = extendsWhat;

        return 0;
    }


    int ParseStruct_MemberQualifiers(
        ccInternalList *targ,
        AGS::Symbol &cursym,
        bool &is_readonly,
        Importness &is_import,
        bool &is_attribute,
        bool &is_static,
        bool &is_protected,
        bool &is_writeprotected)
    {
        // [fw] Check that each qualifier is used exactly once?
        while (true)
        {
            cursym = targ->getnext();

            switch (sym.get_type(cursym))
            {
            default: break;
            case SYM_IMPORT:         is_import = kIm_ImportType1; continue;
            case SYM_ATTRIBUTE:      is_attribute = true;       continue;
            case SYM_PROTECTED:      is_protected = true;      continue;
            case SYM_READONLY:       is_readonly = true;       continue;
            case SYM_STATIC:         is_static = true;         continue;
            case SYM_WRITEPROTECTED: is_writeprotected = true; continue;
            }
            break;
        };

        if (is_protected && is_writeprotected)
        {
            cc_error("Field cannot be both protected and write-protected.");
            return -1;
        }
        return 0;
    }

    int ParseStruct_IsMemberTypeIllegal(ccInternalList *targ, int stname, AGS::Symbol cursym, bool member_is_pointer, Importness member_is_import)
    {
        const AGS::Symbol curtype = sym.get_type(cursym);
        // must either have a type of a struct here.
        if ((curtype != SYM_VARTYPE) &&
            (curtype != SYM_UNDEFINEDSTRUCT))
        {
            // Complain about non-type
            std::string type_name = sym.get_name(cursym);
            std::string prefix = sym.get_name(stname);
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
            cc_error("Invalid syntax near '%s'", sym.get_friendly_name(cursym).c_str());
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
        if ((curflags & SFLG_STRUCTTYPE) && (member_is_pointer == 0))
        {
            cc_error("Member variable cannot be struct");
            return -1;
        }
        if ((member_is_pointer) && (sym.entries[stname].flags & SFLG_MANAGED) && (member_is_import == kIm_NoImport))
        {
            cc_error("Member variable of managed struct cannot be pointer");
            return -1;
        }
        else if ((curflags & SFLG_MANAGED) && (!member_is_pointer))
        {
            cc_error("Cannot declare non-pointer of managed type");
            return -1;
        }
        else if (((curflags & SFLG_MANAGED) == 0) && (member_is_pointer))
        {
            cc_error("Cannot declare pointer to non-managed type");
            return -1;
        }
        return 0;
    }


    int ParseStruct_CheckMemberNotInInheritedStruct(
        AGS::Symbol vname,
        const char * memberExt,
        AGS::Symbol extendsWhat)
    {
        // check that we haven't already inherited a member
        // with the same name
        AGS::Symbol member = vname;
        if (memberExt == nullptr)
        {
            cc_error("Internal compiler error dbc");
            return -1;
        }
        // skip the colons
        memberExt += 2;
        // find the member-name-only sym
        member = sym_find_or_add(memberExt);

        if (FindMemberSym(extendsWhat, member, true) >= 0)
        {
            cc_error("'%s' already defined by inherited class", sym.get_friendly_name(member).c_str());
            return -1;
        }
        // not found -- a good thing, but FindMemberSym will
        // have errored. Clear the error
        ccError = 0;
        return 0;
    }

    // We have accepted something like "struct foo extends bar { const int".
    // We're waiting for the name of the member.
    int ParseStruct_MemberDefnVarOrFuncOrArray(
        ccInternalList *targ,
        ccCompiledScript * scrip,
        AGS::Symbol extendsWhat,
        AGS::Symbol stname,
        AGS::Symbol name_of_current_func, // [fw] ONLY used for funcs in structs
        int nested_level,
        int curtype,
        bool type_is_attribute,
        bool type_is_readonly,
        Importness type_is_import,
        bool type_is_protected,
        bool type_is_writeprotected,
        bool type_is_pointer,
        bool type_is_static,
        int &size_so_far)
    {

        AGS::Symbol vname = targ->getnext(); // normally variable name, array name, or function name, but can be [ too
        bool isDynamicArray = false;

        // Check whether "[]" is behind the type. 
        // [fw] Is this meant to accept "struct foo { const [] bar; }" ??! 
        if (sym.get_type(vname) == SYM_OPENBRACKET && sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
        {
            isDynamicArray = true;
            targ->getnext(); // Eat "]"
            vname = targ->getnext();
        }

        const char *memberExt = sym.get_name(vname);
        memberExt = strstr(memberExt, "::");

        bool isFunction = sym.get_type(targ->peeknext()) == SYM_OPENPARENTHESIS;

        // If this is a member variable of the struct, then change the symbol to the fully qualified name.
        if (!isFunction && sym.get_type(vname) == SYM_VARTYPE && !sym_is_predef_typename(vname) && memberExt == NULL)
        {
            const char *new_name = GetFullNameOfMember(stname, vname);
            vname = sym_find_or_add(new_name);
        }

        // If, OTOH, this is a member that already has a type which is not VARTYPE or which is below sym.normalFloatSym,
        // then complain.
        if (sym.get_type(vname) != 0 &&
            (sym.get_type(vname) != SYM_VARTYPE || sym_is_predef_typename(vname)))
        {
            cc_error("'%s' is already defined", sym.get_friendly_name(vname).c_str());
            return -1;
        }

        // If this is an extension of another type, then we must make sure that names don't clash. 
        if (extendsWhat > 0)
        {
            int retval = ParseStruct_CheckMemberNotInInheritedStruct(vname, memberExt, extendsWhat);
            if (retval < 0) return retval;
        }

        if (isFunction)
        {
            if (type_is_import == kIm_NoImport)
            {
                cc_error("Function in a struct requires the import keyword");
                return -1;
            }
            if (type_is_writeprotected)
            {
                cc_error("'writeprotected' does not apply to functions");
                return -1;
            }

            int retval = ParseFuncdecl_CheckForIllegalCombis(type_is_readonly, name_of_current_func, nested_level);
            if (retval < 0) return retval;
            {
                AGS::Symbol throwaway_symbol = 0;
                bool throwaway_bool;
                int retval =
                    ParseFuncdecl(targ, scrip, vname, curtype, type_is_pointer, isDynamicArray,
                        type_is_static, type_is_import, stname,
                        throwaway_symbol, throwaway_bool);
            }
            if (retval < 0) return retval;

            if (type_is_protected)
                sym.entries[vname].flags |= SFLG_PROTECTED;

            if (name_of_current_func > 0)
            {
                cc_error("Cannot declare struct member function inside a function body");
                return -1;
            }

        }
        else if (isDynamicArray)
        {
            // Someone tried to declare the function syntax for a dynamic array
            // But there was no function declaration
            cc_error("Expected '('");
            return -1;
        }
        else if ((type_is_import != kIm_NoImport) && (!type_is_attribute))
        {
            // member variable cannot be an import
            cc_error("Only struct member functions may be declared with 'import'");
            return -1;
        }
        else if ((type_is_static) && (!type_is_attribute))
        {
            cc_error("Static variables not supported");
            return -1;
        }
        else if ((curtype == stname) && (!type_is_pointer))
        {
            // cannot do  struct A { A a; }
            // since we don't know the size of A, recursiveness
            cc_error("Struct '%s' cannot be a member of itself", sym.get_friendly_name(curtype).c_str());
            return -1;
        }
        else
        {
            // member variable
            sym.entries[vname].stype = SYM_STRUCTMEMBER;
            sym.entries[vname].extends = stname;  // save which struct it belongs to
            sym.entries[vname].ssize = sym.entries[curtype].ssize;
            sym.entries[vname].soffs = size_so_far;
            sym.entries[vname].vartype = (short)curtype;
            if (type_is_readonly)
                sym.entries[vname].flags |= SFLG_READONLY;
            if (type_is_attribute)
                sym.entries[vname].flags |= SFLG_ATTRIBUTE;
            if (type_is_pointer)
            {
                sym.entries[vname].flags |= SFLG_POINTER;
                sym.entries[vname].ssize = 4;
            }
            if (type_is_static)
                sym.entries[vname].flags |= SFLG_STATIC;
            if (type_is_protected)
                sym.entries[vname].flags |= SFLG_PROTECTED;
            else if (type_is_writeprotected)
                sym.entries[vname].flags |= SFLG_WRITEPROTECTED;

            if (type_is_attribute)
            {
                if (type_is_import == kIm_NoImport)
                {
                    cc_error("Attribute must be import");
                    return -1;
                }

                sym.entries[vname].flags |= SFLG_IMPORTED;

                const char *namePrefix = "";

                if (sym.get_type(targ->peeknext()) == SYM_OPENBRACKET)
                {
                    // An indexed attribute
                    targ->getnext();  // skip the [
                    if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
                    {
                        cc_error("Cannot specify array size for attribute");
                        return -1;
                    }

                    sym.entries[vname].flags |= SFLG_ARRAY;
                    sym.entries[vname].arrsize = 0;
                    namePrefix = "i";
                }
                // the variable name will have been jibbled with
                // the struct name added to it -- strip it back off
                const char *memberPart = strstr(sym.get_name(vname), "::");
                if (memberPart == NULL)
                {
                    cc_error("Internal error: Attribute has no struct name");
                    return -1;
                }
                // seek to the actual member name
                memberPart += 2;

                // declare the imports for the Get and Setters
                char attr_funcname[200];
                sprintf(attr_funcname, "%s::get%s_%s", sym.get_name(stname), namePrefix, memberPart);

                int attr_get = scrip->add_new_import(attr_funcname);
                int attr_set = 0;
                if (!type_is_readonly)
                {
                    // setter only if it's not read-only
                    sprintf(attr_funcname, "%s::set%s_%s", sym.get_name(stname), namePrefix, memberPart);
                    attr_set = scrip->add_new_import(attr_funcname);
                }
                sym.entries[vname].set_attrfuncs(attr_get, attr_set);
            }
            else if (sym.get_type(targ->peeknext()) == SYM_OPENBRACKET)
            {
                // An array!
                targ->getnext();  // skip the [
                AGS::Symbol nextt = targ->getnext();
                int array_size;

                if (sym.get_type(nextt) == SYM_CLOSEBRACKET)
                {
                    if ((sym.entries[stname].flags & SFLG_MANAGED))
                    {
                        cc_error("Member variable of managed struct cannot be dynamic array");
                        return -1;
                    }
                    sym.entries[stname].flags |= SFLG_HASDYNAMICARRAY;
                    sym.entries[vname].flags |= SFLG_DYNAMICARRAY;
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
                sym.entries[vname].flags |= SFLG_ARRAY;
                sym.entries[vname].arrsize = array_size;
            }
            else
            {
                size_so_far += sym.entries[vname].ssize;
            }
        }

        // both functions and variables have this set
        sym.entries[vname].flags |= SFLG_STRUCTMEMBER;

        return 0;
    }

    int ParseStruct_MemberDefn(
        ccInternalList *targ,
        ccCompiledScript * scrip,
        AGS::Symbol stname,
        AGS::Symbol name_of_current_func,
        int nested_level,
        AGS::Symbol extendsWhat,
        int &size_so_far)
    {

        bool type_is_readonly = false;
        Importness type_is_import = kIm_NoImport;
        bool type_is_attribute = false;
        bool type_is_pointer = false;
        bool type_is_static = false;
        bool type_is_protected = false;
        bool type_is_writeprotected = false;

        AGS::Symbol curtype; // the type of the current members being defined, given as a symbol

        // parse qualifiers of the member ("import" etc.), set booleans accordingly
        int retval = ParseStruct_MemberQualifiers(
            targ,
            curtype,
            type_is_readonly,
            type_is_import,
            type_is_attribute,
            type_is_static,
            type_is_protected,
            type_is_writeprotected);
        if (retval < 0) return retval;

        // curtype can now be: typename or typename *

        // A member defn. is a pointer if it is AUTOPOINTER or it has an explicit "*"
        if (sym.entries[curtype].flags & SFLG_AUTOPTR)
        {
            type_is_pointer = true;
        }
        else if (strcmp(sym.get_name(targ->peeknext()), "*") == 0)
        {
            type_is_pointer = true;
            targ->getnext();
        }

        // Certain types of members are not allowed in structs; check this
        retval = ParseStruct_IsMemberTypeIllegal(
            targ,
            stname,
            curtype,
            type_is_pointer,
            type_is_import);
        if (retval < 0) return retval;

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
                stname,     // struct
                name_of_current_func,
                nested_level,
                curtype,             // core type
                type_is_attribute,
                type_is_readonly,
                type_is_import,
                type_is_protected,
                type_is_writeprotected,
                type_is_pointer,
                type_is_static,
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

    // Handle a "struct" definition clause
    int ParseStruct(
        ccInternalList *targ,
        ccCompiledScript * scrip,
        AGS::Symbol cursym,
        bool struct_is_managed,
        bool struct_is_builtin,
        bool struct_is_autoptr,
        bool struct_is_stringstruct,
        AGS::Symbol name_of_current_func,
        size_t &nested_level)
    {
        // get token for name of struct
        AGS::Symbol stname = targ->getnext();
        if ((sym.get_type(stname) != 0) &&
            (sym.get_type(stname) != SYM_UNDEFINEDSTRUCT))
        {
            cc_error("'%s' is already defined", sym.get_friendly_name(stname).c_str());
            return -1;
        }

        // Sums up the size of the struct
        int size_so_far = 0;

        // If the struct extends another struct, the token of the other struct's name
        AGS::Symbol extendsWhat = 0;

        // Write the type of stname into the symbol table
        ParseStruct_SetTypeInSymboltable(
            sym.entries[stname],
            struct_is_managed,
            struct_is_builtin,
            struct_is_autoptr);

        // Declare the struct type that implements new strings
        if (struct_is_stringstruct)
            sym.stringStructSym = stname;

        // forward-declaration of struct type
        if (sym.get_type(targ->peeknext()) == SYM_SEMICOLON)
        {
            targ->getnext(); // gobble the ";"
            sym.entries[stname].stype = SYM_UNDEFINEDSTRUCT;
            sym.entries[stname].ssize = 4;
            return 0;
        }

        // So we are in the "real" declaration.
        // optional "extends" clause
        if (sym.get_type(targ->peeknext()) == SYM_EXTENDS)
        {
            // [fw] At this point, it might be better to copy all the parent elements into this child
            // We need extendsWhat later on.
            ParseStruct_ExtendsClause(targ, stname, extendsWhat, size_so_far);
        }

        // mandatory "{"
        if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
        {
            cc_error("Expected '{'");
            return -1;
        }

        // Process every member of the struct in turn
        while (sym.get_type(targ->peeknext()) != SYM_CLOSEBRACE)
        {
            int retval = ParseStruct_MemberDefn(targ, scrip, stname, name_of_current_func, nested_level, extendsWhat, size_so_far);
            if (retval < 0) return retval;
        }

        // align struct on 4-byte boundary in keeping with compiler
        if ((size_so_far % 4) != 0)
            size_so_far += 4 - (size_so_far % 4);
        sym.entries[stname].ssize = size_so_far;

        // gobble the "}"
        targ->getnext();

        // mandatory ";" after struct defn.
        if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
        {
            cc_error("Expected ';'");
            return -1;
        }
        return 0;
    }


    // We've accepted something like "enum foo { bar"; '=' follows
    int ParseEnum_AssignedValue(ccInternalList * targ, int &currentValue)
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

        if (ParseLiteralOrConstvalue(item_value, currentValue, is_neg, "Expected integer or integer constant after '='") < 0)
            return -1;

        return 0;
    }

    void ParseEnum_Item2Symtable(int enum_name, int item_name, int currentValue)
    {
        sym.entries[item_name].stype = SYM_CONSTANT;
        sym.entries[item_name].ssize = 4;
        sym.entries[item_name].arrsize = 1;
        sym.entries[item_name].vartype = enum_name;
        sym.entries[item_name].sscope = 0;
        sym.entries[item_name].flags = SFLG_READONLY;
        // soffs is unused for a constant, so in a gratuitous
        // hack we use it to store the enum's value
        sym.entries[item_name].soffs = currentValue;
    }

    int ParseEnum_name_2_symtable(int enumName)
    {
        if (sym.get_type(enumName) != 0)
        {
            cc_error("'%s' is already defined", sym.get_friendly_name(enumName).c_str());
            return -1;
        }
        sym.entries[enumName].stype = SYM_VARTYPE;
        sym.entries[enumName].ssize = 4; // standard int size
        sym.entries[enumName].vartype = sym.normalIntSym;

        return 0;
    }

    // enum eEnumName { value1, value2 }
    int ParseEnum0(ccInternalList *targ)
    {
        // Get name of the enum, enter it into the symbol table
        int enum_name = targ->getnext();
        int retval = ParseEnum_name_2_symtable(enum_name);
        if (retval < 0) return retval;

        if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
        {
            cc_error("Expected '{'");
            return -1;
        }


        int currentValue = 0;

        while (true)
        {
            if (ReachedEOF(targ))
            {
                cc_error("Unexpected end of input");
                return -1;
            }

            AGS::Symbol item_name = targ->getnext();
            if (sym.get_type(item_name) == SYM_CLOSEBRACE)
                break; // item list empty or ends with trailing ','

            if (sym.get_type(item_name) != 0)
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


    int ParseImport(ccInternalList *targ, AGS::Symbol name_of_current_func, AGS::Symbol cursym, Importness &next_is_import)
    {
        if (name_of_current_func > 0)
        {
            cc_error("'import' not allowed inside function body");
            return -1;
        }

        next_is_import = kIm_ImportType1;
        if (strcmp(sym.get_name(cursym), "_tryimport") == 0)
            next_is_import = kIm_ImportType2;

        if ((sym.get_type(targ->peeknext()) != SYM_VARTYPE) &&
            (sym.get_type(targ->peeknext()) != SYM_READONLY))
        {
            cc_error("Expected a type or 'readonly' after 'import', not '%s'", sym.get_friendly_name(targ->peeknext()).c_str());
            return -1;
        }
        return 0;
    }

    int ParseStatic(ccInternalList *targ, AGS::Symbol name_of_current_func, bool &next_is_static)
    {
        if (name_of_current_func >= 0)
        {
            cc_error("'static' not allowed inside function body");
            return -1;
        }
        next_is_static = 1;
        if ((sym.get_type(targ->peeknext()) != SYM_VARTYPE) &&
            (sym.get_type(targ->peeknext()) != SYM_READONLY))
        {
            cc_error("Expected a type or 'readonly' after 'static'");
            return -1;
        }
        return 0;
    }

    int ParseProtected(ccInternalList *targ, AGS::Symbol name_of_current_func, bool &next_is_protected)
    {
        if (name_of_current_func > 0)
        {
            cc_error("'protected' not allowed inside a function body");
            return -1;
        }
        next_is_protected = 1;
        if ((sym.get_type(targ->peeknext()) != SYM_VARTYPE) &&
            (sym.get_type(targ->peeknext()) != SYM_STATIC) &&
            (sym.get_type(targ->peeknext()) != SYM_READONLY))
        {
            cc_error("Expected a type, 'static' or 'readonly' after 'protected'");
            return -1;
        }
        return 0;
    }

    int ParseExport(ccInternalList *targ, ccCompiledScript * scrip, AGS::Symbol &cursym)
    {
        // export specified symbol
        cursym = targ->getnext();
        while (sym.get_type(cursym) != SYM_SEMICOLON)
        {
            int nextype = sym.get_type(cursym);
            if (nextype == 0)
            {
                cc_error("Can only export global variables and functions, not '%s'", sym.get_friendly_name(cursym).c_str());
                return -1;
            }
            if ((nextype != SYM_GLOBALVAR) && (nextype != SYM_FUNCTION))
            {
                cc_error("Invalid export symbol '%s'", sym.get_friendly_name(cursym).c_str());
                return -1;
            }
            if (sym.entries[cursym].flags & SFLG_IMPORTED)
            {
                cc_error("Cannot export an import");
                return -1;
            }
            if (sym.entries[cursym].flags & SFLG_ISSTRING)
            {
                cc_error("Cannot export string; use char[200] instead");
                return -1;
            }
            // if all functions are being exported anyway, don't bother doing
            // it now
            if ((ccGetOption(SCOPT_EXPORTALL) != 0) && (nextype == SYM_FUNCTION))
            { }
            else if (scrip->add_new_export(sym.get_name(cursym),
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
                cc_error("Expected ',' instead of '%s'", sym.get_friendly_name(cursym).c_str());
                return -1;
            }
            cursym = targ->getnext();
        }

        return 0;
    }

    int ParseVartype_GetVarName(ccInternalList * targ, AGS::Symbol & varname, AGS::Symbol & struct_of_member_fct)
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
        const char *full_name_str = GetFullNameOfMember(struct_of_member_fct, member_of_member_function);
        varname = sym.find(full_name_str);
        if (varname < 0)
        {
            cc_error("'%s' does not contain a function '%s'",
                sym.get_friendly_name(struct_of_member_fct).c_str(),
                sym.get_friendly_name(member_of_member_function).c_str());
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

    int ParseVartype_GetPointerStatus(ccInternalList * targ, int type_of_defn, bool &isPointer)
    {
        isPointer = false;
        if (targ->peeknext() == sym.find("*"))
        {
            // only allow pointers to structs
            if ((sym.entries[type_of_defn].flags & SFLG_STRUCTTYPE) == 0)
            {
                cc_error("Cannot create pointer to basic type");
                return -1;
            }
            if (sym.entries[type_of_defn].flags & SFLG_AUTOPTR)
            {
                cc_error("Invalid use of '*'");
                return -1;
            }
            isPointer = true;
            targ->getnext();
        }

        if (sym.entries[type_of_defn].flags & SFLG_AUTOPTR)
            isPointer = true;

        return 0;
    }


    int ParseVartype_CheckIllegalCombis(bool is_static, bool is_member_definition, bool is_function, bool is_protected, bool loopCheckOff, bool is_import)
    {
        if (is_static && (!is_function || !is_member_definition))
        {
            cc_error("'static' only applies to member functions");
            return -1;
        }

        if (!is_function && is_protected)
        {
            cc_error("'protected' not valid in this context");
            return -1;
        }

        if (!is_function && loopCheckOff)
        {
            cc_error("'noloopcheck' not valid in this context");
            return -1;
        }

        if (!is_function && is_static)
        {
            cc_error("Invalid use of 'static'");
            return -1;
        }

        if (is_function && loopCheckOff && is_import)
        {
            cc_error("'noloopcheck' cannot be applied to imported functions");
            return -1;
        }
        return 0;
    }

    int ParseVartype_FuncDef(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol &func_name, int type_of_defn, bool is_readonly, bool isPointer, bool isDynamicArray, bool is_static, Importness is_import, bool is_member_function_definition, bool is_protected, AGS::Symbol & struct_of_current_func, AGS::Symbol &name_of_current_func)
    {

        bool body_follows;
        // restore flags, since remove_any_imports() zeros them out
        if (is_member_function_definition)
            sym.entries[func_name].flags |= SFLG_STRUCTMEMBER;
        if (is_protected)
            sym.entries[func_name].flags |= SFLG_PROTECTED;
        int retval = ParseFuncdecl(
            targ, scrip, func_name, type_of_defn, isPointer, isDynamicArray,
            is_static, is_import, struct_of_current_func,
            struct_of_current_func, body_follows);
        if (retval < 0) return retval;

        // If we've started a function, remember what it is.
        if (body_follows)
            name_of_current_func = func_name;

        return 0;
    }


    int ParseVartype_VarDef(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &var_name, Globalness is_global, int nested_level, bool is_readonly, int type_of_defn, int next_type, bool isPointer, bool &another_var_follows)
    {

        if (is_global == kGl_Local)
            sym.entries[var_name].sscope = nested_level;
        if (is_readonly)
            sym.entries[var_name].flags |= SFLG_READONLY;

        // parse the definition
        int retval = ParseVardecl(targ, scrip, var_name, type_of_defn, next_type, is_global, isPointer, another_var_follows);
        if (retval < 0) return retval;
        return 0;
    }

    // We accepted a variable type such as "int", so what follows is a function or variable definition
    int ParseVartype(
        ccInternalList *targ,
        ccCompiledScript *scrip,
        AGS::Symbol type_of_defn,           // e.g., "int"
        AGS::NestingStack *nesting_stack,
        Importness is_import,             // can be 0 or 1 or 2 
        bool is_readonly,
        bool is_static,
        bool is_protected,
        AGS::Symbol &name_of_current_func,
        AGS::Symbol &struct_of_current_func, // 0 if _not_ a member function
        bool &loopCheckOff)
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
        if (dynArrayStatus < 0)
            return dynArrayStatus;
        bool isDynamicArray = (dynArrayStatus > 0);

        // Look for "noloopcheck"; if present, gobble it and set the indicator
        // "TYPE noloopcheck foo(...)"
        loopCheckOff = false;
        if (sym.get_type(targ->peeknext()) == SYM_LOOPCHECKOFF)
        {
            targ->getnext();
            loopCheckOff = true;
        }

        Globalness is_global = kGl_Local;
        if (name_of_current_func <= 0)
            is_global = (is_import == kIm_NoImport) ? kGl_GlobalNoImport : kGl_GlobalImport;

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
                cc_error("'%s' is already in use as a type name", sym.get_friendly_name(var_or_func_name).c_str());
                return -1;
            }

            // Check whether var or func is being defined
            int next_type = sym.get_type(targ->peeknext());
            bool is_function = (sym.get_type(targ->peeknext()) == SYM_OPENPARENTHESIS);
            bool is_member_definition = (struct_of_current_func > 0);

            // certains modifiers, such as "static" only go with certain kinds of definitions.
            retval = ParseVartype_CheckIllegalCombis(is_static, is_member_definition, is_function, is_protected, loopCheckOff, (is_import != kIm_NoImport));
            if (retval < 0) return retval;

            if (is_function) // function defn
            {
                int retval = ParseFuncdecl_CheckForIllegalCombis(is_readonly, name_of_current_func, nesting_stack->Depth());
                if (retval < 0) return retval;

                retval = ParseVartype_FuncDef(targ, scrip, var_or_func_name, type_of_defn, is_readonly, isPointer, isDynamicArray, is_static, is_import, is_member_definition, is_protected, struct_of_current_func, name_of_current_func);
                if (retval < 0) return retval;
                another_ident_follows = false; // Can't join another func or var with ','
            }
            else // variable defn
            {
                retval = ParseVartype_VarDef(targ, scrip, var_or_func_name, is_global, nesting_stack->Depth() - 1, is_readonly, type_of_defn, next_type, isPointer, another_ident_follows);
                if (retval < 0) return retval;
            }

        }
        while (another_ident_follows);

        return 0;
    }

    int ParseUndefToken(AGS::Symbol cursym)
    {
        char ascii_explanation[20] = "";
        const char *symname = sym.get_name(cursym);
        if ((symname[0] <= 32) || (symname[0] >= 128))
            sprintf(ascii_explanation, "(ASCII index %02X)", symname[0]);

        cc_error("Undefined token '%s' %s", symname, ascii_explanation);
        return -1;
    }


    int ParseCommand_EndOfDoIfElse(ccInternalList * targ, ccCompiledScript * scrip, AGS::NestingStack *nesting_stack)
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

    int ParseReturn(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol inFuncSym)
    {
        int functionReturnType = sym.entries[inFuncSym].funcparamtypes[0];

        if (sym.get_type(targ->peeknext()) != SYM_SEMICOLON)
        {
            if (functionReturnType == sym.normalVoidSym)
            {
                cc_error("Cannot return value from void function");
                return -1;
            }

            // parse what is being returned
            int retval = ParseExpression(targ, scrip, false);
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
            cc_error("Must return a '%s' value from function", sym.get_friendly_name(functionReturnType).c_str());
            return -1;
        }
        else
        {
            scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
        }

        int cursym = sym.get_type(targ->getnext());
        if (cursym != SYM_SEMICOLON)
        {
            cc_error("Expected ';' instead of '%s'", sym.get_name(cursym));
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
    int ParseIf(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
    {
        // Get expression, must be in parentheses
        if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
        {
            cc_error("Expected '('");
            return -1;
        }

        int retval = ParseExpression(targ, scrip, true);
        if (retval < 0) return retval;

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
    int ParseWhile(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
    {
        // Get expression, must be in parentheses
        if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
        {
            cc_error("Expected '('");
            return -1;
        }

        // point to the start of the code that evaluates the condition
        AGS::CodeLoc condition_eval_loc = scrip->codesize;

        int retval = ParseExpression(targ, scrip, true);
        if (retval < 0) return retval;

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

    int ParseDo(ccInternalList * targ, ccCompiledScript * scrip, AGS::NestingStack *nesting_stack)
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

    int ParseFor_InitClause(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &cursym, size_t nested_level)
    {
        // Check for empty init clause
        if (sym.get_type(cursym) == SYM_SEMICOLON)
            return 0;

        // This will contain buffered code.
        AGS::Symbol vnlist[TEMP_SYMLIST_LENGTH];
        size_t vnlist_len;
        int offset_of_funcname; // -1 means: No function call

        int retval = BufferVarOrFunccall(targ, cursym, vnlist, vnlist_len, offset_of_funcname);
        if (retval < 0) return retval;
        if (sym.get_type(cursym) == SYM_VARTYPE)
        {
            int vtwas = cursym;

            bool isPointer = false;

            if (strcmp(sym.get_name(targ->peeknext()), "*") == 0)
            {
                // only allow pointers to structs
                if ((sym.entries[vtwas].flags & SFLG_STRUCTTYPE) == 0)
                {
                    cc_error("Cannot create pointer to basic type");
                    return -1;
                }
                if (sym.entries[vtwas].flags & SFLG_AUTOPTR)
                {
                    cc_error("Invalid use of '*'");
                    return -1;
                }
                isPointer = true;
                targ->getnext();
            }

            if (sym.entries[vtwas].flags & SFLG_AUTOPTR)
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
                if (cursym == SCODE_META)
                {
                    // eg. "int" was the last word in the file
                    currentline = targ->lineAtEnd;
                    cc_error("Unexpected end of file");
                    return -1;
                }

                int next_type = sym.get_type(targ->peeknext());
                if (next_type == SYM_MEMBERACCESS || next_type == SYM_OPENPARENTHESIS)
                {
                    cc_error("Function declaration not allowed in for loop initialiser");
                    return -1;
                }
                else if (sym.get_type(cursym) != 0)
                {
                    cc_error("Variable '%s' is already defined", sym.get_name(cursym));
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
            retval = ParseAssignment(targ, scrip, cursym, SYM_SEMICOLON, vnlist, vnlist_len);
            if (retval < 0) return retval;
        }
        return 0;
    }

    int ParseFor_WhileClause(ccInternalList * targ, ccCompiledScript * scrip)
    {
        // Check for empty while clause
        if (sym.get_type(targ->peeknext()) == SYM_SEMICOLON)
        {
            // Not having a while clause is tantamount to the while condition "true".
            // So let's write "true" to the AX register.
            scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 1);
            targ->getnext();
            return 0;
        }

        int retval = ParseExpression(targ, scrip, false);
        if (retval < 0) return retval;

        if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
        {
            cc_error("Expected ';'");
            return -1;
        }
        return 0;
    }

    int ParseFor_IterateClause(ccInternalList *targ, ccCompiledScript *scrip, AGS::Symbol &cursym)
    {
        // Check for empty interate clause
        if (sym.get_type(cursym) == SYM_CLOSEPARENTHESIS)
            return 0;

        // This will contain buffered code.
        AGS::Symbol vnlist[TEMP_SYMLIST_LENGTH];
        size_t vnlist_len;
        int offset_of_funcname; // -1 means: No function call

        int retval = BufferVarOrFunccall(targ, cursym, vnlist, vnlist_len, offset_of_funcname);
        if (retval < 0) return retval;

        retval = ParseAssignment(targ, scrip, cursym, SYM_CLOSEPARENTHESIS, vnlist, vnlist_len);
        if (retval < 0) return retval;

        return 0;
    }


    int ParseFor(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol &cursym, AGS::NestingStack *nesting_stack)
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
        // TODO: This is either assignment_or_funccall or a vardef.
        // If it is a vardef, it should begin with VARTYPE.
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

        // TODO: This should be ParseAssignment_or_funccall
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
        nesting_stack->YankChunk(scrip, iterate_clause_loc, pre_fixup_count);

        // Code for "If the expression we just evaluated is false, jump over the loop body."
        // the 0 will be fixed to a proper offset later
        scrip->write_cmd1(SCMD_JZ, 0);
        nesting_stack->SetJumpOutLoc(scrip->codesize - 1); // the address to fix

        return 0;
    }


    int ParseSwitch(ccInternalList * targ, ccCompiledScript * scrip, AGS::NestingStack *nesting_stack)
    {
        // Get the switch expression, must be in parentheses
        if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
        {
            cc_error("Expected '('");
            return -1;
        }

        int retval = ParseExpression(targ, scrip, true);
        if (retval < 0) return retval;

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
            cc_error("Invalid keyword '%s' in switch statement block", sym.get_name(targ->peeknext()));
            return -1;
        }
        return 0;
    }

    int ParseCasedefault(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol cursym, AGS::NestingStack *nesting_stack)
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
            int retval = ParseExpression(targ, scrip, false);
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
            nesting_stack->YankChunk(scrip, start_of_code_loc, numfixups_at_start_of_code);
        }

        // expect and gobble the ':'
        if (sym.get_type(targ->getnext()) != SYM_LABEL)
        {
            cc_error("Expected ':'");
            return -1;
        }

        return 0;
    }

    int ParseBreak(ccInternalList * targ, ccCompiledScript * scrip, AGS::NestingStack *nesting_stack)
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

    int ParseContinue(ccInternalList * targ, ccCompiledScript * scrip, AGS::NestingStack *nesting_stack)
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
        FreePointersOfLocals(scrip, loop_level);

        // Pop local variables from the stack
        int totalsub = StacksizeOfLocals(loop_level);
        if (totalsub > 0)
            scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
        scrip->flush_line_numbers();

        // if it's a for loop, drop the yanked chunk (loop increment) back in
        if (nesting_stack->ChunksExist(loop_level))
            nesting_stack->WriteChunk(scrip, loop_level, 0);
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
    // Thus, we should be at the start of an assignment. Compile it.
    int ParseAssignment_or_funccall(ccInternalList * targ, ccCompiledScript * scrip, AGS::Symbol cursym)
    {
        AGS::Symbol selector_script[TEMP_SYMLIST_LENGTH];
        size_t selector_script_len;
        int func_idx;

        // Read ahead one variable or func call
        size_t expr_start = targ->pos;
        int retval = BufferVarOrFunccall(targ, cursym, selector_script, selector_script_len, func_idx);
        if (retval < 0) return retval;

        if (func_idx >= 0)
        {
            // This is a func call. Back up and read it as an expression
            // We need to back up to 1 place before we were because ParseExpression()
            // expects the symbol that will be processed to be still unread, 
            // whereas in here, the symbol that is processed is already read
            targ->pos = expr_start - 1;
            int retval = ParseExpression(targ, scrip, false);
            if (retval < 0) return retval;

            if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
            {
                cc_error("Expected ';'");
                return -1;
            }
            return 0;
        }

        // Check whether the next symbol is an assignment symbol
        AGS::Symbol nextsym = targ->peeknext();
        int nexttype = sym.get_type(nextsym);
        if (nexttype != SYM_ASSIGN && nexttype != SYM_MASSIGN && nexttype != SYM_SASSIGN)
        {
            cc_error("Parse error at '%s'", sym.get_friendly_name(cursym).c_str());
            return -1;
        }
        return ParseAssignment(targ, scrip, cursym, SYM_SEMICOLON, selector_script, selector_script_len);
    }


    int ParseCommand(
        ccInternalList *targ,
        ccCompiledScript * scrip,
        AGS::Symbol cursym,
        AGS::Symbol &name_of_current_func,
        AGS::Symbol &struct_of_current_func,
        AGS::NestingStack *nesting_stack,
        bool &next_is_noloopcheck)
    {
        int retval;
        int curtype = sym.get_type(cursym);

        switch (curtype)
        {
        default:
            // If it doesn't begin with a keyword, it should be an assignment
            // or a func call.
            retval = ParseAssignment_or_funccall(targ, scrip, cursym);
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
            retval = ParseOpenbrace(scrip, nesting_stack, name_of_current_func, struct_of_current_func, next_is_noloopcheck);
            next_is_noloopcheck = false;
            return retval;

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

    int cc_parse(ccInternalList *targ, ccCompiledScript *scrip, size_t &nested_level, AGS::Symbol &name_of_current_func)
    {
        AGS::Symbol struct_of_current_func = 0; // non-zero only when a struct member function is open

        AGS::NestingStack nesting_stack = AGS::NestingStack();

        // These are indicators that are switched on when keywords are encountered and switched off after they are used.
        // Thus, they are called "NEXT_IS_..." although they are sometimes _used_ with the meaning "THIS_IS...".
        bool next_is_managed = false;
        bool next_is_builtin = false;
        bool next_is_autoptr = false;
        bool next_is_stringstruct = false;
        bool next_is_readonly = false;
        bool next_is_static = false;
        bool next_is_protected = false;
        bool next_is_noloopcheck = false;

        // This is NOT an indicator proper since it can have values from 0 to 2.
        // But 0 seems to mean "next is NOT an import".
        Importness next_is_import = kIm_NoImport; // NOT a bool, can be 0 or 1 or 2

        // Go through the list of tokens one by one. We start off in the global data
        // part - no code is allowed until a function definition is started
        currentline = 1; // This is a global variable. cc_internallist.cpp, cs_internallist_test.cpp, cs_parser.cpp
        int currentlinewas = 0;

        while (!ReachedEOF(targ))
        {
            AGS::Symbol cursym = targ->getnext();

            int retval = cc_parse_HandleLines(targ, scrip, currentlinewas);
            if (retval > 0)
                break; // end of input

            // Handling new sections
            // These section are denoted by a string that begins with NEW_SCRIPT_TOKEN_PREFIX;
            // the section name follows.
            if (strncmp(sym.get_name(cursym), NEW_SCRIPT_TOKEN_PREFIX, 18) == 0)
            {
                // scriptNameBuffer is a static C string. 
                // We can't replace it by a std::string  because it is referenced 
                // externally through ccCurScriptName (defined in cc_error.cpp)
                strncpy(ScriptNameBuffer, sym.get_name(cursym) + 18, 236);
                ScriptNameBuffer[strlen(ScriptNameBuffer) - 1] = 0;  // strip closing quote
                ccCurScriptName = ScriptNameBuffer;
                scrip->start_new_section(ScriptNameBuffer);
                currentline = 0;
                continue;
            }


            const int symType = sym.get_type(cursym);

            switch (symType)
            {
            default: break;

            case 0:
                return ParseUndefToken(cursym);

            case  SYM_AUTOPTR:
            {
                next_is_autoptr = true;
                int type_of_next_sym = sym.get_type(targ->peeknext());
                if (type_of_next_sym != SYM_MANAGED && type_of_next_sym != SYM_BUILTIN)
                {
                    cc_error("Expected 'managed' or 'builtin' after 'autoptr'");
                    return -1;
                }
                continue;
            }

            case SYM_BUILTIN:
            {
                next_is_builtin = true;
                int type_of_next_sym = sym.get_type(targ->peeknext());
                if ((type_of_next_sym != SYM_MANAGED) && (type_of_next_sym != SYM_STRUCT))
                {
                    cc_error("'builtin' can only be used with 'struct' or 'managed struct'");
                    return -1;
                }
                continue;
            }

            case SYM_CONST:
            {
                cc_error("'const' is only valid for function parameters (use 'readonly' instead)");
                return -1;
            }

            case SYM_ENUM:
            {
                int retval = ParseEnum(targ, name_of_current_func);
                if (retval < 0) return retval;

                continue;
            }

            case SYM_EXPORT:
            {
                int retval = ParseExport(targ, scrip, cursym);
                if (retval < 0) return retval;

                continue;
            }

            case SYM_IMPORT:
            {
                int retval = ParseImport(targ, name_of_current_func, cursym, next_is_import);
                if (retval < 0) return retval;
                continue;
            }

            case  SYM_MANAGED:
            {
                next_is_managed = true;
                if (sym.get_type(targ->peeknext()) != SYM_STRUCT)
                {
                    cc_error("'managed' can only be used with 'struct'");
                    return -1;
                }

                continue;
            }

            case SYM_PROTECTED:
            {
                int retval = ParseProtected(targ, name_of_current_func, next_is_protected);
                if (retval < 0) return retval;
                continue;
            }

            case SYM_READONLY:
            {
                next_is_readonly = true;
                if (sym.get_type(targ->peeknext()) != SYM_VARTYPE)
                {
                    cc_error("Expected type after 'readonly'");
                    return -1;
                }
                continue;
            }

            case SYM_STATIC:
            {
                int retval = ParseStatic(targ, name_of_current_func, next_is_static);
                if (retval < 0) return retval;
                continue;
            }

            case SYM_STRINGSTRUCT:
            {
                next_is_stringstruct = true;
                if (sym.stringStructSym > 0)
                {
                    cc_error("stringstruct already defined");
                    return -1;
                }
                if (sym.get_type(targ->peeknext()) != SYM_AUTOPTR)
                {
                    cc_error("Expected 'autoptr' after 'stringstruct'");
                    return -1;
                }
                continue;
            }

            case  SYM_STRUCT:
            {
                int retval = ParseStruct(targ, scrip, cursym, next_is_managed, next_is_builtin, next_is_autoptr, next_is_stringstruct, name_of_current_func, nested_level);
                if (retval < 0) return retval;

                next_is_managed = false;
                next_is_builtin = false;
                next_is_autoptr = false;
                next_is_stringstruct = false;
                continue;
            }

            case SYM_VARTYPE:
            {
                if (sym.get_type(targ->peeknext()) == SYM_DOT)
                {
                    // We're looking at "int ." or similar; treated below the switch
                    break;
                }
                // func or variable definition
                int retval = ParseVartype(targ, scrip, cursym, &nesting_stack, next_is_import, next_is_readonly, next_is_static, next_is_protected, name_of_current_func, struct_of_current_func, next_is_noloopcheck);
                if (retval < 0) return retval;
                next_is_import = kIm_NoImport;
                next_is_readonly = false;
                next_is_static = false;
                next_is_protected = false;
                continue;
            }

            } // switch (symType)

            // Commands are only allowed within a function
            if (name_of_current_func <= 0)
            {
                cc_error("'%s' is illegal outside a function", sym.get_friendly_name(cursym).c_str());
                return -1;
            }

            retval = ParseCommand(targ, scrip, cursym, name_of_current_func, struct_of_current_func, &nesting_stack, next_is_noloopcheck);
            if (retval < 0) return retval;
        } // for

        return 0;
    }

    // compile the code in the INPL parameter into code in the scrip structure,
    // but don't reset anything because more files could follow
    int cc_compile(const char *inpl, ccCompiledScript *scrip)
    {
        // Scan the program code.
        ccInternalList targ;
        int retval = cc_tokenize(inpl, &targ, scrip);
        if (retval < 0) return retval;

        targ.startread();

        size_t nested_level = 0;
        AGS::Symbol name_of_current_func = -1;
        retval = cc_parse(&targ, scrip, nested_level, name_of_current_func);
        if (retval < 0) return retval;

        // Here when the tokens have been exhausted
        if (name_of_current_func > 0)
        {
            currentline = targ.lineAtEnd;
            std::string func_identification = "current function";
            if (name_of_current_func != 0)
            {
                func_identification = "function '&1'";
                func_identification.replace(func_identification.find("&1"), 2, sym.get_name_string(name_of_current_func));
            }
            cc_error("At end of input, but body of '%s' has not been closed", func_identification.c_str());
            return -1;
        }

        if (nested_level == 0)
            return 0;

        currentline = targ.lineAtEnd;
        cc_error("At end of input, but an open '{' is missing its '}'");
        return -1;
    }

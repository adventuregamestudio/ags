
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <vector>
#include "cs_parser.h"
#include "cc_internallist.h"    // ccInternalList
#include "cs_parser_common.h"
#include "cc_symboltable.h"
#include "script/cc_options.h"
#include "script/script_common.h"
#include "script/cc_error.h"
#include "cc_variablesymlist.h"

#include "fmem.h"

extern int currentline;

char ccCopyright[]="ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2020 others";
static char scriptNameBuffer[256];

int  evaluate_expression(ccInternalList*,ccCompiledScript*,int,bool insideBracketedDeclaration);
int  evaluate_assignment(ccInternalList *targ, ccCompiledScript *scrip, bool expectCloseBracket, int cursym, long lilen, long *vnlist, bool insideBracketedDeclaration);
int  parse_sub_expr(long*,int,ccCompiledScript*);
long extract_variable_name(int, ccInternalList*,long*, int*);
int  check_type_mismatch(int typeIs, int typeWantsToBe, int orderMatters);
int  check_operator_valid_for_type(int *vcpuOp, int type1, int type2);
void yank_chunk(ccCompiledScript *scrip, std::vector<ccChunk> *list, int codeoffset, int fixupoffset);
void write_chunk(ccCompiledScript *scrip, ccChunk item);
void clear_chunk_list(std::vector<ccChunk> *list);
int  is_any_type_of_string(int symtype);

void yank_chunk(ccCompiledScript *scrip, std::vector<ccChunk> *list, int codeoffset, int fixupoffset) {
    ccChunk item;
    int index;

    for(index = codeoffset; index < scrip->codesize; index++)
        item.code.push_back(scrip->code[index]);
    for(index = fixupoffset; index < scrip->numfixups; index++) {
        item.fixups.push_back(scrip->fixups[index]);
        item.fixuptypes.push_back(scrip->fixuptypes[index]);
    }
    item.codeoffset = codeoffset;
    item.fixupoffset = fixupoffset;
    list->push_back(item);
    scrip->codesize = codeoffset;
    scrip->numfixups = fixupoffset;
}

void write_chunk(ccCompiledScript *scrip, ccChunk item) {
    int index;
    int limit;
    int adjust;

    scrip->flush_line_numbers();
    adjust = scrip->codesize - item.codeoffset;
    limit = item.code.size();
    for(index = 0; index < limit; index++) {
        scrip->write_code(item.code[index]);
    }
    limit = item.fixups.size();
    for(index = 0; index < limit; index++)
        scrip->add_fixup(item.fixups[index] + adjust, item.fixuptypes[index]);
}

void clear_chunk_list(std::vector<ccChunk> *list) {
    list->clear();
}

int is_part_of_symbol(char thischar, char startchar) {
    // workaround for strings
    static int sayno_next_char = 0;
    static int next_is_escaped = 0;
    if (sayno_next_char) {
        sayno_next_char = 0;
        return 0;
    }
    if ((startchar == '\"') || (startchar == '\'')) {
        if (next_is_escaped) {
            // an escaped " or whatever, so let it through
            next_is_escaped = 0;
            return 1;
        }
        if (thischar == '\\')
            next_is_escaped = 1;

        if (thischar == startchar) sayno_next_char = 1;
        return 1;
    }
    // a decimal number
    if ((startchar >= '0') && (startchar <= '9')) {
        if ((thischar >= '0') && (thischar <= '9'))
            return 1;
        // float constant
        if (thischar == '.')
            return 1;
        return 0;
    }
    // variable name
    if (((startchar >= 'a') && (startchar <= 'z')) ||
        ((startchar >= 'A') && (startchar <= 'Z')) ||
        (startchar == '_')) {
            if (is_alphanum(thischar)) return 1;
            return 0;
    }
    // ==, >=, <=, !=, etc
    if (thischar == '=') {
        if ((startchar == '=') || (startchar == '<') || (startchar == '>')
            || (startchar == '!') || (startchar == '+') || (startchar == '-')
            || (startchar == '*') || (startchar == '/')
            || (startchar == '&') || (startchar == '|') || (startchar == '^'))
            return 1;
    }
    // && and ||, ++ and --
    if ((thischar == '&') && (startchar == '&')) return 1;
    if ((thischar == '|') && (startchar == '|')) return 1;
    if ((thischar == '+') && (startchar == '+')) return 1;
    if ((thischar == '-') && (startchar == '-')) return 1;
    // << and >>
    if ((thischar == '<') && (startchar == '<')) return 1;
    if ((thischar == '>') && (startchar == '>')) return 1;
    // ...
    if ((thischar == '.') && (startchar == '.')) return 1;
    // ::
    if ((thischar == ':') && (startchar == ':')) return 1;

    return 0;
}

char constructedMemberName[MAX_SYM_LEN];
const char *get_member_full_name(int structSym, int memberSym) {

    const char* memberName = sym.get_name(memberSym);

    // de-mangle name, if appropriate
    if (memberName[0] == '.')
        memberName = &memberName[1];

    sprintf(constructedMemberName, "%s::%s", sym.get_name(structSym), memberName);

    return constructedMemberName;
}

int sym_find_or_add(symbolTable &sym, const char *sname) {
    int symdex = sym.find(sname);
    if (symdex < 0) {
        symdex = sym.add(sname);
    }
    return symdex;
}

int cc_tokenize(const char*inpl, ccInternalList*targ, ccCompiledScript*scrip) {
    // *** create the symbol table and parse the text code into symbol code
    int linenum=1,in_struct_declr=-1,bracedepth = 0, last_time=0;
    int parenthesisdepth = 0;
    FMEM*iii=fmem_open(inpl);
    targ->write_meta(SMETA_LINENUM,1);
    while (!fmem_eof(iii)) {
        int thischar, waseof = 0;
        do {  // skip the whitespace
            if (fmem_eof(iii)) {
                waseof = 1;
                break;
            }
            thischar = fmem_getc(iii);
        } while (is_whitespace(thischar));
        // if it was the end of file, abort
        if (waseof)
            break;
        if ((thischar == '\r') || (thischar == '\n')) {
            // write the line number (for debugging)
            linenum++;
            targ->write_meta(SMETA_LINENUM,linenum);
            if (fmem_peekc(iii) =='\n') fmem_getc(iii);
            currentline=linenum;
            // go back and get the whitespace after the CRLF
            continue;
        }
        // it's some sort of symbol, so read it in
        char thissymbol[MAX_SYM_LEN];
        int symlen=1;
        thissymbol[0]=thischar;
        int thisIsEscaped = 0;
        while (is_part_of_symbol(fmem_peekc(iii),thischar)) {

            thissymbol[symlen] = fmem_getc(iii);
            symlen++;
            if (symlen >= MAX_SYM_LEN - 1) break;
        }
        thissymbol[symlen]=0;
        if ((thissymbol[0] == '\'') && (thissymbol[2] == '\'')) {
            // convert the character to its ASCII equivalent
            sprintf(thissymbol,"%d",thissymbol[1]);
        }
        else if (thissymbol[0] == '\'') {
            cc_error("incorrectly terminated character constant");
            return -1;
        }

        if (sym.entries[last_time].stype == SYM_DOT) {
            // mangle member variable accesses so that you can have a
            // struct called Room but also a member property called Room
            char thissymbol_mangled[MAX_SYM_LEN + 1];
            sprintf(thissymbol_mangled, ".%s", thissymbol);
            strcpy(thissymbol, thissymbol_mangled);
        }

        int towrite = sym_find_or_add(sym, thissymbol);
        if (towrite < 0) {
            cc_error("symbol table overflow - could not ensure new symbol.");
            return -1;
        }
        if ((thissymbol[0] >= '0') && (thissymbol[0] <= '9')) {
            if (strchr(thissymbol, '.') != NULL)
                sym.entries[towrite].stype = SYM_LITERALFLOAT;
            else
                sym.entries[towrite].stype = SYM_LITERALVALUE;
        }
        if (sym.entries[towrite].stype == SYM_MASSIGN && thischar == '*' && sym.entries[last_time].stype == SYM_VARTYPE) {
            // Break up *= when a parameter is declared with only the type and an initial value
            // e.g. import void someMethod(InventoryItem *= 0);
            targ->write(sym.find("*"));
            towrite = sym.find("=");
        }

        if (sym.entries[towrite].stype == SYM_OPENPARENTHESIS)
            parenthesisdepth++;
        else if (sym.entries[towrite].stype == SYM_CLOSEPARENTHESIS)
            parenthesisdepth--;

        // deal with forward-declas
        if ((sym.entries[towrite].stype == SYM_SEMICOLON) && (bracedepth == 0))
            in_struct_declr = -1;

        // this bit sorts out renaming struct members to allow different
        // structs to have same member names at different offsets
        if (towrite < 1) ;
        else if (sym.entries[last_time].stype == SYM_STRUCT) {
            in_struct_declr = towrite;
            bracedepth = 0;
        }
        else if ((sym.entries[last_time].stype == SYM_ENUM) && (sym.entries[towrite].stype == 0)) {
            // make sure it doens't get jibbled when used within
            // structs
            sym.entries[towrite].stype = SYM_TEMPORARYTYPE;
        }
        else if ((sym.entries[towrite].stype == SYM_OPENBRACE) && (in_struct_declr >= 0))
            bracedepth++;
        else if ((sym.entries[towrite].stype == SYM_CLOSEBRACE) && (in_struct_declr >= 0)) {
            bracedepth--;
            if (bracedepth <= 0)
                in_struct_declr = -1;
        }
        else if ((sym.entries[towrite].stype == 0 || sym.entries[towrite].stype == SYM_FUNCTION || sym.entries[towrite].stype == SYM_GLOBALVAR) &&
            (in_struct_declr >= 0) && (parenthesisdepth == 0) && (bracedepth > 0)) {
                // change the name of structure members so that the same member name
                // can be used in multiple structs
                // (but only if not currently in a function params list
                // and not an imported func/property type)
                // (and if not the struct type (this allows member functions
                // which return the struct)
                if ((sym.entries[last_time].stype != SYM_PROPERTY) &&
                    (sym.entries[last_time].stype != SYM_IMPORT) &&
                    (sym.entries[last_time].stype != SYM_STATIC) &&
                    (sym.entries[last_time].stype != SYM_PROTECTED) &&
                    (sym.entries[last_time].stype != SYM_WRITEPROTECTED) &&
                    (sym.entries[last_time].stype != SYM_SEMICOLON) &&
                    (sym.entries[last_time].stype != SYM_OPENBRACE) &&
                    (sym.entries[last_time].stype != SYM_OPENBRACKET) &&
                    (towrite != in_struct_declr)) {
                        const char *new_name = get_member_full_name(in_struct_declr, towrite);
                        //      printf("changed '%s' to '%s'\n",sym.get_friendly_name(towrite).c_str(),new_name);
                        towrite = sym_find_or_add(sym, new_name);
                        if (towrite < 0) {
                            cc_error("symbol table error - could not ensure new struct symbol.");
                            return -1;
                        }
                }
        }

        if (thissymbol[0]=='\"') {
            // strip closing speech mark
            thissymbol[strlen(thissymbol)-1] = 0;
            // save the string into the string table area
            sym.entries[towrite].stype = SYM_STRING;
            sym.entries[towrite].soffs = scrip->add_string(&thissymbol[1]);
            // set it to be recognised as a string
            sym.entries[towrite].vartype = sym.normalStringSym;

            if (strncmp(thissymbol, NEW_SCRIPT_TOKEN_PREFIX, 18) == 0)
            {
                linenum = 0;
            }
        }
        targ->write(towrite);
        last_time = towrite;
    }
    fmem_close(iii);
    targ->write_meta(SMETA_END,0);
    // clear any temporary tpyes set
    for (int ii = 0; (size_t)ii < sym.entries.size(); ii++) {
        if (sym.entries[ii].stype == SYM_TEMPORARYTYPE)
            sym.entries[ii].stype = 0;
    }

    return 0;
}

void free_pointer(int spOffset, int zeroCmd, int arraySym, ccCompiledScript *scrip) {

    scrip->write_cmd1(SCMD_LOADSPOFFS, spOffset);
    scrip->write_cmd(zeroCmd);

    if ((sym.entries[arraySym].flags & (SFLG_ARRAY | SFLG_DYNAMICARRAY)) == SFLG_ARRAY) {
        // array of pointers -- release each one
        for (int ee = 1; ee < sym.entries[arraySym].arrsize; ee++) {
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, 4);
            scrip->write_cmd(zeroCmd);
        }
    }

}

void free_pointers_from_struct(int structVarSym, ccCompiledScript *scrip) {
    int structType = sym.entries[structVarSym].vartype;

    for (int dd = 0; (size_t)dd < sym.entries.size(); dd++) {
        if ((sym.entries[dd].stype == SYM_STRUCTMEMBER) &&
            (sym.entries[dd].extends == structType) &&
            ((sym.entries[dd].flags & SFLG_IMPORTED) == 0) &&
            ((sym.entries[dd].flags & SFLG_PROPERTY) == 0)) {

                if (sym.entries[dd].flags & SFLG_POINTER) {
                    int spOffs = (scrip->cur_sp - sym.entries[structVarSym].soffs) - sym.entries[dd].soffs;

                    free_pointer(spOffs, SCMD_MEMZEROPTR, dd, scrip);

                    if (sym.entries[structVarSym].flags & SFLG_ARRAY) {
                        // an array of structs, free any pointers in them
                        for (int ii = 1; ii < sym.entries[structVarSym].arrsize; ii++) {
                            spOffs -= sym.entries[structType].ssize;
                            free_pointer(spOffs, SCMD_MEMZEROPTR, dd, scrip);
                        }
                    }
                }
                else {
                    // non-pointer struct, need to procss its members
                    // **** TODO

                }
        }
    }
}

// Removes local variables from tables, and returns number of bytes to
// remove from stack
// just_count: just returns number of bytes, doesn't actually remove any
int remove_locals(int from_level, int just_count, ccCompiledScript *scrip) {
    int cc, totalsub = 0;
    int zeroPtrCmd = SCMD_MEMZEROPTR;
    if (from_level == 0)
        zeroPtrCmd = SCMD_MEMZEROPTRND;

    for (cc=0; (size_t)cc<sym.entries.size();cc++) {
        if ((sym.entries[cc].sscope > from_level) && (sym.entries[cc].stype == SYM_LOCALVAR)) {
            // caller will sort out stack, so ignore parameters
            if ((sym.entries[cc].flags & SFLG_PARAMETER)==0) {
                if (sym.entries[cc].flags & SFLG_DYNAMICARRAY)
                    totalsub += 4;
                else
                {
                    totalsub += sym.entries[cc].ssize;
                    // remove all elements if array
                    if (sym.entries[cc].flags & SFLG_ARRAY)
                        totalsub += (sym.entries[cc].arrsize - 1) * sym.entries[cc].ssize;
                }
                if (sym.entries[cc].flags & SFLG_STRBUFFER)
                    totalsub += STRING_LENGTH;
            }
            // release the pointer reference if applicable
            if (sym.entries[cc].flags & SFLG_THISPTR) { }
            else if (((sym.entries[cc].flags & SFLG_POINTER) != 0) ||
                ((sym.entries[cc].flags & SFLG_DYNAMICARRAY) != 0))
            {
                free_pointer(scrip->cur_sp - sym.entries[cc].soffs, zeroPtrCmd, cc, scrip);
            }
            else if (sym.entries[sym.entries[cc].vartype].flags & SFLG_STRUCTTYPE) {
                // a struct -- free any pointers it contains
                free_pointers_from_struct(cc, scrip);
            }

            if (just_count == 0) {
                sym.entries[cc].stype = 0;
                sym.entries[cc].sscope = 0;
                sym.entries[cc].flags = 0;
            }
        }
    }
    return totalsub;
}

int deal_with_end_of_ifelse (char*nested_type,long*nested_info,long*nested_start,
                             ccCompiledScript*scrip,ccInternalList*targ,int*nestlevel, std::vector<ccChunk> *nested_chunk) {
     int nested_level = nestlevel[0];
     int is_else=0;
     if (nested_type[nested_level] == NEST_ELSESINGLE) ;
     else if (nested_type[nested_level] == NEST_ELSE) ;
     else if (sym.get_type(targ->peeknext()) == SYM_ELSE) {
         targ->getnext();
         scrip->write_cmd1(SCMD_JMP,0);
         is_else=1;
     }
     if (nested_start[nested_level]) {
         scrip->flush_line_numbers();
         // if it's a for loop, drop the yanked chunk (loop increment) back in
         if(nested_chunk[nested_level].size() > 0) {
             write_chunk(scrip, nested_chunk[nested_level][0]);
             clear_chunk_list(&nested_chunk[nested_level]);
         }
         // it's a while loop, so write a jump back to the check again
        scrip->write_cmd1(SCMD_JMP,-((scrip->codesize+2) - nested_start[nested_level]) );
     }
     // write the correct relative jump location
     scrip->code[nested_info[nested_level]] =
         (scrip->codesize - nested_info[nested_level]) - 1;
     if (is_else) {
         // convert the IF into an ELSE
         if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE) {
             nested_type[nested_level] = NEST_ELSE;
             targ->getnext();
         }
         else
             nested_type[nested_level] = NEST_ELSESINGLE;
         nested_info[nested_level] = scrip->codesize-1;
         return 1;
     }
     else
     {
         nestlevel[0]--;
         if(nested_type[nestlevel[0]] == NEST_FOR)
         {
             nestlevel[0]--;
             // find local variables that have just been removed
             int totalsub = remove_locals (nestlevel[0], 0, scrip);

             if (totalsub > 0) {
                 scrip->cur_sp -= totalsub;
                 scrip->write_cmd2(SCMD_SUB,SREG_SP,totalsub);
             }
         }
     }
     return 0;
}

int deal_with_end_of_do (long *nested_info, long *nested_start, ccCompiledScript *scrip, ccInternalList *targ, int *nestlevel) {
    int cursym;
    int nested_level;

    cursym = targ->getnext();
    nested_level = nestlevel[0];
    scrip->flush_line_numbers();
    if (sym.get_type(cursym) != SYM_WHILE) {
        cc_error("Do without while");
        return -1;
    }
    if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS) {
        cc_error("expected '('");
        return -1;
    }
    scrip->flush_line_numbers();
    if (evaluate_expression(targ, scrip, 1, false))
        return -1;
    if (sym.get_type(targ->peeknext()) != SYM_SEMICOLON) {
        cc_error("expected ';'");
        return -1;
    }
    targ->getnext();
    // Jump back to the start of the loop while the condition is true
    scrip->write_cmd1(SCMD_JNZ, -((scrip->codesize + 2) - nested_start[nested_level]));
    // Write the correct location for the end of the loop
    scrip->code[nested_info[nested_level]] = (scrip->codesize - nested_info[nested_level]) - 1;
    nestlevel[0]--;

    return 0;
}

int deal_with_end_of_switch (int32_t *nested_assign_addr, long *nested_start, std::vector<ccChunk> *nested_chunk, ccCompiledScript *scrip, ccInternalList *targ, int *nestlevel, long *nested_info) {
    int index;
    int limit = nested_chunk->size();
    int nested_level = nestlevel[0];
    int skip;
    int operation = is_any_type_of_string(nested_info[nested_level]) ? SCMD_STRINGSNOTEQ : SCMD_NOTEQUAL;
    if(scrip->code[scrip->codesize - 2] != SCMD_JMP || scrip->code[scrip->codesize - 1] != nested_start[nested_level] - scrip->codesize + 2) {
        // If there was no terminating break, write a jump at the end of the last case
        scrip->write_cmd1(SCMD_JMP, 0);
    }
    skip = scrip->codesize;
    // Write the location for the jump to this point (the jump table)
    scrip->code[nested_start[nested_level] + 1] = (scrip->codesize - nested_start[nested_level]) - 2;
    for(index = 0; index < limit; index++) {
        // Put the result of the expression into AX
        write_chunk(scrip, (*nested_chunk)[index]);
        // Do the comparison
        scrip->write_cmd2(operation, SREG_AX, SREG_BX);
        scrip->write_cmd1(SCMD_JZ, (*nested_chunk)[index].codeoffset - scrip->codesize - 2);
    }
    // Write the default jump if necessary
    if(nested_assign_addr[nested_level] != -1)
        scrip->write_cmd1(SCMD_JMP, nested_assign_addr[nested_level] - scrip->codesize - 2);
    // Write the location for the jump to the end of the switch block (for break statements)
    scrip->code[nested_start[nested_level] + 3] = scrip->codesize - nested_start[nested_level] - 4;
    // Write the jump for the end of the switch block
    scrip->code[skip - 1] = scrip->codesize - skip;
    clear_chunk_list(nested_chunk);
    nestlevel[0]--;

    return 0;
}

int find_member_sym(int structSym, long *memSym, int allowProtected) {
    int oriname = *memSym;
    const char *possname = get_member_full_name(structSym, oriname);

    oriname = sym.find(possname);
    if (oriname < 0) {
        if (sym.entries[structSym].extends > 0) {
            // walk the inheritance tree to find the member
            if (find_member_sym(sym.entries[structSym].extends, memSym, allowProtected) == 0)
                return 0;
            // the inherited member was not found, so fall through to
            // the error message
        }
        cc_error("'%s' is not a public member of '%s'. Are you sure you spelt it correctly (remember, capital letters are important)?",sym.get_friendly_name(*memSym).c_str(),sym.get_friendly_name(structSym).c_str());
        return -1;
    }
    if ((!allowProtected) && (sym.entries[oriname].flags & SFLG_PROTECTED)) {
        cc_error("Cannot access protected member '%s'", sym.get_friendly_name(oriname).c_str());
        return -1;
    }
    *memSym = oriname;
    return 0;
}

std::string friendly_int_symbol(int symidx, bool isNegative) {
    if (isNegative) {
        return "-" + sym.get_friendly_name(symidx);
    } else {
        return sym.get_friendly_name(symidx);
    }
}

int accept_literal_or_constant_value(int fromSym, int &theValue, bool isNegative, const char *errorMsg) {
  if (sym.get_type(fromSym) == SYM_LITERALVALUE) {

    // Prepend '-' so we can parse -2147483648
    std::string literalStrValue = std::string(sym.get_name(fromSym));
    if (isNegative) {
        literalStrValue = '-' + literalStrValue;
    }

    errno = 0;
    char *endptr = 0;
    const long longValue = strtol(literalStrValue.c_str(), &endptr, 10);

    if ((longValue == LONG_MIN || longValue == LONG_MAX) && errno == ERANGE) {
        cc_error("Could not parse integer symbol '%s' because of overflow.", friendly_int_symbol(fromSym, isNegative).c_str());
        return -1;
    }
    if (endptr[0] != 0) {
        cc_error("Could not parse integer symbol '%s' because the whole buffer wasn't converted.", friendly_int_symbol(fromSym, isNegative).c_str());
        return -1;
    }
    if (longValue > INT_MAX || longValue < INT_MIN) {
        cc_error("Could not parse integer symbol '%s' because of overflow.", friendly_int_symbol(fromSym, isNegative).c_str());
        return -1;
    }

    theValue = static_cast<int>(longValue);
  }
  else if (sym.get_type(fromSym) == SYM_CONSTANT) {
    theValue = sym.entries[fromSym].soffs;
    if (isNegative) {
        theValue = -theValue;
    }
  }
  else {
    cc_error((char*)errorMsg);
    return -1;
  }
  return 0;
}

int check_not_eof(ccInternalList &targ) {
  if (targ.peeknext() == SCODE_INVALID) {
    // We are past the last symbol in the file
    targ.getnext();
    currentline = targ.lineAtEnd;
    cc_error("Unexpected end of file");
    return -1;
  }
  return 0;
}

// return the float as an int32 (but not actually converted to int)
int float_to_int_raw(float toconv) {
    union
    {
        float   f;
        int32_t i32;
    } conv;
    conv.f = toconv;
    return conv.i32;
}

int check_for_default_value(ccInternalList &targ, int funcsym, int numparams) {
    if (sym.get_type(targ.peeknext()) == SYM_ASSIGN) {
        const char *vartype = sym.get_name(targ.script[targ.pos - 2]);
        // parameter has default value
        targ.getnext();
        int defValSym = targ.getnext();
        bool negateIt = false;
        bool isFloat = false;
        bool isPointer = strchr(vartype, '*') != NULL;

        if (sym.get_name(defValSym)[0] == '-') {
            // allow negative default value
            negateIt = true;
            defValSym = targ.getnext();
        }

        int defaultValue;
        if (strchr(sym.get_name(defValSym), '.') != NULL) {
            float fval = atof(sym.get_name(defValSym));
            isFloat = true;
            if (negateIt)
                fval = -fval;
            defaultValue = float_to_int_raw(fval);
        }
        // extract the default value
        else if (accept_literal_or_constant_value(defValSym, defaultValue, negateIt, "Parameter default value must be literal") < 0) {
            return -1;
        }

        // check validity of value type to parameter type
        if (defaultValue != 0) {
            // 0 as value or pointer is always valid, so we only check the other cases

            if (isPointer) {
                cc_error("Parameter default pointer value can only be 0");
                return -1;
            }
            if (isFloat ^ (strcmp(vartype, "float") == 0)) {
                // float values allowed only for type float
                cc_error("Parameter default value must be %s or 0", vartype);
                return -1;
            }
            if (strcmp(vartype, "String") == 0) {
                // prevent non-zero integers for type String
                cc_error("Parameter default value for type String must be 0");
                return -1;
            }
        }

        sym.entries[funcsym].funcParamDefaultValues[numparams % 100] = defaultValue;
        sym.entries[funcsym].funcParamHasDefaultValues[numparams % 100] = true;

    }

    return 0;
}

int check_for_dynamic_array_declaration(ccInternalList &targ, int typeSym, bool isPointer)
{
  if (sym.get_type(targ.peeknext()) == SYM_OPENBRACKET)
  {
    // dynamic array
    targ.getnext();
    if (sym.get_type(targ.getnext()) != SYM_CLOSEBRACKET)
    {
      cc_error("fixed size array cannot be used in this way");
      return -1;
    }
    if (sym.entries[typeSym].flags & SFLG_STRUCTTYPE) {
        if (!(sym.entries[typeSym].flags & SFLG_MANAGED)) {
            cc_error("cannot pass non-managed struct array");
            return -1;
        }
        if (!isPointer) {
            cc_error("cannot pass non-pointer struct array");
            return -1;
        }
    }
    return 1;
  }
  return 0;
}


int process_function_declaration(ccInternalList &targ, ccCompiledScript*scrip,
                                 int *funcsymptr, int vtwas, int &in_func,
                                 int &nested_level, int next_is_readonly,
                                 int next_is_import, int isMemberFunction,
                                 int returnsPointer, int func_is_static,
								 int *isMemberFunctionPtr, SymbolDef *oldDefinition,
                 int returnsDynArray) {
  int numparams = 1;
  int funcsym = *funcsymptr;
  int varsize = sym.entries[vtwas].ssize;
  // skip the opening (
  targ.getnext();

  char functionName[MAX_SYM_LEN];
  strcpy(functionName, sym.get_name(funcsym));

  if (check_not_eof(targ))
    return -1;
  if (strcmp(sym.get_name(targ.peeknext()), "this") == 0 || sym.get_type(targ.peeknext()) == SYM_STATIC)
  {
    if(sym.get_type(targ.peeknext()) == SYM_STATIC)
      func_is_static = 1;
    // extender function, eg.  function GoAway(this Character *someone)
    targ.getnext();
	  if (sym.get_type(targ.peeknext()) != SYM_VARTYPE)
	  {
	    if(func_is_static)
	      cc_error("'static' must be followed by a struct name");
	    else
	      cc_error("'this' must be followed by a struct name");
	    return -1;
	  }
	  if ((sym.entries[targ.peeknext()].flags & SFLG_STRUCTTYPE) == 0)
	  {
	    if(func_is_static)
	      cc_error("'static' cannot be used with primitive types");
	    else
	      cc_error("'this' cannot be used with primitive types");
	    return -1;
	  }
	  if (strchr(functionName, ':') != NULL)
	  {
	    cc_error("extender functions cannot be part of a struct");
	    return -1;
	  }

	  sprintf(functionName, "%s::%s", sym.get_name(targ.peeknext()), sym.get_name(funcsym));
	  if (isMemberFunctionPtr != NULL)
	  {
		  *isMemberFunctionPtr = targ.peeknext();
	  }

	  funcsym = sym.find(functionName);
	  if (funcsym < 0)
	  {
		  funcsym = sym.add(functionName);
	  }
    *funcsymptr = funcsym;

	  if (next_is_import == 0) {
      if (scrip->remove_any_import(functionName, oldDefinition))
        return -1;
    }

	  if (sym.entries[funcsym].stype != 0)
	  {
	    cc_error("function '%s' is already defined", functionName);
	    return -1;
	  }
	  sym.entries[funcsym].flags = SFLG_STRUCTMEMBER;
	  if(func_is_static) {
	    sym.entries[funcsym].flags |= SFLG_STATIC;
      }

	  targ.getnext();
	  if (!func_is_static && strcmp(sym.get_name(targ.getnext()), "*") != 0)
	  {
	    cc_error("instance extender function must be pointer");
	    return -1;
	  }

	  if ((sym.get_type(targ.peeknext()) != SYM_COMMA) &&
	      (sym.get_type(targ.peeknext()) != SYM_CLOSEPARENTHESIS))
	  {
	    if(strcmp(sym.get_name(targ.getnext()), "*") == 0)
	      cc_error("static extender function cannot be pointer");
	    else
	      cc_error("parameter name cannot be defined for extender type");
	    return -1;
	  }

	  if (sym.get_type(targ.peeknext()) == SYM_COMMA)
	  {
		  targ.getnext();
	  }
  }
  else if(sym.get_type(funcsym) == SYM_VARTYPE) {
      // Attempt to prefix a vartype with the struct name
      // e.g. MyStruct::Object
      if(isMemberFunction)
      {
          sprintf(functionName, "%s::%s", sym.get_name(isMemberFunction), sym.get_name(funcsym));
          funcsym = sym.find(functionName);
          if (funcsym < 0)
              funcsym = sym.add(functionName);
          *funcsymptr = funcsym;
      }
      if (sym.entries[funcsym].stype != 0) 
      {
          cc_error("function '%s' is already defined", functionName);
          return -1;
      }
  }

  sym.entries[funcsym].stype = SYM_FUNCTION;
  sym.entries[funcsym].ssize = varsize;  // save return type size
  sym.entries[funcsym].funcparamtypes[0] = vtwas;  // return type

  if (returnsPointer)
  {
    sym.entries[funcsym].funcparamtypes[0] |= STYPE_POINTER;
  }
  if (returnsDynArray)
  {
    sym.entries[funcsym].funcparamtypes[0] |= STYPE_DYNARRAY;
  }

  if ((!returnsPointer) && (!returnsDynArray) &&
      ((sym.entries[vtwas].flags & SFLG_STRUCTTYPE) != 0))
  {
    cc_error("Cannot return entire struct from function");
    return -1;
  }
  if ((in_func >= 0) || (nested_level > 0)) {
    cc_error("Nested functions not supported (you may have forgotten a closing brace)");
    return -1;
  }
  if (next_is_readonly) {
    cc_error("readonly cannot be applied to a function");
    return -1;
  }

  int funcNum = -1;

  if (next_is_import)
    in_func = scrip->add_new_import(functionName);
  else
    in_func = scrip->add_new_function(functionName, &funcNum);

  if (in_func < 0) {
    // don't overwrite the "used import" error message
    if (in_func != -2)
      cc_error("Internal compiler error: table overflow");
    return -1;
  }
  sym.entries[funcsym].soffs = in_func;  // save code offset of function
  scrip->cur_sp += 4;  // the return address will be pushed

  int prototype = 0;
  bool next_is_const = false;
  char functype[100]="\0";
  while (1) {
    int cursym = targ.getnext();
    int next_type = sym.get_type(cursym);
    if (next_type == SYM_CLOSEPARENTHESIS) break;
    else if (next_type == SYM_VARARGS) {
      // variable number of arguments
      numparams+=100;
      cursym = targ.getnext();
      if (sym.get_type(cursym) != SYM_CLOSEPARENTHESIS) {
        cc_error("expected ')' after variable-args");
        return -1;
      }
      break;
    }
    else if (next_type == SYM_CONST) {
      next_is_const = true;
    }
    else if (next_type == SYM_VARTYPE) {
      // function parameter
      if ((numparams % 100) >= MAX_FUNCTION_PARAMETERS) {
        cc_error("too many parameters defined for function");
        return -1;
      }
      if (cursym == sym.normalVoidSym) {
        cc_error("'void' invalid type for function parameter");
        return -1;
      }
      int isPointerParam = 0;
      // save the parameter type (numparams starts from 1)
      sym.entries[funcsym].funcparamtypes[numparams % 100] = cursym;
      sym.entries[funcsym].funcParamDefaultValues[numparams % 100] = 0;
      sym.entries[funcsym].funcParamHasDefaultValues[numparams % 100] = false;

      if (next_is_const)
        sym.entries[funcsym].funcparamtypes[numparams % 100] |= STYPE_CONST;

      functype[strlen(functype)+1] = 0;
      functype[strlen(functype)] = (char)cursym;  // save variable type
      if (check_not_eof(targ))
        return -1;
      if (strcmp(sym.get_name(targ.peeknext()), "*") == 0) {
        // pointer
        sym.entries[funcsym].funcparamtypes[numparams % 100] |= STYPE_POINTER;
        isPointerParam = 1;
        targ.getnext();
        if ((sym.entries[cursym].flags & SFLG_MANAGED) == 0) {
          // can only point to managed structs
          cc_error("Cannot declare pointer to non-managed type");
          return -1;
        }
        if (sym.entries[cursym].flags & SFLG_AUTOPTR) {
          cc_error("Invalid use of pointer");
          return -1;
        }
      }

      if (sym.entries[cursym].flags & SFLG_AUTOPTR) {
        sym.entries[funcsym].funcparamtypes[numparams % 100] |= STYPE_POINTER;
        isPointerParam = 1;
      }

      bool createdLocalVar = false;

      if (sym.get_type(targ.peeknext()) != 0) {
        // next token is not a variable name, so it must be a prototype
        prototype = 1;

        if ((sym.get_type(targ.peeknext()) == SYM_GLOBALVAR) && (next_is_import)) {
          // if the paramter name is a global var, but this is just an import declaration
          // then ignore it
          targ.getnext();
        }

        if (check_for_default_value(targ, funcsym, numparams))
          return -1;

        numparams++;
      }
      else if ((next_is_import) || (prototype)) {
        // imported function, so ignore the parameter names
        targ.getnext();

        if (check_for_default_value(targ, funcsym, numparams))
          return -1;

        numparams++;
      }
      else {
        // it's a parameter
        int vartypesym = cursym;
        if ((sym.entries[cursym].flags & SFLG_STRUCTTYPE) && (!isPointerParam)) {
          cc_error("struct cannot be passed as parameter");
          return -1;
        }
        cursym = targ.getnext();
        sym.entries[cursym].stype = SYM_LOCALVAR;
        sym.entries[cursym].extends = 0;
        sym.entries[cursym].arrsize = 1;
        sym.entries[cursym].vartype = vartypesym;
        sym.entries[cursym].ssize = 4; //oldsize;  fix param to 4 bytes for djgpp
        sym.entries[cursym].sscope = nested_level + 1;
        sym.entries[cursym].flags |= SFLG_PARAMETER;
        if (isPointerParam)
          sym.entries[cursym].flags |= SFLG_POINTER;
        if (next_is_const)
          sym.entries[cursym].flags |= SFLG_CONST | SFLG_READONLY;
        // the parameters are pushed backwards, so the top of the
        // stack has the first parameter. The +1 is because the
        // call will push the return address onto the stack as well
        sym.entries[cursym].soffs = scrip->cur_sp - (numparams+1)*4;
        createdLocalVar = true;
        numparams++;
/*              scrip->cur_sp += oldsize;
        scrip->write_cmd2(SCMD_ADD,SREG_SP,oldsize);*/
      }

      int dynArrayStatus = check_for_dynamic_array_declaration(targ, cursym, !!isPointerParam);
      if (dynArrayStatus < 0)
      {
        return -1;
      }
      else if (dynArrayStatus > 0)
      {
        sym.entries[funcsym].funcparamtypes[(numparams - 1) % 100] |= STYPE_DYNARRAY;
        if (createdLocalVar)
        {
          sym.entries[cursym].flags |= SFLG_DYNAMICARRAY | SFLG_ARRAY;
        }
      }

      next_type = sym.get_type(cursym=targ.getnext());
      if (next_type == SYM_CLOSEPARENTHESIS) break;
      else if (next_type == SYM_GLOBALVAR) {
        cc_error("'%s' is a global var; cannot use as name for local",sym.get_friendly_name(cursym).c_str());
        return -1;
      }
      else if (next_type != SYM_COMMA) {
        cc_error("PE02: Parse error at '%s'",sym.get_friendly_name(cursym).c_str());
        return -1;
      }

      next_is_const = false;
    }
    else {
      // something odd was inside the parentheses
      cc_error("PE03: Parse error at '%s'",sym.get_friendly_name(cursym).c_str());
      return -1;
    }
  }
  // save the number of parameters
  sym.entries[funcsym].sscope = (numparams-1);
  if (funcNum >= 0)
    scrip->funcnumparams[funcNum] = sym.entries[funcsym].sscope;

  if (func_is_static)
    sym.entries[funcsym].flags |= SFLG_STATIC;

  if (next_is_import) {
    sym.entries[funcsym].flags |= SFLG_IMPORTED;

    if (isMemberFunction) {
      // for imported member functions, append the number of parameters
      // to the name of the import
      char appendage[10];
      sprintf(appendage, "^%d", sym.entries[funcsym].sscope);

      strcat(scrip->imports[in_func], appendage);
    }

    int nextvar = targ.peeknext();
    // member function expects the ; to still be there whereas
    // normal function does not
    if (!isMemberFunction)
      nextvar = targ.getnext();

    if (sym.get_type(nextvar) != SYM_SEMICOLON) {
      cc_error("';' expected (cannot define body of imported function)");
      return -1;
    }
    in_func=-1;
  }
  else if (sym.get_type(targ.peeknext()) == SYM_OPENBRACE) {
  }
  else {
    cc_error("Expected '{'");
    return -1;
  }

  return 0;
}

int isPartOfExpression(ccInternalList *targ, int j) {
  if (sym.get_type(targ->script[j]) == SYM_NEW)
    return 1;
  if (sym.get_type(targ->script[j]) < NOTEXPRESSION)
    return 1;
  // static member access
  if ((j < targ->length - 1) &&
      (sym.get_type(targ->script[j + 1]) == SYM_DOT) &&
      (sym.get_type(targ->script[j]) == SYM_VARTYPE))
    return 1;
  return 0;
}

// return the index of the lowest priority operator in the list,
// so that either side of it can be evaluated first.
// returns -1 if no operator was found
int find_lowest_bonding_operator(long*slist,int listlen) {
  int k,blevel=0,plevel=0;
  int lowestis = 0,lowestat = -1;
  for (k=0;k<listlen;k++) {
    int thisType = sym.get_type(slist[k]);
    if (thisType == SYM_OPENBRACKET)
      blevel++;
    else if (thisType == SYM_CLOSEBRACKET)
      blevel--;
    else if (thisType == SYM_OPENPARENTHESIS)
      plevel++;
    else if (thisType == SYM_CLOSEPARENTHESIS)
      plevel--;

    if (((thisType == SYM_OPERATOR) || (thisType == SYM_NEW)) &&
        (plevel == 0) && (blevel == 0)) {
      // .ssize stores the precedence
      int thisIsTheOperator = 0;
      if (ccGetOption(SCOPT_LEFTTORIGHT)) {
        // left-to-right; find the right-most operator, then
        // they will be recursively processed left
        if (sym.entries[slist[k]].ssize >= lowestis)
          thisIsTheOperator = 1;
      }
      else {
        // right-to-left; find the left-most operator, then
        // they will be recursively processed right
        if (sym.entries[slist[k]].ssize > lowestis)
          thisIsTheOperator = 1;
      }
      if (thisIsTheOperator) {
        lowestis = sym.entries[slist[k]].ssize;
        lowestat = k;
      }
    }
  }
  return lowestat;
}

int is_any_type_of_string(int symtype) {
    symtype &= ~(STYPE_CONST | STYPE_POINTER);
    if ((symtype == sym.normalStringSym) || (symtype == sym.stringStructSym))
        return 1;
    return 0;
}

int is_string(int valtype) {

  if (strcmp(sym.get_name(valtype),"const string")==0)
    return 1;
  if (strcmp(sym.get_name(valtype),"string")==0)
    return 1;
  if (strcmp(sym.get_name(valtype),"char*")==0)
    return 1;

  return 0;
}

int check_operator_valid_for_type(int *vcpuOpPtr, int type1, int type2) {
  int NULL_TYPE = STYPE_POINTER | sym.nullSym;
  int vcpuOp = *vcpuOpPtr;

  int isError = 0;
  if ((type1 == sym.normalFloatSym) || (type2 == sym.normalFloatSym)) {
    // some operators not valid on floats
    int changeOpTo = vcpuOp;
    switch (vcpuOp) {
    case SCMD_ISEQUAL: // == and != same as integer versions
    case SCMD_NOTEQUAL:
      break;
    case SCMD_ADD:
      changeOpTo = SCMD_FADD;
      break;
    case SCMD_SUB:
      changeOpTo = SCMD_FSUB;
      break;
    case SCMD_ADDREG:
      changeOpTo = SCMD_FADDREG;
      break;
    case SCMD_SUBREG:
      changeOpTo = SCMD_FSUBREG;
      break;
    case SCMD_MULREG:
      changeOpTo = SCMD_FMULREG;
      break;
    case SCMD_DIVREG:
      changeOpTo = SCMD_FDIVREG;
      break;
    case SCMD_GREATER:
      changeOpTo = SCMD_FGREATER;
      break;
    case SCMD_LESSTHAN:
      changeOpTo = SCMD_FLESSTHAN;
      break;
    case SCMD_GTE:
      changeOpTo = SCMD_FGTE;
      break;
    case SCMD_LTE:
      changeOpTo = SCMD_FLTE;
      break;
    default:
      isError = 1;
    }
    vcpuOp = changeOpTo;
    *vcpuOpPtr = changeOpTo;
  }

  if (is_any_type_of_string(type1) && is_any_type_of_string(type2)) {
    if (vcpuOp == SCMD_ISEQUAL) {
      *vcpuOpPtr = SCMD_STRINGSEQUAL;
      return 0;
    }
    else if (vcpuOp == SCMD_NOTEQUAL) {
      *vcpuOpPtr = SCMD_STRINGSNOTEQ;
      return 0;
    }
  }

  if ((type1 & STYPE_POINTER) || (is_string(type1))) {
    isError = 1;
  }
  if (type2) {
    if ((type2 & STYPE_POINTER) || (is_string(type2))) {
      isError = 1;
    }
    if ((vcpuOp == SCMD_ISEQUAL) || (vcpuOp == SCMD_NOTEQUAL)) {
      // pointers can be compared to each other
      // (except strings)
      if (!is_string(type1))
        isError = 0;
    }
  }

  if (isError) {
    cc_error("Operator cannot be applied to this type");
    return -1;
  }
  return 0;
}

int check_type_mismatch(int typeIs, int typeWantsToBe, int orderMatters) {
  int isTypeMismatch = 0;
  int numstrings = 0;

  int typeIsOriginally = typeIs;
  int typeWantsToBeOriginally = typeWantsToBe;

  if (is_string(typeIs))
    numstrings++;
  if (is_string(typeWantsToBe))
    numstrings++;
  if (numstrings == 1) {
    isTypeMismatch = 1;
  }

  // can convert String* to const string
  if ((typeIs == (STYPE_POINTER | sym.stringStructSym)) &&
      (typeWantsToBe == (STYPE_CONST | sym.normalStringSym)))
    return 0;

  // cannot convert 'void' to anything
  if (typeIs == sym.normalVoidSym)
    isTypeMismatch = 1;

  // cannot convert const to non-const
  if (((typeIs & STYPE_CONST) != 0) && ((typeWantsToBe & STYPE_CONST) == 0))
    isTypeMismatch = 1;

  // cannot convert from/to dynamic array
  if ((typeIs == (STYPE_POINTER | sym.nullSym)) &&
      ((typeWantsToBe & STYPE_DYNARRAY) != 0))
  {
    // null is always allowed
    return 0;
  }
  else if ((typeIs & STYPE_DYNARRAY) != (typeWantsToBe & STYPE_DYNARRAY))
    isTypeMismatch = 1;

  // allow safe comparisons from here on
  typeIs &= ~(STYPE_CONST | STYPE_DYNARRAY);
  typeWantsToBe &= ~(STYPE_CONST | STYPE_DYNARRAY);

  // floats cannot mingle with other types
  if ((typeIs == sym.normalFloatSym) && (typeWantsToBe != sym.normalFloatSym))
    isTypeMismatch = 1;
  if ((typeWantsToBe == sym.normalFloatSym) && (typeIs != sym.normalFloatSym))
    isTypeMismatch = 1;

  // a pointer can only be compared with another pointer (except for
  // the   string/char*  exception)
  if (numstrings == 2) { }
  else if ((typeIs & STYPE_POINTER) || (typeWantsToBe & STYPE_POINTER)) {
    // pointers must point to same type
    int pointerIsOk = 0;
    if (typeIs == (STYPE_POINTER | sym.nullSym)) {
      // null can be cast to any pointer type
      if (typeWantsToBe & STYPE_POINTER)
        pointerIsOk = 1;
    }
    // check against inherited classes
    int tryTypeIs = typeIs & ~STYPE_POINTER;
    while (sym.entries[tryTypeIs].extends > 0) {
      tryTypeIs = sym.entries[tryTypeIs].extends;
      if ((tryTypeIs | STYPE_POINTER) == typeWantsToBe) {
        pointerIsOk = 1;
        break;
      }
    }
    // both sides must be pointers
    if ((typeIs & STYPE_POINTER) != (typeWantsToBe & STYPE_POINTER))
      isTypeMismatch = 1;

    if ((pointerIsOk == 0) && (typeIs != typeWantsToBe)) {
      isTypeMismatch = 1;
    }
  }
  else if ((sym.entries[typeIs].flags & SFLG_STRUCTTYPE) ||
           (sym.entries[typeWantsToBe].flags & SFLG_STRUCTTYPE)) {
    if (typeIs != typeWantsToBe) {
      isTypeMismatch = 1;
    }
  }

  if (isTypeMismatch) {
    if (!orderMatters) {
      // if not an assignment so left and right are interchangable,
      // try the other way round
      if (check_type_mismatch(typeWantsToBeOriginally, typeIsOriginally, 1))
        return -1;
    }
    else {
      cc_error("Type mismatch: cannot convert '%s' to '%s'", sym.get_friendly_name(typeIsOriginally).c_str(), sym.get_friendly_name(typeWantsToBeOriginally).c_str());
      return -1;
    }
  }
  return 0;
}

int isVCPUOperatorBoolean(int scmdtype) {
    // returns whether this operator's val type is always bool
    if ((scmdtype >= SCMD_ISEQUAL) &&
        (scmdtype <= SCMD_OR))
        return 1;

    if ((scmdtype >= SCMD_FGREATER) &&
        (scmdtype <= SCMD_FLTE))
        return 1;

    if ((scmdtype == SCMD_STRINGSNOTEQ) ||
        (scmdtype == SCMD_STRINGSEQUAL))
        return 1;

    return 0;
}

long extract_variable_name(int fsym, ccInternalList*targ,long*slist, int *funcAtOffs) {
  *funcAtOffs = -1;

  int mustBeStaticMember = 0;

  if (!sym.entries[fsym].is_loadable_variable()) {

    // allow struct type as first word, but then a static member must be used
    if (sym.get_type(fsym) == SYM_VARTYPE)
      mustBeStaticMember = 1;
    else
      return 0;
  }

  int oriOffs = targ->pos;
  long sslen=1;
  slist[0]=fsym;
  if (targ->peeknext() == SCODE_INVALID) return sslen;
  // it works like this:
  // MyStruct ms; MouseType m; int x;
  // ms.m.x
  // MyStruct::m
  // MouseType::x

  int justHadBrackets = 0;
  int nexttype = sym.get_type(targ->peeknext());
  while ((nexttype == SYM_DOT) || (nexttype == SYM_OPENBRACKET)) {
    // store the . or [
    slist[sslen] = targ->getnext();
    sslen++;
    // get the next one
    slist[sslen] = targ->peeknext();

    if (slist[sslen] == SCODE_INVALID) {
      // this happens if they do:
      // player.Walk(oKey.-4666);
      cc_error("dot operator must be followed by member function or property");
      return -1;
    }

    if (sslen >= TEMP_SYMLIST_LENGTH - 5)
    {
      cc_error("buffer exceeded: you probably have a missing closing bracket on a previous line");
      return -1;
    }

    if (nexttype == SYM_DOT) {
      int reallywant;
      if (sym.get_type(fsym) == SYM_VARTYPE) {
        // static member access, eg. "Math.Func()"
        mustBeStaticMember = 1;
        reallywant = fsym;
      }
      else {
        mustBeStaticMember = 0;
        reallywant = sym.entries[fsym].vartype;
        if (reallywant < 1) {
          cc_error("structure required on left side of '.'");
          return -1;
        }
      }

      if (((sym.entries[fsym].flags & SFLG_ARRAY) != 0) && (justHadBrackets == 0)) {
        cc_error("'[' expected");
        return -1;
      }
      justHadBrackets = 0;

      // allow protected member access with the "this" ptr only
      int allowProtectedMembers = 0;
      if (sym.entries[fsym].flags & SFLG_THISPTR) {
        allowProtectedMembers = 1;
      }
      // convert the member's sym to the structmember version
      if (find_member_sym(reallywant, &slist[sslen], allowProtectedMembers))
        return -1;
      if ((sym.entries[slist[sslen]].flags & SFLG_STRUCTMEMBER) == 0) {
        cc_error("structure member required after '.'");
        return -1;
      }
      if ((mustBeStaticMember) && ((sym.entries[slist[sslen]].flags & SFLG_STATIC) == 0)) {
        cc_error("must have an instance of the struct to access a non-static member");
        return -1;
      }
      fsym = slist[sslen];
      sslen++;
      targ->getnext();

      if (sym.get_type(slist[sslen - 1]) == SYM_FUNCTION) {
        *funcAtOffs = sslen - 1;

        slist[sslen++] = targ->getnext();

        if (sym.get_type(slist[sslen - 1]) != SYM_OPENPARENTHESIS) {
          cc_error("'(' expected");
          return -1;
        }

        // include the member function params in the returned value
        int bdepth = 1;
        while (bdepth > 0) {
          if (check_not_eof(*targ))
            return -1;
          slist[sslen] = targ->getnext();
          if (sslen >= TEMP_SYMLIST_LENGTH - 1)
          {
            cc_error("buffer exceeded: you probably have a missing closing bracket on a previous line");
            return -1;
          }
          sslen++;
          if (sym.get_type(slist[sslen - 1]) == SYM_CLOSEPARENTHESIS)
            bdepth--;
          else if (sym.get_type(slist[sslen - 1]) == SYM_OPENPARENTHESIS)
            bdepth++;
        }

      }
      // save this member for use in a sub-member
    }
    else if (nexttype == SYM_OPENBRACKET) {
      if ((sym.get_type(slist[sslen]) >= NOTEXPRESSION) &&
          ((sym.get_type(slist[sslen]) != SYM_VARTYPE) || ((sym.entries[slist[sslen]].flags & SFLG_STRUCTTYPE) == 0))) {
        cc_error("parse error after '['");
        return -1;
        }
      if (sym.get_type(slist[sslen]) == SYM_CLOSEBRACKET) {
        cc_error("array index not specified");
        return -1;
        }
      if ((sym.entries[slist[sslen-2]].flags & SFLG_ARRAY)==0) {
        cc_error("%s is not an array",sym.get_friendly_name(slist[sslen-2]).c_str());
        return -1;
        }
      int braclevel = 0, linenumWas = currentline;
      // extract the contents of the brackets
      // comma is allowed because you can have like array[func(a,b)]
      // vartype is allowed to permit access to static members, e.g. array[Game.GetColorFromRGB(0, 0, 0)]
      while ((sym.get_type(slist[sslen]) < NOTEXPRESSION) ||
             (sym.get_type(slist[sslen]) == SYM_COMMA) ||
             (sym.get_type(slist[sslen]) == SYM_VARTYPE) && (sym.entries[slist[sslen]].flags & SFLG_STRUCTTYPE)) {
        if (sym.get_type(slist[sslen - 1]) == SYM_VARTYPE && sym.get_type(slist[sslen]) != SYM_DOT)
          break;
        if (targ->getnext() == SCODE_INVALID) {
          currentline = linenumWas;
          cc_error("missing ']'");
          return -1;
        }
        if (sym.get_type(slist[sslen]) == SYM_CLOSEBRACKET) {
          braclevel--;
          if (braclevel < 0) {
            sslen++;
            break;
            }
          }
        if (sym.get_type(slist[sslen]) == SYM_OPENBRACKET)
          braclevel++;
        sslen++;
        if (sslen >= TEMP_SYMLIST_LENGTH - 1)
        {
          cc_error("buffer exceeded: you probably have a missing closing bracket on a previous line");
          return -1;
        }
        slist[sslen] = targ->peeknext();
      }
      justHadBrackets = 1;
    }
    nexttype = sym.get_type(targ->peeknext());
  }
  return sslen;
}

void DoNullCheckOnStringInAXIfNecessary(ccCompiledScript *scrip, int valTypeFrom, int valTypeTo) {

  // Convert normal literal string into String object
  if (((valTypeFrom & (~STYPE_POINTER)) == sym.stringStructSym) &&
     ((valTypeTo & (~STYPE_CONST)) == sym.normalStringSym)) {

    scrip->write_cmd1(SCMD_CHECKNULLREG, SREG_AX);
  }

}

void PerformStringConversionInAX(ccCompiledScript *scrip, int *valTypeFrom, int valTypeTo) {

  // Convert normal literal string into String object
  if (((*valTypeFrom & (~STYPE_CONST)) == sym.normalStringSym) &&
     ((valTypeTo & (~STYPE_POINTER)) == sym.stringStructSym)) {

    scrip->write_cmd1(SCMD_CREATESTRING, SREG_AX);
    *valTypeFrom = STYPE_POINTER | sym.stringStructSym;
  }

}

void set_ax_scope(ccCompiledScript *scrip, int syoffs) {
  // "null" is a global var
  if (sym.get_type(syoffs) == SYM_NULL)
    scrip->ax_val_scope = SYM_GLOBALVAR;
  // if it's a parameter, pretend it's a global var
  // this allows it to be returned back from the function
  else if (sym.entries[syoffs].flags & SFLG_PARAMETER)
    scrip->ax_val_scope = SYM_GLOBALVAR;
  else
    scrip->ax_val_scope = sym.entries[syoffs].stype;
}

int findClosingBracketOffs(int openBracketOffs, long *symlist, int slilen) {
  int endof,braclevel=0;
  for (endof = openBracketOffs + 1; endof < slilen; endof++) {
    int symtype = sym.get_type(symlist[endof]);
    if ((symtype == SYM_OPENBRACKET) || (symtype == SYM_OPENPARENTHESIS))
      braclevel++;
    if ((symtype == SYM_CLOSEBRACKET) || (symtype == SYM_CLOSEPARENTHESIS)) {
      braclevel--;
      if (braclevel < 0) break;
    }
  }
  return endof;
}

int findOpeningBracketOffs(int closeBracketOffs, long *symlist) {
  int endof,braclevel=0;
  for (endof = closeBracketOffs - 1; endof >= 0; endof--) {
    int symtype = sym.get_type(symlist[endof]);
    if ((symtype == SYM_OPENBRACKET) || (symtype == SYM_OPENPARENTHESIS)) {
      braclevel--;
      if (braclevel < 0) break;
    }
    if ((symtype == SYM_CLOSEBRACKET) || (symtype == SYM_CLOSEPARENTHESIS))
      braclevel++;
  }
  return endof;
}

int extractPathIntoParts(VariableSymlist *variablePath, int slilen, long *syml) {
  int variablePathSize = 0;
  int lastOffs = 0;
  int pp;

  // Separate out syml into a VariablePath for the clause
  // between each dot. If it's just a simple variable access,
  // we will only create one.
  for (pp = 0; pp < slilen; pp++) {
    if ((sym.get_type(syml[pp]) == SYM_OPENBRACKET) ||
        (sym.get_type(syml[pp]) == SYM_OPENPARENTHESIS)) {
      // an array index, skip it
      pp = findClosingBracketOffs(pp, syml, slilen);
    }

    int createPath = 0;

    if (sym.get_type(syml[pp]) == SYM_DOT) {
      createPath = 1;
    }
    else if (pp >= slilen - 1) {
      // end of data stream, store the last bit
      pp++;
      createPath = 1;
    }

    if (createPath) {
      if (variablePathSize >= MAX_VARIABLE_PATH) {
        cc_error("variable path too long");
        return -1;
      }
      VariableSymlist *vpp = &variablePath[variablePathSize];
      vpp->init(pp - lastOffs);
      for (int ee = 0; ee < vpp->len; ee++) {
        vpp->syml[ee] = syml[lastOffs + ee];
      }
      lastOffs = pp + 1;
      variablePathSize++;
    }
  }

  return variablePathSize;
}

int readcmd_lastcalledwith=0;
int get_readcmd_for_size(int sizz, int writeinstead) {
  int readcmd = SCMD_MEMREAD;
  if (writeinstead) {
    readcmd = SCMD_MEMWRITE;
    if (sizz == 1)
      readcmd = SCMD_MEMWRITEB;
    else if (sizz == 2)
      readcmd = SCMD_MEMWRITEW;
    }
  else if (sizz == 1)
    readcmd = SCMD_MEMREADB;
  else if (sizz == 2)
    readcmd = SCMD_MEMREADW;

  if (sizz!=0)
    readcmd_lastcalledwith = sizz;
  return readcmd;
  }


int get_array_index_into_ax(ccCompiledScript *scrip, long *symlist, int openBracketOffs, int closeBracketOffs, bool checkBounds, bool multiplySize) {

  // "push" the ax val type (because this is just an array index,
  // we're actually interested in the type of the variable being read)
  int axValTypeWas = scrip->ax_val_type;

  // save the size of the array element, so it doesn't get
  // overwritten by the size of the array index variable
  int saveOldReadcmd = readcmd_lastcalledwith;
  // parse expression inside brackets to return the array index in AX
  if (parse_sub_expr(&symlist[openBracketOffs + 1], closeBracketOffs - (openBracketOffs + 1), scrip))
    return -1;
  readcmd_lastcalledwith = saveOldReadcmd;

  // array index must be an int
  if (check_type_mismatch(scrip->ax_val_type, sym.normalIntSym, 1))
    return -1;

  // "pop" the ax val type
  scrip->ax_val_type = axValTypeWas;

  int arrSym = symlist[openBracketOffs - 1];

  if ((sym.entries[arrSym].flags & SFLG_ARRAY) == 0) {
    cc_error("Internal error: not an array: '%s'", sym.get_friendly_name(arrSym).c_str());
    return -1;
  }

  if (checkBounds) {
    // check the array bounds that have been calculated in AX,
    // before they are added to the overall offset
    if ((sym.entries[arrSym].flags & SFLG_DYNAMICARRAY) == 0)
    {
      scrip->write_cmd2(SCMD_CHECKBOUNDS, SREG_AX, sym.entries[arrSym].arrsize);
    }
  }

  if (multiplySize) {
    // multiply up array index (in AX) by size of array element
    // to get memory offset
    scrip->write_cmd2(SCMD_MUL, SREG_AX, sym.entries[arrSym].ssize);
  }

  return 0;
}

int parseArrayIndexOffsets(ccCompiledScript *scrip, VariableSymlist *thisClause, bool writingOperation, bool *isArrayOffset) {

  if ((thisClause->len > 1) &&
      (sym.get_type(thisClause->syml[1]) == SYM_OPENBRACKET)) {
    // An array
    // find where the brackets end
    int arrIndexEnd = findClosingBracketOffs(1, thisClause->syml, thisClause->len);
    if (arrIndexEnd != thisClause->len - 1) {
      cc_error("Error parsing path; unexpected token after array index");
      return -1;
    }

    bool propertyIndexer = false;
    bool checkBounds = true, multiplySize = true;

    if ((sym.entries[thisClause->syml[0]].flags & SFLG_PROPERTY) ||
        (sym.entries[thisClause->syml[0]].flags & SFLG_POINTER)) {
      // an array property, or array of pointers; in this case,
      // don't touch CX but just calculate the index value into DX
      propertyIndexer = true;
      multiplySize = false;
      // don't check bounds, the property getter will do that
      if (sym.entries[thisClause->syml[0]].flags & SFLG_PROPERTY)
        checkBounds = false;
    }

    // the value to write is in AX; preserve it
    if (writingOperation)
      scrip->push_reg(SREG_AX);

    // save the current offset in CX if there is one,
    // because parse_sub_expr might destroy it
    if (*isArrayOffset)
      scrip->push_reg(SREG_CX);

    // get the byte offset of the array index into AX
    if (get_array_index_into_ax(scrip, thisClause->syml, 1, arrIndexEnd, checkBounds, multiplySize))
      return -1;

    // if there is a current offset saved in CX, restore it
    // then add the result to CX (which is counting the overall offset)
    if (*isArrayOffset) {
      scrip->pop_reg(SREG_CX);

      if (propertyIndexer)
        scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_DX);
      else
        scrip->write_cmd2(SCMD_ADDREG, SREG_CX, SREG_AX);
    }
    else if (propertyIndexer)
      scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_DX);
    else
      scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_CX);

    if (!propertyIndexer)
      *isArrayOffset = true;

    if (writingOperation)
      scrip->pop_reg(SREG_AX);

    // the array offset has now been added to CX (normal array)
    // or put into DX (property)
  }

  return 0;
}
/*
int process_arrays_and_members(int slilen,long*syml,int*soffset,int*extraoffset,
    int *readcmd, ccCompiledScript *scrip, int iswrite, int *addressOf,
    int *memberWasAccessed, int *isProperty, int mustBeWritable,
    int *symlOfVariable) {
  int onoffs = 1;
  // we have extra stuff, like a structure member or array index, so
  // work out the offset
  while (onoffs < slilen) {
    if (sym.get_type(syml[onoffs]) == SYM_OPENBRACKET) {
      // an array index
      int endof = findClosingBracketOffs(onoffs, syml, slilen);

      // save the current offset in CX if there is one,
      // because parse_sub_expr might destroy it
      if (extraoffset[0] != 0)
        scrip->push_reg(SREG_CX);

      if (get_array_index_into_ax(scrip, syml, onoffs, endof, true))
        return -1;

      // if there is a current offset saved in CX, restore it
      // otherwise, reset with no offset
      if (extraoffset[0] != 0)
        scrip->pop_reg(SREG_CX);
      else
        scrip->write_cmd2(SCMD_LITTOREG,SREG_CX,0);

      // add the result to CX (which is counting the overall offset)
      scrip->write_cmd2(SCMD_ADDREG,SREG_CX,SREG_AX);
      onoffs = endof+1;
      extraoffset[0]=1;
    }
    else if (sym.get_type(syml[onoffs]) == SYM_DOT) {
      *memberWasAccessed = 1;

      if (sym.get_type(syml[onoffs + 1]) == SYM_FUNCTION) {
        // a member function call. don't process the bit after
        // the dot, and instead tell it to load the address of
        // the object
        *addressOf = 1;
      }
      else if (sym.entries[syml[onoffs + 1]].flags & SFLG_PROPERTY) {
        // property pesudo-function
        // treat like a function for now
        *addressOf = 1;
        *isProperty = syml[onoffs + 1];
        sym.entries[*isProperty].flags |= SFLG_ACCESSED;
        *symlOfVariable = onoffs + 1;

        if (mustBeWritable) {
          // cannot use ++ or -- with property, because the memory
          // access shortcut won't work
          // Therefore, tell the caller to do it properly
          // and call us again to write the value
          readonly_cannot_cause_error = 1;
        }
        else if (iswrite) {
          if (sym.entries[syml[onoffs+1]].flags & SFLG_READONLY) {
            cc_error("property '%s' is read-only", sym.get_friendly_name(syml[onoffs + 1]).c_str());
            return -1;
          }
        }

        if (slilen > onoffs + 2) {
          // they did  lstList.OwningGUI.ID  for instance
          cc_error("nested property access not currently supported");
          return -1;
        }

      }
      else {
        *symlOfVariable = onoffs + 1;
        // since the member has a fixed offset into the structure, don't
        // write out any code to calculate the offset - instead, modify
        // the hard offset value which will be written to MAR
        soffset[0] += sym.entries[syml[onoffs+1]].soffs;
        readcmd[0] = get_readcmd_for_size(sym.entries[syml[onoffs+1]].ssize,iswrite);

        // if one of the struct members in the path is read-only, don't allow it
        if ((iswrite) || (mustBeWritable)) {
          if (sym.entries[syml[onoffs+1]].flags & SFLG_READONLY) {
            cc_error("variable '%s' is read-only", sym.get_friendly_name(syml[onoffs + 1]).c_str());
            return -1;
          }
        }
      }
      onoffs+=2;
    }
  }
  return 0;
}
*/

int call_property_func(ccCompiledScript *scrip, int propSym, int isWrite) {
  // a Property Get
  int numargs = 0;

  // AX contains the struct address

  // Always a struct member -- set OP = AX
  if ((sym.entries[propSym].flags & SFLG_STATIC) == 0) {
    scrip->push_reg(SREG_OP);
    scrip->write_cmd1(SCMD_CALLOBJ, SREG_AX);
  }

  if (isWrite) {
    // BX contains the new value
    if (sym.entries[propSym].flags & SFLG_IMPORTED)
      scrip->write_cmd1(SCMD_PUSHREAL, SREG_BX);
    else {
      cc_error("internal error: prop is not import");
      return -1;
    }

    numargs++;
  }

  if (sym.entries[propSym].flags & SFLG_ARRAY) {
    // array indexer is in DX
    if (sym.entries[propSym].flags & SFLG_IMPORTED)
      scrip->write_cmd1(SCMD_PUSHREAL, SREG_DX);
    else {
      cc_error("internal error: prop is not import");
      return -1;
    }

    numargs++;
  }

  if (sym.entries[propSym].flags & SFLG_IMPORTED) {
    // tell it how many args for this call (nested imported functions
    // causes stack problems otherwise)
    scrip->write_cmd1(SCMD_NUMFUNCARGS, numargs);
  }

  int propFunc;
  if (isWrite)
    propFunc = sym.entries[propSym].get_propset();
  else
    propFunc = sym.entries[propSym].get_propget();

  if (propFunc < 0) {
    cc_error("Internal error: property in use but not set");
    return -1;
  }

  // AX = Func Address
  scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, propFunc);

  if (sym.entries[propSym].flags & SFLG_IMPORTED) {
    scrip->fixup_previous(FIXUP_IMPORT);
    // do the call
    scrip->write_cmd1(SCMD_CALLEXT, SREG_AX);
    if (numargs > 0)
      scrip->write_cmd1(SCMD_SUBREALSTACK,numargs);
  }
  else {
    scrip->fixup_previous(FIXUP_FUNCTION);
    scrip->write_cmd1(SCMD_CALL, SREG_AX);

    // restore the stack
    if (numargs > 0) {
      scrip->cur_sp -= numargs*4;
      scrip->write_cmd2(SCMD_SUB,SREG_SP,numargs*4);
    }
  }

  if (!isWrite) {
    // function return type
    scrip->ax_val_type = sym.entries[propSym].vartype;
    scrip->ax_val_scope = SYM_LOCALVAR;
    if (sym.entries[propSym].flags & SFLG_DYNAMICARRAY)
      scrip->ax_val_type |= STYPE_DYNARRAY;
    if (sym.entries[propSym].flags & SFLG_POINTER)
      scrip->ax_val_type |= STYPE_POINTER;
    if (sym.entries[propSym].flags & SFLG_CONST)
      scrip->ax_val_type |= STYPE_CONST;
  }

  if ((sym.entries[propSym].flags & SFLG_STATIC) == 0) {
    scrip->pop_reg(SREG_OP);
  }

  return 0;
}

int do_variable_memory_access(ccCompiledScript *scrip, int variableSym,
                              int variableSymType, bool isProperty,
                              int writing, int mustBeWritable,
                              bool addressof, bool extraoffset,
                              int soffset, bool isPointer,
                              bool wholePointerAccess,
                              int mainVariableSym, int mainVariableType,
                              bool isDynamicArray, bool negateLiteral) {
  int gotValType = 0;
  int readcmd = get_readcmd_for_size(sym.entries[variableSym].ssize, writing);

  if (mainVariableType == SYM_VARTYPE) {
    // it's a static member property
    if (!isProperty) {
      cc_error("static non-property access: internal error");
      return -1;
    }
    // just write 0 to AX for ease of debugging if anything
    // goes wrong
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

    gotValType = sym.entries[variableSym].vartype;
    if (sym.entries[variableSym].flags & SFLG_CONST)
      gotValType |= STYPE_CONST;
  }
  else if ((mainVariableType == SYM_LITERALVALUE) || (mainVariableType == SYM_CONSTANT)) {
    if ((writing) || (mustBeWritable)) {
      if(mainVariableType == SYM_LITERALVALUE)
        cc_error("cannot write to a literal value");
      else
        cc_error("cannot write to constant");
      return -1;
    }
    int varSymValue;
    if (accept_literal_or_constant_value(variableSym, varSymValue, negateLiteral, "Error parsing integer value") < 0) {
      return -1;
    }
    scrip->write_cmd2(SCMD_LITTOREG,SREG_AX, varSymValue);
    gotValType = sym.normalIntSym;
  }
  else if (mainVariableType == SYM_LITERALFLOAT) {
    if ((writing) || (mustBeWritable)) {
      cc_error("cannot write to a literal value");
      return -1;
    }
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, float_to_int_raw((float)atof(sym.get_name(variableSym))));
    gotValType = sym.normalFloatSym;
  }
  else if ((mainVariableType == SYM_LOCALVAR) ||
           (mainVariableType == SYM_GLOBALVAR)) {

    gotValType = sym.entries[variableSym].vartype;
    if (sym.entries[variableSym].flags & SFLG_CONST)
      gotValType |= STYPE_CONST;

    // a "normal" variable
    if (isPointer) {
      // the address is already in MAR by the caller
      if ((!wholePointerAccess) && ((!addressof) || (soffset) || (extraoffset)))
        scrip->write_cmd(SCMD_CHECKNULL);
      if (soffset)
        scrip->write_cmd2(SCMD_ADD, SREG_MAR, soffset);
    }
    else if (mainVariableType == SYM_LOCALVAR) {
      // a local one
      scrip->write_cmd1(SCMD_LOADSPOFFS, scrip->cur_sp - soffset);
    }
    else {
      // global variable
      if (sym.entries[mainVariableSym].flags & SFLG_IMPORTED) {
        // imported variable, so get the import address and then add any offset
        scrip->write_cmd2(SCMD_LITTOREG,SREG_MAR,sym.entries[mainVariableSym].soffs);
        scrip->fixup_previous(FIXUP_IMPORT);
        if (soffset != 0)
          scrip->write_cmd2(SCMD_ADD,SREG_MAR,soffset);
      }
      else {
        scrip->write_cmd2(SCMD_LITTOREG,SREG_MAR,soffset);
        scrip->fixup_previous(FIXUP_GLOBALDATA);
      }
    }

    if (extraoffset)
    {
      if (isDynamicArray)
      {
        scrip->write_cmd1(SCMD_MEMREADPTR, SREG_MAR);
        scrip->write_cmd(SCMD_CHECKNULL);
        scrip->write_cmd1(SCMD_DYNAMICBOUNDS, SREG_CX);
      }

      scrip->write_cmd2(SCMD_ADDREG,SREG_MAR,SREG_CX);
    }
    else if (isDynamicArray)
    {
      // not accessing an element of it, must be whole thing
      wholePointerAccess = true;
      gotValType |= STYPE_DYNARRAY;
    }

    if ((wholePointerAccess) && (writing))
      scrip->write_cmd1(SCMD_MEMWRITEPTR, SREG_AX);
    else if ((wholePointerAccess) && (!writing))
      scrip->write_cmd1(SCMD_MEMREADPTR, SREG_AX);
    else if (addressof)
      scrip->write_cmd2(SCMD_REGTOREG,SREG_MAR,SREG_AX);
    else
      scrip->write_cmd1(readcmd,SREG_AX);
    }
  else if (mainVariableType == SYM_STRING) {
    if (writing) {
      cc_error("cannot write to a literal string");
      return -1;
    }

    scrip->write_cmd2(SCMD_LITTOREG,SREG_AX,soffset);
    scrip->fixup_previous(FIXUP_STRING);
    gotValType = sym.normalStringSym | STYPE_CONST;
  }
  else if (mainVariableType == SYM_STRUCTMEMBER) {
    cc_error("must include parent structure of member '%s'",sym.get_friendly_name(mainVariableSym).c_str());
    return -1;
    }
  else if (mainVariableType == SYM_NULL) {
    if (writing) {
      cc_error("Invalid use of null");
      return -1;
    }
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    gotValType = sym.nullSym | STYPE_POINTER;
  }
  else {
    cc_error("read/write ax called with non-variable parameter ('%s')",sym.get_friendly_name(variableSym).c_str());
    return -1;
    }

  if ((addressof) && (!isProperty))
    gotValType |= STYPE_POINTER;
  else if ((isProperty) && (sym.entries[variableSym].flags & SFLG_POINTER))
    gotValType |= STYPE_POINTER;

  if (writing) {
    if (check_type_mismatch(scrip->ax_val_type, gotValType, 1))
      return -1;
  }
  else
    scrip->ax_val_type = gotValType;

  if (isProperty) {
    // process_arrays_and_members will have set addressOf to true,
    // so AX now contains the struct address, and BX
    // contains the new value if this is a Set
    if (call_property_func(scrip, variableSym, writing))
      return -1;
  }

  return 0;
}


// If the variable being read is actually a property, not a
// member variable, then read_variable_into_ax sets this
int readonly_cannot_cause_error = 0;

int do_variable_ax(int slilen,long*syml,ccCompiledScript*scrip,int writing, int mustBeWritable, bool negateLiteral = false) {
  // read the various types of values into AX
  int ee;

  if (!writing)
    set_ax_scope(scrip, syml[0]);

  // separate out the variable path, into a variablePath
  // for the bit between each dot
  VariableSymlist variablePath[MAX_VARIABLE_PATH];

  int variablePathSize = extractPathIntoParts(variablePath, slilen, syml);
  if (variablePathSize == -1)
    return -1;

  int currentByteOffset = 0;
  // remember the type of the first variable in the list (since
  // that determines whether this is global/local)
  int firstVariableType, firstVariableSym = -1;
  bool isArrayOffset = false;
  bool isPointer = false;
  bool isDynamicArray = false;

  for (ee = 0; ee < variablePathSize; ee++) {
    VariableSymlist *thisClause = &variablePath[ee];

    int variableSym = thisClause->syml[0];
    int variableSymType = sym.get_type(variableSym);
    if (firstVariableSym < 0) {
      firstVariableSym = variableSym;
      firstVariableType = variableSymType;
    }

    // each clause in the chain can be marked as Accessed
    sym.entries[variableSym].flags |= SFLG_ACCESSED;

    bool getAddressOnlyIntoAX = false;
    bool doMemoryAccessNow = false;
    bool isLastClause = (ee == variablePathSize - 1);
    bool cannotAssign = false;
    bool isProperty = false;
    bool accessActualPointer = false;

    // the memory access only wants to write if this is the
    // end of the path, not an intermediate pathing property
    bool writingThisTime = isLastClause && writing;

    if (sym.entries[variableSym].flags & SFLG_PROPERTY) { }
    else if (sym.entries[variableSym].flags & SFLG_IMPORTED) { }
    else if ((variableSymType == SYM_GLOBALVAR) || (variableSymType == SYM_LOCALVAR) ||
        (variableSymType == SYM_STRUCTMEMBER) || (variableSymType == SYM_STRING)) {
      // since the member has a fixed offset into the structure, don't
      // write out any code to calculate the offset - instead, modify
      // the hard offset value which will be written to MAR
      currentByteOffset += sym.entries[variableSym].soffs;
    }

    if (variableSymType == SYM_FUNCTION) {
      // member function. all that's wanted is the owning object's
      // address
      getAddressOnlyIntoAX = true;
      doMemoryAccessNow = true;

      if (!isLastClause) {
        cc_error("Function().Member not supported");
        return -1;
      }
    }
    else if (sym.entries[variableSym].flags & SFLG_PROPERTY) {
      // since a property is effectively a function call, load
      // the object's address
      getAddressOnlyIntoAX = true;
      doMemoryAccessNow = true;
      isProperty = true;

      if ((sym.entries[variableSym].flags & SFLG_ARRAY) &&
          ((thisClause->len <= 1) ||
           (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET))) {
        // normally, the whole array can be used as a pointer.
        // this is not the case with an property array, so catch
        // it here and give an error
        cc_error("Expected array index after '%s'", sym.get_friendly_name(variableSym).c_str());
        return -1;
      }

      if (parseArrayIndexOffsets(scrip, thisClause, writing != 0, &isArrayOffset))
        return -1;

      if (mustBeWritable) {
        // cannot use ++ or -- with property, because the memory
        // access shortcut won't work
        // Therefore, tell the caller to do it properly
        // and call us again to write the value
        readonly_cannot_cause_error = 1;
      }
      else if (writing) {

        if ((writingThisTime) && (sym.entries[variableSym].flags & SFLG_READONLY)) {
          cc_error("property '%s' is read-only", sym.get_friendly_name(variableSym).c_str());
          return -1;
        }

        // Property Set -- move the new value into BX, so
        // that the object address can be retrieved into AX
        scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
      }

    }
    else if (sym.entries[variableSym].flags & SFLG_POINTER) {
      bool isArrayOfPointers = false;

      if (sym.entries[variableSym].flags & SFLG_ARRAY) {
        // array of pointers

        if ((thisClause->len <= 1) ||
           (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET)) {
          // normally, the whole array can be used as a pointer.
          // this is not the case with an pointer array, so catch
          // it here and give an error
          if (sym.entries[variableSym].flags & SFLG_DYNAMICARRAY)
          {
            isDynamicArray = true;
          }
          else
          {
            cc_error("Expected array index after '%s'", sym.get_friendly_name(variableSym).c_str());
            return -1;
          }
        }
        else
        {
          // put array index into DX
          if (parseArrayIndexOffsets(scrip, thisClause, writing != 0, &isArrayOffset))
            return -1;

          isArrayOfPointers = true;
        }
      }

      // if they are just saying "ptr" (or doing a "ptr.Func" call)
      // then move the address being pointed to into AX
      // (member function call passes in "ptr.")
      if (isLastClause)
        getAddressOnlyIntoAX = true;

      // Push the pointer address onto the stack, where it can be
      // retrieved by do_variable_memory_access later on
      if (sym.entries[variableSym].flags & SFLG_THISPTR) {
        if (isPointer) {
          // already a pointer on the stack
          cc_error("Nested this pointers??");
          return -1;
        }

        // for the "this" pointer, just use the Object Pointer
        scrip->push_reg(SREG_OP);
      }
      else {
        // currentByteOffset has been set at the start of this loop
        // so it is safe to use

        if (isPointer) {
          // already a pointer on the stack
          scrip->pop_reg(SREG_MAR);
          scrip->write_cmd(SCMD_CHECKNULL);
          if (currentByteOffset > 0)
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, currentByteOffset);
        }
        else if (firstVariableType == SYM_LOCALVAR) {
          scrip->write_cmd1(SCMD_LOADSPOFFS, scrip->cur_sp - currentByteOffset);
        }
        else if (firstVariableType == SYM_GLOBALVAR) {

          if (sym.entries[firstVariableSym].flags & SFLG_IMPORTED) {
            scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, sym.entries[firstVariableSym].soffs);
            scrip->fixup_previous(FIXUP_IMPORT);
            if (currentByteOffset)
              scrip->write_cmd2(SCMD_ADD, SREG_MAR, currentByteOffset);
          }
          else {
            scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, currentByteOffset);
            scrip->fixup_previous(FIXUP_GLOBALDATA);
          }
        }
        else {
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
        getAddressOnlyIntoAX = true;
        accessActualPointer = true;
        doMemoryAccessNow = true;
      }
      currentByteOffset = 0;

      // Pointer
      isPointer = true;
    }
    else {

      if (sym.entries[variableSym].flags & SFLG_DYNAMICARRAY)
      {
        isDynamicArray = true;
      }

      if (parseArrayIndexOffsets(scrip, thisClause, writing != 0, &isArrayOffset))
        return -1;

    }

    // if one of the struct members in the path is read-only, don't allow it
    if (((writing) || (mustBeWritable)) && (readonly_cannot_cause_error == 0)) {
      // allow writing to read-only pointers if it's actually
      // a property being accessed
      if ((sym.entries[variableSym].flags & SFLG_POINTER) && (!isLastClause)) { }
      else if (sym.entries[variableSym].flags & SFLG_READONLY) {
        cc_error("variable '%s' is read-only", sym.get_friendly_name(variableSym).c_str());
        return -1;
      }
      else if (sym.entries[variableSym].flags & SFLG_WRITEPROTECTED) {
        // write-protected variables can only be written by
        // the this ptr
        if ((ee > 0) && (sym.entries[variablePath[ee - 1].syml[0]].flags & SFLG_THISPTR)) { }
        else {
          cc_error("variable '%s' is write-protected", sym.get_friendly_name(variableSym).c_str());
          return -1;
        }

      }
    }


    if (sym.entries[variableSym].flags & SFLG_ARRAY) {
      // array without index specified -- get address
      if ((thisClause->len == 1) ||
          (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET)) {

        if ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0)
        {
          getAddressOnlyIntoAX = true;
          cannotAssign = true;
        }
      }

    }

    if (sym.entries[variableSym].flags & SFLG_POINTER) { }
    else if (sym.entries[sym.entries[variableSym].vartype].flags & SFLG_STRUCTTYPE) {
      // struct variable without member access
      if (isLastClause)
      {
        if ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0)
        {
          getAddressOnlyIntoAX = true;
          cannotAssign = true;
        }
      }
    }

    if ((writing) && (cannotAssign)) {
      // an entire array or struct cannot be assigned to
      cc_error("cannot assign to '%s'", sym.get_friendly_name(variableSym).c_str());
      return -1;
    }

    if ((doMemoryAccessNow) || (isLastClause)) {

      int cachedAxValType = scrip->ax_val_type;

      // if a pointer in use, then its address was pushed on the
      // stack, so restore it here
      if (isPointer)
        scrip->pop_reg(SREG_MAR);

      // in a writing operation, but not doing it just yet -- push
      // AX to save the value to write
      if ((writing) && (!writingThisTime))
        scrip->push_reg(SREG_AX);

      if (do_variable_memory_access(scrip, variableSym, variableSymType,
                                    isProperty, writingThisTime, mustBeWritable,
                                    getAddressOnlyIntoAX, isArrayOffset,
                                    currentByteOffset, isPointer,
                                    accessActualPointer,
                                    firstVariableSym, firstVariableType,
                                    isDynamicArray, negateLiteral))
        return -1;

      // reset stuff
      currentByteOffset = 0;
      isArrayOffset = false;
      isPointer = false;
      isDynamicArray = false;
      //firstVariableSym = -1;
      firstVariableType = SYM_GLOBALVAR;

      if (!isLastClause) {
        if ((isProperty) || (getAddressOnlyIntoAX)) {
          // pathing, eg. lstItems.OwningGUI.ID
          // we just read a pointer address, so re-push it for use
          // next time round
          if (writing) {
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
            scrip->push_reg(SREG_AX);

          isPointer = true;
        }
        else {
          cc_error("Invalid pathing: unexpected '%s'", sym.get_friendly_name(variablePath[ee + 1].syml[0]).c_str());
          return -1;
        }

      }

      // proceed round loop to next clause
    }


  }

  // free the VariablePaths
  for (ee = 0; ee < variablePathSize; ee++)
    variablePath[ee].destroy();

  return 0;
}


int read_variable_into_ax(int slilen,long*syml,ccCompiledScript*scrip, int mustBeWritable = 0, bool negateLiteral = false) {

  return do_variable_ax(slilen,syml,scrip, 0, mustBeWritable, negateLiteral);
}

int write_ax_to_variable(int slilen,long*syml,ccCompiledScript*scrip) {

  return do_variable_ax(slilen,syml,scrip, 1, 0);
}



int parse_sub_expr(long*symlist,int listlen,ccCompiledScript*scrip) {
/*  printf("Parse expression: '");
  int j;
  for (j=0;j<listlen;j++)
    printf("%s ",sym.get_friendly_name(symlist[j]).c_str());
  printf("'\n");*/

  if (listlen == 0) {
    cc_error("Empty sub-expression?");
    return -1;
  }

  int oploc = find_lowest_bonding_operator(symlist,listlen);
  bool hasNegatedLiteral;

  // A literal is being negated on the left
  if ((oploc == 0) && (listlen > 1) && (sym.get_type(symlist[1]) == SYM_LITERALVALUE) &&
    (sym.entries[symlist[oploc]].operatorToVCPUCmd() == SCMD_SUBREG)) {
    oploc = find_lowest_bonding_operator(&symlist[1], listlen - 1);
    if(oploc > -1)
      oploc++;
    hasNegatedLiteral = true;
  }

  if (oploc == 0) {
    // The operator is the first thing in the expression
    if (sym.get_type(symlist[oploc]) == SYM_NEW)
    {
      if (listlen < 2 || sym.get_type(symlist[oploc + 1]) != SYM_VARTYPE)
      {
        cc_error("expected type after 'new'");
        return -1;
      }

      if(listlen > 3 && sym.get_type(symlist[oploc + 2]) == SYM_OPENBRACKET && sym.get_type(symlist[listlen - 1]) == SYM_CLOSEBRACKET)
      {
          int arrayType = symlist[oploc + 1];

          if (parse_sub_expr(&symlist[oploc + 3], listlen - 4, scrip))
            return -1;

          if (scrip->ax_val_type != sym.normalIntSym)
          {
            cc_error("array size must be an int");
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
            cc_error("cannot create dynamic array of unmanaged struct");
            return -1;
          }

          scrip->write_cmd3(SCMD_NEWARRAY, SREG_AX, size, isManagedType);
          scrip->ax_val_type = arrayType | STYPE_DYNARRAY;

          if (isManagedType)
            scrip->ax_val_type |= STYPE_POINTER;
      }
      else
      {
          if(sym.entries[symlist[oploc + 1]].flags & SFLG_BUILTIN)
          {
            cc_error("Built-in type '%s' cannot be instantiated directly", sym.get_name(symlist[oploc + 1]));
            return -1;
          }
          const size_t size = sym.entries[symlist[oploc + 1]].ssize;
          scrip->write_cmd2(SCMD_NEWUSEROBJECT, SREG_AX, size);
          scrip->ax_val_type = symlist[oploc + 1] | STYPE_POINTER;
      }

      return 0;
    }
    else if (sym.entries[symlist[oploc]].operatorToVCPUCmd() == SCMD_SUBREG) {
      // "-" operator (it wants to negate whatever comes next)
      if (listlen < 2) {
        cc_error("parse error at '-'");
        return -1;
      }
      // parse the rest of the expression into AX
      if (parse_sub_expr(&symlist[1],listlen-1,scrip))
        return -1;
      int cpuOp = SCMD_SUBREG;
      if (check_operator_valid_for_type(&cpuOp, scrip->ax_val_type, 0))
        return -1;
      // now, subtract the result from 0 (which negates it)
      scrip->write_cmd2(SCMD_LITTOREG,SREG_BX,0);
      scrip->write_cmd2(cpuOp, SREG_BX, SREG_AX);
      scrip->write_cmd2(SCMD_REGTOREG,SREG_BX,SREG_AX);
      return 0;
    }
    else if (sym.entries[symlist[oploc]].operatorToVCPUCmd() == SCMD_NOTREG) {
      // "!" operator (NOT whatever comes next)
      if (listlen < 2) {
        cc_error("parse error at '!'");
        return -1;
      }
      // parse the rest of the expression into AX
      if (parse_sub_expr(&symlist[1],listlen-1,scrip))
        return -1;
      int cpuOp = SCMD_NOTREG;
      if (check_operator_valid_for_type(&cpuOp, scrip->ax_val_type, 0))
        return -1;
      // now, NOT the result
      scrip->write_cmd1(SCMD_NOTREG,SREG_AX);
      return 0;
    }
    else {
      // this operator needs a left hand side
      cc_error("Parse error: unexpected operator '%s'",sym.get_friendly_name(symlist[oploc]).c_str());
      return -1;
    }
  }

  if (oploc > 0) {
    // There is an operator in the expression, eg.  "5 + var1"
    int vcpuOperator = sym.entries[symlist[oploc]].operatorToVCPUCmd();

    if (vcpuOperator == SCMD_NOTREG) {
      // you can't do   a = b ! c;
      cc_error("Invalid use of operator '!'");
      return -1;
    }
    // A value is being negated on the right
    if ((vcpuOperator == SCMD_SUBREG) && (oploc > 1) && (sym.get_type(symlist[oploc - 1]) == SYM_OPERATOR)) {
      oploc = find_lowest_bonding_operator(symlist, oploc);
      vcpuOperator = sym.entries[symlist[oploc]].operatorToVCPUCmd();
    }
    // process the left hand side and save result onto stack
    if (parse_sub_expr(&symlist[0],oploc,scrip))
      return -1;

    if (oploc + 1 >= listlen) {
      // there is no right hand side for the expression
      cc_error("Parse error: invalid use of operator '%s'",sym.get_friendly_name(symlist[oploc]).c_str());
      return -1;
    }

    int jumpLocationOffset = -1;
    if (vcpuOperator == SCMD_AND) {
      // && operator lazy evaluation ... if AX is 0 then the AND
      // has failed, so just jump directly past the AND instruction;
      // AX will still be 0 so that will do as the result of the
      // calculation
      scrip->write_cmd1(SCMD_JZ, 0);
      jumpLocationOffset = scrip->codesize;
    }
    else if (vcpuOperator == SCMD_OR) {
      // || operator lazy evaluation ... if AX is non-zero then
      // the OR has succeeded, so just jump directly past the OR
      // instruction; AX will still be non-zero so that will do as
      // the result of the calculation
      scrip->write_cmd1(SCMD_JNZ, 0);
      jumpLocationOffset = scrip->codesize;
    }

    int valtypewas = scrip->ax_val_type;

    scrip->push_reg(SREG_AX);
    if (parse_sub_expr(&symlist[oploc+1],listlen-(oploc+1),scrip))
      return -1;
    scrip->pop_reg(SREG_BX);

    if (check_type_mismatch(scrip->ax_val_type, valtypewas, 0))
      return -1;
    if (check_operator_valid_for_type(&vcpuOperator, scrip->ax_val_type, valtypewas))
      return -1;
    // now LHS result is in BX, and RHS result is in AX
    scrip->write_cmd2(vcpuOperator, SREG_BX, SREG_AX);
    scrip->write_cmd2(SCMD_REGTOREG,SREG_BX,SREG_AX);

    if (jumpLocationOffset > 0) {
      // write the jump offset
      scrip->code[jumpLocationOffset - 1] = scrip->codesize - jumpLocationOffset;
    }

    // Operators like == return a bool (in our case, that's an int);
    // Other operators like + return the type that they're operating on
    if (isVCPUOperatorBoolean(vcpuOperator))
      scrip->ax_val_type = sym.normalIntSym;

    return 0;
  }

  // There is no operator in the expression -- therefore, there will
  // just be a variable name or function call

  long vnlist[TEMP_SYMLIST_LENGTH],lilen;
  int funcAtOffs = 0;
  ccInternalList tlist;
  tlist.pos=0;
  tlist.length=listlen;
  tlist.script=symlist;
  tlist.cancelCurrentLine = 0;
  lilen=extract_variable_name(tlist.getnext(),&tlist,&vnlist[0], &funcAtOffs);
  // stop it trying to free the memory
  tlist.script=NULL;
  tlist.length=0;
  if (lilen < 0)
    return -1;
/*  printf("lilen: %d, list is ");
  for (j=0;j<lilen;j++) printf("%s ",sym.get_friendly_name(vnlist[j]).c_str());
  printf("\n");*/

  if (sym.get_type(symlist[0]) == SYM_OPENPARENTHESIS) {
    int aa,fnd=-1,level=0;
    // find the corresponding closing parenthesis
    for (aa=1;aa<listlen;aa++) {
      if (sym.get_type(symlist[aa]) == SYM_CLOSEPARENTHESIS) {
        level--;
        if (level<0) { fnd=aa; break; }
        }
      else if (sym.get_type(symlist[aa]) == SYM_OPENPARENTHESIS)
        level++;
      }
    if (fnd < 0) {
      cc_error("Bracketed expression not terminated");
      return -1;
    }
    if (fnd <= 1) {
      cc_error("Empty bracketed expression");
      return -1;
    }

    if (parse_sub_expr(&symlist[1],fnd-1,scrip) < 0) return -1;
    symlist+=fnd+1;
    listlen-=fnd+1;
    if (listlen > 0) {
      // there is some code after the )
      // this should not be possible, unless the user does
      // something like "if ((x) 1234)" ie. with an operator missing
      cc_error("Parse error: operator expected");
      return -1;
/*
      scrip->push_reg(SREG_AX);
      int op = symlist[0];
      if (sym.get_type(op) != SYM_OPERATOR) {
        cc_error("expected operator, not '%s'",sym.get_friendly_name(op).c_str());
        return -1;
        }
      if (parse_sub_expr(&symlist[1],listlen-1,scrip) < 0) return -1;
      scrip->pop_reg(SREG_BX);
      // now LHS is in BX, RHS is in AX - so do the maths
      scrip->write_cmd2(sym.entries[op].operatorToVCPUCmd(),SREG_BX,SREG_AX);
      // copy the result into AX for return
      scrip->write_cmd2(SCMD_REGTOREG,SREG_BX,SREG_AX);*/
      }
    return 0;
    }
  else if (sym.get_type(symlist[0]) == 0) {
    cc_error("undefined symbol '%s'",sym.get_friendly_name(symlist[0]).c_str());
    return -1;
    }
  else if (hasNegatedLiteral && (listlen == 2)) {
    if (read_variable_into_ax(1,&symlist[1],scrip,0,true))
      return -1;
  }
  else if (sym.get_type(symlist[0]) == SYM_OPERATOR) {
    // If someone follows the negation operator with bogus tokens, the problem is actually on the right of it
    if ((sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG) && (listlen > 2))
      cc_error("Parse error: unexpected '%s'",sym.get_friendly_name(symlist[2]).c_str());
    else
      cc_error("Parse error: unexpected '%s'",sym.get_friendly_name(symlist[0]).c_str());
    return -1;
    }
  else if ((sym.get_type(symlist[0]) == SYM_FUNCTION) || (funcAtOffs > 0)) {
    long *usingList;
    int usingListLen;
    int using_op = 0;

    if (funcAtOffs > 0) {
      // member function
      usingList = &vnlist[funcAtOffs];
      // FIX 16/09/05: use listlen rather than lilen, hope this doesn't break anything
      usingListLen = listlen - funcAtOffs;

      using_op = funcAtOffs;

      funcAtOffs = 0;
    }
    else {
      // normal function
      usingList = symlist;
      usingListLen = listlen;
      funcAtOffs = 0;
    }

    int funcsym = usingList[funcAtOffs];

    // a function call
    if (sym.get_type(usingList[funcAtOffs + 1]) != SYM_OPENPARENTHESIS) {
      cc_error("expected '('");
      return -1;
    }

    // static function doesn't want the "this" ptr
    if (sym.entries[funcsym].flags & SFLG_STATIC)
      using_op = 0;

    usingList += 2;
    usingListLen -= 2;

    int ct, bdepth=0, num_supplied_args = 1;
    int opsSinceComma = 0;

    for (ct = funcAtOffs; ct < usingListLen; ct++) {
      if (sym.get_type(usingList[ct]) == SYM_OPENPARENTHESIS) bdepth++;
      if (sym.get_type(usingList[ct]) == SYM_CLOSEPARENTHESIS) {
        bdepth--;
        if (bdepth < 0) break;
      }
      if ((sym.get_type(usingList[ct]) == SYM_COMMA) && (bdepth == 0)) {
        num_supplied_args++;
        if (opsSinceComma < 1) {
          cc_error("missing argument in function call");
          return -1;
        }
        opsSinceComma = 0;
      }
      else
        opsSinceComma++;
    }
    // function call with no arguments -- set num_supplied back to 0
    if ((opsSinceComma == 0) && (num_supplied_args == 1))
      num_supplied_args = 0;

    if (bdepth >= 0) {
      cc_error("parser confused near '%s'",sym.get_friendly_name(usingList[-2]).c_str());
      return -1;
    }

    // push OP onto the stack, before the parameters
    if (using_op)
      scrip->push_reg(SREG_OP);

    // now, usingList[ct] = ')'
    int flen = ct;   // length of function parameters
    int orisize = ct;
    int thispar = 0;
    int numargs = 0;
    int func_args = sym.entries[funcsym].get_num_args();

    if (num_supplied_args < func_args) {
      // not enough arguments -- see if we can supply default values
      for (int ii = func_args; ii > num_supplied_args; ii--) {

        if (!sym.entries[funcsym].funcParamHasDefaultValues[ii]) {
          cc_error("Not enough parameters in call to function");
          return -1;
        }

        // push the default value onto the stack
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, sym.entries[funcsym].funcParamDefaultValues[ii]);

        if (sym.entries[funcsym].flags & SFLG_IMPORTED)
          scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
          scrip->push_reg(SREG_AX);

        numargs++;
      }
      // pretend we supplied enough, so that type checks with this
      // later will work
      num_supplied_args = func_args;
    }

    if (flen > 0)
     do {
      // now go backwards through the parameters (cos that's the direction
      // in which they need to be put on the stack)
      thispar = 0;
      bdepth = 0;
      for (ct = flen-1; ct >= 0; ct--) {
        // going backwards so ) increases the depth level
        if (sym.get_type(usingList[ct]) == SYM_CLOSEPARENTHESIS)
          bdepth++;
        if (sym.get_type(usingList[ct]) == SYM_OPENPARENTHESIS)
          bdepth--;
        if ((sym.get_type(usingList[ct]) == SYM_COMMA) && (bdepth == 0)) {
          thispar = ct+1;
          break;
          }
        }
      if (sym.get_type(usingList[thispar]) == SYM_CLOSEPARENTHESIS) {
        // they did  Display("Jibble",);
        cc_error("Unexpected ')'");
        return -1;
      }
      if (parse_sub_expr(&usingList[thispar],flen - thispar,scrip)) return -1;

      if (num_supplied_args - numargs <= func_args) {
        // if non-variable arguments, check types
        int parameterType = sym.entries[funcsym].funcparamtypes[num_supplied_args - numargs];

        PerformStringConversionInAX(scrip, &scrip->ax_val_type, parameterType);

        if (check_type_mismatch(scrip->ax_val_type, parameterType, 1))
          return -1;

        DoNullCheckOnStringInAXIfNecessary(scrip, scrip->ax_val_type, parameterType);
      }


      if (sym.entries[funcsym].flags & SFLG_IMPORTED)
        scrip->write_cmd1(SCMD_PUSHREAL,SREG_AX);
      else
        scrip->push_reg(SREG_AX);
      numargs++;
      flen = thispar-1;

      } while (thispar > 0);
    // we've now pushed all the arguments to the function
    usingList += orisize;
    usingListLen -= orisize;

    if (sym.get_type(usingList[0]) != SYM_CLOSEPARENTHESIS) {
      cc_error("expected ')'");
      return -1;
    }

    usingList++;
    usingListLen--;
    // check that the user provided the right number of args
    // if it's a variable arg function, check that there are enough
    if ((sym.entries[funcsym].sscope >= 100) && (numargs >= sym.entries[funcsym].sscope - 100)) ;
    else if (sym.entries[funcsym].sscope == numargs) ;
    else {
      cc_error("wrong number of parameters in call to '%s'",sym.get_friendly_name(funcsym).c_str());
      return -1;
      }
    sym.entries[funcsym].flags |= SFLG_ACCESSED;

    if (using_op) {
      // write the address of the function's object to the OP reg
      read_variable_into_ax(using_op, vnlist, scrip);
      scrip->write_cmd1(SCMD_CALLOBJ, SREG_AX);
    }

    if (sym.entries[funcsym].flags & SFLG_IMPORTED) {
      // tell it how many args for this call (nested imported functions
      // causes stack problems otherwise)
      scrip->write_cmd1(SCMD_NUMFUNCARGS, numargs);
    }
    // call it
    scrip->write_cmd2(SCMD_LITTOREG,SREG_AX,sym.entries[funcsym].soffs);
    if (sym.entries[funcsym].flags & SFLG_IMPORTED) {
      scrip->fixup_previous(FIXUP_IMPORT);
      // do the call
      scrip->write_cmd1(SCMD_CALLEXT,SREG_AX);
      if (numargs > 0)
        scrip->write_cmd1(SCMD_SUBREALSTACK,numargs);
    }
    else {
      scrip->fixup_previous(FIXUP_FUNCTION);
      scrip->write_cmd1(SCMD_CALL,SREG_AX);

      // restore the stack
      if (numargs > 0) {
        scrip->cur_sp -= numargs*4;
        scrip->write_cmd2(SCMD_SUB,SREG_SP,numargs*4);
      }
    }
    // function return type
    scrip->ax_val_type = sym.entries[funcsym].funcparamtypes[0];
    scrip->ax_val_scope = SYM_LOCALVAR;

    if (using_op)
      scrip->pop_reg(SREG_OP);

    // make sure there's nothing left to process in this clause
    if (usingListLen > 0) {
      cc_error("expected semicolon after '%s'",sym.get_friendly_name(usingList[-1]).c_str());
      return -1;
    }
  }
  else if (listlen == lilen) {
    if (read_variable_into_ax(lilen,&vnlist[0],scrip)) return -1;
    }
  else if (listlen == 1) {
    if (read_variable_into_ax(1,&symlist[0],scrip)) return -1;
    }
  else {
    cc_error("Parse error in expr near '%s'",sym.get_friendly_name(symlist[0]).c_str());
    return -1;
    }

  return 0;
}

// evaluate the supplied expression, putting the result into AX
// returns 0 on success or -1 if compile error
// leaves targ pointing to last token in expression, so do getnext() to
// get the following ; or whatever
// --
// insideBracketedDeclaration is a special flag used to denote an expression that
// starts inside a declaration that contains more than one evaluatable chunk in brackets
// and expects the closing bracket to terminate the declaration. The while() loop relies
// on the fact that a while loop's bracket completely surrounds its expression. With a
// for loop, we need to break that up into segments:
// for(a = 0; a < 10; a++)
//     |seg1| seg 2 |seg3|
// So segment 3 is an expression that looks like this:
// ; a++)
// For this to work properly, we can't just increment the initial brackdepth. We need
// to parse the expression in a slightly different way so that the final bracket is not
// consumed as part of evaluating the expression.
int evaluate_expression(ccInternalList*targ,ccCompiledScript*scrip,int countbrackets, bool insideBracketedDeclaration) {
  ccInternalList ours;
  int j,ourlen=0,brackdepth=0;
  int hadMetaOnly = 1;
  bool lastWasNew = false;

  for (j=targ->pos; j < targ->length; j++) {
    if (targ->script[j] == SCODE_META) {
      j+=2;
      continue;
    }

    if (sym.get_type(targ->script[j]) == SYM_OPENPARENTHESIS)
      brackdepth++;
    else if (sym.get_type(targ->script[j]) == SYM_CLOSEPARENTHESIS && (brackdepth > 0 || !insideBracketedDeclaration)) {
      brackdepth--;
      continue;
    }
    else if (sym.get_type(targ->script[j]) == SYM_NEW)
    {
      lastWasNew = true;
    }
    else if (lastWasNew)
    {
      lastWasNew = false;
    }
    else if (((!isPartOfExpression(targ, j)) && (brackdepth == 0))
          || ((brackdepth == 0) && (countbrackets!=0))
          || sym.get_type(targ->script[j]) == SYM_CLOSEPARENTHESIS) {
      ourlen = j - targ->pos;
      if ((ourlen < 1) || (hadMetaOnly == 1)) {
        cc_error("PE01: Parse error at '%s'",sym.get_friendly_name(targ->script[j]).c_str());
        return -1;
        }
      ours.script = (long*)malloc(ourlen * sizeof(long));
      memcpy(ours.script,&targ->script[targ->pos],ourlen*sizeof(long));
      int k,l;
      for (k=0;k<ourlen;k++) {
        if (ours.script[k] == SCODE_META) {
          ourlen-=3;
          for (l=k; l < ourlen; l++)
            ours.script[l] = ours.script[l+3];
          k--;
        }
      }
      ours.length = ourlen;
      ours.pos = 0;
      ours.allocated = ourlen;
      break;
    }

    // had a real token, not just metadata
    hadMetaOnly = 0;
  }

  if (j >= targ->length) {
    free(ours.script);
    ours.script=NULL;
    cc_error("end of input reached in middle of expression");
    return -1;
    }
  targ->pos = j;
  int retcode=0;
  // we now have the expression in 'ours'
  retcode=parse_sub_expr(&ours.script[0],ours.length,scrip);

  free(ours.script);
  ours.script = NULL;
  return retcode;
  }

int evaluate_assignment(ccInternalList *targ, ccCompiledScript *scrip, bool expectCloseBracket, int cursym, long lilen, long *vnlist, bool insideBracketedDeclaration) {
    if (!sym.entries[cursym].is_loadable_variable()) {
        // allow through static properties
        if ((sym.get_type(cursym) == SYM_VARTYPE) && (lilen > 2) &&
            (sym.entries[vnlist[2]].flags & SFLG_STATIC))
        { }
        else {
            cc_error("variable required on left of assignment %s ", sym.get_name(cursym));
            return -1;
        }
    }
    bool isAccessingDynamicArray = false;

    if (((sym.entries[cursym].flags & SFLG_DYNAMICARRAY) != 0) && (lilen < 2))
    {
        if (sym.get_type(targ->peeknext()) != SYM_ASSIGN)
        {
            cc_error("invalid use of operator with array");
            return -1;
        }
        isAccessingDynamicArray = true;
    }
    else if (((sym.entries[cursym].flags & SFLG_ARRAY) != 0) && (lilen < 2))
    {
        cc_error("cannot assign value to entire array");
        return -1;
    }
    if (sym.entries[cursym].flags & SFLG_ISSTRING) {
        cc_error ("cannot assign to string; use Str* functions instead");
        return -1;
    }
    /*
    if (sym.entries[cursym].flags & SFLG_READONLY) {
    cc_error("variable '%s' is read-only", sym.get_name(cursym));
    return -1;
    }
    */
    int MARIntactAssumption = 0;
    int asstype = targ->getnext();
    if (sym.get_type(asstype) == SYM_SASSIGN) {

        // ++ or --
        readonly_cannot_cause_error = 0;

        if (read_variable_into_ax(lilen,&vnlist[0],scrip, 1))
            return -1;

        int cpuOp = sym.entries[asstype].ssize;

        if (check_operator_valid_for_type(&cpuOp, scrip->ax_val_type, 0))
            return -1;

        scrip->write_cmd2(cpuOp, SREG_AX, 1);

        if (!readonly_cannot_cause_error) {
            MARIntactAssumption = 1;
            // since the MAR won't have changed, we can directly write
            // the value back to it without re-calculating the offset
            scrip->write_cmd1(get_readcmd_for_size(readcmd_lastcalledwith,1),SREG_AX);
        }
    }
    // not ++ or --, so we need to evaluate the RHS
    else if (evaluate_expression(targ,scrip,0,insideBracketedDeclaration) < 0)
        return -1;

    if (sym.get_type(asstype) == SYM_MASSIGN) {
        // it's a += or -=, so read in and adjust the result
        scrip->push_reg(SREG_AX);
        int varTypeRHS = scrip->ax_val_type;

        if (read_variable_into_ax(lilen,&vnlist[0],scrip))
            return -1;
        if (check_type_mismatch(varTypeRHS, scrip->ax_val_type, 1))
            return -1;

        int cpuOp = sym.entries[asstype].ssize;

        if (check_operator_valid_for_type(&cpuOp, varTypeRHS, scrip->ax_val_type))
            return -1;

        scrip->pop_reg(SREG_BX);
        scrip->write_cmd2(cpuOp, SREG_AX, SREG_BX);
    }

    if (sym.get_type(asstype) == SYM_ASSIGN) {
        // Convert normal literal string into String object
        int finalPartOfLHS = lilen - 1;
        if (sym.get_type(vnlist[lilen - 1]) == SYM_CLOSEBRACKET) {
            // deal with  a[1] = b
            finalPartOfLHS = findOpeningBracketOffs(lilen - 1, vnlist) - 1;
            if (finalPartOfLHS < 0) {
                cc_error("No [ for ] to match");
                return -1;
            }
        }
        PerformStringConversionInAX(scrip, &scrip->ax_val_type, sym.entries[vnlist[finalPartOfLHS]].vartype);
    }

    if (MARIntactAssumption) ;
    // so copy the result (currently in AX) into the variable
    else if (write_ax_to_variable(lilen,&vnlist[0],scrip))
        return -1;

    if(expectCloseBracket) {
        if (sym.get_type(targ->getnext()) != SYM_CLOSEPARENTHESIS) {
            cc_error("Expected ')'");
            return -1;
        }
    }
    else
        if (sym.get_type(targ->getnext()) != SYM_SEMICOLON) {
            cc_error("Expected ';'");
            return -1;
        }

    return 0;
}

int parse_variable_declaration(long cursym,int *next_type,int isglobal,
    int varsize,ccCompiledScript*scrip,ccInternalList*targ, int vtwas,
    int isPointer) {
  long lbuffer = 0;
  std::vector<char> xbuffer;
  long *getsvalue = &lbuffer;
  int need_fixup = 0;
  int array_size = 1;
  if (sym.get_type(cursym) != 0) {
    cc_error ("Symbol '%s' already defined");
    return -1;
  }

  if ((sym.entries[vtwas].flags & SFLG_MANAGED) && (!isPointer) && (isglobal != 2)) {
    // managed structs must be allocated via ccRegisterObject,
    // and cannot be declared normally in the script (unless imported)
    cc_error("Cannot declare local instance of managed type");
    return -1;
  }

  if (vtwas == sym.normalVoidSym) {
    cc_error("'void' not a valid variable type");
    return -1;
  }

  sym.entries[cursym].extends = 0;
  sym.entries[cursym].stype = (isglobal != 0) ? SYM_GLOBALVAR : SYM_LOCALVAR;
  if (isPointer) {
    varsize = 4;
  }
  sym.entries[cursym].ssize = varsize;
  sym.entries[cursym].arrsize = 1;
  sym.entries[cursym].vartype = vtwas;
  if (isPointer)
    sym.entries[cursym].flags |= SFLG_POINTER;

  if (((sym.entries[vtwas].flags & SFLG_MANAGED) == 0) && (isPointer) && (isglobal != 2)) {
    // can only point to managed structs
    cc_error("Cannot declare pointer to non-managed type");
    return -1;
  }

  if (next_type[0] == SYM_OPENBRACKET) {
    // an array
    targ->getnext();  // skip the [

    if (sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
    {
      sym.entries[cursym].flags |= SFLG_DYNAMICARRAY;
      array_size = 0;
      varsize = 4;
      //cc_error("dynamic arrays not yet supported"); return -1;
    }
    else
    {
      int nextt = targ->getnext();

      if (accept_literal_or_constant_value(nextt, array_size, false, "Array size must be constant value") < 0) {
        return -1;
      }

      if (sym.entries[vtwas].flags & SFLG_HASDYNAMICARRAY) {
        cc_error("Cannot declare an array of a type containing dynamic array(s)");
        return -1;
      }

      if (array_size < 1) {
        cc_error("Array size must be >=1");
        return -1;
      }

      varsize *= array_size;
    }
    sym.entries[cursym].flags |= SFLG_ARRAY;
    sym.entries[cursym].arrsize = array_size;

    if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
    {
      cc_error("expected ']'");
      return -1;
    }

    next_type[0] = sym.get_type(targ->peeknext());
    xbuffer.resize(varsize + 1);
    getsvalue = (long*)&xbuffer.front();
  }
  else if (varsize > 4) {
    xbuffer.resize(varsize + 1);
    getsvalue = (long*)&xbuffer.front();
  }

  if (strcmp(sym.get_name(vtwas),"string")==0) {
    sym.entries[cursym].flags |= SFLG_ISSTRING;
    // if it's a string, allocate it some space
    if (ccGetOption(SCOPT_OLDSTRINGS) == 0) {
      cc_error("type 'string' is no longer supported; use String instead");
      return -1;
    }
    else if (sym.entries[cursym].flags & SFLG_DYNAMICARRAY)
    {
      cc_error("arrays of old-style strings are not supported");
      return -1;
    }
    else if (isglobal == 2) {
      // importing a string
      // cannot import, because string is really char*, and the pointer
      // won't resolve properly
      cc_error("cannot import string; use char[] instead");
      return -1;
    }
    else if (isglobal == 1) {
      getsvalue[0] = scrip->add_global(STRING_LENGTH,NULL);
      if (getsvalue[0] < 0)
        return -1;
      need_fixup = 1;
    }
    else if (isglobal == 0) {
      //getsvalue[0] = scrip->cur_sp;
      getsvalue = NULL;
      // save the address of this bit of memory to assign the pointer to it
      // we can't use scrip->cur_sp since we don't know if we'll be in
      // a nested function call at the time
      scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_CX);
      //scrip->add_fixup(scrip->codesize-2,FIXUP_STACK);
      scrip->cur_sp += STRING_LENGTH;
      scrip->write_cmd2(SCMD_ADD,SREG_SP,STRING_LENGTH);
      sym.entries[cursym].flags |= SFLG_STRBUFFER;
      //need_fixup = 1;
    }
  }

  // assign an initial value to the variable
  if (next_type[0] == SYM_ASSIGN) {
    if (isglobal == 2) {
      cc_error("cannot set initial value of imported variables");
      return -1;
    }
    if ((sym.entries[cursym].flags & (SFLG_ARRAY | SFLG_DYNAMICARRAY)) == SFLG_ARRAY) {
      cc_error("cannot assign value to array");
      return -1;
    }
    if (sym.entries[cursym].flags & SFLG_ISSTRING) {
      cc_error("cannot assign value to string, use StrCopy");
      return -1;
    }
    targ->getnext();  // skip the '='

    int actualVarType = vtwas;
    if (sym.entries[cursym].flags & SFLG_POINTER)
      actualVarType |= STYPE_POINTER;

    if (sym.entries[cursym].flags & SFLG_DYNAMICARRAY)
      actualVarType |= STYPE_DYNARRAY;

    if (isglobal) {
      if ((sym.entries[cursym].flags & (SFLG_POINTER | SFLG_DYNAMICARRAY)) != 0) {
        cc_error("cannot assign initial value to global pointer");
        return -1;
      }
      bool is_neg = false;
      if (sym.get_name(targ->peeknext())[0] == '-') {
        is_neg = true;
        targ->getnext();
      }
      if (sym.entries[cursym].vartype == sym.normalFloatSym) {
        // initialize float
        if (sym.get_type(targ->peeknext()) != SYM_LITERALFLOAT) {
          cc_error("Expected floating point value after '='");
          return -1;
        }
        float tehValue = (float)atof(sym.get_name(targ->getnext()));
        if (is_neg)
          tehValue = -tehValue;
        getsvalue[0] = float_to_int_raw(tehValue);
      }
      else if (sym.entries[cursym].ssize > 4) {
        cc_error("cannot initialize struct type");
        return -1;
      }
      else {
        int getsvalue_int;
        if (accept_literal_or_constant_value(targ->getnext(), getsvalue_int, is_neg, "Expected integer value after '='") < 0) {
          return -1;
        }
        getsvalue[0] = (long)getsvalue_int;

      }
    }
    else {

      if (evaluate_expression(targ,scrip,0,false))
        return -1;

      PerformStringConversionInAX(scrip, &scrip->ax_val_type, actualVarType);

      if (check_type_mismatch(scrip->ax_val_type, actualVarType, 1))
        return -1;
      need_fixup = 2;
    }
    next_type[0] = sym.get_type(targ->peeknext());
  }

  if (isglobal == 2) {
    // an imported variable
    sym.entries[cursym].soffs = scrip->add_new_import(sym.get_name(cursym));
    sym.entries[cursym].flags |= SFLG_IMPORTED;
    if (sym.entries[cursym].soffs == -1) {
      cc_error("Internal error: import table overflow");
      return -1;
      }
    }
  else if (isglobal) {
    // a global variable
    sym.entries[cursym].soffs = scrip->add_global(varsize,reinterpret_cast<const char*>(&getsvalue[0]));
    if (sym.entries[cursym].soffs < 0)
      return -1;
    if (need_fixup == 1) scrip->add_fixup(sym.entries[cursym].soffs,FIXUP_DATADATA);
    }
  else {
    // local variable
    sym.entries[cursym].soffs = scrip->cur_sp;
    scrip->write_cmd2(SCMD_REGTOREG,SREG_SP,SREG_MAR);
    if (need_fixup == 2) {
      // expression worked out into ax
      if ((sym.entries[cursym].flags & (SFLG_POINTER | SFLG_DYNAMICARRAY)) != 0)
      {
        scrip->write_cmd1(SCMD_MEMINITPTR, SREG_AX);
      }
      else
        scrip->write_cmd1(get_readcmd_for_size(varsize,1),SREG_AX);
    }
    else if (getsvalue == NULL)
      // local string, so the memory chunk pointer needs to be written
      scrip->write_cmd1(SCMD_MEMWRITE, SREG_CX);
    else  // local variable without initial value -- zero it
      scrip->write_cmd1(SCMD_ZEROMEMORY, varsize);

    if (need_fixup == 1) {
      sym.entries[cursym].flags |= SFLG_STRBUFFER;
      scrip->fixup_previous(FIXUP_STACK);
    }
    if(varsize > 0) {
      scrip->cur_sp += varsize;
      scrip->write_cmd2(SCMD_ADD,SREG_SP,varsize);
    }
  }
  if (next_type[0] == SYM_COMMA) {
    targ->getnext();  // skip the comma
    return 2;
    }
  if (check_not_eof(*targ))
    return -1;
  if (next_type[0] != SYM_SEMICOLON) {
    cc_error("Expected ',' or ';', not '%s'",sym.get_friendly_name(targ->peeknext()).c_str());
    return -1;
    }
  targ->getnext();  // skip the semicolon
  return 0;
  }

#define INC_NESTED_LEVEL \
    if (nested_level >= MAX_NESTED_LEVEL) {\
    cc_error("too many nested if/else statements");\
    return -1;\
    }\
    nested_level++

// compile the code in the INPL parameter into code in the scrip structure,
// but don't reset anything because more files could follow
int __cc_compile_file(const char*inpl,ccCompiledScript*scrip) {
    ccInternalList targ;
    if (cc_tokenize(inpl,&targ,scrip)) return -1;

    int aa,in_func = -1, nested_level = 0;
    int isMemberFunction = 0;
    int inFuncSym = -1;
    char nested_type[MAX_NESTED_LEVEL];
    long nested_info[MAX_NESTED_LEVEL];
    long nested_start[MAX_NESTED_LEVEL];
    std::vector<ccChunk> nested_chunk[MAX_NESTED_LEVEL];
    int32_t nested_assign_addr[MAX_NESTED_LEVEL];
    char next_is_import = 0, next_is_readonly = 0;
    char next_is_managed = 0, next_is_static = 0;
    char next_is_protected = 0, next_is_stringstruct = 0;
    char next_is_autoptr = 0, next_is_noloopcheck = 0;
    char next_is_builtin = 0;
    nested_type[0]=NEST_NOTHING;

    // *** now we have the program as a list of symbols in targ
    // go through it one by one. We start off in the global data
    // part - no code is allowed until a function definition is started
    currentline=1;
    targ.startread();
    int currentlinewas=0;
    for (aa=0;aa<targ.length;aa++) {
        int cursym = targ.getnext();
        if (currentline == -10) break; // end of stream was reached
        if ((currentline != currentlinewas) && (ccGetOption(SCOPT_LINENUMBERS)!=0)) {
            scrip->set_line_number(currentline);
            currentlinewas = currentline;
        }

        if (cursym == SCODE_INVALID) {
            cc_error("Internal compiler error: invalid symbol found");
            return -1;
        }
        else if (cursym == SCODE_META) {
            long metatype = targ.getnext();
            if (metatype==SMETA_END) break;
            else if (metatype==SMETA_LINENUM) {
                cc_error("Internal errror: unexpected meta tag");
                return -1;
            }
            else {
                cc_error("Internal compiler error: invalid meta tag found in stream");
                return -1;
            }
        }

        if (strncmp(sym.get_name(cursym), NEW_SCRIPT_TOKEN_PREFIX, 18) == 0)
        {
            strcpy(scriptNameBuffer, &sym.get_name(cursym)[18]);
            scriptNameBuffer[strlen(scriptNameBuffer) - 1] = 0;  // strip closing speech mark
            ccCurScriptName = scriptNameBuffer;

            scrip->start_new_section(scriptNameBuffer);
            currentline = 0;
            continue;
        }

        int symType = sym.get_type(cursym);

        if (symType == SYM_OPENBRACE) {
            if (in_func < 0) {
                cc_error("Unexpected '{'");
                return -1;
            }
            if ((nested_type[nested_level] == NEST_IFSINGLE) ||
                (nested_type[nested_level] == NEST_ELSESINGLE) ||
                (nested_type[nested_level] == NEST_DOSINGLE)) {
                    cc_error("Internal compiler error in openbrace");
                    return -1;
            }
            INC_NESTED_LEVEL;
            if (nested_level == 1) {
                nested_type[nested_level]=NEST_FUNCTION;
                // write base address of function for any relocation needed later
                scrip->write_cmd1(SCMD_THISBASE, scrip->codesize);
                if (next_is_noloopcheck)
                    scrip->write_cmd(SCMD_LOOPCHECKOFF);

                // loop through all parameters and check if they are pointers
                // the first entry is the return value
                for (int pa = 1; pa <= sym.entries[inFuncSym].sscope; pa++) {
                    if (sym.entries[inFuncSym].funcparamtypes[pa] & (STYPE_POINTER | STYPE_DYNARRAY)) {
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
                if ((isMemberFunction) && ((sym.entries[inFuncSym].flags & SFLG_STATIC) == 0)) {
                    int thisSym = sym.find("this");
                    if (thisSym > 0) {
                        int varsize = 4;
                        // declare "this" inside member functions
                        sym.entries[thisSym].stype = SYM_LOCALVAR;
                        sym.entries[thisSym].vartype = isMemberFunction;
                        sym.entries[thisSym].ssize = varsize; // pointer to struct
                        sym.entries[thisSym].sscope = nested_level;
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
            else nested_type[nested_level]=NEST_NOTHING;
            nested_start[nested_level]=0;

            next_is_noloopcheck = 0;
        }
        else if (symType == SYM_CLOSEBRACE) {
            // it's a single-line if statement
            if ((nested_type[nested_level] == NEST_IFSINGLE) ||
                (nested_type[nested_level] == NEST_ELSESINGLE) ||
                (nested_type[nested_level] == NEST_DOSINGLE)) {
                    cc_error("Unexpected '}'");
                    return -1;
            }
            nested_level--;
            if (nested_level < 0) {
                cc_error("Unexpected '}'");
                return -1;
            }

            if (nested_level == 0) {
                // ensure that 0 is returned since they haven't specified anything else
                scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
            }

            // find local variables that have just been removed
            int totalsub = remove_locals (nested_level, 0, scrip);

            if (totalsub > 0) {
                scrip->cur_sp -= totalsub;
                scrip->write_cmd2(SCMD_SUB,SREG_SP,totalsub);
            }
            if (nested_level == 0) {
                in_func = -1;
                inFuncSym = -1;
                isMemberFunction = 0;
                scrip->write_cmd(SCMD_RET);
                scrip->cur_sp -= 4;  // return address removed from stack
            }
            else if ((nested_type[nested_level+1] == NEST_IF) ||
                (nested_type[nested_level+1] == NEST_ELSE) ||
                (nested_type[nested_level+1] == NEST_DO) ||
                (nested_type[nested_level+1] == NEST_SWITCH)) {
                    INC_NESTED_LEVEL;
                    if (nested_type[nested_level] == NEST_DO)
                    {
                        if (deal_with_end_of_do(nested_info,nested_start,scrip,&targ,&nested_level))
                            return -1;
                    }
                    else
                        if (nested_type[nested_level] == NEST_SWITCH)
                        {
                        	if (deal_with_end_of_switch(nested_assign_addr,nested_start,&nested_chunk[nested_level],scrip,&targ,&nested_level,nested_info))
                                return -1;
                        }
                        else
                            if (deal_with_end_of_ifelse(nested_type,nested_info,nested_start,scrip,&targ,&nested_level,nested_chunk))
                                continue;
            }
            while ((nested_type[nested_level] == NEST_IFSINGLE) ||
                (nested_type[nested_level] == NEST_ELSESINGLE) ||
                (nested_type[nested_level] == NEST_DOSINGLE)) {
                    // loop round doing all the end of elses, but break once an IF
                    // has been turned into an ELSE
                    if (nested_type[nested_level] == NEST_DOSINGLE)
                    {
                        if (deal_with_end_of_do(nested_info,nested_start,scrip,&targ,&nested_level))
                            return -1;
                    }
                    else
                        if (deal_with_end_of_ifelse(nested_type,nested_info,nested_start,scrip,&targ,&nested_level,nested_chunk))
                            break;
            }
        }
        else if (symType == SYM_STRUCT) {
            // a "struct" definition
            int stname = targ.getnext();
            if ((sym.get_type(stname) != 0) &&
                (sym.get_type(stname) != SYM_UNDEFINEDSTRUCT)) {
                    cc_error("'%s' is already defined",sym.get_friendly_name(stname).c_str());
                    return -1;
            }
            int size_so_far = 0;
            int extendsWhat = 0;
            sym.entries[stname].extends = 0;
            sym.entries[stname].stype = SYM_VARTYPE;
            sym.entries[stname].flags |= SFLG_STRUCTTYPE;
            sym.entries[stname].ssize = 0;

            if (sym.get_type(targ.peeknext()) == SYM_SEMICOLON) {
                // forward-declaration of struct type
                targ.getnext();
                sym.entries[stname].stype = SYM_UNDEFINEDSTRUCT;
                sym.entries[stname].ssize = 4;
                if (next_is_managed) {
                    sym.entries[stname].flags |= SFLG_MANAGED;
                    next_is_managed = 0;
                }
                continue;
            }

            if (next_is_managed) {
                sym.entries[stname].flags |= SFLG_MANAGED;
                next_is_managed = 0;
            }

            if (next_is_builtin) {
                sym.entries[stname].flags |= SFLG_BUILTIN;
                next_is_builtin = 0;
            }

            if (next_is_autoptr) {
                sym.entries[stname].flags |= SFLG_AUTOPTR;
                next_is_autoptr = 0;
            }

            if (next_is_stringstruct) {
                sym.stringStructSym = stname;
                next_is_stringstruct = 0;
            }

            if (sym.get_type(targ.peeknext()) == SYM_EXTENDS) {
                targ.getnext();
                extendsWhat = targ.getnext();
                if (sym.get_type(extendsWhat) != SYM_VARTYPE) {
                    cc_error("Invalid use of 'extends'");
                    return -1;
                }
                if ((sym.entries[extendsWhat].flags & SFLG_STRUCTTYPE) == 0) {
                    cc_error("Must extend a struct type");
                    return -1;
                }
                if ((sym.entries[extendsWhat].flags & SFLG_MANAGED) == 0 && (sym.entries[stname].flags & SFLG_MANAGED)) {
                    cc_error("Incompatible types. Managed struct cannot extend unmanaged struct '%s'", sym.get_name(extendsWhat));
                    return -1;
                }
                if ((sym.entries[extendsWhat].flags & SFLG_MANAGED) && (sym.entries[stname].flags & SFLG_MANAGED) == 0) {
                    cc_error("Incompatible types. Unmanaged struct cannot extend managed struct '%s'", sym.get_name(extendsWhat));
                    return -1;
                }
                if ((sym.entries[extendsWhat].flags & SFLG_BUILTIN) && (sym.entries[stname].flags & SFLG_BUILTIN) == 0) {
                    cc_error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", sym.get_name(extendsWhat));
                    return -1;
                }
                size_so_far = sym.entries[extendsWhat].ssize;
                sym.entries[stname].extends = extendsWhat;
            }
            if (sym.get_type(targ.getnext()) != SYM_OPENBRACE) {
                cc_error("expected '{'");
                return -1;
            }

            while (sym.get_type(targ.peeknext()) != SYM_CLOSEBRACE) {
                cursym = targ.getnext();
                int member_is_readonly = 0;
                int member_is_import = 0;
                int member_is_property = 0;
                int member_is_pointer = 0;
                int member_is_static = 0;
                int member_is_protected = 0;
                int member_is_writeprotected = 0;

                // loop for all qualifiers.
                bool foundQualifier;
                do {
                    foundQualifier = false;

                    if (sym.get_type(cursym) == SYM_PROTECTED) {
                        // protected
                        member_is_protected = 1;
                        foundQualifier = true;
                        cursym = targ.getnext();
                    }
                    if (sym.get_type(cursym) == SYM_WRITEPROTECTED) {
                        // write-protected
                        member_is_writeprotected = 1;
                        foundQualifier = true;
                        cursym = targ.getnext();
                    }
                    if (sym.get_type(cursym) == SYM_READONLY) {
                        // read only member, carry on
                        member_is_readonly = 1;
                        foundQualifier = true;
                        cursym = targ.getnext();
                    }
                    if (sym.get_type(cursym) == SYM_IMPORT) {
                        member_is_import = 1;
                        foundQualifier = true;
                        cursym = targ.getnext();
                    }
                    if (sym.get_type(cursym) == SYM_STATIC) {
                        member_is_static = 1;
                        foundQualifier = true;
                        cursym = targ.getnext();
                    }
                    if (sym.get_type(cursym) == SYM_PROPERTY) {
                        // a "property" is a member variable that is actually a pair of functions
                        member_is_property = 1;
                        foundQualifier = true;
                        cursym = targ.getnext();
                    }
                } while (foundQualifier);

                if (member_is_protected && member_is_writeprotected) {
                    cc_error("Field cannot be both protected and write-protected.");
                    return -1;
                }

                if ((sym.get_type(cursym) != SYM_VARTYPE) &&
                    (sym.get_type(cursym) != SYM_UNDEFINEDSTRUCT)) {

                        const char *symName = sym.get_name(cursym);
                        bool error = true;
                        /*if (strstr(symName, "::") != NULL)
                        {
                        // Check if there is a non-struct-member version of this
                        // type (sometimes types are mangled when used in a struct
                        // when they shouldn't be)
                        int unmangledSym = sym.find(&strstr(symName, "::")[2]);
                        if ((unmangledSym > 0) && (sym.get_type(unmangledSym) == SYM_VARTYPE))
                        {
                        error = false;
                        cursym = unmangledSym;
                        }
                        }*/

                        if (error)
                        {
                            cc_error("Syntax error at '%s'; expected variable type", symName);
                            return -1;
                        }
                }
                if (cursym == sym.normalStringSym) {
                    cc_error("'string' not allowed inside struct");
                    return -1;
                }

                if (targ.peeknext() < 0) {
                    cc_error("Invalid syntax near '%s'", sym.get_friendly_name(cursym).c_str());
                    return -1;
                }

                if (check_not_eof(targ))
                    return -1;
                if (sym.entries[cursym].flags & SFLG_AUTOPTR) {
                    member_is_pointer = 1;
                }
                else if (strcmp(sym.get_name(targ.peeknext()), "*") == 0) {
                    member_is_pointer = 1;
                    targ.getnext();
                }
                else if (sym.get_type(cursym) == SYM_UNDEFINEDSTRUCT) {
                    cc_error("Invalid use of forward-declared struct");
                    return -1;
                }

                if ((sym.entries[cursym].flags & SFLG_STRUCTTYPE) && (member_is_pointer == 0)) {
                    cc_error("Member variable cannot be struct");
                    return -1;
                }
                if ((member_is_pointer) && (sym.entries[stname].flags & SFLG_MANAGED) && (!member_is_import)) {
                    cc_error("Member variable of managed struct cannot be pointer");
                    return -1;
                }
                else if ((sym.entries[cursym].flags & SFLG_MANAGED) && (!member_is_pointer)) {
                    cc_error("Cannot declare non-pointer of managed type");
                    return -1;
                }
                else if (((sym.entries[cursym].flags & SFLG_MANAGED) == 0) && (member_is_pointer)) {
                    cc_error("Cannot declare pointer to non-managed type");
                    return -1;
                }

                // run through all variables declared on this line
                do {
                    int vname = targ.getnext();
                    bool isDynamicArray = false;
                    if (sym.get_type(vname) == SYM_COMMA)
                        vname = targ.getnext();

                    if (sym.get_type(vname) == SYM_OPENBRACKET && sym.get_type(targ.peeknext()) == SYM_CLOSEBRACKET) {
                        isDynamicArray = true;
                        targ.getnext();
                        vname = targ.getnext();
                    }
                    bool isFunction = sym.get_type(targ.peeknext()) == SYM_OPENPARENTHESIS;
                    const char *memberExt = sym.get_name(vname);
                    memberExt = strstr(memberExt, "::");
                    if (!isFunction && sym.get_type(vname) == SYM_VARTYPE && vname > sym.normalFloatSym && memberExt == NULL) {
                        const char *new_name = get_member_full_name(stname, vname);
                        vname = sym_find_or_add(sym, new_name);
                    }
                    if (sym.get_type(vname) != 0 && (sym.get_type(vname) != SYM_VARTYPE || vname <= sym.normalFloatSym)) {
                        cc_error("'%s' is already defined",sym.get_friendly_name(vname).c_str());
                        return -1;
                    }
                    if (extendsWhat > 0) {
                        // check that we haven't already inherited a member
                        // with the same name
                        long member = vname;
                        if (memberExt == NULL) {
                            cc_error("Internal compiler error dbc");
                            return -1;
                        }
                        // skip the colons
                        memberExt += 2;
                        // find the member-name-only sym
                        member = sym.find(memberExt);
                        // if it's never referenced it won't exist, so create it
                        if (member < 1) {
                            member = sym.add_ex(memberExt, 0, 0);
                        }

                        if (find_member_sym(extendsWhat, &member, true) == 0) {
                            cc_error("'%s' already defined by inherited class", sym.get_friendly_name(member).c_str());
                            return -1;
                        }
                        // not found -- a good thing, but find_member_sym will
                        // have errored. Clear the error
                        ccError = 0;
                    }

                    if (isFunction) {
                        // member function
                        if (!member_is_import) {
                            cc_error("function in a struct requires the import keyword");
                            return -1;
                        }
                        if (member_is_writeprotected) {
                            cc_error("'writeprotected' does not apply to functions");
                            return -1;
                        }

                        if (process_function_declaration(targ, scrip, &vname, cursym, in_func,
                            nested_level, member_is_readonly, member_is_import, stname,
                            member_is_pointer, member_is_static, NULL, NULL, isDynamicArray))
                            return -1;

                        if (member_is_protected)
                            sym.entries[vname].flags |= SFLG_PROTECTED;

                        if (in_func >= 0) {
                            cc_error("Cannot define member function body inside struct");
                            return -1;
                        }

                    }
                    else if (isDynamicArray) {
                        // Someone tried to declare the function syntax for a dynamic array
                        // But there was no function declaration
                        cc_error("expected '('");
                        return -1;
                    }
                    else if ((member_is_import) && (!member_is_property)) {
                        // member variable cannot be an import
                        cc_error("'import' not valid in this context");
                        return -1;
                    }
                    else if ((member_is_static) && (!member_is_property)) {
                        cc_error("static variables not supported");
                        return -1;
                    }
                    else if ((cursym == stname) && (!member_is_pointer)) {
                        // cannot do  struct A { A a; }
                        // since we don't know the size of A, recursiveness
                        cc_error("struct '%s' cannot be a member of itself", sym.get_friendly_name(cursym).c_str());
                        return -1;
                    }
                    else {
                        // member variable
                        sym.entries[vname].stype = SYM_STRUCTMEMBER;
                        sym.entries[vname].extends = stname;  // save which struct it belongs to
                        sym.entries[vname].ssize = sym.entries[cursym].ssize;
                        sym.entries[vname].soffs = size_so_far;
                        sym.entries[vname].vartype = (short)cursym;
                        if (member_is_readonly)
                            sym.entries[vname].flags |= SFLG_READONLY;
                        if (member_is_property)
                            sym.entries[vname].flags |= SFLG_PROPERTY;
                        if (member_is_pointer) {
                            sym.entries[vname].flags |= SFLG_POINTER;
                            sym.entries[vname].ssize = 4;
                        }
                        if (member_is_static)
                            sym.entries[vname].flags |= SFLG_STATIC;
                        if (member_is_protected)
                            sym.entries[vname].flags |= SFLG_PROTECTED;
                        else if (member_is_writeprotected)
                            sym.entries[vname].flags |= SFLG_WRITEPROTECTED;

                        if (member_is_property) {
                            if (!member_is_import) {
                                cc_error("Property must be import");
                                return -1;
                            }
                            else {
                                sym.entries[vname].flags |= SFLG_IMPORTED;
                            }

                            const char *namePrefix = "";

                            if (sym.get_type(targ.peeknext()) == SYM_OPENBRACKET) {
                                // An indexed property!
                                targ.getnext();  // skip the [
                                if (sym.get_type(targ.getnext()) != SYM_CLOSEBRACKET) {
                                    cc_error("cannot specify array size for property");
                                    return -1;
                                }

                                sym.entries[vname].flags |= SFLG_ARRAY;
                                sym.entries[vname].arrsize = 0;
                                namePrefix = "i";
                            }
                            // the variable name will have been jibbled with
                            // the struct name added to it -- strip it back off
                            const char *memberPart = strstr(sym.get_name(vname), "::");
                            if (memberPart == NULL) {
                                cc_error("internal error: property has no struct name");
                                return -1;
                            }
                            // seek to the actual member name
                            memberPart += 2;

                            // declare the imports for the Get and Setters
                            char propFuncName[200];
                            sprintf(propFuncName, "%s::get%s_%s", sym.get_name(stname), namePrefix, memberPart);

                            int propGet = scrip->add_new_import(propFuncName);
                            int propSet = 0xffff;
                            if (!member_is_readonly) {
                                // setter only if it's not read-only
                                sprintf(propFuncName, "%s::set%s_%s", sym.get_name(stname), namePrefix, memberPart);
                                propSet = scrip->add_new_import(propFuncName);
                            }
                            sym.entries[vname].set_propfuncs(propGet, propSet);
                        }
                        else if (sym.get_type(targ.peeknext()) == SYM_OPENBRACKET) {
                            // An array!
                            targ.getnext();  // skip the [
                            int nextt = targ.getnext();
                            int array_size;

                            if (sym.get_type(nextt) == SYM_CLOSEBRACKET) {
                                if ((sym.entries[stname].flags & SFLG_MANAGED)) {
                                    cc_error("Member variable of managed struct cannot be dynamic array");
                                    return -1;
                                }
                                sym.entries[stname].flags |= SFLG_HASDYNAMICARRAY;
                                sym.entries[vname].flags |= SFLG_DYNAMICARRAY;
                                array_size = 0;
                                size_so_far += 4;
                            }
                            else {
                                if (accept_literal_or_constant_value(nextt, array_size, false, "Array size must be constant value") < 0) {
                                    return -1;
                                }

                                if (array_size < 1) {
                                    cc_error("array size cannot be less than 1");
                                    return -1;
                                }

                                size_so_far += array_size * sym.entries[vname].ssize;

                                if (sym.get_type(targ.getnext()) != SYM_CLOSEBRACKET) {
                                    cc_error("expected ']'");
                                    return -1;
                                }
                            }
                            sym.entries[vname].flags |= SFLG_ARRAY;
                            sym.entries[vname].arrsize = array_size;
                        }
                        else
                            size_so_far += sym.entries[vname].ssize;
                    }

                    // both functions and variables have this set
                    sym.entries[vname].flags |= SFLG_STRUCTMEMBER;

                } while (sym.get_type(targ.peeknext()) == SYM_COMMA) ;

                // line must end with semicolon
                if (sym.get_type(targ.getnext()) != SYM_SEMICOLON) {
                    cc_error("expected ';'");
                    return -1;
                }
            }
            // align struct on 4-byte boundary in keeping with compiler
            if ((size_so_far % 4) != 0) size_so_far += 4 - (size_so_far % 4);
            sym.entries[stname].ssize = size_so_far;
            // read in the }
            targ.getnext();
            if (sym.get_type(targ.getnext()) != SYM_SEMICOLON) {
                cc_error("missing semicolon after struct declaration");
                return -1;
            }
        }
        else if (symType == SYM_ENUM) {
            // enum eEnumName { value1, value2 };

            if (in_func >= 0) {
                cc_error("enum declaration not allowed here");
                return -1;
            }

            int enumName = targ.getnext();
            if (sym.get_type(enumName) != 0) {
                cc_error("'%s' is already defined",sym.get_friendly_name(enumName).c_str());
                return -1;
            }
            sym.entries[enumName].stype = SYM_VARTYPE;
            // standard int size
            sym.entries[enumName].ssize = 4;
            sym.entries[enumName].vartype = sym.normalIntSym;

            if (sym.get_type(targ.getnext()) != SYM_OPENBRACE) {
                cc_error("expected '{'");
                return -1;
            }

            int currentValue = 0;

            while (1) {
                if (check_not_eof(targ))
                    return -1;
                int nextOne = targ.getnext();

                if (sym.get_type(nextOne) == SYM_CLOSEBRACE) {
                    break;
                }
                else if (sym.get_type(nextOne) == 0) {

                    int declareVariableSym = nextOne;

                    // increment the value of the enum entry
                    currentValue++;

                    int nextSym = targ.getnext();

                    if (sym.get_type(nextSym) == SYM_ASSIGN) {
                        // a specifically indexed entry

                        bool is_neg = false;
                        if (sym.get_name(targ.peeknext())[0] == '-') {
                            is_neg = true;
                            targ.getnext();
                        }

                        nextSym = targ.getnext();

                        if (accept_literal_or_constant_value(nextSym, currentValue, is_neg, "enum must be set to literal value") < 0) {
                            return -1;
                        }

                        nextSym = targ.getnext();
                    }

                    // TODO: declare declareVariableSym as a const variable
                    sym.entries[declareVariableSym].stype = SYM_CONSTANT;
                    sym.entries[declareVariableSym].ssize = 4;
                    sym.entries[declareVariableSym].arrsize = 1;
                    sym.entries[declareVariableSym].vartype = enumName;
                    sym.entries[declareVariableSym].sscope = 0;
                    sym.entries[declareVariableSym].flags = SFLG_READONLY;
                    // soffs is unused for a constant, so in a gratiuitous
                    // hack we use it to store the enum's value
                    sym.entries[declareVariableSym].soffs = currentValue;


                    // proceed to check the next one
                    if (sym.get_type(nextSym) == SYM_CLOSEBRACE) {
                        break;
                    }
                    else if (sym.get_type(nextSym) != SYM_COMMA) {
                        cc_error("enum parse error at '%s'", sym.get_friendly_name(nextSym).c_str());
                        return -1;
                    }

                }
                else {
                    cc_error("unexpected '%s'", sym.get_friendly_name(nextOne).c_str());
                    return -1;
                }
            }

            if (sym.get_type(targ.getnext()) != SYM_SEMICOLON) {
                cc_error("expected ';'");
                return -1;
            }

        }
        else if (symType == SYM_BUILTIN) {
            next_is_builtin = 1;
            if (sym.get_type(targ.peeknext()) != SYM_MANAGED && sym.get_type(targ.peeknext()) != SYM_STRUCT) {
                cc_error("Invalid use of 'builtin'");
                return -1;
            }
        }
        else if (symType == SYM_MANAGED) {
            next_is_managed = 1;
            if (sym.get_type(targ.peeknext()) != SYM_STRUCT) {
                cc_error("Invalid use of 'managed'");
                return -1;
            }
        }
        else if (symType == SYM_AUTOPTR) {
            next_is_autoptr = 1;
            if (sym.get_type(targ.peeknext()) != SYM_MANAGED && sym.get_type(targ.peeknext()) != SYM_BUILTIN) {
                cc_error("Invalid use of 'autoptr'");
                return -1;
            }
        }
        else if (symType == SYM_STRINGSTRUCT) {
            next_is_stringstruct = 1;
            if (sym.stringStructSym > 0) {
                cc_error("stringstruct already defined");
                return -1;
            }
            if (sym.get_type(targ.peeknext()) != SYM_AUTOPTR) {
                cc_error("Invalid use of 'stringstruct'");
                return -1;
            }
        }
        else if (symType == SYM_IMPORT) {
            if (in_func >= 0) {
                cc_error("'import' not allowed inside function body");
                return -1;
            }

            if (strcmp(sym.get_name(cursym), "_tryimport") == 0)
                next_is_import = 2;
            else
                next_is_import = 1;

            if ((sym.get_type(targ.peeknext()) != SYM_VARTYPE) &&
                (sym.get_type(targ.peeknext()) != SYM_READONLY)) {
                    cc_error("expected variable or function after import, not '%s'", sym.get_friendly_name(targ.peeknext()).c_str());
                    return -1;
            }
        }
        else if (symType == SYM_STATIC) {
            if (in_func >= 0) {
                cc_error("'static' not allowed inside function body");
                return -1;
            }
            next_is_static = 1;
            if ((sym.get_type(targ.peeknext()) != SYM_VARTYPE) &&
                (sym.get_type(targ.peeknext()) != SYM_READONLY)) {
                    cc_error("expected variable or function after static");
                    return -1;
            }
        }
        else if (symType == SYM_PROTECTED) {
            if (in_func >= 0) {
                cc_error("'protected' not allowed here");
                return -1;
            }
            next_is_protected = 1;
            if ((sym.get_type(targ.peeknext()) != SYM_VARTYPE) &&
                (sym.get_type(targ.peeknext()) != SYM_STATIC) &&
                (sym.get_type(targ.peeknext()) != SYM_READONLY)) {
                    cc_error("expected function after protected");
                    return -1;
            }
        }
        else if (symType == SYM_READONLY) {
            next_is_readonly = 1;
            if (sym.get_type(targ.peeknext()) != SYM_VARTYPE) {
                cc_error("expected variable after readonly");
                return -1;
            }
        }
        else if (symType == SYM_CONST) {
            cc_error("'const' is only valid for function parameters (use 'readonly' instead)");
            return -1;
        }
        else if (symType == SYM_EXPORT) {
            // export specified symbol
            cursym = targ.getnext();
            while (sym.get_type(cursym) != SYM_SEMICOLON) {
                int nextype = sym.get_type(cursym);
                if (nextype == 0) {
                    cc_error("cannot export undefined symbol '%s'",sym.get_friendly_name(cursym).c_str());
                    return -1;
                }
                if ((nextype != SYM_GLOBALVAR) && (nextype != SYM_FUNCTION)) {
                    cc_error("invalid export symbol '%s'",sym.get_friendly_name(cursym).c_str());
                    return -1;
                }
                if (sym.entries[cursym].flags & SFLG_IMPORTED) {
                    cc_error("cannot export an import");
                    return -1;
                }
                if (sym.entries[cursym].flags & SFLG_ISSTRING) {
                    cc_error("cannot export string; use char[200] instead");
                    return -1;
                }
                // if all functions are being exported anyway, don't bother doing
                // it now
                if ((ccGetOption(SCOPT_EXPORTALL)!=0) && (nextype == SYM_FUNCTION));
                else if (scrip->add_new_export(sym.get_name(cursym),
                    (nextype == SYM_GLOBALVAR) ? EXPORT_DATA : EXPORT_FUNCTION,
                    sym.entries[cursym].soffs, sym.entries[cursym].sscope) == -1) {
                        return -1;
                }
                if (check_not_eof(targ))
                  return -1;
                cursym = targ.getnext();
                if (sym.get_type(cursym) == SYM_SEMICOLON) break;
                if (sym.get_type(cursym) != SYM_COMMA) {
                    cc_error("export parse error at '%s'",sym.get_friendly_name(cursym).c_str());
                    return -1;
                }
                cursym = targ.getnext();
            }
        }
        else if ((symType == SYM_VARTYPE) && (sym.get_type(targ.peeknext()) != SYM_DOT)) {
            // variable type, so what follows is a function or variable declaration
            int varsize = sym.entries[cursym].ssize;
            int vtwas = cursym;
            if ((nested_type[nested_level] == NEST_IFSINGLE) ||
                (nested_type[nested_level] == NEST_ELSESINGLE) ||
                (nested_type[nested_level] == NEST_DOSINGLE)) {
                    cc_error("Unexpected '%s'",sym.get_friendly_name(cursym).c_str());
                    return -1;
            }
            if ((nested_type[nested_level] == NEST_SWITCH)) {
                cc_error("Variable declaration may be skipped by case label. Use braces to limit its scope or move it outside the switch statement block");
                return -1;
            }

            next_is_noloopcheck = 0;
            int isPointer = 0;
            bool isDynamicArray = 0;
            int loopCheckOff = 0;

            if (check_not_eof(targ))
                return -1;
            if (strcmp(sym.get_name(targ.peeknext()), "*") == 0) {
                // only allow pointers to structs
                if ((sym.entries[vtwas].flags & SFLG_STRUCTTYPE) == 0) {
                    cc_error("Cannot create pointer to basic type");
                    return -1;
                }
                if (sym.entries[vtwas].flags & SFLG_AUTOPTR) {
                    cc_error("Invalid use of '*'");
                    return -1;
                }
                isPointer = 1;
                targ.getnext();
            }

            if (sym.entries[vtwas].flags & SFLG_AUTOPTR)
                isPointer = 1;

            int dynArrayStatus = check_for_dynamic_array_declaration(targ, vtwas, !!isPointer);
            if (dynArrayStatus < 0) return -1;
            if (dynArrayStatus > 0)
            {
                isDynamicArray = true;
            }

            if (sym.get_type(targ.peeknext()) == SYM_LOOPCHECKOFF) {
                targ.getnext();
                loopCheckOff = 1;
            }

startvarbit:
            if (check_not_eof(targ))
                return -1;
            cursym = targ.getnext();

            int member_function_definition = 0;
            int structSym = 0;

            int next_type = sym.get_type(targ.peeknext());
            if (next_type == SYM_MEMBERACCESS) {
                // defining member function,  Class::Function
                targ.getnext();
                int whichmember = targ.getnext();
                structSym = cursym;
                // change cursym to be the full function name
                const char *mfullname = get_member_full_name(cursym, whichmember);
                cursym = sym.find(mfullname);
                if (cursym < 0) {
                    cc_error("'%s' does not contain a function '%s'", sym.get_friendly_name(structSym).c_str(), sym.get_friendly_name(whichmember).c_str());
                    return -1;
                }
                isMemberFunction = structSym;
                next_type = sym.get_type(targ.peeknext());
                member_function_definition = 1;
            }

            SymbolDef oldDefinition;
            oldDefinition.stype = 0;
            bool isFunction = next_type == SYM_OPENPARENTHESIS;

            if (next_is_import != 1) {
                if (scrip->remove_any_import(sym.get_name(cursym), &oldDefinition))
                    return -1;
            }
            if (sym.get_type(cursym) != 0 && (!isFunction && !isMemberFunction || sym.get_type(cursym) != SYM_VARTYPE || cursym <= sym.normalFloatSym)) {
                cc_error("Variable '%s' is already defined",sym.get_friendly_name(cursym).c_str());
                return -1;
            }

            int isglobal = (in_func < 0) ? 1 : 0;
            if (next_is_import)
                isglobal = 2;

            if (isFunction) {
                // it's a function
                if (process_function_declaration(targ, scrip, &cursym, vtwas, in_func,
                    nested_level, next_is_readonly, next_is_import, structSym,
                    isPointer, next_is_static, &isMemberFunction, &oldDefinition, isDynamicArray))
                    return -1;

                // restore the flag, since remve_any_imports zeros it out
                if (member_function_definition)
                    sym.entries[cursym].flags |= SFLG_STRUCTMEMBER;
                else if (next_is_static) {
                    cc_error("'static' only applies to member functions");
                    return -1;
                }

                if (next_is_protected)
                    sym.entries[cursym].flags |= SFLG_PROTECTED;

                if (in_func >= 0)
                    inFuncSym = cursym;

                if (!next_is_import)
                    next_is_noloopcheck = loopCheckOff;
                else if (loopCheckOff) {
                    cc_error("'noloopcheck' cannot be applied to imported functions");
                    return -1;
                }

            } // end if function
            else if (member_function_definition) {
                cc_error("Expected '('");
                return -1;
            }
            else if (next_is_protected) {
                cc_error("'protected' not valid in this context");
                return -1;
            }
            else if (loopCheckOff) {
                cc_error("'noloopcheck' not valid in this context");
                return -1;
            }
            else {
                // variable declaration
                if (!isglobal)
                    // local variable declaration only
                    sym.entries[cursym].sscope = nested_level;
                if (next_is_readonly)
                    sym.entries[cursym].flags |= SFLG_READONLY;
                if (next_is_static) {
                    cc_error("Invalid use of 'static'");
                    return -1;
                }

                // parse the declaration
                int reslt = parse_variable_declaration(cursym,&next_type,isglobal,varsize,scrip,&targ,vtwas, isPointer);
                if (reslt < 0) return -1;
                if (reslt == 2) goto startvarbit;
            }
            next_is_import = 0;
            next_is_readonly = 0;
            next_is_static = 0;
            next_is_protected = 0;

            if (oldDefinition.stype) {
                // there was a forward declaration -- check that
                // the real declaration matches it
                ccError = 0;
                if (!isglobal)
                    cc_error("Local variable cannot have the same name as an import");
                else if (oldDefinition.stype != sym.entries[cursym].stype)
                    cc_error("Type of identifier differs from original declaration");
                else if (oldDefinition.flags != (sym.entries[cursym].flags & ~SFLG_IMPORTED))
                    cc_error("Attributes of identifier do not match prototype");
                else if (oldDefinition.ssize != sym.entries[cursym].ssize)
                    cc_error("Size of identifier does not match prototype");
                else if ((sym.entries[cursym].flags & SFLG_ARRAY) && (oldDefinition.arrsize != sym.entries[cursym].arrsize))
                    cc_error("Array size '%d' of identifier does not match prototype which is '%d'", sym.entries[cursym].arrsize, oldDefinition.arrsize);
                else if (oldDefinition.stype == SYM_FUNCTION) {
                    // function-only checks
                    if (oldDefinition.sscope != sym.entries[cursym].sscope)
                        cc_error("Function declaration has wrong number of arguments to prototype");
                    else {
                        // this is <= because the return type is the first one
                        for (int ii = 0; ii <= sym.entries[cursym].get_num_args(); ii++) {
                            if (oldDefinition.funcparamtypes[ii] != sym.entries[cursym].funcparamtypes[ii])
                                cc_error("Parameter type does not match prototype");

                            // copy the default values from the function prototype
                            sym.entries[cursym].funcParamDefaultValues[ii] = oldDefinition.funcParamDefaultValues[ii];
                            sym.entries[cursym].funcParamHasDefaultValues[ii] = oldDefinition.funcParamHasDefaultValues[ii];
                        }
                    }
                }
                if (ccError)
                    return -1;
            }

            continue;
        }
        else if (in_func < 0) {
            cc_error("Parse error: unexpected '%s'",sym.get_friendly_name(cursym).c_str());
            return -1;
        }
        else if (symType == 0) {
            char extratex[20] = "";
            const char *symname = sym.get_name(cursym);
            if ((symname[0] <= 32) || (symname[0] >= 128))
                sprintf (extratex, " (ASCII index %02X)", symname[0]);

            cc_error("Undefined token '%s' %s", symname, extratex);
            return -1;
        }
        else {
            long vnlist[TEMP_SYMLIST_LENGTH],lilen;
            int funcAtOffs;
            int targPosWas = targ.pos;
            lilen = extract_variable_name(cursym, &targ, &vnlist[0], &funcAtOffs);
            if (lilen < 0)
                return -1;

            // inside a function, so this is some code
            if ((sym.get_type(cursym) == SYM_FUNCTION) || (funcAtOffs > 0)) {
                // calling a function
                if (funcAtOffs > 0) {
                    // member function -- wind back to process whole expression
                    targ.pos = targPosWas;
                }

                targ.pos --;

                if (evaluate_expression(&targ,scrip,0,false) < 0)
                    return -1;

                if (sym.get_type(targ.getnext()) != SYM_SEMICOLON) {
                    cc_error("Expected ';'");
                    return -1;
                }

            }
            else if ((sym.get_type(targ.peeknext()) == SYM_ASSIGN) ||
                (sym.get_type(targ.peeknext()) == SYM_MASSIGN) ||
                (sym.get_type(targ.peeknext()) == SYM_SASSIGN)) {
                    // it's an assignment = or += -=
                    if (evaluate_assignment(&targ, scrip, false, cursym, lilen, vnlist, false)) {
                        return -1;
                    }
                    

            }
            else if (sym.get_type(cursym) == SYM_RETURN) {

                int functionReturnType = sym.entries[inFuncSym].funcparamtypes[0];

                if (sym.get_type(targ.peeknext()) != SYM_SEMICOLON) {
                    if (functionReturnType == sym.normalVoidSym) {
                        cc_error("Cannot return value from void function");
                        return -1;
                    }

                    // parse what is being returned
                    if (evaluate_expression(&targ,scrip,0,false) < 0)
                        return -1;
                    // convert into String if appropriate
                    PerformStringConversionInAX(scrip, &scrip->ax_val_type, functionReturnType);
                    // check return type is correct
                    if (check_type_mismatch(scrip->ax_val_type, functionReturnType, 1))
                        return -1;

                    if ((is_string(scrip->ax_val_type)) &&
                        (scrip->ax_val_scope == SYM_LOCALVAR)) {
                            cc_error("Cannot return local string from function");
                            return -1;
                    }
                }
                else if ((functionReturnType != sym.normalIntSym) && (functionReturnType != sym.normalVoidSym)) {
                    cc_error("Must return a '%s' value from function", sym.get_friendly_name(functionReturnType).c_str());
                    return -1;
                }
                else {
                    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
                }

                if (sym.get_type(targ.getnext()) != SYM_SEMICOLON) {
                    cc_error("Parse error in 'return' clause");
                    return -1;
                }
                // count total space taken by all local variables
                int totalsub = remove_locals (0, 1, scrip);

                if (totalsub > 0)
                    scrip->write_cmd2(SCMD_SUB,SREG_SP,totalsub);
                scrip->write_cmd(SCMD_RET);
                // We don't alter cur_sp since there can be code after the RETURN
            }
            else if ((sym.get_type(cursym) == SYM_IF) ||
                (sym.get_type(cursym) == SYM_WHILE)) {
                    // while has the same syntax as if, but store it like an Else
                    // so that it can't be followed by an "else"
                    int iswhile = (sym.get_type(cursym) == SYM_WHILE);
                    if (sym.get_type(targ.peeknext()) != SYM_OPENPARENTHESIS) {
                        cc_error("expected '('");
                        return -1;
                    }
                    long oriaddr = scrip->codesize;

                    if (evaluate_expression(&targ,scrip,1,false))
                        return -1;
                    // since AX will hold the result of the check, we can use JZ
                    // to determine whether to jump or not (0 means test failed, so
                    // skip content of "if" block)
                    scrip->write_cmd1(SCMD_JZ,0);
                    // the 0 will be fixed to a proper offset later
                    INC_NESTED_LEVEL;

                    if (sym.get_type(targ.peeknext()) == SYM_OPENBRACE) {
                        targ.getnext();
                        if (iswhile)
                            nested_type[nested_level] = NEST_ELSE;
                        else
                            nested_type[nested_level] = NEST_IF;
                    }
                    else if (iswhile)
                        nested_type[nested_level] = NEST_ELSESINGLE;
                    else
                        nested_type[nested_level] = NEST_IFSINGLE;

                    nested_start[nested_level] = 0;
                    nested_info[nested_level] = scrip->codesize-1;
                    if (iswhile)
                        nested_start[nested_level] = oriaddr;
                    continue;
            }
            else if (sym.get_type(cursym) == SYM_DO) {
                // We need a jump at a known location for the break command to work:
                scrip->write_cmd1(SCMD_JMP, 2); // Jump past the next jump :D
                scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the end of the loop
                INC_NESTED_LEVEL;

                if (sym.get_type(targ.peeknext()) == SYM_OPENBRACE) {
                    targ.getnext();
                    nested_type[nested_level] = NEST_DO;
                }
                else
                    nested_type[nested_level] = NEST_DOSINGLE;

                nested_start[nested_level] = scrip->codesize;
                nested_info[nested_level] = scrip->codesize - 1; // This is where we need to put the real address of the end of the loop
                continue;
            }
            else if (sym.get_type(cursym) == SYM_FOR) {
                INC_NESTED_LEVEL;
                nested_type[nested_level] = NEST_FOR;
                nested_start[nested_level] = 0;
                if (sym.get_type(targ.peeknext()) != SYM_OPENPARENTHESIS) {
                    cc_error("expected '('");
                    return -1;
                }
                targ.getnext(); // Skip the (
                cursym = targ.getnext();
                if (sym.get_type(cursym) != SYM_SEMICOLON) {
                    if(sym.get_type(cursym) == SYM_CLOSEPARENTHESIS) {
                        cc_error("Missing ';' inside for loop declaration");
                        return -1;
                    }
                    lilen = extract_variable_name(cursym, &targ, &vnlist[0], &funcAtOffs);
                    if (lilen < 0)
                        return -1;
                    if (sym.get_type(cursym) == SYM_VARTYPE) {
                        int varsize = sym.entries[cursym].ssize;
                        int vtwas = cursym;

                        int isPointer = 0;

                        if (strcmp(sym.get_name(targ.peeknext()), "*") == 0) {
                            // only allow pointers to structs
                            if ((sym.entries[vtwas].flags & SFLG_STRUCTTYPE) == 0) {
                                cc_error("Cannot create pointer to basic type");
                                return -1;
                            }
                            if (sym.entries[vtwas].flags & SFLG_AUTOPTR) {
                                cc_error("Invalid use of '*'");
                                return -1;
                            }
                            isPointer = 1;
                            targ.getnext();
                        }

                        if (sym.entries[vtwas].flags & SFLG_AUTOPTR)
                            isPointer = 1;

                        if (sym.get_type(targ.peeknext()) == SYM_LOOPCHECKOFF) {
                            cc_error("'noloopcheck' is not applicable in this context");
                            return -1;
                        }

                        int reslt;
                        // FIXME: This duplicates common variable declaration parsing at/around the "startvarbit" label
                        do {
                            cursym = targ.getnext();
                            if (cursym == SCODE_META) {
                                // eg. "int" was the last word in the file
                                currentline = targ.lineAtEnd;
                                cc_error("Unexpected end of file");
                                return -1;
                            }

                            int next_type = sym.get_type(targ.peeknext());
                            if (next_type == SYM_MEMBERACCESS || next_type == SYM_OPENPARENTHESIS) {
                                cc_error("Function declaration not allowed in for loop initialiser");
                                return -1;
                            }
                            else if (sym.get_type(cursym) != 0) {
                                cc_error("Variable '%s' is already defined",sym.get_name(cursym));
                                return -1;
                            }
                            else if (next_is_protected) {
                                cc_error("'protected' not valid in this context");
                                return -1;
                            }
                            else if (next_is_static) {
                                cc_error("Invalid use of 'static'");
                                return -1;
                            }
                            else {
                                // variable declaration
                                sym.entries[cursym].sscope = nested_level;
                                if (next_is_readonly)
                                    sym.entries[cursym].flags |= SFLG_READONLY;

                                // parse the declaration
                                reslt = parse_variable_declaration(cursym, &next_type, 0, varsize, scrip, &targ, vtwas, isPointer);
                                if (reslt < 0) return -1;
                            }
                        }
                        while(reslt == 2);
                    }
                    else
                        if (evaluate_assignment(&targ, scrip, false, cursym, lilen, vnlist, false))
                            return -1;
                }
                long oriaddr = scrip->codesize;
                bool hasLimitCheck;
                if (sym.get_type(targ.peeknext()) != SYM_SEMICOLON) {
                    if(sym.get_type(targ.peeknext()) == SYM_CLOSEPARENTHESIS) {
                        cc_error("Missing ';' inside for loop declaration");
                        return -1;
                    }
                    hasLimitCheck = true;
                    if (evaluate_expression(&targ,scrip,0,false))
                        return -1;
                    if (sym.get_type(targ.peeknext()) != SYM_SEMICOLON) {
                        cc_error("expected ';'");
                        return -1;
                    }
                }
                else
                    hasLimitCheck = false; // Allow for loops without limit checks
                long assignaddr = scrip->codesize;
                int pre_fixup_count = scrip->numfixups;
                targ.getnext(); // Skip the ;
                cursym = targ.getnext();
                if (sym.get_type(cursym) != SYM_CLOSEPARENTHESIS) {
                    lilen = extract_variable_name(cursym, &targ, &vnlist[0], &funcAtOffs);
                    if (lilen < 0)
                        return -1;
                    if (evaluate_assignment(&targ, scrip, true, cursym, lilen, vnlist, true))
                        return -1;
                } // Allow for loops without increments
                INC_NESTED_LEVEL;
                yank_chunk(scrip, &nested_chunk[nested_level], assignaddr, pre_fixup_count);
                if(!hasLimitCheck)
                    scrip->write_cmd2(SCMD_LITTOREG,SREG_AX,1); // Fake out the AX register so that jump works correctly
                // since AX will hold the result of the check, we can use JZ
                // to determine whether to jump or not (0 means test failed, so
                // skip content of "if" block)
                scrip->write_cmd1(SCMD_JZ,0);
                // the 0 will be fixed to a proper offset later
                if (sym.get_type(targ.peeknext()) == SYM_OPENBRACE) {
                    targ.getnext();
                    nested_type[nested_level] = NEST_ELSE;
                }
                else
                    nested_type[nested_level] = NEST_ELSESINGLE;
                nested_info[nested_level] = scrip->codesize-1;
                nested_start[nested_level] = oriaddr;
                continue;
            }
            else if (sym.get_type(cursym) == SYM_SWITCH) {
                if(sym.get_type(targ.peeknext()) != SYM_OPENPARENTHESIS) {
                    cc_error("expected '('");
                    return -1;
                }
                INC_NESTED_LEVEL;
                if (evaluate_expression(&targ,scrip,1,false)) // switch() expression
                    return -1;
                // Store the variable type to enforce it later
                nested_info[nested_level] = scrip->ax_val_type;
                nested_type[nested_level] = NEST_SWITCH;
                // Copy the result to the BX register, ready for case statements
                scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
                scrip->flush_line_numbers();
                nested_start[nested_level] = scrip->codesize;
                scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the lookup table
                scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to beyond the switch statement (for break)
                if(sym.get_type(targ.peeknext()) != SYM_OPENBRACE) {
                    cc_error("expected '{'");
                    return -1;
                }
                nested_assign_addr[nested_level] = -1; // Location of default: label
                targ.getnext();
                if(targ.peeknext() == SCODE_META) {
                    currentline = targ.lineAtEnd;
                    cc_error("Unexpected end of file");
                    return -1;
                }
                if(sym.get_type(targ.peeknext()) != SYM_CASE && sym.get_type(targ.peeknext()) != SYM_DEFAULT && sym.get_type(targ.peeknext()) != SYM_CLOSEBRACE) {
                    cc_error("Invalid keyword '%s' in switch statement block", sym.get_name(targ.peeknext()));
                    return -1;
                }
            }
            else if ((sym.get_type(cursym) == SYM_CASE) ||
                (sym.get_type(cursym) == SYM_DEFAULT)) {
                if(nested_type[nested_level] != NEST_SWITCH) {
                    cc_error("Case label not valid outside switch statement block");
                    return -1;
                }
                if (sym.get_type(cursym) == SYM_DEFAULT) {
                    if(nested_assign_addr[nested_level] != -1) {
                        cc_error("Multiple default labels in a switch statement block");
                        return -1;
                    }
                    nested_assign_addr[nested_level] = scrip->codesize;
                }
                else {
                    int oriaddr = scrip->codesize;
                    int orifixupcount = scrip->numfixups;
                    int vcpuOperator = SCMD_ISEQUAL;
                    // Push the switch variable onto the stack
                    scrip->push_reg(SREG_BX);
                    if (evaluate_expression(&targ,scrip,0,false)) // case n: label expression, result is in AX
                        return -1;
                    if (check_type_mismatch(scrip->ax_val_type, nested_info[nested_level], 0))
                        return -1;
                    if (check_operator_valid_for_type(&vcpuOperator, scrip->ax_val_type, nested_info[nested_level]))
                        return -1;
                    // Pop the switch variable, ready for comparison
                    scrip->pop_reg(SREG_BX);
                    yank_chunk(scrip, &nested_chunk[nested_level], oriaddr, orifixupcount);
                }
                if(sym.get_type(targ.peeknext()) != SYM_LABEL) {
                    cc_error("expected ':'");
                    return -1;
                }
                targ.getnext();
            }
            else if (sym.get_type(cursym) == SYM_BREAK) {
                int loop_level;
                loop_level = nested_level;
                while(loop_level > 0 && nested_start[loop_level] == 0)
                    loop_level--;
                if (loop_level > 0) {
                    if (sym.get_type(targ.getnext()) != SYM_SEMICOLON) {
                        cc_error("expected ';'");
                        return -1;
                    }
                    int totalsub = remove_locals(loop_level - 1, 1, scrip);
                    if (totalsub > 0)
                        scrip->write_cmd2(SCMD_SUB,SREG_SP,totalsub);
                    scrip->flush_line_numbers();
                    scrip->write_cmd2(SCMD_LITTOREG,SREG_AX,0); // Clear out the AX register so that the jump works correctly
                    if (nested_type[loop_level] == NEST_SWITCH)
                        scrip->write_cmd1(SCMD_JMP, -(scrip->codesize - nested_start[loop_level])); // Jump to the known break point
                    else
                        scrip->write_cmd1(SCMD_JMP, -(scrip->codesize - nested_info[loop_level] + 3)); // Jump to the known break point
                }
                else {
                    cc_error("Break only valid inside a loop or switch statement block");
                    return -1;
                }
            }
            else if (sym.get_type(cursym) == SYM_CONTINUE) {
                int loop_level;
                loop_level = nested_level;
                while(loop_level > 0 && (nested_start[loop_level] == 0 || nested_type[loop_level] == NEST_SWITCH))
                    loop_level--;
                if (loop_level > 0) {
                    if (sym.get_type(targ.getnext()) != SYM_SEMICOLON) {
                        cc_error("expected ';'");
                        return -1;
                    }
                    int totalsub = remove_locals(loop_level - 1, 1, scrip);
                    if (totalsub > 0)
                        scrip->write_cmd2(SCMD_SUB,SREG_SP,totalsub);
                    // if it's a for loop, drop the yanked chunk (loop increment) back in
                    if(nested_chunk[loop_level].size() > 0)
                        write_chunk(scrip, nested_chunk[loop_level][0]);
                    scrip->flush_line_numbers();
                    scrip->write_cmd2(SCMD_LITTOREG,SREG_AX,0); // Clear out the AX register so that the jump works correctly
                    scrip->write_cmd1(SCMD_JMP, -((scrip->codesize+2) - nested_start[loop_level])); // Jump to the start of the loop
                }
                else {
                    cc_error("Continue not valid outside a loop");
                    return -1;
                }
            }
            else {
                cc_error("PE04: parse error at '%s'",sym.get_friendly_name(cursym).c_str());
                return -1;
            }
            // sort out jumps when a single-line if or else has finished
            while ((nested_type[nested_level] == NEST_IFSINGLE) ||
                (nested_type[nested_level] == NEST_ELSESINGLE) ||
                (nested_type[nested_level] == NEST_DOSINGLE)) {
                    if (nested_type[nested_level] == NEST_DOSINGLE)
                    {
                        if (deal_with_end_of_do(nested_info,nested_start,scrip,&targ,&nested_level))
                            return -1;
                    }
                    else
                        if (deal_with_end_of_ifelse(nested_type,nested_info,nested_start,scrip,&targ,&nested_level,nested_chunk))
                            break;
            }
        }
    }
    if ((in_func >= 0) || (nested_level > 0)) {
        currentline = targ.lineAtEnd;
        cc_error("Function still open, missing }");
        return -1;
    }
    return 0;
}


// compile the specified code into the specified struct
int cc_compile(const char*inpl, ccCompiledScript*scrip) {
    int toret = 0;
    /* this malloc might not alloc enough memory
    char*mainbuf=(char*)malloc(strlen(inpl)+5000);
    ccError = 0;
    // run the preprocessor on the code
    cc_preprocess(inpl, mainbuf);
    if (ccError) return -1;
    // now, compile the preprocessed code
    if (__cc_compile_file(mainbuf,scrip))
    toret=-1;
    free(mainbuf);*/

    if (__cc_compile_file(inpl,scrip))
        toret=-1;
    return toret;
}


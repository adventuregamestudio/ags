//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script/cc_compiledscript.h"
#include "script/cc_internal.h"       // macro definitions
#include "script/cc_symboltable.h"     // symbolTable
#include "script/cc_common.h"      // ccGetOption

void ccCompiledScript::write_cmd(int cmdd) {
    write_code(cmdd);
}

void ccCompiledScript::write_cmd1(int cmdd,int param) {
    write_code(cmdd);
    write_code(param);
}
void ccCompiledScript::write_cmd2(int cmdd,int param,int param2) {
    write_code(cmdd);
    write_code(param);
    write_code(param2);
}
void ccCompiledScript::write_cmd3(int cmdd,int param,int param2,int param3) {
    write_code(cmdd);
    write_code(param);
    write_code(param2);
    write_code(param3);
}

void ccCompiledScript::push_reg(int regg) {
    write_cmd1(SCMD_PUSHREG,regg);
    cur_sp += 4;
}

void ccCompiledScript::pop_reg(int regg) {
    write_cmd1(SCMD_POPREG,regg);
    cur_sp -= 4;
}

int ccCompiledScript::add_global(int size, const char*vall) {
    size_t old_size = globaldata.size();
    globaldata.resize(old_size + size);
    if (vall && size > 0)
        memcpy(&globaldata[old_size], vall, size);
    return old_size;
}

int ccCompiledScript::add_string(const char *strr) {
    size_t len = strlen(strr);
    size_t old_size = strings.size();
    strings.resize(old_size + len + 1);
    char *write_ptr = &strings[0] + old_size;
    for (size_t src = 0; src <= len; ++src) {
        char ch = strr[src];
        if (ch=='\\') {
            src++;
            ch = strr[src];

            if (ch == '[') { // pass through as \[
                *write_ptr = '\\';
                write_ptr++;
            } else {
                ch = GetEscapedChar(ch);
            }
        }
        *write_ptr = ch;
        write_ptr++;
    }
    return old_size;
}

void ccCompiledScript::add_fixup(int32_t locc, char ftype) {
    fixups.push_back(locc);
    fixuptypes.push_back(ftype);
}

void ccCompiledScript::fixup_previous(char ftype) {
    add_fixup(code.size() - 1,ftype);
}

int ccCompiledScript::add_new_function(const char*namm, int *idx) {
    functions.push_back(namm);
    funccodeoffs.push_back(code.size());
    funcnumparams.push_back(0);
    if (idx)
        *idx = functions.size() - 1;
    return code.size();
}

int ccCompiledScript::add_new_import(const char*namm)
{
    imports.push_back(namm);
    return imports.size() - 1;
}

int ccCompiledScript::find_or_add_import(const char *namm)
{
    for (size_t i = 0; i < imports.size(); i++)
    {
        if (strcmp(imports[i].c_str(), namm) == 0)
        {
            return i;
        }
    }
    
    return add_new_import(namm);
}

int ccCompiledScript::remove_any_import (const char*namm, SymbolDef *oldSym) {
    // Remove any import with the specified name
    int sidx = sym.find(namm);
    if (sidx < 0)
        return 0;
    if ((sym.entries[sidx].flags & SFLG_IMPORTED) == 0)
        return 0;
    // if this import has been referenced, flag an error
    if (sym.entries[sidx].flags & SFLG_ACCESSED) {
        cc_error("Already referenced name '%s' as import; you must define it before using it", namm);
        return -1;
    }
    // if they set the No Override Imports flag, don't allow it
    if (ccGetOption(SCOPT_NOIMPORTOVERRIDE)) {
        cc_error("Variable '%s' is already imported", namm);
        return -1;
    }

    if (oldSym) {
        // Copy the import declaration to a backup struct
        // This allows a type comparison to be done
        // strip the imported flag, since it the real def won't be
        oldSym->flags = sym.entries[sidx].flags & ~SFLG_IMPORTED;
        oldSym->stype = sym.entries[sidx].stype;
        oldSym->sscope = sym.entries[sidx].sscope;
        // Return size may have been unknown at the time of forward declaration. Check the actual return type for those cases.
        if(sym.entries[sidx].stype == SYM_FUNCTION && sym.entries[sidx].ssize == 0) {
            oldSym->ssize = sym.entries[sym.entries[sidx].funcparams[0].Type & ~(STYPE_POINTER | STYPE_DYNARRAY)].ssize;
        } else {
            oldSym->ssize = sym.entries[sidx].ssize;
        }
        oldSym->arrsize = sym.entries[sidx].arrsize;
        if (sym.entries[sidx].stype == SYM_FUNCTION) {
            // <= because of return type
            oldSym->funcparams = sym.entries[sidx].funcparams;
        }
    }

    // remove its type so that it can be declared
    sym.entries[sidx].stype = 0;
    sym.entries[sidx].flags = 0;

    // check also for a number-of-parameters appended version
    char appended[200];
    sprintf(appended, "%s^", namm);
    int applen = strlen(appended);

    for (size_t i = 0; i < imports.size(); i++) {
        if (strcmp(imports[i].c_str(), namm) == 0) {
            // Just null the name of the import
            // DO NOT remove the import from the list, as some other
            // import indexes might already be referenced by the code
            // compiled so far.
            imports[i] = std::string();
        }
        else if (strncmp(imports[i].c_str(), appended, applen) == 0) {
            imports[i] = std::string();
        }
    }
    return 0;
}

int ccCompiledScript::add_new_export(const char*namm, int etype, int32_t eoffs, int numArgs)
{
    if (eoffs >= 0x00ffffff) {
        cc_error("export offset too high; script data size too large?");
        return -1;
    }

    std::string newName = namm;
    // mangle the name for functions to record parameters
    if (etype == EXPORT_FUNCTION) {
        char tempAddition[20];
        sprintf(tempAddition, "$%d", numArgs);
        newName.append(tempAddition);
    }
    // Check if it's already exported
    for (size_t i = 0; i < exports.size(); i++) {
        if (strcmp(exports[i].c_str(), newName.c_str()) == 0) {
            return i;
        }
    }
    exports.push_back(newName);
    export_addr.push_back(eoffs | ((int32_t)etype << 24L));
    return exports.size() - 1;
}

void ccCompiledScript::flush_line_numbers() {
    if (next_line) {
        int linum = next_line;
        next_line = 0;
        write_code(SCMD_LINENUM);
        write_code(linum);
    }
}

void ccCompiledScript::write_code(int32_t byy) {
    flush_line_numbers();
    code.push_back(byy);
}

void ccCompiledScript::start_new_section(const char *name) {

    if ((sectionNames.size() == 0) ||
        (code.size() != sectionOffsets.back()))
    {
        sectionNames.push_back(name);
        sectionOffsets.push_back(code.size());
    }
    else
    {
        // nothing was in the last section, so overwrite it with this new one
        sectionNames.back() = name;
        sectionOffsets.back() = code.size();
    }
}

void ccCompiledScript::free_extra() {
    functions = {};
    funccodeoffs = {};
    funcnumparams = {};
}

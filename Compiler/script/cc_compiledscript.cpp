#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_compiledscript.h"
#include "script/script_common.h"       // macro definitions
#include "cc_symboltable.h"     // symbolTable
#include "script/cc_options.h"      // ccGetOption
#include "script/cc_error.h"

void ccCompiledScript::write_cmd(int cmdd)
{
    write_code(cmdd);
}

void ccCompiledScript::write_cmd1(int cmdd, int param)
{
    write_code(cmdd);
    write_code(param);
}

void ccCompiledScript::write_cmd2(int cmdd, int param, int param2)
{
    write_code(cmdd);
    write_code(param);
    write_code(param2);
}

void ccCompiledScript::write_cmd3(int cmdd, int param, int param2, int param3)
{
    write_code(cmdd);
    write_code(param);
    write_code(param2);
    write_code(param3);
}

void ccCompiledScript::push_reg(int regg)
{
    write_cmd1(SCMD_PUSHREG, regg);
    cur_sp += 4;
}

void ccCompiledScript::pop_reg(int regg)
{
    write_cmd1(SCMD_POPREG, regg);
    cur_sp -= 4;
}

ccCompiledScript::ccCompiledScript()
{
    init();
    ax_val_type = 0;
    ax_val_scope = 0;
}

ccCompiledScript::~ccCompiledScript()
{
    shutdown();
}


// [fw] Note: Existing callers expected this function to return < 0 on overflow
// [fw] TODO: All this should become an STL vector for automatic memory management
int ccCompiledScript::add_global(int siz, const char *vall)
{
    // The new global variable will be moved to &(globaldata[offset])
    int offset = globaldatasize;

    // Extend global data to make space for the new variable; 
    // note that this may change globaldata
    globaldatasize += siz;
    void *new_memoryspace = realloc(globaldata, globaldatasize);
    if (!new_memoryspace)
    {
        // Memory overflow. Note, STL says: "The old memory block is not freed"
        free((void *)globaldata);
        globaldata = nullptr;
        return -1;
    }
    globaldata = (char *)new_memoryspace;

    if (vall != NULL)
    {
        memcpy(&(globaldata[offset]), vall, siz); // move the global into the new space
    }
    else
    {
        memset(&(globaldata[offset]), 0, siz); // fill the new space with 0-bytes
    }

    return offset;
}

int ccCompiledScript::add_string(const char*strr)
{
    strings = (char*)realloc(strings, stringssize + strlen(strr) + 5);
    unsigned int la, opi = 0;
    for (la = 0; la <= strlen(strr); la++)
    {
        char ch = strr[la];
        if (strr[la] == '\\')
        {
            la++;
            ch = strr[la];
            if (strr[la] == 'n') { ch = 10; }
            else if (strr[la] == 'r') { ch = 13; }
            else if (strr[la] == '[')
            { // pass through as \[
                strings[stringssize + opi] = '\\';
                opi++;
            }
        }
        strings[stringssize + opi] = ch;
        opi++;
    }
    //  memcpy(&strings[stringssize],strr,strlen(strr)+1);
    int toret = stringssize;
    stringssize += strlen(strr) + 1;
    return toret;
}

void ccCompiledScript::add_fixup(int32_t locc, char ftype) {
    fixuptypes = (char*)realloc(fixuptypes, numfixups + 5);
    fixups = (int32_t*)realloc(fixups, (numfixups * sizeof(int32_t)) + 10);
    fixuptypes[numfixups] = ftype;
    fixups[numfixups] = locc;
    numfixups++;
}

void ccCompiledScript::fixup_previous(char ftype)
{
    add_fixup(codesize - 1, ftype);
}

int ccCompiledScript::add_new_function(const char*namm, int *idx)
{
    if (numfunctions >= MAX_FUNCTIONS) return -1;

    functions[numfunctions] = (char*)malloc(strlen(namm) + 20);
    strcpy(functions[numfunctions], namm);
    funccodeoffs[numfunctions] = codesize;
    funcnumparams[numfunctions] = 0;
    if (idx)
        *idx = numfunctions;
    numfunctions++;
    return codesize;
}

int ccCompiledScript::add_new_import(const char *namm)
{
    if (numimports >= importsCapacity)
    {
        importsCapacity += 1000;
        imports = (char**)realloc(imports, sizeof(char*) * importsCapacity);
    }
    imports[numimports] = (char*)malloc(strlen(namm) + 12);
    strcpy(imports[numimports], namm);
    numimports++;
    return numimports - 1;
}

// Remove any import with the specified name, using the deprecated SymbolDef
// NOTE: Do not use this in new code; this is slated to die and go away 
// (Rationale: Does more than the name implies; does two scantly related things)
int ccCompiledScript::remove_any_import(const char *namm, SymbolDef *oldSym)
{
    int i, sidx;
    sidx = sym.find(namm);
    if (sidx < 0)
        return 0;
    if ((sym.entries[sidx].flags & SFLG_IMPORTED) == 0)
        return 0;

    // if this import has been referenced, flag an error
    if (sym.entries[sidx].flags & SFLG_ACCESSED)
    {
        cc_error("Already referenced name as import; you must define it before using it");
        return -1;
    }

    // if they set the No Override Imports flag, don't allow it
    if (ccGetOption(SCOPT_NOIMPORTOVERRIDE))
    {
        cc_error("Variable '%s' is already imported", namm);
        return -1;
    }

    if (oldSym)
    {
        // Copy the import declaration to a backup struct
        // This allows a type comparison to be done
        // strip the imported flag, since it the real def won't be
        oldSym->flags = sym.entries[sidx].flags & ~SFLG_IMPORTED;
        oldSym->stype = sym.entries[sidx].stype;
        oldSym->sscope = sym.entries[sidx].sscope;
        // Return size may have been unknown at the time of forward declaration. Check the actual return type for those cases.
        if (sym.entries[sidx].stype == SYM_FUNCTION && sym.entries[sidx].ssize == 0)
        {
            oldSym->ssize = sym.entries[sym.entries[sidx].funcparamtypes[0] & ~(STYPE_POINTER | STYPE_DYNARRAY)].ssize;
        }
        else
        {
            oldSym->ssize = sym.entries[sidx].ssize;
        }
        oldSym->arrsize = sym.entries[sidx].arrsize;
        if (sym.entries[sidx].stype == SYM_FUNCTION)
        {
            // <= because of return type
            for (i = 0; i <= sym.entries[sidx].get_num_args(); i++)
            {
                oldSym->funcparamtypes[i] = sym.entries[sidx].funcparamtypes[i];
                oldSym->funcParamDefaultValues[i] = sym.entries[sidx].funcParamDefaultValues[i];
                oldSym->funcParamHasDefaultValues[i] = sym.entries[sidx].funcParamHasDefaultValues[i];
            }
        }
    }

    // remove its type so that it can be declared
    sym.entries[sidx].stype = 0;
    sym.entries[sidx].flags = 0;

    // check also for a number-of-parameters appended version
    char appended[200];
    sprintf(appended, "%s^", namm);
    int applen = strlen(appended);

    for (i = 0; i < numimports; i++)
    {
        if (strcmp(imports[i], namm) == 0)
        {
            // Just null the name of the import
            // DO NOT remove the import from the list, as some other
            // import indexes might already be referenced by the code
          // compiled so far.
            imports[i][0] = 0;
        }
        else if (strncmp(imports[i], appended, applen) == 0)
        {
            imports[i][0] = 0;
        }

    }
    return 0;
}


// Copy the symbol table entry of an import (or forward) declaration
int ccCompiledScript::copy_import_symbol_table_entry(ags::Symbol_t idx, SymbolTableEntry *dest)
{
    size_t entries_idx = idx;
    dest->flags = sym.entries[entries_idx].flags;
    dest->stype = sym.entries[entries_idx].stype;
    dest->sscope = sym.entries[entries_idx].sscope;
    dest->ssize = sym.entries[entries_idx].ssize;
    // Return size may have been unknown at the time of forward declaration. Check the actual return type for those cases.
    if (dest->ssize == 0 && sym.entries[entries_idx].stype == SYM_FUNCTION)
    {
        dest->ssize = sym.entries[sym.entries[entries_idx].funcparamtypes[0] & ~(STYPE_POINTER | STYPE_DYNARRAY)].ssize;
    }
    dest->arrsize = sym.entries[entries_idx].arrsize;

    if (sym.entries[entries_idx].stype == SYM_FUNCTION)
    {
        for (size_t arg = 0; static_cast<int>(arg) <= sym.entries[entries_idx].get_num_args(); arg++)
        {
            dest->funcparamtypes.push_back(sym.entries[entries_idx].funcparamtypes.at(arg));
            dest->funcParamDefaultValues.push_back(sym.entries[entries_idx].funcParamDefaultValues.at(arg));
            dest->funcParamHasDefaultValues.push_back(sym.entries[entries_idx].funcParamHasDefaultValues.at(arg));
        }
    }
    return 0;
}

// Remove any import with the specified name, using the modern SymbolTableEntry
int ccCompiledScript::just_remove_any_import(ags::Symbol_t idx)
{
    const char *n = sym.get_name(idx);
    if (n == nullptr) return 0;
    std::string name = n;
    std::string name_with_hat = name;
    name_with_hat.push_back('^');

    if ((sym.entries[idx].flags & SFLG_IMPORTED) == 0) return 0;

    // if this import has been referenced, flag an error
    if (sym.entries[idx].flags & SFLG_ACCESSED)
    {
        cc_error("Already referenced name as import; you must define it before using it");
        return -1;
    }

    // if they set the No Override Imports flag, don't allow it
    if (ccGetOption(SCOPT_NOIMPORTOVERRIDE))
    {
        cc_error("Variable '%s' is already imported", sym.get_name(idx));
        return -1;
    }

    // remove its type so that it can be declared
    sym.entries[idx].stype = 0;
    sym.entries[idx].flags = 0;

    // check also for a number-of-parameters appended version

    for (size_t imports_idx = 0; static_cast<int>(imports_idx) < numimports; imports_idx++)
    {
        if ((name.compare(imports[imports_idx]) == 0) ||
            (strncmp(imports[imports_idx], name_with_hat.c_str(), name_with_hat.length()) == 0))
        {
            // Just null the name of the import
            // DO NOT remove the import from the list, as some other
            // import indexes might already be referenced by the code
            // compiled so far.
            imports[imports_idx][0] = '\0';
        }
    }
    return 0;
}

int ccCompiledScript::add_new_export(const char*namm, int etype, long eoffs, int numArgs)
{
    if (numexports >= exportsCapacity)
    {
        exportsCapacity += 1000;
        exports = (char**)realloc(exports, sizeof(char*) * exportsCapacity);
        export_addr = (int32_t*)realloc(export_addr, sizeof(int32_t) * exportsCapacity);
    }
    if (eoffs >= 0x00ffffff)
    {
        cc_error("export offset too high; script data size too large?");
        return -1;
    }
    char *newName = (char*)malloc(strlen(namm) + 20);
    strcpy(newName, namm);
    // mangle the name for functions to record parameters
    if (etype == EXPORT_FUNCTION)
    {
        char tempAddition[20];
        sprintf(tempAddition, "$%d", numArgs);
        strcat(newName, tempAddition);
    }
    // Check if it's already exported
    for (int aa = 0; aa < numexports; aa++)
    {
        if (strcmp(exports[aa], newName) == 0)
        {
            free(newName);
            return aa;
        }
    }
    exports[numexports] = newName;

    export_addr[numexports] = eoffs | ((long)etype << 24L);
    numexports++;
    return numexports - 1;
}

void ccCompiledScript::flush_line_numbers() 
{
    if (next_line)
    {
        int linum = next_line;
        next_line = 0;
        write_code(SCMD_LINENUM);
        write_code(linum);
    }
}

void ccCompiledScript::write_code(intptr_t byy) 
{
    /*
    // [fw] For DEBUGGING. Put a breakpoint on the "iii" line to have
    //      flow halt when bytecode is compiled for a specific code[] index.
    if (codesize == 28)
    {
        int iii = 0;
    }
    */
    flush_line_numbers();
    if (codesize >= codeallocated - 2)
    {
        codeallocated += 500;
        code = (intptr_t*)realloc(code, codeallocated * sizeof(intptr_t));
    }
    code[codesize] = byy;
    codesize++;
}

const char* ccCompiledScript::start_new_section(const char *name) 
{

    if ((numSections == 0) ||
        (codesize != sectionOffsets[numSections - 1]))
    {
        if (numSections >= capacitySections)
        {
            capacitySections += 100;
            sectionNames = (char**)realloc(sectionNames, sizeof(char*) * capacitySections);
            sectionOffsets = (int32_t*)realloc(sectionOffsets, sizeof(int32_t) * capacitySections);
        }
        sectionNames[numSections] = (char*)malloc(strlen(name) + 1);
        strcpy(sectionNames[numSections], name);
        sectionOffsets[numSections] = codesize;

        numSections++;
    }
    else
    {
        // nothing was in the last section, so overwrite it with this new one
        free(sectionNames[numSections - 1]);
        sectionNames[numSections - 1] = (char*)malloc(strlen(name) + 1);
        strcpy(sectionNames[numSections - 1], name);
    }

    return sectionNames[numSections - 1];
}

void ccCompiledScript::init() 
{
    globaldata = NULL;
    globaldatasize = 0;
    code = NULL;
    codesize = 0;
    codeallocated = 0;
    numfunctions = 0;
    strings = NULL;
    stringssize = 0;
    cur_sp = 0;
    fixups = NULL;
    fixuptypes = NULL;
    numfixups = 0;
    numimports = 0;
    numexports = 0;
    numSections = 0;
    importsCapacity = 0;
    imports = NULL;
    exportsCapacity = 0;
    exports = NULL;
    export_addr = NULL;
    capacitySections = 0;
    sectionNames = NULL;
    sectionOffsets = NULL;
    next_line = 0;
}

// free the extra bits that ccScript doesn't have
void ccCompiledScript::free_extra() 
{
    int aa;
    for (aa = 0; aa < numfunctions; aa++)
    {
        free(functions[aa]);
    }
    numfunctions = 0;
}

void ccCompiledScript::shutdown() 
{
    free_extra();
}



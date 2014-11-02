
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_compiledscript.h"
#include "script/script_common.h"       // macro definitions
#include "cc_symboltable.h"     // symbolTable
#include "script/cc_options.h"      // ccGetOption
#include "script/cc_error.h"

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

ccCompiledScript::ccCompiledScript() {
    init();
    ax_val_type = 0;
    ax_val_scope = 0;
}
ccCompiledScript::~ccCompiledScript() {
    shutdown();
}

int ccCompiledScript::add_global(int siz,char*vall) {
    //  printf("Add global size %d at %d\n",siz,globaldatasize);
    //  if (remove_any_import (vall)) return -2;
    globaldata = (char*)realloc(globaldata, globaldatasize + siz);
    if (vall != NULL)
        memcpy(&globaldata[globaldatasize],vall,siz);
    else memset(&globaldata[globaldatasize],0,siz);
    int toret = globaldatasize;
    globaldatasize += siz;
    return toret;
}
int ccCompiledScript::add_string(char*strr) {
    strings = (char*)realloc(strings, stringssize + strlen(strr) + 5);
    unsigned int la,opi=0;
    for (la = 0; la <= strlen(strr); la++) {
        if (strr[la]=='\\') {
            la++;
            if (strr[la] == 'n') strr[la]=10;
            else if (strr[la] == 'r') strr[la]=13;
            else if (strr[la] == '[') { // pass through as \[
                strings[stringssize + opi] = '\\';
                opi++;
            }
        }
        strings[stringssize+opi]=strr[la];
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
void ccCompiledScript::fixup_previous(char ftype) {
    add_fixup(codesize-1,ftype);
}
int ccCompiledScript::add_new_function(char*namm, int *idx) {
    if (numfunctions >= MAX_FUNCTIONS) return -1;
    //  if (remove_any_import (namm)) return -2;
    functions[numfunctions] = (char*)malloc(strlen(namm)+20);
    strcpy(functions[numfunctions],namm);
    funccodeoffs[numfunctions] = codesize;
    funcnumparams[numfunctions] = 0;
    //  code = (long*)realloc(code, codesize + 5000);
    if (idx)
        *idx = numfunctions;
    numfunctions++;
    return codesize;
}
int ccCompiledScript::add_new_import(char*namm) 
{
    if (numimports >= importsCapacity) 
    {
        importsCapacity += 1000;
        imports = (char**)realloc(imports, sizeof(char*) * importsCapacity);
    }
    imports[numimports] = (char*)malloc(strlen(namm)+12);
    strcpy(imports[numimports],namm);
    numimports++;
    return numimports-1;
}
int ccCompiledScript::remove_any_import (char*namm, SymbolDef *oldSym) {
    // Remove any import with the specified name
    int i, sidx;
    sidx = sym.find(namm);
    if (sidx < 0)
        return 0;
    if ((sym.flags[sidx] & SFLG_IMPORTED) == 0)
        return 0;
    // if this import has been referenced, flag an error
    if (sym.flags[sidx] & SFLG_ACCESSED) {
        cc_error("Already referenced name as import; you must define it before using it");
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
        oldSym->flags = sym.flags[sidx] & ~SFLG_IMPORTED;
        oldSym->stype = sym.stype[sidx];
        oldSym->sscope = sym.sscope[sidx];
        // Return size may have been unknown at the time of forward declaration. Check the actual return type for those cases.
        if(sym.ssize[sidx] == 0)
            oldSym->ssize = sym.ssize[sym.funcparamtypes[sidx][0] & ~(STYPE_POINTER | STYPE_DYNARRAY)];
        else
            oldSym->ssize = sym.ssize[sidx];
        oldSym->arrsize = sym.arrsize[sidx];
        if (sym.stype[sidx] == SYM_FUNCTION) {
            // <= because of return type
            for (i = 0; i <= sym.get_num_args(sidx); i++) {
                oldSym->funcparamtypes[i] = sym.funcparamtypes[sidx][i];
                oldSym->funcParamDefaultValues[i] = sym.funcParamDefaultValues[sidx][i];
            }
        }
    }

    // remove its type so that it can be declared
    sym.stype[sidx] = 0;
    sym.flags[sidx] = 0;

    // check also for a number-of-parameters appended version
    char appended[200];
    sprintf(appended, "%s^", namm);
    int applen = strlen(appended);

    for (i = 0; i < numimports; i++) {
        if (strcmp(imports[i], namm) == 0) {
            // Just null the name of the import
            // DO NOT remove the import from the list, as some other
            // import indexes might already be referenced by the code
            // compiled so far.
            imports[i][0] = 0;
        }
        else if (strncmp(imports[i], appended, applen) == 0) {
            imports[i][0] = 0;
        }

    }
    return 0;
}

int ccCompiledScript::add_new_export(char*namm,int etype,long eoffs, int numArgs) 
{
    if (numexports >= exportsCapacity) 
    {
        exportsCapacity += 1000;
        exports = (char**)realloc(exports, sizeof(char*) * exportsCapacity);
        export_addr = (int32_t*)realloc(export_addr, sizeof(int32_t) * exportsCapacity);
    }
    if (eoffs >= 0x00ffffff) {
        cc_error("export offset too high; script data size too large?");
        return -1;
    }
    char *newName = (char*)malloc(strlen(namm)+20);
    strcpy(newName, namm);
    // mangle the name for functions to record parameters
    if (etype == EXPORT_FUNCTION) {
        char tempAddition[20];
        sprintf(tempAddition, "$%d", numArgs);
        strcat(newName, tempAddition);
    }
    // Check if it's already exported
    for (int aa = 0; aa < numexports; aa++) {
        if (strcmp(exports[aa], newName) == 0) {
            free(newName);
            return aa;
        }
    }
    exports[numexports] = newName;

    export_addr[numexports] = eoffs | ((long)etype << 24L);
    numexports++;
    return numexports-1;
}
void ccCompiledScript::flush_line_numbers() {
    if (next_line) {
        int linum = next_line;
        next_line = 0;
        write_code(SCMD_LINENUM);
        write_code(linum);
    }
}
void ccCompiledScript::write_code(intptr_t byy) {
    flush_line_numbers();
    if (codesize >= codeallocated - 2) {
        codeallocated += 500;
        code = (intptr_t*)realloc(code,codeallocated*sizeof(intptr_t));
    }
    code[codesize] = byy;
    codesize++;
}

intptr_t ccCompiledScript::yank_chunk(int32_t start, intptr_t **nested_chunk, int index) {
    intptr_t chunk_size;
    intptr_t chunk_index;

    if (start < codesize) {
        chunk_index = chunk_size = codesize - start;
        nested_chunk[index] = (intptr_t *) malloc(chunk_size * sizeof(intptr_t));
        while(chunk_index-- > 0) // speed up with memcpy?
            nested_chunk[index][chunk_index] = code[start + chunk_index];
        codesize = start;
    }
    else
        chunk_size = 0;

    return chunk_size;
}

void ccCompiledScript::write_chunk(intptr_t **nested_chunk, int index, intptr_t chunk_size, bool dispose, int fixup_start, int fixup_stop, int32_t adjust) {
    intptr_t chunk_index;
    if(chunk_size > 0)
    {
        if(codesize + chunk_size >= codeallocated) {
            codeallocated += chunk_size + 500;
            code = (intptr_t *) realloc(code, codeallocated * sizeof(intptr_t));
        }
        chunk_index = chunk_size;
        while(chunk_index-- > 0) // speed up with memcpy?
            code[codesize + chunk_index] = nested_chunk[index][chunk_index];
        codesize += chunk_size;
        if(dispose)
            free(nested_chunk[index]);
    }
    while(fixup_start < fixup_stop)
    {
        fixups[fixup_start] = fixups[fixup_start] + adjust;
        fixup_start++;
    }
}

const char* ccCompiledScript::start_new_section(const char *name) {

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
void ccCompiledScript::init() {
    globaldata = NULL;
    globaldatasize = 0;
    code = NULL;
    codesize = 0;
    codeallocated = 0;
    numfunctions = 0;
    strings = NULL;
    stringssize = 0;
    cur_sp=0;
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
void ccCompiledScript::free_extra() {
    int aa;
    for (aa=0;aa<numfunctions;aa++)
        free(functions[aa]);
    numfunctions=0;
}
void ccCompiledScript::shutdown() {
    free_extra();
}

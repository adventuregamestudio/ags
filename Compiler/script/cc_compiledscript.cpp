#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_compiledscript.h"
#include "script/script_common.h"   // macro definitions
#include "cc_symboltable.h"         // SymbolTable
#include "script/cc_options.h"      // ccGetOption
#include "script/cc_error.h"

void ccCompiledScript::push_reg(AGS::CodeCell regg)
{
    write_cmd1(SCMD_PUSHREG, regg);
    cur_sp += 4;
}

void ccCompiledScript::pop_reg(AGS::CodeCell regg)
{
    write_cmd1(SCMD_POPREG, regg);
    cur_sp -= 4;
}

ccCompiledScript::ccCompiledScript()
{
    init();
    ax_vartype = 0;
    ax_val_scope = 0;
}

ccCompiledScript::~ccCompiledScript()
{
    shutdown();
}

// [fw] Note: Existing callers expected this function to return < 0 on overflow
// [fw] TODO: All this should become an STL vector for automatic memory management
AGS::GlobalLoc ccCompiledScript::add_global(size_t siz, void *vall)
{
    // The new global variable will be moved to &(globaldata[offset])
    AGS::GlobalLoc offset = globaldatasize;

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

AGS::StringsLoc ccCompiledScript::add_string(std::string const &literal)
{
    // Note: processing  of '\\' and '[' combinations moved to the scanner
    // because the scanner must deal with '\\' anyway.
    size_t const literal_len = literal.size() + 1; // length including the terminating '\0'

    strings = (char *)realloc(strings, stringssize + literal_len);
    size_t const start_of_new_string = stringssize;

    memcpy(&strings[start_of_new_string], literal.c_str(), literal_len);
    stringssize += literal_len;
    return start_of_new_string;
}

void ccCompiledScript::add_fixup(AGS::CodeLoc locc, AGS::FixupType ftype)
{
    fixuptypes = (char *)realloc(fixuptypes, numfixups + 5);
    fixups = static_cast<AGS::CodeLoc *>(realloc(
        fixups,
        (numfixups * sizeof(AGS::CodeLoc)) + 10));
    fixuptypes[numfixups] = ftype;
    fixups[numfixups] = locc;
    numfixups++;
}

AGS::CodeLoc ccCompiledScript::add_new_function(std::string const &func_name, int *idx)
{
    FuncProps fp;
    fp.Name = func_name;
    fp.CodeOffs = codesize;
    fp.NumOfParams = 0;
    functions.push_back(fp);
    if (idx)
        *idx = functions.size() - 1;
    return codesize;
}

int ccCompiledScript::add_new_import(std::string const &import_name)
{
    if (numimports >= importsCapacity)
    {
        importsCapacity += 1000;
        imports = static_cast<char **>(realloc(imports, sizeof(char *) * importsCapacity));
    }
    imports[numimports] = static_cast<char *>(malloc(import_name.size() + 12));
    strcpy(imports[numimports], import_name.c_str());
    numimports++;
    return numimports - 1;
}


int ccCompiledScript::add_new_export(std::string const &name, AGS::Exporttype etype, AGS::CodeLoc eoffs, size_t num_of_args)
{
    // add_new_export(std::string const &name, AGS::Vartype vartype, AGS::CodeLoc location, size_t num_of_arguments = 0);
    if (numexports >= exportsCapacity)
    {
        exportsCapacity += 1000;
        exports = static_cast<char **>(realloc(exports, sizeof(char *) * exportsCapacity));
        export_addr = static_cast<int32_t *>(realloc(export_addr, sizeof(int32_t) * exportsCapacity));
    }
    if (eoffs >= 0x00ffffff)
    {
        cc_error("export offset too high; script data size too large?");
        return -1;
    }
    char *newName = static_cast<char *>(malloc(name.size() + 20));
    strcpy(newName, name.c_str());
    // mangle the name for functions to record parameters
    if (etype == EXPORT_FUNCTION)
    {
        char tempAddition[20];
        sprintf(tempAddition, "$%d", num_of_args);
        strcat(newName, tempAddition);
    }
    // Check if it's already exported
    for (int exports_idx = 0; exports_idx < numexports; exports_idx++)
    {
        if (strcmp(exports[exports_idx], newName) == 0)
        {
            free(newName);
            return exports_idx;
        }
    }
    exports[numexports] = newName;

    export_addr[numexports] = eoffs | (static_cast<long>(etype) << 24L);
    numexports++;
    return numexports - 1;
}

void ccCompiledScript::flush_line_numbers()
{
    if (!next_line)
        return;

    int linum = next_line;
    next_line = 0;
    write_cmd1(SCMD_LINENUM, linum);
}

void ccCompiledScript::write_code(AGS::CodeCell byy)
{
    flush_line_numbers();
    if (codesize >= codeallocated - 2)
    {
        codeallocated += 500;
        code = static_cast<int32_t *>(realloc(code, codeallocated * sizeof(intptr_t)));
    }
    code[codesize] = byy;
    codesize++;
}

std::string ccCompiledScript::start_new_section(std::string const &name)
{

    if ((numSections == 0) ||
        (codesize != sectionOffsets[numSections - 1]))
    {
        if (numSections >= capacitySections)
        {
            capacitySections += 100;
            sectionNames = static_cast<char **>(realloc(sectionNames, sizeof(char *) * capacitySections));
            sectionOffsets = static_cast<int32_t *>(realloc(sectionOffsets, sizeof(int32_t) * capacitySections));
        }
        sectionNames[numSections] = (char *)malloc(name.size() + 1);
        strcpy(sectionNames[numSections], name.c_str());
        sectionOffsets[numSections] = codesize;

        numSections++;
    }
    else
    {
        // nothing was in the last section, so overwrite it with this new one
        free(sectionNames[numSections - 1]);
        sectionNames[numSections - 1] = (char *)malloc(name.size() + 1);
        strcpy(sectionNames[numSections - 1], name.c_str());
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
    ax_vartype = 0;
    ax_val_scope = 0;
}

// free the extra bits that ccScript doesn't have
void ccCompiledScript::free_extra()
{
    functions.clear();
}

void ccCompiledScript::shutdown()
{
    free_extra();
}

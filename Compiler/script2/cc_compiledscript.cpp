#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "cc_compiledscript.h"
#include "script/script_common.h"   // macro definitions
#include "cc_symboltable.h"         // SymbolTable
#include "script/cc_options.h"      // ccGetOption
#include "script/cc_error.h"

void AGS::ccCompiledScript::write_lineno(size_t lno)
{
    if (EmitLineNumbers)
        write_cmd(SCMD_LINENUM, lno);
    LastEmittedLineno = lno;
}

void AGS::ccCompiledScript::push_reg(CodeCell regg)
{
    write_cmd(SCMD_PUSHREG, regg);
    OffsetToLocalVarBlock += SIZE_OF_STACK_CELL;
}

void AGS::ccCompiledScript::pop_reg(CodeCell regg)
{
    write_cmd(SCMD_POPREG, regg);
    OffsetToLocalVarBlock -= SIZE_OF_STACK_CELL;
}

ccCompiledScript::ccCompiledScript(bool emit_line_numbers)
{
    init();
    ax_vartype = 0;
    ax_scope_type = ScT::kGlobal;

    EmitLineNumbers = emit_line_numbers;
}

AGS::ccCompiledScript::~ccCompiledScript()
{
    shutdown();
}

// [fw] Note: Existing callers expected this function to return < 0 on overflow
AGS::GlobalLoc AGS::ccCompiledScript::add_global(size_t siz, void *vall)
{
    // The new global variable will be moved to &(globaldata[offset])
    GlobalLoc offset = globaldatasize;

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

AGS::StringsLoc AGS::ccCompiledScript::add_string(std::string const &literal)
{
    // Note: processing  of '\\' and '[' combinations moved to the scanner
    // because the scanner must deal with '\\' anyway.
    size_t const literal_len = literal.size() + 1; // length including the terminating '\0'

    strings = (char *) realloc(strings, stringssize + literal_len);
    size_t const start_of_new_string = stringssize;

    memcpy(&strings[start_of_new_string], literal.c_str(), literal_len);
    stringssize += literal_len;
    return start_of_new_string;
}

void AGS::ccCompiledScript::add_fixup(CodeLoc where, FixupType ftype)
{
    fixuptypes = (char *) realloc(fixuptypes, numfixups + 5);
    fixups = static_cast<CodeLoc *>(realloc(
        fixups,
        (numfixups * sizeof(CodeLoc)) + 10));
    fixuptypes[numfixups] = ftype;
    fixups[numfixups] = where;
    numfixups++;
}

AGS::CodeLoc AGS::ccCompiledScript::add_new_function(std::string const &func_name, int *index_allocated)
{
    FuncProps fp;
    fp.Name = func_name;
    fp.CodeOffs = codesize;
    fp.NumOfParams = 0;
    functions.push_back(fp);
    if (index_allocated)
        *index_allocated = functions.size() - 1;
    return codesize;
}

int AGS::ccCompiledScript::add_new_import(std::string const &import_name)
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

int AGS::ccCompiledScript::add_new_export(std::string const &name, Exporttype etype, CodeLoc location, size_t num_of_arguments)
{
    // add_new_export(std::string const &name, AGS::Vartype vartype, AGS::CodeLoc location, size_t num_of_arguments = 0);
    if (numexports >= exportsCapacity)
    {
        exportsCapacity += 1000;
        exports = static_cast<char **>(realloc(exports, sizeof(char *) * exportsCapacity));
        export_addr = static_cast<int32_t *>(realloc(export_addr, sizeof(int32_t) * exportsCapacity));
    }
    if (location >= 0x00ffffff)
    {
        cc_error("export offset too high; script data size too large?");
        return -1;
    }
    
    // mangle the name for functions to record parameters
    std::string new_name(name);
    if (etype == EXPORT_FUNCTION)
        new_name.append("$").append(std::to_string(num_of_arguments));
 
    // Check if it's already exported
    for (int exports_idx = 0; exports_idx < numexports; exports_idx++)
        if (0 == new_name.compare(exports[exports_idx]))
            return exports_idx;

    size_t const new_name_size = new_name.size() + 1;
    exports[numexports] = static_cast<char *>(malloc(new_name_size));
    strncpy(exports[numexports], new_name.c_str(), new_name_size);
    export_addr[numexports] = location | (static_cast<long>(etype) << 24L);
    return numexports++;
}

void AGS::ccCompiledScript::write_code(CodeCell cell)
{
    if (codesize >= codeallocated - 2)
    {
        codeallocated += 500;
        code = static_cast<int32_t *>(realloc(code, codeallocated * sizeof(int32_t)));
    }
    code[codesize] = cell;
    codesize++;
}

std::string AGS::ccCompiledScript::start_new_section(std::string const &name)
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

void AGS::ccCompiledScript::init()
{
    globaldata = NULL;
    globaldatasize = 0;
    code = NULL;
    codesize = 0;
    codeallocated = 0;
    strings = NULL;
    stringssize = 0;
    OffsetToLocalVarBlock = 0;
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
    LastEmittedLineno = 0;
    ax_vartype = 0;
    ax_scope_type = ScT::kGlobal;
}

// free the extra bits that ccScript doesn't have
void AGS::ccCompiledScript::free_extra()
{
    functions.clear();
}

void AGS::ccCompiledScript::shutdown()
{
    free_extra();
}

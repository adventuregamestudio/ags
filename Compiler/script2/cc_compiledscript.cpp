#include <stdlib.h>
#include <string>
#include "cc_compiledscript.h"
#include "script/script_common.h"   // macro definitions
#include "script/cc_error.h"

void AGS::ccCompiledScript::WriteLineno(size_t lno)
{
    if (EmitLineNumbers)
        WriteCmd(SCMD_LINENUM, lno);
    LastEmittedLineno = lno;
}

void AGS::ccCompiledScript::PushReg(CodeCell regg)
{
    WriteCmd(SCMD_PUSHREG, regg);
    OffsetToLocalVarBlock += SIZE_OF_STACK_CELL;
}

void AGS::ccCompiledScript::PopReg(CodeCell regg)
{
    WriteCmd(SCMD_POPREG, regg);
    OffsetToLocalVarBlock -= SIZE_OF_STACK_CELL;
}

ErrorType AGS::ccCompiledScript::ResizeMemory(size_t const needed, size_t const min_size, void *&start, size_t &allocated)
{
    if (allocated >= needed)
        return kERR_None; // nothing to be done

    size_t new_size = (allocated < min_size) ? min_size : allocated;
    while (new_size < needed)
        new_size += new_size / 2;

    void *new_start = realloc(start, new_size);
    if (!new_start)
    {
        // Note, according to the STL, the old block is NOT freed if allocation fails
        free(start);
        start = nullptr;
        return kERR_InternalError;
    }

    start = new_start;
    allocated = new_size;
    return kERR_None;
}

AGS::ccCompiledScript::ccCompiledScript(bool emit_line_numbers)
    : EmitLineNumbers(emit_line_numbers)
{
    globaldata = NULL;
    globaldatasize = 0;
    code = NULL;
    codesize = 0;
    strings = NULL;
    stringssize = 0;
    OffsetToLocalVarBlock = 0;
    fixups = NULL;
    fixuptypes = NULL;
    numfixups = 0;
    numimports = 0;
    numexports = 0;
    numSections = 0;
    imports = NULL;
    exports = NULL;
    export_addr = NULL;
    sectionNames = NULL;
    sectionOffsets = NULL;

    LastEmittedLineno = INT_MAX;
    EmitLineNumbers = emit_line_numbers;
    AX_Vartype = 0;
    AX_ScopeType = ScT::kGlobal;
    Functions = {};
    ImportIdx = {};

    EmitLineNumbers = emit_line_numbers;
}

AGS::ccCompiledScript::~ccCompiledScript()
{
    FreeExtra();
}

// [fw] Note: Existing callers expected this function to return < 0 on overflow
AGS::GlobalLoc AGS::ccCompiledScript::AddGlobal(size_t value_size, void *value_ptr)
{
    if (0u == value_size)
        return globaldatasize; // nothing to do

    // The new global variable will be moved to &(globaldata[offset])
    size_t const offset = (globaldatasize < 0) ? 0u : static_cast<size_t>(globaldatasize);

    void *chunk_start = globaldata;
    ErrorType retval =
        ResizeChunk(offset + value_size, 100u, chunk_start, _globaldataAllocated);
    if (retval < 0) return -1;
    globaldata = reinterpret_cast<char *>(chunk_start);

    if (nullptr != value_ptr)
        memcpy(&(globaldata[offset]), value_ptr, value_size); // move the global into the new space
    else
        memset(&(globaldata[offset]), 0, value_size); // fill the new space with 0-bytes

    globaldatasize += value_size;
    return offset;
}

AGS::StringsLoc AGS::ccCompiledScript::AddString(std::string const &literal)
{
    size_t const literal_len = literal.size() + 1u; // length including the terminating '\0'

    // The new string will be moved to &(strings[offset])
    size_t const offset = (stringssize < 0) ? 0u : static_cast<size_t>(stringssize);

    void *chunk_start = strings;
    ErrorType retval =
        ResizeChunk(stringssize + literal_len, 100u, chunk_start, _stringsAllocated);
    if (retval < 0) return -1;
    strings = reinterpret_cast<char *>(chunk_start);

    memcpy(&(strings[offset]), literal.c_str(), literal_len);
    stringssize += literal_len;
    return offset;
}

int AGS::ccCompiledScript::AddFixup(CodeLoc where, FixupType ftype)
{
    void *chunk_start = fixuptypes;
    ErrorType retval =
        ResizeChunk(numfixups + 1, 10u, chunk_start, _fixupTypesAllocated);
    if (retval < 0) return -1;
    fixuptypes = reinterpret_cast<char *>(chunk_start);
    fixuptypes[numfixups] = ftype;

    chunk_start = fixups;
    constexpr size_t el_size = sizeof(decltype(fixups[0u]));
    retval =
        ResizeChunk((numfixups + 1) * el_size, 10u * el_size, chunk_start, _fixupsAllocated);
    if (retval < 0) return -1;
    fixups = reinterpret_cast<CodeLoc *>(chunk_start);
    fixups[numfixups] = where;

    return numfixups++;
}

AGS::CodeLoc AGS::ccCompiledScript::AddNewFunction(std::string const &func_name, size_t num_of_parameters)
{
    FuncProps fp;
    fp.Name = func_name;
    fp.CodeOffs = codesize;
    fp.NumOfParams = num_of_parameters;
    Functions.push_back(fp);
    return codesize;
}

int AGS::ccCompiledScript::FindOrAddImport(std::string const &import_name)
{
    if (0u < ImportIdx.count(import_name))
        return ImportIdx[import_name];

    void *chunk_start = imports;
    constexpr size_t el_size = sizeof(decltype(imports[0u]));
    ErrorType retval =
        ResizeChunk((numimports + 1) * el_size, 10u * el_size, chunk_start, _importsAllocated);
    if (retval < 0) return -1;
    imports = reinterpret_cast<decltype(imports)>(chunk_start);
    imports[numimports] = static_cast<char *>(malloc(import_name.size() + 12u));
    if (nullptr == imports[numimports]) return -1;
    strcpy(imports[numimports], import_name.c_str());
    return (ImportIdx[import_name] = numimports++);
}

int AGS::ccCompiledScript::AddExport(std::string const &name, CodeLoc const location, size_t const num_of_arguments)
{
    bool const is_function = (INT_MAX != num_of_arguments);
    // Exported functions get the number of parameters appended
    std::string const export_name =
        is_function ? name + "$" + std::to_string(num_of_arguments) : name;

    if (location >= 0x00ffffff)
    {
        cc_error("export offset too high; script data size too large?");
        return -1;
    }

    if (0u < ExportIdx.count(export_name))
        return ExportIdx[export_name];

    void *chunk_start = exports;
    constexpr size_t el_size = sizeof(decltype(exports[0u]));
    ErrorType retval =
        ResizeChunk((numexports + 1) * el_size, 10u * el_size, chunk_start, _exportsAllocated);
    if (retval < 0) return -1;
    exports = reinterpret_cast<decltype(exports)>(chunk_start);
    
    chunk_start = export_addr;
    constexpr size_t el2_size = sizeof(decltype(export_addr[0u]));
    retval =
        ResizeChunk((numexports + 1) * el2_size, 10u * el2_size, chunk_start, _exportAddrAllocated);
    if (retval < 0) return -1;
    export_addr = reinterpret_cast<decltype(export_addr)>(chunk_start);
    
    size_t const entry_size = export_name.size() + 1u;
    exports[numexports] = static_cast<char *>(malloc(entry_size));
    if (nullptr == exports[numexports])
        return -1;
    strncpy(exports[numexports], export_name.c_str(), entry_size);
    export_addr[numexports] =
        location |
        static_cast<CodeLoc>(is_function? EXPORT_FUNCTION : EXPORT_DATA) << 24L;
    return (ExportIdx[export_name] = numexports++);
}

void AGS::ccCompiledScript::WriteCode(CodeCell cell)
{
    void *chunk_start = code;
    constexpr size_t el_size = sizeof(decltype(code[0u]));
    ErrorType retval =
        ResizeChunk((codesize + 4) * el_size, 500u * el_size, chunk_start, _codeAllocated);
    if (retval < 0) return;
    code = reinterpret_cast<decltype(code)>(chunk_start);
    code[codesize] = cell;
    codesize++;
}

int AGS::ccCompiledScript::StartNewSection(std::string const &name)
{
    if ((numSections == 0) ||
        (codesize != sectionOffsets[numSections - 1]))
    {
        void *chunk_start = sectionNames;
        constexpr size_t el_size = sizeof(decltype(sectionNames[0u]));
        ErrorType retval =
            ResizeChunk((codesize + 4) * el_size, 10u * el_size, chunk_start, _sectionNamesAllocated);
        if (retval < 0) return -1;
        sectionNames = reinterpret_cast<decltype(sectionNames)>(chunk_start);
        chunk_start = sectionOffsets;
        constexpr size_t el2_size = sizeof(decltype(sectionOffsets[0u]));
        retval =
            ResizeChunk((codesize + 4) * el_size, 10u * el2_size, chunk_start, _sectionOffsetsAllocated);
        if (retval < 0) return -1;
        sectionOffsets = reinterpret_cast<decltype(sectionOffsets)>(chunk_start);

        sectionNames[numSections] = (char *) malloc(name.size() + 1);
        if (nullptr == sectionNames[numSections])
            return -1;
        strcpy(sectionNames[numSections], name.c_str());
        sectionOffsets[numSections] = codesize;

        numSections++;
    }
    else
    {
        // nothing was in the last section, so overwrite it with this new one
        free(sectionNames[numSections - 1]);
        sectionNames[numSections - 1] = (char *) malloc(name.size() + 1);
        if (nullptr == sectionNames[numSections])
            return -1;
        strcpy(sectionNames[numSections - 1], name.c_str());
    }

    return 0;
}

// free the extra bits that ccScript doesn't have
void AGS::ccCompiledScript::FreeExtra()
{
    Functions.clear();
    Functions.shrink_to_fit();
    ImportIdx.clear();
    ExportIdx.clear();
}

#include <stdlib.h>
#include <string>
#include <stdexcept>
#include "cc_compiledscript.h"
#include "script/cc_common.h"
#include "script/cc_internal.h"

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

ErrorType AGS::ccCompiledScript::ResizeMemory(size_t const el_size, size_t const needed, size_t const minimum, void *&chunk, size_t &allocated)
{
    if (allocated >= needed)
        return kERR_None; // nothing to be done

    size_t new_element_number = (allocated < minimum) ? minimum : allocated;
    while (new_element_number < needed)
        new_element_number += new_element_number / 2;

    void *new_chunk = realloc(chunk, el_size * new_element_number);
    if (!new_chunk)
    {
        // Note, according to the STL, the old block is NOT freed if allocation fails
        free(chunk);
        chunk = nullptr;
        return kERR_InternalError;
    }

    chunk = new_chunk;
    allocated = new_element_number;
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
    Functions = {};
    ImportIdx = {};
}

void AGS::ccCompiledScript::ReplaceLabels()
{
    std::vector<CodeLoc> RemainingLabels;

    for (size_t idx = 0; idx < Labels.size(); idx++)
    {
        CodeLoc const loc = Labels[idx];
        if (loc >= codesize)
            continue;
        try
        {
            CodeCell value = Label2Value.at(code[loc]);
            code[loc] = value;
        }
        catch (const std::out_of_range)
        {
            // Can't resolve the label at 'loc', so hang on to this location
            RemainingLabels.push_back(loc);
        }
    }
    Labels.assign(RemainingLabels.begin(), RemainingLabels.end());
}

AGS::ccCompiledScript::~ccCompiledScript()
{
    FreeExtra();
}

// [fw] Note: Existing callers expected this function to return < 0 on overflow
AGS::GlobalLoc AGS::ccCompiledScript::AddGlobal(size_t const value_size, void *value_ptr)
{
    if (0u == value_size)
        return globaldatasize; // nothing to do

    // The new global variable will be moved to &(globaldata[offset])
    size_t const offset = (globaldatasize < 0) ? 0u : static_cast<size_t>(globaldatasize);

    void *globaldata_chunk = globaldata;
    size_t const el_size = sizeof(globaldata[0u]); // size of 1 element, i.e. 1 char
    size_t const needed = offset + value_size;
    size_t const minimum_allocated = 100u;
    ErrorType retval =
        ResizeChunk(el_size, needed, minimum_allocated, globaldata_chunk, _globaldataAllocated);
    if (retval < 0) return -1;
    globaldata = static_cast<decltype(globaldata)>(globaldata_chunk);

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

    void *strings_chunk = strings;
    size_t const el_size = sizeof(decltype(strings[0u])); // size of 1 element, i.e. 1 char
    size_t const needed = static_cast<size_t>(stringssize) + literal_len + 1u;
    size_t const minimum_allocated = 100u;
    ErrorType retval =
        ResizeChunk(el_size, needed, minimum_allocated, strings_chunk, _stringsAllocated);
    if (retval < 0) return -1;
    strings = static_cast<char *>(strings_chunk);

    memcpy(&(strings[offset]), literal.c_str(), literal_len);
    stringssize += literal_len;
    return offset;
}

int AGS::ccCompiledScript::AddFixup(CodeLoc const where, FixupType const ftype)
{
    {
        // See to it that 'fixuptypes' has sufficient capacity
        void *fixuptypes_chunk = fixuptypes;
        size_t const el_size = sizeof(decltype(fixuptypes[0u]));  // size of one array element
        size_t const needed = numfixups + 1u;
        size_t const minimum_allocated = 10u;
        ErrorType retval =
            ResizeChunk(el_size, needed, minimum_allocated, fixuptypes_chunk, _fixupTypesAllocated);
        if (retval < 0) return -1;
        fixuptypes = static_cast<decltype(fixuptypes)>(fixuptypes_chunk);
    }
    fixuptypes[numfixups] = ftype;

    {
        // See to it that 'fixups' has sufficient capacity
        void *fixups_chunk = fixups;
        size_t const el_size = sizeof(decltype(fixups[0u])); // size of one array element
        size_t const needed = numfixups + 1u;
        size_t const minimum_allocated = 10u;
        ErrorType retval =
            ResizeChunk(el_size, needed, minimum_allocated, fixups_chunk, _fixupsAllocated);
        if (retval < 0) return -1;
        fixups = static_cast<decltype(fixups)>(fixups_chunk);
    }
    fixups[numfixups] = where;

    return numfixups++;
}

AGS::CodeLoc AGS::ccCompiledScript::AddNewFunction(std::string const &func_name, size_t const num_of_parameters)
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

    // See to it that 'imports[]' has sufficient capacity
    void *imports_chunk = imports;
    size_t const el_size = sizeof(decltype(imports[0u])); // size of one array element
    size_t const needed = numimports + 1;
    size_t const minimum_allocated = 10u;
    ErrorType retval =
        ResizeChunk(el_size, needed, minimum_allocated, imports_chunk, _importsAllocated);
    if (retval < 0) return -1;
    imports = static_cast<decltype(imports)>(imports_chunk);

    // [fw] So why does each 'imports' string need to have space for 11 additional characters?
    size_t const import_name_size = import_name.size();
    imports[numimports] = static_cast<char *>(malloc(12u + import_name_size));
    if (nullptr == imports[numimports]) return -1;
    strncpy(imports[numimports], import_name.c_str(), 1u + import_name_size);
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

    {
        // See to it that 'exports[]' has sufficient capacity
        void *exports_chunk = exports;
        size_t const el_size = sizeof(decltype(exports[0u])); // size of one array element
        size_t const needed = numexports + 1;
        size_t const minimum_allocated = 10u;
        ErrorType retval =
            ResizeChunk(el_size, needed, minimum_allocated, exports_chunk, _exportsAllocated);
        if (retval < 0) return -1;
        exports = static_cast<decltype(exports)>(exports_chunk);
    }

    {
        // See to it that 'export_addr[]' has sufficient capacity
        void *export_addr_chunk = export_addr;
        size_t const el_size = sizeof(decltype(export_addr[0u])); // size of one array element
        size_t const needed = numexports + 1;
        size_t const minimum_allocated = 10u;
        ErrorType retval =
            ResizeChunk(el_size, needed, minimum_allocated, export_addr_chunk, _exportAddrAllocated);
        if (retval < 0) return -1;
        export_addr = static_cast<decltype(export_addr)>(export_addr_chunk);
    }
    
    size_t const export_name_size = export_name.size();
    exports[numexports] = static_cast<char *>(malloc(1u + export_name_size));
    if (nullptr == exports[numexports])
        return -1;
    strncpy(exports[numexports], export_name.c_str(), 1u + export_name_size);
    export_addr[numexports] =
        location |
        static_cast<CodeLoc>(is_function? EXPORT_FUNCTION : EXPORT_DATA) << 24L;
    return (ExportIdx[export_name] = numexports++);
}

void AGS::ccCompiledScript::WriteCode(CodeCell const cell)
{
    // See to it that code[] has adequate capacity
    void *code_chunk = code;
    size_t const el_size = sizeof(decltype(code[0u])); // size of one array element
    size_t const needed = 1u + codesize;
    size_t const minimum_allocated = 500u;
    ErrorType retval =
        ResizeChunk(el_size, needed, minimum_allocated, code_chunk, _codeAllocated);
    if (retval < 0) return;
    code = static_cast<decltype(code)>(code_chunk);

    code[codesize] = cell;
    codesize++;
}

ErrorType AGS::ccCompiledScript::StartNewSection(std::string const &name)
{
    if (numSections > 0 && codesize == sectionOffsets[numSections - 1])
    {
        // nothing was in the last section, so free its data;
        // it will be overwritten with the data of this current section
        --numSections;
        free(sectionNames[numSections]);
    }

    {
        // See to it that the array 'sectionNames' has sufficient capacity.
        void *section_names_chunk = sectionNames;
        size_t const el_size = sizeof(decltype(sectionNames[0u])); // size of an array element
        size_t const needed = numSections + 1u;
        size_t const minimum_allocated = 10u;
        ErrorType retval =
            ResizeChunk(el_size, needed, minimum_allocated, section_names_chunk, _sectionNamesAllocated);
        if (retval < 0)
            return kERR_InternalError;
        sectionNames = static_cast<decltype(sectionNames)>(section_names_chunk);
    }

    {
        // See to it that the array 'sectionOffsets' has sufficient capacity.
        void *section_offsets_chunk = sectionOffsets;
        size_t const el_size = sizeof(decltype(sectionOffsets[0u])); // size of an array element
        size_t const needed = numSections + 1u;
        size_t const minimum_allocated = 10u;
        ErrorType retval =
            ResizeChunk(el_size, needed, minimum_allocated, section_offsets_chunk, _sectionOffsetsAllocated);
        if (retval < 0)
            return kERR_InternalError;
        sectionOffsets = static_cast<decltype(sectionOffsets)>(section_offsets_chunk);
    }

    // Record that the new section starts at this point
    size_t const name_size = name.size();
    sectionNames[numSections] = static_cast<char *>(malloc(1u + name_size));
    if (nullptr == sectionNames[numSections])
        return kERR_InternalError;
    strncpy(sectionNames[numSections], name.c_str(), 1u + name_size);
    sectionOffsets[numSections] = codesize;
    numSections++;

    return kERR_None;
}

// free the extra bits that ccScript doesn't have
void AGS::ccCompiledScript::FreeExtra()
{
    Functions.clear();
    Functions.shrink_to_fit();
    ImportIdx.clear();
    ExportIdx.clear();
    Labels.clear();
    Labels.shrink_to_fit();
}

void AGS::Snippet::Paste(ccCompiledScript &scrip)
{
    CodeLoc const code_start = scrip.codesize;
    // Don't generate additional LINUM directives, that would throw off the fixups and labels
    for (size_t idx = 0; idx < Code.size(); idx++)
        scrip.WriteCode(Code[idx]);
    for (size_t idx = 0; idx < Fixups.size(); idx++)
        scrip.AddFixup(code_start + Fixups[idx], FixupTypes[idx]);
    for (size_t idx = 0; idx < Labels.size(); idx++)
        scrip.Labels.push_back(code_start + Labels[idx]);
}

bool AGS::Snippet::IsEmpty()
{
    for (size_t idx = 0; idx < Code.size(); idx++)
    {
        if (SCMD_LINENUM != Code[idx])
            return false; // found some content
        idx++; // skip the parameter of the 'linenum' pseudo-directive
    }
    return true;
}

void AGS::RestorePoint::Cut(Snippet &snippet, bool const keep_starting_linum)
{
    int32_t const orig_codesize = _scrip.codesize;

    // Reset the code to the remembered location.
    CodeLoc rcl = _rememberedCodeLocation;
    if (keep_starting_linum)
    {
        if (_scrip.code[rcl] == SCMD_LINENUM) // 
            rcl += 2; // skip the linenum pseudo-directive
    }
    if (_scrip.codesize <= rcl)
        return; // all done
    _scrip.codesize = rcl;

    // Save the code including leading LINUM directives, if any, into the snippet
    snippet.Code.clear();
    for (int idx = _rememberedCodeLocation; idx < orig_codesize; idx++)
        snippet.Code.push_back(_scrip.code[idx]);

    // Assume fixups are ordered by location. Find the first fixup that has been cut out.
    int first_cut_fixup;
    for (first_cut_fixup = _scrip.numfixups - 1; first_cut_fixup >= 0; first_cut_fixup--)
        if (_scrip.fixups[first_cut_fixup] < rcl)
            break;
    first_cut_fixup++;

    // Save the fixups into the snippet and delete them out of _scrip
    snippet.Fixups.clear();
    snippet.FixupTypes.clear();
    for (int idx = first_cut_fixup; idx < _scrip.numfixups; idx++)
    {
        snippet.Fixups.push_back(_scrip.fixups[idx] - _rememberedCodeLocation);
        _scrip.fixups[idx] = 0;
        snippet.FixupTypes.push_back(_scrip.fixuptypes[idx]);
        _scrip.fixuptypes[idx] = '\0';
        
    }
    _scrip.numfixups = first_cut_fixup;

    // Assume label locations are ordered by location. Find the first location that has been cut out.
    int first_cut_label;
    for (first_cut_label = _scrip.Labels.size() - 1; first_cut_label >= 0; first_cut_label--)
        if (_scrip.Labels[first_cut_label] < rcl)
            break;
    first_cut_label++;

    // Save the labels into the snippet and delete them out of _scrip.
    snippet.Labels.clear();
    for (size_t idx = first_cut_label; idx < _scrip.Labels.size(); idx++)
        snippet.Labels.push_back(_scrip.Labels[idx] - _rememberedCodeLocation);
    _scrip.Labels.resize(first_cut_label);
}

void AGS::ForwardJump::AddParam(int offset)
{
    // If the current value for the last emitted lineno doesn't match the
    // saved value then the saved value won't work for all jumps so it
    // must be set to invalid.
    if (_jumpDestParamLocs.empty())
        _lastEmittedSrcLineno = _scrip.LastEmittedLineno;
    else if (_lastEmittedSrcLineno != _scrip.LastEmittedLineno)
        _lastEmittedSrcLineno = INT_MAX;
    _jumpDestParamLocs.push_back(_scrip.codesize + offset);
}

void AGS::ForwardJump::Patch(size_t cur_line, bool keep_linenum)
{
    if (!keep_linenum && !_jumpDestParamLocs.empty())
    {
        // There are two ways of reaching the bytecode that will be emitted next:
        // through the jump or from the previous bytecode command. If the source line
        // of both isn't identical then a line opcode must be emitted next.
        if (cur_line != _scrip.LastEmittedLineno || cur_line != _lastEmittedSrcLineno)
            _scrip.LastEmittedLineno = INT_MAX;
    }
    for (auto loc = _jumpDestParamLocs.cbegin(); loc != _jumpDestParamLocs.cend(); loc++)
        _scrip.code[*loc] = _scrip.RelativeJumpDist(*loc, _scrip.codesize);
    _jumpDestParamLocs.clear();
}

void AGS::BackwardJumpDest::Set(CodeLoc cl)
{
    _dest = (cl >= 0) ? cl : _scrip.codesize;
    _lastEmittedSrcLineno = _scrip.LastEmittedLineno;
}

void AGS::BackwardJumpDest::WriteJump(CodeCell jump_op, size_t cur_line)
{
    if (SCMD_LINENUM != _scrip.code[_dest] &&
        _scrip.LastEmittedLineno != _lastEmittedSrcLineno)
    {
        _scrip.WriteLineno(cur_line);
    }
    _scrip.WriteCmd(jump_op, _scrip.RelativeJumpDist(_scrip.codesize + 1, _dest));
}

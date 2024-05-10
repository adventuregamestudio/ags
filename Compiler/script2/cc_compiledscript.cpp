//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
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

AGS::ccCompiledScript::ccCompiledScript(bool emit_line_numbers)
    : EmitLineNumbers(emit_line_numbers)
{
    OffsetToLocalVarBlock = 0;

    LastEmittedLineno = INT_MAX;
    EmitLineNumbers = emit_line_numbers;
    Functions = {};
    ImportIdx = {};
}

void AGS::ccCompiledScript::ReplaceLabels()
{
    std::vector<CodeLoc> RemainingLabels;

    for (size_t idx = 0u; idx < Labels.size(); idx++)
    {
        CodeLoc const loc = Labels[idx];
        if (loc >= Codesize_i32())
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
    assert(globaldata.size() + value_size <= INT32_MAX);
    if (0u == value_size)
        return static_cast<int32_t>(globaldata.size()); // nothing to do

    // The new global variable will be moved to &(globaldata[offset])
    size_t const offset = globaldata.size();
    globaldata.resize(globaldata.size() + value_size);

    if (nullptr != value_ptr)
        memcpy(&(globaldata[offset]), value_ptr, value_size); // move the global into the new space
    else
        memset(&(globaldata[offset]), 0, value_size); // fill the new space with 0-bytes

    return static_cast<int32_t>(offset);
}

AGS::StringsLoc AGS::ccCompiledScript::AddString(std::string const &literal)
{
    size_t const literal_len = literal.size() + 1u; // length including the terminating '\0'
    assert(strings.size() + literal_len <= INT32_MAX);

    // The new string will be moved to &(strings[offset])
    size_t const offset = strings.size();
    strings.resize(strings.size() + literal_len);

    memcpy(&(strings[offset]), literal.c_str(), literal_len);
    return static_cast<int32_t>(offset);
}

int AGS::ccCompiledScript::AddFixup(CodeLoc const where, FixupType const ftype)
{
    assert(fixups.size() < INT32_MAX);
    fixuptypes.push_back(ftype);
    fixups.push_back(where);
    return static_cast<int32_t>(fixups.size() - 1);
}

AGS::CodeLoc AGS::ccCompiledScript::AddNewFunction(std::string const &func_name, size_t const params_count)
{
    FuncProps fp;
    fp.Name = func_name;
    fp.CodeOffs = Codesize_i32();
    fp.ParamsCount = params_count;
    Functions.push_back(fp);
    return Codesize_i32();
}

int AGS::ccCompiledScript::FindOrAddImport(std::string const &import_name)
{
    if (0u < ImportIdx.count(import_name))
        return ImportIdx[import_name];

    imports.push_back(import_name);
    return (ImportIdx[import_name] = static_cast<int32_t>(imports.size() - 1));
}

int AGS::ccCompiledScript::AddExport(std::string const &name, CodeLoc const location, size_t const arguments_count)
{
    bool const is_function = (INT_MAX != arguments_count);
    // Exported functions get the number of parameters appended
    std::string const export_name =
        is_function ? name + "$" + std::to_string(arguments_count) : name;

    if (location >= 0x00ffffff)
    {
        cc_error("export offset too high; script data size too large?");
        return -1;
    }

    if (0u < ExportIdx.count(export_name))
        return ExportIdx[export_name];
    
    exports.push_back(export_name);
    export_addr.push_back(
        location |
        static_cast<CodeLoc>(is_function? EXPORT_FUNCTION : EXPORT_DATA) << 24L);
    return (ExportIdx[export_name] = static_cast<int32_t>(exports.size() - 1));
}

void AGS::ccCompiledScript::WriteCode(CodeCell const cell)
{
    assert(code.size() < INT32_MAX);
    code.push_back(cell);
}

ErrorType AGS::ccCompiledScript::StartNewSection(std::string const &name)
{
    if (sectionNames.size() > 0 && Codesize_i32() == sectionOffsets.back())
    {
        // nothing was in the last section, so free its data;
        // it will be overwritten with the data of this current section
        sectionNames.pop_back();
        sectionOffsets.pop_back();
    }

    sectionNames.push_back(name);
    sectionOffsets.push_back(Codesize_i32());
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
    CodeLoc const code_start = scrip.Codesize_i32();
    // Don't generate additional LINUM directives, that would throw off the fixups and labels
    for (size_t idx = 0u; idx < Code.size(); idx++)
        scrip.WriteCode(Code[idx]);
    for (size_t idx = 0u; idx < Fixups.size(); idx++)
        scrip.AddFixup(code_start + Fixups[idx], FixupTypes[idx]);
    for (size_t idx = 0u; idx < Labels.size(); idx++)
        scrip.Labels.push_back(code_start + Labels[idx]);
}

bool AGS::Snippet::IsEmpty()
{
    for (size_t idx = 0u; idx < Code.size(); idx++)
    {
        if (SCMD_LINENUM != Code[idx])
            return false; // found some content
        idx++; // skip the parameter of the 'linenum' pseudo-directive
    }
    return true;
}

void AGS::RestorePoint::Cut(Snippet &snippet, bool const keep_starting_linum)
{
    int32_t const orig_codesize = _scrip.Codesize_i32();

    // Reset the code to the remembered location.
    CodeLoc rcl = _rememberedCodeLocation;
    if (keep_starting_linum)
    {
        if (_scrip.code[rcl] == SCMD_LINENUM) // 
            rcl += 2; // skip the linenum pseudo-directive
    }
    if (_scrip.Codesize_i32() <= rcl)
        return; // all done

    // Save the code including leading LINUM directives, if any, into the snippet
    snippet.Code.clear();
    for (int idx = _rememberedCodeLocation; idx < orig_codesize; idx++)
        snippet.Code.push_back(_scrip.code[idx]);
    // Cut the original code
    _scrip.code.resize(static_cast<size_t>(rcl));

    // Assume fixups are ordered by location. Find the first fixup that has been cut out.
    size_t first_cut_fixup;
    for (first_cut_fixup = _scrip.fixups.size(); first_cut_fixup > 0; first_cut_fixup--)
    {
        if (_scrip.fixups[first_cut_fixup - 1] < rcl)
            break;
    }

    // Save the fixups into the snippet and delete them out of _scrip
    snippet.Fixups.clear();
    snippet.FixupTypes.clear();
    for (size_t idx = first_cut_fixup; idx < _scrip.fixups.size(); idx++)
    {
        snippet.Fixups.push_back(_scrip.fixups[idx] - _rememberedCodeLocation);
        snippet.FixupTypes.push_back(_scrip.fixuptypes[idx]);
    }
    _scrip.fixups.resize(first_cut_fixup);
    _scrip.fixuptypes.resize(first_cut_fixup);

    // Assume label locations are ordered by location. Find the first location that has been cut out.
    size_t first_cut_label;
    for (first_cut_label = _scrip.Labels.size(); first_cut_label > 0; first_cut_label--)
    {
        if (_scrip.Labels[first_cut_label - 1] < rcl)
            break;
    }

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
    _jumpDestParamLocs.push_back(_scrip.Codesize_i32() + offset);
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
        _scrip.code[*loc] = _scrip.RelativeJumpDist(*loc, _scrip.Codesize_i32());
    _jumpDestParamLocs.clear();
}

void AGS::BackwardJumpDest::Set(CodeLoc cl)
{
    _dest = (cl >= 0) ? cl : _scrip.Codesize_i32();
    _lastEmittedSrcLineno = _scrip.LastEmittedLineno;
}

void AGS::BackwardJumpDest::WriteJump(CodeCell jump_op, size_t cur_line)
{
    if (SCMD_LINENUM != _scrip.code[_dest] &&
        _scrip.LastEmittedLineno != _lastEmittedSrcLineno)
    {
        _scrip.WriteLineno(cur_line);
    }
    _scrip.WriteCmd(jump_op, _scrip.RelativeJumpDist(_scrip.Codesize_i32() + 1, _dest));
}

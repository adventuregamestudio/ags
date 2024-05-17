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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cs_compiler.h"
#include "cc_compiledscript.h"
#include "cc_symboltable.h"

#include "script/cc_common.h"
#include "script/cc_internal.h"

#include "cs_parser.h"


void ccGetExtensions2(std::vector<std::string> &exts)
{
    // A generic "AGS 4.0" extension, for easier detection
    // of a new compiler. Feel free to add more, specifying each
    // new added feature individually.
    exts.push_back("AGS4");
    // Managed ptr in managed structs
    exts.push_back("NESTEDPOINTERS");
}


// Compiles RTTI table for the given script.
static std::unique_ptr<RTTI> ccCompileRTTI(const SymbolTable &symt, const SectionList &seclist)
{
    RTTIBuilder rtb;
    std::string buf; // for constructing names

    // Add sections as locations
    // CHECKME: do we have to add all?
    const auto &sections = seclist.GetSections();
    for (size_t l = 0; l < sections.size(); ++l)
    {
        rtb.AddLocation(sections[l], l, 0u);
    }

    // Add "no type" with id 0
    rtb.AddType("", 0u, 0u, 0u, 0u, 0u);
    // Scan through all the symbols and save type infos,
    // and gather preliminary data on type fields and strings
    for (size_t t = 0; t < symt.entries.size(); t++)
    {
        const SymbolTableEntry &ste = symt.entries[t];

        // Detect a declared type, atomic or struct (regular and managed);
        // ignore all the compound types (derived from "real" types).
        if (ste.VartypeD && (ste.VartypeD->BaseVartype == 0))
        {
            uint32_t section_id = 0u;
            if (ste.Declared < INT_MAX)
                section_id = seclist.GetSectionIdAt(ste.Declared);
            uint32_t flags = 0u;
            if (ste.VartypeD->Flags[VTF::kManaged])
                flags |= RTTI::kType_Managed;
            if (ste.VartypeD->Flags[VTF::kStruct])
                flags |= RTTI::kType_Struct;
            rtb.AddType(ste.Name, t, section_id, ste.VartypeD->Parent, flags, ste.VartypeD->Size);
        }
        // Detect a struct's mem field (not function or attribute, etc)
        else if (ste.ComponentD && ste.VariableD)
        {
            buf = ste.Name.substr(ste.Name.rfind(":") + 1);
            uint32_t flags = 0u;
            const auto &field_type = symt.entries[ste.VariableD->Vartype];
            if ((field_type.VartypeD->Type == VTT::kDynpointer) ||
                (field_type.VartypeD->Type == VTT::kDynarray))
                flags |= RTTI::kField_ManagedPtr;
            if ((field_type.VartypeD->Type == VTT::kArray) ||
                (field_type.VartypeD->Type == VTT::kDynarray))
                flags |= RTTI::kField_Array;
            uint32_t num_elems = 0u;
            for (const auto sz : field_type.VartypeD->Dims)
                num_elems += sz; // CHECKME if correct
            rtb.AddField(ste.ComponentD->Parent, buf, ste.ComponentD->Offset,
                symt.GetFirstBaseVartype(ste.VariableD->Vartype), flags, num_elems);
        }
    }

    return std::unique_ptr<RTTI>(
        new RTTI(std::move(rtb.Finalize())));
}

static void ccCompileDataTOC(ScriptTOCBuilder &tocb,
    const SymbolTable &symt, const std::vector<SymbolTableEntry> &entries, const SectionList &seclist,
    const RTTI *rtti)
{
    for (size_t t = 0; t < entries.size(); t++)
    {
        const SymbolTableEntry &ste = entries[t];

        // Add global variables, local variables and function parameters
        if (ste.VariableD && !ste.ComponentD && !ste.VariableD->TypeQualifiers[TQ::kImport])
        {
            ScriptTOC::Variable var;
            const auto &field_type = symt.entries[ste.VariableD->Vartype];
            if (!field_type.VartypeD)
                continue; // probably a special "variable" like "this"

            uint32_t f_typeid = symt.GetFirstBaseVartype(ste.VariableD->Vartype);
            uint32_t f_flags = 0u;
            if ((field_type.VartypeD->Type == VTT::kDynpointer) ||
                (field_type.VartypeD->Type == VTT::kDynarray))
                f_flags |= RTTI::kField_ManagedPtr;
            if ((field_type.VartypeD->Type == VTT::kArray) ||
                (field_type.VartypeD->Type == VTT::kDynarray))
                f_flags |= RTTI::kField_Array;

            uint32_t num_elems = 0u;
            for (const auto sz : field_type.VartypeD->Dims)
                num_elems += sz; // CHECKME if correct

            uint32_t section_id = 0u;
            if (ste.Declared < INT_MAX)
                section_id = seclist.GetSectionIdAt(ste.Declared);

            // Global variable
            if (ste.Scope == 0u)
            {
                tocb.AddGlobalVar(ste.Name, section_id, ste.VariableD->Offset,
                    0u /* v_flags */, f_typeid, f_flags, num_elems);
            }
            // Scoped variable: local or parameter
            else
            {
                uint32_t v_flags = ScriptTOC::kVariable_Local;
                if (ste.Scope == SymbolTableConstant::kParameterScope)
                    v_flags |= ScriptTOC::kVariable_Parameter;
                tocb.AddLocalVar(ste.Name, section_id, ste.VariableD->Offset,
                    ste.LifeScope.first, ste.LifeScope.second,
                    v_flags, f_typeid, f_flags, num_elems);
            }
        }

        // Add functions (not import declarations)
        if (ste.FunctionD && !ste.FunctionD->TypeQualifiers[TQ::kImport])
        {
            uint32_t func_flags = 0u;
            if (ste.FunctionD->IsVariadic)
                func_flags |= ScriptTOC::kFunction_Variadic;

            const auto &ret_param = ste.FunctionD->Parameters[0];
            const auto &field_type = symt.entries[ret_param.Vartype];
            uint32_t rval_type = symt.GetFirstBaseVartype(ret_param.Vartype);
            uint32_t rval_flags = 0u;
            if ((field_type.VartypeD->Type == VTT::kDynpointer) ||
                (field_type.VartypeD->Type == VTT::kDynarray))
                rval_flags |= RTTI::kField_ManagedPtr;
            if ((field_type.VartypeD->Type == VTT::kArray) ||
                (field_type.VartypeD->Type == VTT::kDynarray))
                rval_flags |= RTTI::kField_Array;

            uint32_t section_id = 0u;
            if (ste.Declared < INT_MAX)
                section_id = seclist.GetSectionIdAt(ste.Declared);

            const uint32_t func_id =
                tocb.AddFunction(ste.Name, section_id, ste.LifeScope.first, ste.LifeScope.second,
                    func_flags, rval_type, rval_flags);

            // Add function parameters
            for (size_t i = 1; i < ste.FunctionD->Parameters.size(); ++i)
            {
                const auto &param = ste.FunctionD->Parameters[i];
                const auto &field_type = symt.entries[param.Vartype];
                uint32_t param_type = symt.GetFirstBaseVartype(param.Vartype);
                uint32_t param_flags = 0u;
                if ((field_type.VartypeD->Type == VTT::kDynpointer) ||
                    (field_type.VartypeD->Type == VTT::kDynarray))
                    param_flags |= RTTI::kField_ManagedPtr;
                if ((field_type.VartypeD->Type == VTT::kArray) ||
                    (field_type.VartypeD->Type == VTT::kDynarray))
                    param_flags |= RTTI::kField_Array;
                std::string param_name = symt.entries[param.Name].Name;
                tocb.AddFunctionParam(func_id, param_name, 0u /* TODO: param offset? */,
                    param_type, param_flags);
            }
        }
    }
}

static std::unique_ptr<ScriptTOC> ccCompileDataTOC(const SymbolTable &symt, const SectionList &seclist, const RTTI *rtti)
{
    ScriptTOCBuilder tocb;

    ccCompileDataTOC(tocb, symt, symt.entries, seclist, rtti);
    ccCompileDataTOC(tocb, symt, symt.localEntries, seclist, rtti);

    return std::unique_ptr<ScriptTOC>(
        new ScriptTOC(std::move(tocb.Finalize(rtti))));
}


ccScript *ccCompileText2(std::string const &script, std::string const &scriptName, uint64_t const options, MessageHandler &mh)
{
    ccCompiledScript *compiled_script =
        new ccCompiledScript(FlagIsSet(options, SCOPT_LINENUMBERS));
    SymbolTable symt; // for gathering rtti
    SectionList seclist;

    compiled_script->StartNewSection(scriptName.empty() ? scriptName : "Unnamed script");
    cc_compile(script, options, *compiled_script, symt, seclist, mh);
    if (mh.HasError())
    {
        auto const &err = mh.GetError();

        constexpr size_t buffer_size = 256;
        static char message_buffer[buffer_size];
        message_buffer[0] = '!';
        strncpy_s(
            message_buffer + 1,
            buffer_size - 1,
            err.Message.c_str(),
            err.Message.length() + 1);

        static char section_buffer[buffer_size];
        strncpy_s(
            section_buffer,
            buffer_size,
            err.Section.c_str(),
            err.Section.length() + 1);

        ccCurScriptName = section_buffer;
        currentline = err.Lineno;
        if (mh.kSV_InternalError == err.Severity)
            cc_error(message_buffer + 1); // Don't have leading '!'
        else
            cc_error(message_buffer); // Have leading '!'
    
        delete compiled_script; // Note: delete calls the destructor
        return NULL;
    }

    // Construct RTTI
    if (FlagIsSet(options, SCOPT_RTTI))
    {
        compiled_script->rtti = ccCompileRTTI(symt, seclist);
    }

    if (FlagIsSet(options, SCOPT_SCRIPT_TOC))
    {
        compiled_script->sctoc = ccCompileDataTOC(symt, seclist, compiled_script->rtti.get());
    }

    ccCurScriptName = nullptr;
    cc_clear_error();
    compiled_script->FreeExtra();
    return compiled_script;
}

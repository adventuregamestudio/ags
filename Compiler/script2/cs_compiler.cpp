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
        rtb.AddLocation(sections[l], l);
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
            if (field_type.VartypeD->Type == VTT::kArray)
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

    compiled_script->rtti = ccCompileRTTI(symt, seclist);

    ccCurScriptName = nullptr;
    cc_clear_error();
    compiled_script->FreeExtra();
    return compiled_script;
}

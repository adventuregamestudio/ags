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
#include <algorithm>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script/cs_compiler.h"
#include "script/cc_compiledscript.h"
#include "script/cc_symboltable.h"
#include "script/cc_common.h"
#include "script/cc_internal.h"
#include "script/cs_parser.h"

const char *ccSoftwareVersion = "1.0";

std::vector<const char*> defaultheaders;
std::vector<const char*> defaultHeaderNames;

void ccGetExtensions(std::vector<std::string> &exts)
{
    // TODO: we may consider creating a managed user object an extension, etc,
    // although it was introduced years ago when extensions were not declared.

    // Managed ptr in managed structs
    exts.push_back("NESTEDPOINTERS");
    return;
}

int ccAddDefaultHeader(const char* nhead, const char *nName)
{
    defaultheaders.push_back(nhead);
    defaultHeaderNames.push_back(nName);
    return 0;
}

void ccRemoveDefaultHeaders() {
    defaultheaders.clear();
    defaultHeaderNames.clear();
}

void ccSetSoftwareVersion(const char *versionNumber) {
    ccSoftwareVersion = versionNumber;
}


// Convert SFLG_* flags to RTTI::TypeFlags
inline uint32_t SflgToRTTIType(int ste_flags)
{
    return 0u
        | RTTI::kType_Struct * ((ste_flags & SFLG_STRUCTTYPE) != 0)
        | RTTI::kType_Managed * ((ste_flags & SFLG_MANAGED) != 0);
}

// Convert SFLG_* flags to RTTI::FieldFlags
inline uint32_t SflgToRTTIField(int ste_flags)
{
    return 0u
        | RTTI::kField_ManagedPtr * ((ste_flags & SFLG_DYNAMICARRAY) != 0 ||
            // compiler marks static arrays of pointers as SFLG_POINTER;
            // but the array itself is not a pointer, so don't mark that as one
            (((ste_flags & SFLG_POINTER) != 0) && ((ste_flags & SFLG_ARRAY) == 0)))
        | RTTI::kField_Array * ((ste_flags & SFLG_ARRAY) != 0);
}

// Convert STYPE_* flags to RTTI::FieldFlags
inline uint32_t StypeToRTTIField(int stype_flags)
{
    return 0u
        | RTTI::kField_ManagedPtr * ((stype_flags & STYPE_DYNARRAY) != 0 || (stype_flags & STYPE_POINTER) != 0);
}

// Compiles RTTI table for the given script.
static std::unique_ptr<RTTI> ccCompileRTTI(const symbolTable &sym)
{
    RTTIBuilder rtb;
    std::string buf; // for constructing names

    // Add sections as locations
    // CHECKME: do we have to add all?
    for (size_t l = 0; l < sym.sections.size(); ++l)
    {
        rtb.AddLocation(sym.sections[l], l, 0u);
    }

    // Add "no type" with id 0
    rtb.AddType("", 0u, 0u, 0u, 0u, 0u);
    // Scan through all the symbols and save type infos,
    // and gather preliminary data on type fields and strings
    for (size_t t = 0; t < sym.entries.size(); ++t)
    {
        const SymbolTableEntry &ste = sym.entries[t];

        if ((ste.stype == SYM_VARTYPE) || ((ste.flags & SFLG_STRUCTTYPE) != 0) ||
            ((ste.flags & SFLG_MANAGED) != 0))
        {
            uint32_t flags = SflgToRTTIType(ste.flags);
            rtb.AddType(ste.sname, t, ste.section, ste.extends, flags, ste.ssize);
        }
        else if ((ste.stype == SYM_STRUCTMEMBER) && ((ste.flags & SFLG_STRUCTMEMBER) != 0) &&
            ((ste.flags & SFLG_PROPERTY) == 0))
        {
            buf = ste.sname.substr(ste.sname.rfind(":") + 1);
            uint32_t flags = SflgToRTTIField(ste.flags);
            rtb.AddField(ste.extends, buf, ste.soffs, ste.vartype, flags,
                static_cast<uint32_t>(ste.arrsize));
        }
    }

    return std::unique_ptr<RTTI>(
        new RTTI(std::move(rtb.Finalize())));
}

static void ccCompileDataTOC(ScriptTOCBuilder &tocb,
    const std::vector<SymbolTableEntry> &entries, const RTTI *rtti)
{
    for (size_t t = 0; t < entries.size(); ++t)
    {
        const SymbolTableEntry &ste = entries[t];

        // Add global variables (SYM_GLOBALVAR), local variables and function parameters (SYM_LOCALVAR)
        // NOTE: we add imported variables, because we need to know their declared  types when
        // using TOC for debugging purposes. Actual addresses must be resolved at linking stage.
        // TODO: consider alternate solutions to avoid cluttering each script's TOC with
        // generated game objects. If a better way found, then imported vars may be easily excluded
        // from here without breaking TOC serialization format.
        if (ste.stype == SYM_GLOBALVAR || ste.stype == SYM_LOCALVAR)
        {
            uint32_t f_flags = SflgToRTTIField(ste.flags);
            if (ste.stype == SYM_GLOBALVAR)
            {
                uint32_t v_flags = 0u;
                if (ste.flags & SFLG_IMPORTED)
                    v_flags = ScriptTOC::kVariable_Import;

                tocb.AddGlobalVar(ste.sname, ste.section, ste.soffs,
                    v_flags, ste.vartype, f_flags, static_cast<uint32_t>(ste.arrsize));
            }
            else
            {
                uint32_t v_flags = ScriptTOC::kVariable_Local;
                if (ste.flags & SFLG_PARAMETER)
                    v_flags |= ScriptTOC::kVariable_Parameter;
                tocb.AddLocalVar(ste.sname, ste.section, ste.soffs,
                    ste.scope_section_begin, ste.scope_section_end,
                    v_flags, ste.vartype, f_flags, static_cast<uint32_t>(ste.arrsize));
            }
            
        }

        // Add functions
        // NOTE: we skip imported functions, as these are not useful in TOC at the moment;
        // but if a need arises, we may easily add these as well.
        if (ste.stype == SYM_FUNCTION)
        {
            if (ste.flags & SFLG_IMPORTED)
                continue; // skip function import declarations

            uint32_t func_flags = 0u;
            if (entries[t].is_variadic_function())
                func_flags |= ScriptTOC::kFunction_Variadic;
            // NOTE: old compiler saves function's return value and parameter flags
            // as STYPE_* constants, not SFLG_*
            uint32_t rval_type = entries[t].funcparams[0].Type & STYPE_MASK;
            int stype_flags = entries[t].funcparams[0].Type & ~STYPE_MASK;
            uint32_t rval_flags = StypeToRTTIField(stype_flags);
            const uint32_t func_id =
                tocb.AddFunction(ste.sname, ste.section, ste.scope_section_begin, ste.scope_section_end,
                    func_flags, rval_type, rval_flags);

            // Add function parameters
            for (int i = 1; i < ste.get_num_args() + 1; ++i)
            {
                uint32_t param_type = entries[t].funcparams[i].Type & STYPE_MASK;
                int stype_flags = entries[t].funcparams[i].Type & ~STYPE_MASK;
                const std::string &name = entries[t].funcparams[i].Name;
                uint32_t param_flags = StypeToRTTIField(stype_flags);
                tocb.AddFunctionParam(func_id, name, 0u /* TODO: param offset? */,
                    param_type, param_flags);
            }
        }
    }
}

static std::unique_ptr<ScriptTOC> ccCompileDataTOC(const symbolTable &sym, const RTTI *rtti)
{
    ScriptTOCBuilder tocb;

    ccCompileDataTOC(tocb, sym.entries, rtti);
    ccCompileDataTOC(tocb, sym.localEntries, rtti);

    return std::unique_ptr<ScriptTOC>(
        new ScriptTOC(std::move(tocb.Finalize(rtti))));
}

ccScript* ccCompileText(const char *texo, const char *scriptName) {
    ccCompiledScript *cctemp = new ccCompiledScript();

    sym.reset();

    if (scriptName == NULL)
        scriptName = "Main script";

    cc_clear_error();

    for (size_t t=0;t<defaultheaders.size();t++) {
        if (defaultHeaderNames[t])
            ccCurScriptName = defaultHeaderNames[t];
        else
            ccCurScriptName = "Internal header file";

        cctemp->start_new_section(ccCurScriptName);
        cc_compile(defaultheaders[t],cctemp);
        if (cc_has_error()) break;
    }

    if (!cc_has_error()) {
        ccCurScriptName = scriptName;
        cctemp->start_new_section(ccCurScriptName);
        cc_compile(texo,cctemp);
    }

    if (cc_has_error()) {
        delete cctemp;
        return NULL;
    }

    for (size_t t=0; t<sym.entries.size();t++) {
        int stype = sym.get_type(t);
        // blank out the name for imports that are not used, to save space
        // in the output file
        if (((sym.entries[t].flags & SFLG_IMPORTED)!=0) && ((sym.entries[t].flags & SFLG_ACCESSED)==0)) {

            if ((stype == SYM_FUNCTION) || (stype == SYM_GLOBALVAR)) {
                // unused func/variable
                cctemp->imports[sym.entries[t].soffs] = std::string();
            }
            else if (sym.entries[t].flags & SFLG_PROPERTY) {
                // unused property -- get rid of the getter and setter
                int propGet = sym.entries[t].get_propget();
                int propSet = sym.entries[t].get_propset();
                if (propGet >= 0)
                    cctemp->imports[propGet] = std::string();
                if (propSet >= 0)
                    cctemp->imports[propSet] = std::string();
            }
        }

        if ((sym.get_type(t) != SYM_GLOBALVAR) &&
            (sym.get_type(t) != SYM_LOCALVAR)) continue;

        if (sym.entries[t].flags & SFLG_IMPORTED) continue;
        if (ccGetOption(SCOPT_SHOWWARNINGS)==0) ;
        else if ((sym.entries[t].flags & SFLG_ACCESSED)==0) {
            printf("warning: variable '%s' is never used\n",sym.get_friendly_name(t).c_str());
        }
    }

    if (ccGetOption(SCOPT_EXPORTALL)) {
        // export all functions
        for (size_t t=0; t < cctemp->functions.size(); ++t) {
            if (cctemp->add_new_export(cctemp->functions[t].c_str(), EXPORT_FUNCTION,
                cctemp->funccodeoffs[t], cctemp->funcnumparams[t]) == -1) {
                    return NULL;
            }

        }
    }

    // Construct RTTI
    if (ccGetOption(SCOPT_RTTI)) {
        cctemp->rtti = ccCompileRTTI(sym);
    }

    // Construct TOC
    if (ccGetOption(SCOPT_SCRIPT_TOC)) {
        cctemp->sctoc = ccCompileDataTOC(sym, cctemp->rtti.get() /* may be null */);
    }

    cctemp->free_extra();
    return cctemp;
}

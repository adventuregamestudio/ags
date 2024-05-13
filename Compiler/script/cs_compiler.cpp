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
#include "util/memory_compat.h"

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
            uint32_t flags = 0u;
            if ((ste.flags & SFLG_STRUCTTYPE))
                flags = RTTI::kType_Struct;
            if ((ste.flags & SFLG_MANAGED))
                flags = RTTI::kType_Managed;
            rtb.AddType(ste.sname, t, ste.section, ste.extends, flags, ste.ssize);
        }
        else if ((ste.stype == SYM_STRUCTMEMBER) && ((ste.flags & SFLG_STRUCTMEMBER) != 0) &&
            ((ste.flags & SFLG_PROPERTY) == 0))
        {
            buf = ste.sname.substr(ste.sname.rfind(":") + 1);
            uint32_t flags = 0u;
            if ((ste.flags & SFLG_DYNAMICARRAY) || (ste.flags & SFLG_POINTER))
                flags |= RTTI::kField_ManagedPtr;
            if (ste.flags & SFLG_ARRAY)
                flags |= RTTI::kField_Array;
            rtb.AddField(ste.extends, buf, ste.soffs, ste.vartype, flags,
                static_cast<uint32_t>(ste.arrsize));
        }
    }

    return std::unique_ptr<RTTI>(
        new RTTI(std::move(rtb.Finalize())));
}

static std::unique_ptr<ScriptDataTOC> ccCompileDataTOC(const symbolTable &sym) {
    auto toc = std::make_unique<ScriptDataTOC>();

    for (size_t t = 0; t < sym.entries.size(); ++t)
    {
        const SymbolTableEntry &ste = sym.entries[t];

        if (ste.stype == SYM_GLOBALVAR)
        {
            ScriptDataTOC::VariableDef var;
            uint32_t flags = 0u;
            if ((ste.flags & SFLG_DYNAMICARRAY) || (ste.flags & SFLG_POINTER))
                flags |= RTTI::kField_ManagedPtr;
            if (ste.flags & SFLG_ARRAY)
                flags |= RTTI::kField_Array;
            if (ste.flags & SFLG_IMPORTED)
            {
                flags |= RTTI::kField_Import;
                continue; // TODO
            }

            var.name = ste.sname.c_str();
            var.f_typeid = ste.vartype;
            var.offset = ste.soffs;
            var.flags = flags;
            var.num_elems = static_cast<uint32_t>(ste.arrsize);
            toc->VarDefs.push_back(var);
        }
    }

    return std::move(toc);
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

    if (ccGetOption(SCOPT_DATATOC)) {
        cctemp->datatoc = ccCompileDataTOC(sym);
    }

    cctemp->free_extra();
    return cctemp;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script/cs_compiler.h"
#include "script/cc_macrotable.h"
#include "script/cc_compiledscript.h"
#include "script/cc_symboltable.h"
#include "script/cc_common.h"
#include "script/cc_internal.h"
#include "script/cs_parser.h"

const char *ccSoftwareVersion = "1.0";

std::vector<const char*> defaultheaders;
std::vector<const char*> defaultHeaderNames;

MacroTable predefinedMacros;

void ccGetExtensions(std::vector<std::string> &exts)
{
    // TODO: we may consider creating a managed user object an extension, etc,
    // although it was introduced years ago when extensions were not declared.
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

ccScript* ccCompileText(const char *texo, const char *scriptName) {
    ccCompiledScript *cctemp = new ccCompiledScript();
    cctemp->init();

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
        cctemp->shutdown();
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
                cctemp->imports[sym.entries[t].soffs][0] = 0;
            }
            else if (sym.entries[t].flags & SFLG_PROPERTY) {
                // unused property -- get rid of the getter and setter
                int propGet = sym.entries[t].get_propget();
                int propSet = sym.entries[t].get_propset();
                if (propGet >= 0)
                    cctemp->imports[propGet][0] = 0;
                if (propSet >= 0)
                    cctemp->imports[propSet][0] = 0;
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
        for (size_t t=0;t<cctemp->numfunctions;t++) {
            if (cctemp->add_new_export(cctemp->functions[t],EXPORT_FUNCTION,
                cctemp->funccodeoffs[t], cctemp->funcnumparams[t]) == -1) {
                    cctemp->shutdown();
                    return NULL;
            }

        }
    }

    cctemp->free_extra();
    return cctemp;
}

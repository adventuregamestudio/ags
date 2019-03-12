#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cs_compiler.h"
#include "cc_macrotable.h"
#include "cc_compiledscript.h"
#include "cc_symboltable.h"

#include "script/cc_error.h"
#include "script/cc_options.h"
#include "script/script_common.h"

#include "cs_prepro.h"
#include "cs_parser.h"

const char *ccSoftwareVersion = "1.0";

struct ScriptHeader
{
    std::string Name;
    std::string Content;
};

std::vector<ScriptHeader> defaultHeaders;

MacroTable predefinedMacros;

int ccAddDefaultHeader(char *hd_content, char *hd_name)
{
    if (!hd_content)
        hd_content = "";
    if (!hd_name)
        hd_name = "Internal Header File";
    struct ScriptHeader head = { std::string(hd_name), std::string(hd_content) };
    defaultHeaders.push_back(head);

    return 0;
}

void ccRemoveDefaultHeaders()
{
    defaultHeaders.clear();
}

void ccSetSoftwareVersion(const char *versionNumber)
{
    ccSoftwareVersion = versionNumber;
}

ccScript *ccCompileText(const char *texo, const char *scriptName)
{
    ccCompiledScript *compiled_script = new ccCompiledScript();
    compiled_script->init();

    sym.reset();

    if (scriptName == NULL)
        scriptName = "Main script";

    ccError = 0;
    ccErrorLine = 0;

    size_t const num_of_headers = defaultHeaders.size();
    for (size_t header = 0; header < num_of_headers; header++)
    {
        ccCurScriptName = defaultHeaders[header].Name.c_str();
        compiled_script->start_new_section(ccCurScriptName);
        cc_compile(defaultHeaders[header].Content.c_str(), compiled_script);
        if (ccError) break;
    }

    if (!ccError)
    {
        ccCurScriptName = scriptName;
        compiled_script->start_new_section(ccCurScriptName);
        cc_compile(texo, compiled_script);
    }

    if (ccError)
    {
        compiled_script->shutdown();
        delete compiled_script;
        return NULL;
    }

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        int stype = sym.get_type(entries_idx);
        // blank out the name for imports that are not used, to save space
        // in the output file
        if (((sym.entries[entries_idx].flags & SFLG_IMPORTED) != 0) && ((sym.entries[entries_idx].flags & SFLG_ACCESSED) == 0))
        {

            if ((stype == SYM_FUNCTION) || (stype == SYM_GLOBALVAR))
            {
                // unused func/variable
                compiled_script->imports[sym.entries[entries_idx].soffs][0] = 0;
            }
            else if (sym.entries[entries_idx].flags & SFLG_ATTRIBUTE)
            {
                // unused attribute -- get rid of the getter and setter
                int attr_get = sym.entries[entries_idx].get_attrget();
                if (attr_get > 0)
                    compiled_script->imports[attr_get][0] = 0;
                int attr_set = sym.entries[entries_idx].get_attrset();
                if (attr_set > 0)
                    compiled_script->imports[attr_set][0] = 0;
            }
        }

        if ((sym.get_type(entries_idx) != SYM_GLOBALVAR) &&
            (sym.get_type(entries_idx) != SYM_LOCALVAR))
            continue;

        if (sym.entries[entries_idx].flags & SFLG_IMPORTED)
            continue;
        if (ccGetOption(SCOPT_SHOWWARNINGS) == 0);
        else if ((sym.entries[entries_idx].flags & SFLG_ACCESSED) == 0)
        {
            printf("warning: variable '%s' is never used\n", sym.get_friendly_name(entries_idx).c_str());
        }
    }

    if (ccGetOption(SCOPT_EXPORTALL))
    {
        // export all functions
        for (size_t func_num = 0; func_num < compiled_script->numfunctions; func_num++)
        {
            if (compiled_script->add_new_export(compiled_script->functions[func_num], EXPORT_FUNCTION,
                compiled_script->funccodeoffs[func_num], compiled_script->funcnumparams[func_num]) == -1)
            {
                compiled_script->shutdown();
                return NULL;
            }

        }
    }

    compiled_script->free_extra();
    return compiled_script;
}

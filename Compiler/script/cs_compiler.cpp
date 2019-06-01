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

void ccCompileText_KillUnusedImports(ccCompiledScript * compiled_script)
{
    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        int stype = sym.get_type(entries_idx);
        // blank out the name for imports that are not used, to save space
        // in the output file
        // Don't mind attributes - they are short cuts for the respective getter
        // and setter funcs. If _those_ are unused, then they will be caught
        // in the same that way normal functions are.
        if (((sym.entries.at(entries_idx).flags & kSFLG_Imported) != 0) && ((sym.entries.at(entries_idx).flags & kSFLG_Accessed) == 0))
            if ((stype == kSYM_Function) || (stype == kSYM_GlobalVar))
                compiled_script->imports[sym.entries.at(entries_idx).soffs][0] = '\0';

        if ((sym.get_type(entries_idx) != kSYM_GlobalVar) &&
            (sym.get_type(entries_idx) != kSYM_LocalVar))
            continue;

        if (sym.entries.at(entries_idx).flags & kSFLG_Imported)
            continue;
        if (ccGetOption(SCOPT_SHOWWARNINGS) == 0)
            ;
        else if ((sym.entries.at(entries_idx).flags & kSFLG_Accessed) == 0)
            printf("warning: variable '%s' is never used\n", sym.get_name_string(entries_idx).c_str());
    }
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
    std::string sourcecode = "";
    // Note: Currently, all compilations that arrive here are set up in the following way:
    // 1. The defaultHeaders list is blanked out (ccRemoveDefaultHeaders())
    // 2. Then all the necessary headers are added (ccAddDefaultHeader() repeatedly);
    // 3. Then compilation proper starts.
    // So in essence, we don't re-use anything; we start from scratch each time.
    // Compiling the headers separately doesn't save any time in this case,
    // so we just concatenate all sources and present them to the compiler as one.
    // We will still know what source starts where, because currently all the
    // sources start with a __NEWSCRIPTSTART_ token
    for (size_t header = 0; header < num_of_headers; header++)
        sourcecode += defaultHeaders[header].Content;
    sourcecode += texo;
    ccCurScriptName = scriptName;
    compiled_script->start_new_section(ccCurScriptName);
    cc_compile(sourcecode.c_str(), compiled_script);

    if (ccError)
    {
        compiled_script->shutdown();
        delete compiled_script;
        return NULL;
    }

    ccCompileText_KillUnusedImports(compiled_script);

    if (ccGetOption(SCOPT_EXPORTALL))
    {
        // export all functions
        for (size_t func_num = 0; func_num < compiled_script->functions.size(); func_num++)
        {
            if (-1 == compiled_script->add_new_export(
                compiled_script->functions[func_num].Name,
                EXPORT_FUNCTION,
                compiled_script->functions[func_num].CodeOffs,
                compiled_script->functions[func_num].NumOfParams))
            {
                compiled_script->shutdown();
                return NULL;
            }
        }
    }

    compiled_script->free_extra();
    return compiled_script;
}

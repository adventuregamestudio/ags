#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cs_compiler.h"
#include "cc_compiledscript.h"
#include "cc_symboltable.h"

#include "script/cc_error.h"
#include "script/cc_options.h"
#include "script/script_common.h"

#include "cs_parser.h"

const char *ccSoftwareVersion = "1.0";

struct ScriptHeader
{
    std::string Name;
    std::string Content;
};

std::vector<ScriptHeader> defaultHeaders;

int ccAddDefaultHeader(char *hd_content, char *hd_name)
{
    struct ScriptHeader head =
        { std::string(hd_name? hd_name : "Internal Header File"),
          std::string(hd_content? hd_content : "") };
    defaultHeaders.push_back(head);

    return 0;
}

void ccRemoveDefaultHeaders()
{
    defaultHeaders.clear();
}

void ccSetSoftwareVersion(const char *version)
{
    ccSoftwareVersion = version;
}

ccScript *ccCompileText(const char *script, const char *scriptName)
{
    long const options =
        SCOPT_EXPORTALL * ccGetOption(SCOPT_EXPORTALL) |
        SCOPT_SHOWWARNINGS * ccGetOption(SCOPT_SHOWWARNINGS) |
        SCOPT_LINENUMBERS * ccGetOption(SCOPT_LINENUMBERS) |
        SCOPT_AUTOIMPORT * ccGetOption(SCOPT_AUTOIMPORT) |
        SCOPT_DEBUGRUN * ccGetOption(SCOPT_DEBUGRUN) |
        SCOPT_NOIMPORTOVERRIDE * ccGetOption(SCOPT_NOIMPORTOVERRIDE) |
        SCOPT_OLDSTRINGS * ccGetOption(SCOPT_OLDSTRINGS) |
        false;
    return ccCompileText(script, scriptName, options);
}

ccScript *ccCompileText(const char *script, const char *scriptName, long options)
{
    ccCompiledScript *compiled_script =
        new ccCompiledScript(0 != FlagIsSet(options, SCOPT_LINENUMBERS));

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
    sourcecode += script;
    ccCurScriptName = scriptName;
    compiled_script->start_new_section(ccCurScriptName);
    cc_compile(sourcecode, *compiled_script);

    if (ccError)
    {
        delete compiled_script; // Note: delete calls the destructor
        return NULL;
    }

    // Sanity check for IMPORT fixups
    for (size_t fixup_idx = 0; fixup_idx < static_cast<size_t>(compiled_script->numfixups); fixup_idx++)
    {
        if (FIXUP_IMPORT != compiled_script->fixuptypes[fixup_idx])
            continue;
        int const code_idx = compiled_script->fixups[fixup_idx];
        if (code_idx < 0 || code_idx >= compiled_script->codesize)
        {
            cc_error(
                "!Fixup #%d references non-existent code offset #%d",
                fixup_idx,
                code_idx);
            delete compiled_script; // Note: delete calls the destructor
            return NULL;
        }
        int const cv = compiled_script->code[code_idx];
        if (cv < 0 || cv >= compiled_script->numimports ||
            '\0' == compiled_script->imports[cv][0])
        {
            cc_error(
                "!Fixup #%d references non-existent import #%d",
                fixup_idx,
                cv);
            delete compiled_script; // Note: delete calls the destructor
            return NULL;
        }
    }

    if (FlagIsSet(options, SCOPT_EXPORTALL))
    {
        // export all functions
        for (size_t func_num = 0; func_num < compiled_script->Functions.size(); func_num++)
        {
            if (-1 == compiled_script->AddExport(
                compiled_script->Functions[func_num].Name,
                compiled_script->Functions[func_num].CodeOffs,
                compiled_script->Functions[func_num].NumOfParams))
            {
                cc_error("Export function failed");
                delete compiled_script; // Note: delete calls the destructor
                return NULL;
            }
        }
    }

    compiled_script->FreeExtra();
    return compiled_script;
}

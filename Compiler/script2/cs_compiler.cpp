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
    // All preprocessing is done elsewhere; only preprocessed files arrive here.
    // So there's no need to keep track of the version any longer.
    // Note: Compiler options keep track on whether old-style strings are allowed
}

// A  wrapper around cc_compile(), in order to squeeze the C++ style parameters 
// through the limited means of Managed C++ (CLR) into the C# Editor.
ccScript *ccCompileText(const char *script, const char *scriptName)
{
    // All warnings and the error (if present) end up here.
    // TODO: This is what will need to be sqeezed through the interface
    // into the editor so that the editor can display the warnings, too
    MessageHandler mh;

    long const options =
        SCOPT_EXPORTALL * ccGetOption(SCOPT_EXPORTALL) |
        SCOPT_SHOWWARNINGS * ccGetOption(SCOPT_SHOWWARNINGS) |
        SCOPT_LINENUMBERS * ccGetOption(SCOPT_LINENUMBERS) |
        SCOPT_AUTOIMPORT * ccGetOption(SCOPT_AUTOIMPORT) |
        SCOPT_DEBUGRUN * ccGetOption(SCOPT_DEBUGRUN) |
        SCOPT_NOIMPORTOVERRIDE * ccGetOption(SCOPT_NOIMPORTOVERRIDE) |
        SCOPT_OLDSTRINGS * ccGetOption(SCOPT_OLDSTRINGS) |
        false;
    
    ccCompiledScript *compiled_script =
        new ccCompiledScript(0 != FlagIsSet(options, SCOPT_LINENUMBERS));

    std::string sourcecode = "";
    for (size_t header = 0; header < defaultHeaders.size(); header++)
        sourcecode += defaultHeaders[header].Content;
    sourcecode += script;

    compiled_script->StartNewSection(scriptName ? scriptName : "Unnamed script");
    int const error_code = cc_compile(sourcecode, options, *compiled_script, mh);
    if (error_code < 0)
    {
        auto const &err = mh.GetError();
        static char buffer[256];
        ccCurScriptName = buffer;
        strncpy_s(
            buffer,
            err.Section.c_str(),
            sizeof(buffer) / sizeof(char) - 1);
        currentline = err.Lineno;
        cc_error(err.Message.c_str());
    
        delete compiled_script; // Note: delete calls the destructor
        return NULL;
    }

    ccError = 0;
    ccErrorLine = 0;
    compiled_script->FreeExtra();
    return compiled_script;
}

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

// A  wrapper around cc_compile(), in order to squeeze the C++ style parameters 
// through the limited means of Managed C++ (CLR) into the C# Editor.
ccScript *ccCompileText2(char const *script, char const *scriptName)
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

    compiled_script->StartNewSection(scriptName ? scriptName : "Unnamed script");
    int const error_code = cc_compile(script, options, *compiled_script, mh);
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
    ccCurScriptName = nullptr;
    ccError = 0;
    ccErrorLine = 0;
    compiled_script->FreeExtra();
    return compiled_script;
}

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
ccScript *ccCompileText2(std::string const &script, std::string const &scriptName, long const options, MessageHandler &mh)
{
    ccCompiledScript *compiled_script =
        new ccCompiledScript(FlagIsSet(options, SCOPT_LINENUMBERS));

    compiled_script->StartNewSection(scriptName.empty() ? scriptName : "Unnamed script");
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

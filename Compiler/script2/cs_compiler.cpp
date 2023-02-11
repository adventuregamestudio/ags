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

ccScript *ccCompileText2(std::string const &script, std::string const &scriptName, uint64_t const options, MessageHandler &mh)
{
    ccCompiledScript *compiled_script =
        new ccCompiledScript(FlagIsSet(options, SCOPT_LINENUMBERS));

    compiled_script->StartNewSection(scriptName.empty() ? scriptName : "Unnamed script");
    cc_compile(script, options, *compiled_script, mh);
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

    ccCurScriptName = nullptr;
    cc_clear_error();
    compiled_script->FreeExtra();
    return compiled_script;
}

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#ifdef _DEBUG

#include <string.h>
#include "ac/game_version.h"
#include "debug/assert.h"
#include "script/script_api.h"
#include "script/runtimescriptvalue.h"

const char *ScriptVSprintf__(char *buffer, size_t buf_length, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const char *res_buffer = ScriptVSprintf(buffer, buf_length, format, args);
    va_end(args);
    return res_buffer;
}

void Test_ScriptSprintf()
{
    const int argi = 123;
    const float argf = 0.456F;
    const char *argcc = "string literal";
    RuntimeScriptValue params[10];
    params[0].SetInt32(argi);
    params[1].SetFloat(argf);
    params[2].SetStringLiteral(argcc);

    char ScSfBuffer[STD_BUFFER_SIZE];
    //
    // Called-from-script variant
    //
    // Correct format, extra placeholder
    const char *result = 
        ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE,
                      "testing ScriptSprintf:\nThis is int: %10d\nThis is float: %.4f\nThis is string: '%s'\nThis placeholder will be ignored: %d",
                      params, 3);
    assert(strcmp(result, "testing ScriptSprintf:\nThis is int:        123\nThis is float: 0.4560\nThis is string: 'string literal'\nThis placeholder will be ignored: %d") == 0);
    // Literal percent sign
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%d%%", params, 3);
    assert(strcmp(result, "123%") == 0);
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "123%%", NULL, 0);
    assert(strcmp(result, "123%") == 0);
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%%5d%%0.5f%%s", params, 3);
    assert(strcmp(result, "%5d%0.5f%s") == 0);

    // Invalid format
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%zzzzzz", params, 3);
    assert(strcmp(result, "%zzzzzz") == 0);
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%12.34%d", params, 3);
    assert(strcmp(result, "%12.34123") == 0);

    // Not enough arguments
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%5d%0.5f%s", params, 0);
    assert(strcmp(result, "%5d%0.5f%s") == 0);

    // Not enough buffer space
    result = ScriptSprintf(ScSfBuffer, 9, "12345678%d", params, 3);
    assert(strcmp(result, "12345678") == 0);
    result = ScriptSprintf(ScSfBuffer, 11, "12345678%d", params, 3);
    assert(strcmp(result, "1234567812") == 0);
    // Not enough buffer space and not enough params
    result = ScriptSprintf(ScSfBuffer, 10, "12345678%d", params, 0);
    assert(strcmp(result, "12345678%") == 0);
    result = ScriptSprintf(ScSfBuffer, 11, "12345678%d", params, 0);
    assert(strcmp(result, "12345678%d") == 0);

    //
    // Called-from-plugin variant
    // Note that since this is variadic function, number of parameters must
    // always be equal or higher than number of placeholders in buffer string.
    //
    // Correct format, matching number of arguments
    result = 
        ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE,
        "testing ScriptVSprintf:\nThis is int: %10d\nThis is float: %.4f\nThis is string: '%s'\n",
        argi, argf, argcc);
    assert(strcmp(result, "testing ScriptVSprintf:\nThis is int:        123\nThis is float: 0.4560\nThis is string: 'string literal'\n") == 0);
    // Literal percent sign
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%d%%", argi);
    assert(strcmp(result, "123%") == 0);
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "123%%");
    assert(strcmp(result, "123%") == 0);
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%%5d%%0.5f%%s");
    assert(strcmp(result, "%5d%0.5f%s") == 0);

    // Invalid format
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%zzzzzz", argi, argf, argcc);
    assert(strcmp(result, "%zzzzzz") == 0);
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%12.34%d", argi, argf, argcc);
    assert(strcmp(result, "%12.34123") == 0);

    // Not enough buffer space
    result = ScriptVSprintf__(ScSfBuffer, 9, "12345678%d", argi, argf, argcc);
    assert(strcmp(result, "12345678") == 0);
    result = ScriptVSprintf__(ScSfBuffer, 11, "12345678%d", argi, argf, argcc);
    assert(strcmp(result, "1234567812") == 0);
}

#endif // _DEBUG

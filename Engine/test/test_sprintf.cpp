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
#include "debug/assert.h"
#include "script/script_api.h"
#include "script/runtimescriptvalue.h"

void Test_ScriptSprintf()
{
    RuntimeScriptValue params[10];

    params[0].SetInt32(123);
    params[1].SetFloat(0.456F);
    params[2].SetStringLiteral("string literal");

    // Correct format, extra placeholder
    const char *result = 
        ScriptSprintf(ScSfBuffer, 3000,
                      "testing ScriptSprintf:\nThis is int: %10d\nThis is float: %.4f\nThis is string: '%s'\nThis placeholder will be ignored: %d",
                      params, 3);
    assert(strcmp(result, "testing ScriptSprintf:\nThis is int:        123\nThis is float: 0.4560\nThis is string: 'string literal'\nThis placeholder will be ignored: %d") == 0);
    // Literal percent sign
    result = ScriptSprintf(ScSfBuffer, 3000, "aaa%%aaa", params, 3);
    assert(strcmp(result, "aaa%aaa") == 0);

    // Invalid format
    result = ScriptSprintf(ScSfBuffer, 3000, "%zzzzzz", params, 3);
    assert(strcmp(result, "%zzzzzz") == 0);
    result = ScriptSprintf(ScSfBuffer, 3000, "%12.34%d", params, 3);
    assert(strcmp(result, "%12.34123") == 0);

    // Not enough arguments
    result = ScriptSprintf(ScSfBuffer, 3000, "%5d%0.5f%s", params, 0);
    assert(strcmp(result, "%5d%0.5f%s") == 0);

    // Not enough buffer space
    result = ScriptSprintf(ScSfBuffer, 8, "12345678%d", params, 3);
    assert(strcmp(result, "12345678") == 0);
    result = ScriptSprintf(ScSfBuffer, 10, "12345678%d", params, 3);
    assert(strcmp(result, "1234567812") == 0);
    // Not enough buffer space and not enough params
    result = ScriptSprintf(ScSfBuffer, 9, "12345678%d", params, 0);
    assert(strcmp(result, "12345678%") == 0);
    result = ScriptSprintf(ScSfBuffer, 10, "12345678%d", params, 0);
    assert(strcmp(result, "12345678%d") == 0);
    
}

#endif // _DEBUG

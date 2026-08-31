//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
// 
// Stubs for the certain project-dependent script-related cc_* functions.
// 
//=============================================================================
#include "util/string.h"

using namespace AGS::Common;

String cc_format_error(const String &message)
{
    return message;
}

String cc_get_callstack(int max_lines)
{
    return {};
}

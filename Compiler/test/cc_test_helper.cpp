//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <string>
#include "util/string_compat.h"
#include "util/string.h"

extern int currentline; // in script/script_common

typedef AGS::Common::String AGSString;

std::string last_cc_error_buf;

void clear_error(void)
{
    last_cc_error_buf.clear();
}

const char* last_seen_cc_error(void)
{
    return last_cc_error_buf.c_str();
}

// Reimplementation of project-dependent functions from Common
// IMPORTANT: the last_seen_cc_error must contain unformatted error message.
// It is being used in test and compared to hard-coded strings.
AGSString cc_format_error(const AGSString &message)
{
    last_cc_error_buf = message.GetCStr();
    return message;
}

AGSString cc_get_callstack(int max_lines)
{
    return "";
}

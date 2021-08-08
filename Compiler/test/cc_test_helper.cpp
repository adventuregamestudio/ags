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

#include <string>
#include "util/string_compat.h"
#include "script/cc_error.h"

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

// IMPORTANT: the last_seen_cc_error must contain unformatted error message.
// It is being used in test and compared to hard-coded strings.
std::pair<AGSString, AGSString> cc_error_at_line(const char* error_msg)
{
    // printf("error: %s\n", error_msg);
    last_cc_error_buf = ags_strdup(error_msg);
    return std::make_pair(AGSString::FromFormat("Error (line %d): %s", currentline, error_msg), AGSString());
}

AGSString cc_error_without_line(const char* error_msg)
{
    last_cc_error_buf = ags_strdup(error_msg);
    return AGSString::FromFormat("Error (line unknown): %s", error_msg);
}
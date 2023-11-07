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
//
// helper library for compiler testing
//
//=============================================================================

#ifndef __CC_TEST_HELPER_H
#define __CC_TEST_HELPER_H

#include "util/string.h"

extern void clear_error(void);
extern const char *last_seen_cc_error(void);
extern std::pair<AGS::Common::String, AGS::Common::String> cc_error_at_line(const char* error_msg);
extern AGS::Common::String cc_error_without_line(const char* error_msg);

#endif // __CC_TEST_HELPER_H
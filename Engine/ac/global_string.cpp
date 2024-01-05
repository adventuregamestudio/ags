//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include <string.h>
#include "ac/common.h"
#include "ac/global_string.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "util/string_compat.h"

int StrGetCharAt (const char *strin, int posn) {
    if ((posn < 0) || (static_cast<size_t>(posn) >= strlen(strin)))
        return 0;
    return strin[posn];
}

void StrSetCharAt (char *strin, int posn, int nchar) {
    size_t len = strlen(strin);
    if ((posn < 0) || (static_cast<size_t>(posn) > len) || (posn >= MAX_MAXSTRLEN))
        quit("!StrSetCharAt: tried to write past end of string");

    strin[posn] = static_cast<char>(nchar);
    if (static_cast<size_t>(posn) == len)
        strin[posn + 1] = 0;
    commit_scstr_update(strin);
}

void _sc_strcat(char*s1, const char*s2) {
    VALIDATE_STRING(s2);
    size_t buflen = check_scstrcapacity(s1);
    size_t s1_len = strlen(s1);
    size_t buf_avail = (buflen - s1_len);
    snprintf(s1 + s1_len, buf_avail, "%s", s2);
    commit_scstr_update(s1);
}

void _sc_strlower (char *desbuf) {
    VALIDATE_STRING(desbuf);
    ags_strlwr(desbuf);
    commit_scstr_update(desbuf);
}

void _sc_strupper (char *desbuf) {
    VALIDATE_STRING(desbuf);
    ags_strupr(desbuf);
    commit_scstr_update(desbuf);
}

void _sc_strcpy(char*destt, const char *text) {
    VALIDATE_STRING(destt);
    size_t buflen = check_scstrcapacity(destt);
    snprintf(destt, buflen, "%s", text);
    commit_scstr_update(destt);
}

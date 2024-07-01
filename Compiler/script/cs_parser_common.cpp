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
#include "cs_parser_common.h"

int is_whitespace(char cht) {
    // space, tab, EOF, VT (dunno, Chrille had this char appearing)
    if ((cht==' ') || (cht==9) || (cht == 26) || (cht == 11)) return 1;
    return 0;
}

void skip_whitespace(char**pttt) {
    char*mpt=pttt[0];
    while (is_whitespace(mpt[0])) mpt++;
    pttt[0]=mpt;
}

int is_digit(int chrac) {
    if ((chrac >= '0') && (chrac <= '9')) return 1;
    return 0;
}

int is_alphanum(int chrac) {
    if ((chrac>='A') & (chrac<='Z')) return 1;
    if ((chrac>='a') & (chrac<='z')) return 1;
    if ((chrac>='0') & (chrac<='9')) return 1;
    if (chrac == '_') return 1;
    if (chrac == '\"') return 1;
    if (chrac == '\'') return 1;
    return 0;
}

int get_escaped_char(char c)
{
    switch (c)
    {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    default: return c;
    }
}

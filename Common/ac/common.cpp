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
#include "ac/common.h"
#include "util/string.h"

using namespace AGS::Common;
int our_eip = 0;

void quit(const String &str)
{
    quit(str.GetCStr());
}

void quitprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    String text = String::FromFormatV(fmt, ap);
    va_end(ap);
    quit(text);
}

void set_our_eip(int eip)
{
    our_eip = eip;
}

int get_our_eip()
{
    return our_eip;
}

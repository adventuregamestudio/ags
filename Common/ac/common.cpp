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
#include "ac/common.h"
#include "util/string.h"

using namespace AGS::Common;

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

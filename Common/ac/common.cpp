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

#include "ac/common.h"
#include "util/string.h"

using namespace AGS::Common;

const char *croom_h_copyright = "ChrisRoom v2.00 - CRM reader/writer copyright (c) 1995, 1998, 1999 by Chris Jones.";
const char *game_file_sig = "Adventure Creator Game File v2";

void quitprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    String text = String::FromFormatV(fmt, ap);
    va_end(ap);
    quit(text.GetCStr());
}

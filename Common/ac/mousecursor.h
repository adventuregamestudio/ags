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
#ifndef __AC_MOUSECURSOR_H
#define __AC_MOUSECURSOR_H

#include "util/stream.h"
#include "util/string.h"

#define MCF_ANIMMOVE 1
#define MCF_DISABLED 2
#define MCF_STANDARD 4
#define MCF_HOTSPOT  8  // only animate when over hotspot

#define LEGACY_MAX_CURSOR_NAME_LENGTH 10

enum CursorSvgVersion
{
    kCursorSvgVersion_Initial = 0,
    kCursorSvgVersion_36016   = 1, // animation delay
};

// IMPORTANT: exposed to plugin API as AGSCursor!
// do not change topmost fields, unless planning breaking compatibility.
struct MouseCursor {
    int   pic = 0;
    short hotx = 0, hoty = 0;
    short view = -1;
    // This is a deprecated name field, but must stay here for compatibility
    // with the plugin API (unless the plugin interface is reworked)
    char  legacy_name[LEGACY_MAX_CURSOR_NAME_LENGTH]{};
    char  flags = 0;

    // Following fields are not part of the plugin API
    AGS::Common::String name;
    int   animdelay = 5;

    MouseCursor() = default;

    void ReadFromFile(AGS::Common::Stream *in);
    void WriteToFile(AGS::Common::Stream *out);
    void ReadFromSavegame(AGS::Common::Stream *in, int cmp_ver);
    void WriteToSavegame(AGS::Common::Stream *out) const;
};

#endif // __AC_MOUSECURSOR_H
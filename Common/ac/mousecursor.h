//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MCF_ANIMMOVE 1
#define MCF_DISABLED 2
#define MCF_STANDARD 4
#define MCF_HOTSPOT  8  // only animate when over hotspot

#define MAX_CURSOR_NAME_LENGTH 10

// IMPORTANT: exposed to plugin API as AGSCursor!
// do not change topmost fields, unless planning breaking compatibility.
struct MouseCursor {
    int   pic = 0;
    short hotx = 0, hoty = 0;
    short view = -1;
    char  name[MAX_CURSOR_NAME_LENGTH]{};
    char  flags = 0;

    // up to here is a part of plugin API
    int   animdelay = 5;

    MouseCursor() = default;

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
    void ReadFromSavegame(Common::Stream *in, int cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AC_MOUSECURSOR_H
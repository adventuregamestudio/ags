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

#ifndef __AC_MOUSECURSOR_H
#define __AC_MOUSECURSOR_H

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MCF_ANIMMOVE 1
#define MCF_DISABLED 2
#define MCF_STANDARD 4
#define MCF_HOTSPOT  8  // only animate when over hotspot

enum CursorSvgVersion
{
    kCursorSvgVersion_Initial = 0,
    kCursorSvgVersion_36016   = 1, // animation delay
};


struct MouseCursor {
    int   pic = 0;
    short hotx = 0, hoty = 0;
    short view = -1;
    char  name[10]{};
    char  flags = 0;
    int   animdelay = 5;

    MouseCursor() = default;

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
    void ReadFromSavegame(Common::Stream *in, int cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AC_MOUSECURSOR_H
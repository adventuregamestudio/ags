//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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

// Cursor flags
#define MCF_ANIMMOVE 0x01
#define MCF_DISABLED 0x02  // enabled for the cursor-cycling and display
#define MCF_STANDARD 0x04  // a part of the cursor-cycling
#define MCF_HOTSPOT  0x08  // only animate when over hotspot
#define MCF_EVENT    0x10  // generates interaction events

#define LEGACY_MAX_CURSOR_NAME_LENGTH 10

// Standard cursor roles.
// These are used in built-in engine's behavior.
enum CursorRole
{
    kCursorRole_None        = -1,
    kCursorRole_Walk        = 0,
    kCursorRole_Look        = 1,
    kCursorRole_Interact    = 2,
    kCursorRole_UseInv      = 4,
    kCursorRole_Pointer     = 6,
    kCursorRole_Wait        = 7,
    kNumCursorRoles
};

enum CursorSvgVersion
{
    kCursorSvgVersion_Initial = 0,
    kCursorSvgVersion_36016   = 1, // animation delay
};

struct MouseCursor
{
    uint8_t flags = 0;
    CursorRole role = kCursorRole_None;
    int   pic = 0;
    short hotx = 0, hoty = 0;
    short view = -1;
    int   animdelay = 5;

    AGS::Common::String name; // script name

    MouseCursor() = default;

    void ReadFromFile(AGS::Common::Stream *in);
    void WriteToFile(AGS::Common::Stream *out);
    void ReadFromSavegame(AGS::Common::Stream *in, int cmp_ver);
    void WriteToSavegame(AGS::Common::Stream *out) const;
};

#endif // __AC_MOUSECURSOR_H

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
#ifndef __AC_INVENTORYITEMINFO_H
#define __AC_INVENTORYITEMINFO_H

#include "ac/game_version.h"
#include "util/geometry.h"
#include "util/stream.h"
#include "util/string.h"

#define IFLG_STARTWITH 1
#define LEGACY_MAX_INVENTORY_NAME_LENGTH 25

enum InvitemSvgVersion
{
    kInvitemSvgVersion_Initial = 0,
    kInvitemSvgVersion_36304 = 3060304, // hotspot
    kInvitemSvgVersion_36314 = 3060314, // hotspot align
    kInvitemSvgVersion_Current = kInvitemSvgVersion_36314
};

struct InventoryItemInfo
{
    AGS::Common::String name;
    int  pic = 0;
    int  cursorPic = 0, hotx = 0, hoty = 0;
    FrameAlignment hotAlign = kAlignNone;
    uint8_t flags = 0; // IFLG_*

    void ReadFromFile(AGS::Common::Stream *in, GameDataVersion game_ver);
    void WriteToFile(AGS::Common::Stream *out) const;
    void ReadFromSavegame(AGS::Common::Stream *in, InvitemSvgVersion svg_ver);
    void WriteToSavegame(AGS::Common::Stream *out) const;
};

#endif // __AC_INVENTORYITEMINFO_H
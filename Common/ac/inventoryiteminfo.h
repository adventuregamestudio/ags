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
#ifndef __AC_INVENTORYITEMINFO_H
#define __AC_INVENTORYITEMINFO_H

#include "util/stream.h"
#include "util/string.h"

#define IFLG_STARTWITH 1
#define LEGACY_MAX_INVENTORY_NAME_LENGTH 25

struct InventoryItemInfo {
    AGS::Common::String name;
    int  pic;
    int  cursorPic, hotx, hoty;
    int  reserved[5];
    uint8_t flags; // IFLG_STARTWITH

    void ReadFromFile(AGS::Common::Stream *in);
    void WriteToFile(AGS::Common::Stream *out);
    void ReadFromSavegame(AGS::Common::Stream *in);
    void WriteToSavegame(AGS::Common::Stream *out) const;
};

#endif // __AC_INVENTORYITEMINFO_H
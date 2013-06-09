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

#ifndef __AC_INVENTORYITEMINFO_H
#define __AC_INVENTORYITEMINFO_H

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define IFLG_STARTWITH 1
#define INVITEMINFO_LEGACY_NAME_LENGTH   25
#define INVITEMINFO_LEGACY_RESERVED_INTS 5

struct InventoryItemInfo {
    char name[INVITEMINFO_LEGACY_NAME_LENGTH];
    int  pic;
    int  cursorPic, hotx, hoty;
    int  reserved[INVITEMINFO_LEGACY_RESERVED_INTS];
    char flags;

    InventoryItemInfo();
    void ReadFromFile_v321(Common::Stream *in);
    void WriteToFile_v321(Common::Stream *out);
    void ReadFromSavedGame(Common::Stream *in);
    void WriteToSavedGame(Common::Stream *out);
};

#endif // __AC_INVENTORYITEMINFO_H
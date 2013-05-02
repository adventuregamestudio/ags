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

#include <memory.h>
#include "ac/inventoryiteminfo.h"
#include "util/stream.h"

using AGS::Common::Stream;

InventoryItemInfo::InventoryItemInfo()
    : pic(0)
    , cursorPic(0)
    , hotx(0)
    , hoty(0)
    , flags(0)
{
    memset(&name, 0, sizeof(name));
    memset(&reserved, 0, sizeof(reserved));
}

void InventoryItemInfo::ReadFromFile(Stream *in)
{
    in->Read(name, INVITEMINFO_LEGACY_NAME_LENGTH);
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    hotx = in->ReadInt32();
    hoty = in->ReadInt32();
    in->ReadArrayOfInt32(reserved, INVITEMINFO_LEGACY_RESERVED_INTS);
    flags = in->ReadInt8();
}

void InventoryItemInfo::WriteToFile(Stream *out)
{
    out->Write(name, INVITEMINFO_LEGACY_NAME_LENGTH);
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
    out->WriteInt32(hotx);
    out->WriteInt32(hoty);
    out->WriteArrayOfInt32(reserved, INVITEMINFO_LEGACY_RESERVED_INTS);
    out->WriteInt8(flags);
}

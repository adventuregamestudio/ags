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
#include "ac/inventoryiteminfo.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

void InventoryItemInfo::ReadFromFile(Stream *in)
{
    name.ReadCount(in, LEGACY_MAX_INVENTORY_NAME_LENGTH);
    in->Seek(3); // alignment padding to int32
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    hotx = in->ReadInt32();
    hoty = in->ReadInt32();
    in->ReadArrayOfInt32(reserved, 5);
    flags = in->ReadInt8();
    in->Seek(3); // alignment padding to int32
}

void InventoryItemInfo::WriteToFile(Stream *out)
{
    name.WriteCount(out, LEGACY_MAX_INVENTORY_NAME_LENGTH);
    out->WriteByteCount(0, 3); // alignment padding to int32
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
    out->WriteInt32(hotx);
    out->WriteInt32(hoty);
    out->WriteArrayOfInt32(reserved, 5);
    out->WriteInt8(flags);
    out->WriteByteCount(0, 3); // alignment padding to int32
}

void InventoryItemInfo::ReadFromSavegame(Stream *in)
{
    name = StrUtil::ReadString(in);
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
}

void InventoryItemInfo::WriteToSavegame(Stream *out) const
{
    StrUtil::WriteString(name, out);
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
}

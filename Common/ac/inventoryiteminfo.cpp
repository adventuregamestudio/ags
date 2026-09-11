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
#include "ac/inventoryiteminfo.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

void InventoryItemInfo::ReadFromFile(Stream *in, GameDataVersion game_ver)
{
    name.ReadCount(in, LEGACY_MAX_INVENTORY_NAME_LENGTH);
    in->Seek(3); // alignment padding to int32
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    hotx = in->ReadInt32();
    hoty = in->ReadInt32();
    hotAlign = static_cast<FrameAlignment>(in->ReadInt32()); // since kGameVersion_363_14
    in->Seek(sizeof(int32_t) * 4); // reserved
    flags = in->ReadInt8();
    in->Seek(3); // alignment padding to int32

    if (game_ver < kGameVersion_363_10)
    {
        hotAlign = ((hotx == 0) && (hoty == 0)) ? kAlignMiddleCenter : kAlignTopLeft;
    }
}

void InventoryItemInfo::WriteToFile(Stream *out) const
{
    name.WriteCount(out, LEGACY_MAX_INVENTORY_NAME_LENGTH);
    out->WriteByteCount(0, 3); // alignment padding to int32
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
    out->WriteInt32(hotx);
    out->WriteInt32(hoty);
    out->WriteInt32(hotAlign); // since kGameVersion_363_14
    out->WriteByteCount(0, sizeof(int32_t) * 4); // reserved
    out->WriteInt8(flags);
    out->WriteByteCount(0, 3); // alignment padding to int32
}

void InventoryItemInfo::ReadFromSavegame(Stream *in, InvitemSvgVersion svg_ver)
{
    name = StrUtil::ReadString(in);
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    if (svg_ver >= kInvitemSvgVersion_36304)
    {
        hotx = in->ReadInt16();
        hoty = in->ReadInt16();
    }
}

void InventoryItemInfo::WriteToSavegame(Stream *out) const
{
    StrUtil::WriteString(name, out);
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
    // kInvitemSvgVersion_36304
    out->WriteInt16(hotx);
    out->WriteInt16(hoty);
}

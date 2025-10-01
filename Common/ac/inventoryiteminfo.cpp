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
#include "ac/inventoryiteminfo.h"
#include "ac/common_defines.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

void InventoryItemInfo::RemapOldInteractions()
{
    ScriptEventHandlers new_interactions;
    // this is just for safety, it's supposed to be that large
    interactions.Handlers.resize(NUM_STANDARD_VERBS);
    new_interactions.Handlers.resize(NUM_STANDARD_VERBS);
    new_interactions.Handlers[MODE_WALK]    = {};
    new_interactions.Handlers[MODE_LOOK]    = interactions.Handlers[0];
    new_interactions.Handlers[MODE_HAND]    = interactions.Handlers[1];
    new_interactions.Handlers[MODE_TALK]    = interactions.Handlers[2];
    new_interactions.Handlers[MODE_USE]     = interactions.Handlers[3];
    // "Other click" event handler is assigned to any other standard interaction
    new_interactions.Handlers[MODE_PICKUP]  = interactions.Handlers[4];
    new_interactions.Handlers[MODE_CUSTOM1] = interactions.Handlers[4];
    new_interactions.Handlers[MODE_CUSTOM2] = interactions.Handlers[4];

    interactions = std::move(new_interactions);
    interactions.ScriptModule = events.ScriptModule;
}

void InventoryItemInfo::ResolveEventHandlers()
{
    events.CreateIndexedList(std::vector<String>() = {
        "OnAnyClick"
    });
}

void InventoryItemInfo::ReadFromFile(Stream *in)
{
    name.ReadCount(in, LEGACY_MAX_INVENTORY_NAME_LENGTH);
    in->Seek(3); // alignment padding to int32
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    hotx = in->ReadInt32();
    hoty = in->ReadInt32();
    in->Seek(sizeof(int32_t) * 5); // 5 reserved ints
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
    out->WriteByteCount(0, sizeof(int32_t) * 5);
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

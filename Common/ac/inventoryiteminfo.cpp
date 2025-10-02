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

/* static */ ScriptEventSchema InventoryItemInfo::_eventSchema = {{
        { "OnAnyClick", kInventoryEvent_AnyClick }
    }};

void InventoryItemInfo::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = interactions.GetHandlers();
    std::vector<ScriptEventHandler> new_interactions;
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions[MODE_WALK]    = {};
    new_interactions[MODE_LOOK]    = old_interactions[0];
    new_interactions[MODE_HAND]    = old_interactions[1];
    new_interactions[MODE_TALK]    = old_interactions[2];
    new_interactions[MODE_USE]     = old_interactions[3];
    // "Other click" event handler is assigned to any other standard interaction
    new_interactions[MODE_PICKUP]  = old_interactions[4];
    new_interactions[MODE_CUSTOM1] = old_interactions[4];
    new_interactions[MODE_CUSTOM2] = old_interactions[4];

    interactions.SetScriptModule(Events.GetScriptModule());
    interactions.SetHandlers(std::move(new_interactions));
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

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
#include <string.h> // memset
#include <stdlib.h> // free
#include "ac/common.h"
#include "ac/game_version.h"
#include "ac/roomstatus.h"
#include "game/customproperties.h"
#include "game/savegame_components.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Engine;


void HotspotState::ReadFromSavegame(Common::Stream *in, int save_ver)
{
    Enabled = in->ReadInt8() != 0;
    if (save_ver >= kRoomStatSvgVersion_36016)
    {
        Name = StrUtil::ReadString(in);
    }
}

void HotspotState::WriteToSavegame(Common::Stream *out) const
{
    out->WriteInt8(Enabled);
    StrUtil::WriteString(Name, out);
}


RoomStatus::RoomStatus()
{
    contentFormat = kRoomStatSvgVersion_Current; // set current to avoid fixups
    beenhere = 0;
    numobj = 0;
    tsdatasize = 0;

    memset(&region_enabled, 0, sizeof(region_enabled));
    memset(&walkbehind_base, 0, sizeof(walkbehind_base));
    memset(&interactionVariableValues, 0, sizeof(interactionVariableValues));
}

RoomStatus::~RoomStatus()
{
}

void RoomStatus::FreeScriptData()
{
    tsdata.clear();
    tsdatasize = 0;
}

void RoomStatus::FreeProperties()
{
    roomProps.clear();
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hsProps[i].clear();
    }
    objProps.clear();
}

void RoomStatus::ReadFromSavegame(Stream *in, GameDataVersion data_ver, RoomStatSvgVersion save_ver)
{
    FreeScriptData();
    FreeProperties();

    beenhere = in->ReadInt8();
    numobj = static_cast<uint32_t>(in->ReadInt32());
    obj.resize(numobj);
    objProps.resize(numobj);
    intrObject.resize(numobj);
    for (uint32_t i = 0; i < numobj; ++i)
    {
        obj[i].ReadFromSavegame(in, save_ver);
        Properties::ReadValues(objProps[i], in);
        if (data_ver <= kGameVersion_272)
            SavegameComponents::ReadInteraction272(intrObject[i], in);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hotspot[i].ReadFromSavegame(in, save_ver);
        Properties::ReadValues(hsProps[i], in);
        if (data_ver <= kGameVersion_272)
            SavegameComponents::ReadInteraction272(intrHotspot[i], in);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        region_enabled[i] = in->ReadInt8();
        if (data_ver <= kGameVersion_272)
            SavegameComponents::ReadInteraction272(intrRegion[i], in);
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        walkbehind_base[i] = in->ReadInt32();
    }

    Properties::ReadValues(roomProps, in);
    if (data_ver <= kGameVersion_272)
    {
        SavegameComponents::ReadInteraction272(intrRoom, in);
        in->ReadArrayOfInt32(interactionVariableValues, MAX_GLOBAL_VARIABLES);
    }

    tsdatasize = static_cast<uint32_t>(in->ReadInt32());
    if (tsdatasize)
    {
        tsdata.resize(tsdatasize);
        in->Read(tsdata.data(), tsdatasize);
    }

    contentFormat = save_ver;
    if (save_ver >= kRoomStatSvgVersion_36041)
    {
        contentFormat = (RoomStatSvgVersion)in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }
}

void RoomStatus::WriteToSavegame(Stream *out, GameDataVersion data_ver) const
{
    out->WriteInt8(beenhere);
    out->WriteInt32(numobj);
    for (uint32_t i = 0; i < numobj; ++i)
    {
        obj[i].WriteToSavegame(out);
        Properties::WriteValues(objProps[i], out);
        if (data_ver <= kGameVersion_272)
            SavegameComponents::WriteInteraction272(intrObject[i], out);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hotspot[i].WriteToSavegame(out);
        Properties::WriteValues(hsProps[i], out);
        if (data_ver <= kGameVersion_272)
            SavegameComponents::WriteInteraction272(intrHotspot[i], out);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        out->WriteInt8(region_enabled[i]);
        if (data_ver <= kGameVersion_272)
            SavegameComponents::WriteInteraction272(intrRegion[i], out);
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        out->WriteInt32(walkbehind_base[i]);
    }

    Properties::WriteValues(roomProps, out);
    if (data_ver <= kGameVersion_272)
    {
        SavegameComponents::WriteInteraction272(intrRoom, out);
        out->WriteArrayOfInt32(interactionVariableValues, MAX_GLOBAL_VARIABLES);
    }

    out->WriteInt32(static_cast<int32_t>(tsdatasize));
    if (tsdatasize)
        out->Write(tsdata.data(), tsdatasize);

    // kRoomStatSvgVersion_36041
    out->WriteInt32(contentFormat);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
}

std::unique_ptr<RoomStatus> room_statuses[MAX_ROOMS];

// Replaces all accesses to the roomstats array
RoomStatus* getRoomStatus(int room)
{
    if (!room_statuses[room])
    {
        // First access, allocate and initialise the status
        room_statuses[room].reset(new RoomStatus());
    }
    return room_statuses[room].get();
}

// Used in places where it is only important to know whether the player
// had previously entered the room. In this case it is not necessary
// to initialise the status because a player can only have been in
// a room if the status is already initialised.
bool isRoomStatusValid(int room)
{
    return (room_statuses[room] != nullptr);
}

void resetRoomStatuses()
{
    for (int i = 0; i < MAX_ROOMS; i++)
    {
        room_statuses[i].reset();
    }
}

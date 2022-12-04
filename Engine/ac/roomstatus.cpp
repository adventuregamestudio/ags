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
#include <string.h> // memset
#include <stdlib.h> // free
#include "ac/common.h"
#include "ac/game_version.h"
#include "ac/roomstatus.h"
#include "game/customproperties.h"
#include "game/savegame_components.h"
#include "util/alignedstream.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Engine;


void HotspotState::ReadFromSavegame(Common::Stream *in, int save_ver)
{
    Enabled = in->ReadInt8() != 0;
    if (save_ver > 0)
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

void RoomStatus::ReadFromFile_v321(Stream *in)
{
    FreeScriptData();
    FreeProperties();

    beenhere = in->ReadInt32();
    numobj = in->ReadInt32();
    obj.resize(MAX_ROOM_OBJECTS_v300);
    objProps.resize(MAX_ROOM_OBJECTS_v300);
    intrObject.resize(MAX_ROOM_OBJECTS_v300);
    ReadRoomObjects_Aligned(in);

    int16_t dummy[MAX_LEGACY_ROOM_FLAGS]; // cannot seek with AlignedStream
    in->ReadArrayOfInt16(dummy, MAX_LEGACY_ROOM_FLAGS); // flagstates (OBSOLETE)
    tsdatasize = static_cast<uint32_t>(in->ReadInt32());
    in->ReadInt32(); // tsdata
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        intrHotspot[i].ReadFromSavedgame_v321(in);
    }
    for (auto &intr : intrObject)
    {
        intr.ReadFromSavedgame_v321(in);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        intrRegion[i].ReadFromSavedgame_v321(in);
    }
    intrRoom.ReadFromSavedgame_v321(in);
    for (size_t i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
        hotspot[i].Enabled = in->ReadInt8() != 0;
    in->ReadArrayOfInt8((int8_t*)region_enabled, MAX_ROOM_REGIONS);
    in->ReadArrayOfInt16(walkbehind_base, MAX_WALK_BEHINDS);
    in->ReadArrayOfInt32(interactionVariableValues, MAX_GLOBAL_VARIABLES);

    if (loaded_game_file_version >= kGameVersion_340_4)
    {
        Properties::ReadValues(roomProps, in);
        for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
        {
            Properties::ReadValues(hsProps[i], in);
        }
        for (auto &props : objProps)
        {
            Properties::ReadValues(props, in);
        }
    }
}

void RoomStatus::ReadRoomObjects_Aligned(Common::Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (auto &o : obj)
    {
        o.ReadFromSavegame(&align_s, 0);
        align_s.Reset();
    }
}

void RoomStatus::ReadFromSavegame(Stream *in, int save_ver)
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
        if (loaded_game_file_version <= kGameVersion_272)
            SavegameComponents::ReadInteraction272(intrObject[i], in);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hotspot[i].ReadFromSavegame(in, save_ver);
        Properties::ReadValues(hsProps[i], in);
        if (loaded_game_file_version <= kGameVersion_272)
            SavegameComponents::ReadInteraction272(intrHotspot[i], in);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        region_enabled[i] = in->ReadInt8();
        if (loaded_game_file_version <= kGameVersion_272)
            SavegameComponents::ReadInteraction272(intrRegion[i], in);
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        walkbehind_base[i] = in->ReadInt32();
    }

    Properties::ReadValues(roomProps, in);
    if (loaded_game_file_version <= kGameVersion_272)
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
}

void RoomStatus::WriteToSavegame(Stream *out) const
{
    out->WriteInt8(beenhere);
    out->WriteInt32(numobj);
    for (uint32_t i = 0; i < numobj; ++i)
    {
        obj[i].WriteToSavegame(out);
        Properties::WriteValues(objProps[i], out);
        if (loaded_game_file_version <= kGameVersion_272)
            SavegameComponents::WriteInteraction272(intrObject[i], out);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hotspot[i].WriteToSavegame(out);
        Properties::WriteValues(hsProps[i], out);
        if (loaded_game_file_version <= kGameVersion_272)
            SavegameComponents::WriteInteraction272(intrHotspot[i], out);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        out->WriteInt8(region_enabled[i]);
        if (loaded_game_file_version <= kGameVersion_272)
            SavegameComponents::WriteInteraction272(intrRegion[i], out);
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        out->WriteInt32(walkbehind_base[i]);
    }

    Properties::WriteValues(roomProps, out);
    if (loaded_game_file_version <= kGameVersion_272)
    {
        SavegameComponents::WriteInteraction272(intrRoom, out);
        out->WriteArrayOfInt32(interactionVariableValues, MAX_GLOBAL_VARIABLES);
    }

    out->WriteInt32(static_cast<int32_t>(tsdatasize));
    if (tsdatasize)
        out->Write(tsdata.data(), tsdatasize);
}

// JJS: Replacement for the global roomstats array in the original engine.

RoomStatus* room_statuses[MAX_ROOMS];

// Replaces all accesses to the roomstats array
RoomStatus* getRoomStatus(int room)
{
    if (room_statuses[room] == nullptr)
    {
        // First access, allocate and initialise the status
        room_statuses[room] = new RoomStatus();
    }
    return room_statuses[room];
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
        if (room_statuses[i] != nullptr)
        {
            delete room_statuses[i];
            room_statuses[i] = nullptr;
        }
    }
}

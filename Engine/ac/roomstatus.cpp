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
    tsdata = nullptr;
    
    memset(&region_enabled, 0, sizeof(region_enabled));
    memset(&walkbehind_base, 0, sizeof(walkbehind_base));
}

RoomStatus::~RoomStatus()
{
    if (tsdata)
        delete [] tsdata;
}

void RoomStatus::FreeScriptData()
{
    if (tsdata)
        delete [] tsdata;
    tsdata = nullptr;
    tsdatasize = 0;
}

void RoomStatus::FreeProperties()
{
    roomProps.clear();
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hsProps[i].clear();
    }
    for (int i = 0; i < MAX_ROOM_OBJECTS; ++i)
    {
        objProps[i].clear();
    }
}

void RoomStatus::ReadFromSavegame(Stream *in, int32_t cmp_ver)
{
    FreeScriptData();
    FreeProperties();

    beenhere = in->ReadInt8();
    numobj = in->ReadInt32();
    for (int i = 0; i < numobj; ++i)
    {
        obj[i].ReadFromSavegame(in, cmp_ver);
        Properties::ReadValues(objProps[i], in);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hotspot[i].ReadFromSavegame(in, cmp_ver);
        Properties::ReadValues(hsProps[i], in);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        region_enabled[i] = in->ReadInt8();
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        walkbehind_base[i] = in->ReadInt32();
    }

    Properties::ReadValues(roomProps, in);

    tsdatasize = in->ReadInt32();
    if (tsdatasize)
    {
        tsdata = new char[tsdatasize];
        in->Read(tsdata, tsdatasize);
    }
}

void RoomStatus::WriteToSavegame(Stream *out) const
{
    out->WriteInt8(beenhere);
    out->WriteInt32(numobj);
    for (int i = 0; i < numobj; ++i)
    {
        obj[i].WriteToSavegame(out);
        Properties::WriteValues(objProps[i], out);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        hotspot[i].WriteToSavegame(out);
        Properties::WriteValues(hsProps[i], out);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        out->WriteInt8(region_enabled[i]);
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        out->WriteInt32(walkbehind_base[i]);
    }

    Properties::WriteValues(roomProps, out);

    out->WriteInt32(tsdatasize);
    if (tsdatasize)
        out->Write(tsdata, tsdatasize);
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

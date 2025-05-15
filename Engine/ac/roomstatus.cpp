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


void HotspotState::ReadFromSavegame(Common::Stream *in, RoomStatSvgVersion save_ver)
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

void WalkareaState::ReadFromSavegame(Common::Stream *in, RoomStatSvgVersion save_ver)
{
    // reserve 3 int32 for existing older WA properties:
    // enabled (or rather flags?), scaling min & max, anything else?
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();
    FaceDirectionRatio = in->ReadFloat32();
}

void WalkareaState::WriteToSavegame(Common::Stream *out) const
{
    // reserve 3 int32 for existing older WA properties:
    // enabled (or rather flags?), scaling min & max, anything else?
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteFloat32(FaceDirectionRatio);
}


RoomStatus::RoomStatus()
{
    contentFormat = kRoomStatSvgVersion_Current; // set current to avoid fixups
    beenhere = 0;
    numobj = 0;
    tsdatasize = 0;

    memset(&region_enabled, 0, sizeof(region_enabled));
    memset(&walkbehind_base, 0, sizeof(walkbehind_base));
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
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        regProps[i].clear();
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        waProps[i].clear();
    }
}

void RoomStatus::SetBgShader(int shader_id, int shader_handle)
{
    _bgShaderID = shader_id;
    _bgShaderHandle = shader_handle;
}

void RoomStatus::ReadFromSavegame(Stream *in, RoomStatSvgVersion cmp_ver)
{
    FreeScriptData();
    FreeProperties();

    beenhere = in->ReadInt8();
    numobj = static_cast<uint32_t>(in->ReadInt32());

    int num_hotspots = MAX_ROOM_HOTSPOTS;
    int num_regions = MAX_ROOM_REGIONS;
    int num_walkbehinds = MAX_WALK_BEHINDS;
    int num_walkareas = MAX_WALK_AREAS;
    if (cmp_ver >= kRoomStatSvgVersion_40003)
    {
        num_hotspots = in->ReadInt32();
        num_regions = in->ReadInt32();
        num_walkbehinds = in->ReadInt32();
        num_walkareas = in->ReadInt32();
    }

    obj.resize(numobj);
    objProps.resize(numobj);
    for (uint32_t i = 0; i < numobj; ++i)
    {
        obj[i].ReadFromSavegame(in, cmp_ver);
        Properties::ReadValues(objProps[i], in);
    }
    for (int i = 0; i < num_hotspots; ++i)
    {
        hotspot[i].ReadFromSavegame(in, cmp_ver);
        Properties::ReadValues(hsProps[i], in);
    }
    for (int i = 0; i < num_regions; ++i)
    {
        region_enabled[i] = in->ReadInt8();
    }
    for (int i = 0; i < num_walkbehinds; ++i)
    {
        walkbehind_base[i] = in->ReadInt32();
    }
    if (cmp_ver >= kRoomStatSvgVersion_40003)
    {
        for (int i = 0; i < num_walkareas; ++i)
        {
            walkareas[i].ReadFromSavegame(in, cmp_ver);
        }
    }

    Properties::ReadValues(roomProps, in);

    tsdatasize = static_cast<uint32_t>(in->ReadInt32());
    if (tsdatasize)
    {
        tsdata.resize(tsdatasize);
        in->Read(tsdata.data(), tsdatasize);
    }

    contentFormat = cmp_ver;
    if (cmp_ver >= kRoomStatSvgVersion_36041)
    {
        contentFormat = (RoomStatSvgVersion)in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }

    if (cmp_ver >= kRoomStatSvgVersion_40003)
    {
        face_dir_ratio = in->ReadFloat32();
        _bgShaderID = in->ReadInt32(); // used since kRoomStatSvgVersion_40018
        _bgShaderHandle = in->ReadInt32();
        in->ReadInt32(); // reserved
    }
    else
    {
        face_dir_ratio = 1.f;
        _bgShaderID = 0;
        _bgShaderHandle = 0;
    }

    if (cmp_ver >= kRoomStatSvgVersion_40008)
    {
        for (int i = 0; i < num_regions; ++i)
        {
            Properties::ReadValues(regProps[i], in);
        }
        for (int i = 0; i < num_walkareas; ++i)
        {
            Properties::ReadValues(waProps[i], in);
        }
    }
}

void RoomStatus::WriteToSavegame(Stream *out) const
{
    out->WriteInt8(beenhere);
    out->WriteInt32(numobj);
    // -- kRoomStatSvgVersion_40003
    out->WriteInt32(MAX_ROOM_HOTSPOTS);
    out->WriteInt32(MAX_ROOM_REGIONS);
    out->WriteInt32(MAX_WALK_AREAS);
    out->WriteInt32(MAX_WALK_BEHINDS);
    // --
    for (uint32_t i = 0; i < numobj; ++i)
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
    // -- kRoomStatSvgVersion_40003
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        walkareas[i].WriteToSavegame(out);
    }
    // --

    Properties::WriteValues(roomProps, out);

    out->WriteInt32(static_cast<int32_t>(tsdatasize));
    if (tsdatasize)
        out->Write(tsdata.data(), tsdatasize);

    // kRoomStatSvgVersion_36041
    out->WriteInt32(contentFormat);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);

    // -- kRoomStatSvgVersion_40003
    out->WriteFloat32(face_dir_ratio);
    out->WriteInt32(_bgShaderID); // used since kRoomStatSvgVersion_40018
    out->WriteInt32(_bgShaderHandle);
    out->WriteInt32(0); // reserved

    // -- kRoomStatSvgVersion_40008
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        Properties::WriteValues(regProps[i], out);
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        Properties::WriteValues(waProps[i], out);
    }
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

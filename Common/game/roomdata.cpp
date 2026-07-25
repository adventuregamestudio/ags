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
#include "game/roomdata.h"
#include "game/room_file.h"

namespace AGS
{
namespace Common
{

RoomOptions::RoomOptions()
    : StartupMusic(0)
    , SaveLoadDisabled(false)
    , PlayerCharOff(false)
    , PlayerView(0)
    , MusicVolume(kRoomVolumeNormal)
    , Flags(0)
{
}

RoomBgFrame::RoomBgFrame()
    : IsPaletteShared(false)
{
    memset(Palette, 0, sizeof(Palette));
}

RoomEdges::RoomEdges()
    : Left(0)
    , Right(0)
    , Top(0)
    , Bottom(0)
{
}

RoomEdges::RoomEdges(int l, int r, int t, int b)
    : Left(l)
    , Right(r)
    , Top(t)
    , Bottom(b)
{
}

RoomObjectInfo::RoomObjectInfo()
    : Room(-1)
    , X(0)
    , Y(0)
    , Sprite(0)
    , IsOn(false)
    , Baseline(0)
    , Transparency(0)
    , Flags(0)
{
}

RoomRegion::RoomRegion()
    : Light(0)
    , Tint(0)
{
}

WalkArea::WalkArea()
    : CharacterView(0)
    , ScalingFar(0)
    , ScalingNear(NOT_VECTOR_SCALED)
    , PlayerView(0)
    , Top(-1)
    , Bottom(-1)
{
}

WalkBehind::WalkBehind()
    : Baseline(0)
{
}

MessageInfo::MessageInfo()
    : DisplayAs(0)
    , Flags(0)
{
}

RoomData::RoomData()
{
    InitDefaults();
}

RoomData::~RoomData()
{
    Free();
}

void RoomData::Free()
{
    for (size_t i = 0; i < (size_t)MAX_ROOM_BGFRAMES; ++i)
        BgFrames[i].GraphicBuf = {};
    HotspotMaskBuf = {};
    RegionMaskBuf = {};
    WalkAreaMaskBuf = {};
    WalkBehindMaskBuf = {};

    LocalVariables.clear();
    Interaction.reset();
    Properties.clear();
    for (size_t i = 0; i < (size_t)MAX_ROOM_HOTSPOTS; ++i)
    {
        Hotspots[i].Interaction.reset();
        Hotspots[i].Properties.clear();
    }
    Objects.clear();
    for (size_t i = 0; i < (size_t)MAX_ROOM_REGIONS; ++i)
    {
        Regions[i].Interaction.reset();
        Regions[i].Properties.clear();
    }

    FreeMessages();
    FreeScripts();
}

void RoomData::FreeMessages()
{
    for (uint32_t i = 0; i < MessageCount; ++i)
    {
        Messages[i].Free();
        MessageInfos[i] = MessageInfo();
    }
    MessageCount = 0;
}

void RoomData::FreeScripts()
{
    CompiledScript.reset();

    EventHandlers.reset();
    for (uint32_t i = 0; i < HotspotCount; ++i)
        Hotspots[i].EventHandlers.reset();
    for (auto &obj : Objects)
        obj.EventHandlers.reset();
    for (uint32_t i = 0; i < RegionCount; ++i)
        Regions[i].EventHandlers.reset();
}

void RoomData::InitDefaults()
{
    DataVersion     = kRoomVersion_Current;
    GameID          = NO_GAME_ID_IN_ROOM_FILE;

    _legacyResolution = kRoomResolution_Real;
    MaskResolution  = 1;
    Width           = 320;
    Height          = 200;

    Options         = RoomOptions();
    Edges           = RoomEdges(0, 317, 40, 199);

    BgFrameCount    = 1;
    HotspotCount    = 0;
    RegionCount     = 0;
    WalkAreaCount   = 0;
    WalkBehindCount = 0;
    MessageCount    = 0;

    for (size_t i = 0; i < (size_t)MAX_ROOM_HOTSPOTS; ++i)
        Hotspots[i] = RoomHotspot();
    for (size_t i = 0; i < (size_t)MAX_ROOM_REGIONS; ++i)
        Regions[i] = RoomRegion();
    for (size_t i = 0; i < (size_t)MAX_WALK_AREAS; ++i)
        WalkAreas[i] = WalkArea();
    for (size_t i = 0; i < (size_t)MAX_WALK_BEHINDS; ++i)
        WalkBehinds[i] = WalkBehind();
    
    BackgroundBPP = 1;
    BgAnimSpeed = 5;

    memset(Palette, 0, sizeof(Palette));
}

void RoomData::SetLegacyResolution(RoomResolutionType resolution)
{
    _legacyResolution = resolution;
}

float RoomData::GetMaskScale(RoomAreaMask mask) const
{
    switch (mask)
    {
    case kRoomAreaWalkBehind: return 1.f; // walk-behinds always 1:1 with room size
    case kRoomAreaHotspot:
    case kRoomAreaWalkable:
    case kRoomAreaRegion:
        return 1.f / MaskResolution;
    default: return 0.f;
    }
}

bool RoomData::HasRegionLightLevel(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return Regions[id].Tint == 0;
    return false;
}

bool RoomData::HasRegionTint(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return Regions[id].Tint != 0;
    return false;
}

int RoomData::GetRegionLightLevel(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return HasRegionLightLevel(id) ? Regions[id].Light : 0;
    return 0;
}

int RoomData::GetRegionTintLuminance(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return HasRegionTint(id) ? (Regions[id].Light * 10) / 25 : 0;
    return 0;
}

} // namespace Common
} // namespace AGS

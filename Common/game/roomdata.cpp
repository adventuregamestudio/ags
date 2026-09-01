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
#include "ac/common_defines.h"
#include "game/room_version.h"

namespace AGS
{
namespace Common
{

RoomOptions::RoomOptions()
    : PlayerCharOff(false)
    , PlayerView(0)
    , FaceDirectionRatio(0.f)
    , Flags(0)
{
}

RoomBgFrame::RoomBgFrame()
{
    std::memset(Palette, 0, sizeof(Palette));
}

RoomBgFrame::RoomBgFrame(const RoomBgFrame &src)
{
    *this = src;
}

RoomBgFrame &RoomBgFrame::operator =(const RoomBgFrame &src)
{
    std::copy(src.Palette, src.Palette + 256, Palette);
    IsPaletteShared = src.IsPaletteShared;
    GraphicBuf = src.GraphicBuf;
    return *this;
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

/* static */ ScriptEventSchema RoomHotspot::_eventSchema = {{
        { "OnAnyClick", kHotspotEvent_AnyClick },
        { "OnMouseMove", kHotspotEvent_MouseOver },
        { "OnWalkOn", kHotspotEvent_StandOn }
    }};

void RoomHotspot::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    std::vector<ScriptEventHandler> new_interactions;
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions[STD_MODE_WALK] = {};
    new_interactions[STD_MODE_LOOK] = old_interactions[1];
    new_interactions[STD_MODE_HAND] = old_interactions[2];
    new_interactions[STD_MODE_TALK] = old_interactions[4];
    new_interactions[STD_MODE_USE]  = old_interactions[3];
    new_interactions[STD_MODE_PICKUP] = old_interactions[7];
    new_interactions[STD_MODE_CUSTOM1] = old_interactions[8];
    new_interactions[STD_MODE_CUSTOM2] = old_interactions[9];
    Interactions.SetHandlers(new_interactions);
    Interactions.SetScriptModule(_events.GetScriptModule());

    _events.SetHandler(kHotspotEvent_StandOn, old_interactions[0].FunctionName);
    _events.SetHandler(kHotspotEvent_AnyClick, old_interactions[5].FunctionName);
    _events.SetHandler(kHotspotEvent_MouseOver, old_interactions[6].FunctionName);
}

/* static */ ScriptEventSchema RoomObjectInfo::_eventSchema = {{
        { "OnAnyClick", kRoomObjectEvent_AnyClick },
        { "OnFrameEvent", kRoomObjectEvent_OnFrameEvent },
    }};

void RoomObjectInfo::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    std::vector<ScriptEventHandler> new_interactions;
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions[STD_MODE_WALK] = {};
    new_interactions[STD_MODE_LOOK] = old_interactions[0];
    new_interactions[STD_MODE_HAND] = old_interactions[1];
    new_interactions[STD_MODE_TALK] = old_interactions[2];
    new_interactions[STD_MODE_USE] = old_interactions[3];
    new_interactions[STD_MODE_PICKUP] = old_interactions[5];
    new_interactions[STD_MODE_CUSTOM1] = old_interactions[6];
    new_interactions[STD_MODE_CUSTOM2] = old_interactions[7];
    Interactions.SetHandlers(new_interactions);
    Interactions.SetScriptModule(_events.GetScriptModule());

    _events.SetHandler(kRoomObjectEvent_AnyClick, old_interactions[4].FunctionName);
}

/* static */ ScriptEventSchema RoomRegion::_eventSchema = {{
        { "OnStanding", kRegionEvent_Standing },
        { "OnWalksOnto", kRegionEvent_WalkOn },
        { "OnWalksOff", kRegionEvent_WalkOff },
    }};

void RoomRegion::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(3);

    _events.SetHandler(kRegionEvent_Standing, old_interactions[0].FunctionName);
    _events.SetHandler(kRegionEvent_WalkOn, old_interactions[1].FunctionName);
    _events.SetHandler(kRegionEvent_WalkOff, old_interactions[2].FunctionName);
    Interactions = {};
}

RoomData::RoomData()
    : RoomObjectBase(&RoomData::_eventSchema)
{
    InitDefaults();
}

RoomData::RoomData(const RoomData &src)
{
    *this = src;
}

RoomData::RoomData(RoomData &&src)
{
    *this = std::move(src);
}

RoomData::~RoomData()
{
    Free();
}

RoomData &RoomData::operator =(const RoomData &src)
{
    RoomObjectBase::operator=(src);

    GameID = src.GameID;
    Name = src.Name;
    ScriptName = src.ScriptName;
    DataVersion = src.DataVersion;
    MaskResolution = src.MaskResolution;
    Width = src.Width;
    Height = src.Height;
    std::copy(src.Palette, src.Palette + 256, Palette);
    Options = src.Options;

    BackgroundBPP = src.BackgroundBPP;
    BgFrameCount = src.BgFrameCount;
    BgFrames = src.BgFrames;
    BgAnimSpeed = src.BgAnimSpeed;
    Edges = src.Edges;
    HotspotMaskBuf = src.HotspotMaskBuf;
    RegionMaskBuf = src.RegionMaskBuf;
    WalkAreaMaskBuf = src.WalkAreaMaskBuf;
    WalkBehindMaskBuf = src.WalkBehindMaskBuf;
    HotspotCount = src.HotspotCount;
    std::copy(src.Hotspots, src.Hotspots + MAX_ROOM_HOTSPOTS, Hotspots);
    Objects = src.Objects;
    RegionCount = src.RegionCount;
    std::copy(src.Regions, src.Regions + MAX_ROOM_REGIONS, Regions);
    WalkAreaCount = src.WalkAreaCount;
    std::copy(src.WalkAreas, src.WalkAreas + MAX_WALK_AREAS, WalkAreas);
    WalkBehindCount = src.WalkBehindCount;
    std::copy(src.WalkBehinds, src.WalkBehinds + MAX_WALK_BEHINDS, WalkBehinds);

    Properties = src.Properties;
    Interactions = src.Interactions;
    CompiledScript.reset(src.CompiledScript ? new ccScript(*src.CompiledScript) : nullptr);
    StrOptions = src.StrOptions;
    return *this;
}

void RoomData::Free()
{
    BgFrames = {};
    HotspotMaskBuf = {};
    RegionMaskBuf = {};
    WalkAreaMaskBuf = {};
    WalkBehindMaskBuf = {};

    Properties.clear();
    for (size_t i = 0; i < (size_t)MAX_ROOM_HOTSPOTS; ++i)
    {
        Hotspots[i].Properties.clear();
    }
    Objects.clear();
    for (size_t i = 0; i < (size_t)MAX_ROOM_REGIONS; ++i)
    {
        Regions[i].Properties.clear();
    }

    CompiledScript.reset();

    Interactions = {};
    _events.ClearHandlers();
    for (uint32_t i = 0; i < HotspotCount; ++i)
    {
        Hotspots[i].Interactions = {};
        Hotspots[i].ClearEventHandlers();
    }
    for (auto &obj : Objects)
    {
        obj.Interactions = {};
        obj.ClearEventHandlers();
    }
    for (uint32_t i = 0; i < RegionCount; ++i)
    {
        Regions[i].Interactions = {};
        Regions[i].ClearEventHandlers();
    }
}

void RoomData::InitDefaults()
{
    DataVersion     = kRoomVersion_Current;
    GameID          = NO_GAME_ID_IN_ROOM_FILE;

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

    BgFrames.resize(BgFrameCount);

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

/* static */ ScriptEventSchema RoomData::_eventSchema = {{
    { "OnLeaveLeft", kRoomEvent_EdgeLeft },
    { "OnLeaveRight", kRoomEvent_EdgeRight },
    { "OnLeaveBottom", kRoomEvent_EdgeBottom},
    { "OnLeaveTop", kRoomEvent_EdgeTop },
    { "OnFirstTimeEnter", kRoomEvent_FirstEnter },
    { "OnLoad", kRoomEvent_BeforeFadein },
    { "OnRepExec", kRoomEvent_Repexec },
    { "OnAfterFadeIn", kRoomEvent_AfterFadein },
    { "OnLeave", kRoomEvent_BeforeFadeout },
    { "OnUnload", kRoomEvent_AfterFadeout },
    }};

void RoomData::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(10);

    _events.SetHandler(kRoomEvent_EdgeLeft, old_interactions[0].FunctionName);
    _events.SetHandler(kRoomEvent_EdgeRight, old_interactions[1].FunctionName);
    _events.SetHandler(kRoomEvent_EdgeBottom, old_interactions[2].FunctionName);
    _events.SetHandler(kRoomEvent_EdgeTop, old_interactions[3].FunctionName);
    _events.SetHandler(kRoomEvent_FirstEnter, old_interactions[4].FunctionName);
    _events.SetHandler(kRoomEvent_BeforeFadein, old_interactions[5].FunctionName);
    _events.SetHandler(kRoomEvent_Repexec, old_interactions[6].FunctionName);
    _events.SetHandler(kRoomEvent_AfterFadein, old_interactions[7].FunctionName);
    _events.SetHandler(kRoomEvent_BeforeFadeout, old_interactions[8].FunctionName);
    _events.SetHandler(kRoomEvent_AfterFadeout, old_interactions[9].FunctionName);
    Interactions = {};
}

const BitmapData &RoomData::GetMask(RoomAreaMask mask) const
{
    switch (mask)
    {
    case kRoomAreaHotspot: return HotspotMaskBuf;
    case kRoomAreaWalkBehind: return WalkBehindMaskBuf;
    case kRoomAreaWalkable: return WalkAreaMaskBuf;
    case kRoomAreaRegion: return RegionMaskBuf;
    default: assert(false); return WalkAreaMaskBuf; // have to return a reference to something...
    }
}

void RoomData::SetMask(RoomAreaMask mask, PixelBuffer &&pxbuf)
{
    switch (mask)
    {
    case kRoomAreaHotspot: HotspotMaskBuf = std::move(pxbuf); break;
    case kRoomAreaWalkBehind: WalkBehindMaskBuf = std::move(pxbuf); break;
    case kRoomAreaWalkable: WalkAreaMaskBuf = std::move(pxbuf); break;
    case kRoomAreaRegion: RegionMaskBuf = std::move(pxbuf); break;
    default: assert(0); break;
    }
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
        return HasRegionTint(id) ? GfxDef::Value250ToValue100(Regions[id].Light) : 0;
    return 0;
}

} // namespace Common
} // namespace AGS

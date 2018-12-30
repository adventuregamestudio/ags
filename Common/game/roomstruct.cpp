
#include "ac/common.h"
#include "ac/wordsdictionary.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/roomstruct.h"
#include "game/room_file.h"
#include "game/room_version.h"
#include "gfx/bitmap.h"
#include "script/cc_script.h"
#include "util/compress.h"
#include "util/stream.h"
#include "util/string_utils.h"

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
    : Sprite(0)
    , X(0)
    , Y(0)
    , Room(-1)
    , IsOn(false)
    , Baseline(0xFF)
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
    , Light(0)
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

RoomStruct::RoomStruct()
{
    InitDefaults();
}

RoomStruct::~RoomStruct()
{
    Free();
}

void RoomStruct::Free()
{
    for (size_t i = 0; i < MAX_ROOM_BGFRAMES; ++i)
        BgFrames[i].Graphic.reset();
    HotspotMask.reset();
    RegionMask.reset();
    WalkAreaMask.reset();
    WalkBehindMask.reset();

    LocalVariables.clear();
    Interaction.reset();
    Properties.clear();
    for (size_t i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        Hotspots[i].Interaction.reset();
        Hotspots[i].Properties.clear();
    }
    for (size_t i = 0; i < MAX_ROOM_OBJECTS; ++i)
    {
        Objects[i].Interaction.reset();
        Objects[i].Properties.clear();
    }
    for (size_t i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        Regions[i].Interaction.reset();
        Regions[i].Properties.clear();
    }

    FreeMessages();
    FreeScripts();
}

void RoomStruct::FreeMessages()
{
    for (size_t i = 0; i < MessageCount; ++i)
    {
        Messages[i].Free();
        MessageInfos[i] = MessageInfo();
    }
    MessageCount = 0;
}

void RoomStruct::FreeScripts()
{
    CompiledScript.reset();

    EventHandlers.reset();
    for (size_t i = 0; i < HotspotCount; ++i)
        Hotspots[i].EventHandlers.reset();
    for (size_t i = 0; i < ObjectCount; ++i)
        Objects[i].EventHandlers.reset();
    for (size_t i = 0; i < RegionCount; ++i)
        Regions[i].EventHandlers.reset();
}

void RoomStruct::InitDefaults()
{
    DataVersion     = kRoomVersion_Current;
    GameID          = NO_GAME_ID_IN_ROOM_FILE;

    Resolution      = 1;
    Width           = 320;
    Height          = 200;

    Options         = RoomOptions();
    Edges           = RoomEdges(0, 317, 40, 199);

    BgFrameCount    = 1;
    HotspotCount    = 0;
    ObjectCount     = 0;
    RegionCount     = 0;
    WalkAreaCount   = 0;
    WalkBehindCount = 0;
    MessageCount    = 0;

    for (size_t i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        Hotspots[i] = RoomHotspot();
        if (i == 0)
            Hotspots[i].Name = "No hotspot";
        else
            Hotspots[i].Name.Format("Hotspot %u", i);
    }
    for (size_t i = 0; i < MAX_ROOM_OBJECTS; ++i)
        Objects[i] = RoomObjectInfo();
    for (size_t i = 0; i < MAX_ROOM_REGIONS; ++i)
        Regions[i] = RoomRegion();
    for (size_t i = 0; i <= MAX_WALK_AREAS; ++i)
        WalkAreas[i] = WalkArea();
    for (size_t i = 0; i < MAX_WALK_BEHINDS; ++i)
        WalkBehinds[i] = WalkBehind();
    
    BackgroundBPP = 1;
    BgAnimSpeed = 5;

    memset(Palette, 0, sizeof(Palette));
}

bool RoomStruct::HasRegionLightLevel(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return Regions[id].Tint == 0;
    return false;
}

bool RoomStruct::HasRegionTint(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return Regions[id].Tint != 0;
    return false;
}

int RoomStruct::GetRegionLightLevel(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return HasRegionLightLevel(id) ? Regions[id].Light : 0;
    return 0;
}

int RoomStruct::GetRegionTintLuminance(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return HasRegionTint(id) ? (Regions[id].Light * 10) / 25 : 0;
    return 0;
}

void load_room(const char *filename, RoomStruct *room, bool game_is_hires, const std::vector<SpriteInfo> &sprinfos)
{
    room->Free();
    room->InitDefaults();

    update_polled_stuff_if_runtime();

    RoomDataSource src;
    HRoomFileError err = OpenRoomFile(filename, src);
    if (err)
    {
        update_polled_stuff_if_runtime();  // it can take a while to load the file sometimes
        err = ReadRoomData(room, src.InputStream.get(), src.DataVersion);
        if (err)
            err = UpdateRoomData(room, src.DataVersion, game_is_hires, sprinfos);
    }
    if (!err)
        quitprintf("Unable to load the room file '%s'.\n%s.", filename, err->FullMessage().GetCStr());
}

} // namespace Common
} // namespace AGS

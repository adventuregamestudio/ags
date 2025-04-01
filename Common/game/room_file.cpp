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
#include "ac/common.h" // update_polled_stuff
#include "ac/common_defines.h"
#include "ac/gamestructdefines.h"
#include "ac/wordsdictionary.h" // TODO: extract string decryption
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/customproperties.h"
#include "game/room_file.h"
#include "game/roomstruct.h"
#include "gfx/bitmap.h"
#include "script/cc_common.h"
#include "script/cc_script.h"
#include "util/compress.h"
#include "util/data_ext.h"
#include "util/string_utils.h"

// default number of hotspots to read from the room file
#define MIN_ROOM_HOTSPOTS  20
#define LEGACY_ROOM_PASSWORD_LENGTH 11
#define ROOM_MESSAGE_FLAG_DISPLAYNEXT 200
// Reserved room options (each is a byte)
#define ROOM_OPTIONS_RESERVED 4


namespace AGS
{
namespace Common
{

HRoomFileError OpenRoomFileFromAsset(const String &filename, RoomDataSource &src, AssetManager *mgr)
{
    // Cleanup source struct
    src = RoomDataSource();
    // Try to find and open room file
    auto in = mgr->OpenAsset(filename);
    if (in == nullptr)
        return new RoomFileError(kRoomFileErr_FileOpenFailed, String::FromFormat("Filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    src.InputStream = std::move(in);
    return ReadRoomHeader(src);
}

// Read base room object's fields
void ReadRoomObjectBase(RoomObjectInfo &obj, Stream *in)
{
    obj.Sprite = (uint16_t)in->ReadInt16();
    obj.X = in->ReadInt16();
    obj.Y = in->ReadInt16();
    obj.Room = in->ReadInt16();
    uint16_t on = in->ReadInt16();
    // Only treat "on" as a visible flag, otherwise it breaks game logic
    // when reading older rooms (i.e. to make object visible you must turn
    // both enabled and visible properties).
    obj.Flags |= (OBJF_VISIBLE) * on;
}

// Write base room object's fields
void WriteRoomObjectBase(const RoomObjectInfo &obj, Stream *out)
{
    // TODO: expand serialization into 32-bit values at least for the sprite index!!
    out->WriteInt16((uint16_t)obj.Sprite);
    out->WriteInt16((int16_t)obj.X);
    out->WriteInt16((int16_t)obj.Y);
    out->WriteInt16((int16_t)obj.Room);
    out->WriteInt16(0); // [OBSOLETE], old enabled + visible flag
}

// Main room data
HError ReadMainBlock(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    room->BackgroundBPP = in->ReadInt32();
    if (room->BackgroundBPP < 1)
        room->BackgroundBPP = 1;

    room->WalkBehindCount = in->ReadInt16();
    if (room->WalkBehindCount > MAX_WALK_BEHINDS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walk-behinds (in room: %d, max: %d).", room->WalkBehindCount, MAX_WALK_BEHINDS));

    // Walk-behinds baselines
    for (uint32_t i = 0; i < room->WalkBehindCount; ++i)
        room->WalkBehinds[i].Baseline = in->ReadInt16();

    room->HotspotCount = in->ReadInt32();
    if (room->HotspotCount == 0)
        room->HotspotCount = MIN_ROOM_HOTSPOTS;
    if (room->HotspotCount > MAX_ROOM_HOTSPOTS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many hotspots (in room: %d, max: %d).", room->HotspotCount, MAX_ROOM_HOTSPOTS));

    // Hotspots walk-to points
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
    {
        room->Hotspots[i].WalkTo.X = in->ReadInt16();
        room->Hotspots[i].WalkTo.Y = in->ReadInt16();
    }

    // Hotspots names and script names
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
    {
        room->Hotspots[i].Name = StrUtil::ReadString(in);
    }

    for (uint32_t i = 0; i < room->HotspotCount; ++i)
    {
        room->Hotspots[i].ScriptName = StrUtil::ReadString(in);
    }

    // TODO: remove from format later
    uint32_t polypoint_areas = in->ReadInt32();
    if (polypoint_areas > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Legacy poly-point areas are no longer supported.");
    // Room edges
    room->Edges.Top = in->ReadInt16();
    room->Edges.Bottom = in->ReadInt16();
    room->Edges.Left = in->ReadInt16();
    room->Edges.Right = in->ReadInt16();

    // Room objects
    uint16_t obj_count = in->ReadInt16();
    if (obj_count > MAX_ROOM_OBJECTS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine,
            String::FromFormat("Too many objects (in room: %d, max: %d).", obj_count, MAX_ROOM_OBJECTS));

    room->Objects.resize(obj_count);
    for (auto &obj : room->Objects)
        ReadRoomObjectBase(obj, in);

    // Legacy interaction variables (were cut out)
    size_t localvar_count = in->ReadInt32();
    if (localvar_count > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Interaction variables are no longer supported.");

    room->RegionCount = in->ReadInt32();
    if (room->RegionCount > MAX_ROOM_REGIONS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many regions (in room: %d, max: %d).", room->RegionCount, MAX_ROOM_REGIONS));

    // Interaction script links
    // NOTE: we keep pre-3.6.2 interaction format for now, room interactions don't need module selection
    room->EventHandlers = InteractionEvents::CreateFromStream_v361(in);

    for (uint32_t i = 0; i < room->HotspotCount; ++i)
    {
        room->Hotspots[i].EventHandlers = InteractionEvents::CreateFromStream_v361(in);
    }
    for (auto &obj : room->Objects)
    {
        obj.EventHandlers = InteractionEvents::CreateFromStream_v361(in);
    }
    for (uint32_t i = 0; i < room->RegionCount; ++i)
    {
        room->Regions[i].EventHandlers = InteractionEvents::CreateFromStream_v361(in);
    }

    // Room object baselines
    for (auto &obj : room->Objects)
        obj.Baseline = in->ReadInt32();

    room->Width = in->ReadInt16();
    room->Height = in->ReadInt16();

    // NOTE: we add the read flags with the value set by ReadRoomObjectBase
    for (auto &obj : room->Objects)
        obj.Flags |= in->ReadInt16();

    // NOTE: we read MaskResolution here, but in old room formats this field
    // corresponds to "legacy resolution". Today we re-apply this value in
    // UpdateRoomData() under certain conditions.
    room->MaskResolution = in->ReadInt16();
    room->WalkAreaCount = in->ReadInt32();
    if (room->WalkAreaCount > MAX_WALK_AREAS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walkable areas (in room: %d, max: %d).", room->WalkAreaCount, MAX_WALK_AREAS));

    for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
        room->WalkAreas[i].ScalingFar = in->ReadInt16();
    for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
        room->WalkAreas[i].PlayerView = in->ReadInt16();
    for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
        room->WalkAreas[i].ScalingNear = in->ReadInt16();
    for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
        room->WalkAreas[i].Top = in->ReadInt16();
    for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
        room->WalkAreas[i].Bottom = in->ReadInt16();

    in->Seek(LEGACY_ROOM_PASSWORD_LENGTH); // skip password
    in->ReadInt8();// [DEPRECATED]
    in->ReadInt8();// [DEPRECATED]
    room->Options.PlayerCharOff = in->ReadInt8() != 0;
    room->Options.PlayerView = in->ReadInt8();
    in->ReadInt8();// [DEPRECATED]
    room->Options.Flags = in->ReadInt8();
    in->Seek(ROOM_OPTIONS_RESERVED);
    
    // 2.* format legacy room messages
    uint16_t legacy_msg_count = in->ReadInt16();
    if (legacy_msg_count > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Legacy room messages are no longer supported.");

    room->GameID = in->ReadInt32();

    // Very old format legacy room animations (FullAnimation)
    size_t fullanim_count = in->ReadInt16();
    if (fullanim_count > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Room animations are no longer supported.");

    // NOTE: this WA value was written for the second time here, for some weird reason
    for (uint32_t i = 0; i < (uint32_t)MAX_WALK_AREAS; ++i)
        room->WalkAreas[i].PlayerView = in->ReadInt16();
    for (uint32_t i = 0; i < room->RegionCount; ++i)
        room->Regions[i].Light = in->ReadInt16();
    for (uint32_t i = 0; i < room->RegionCount; ++i)
        room->Regions[i].Tint = in->ReadInt32();

    // Primary background
    room->BgFrames[0].Graphic = load_lzw(in, room->BackgroundBPP, &room->Palette);
    // Area masks
    room->RegionMask = load_rle_bitmap8(in);
    room->WalkAreaMask = load_rle_bitmap8(in);
    room->WalkBehindMask = load_rle_bitmap8(in);
    room->HotspotMask = load_rle_bitmap8(in);
    return HError::None();
}

// Room script sources (original text)
HError ReadScriptBlock(char *&buf, Stream *in, RoomFileVersion /*data_ver*/)
{
    size_t len = in->ReadInt32();
    buf = new char[len + 1];
    in->Read(buf, len);
    buf[len] = 0;

    for (size_t i = 0; i < len; ++i)
        buf[i] += passwencstring[i % 11];
    return HError::None();
}

// Compiled room script
HError ReadCompSc3Block(RoomStruct *room, Stream *in, RoomFileVersion /*data_ver*/)
{
    room->CompiledScript.reset(ccScript::CreateFromStream(in));
    if (room->CompiledScript == nullptr)
        return new RoomFileError(kRoomFileErr_ScriptLoadFailed, cc_get_error().ErrorString);
    return HError::None();
}

// Room object names
HError ReadObjNamesBlock(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    size_t name_count = static_cast<uint8_t>(in->ReadInt8());
    if (name_count != room->Objects.size())
        return new RoomFileError(kRoomFileErr_InconsistentData,
            String::FromFormat("In the object names block, expected name count: %zu, got %zu", room->Objects.size(), name_count));

    for (auto &obj : room->Objects)
    {
        obj.Name = StrUtil::ReadString(in);
    }
    return HError::None();
}

// Room object script names
HError ReadObjScNamesBlock(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    size_t name_count = static_cast<uint8_t>(in->ReadInt8());
    if (name_count != room->Objects.size())
        return new RoomFileError(kRoomFileErr_InconsistentData,
            String::FromFormat("In the object script names block, expected name count: %zu, got %zu", room->Objects.size(), name_count));

    for (auto &obj : room->Objects)
    {
        obj.ScriptName = StrUtil::ReadString(in);
    }
    return HError::None();
}

// Secondary backgrounds
HError ReadAnimBgBlock(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    room->BgFrameCount = in->ReadInt8();
    if (room->BgFrameCount > MAX_ROOM_BGFRAMES)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many room backgrounds (in room: %d, max: %d).", room->BgFrameCount, MAX_ROOM_BGFRAMES));

    room->BgAnimSpeed = in->ReadInt8();
    for (size_t i = 0; i < room->BgFrameCount; ++i)
        room->BgFrames[i].IsPaletteShared = in->ReadInt8() != 0;

    for (size_t i = 1; i < room->BgFrameCount; ++i)
    {
        room->BgFrames[i].Graphic =
            load_lzw(in, room->BackgroundBPP, &room->BgFrames[i].Palette);
    }
    return HError::None();
}

// Read custom properties
HError ReadPropertiesBlock(RoomStruct *room, Stream *in, RoomFileVersion /*data_ver*/)
{
    int prop_ver = in->ReadInt32();
    if (prop_ver != 1)
        return new RoomFileError(kRoomFileErr_PropertiesBlockFormat, String::FromFormat("Expected version %d, got %d", 1, prop_ver));

    int errors = 0;
    errors += Properties::ReadValues(room->Properties, in);
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
        errors += Properties::ReadValues(room->Hotspots[i].Properties, in);
    for (auto &obj : room->Objects)
        errors += Properties::ReadValues(obj.Properties, in);

    if (errors > 0)
        return new RoomFileError(kRoomFileErr_InvalidPropertyValues);
    return HError::None();
}

// Early development version of "ags4"
HError ReadExt399(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    // New room object properties
    for (auto &obj : room->Objects)
    {
        obj.BlendMode = (BlendMode)in->ReadInt32();
        // Reserved for colour options
        in->Seek(sizeof(int32_t) * 4); // flags, transparency, tint rbgs, light level
        // Reserved for transform options (see list in savegame format)
        in->Seek(sizeof(int32_t) * 11);
    }
    return HError::None();
}

// Extended walkable areas and related room properties
HError ReadExt_400_WalkOpts(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    // New room properties
    room->Options.FaceDirectionRatio = in->ReadFloat32();
    // reserve few more 32-bit values (for a total of 4)
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();

    size_t wa_count = in->ReadInt32();
    if (wa_count != room->WalkAreaCount)
        return new Error(String::FromFormat("Mismatching number of walkable areas: read %zu expected %zu", wa_count, room->WalkAreaCount));
    for (size_t i = 0; i < wa_count; ++i)
    {
        auto &wa = room->WalkAreas[i];
        wa.FaceDirectionRatio = in->ReadFloat32();
        // reserve few more 32-bit values (for a total of 4)
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
    }
    return HError::None();
}

HError ReadExt_400_CustomProps(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    size_t region_count = in->ReadInt32();
    if (region_count != room->RegionCount)
        return new Error(String::FromFormat("Mismatching number of regions: read %zu expected %zu", region_count, room->RegionCount));
    int errors = 0;
    for (size_t i = 0; i < room->RegionCount; ++i)
    {
        errors += Properties::ReadValues(room->Regions[i].Properties, in);
    }
    if (errors > 0)
        return new RoomFileError(kRoomFileErr_InvalidPropertyValues);

    size_t wa_count = in->ReadInt32();
    if (wa_count != room->WalkAreaCount)
        return new Error(String::FromFormat("Mismatching number of walkable areas: read %zu expected %zu", wa_count, room->WalkAreaCount));
    for (size_t i = 0; i < room->WalkAreaCount; ++i)
    {
        Properties::ReadValues(room->WalkAreas[i].Properties, in);
    }
    if (errors > 0)
        return new RoomFileError(kRoomFileErr_InvalidPropertyValues);
    return HError::None();
}

HError ReadExt_400_RoomNames(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    room->ScriptName = StrUtil::ReadString(in);
    room->Name = StrUtil::ReadString(in);
    return HError::None();
}

HError ReadRoomBlock(RoomStruct *room, Stream *in, RoomFileBlock block, const String &ext_id,
    soff_t block_len, RoomFileVersion data_ver)
{
    //
    // First check classic block types, identified with a numeric id
    //
    switch (block)
    {
    case kRoomFblk_Main:
        return ReadMainBlock(room, in, data_ver);
    case kRoomFblk_Script:
        in->Seek(block_len); // no longer read source script text into RoomStruct
        return HError::None();
    case kRoomFblk_CompScript3:
        return ReadCompSc3Block(room, in, data_ver);
    case kRoomFblk_ObjectNames:
        return ReadObjNamesBlock(room, in, data_ver);
    case kRoomFblk_ObjectScNames:
        return ReadObjScNamesBlock(room, in, data_ver);
    case kRoomFblk_AnimBg:
        return ReadAnimBgBlock(room, in, data_ver);
    case kRoomFblk_Properties:
        return ReadPropertiesBlock(room, in, data_ver);
    case kRoomFblk_CompScript:
    case kRoomFblk_CompScript2:
        return new RoomFileError(kRoomFileErr_OldBlockNotSupported,
            String::FromFormat("Type: %d.", block));
    case kRoomFblk_None:
        break; // continue to string ids
    default:
        return new RoomFileError(kRoomFileErr_UnknownBlockType,
            String::FromFormat("Type: %d, known range: %d - %d.", block, kRoomFblk_Main, kRoomFblk_ObjectScNames));
    }

    // Add extensions here checking ext_id, which is an up to 16-chars name
    if (ext_id.CompareNoCase("ext_sopts") == 0)
    {
        StrUtil::ReadStringMap(room->StrOptions, in);
        return HError::None();
    }
    // Early development version of "ags4"
    else if (ext_id.CompareNoCase("ext_ags399") == 0)
    {
        return ReadExt399(room, in, data_ver);
    }
    else if (ext_id.CompareNoCase("v400_walkopts") == 0)
    {
        return ReadExt_400_WalkOpts(room, in, data_ver);
    }
    else if (ext_id.CompareNoCase("v400_customprops") == 0)
    {
        return ReadExt_400_CustomProps(room, in, data_ver);
    }
    else if (ext_id.CompareNoCase("v400_roomnames") == 0)
    {
        return ReadExt_400_RoomNames(room, in, data_ver);
    }

    return new RoomFileError(kRoomFileErr_UnknownBlockType,
        String::FromFormat("Type: %s", ext_id.GetCStr()));
}


// RoomBlockReader reads whole room data, block by block
class RoomBlockReader : public DataExtReader
{
public:
    RoomBlockReader(RoomStruct *room, RoomFileVersion data_ver, std::unique_ptr<Stream> &&in)
        : DataExtReader(std::move(in), kDataExt_NumID8 | kDataExt_File64)
        , _room(room)
        , _dataVer(data_ver)
    {}

    // Helper function that extracts legacy room script
    HError ReadRoomScript(String &script)
    {
        HError err = FindOne(kRoomFblk_Script);
        if (!err)
            return err;
        char *buf = nullptr;
        err = ReadScriptBlock(buf, _in.get(), _dataVer);
        script = buf;
        delete buf;
        return err;
    }

private:
    String GetOldBlockName(int block_id) const override
    { return GetRoomBlockName((RoomFileBlock)block_id); }
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override
    {
        read_next = true;
        return ReadRoomBlock(_room, in, (RoomFileBlock)block_id, ext_id, block_len, _dataVer);
    }

    RoomStruct *_room {};
    RoomFileVersion _dataVer {};
};


HRoomFileError ReadRoomData(RoomStruct *room, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver)
{
    room->DataVersion = data_ver;
    RoomBlockReader reader(room, data_ver, std::move(in));
    HError err = reader.Read();
    return err ? HRoomFileError::None() : new RoomFileError(kRoomFileErr_BlockListFailed, err);
}

HRoomFileError UpdateRoomData(RoomStruct *room, RoomFileVersion data_ver, const std::vector<SpriteInfo> &sprinfos)
{
    // For objects loaded from an older room, mark everything as
    // Enabled, because there's no good way to distinct if it was
    // disabled or only made invisible by setting "on" property.
    if (data_ver < kRoomVersion_400)
    {
        for (auto &obj : room->Objects)
        {
            obj.Flags |= OBJF_ENABLED;
        }
    }

    // if they set a continiously scaled area where the top
    // and bottom zoom levels are identical, set it as a normal
    // scaled area
    for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
    {
        if (room->WalkAreas[i].ScalingFar == room->WalkAreas[i].ScalingNear)
            room->WalkAreas[i].ScalingNear = NOT_VECTOR_SCALED;
    }

    // sync bpalettes[0] with room.pal
    memcpy(room->BgFrames[0].Palette, room->Palette, sizeof(RGB) * 256);
    return HRoomFileError::None();
}

HError LoadRoom(const String &filename, RoomStruct *room, AssetManager *mgr,
    const std::vector<SpriteInfo> &sprinfos)
{
    room->Free();
    room->InitDefaults();

    RoomDataSource src;
    HRoomFileError err = OpenRoomFileFromAsset(filename, src, mgr);
    if (err)
    {
        err = ReadRoomData(room, std::move(src.InputStream), src.DataVersion);
        if (err)
            err = UpdateRoomData(room, src.DataVersion, sprinfos);
    }
    if (!err)
        return new Error(String::FromFormat("Failed loading a room from file '%s'.", filename.GetCStr()), err);
    return HError::None();
}

HRoomFileError ExtractScriptText(String &script, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver)
{
    RoomBlockReader reader(nullptr, data_ver, std::move(in));
    HError err = reader.ReadRoomScript(script);
    if (!err)
        new RoomFileError(kRoomFileErr_BlockListFailed, err);
    return HRoomFileError::None();
}

void WriteMainBlock(const RoomStruct *room, Stream *out)
{
    out->WriteInt32(room->BackgroundBPP);
    out->WriteInt16((uint16_t)room->WalkBehindCount);
    // Walk-behinds baselines
    for (uint32_t i = 0; i < room->WalkBehindCount; ++i)
        out->WriteInt16(room->WalkBehinds[i].Baseline);

    out->WriteInt32(room->HotspotCount);
    // Hotspots walk-to points
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
    {
        out->WriteInt16(room->Hotspots[i].WalkTo.X);
        out->WriteInt16(room->Hotspots[i].WalkTo.Y);
    }
    // Hotspots names and script names
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
        Common::StrUtil::WriteString(room->Hotspots[i].Name, out);
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
        Common::StrUtil::WriteString(room->Hotspots[i].ScriptName, out);

    out->WriteInt32(0); // legacy poly-point areas

    // Room edges
    out->WriteInt16(room->Edges.Top);
    out->WriteInt16(room->Edges.Bottom);
    out->WriteInt16(room->Edges.Left);
    out->WriteInt16(room->Edges.Right);

    // Room objects
    out->WriteInt16((uint16_t)room->Objects.size());
    for (const auto &obj : room->Objects)
    {
        WriteRoomObjectBase(obj, out);
    }

    out->WriteInt32(0); // legacy interaction vars
    out->WriteInt32(MAX_ROOM_REGIONS);

    // NOTE: we keep pre-3.6.2 interaction format for now, room interactions don't need module selection
    room->EventHandlers->Write_v361(out);
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
        room->Hotspots[i].EventHandlers->Write_v361(out);
    for (const auto &obj : room->Objects)
        obj.EventHandlers->Write_v361(out);
    for (uint32_t i = 0; i < room->RegionCount; ++i)
        room->Regions[i].EventHandlers->Write_v361(out);

    // Room object baselines
    for (const auto &obj : room->Objects)
        out->WriteInt32(obj.Baseline);

    out->WriteInt16(room->Width);
    out->WriteInt16(room->Height);

    for (const auto &obj : room->Objects)
        out->WriteInt16(obj.Flags);
    out->WriteInt16(room->MaskResolution);

    // write the zoom and light levels
    out->WriteInt32(MAX_WALK_AREAS);
    for (uint32_t i = 0; i < (uint32_t)MAX_WALK_AREAS; ++i)
        out->WriteInt16(room->WalkAreas[i].ScalingFar);
    for (uint32_t i = 0; i < (uint32_t)MAX_WALK_AREAS; ++i)
        out->WriteInt16(room->WalkAreas[i].PlayerView);
    for (uint32_t i = 0; i < (uint32_t)MAX_WALK_AREAS; ++i)
        out->WriteInt16(room->WalkAreas[i].ScalingNear);
    for (uint32_t i = 0; i < (uint32_t)MAX_WALK_AREAS; ++i)
        out->WriteInt16(room->WalkAreas[i].Top);
    for (uint32_t i = 0; i < (uint32_t)MAX_WALK_AREAS; ++i)
        out->WriteInt16(room->WalkAreas[i].Bottom);

    out->WriteByteCount(0, LEGACY_ROOM_PASSWORD_LENGTH);
    out->WriteInt8(0);// [DEPRECATED]
    out->WriteInt8(0);// [DEPRECATED]
    out->WriteInt8(room->Options.PlayerCharOff ? 1 : 0);
    out->WriteInt8(room->Options.PlayerView);
    out->WriteInt8(0);// [DEPRECATED]
    out->WriteInt8(room->Options.Flags);
    out->WriteByteCount(0, ROOM_OPTIONS_RESERVED);
    out->WriteInt16(0);// [DEPRECATED]
    out->WriteInt32(room->GameID);

    out->WriteInt16(0); // legacy room animations

    // NOTE: this WA value was written for the second time here, for some weird reason
    for (uint32_t i = 0; i < (uint32_t)MAX_WALK_AREAS; ++i)
        out->WriteInt16(room->WalkAreas[i].PlayerView);
    for (uint32_t i = 0; i < (uint32_t)MAX_ROOM_REGIONS; ++i)
        out->WriteInt16(room->Regions[i].Light);
    for (uint32_t i = 0; i < (uint32_t)MAX_ROOM_REGIONS; ++i)
        out->WriteInt32(room->Regions[i].Tint);

    save_lzw(out, room->BgFrames[0].Graphic.get(), &room->Palette);
    save_rle_bitmap8(out, room->RegionMask.get());
    save_rle_bitmap8(out, room->WalkAreaMask.get());
    save_rle_bitmap8(out, room->WalkBehindMask.get());
    save_rle_bitmap8(out, room->HotspotMask.get());
}

void WriteCompSc3Block(const RoomStruct *room, Stream *out)
{
    room->CompiledScript->Write(out);
}

void WriteObjNamesBlock(const RoomStruct *room, Stream *out)
{
    out->WriteByte((uint8_t)room->Objects.size());
    for (const auto &obj : room->Objects)
        Common::StrUtil::WriteString(obj.Name, out);
}

void WriteObjScNamesBlock(const RoomStruct *room, Stream *out)
{
    out->WriteByte((uint8_t)room->Objects.size());
    for (const auto &obj : room->Objects)
        Common::StrUtil::WriteString(obj.ScriptName, out);
}

void WriteAnimBgBlock(const RoomStruct *room, Stream *out)
{
    out->WriteByte((int8_t)room->BgFrameCount);
    out->WriteByte(room->BgAnimSpeed);

    for (size_t i = 0; i < room->BgFrameCount; ++i)
        out->WriteInt8(room->BgFrames[i].IsPaletteShared ? 1 : 0);
    for (size_t i = 1; i < room->BgFrameCount; ++i)
        save_lzw(out, room->BgFrames[i].Graphic.get(), &room->BgFrames[i].Palette);
}

void WritePropertiesBlock(const RoomStruct *room, Stream *out)
{
    out->WriteInt32(1);  // Version 1 of properties block
    Properties::WriteValues(room->Properties, out);
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
        Properties::WriteValues(room->Hotspots[i].Properties, out);
    for (const auto &obj : room->Objects)
        Properties::WriteValues(obj.Properties, out);
}

void WriteStrOptions(const RoomStruct *room, Stream *out)
{
    StrUtil::WriteStringMap(room->StrOptions, out);
}

void WriteExt399(const RoomStruct *room, Stream *out)
{
    // New object properties
    for (const auto &obj : room->Objects)
    {
        out->WriteInt32(obj.BlendMode);
        // Reserved for colour options
        out->WriteByteCount(0, sizeof(int32_t) * 4); // flags, transparency, tint rgbs, light level
        // Reserved for transform options (see list in savegame format)
        out->WriteByteCount(0, sizeof(int32_t) * 11);
    }
}

void WriteExt_400_WalkareaOpts(const RoomStruct *room, Stream *out)
{
    // New room properties
    out->WriteFloat32(room->Options.FaceDirectionRatio);
    // reserve few more 32-bit values (for a total of 4)
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);

    out->WriteInt32(room->WalkAreaCount);
    for (size_t i = 0; i < room->WalkAreaCount; ++i)
    {
        const auto &wa = room->WalkAreas[i];
        out->WriteFloat32(wa.FaceDirectionRatio);
        // reserve few more 32-bit values (for a total of 4)
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
}

void WriteExt_400_CustomProps(const RoomStruct *room, Stream *out)
{
    out->WriteInt32(room->RegionCount);
    for (size_t i = 0; i < room->RegionCount; ++i)
    {
        Properties::WriteValues(room->Regions[i].Properties, out);
    }

    out->WriteInt32(room->WalkAreaCount);
    for (size_t i = 0; i < room->WalkAreaCount; ++i)
    {
        Properties::WriteValues(room->WalkAreas[i].Properties, out);
    }
}

void WriteExt_400_RoomNames(const RoomStruct *room, Stream *out)
{
    StrUtil::WriteString(room->ScriptName, out);
    StrUtil::WriteString(room->Name, out);
}

HRoomFileError WriteRoomData(const RoomStruct *room, Stream *out, RoomFileVersion data_ver)
{
    if (data_ver < kRoomVersion_Current)
        return new RoomFileError(kRoomFileErr_FormatNotSupported, "We no longer support saving room in the older format.");

    // Header
    out->WriteInt16(data_ver);
    // Main data
    WriteRoomBlock(room, kRoomFblk_Main, WriteMainBlock, out);
    // Compiled script
    if (room->CompiledScript)
        WriteRoomBlock(room, kRoomFblk_CompScript3, WriteCompSc3Block, out);
    // Object names
    if (room->Objects.size() > 0)
    {
        WriteRoomBlock(room, kRoomFblk_ObjectNames, WriteObjNamesBlock, out);
        WriteRoomBlock(room, kRoomFblk_ObjectScNames, WriteObjScNamesBlock, out);
    }
    // Secondary background frames
    if (room->BgFrameCount > 1)
        WriteRoomBlock(room, kRoomFblk_AnimBg, WriteAnimBgBlock, out);
    // Custom properties
    WriteRoomBlock(room, kRoomFblk_Properties, WritePropertiesBlock, out);

    // String options
    WriteRoomBlock(room, "ext_sopts", WriteStrOptions, out);

    // Early development version of "ags4"
    WriteRoomBlock(room, "ext_ags399", WriteExt399, out);
    WriteRoomBlock(room, "v400_walkopts", WriteExt_400_WalkareaOpts, out);
    WriteRoomBlock(room, "v400_customprops", WriteExt_400_CustomProps, out);
    WriteRoomBlock(room, "v400_roomnames", WriteExt_400_RoomNames, out);

    // Write end of room file
    out->WriteByte(kRoomFile_EOF);
    return HRoomFileError::None();
}

} // namespace Common
} // namespace AGS

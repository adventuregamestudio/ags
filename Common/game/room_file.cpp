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
#define LEGACY_HOTSPOT_NAME_LEN 30
#define LEGACY_ROOM_PASSWORD_LENGTH 11
#define LEGACY_ROOM_PASSWORD_SALT 60
#define ROOM_MESSAGE_FLAG_DISPLAYNEXT 200
// Reserved room options (each is a byte)
#define ROOM_OPTIONS_RESERVED 4
#define LEGACY_TINT_IS_ENABLED 0x80000000
// The sizes of the ancient FullAnimation struct,
// used for skipping data chunk, because we currently don't know how
// to handle these in game. See room_file_deprecated.cpp.
#define LEGACY_ROOM_ANIMATION_SIZE (5 * sizeof(int) + 4 * sizeof(char)) /* 2 bytes of padding */
#define LEGACY_ROOM_FULLANIMATION_SIZE (LEGACY_ROOM_ANIMATION_SIZE * 10 + sizeof(int))


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

void ReadRoomObject(RoomObjectInfo &obj, Stream *in)
{
    obj.Sprite = (uint16_t)in->ReadInt16();
    obj.X = in->ReadInt16();
    obj.Y = in->ReadInt16();
    obj.Room = in->ReadInt16();
    obj.IsOn = in->ReadInt16() != 0;
}

void WriteRoomObject(const RoomObjectInfo &obj, Stream *out)
{
    // TODO: expand serialization into 32-bit values at least for the sprite index!!
    out->WriteInt16((uint16_t)obj.Sprite);
    out->WriteInt16((int16_t)obj.X);
    out->WriteInt16((int16_t)obj.Y);
    out->WriteInt16((int16_t)obj.Room);
    out->WriteInt16(obj.IsOn ? 1 : 0);
}


// Main room data
HError ReadMainBlock(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    int bpp;
    if (data_ver >= kRoomVersion_208)
        bpp = in->ReadInt32();
    else
        bpp = 1;

    if (bpp < 1)
        bpp = 1;

    room->BackgroundBPP = bpp;
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
        if (data_ver >= kRoomVersion_3415)
            room->Hotspots[i].Name = StrUtil::ReadString(in);
        else if (data_ver >= kRoomVersion_303a)
            room->Hotspots[i].Name = String::FromStream(in);
        else
            room->Hotspots[i].Name = String::FromStreamCount(in, LEGACY_HOTSPOT_NAME_LEN);
    }

    if (data_ver >= kRoomVersion_270)
    {
        for (uint32_t i = 0; i < room->HotspotCount; ++i)
        {
            if (data_ver >= kRoomVersion_3415)
                room->Hotspots[i].ScriptName = StrUtil::ReadString(in);
            else
                room->Hotspots[i].ScriptName = String::FromStreamCount(in, LEGACY_MAX_SCRIPT_NAME_LEN);
        }
    }

    // TODO: remove from format later
    uint32_t polypoint_areas = in->ReadInt32();
    if (polypoint_areas > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Legacy poly-point areas are no longer supported.");
    /* NOTE: implementation hidden in room_file_deprecated.cpp
        for (uint32_t i = 0; i < polypoint_areas; ++i)
            wallpoints[i].Read(in);
    */

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
        ReadRoomObject(obj, in);

    // Legacy interactions
    if (data_ver >= kRoomVersion_253)
    {
        size_t localvar_count = in->ReadInt32();
        if (localvar_count > 0)
        {
            room->LocalVariables.resize(localvar_count);
            for (size_t i = 0; i < localvar_count; ++i)
                room->LocalVariables[i].Read(in);
        }
    }

    if (data_ver >= kRoomVersion_241 && data_ver < kRoomVersion_300a)
    {
        for (uint32_t i = 0; i < room->HotspotCount; ++i)
            room->Hotspots[i].Interaction = Interaction::CreateFromStream(in);
        for (auto &obj : room->Objects)
            obj.Interaction = Interaction::CreateFromStream(in);
        room->Interaction = Interaction::CreateFromStream(in);
    }

    if (data_ver >= kRoomVersion_255b)
    {
        room->RegionCount = in->ReadInt32();
        if (room->RegionCount > MAX_ROOM_REGIONS)
            return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many regions (in room: %d, max: %d).", room->RegionCount, MAX_ROOM_REGIONS));

        if (data_ver < kRoomVersion_300a)
        {
            for (uint32_t i = 0; i < room->RegionCount; ++i)
                room->Regions[i].Interaction = Interaction::CreateFromStream(in);
        }
    }

    // Event script links
    // NOTE: we keep pre-3.6.2 interaction format for now, room interactions don't need module selection
    if (data_ver >= kRoomVersion_300a)
    {
        room->EventHandlers = InteractionEvents::CreateFromStream_v361(in);
        for (uint32_t i = 0; i < room->HotspotCount; ++i)
            room->Hotspots[i].EventHandlers = InteractionEvents::CreateFromStream_v361(in);
        for (auto &obj : room->Objects)
            obj.EventHandlers = InteractionEvents::CreateFromStream_v361(in);
        for (uint32_t i = 0; i < room->RegionCount; ++i)
            room->Regions[i].EventHandlers = InteractionEvents::CreateFromStream_v361(in);
    }

    if (data_ver >= kRoomVersion_200_alpha)
    {
        for (auto &obj : room->Objects)
            obj.Baseline = in->ReadInt32();
        room->Width = in->ReadInt16();
        room->Height = in->ReadInt16();
    }

    if (data_ver >= kRoomVersion_262)
        for (auto &obj : room->Objects)
            obj.Flags = in->ReadInt16();

    // NOTE: we read MaskResolution here, but in old room formats this field
    // corresponds to "legacy resolution". Today we re-apply this value in
    // UpdateRoomData() under certain conditions.
    if (data_ver >= kRoomVersion_200_final)
        room->MaskResolution = in->ReadInt16();

    room->WalkAreaCount = MAX_WALK_AREAS;
    if (data_ver >= kRoomVersion_240)
        room->WalkAreaCount = in->ReadInt32();
    if (room->WalkAreaCount > MAX_WALK_AREAS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walkable areas (in room: %d, max: %d).", room->WalkAreaCount, MAX_WALK_AREAS));

    if (data_ver >= kRoomVersion_200_alpha7)
        for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
            room->WalkAreas[i].ScalingFar = in->ReadInt16();
    if (data_ver >= kRoomVersion_214)
        for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
            room->WalkAreas[i].PlayerView = in->ReadInt16();
    if (data_ver >= kRoomVersion_251)
    {
        for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
            room->WalkAreas[i].ScalingNear = in->ReadInt16();
        for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
            room->WalkAreas[i].Top = in->ReadInt16();
        for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
            room->WalkAreas[i].Bottom = in->ReadInt16();
    }

    in->Seek(LEGACY_ROOM_PASSWORD_LENGTH); // skip password
    room->Options.StartupMusic = in->ReadInt8();
    room->Options.SaveLoadDisabled = in->ReadInt8() != 0;
    room->Options.PlayerCharOff = in->ReadInt8() != 0;
    room->Options.PlayerView = in->ReadInt8();
    room->Options.MusicVolume = (RoomVolumeMod)in->ReadInt8();
    room->Options.Flags = in->ReadInt8();
    in->Seek(ROOM_OPTIONS_RESERVED);

    room->MessageCount = in->ReadInt16();
    if (room->MessageCount > MAX_MESSAGES)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many room messages (in room: %d, max: %d).", room->MessageCount, MAX_MESSAGES));

    if (data_ver >= kRoomVersion_272)
        room->GameID = in->ReadInt32();

    if (data_ver >= kRoomVersion_pre114_3)
    {
        for (uint32_t i = 0; i < room->MessageCount; ++i)
        {
            room->MessageInfos[i].DisplayAs = in->ReadInt8();
            room->MessageInfos[i].Flags = in->ReadInt8();
        }
    }

    std::vector<char> mbuf(MAX_MESSAGE_PRE261_LEN + 1);
    for (uint32_t i = 0; i < room->MessageCount; ++i)
    {
        if (data_ver < kRoomVersion_261) // Room messages are not encrypted on < 2.61
        {
            StrUtil::ReadCStr(mbuf.data(), in, MAX_MESSAGE_PRE261_LEN + 1);
            room->Messages[i] = mbuf.data();
        }
        else
        {
            room->Messages[i] = read_string_decrypt(in, mbuf);
        }
    }

    // Very old format legacy room animations (FullAnimation)
    if (data_ver >= kRoomVersion_pre114_6)
    {
        // TODO: remove from format later
        size_t fullanim_count = in->ReadInt16();
        if (fullanim_count > 0)
            Debug::Printf("WARNING: Legacy room animations not supported, data skipped.");
        in->Seek(LEGACY_ROOM_FULLANIMATION_SIZE * fullanim_count);
        /* NOTE: implementation hidden in room_file_deprecated.cpp
            in->ReadArray(&fullanims[0], sizeof(FullAnimation), fullanim_count);
        */
    }

    // Ancient "graphical scripts". We currently don't support them because
    // there's no knowledge on how to convert them to modern engine.
    if ((data_ver >= kRoomVersion_pre114_4) && (data_ver < kRoomVersion_250a))
    {
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Pre-2.5 graphical scripts are no longer supported.");
        /* NOTE: implementation hidden in room_file_deprecated.cpp
            ReadPre250Scripts(in);
        */
    }

    if (data_ver >= kRoomVersion_114)
    { // NOTE: this WA value was written for the second time here, for some weird reason
        for (uint32_t i = 0; i < (size_t)MAX_WALK_AREAS; ++i)
            room->WalkAreas[i].PlayerView = in->ReadInt16();
    }
    if (data_ver >= kRoomVersion_255b)
    {
        for (uint32_t i = 0; i < room->RegionCount; ++i)
            room->Regions[i].Light = in->ReadInt16();
        for (uint32_t i = 0; i < room->RegionCount; ++i)
            room->Regions[i].Tint = in->ReadInt32();
    }

    // Primary background (LZW or RLE compressed depending on format)
    if (data_ver >= kRoomVersion_pre114_5)
        room->BgFrames[0].Graphic =
            load_lzw(in, room->BackgroundBPP, &room->Palette);
    else
        room->BgFrames[0].Graphic = load_rle_bitmap8(in);

    // Area masks
    if (data_ver >= kRoomVersion_255b)
        room->RegionMask = load_rle_bitmap8(in);
    else if (data_ver >= kRoomVersion_114)
        skip_rle_bitmap8(in); // an old version - clear the 'shadow' area into a blank regions bmp (???)
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
        if (data_ver >= kRoomVersion_3415)
            obj.Name = StrUtil::ReadString(in);
        else
            obj.Name.ReadCount(in, LEGACY_MAXOBJNAMELEN);
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
        if (data_ver >= kRoomVersion_3415)
            obj.ScriptName = StrUtil::ReadString(in);
        else
            obj.ScriptName.ReadCount(in, LEGACY_MAX_SCRIPT_NAME_LEN);
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
    if (data_ver >= kRoomVersion_255a)
    {
        for (size_t i = 0; i < room->BgFrameCount; ++i)
            room->BgFrames[i].IsPaletteShared = in->ReadInt8() != 0;
    }

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

    return new RoomFileError(kRoomFileErr_UnknownBlockType,
        String::FromFormat("Type: %s", ext_id.GetCStr()));
}


// RoomBlockReader reads whole room data, block by block
class RoomBlockReader : public DataExtReader
{
public:
    RoomBlockReader(RoomStruct *room, RoomFileVersion data_ver, std::unique_ptr<Stream> &&in)
        : DataExtReader(std::move(in),
            kDataExt_NumID8 | ((data_ver < kRoomVersion_350) ? kDataExt_File32 : kDataExt_File64))
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

HRoomFileError UpdateRoomData(RoomStruct *room, RoomFileVersion data_ver, bool game_is_hires, const std::vector<SpriteInfo> &sprinfos)
{
    if (data_ver < kRoomVersion_200_final)
    {
        room->MaskResolution = room->BgFrames[0].Graphic->GetWidth() > 320 ? RoomStruct::LegacyMaskHiresFactor : 1;
    }
    if (data_ver < kRoomVersion_3508)
    {
        // Save legacy resolution if it DOES NOT match game's;
        // otherwise it gets treated as "real resolution" by default
        const RoomResolutionType game_res_factor = game_is_hires ? kRoomResolution_High : kRoomResolution_Low;
        if (room->MaskResolution != game_res_factor)
        {
            room->SetLegacyResolution((RoomResolutionType)room->MaskResolution);
        }
    }

    // Old version - copy walkable areas to regions
    if (data_ver < kRoomVersion_255b)
    {
        if (!room->RegionMask)
            room->RegionMask.reset(BitmapHelper::CreateBitmap(room->WalkAreaMask->GetWidth(), room->WalkAreaMask->GetHeight(), 8));
        room->RegionMask->Blit(room->WalkAreaMask.get(), 0, 0, 0, 0, room->RegionMask->GetWidth(), room->RegionMask->GetHeight());
        for (size_t i = 0; i < MAX_ROOM_REGIONS; ++i)
        { // sic!! walkable areas were storing Light level in this field pre-2.55
            room->Regions[i].Light = room->WalkAreas[i].PlayerView;
            room->Regions[i].Tint = 255;
        }
    }

    // Fill in dummy interaction objects into unused slots
    // TODO: remove this later, need to rework the legacy interaction usage around the engine code to prevent crashes
    if (data_ver < kRoomVersion_300a)
    {
        if (!room->Interaction)
            room->Interaction.reset(new Interaction());
        for (size_t i = 0; i < (size_t)MAX_ROOM_HOTSPOTS; ++i)
            if (!room->Hotspots[i].Interaction)
                room->Hotspots[i].Interaction.reset(new Interaction());
        for (auto &obj : room->Objects)
            if (!obj.Interaction)
                obj.Interaction.reset(new Interaction());
        for (size_t i = 0; i < (size_t)MAX_ROOM_REGIONS; ++i)
            if (!room->Regions[i].Interaction)
                room->Regions[i].Interaction.reset(new Interaction());
    }

    // Upgade room object script names
    if (data_ver < kRoomVersion_300a)
    {
        for (auto &obj : room->Objects)
        {
            if (obj.ScriptName.GetLength() > 0)
            {
                String jibbledScriptName;
                jibbledScriptName.Format("o%s", obj.ScriptName.GetCStr());
                jibbledScriptName.MakeLower();
                if (jibbledScriptName.GetLength() >= 2)
                    jibbledScriptName.SetAt(1, toupper(jibbledScriptName[1u]));
                obj.ScriptName = jibbledScriptName;
            }
        }
    }

    // Pre-3.0.3, multiply up co-ordinates for high-res games to bring them
    // to the proper game coordinate system.
    // If you change this, also change convert_room_coordinates_to_data_res
    // function in the engine
    if (data_ver < kRoomVersion_303b && game_is_hires)
    {
        const int mul = HIRES_COORD_MULTIPLIER;
        for (auto &obj : room->Objects)
        {
            obj.X *= mul;
            obj.Y *= mul;
            if (obj.Baseline > 0)
            {
                obj.Baseline *= mul;
            }
        }

        for (uint32_t i = 0; i < room->HotspotCount; ++i)
        {
            room->Hotspots[i].WalkTo.X *= mul;
            room->Hotspots[i].WalkTo.Y *= mul;
        }

        for (uint32_t i = 0; i < room->WalkBehindCount; ++i)
        {
            room->WalkBehinds[i].Baseline *= mul;
        }

        room->Edges.Left *= mul;
        room->Edges.Top *= mul;
        room->Edges.Bottom *= mul;
        room->Edges.Right *= mul;
        room->Width *= mul;
        room->Height *= mul;
    }

    // Adjust object Y coordinate by adding sprite's height
    // NOTE: this is impossible to do without game sprite information loaded beforehand
    // NOTE: this should be done after coordinate conversion above for simplicity
    if (data_ver < kRoomVersion_300a)
    {
        for (auto &obj : room->Objects)
            obj.Y += sprinfos[obj.Sprite].Height;
    }

    if (data_ver >= kRoomVersion_251)
    {
        // if they set a contiuously scaled area where the top
        // and bottom zoom levels are identical, set it as a normal
        // scaled area
        for (uint32_t i = 0; i < room->WalkAreaCount; ++i)
        {
            if (room->WalkAreas[i].ScalingFar == room->WalkAreas[i].ScalingNear)
                room->WalkAreas[i].ScalingNear = NOT_VECTOR_SCALED;
        }
    }

    // Convert the old format region tint saturation
    if (data_ver < kRoomVersion_3404)
    {
        for (uint32_t i = 0; i < room->RegionCount; ++i)
        {
            if ((room->Regions[i].Tint & LEGACY_TINT_IS_ENABLED) != 0)
            {
                room->Regions[i].Tint &= ~LEGACY_TINT_IS_ENABLED;
                // older versions of the editor had a bug - work around it
                int tint_amount = (room->Regions[i].Light > 0 ? room->Regions[i].Light : 50);
                room->Regions[i].Tint |= (tint_amount & 0xFF) << 24;
                room->Regions[i].Light = 255;
            }
        }
    }

    // Older format room messages had flags appended to the message string
    // TODO: find out which data versions had these; is it safe to assume this was before kRoomVersion_pre114_3?
    for (uint32_t i = 0; i < room->MessageCount; ++i)
    {
        if (!room->Messages[i].IsEmpty() &&
            static_cast<uint8_t>(room->Messages[i].GetLast()) == ROOM_MESSAGE_FLAG_DISPLAYNEXT)
        {
            room->Messages[i].ClipRight(1);
            room->MessageInfos[i].Flags |= MSG_DISPLAYNEXT;
        }
    }

    // sync bpalettes[0] with room.pal
    memcpy(room->BgFrames[0].Palette, room->Palette, sizeof(RGB) * 256);
    return HRoomFileError::None();
}

HError LoadRoom(const String &filename, RoomStruct *room, AssetManager *mgr,
    bool game_is_hires, const std::vector<SpriteInfo> &sprinfos)
{
    room->Free();
    room->InitDefaults();

    RoomDataSource src;
    HRoomFileError err = OpenRoomFileFromAsset(filename, src, mgr);
    if (err)
    {
        err = ReadRoomData(room, std::move(src.InputStream), src.DataVersion);
        if (err)
            err = UpdateRoomData(room, src.DataVersion, game_is_hires, sprinfos);
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
    for (uint32_t i = 0; i < room->WalkBehindCount; ++i)
        out->WriteInt16(room->WalkBehinds[i].Baseline);

    out->WriteInt32(room->HotspotCount);
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
    {
        out->WriteInt16(room->Hotspots[i].WalkTo.X);
        out->WriteInt16(room->Hotspots[i].WalkTo.Y);
    }
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
        Common::StrUtil::WriteString(room->Hotspots[i].Name, out);
    for (uint32_t i = 0; i < room->HotspotCount; ++i)
        Common::StrUtil::WriteString(room->Hotspots[i].ScriptName, out);

    out->WriteInt32(0); // legacy poly-point areas

    out->WriteInt16(room->Edges.Top);
    out->WriteInt16(room->Edges.Bottom);
    out->WriteInt16(room->Edges.Left);
    out->WriteInt16(room->Edges.Right);

    out->WriteInt16((uint16_t)room->Objects.size());
    for (const auto &obj : room->Objects)
    {
        WriteRoomObject(obj, out);
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

    for (const auto &obj : room->Objects)
        out->WriteInt32(obj.Baseline);
    out->WriteInt16(room->Width);
    out->WriteInt16(room->Height);
    for (const auto &obj : room->Objects)
        out->WriteInt16(obj.Flags);
    out->WriteInt16(room->MaskResolution);

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
    out->WriteInt8(room->Options.StartupMusic);
    out->WriteInt8(room->Options.SaveLoadDisabled ? 1 : 0);
    out->WriteInt8(room->Options.PlayerCharOff ? 1 : 0);
    out->WriteInt8(room->Options.PlayerView);
    out->WriteInt8(room->Options.MusicVolume);
    out->WriteInt8(room->Options.Flags);
    out->WriteByteCount(0, ROOM_OPTIONS_RESERVED);
    out->WriteInt16((uint16_t)room->MessageCount);
    out->WriteInt32(room->GameID);
    for (uint32_t i = 0; i < room->MessageCount; ++i)
    {
        out->WriteInt8(room->MessageInfos[i].DisplayAs);
        out->WriteInt8(room->MessageInfos[i].Flags);
    }
    for (uint32_t i = 0; i < room->MessageCount; ++i)
        write_string_encrypt(out, room->Messages[i].GetCStr());

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

    // Write end of room file
    out->WriteByte(kRoomFile_EOF);
    return HRoomFileError::None();
}

} // namespace Common
} // namespace AGS

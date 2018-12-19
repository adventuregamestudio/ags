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

#include "ac/common.h" // update_polled_stuff
#include "ac/roomstruct.h"
#include "ac/wordsdictionary.h"
#include "debug/out.h"
#include "core/assetmanager.h"
#include "game/room_file.h"
#include "gfx/bitmap.h"
#include "script/cc_error.h"
#include "util/compress.h"
#include "util/string_utils.h"

// default number of hotspots to read from the room file
#define MIN_HOTSPOTS  20
#define LEGACY_HOTSPOT_NAME_LEN 30
#define ROOM_PASSWORD_LENGTH 11
#define LEGACY_ROOM_PASSWORD_SALT 60
#define ROOM_MESSAGE_FLAG_DISPLAYNEXT 200


namespace AGS
{
namespace Common
{

RoomDataSource::RoomDataSource()
    : DataVersion(kRoomVersion_Undefined)
{
}

String GetRoomFileErrorText(RoomFileErrorType err)
{
    switch (err)
    {
    case kRoomFileErr_NoError:
        return "No error.";
    case kRoomFileErr_FileOpenFailed:
        return "Room file was not found or could not be opened.";
    case kRoomFileErr_FormatNotSupported:
        return "Format version not supported.";
    case kRoomFileErr_UnexpectedEOF:
        return "Unexpected end of file.";
    case kRoomFileErr_UnknownBlockType:
        return "Unknown block type.";
    case kRoomFileErr_OldBlockNotSupported:
        return "Block type is too old and not supported by this version of the engine.";
    case kRoomFileErr_IncompatibleEngine:
        return "This engine cannot handle requested room content.";
    case kRoomFileErr_ScriptLoadFailed:
        return "Script load failed.";
    case kRoomFileErr_InconsistentData:
        return "Inconsistent room data, or file is corrupted.";
    case kRoomFileErr_PropertiesBlockFormat:
        return "Unknown format of the custom properties block.";
    case kRoomFileErr_InvalidPropertyValues:
        return "Errors encountered when reading custom properties.";
    }
    return "Unknown error.";
}

HRoomFileError OpenRoomFile(const String &filename, RoomDataSource &src)
{
    // Cleanup source struct
    src = RoomDataSource();
    // Try to open room file
    Stream *in = AssetManager::OpenAsset(filename);
    if (in == NULL)
        return new RoomFileError(kRoomFileErr_FileOpenFailed, String::FromFormat("Filename: %s.", filename.GetCStr()));
    // Read room header
    src.Filename = filename;
    src.DataVersion = (RoomFileVersion)in->ReadInt16();
    if (src.DataVersion < kRoomVersion_250b || src.DataVersion > kRoomVersion_Current)
        return new RoomFileError(kRoomFileErr_FormatNotSupported, String::FromFormat("Required format version: %d, supported %d - %d", src.DataVersion, kRoomVersion_250b, kRoomVersion_Current));
    // Everything is fine, return opened stream
    src.InputStream.reset(in);
    return HRoomFileError::None();
}


enum RoomFileBlock
{
    kRoomFblk_None              = 0,
    kRoomFblk_Main              = 1,
    kRoomFblk_Script            = 2,
    kRoomFblk_CompScript        = 3,
    kRoomFblk_CompScript2       = 4,
    kRoomFblk_ObjectNames       = 5,
    kRoomFblk_AnimBg            = 6,
    kRoomFblk_CompScript3       = 7,
    kRoomFblk_Properties        = 8,
    kRoomFblk_ObjectScNames     = 9,
    kRoomFile_EOF               = 0xFF
};


// Main room data
HRoomFileError ReadMainBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    char buffer[3000];

    int bpp;
    if (data_ver >= kRoomVersion_208)
        bpp = in->ReadInt32();
    else
        bpp = 1;

    if (bpp < 1)
        bpp = 1;

    rstruc->bytes_per_pixel = bpp;
    rstruc->numobj = in->ReadInt16();
    if (rstruc->numobj > MAX_OBJ)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walk-behinds (in room: %d, max: %d).", rstruc->numobj, MAX_OBJ));

    // Walk-behinds baselines
    in->ReadArrayOfInt16(&rstruc->objyval[0], rstruc->numobj);

    rstruc->numhotspots = in->ReadInt32();
    if (rstruc->numhotspots == 0)
        rstruc->numhotspots = MIN_HOTSPOTS;
    if (rstruc->numhotspots > MAX_HOTSPOTS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many hotspots (in room: %d, max: %d).", rstruc->numhotspots, MAX_HOTSPOTS));

    // Hotspots walk-to points
    for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
    {
        rstruc->hswalkto[i].x = in->ReadInt16();
        rstruc->hswalkto[i].y = in->ReadInt16();
    }

    // Hotspots names and script names
    for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
    {
        if (data_ver >= kRoomVersion_3415)
            rstruc->hotspotnames[i] = StrUtil::ReadString(in);
        else if (data_ver >= kRoomVersion_303a)
            rstruc->hotspotnames[i] = String::FromStream(in);
        else
            rstruc->hotspotnames[i] = String::FromStreamCount(in, LEGACY_HOTSPOT_NAME_LEN);
    }

    if (data_ver >= kRoomVersion_270)
    {
        for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
        {
            if (data_ver >= kRoomVersion_3415)
                rstruc->hotspotScriptNames[i] = StrUtil::ReadString(in);
            else
                rstruc->hotspotScriptNames[i] = String::FromStreamCount(in, MAX_SCRIPT_NAME_LEN);
        }
    }

    // TODO: remove from format later
    int polypoint_areas = in->ReadInt32();
    if (polypoint_areas > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Legacy poly-point areas are no longer supported.");
    /* NOTE: implementation hidden in room_file_deprecated.cpp
        for (size_t i = 0; i < (size_t)polypoint_areas; ++i)
            wallpoints[i].Read(in);
    */

    update_polled_stuff_if_runtime();

    rstruc->top = in->ReadInt16();
    rstruc->bottom = in->ReadInt16();
    rstruc->left = in->ReadInt16();
    rstruc->right = in->ReadInt16();

    // Room objects
    rstruc->numsprs = in->ReadInt16();
    if (rstruc->numsprs > MAX_INIT_SPR)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many objects (in room: %d, max: %d).", rstruc->numsprs, MAX_INIT_SPR));

    for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
        rstruc->sprs[i].ReadFromFile(in);

    // Legacy interactions
    if (data_ver >= kRoomVersion_253)
    {
        rstruc->numLocalVars = in->ReadInt32();
        if (rstruc->numLocalVars > 0)
        {
            rstruc->localvars = new InteractionVariable[rstruc->numLocalVars];
            for (size_t i = 0; i < (size_t)rstruc->numLocalVars; ++i)
                rstruc->localvars[i].Read(in);
        }
    }

    if (data_ver >= kRoomVersion_241 && data_ver < kRoomVersion_300a)
    {
        for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
            rstruc->intrHotspot[i] = Interaction::CreateFromStream(in);
        for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
            rstruc->intrObject[i] = Interaction::CreateFromStream(in);
        rstruc->intrRoom = Interaction::CreateFromStream(in);
    }

    if (data_ver >= kRoomVersion_255b)
    {
        rstruc->numRegions = in->ReadInt32();
        if (rstruc->numRegions > MAX_REGIONS)
            return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many regions (in room: %d, max: %d).", rstruc->numRegions, MAX_REGIONS));

        if (data_ver < kRoomVersion_300a)
        {
            for (size_t i = 0; i < (size_t)rstruc->numRegions; ++i)
                rstruc->intrRegion[i] = Interaction::CreateFromStream(in);
        }
    }

    // Event script links
    if (data_ver >= kRoomVersion_300a)
    {
        rstruc->hotspotScripts = new InteractionScripts*[rstruc->numhotspots];
        rstruc->objectScripts = new InteractionScripts*[rstruc->numsprs];
        rstruc->regionScripts = new InteractionScripts*[rstruc->numRegions];
        rstruc->roomScripts = InteractionScripts::CreateFromStream(in);
        for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
            rstruc->hotspotScripts[i] = InteractionScripts::CreateFromStream(in);
        for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
            rstruc->objectScripts[i] = InteractionScripts::CreateFromStream(in);
        for (size_t i = 0; i < (size_t)rstruc->numRegions; ++i)
            rstruc->regionScripts[i] = InteractionScripts::CreateFromStream(in);
    }

    if (data_ver >= kRoomVersion_200_alpha)
    {
        in->ReadArrayOfInt32(&rstruc->objbaseline[0], rstruc->numsprs);
        rstruc->width = in->ReadInt16();
        rstruc->height = in->ReadInt16();
    }

    if (data_ver >= kRoomVersion_262)
        in->ReadArrayOfInt16(&rstruc->objectFlags[0], rstruc->numsprs);

    if (data_ver >= kRoomVersion_200_final)
        rstruc->resolution = in->ReadInt16();

    rstruc->numwalkareas = MAX_WALK_AREAS;
    if (data_ver >= kRoomVersion_240)
        rstruc->numwalkareas = in->ReadInt32();
    if (rstruc->numwalkareas > MAX_WALK_AREAS + 1)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walkable areas (in room: %d, max: %d).", rstruc->numwalkareas, MAX_WALK_AREAS + 1));

    if (data_ver >= kRoomVersion_200_alpha7)
        in->ReadArrayOfInt16(&rstruc->walk_area_zoom[0], rstruc->numwalkareas);
    if (data_ver >= kRoomVersion_214)
        in->ReadArrayOfInt16(&rstruc->walk_area_light[0], rstruc->numwalkareas);
    if (data_ver >= kRoomVersion_251)
    {
        in->ReadArrayOfInt16(&rstruc->walk_area_zoom2[0], rstruc->numwalkareas);
        in->ReadArrayOfInt16(&rstruc->walk_area_top[0], rstruc->numwalkareas);
        in->ReadArrayOfInt16(&rstruc->walk_area_bottom[0], rstruc->numwalkareas);

        for (size_t i = 0; i < (size_t)rstruc->numwalkareas; ++i)
        {
            // if they set a contiuously scaled area where the top
            // and bottom zoom levels are identical, set it as a normal
            // scaled area
            if (rstruc->walk_area_zoom[i] == rstruc->walk_area_zoom2[i])
                rstruc->walk_area_zoom2[i] = NOT_VECTOR_SCALED;
        }
    }

    in->Read(&rstruc->password[0], ROOM_PASSWORD_LENGTH);
    if (data_ver < kRoomVersion_200_alpha)
    {
        for (size_t i = 0; i < (size_t)ROOM_PASSWORD_LENGTH; ++i)
            rstruc->password[i] += LEGACY_ROOM_PASSWORD_SALT;
    }
    else
    {
        for (size_t i = 0; i < (size_t)ROOM_PASSWORD_LENGTH; ++i)
            rstruc->password[i] += passwencstring[i];
    }

    in->Read(&rstruc->options[0], 10);

    rstruc->nummes = in->ReadInt16();
    if (rstruc->nummes > MAXMESS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many room messages (in room: %d, max: %d).", rstruc->nummes, MAXMESS));

    if (data_ver >= kRoomVersion_272)
        rstruc->gameId = in->ReadInt32();

    if (data_ver >= kRoomVersion_pre114_3)
    {
        for (size_t i = 0; i < (size_t)rstruc->nummes; ++i)
            rstruc->msgi[i].ReadFromFile(in);
    }

    for (size_t i = 0; i < (size_t)rstruc->nummes; ++i)
    {
        if (data_ver >= kRoomVersion_261)
            read_string_decrypt(in, buffer, sizeof(buffer));
        else
            fgetstring_limit(buffer, in, sizeof(buffer));

        int buffer_length = strlen(buffer);
        rstruc->message[i] = (char *)malloc(buffer_length + 2);
        strcpy(rstruc->message[i], buffer);

        if ((buffer_length > 0) && (buffer[buffer_length - 1] == (char)ROOM_MESSAGE_FLAG_DISPLAYNEXT))
        {
            rstruc->message[i][strlen(buffer) - 1] = 0;
            rstruc->msgi[i].flags |= MSG_DISPLAYNEXT;
        }
    }

    // Very old format legacy room animations (FullAnimation)
    if (data_ver >= kRoomVersion_pre114_6)
    {
        // TODO: remove from format later
        size_t fullanim_count = in->ReadInt16();
        if (fullanim_count > 0)
            return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Room animations are no longer supported.");
        /* NOTE: implementation hidden in room_file_deprecated.cpp
            in->ReadArray(&fullanims[0], sizeof(FullAnimation), fullanim_count);
        */
    }

    if ((data_ver >= kRoomVersion_pre114_4) && (data_ver < kRoomVersion_250a))
    {
        load_script_configuration(in);
        load_graphical_scripts(in, rstruc);
    }

    if (data_ver >= kRoomVersion_114)
        in->ReadArrayOfInt16(&rstruc->shadinginfo[0], MAX_WALK_AREAS + 1);
    if (data_ver >= kRoomVersion_255b)
    {
        in->ReadArrayOfInt16(&rstruc->regionLightLevel[0], rstruc->numRegions);
        in->ReadArrayOfInt32(&rstruc->regionTintLevel[0], rstruc->numRegions);
    }

    update_polled_stuff_if_runtime();
    // Primary background
    soff_t lzw_at = in->GetPosition();
    if (data_ver >= kRoomVersion_pre114_5)
        lzw_at = load_lzw(in, &rstruc->ebscene[0], rstruc->bytes_per_pixel, rstruc->pal);
    else
        lzw_at = loadcompressed_allegro(in, &rstruc->ebscene[0], rstruc->pal, lzw_at);

    if ((rstruc->ebscene[0]->GetWidth() > 320) & (data_ver < kRoomVersion_200_final))
        rstruc->resolution = 2;

    update_polled_stuff_if_runtime();
    // Mask bitmaps
    if (data_ver >= kRoomVersion_255b)
    {
        lzw_at = loadcompressed_allegro(in, &rstruc->regions, rstruc->pal, lzw_at);
    }
    else if (data_ver >= kRoomVersion_114)
    {
        // an old version - clear the 'shadow' area into a blank regions bmp
        lzw_at = loadcompressed_allegro(in, &rstruc->regions, rstruc->pal, lzw_at);
        delete rstruc->regions;
        rstruc->regions = NULL;
    }

    update_polled_stuff_if_runtime();
    lzw_at = loadcompressed_allegro(in, &rstruc->walls, rstruc->pal, lzw_at);
    update_polled_stuff_if_runtime();
    lzw_at = loadcompressed_allegro(in, &rstruc->object, rstruc->pal, lzw_at);
    update_polled_stuff_if_runtime();
    lzw_at = loadcompressed_allegro(in, &rstruc->lookat, rstruc->pal, lzw_at);
    return HRoomFileError::None();
}

// Room script sources (original text); only attached in the old pre-3.* games.
HRoomFileError ReadScriptBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    size_t len = in->ReadInt32();
    rstruc->scripts = (char *)malloc(len + 1);
    in->Read(rstruc->scripts, len);
    rstruc->scripts[len] = 0;
    for (size_t i = 0; i < len; ++i)
        rstruc->scripts[i] += passwencstring[i % 11];
    return HRoomFileError::None();
}

// Compiled room script
HRoomFileError ReadCompSc3Block(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    rstruc->compiled_script.reset(ccScript::CreateFromStream(in));
    if (rstruc->compiled_script == NULL)
        return new RoomFileError(kRoomFileErr_ScriptLoadFailed, ccErrorString);
    return HRoomFileError::None();
}

// Room object names
HRoomFileError ReadObjNamesBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    int name_count = in->ReadByte();
    if (name_count != rstruc->numsprs)
        return new RoomFileError(kRoomFileErr_InconsistentData,
            String::FromFormat("In the object names block, expected name count: %d, got %d", rstruc->numsprs, name_count));

    for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
    {
        if (data_ver >= kRoomVersion_3415)
            rstruc->objectnames[i] = StrUtil::ReadString(in);
        else
            rstruc->objectnames[i].ReadCount(in, LEGACY_MAXOBJNAMELEN);
    }
    return HRoomFileError::None();
}

// Room object script names
HRoomFileError ReadObjScNamesBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    int name_count = in->ReadByte();
    if (name_count != rstruc->numsprs)
        return new RoomFileError(kRoomFileErr_InconsistentData,
            String::FromFormat("In the object script names block, expected name count: %d, got %d", rstruc->numsprs, name_count));

    for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
    {
        if (data_ver >= kRoomVersion_3415)
            rstruc->objectscriptnames[i] = StrUtil::ReadString(in);
        else
            rstruc->objectscriptnames[i].ReadCount(in, MAX_SCRIPT_NAME_LEN);
    }
    return HRoomFileError::None();
}

// Secondary backgrounds
HRoomFileError ReadAnimBgBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    rstruc->num_bscenes = in->ReadByte();
    if (rstruc->num_bscenes > MAX_BSCENE)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many room backgrounds (in room: %d, max: %d).", rstruc->num_bscenes, MAX_BSCENE));

    rstruc->bscene_anim_speed = in->ReadByte();
    if (data_ver >= kRoomVersion_255a)
        in->Read(&rstruc->ebpalShared[0], rstruc->num_bscenes);
    else
        memset(&rstruc->ebpalShared[0], 0, rstruc->num_bscenes);

    for (size_t i = 1; i < (size_t)rstruc->num_bscenes; ++i)
    {
        update_polled_stuff_if_runtime();
        load_lzw(in, &rstruc->ebscene[i], rstruc->bytes_per_pixel, rstruc->bpalettes[i]);
    }
    return HRoomFileError::None();
}

// Read custom properties
HRoomFileError ReadPropertiesBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    int prop_ver = in->ReadInt32();
    if (prop_ver != 1)
        return new RoomFileError(kRoomFileErr_PropertiesBlockFormat, String::FromFormat("Expected version %d, got %d", 1, prop_ver));

    int errors = 0;
    errors += Properties::ReadValues(rstruc->roomProps, in);
    for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
        errors += Properties::ReadValues(rstruc->hsProps[i], in);
    for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
        errors += Properties::ReadValues(rstruc->objProps[i], in);

    if (errors > 0)
        return new RoomFileError(kRoomFileErr_InvalidPropertyValues);
    return HRoomFileError::None();
}

HRoomFileError ReadRoomBlock(RoomStruct *room, Stream *in, RoomFileBlock block, RoomFileVersion data_ver)
{
    soff_t block_len = data_ver < kRoomVersion_350 ? in->ReadInt32() : in->ReadInt64();
    soff_t block_end = in->GetPosition() + block_len;

    HRoomFileError err;
    switch (block)
    {
    case kRoomFblk_Main:
        err = ReadMainBlock(room, in, data_ver);
        break;
    case kRoomFblk_Script:
        err = ReadScriptBlock(room, in, data_ver);
        break;
    case kRoomFblk_CompScript3:
        err = ReadCompSc3Block(room, in, data_ver);
        break;
    case kRoomFblk_ObjectNames:
        err = ReadObjNamesBlock(room, in, data_ver);
        break;
    case kRoomFblk_ObjectScNames:
        err = ReadObjScNamesBlock(room, in, data_ver);
        break;
    case kRoomFblk_AnimBg:
        err = ReadAnimBgBlock(room, in, data_ver);
        break;
    case kRoomFblk_Properties:
        err = ReadPropertiesBlock(room, in, data_ver);
        break;
    case kRoomFblk_CompScript:
    case kRoomFblk_CompScript2:
        return new RoomFileError(kRoomFileErr_OldBlockNotSupported,
            String::FromFormat("Type: %d.", block));
    default:
        return new RoomFileError(kRoomFileErr_UnknownBlockType,
            String::FromFormat("Type: %d, known range: %d - %d.", block, kRoomFblk_Main, kRoomFblk_ObjectScNames));
    }

    if (!err)
        return err;

    soff_t cur_pos = in->GetPosition();
    if (cur_pos > block_end)
    {
        return new RoomFileError(kRoomFileErr_BlockDataOverlapping,
            String::FromFormat("Type: %d, expected to end at offset: %u, finished reading at %u.", block, block_end, cur_pos));
    }
    else if (cur_pos < block_end)
    {
        Debug::Printf(kDbgMsg_Warn, "WARNING: room data blocks nonsequential, block type %d expected to end at %u, finished reading at %u",
            block, block_end, cur_pos);
        in->Seek(block_end, Common::kSeekBegin);
    }
    return HRoomFileError::None();
}


HRoomFileError ReadRoomData(RoomStruct *room, Stream *in, RoomFileVersion data_ver)
{
    room->wasversion = data_ver;

    RoomFileBlock block;
    do
    {
        update_polled_stuff_if_runtime();
        int b = in->ReadByte();
        if (b < 0)
            return new RoomFileError(kRoomFileErr_UnexpectedEOF);
        block = (RoomFileBlock)b;
        if (block != kRoomFile_EOF)
        {
            HRoomFileError err = ReadRoomBlock(room, in, block, data_ver);
            if (!err)
                return err;
        }
    }
    while (block != kRoomFile_EOF);
    return HRoomFileError::None();
}

HRoomFileError UpdateRoomData(RoomStruct *rstruc, RoomFileVersion data_ver, bool game_is_hires)
{
    if (data_ver < kRoomVersion_255b)
    {
        // Old version - copy walkable areas to Regions
        if (!rstruc->regions)
            rstruc->regions = BitmapHelper::CreateBitmap(rstruc->walls->GetWidth(), rstruc->walls->GetHeight(), 8);
        rstruc->regions->Fill(0);
        rstruc->regions->Blit(rstruc->walls, 0, 0, 0, 0, rstruc->regions->GetWidth(), rstruc->regions->GetHeight());
        for (size_t i = 0; i < MAX_REGIONS; ++i)
        {
            rstruc->regionLightLevel[i] = rstruc->walk_area_light[i];
            rstruc->regionTintLevel[i] = 255;
        }
    }

    // Fill in dummy interaction objects into unused slots
    // TODO: remove this later, need to rework the legacy interaction usage around the engine code to prevent crashes
    if (data_ver < kRoomVersion_300a)
    {
        if (!rstruc->intrRoom)
            rstruc->intrRoom = new Interaction();
        for (size_t i = 0; i < (size_t)MAX_HOTSPOTS; ++i)
            if (!rstruc->intrHotspot[i])
                rstruc->intrHotspot[i] = new Interaction();
        for (size_t i = 0; i < (size_t)MAX_INIT_SPR; ++i)
            if (!rstruc->intrObject[i])
                rstruc->intrObject[i] = new Interaction();
        for (size_t i = 0; i < (size_t)MAX_REGIONS; ++i)
            if (!rstruc->intrRegion[i])
                rstruc->intrRegion[i] = new Interaction();
    }

    if (data_ver < kRoomVersion_303b && game_is_hires)
    {
        // Pre-3.0.3, multiply up co-ordinates
        // If you change this, also change convert_room_coordinates_to_low_res
        // function in the engine
        for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
        {
            rstruc->sprs[i].x *= 2;
            rstruc->sprs[i].y *= 2;
            if (rstruc->objbaseline[i] > 0)
            {
                rstruc->objbaseline[i] *= 2;
            }
        }

        for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
        {
            rstruc->hswalkto[i].x *= 2;
            rstruc->hswalkto[i].y *= 2;
        }

        for (size_t i = 0; i < (size_t)rstruc->numobj; ++i)
        {
            rstruc->objyval[i] *= 2;
        }

        rstruc->left *= 2;
        rstruc->top *= 2;
        rstruc->bottom *= 2;
        rstruc->right *= 2;
        rstruc->width *= 2;
        rstruc->height *= 2;
    }

    if (data_ver < kRoomVersion_3404)
    {
        // Convert the old format tint saturation
        for (size_t i = 0; i < (size_t)MAX_REGIONS; ++i)
        {
            if ((rstruc->regionTintLevel[i] & LEGACY_TINT_IS_ENABLED) != 0)
            {
                rstruc->regionTintLevel[i] &= ~LEGACY_TINT_IS_ENABLED;
                // older versions of the editor had a bug - work around it
                int tint_amount = (rstruc->regionLightLevel[i] > 0 ? rstruc->regionLightLevel[i] : 50);
                rstruc->regionTintLevel[i] |= (tint_amount & 0xFF) << 24;
                rstruc->regionLightLevel[i] = 255;
            }
        }
    }

    // sync bpalettes[0] with room.pal
    memcpy(&rstruc->bpalettes[0][0], &rstruc->pal[0], sizeof(color) * 256);
    return HRoomFileError::None();
}

} // namespace Common
} // namespace AGS

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
    if (src.DataVersion < kRoomVersion_303b || src.DataVersion > kRoomVersion_Current)
        new RoomFileError(kRoomFileErr_FormatNotSupported, String::FromFormat("Required format version: %d, supported %d - %d", src.DataVersion, kRoomVersion_303b, kRoomVersion_Current));
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


// CLNUP only for importing from 341 format, could be removed in the future
// this is necessary because we no longer use the "resolution" and the mask might be low res while the room high res
// there's a similar routine on ags.native so the editor can import and resize masks
// 
// TODO: we need a --hidden-- coordinate conversion between mask and room coordinate space.
// return back to this after RoomStruct is refactored.
//
void fix_mask_area_size(RoomStruct *rstruc, Bitmap *&mask)
{
    if (mask == NULL) return;
    if (mask->GetWidth() != rstruc->width || mask->GetHeight() != rstruc->height) {
        int oldw = mask->GetWidth(), oldh = mask->GetHeight();
        Bitmap *resized = BitmapHelper::CreateBitmap(rstruc->width, rstruc->height, 8);
        resized->Clear();
        resized->StretchBlt(mask, RectWH(0, 0, oldw, oldh), RectWH(0, 0, resized->GetWidth(), resized->GetHeight()));
        delete mask;
        mask = resized;
    }
}

// Main room data
HRoomFileError ReadMainBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    char buffer[3000];

    int bpp = in->ReadInt32();
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
        for (int i = 0; i < rstruc->numhotspots; ++i)
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

    rstruc->top = in->ReadInt16();
    rstruc->bottom = in->ReadInt16();
    rstruc->left = in->ReadInt16();
    rstruc->right = in->ReadInt16();

    // Room objects
    rstruc->numsprs = in->ReadInt16();
    if (rstruc->numsprs > MAX_INIT_SPR)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many objects (in room: %d, max: %d).", rstruc->numsprs, MAX_INIT_SPR));

    for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
    {
        rstruc->sprs[i].ReadFromFile(in);
    }

    // Legacy interaction variables (were cut out)
    rstruc->numLocalVars = in->ReadInt32();
    if (rstruc->numLocalVars > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Interaction variables are no longer supported.");
    
    // Room regions
    rstruc->numRegions = in->ReadInt32();
    if (rstruc->numRegions > MAX_REGIONS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many regions (in room: %d, max: %d).", rstruc->numRegions, MAX_REGIONS));

    // Interaction script links
    rstruc->hotspotScripts = new InteractionScripts*[rstruc->numhotspots];
    rstruc->objectScripts = new InteractionScripts*[rstruc->numsprs];
    rstruc->regionScripts = new InteractionScripts*[rstruc->numRegions];
    rstruc->roomScripts = InteractionScripts::CreateFromStream(in);

    for (size_t i = 0; i < (size_t)rstruc->numhotspots; ++i)
    {
        rstruc->hotspotScripts[i] = InteractionScripts::CreateFromStream(in);
    }
    for (size_t i = 0; i < (size_t)rstruc->numsprs; ++i)
    {
        rstruc->objectScripts[i] = InteractionScripts::CreateFromStream(in);
    }
    for (size_t i = 0; i < (size_t)rstruc->numRegions; ++i)
    {
        rstruc->regionScripts[i] = InteractionScripts::CreateFromStream(in);
    }

    in->ReadArrayOfInt32(&rstruc->objbaseline[0], rstruc->numsprs);
    rstruc->width = in->ReadInt16();
    rstruc->height = in->ReadInt16();

    in->ReadArrayOfInt16(&rstruc->objectFlags[0], rstruc->numsprs);
    // TODO: remove this when we change gamedata format
    in->ReadInt16();  // rstruc->resolution

    rstruc->numwalkareas = in->ReadInt32();
    if (rstruc->numwalkareas > MAX_WALK_AREAS + 1)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walkable areas (in room: %d, max: %d).", rstruc->numwalkareas, MAX_WALK_AREAS + 1));

    in->ReadArrayOfInt16(&rstruc->walk_area_zoom[0], rstruc->numwalkareas);
    in->ReadArrayOfInt16(&rstruc->walk_area_light[0], rstruc->numwalkareas);
    in->ReadArrayOfInt16(&rstruc->walk_area_zoom2[0], rstruc->numwalkareas);
    in->ReadArrayOfInt16(&rstruc->walk_area_top[0], rstruc->numwalkareas);
    in->ReadArrayOfInt16(&rstruc->walk_area_bottom[0], rstruc->numwalkareas);

    in->Read(&rstruc->password[0], ROOM_PASSWORD_LENGTH);
    for (size_t i = 0; i < 11; ++i)
        rstruc->password[i] += passwencstring[i];
    in->Read(&rstruc->options[0], sizeof(rstruc->options));
    rstruc->nummes = in->ReadInt16();
    rstruc->gameId = in->ReadInt32();

    for (size_t i = 0; i < (size_t)rstruc->nummes; ++i)
    {
        rstruc->msgi[i].ReadFromFile(in);
    }

    for (size_t i = 0; i < (size_t)rstruc->nummes; ++i)
    {
        read_string_decrypt(in, buffer, sizeof(buffer));
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
    rstruc->numanims = in->ReadInt16();
    if (rstruc->numanims > 0)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, "Room animations are no longer supported.");

    in->ReadArrayOfInt16(&rstruc->shadinginfo[0], 16);
    in->ReadArrayOfInt16(&rstruc->regionLightLevel[0], rstruc->numRegions);
    in->ReadArrayOfInt32(&rstruc->regionTintLevel[0], rstruc->numRegions);

    update_polled_stuff_if_runtime();
    // Primary background
    long lzw_at = load_lzw(in, &rstruc->ebscene[0], rstruc->bytes_per_pixel, rstruc->pal);

    // Mask bitmaps
    update_polled_stuff_if_runtime();
    lzw_at = loadcompressed_allegro(in, &rstruc->regions, rstruc->pal, lzw_at);

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
    in->Read(rstruc->ebpalShared, rstruc->num_bscenes);

    for (size_t i = 1; i < rstruc->num_bscenes; ++i)
    {
        update_polled_stuff_if_runtime();
        load_lzw(in, &rstruc->ebscene[i], rstruc->bytes_per_pixel, rstruc->bpalettes[i]);
    }
    return HRoomFileError::None();
}

HRoomFileError ReadPropertiesBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    // Read custom properties
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
    size_t block_len = in->ReadInt32();
    size_t block_end = in->GetPosition() + block_len;

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

    size_t cur_pos = in->GetPosition();
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

HRoomFileError UpdateRoomData(RoomStruct *rstruc, RoomFileVersion data_ver)
{
    // if they set a continiously scaled area where the top
    // and bottom zoom levels are identical, set it as a normal
    // scaled area
    for (size_t i = 0; i < (size_t)rstruc->numwalkareas; ++i)
    {
        if (rstruc->walk_area_zoom[i] == rstruc->walk_area_zoom2[i])
            rstruc->walk_area_zoom2[i] = NOT_VECTOR_SCALED;
    }

    // Convert the old format region tint saturation
    if (data_ver < kRoomVersion_3404)
    {
        for (size_t i = 0; i < MAX_REGIONS; ++i)
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

    // CLNUP ensure masks are correct size so we can test 3.4.1 games that haven't been upgraded by the editor (which fixes them upon importing the project)
    fix_mask_area_size(rstruc, rstruc->regions);
    fix_mask_area_size(rstruc, rstruc->walls);
    fix_mask_area_size(rstruc, rstruc->object);
    fix_mask_area_size(rstruc, rstruc->lookat);

    // sync bpalettes[0] with room.pal
    memcpy(rstruc->bpalettes, rstruc->pal, sizeof(color) * 256);
    return HRoomFileError::None();
}

} // namespace Common
} // namespace AGS

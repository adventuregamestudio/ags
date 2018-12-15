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


HRoomFileError ReadMainBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    int   f;
    char  buffer[3000];
    soff_t tesl;

    rstruc->width = 320;
    rstruc->height = 200;
    rstruc->resolution = 1;
    rstruc->numwalkareas = 0;
    rstruc->numhotspots = 0;

    memset(&rstruc->shadinginfo[0], 0, sizeof(short) * 16);
    memset(&rstruc->sprs[0], 0, sizeof(sprstruc) * MAX_INIT_SPR);
    memset(&rstruc->objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
    memset(&rstruc->objectFlags[0], 0, sizeof(short) * MAX_INIT_SPR);
    memset(&rstruc->hswalkto[0], 0, sizeof(_Point) * MAX_HOTSPOTS);
    memset(&rstruc->walk_area_zoom[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
    memset(&rstruc->walk_area_light[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));

    for (f = 0; f < MAX_HOTSPOTS; f++) {
        rstruc->hotspotScriptNames[f].Free();
        if (f == 0)
            rstruc->hotspotnames[f] = "No hotspot";
        else
            rstruc->hotspotnames[f].Format("Hotspot %d", f);
    }

    if (data_ver >= kRoomVersion_208)
        _acroom_bpp = in->ReadInt32();
    else
        _acroom_bpp = 1;

    if (_acroom_bpp < 1)
        _acroom_bpp = 1;

    rstruc->bytes_per_pixel = _acroom_bpp;
    rstruc->numobj = in->ReadInt16();
    if (rstruc->numobj > MAX_OBJ)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walk-behinds (in room: %d, max: %d).", rstruc->numobj, MAX_OBJ));

    in->ReadArrayOfInt16(&rstruc->objyval[0], rstruc->numobj);

    rstruc->numhotspots = in->ReadInt32();
    if (rstruc->numhotspots == 0)
        rstruc->numhotspots = 20;
    if (rstruc->numhotspots > MAX_HOTSPOTS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many hotspots (in room: %d, max: %d).", rstruc->numhotspots, MAX_HOTSPOTS));

    // Points are a pair of shorts
    // [IKM] TODO: read/write member for _Point?
    in->ReadArrayOfInt16((int16_t*)&rstruc->hswalkto[0], 2 * rstruc->numhotspots);

    for (f = 0; f < rstruc->numhotspots; f++)
    {
        if (data_ver >= kRoomVersion_3415)
            rstruc->hotspotnames[f] = StrUtil::ReadString(in);
        else if (data_ver >= kRoomVersion_303a)
            rstruc->hotspotnames[f] = String::FromStream(in);
        else
            rstruc->hotspotnames[f] = String::FromStreamCount(in, 30);
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

    rstruc->numwalkareas = in->ReadInt32();
    for (int iteratorCount = 0; iteratorCount < rstruc->numwalkareas; ++iteratorCount)
    {
        rstruc->wallpoints[iteratorCount].ReadFromFile(in);
    }

    update_polled_stuff_if_runtime();

    rstruc->top = in->ReadInt16();
    rstruc->bottom = in->ReadInt16();
    rstruc->left = in->ReadInt16();
    rstruc->right = in->ReadInt16();

    rstruc->numsprs = in->ReadInt16();

    if (rstruc->numsprs > MAX_INIT_SPR)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many objects (in room: %d, max: %d).", rstruc->numsprs, MAX_INIT_SPR));

    for (int iteratorCount = 0; iteratorCount < rstruc->numsprs; ++iteratorCount)
    {
        rstruc->sprs[iteratorCount].ReadFromFile(in);
    }

    // Legacy interactions
    if (data_ver >= kRoomVersion_253)
    {
        rstruc->numLocalVars = in->ReadInt32();
        if (rstruc->numLocalVars > 0)
        {
            rstruc->localvars = new InteractionVariable[rstruc->numLocalVars];
            for (int iteratorCount = 0; iteratorCount < rstruc->numLocalVars; ++iteratorCount)
                rstruc->localvars[iteratorCount].Read(in);
        }
    }

    rstruc->numRegions = 0;

    if (data_ver >= kRoomVersion_241) {

        // free all of the old interactions
        for (f = 0; f < MAX_HOTSPOTS; f++) {
            if (rstruc->intrHotspot[f] != NULL) {
                delete rstruc->intrHotspot[f];
                rstruc->intrHotspot[f] = NULL;
            }

            if (data_ver < kRoomVersion_300a)
            {
                if (f < rstruc->numhotspots)
                    rstruc->intrHotspot[f] = Interaction::CreateFromStream(in);
                else
                    rstruc->intrHotspot[f] = new Interaction();
            }
        }

        for (f = 0; f < MAX_INIT_SPR; f++) {
            if (rstruc->intrObject[f] != NULL) {
                delete rstruc->intrObject[f];
                rstruc->intrObject[f] = NULL;
            }

            if (data_ver < kRoomVersion_300a)
            {
                if (f < rstruc->numsprs)
                    rstruc->intrObject[f] = Interaction::CreateFromStream(in);
                else
                    rstruc->intrObject[f] = new Interaction();
            }
        }

        if (data_ver < kRoomVersion_300a)
        {
            delete rstruc->intrRoom;
            rstruc->intrRoom = Interaction::CreateFromStream(in);
        }

        for (f = 0; f < MAX_REGIONS; f++) {
            if (rstruc->intrRegion[f] != NULL)
                delete rstruc->intrRegion[f];
            rstruc->intrRegion[f] = new Interaction();
        }

        if (data_ver >= kRoomVersion_255b) {
            rstruc->numRegions = in->ReadInt32();
            if (rstruc->numRegions > MAX_REGIONS)
                return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many regions (in room: %d, max: %d).", rstruc->numRegions, MAX_REGIONS));

            if (data_ver < kRoomVersion_300a)
            {
                for (f = 0; f < rstruc->numRegions; f++) {
                    delete rstruc->intrRegion[f];
                    rstruc->intrRegion[f] = Interaction::CreateFromStream(in);
                }
            }
        }

        if (data_ver >= kRoomVersion_300a)
        {
            rstruc->hotspotScripts = new InteractionScripts*[rstruc->numhotspots];
            rstruc->objectScripts = new InteractionScripts*[rstruc->numsprs];
            rstruc->regionScripts = new InteractionScripts*[rstruc->numRegions];
            rstruc->roomScripts = InteractionScripts::CreateFromStream(in);
            int bb;
            for (bb = 0; bb < rstruc->numhotspots; bb++) {
                rstruc->hotspotScripts[bb] = InteractionScripts::CreateFromStream(in);
            }
            for (bb = 0; bb < rstruc->numsprs; bb++) {
                rstruc->objectScripts[bb] = InteractionScripts::CreateFromStream(in);
            }
            for (bb = 0; bb < rstruc->numRegions; bb++) {
                rstruc->regionScripts[bb] = InteractionScripts::CreateFromStream(in);
            }

        }
    }



    if (data_ver >= kRoomVersion_200_alpha) {
        in->ReadArrayOfInt32(&rstruc->objbaseline[0], rstruc->numsprs);
        rstruc->width = in->ReadInt16();
        rstruc->height = in->ReadInt16();
    }

    if (data_ver >= kRoomVersion_262)
        in->ReadArrayOfInt16(&rstruc->objectFlags[0], rstruc->numsprs);

    if (data_ver >= kRoomVersion_200_final)
        rstruc->resolution = in->ReadInt16();

    int num_walk_areas = MAX_WALK_AREAS;
    if (data_ver >= kRoomVersion_240)
        num_walk_areas = in->ReadInt32();

    if (num_walk_areas > MAX_WALK_AREAS + 1)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many walkable areas (in room: %d, max: %d).", num_walk_areas, MAX_WALK_AREAS + 1));

    if (data_ver >= kRoomVersion_200_alpha7)
        in->ReadArrayOfInt16(&rstruc->walk_area_zoom[0], num_walk_areas);
    if (data_ver >= kRoomVersion_214)
        in->ReadArrayOfInt16(&rstruc->walk_area_light[0], num_walk_areas);
    if (data_ver >= kRoomVersion_251) {
        in->ReadArrayOfInt16(&rstruc->walk_area_zoom2[0], num_walk_areas);
        in->ReadArrayOfInt16(&rstruc->walk_area_top[0], num_walk_areas);
        in->ReadArrayOfInt16(&rstruc->walk_area_bottom[0], num_walk_areas);

        for (f = 0; f < num_walk_areas; f++) {
            // if they set a contiuously scaled area where the top
            // and bottom zoom levels are identical, set it as a normal
            // scaled area
            if (rstruc->walk_area_zoom[f] == rstruc->walk_area_zoom2[f])
                rstruc->walk_area_zoom2[f] = NOT_VECTOR_SCALED;
        }
    }


    in->Read(&rstruc->password[0], 11);
    in->Read(&rstruc->options[0], 10);
    rstruc->nummes = in->ReadInt16();
    if (rstruc->nummes > MAXMESS)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many room messages (in room: %d, max: %d).", rstruc->nummes, MAXMESS));

    if (data_ver >= kRoomVersion_272)
        rstruc->gameId = in->ReadInt32();

    if (data_ver >= kRoomVersion_pre114_3)
    {
        for (int iteratorCount = 0; iteratorCount < rstruc->nummes; ++iteratorCount)
        {
            rstruc->msgi[iteratorCount].ReadFromFile(in);
        }
    }
    else
        memset(&rstruc->msgi[0], 0, sizeof(MessageInfo) * MAXMESS);

    for (f = 0; f < rstruc->nummes; f++) {
        if (data_ver >= kRoomVersion_261)
            read_string_decrypt(in, buffer);
        else
            fgetstring_limit(buffer, in, 2999);

        int buffer_length = strlen(buffer);

        rstruc->message[f] = (char *)malloc(buffer_length + 2);
        strcpy(rstruc->message[f], buffer);

        if ((buffer_length > 0) && (buffer[buffer_length - 1] == (char)200)) {
            rstruc->message[f][strlen(buffer) - 1] = 0;
            rstruc->msgi[f].flags |= MSG_DISPLAYNEXT;
        }
    }

    rstruc->numanims = 0;
    if (data_ver >= kRoomVersion_pre114_6) {
        rstruc->numanims = in->ReadInt16();

        if (rstruc->numanims > 0)
            // [IKM] CHECKME later: this will cause trouble if structure changes
            in->Seek(sizeof(FullAnimation) * rstruc->numanims);
        //      in->ReadArray(&rstruc->anims[0], sizeof(FullAnimation), rstruc->numanims);
    }
    else {
        rstruc->numanims = 0;
        memset(&rstruc->anims[0], 0, sizeof(FullAnimation) * MAXANIMS);
    }

    if ((data_ver >= kRoomVersion_pre114_4) && (data_ver < kRoomVersion_250a)) {
        load_script_configuration(in);
        load_graphical_scripts(in, rstruc);
    }


    if (data_ver >= kRoomVersion_114)
        in->ReadArrayOfInt16(&rstruc->shadinginfo[0], 16);
    if (data_ver >= kRoomVersion_255b) {
        in->ReadArrayOfInt16(&rstruc->regionLightLevel[0], rstruc->numRegions);
        in->ReadArrayOfInt32(&rstruc->regionTintLevel[0], rstruc->numRegions);
    }


    if (data_ver < kRoomVersion_3404)
    {
        // Convert the old format tint saturation
        for (int i = 0; i < MAX_REGIONS; ++i)
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

    update_polled_stuff_if_runtime();
    if (data_ver >= kRoomVersion_pre114_5) {
        tesl = load_lzw(in, rstruc->ebscene[0], rstruc->pal);
        rstruc->ebscene[0] = recalced;
    }
    else
        tesl = loadcompressed_allegro(in, &rstruc->ebscene[0], rstruc->pal, in->GetPosition());

    if ((rstruc->ebscene[0]->GetWidth() > 320) & (data_ver < kRoomVersion_200_final))
        rstruc->resolution = 2;

    update_polled_stuff_if_runtime();
    if (data_ver >= kRoomVersion_255b)
        tesl = loadcompressed_allegro(in, &rstruc->regions, rstruc->pal, tesl);
    else if (data_ver >= kRoomVersion_114) {
        tesl = loadcompressed_allegro(in, &rstruc->regions, rstruc->pal, tesl);
        // an old version - ->Clear the 'shadow' area into a blank regions bmp
        delete rstruc->regions;
        rstruc->regions = NULL;
    }

    update_polled_stuff_if_runtime();
    tesl = loadcompressed_allegro(in, &rstruc->walls, rstruc->pal, tesl);

    update_polled_stuff_if_runtime();
    tesl = loadcompressed_allegro(in, &rstruc->object, rstruc->pal, tesl);

    update_polled_stuff_if_runtime();
    tesl = loadcompressed_allegro(in, &rstruc->lookat, rstruc->pal, tesl);

    if (data_ver < kRoomVersion_255b) {
        // Old version - copy walkable areas to Regions
        if (rstruc->regions == NULL)
            rstruc->regions = BitmapHelper::CreateBitmap(rstruc->walls->GetWidth(), rstruc->walls->GetHeight(), 8);
        rstruc->regions->Fill(0);
        rstruc->regions->Blit(rstruc->walls, 0, 0, 0, 0, rstruc->regions->GetWidth(), rstruc->regions->GetHeight());
        for (f = 0; f <= 15; f++) {
            rstruc->regionLightLevel[f] = rstruc->walk_area_light[f];
            rstruc->regionTintLevel[f] = 255;
        }
    }

    if (data_ver < kRoomVersion_200_alpha) {
        for (f = 0; f < 11; f++)
            rstruc->password[f] += 60;
    }
    else {
        for (f = 0; f < 11; f++)
            rstruc->password[f] += passwencstring[f];
    }

    return HRoomFileError::None();
}

HRoomFileError ReadScriptBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    int   lee;
    int   hh;

    lee = in->ReadInt32();
    rstruc->scripts = (char *)malloc(lee + 5);
    // MACPORT FIX: swap
    in->Read(rstruc->scripts, lee);
    rstruc->scripts[lee] = 0;

    for (hh = 0; hh < lee; hh++)
        rstruc->scripts[hh] += passwencstring[hh % 11];

    return HRoomFileError::None();
}

HRoomFileError ReadCompSc3Block(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    rstruc->compiled_script.reset(ccScript::CreateFromStream(in));
    if (rstruc->compiled_script == NULL)
        return new RoomFileError(kRoomFileErr_ScriptLoadFailed, ccErrorString);
    return HRoomFileError::None();
}

HRoomFileError ReadObjNamesBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    int name_count = in->ReadByte();
    if (name_count != rstruc->numsprs)
        return new RoomFileError(kRoomFileErr_InconsistentData,
            String::FromFormat("In the object names block, expected name count: %d, got %d", rstruc->numsprs, name_count));

    for (int i = 0; i < rstruc->numsprs; ++i)
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

    for (int i = 0; i < rstruc->numsprs; ++i)
    {
        if (data_ver >= kRoomVersion_3415)
            rstruc->objectscriptnames[i] = StrUtil::ReadString(in);
        else
            rstruc->objectscriptnames[i].ReadCount(in, MAX_SCRIPT_NAME_LEN);
    }
    return HRoomFileError::None();
}

HRoomFileError ReadAnimBgBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    int   ct;
    long  fpos;

    rstruc->num_bscenes = in->ReadByte();
    if (rstruc->num_bscenes > MAX_BSCENE)
        return new RoomFileError(kRoomFileErr_IncompatibleEngine, String::FromFormat("Too many room backgrounds (in room: %d, max: %d).", rstruc->num_bscenes, MAX_BSCENE));

    rstruc->bscene_anim_speed = in->ReadByte();
    if (data_ver >= kRoomVersion_255a)
        in->Read(&rstruc->ebpalShared[0], rstruc->num_bscenes);
    else
        memset(&rstruc->ebpalShared[0], 0, rstruc->num_bscenes);

    fpos = in->GetPosition();
    //        fclose(in);

    for (ct = 1; ct < rstruc->num_bscenes; ct++) {
        update_polled_stuff_if_runtime();
        //          fpos = load_lzw(files,rstruc->ebscene[ct],rstruc->pal,fpos);
        fpos = load_lzw(in, rstruc->ebscene[ct], rstruc->bpalettes[ct]);
        rstruc->ebscene[ct] = recalced;
    }
    //        in = Common::AssetManager::OpenAsset(files, "rb");
    //        Seek(in, fpos, SEEK_SET);
    return HRoomFileError::None();
}

HRoomFileError ReadPropertiesBlock(RoomStruct *rstruc, Stream *in, RoomFileVersion data_ver)
{
    // Read custom properties
    int prop_ver = in->ReadInt32();
    if (prop_ver != 1)
        return new RoomFileError(kRoomFileErr_PropertiesBlockFormat, String::FromFormat("Expected version %d, got %d", 1, prop_ver));

    int errors = 0, gg;

    errors += Properties::ReadValues(rstruc->roomProps, in);
    for (gg = 0; gg < rstruc->numhotspots; gg++)
        errors += Properties::ReadValues(rstruc->hsProps[gg], in);
    for (gg = 0; gg < rstruc->numsprs; gg++)
        errors += Properties::ReadValues(rstruc->objProps[gg], in);

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
    if (data_ver < kRoomVersion_303b && game_is_hires)
    {
        // Pre-3.0.3, multiply up co-ordinates
        // If you change this, also change convert_room_coordinates_to_low_res
        // function in the engine
        int f;
        for (f = 0; f < rstruc->numsprs; f++)
        {
            rstruc->sprs[f].x *= 2;
            rstruc->sprs[f].y *= 2;
            if (rstruc->objbaseline[f] > 0)
            {
                rstruc->objbaseline[f] *= 2;
            }
        }

        for (f = 0; f < rstruc->numhotspots; f++)
        {
            rstruc->hswalkto[f].x *= 2;
            rstruc->hswalkto[f].y *= 2;
        }

        for (f = 0; f < rstruc->numobj; f++)
        {
            rstruc->objyval[f] *= 2;
        }

        rstruc->left *= 2;
        rstruc->top *= 2;
        rstruc->bottom *= 2;
        rstruc->right *= 2;
        rstruc->width *= 2;
        rstruc->height *= 2;
    }

    // sync bpalettes[0] with room.pal
    memcpy(&rstruc->bpalettes[0][0], &rstruc->pal[0], sizeof(color) * 256);
    return HRoomFileError::None();
}

} // namespace Common
} // namespace AGS


#include "ac/common.h"
#include "ac/wordsdictionary.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/roominfo.h"
#include "gfx/bitmap.h"
#include "script/cc_script.h"
#include "util/compress.h"
#include "util/stream.h"
#include "util/string_utils.h"

// FIXME later, used in lzw decompression
int _acroom_bpp = 1; // bytes per pixel of currently loading room

namespace AGS
{
namespace Common
{

RoomObjectInfo::RoomObjectInfo()
    : Id(0)
    , X(0)
    , Y(0)
    , RoomIndex(-1)
    , IsOn(false)
{
}

void RoomObjectInfo::ReadFromFile(Common::Stream *in)
{
    Id = in->ReadInt16();
    X = in->ReadInt16();
    Y = in->ReadInt16();
    RoomIndex = in->ReadInt16();
    IsOn = in->ReadInt16() ? true : false;
}

RoomInfo::RoomInfo()
    : WalkAreaMask(NULL)
    , WalkBehindMask(NULL)
    , HotspotMask(NULL)
    , RegionMask(NULL)
    , RoomInteraction(NULL)
    , RoomScripts(NULL)
    , TextScripts(NULL)
    , CompiledScript(NULL)
{
    InitDefaults();
}

RoomInfo::~RoomInfo()
{
    Free();
}

/* static */ bool RoomInfo::Load(RoomInfo &room, const String &filename, bool game_is_hires)
{
    update_polled_stuff_if_runtime();

    Stream *in = AssetManager::OpenAsset(filename);
    if (in == NULL)
    {
        String err_msg = String::FromFormat(
            "Load_room: Unable to load the room file '%s'\n"
            "Make sure that you saved the room to the correct folder (it should be\n"
            "in your game's sub-folder of the AGS directory).\n"
            "Also check that the player character's starting room is set correctly.\n",
            filename.GetCStr());
        quit(err_msg);
    }
    update_polled_stuff_if_runtime();  // it can take a while to load the file sometimes

    RoomFormatBlock last_block;
    RoomInfoError load_err = room.ReadFromFile(in, game_is_hires, &last_block);
    delete in;

    switch (load_err)
    {
    case kRoomInfoErr_InternalLogicError:
        quit("Load_Room: internal logic error.\n");
        break;
    case kRoomInfoErr_UnexpectedEOF:
        quit("LoadRoom: unexpected end of file while loading room");
        break;
    case kRoomInfoErr_FormatNotSupported:
        quit("Load_Room: Bad packed file. Either the file requires a newer or older version of\n"
            "this program or the file is corrupt.\n");
        break;
    case kRoomInfoErr_UnknownBlockType:
        quit(String::FromFormat("LoadRoom: unknown block type %d encountered in '%s'", last_block, filename));
        break;
    case kRoomInfoErr_OldBlockNotSupported:
        quit("Load_room: old room format. Please upgrade the room.");
        break;
    case kRoomInfoErr_InconsistentDataForObjectNames:
        quit("Load_room: inconsistent blocks for object names");
        break;
    case kRoomInfoErr_ScriptLoadFailed:
        quit("Load_room: Script load failed; need newer version?");
        break;
    case kRoomInfoErr_PropertiesFormatNotSupported:
        quit("LoadRoom: unknown Custom Properties block encounreted");
        break;
    case kRoomInfoErr_PropertiesLoadFailed:
        quit("LoadRoom: error reading custom properties block");
        break;
    case kRoomInfoErr_InconsistentDataForObjectScriptNames:
        quit("Load_room: inconsistent blocks for object script names");
        break;
    }
    return load_err == kRoomInfoErr_NoError;
}

void RoomInfo::Free()
{
    delete WalkAreaMask;
    delete WalkBehindMask;
    delete HotspotMask;
    delete RegionMask;
    WalkAreaMask = NULL;
    WalkBehindMask = NULL;
    HotspotMask = NULL;
    RegionMask = NULL;
    
    WalkBehindBaselines.Free();
    RoomObjects.Free();
    for (int i = 0; i < RoomObjectInteractions.GetCount(); ++i)
    {
        delete RoomObjectInteractions[i];
    }
    RoomObjectInteractions.Free();
    for (int i = 0; i < RoomObjectScripts.GetCount(); ++i)
    {
        delete RoomObjectScripts[i];
    }
    RoomObjectScripts.Free();
    RoomObjectBaselines.Free();
    RoomObjectFlags.Free();
    RoomObjectNames.Free();
    RoomObjectScriptNames.Free();
    RoomObjectProperties.Free();
    Password.Free();
    Messages.Free();
    MessageInfos.Free();
    
    WalkAreaShadingView.Free();
    WallPoints.Free();
    HotspotWalkToPoints.Free();
    HotspotNames.Free();
    HotspotScriptNames.Free();
    for (int i = 0; i < HotspotInteractions.GetCount(); ++i)
    {
        delete HotspotInteractions[i];
    }
    HotspotInteractions.Free();
    for (int i = 0; i < RegionInteractions.GetCount(); ++i)
    {
        delete RegionInteractions[i];
    }
    RegionInteractions.Free();
    delete RoomInteraction;
    RoomInteraction = NULL;
    for (int i = 0; i < HotspotScripts.GetCount(); ++i)
    {
        delete HotspotScripts[i];
    }
    HotspotScripts.Free();
    for (int i = 0; i < RegionScripts.GetCount(); ++i)
    {
        delete RegionScripts[i];
    }
    RegionScripts.Free();
    delete RoomScripts;
    RoomScripts = NULL;
    
    RegionLightLevels.Free();
    RegionTintLevels.Free();
    WalkAreaZoom.Free();
    WalkAreaZoom2.Free();
    WalkAreaLight.Free();
    WalkAreaTop.Free();
    WalkAreaBottom.Free();
    if (TextScripts)
    {
        free(TextScripts);
        TextScripts = NULL;
    }
    delete CompiledScript;
    CompiledScript = NULL;
    
    for (int i = 0; i < BackgroundScenes.GetCount(); ++i)
    {
        delete BackgroundScenes[i];
        update_polled_stuff_if_runtime();
    }
    BackgroundScenes.Free();
    BkgScenePalettes.Free();
    LocalVariables.Free();
    BkgScenePaletteShared.Free();
    HotspotProperties.Free();
}

RoomInfoError RoomInfo::ReadFromFile(Stream *in, bool game_is_hires, RoomFormatBlock *last_block)
{
    Free();

    if (!in)
    {
        return kRoomInfoErr_InternalLogicError;
    }

    InitDefaults();

    LoadedVersion = (RoomFileVersion)in->ReadInt16();
    if (LoadedVersion < kRoomVersion_250b || LoadedVersion > kRoomVersion_Current)
    {
        return kRoomInfoErr_FormatNotSupported;
    }

    RoomFormatBlock block_type = kRoomBlock_None;
    while(true)
    {
        block_type = (RoomFormatBlock)in->ReadByte();
        if (last_block)
        {
            *last_block = block_type;
        }
        if (block_type < 0)
        {
            return kRoomInfoErr_UnexpectedEOF;
        }
        if (block_type == kRoomBlock_End)
        {
            break;
        }
        RoomInfoError error = ReadBlock(in, block_type);
        if (error != kRoomInfoErr_NoError)
        {
            return error;
        }
        update_polled_stuff_if_runtime();
    };

    ProcessAfterRead(game_is_hires);
    return kRoomInfoErr_NoError;
}

void RoomInfo::InitDefaults()
{
    Width       = 320;
    Height      = 200;
    LeftEdge    = 0;
    RightEdge   = 317;
    TopEdge     = 40;
    BottomEdge  = 199;
    Resolution  = 1;
    BytesPerPixel = 1;

    WalkBehindCount = 0;
    RoomObjectCount = 0;
    MessageCount    = 0;
    AnimationCount  = 0;
    WalkAreaCount   = 0;
    HotspotCount    = 0;
    RegionCount     = 0;
    BkgSceneCount   = 1;
    LocalVariableCount = 0;

    BackgroundScenes.New(1, NULL);
    BkgScenePalettes.New(1);
    BkgScenePaletteShared.New(1, 0);

    BkgSceneAnimSpeed = 5;

    CompiledScriptShared = false;
    CompiledScriptSize = 0;

    memset(Palette, 0, sizeof(Palette));
    memset(Options, 0, sizeof(Options));

    GameId = NO_GAME_ID_IN_ROOM_FILE;
    LoadedVersion = kRoomVersion_Current;

    // TODO: this is done for safety reasons;
    // should be reworked when all the code is altered to remove any
    // usage of MAX_* constants for room objects and regions.
    RoomObjects.New(MAX_INIT_SPR);
    RoomObjectInteractions.New(MAX_INIT_SPR, NULL);
    RoomObjectScripts.New(MAX_INIT_SPR, NULL);
    RoomObjectBaselines.New(MAX_INIT_SPR, 0xFF);
    RoomObjectFlags.New(MAX_INIT_SPR, 0);
    RoomObjectNames.New(MAX_INIT_SPR);
    RoomObjectScriptNames.New(MAX_INIT_SPR);
    RoomObjectProperties.New(MAX_INIT_SPR);

    WalkBehindBaselines.New(MAX_OBJ);

    WalkAreaShadingView.New(MAX_WALK_AREAS + 1, 0);
    WalkAreaZoom.New(MAX_WALK_AREAS + 1, 0);
    WalkAreaZoom2.New(MAX_WALK_AREAS + 1, NOT_VECTOR_SCALED);
    WalkAreaLight.New(MAX_WALK_AREAS + 1, 0);
    WalkAreaTop.New(MAX_WALK_AREAS + 1, -1);
    WalkAreaBottom.New(MAX_WALK_AREAS + 1, -1);
        
    HotspotWalkToPoints.New(MAX_HOTSPOTS);
    HotspotNames.New(MAX_HOTSPOTS);
    HotspotScriptNames.New(MAX_HOTSPOTS);
    HotspotInteractions.New(MAX_HOTSPOTS, NULL);
    HotspotScripts.New(MAX_HOTSPOTS, NULL);
    HotspotProperties.New(MAX_HOTSPOTS);

    RegionInteractions.New(MAX_REGIONS, NULL);
    RegionScripts.New(MAX_REGIONS, NULL);
    RegionLightLevels.New(MAX_REGIONS, 0);
    RegionTintLevels.New(MAX_REGIONS, 0);
    
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        RoomObjectInteractions[i] = new NewInteraction();
    }
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        HotspotInteractions[i] = new NewInteraction();
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        RegionInteractions[i] = new NewInteraction();
    }
    // end TODO
}

RoomInfoError RoomInfo::ReadBlock(Stream *in, RoomFormatBlock block_type)
{
    if (!in || block_type == kRoomBlock_End)
    {
        return kRoomInfoErr_InternalLogicError;
    }

    int block_length      = in->ReadInt32();
    size_t next_block_pos = in->GetPosition() + block_length;
    RoomInfoError read_block_err = kRoomInfoErr_NoError;

    switch (block_type)
    {
    case kRoomBlock_Main:
        read_block_err = ReadMainBlock(in);
        break;
    case kRoomBlock_Script:
        read_block_err = ReadScriptBlock(in);
        break;
    case kRoomBlock_CompScript:
    case kRoomBlock_CompScript2:
        return kRoomInfoErr_OldBlockNotSupported;
    case kRoomBlock_ObjectNames:
        read_block_err = ReadObjectNamesBlock(in);
        break;
    case kRoomBlock_AnimBkg:
        read_block_err = ReadAnimBkgBlock(in);
        break;
    case kRoomBlock_CompScript3:
        read_block_err = ReadScript3Block(in);
        break;
    case kRoomBlock_Properties:
        read_block_err = ReadPropertiesBlock(in);
        break;
    case kRoomBlock_ObjectScriptNames:
        read_block_err = ReadObjectScriptNamesBlock(in);
        break;
    default:
        return kRoomInfoErr_UnknownBlockType;
    };

    if (read_block_err != kRoomInfoErr_NoError)
    {
        return read_block_err;
    }

    size_t current_pos = in->GetPosition();
    if (current_pos != next_block_pos)
    {
        Out::FPrint("WARNING: room data blocks nonsequential, block type %d expected to end at %d, reading ended at %d",
            block_type, next_block_pos, current_pos);
        in->Seek(Common::kSeekBegin, next_block_pos);
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadMainBlock(Stream *in)
{
    if (LoadedVersion >= kRoomVersion_208)
    {
        _acroom_bpp = in->ReadInt32();
    }
    else
    {
        _acroom_bpp = 1;
    }

    if (_acroom_bpp < 1)
    {
        _acroom_bpp = 1;
    }

    BytesPerPixel = _acroom_bpp;
    WalkBehindCount = in->ReadInt16();
    WalkBehindBaselines.ReadRawOver(in, 0, WalkBehindCount);

    HotspotCount = in->ReadInt32();
    // FIXME: check version!!!
    // Newer versions with dynamic limits should not do this,
    if (HotspotCount == 0)
    {
        HotspotCount = 20;
    }

    for (int i = 0; i < HotspotCount; ++i)
    {
        HotspotWalkToPoints[i].x = in->ReadInt16();
        HotspotWalkToPoints[i].y = in->ReadInt16();
    }

	for (int i = 0; i < HotspotCount; ++i)
	{
        if (LoadedVersion >= kRoomVersion_303a)
		{
            HotspotNames[i].Read(in, 2999);
		}
		else
		{
            HotspotNames[i].ReadCount(in, 30);
		}
	}

    if (LoadedVersion >= kRoomVersion_270)
    {
        for (int i = 0; i < HotspotCount; ++i)
	    {
            HotspotScriptNames[i].ReadCount(in, MAX_SCRIPT_NAME_LEN);
        }
    }
    
    WalkAreaCount = in->ReadInt32();
    WallPoints.New(WalkAreaCount);
    for (int i = 0; i < WalkAreaCount; ++i)
    {
        WallPoints[i].ReadFromFile(in);
    }
  
    update_polled_stuff_if_runtime();

    TopEdge = in->ReadInt16();
    BottomEdge = in->ReadInt16();
    LeftEdge = in->ReadInt16();
    RightEdge = in->ReadInt16();

    RoomObjectCount = in->ReadInt16();
    for (int i = 0; i < RoomObjectCount; ++i)
    {
        RoomObjects[i].ReadFromFile(in);
    }

    if (LoadedVersion >= kRoomVersion_253)
    {
        LocalVariableCount = in->ReadInt32();
        LocalVariables.New(LocalVariableCount);
        for (int i = 0; i < LocalVariableCount; ++i)
        {
            LocalVariables[i].ReadFromFile(in);
        }
    }

    if (LoadedVersion >= kRoomVersion_241)
    {
        if (LoadedVersion < kRoomVersion_300a) 
        {
            for (int i = 0; i < HotspotCount; ++i)
            {
                delete HotspotInteractions[i];
                HotspotInteractions[i] = deserialize_new_interaction(in);
            }
            for (int i = 0; i < RoomObjectCount; ++i)
            {
                delete RoomObjectInteractions[i];
                RoomObjectInteractions[i] = deserialize_new_interaction(in);
            }
            RoomInteraction = deserialize_new_interaction(in);
        } // LoadedVersion >= kRoomVersion_300a

        if (LoadedVersion >= kRoomVersion_255b)
        {
            RegionCount = in->ReadInt32();
            if (LoadedVersion < kRoomVersion_300a) 
            {
                for (int i = 0; i < RegionCount; ++i)
                {
                    delete RegionInteractions[i];
                    RegionInteractions[i] = deserialize_new_interaction(in);
		        }
            }
        }

	    if (LoadedVersion >= kRoomVersion_300a)
        {
            RoomScripts = new InteractionScripts();
	        deserialize_interaction_scripts(in, RoomScripts);
            for (int i = 0; i < HotspotCount; ++i)
            {
                HotspotScripts[i] = new InteractionScripts();
                deserialize_interaction_scripts(in, HotspotScripts[i]);
            }
            for (int i = 0; i < RoomObjectCount; ++i)
            {
                RoomObjectScripts[i] = new InteractionScripts();
                deserialize_interaction_scripts(in, RoomObjectScripts[i]);
            }
            for (int i = 0; i < RegionCount; ++i)
            {
                RegionScripts[i] = new InteractionScripts();
                deserialize_interaction_scripts(in, RegionScripts[i]);
            }
	    } // LoadedVersion >= kRoomVersion_300a
    } // LoadedVersion >= kRoomVersion_241

    if (LoadedVersion >= kRoomVersion_200_alpha)
    {
        RoomObjectBaselines.ReadRawOver(in, 0, RoomObjectCount);
        Width = in->ReadInt16();
        Height = in->ReadInt16();
    }

    if (LoadedVersion >= kRoomVersion_262)
    {
        RoomObjectFlags.ReadRawOver(in, 0, RoomObjectCount);
    }

    if (LoadedVersion >= kRoomVersion_200_final)
    {
        Resolution = in->ReadInt16();
    }

    WalkAreaCount = MAX_WALK_AREAS;
    if (LoadedVersion >= kRoomVersion_240)
    {
        WalkAreaCount = in->ReadInt32();
    }
    
    if (LoadedVersion >= kRoomVersion_200_alpha7)
    {
        WalkAreaZoom.ReadRawOver(in, 0, WalkAreaCount);
    }

    if (LoadedVersion >= kRoomVersion_214)
    {
        WalkAreaLight.ReadRawOver(in, 0, WalkAreaCount);
    }

    if (LoadedVersion >= kRoomVersion_251)
    {
        WalkAreaZoom2.ReadRawOver(in, 0, WalkAreaCount);
        WalkAreaTop.ReadRawOver(in, 0, WalkAreaCount);
        WalkAreaBottom.ReadRawOver(in, 0, WalkAreaCount);

        for (int i = 0; i < WalkAreaCount; ++i)
        {
            // if they set a contiuously scaled area where the top
            // and bottom zoom levels are identical, set it as a normal
            // scaled area
            if (WalkAreaZoom[i] == WalkAreaZoom2[i])
            {
                WalkAreaZoom2[i] = NOT_VECTOR_SCALED;
            }
        }
    }

    Password.ReadCount(in, 11);
    in->Read(Options, 10);

    MessageCount = in->ReadInt16();
    Messages.New(MessageCount);

    if (LoadedVersion >= kRoomVersion_272)
    {
        GameId = in->ReadInt32();
    }

    if (LoadedVersion >= kRoomVersion_pre114_3)
    {
        MessageInfos.New(MessageCount);
        for (int i = 0; i < MessageCount; ++i)
        {
            MessageInfos[i].ReadFromFile(in);
        }
    }

    char message_buffer[3000];
    for (int i = 0; i < MessageCount; ++i)
    {
        if (LoadedVersion >= kRoomVersion_261)
        {
            read_string_decrypt(in, message_buffer);
        }
        else
        {
            fgetstring_limit(message_buffer, in, 2999);
        }

        int msg_last_char = strlen(message_buffer) - 1;
        if (message_buffer[msg_last_char] == (char)200)
        {
          message_buffer[msg_last_char] = 0;
          MessageInfos[i].flags |= MSG_DISPLAYNEXT;
        }
        Messages[i] = message_buffer;
    }

    if (LoadedVersion >= kRoomVersion_pre114_6)
    {
        AnimationCount = in->ReadInt16();
        if (AnimationCount > 0)
            // [IKM] CHECKME later: this will cause trouble if structure changes
            in->Seek (Common::kSeekCurrent, sizeof(FullAnimation) * AnimationCount);
    //      in->ReadArray(&rstruc->anims[0], sizeof(FullAnimation), rstruc->numanims);
    }

    if (LoadedVersion >= kRoomVersion_pre114_4 && LoadedVersion < kRoomVersion_250a)
    {
        LoadScriptConfiguration(in);
        LoadGraphicalScripts(in);
    }

    if (LoadedVersion >= kRoomVersion_114)
    {
        WalkAreaShadingView.ReadRawOver(in, 0, MAX_WALK_AREAS + 1 /* FIXME use variable */);
    }

    if (LoadedVersion >= kRoomVersion_255b)
    {
        RegionLightLevels.ReadRawOver(in, 0, RegionCount);
        RegionTintLevels.ReadRawOver(in, 0, RegionCount);
    }

    update_polled_stuff_if_runtime();

    // Background scenes array should already have one element at this point
    if (LoadedVersion >= kRoomVersion_pre114_5)
    {
        load_lzw(in, BackgroundScenes[0], Palette);
        BackgroundScenes[0] = recalced;
    }
    else
    {
        loadcompressed_allegro(in, &BackgroundScenes[0], Palette);
    }

    if (BackgroundScenes[0]->GetWidth() > 320 && LoadedVersion < kRoomVersion_200_final)
    {
        Resolution = 2;
    }

    update_polled_stuff_if_runtime();
    if (LoadedVersion >= kRoomVersion_255b)
    {
        loadcompressed_allegro(in, &RegionMask, Palette);
    }
    else if (LoadedVersion >= kRoomVersion_114)
    {
        loadcompressed_allegro(in, &RegionMask, Palette);
        // an old version - clear the 'shadow' area into a blank regions bmp
        delete RegionMask;
        RegionMask = NULL;
    }

    update_polled_stuff_if_runtime();
    loadcompressed_allegro(in, &WalkAreaMask, Palette);
    update_polled_stuff_if_runtime();
    loadcompressed_allegro(in, &WalkBehindMask, Palette);
    update_polled_stuff_if_runtime();
    loadcompressed_allegro(in, &HotspotMask, Palette);

    if (LoadedVersion < kRoomVersion_255b)
    {
        // Old version - copy walkable areas to Regions
        if (RegionMask == NULL)
        {
            RegionMask = BitmapHelper::CreateBitmap(WalkAreaMask->GetWidth(), WalkAreaMask->GetHeight(), 8);
            Graphics graphics(RegionMask);
            graphics.Fill(0);
            graphics.Blit(WalkAreaMask, 0, 0, 0, 0, WalkAreaMask->GetWidth(), WalkAreaMask->GetHeight());
            RegionLightLevels = WalkAreaLight;
        }
    }

    if (LoadedVersion < kRoomVersion_200_alpha)
    {
        for (int i = 0; i < 11; ++i)
        {
            Password.SetAt(i, Password[i] + 60);
        }
    }
    else
    {
        for (int i = 0; i < 11; ++i)
        {
            Password.SetAt(i, Password[i] + passwencstring[i]);
        }
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadScriptBlock(Stream *in)
{
    int script_len = in->ReadInt32();
    TextScripts = (char *)malloc(script_len + 5);
    in->Read(TextScripts, script_len);
    TextScripts[script_len] = 0;

    for (int i = 0; i < script_len; ++i)
    {
        TextScripts[i] += passwencstring[i % 11];
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadObjectNamesBlock(Stream *in)
{
    // TODO: this will cause problems if object count > 255
    if (in->ReadByte() != RoomObjectCount)
    {
        return kRoomInfoErr_InconsistentDataForObjectNames;
    }
    for (int i = 0; i < RoomObjectCount; ++i)
    {
        RoomObjectNames[i].ReadCount(in, MAXOBJNAMELEN);
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadAnimBkgBlock(Stream *in)
{
    BkgSceneCount = in->ReadByte();
    BkgSceneAnimSpeed = in->ReadByte();

    BackgroundScenes.AppendCount(BkgSceneCount - 1, NULL);
    BkgScenePalettes.AppendCount(BkgSceneCount - 1);
    BkgScenePaletteShared.AppendCount(BkgSceneCount - 1, 0);

    if (LoadedVersion >= kRoomVersion_255a)
    {
        BkgScenePaletteShared.ReadRawOver(in, 0, BkgSceneCount);
    }

    for (int i = 1; i < BkgSceneCount; ++i)
    {
        update_polled_stuff_if_runtime();
        load_lzw(in, BackgroundScenes[i], BkgScenePalettes[i]);
        BackgroundScenes[i] = recalced;
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadScript3Block(Stream *in)
{
    CompiledScript = ccScript::CreateFromStream(in);
    CompiledScriptShared = false;
    return CompiledScript ? kRoomInfoErr_NoError : kRoomInfoErr_ScriptLoadFailed;
}

RoomInfoError RoomInfo::ReadPropertiesBlock(Stream *in)
{
    if (in->ReadInt32() != 1)
    {
        return kRoomInfoErr_PropertiesFormatNotSupported;
    }

    if (RoomProperties.UnSerialize(in))
    {
        return kRoomInfoErr_PropertiesLoadFailed;
    }

    for (int i = 0; i < HotspotCount; ++i)
    {
        if (HotspotProperties[i].UnSerialize(in))
        {
            return kRoomInfoErr_PropertiesLoadFailed;
        }
    }
    for (int i = 0; i < RoomObjectCount; ++i)
    {
        if (RoomObjectProperties[i].UnSerialize(in))
        {
            return kRoomInfoErr_PropertiesLoadFailed;
        }
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadObjectScriptNamesBlock(Stream *in)
{
    if (in->ReadByte() != RoomObjectCount)
    {
        return kRoomInfoErr_InconsistentDataForObjectScriptNames;
    }
    for (int i = 0; i < RoomObjectCount; ++i)
    {
        RoomObjectScriptNames[i].ReadCount(in, MAX_SCRIPT_NAME_LEN);
    }

    return kRoomInfoErr_NoError;
}

void RoomInfo::ProcessAfterRead(bool game_is_hires)
{
    // Synchronize background 0 palette with room.pal
    memcpy(&BkgScenePalettes[0], &Palette, sizeof(color) * 256);

    if (LoadedVersion < kRoomVersion_303b && game_is_hires)
    {
        // Pre-3.0.3, multiply up co-ordinates
        // If you change this, also change convert_room_coordinates_to_low_res
        // function in the engine.
	    for (int i = 0; i < RoomObjectCount; ++i)
	    {
            RoomObjects[i].X <<= 1;
            RoomObjects[i].Y <<= 1;
            if (RoomObjectBaselines[i] > 0)
            {
                RoomObjectBaselines[i] <<= 1;
            }
        }

        for (int i = 0; i < HotspotCount; ++i)
        {
            HotspotWalkToPoints[i].x <<= 1;
            HotspotWalkToPoints[i].y <<= 1;
        }

        for (int i = 0; i < WalkBehindCount; ++i)
        {
            WalkBehindBaselines[i] <<= 1;
        }

        LeftEdge    <<= 1;
        TopEdge     <<= 1;
        BottomEdge  <<= 1;
        RightEdge   <<= 1;
        Width       <<= 1;
        Height      <<= 1;
    }
}

void RoomInfo::WriteToFile(Stream *out)
{
}

} // namespace Common
} // namespace AGS

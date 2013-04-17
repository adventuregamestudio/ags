
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

RoomBackgroundInfo::RoomBackgroundInfo()
    : Graphic(NULL)
    , PaletteShared(0)
{
}

RoomBackgroundInfo::RoomBackgroundInfo(const RoomBackgroundInfo &bkg_info)
{
    Graphic = BitmapHelper::CreateBitmapReference(bkg_info.Graphic);
    memcpy(Palette, bkg_info.Palette, sizeof(Palette));
    PaletteShared = bkg_info.PaletteShared;
}

RoomBackgroundInfo::~RoomBackgroundInfo()
{
    delete Graphic;
}

RoomObjectInfo::RoomObjectInfo()
    : Id(0)
    , X(0)
    , Y(0)
    , RoomIndex(-1)
    , IsOn(false)
    , Baseline(0xFF)
    , Flags(0)
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

RoomRegionInfo::RoomRegionInfo()
    : Light(0)
    , Tint(0)
{
}

WalkAreaInfo::WalkAreaInfo()
    : ShadingView(0)
    , Zoom(0)
    , Zoom2(NOT_VECTOR_SCALED)
    , Light(0)
    , Top(-1)
    , Bottom(-1)
{
}

WalkBehindInfo::WalkBehindInfo()
    : Baseline(0)
{
}

RoomInfo::RoomInfo()
    : HotspotMask(NULL)
    , RegionMask(NULL)
    , WalkAreaMask(NULL)
    , WalkBehindMask(NULL)
    , TextScript(NULL)
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
    Backgrounds.Free();
    delete HotspotMask;
    delete RegionMask;
    delete WalkAreaMask;
    delete WalkBehindMask;
    HotspotMask = NULL;
    RegionMask = NULL;
    WalkAreaMask = NULL;
    WalkBehindMask = NULL;
    Hotspots.Free();
    Objects.Free();
    Regions.Free();
    WalkAreas.Free();
    WalkBehinds.Free();
    
    Password.Free();
    Messages.Free();
    MessageInfos.Free();
    
    EventHandlers.Free();
    LocalVariables.Free();
    
    if (TextScript)
    {
        free(TextScript);
        TextScript = NULL;
    }
    if (!CompiledScriptShared)
    {
        delete CompiledScript;
    }
    CompiledScript = NULL;
    CompiledScriptShared = false;
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
    LoadedVersion = kRoomVersion_Current;
    GameId = NO_GAME_ID_IN_ROOM_FILE;

    Width           = 320;
    Height          = 200;
    Edges.Left      = 0;
    Edges.Right     = 317;
    Edges.Top       = 40;
    Edges.Bottom    = 199;
    Resolution      = 1;
    BytesPerPixel   = 1;

    WalkBehindCount = 0;
    ObjectCount = 0;
    MessageCount    = 0;
    AnimationCount  = 0;
    WalkAreaCount   = 0;
    HotspotCount    = 0;
    RegionCount     = 0;
    BkgSceneCount   = 1;
    LocalVariableCount = 0;

    Backgrounds.New(1);
    BkgSceneAnimSpeed = 5;

    CompiledScriptShared = false;
    CompiledScriptSize = 0;

    memset(Palette, 0, sizeof(Palette));
    memset(Options, 0, sizeof(Options));

    // TODO: this is done for safety reasons;
    // should be reworked when all the code is altered to remove any
    // usage of MAX_* constants for room objects and regions.
    Hotspots.New(MAX_HOTSPOTS);
    Objects.New(MAX_INIT_SPR);
    Regions.New(MAX_REGIONS);    
    WalkAreas.New(MAX_WALK_AREAS + 1);
    WalkBehinds.New(MAX_OBJ);    
    
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        Hotspots[i].EventHandlers.Interaction = new NewInteraction();
    }
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        Objects[i].EventHandlers.Interaction = new NewInteraction();
    }    
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        Regions[i].EventHandlers.Interaction = new NewInteraction();
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
    for (int i = 0; i < WalkBehindCount; ++i)
    {
        WalkBehinds[i].Baseline = in->ReadInt16();;
    }

    HotspotCount = in->ReadInt32();
    // FIXME: check version!!!
    // Newer versions with dynamic limits should not do this,
    if (HotspotCount == 0)
    {
        HotspotCount = 20;
    }

    for (int i = 0; i < HotspotCount; ++i)
    {
        Hotspots[i].WalkToPoint.x = in->ReadInt16();
        Hotspots[i].WalkToPoint.y = in->ReadInt16();
    }

	for (int i = 0; i < HotspotCount; ++i)
	{
        if (LoadedVersion >= kRoomVersion_303a)
		{
            Hotspots[i].Name.Read(in, 2999);
		}
		else
		{
            Hotspots[i].Name.ReadCount(in, 30);
		}
	}

    if (LoadedVersion >= kRoomVersion_270)
    {
        for (int i = 0; i < HotspotCount; ++i)
	    {
            Hotspots[i].ScriptName.ReadCount(in, MAX_SCRIPT_NAME_LEN);
        }
    }
    
    WalkAreaCount = in->ReadInt32();
    for (int i = 0; i < WalkAreaCount; ++i)
    {
        WalkAreas[i].WallPoints.ReadFromFile(in);
    }
  
    update_polled_stuff_if_runtime();

    Edges.Top = in->ReadInt16();
    Edges.Bottom = in->ReadInt16();
    Edges.Left = in->ReadInt16();
    Edges.Right = in->ReadInt16();

    ObjectCount = in->ReadInt16();
    for (int i = 0; i < ObjectCount; ++i)
    {
        Objects[i].ReadFromFile(in);
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
                delete Hotspots[i].EventHandlers.Interaction;
                Hotspots[i].EventHandlers.Interaction = deserialize_new_interaction(in);
            }
            for (int i = 0; i < ObjectCount; ++i)
            {
                delete Objects[i].EventHandlers.Interaction;
                Objects[i].EventHandlers.Interaction = deserialize_new_interaction(in);
            }
            EventHandlers.Interaction = deserialize_new_interaction(in);
        } // LoadedVersion >= kRoomVersion_300a

        if (LoadedVersion >= kRoomVersion_255b)
        {
            RegionCount = in->ReadInt32();
            if (LoadedVersion < kRoomVersion_300a) 
            {
                for (int i = 0; i < RegionCount; ++i)
                {
                    delete Regions[i].EventHandlers.Interaction;
                    Regions[i].EventHandlers.Interaction = deserialize_new_interaction(in);
		        }
            }
        }

	    if (LoadedVersion >= kRoomVersion_300a)
        {
            EventHandlers.ScriptFnRef = new InteractionScripts();
	        deserialize_interaction_scripts(in, EventHandlers.ScriptFnRef);
            for (int i = 0; i < HotspotCount; ++i)
            {
                Hotspots[i].EventHandlers.ScriptFnRef = new InteractionScripts();
                deserialize_interaction_scripts(in, Hotspots[i].EventHandlers.ScriptFnRef);
            }
            for (int i = 0; i < ObjectCount; ++i)
            {
                Objects[i].EventHandlers.ScriptFnRef = new InteractionScripts();
                deserialize_interaction_scripts(in, Objects[i].EventHandlers.ScriptFnRef);
            }
            for (int i = 0; i < RegionCount; ++i)
            {
                Regions[i].EventHandlers.ScriptFnRef = new InteractionScripts();
                deserialize_interaction_scripts(in, Regions[i].EventHandlers.ScriptFnRef);
            }
	    } // LoadedVersion >= kRoomVersion_300a
    } // LoadedVersion >= kRoomVersion_241

    if (LoadedVersion >= kRoomVersion_200_alpha)
    {
        for (int i = 0; i < ObjectCount; ++i)
        {
            Objects[i].Baseline = in->ReadInt32();
        }
        Width = in->ReadInt16();
        Height = in->ReadInt16();
    }

    if (LoadedVersion >= kRoomVersion_262)
    {
        for (int i = 0; i < ObjectCount; ++i)
        {
            Objects[i].Flags = in->ReadInt16();
        }
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
        for (int i = 0; i < WalkAreaCount; ++i)
        {
            WalkAreas[i].Zoom = in->ReadInt16();
        }
    }

    if (LoadedVersion >= kRoomVersion_214)
    {
        for (int i = 0; i < WalkAreaCount; ++i)
        {
            WalkAreas[i].Light = in->ReadInt16();
        }
    }

    if (LoadedVersion >= kRoomVersion_251)
    {
        for (int i = 0; i < WalkAreaCount; ++i)
        {
            WalkAreas[i].Zoom2 = in->ReadInt16();
        }
        for (int i = 0; i < WalkAreaCount; ++i)
        {
            WalkAreas[i].Top = in->ReadInt16();
        }
        for (int i = 0; i < WalkAreaCount; ++i)
        {
            WalkAreas[i].Bottom = in->ReadInt16();
        }

        for (int i = 0; i < WalkAreaCount; ++i)
        {
            // if they set a contiuously scaled area where the top
            // and bottom zoom levels are identical, set it as a normal
            // scaled area
            if (WalkAreas[i].Zoom == WalkAreas[i].Zoom2)
            {
                WalkAreas[i].Zoom2 = NOT_VECTOR_SCALED;
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
        for (int i = 0; i < MAX_WALK_AREAS + 1; ++i)
        {
            WalkAreas[i].ShadingView = in->ReadInt16();
        }
    }

    if (LoadedVersion >= kRoomVersion_255b)
    {
        for (int i = 0; i < RegionCount; ++i)
        {
            Regions[i].Light = in->ReadInt16();
        }
        for (int i = 0; i < RegionCount; ++i)
        {
            Regions[i].Tint = in->ReadInt32();
        }
    }

    update_polled_stuff_if_runtime();

    // Background scenes array should already have one element at this point
    if (LoadedVersion >= kRoomVersion_pre114_5)
    {
        load_lzw(in, Backgrounds[0].Graphic, Palette);
        Backgrounds[0].Graphic = recalced;
    }
    else
    {
        loadcompressed_allegro(in, &Backgrounds[0].Graphic, Palette);
    }

    if (Backgrounds[0].Graphic->GetWidth() > 320 && LoadedVersion < kRoomVersion_200_final)
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
            RegionMask = BitmapHelper::CreateBitmapCopy(WalkAreaMask, 8);
            for (int i = 0; i < MAX_REGIONS; ++i)
            {
                Regions[i].Light = WalkAreas[i].Light;
            }
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
    TextScript = (char *)malloc(script_len + 5);
    in->Read(TextScript, script_len);
    TextScript[script_len] = 0;

    for (int i = 0; i < script_len; ++i)
    {
        TextScript[i] += passwencstring[i % 11];
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadObjectNamesBlock(Stream *in)
{
    // TODO: this will cause problems if object count > 255
    if (in->ReadByte() != ObjectCount)
    {
        return kRoomInfoErr_InconsistentDataForObjectNames;
    }
    for (int i = 0; i < ObjectCount; ++i)
    {
        Objects[i].Name.ReadCount(in, MAXOBJNAMELEN);
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadAnimBkgBlock(Stream *in)
{
    BkgSceneCount = in->ReadByte();
    BkgSceneAnimSpeed = in->ReadByte();

    Backgrounds.AppendCount(BkgSceneCount - 1);

    if (LoadedVersion >= kRoomVersion_255a)
    {
        for (int i = 0; i < BkgSceneCount; ++i)
        {
            Backgrounds[i].PaletteShared = in->ReadInt8();
        }
    }

    for (int i = 1; i < BkgSceneCount; ++i)
    {
        update_polled_stuff_if_runtime();
        load_lzw(in, Backgrounds[i].Graphic, Backgrounds[i].Palette);
        Backgrounds[i].Graphic = recalced;
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

    if (Properties.UnSerialize(in))
    {
        return kRoomInfoErr_PropertiesLoadFailed;
    }

    for (int i = 0; i < HotspotCount; ++i)
    {
        if (Hotspots[i].Properties.UnSerialize(in))
        {
            return kRoomInfoErr_PropertiesLoadFailed;
        }
    }
    for (int i = 0; i < ObjectCount; ++i)
    {
        if (Objects[i].Properties.UnSerialize(in))
        {
            return kRoomInfoErr_PropertiesLoadFailed;
        }
    }

    return kRoomInfoErr_NoError;
}

RoomInfoError RoomInfo::ReadObjectScriptNamesBlock(Stream *in)
{
    if (in->ReadByte() != ObjectCount)
    {
        return kRoomInfoErr_InconsistentDataForObjectScriptNames;
    }
    for (int i = 0; i < ObjectCount; ++i)
    {
        Objects[i].ScriptName.ReadCount(in, MAX_SCRIPT_NAME_LEN);
    }

    return kRoomInfoErr_NoError;
}

void RoomInfo::ProcessAfterRead(bool game_is_hires)
{
    // Synchronize background 0 palette with room.pal
    memcpy(&Backgrounds[0].Palette, &Palette, sizeof(color) * 256);

    if (LoadedVersion < kRoomVersion_303b && game_is_hires)
    {
        // Pre-3.0.3, multiply up co-ordinates
        // If you change this, also change convert_room_coordinates_to_low_res
        // function in the engine.
	    for (int i = 0; i < ObjectCount; ++i)
	    {
            Objects[i].X <<= 1;
            Objects[i].Y <<= 1;
            if (Objects[i].Baseline > 0)
            {
                Objects[i].Baseline <<= 1;
            }
        }

        for (int i = 0; i < HotspotCount; ++i)
        {
            Hotspots[i].WalkToPoint.x <<= 1;
            Hotspots[i].WalkToPoint.y <<= 1;
        }

        for (int i = 0; i < WalkBehindCount; ++i)
        {
            WalkBehinds[i].Baseline <<= 1;
        }

        Edges.Left    <<= 1;
        Edges.Top     <<= 1;
        Edges.Bottom  <<= 1;
        Edges.Right   <<= 1;
        Width       <<= 1;
        Height      <<= 1;
    }
}

void RoomInfo::WriteToFile(Stream *out)
{
}

} // namespace Common
} // namespace AGS


#include "ac/button.h"
#include "ac/character.h"
#include "ac/dialog.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/game.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/region.h"
#include "ac/richgamemedia.h"
#include "ac/room.h"
#include "ac/system.h"
#include "ac/timer.h"
#include "ac/viewport.h"
#include "ac/view.h"
#include "ac/dynobj/cc_serializer.h"
#include "debug/out.h"
#include "game/game_objects.h"
#include "game/savedgame.h"
#include "game/script_objects.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "main/main.h"
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "script/cc_error.h"
#include "script/script.h"
#include "util/alignedstream.h"
#include "util/filestream.h"

extern int our_eip;
extern int oldeip;

namespace AGS
{
namespace Engine
{

using Common::AlignedStream;
using Common::Bitmap;
using Common::Stream;
namespace BitmapHelper = Common::BitmapHelper;
namespace Out = Common::Out;

enum SavedGameChapter
{
    kSvgChapter_GameState_PlayStruct,
    kSvgChapter_GameState_Audio,
    kSvgChapter_GameState_Characters,
    kSvgChapter_GameState_Dialogs,
    kSvgChapter_GameState_GUI,
    kSvgChapter_GameState_InventoryItems,
    kSvgChapter_GameState_MouseCursors,
    kSvgChapter_GameState_Views,
    kSvgChapter_GameState_DynamicSprites,
    kSvgChapter_GameState_Overlays,
    kSvgChapter_GameState_DynamicSurfaces,
    kSvgChapter_GameState_ScriptModules,
    kSvgChapter_RoomStates_AllRooms,
    kSvgChapter_RoomStates_ThisRoom,
    kSvgChapter_ManagedPool,
    kSvgChapter_PluginData,

    kSvgChapter_FirstChapter = kSvgChapter_GameState_PlayStruct,
    kSvgChapter_LastChapter = kSvgChapter_PluginData
};

// A struct to temporarily keep some of the loaded data before related game objects
// are properly initialized.
struct SavedGameRestorationData
{
    int             DoAmbient[MAX_SOUND_CHANNELS];
    Bitmap          *DynamicallyCreatedSurfacesFromSaveGame[MAX_DYNAMIC_SURFACES];
    Array<Bitmap*>  BkgScenes;
    Array<char>     GlobalData;
    Array<char>     ScriptModuleData[MAX_SCRIPT_MODULES];
    Array<int16_t>  RegionLightLevels;
    Array<int32_t>  RegionTintLevels;
    Array<int16_t>  WalkAreaZoomLevels;
    Array<int16_t>  WalkAreaZoom2Levels;
    Array<MoveList> ObjMoveLists;
    Array<MoveList> CharMoveLists;
    int             MusicVolume;
    int             CursorMode;
    int             MouseCursor;
};

struct SavedGameChapterInfo
{
    SavedGameChapter    Id;
    const char          *Name;
    void                (*Serialize)  (Stream*);
    SavedGameError      (*Deserialize)(Stream*, SavedGameRestorationData&);
};

void save_game_data_ch_play_struct(Stream*);
void save_game_data_ch_audio(Stream*);
void save_game_data_ch_characters(Stream*);
void save_game_data_ch_dialogs(Stream*);
void save_game_data_ch_gui(Stream*);
void save_game_data_ch_invitems(Stream*);
void save_game_data_ch_mousecursors(Stream*);
void save_game_data_ch_views(Stream*);
void save_game_data_ch_dynamicsprites(Stream*);
void save_game_data_ch_overlays(Stream*);
void save_game_data_ch_dynamicsurfaces(Stream*);
void save_game_data_ch_script_modules(Stream*);
void save_game_data_ch_allrooms(Stream*);
void save_game_data_ch_thisroom(Stream*);
void save_game_data_ch_managed_pool(Stream*);
void save_game_data_ch_plugin_data(Stream*);
SavedGameError restore_game_data_ch_play_struct(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_audio(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_characters(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_dialogs(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_gui(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_invitems(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_mousecursors(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_views(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_dynamicsprites(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_overlays(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_dynamicsurfaces(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_script_modules(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_allrooms(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_thisroom(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_managed_pool(Stream*, SavedGameRestorationData&);
SavedGameError restore_game_data_ch_plugin_data(Stream*, SavedGameRestorationData&);

SavedGameChapterInfo SavedGameChapters[] =
{
    {
        kSvgChapter_GameState_PlayStruct, "Game State",
        save_game_data_ch_play_struct,
        restore_game_data_ch_play_struct
    },
    {
        kSvgChapter_GameState_Audio, "Audio",
        save_game_data_ch_audio,
        restore_game_data_ch_audio
    },
    {
        kSvgChapter_GameState_Characters, "Characters",
        save_game_data_ch_characters,
        restore_game_data_ch_characters
    },
    {
        kSvgChapter_GameState_Dialogs, "Dialogs",
        save_game_data_ch_dialogs,
        restore_game_data_ch_dialogs
    },
    {
        kSvgChapter_GameState_GUI, "GUI",
        save_game_data_ch_gui,
        restore_game_data_ch_gui
    },
    {
        kSvgChapter_GameState_InventoryItems, "Inventory Items",
        save_game_data_ch_invitems,
        restore_game_data_ch_invitems
    },
    {
        kSvgChapter_GameState_MouseCursors, "Mouse Cursors",
        save_game_data_ch_mousecursors,
        restore_game_data_ch_mousecursors
    },
    {
        kSvgChapter_GameState_Views, "Views",
        save_game_data_ch_views,
        restore_game_data_ch_views
    },
    {
        kSvgChapter_GameState_DynamicSprites, "Dynamic Sprites",
        save_game_data_ch_dynamicsprites,
        restore_game_data_ch_dynamicsprites
    },
    {
        kSvgChapter_GameState_Overlays, "Overlays",
        save_game_data_ch_overlays,
        restore_game_data_ch_overlays
    },
    {
        kSvgChapter_GameState_DynamicSurfaces, "Dynamic Surfaces",
        save_game_data_ch_dynamicsurfaces,
        restore_game_data_ch_dynamicsurfaces
    },
    {
        kSvgChapter_GameState_ScriptModules, "Script Modules",
        save_game_data_ch_script_modules,
        restore_game_data_ch_script_modules
    },
    {
        kSvgChapter_RoomStates_AllRooms, "Room States",
        save_game_data_ch_allrooms,
        restore_game_data_ch_allrooms
    },
    {
        kSvgChapter_RoomStates_ThisRoom, "Running Room State",
        save_game_data_ch_thisroom,
        restore_game_data_ch_thisroom
    },
    {
        kSvgChapter_ManagedPool, "",
        save_game_data_ch_managed_pool,
        restore_game_data_ch_managed_pool
    },
    {
        kSvgChapter_PluginData, "",
        save_game_data_ch_plugin_data,
        restore_game_data_ch_plugin_data
    }
};

//=============================================================================
//
// Helper structs
//
//=============================================================================

const char *SavedGameOpening::LegacySavedGameSignature = "Adventure Game Studio saved game";
const char *SavedGameOpening::SavedGameSignature       = "Adventure Game Studio Saved Game";
const size_t SavedGameOpening::SavedGameSigLength      = 32;
const size_t LegacyDescriptionLength                   = 180;

const char *SavedGameErrorText[kSavedGameErrorNumber] = {
    // kSvgErr_NoError
    "No error",
    // kSvgErr_FileNotFound
    "File not found",
    // kSvgErr_SignatureFailed
    "Not an AGS saved game or unsupported format",
    // kSvgErr_FormatVersionNotSupported
    "Saved game format not supported",
    // kSvgErr_IncompatibleEngine
    "Saved game was written by incompatible interpreter",
    // kSvgErr_GameGuidFailed
    "Game GUID does not match, saved by different game",
    // kSvgErr_DifferentMainDataFile
    "Main game file does not match, probably saved by different game",
    // kSvgErr_InconsistentFormat
    "Inconsistent format or file corrupted",
    // kSvgErr_DataVersionNotSupported
    "Game data version not supported",
    // kSvgErr_GameContentAssertionFailed
    "Saved content does not match current game",
    // kSvgErr_LegacyDifferentResolution
    "Saved with different display resolution",
    // kSvgErr_LegacyDifferentColorDepth
    "Saved with different colour depth",
    // kSvgErr_GameObjectInitFailed
    "Game object initialization failed"
};

SavedGameOpening::SavedGameOpening()
    : InputStream(NULL)
{
}

SavedGameOpening::~SavedGameOpening()
{
    delete InputStream;
}

void SavedGameOpening::Clear()
{
    Filename.Empty();
    Version = kSvgVersion_Undefined;
    delete InputStream;
    InputStream = NULL;
}

void SavedGameInfo::ReadFromFile(Stream *in)
{
    EngineName = String::FromStream(in);
    EngineVersion = String::FromStream(in);
    Guid = String::FromStream(in);
    GameTitle = String::FromStream(in);
    MainDataFilename = String::FromStream(in);
}

void SavedGameInfo::WriteToFile(Stream *out)
{
    EngineName.Write(out);
    EngineVersion.Write(out);
    Guid.Write(out);
    GameTitle.Write(out);
    MainDataFilename.Write(out);
}

const int32_t SvgChapterOpenSig  = 0xABCDEFFF;
const int32_t SvgChapterCloseSig = 0xFEDCBAAA;

//=============================================================================
//
// Serialization
//
//=============================================================================

void save_game_chapter(Stream *out, int index)
{
    out->WriteInt32(SvgChapterOpenSig);
    SavedGameChapters[index].Serialize(out);
    out->WriteInt32(SvgChapterCloseSig);
}

void save_game_data_ch_play_struct(Stream *out)
{
    out->WriteInt32(kRtGameVersion_Current);

    out->WriteInt32(final_col_dep);
    // Game base
    game.WriteForSavedGame(out);
    // play struct
    play.WriteToSavedGame(out);

    // Global variables
    out->WriteInt32 (numGlobalVars);
    for (int i = 0; i < numGlobalVars; ++i)
    {
        globalvars[i].WriteToFile(out);
    }

    out->WriteInt32(frames_per_second);
    out->WriteInt32(loopcounter);
    out->WriteInt32(ifacepopped);
    out->WriteInt32(game_paused);
    // Mouse cursor
    out->WriteInt32(cur_mode);
    out->WriteInt32(cur_cursor);
    out->WriteInt32(mouse_on_iface);
    // Viewport
    out->WriteInt32(offsetx);
    out->WriteInt32(offsety);

    // DoOnce tokens
    out->WriteInt32(play.DoOnceTokenCount);
    for (int i = 0; i < play.DoOnceTokenCount; ++i)
    {
        play.DoOnceTokens[i].Write(out);
    }

    // Game palette
    // TODO: probably no need to save this for hi/true-res game
    out->WriteArray(palette, sizeof(color), 256);
}

void save_game_data_ch_audio(Stream *out)
{
    // Audio clip types
    // CHECKME: what is it for in save game?
    out->WriteInt32(kRtAudioVersion_Current);
    out->WriteInt32(game.AudioClipTypeCount);
    out->WriteInt32(game.AudioClipCount);
    for (int i = 0; i < game.AudioClipTypeCount; ++i)
    {
        game.AudioClipTypes[i].WriteToFile(out);
    }
    out->WriteInt32(MAX_SOUND_CHANNELS);
    for (int i = 0; i < MAX_SOUND_CHANNELS; ++i)
    {
        ambient[i].WriteToFile(out);
    }

    // Audio clips and crossfade
    out->WriteInt32(MAX_SOUND_CHANNELS);
    for (int i = 0; i < MAX_SOUND_CHANNELS; ++i)
    {
        if ((channels[i] != NULL) && (channels[i]->done == 0) && (channels[i]->sourceClip != NULL))
        {
            out->WriteInt32(((ScriptAudioClip*)channels[i]->sourceClip)->id);
            out->WriteInt32(channels[i]->get_pos());
            out->WriteInt32(channels[i]->priority);
            out->WriteInt32(channels[i]->repeat ? 1 : 0);
            out->WriteInt32(channels[i]->vol);
            out->WriteInt32(channels[i]->panning);
            out->WriteInt32(channels[i]->volAsPercentage);
            out->WriteInt32(channels[i]->panningAsPercentage);
        }
        else
        {
            out->WriteInt32(-1);
        }
    }
    out->WriteInt32(crossFading);
    out->WriteInt32(crossFadeVolumePerStep);
    out->WriteInt32(crossFadeStep);
    out->WriteInt32(crossFadeVolumeAtStart);
    // CHECKME: why this needs to be saved?
    out->WriteInt32(current_music_type);
}

void save_game_data_ch_characters(Stream *out)
{
    out->WriteInt32(kRtCharacterVersion_Current);
    out->WriteInt32(game.CharacterCount);
    for (int i = 0; i < game.CharacterCount; ++i)
    {
        game.Characters[i].WriteToSavedGame(out);
        charextra[i].WriteToSavedGame(out);
    }
    if (game.CharacterInteractionScripts.IsEmpty())
    {
        for (int i = 0; i < game.CharacterCount; ++i)
        {
            out->WriteArrayOfInt32(&game.CharacterInteractions[i]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
        }
    }
}

void save_game_data_ch_dialogs(Stream *out)
{
    // Dialogs state
    out->WriteInt32(kRtDialogVersion_Current);
    out->WriteInt32(game.DialogCount);
    for (int i = 0; i < game.DialogCount; ++i)
    {
        out->WriteArrayOfInt32(dialog[i].optionflags, MAXTOPICOPTIONS);
    }
}

void save_game_data_ch_gui(Stream *out)
{
    // Gui state
    out->WriteInt32(kRtGUIVersion_Current);
    out->WriteInt32(game.GuiCount);
    write_gui_for_savedgame(out,guis,&game);
    // Gui draw order
    out->WriteArrayOfInt32(play.GuiDrawOrder.GetCArr(), game.GuiCount);
    out->WriteInt32(numAnimButs);
    for (int i = 0; i < numAnimButs; ++i)
    {
        animbuts[i].WriteToFile(out);
    }
}

void save_game_data_ch_invitems(Stream *out)
{
    out->WriteInt32(kRtInvItemVersion_Current);
    out->WriteInt32(game.InvItemCount);
    for (int i = 0; i < game.InvItemCount; ++i)
    {
        game.InventoryItems[i].WriteToSavedGame(out);
    }
    if (game.InvItemInteractionScripts.IsEmpty())
    {
        for (int i = 0; i < game.InvItemCount; ++i)
        {
            out->WriteArrayOfInt32(&game.InvItemInteractions[i]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
        }
    }
}

void save_game_data_ch_mousecursors(Stream *out)
{
    out->WriteInt32(kRtMouseCursorVersion_Current);
    out->WriteInt32(game.MouseCursorCount);
    for (int i = 0; i < game.MouseCursorCount; ++i)
    {
        game.MouseCursors[i].WriteToFile(out);
    }
}

void save_game_data_ch_views(Stream *out)
{
    // View states
    out->WriteInt32(kRtViewVersion_Current);
    out->WriteInt32(game.ViewCount);
    for (int i = 0; i < game.ViewCount; ++i)
    {
        for (int j = 0; j < views[i].numLoops; ++j)
        {
            for (int k = 0; k < views[i].loops[j].numFrames; ++k)
            {
                out->WriteInt32(views[i].loops[j].frames[k].sound);
                out->WriteInt32(views[i].loops[j].frames[k].pic);
            }
        }
    }
}

void save_game_data_ch_dynamicsprites(Stream *out)
{
    // Dynamic sprites
    out->WriteInt32(kRtDynamicSpriteVersion_Current);
    out->WriteInt32(spriteset.elements);
    for (int i = 1; i < spriteset.elements; ++i) {
        if (game.SpriteFlags[i] & SPF_DYNAMICALLOC) {
            out->WriteInt32(i);
            out->WriteInt8(game.SpriteFlags[i]);
            serialize_bitmap(spriteset[i], out);
        }
    }
    // end of dynamic sprite list
    out->WriteInt32(0);
}

void save_game_data_ch_overlays(Stream *out)
{
    // Active overlays
    out->WriteInt32(kRtOverlayVersion_Current);
    out->WriteInt32(numscreenover);
    for (int i = 0; i < numscreenover; ++i)
    {
        screenover[i].WriteToFile(out);
    }
    // CHECKME: why saving the bitmap?
    // isn't overlay using one of the existing sprites from the sprite set?
    for (int i = 0; i < numscreenover; ++i)
    {
        serialize_bitmap(screenover[i].pic, out);
    }
}

void save_game_data_ch_dynamicsurfaces(Stream *out)
{
    // Dynamic surfaces
    out->WriteInt32(kRtDynamicSurfaceVersion_Current);
    out->WriteInt32(MAX_DYNAMIC_SURFACES);
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        if (dynamicallyCreatedSurfaces[i] == NULL)
        {
            out->WriteInt8(0);
        }
        else
        {
            out->WriteInt8(1);
            serialize_bitmap(dynamicallyCreatedSurfaces[i], out);
        }
    }
}

void save_game_data_ch_script_modules(Stream *out)
{
    out->WriteInt32(kRtScriptModuleVersion_Current);
    // write the data segment of the global script
    int gdatasize = gameinst->globaldatasize;
    out->WriteInt32(gdatasize);
    if (gdatasize > 0)
    {
        out->Write(gameinst->globaldata, gdatasize);
    }
    // write the script modules data segments
    out->WriteInt32(numScriptModules);
    for (int i = 0; i < numScriptModules; ++i)
    {
        int glsize = moduleInst[i]->globaldatasize;
        out->WriteInt32(glsize);
        if (glsize > 0)
        {
            out->Write(moduleInst[i]->globaldata, glsize);
        }
    }
}

void save_game_data_ch_allrooms(Stream *out)
{
    out->WriteInt32(kRtRoomStateVersion_Current);
    // write the room state for all the rooms the player has been in
    int room_state_valid_count = GetRoomStateCount();
    out->WriteInt32(room_state_valid_count);
    for (int i = 0; i < room_statuses.GetCount(); ++i)
    {
        if (IsRoomStateValid(i))
        {
            RoomState* roomstat = GetRoomState(i);
            if (roomstat->BeenHere)
            {
                out->WriteInt32(i);
                roomstat->WriteToSavedGame(out);
            }
            else
            {
                out->WriteInt32(-1);
            }
        }
    }
}

void save_game_data_ch_thisroom(Stream *out)
{
    // Current player's room
    out->WriteInt32(displayed_room);
    if (displayed_room < 0)
    {
        return;
    }

    out->WriteInt32(kRtRunningRoomStateVersion_Current);

    out->WriteBool(thisroom.IsPersistent);

    out->WriteInt32(thisroom.Backgrounds.GetCount());
    for (int i = 0; i < thisroom.Backgrounds.GetCount(); ++i)
    {
        if (play.RoomBkgWasModified[i])
        {
            serialize_bitmap(thisroom.Backgrounds[i].Graphic, out);
        }
    }

    out->WriteInt32(thisroom.Regions.GetCount());
    for (int i = 0; i < thisroom.Regions.GetCount(); ++i)
    {
        out->WriteInt16(thisroom.Regions[i].Light);
        out->WriteInt32(thisroom.Regions[i].Tint);
    }
    out->WriteInt32(thisroom.WalkAreas.GetCount());
    for (int i = 0; i < thisroom.WalkAreas.GetCount(); ++i)
    {
        out->WriteInt16(thisroom.WalkAreas[i].Zoom);
        out->WriteInt16(thisroom.WalkAreas[i].Zoom2);
    }

    out->WriteInt32((raw_saved_screen == NULL) ? 0 : 1);
    if (raw_saved_screen)
    {
        serialize_bitmap (raw_saved_screen, out);
    }
    
    out->WriteInt32(CharMoveLists.GetCount());
    for (int i = 0; i < CharMoveLists.GetCount(); ++i)
    {
        CharMoveLists[i].WriteToFile(out);
    }
    out->WriteInt32(ObjMoveLists.GetCount());
    for (int i = 0; i < ObjMoveLists.GetCount(); ++i)
    {
        ObjMoveLists[i].WriteToFile(out);
    }    

    // save the room music volume
    out->WriteInt32(thisroom.Options[kRoomBaseOpt_MusicVolume]);

    // if current room is not persistent, save the temporary room status
    if (!thisroom.IsPersistent)
    {
        troom.WriteToSavedGame(out);
    }
}

void save_game_data_ch_managed_pool(Stream *out)
{
    out->WriteInt32(kRtManagedPoolVersion_Current);
    out->WriteInt32(scrHotspot.GetCount());
    out->WriteInt32(scrObj.GetCount());
    out->WriteInt32(scrRegion.GetCount());
    ccSerializeAllObjects(out);
}

void save_game_data_ch_plugin_data(Stream *out)
{
    out->WriteInt32(kRtPluginStateVersion_Current);
    // [IKM] Plugins expect FILE pointer! // TODO something with this later...
    platform->RunPluginHooks(AGSE_SAVEGAME, (long)((Common::FileStream*)out)->GetHandle());
}

// TODO: put this elsewhere
void do_before_save()
{
    if (play.CurrentMusicIndex >= 0)
    {
        if (IsMusicPlaying() == 0)
        {
            play.CurrentMusicIndex = -1;
        }
    }

    if (displayed_room >= 0)
    {
        // update the current room script's data segment copy
        if (roominst)
        {
            save_room_data_segment();
        }

        // Update the saved interaction variable values
        for (int i = 0; i < thisroom.LocalVariableCount; ++i)
        {
            croom->InteractionVariableValues[i] = thisroom.LocalVariables[i].value;
        }
    }
}

void SaveGameData(Stream *out)
{
    do_before_save();

    // Game state
    save_game_chapter(out, kSvgChapter_GameState_PlayStruct);
    save_game_chapter(out, kSvgChapter_GameState_Audio);
    save_game_chapter(out, kSvgChapter_GameState_Characters);
    save_game_chapter(out, kSvgChapter_GameState_Dialogs);
    save_game_chapter(out, kSvgChapter_GameState_GUI);
    save_game_chapter(out, kSvgChapter_GameState_InventoryItems);
    save_game_chapter(out, kSvgChapter_GameState_MouseCursors);
    save_game_chapter(out, kSvgChapter_GameState_Views);
    save_game_chapter(out, kSvgChapter_GameState_DynamicSprites);
    save_game_chapter(out, kSvgChapter_GameState_Overlays);
    save_game_chapter(out, kSvgChapter_GameState_DynamicSurfaces);
    save_game_chapter(out, kSvgChapter_GameState_ScriptModules);
    // Room states
    save_game_chapter(out, kSvgChapter_RoomStates_AllRooms);
    save_game_chapter(out, kSvgChapter_RoomStates_ThisRoom);

    save_game_chapter(out, kSvgChapter_ManagedPool);
    save_game_chapter(out, kSvgChapter_PluginData);
}

//=============================================================================
//
// Deserialization
//
//=============================================================================

void do_before_restore()
{
    unload_old_room();
    remove_screen_overlay(-1);
    is_complete_overlay = 0;
    is_text_overlay = 0;

    for (int i = 1; i < spriteset.elements; i++)
    {
        if (game.SpriteFlags[i] & SPF_DYNAMICALLOC)
        {
            // do this early, so that it changing guibuts doesn't
            // affect the restored data
            free_dynamic_sprite(i);
        }
    }

    for (int i = 0; i < game.GuiCount; ++i)
    {
        delete guibg[i];
        guibg[i] = NULL;

        if (guibgbmp[i])
        {
            gfxDriver->DestroyDDB(guibgbmp[i]);
        }
        guibgbmp[i] = NULL;
    }

    delete gameinstFork;
    delete gameinst;
    gameinstFork = NULL;
    gameinst = NULL;
    for (int i = 0; i < numScriptModules; ++i)
    {
        delete moduleInstFork[i];
        delete moduleInst[i];
        moduleInst[i] = NULL;
    }

    if (dialogScriptsInst != NULL)
    {
        delete dialogScriptsInst;
        dialogScriptsInst = NULL;
    }

    ResetRoomStates();

    free_do_once_tokens();

    for (int i = 0; i < game.GuiCount; ++i)
    {
        unexport_gui_controls(i);
    }

    //int crossfadeInChannelWas = play.CrossfadingInChannel;
    //int crossfadeOutChannelWas = play.CrossfadingOutChannel;
    for (int i = 0; i < MAX_SOUND_CHANNELS; i++)
    {
        stop_and_destroy_channel_ex(i, false);
    }
    //play.CrossfadingInChannel = crossfadeInChannelWas;
    //play.CrossfadingOutChannel = crossfadeOutChannelWas;

    clear_music_cache();
}

SavedGameError restore_game_chapter(Stream *in, int index, SavedGameRestorationData &restore_data)
{
    if (in->ReadInt32() != SvgChapterOpenSig)
    {
        Out::FPrint("Restore game error: mismatching opening signature at chapter %d, '%s'", index + 1, SavedGameChapters[index].Name);
        return kSvgErr_InconsistentFormat;
    }
    SavedGameError err = SavedGameChapters[index].Deserialize(in, restore_data);
    if (err != kSvgErr_NoError)
    {
        Out::FPrint("Restore game error: failed to read chapter %d, '%s'", index + 1, SavedGameChapters[index].Name);
        return err;
    }
    if (in->ReadInt32() != SvgChapterCloseSig)
    {
        Out::FPrint("Restore game error: mismatching closing signature at chapter %d, '%s'", index + 1, SavedGameChapters[index].Name);
        return kSvgErr_InconsistentFormat;
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_play_struct(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeGameVersion version = (RuntimeGameVersion)in->ReadInt32();
    if (version != kRtGameVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }

    if (in->ReadInt32() != final_col_dep)
    {
        return kSvgErr_LegacyDifferentColorDepth;
    }

    // Game base
    game.ReadFromSavedGame(in);
    int musicvox = play.UseSeparateMusicLib;
    int speech_was = play.SpeechVoiceMode;
    // play struct
    play.ReadFromSavedGame(in);

    // Preserve whether the music vox is available
    play.UseSeparateMusicLib = musicvox;
    // If they had the vox when they saved it, but they don't now
    if ((speech_was < 0) && (play.SpeechVoiceMode >= 0))
        play.SpeechVoiceMode = (-play.SpeechVoiceMode) - 1;
    // If they didn't have the vox before, but now they do
    else if ((speech_was >= 0) && (play.SpeechVoiceMode < 0))
        play.SpeechVoiceMode = (-play.SpeechVoiceMode) - 1;

    // Global variables
    if (in->ReadInt32() != numGlobalVars)
    {
        Out::FPrint("Restore game error: mismatching number of Global Variables");
        return kSvgErr_GameContentAssertionFailed;
    }
    for (int i = 0; i < numGlobalVars; ++i)
    {
        globalvars[i].ReadFromFile(in);
    }

    set_game_speed(in->ReadInt32());
    loopcounter = in->ReadInt32();
    ifacepopped = in->ReadInt32();
    game_paused = in->ReadInt32();
    // Mouse cursor
    restore_data.CursorMode = in->ReadInt32();
    restore_data.MouseCursor = in->ReadInt32();
    mouse_on_iface = in->ReadInt32();
    // Viewport
    offsetx = in->ReadInt32();
    offsety = in->ReadInt32();

    // DoOnce tokens
    play.DoOnceTokenCount = in->ReadInt32();
    play.DoOnceTokens.SetLength(play.DoOnceTokenCount);
    for (int i = 0; i < play.DoOnceTokenCount; ++i)
    {
        play.DoOnceTokens[i].Read(in);
    }

    // Game palette
    // TODO: probably no need to save this for hi/true-res game
    in->ReadArray(palette, sizeof(color), 256);
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_audio(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeAudioVersion version = (RuntimeAudioVersion)in->ReadInt32();
    if (version != kRtAudioVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != game.AudioClipTypeCount)
    {
        Out::FPrint("Restore game error: mismatching number of Audio Clip Types");
        return kSvgErr_GameContentAssertionFailed;
    }
    if (in->ReadInt32() != game.AudioClipCount)
    {
        Out::FPrint("Restore game error: mismatching number of Audio Clips");
        return kSvgErr_GameContentAssertionFailed;
    }
    
    for (int i = 0; i < game.AudioClipTypeCount; ++i)
    {
        game.AudioClipTypes[i].ReadFromFile(in);
    }
    int ambient_count = in->ReadInt32();
    for (int i = 0; i < ambient_count; ++i)
    {
        ambient[i].ReadFromFile(in);
    }

    int sound_channel_count = in->ReadInt32();
    for (int i = 1; i < sound_channel_count; i++)
    {
        if (ambient[i].channel == 0)
            restore_data.DoAmbient[i] = 0;
        else {
            restore_data.DoAmbient[i] = ambient[i].num;
            ambient[i].channel = 0;
        }
    }

    // Audio clips and crossfade
    int crossfadeInChannelWas = play.CrossfadingInChannel;
    int crossfadeOutChannelWas = play.CrossfadingOutChannel;
    play.CrossfadingInChannel = 0;
    play.CrossfadingOutChannel = 0;
    Array<int> channelPositions;
    channelPositions.New(sound_channel_count);
    for (int i = 0; i < sound_channel_count; ++i)
    {
        channelPositions[i] = 0;
        int audioClipIndex = in->ReadInt32();
        if (audioClipIndex >= 0)
        {
            if (audioClipIndex >= game.AudioClipCount)
            {
                Out::FPrint("Restore game error: invalid audio clip index");
                return kSvgErr_GameObjectInitFailed;
            }

            channelPositions[i] = in->ReadInt32();
            if (channelPositions[i] < 0) channelPositions[i] = 0;
            int priority = in->ReadInt32();
            int repeat = in->ReadInt32();
            int vol = in->ReadInt32();
            int pan = in->ReadInt32();
            int volAsPercent = in->ReadInt32();
            int panAsPercent = in->ReadInt32();
            play_audio_clip_on_channel(i, &game.AudioClips[audioClipIndex], priority, repeat, channelPositions[i]);
            if (channels[i] != NULL)
            {
                channels[i]->set_panning(pan);
                channels[i]->set_volume(vol);
                channels[i]->panningAsPercentage = panAsPercent;
                channels[i]->volAsPercentage = volAsPercent;
            }
        }
    }
    if ((crossfadeInChannelWas > 0) && (channels[crossfadeInChannelWas] != NULL))
        play.CrossfadingInChannel = crossfadeInChannelWas;
    if ((crossfadeOutChannelWas > 0) && (channels[crossfadeOutChannelWas] != NULL))
        play.CrossfadingOutChannel = crossfadeOutChannelWas;

    // If there were synced audio tracks, the time taken to load in the
    // different channels will have thrown them out of sync, so re-time it
    for (int i = 0; i < sound_channel_count; i++)
    {
        if ((channelPositions[i] > 0) && (channels[i] != NULL) && (channels[i]->done == 0))
        {
            channels[i]->seek(channelPositions[i]);
        }
    }
    crossFading = in->ReadInt32();
    crossFadeVolumePerStep = in->ReadInt32();
    crossFadeStep = in->ReadInt32();
    crossFadeVolumeAtStart = in->ReadInt32();
    // CHECKME: why this needs to be saved?
    current_music_type = in->ReadInt32();
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_characters(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeCharacterVersion version = (RuntimeCharacterVersion)in->ReadInt32();
    if (version != kRtCharacterVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != game.CharacterCount)
    {
        Out::FPrint("Restore game error: mismatching number of Characters");
        return kSvgErr_GameContentAssertionFailed;
    }

    for (int i = 0; i < game.CharacterCount; ++i)
    {
        game.Characters[i].ReadFromSavedGame(in);
        charextra[i].ReadFromSavedGame(in);
    }
    if (game.CharacterInteractionScripts.IsEmpty())
    {
        for (int i = 0; i < game.CharacterCount; ++i)
        {
            in->ReadArrayOfInt32(&game.CharacterInteractions[i]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
        }
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_dialogs(Stream *in, SavedGameRestorationData &restore_data)
{
    // Dialogs state
    RuntimeDialogVersion version = (RuntimeDialogVersion)in->ReadInt32();
    if (version != kRtDialogVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != game.DialogCount)
    {
        Out::FPrint("Restore game error: mismatching number of Dialogs");
        return kSvgErr_GameContentAssertionFailed;
    }

    for (int i = 0; i < game.DialogCount; ++i)
    {
        in->ReadArrayOfInt32(dialog[i].optionflags, MAXTOPICOPTIONS);
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_gui(Stream *in, SavedGameRestorationData &restore_data)
{
    // Gui state
    RuntimeGUIVersion version = (RuntimeGUIVersion)in->ReadInt32();
    if (version != kRtGUIVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != game.GuiCount)
    {
        Out::FPrint("Restore game error: mismatching number of GUI");
        return kSvgErr_GameContentAssertionFailed;
    }

    read_gui_from_savedgame(in, version, guis, &game);
    // Gui draw order
    play.GuiDrawOrder.ReadRawOver(in, game.GuiCount);
    numAnimButs = in->ReadInt32();
    for (int i = 0; i < numAnimButs; ++i)
    {
        animbuts[i].ReadFromFile(in);
    }

    for (int i = 0; i < game.GuiCount; ++i)
    {
        export_gui_controls(i);
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_invitems(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeInvItemVersion version = (RuntimeInvItemVersion)in->ReadInt32();
    if (version != kRtInvItemVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != game.InvItemCount)
    {
        Out::FPrint("Restore game error: mismatching number of Inventory Items");
        return kSvgErr_GameContentAssertionFailed;
    }

    for (int i = 0; i < game.InvItemCount; ++i)
    {
        game.InventoryItems[i].ReadFromSavedGame(in);
    }
    if (game.InvItemInteractionScripts.IsEmpty())
    {
        for (int i = 0; i < game.InvItemCount; ++i)
        {
            in->ReadArrayOfInt32 (&game.InvItemInteractions[i]->timesRun[0], MAX_NEWINTERACTION_EVENTS);
        }
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_mousecursors(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeMouseCursorVersion version = (RuntimeMouseCursorVersion)in->ReadInt32();
    if (version != kRtMouseCursorVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != game.MouseCursorCount)
    {
        Out::FPrint("Restore game error: mismatching number of Mouse Cursors");
        return kSvgErr_GameContentAssertionFailed;
    }

    for (int i = 0; i < game.MouseCursorCount; ++i)
    {
        game.MouseCursors[i].ReadFromFile(in);
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_views(Stream *in, SavedGameRestorationData &restore_data)
{
    // View states
    RuntimeViewVersion version = (RuntimeViewVersion)in->ReadInt32();
    if (version != kRtViewVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != game.ViewCount)
    {
        Out::FPrint("Restore game error: mismatching number of Views");
        return kSvgErr_GameContentAssertionFailed;
    }

    for (int i = 0; i < game.ViewCount; ++i)
    {
        for (int j = 0; j < views[i].numLoops; ++j)
        {
            for (int k = 0; k < views[i].loops[j].numFrames; ++k)
            {
                views[i].loops[j].frames[k].sound = in->ReadInt32();
                views[i].loops[j].frames[k].pic = in->ReadInt32();
            }
        }
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_dynamicsprites(Stream *in, SavedGameRestorationData &restore_data)
{
    // Dynamic sprites
    RuntimeDynamicSpriteVersion version = (RuntimeDynamicSpriteVersion)in->ReadInt32();
    if (version != kRtDynamicSpriteVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    // ensure the sprite set is at least as large as it was
    // when the game was saved
    spriteset.enlargeTo(in->ReadInt32());
    // get serialized dynamic sprites
    int sprnum = in->ReadInt32();
    while (sprnum)
    {
        unsigned char spriteflag = in->ReadByte();
        add_dynamic_sprite(sprnum, read_serialized_bitmap(in));
        game.SpriteFlags[sprnum] = spriteflag;
        sprnum = in->ReadInt32();
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_overlays(Stream *in, SavedGameRestorationData &restore_data)
{
    // Active overlays
    RuntimeOverlayVersion version = (RuntimeOverlayVersion)in->ReadInt32();
    if (version != kRtOverlayVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }

    numscreenover = in->ReadInt32();
    for (int i = 0; i < numscreenover; ++i)
    {
        screenover[i].ReadFromFile(in);
    }

    for (int i=0;i<numscreenover;i++)
    {
        if (screenover[i].pic != NULL)
        {
            screenover[i].pic = read_serialized_bitmap(in);
            screenover[i].bmp = gfxDriver->CreateDDBFromBitmap(screenover[i].pic, false);
        }
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_dynamicsurfaces(Stream *in, SavedGameRestorationData &restore_data)
{
    // Dynamic surfaces
    RuntimeDynamicSurfaceVersion version = (RuntimeDynamicSurfaceVersion)in->ReadInt32();
    if (version != kRtDynamicSurfaceVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    int dynsurface_count = in->ReadInt32();
    // load into a temp array since ccUnserialiseObjects will destroy
    // it otherwise
    for (int i = 0; i < dynsurface_count; i++)
    {
        if (in->ReadInt8() == 0)
        {
            restore_data.DynamicallyCreatedSurfacesFromSaveGame[i] = NULL;
        }
        else
        {
            restore_data.DynamicallyCreatedSurfacesFromSaveGame[i] = read_serialized_bitmap(in);
        }
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_script_modules(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeScriptModuleVersion version = (RuntimeScriptModuleVersion)in->ReadInt32();
    if (version != kRtScriptModuleVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    if (in->ReadInt32() != gamescript->globaldatasize)
    {
        Out::FPrint("Restore game error: mismatching size of global script data");
        return kSvgErr_GameContentAssertionFailed;
    }
    // read the global script data segment
    restore_data.GlobalData.ReadRaw(in, gamescript->globaldatasize);

    if (in->ReadInt32() != numScriptModules)
    {
        Out::FPrint("Restore game error: mismatching number of Script Modules");
        return kSvgErr_GameContentAssertionFailed;
    }
    for (int i = 0; i < numScriptModules; ++i)
    {
        if (in->ReadInt32() != scriptModules[i]->globaldatasize)
        {
            Out::FPrint("Restore game error: mismatching size of script module data, module %d", i);
            return kSvgErr_GameContentAssertionFailed;
        }
        restore_data.ScriptModuleData[i].ReadRaw(in, scriptModules[i]->globaldatasize);
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_allrooms(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeRoomStateVersion version = (RuntimeRoomStateVersion)in->ReadInt32();
    if (version != kRtRoomStateVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }

    int room_state_valid_count = in->ReadInt32();
    // read the room state for all the rooms the player has been in
    for (; room_state_valid_count > 0; --room_state_valid_count)
    {
        int room_index = in->ReadInt32();
        if (room_index >= 0)
        {
            RoomState* room_state = GetRoomState(room_index);
            room_state->ReadFromSavedGame(in);
        }
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_thisroom(Stream *in, SavedGameRestorationData &restore_data)
{
    displayed_room = in->ReadInt32();
    if (displayed_room < 0)
    {
        return kSvgErr_NoError;
    }

    RuntimeRunningRoomStateVersion version = (RuntimeRunningRoomStateVersion)in->ReadInt32();
    if (version != kRtRunningRoomStateVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }

    bool is_room_persistent = in->ReadBool();

    int bkg_scene_count = in->ReadInt32();
    restore_data.BkgScenes.New(bkg_scene_count);
    for (int i = 0; i < bkg_scene_count; ++i)
    {
        restore_data.BkgScenes[i] = NULL;
        if (play.RoomBkgWasModified[i])
        {
            restore_data.BkgScenes[i] = read_serialized_bitmap (in);
        }
    }

    int room_region_count = in->ReadInt32();
    restore_data.RegionLightLevels.New(room_region_count);
    restore_data.RegionTintLevels.New(room_region_count);
    for (int i = 0; i < room_region_count; ++i)
    {
        restore_data.RegionLightLevels[i] = in->ReadInt16();
        restore_data.RegionTintLevels[i] = in->ReadInt32();
    }
    int room_walkarea_count = in->ReadInt32();
    restore_data.WalkAreaZoomLevels.New(room_walkarea_count);
    restore_data.WalkAreaZoom2Levels.New(room_walkarea_count);
    for (int i = 0; i < room_walkarea_count; ++i)
    {
        restore_data.WalkAreaZoomLevels[i] = in->ReadInt16();
        restore_data.WalkAreaZoom2Levels[i] = in->ReadInt16();
    }

    delete raw_saved_screen;
    raw_saved_screen = NULL;
    if (in->ReadInt32() != 0)
    {
        raw_saved_screen = read_serialized_bitmap(in);
    }

    int char_movlist_count = in->ReadInt32();
    restore_data.CharMoveLists.New(char_movlist_count);
    for (int i = 0; i < char_movlist_count; ++i)
    {
        restore_data.CharMoveLists[i].ReadFromFile(in);
    }
    int obj_movlist_count = in->ReadInt32();
    restore_data.ObjMoveLists.New(obj_movlist_count);
    for (int i = 0; i < obj_movlist_count; ++i)
    {
        restore_data.ObjMoveLists[i].ReadFromFile(in);
    }

    restore_data.MusicVolume = in->ReadInt32();

    // save the current troom, in case they save in room 600 or whatever
    if (!is_room_persistent)
    {
        troom.ReadFromSavedGame(in);
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_managed_pool(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimeManagedPoolVersion version = (RuntimeManagedPoolVersion)in->ReadInt32();
    if (version != kRtManagedPoolVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }

    // Allocate needed size before unserializing managed objects.
    // The script object arrays cannot be _enlarged_ while objects are
    // registered at this point, because that may cause reallocation and change
    // of their position in memory; this will require different way to store them
    // and/or managed pool redesign.
    scrHotspot.SetLength(in->ReadInt32());
    scrObj.SetLength(in->ReadInt32());
    scrRegion.SetLength(in->ReadInt32());

    if (ccUnserializeAllObjects(in, &ccUnserializer))
    {
        Out::FPrint("Restore game error: managed object deserialization failed: %s", ccErrorString);
        return kSvgErr_GameObjectInitFailed;
    }
    return kSvgErr_NoError;
}

SavedGameError restore_game_data_ch_plugin_data(Stream *in, SavedGameRestorationData &restore_data)
{
    RuntimePluginStateVersion version = (RuntimePluginStateVersion)in->ReadInt32();
    if (version != kRtPluginStateVersion_Current)
    {
        return kSvgErr_DataVersionNotSupported;
    }
    // [IKM] Plugins expect FILE pointer! // TODO something with this later...
    platform->RunPluginHooks(AGSE_RESTOREGAME, (long)((Common::FileStream*)in)->GetHandle());
    return kSvgErr_NoError;
}

SavedGameError do_after_restore(SavedGameRestorationData &restore_data)
{
    // restore these to the ones retrieved from the save game
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        dynamicallyCreatedSurfaces[i] = restore_data.DynamicallyCreatedSurfacesFromSaveGame[i];
    }

    if (create_global_script())
    {
        Out::FPrint("Restore game error: unable to recreate global script: %s", ccErrorString);
        return kSvgErr_GameObjectInitFailed;
    }

    // read the global data into the newly created script
    memcpy(&gameinst->globaldata[0], restore_data.GlobalData.GetCArr(), restore_data.GlobalData.GetCount());

    // restore the script module data
    for (int i = 0; i < numScriptModules; ++i)
    {
        memcpy(&moduleInst[i]->globaldata[0], restore_data.ScriptModuleData[i].GetCArr(), restore_data.ScriptModuleData[i].GetCount());
    }

    setup_player_character(game.PlayerCharacterIndex);

    int gstimer=play.GlobalScriptTimer;
    int oldx1 = play.MouseBoundLeft, oldx2 = play.MouseBoundRight;
    int oldy1 = play.MouseBoundTop, oldy2 = play.MouseBoundBottom;
    int musicWasRepeating = play.CurrentMusicLoopMode;
    int newms = play.CurrentMusicIndex;

    // disable the queue momentarily
    int queuedMusicSize = play.MusicQueueLength;
    play.MusicQueueLength = 0;

    update_polled_stuff_if_runtime();

    if (displayed_room >= 0)
        load_new_room(displayed_room,NULL);//&game.Characters[game.PlayerCharacterIndex]);

    update_polled_stuff_if_runtime();

    play.GlobalScriptTimer=gstimer;

    // restore the correct room volume (they might have modified
    // it with SetMusicVolume)
    thisroom.Options[kRoomBaseOpt_MusicVolume] = restore_data.MusicVolume;

    filter->SetMouseLimit(oldx1,oldy1,oldx2,oldy2);

    set_cursor_mode(restore_data.CursorMode);
    set_mouse_cursor(restore_data.MouseCursor);
    if (restore_data.CursorMode == MODE_USE)
        SetActiveInventory (playerchar->activeinv);
    // ensure that the current cursor is locked
    spriteset.precache(game.MouseCursors[restore_data.MouseCursor].pic);

#if (ALLEGRO_DATE > 19990103)
    set_window_title(play.GameName);
#endif

    update_polled_stuff_if_runtime();

    if (displayed_room >= 0) {

        for (int i = 0; i < restore_data.BkgScenes.GetCount(); i++)
        {
            if (restore_data.BkgScenes[i])
            {
                delete thisroom.Backgrounds[i].Graphic;
                thisroom.Backgrounds[i].Graphic = restore_data.BkgScenes[i];
            }
        }

        in_new_room = 3;  // don't run "enters screen" events
        // now that room has loaded, copy saved light levels in
        for (int i = 0; i < thisroom.RegionCount; ++i)
        {
            thisroom.Regions[i].Light = restore_data.RegionLightLevels[i];
            thisroom.Regions[i].Tint = restore_data.RegionTintLevels[i];
        }
        generate_light_table();

        for (int i = 0; i < thisroom.WalkAreaCount; ++i)
        {
            thisroom.WalkAreas[i].Zoom = restore_data.WalkAreaZoomLevels[i];
            thisroom.WalkAreas[i].Zoom2 = restore_data.WalkAreaZoom2Levels[i];
        }

        for (int i = 0; i < CharMoveLists.GetCount(); ++i)
        {
            CharMoveLists[i] = restore_data.CharMoveLists[i];
        }
        for (int i = 0; i < ObjMoveLists.GetCount(); ++i)
        {
            ObjMoveLists[i] = restore_data.ObjMoveLists[i];
        }

        on_background_frame_change();
    }

    gui_disabled_style = convert_gui_disabled_style(game.Options[OPT_DISABLEOFF]);

    // restore the queue now that the music is playing
    play.MusicQueueLength = queuedMusicSize;

    if (play.DigitalMasterVolume >= 0)
        System_SetVolume(play.DigitalMasterVolume);

    for (int i = 1; i < MAX_SOUND_CHANNELS; ++i)
    {
        if (restore_data.DoAmbient[i])
        {
            PlayAmbientSound(i, restore_data.DoAmbient[i], ambient[i].vol, ambient[i].x, ambient[i].y);
        }
    }

    for (int i = 0; i < game.GuiCount; ++i)
    {
        guibg[i] = BitmapHelper::CreateBitmap (guis[i].wid, guis[i].hit, final_col_dep);
        guibg[i] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[i]);
    }

    if (gfxDriver->SupportsGammaControl())
    {
        gfxDriver->SetGamma(play.GammaAdjustment);
    }

    guis_need_update = 1;

    play.IgnoreUserInputUntilTime = 0;
    update_polled_stuff_if_runtime();

    platform->RunPluginHooks(AGSE_POSTRESTOREGAME, 0);

    if (displayed_room < 0)
    {
        // the restart point, no room was loaded
        load_new_room(playerchar->room, playerchar);
        playerchar->prevroom = -1;

        first_room_initialization();
    }

    if ((play.MusicQueueLength > 0) && (cachedQueuedMusic == NULL))
    {
        cachedQueuedMusic = load_music_from_disk(play.MusicQueue[0], 0);
    }
    return kSvgErr_NoError;
}

void ReadRichMediaHeader_Aligned(RICH_GAME_MEDIA_HEADER &rich_media_header, Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    rich_media_header.ReadFromFile(&align_s);
}

SavedGameError OpenSavedGame(const String &filename, SavedGameOpening &opening)
{
    opening.Clear();
    Stream *in = Common::File::OpenFileRead(filename);
    if (!in)
    {
        return kSvgErr_FileNotFound;
    }

    // skip Vista header
    RICH_GAME_MEDIA_HEADER rich_media_header;
    ReadRichMediaHeader_Aligned(rich_media_header, in);

    // check saved game signature
    String svg_sig = String::FromStreamCount(in, SavedGameOpening::SavedGameSigLength);
    SavedGameVersion svg_version;
    String description;
    if (svg_sig.Compare(SavedGameOpening::SavedGameSignature) == 0)
    {
        svg_version = (SavedGameVersion)in->ReadInt32();
    }
    else if (svg_sig.Compare(SavedGameOpening::LegacySavedGameSignature) == 0)
    {
        size_t pos = in->GetPosition();
        for (size_t desc_idx = 0; (in->ReadByte() > 0) && (desc_idx <= LegacyDescriptionLength); ++desc_idx);
        svg_version = (SavedGameVersion)in->ReadInt32();
        in->Seek(Common::kSeekBegin, pos);
    }
    else
    {
        // not a save game
        delete in;
        return kSvgErr_SignatureFailed;
    }

    // check saved game format version
    if (svg_version < kSvgVersion_LowestSupported ||
        svg_version > kSvgVersion_Current)
    {
        delete in;
        return kSvgErr_FormatVersionNotSupported;
    }

    int oldeip = our_eip;
    our_eip = 2050;

    opening.Filename = filename;
    opening.Version = svg_version;
    opening.InputStream = in;
    return kSvgErr_NoError;
}

SavedGameError ReadSavedGameDescription(const Common::String &filename, Common::String &description)
{
    description.Empty();
    SavedGameOpening opening;
    SavedGameError error_code = OpenSavedGame(filename, opening);
    if (error_code == kSvgErr_NoError)
    {
        Stream *in = opening.InputStream;
        if (opening.Version <= kSvgVersion_321)
        {
            description.Read(in, LegacyDescriptionLength);
        }
        else
        {
            description.Read(in);
        }
        safeguard_string(description);
    }
    our_eip = oldeip;
    return error_code;
}

SavedGameError ReadSavedGameScreenshot(const Common::String &filename, int &sprite_slot)
{
    sprite_slot = 0;
    SavedGameOpening opening;
    SavedGameError error_code = OpenSavedGame(filename, opening);
    if (error_code != kSvgErr_NoError)
    {
        our_eip = oldeip;
        return error_code;
    }

    Stream *in = opening.InputStream;
    if (opening.Version <= kSvgVersion_321)
    {
        // skip description and format version
        for (size_t desc_idx = 0; (in->ReadByte() > 0) && (desc_idx <= LegacyDescriptionLength); ++desc_idx);
        in->ReadInt32();
    }
    else
    {
        // skip description
        while(in->ReadByte() > 0);
    }
    // Read bitmap
    Bitmap *screenshot = restore_game_screenshot(in);
    if (screenshot)
    {
        int slot = spriteset.findFreeSlot();
        if (slot > 0)
        {
            // add it into the sprite set
            add_dynamic_sprite(slot, gfxDriver->ConvertBitmapToSupportedColourDepth(screenshot));
            sprite_slot = slot;
        }
        else
        {
            delete screenshot;
        }
    }

    our_eip = oldeip;
    return kSvgErr_NoError;
}

SavedGameError RestoreGameData(Stream *in)
{
    do_before_restore();
    SavedGameRestorationData restore_data;
    for (int ch = kSvgChapter_FirstChapter; ch < kSvgChapter_LastChapter; ++ch)
    {
        SavedGameError error = restore_game_chapter(in, ch, restore_data);
        if (error != kSvgErr_NoError)
        {
            return error;
        }
    }
    return do_after_restore(restore_data);
}

} // namespace Engine
} // namespace AGS

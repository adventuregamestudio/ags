//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/button.h"
#include "ac/character.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/file.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gamesetup.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/region.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/system.h"
#include "ac/timer.h"
#include "ac/dynobj/cc_serializer.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/managedobjectpool.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "font/fonts.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "game/savegame.h"
#include "game/savegame_components.h"
#include "game/savegame_internal.h"
#include "main/game_run.h"
#include "main/engine.h"
#include "main/main.h"
#include "main/update.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "plugin/agsplugin_evts.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/cc_common.h"
#include "util/file.h"
#include "util/memory_compat.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace Common;
using namespace Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern AGS::Engine::IGraphicsDriver *gfxDriver;
extern RoomStatus troom;
extern RoomStatus *croom;
extern std::vector<ViewStruct> views;


namespace AGS
{
namespace Engine
{

const String SavegameSource::Signature       = "Adventure Game Studio saved game v2";
// Size of the "windows vista rich game media header" feature, in bytes
const size_t LegacyRichHeaderSize = (6 * sizeof(int32_t)) + 16 + (1024 * sizeof(int16_t) * 4);

SavegameSource::SavegameSource()
    : Version(kSvgVersion_Undefined)
{
}

SavegameDescription::SavegameDescription()
    : LegacyID(0)
    , MainDataVersion(kGameVersion_Undefined)
    , ColorDepth(0)
{
}

PreservedParams::PreservedParams()
    : SpeechVOX(0)
    , MusicVOX(0)
    , GlScDataSize(0)
{
}

RestoredData::RestoredData()
    : FPS(0)
    , CursorID(0)
    , CursorMode(0)
{
    memset(RoomLightLevels, 0, sizeof(RoomLightLevels));
    memset(RoomTintLevels, 0, sizeof(RoomTintLevels));
    memset(RoomZoomLevels1, 0, sizeof(RoomZoomLevels1));
    memset(RoomZoomLevels2, 0, sizeof(RoomZoomLevels2));
}

String GetSavegameErrorText(SavegameErrorType err)
{
    switch (err)
    {
    case kSvgErr_NoError:
        return "No error.";
    case kSvgErr_FileOpenFailed:
        return "File not found or could not be opened.";
    case kSvgErr_SignatureFailed:
        return "Not an AGS saved game or unsupported format.";
    case kSvgErr_FormatVersionNotSupported:
        return "Save format version not supported.";
    case kSvgErr_IncompatibleEngine:
        return "Save was written by incompatible engine, or file is corrupted.";
    case kSvgErr_GameGuidMismatch:
        return "Game GUID does not match, saved by a different game.";
    case kSvgErr_ComponentListOpeningTagFormat:
        return "Failed to parse opening tag of the components list.";
    case kSvgErr_ComponentListClosingTagMissing:
        return "Closing tag of the components list was not met.";
    case kSvgErr_ComponentOpeningTagFormat:
        return "Failed to parse opening component tag.";
    case kSvgErr_ComponentClosingTagFormat:
        return "Failed to parse closing component tag.";
    case kSvgErr_ComponentSizeMismatch:
        return "Component data size mismatch.";
    case kSvgErr_UnsupportedComponent:
        return "Unknown and/or unsupported component.";
    case kSvgErr_ComponentSerialization:
        return "Failed to write the savegame component.";
    case kSvgErr_ComponentUnserialization:
        return "Failed to restore the savegame component.";
    case kSvgErr_InconsistentFormat:
        return "Inconsistent format, or file is corrupted.";
    case kSvgErr_UnsupportedComponentVersion:
        return "Component data version not supported.";
    case kSvgErr_GameContentAssertion:
        return "Saved content does not match current game.";
    case kSvgErr_InconsistentData:
        return "Inconsistent save data, or file is corrupted.";
    case kSvgErr_InconsistentPlugin:
        return "One of the game plugins did not restore its game data correctly.";
    case kSvgErr_DifferentColorDepth:
        return "Saved with the engine running at a different colour depth.";
    case kSvgErr_GameObjectInitFailed:
        return "Game object initialization failed after save restoration.";
    default:
        return "Unknown error.";
    }
}

Bitmap *RestoreSaveImage(Stream *in)
{
    if (in->ReadInt32())
        return read_serialized_bitmap(in);
    return nullptr;
}

void SkipSaveImage(Stream *in)
{
    if (in->ReadInt32())
        skip_serialized_bitmap(in);
}

HSaveError ReadDescription(Stream *in, SavegameVersion &svg_ver, SavegameDescription &desc, SavegameDescElem elems)
{
    svg_ver = (SavegameVersion)in->ReadInt32();
    if (svg_ver < kSvgVersion_LowestSupported || svg_ver > kSvgVersion_Current)
        return new SavegameError(kSvgErr_FormatVersionNotSupported,
            String::FromFormat("Required: %d, supported: %d - %d.", svg_ver, kSvgVersion_LowestSupported, kSvgVersion_Current));

    // Enviroment information
    if (svg_ver >= kSvgVersion_351)
        in->ReadInt32(); // enviroment info size
    if (elems & kSvgDesc_EnvInfo)
    {
        desc.EngineName = StrUtil::ReadString(in);
        desc.EngineVersion.SetFromString(StrUtil::ReadString(in));
        desc.GameGuid = StrUtil::ReadString(in);
        desc.GameTitle = StrUtil::ReadString(in);
        desc.MainDataFilename = StrUtil::ReadString(in);
        if (svg_ver >= kSvgVersion_Cmp_64bit)
            desc.MainDataVersion = (GameDataVersion)in->ReadInt32();
        desc.ColorDepth = in->ReadInt32();
        if (svg_ver >= kSvgVersion_351)
            desc.LegacyID = in->ReadInt32();
    }
    else
    {
        StrUtil::SkipString(in); // engine name
        StrUtil::SkipString(in); // engine version
        StrUtil::SkipString(in); // game guid
        StrUtil::SkipString(in); // game title
        StrUtil::SkipString(in); // main data filename
        if (svg_ver >= kSvgVersion_Cmp_64bit)
            in->ReadInt32(); // game data version
        in->ReadInt32(); // color depth
        if (svg_ver >= kSvgVersion_351)
            in->ReadInt32(); // game legacy id
    }
    // User description
    if (elems & kSvgDesc_UserText)
        desc.UserText = StrUtil::ReadString(in);
    else
        StrUtil::SkipString(in);
    if (elems & kSvgDesc_UserImage)
        desc.UserImage.reset(RestoreSaveImage(in));
    else
        SkipSaveImage(in);

    return HSaveError::None();
}

// Tests for the save signature, returns first supported version of found save type
SavegameVersion CheckSaveSignature(Stream *in)
{
    soff_t pre_sig_pos = in->GetPosition();
    String svg_sig = String::FromStreamCount(in, SavegameSource::Signature.GetLength());
    if (svg_sig.Compare(SavegameSource::Signature) == 0)
        return kSvgVersion_Components;
    in->Seek(pre_sig_pos, kSeekBegin);
    return kSvgVersion_Undefined;
}

HSaveError OpenSavegameBase(const String &filename, SavegameSource *src, SavegameDescription *desc, SavegameDescElem elems)
{
    UStream in(File::OpenFileRead(filename));
    if (!in.get())
        return new SavegameError(kSvgErr_FileOpenFailed, String::FromFormat("Requested filename: %s.", filename.GetCStr()));

    // Check saved game signature
    SavegameVersion sig_ver = CheckSaveSignature(in.get());
    if (sig_ver == kSvgVersion_Undefined)
    {
        // Skip MS Windows Vista rich media header (was present in older saves)
        in->Seek(LegacyRichHeaderSize);
        sig_ver = CheckSaveSignature(in.get());
        if (sig_ver == kSvgVersion_Undefined)
            return new SavegameError(kSvgErr_SignatureFailed);
    }

    SavegameVersion svg_ver;
    SavegameDescription temp_desc;
    HSaveError err = ReadDescription(in.get(), svg_ver, temp_desc, desc ? elems : kSvgDesc_None);
    if (!err)
        return err;

    if (src)
    {
        src->Filename = filename;
        src->Version = svg_ver;
        src->InputStream.reset(in.release()); // give the stream away to the caller
    }
    if (desc)
    {
        if (elems & kSvgDesc_EnvInfo)
        {
            desc->EngineName = temp_desc.EngineName;
            desc->EngineVersion = temp_desc.EngineVersion;
            desc->GameGuid = temp_desc.GameGuid;
            desc->LegacyID = temp_desc.LegacyID;
            desc->GameTitle = temp_desc.GameTitle;
            desc->MainDataFilename = temp_desc.MainDataFilename;
            desc->MainDataVersion = temp_desc.MainDataVersion;
            desc->ColorDepth = temp_desc.ColorDepth;
        }
        if (elems & kSvgDesc_UserText)
            desc->UserText = temp_desc.UserText;
        if (elems & kSvgDesc_UserImage)
            desc->UserImage.reset(temp_desc.UserImage.release());
    }
    return err;
}

HSaveError OpenSavegame(const String &filename, SavegameSource &src, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, &src, &desc, elems);
}

HSaveError OpenSavegame(const String &filename, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, nullptr, &desc, elems);
}

// Prepares engine for actual save restore (stops processes, cleans up memory)
void DoBeforeRestore(PreservedParams &pp, SaveCmpSelection select_cmp)
{
    pp.SpeechVOX = play.voice_avail;
    pp.MusicVOX = play.separate_music_lib;
    memcpy(pp.GameOptions, game.options, GameSetupStruct::MAX_OPTIONS * sizeof(int));

    unload_old_room();
    raw_saved_screen = nullptr;
    remove_all_overlays();
    play.complete_overlay_on = 0;
    play.text_overlay_on = 0;

    // cleanup dynamic sprites
    // NOTE: sprite 0 is a special constant sprite that cannot be dynamic (? is this actually true)
    for (size_t i = 1; i < spriteset.GetSpriteSlotCount(); ++i)
    {
        if (game.SpriteInfos[i].Flags & SPF_DYNAMICALLOC)
        {
            free_dynamic_sprite(i);
        }
    }

    // Cleanup drawn caches
    clear_drawobj_cache();

    // preserve script data sizes and cleanup scripts
    pp.GlScDataSize = gameinst->globaldatasize;
    pp.ScriptModuleNames.resize(numScriptModules);
    pp.ScMdDataSize.resize(numScriptModules);
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        pp.ScriptModuleNames[i] = moduleInst[i]->instanceof->GetScriptName();
        pp.ScMdDataSize[i] = moduleInst[i]->globaldatasize;
    }

    FreeAllScriptInstances();

    // reset saved room states
    resetRoomStatuses();
    // reset temp room state
    troom = RoomStatus();
    // reset (some of the?) GameState data
    // FIXME: investigate and refactor to be able to just reset whole object
    play.FreeProperties();
    play.FreeViewportsAndCameras();
    free_do_once_tokens();

    RemoveAllButtonAnimations();
    // Clear the managed object pool
    ccUnregisterAllObjects();

    if ((select_cmp & kSaveCmp_Audio) != 0)
    {
        for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
        {
            stop_and_destroy_channel(i);
        }
    }
}

static HSaveError RestoreAudio(const RestoredData &r_data)
{
    // recache queued clips
    // FIXME: this looks wrong, investigate if these
    // a) have to be deleted instead of resetting to null (store in unique_ptr!)
    // b) perhaps this should be done in DoBeforeRestore instead
    for (int i = 0; i < play.new_music_queue_size; ++i)
    {
        play.new_music_queue[i].cachedClip = nullptr;
    }

    if (play.audio_master_volume >= 0)
    {
        int temp_vol = play.audio_master_volume;
        play.audio_master_volume = -1; // reset to invalid state before re-applying
        System_SetVolume(temp_vol);
    }

    // Run audio clips on channels
    // these two crossfading parameters have to be temporarily reset
    const int cf_in_chan = play.crossfading_in_channel;
    const int cf_out_chan = play.crossfading_out_channel;
    play.crossfading_in_channel = 0;
    play.crossfading_out_channel = 0;
    
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
    {
        const RestoredData::ChannelInfo &chan_info = r_data.AudioChans[i];
        if (chan_info.ClipID < 0)
            continue;
        if ((size_t)chan_info.ClipID >= game.audioClips.size())
        {
            return new SavegameError(kSvgErr_GameObjectInitFailed,
                String::FromFormat("Invalid audio clip index: %d (clip count: %zu).", chan_info.ClipID, game.audioClips.size()));
        }
        play_audio_clip_on_channel(i, &game.audioClips[chan_info.ClipID],
            chan_info.Priority, chan_info.Repeat, chan_info.Pos);

        auto* ch = AudioChans::GetChannel(i);
        if (ch != nullptr)
        {
            ch->set_volume_direct(chan_info.VolAsPercent, chan_info.Vol);
            ch->set_speed(chan_info.Speed);
            ch->set_panning(chan_info.Pan);
            ch->xSource = chan_info.XSource;
            ch->ySource = chan_info.YSource;
            ch->maximumPossibleDistanceAway = chan_info.MaxDist;
        }
    }
    if ((cf_in_chan > 0) && (AudioChans::GetChannel(cf_in_chan) != nullptr))
        play.crossfading_in_channel = cf_in_chan;
    if ((cf_out_chan > 0) && (AudioChans::GetChannel(cf_out_chan) != nullptr))
        play.crossfading_out_channel = cf_out_chan;

    // If there were synced audio tracks, the time taken to load in the
    // different channels will have thrown them out of sync, so re-time it
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        int pos = r_data.AudioChans[i].Pos;
        if ((pos > 0) && (ch != nullptr))
        {
            ch->seek(pos);
        }
    }
 
    update_directional_sound_vol();
    return HSaveError::None();
}

static void RestoreViewportsAndCameras(const RestoredData &r_data)
{
    for (size_t i = 0; i < r_data.Cameras.size(); ++i)
    {
        const auto &cam_dat = r_data.Cameras[i];
        auto cam = play.GetRoomCamera(i);
        cam->SetID(cam_dat.ID);
        if ((cam_dat.Flags & kSvgCamPosLocked) != 0)
            cam->Lock();
        else
            cam->Release();
        // Set size first, or offset position may clamp to the room
        cam->SetSize(Size(cam_dat.Width, cam_dat.Height));
        cam->SetAt(cam_dat.Left, cam_dat.Top);
    }
    for (size_t i = 0; i < r_data.Viewports.size(); ++i)
    {
        const auto &view_dat = r_data.Viewports[i];
        auto view = play.GetRoomViewport(i);
        view->SetID(view_dat.ID);
        view->SetVisible((view_dat.Flags & kSvgViewportVisible) != 0);
        view->SetRect(RectWH(view_dat.Left, view_dat.Top, view_dat.Width, view_dat.Height));
        view->SetZOrder(view_dat.ZOrder);
        // Restore camera link
        int cam_index = view_dat.CamID;
        if (cam_index < 0) continue;
        auto cam = play.GetRoomCamera(cam_index);
        view->LinkCamera(cam);
        cam->LinkToViewport(view);
    }
    play.InvalidateViewportZOrder();
}

// Resets a number of options that are not supposed to be changed at runtime
static void CopyPreservedGameOptions(GameSetupStructBase &gs, const PreservedParams &pp)
{
    const auto restricted_opts = GameSetupStructBase::GetRestrictedOptions();
    for (auto opt : restricted_opts)
        gs.options[opt] = pp.GameOptions[opt];
    const auto preserved_opts = GameSetupStructBase::GetPreservedOptions();
    for (auto opt : preserved_opts)
        gs.options[opt] = pp.GameOptions[opt];
}

// A callback that tests if DynamicSprite refers a valid sprite in cache.
// Used in a call to ccTraverseManagedObjects.
static void ValidateDynamicSprite(int handle, IScriptObject *obj)
{
    ScriptDynamicSprite *dspr = static_cast<ScriptDynamicSprite*>(obj);
    if (dspr->slot < 0 || dspr->slot >= game.SpriteInfos.size() ||
        !game.SpriteInfos[dspr->slot].IsDynamicSprite())
    {
        dspr->slot = -1;
    }
}

// Final processing after successfully restoring from save
HSaveError DoAfterRestore(const PreservedParams &pp, RestoredData &r_data, SaveCmpSelection select_cmp)
{
    // Preserve whether the music vox is available
    play.voice_avail = pp.SpeechVOX;
    play.separate_music_lib = pp.MusicVOX;

    // Restore particular game options that must not change at runtime
    CopyPreservedGameOptions(game, pp);

    // Restore debug flags
    if (debug_flags & DBG_DEBUGMODE)
        play.debug_mode = 1;

    // Restore Overlay bitmaps (older save format, which stored them along with overlays)
    auto &overs = get_overlays();
    for (auto &over_im : r_data.OverlayImages)
    {
        auto &over = overs[over_im.first];
        over.SetImage(std::move(over_im.second), over.offsetX, over.offsetY);
    }
    // Restore dynamic surfaces
    const size_t dynsurf_num = std::min((size_t)MAX_DYNAMIC_SURFACES, r_data.DynamicSurfaces.size());
    for (size_t i = 0; i < dynsurf_num; ++i)
    {
        dynamicallyCreatedSurfaces[i] = std::move(r_data.DynamicSurfaces[i]);
    }

    // Re-export any missing audio channel script objects, e.g. if restoring old save
    export_missing_audiochans();

    AllocScriptModules();
    if (create_global_script())
    {
        return new SavegameError(kSvgErr_GameObjectInitFailed,
            String::FromFormat("Unable to recreate global script: %s",
                cc_get_error().ErrorString.GetCStr()));
    }

    // read the global data into the newly created script
    if (!r_data.GlobalScript.Data.empty())
        memcpy(gameinst->globaldata, &r_data.GlobalScript.Data.front(),
                std::min((size_t)gameinst->globaldatasize, r_data.GlobalScript.Data.size()));

    // restore the script module data
    for (auto &sc_entry : r_data.ScriptModules)
    {
        const String &name = sc_entry.first;
        auto &scdata = sc_entry.second;
        if (scdata.Data.empty())
            continue;
        for (auto &scmoduleinst : moduleInst)
        {
            if (name.Compare(scmoduleinst->instanceof->GetScriptName()) == 0)
            {
                memcpy(scmoduleinst->globaldata, &scdata.Data.front(),
                    std::min((size_t)scmoduleinst->globaldatasize, scdata.Data.size()));
                break;
            }
        }
    }

    setup_player_character(game.playercharacter);

    // Save some parameters to restore them after room load
    const int gstimer = play.gscript_timer;
    const Rect mouse_bounds = play.mbounds;
    // load the room the game was saved in
    if (displayed_room >= 0)
        load_new_room(displayed_room, nullptr);
    else
        set_room_placeholder();

    // Apply restored RTTI placeholder, after current room was loaded;
    // remap typeids in the deserialized managed objects, where necessary
    std::unordered_map<uint32_t, uint32_t> loc_l2g, type_l2g;
    ccInstance::JoinRTTI(r_data.GenRTTI, loc_l2g, type_l2g);
    pool.RemapTypeids(type_l2g);

    // Reapply few parameters after room load
    play.gscript_timer = gstimer;
    play.mbounds = mouse_bounds;


    set_cursor_mode(r_data.CursorMode);
    set_mouse_cursor(r_data.CursorID, true);
    if (r_data.CursorMode == MODE_USE)
        SetActiveInventory(playerchar->activeinv);
    // precache current cursor
    spriteset.PrecacheSprite(game.mcurs[r_data.CursorID].pic);

    sys_window_set_title(play.game_name.GetCStr());

    if (displayed_room >= 0)
    {
        // Fixup the frame index, in case the restored room does not have enough background frames
        if (play.bg_frame < 0 || static_cast<size_t>(play.bg_frame) >= thisroom.BgFrameCount)
            play.bg_frame = 0;

        for (int i = 0; i < MAX_ROOM_BGFRAMES; ++i)
        {
            if (r_data.RoomBkgScene[i])
            {
                thisroom.BgFrames[i].Graphic = r_data.RoomBkgScene[i];
            }
        }

        in_new_room=3;  // don't run "enters screen" events
        // now that room has loaded, copy saved light levels in
        for (size_t i = 0; i < MAX_ROOM_REGIONS; ++i)
        {
            thisroom.Regions[i].Light = r_data.RoomLightLevels[i];
            thisroom.Regions[i].Tint = r_data.RoomTintLevels[i];
        }
        generate_light_table();

        for (size_t i = 0; i < MAX_WALK_AREAS; ++i)
        {
            thisroom.WalkAreas[i].ScalingFar = r_data.RoomZoomLevels1[i];
            thisroom.WalkAreas[i].ScalingNear = r_data.RoomZoomLevels2[i];
        }

        on_background_frame_change();
    }

    if ((select_cmp & kSaveCmp_Audio) != 0)
    {
        HSaveError err = RestoreAudio(r_data);
        if (!err)
            return err;
    }

    adjust_fonts_for_render_mode(game.options[OPT_ANTIALIASFONTS] != 0);

    restore_characters();
    restore_overlays();

    prepare_gui_runtime(false /* not startup */);

    RestoreViewportsAndCameras(r_data);
    set_game_speed(r_data.FPS);

    // Run fixups over managed objects if necessary
    if ((select_cmp & kSaveCmp_DynamicSprites) == 0)
    {
        // If dynamic sprite images were not restored from this save, then invalidate all
        // DynamicSprite objects in the managed pool
        ccTraverseManagedObjects(ScriptDynamicSprite::TypeID, ValidateDynamicSprite);
    }

    // Run optional plugin event, reporting game restored
    pl_run_plugin_hooks(AGSE_POSTRESTOREGAME, 0);

    // Next load up any immediately required resources
    // If this is a restart point and no room was loaded, then load startup room
    if (displayed_room < 0)
    {
        load_new_room(playerchar->room, playerchar);
        first_room_initialization();
    }

    Mouse::SetMoveLimit(play.mbounds); // apply mouse bounds
    
    // Apply accessibility options, must be done last, because some
    // may override restored game settings
    ApplyAccessibilityOptions();

    play.ClearIgnoreInput(); // don't keep ignored input after save restore
    update_polled_stuff();

    return HSaveError::None();
}

// Fixes up a requested component selection, in case we must override or
// substitute something internally.
static SaveCmpSelection FixupCmpSelection(SaveCmpSelection select_cmp)
{
    // If kSaveCmp_DynamicSprites is not set, then set kSaveCmp_ObjectSprites
    //     ensure that object-owned images are still serialized.
    return (SaveCmpSelection)(select_cmp | 
        kSaveCmp_ObjectSprites * ((select_cmp & kSaveCmp_DynamicSprites) == 0));
}

HSaveError RestoreGameState(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp)
{
    select_cmp = FixupCmpSelection(select_cmp);

    PreservedParams pp;
    RestoredData r_data;
    DoBeforeRestore(pp, select_cmp);
    HSaveError err = SavegameComponents::ReadAll(in, svg_version, select_cmp, pp, r_data);
    if (!err)
        return err;
    return DoAfterRestore(pp, r_data, select_cmp);
}


void WriteSaveImage(Stream *out, const Bitmap *screenshot)
{
    // store the screenshot at the start to make it easily accesible
    out->WriteInt32((screenshot == nullptr) ? 0 : 1);

    if (screenshot)
        serialize_bitmap(screenshot, out);
}

void WriteDescription(Stream *out, const String &user_text, const Bitmap *user_image)
{
    // Data format version
    out->WriteInt32(kSvgVersion_Current);
    soff_t env_pos = out->GetPosition();
    out->WriteInt32(0);
    // Enviroment information
    StrUtil::WriteString(get_engine_name(), out);
    StrUtil::WriteString(EngineVersion.LongString, out);
    StrUtil::WriteString(game.guid, out);
    StrUtil::WriteString(game.gamename, out);
    StrUtil::WriteString(ResPaths.GamePak.Name, out);
    out->WriteInt32(loaded_game_file_version);
    out->WriteInt32(game.GetColorDepth());
    out->WriteInt32(game.uniqueid);
    soff_t env_end_pos = out->GetPosition();
    out->Seek(env_pos, kSeekBegin);
    out->WriteInt32(env_end_pos - env_pos);
    out->Seek(env_end_pos, kSeekBegin);
    // User description
    StrUtil::WriteString(user_text, out);
    WriteSaveImage(out, user_image);
}

std::unique_ptr<Stream> StartSavegame(const String &filename, const String &user_text, const Bitmap *user_image)
{
    auto out = File::CreateFile(filename);
    if (!out)
        return nullptr;

    // Savegame signature
    out->Write(SavegameSource::Signature.GetCStr(), SavegameSource::Signature.GetLength());

    // CHECKME: what is this plugin hook suppose to mean, and if it is called here correctly
    pl_run_plugin_hooks(AGSE_PRESAVEGAME, 0);

    // Write descrition block
    WriteDescription(out.get(), user_text, user_image);
    return out;
}

void DoBeforeSave()
{
    if (displayed_room >= 0)
    {
        // update the current room script's data segment copy
        if (roominst)
            save_room_data_segment();
    }
}

void SaveGameState(Stream *out, SaveCmpSelection select_cmp)
{
    select_cmp = FixupCmpSelection(select_cmp);

    DoBeforeSave();
    SavegameComponents::WriteAllCommon(out, select_cmp);
}

void ReadPluginSaveData(Stream *in, PluginSvgVersion svg_ver, soff_t max_size)
{
    const soff_t start_pos = in->GetPosition();
    const soff_t end_pos = start_pos + max_size;

    if (svg_ver >= kPluginSvgVersion_36115)
    {
        int num_plugins_read = in->ReadInt32();
        soff_t cur_pos = start_pos;
        while ((num_plugins_read--) > 0 && (cur_pos < end_pos))
        {
            String pl_name = StrUtil::ReadString(in);
            size_t data_size = in->ReadInt32();
            soff_t data_start = in->GetPosition();

            auto guard_stream = std::make_unique<Stream>(
                std::make_unique<StreamSection>(in->GetStreamBase(), in->GetPosition(), end_pos));
            int32_t fhandle = add_file_stream(std::move(guard_stream), "RestoreGame");
            pl_run_plugin_hook_by_name(pl_name, AGSE_RESTOREGAME, fhandle);
            close_file_stream(fhandle, "RestoreGame");

            // Seek to the end of plugin data, in case it ended up reading not in the end
            cur_pos = data_start + data_size;
            in->Seek(cur_pos, kSeekBegin);
        }
    }
    else
    {
        String pl_name;
        for (int pl_index = 0; pl_query_next_plugin_for_event(AGSE_RESTOREGAME, pl_index, pl_name); ++pl_index)
        {
            auto guard_stream = std::make_unique<Stream>(
                std::make_unique<StreamSection>(in->GetStreamBase(), in->GetPosition(), end_pos));
            int32_t fhandle = add_file_stream(std::move(guard_stream), "RestoreGame");
            pl_run_plugin_hook_by_index(pl_index, AGSE_RESTOREGAME, fhandle);
            close_file_stream(fhandle, "RestoreGame");
        }
    }
}

void WritePluginSaveData(Stream *out)
{
    soff_t pluginnum_pos = out->GetPosition();
    out->WriteInt32(0); // number of plugins which wrote data

    int num_plugins_wrote = 0;
    String pl_name;
    for (int pl_index = 0; pl_query_next_plugin_for_event(AGSE_SAVEGAME, pl_index, pl_name); ++pl_index)
    {
        // NOTE: we don't care if they really write anything,
        // but count them so long as they subscribed to AGSE_SAVEGAME
        num_plugins_wrote++;

        // Write a header for plugin data
        StrUtil::WriteString(pl_name, out);
        soff_t data_size_pos = out->GetPosition();
        out->WriteInt32(0); // data size

        // Create a stream section and write plugin data
        soff_t data_start_pos = out->GetPosition();
        auto guard_stream = std::make_unique<Stream>(
            std::make_unique<StreamSection>(out->GetStreamBase(), out->GetPosition(), INT64_MAX));
        int32_t fhandle = add_file_stream(std::move(guard_stream), "SaveGame");
        pl_run_plugin_hook_by_index(pl_index, AGSE_SAVEGAME, fhandle);
        close_file_stream(fhandle, "SaveGame");

        // Finalize header
        soff_t data_end_pos = out->GetPosition();
        out->Seek(data_size_pos, kSeekBegin);
        out->WriteInt32(data_end_pos - data_start_pos);
        out->Seek(0, kSeekEnd);
    }

    // Write number of plugins
    out->Seek(pluginnum_pos, kSeekBegin);
    out->WriteInt32(num_plugins_wrote);
    out->Seek(0, kSeekEnd);
}

} // namespace Engine
} // namespace AGS

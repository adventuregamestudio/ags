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
#include "ac/string.h"
#include "ac/system.h"
#include "ac/timer.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/dynobj/scriptrestoredsaveinfo.h"
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
#include "script/script_runtime.h"
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

const String SavegameSource::LegacySignature = "Adventure Game Studio saved game";
const String SavegameSource::Signature       = "Adventure Game Studio saved game v2";
// Size of the "windows vista rich game media header" feature, in bytes
const size_t LegacyRichHeaderSize = (6 * sizeof(int32_t)) + 16 + (1024 * sizeof(int16_t) * 4);

SavegameSource::SavegameSource()
    : Version(kSvgVersion_Undefined)
{
}

SavegameDescription::SavegameDescription(const SavegameDescription &desc)
{
    Slot = desc.Slot;
    EngineName = (desc.EngineName);
    EngineVersion = desc.EngineVersion;
    GameGuid = desc.GameGuid;
    LegacyID = desc.LegacyID;
    GameTitle = desc.GameTitle;
    MainDataFilename = desc.MainDataFilename;
    MainDataVersion = desc.MainDataVersion;
    ColorDepth = desc.ColorDepth;
    UserText = desc.UserText;
    if (desc.UserImage)
        UserImage.reset(BitmapHelper::CreateBitmapCopy(desc.UserImage.get()));
}

RestoredData::RestoredData()
    : FPS(0)
    , DoOnceCount(0u)
    , RoomVolume(kRoomVolumeNormal)
    , CursorID(0)
    , CursorMode(0)
{
    memset(RoomLightLevels, 0, sizeof(RoomLightLevels));
    memset(RoomTintLevels, 0, sizeof(RoomTintLevels));
    memset(RoomZoomLevels1, 0, sizeof(RoomZoomLevels1));
    memset(RoomZoomLevels2, 0, sizeof(RoomZoomLevels2));
    memset(DoAmbient, 0, sizeof(DoAmbient));
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
    {
        return kSvgVersion_Components;
    }
    else
    {
        in->Seek(pre_sig_pos, kSeekBegin);
        svg_sig = String::FromStreamCount(in, SavegameSource::LegacySignature.GetLength());
        if (svg_sig.Compare(SavegameSource::LegacySignature) == 0)
            return kSvgVersion_321;
    }
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
    // unregister gui controls from API exports
    // CHECKME: find out why are we doing this here? why only to gui controls?
    for (int i = 0; i < game.numgui; ++i)
    {
        unexport_gui_controls(i);
    }
    // Clear the managed object pool
    ccUnregisterAllObjects();

    if ((select_cmp & kSaveCmp_Audio) != 0)
    {
        for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
        {
            stop_and_destroy_channel_ex(i, false);
        }
        clear_music_cache();
    }
}

void FillPreservedParams(PreservedParams &pp)
{
    // preserve script data sizes
    pp.GlScDataSize = gameinst->globaldatasize;
    pp.ScriptModuleNames.resize(numScriptModules);
    pp.ScMdDataSize.resize(numScriptModules);
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        pp.ScriptModuleNames[i] = moduleInst[i]->instanceof->GetScriptName();
        pp.ScMdDataSize[i] = moduleInst[i]->globaldatasize;
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

    if (play.digital_master_volume >= 0)
    {
        int temp_vol = play.digital_master_volume;
        play.digital_master_volume = -1; // reset to invalid state before re-applying
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

            if ((chan_info.Flags & kSvgAudioPaused) != 0)
                ch->pause();
        }
    }
    if ((cf_in_chan > 0) && (AudioChans::GetChannel(cf_in_chan) != nullptr))
        play.crossfading_in_channel = cf_in_chan;
    if ((cf_out_chan > 0) && (AudioChans::GetChannel(cf_out_chan) != nullptr))
        play.crossfading_out_channel = cf_out_chan;

    // Test if the old-style audio had playing music and it was properly loaded
    if (current_music_type > 0)
    {
        if ((crossFading > 0 && !AudioChans::GetChannelIfPlaying(crossFading)) ||
            (crossFading <= 0 && !AudioChans::GetChannelIfPlaying(SCHAN_MUSIC)))
        {
            current_music_type = 0; // playback failed, reset flag
        }
    }

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

    for (int i = NUM_SPEECH_CHANS; i < game.numGameChannels; ++i)
    {
        if (r_data.DoAmbient[i])
            PlayAmbientSound(i, r_data.DoAmbient[i], ambient[i].vol, ambient[i].x, ambient[i].y);
    }

    update_directional_sound_vol();
    return HSaveError::None();
}

static void RestoreViewportsAndCameras(const RestoredData &r_data)
{
    // If restored from older saves, we have to adjust
    // cam and view sizes to a main viewport, which is init later
    const auto &main_view = play.GetMainViewport();

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
        if (r_data.LegacyViewCamera)
            cam->SetSize(main_view.GetSize());
        else
            cam->SetSize(Size(cam_dat.Width, cam_dat.Height));
        cam->SetAt(cam_dat.Left, cam_dat.Top);
    }
    for (size_t i = 0; i < r_data.Viewports.size(); ++i)
    {
        const auto &view_dat = r_data.Viewports[i];
        auto view = play.GetRoomViewport(i);
        view->SetID(view_dat.ID);
        view->SetVisible((view_dat.Flags & kSvgViewportVisible) != 0);
        if (r_data.LegacyViewCamera)
            view->SetRect(RectWH(view_dat.Left, view_dat.Top, main_view.GetWidth(), main_view.GetHeight()));
        else
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
    if (dspr->slot < 0 || static_cast<uint32_t>(dspr->slot) >= game.SpriteInfos.size() ||
        !game.SpriteInfos[dspr->slot].IsDynamicSprite())
    {
        dspr->slot = -1;
    }
}

// Call a scripting event to let user validate the restored save
static HSaveError ValidateRestoredSave(const SavegameDescription &save_desc, const RestoredData &r_data, SaveRestoreFeedback &feedback)
{
    auto *saveinfo = new ScriptRestoredSaveInfo(r_data.Result.RestoreFlags, save_desc, r_data.DataCounts,
        (r_data.Result.RestoreFlags & kSaveRestore_MismatchMask) != 0);
    int handle = ccRegisterManagedObject(saveinfo, saveinfo);
    ccAddObjectReference(handle); // add internal ref

    RuntimeScriptValue params[1] = { RuntimeScriptValue().SetScriptObject(saveinfo, saveinfo) };
    RunScriptFunctionInModules("validate_restored_save", 1, params); // TODO: run in room script too?

    const bool do_cancel = saveinfo->GetCancel();
    const SaveCmpSelection retry_ignore_cmp = saveinfo->GetRetryWithoutComponents();
    ccReleaseObjectReference(handle); // rem internal ref

    if (do_cancel)
    {
        return new SavegameError(kSvgErr_GameContentAssertion, r_data.Result.FirstMismatchError);
    }

    if (retry_ignore_cmp != 0)
    {
        feedback.RetryWithClearGame = true;
        feedback.RetryWithoutComponents = retry_ignore_cmp;
        return new SavegameError(kSvgErr_GameContentAssertion);
    }

    return HSaveError::None();
}

// Final processing after successfully restoring from save
HSaveError DoAfterRestore(const PreservedParams &pp, RestoredData &r_data, SaveCmpSelection select_cmp)
{
    // Use a yellow dialog highlight for older game versions
    // CHECKME: it is dubious that this should be right here
    if(loaded_game_file_version < kGameVersion_331)
        play.dialog_options_highlight_color = DIALOG_OPTIONS_HIGHLIGHT_COLOR_DEFAULT;

    // Preserve whether the music vox is available
    play.voice_avail = pp.SpeechVOX;
    play.separate_music_lib = pp.MusicVOX;

    // Restore particular game options that must not change at runtime
    CopyPreservedGameOptions(game, pp);

    // Restore debug flags
    if (debug_flags & DBG_DEBUGMODE)
        play.debug_mode = 1;

    // Remap old sound nums in case we restored a save having a different list of audio clips
    RemapLegacySoundNums(game, views, loaded_game_file_version);

    // Restore Overlay bitmaps (older save format, which stored them along with overlays)
    auto &overs = get_overlays();
    for (auto &over_im : r_data.OverlayImages)
    {
        auto &over = overs[over_im.first];
        over.SetImage(std::move(over_im.second), over.HasAlphaChannel(), over.offsetX, over.offsetY);
    }
    // Restore dynamic surfaces
    const size_t dynsurf_num = std::min((size_t)MAX_DYNAMIC_SURFACES, r_data.DynamicSurfaces.size());
    for (size_t i = 0; i < dynsurf_num; ++i)
    {
        dynamicallyCreatedSurfaces[i] = std::move(r_data.DynamicSurfaces[i]);
    }

    // Rebuild GUI links to child controls
    GUIRefCollection guictrl_refs(guibuts, guiinv, guilabels, guilist, guislider, guitext);
    GUI::RebuildGUI(guis, guictrl_refs);

    // Re-export any missing audio channel script objects, e.g. if restoring old save
    export_missing_audiochans();

    // CHECKME: find out why are we doing this here? why only to gui controls?
    for (int i = 0; i < game.numgui; ++i)
        export_gui_controls(i);

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
    {
        load_new_room(displayed_room, nullptr);
        r_data.DataCounts.RoomScriptDataSz = croom->tsdatasize;
    }
    else
    {
        set_room_placeholder();
    }

    // Reapply few parameters after room load
    play.gscript_timer = gstimer;
    play.mbounds = mouse_bounds;

    // restore the correct room volume (they might have modified
    // it with SetMusicVolume)
    thisroom.Options.MusicVolume = r_data.RoomVolume;

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
    restore_movelists();

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

    // After all of the game logical state is initialized and reapplied values from save,
    // call a "validate" script callback, to let user check the restored save
    // and make final decision: whether to continue with the game, or cancel and quit.
    HSaveError validate_err = ValidateRestoredSave(pp.Desc, r_data, r_data.Result.Feedback);
    if (!validate_err)
        return validate_err;

    // Run optional plugin event, reporting game restored
    pl_run_plugin_hooks(AGSE_POSTRESTOREGAME, 0);

    // Next load up any immediately required resources
    // If this is a restart point and no room was loaded, then load startup room
    if (displayed_room < 0)
    {
        load_new_room(playerchar->room, playerchar);
        first_room_initialization();
    }

    // Fill in queued music cache
    if ((play.music_queue_size > 0) && (cachedQueuedMusic == nullptr))
    {
        cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
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

HSaveError RestoreGameState(Stream *in, const SavegameDescription &desc, const RestoreGameStateOptions &options, SaveRestoreFeedback &feedback)
{
    SaveCmpSelection select_cmp = FixupCmpSelection(options.SelectedComponents);
    const bool has_validate_cb = DoesScriptFunctionExistInModules("validate_restored_save");

    PreservedParams pp(desc);
    RestoredData r_data;
    DoBeforeRestore(pp, select_cmp); // WARNING: this frees scripts and some other data

    // Mark the clear game data state for restoration process
    r_data.Result.RestoreFlags = (SaveRestorationFlags)(
          (kSaveRestore_ClearData * options.IsGameClear) // tell that the game data is reset
        | (kSaveRestore_AllowMismatchLess * has_validate_cb) // allow less data in saves
        );

    HSaveError err = SavegameComponents::ReadAll(in, options.SaveVersion, select_cmp, pp, r_data);
    feedback = r_data.Result.Feedback;
    if (!err)
        return err;
    return DoAfterRestore(pp, r_data, select_cmp);
}

HSaveError PrescanSaveState(Stream *in, const SavegameDescription &desc,
    const RestoreGameStateOptions &options)
{
    SaveCmpSelection select_cmp = FixupCmpSelection(options.SelectedComponents);
    const bool has_validate_cb = DoesScriptFunctionExistInModules("validate_restored_save");

    PreservedParams pp(desc);
    RestoredData r_data;
    FillPreservedParams(pp);

    // Mark the clear game data state for restoration process
    r_data.Result.RestoreFlags = (SaveRestorationFlags)(
          (kSaveRestore_ClearData) // always tell that the game data is reset for prescanning
        | (kSaveRestore_AllowMismatchLess * has_validate_cb) // allow less data in saves
        );

    HSaveError err = SavegameComponents::PrescanAll(in, options.SaveVersion, select_cmp, pp, r_data);
    if (!err)
    {
        return err;
    }

    if (has_validate_cb)
    {
        // After we have prescanned and gathered save info,
        // call a "validate" script callback, to let user check the restored save
        // and make final decision: whether it is considered compatible or not.
        err = ValidateRestoredSave(pp.Desc, r_data, r_data.Result.Feedback);
    }
    return err;
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
    if (play.cur_music_number >= 0)
    {
        if (IsMusicPlaying() == 0)
            play.cur_music_number = -1;
    }

    if (displayed_room >= 0)
    {
        // update the current room script's data segment copy
        if (roominst)
            save_room_data_segment();

        // Update the saved interaction variable values
        for (size_t i = 0; i < thisroom.LocalVariables.size() && i < (size_t)MAX_INTERACTION_VARIABLES; ++i)
            croom->interactionVariableValues[i] = thisroom.LocalVariables[i].Value;
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

//=============================================================================
//
// RestoredSaveInfo API
//
//=============================================================================

} // namespace Engine
} // namespace AGS

#include "debug/debug_log.h"

bool SaveInfo_GetCancel(ScriptRestoredSaveInfo *info)
{
    return info->GetCancel();
}

void SaveInfo_SetCancel(ScriptRestoredSaveInfo *info, bool cancel)
{
    info->SetCancel(cancel);
}

int SaveInfo_GetRetryWithoutComponents(ScriptRestoredSaveInfo *info)
{
    return info->GetRetryWithoutComponents();
}

void SaveInfo_SetRetryWithoutComponents(ScriptRestoredSaveInfo *info, int cmp_selection)
{
    info->SetRetryWithoutComponents(static_cast<SaveCmpSelection>(cmp_selection));
}

int SaveInfo_GetResult(ScriptRestoredSaveInfo *info)
{
    return info->GetResult();
}

int SaveInfo_GetSlot(ScriptRestoredSaveInfo *info)
{
    return info->GetDesc().Slot;
}

const char* SaveInfo_GetDescription(ScriptRestoredSaveInfo *info)
{
    return CreateNewScriptString(info->GetDesc().UserText.GetCStr());
}

const char* SaveInfo_GetEngineVersion(ScriptRestoredSaveInfo *info)
{
    return CreateNewScriptString(info->GetDesc().EngineVersion.LongString.GetCStr());
}

int SaveInfo_GetAudioClipTypeCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().AudioClipTypes;
}

int SaveInfo_GetCharacterCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Characters;
}

int SaveInfo_GetDialogCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Dialogs;
}

int SaveInfo_GetGUICount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().GUIs;
}

int SaveInfo_GetGUIControlCount(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().GUIControls.size())
    {
        debug_script_warn("RestoredSaveInfo::GUIControlCount: index %d out of bounds (%d..%d)", index, 0, info->GetCounts().GUIs);
        return 0;
    }
    return info->GetCounts().GUIControls[index];
}

int SaveInfo_GetInventoryItemCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().InventoryItems;
}

int SaveInfo_GetCursorCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Cursors;
}

int SaveInfo_GetViewCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Views;
}

int SaveInfo_GetViewLoopCount(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ViewLoops.size())
    {
        debug_script_warn("RestoredSaveInfo::ViewLoopCount: index %d out of bounds (%d..%d)", index, 0, info->GetCounts().Views);
        return 0;
    }
    return info->GetCounts().ViewLoops[index];
}

int SaveInfo_GetViewFrameCount(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ViewFrames.size())
    {
        debug_script_warn("RestoredSaveInfo::ViewFrameCount: index %d out of bounds (%d..%d)", index, 0, info->GetCounts().Views);
        return 0;
    }
    return info->GetCounts().ViewFrames[index];
}

int SaveInfo_GetGlobalScriptDataSize(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().GlobalScriptDataSz;
}

int SaveInfo_GetScriptModuleCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().ScriptModules;
}

int SaveInfo_GetScriptModuleDataSize(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ScriptModuleDataSz.size())
    {
        debug_script_warn("RestoredSaveInfo::ScriptModuleDataSize: index %d out of bounds (%d..%d)", index, 0, info->GetCounts().ScriptModules);
        return 0;
    }
    return info->GetCounts().ScriptModuleDataSz[index];
}

int SaveInfo_GetRoomScriptDataSize(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().RoomScriptDataSz;
}

RuntimeScriptValue Sc_SaveInfo_GetCancel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_GetCancel);
}

RuntimeScriptValue Sc_SaveInfo_SetCancel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptRestoredSaveInfo, SaveInfo_SetCancel);
}

RuntimeScriptValue Sc_SaveInfo_GetRetryWithoutComponents(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetRetryWithoutComponents);
}

RuntimeScriptValue Sc_SaveInfo_SetRetryWithoutComponents(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRestoredSaveInfo, SaveInfo_SetRetryWithoutComponents);
}

RuntimeScriptValue Sc_SaveInfo_GetResult(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetResult);
}

RuntimeScriptValue Sc_SaveInfo_GetSlot(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetSlot);
}

RuntimeScriptValue Sc_SaveInfo_GetDescription(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetDescription);
}

RuntimeScriptValue Sc_SaveInfo_GetEngineVersion(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetEngineVersion);
}

RuntimeScriptValue Sc_SaveInfo_GetAudioClipTypeCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetAudioClipTypeCount);
}

RuntimeScriptValue Sc_SaveInfo_GetCharacterCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetCharacterCount);
}

RuntimeScriptValue Sc_SaveInfo_GetDialogCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetDialogCount);
}

RuntimeScriptValue Sc_SaveInfo_GetGUICount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetGUICount);
}

RuntimeScriptValue Sc_SaveInfo_GetGUIControlCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetGUIControlCount);
}

RuntimeScriptValue Sc_SaveInfo_GetInventoryItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetInventoryItemCount);
}

RuntimeScriptValue Sc_SaveInfo_GetCursorCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetCursorCount);
}

RuntimeScriptValue Sc_SaveInfo_GetViewCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetViewCount);
}

RuntimeScriptValue Sc_SaveInfo_GetViewLoopCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetViewLoopCount);
}

RuntimeScriptValue Sc_SaveInfo_GetViewFrameCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetViewFrameCount);
}

RuntimeScriptValue Sc_SaveInfo_GetGlobalScriptDataSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetGlobalScriptDataSize);
}

RuntimeScriptValue Sc_SaveInfo_GetScriptModuleCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetScriptModuleCount);
}

RuntimeScriptValue Sc_SaveInfo_GetScriptModuleDataSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetScriptModuleDataSize);
}

RuntimeScriptValue Sc_SaveInfo_GetRoomScriptDataSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetRoomScriptDataSize);
}

void RegisterSaveInfoAPI()
{
    ScFnRegister saveinfo_api[] = {
        { "RestoredSaveInfo::get_Cancel",               API_FN_PAIR(SaveInfo_GetCancel) },
        { "RestoredSaveInfo::set_Cancel",               API_FN_PAIR(SaveInfo_SetCancel) },
        { "RestoredSaveInfo::get_RetryWithoutComponents", API_FN_PAIR(SaveInfo_GetRetryWithoutComponents) },
        { "RestoredSaveInfo::set_RetryWithoutComponents", API_FN_PAIR(SaveInfo_SetRetryWithoutComponents) },
        { "RestoredSaveInfo::get_Result",               API_FN_PAIR(SaveInfo_GetResult) },
        { "RestoredSaveInfo::get_Slot",                 API_FN_PAIR(SaveInfo_GetSlot) },
        { "RestoredSaveInfo::get_Description",          API_FN_PAIR(SaveInfo_GetDescription) },
        { "RestoredSaveInfo::get_EngineVersion",        API_FN_PAIR(SaveInfo_GetEngineVersion) },
        { "RestoredSaveInfo::get_AudioClipTypeCount",   API_FN_PAIR(SaveInfo_GetAudioClipTypeCount) },
        { "RestoredSaveInfo::get_CharacterCount",       API_FN_PAIR(SaveInfo_GetCharacterCount) },
        { "RestoredSaveInfo::get_DialogCount",          API_FN_PAIR(SaveInfo_GetDialogCount) },
        { "RestoredSaveInfo::get_GUICount",             API_FN_PAIR(SaveInfo_GetGUICount) },
        { "RestoredSaveInfo::geti_GUIControlCount",     API_FN_PAIR(SaveInfo_GetGUIControlCount) },
        { "RestoredSaveInfo::get_InventoryItemCount",   API_FN_PAIR(SaveInfo_GetInventoryItemCount) },
        { "RestoredSaveInfo::get_CursorCount",          API_FN_PAIR(SaveInfo_GetCursorCount) },
        { "RestoredSaveInfo::get_ViewCount",            API_FN_PAIR(SaveInfo_GetViewCount) },
        { "RestoredSaveInfo::geti_ViewLoopCount",       API_FN_PAIR(SaveInfo_GetViewLoopCount) },
        { "RestoredSaveInfo::geti_ViewFrameCount",      API_FN_PAIR(SaveInfo_GetViewFrameCount) },
        { "RestoredSaveInfo::get_GlobalScriptDataSize", API_FN_PAIR(SaveInfo_GetGlobalScriptDataSize) },
        { "RestoredSaveInfo::get_ScriptModuleCount",    API_FN_PAIR(SaveInfo_GetScriptModuleCount) },
        { "RestoredSaveInfo::geti_ScriptModuleDataSize",API_FN_PAIR(SaveInfo_GetScriptModuleDataSize) },
        { "RestoredSaveInfo::get_RoomScriptDataSize",   API_FN_PAIR(SaveInfo_GetRoomScriptDataSize) },
    };

    ccAddExternalFunctions(saveinfo_api);
}

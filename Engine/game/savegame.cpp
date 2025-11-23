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
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/region.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/shader_script.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/timer.h"
#include "ac/dynobj/cc_serializer.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/managedobjectpool.h"
#include "ac/dynobj/scriptrestoredsaveinfo.h"
#include "ac/dynobj/scriptshader.h"
#include "ac/dynobj/scriptuserobject.h"
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
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/cc_common.h"
#include "script/script_runtime.h"
#include "util/compress.h"
#include "util/file.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"
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
    case kSvgErr_ComponentUncompressedSizeMismatch:
        return "Uncompressed component data size mismatch.";
    case kSvgErr_InternalError:
        return "Internal program error.";
    default:
        return "Unknown error.";
    }
}

Bitmap *ReadBitmap(Stream *in, bool compressed)
{
    const int picwid = in->ReadInt32();
    const int pichit = in->ReadInt32();
    const int piccoldep = in->ReadInt32();
    Bitmap *thispic = BitmapHelper::CreateBitmap(picwid, pichit, piccoldep);
    if (thispic == nullptr)
        return nullptr;

    if (compressed)
    {
        in->ReadInt32(); // reserved
        size_t compressed_sz = in->ReadInt32();
        inflate_decompress(thispic->GetDataForWriting(), thispic->GetWidth() * thispic->GetHeight() * thispic->GetBPP(),
                           thispic->GetBPP(), in, compressed_sz);
    }
    else
    {
        // TODO: move this code to BitmapHelper?
        for (int h = 0; h < pichit; ++h)
        {
            switch (piccoldep)
            {
            case 8:
                in->ReadArray(thispic->GetScanLineForWriting(h), picwid, 1);
                break;
            case 16:
                in->ReadArrayOfInt16((int16_t *)thispic->GetScanLineForWriting(h), picwid);
                break;
            case 32:
                in->ReadArrayOfInt32((int32_t *)thispic->GetScanLineForWriting(h), picwid);
                break;
            }
        }
    }

    return thispic;
}

void SkipBitmap(Stream *in, bool compressed)
{
    int picwid = in->ReadInt32();
    int pichit = in->ReadInt32();
    int piccoldep = in->ReadInt32();

    if (compressed)
    {
        in->ReadInt32(); // reserved
        size_t compress_sz = in->ReadInt32();
        in->Seek(compress_sz);
    }
    else
    {
        int bpp = (piccoldep + 7) / 8;
        in->Seek(picwid * pichit * bpp);
    }
}

void WriteBitmap(const Common::Bitmap *thispic, Stream *out, bool compressed)
{
    assert(thispic);
    if (!thispic)
        return;

    out->WriteInt32(thispic->GetWidth());
    out->WriteInt32(thispic->GetHeight());
    out->WriteInt32(thispic->GetColorDepth());

    if (compressed)
    {
        out->WriteInt32(0); // reserved
        soff_t size_pos = out->GetPosition();
        out->WriteInt32(0); // size placeholder
        deflate_compress(thispic->GetData(), thispic->GetWidth() * thispic->GetHeight() * thispic->GetBPP(),
                         thispic->GetBPP(), out);
        soff_t end_pos = out->GetPosition();
        out->Seek(size_pos, kSeekBegin);
        out->WriteInt32(end_pos - size_pos - sizeof(int32_t));
        out->Seek(end_pos, kSeekEnd);
    }
    else
    {
        // TODO: move this code to BitmapHelper?
        for (int h = 0; h < thispic->GetHeight(); ++h)
        {
            switch (thispic->GetColorDepth())
            {
            case 8:
                out->WriteArray(&thispic->GetScanLine(h)[0], thispic->GetWidth(), 1);
                break;
            case 16:
                out->WriteArrayOfInt16((const int16_t *)&thispic->GetScanLine(h)[0], thispic->GetWidth());
                break;
            case 32:
                out->WriteArrayOfInt32((const int32_t *)&thispic->GetScanLine(h)[0], thispic->GetWidth());
                break;
            }
        }
    }
}

Bitmap *RestoreUserImage(Stream *in)
{
    uint32_t flags = in->ReadInt32();
    if ((flags & kSvgImage_Present) != 0)
        return ReadBitmap(in, (flags & kSvgImage_Deflate) != 0);
    return nullptr;
}

void SkipUserImage(Stream *in)
{
    uint32_t flags = in->ReadInt32();
    if ((flags & kSvgImage_Present) != 0)
        SkipBitmap(in, (flags & kSvgImage_Deflate) != 0);
}

HSaveError ReadDescription(Stream *in, SavegameVersion &svg_ver, SavegameDescription &desc, SavegameDescElem elems)
{
    svg_ver = (SavegameVersion)in->ReadInt32();
    if (svg_ver < kSvgVersion_LowestSupported || svg_ver > kSvgVersion_Current)
        return new SavegameError(kSvgErr_FormatVersionNotSupported,
            String::FromFormat("Required: %d, supported: %d - %d.", svg_ver, kSvgVersion_LowestSupported, kSvgVersion_Current));

    const bool has_fileformat = (svg_ver >= kSvgVersion_363 && svg_ver < kSvgVersion_399)
        || (svg_ver >= kSvgVersion_400_18);

    // File format info
    if (has_fileformat)
    {
        desc.Format.FileFormatOffset = in->GetPosition();
        desc.Format.FileFormatSize = in->ReadInt32();
        desc.Format.Flags = in->ReadInt32();
        desc.Format.EnvInfoOffset = in->ReadInt32();
        desc.Format.UserDescOffset = in->ReadInt32();
        desc.Format.GameDataOffset = in->ReadInt32();
    }

    // If env info offset is valid, then skip right to env info
    if (desc.Format.EnvInfoOffset > 0)
        in->Seek(desc.Format.EnvInfoOffset, kSeekBegin);

    // Enviroment information
    uint32_t env_info_size = 0u;
    const soff_t env_info_pos = in->GetPosition();
    if (svg_ver >= kSvgVersion_351)
        env_info_size = in->ReadInt32(); // header size
    desc.EngineName = StrUtil::ReadString(in);
    desc.EngineVersion.SetFromString(StrUtil::ReadString(in));
    desc.GameGuid = StrUtil::ReadString(in);
    desc.GameTitle = StrUtil::ReadString(in);
    desc.MainDataFilename = StrUtil::ReadString(in);
    if (svg_ver >= kSvgVersion_ComponentsEx)
        desc.MainDataVersion = (GameDataVersion)in->ReadInt32();
    desc.ColorDepth = in->ReadInt32();
    if (svg_ver >= kSvgVersion_351)
        desc.LegacyID = in->ReadInt32();

    // If user desc offset is valid, then skip any remaining part
    // (this is in case there is some data that we do not support)
    soff_t user_desc_pos = in->GetPosition();
    if (svg_ver >= kSvgVersion_351)
    {
        user_desc_pos = (desc.Format.UserDescOffset > 0u) ?
            desc.Format.UserDescOffset : (env_info_pos + env_info_size);
        in->Seek(user_desc_pos, kSeekBegin);
    }

    // User description
    if (elems & kSvgDesc_UserText)
        desc.UserText = StrUtil::ReadString(in);
    else
        StrUtil::SkipString(in);
    //
    if (elems & kSvgDesc_UserImage)
        desc.UserImage.reset(RestoreUserImage(in));
    else
        SkipUserImage(in);

    // Assign backward-compatible file format values
    if (!has_fileformat)
    {
        desc.Format.Flags = 0u;
        desc.Format.EnvInfoOffset = env_info_pos;
        desc.Format.UserDescOffset = user_desc_pos;
        desc.Format.GameDataOffset = in->GetPosition();
    }

    // Skip directly to the game state data
    // (this is in case there is some data that we do not support)
    if (has_fileformat)
    {
        in->Seek(desc.Format.GameDataOffset, kSeekBegin);
    }

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

    if (sig_ver < kSvgVersion_Components)
    {
        return new SavegameError(kSvgErr_FormatVersionNotSupported, String::FromFormat("Engine no longer supports pre-3.5.0 saves."));
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
        src->Format = temp_desc.Format;
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
        if (elems & kSvgDesc_FileFormat)
            desc->Format = temp_desc.Format;
        if (elems & kSvgDesc_UserText)
            desc->UserText = temp_desc.UserText;
        if (elems & kSvgDesc_UserImage)
            desc->UserImage = std::move(temp_desc.UserImage);
    }
    return err;
}

bool DoesSavegameExist(const String &filename)
{
    return File::IsFile(filename);
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
    pp.GlScDataSize = gamescript->GetGlobalData().size();
    pp.ScriptModuleNames.resize(numScriptModules);
    pp.ScMdDataSize.resize(numScriptModules);
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        pp.ScriptModuleNames[i] = scriptModules[i]->GetScriptName();
        pp.ScMdDataSize[i] = scriptModules[i]->GetGlobalData().size();
    }

    // TODO: investigate if we actually have to do this when restoring a save
    UnlinkAllScripts();

    // reset saved room states
    ResetRoomStates();
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

void FillPreservedParams(PreservedParams &pp)
{
    // preserve script data sizes
    pp.GlScDataSize = gamescript->GetGlobalData().size();
    pp.ScriptModuleNames.resize(numScriptModules);
    pp.ScMdDataSize.resize(numScriptModules);
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        pp.ScriptModuleNames[i] = scriptModules[i]->GetScriptName();
        pp.ScMdDataSize[i] = scriptModules[i]->GetGlobalData().size();
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
        if (chan_info.ClipID < 0 && chan_info.FileName.IsEmpty())
            continue;

        if (chan_info.ClipID >= 0)
        {
            if ((size_t)chan_info.ClipID >= game.audioClips.size())
            {
                return new SavegameError(kSvgErr_GameObjectInitFailed,
                                         String::FromFormat("Invalid audio clip index: %d (clip count: %zu).", chan_info.ClipID, game.audioClips.size()));
            }

            int audio_type = chan_info.AudioType != AUDIOTYPE_UNDEFINED ? chan_info.AudioType : game.audioClips[chan_info.ClipID].type;
            play_audio_clip(AudioPlayback(&game.audioClips[chan_info.ClipID], audio_type), i,
                chan_info.Priority, chan_info.Repeat, chan_info.Pos);
        }
        else
        {
            auto sound = load_sound_clip(chan_info.FileName, chan_info.BundlingType, chan_info.Repeat);
            if (sound)
                play_sound_on_channel(std::move(sound), i, chan_info.Priority, chan_info.Repeat, chan_info.Pos);
        }

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
        view->SetShader(view_dat.ShaderID, view_dat.ShaderHandle);
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
        dspr->slot = 0; // 0 is a "safety placeholder" sprite id
    }
}

// Call a scripting event to let user validate the restored save
static HSaveError ValidateRestoredSave(const SavegameDescription &save_desc, const RestoredData &r_data, SaveRestoreFeedback &feedback)
{
    auto *saveinfo = new ScriptRestoredSaveInfo(
        (SaveRestorationFlags)(r_data.Result.RestoreFlags & kSaveRestore_ResultMask),
        save_desc, r_data.DataCounts,
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
    // Fixup game options for older games, in case the save was made by a advanced engine
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
        over.SetImage(std::move(over_im.second), over.GetOffsetX(), over.GetOffsetY());
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

    if (!LinkGlobalScripts())
    {
        return new SavegameError(kSvgErr_GameObjectInitFailed,
            String::FromFormat("Failed to link global script: %s",
                cc_get_error().ErrorString.GetCStr()));
    }

    // read the global data into the newly created script
    if (!r_data.GlobalScript.Data.empty())
    {
        gamescript->CopyGlobalData(r_data.GlobalScript.Data);
    }

    // restore the script module data
    for (auto &sc_entry : r_data.ScriptModules)
    {
        const String &name = sc_entry.first;
        auto &scdata = sc_entry.second;
        if (scdata.Data.empty())
            continue;
        for (auto &scmodule : scriptModules)
        {
            if (name.Compare(scmodule->GetScriptName()) == 0)
            {
                scmodule->CopyGlobalData(scdata.Data);
                break;
            }
        }
    }

    setup_player_character(game.playercharacter);

    // Save some parameters to restore them after room load
    const int gstimer = play.gscript_timer;
    const Rect mouse_bounds = play.mbounds;
    // load the room the game was saved in
    displayed_room = r_data.Room;
    if (displayed_room >= 0)
    {
        load_new_room(displayed_room, nullptr);
    }
    else
    {
        set_room_placeholder();
    }

    // Apply restored RTTI placeholder, after current room was loaded;
    // remap typeids in the deserialized managed objects, where necessary
    std::unordered_map<uint32_t, uint32_t> loc_l2g, type_l2g;
    RuntimeScript::JoinRTTI(r_data.GenRTTI, loc_l2g, type_l2g);
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
                // Blit, don't replace image, in case we restored a image of different size
                thisroom.BgFrames[i].Graphic->Blit(r_data.RoomBkgScene[i].get());
            }
        }

        for (int i = 0; i < kNumRoomAreaTypes; ++i)
        {
            if (r_data.RoomMask[i])
            {
                // Blit, don't replace image, in case we restored a image of different size
                thisroom.SetMask(static_cast<RoomAreaMask>(i), r_data.RoomMask[i].get());
            }
        }

        in_new_room = kEnterRoom_RestoredSave;  // don't run "enters screen" events
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

    RestoreShaders();

    adjust_fonts_for_render_mode(game.options[OPT_ANTIALIASFONTS] != 0);

    restore_characters();
    restore_movelists();
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

    // After all of the game logical state is initialized and reapplied values from save,
    // call a "validate" script callback, to let user check the restored save
    // and make final decision: whether to continue with the game, or cancel and quit.
    HSaveError validate_err = ValidateRestoredSave(pp.Desc, r_data, r_data.Result.Feedback);
    if (!validate_err)
        return validate_err;

    // Run optional plugin event, reporting game restored
    pl_run_plugin_hooks(kPluginEvt_PostRestoreGame, 0);

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

HSaveError RestoreGameState(Stream *in, SavegameVersion save_ver, const SavegameDescription &desc,
                            const RestoreGameStateOptions &options, SaveRestoreFeedback &feedback)
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

    HSaveError err = SavegameComponents::ReadAll(in, save_ver, select_cmp, pp, r_data);
    feedback = r_data.Result.Feedback;
    if (!err)
        return err;
    return DoAfterRestore(pp, r_data, select_cmp);
}

HSaveError PrescanSaveState(Stream *in, SavegameVersion save_ver, const SavegameDescription &desc,
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

    HSaveError err = SavegameComponents::PrescanAll(in, save_ver, select_cmp, pp, r_data);
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

HSaveError ReadSaveDescription(const String &filename, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegame(filename, desc, elems);
}

HSaveError RestoreSavegame(const String &filename, const RestoreGameStateOptions &options, SaveRestoreFeedback &feedback)
{
    SavegameSource src;
    SavegameDescription desc;
    HSaveError err = OpenSavegame(filename, src, desc, (SavegameDescElem)(kSvgDesc_FileFormat | kSvgDesc_EnvInfo));
    if (!err)
        return err;

    err = RestoreGameState(src.InputStream.get(), src.Version, desc, options, feedback);
    return err;
}

void WriteUserImage(Stream *out, const Bitmap *screenshot, bool compress)
{
    uint32_t flags = (screenshot != nullptr) * kSvgImage_Present
        | compress * kSvgImage_Deflate;
    out->WriteInt32(flags);

    if (screenshot)
        WriteBitmap(screenshot, out, compress);
}

void WriteFileFormat(Stream *out, const SavegameFileFormat &format)
{
    if (format.FileFormatOffset > 0)
        out->Seek(format.FileFormatOffset, kSeekBegin);

    out->WriteInt32(format.FileFormatSize);
    out->WriteInt32(format.Flags);
    out->WriteInt32(format.EnvInfoOffset);
    out->WriteInt32(format.UserDescOffset);
    out->WriteInt32(format.GameDataOffset);
}

void WriteDescription(Stream *out, const String &user_text, const Bitmap *user_image, SavegameFileFormat &format)
{
    // Data format version
    out->WriteInt32(kSvgVersion_Current);
    soff_t fileformat_pos = out->GetPosition();
    WriteFileFormat(out, SavegameFileFormat()); // write placeholder
    // Enviroment information
    soff_t env_info_pos = out->GetPosition();
    out->WriteInt32(0); // size placeholder
    // FIXME: pass as argument, do not reference global game objects here!
    StrUtil::WriteString(get_engine_name(), out);
    StrUtil::WriteString(EngineVersion.LongString, out);
    StrUtil::WriteString(game.guid, out);
    StrUtil::WriteString(game.gamename, out);
    StrUtil::WriteString(ResPaths.GamePak.Name, out);
    out->WriteInt32(loaded_game_file_version);
    out->WriteInt32(game.GetColorDepth());
    out->WriteInt32(game.uniqueid);
    // Write env info offset field
    soff_t env_info_end_pos = out->GetPosition();
    out->Seek(env_info_pos, kSeekBegin);
    out->WriteInt32(env_info_end_pos - env_info_pos);
    out->Seek(env_info_end_pos, kSeekBegin);
    // User description
    soff_t user_desc_pos = out->GetPosition();
    StrUtil::WriteString(user_text, out);
    WriteUserImage(out, user_image, true /* always compress screenshots */);

    // Fill few known fields for SavegameFileFormat
    format.FileFormatOffset = fileformat_pos;
    format.FileFormatSize = env_info_pos - fileformat_pos;
    format.EnvInfoOffset = env_info_pos;
    format.UserDescOffset = user_desc_pos;
}

std::unique_ptr<Stream> StartSavegame(const String &filename, const String &user_text, const Bitmap *user_image,
                                      SavegameFileFormat &format)
{
    auto out = File::CreateFile(filename);
    if (!out)
        return nullptr;

    // Savegame signature
    out->Write(SavegameSource::Signature.GetCStr(), SavegameSource::Signature.GetLength());
    // Write description block
    WriteDescription(out.get(), user_text, user_image, format);
    return out;
}

void DoBeforeSave()
{
    if (displayed_room >= 0)
    {
        // update the current room script's data segment copy
        if (roomscript)
            save_room_data_segment();
    }
}

void SaveGameState(Stream *out, SaveCmpSelection select_cmp, bool compress)
{
    select_cmp = FixupCmpSelection(select_cmp);

    DoBeforeSave();
    SavegameComponents::WriteAllCommon(out, select_cmp, compress);
}

HSaveError ReadPluginSaveData(Stream *in, PluginSvgVersion svg_ver, soff_t max_size)
{
    const soff_t start_pos = in->GetPosition();
    const soff_t end_pos = start_pos + max_size;

    if (svg_ver >= kPluginSvgVersion_36115)
    {
        // IMPORTANT: we have to use an intermediate vector here for plugin data,
        // because we need to safeguard a stream section, but the savegame stream is
        // restricted from Seeking, as it may be doing decompression right in memory.
        std::vector<uint8_t> plugin_data;
        int num_plugins_read = in->ReadInt32();
        soff_t cur_pos = start_pos;
        while ((num_plugins_read--) > 0 && (cur_pos < end_pos))
        {
            String pl_name = StrUtil::ReadString(in);
            size_t data_size = in->ReadInt32();
            soff_t data_start = in->GetPosition();

            try
            {
                // Allocate enough to read data_size, but then trim if read less
                plugin_data.resize(data_size);
                plugin_data.resize(in->Read(plugin_data.data(), data_size));
                auto guard_stream = std::make_unique<Stream>(
                    std::make_unique<VectorStream>(plugin_data, kStream_Read));
                int32_t fhandle = add_file_stream(std::move(guard_stream), "RestoreGame");
                pl_run_plugin_hook_by_name(pl_name, kPluginEvt_RestoreGame, fhandle);
                close_file_stream(fhandle, "RestoreGame");
                guard_stream = nullptr;
            }
            catch (std::runtime_error ex)
            {
                return new SavegameError(kSvgErr_InternalError, String(ex.what()));
            }

            // Read-out until the end of plugin data, in case it ended up reading not in the end
            cur_pos = data_start + data_size;
            in->ReadByteCount(cur_pos - in->GetPosition());
        }
    }
    else
    {
        // NOTE: we assume that the old save format won't be found in a compressed save,
        // so we can use StreamSection here. Fix the code below if this assumption becomes incorrect.
        String pl_name;
        for (uint32_t pl_index = 0; pl_query_next_plugin_for_event(kPluginEvt_RestoreGame, pl_index, pl_name); ++pl_index)
        {
            try
            {
                auto guard_stream = std::make_unique<Stream>(
                    std::make_unique<StreamSection>(in->GetStreamBase(), in->GetPosition(), end_pos));
                int32_t fhandle = add_file_stream(std::move(guard_stream), "RestoreGame");
                pl_run_plugin_hook_by_index(pl_index, kPluginEvt_RestoreGame, fhandle);
                close_file_stream(fhandle, "RestoreGame");
            }
            catch (std::runtime_error ex)
            {
                return new SavegameError(kSvgErr_InternalError, String(ex.what()));
            }
        }
    }

    return HSaveError::None();
}

HSaveError WritePluginSaveData(Stream *out)
{
    uint32_t num_plugins_wrote = 0;
    String pl_name;
    for (uint32_t pl_index = 0; pl_query_next_plugin_for_event(kPluginEvt_SaveGame, pl_index, pl_name); ++pl_index)
    {
        // NOTE: we don't care if they really write anything,
        // but count them so long as they subscribed to AGSE_SAVEGAME
        num_plugins_wrote++;
    }
    out->WriteInt32(num_plugins_wrote); // number of plugins which wrote data

    // IMPORTANT: we have to use an intermediate vector here for plugin data,
    // because we need to prepend its size, but the savegame stream is
    // restricted from Seeking, as it may be doing compression right in memory.
    std::vector<uint8_t> plugin_data;
    for (uint32_t pl_index = 0; pl_query_next_plugin_for_event(kPluginEvt_SaveGame, pl_index, pl_name); ++pl_index)
    {
        // Create a stream section and write plugin data
        // FIXME: implement a size limit for plugin data:
        // * support a limit in VectorStream
        // * define a reasonable default limit (e.g. 16-32 MB)
        // * compare with system memory?
        plugin_data.clear();

        try
        {
            auto guard_stream = std::make_unique<Stream>(
                std::make_unique<VectorStream>(plugin_data, kStream_Write));
            int32_t fhandle = add_file_stream(std::move(guard_stream), "SaveGame");
            pl_run_plugin_hook_by_index(pl_index, kPluginEvt_SaveGame, fhandle);
            close_file_stream(fhandle, "SaveGame");
            guard_stream = nullptr;
        }
        catch (std::runtime_error ex)
        {
            return new SavegameError(kSvgErr_InternalError, String(ex.what()));
        }

        // Write a header for plugin data
        StrUtil::WriteString(pl_name, out);
        out->WriteInt32(plugin_data.size()); // data size
        out->Write(plugin_data.data(), plugin_data.size());
    }

    return HSaveError::None();
}

HSaveError SaveGame(const String &filename, const String &user_text, const Bitmap *user_image,
                    SaveCmpSelection select_cmp, bool compress_data)
{
    SavegameFileFormat format;
    format.Flags = kSvgFmt_DeflateComponents * compress_data;
    std::unique_ptr<Stream> out(StartSavegame(filename, user_text, user_image, format));
    if (!out)
        return new SavegameError(kSvgErr_FileOpenFailed, String::FromFormat("Requested filename: %s.", filename.GetCStr()));

    format.GameDataOffset = out->GetPosition();
    SaveGameState(out.get(), select_cmp, compress_data);

    // Finalize the save file, write composed file format
    WriteFileFormat(out.get(), format);
    return HSaveError::None();
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

bool SaveInfo_HasExtraData(ScriptRestoredSaveInfo *info)
{
    return (info->GetResult() & kSaveRestore_ExtraDataInSave) != 0;
}

bool SaveInfo_HasMissingData(ScriptRestoredSaveInfo *info)
{
    return (info->GetResult() & kSaveRestore_MissingDataInSave) != 0;
}

bool SaveInfo_IsPrescan(ScriptRestoredSaveInfo *info)
{
    return (info->GetResult() & kSaveRestore_Prescan) != 0;
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

const char *SaveInfo_GetGameVersion(ScriptRestoredSaveInfo *info)
{
    return CreateNewScriptString(
        (info->GetCounts().GameInfo.count("version") > 0) ?
        info->GetCounts().GameInfo.at("version").GetCStr()
        : "");
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

const char *SaveInfo_GetScriptModuleNames(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ScriptModuleDataSz.size())
    {
        debug_script_warn("RestoredSaveInfo::ScriptModuleNames: index %d out of bounds (%d..%d)", index, 0, info->GetCounts().ScriptModules);
        return 0;
    }
    return CreateNewScriptString(info->GetCounts().ScriptModuleNames[index]);
}

int SaveInfo_GetScriptModuleDataSizes(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ScriptModuleDataSz.size())
    {
        debug_script_warn("RestoredSaveInfo::ScriptModuleDataSize: index %d out of bounds (%d..%d)", index, 0, info->GetCounts().ScriptModules);
        return 0;
    }
    return info->GetCounts().ScriptModuleDataSz[index];
}

int SaveInfo_GetRoom(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Room;
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

RuntimeScriptValue Sc_SaveInfo_HasExtraData(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_HasExtraData);
}

RuntimeScriptValue Sc_SaveInfo_HasMissingData(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_HasMissingData);
}

RuntimeScriptValue Sc_SaveInfo_IsPrescan(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_IsPrescan);
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

RuntimeScriptValue Sc_SaveInfo_GetGameVersion(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetGameVersion);
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

RuntimeScriptValue Sc_SaveInfo_GetScriptModuleNames(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetScriptModuleNames);
}

RuntimeScriptValue Sc_SaveInfo_GetScriptModuleDataSizes(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetScriptModuleDataSizes);
}

RuntimeScriptValue Sc_SaveInfo_GetRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetRoom);
}

void RegisterSaveInfoAPI()
{
    ScFnRegister saveinfo_api[] = {
        { "RestoredSaveInfo::get_Cancel",               API_FN_PAIR(SaveInfo_GetCancel) },
        { "RestoredSaveInfo::set_Cancel",               API_FN_PAIR(SaveInfo_SetCancel) },
        { "RestoredSaveInfo::get_RetryWithoutComponents", API_FN_PAIR(SaveInfo_GetRetryWithoutComponents) },
        { "RestoredSaveInfo::set_RetryWithoutComponents", API_FN_PAIR(SaveInfo_SetRetryWithoutComponents) },
        { "RestoredSaveInfo::get_IsPrescan",            API_FN_PAIR(SaveInfo_IsPrescan) },
        { "RestoredSaveInfo::get_HasExtraData",         API_FN_PAIR(SaveInfo_HasExtraData) },
        { "RestoredSaveInfo::get_HasMissingData",       API_FN_PAIR(SaveInfo_HasMissingData) },
        { "RestoredSaveInfo::get_Slot",                 API_FN_PAIR(SaveInfo_GetSlot) },
        { "RestoredSaveInfo::get_Description",          API_FN_PAIR(SaveInfo_GetDescription) },
        { "RestoredSaveInfo::get_EngineVersion",        API_FN_PAIR(SaveInfo_GetEngineVersion) },
        { "RestoredSaveInfo::get_GameVersion",          API_FN_PAIR(SaveInfo_GetGameVersion) },
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
        { "RestoredSaveInfo::geti_ScriptModuleNames",   API_FN_PAIR(SaveInfo_GetScriptModuleNames) },
        { "RestoredSaveInfo::geti_ScriptModuleDataSizes",API_FN_PAIR(SaveInfo_GetScriptModuleDataSizes) },
        { "RestoredSaveInfo::get_Room",                 API_FN_PAIR(SaveInfo_GetRoom) },
    };

    ccAddExternalFunctions(saveinfo_api);
}

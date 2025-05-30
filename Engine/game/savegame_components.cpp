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
#include <map>
#include "game/savegame_components.h"
#include "ac/audiocliptype.h"
#include "ac/button.h"
#include "ac/character.h"
#include "ac/common.h"
#include "ac/dialog.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/movelist.h"
#include "ac/overlay.h"
#include "ac/roomstatus.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "ac/system.h"
#include "ac/dynobj/cc_serializer.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/out.h"
#include "game/savegame_internal.h"
#include "gfx/bitmap.h"
#include "gui/animatingguibutton.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/script.h"
#include "util/deflatestream.h"
#include "util/memory_compat.h"
#include "util/string_utils.h"

using namespace Common;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RGB palette[256];
extern std::vector<ViewStruct> views;
extern std::unique_ptr<Bitmap> dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
extern RoomStruct thisroom;
extern RoomStatus troom;


namespace AGS
{
namespace Engine
{

namespace SavegameComponents
{

//-----------------------------------------------------------------------------
//
// Helper assertion functions.
//
//-----------------------------------------------------------------------------

// Tag used to mark the beginning of a save component list
const String ComponentListTag = "Components";

// Writes a opening or closing tag for a save component
void WriteFormatTag(Stream *out, const String &tag, bool open = true)
{
    String full_tag = String::FromFormat(open ? "<%s>" : "</%s>", tag.GetCStr());
    out->Write(full_tag.GetCStr(), full_tag.GetLength());
}

// Reads a opening or closing save component tag and asserts its format
bool ReadFormatTag(Stream *in, String &tag, bool open = true)
{
    if (in->ReadByte() != '<')
        return false;
    if (!open && in->ReadByte() != '/')
        return false;
    tag.Empty();
    while (!in->EOS())
    {
        char c = in->ReadByte();
        if (c == '>')
            return true;
        tag.AppendChar(c);
    }
    return false; // reached EOS before closing symbol
}

// Reads a component tag and asserts that it matches expected name
bool AssertFormatTag(Stream *in, const String &tag, bool open = true)
{
    String read_tag;
    if (!ReadFormatTag(in, read_tag, open))
        return false;
    return read_tag.Compare(tag) == 0;
}

// Reads a component tag and asserts that it matches expected name; formats error message on failure
bool AssertFormatTagStrict(HSaveError &err, Stream *in, const String &tag, bool open = true)
{
    String read_tag;
    if (!ReadFormatTag(in, read_tag, open) || read_tag.Compare(tag) != 0)
    {
        err = new SavegameError(kSvgErr_InconsistentFormat,
            String::FromFormat("Mismatching tag: %s.", tag.GetCStr()));
        return false;
    }
    return true;
}

// Asserts that the read data count does not exceed engine capabilities
inline bool AssertCompatLimit(HSaveError &err, int count, int max_count, const char *content_name)
{
    if (count > max_count)
    {
        err = new SavegameError(kSvgErr_IncompatibleEngine,
            String::FromFormat("Incompatible number of %s (count: %d, max: %d).",
            content_name, count, max_count));
        return false;
    }
    return true;
}

// Asserts that the read data range does not exceed engine capabilities
inline bool AssertCompatRange(HSaveError &err, int value, int min_value, int max_value, const char *content_name)
{
    if (value < min_value || value > max_value)
    {
        err = new SavegameError(kSvgErr_IncompatibleEngine,
            String::FromFormat("Restore game error: incompatible %s (id: %d, range: %d - %d).",
            content_name, value, min_value, max_value));
        return false;
    }
    return true;
}

// Handles save-game mismatch; chosen action depends on SaveRestorationFlags
inline bool HandleGameContentMismatch(HSaveError &err, const uint32_t new_val, const uint32_t original_val,
    const String &error_text, SaveRestoreResult &result)
{
    SaveRestorationFlags &restore_flags = result.RestoreFlags;
    if (((new_val > original_val) && (restore_flags & kSaveRestore_AllowMismatchExtra) == 0) ||
        ((new_val < original_val) && (restore_flags & kSaveRestore_AllowMismatchLess) == 0))
    {
        err = new SavegameError(kSvgErr_GameContentAssertion, error_text);
        return false; // numbers mismatch, and not allowed to, fail
    }

    if (result.FirstMismatchError.IsEmpty())
        result.FirstMismatchError = error_text;
    Debug::Printf(kDbgMsg_Warn, "Restored save mismatches game: %s", error_text.GetCStr());

    if (new_val > original_val)
    {
        restore_flags = (SaveRestorationFlags)(restore_flags | kSaveRestore_ExtraDataInSave);
    }
    else
    {
        restore_flags = (SaveRestorationFlags)(restore_flags | kSaveRestore_MissingDataInSave);
        if ((restore_flags & kSaveRestore_ClearData) == 0)
        {
            result.Feedback.RetryWithClearGame = true;
            result.Feedback.RetryWithoutComponents = kSaveCmp_All;
            err = new SavegameError(kSvgErr_GameContentAssertion);
            return false; // mismatch is allowed, but we require a clear game data to proceed
        }
    }
    return true;
}

// Handles save-game mismatch where save has an extra object that the game does not
inline bool HandleExtraGameComponent(HSaveError &err, const char *content_name, const String &obj_name, SaveRestoreResult &result)
{
    const String error_text = String::FromFormat("Extra %s found in save that does not exist in the game: %s.", content_name, obj_name.GetCStr());
    return HandleGameContentMismatch(err, 1, 0, error_text, result);
}

// Handles save-game mismatch where save is missing an object that the game has
inline bool HandleMissingGameComponent(HSaveError &err, const char *content_name, const String &obj_name, SaveRestoreResult &result)
{
    const String error_text = String::FromFormat("Missing %s in save that exists in the game: %s.", content_name, obj_name.GetCStr());
    return HandleGameContentMismatch(err, 0, 1, error_text, result);
}

// Tests a match between game's and save's data count, handles mismatch using SaveRestorationFlags
inline bool AssertGameContent(HSaveError &err, const uint32_t new_val, const uint32_t original_val, const char *content_name,
    SaveRestoreResult &result, uint32_t &record_count)
{
    record_count = new_val;
    if (new_val == original_val)
        return true; // numbers match, success

    const String error_text = String::FromFormat("Mismatching number of %s (game: %u, save: %u).",
        content_name, original_val, new_val);
    return HandleGameContentMismatch(err, new_val, original_val, error_text, result);
}

// Tests a match between game's and save's data count, uses default mismatch handling (always error)
inline bool AssertGameContent(HSaveError &err, const uint32_t new_val, const uint32_t original_val, const char *content_name)
{
    SaveRestoreResult dummy_result;
    uint32_t dummy_count;
    return AssertGameContent(err, new_val, original_val, content_name, dummy_result, dummy_count);
}

// Tests a match between game's and save's data count, handles mismatch using SaveRestorationFlags
inline bool AssertGameObjectContent(HSaveError &err, const uint32_t new_val, const uint32_t original_val, const char *content_name,
                                    const char *obj_type, const uint32_t obj_id,
                                    SaveRestoreResult &result, uint32_t &record_count)
{
    record_count = new_val;
    if (new_val == original_val)
        return true; // numbers match, success

    const String error_text = String::FromFormat("Mismatching number of %s, %s #%u (game: %u, save: %u).",
        content_name, obj_type, obj_id, original_val, new_val);
    return HandleGameContentMismatch(err, new_val, original_val, error_text, result);
}

// Tests a match between game's and save's data count, handles mismatch using SaveRestorationFlags
inline bool AssertGameObjectContent2(HSaveError &err, const uint32_t new_val, const uint32_t original_val, const char *content_name,
                                    const char *obj1_type, const uint32_t obj1_id, const char *obj2_type, const uint32_t obj2_id,
                                    SaveRestoreResult &result, uint32_t &record_count)
{
    record_count = new_val;
    if (new_val == original_val)
        return true; // numbers match, success

    const String error_text = String::FromFormat("Mismatching number of %s, %s #%u, %s #%u (game: %u, save: %u).",
        content_name, obj1_type, obj1_id, obj2_type, obj2_id, original_val, new_val);
    return HandleGameContentMismatch(err, new_val, original_val, error_text, result);
}

//-----------------------------------------------------------------------------
//
// Save components writers and readers.
//
//-----------------------------------------------------------------------------

void WriteCameraState(const Camera &cam, Stream *out)
{
    int flags = 0;
    if (cam.IsLocked()) flags |= kSvgCamPosLocked;
    out->WriteInt32(flags);
    const Rect &rc = cam.GetRect();
    out->WriteInt32(rc.Left);
    out->WriteInt32(rc.Top);
    out->WriteInt32(rc.GetWidth());
    out->WriteInt32(rc.GetHeight());
    // kGSSvgVersion_400_18
    out->WriteInt32(cam.GetShaderID());
    out->WriteInt32(cam.GetShaderHandle());
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
}

void WriteViewportState(const Viewport &view, Stream *out)
{
    int flags = 0;
    if (view.IsVisible()) flags |= kSvgViewportVisible;
    out->WriteInt32(flags);
    const Rect &rc = view.GetRect();
    out->WriteInt32(rc.Left);
    out->WriteInt32(rc.Top);
    out->WriteInt32(rc.GetWidth());
    out->WriteInt32(rc.GetHeight());
    out->WriteInt32(view.GetZOrder());
    auto cam = view.GetCamera();
    if (cam)
        out->WriteInt32(cam->GetID());
    else
        out->WriteInt32(-1);
    // kGSSvgVersion_400_18
    out->WriteInt32(view.GetShaderID());
    out->WriteInt32(view.GetShaderHandle());
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
}

HSaveError WriteGameState(Stream *out)
{
    // Game base
    game.WriteForSavegame(out);
    // Game palette
    // TODO: probably no need to save this for hi/true-res game
    out->WriteArray(palette, sizeof(RGB), 256);

    // Game state
    play.WriteForSavegame(out);
    // Other dynamic values
    out->WriteInt32(frames_per_second);
    out->WriteInt32(get_loop_counter());
    out->WriteInt32(ifacepopped);
    out->WriteInt32(game_paused);
    // Mouse cursor
    out->WriteInt32(cur_mode);
    out->WriteInt32(cur_cursor);
    out->WriteInt32(mouse_on_iface);

    // Viewports and cameras
    int viewcam_flags = 0;
    if (play.IsAutoRoomViewport())
        viewcam_flags |= kSvgGameAutoRoomView;
    out->WriteInt32(viewcam_flags);
    out->WriteInt32(play.GetRoomCameraCount());
    for (int i = 0; i < play.GetRoomCameraCount(); ++i)
        WriteCameraState(*play.GetRoomCamera(i), out);
    out->WriteInt32(play.GetRoomViewportCount());
    for (int i = 0; i < play.GetRoomViewportCount(); ++i)
        WriteViewportState(*play.GetRoomViewport(i), out);

    return HSaveError::None();
}

void ReadCameraState(GameStateSvgVersion svg_ver, RestoredData &r_data, Stream *in)
{
    RestoredData::CameraData cam;
    cam.ID = r_data.Cameras.size();
    cam.Flags = in->ReadInt32();
    cam.Left = in->ReadInt32();
    cam.Top = in->ReadInt32();
    cam.Width = in->ReadInt32();
    cam.Height = in->ReadInt32();
    if (svg_ver >= kGSSvgVersion_400_18)
    {
        cam.ShaderID = in->ReadInt32();
        cam.ShaderHandle = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
    }
    r_data.Cameras.push_back(cam);
}

void ReadViewportState(GameStateSvgVersion svg_ver, RestoredData &r_data, Stream *in)
{
    RestoredData::ViewportData view;
    view.ID = r_data.Viewports.size();
    view.Flags = in->ReadInt32();
    view.Left = in->ReadInt32();
    view.Top = in->ReadInt32();
    view.Width = in->ReadInt32();
    view.Height = in->ReadInt32();
    view.ZOrder = in->ReadInt32();
    view.CamID = in->ReadInt32();
    if (svg_ver >= kGSSvgVersion_400_18)
    {
        view.ShaderID = in->ReadInt32();
        view.ShaderHandle = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
    }
    r_data.Viewports.push_back(view);
}

HSaveError ReadGameState(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    GameStateSvgVersion svg_ver = (GameStateSvgVersion)cmp_ver;
    // Game base
    game.ReadFromSavegame(in);
    // Game palette
    in->ReadArray(palette, sizeof(RGB), 256);

    // Game state
    play.ReadFromSavegame(in, loaded_game_file_version, svg_ver, r_data);

    // Other dynamic values
    r_data.FPS = in->ReadInt32();
    set_loop_counter(in->ReadInt32());
    ifacepopped = in->ReadInt32();
    game_paused = in->ReadInt32();
    // Mouse cursor state
    r_data.CursorMode = in->ReadInt32();
    r_data.CursorID = in->ReadInt32();
    mouse_on_iface = in->ReadInt32();

    // Viewports and cameras
    {
        int viewcam_flags = in->ReadInt32();
        play.SetAutoRoomViewport((viewcam_flags & kSvgGameAutoRoomView) != 0);
        // TODO: we create viewport and camera objects here because they are
        // required for the managed pool deserialization, but read actual
        // data into temp structs because we need to apply it after active
        // room is loaded.
        // See comments to RestoredData struct for further details.
        int cam_count = in->ReadInt32();
        for (int i = 0; i < cam_count; ++i)
        {
            play.CreateRoomCamera();
            ReadCameraState(svg_ver, r_data, in);
        }
        int view_count = in->ReadInt32();
        for (int i = 0; i < view_count; ++i)
        {
            play.CreateRoomViewport();
            ReadViewportState(svg_ver, r_data, in);
        }
    }
    return err;
}

// Savegame data format for Audio state
enum AudioSvgVersion
{
    kAudioSvgVersion_Initial  = 0,
    kAudioSvgVersion_35026    = 1, // source position settings
    kAudioSvgVersion_36009    = 2, // up number of channels
    kAudioSvgVersion_36130    = 3060130, // playback state
};

HSaveError WriteAudio(Stream *out)
{
    // Game content assertion
    out->WriteInt32(game.audioClipTypes.size());
    out->WriteInt8(TOTAL_AUDIO_CHANNELS);
    out->WriteInt8(game.numGameChannels);
    out->WriteInt16(0); // reserved 2 bytes (remains of int32)
    // Audio types
    for (size_t i = 0; i < game.audioClipTypes.size(); ++i)
    {
        game.audioClipTypes[i].WriteToSavegame(out);
        out->WriteInt32(play.default_audio_type_volumes[i]);
    }

    // Audio clips and crossfade
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; i++)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        if ((ch != nullptr) && (ch->sourceClipID >= 0))
        {
            out->WriteInt32(ch->sourceClipID);
            out->WriteInt32(ch->get_pos());
            out->WriteInt32(ch->priority);
            out->WriteInt32(ch->repeat ? 1 : 0);
            out->WriteInt32(ch->get_volume255());
            out->WriteInt32(0); // was redundant data
            out->WriteInt32(ch->get_volume100());
            out->WriteInt32(ch->get_panning());
            out->WriteInt32(ch->get_speed());
            // since version kAudioSvgVersion_35026
            out->WriteInt32(ch->xSource);
            out->WriteInt32(ch->ySource);
            out->WriteInt32(ch->maximumPossibleDistanceAway);
            // since version kAudioSvgVersion_36130
            int playback_flags = 0;
            if (ch->is_paused())
                playback_flags |= kSvgAudioPaused;
            out->WriteInt32(playback_flags);
            out->WriteInt32(0); // reserved 3 ints
            out->WriteInt32(0);
            out->WriteInt32(0);
        }
        else
        {
            out->WriteInt32(-1);
        }
    }
    out->WriteInt32(0); // DEPRECATED: legacy crossfade params
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0); // DEPRECATED current_music_type

    // Skip legacy Ambient sounds
    // FIXME: CLNUP save format?
    for (int i = 0; i < game.numGameChannels; ++i)
    {
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
    return HSaveError::None();
}

HSaveError ReadAudio(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    // Game content assertion
    const uint32_t audiocliptype_read = in->ReadInt32();
    if (!AssertGameContent(err, audiocliptype_read, game.audioClipTypes.size(), "Audio Clip Types", r_data.Result, r_data.DataCounts.AudioClipTypes))
        return err;

    int total_channels, max_game_channels;
    if (cmp_ver >= kAudioSvgVersion_36009)
    {
        total_channels = in->ReadInt8();
        max_game_channels = in->ReadInt8();
        in->ReadInt16(); // reserved 2 bytes
        if (!AssertCompatLimit(err, total_channels, TOTAL_AUDIO_CHANNELS, "System Audio Channels") ||
            !AssertCompatLimit(err, max_game_channels, MAX_GAME_CHANNELS, "Game Audio Channels"))
            return err;
    }
    else
    { /* DEPRECATED */
        in->ReadInt32(); // unused in prev format ver
    }

    // Audio types
    for (uint32_t i = 0; i < audiocliptype_read; ++i)
    {
        game.audioClipTypes[i].ReadFromSavegame(in);
        play.default_audio_type_volumes[i] = in->ReadInt32();
    }

    // Active playbacks and crossfade
    for (int i = 0; i < total_channels; ++i)
    {
        RestoredData::ChannelInfo &chan_info = r_data.AudioChans[i];
        chan_info.Pos = 0;
        chan_info.ClipID = in->ReadInt32();
        if (chan_info.ClipID >= 0)
        {
            chan_info.Pos = in->ReadInt32();
            if (chan_info.Pos < 0)
                chan_info.Pos = 0;
            chan_info.Priority = in->ReadInt32();
            chan_info.Repeat = in->ReadInt32();
            chan_info.Vol = in->ReadInt32();
            in->ReadInt32(); // was redundant data
            chan_info.VolAsPercent = in->ReadInt32();
            chan_info.Pan = in->ReadInt32();
            chan_info.Speed = 1000;
            chan_info.Speed = in->ReadInt32();
            if (cmp_ver >= kAudioSvgVersion_35026)
            {
                chan_info.XSource = in->ReadInt32();
                chan_info.YSource = in->ReadInt32();
                chan_info.MaxDist = in->ReadInt32();
            }
            if (cmp_ver >= kAudioSvgVersion_36130)
            {
                chan_info.Flags = in->ReadInt32();
                in->ReadInt32(); // reserved 3 ints
                in->ReadInt32();
                in->ReadInt32();
            }
        }
    }
    in->ReadInt32(); // DEPRECATED: legacy crossfade params
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32(); // DEPRECATED current_music_type
    
    // Skip legacy Ambient sounds
    // FIXME: CLNUP save format?
    for (int i = 0; i < max_game_channels; ++i)
    {
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
    }
    return err;
}

HSaveError PrescanAudio(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    const uint32_t audiocliptype_read = in->ReadInt32();
    if (!AssertGameContent(err, audiocliptype_read, game.audioClipTypes.size(), "Audio Clip Types", r_data.Result, r_data.DataCounts.AudioClipTypes))
        return err;

    int total_channels, max_game_channels;
    if (cmp_ver >= kAudioSvgVersion_36009)
    {
        total_channels = in->ReadInt8();
        max_game_channels = in->ReadInt8();
        if (!AssertCompatLimit(err, total_channels, TOTAL_AUDIO_CHANNELS, "System Audio Channels") ||
            !AssertCompatLimit(err, max_game_channels, MAX_GAME_CHANNELS, "Game Audio Channels"))
            return err;
    }
    return HSaveError::None();
}

HSaveError WriteCharacters(Stream *out)
{
    out->WriteInt32(game.numcharacters);
    for (int i = 0; i < game.numcharacters; ++i)
    {
        game.chars[i].WriteToSavegame(out);
        charextra[i].WriteToSavegame(out);
        Properties::WriteValues(play.charProps[i], out);
    }
    return HSaveError::None();
}

HSaveError ReadCharacters(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    HSaveError err;
    const uint32_t characters_read = in->ReadInt32();
    if (!AssertGameContent(err, characters_read, game.numcharacters, "Characters", r_data.Result, r_data.DataCounts.Characters))
        return err;

    const CharacterSvgVersion svg_ver = static_cast<CharacterSvgVersion>(cmp_ver);
    for (uint32_t i = 0; i < characters_read; ++i)
    {
        auto &chi = game.chars[i];
        auto &chex = charextra[i];
        chi.ReadFromSavegame(in, svg_ver);
        chex.ReadFromSavegame(in, svg_ver);
        Properties::ReadValues(play.charProps[i], in);

        // Upgrade legacy turning mask
        if (svg_ver < kCharSvgVersion_400_16)
        {
            int turns = (chi.walking % LEGACY_CHAR_TURNING_BACKWARDS) / LEGACY_CHAR_TURNING_AROUND;
            bool turn_ccw = chi.walking % LEGACY_CHAR_TURNING_BACKWARDS;
            if (turns > 0)
            {
                chex.SetTurning(true, turn_ccw, turns);
            }
            chi.walking = chi.walking % LEGACY_CHAR_TURNING_AROUND;
        }
    }
    return err;
}

HSaveError PrescanCharacters(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    const uint32_t characters_read = in->ReadInt32();
    if (!AssertGameContent(err, characters_read, game.numcharacters, "Characters", r_data.Result, r_data.DataCounts.Characters))
        return err;
    return HSaveError::None();
}

HSaveError WriteDialogs(Stream *out)
{
    out->WriteInt32(game.numdialog);
    for (int i = 0; i < game.numdialog; ++i)
    {
        dialog[i].WriteToSavegame(out);
        Properties::WriteValues(play.dialogProps[i], out);
    }
    return HSaveError::None();
}

HSaveError ReadDialogs(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    HSaveError err;
    const uint32_t dialogs_read = in->ReadInt32();
    if (!AssertGameContent(err, dialogs_read, game.numdialog, "Dialogs", r_data.Result, r_data.DataCounts.Dialogs))
        return err;

    const DialogSvgVersion svg_ver = (DialogSvgVersion)cmp_ver;
    for (uint32_t i = 0; i < dialogs_read; ++i)
    {
        dialog[i].ReadFromSavegame(in, svg_ver);
        if (cmp_ver >= kDialogSvgVersion_40008)
            Properties::ReadValues(play.dialogProps[i], in);
    }
    return err;
}

HSaveError PrescanDialogs(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    const uint32_t dialogs_read = in->ReadInt32();
    if (!AssertGameContent(err, dialogs_read, game.numdialog, "Dialogs", r_data.Result, r_data.DataCounts.Dialogs))
        return err;
    return HSaveError::None();
}

HSaveError WriteGUI(Stream *out)
{
    // GUI state
    WriteFormatTag(out, "GUIs");
    out->WriteInt32(game.numgui);
    for (int i = 0; i < game.numgui; ++i)
    {
        guis[i].WriteToSavegame(out);
        Properties::WriteValues(play.guiProps[i], out);
    }

    WriteFormatTag(out, "GUIButtons");
    out->WriteInt32(static_cast<int32_t>(guibuts.size()));
    for (size_t i = 0; i < guibuts.size(); ++i)
    {
        guibuts[i].WriteToSavegame(out);
        Properties::WriteValues(play.guicontrolProps[kGUIButton][i], out);
    }

    WriteFormatTag(out, "GUILabels");
    out->WriteInt32(static_cast<int32_t>(guilabels.size()));
    for (size_t i = 0; i < guilabels.size(); ++i)
    {
        guilabels[i].WriteToSavegame(out);
        Properties::WriteValues(play.guicontrolProps[kGUILabel][i], out);
    }

    WriteFormatTag(out, "GUIInvWindows");
    out->WriteInt32(static_cast<int32_t>(guiinv.size()));
    for (size_t i = 0; i < guiinv.size(); ++i)
    {
        guiinv[i].WriteToSavegame(out);
        Properties::WriteValues(play.guicontrolProps[kGUIInvWindow][i], out);
    }

    WriteFormatTag(out, "GUISliders");
    out->WriteInt32(static_cast<int32_t>(guislider.size()));
    for (size_t i = 0; i < guislider.size(); ++i)
    {
        guislider[i].WriteToSavegame(out);
        Properties::WriteValues(play.guicontrolProps[kGUISlider][i], out);
    }

    WriteFormatTag(out, "GUITextBoxes");
    out->WriteInt32(static_cast<int32_t>(guitext.size()));
    for (size_t i = 0; i < guitext.size(); ++i)
    {
        guitext[i].WriteToSavegame(out);
        Properties::WriteValues(play.guicontrolProps[kGUITextBox][i], out);
    }

    WriteFormatTag(out, "GUIListBoxes");
    out->WriteInt32(static_cast<int32_t>(guilist.size()));
    for (size_t i = 0; i < guilist.size(); ++i)
    {
        guilist[i].WriteToSavegame(out);
        Properties::WriteValues(play.guicontrolProps[kGUIListBox][i], out);
    }

    // Animated buttons
    WriteFormatTag(out, "AnimatedButtons");
    size_t num_abuts = GetAnimatingButtonCount();
    out->WriteInt32(num_abuts);
    for (size_t i = 0; i < num_abuts; ++i)
        GetAnimatingButtonByIndex(i)->WriteToSavegame(out);
    return HSaveError::None();
}

// Builds a list of ranges of control indexes per type per each GUI;
// this lets to know which ranges in global flat control arrays belong to which GUI.
//
// As input this takes a list of control references from GUIMain,
//   where for each GUIMain it lists pairs of ControlType : index in its type's global array.
//
// As output this gives an array of per-Control-Type vectors,
//   where each vector holds a series of ranges [first; last),
//   telling which controls in global array belong to 0th, 1st, 2nd etc GUIMains.
void BuildGUIControlRangeReference(const std::vector<std::vector<GUIMain::ControlRef>> &gui_ctrl_refs,
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, kGUIControlTypeNum> &ctrlidx_per_gui)
{
    // Prepare index arrays for each control type
    for (int type = kGUIButton; type < kGUIControlTypeNum; ++type)
    {
        ctrlidx_per_gui[type].resize(gui_ctrl_refs.size());
    }

    // For each GUI...
    for (size_t gui_index = 0; gui_index < gui_ctrl_refs.size(); ++gui_index)
    {
        // Assign the "empty" index range starting at the previous GUI controls range
        if (gui_index > 0)
        {
            for (int type = kGUIButton; type < kGUIControlTypeNum; ++type)
                ctrlidx_per_gui[type][gui_index] = std::make_pair(
                    ctrlidx_per_gui[type][gui_index - 1].second,
                    ctrlidx_per_gui[type][gui_index - 1].second);
        }
        
        // For each child control reference of this GUI...
        for (const auto &ctrl_ref : gui_ctrl_refs[gui_index])
        {
            GUIControlType type = ctrl_ref.first;
            uint32_t index = static_cast<uint32_t>(ctrl_ref.second);
            ctrlidx_per_gui[type][gui_index].second = std::max(index + 1, ctrlidx_per_gui[type][gui_index].second);
        }
    }
}

// Moves gui controls from source array to dest array, in portions, belonging to matching GUIMains,
// using their ranges per GUI as a reference. The copied amount per GUI is min of controls in old GUI and new GUI.
template <typename TGUIControl>
void MoveGUIControlArray(const std::vector<TGUIControl> &src_arr, std::vector<TGUIControl> &dest_arr,
    const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_src,
    const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_dst)
{
    for (size_t cur_gui = 0; cur_gui < ctrlidx_per_gui_src.size() && cur_gui < ctrlidx_per_gui_dst.size(); ++cur_gui)
    {
        for (size_t src_idx = ctrlidx_per_gui_src[cur_gui].first, dst_idx = ctrlidx_per_gui_dst[cur_gui].first;
                src_idx < ctrlidx_per_gui_src[cur_gui].second &&
                src_idx < src_arr.size() &&
                dst_idx < ctrlidx_per_gui_dst[cur_gui].second &&
                dst_idx < dest_arr.size();
            ++src_idx, ++dst_idx)
        {
            dest_arr[dst_idx] = std::move(src_arr[src_idx]);
        }
    }
}

template <typename TGUIControl, GUIControlType control_type>
bool ReadGUIControlArray(std::vector<TGUIControl> &obj_arr, Stream *in, GuiSvgVersion svg_ver,
    HSaveError &err, const char *tag, const char *friendly_name, RestoredData& r_data,
    const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_old,
    const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_new)
{
    if (!AssertFormatTagStrict(err, in, tag))
        return false;
    const uint32_t guiobj_read_count = in->ReadInt32();
    if (!AssertGameContent(err, guiobj_read_count, obj_arr.size(), friendly_name, r_data.Result, r_data.DataCounts.Dummy))
        return false;

    // We read and apply controls in a 3-step way:
    // 1. Copy game's controls into the temporary array, size equal to number of elements in save.
    // 2. Read save data into controls in temporary array.
    // 3. Copy controls from temporary array back to game's array.
    //
    // Why: because not all fields are written in saves, so we first copy full fields to temp array,
    // then read dynamic fields only from a save into temp array; keep persistent fields this way.
    std::vector<TGUIControl> guiobj_read(guiobj_read_count);
    MoveGUIControlArray<TGUIControl>(obj_arr, guiobj_read, ctrlidx_per_gui_new, ctrlidx_per_gui_old);
    // Read actual saved GUI control data
    for (uint32_t i = 0; i < guiobj_read_count; ++i)
    {
        guiobj_read[i].ReadFromSavegame(in, svg_ver);
        if (svg_ver >= kGuiSvgVersion_40008)
            Properties::ReadValues(play.guicontrolProps[control_type][i], in);
    }
    MoveGUIControlArray<TGUIControl>(guiobj_read, obj_arr, ctrlidx_per_gui_old, ctrlidx_per_gui_new);
    return true;
}

HSaveError ReadGUI(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    HSaveError err;
    const GuiSvgVersion svg_ver = (GuiSvgVersion)cmp_ver;
    // GUI state
    if (!AssertFormatTagStrict(err, in, "GUIs"))
        return err;
    const uint32_t guis_read = in->ReadInt32();
    if (!AssertGameContent(err, guis_read, game.numgui, "GUIs", r_data.Result, r_data.DataCounts.GUIs))
        return err;
    std::vector<std::vector<GUIMain::ControlRef>> guictrl_refs_old(guis_read);
    for (uint32_t i = 0; i < guis_read; ++i)
    {
        guis[i].ReadFromSavegame(in, svg_ver, guictrl_refs_old[i]);
        if (svg_ver >= kGuiSvgVersion_40008)
            Properties::ReadValues(play.guiProps[i], in);
    }

    // For older save formats: copy pre-existing control ref array
    if (svg_ver < kGuiSvgVersion_36200)
    {
        for (uint32_t i = 0; i < guis_read; ++i)
            guictrl_refs_old[i] = guis[i].GetControlRefs();
    }

    r_data.DataCounts.GUIControls.resize(guis_read);

    // Build a reference of the range of control indexes per type per each GUI;
    // this lets us know which range of elements in control arrays belong to which GUI.
    // old refs are read from the save, new refs are from the current game data
    std::vector<std::vector<GUIMain::ControlRef>> guictrl_refs_new;
    for (const auto &gui : guis)
        guictrl_refs_new.push_back(gui.GetControlRefs());
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, kGUIControlTypeNum> ctrlidx_per_gui_new;
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, kGUIControlTypeNum> ctrlidx_per_gui_old;
    BuildGUIControlRangeReference(guictrl_refs_new, ctrlidx_per_gui_new);
    BuildGUIControlRangeReference(guictrl_refs_old, ctrlidx_per_gui_old);

    if (!ReadGUIControlArray<GUIButton, kGUIButton>(
            guibuts, in, svg_ver, err, "GUIButtons", "GUI Buttons", r_data,
            ctrlidx_per_gui_old[kGUIButton], ctrlidx_per_gui_new[kGUIButton]))
        return err;

    if (!ReadGUIControlArray<GUILabel, kGUILabel>(
            guilabels, in, svg_ver, err, "GUILabels", "GUI Labels", r_data,
            ctrlidx_per_gui_old[kGUILabel], ctrlidx_per_gui_new[kGUILabel]))
        return err;

    if (!ReadGUIControlArray<GUIInvWindow, kGUIInvWindow>(
            guiinv, in, svg_ver, err, "GUIInvWindows", "GUI InvWindows", r_data,
            ctrlidx_per_gui_old[kGUIInvWindow], ctrlidx_per_gui_new[kGUIInvWindow]))
        return err;

    if (!ReadGUIControlArray<GUISlider, kGUISlider>(
            guislider, in, svg_ver, err, "GUISliders", "GUI Sliders", r_data,
            ctrlidx_per_gui_old[kGUISlider], ctrlidx_per_gui_new[kGUISlider]))
        return err;

    if (!ReadGUIControlArray<GUITextBox, kGUITextBox>(
            guitext, in, svg_ver, err, "GUITextBoxes", "GUI TextBoxes", r_data,
            ctrlidx_per_gui_old[kGUITextBox], ctrlidx_per_gui_new[kGUITextBox]))
        return err;

    if (!ReadGUIControlArray<GUIListBox, kGUIListBox>(
            guilist, in, svg_ver, err, "GUIListBoxes", "GUI ListBoxes", r_data,
            ctrlidx_per_gui_old[kGUIListBox], ctrlidx_per_gui_new[kGUIListBox]))
        return err;

    // Animated buttons
    if (!AssertFormatTagStrict(err, in, "AnimatedButtons"))
        return err;
    int anim_count = in->ReadInt32();
    for (int i = 0; i < anim_count; ++i)
    {
        AnimatingGUIButton abut;
        abut.ReadFromSavegame(in, cmp_ver);
        AddButtonAnimation(abut);
    }
    return err;
}

HSaveError PrescanGUI(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    // GUI state
    if (!AssertFormatTagStrict(err, in, "GUIs"))
        return err;
    const uint32_t guis_read = in->ReadInt32();
    if (!AssertGameContent(err, guis_read, game.numgui, "GUIs", r_data.Result, r_data.DataCounts.GUIs))
        return err;

    r_data.DataCounts.GUIControls.resize(guis_read);

    const GuiSvgVersion svg_ver = (GuiSvgVersion)cmp_ver;
    std::vector<std::vector<GUIMain::ControlRef>> guictrl_refs_old(guis_read);
    String assert_buf;
    for (uint32_t i = 0; i < guis_read; ++i)
    {
        GUIMain::SkipSavestate(in, svg_ver, &guictrl_refs_old[i]);
        if (svg_ver >= kGuiSvgVersion_40008)
            Properties::SkipValues(in);

        assert_buf.Format("GUI %u Controls", i);
        if (i < guis.size() &&
                !AssertGameContent(err, guictrl_refs_old[i].size(), guis[i].GetControlCount(), assert_buf.GetCStr(), r_data.Result, r_data.DataCounts.GUIControls[i]))
            return err;
    }

    // For older save formats: copy pre-existing control ref array
    if (svg_ver < kGuiSvgVersion_36200)
    {
        for (uint32_t i = 0; i < guis_read; ++i)
            guictrl_refs_old[i] = guis[i].GetControlRefs();
    }

    std::array<size_t, kGUIControlTypeNum> ctrl_counts_old = {}, ctrl_counts_new = {};
    for (const auto &refs_arr : guictrl_refs_old)
        for (const auto &ref : refs_arr)
            ctrl_counts_old[ref.first]++;
    for (const auto &gui : guis)
        for (const auto &ref : gui.GetControlRefs())
            ctrl_counts_new[ref.first]++;

    // Overall control objects check
    std::array<const char *, kGUIControlTypeNum> ctrl_name = {{
        "", "GUI Buttons", "GUI Labels", "GUI InvWindows", "GUI Sliders", "GUI TextBoxes", "GUI ListBoxes"
    }};
    for (int type = kGUIButton; type < kGUIControlTypeNum; ++type)
    {
        if (!AssertGameContent(err, ctrl_counts_old[type], ctrl_counts_new[type], ctrl_name[type], r_data.Result, r_data.DataCounts.Dummy))
            return err;
    }

    return HSaveError::None();
}

HSaveError WriteInventory(Stream *out)
{
    out->WriteInt32(game.numinvitems);
    for (int i = 0; i < game.numinvitems; ++i)
    {
        game.invinfo[i].WriteToSavegame(out);
        Properties::WriteValues(play.invProps[i], out);
    }
    return HSaveError::None();
}

HSaveError ReadInventory(Stream *in, int32_t /*cmp_ver*/, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    HSaveError err;
    const uint32_t invitems_read = in->ReadInt32();
    if (!AssertGameContent(err, invitems_read, game.numinvitems, "Inventory Items", r_data.Result, r_data.DataCounts.InventoryItems))
        return err;
    for (uint32_t i = 0; i < invitems_read; ++i)
    {
        game.invinfo[i].ReadFromSavegame(in);
        Properties::ReadValues(play.invProps[i], in);
    }
    return err;
}

HSaveError PrescanInventory(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    const uint32_t invitems_read = in->ReadInt32();
    if (!AssertGameContent(err, invitems_read, game.numinvitems, "Inventory Items", r_data.Result, r_data.DataCounts.InventoryItems))
        return err;
    return HSaveError::None();
}

HSaveError WriteMouseCursors(Stream *out)
{
    out->WriteInt32(game.numcursors);
    for (int i = 0; i < game.numcursors; ++i)
    {
        game.mcurs[i].WriteToSavegame(out);
    }
    return HSaveError::None();
}

HSaveError ReadMouseCursors(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    HSaveError err;
    const uint32_t cursors_read = in->ReadInt32();
    if (!AssertGameContent(err, cursors_read, game.numcursors, "Mouse Cursors", r_data.Result, r_data.DataCounts.Cursors))
        return err;
    for (uint32_t i = 0; i < cursors_read; ++i)
    {
        game.mcurs[i].ReadFromSavegame(in, cmp_ver);
    }
    return err;
}

HSaveError PrescanMouseCursors(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    const uint32_t cursors_read = in->ReadInt32();
    if (!AssertGameContent(err, cursors_read, game.numcursors, "Mouse Cursors", r_data.Result, r_data.DataCounts.Cursors))
        return err;
    return HSaveError::None();
}

enum ViewSvgVersion
{
    kViewSvgVersion_Initial = 0,
    kViewSvgVersion_4000018 = 4000018, // view frame offsets
};

HSaveError WriteViews(Stream *out)
{
    out->WriteInt32(game.numviews);
    for (int view = 0; view < game.numviews; ++view)
    {
        out->WriteInt32(views[view].numLoops);
        for (int loop = 0; loop < views[view].numLoops; ++loop)
        {
            out->WriteInt32(views[view].loops[loop].numFrames);
            for (int frame = 0; frame < views[view].loops[loop].numFrames; ++frame)
            {
                auto &vf = views[view].loops[loop].frames[frame];
                out->WriteInt32(vf.sound);
                out->WriteInt32(vf.pic);
                // ---- kViewSvgVersion_4000018
                out->WriteInt32(vf.flags);
                out->WriteInt16(vf.speed);
                out->WriteInt16(vf.xoffs);
                out->WriteInt16(vf.yoffs);
            }
        }
    }
    return HSaveError::None();
}

HSaveError ReadViews(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams & /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    const uint32_t views_read = in->ReadInt32();
    if (!AssertGameContent(err, views_read, game.numviews, "Views", r_data.Result, r_data.DataCounts.Views))
        return err;

    r_data.DataCounts.ViewLoops.resize(views_read);
    r_data.DataCounts.ViewFrames.resize(views_read);
    for (uint32_t view = 0; view < views_read; ++view)
    {
        const uint32_t loops_read = in->ReadInt32();
        if (!AssertGameObjectContent(err, loops_read, views[view].numLoops,
            "Loops", "View", view, r_data.Result, r_data.DataCounts.ViewLoops[view]))
            return err;

        for (uint32_t loop = 0; loop < loops_read; ++loop)
        {
            const uint32_t frames_read = in->ReadInt32();
            if (!AssertGameObjectContent2(err, frames_read, views[view].loops[loop].numFrames,
                "Frame", "View", view, "Loop", loop, r_data.Result, r_data.DataCounts.Dummy))
                return err;

            r_data.DataCounts.ViewFrames[view] += frames_read;
            for (uint32_t frame = 0; frame < frames_read; ++frame)
            {
                auto &vf = views[view].loops[loop].frames[frame];
                vf.sound = in->ReadInt32();
                vf.pic = in->ReadInt32();

                if (cmp_ver >= kViewSvgVersion_4000018)
                {
                    vf.flags = (SpriteTransformFlags)in->ReadInt32();
                    vf.speed = in->ReadInt16();
                    vf.xoffs = in->ReadInt16();
                    vf.yoffs = in->ReadInt16();
                }
            }
        }
    }
    return err;
}

HSaveError PrescanViews(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    const uint32_t views_read = in->ReadInt32();
    if (!AssertGameContent(err, views_read, game.numviews, "Views", r_data.Result, r_data.DataCounts.Views))
        return err;

    r_data.DataCounts.ViewLoops.resize(views_read);
    r_data.DataCounts.ViewFrames.resize(views_read);
    for (uint32_t view = 0; view < views_read; ++view)
    {
        const uint32_t loops_read = in->ReadInt32();
        if (!AssertGameObjectContent(err, loops_read, views[view].numLoops,
            "Loops", "View", view, r_data.Result, r_data.DataCounts.ViewLoops[view]))
            return err;

        for (uint32_t loop = 0; loop < loops_read; ++loop)
        {
            const uint32_t frames_read = in->ReadInt32();
            if (!AssertGameObjectContent2(err, frames_read, views[view].loops[loop].numFrames,
                "Frame", "View", view, "Loop", loop, r_data.Result, r_data.DataCounts.Dummy))
                return err;

            r_data.DataCounts.ViewFrames[view] += frames_read;
            in->Seek(frames_read * (
                  sizeof(int32_t) * 2
                + sizeof(int32_t) + sizeof(int16_t) * 3
                ));
        }
    }
    return HSaveError::None();
}

HSaveError WriteDynamicSpritesImpl(Stream *out, int match_flags)
{
    int count = 0;
    int top_index = 1;
    for (size_t i = 1; i < spriteset.GetSpriteSlotCount(); ++i)
    {
        if ((game.SpriteInfos[i].Flags & match_flags) == match_flags)
        {
            count++;
            top_index = i;
        }
    }
    out->WriteInt32(count);
    out->WriteInt32(top_index);

    for (size_t i = 1; i < spriteset.GetSpriteSlotCount(); ++i)
    {
        if ((game.SpriteInfos[i].Flags & match_flags) == match_flags)
        {
            out->WriteInt32(i);
            out->WriteInt32(game.SpriteInfos[i].Flags);
            WriteBitmap(spriteset[i], out, false /* not compressed (expect component is compressed) */);
        }
    }
    return HSaveError::None();
}

HSaveError ReadDynamicSpritesImpl(Stream *in, int match_flags)
{
    HSaveError err;
    const int spr_count = in->ReadInt32();
    // ensure the sprite set is at least large enough
    // to accomodate top dynamic sprite index
    const int top_index = in->ReadInt32();
    spriteset.EnlargeTo(top_index);
    for (int i = 0; i < spr_count; ++i)
    {
        int id = in->ReadInt32();
        int flags = in->ReadInt32();
        if ((flags & match_flags) == match_flags)
        {
            std::unique_ptr<Bitmap> image(ReadBitmap(in, false /* not compressed (expect component is compressed) */));
            add_dynamic_sprite(id, std::move(image), flags);
        }
        else
        {
            SkipBitmap(in, false /* not compressed */);
        }
    }
    return err;
}

HSaveError WriteDynamicSprites(Stream *out)
{
    return WriteDynamicSpritesImpl(out, SPF_DYNAMICALLOC);
}

HSaveError WriteObjectSprites(Stream *out)
{
    return WriteDynamicSpritesImpl(out, SPF_DYNAMICALLOC | SPF_OBJECTOWNED);
}

HSaveError ReadDynamicSprites(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData& /*r_data*/)
{
    return ReadDynamicSpritesImpl(in, SPF_DYNAMICALLOC);
}

HSaveError ReadObjectSprites(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData& /*r_data*/)
{
    return ReadDynamicSpritesImpl(in, SPF_DYNAMICALLOC | SPF_OBJECTOWNED);
}

HSaveError WriteOverlays(Stream *out)
{
    const auto &overs = get_overlays();
    // Calculate and save valid overlays only
    uint32_t valid_count = 0;
    for (const auto &over : overs)
    {
        if (over.type >= 0)
            valid_count++;
    }
    out->WriteInt32(valid_count);

    for (const auto &over : overs)
    {
        if (over.type >= 0)
            over.WriteToSavegame(out);
    }
    return HSaveError::None();
}

HSaveError ReadOverlays(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    // Remember that overlay indexes may be non-sequential
    // the vector may be resized during read
    size_t over_count = in->ReadInt32();
    auto &overs = get_overlays();
    overs.resize(over_count); // reserve minimal size
    for (size_t i = 0; i < over_count; ++i)
    {
        ScreenOverlay over;
        bool has_bitmap;
        over.ReadFromSavegame(in, has_bitmap, cmp_ver);
        if (over.type < 0)
            continue; // safety abort
        if (has_bitmap)
            r_data.OverlayImages[over.type].reset(ReadBitmap(in, false /* not compressed (expect component is compressed) */));
        if (overs.size() <= static_cast<uint32_t>(over.type))
            overs.resize(over.type + 1);
        overs[over.type] = std::move(over);
    }
    return HSaveError::None();
}

HSaveError WriteDynamicSurfaces(Stream *out)
{
    out->WriteInt32(MAX_DYNAMIC_SURFACES);
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        if (dynamicallyCreatedSurfaces[i] == nullptr)
        {
            out->WriteInt8(0);
        }
        else
        {
            out->WriteInt8(1);
            WriteBitmap(dynamicallyCreatedSurfaces[i].get(), out, false /* not compressed (expect component is compressed) */);
        }
    }
    return HSaveError::None();
}

HSaveError ReadDynamicSurfaces(Stream *in, int32_t /*cmp_ver*/, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    if (!AssertCompatLimit(err, in->ReadInt32(), MAX_DYNAMIC_SURFACES, "Dynamic Surfaces"))
        return err;
    // Load the surfaces into a temporary array since ccUnserialiseObjects will destroy them otherwise
    r_data.DynamicSurfaces.resize(MAX_DYNAMIC_SURFACES);
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        if (in->ReadInt8() == 0)
            r_data.DynamicSurfaces[i] = nullptr;
        else
            r_data.DynamicSurfaces[i].reset(ReadBitmap(in, false /* not compressed (expect component is compressed) */));
    }
    return err;
}

enum ScriptModulesSvgVersion
{
    kScriptModulesSvgVersion_Initial    = 0,
    kScriptModulesSvgVersion_36200      = 3060200, // module names
};

HSaveError WriteScriptModules(Stream *out)
{
    // write the data segment of the global script
    int data_len = gamescript->GetGlobalData().size();
    out->WriteInt32(data_len);
    if (data_len > 0)
        out->Write(gamescript->GetGlobalData().data(), data_len);
    // write the script modules data segments
    out->WriteInt32(numScriptModules);
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        StrUtil::WriteString(scriptModules[i]->GetScriptName(), out);
        data_len = scriptModules[i]->GetGlobalData().size();
        out->WriteInt32(data_len);
        if (data_len > 0)
            out->Write(scriptModules[i]->GetGlobalData().data(), data_len);
    }
    return HSaveError::None();
}

HSaveError ReadScriptModules(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams &pp, RestoredData &r_data)
{
    HSaveError err;
    // read the global script data segment
    uint32_t data_len = in->ReadInt32();
    if (!AssertGameContent(err, data_len, pp.GlScDataSize, "global script data", r_data.Result, r_data.DataCounts.GlobalScriptDataSz))
        return err;
    r_data.GlobalScript.Data.resize(data_len);
    if (data_len > 0u)
        in->Read(r_data.GlobalScript.Data.data(), data_len);

    std::vector<bool> modules_match(pp.ScriptModuleNames.size());
    const uint32_t modules_read = in->ReadInt32();
    r_data.DataCounts.ScriptModules = modules_read;
    r_data.DataCounts.ScriptModuleDataSz.resize(modules_read);
    r_data.DataCounts.ScriptModuleNames.resize(modules_read);
    for (size_t read_module_index = 0; read_module_index < modules_read; ++read_module_index)
    {
        const String module_name = (cmp_ver < kScriptModulesSvgVersion_36200) ?
            (read_module_index < pp.ScriptModuleNames.size() ? pp.ScriptModuleNames[read_module_index] : "") :
            StrUtil::ReadString(in);
        r_data.DataCounts.ScriptModuleNames[read_module_index] = module_name;

        data_len = in->ReadInt32();

        // Try to find existing module length and assert its presence and matching size
        uint32_t game_module_index = UINT32_MAX;
        for (size_t i = 0; i < pp.ScriptModuleNames.size(); ++i)
        {
            if (module_name.Compare(pp.ScriptModuleNames[i]) == 0)
            {
                game_module_index = i;
                break;
            }
        }

        if (game_module_index < UINT32_MAX)
        {
            // Found matching module in the game
            if (!AssertGameObjectContent(err, data_len, pp.ScMdDataSize[game_module_index],
                "script module data", module_name.GetCStr(), game_module_index, r_data.Result,
                r_data.DataCounts.ScriptModuleDataSz[read_module_index]))
                return err;
            modules_match[game_module_index] = true;
        }
        else
        {
            // No such module in the game: treat this as "more data"
            if (!HandleExtraGameComponent(err, "script module", module_name, r_data.Result))
                return err;
        }

        RestoredData::ScriptData scdata;
        scdata.Data.resize(data_len);
        if (data_len > 0u)
            in->Read(scdata.Data.data(), data_len);
        r_data.ScriptModules[module_name] = std::move(scdata);
    }

    // Assert that all game's script modules were read from the save
    for (size_t i = 0; i < modules_match.size(); ++i)
    {
        if (!modules_match[i] &&
            !HandleMissingGameComponent(err, "script module", pp.ScriptModuleNames[i], r_data.Result))
            return err;
    }

    return HSaveError::None();
}

HSaveError PrescanScriptModules(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams &pp, RestoredData &r_data)
{
    HSaveError err;
    uint32_t data_len = in->ReadInt32();
    if (!AssertGameContent(err, data_len, pp.GlScDataSize, "global script data", r_data.Result, r_data.DataCounts.GlobalScriptDataSz))
        return err;
    in->Seek(data_len);

    std::vector<bool> modules_match(pp.ScriptModuleNames.size());
    const uint32_t modules_read = in->ReadInt32();
    r_data.DataCounts.ScriptModules = modules_read;
    r_data.DataCounts.ScriptModuleDataSz.resize(modules_read);
    r_data.DataCounts.ScriptModuleNames.resize(modules_read);
    for (size_t read_module_index = 0; read_module_index < modules_read; ++read_module_index)
    {
        const String module_name = (cmp_ver < kScriptModulesSvgVersion_36200) ?
            (read_module_index < pp.ScriptModuleNames.size() ? pp.ScriptModuleNames[read_module_index] : "") :
            StrUtil::ReadString(in);
        r_data.DataCounts.ScriptModuleNames[read_module_index] = module_name;

        data_len = in->ReadInt32();

        // Try to find existing module length and assert its presence and matching size
        uint32_t game_module_index = UINT32_MAX;
        for (size_t i = 0; i < pp.ScriptModuleNames.size(); ++i)
        {
            if (module_name.Compare(pp.ScriptModuleNames[i]) == 0)
            {
                game_module_index = i;
                break;
            }
        }

        if (game_module_index < UINT32_MAX)
        {
            // Found matching module in the game
            if (!AssertGameObjectContent(err, data_len, pp.ScMdDataSize[game_module_index],
                "script module data", module_name.GetCStr(), game_module_index, r_data.Result,
                r_data.DataCounts.ScriptModuleDataSz[read_module_index]))
                return err;
            modules_match[game_module_index] = true;
        }
        else
        {
            // No such module in the game: treat this as "more data"
            if (!HandleExtraGameComponent(err, "script module", module_name, r_data.Result))
                return err;
        }

        in->Seek(data_len);
    }

    // Assert that all game's script modules were read from the save
    for (size_t i = 0; i < modules_match.size(); ++i)
    {
        if (!modules_match[i] &&
            !HandleMissingGameComponent(err, "script module", pp.ScriptModuleNames[i], r_data.Result))
            return err;
    }

    return HSaveError::None();
}

HSaveError WriteRoomStates(Stream *out)
{
    // write the room state for all the rooms the player has been in
    out->WriteInt32(MAX_ROOMS);
    for (int i = 0; i < MAX_ROOMS; ++i)
    {
        RoomStatus *roomstat = GetRoomStateIfExists(i);
        if (roomstat)
        {
            if (roomstat->beenhere)
            {
                out->WriteInt32(i);
                WriteFormatTag(out, "RoomState", true);
                roomstat->WriteToSavegame(out);
                WriteFormatTag(out, "RoomState", false);
            }
            else
            {
                out->WriteInt32(-1);
            }
        }
        else
        {
            out->WriteInt32(-1);
        }
    }
    return HSaveError::None();
}

HSaveError ReadRoomStates(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& /*r_data*/)
{
    HSaveError err;
    int roomstat_count = in->ReadInt32();
    for (; roomstat_count > 0; --roomstat_count)
    {
        int id = in->ReadInt32();
        // If id == -1, then the player has not been there yet (or room state was reset)
        if (id != -1)
        {
            if (!AssertCompatRange(err, id, 0, MAX_ROOMS - 1, "room index"))
                return err;
            if (!AssertFormatTagStrict(err, in, "RoomState", true))
                return err;
            RoomStatus *roomstat = GetRoomState(id);
            roomstat->ReadFromSavegame(in, (RoomStatSvgVersion)cmp_ver);
            if (!AssertFormatTagStrict(err, in, "RoomState", false))
                return err;
        }
    }
    return HSaveError::None();
}

HSaveError WriteThisRoom(Stream *out)
{
    out->WriteInt32(displayed_room);
    if (displayed_room < 0)
        return HSaveError::None();

    // modified room backgrounds
    for (int i = 0; i < MAX_ROOM_BGFRAMES; ++i)
    {
        out->WriteBool(play.raw_modified[i] != 0);
        if (play.raw_modified[i])
            WriteBitmap(thisroom.BgFrames[i].Graphic.get(), out, false /* not compressed (expect component is compressed) */);
    }
    out->WriteBool(false); //[DEPRECATED]

    // room region state
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        out->WriteInt32(thisroom.Regions[i].Light);
        out->WriteInt32(thisroom.Regions[i].Tint);
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        out->WriteInt32(thisroom.WalkAreas[i].ScalingFar);
        out->WriteInt32(thisroom.WalkAreas[i].ScalingNear);
    }

    out->WriteInt32(0); // [DEPRECATED]

    // persistent room's indicator
    const bool persist = displayed_room < MAX_ROOMS;
    out->WriteBool(persist);
    // write the current troom state, in case they save in temporary room
    if (!persist)
        troom.WriteToSavegame(out);
    return HSaveError::None();
}

HSaveError ReadThisRoom(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    HSaveError err;
    r_data.Room = in->ReadInt32();
    r_data.DataCounts.Room = r_data.Room;
    if (r_data.Room < 0)
        return HSaveError::None();

    // modified room backgrounds
    for (int i = 0; i < MAX_ROOM_BGFRAMES; ++i)
    {
        play.raw_modified[i] = in->ReadBool();
        if (play.raw_modified[i])
            r_data.RoomBkgScene[i].reset(ReadBitmap(in, false /* not compressed (expect component is compressed) */));
        else
            r_data.RoomBkgScene[i] = nullptr;
    }
    if (in->ReadBool())
        SkipBitmap(in, false /* not compressed (expect component is compressed) */); //[DEPRECATED]

    // room region state
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        r_data.RoomLightLevels[i] = in->ReadInt32();
        r_data.RoomTintLevels[i] = in->ReadInt32();
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        r_data.RoomZoomLevels1[i] = in->ReadInt32();
        r_data.RoomZoomLevels2[i] = in->ReadInt32();
    }

    in->ReadInt32();// [DEPRECATED]

    // read the current troom state, in case they saved in temporary room
    if (!in->ReadBool())
        troom.ReadFromSavegame(in, (RoomStatSvgVersion)cmp_ver);

    return HSaveError::None();
}

HSaveError PrescanThisRoom(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    r_data.DataCounts.Room = in->ReadInt32();
    return HSaveError::None();
}

HSaveError WriteMoveLists(Stream *out)
{
    const auto &movelists = get_movelists();
    out->WriteInt32(static_cast<int32_t>(movelists.size()));
    for (const auto &movelist : movelists)
    {
        movelist.WriteToSavegame(out);
    }
    return HSaveError::None();
}

HSaveError ReadMoveLists(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    HSaveError err;
    uint32_t movelist_count = in->ReadInt32();
    for (uint32_t i = 0; i < movelist_count; ++i)
    {
        MoveList mlist;
        err = mlist.ReadFromSavegame(in, cmp_ver);
        if (!err)
            return err;
        add_movelist(std::move(mlist), i);
    }
    return err;
}

HSaveError PrescanMoveLists(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    return HSaveError::None();
}

HSaveError WriteManagedPool(Stream *out)
{
    ccSerializeAllObjects(out);
    return HSaveError::None();
}

HSaveError ReadManagedPool(Stream *in, int32_t /*cmp_ver*/, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& r_data)
{
    if (ccUnserializeAllObjects(in, &r_data.ManObjReader))
    {
        return new SavegameError(kSvgErr_GameObjectInitFailed,
            String::FromFormat("Managed pool deserialization failed: %s",
                cc_get_error().ErrorString.GetCStr()));
    }
    return HSaveError::None();
}

HSaveError WriteRTTI(Stream *out)
{
    // Write the minimal necessary RTTI data, enough to resolve types when restoring a save;
    // NOTE: we might just dump whole RTTI here, if it's necessary to keep all field descs and names
    const auto &rtti = RuntimeScript::GetJointRTTI()->AsConstRTTI();
    const auto &helper = RuntimeScript::GetRTTIHelper();
    const auto &locs = rtti.GetLocations();
    const auto &types = rtti.GetTypes();
    // NOTE: we don't write IDs here, as the Joint RTTI assumes to have them strcitly sequential
    out->WriteInt32(locs.size());
    for (const auto &loc : locs)
    {
        StrUtil::WriteCStr(loc.name, out);
        out->WriteInt32(loc.flags);
    }
    out->WriteInt32(types.size());
    for (const auto &type : types)
    {
        StrUtil::WriteCStr(type.name, out);
        out->WriteInt32(type.loc_id);
        out->WriteInt32(type.parent_id);
        out->WriteInt32(type.flags);
        out->WriteInt32(type.size);
        // Managed ptrs offsets
        const auto man_offs = helper->GetManagedOffsetsForType(type.this_id);
        out->WriteInt32(man_offs.second - man_offs.first);
        for (auto off = man_offs.first; off < man_offs.second; ++off)
            out->WriteInt32(*off);
    }
    return HSaveError::None();
}

HSaveError ReadRTTI(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData &r_data)
{
    // Restore the minimal necessary RTTI data, enough to resolve types when restoring a save
    // Create a pseudo-RTTI with "generated" types, and fields only for managed pointers
    RTTIBuilder rb;
    char buf[256];
    uint32_t num_locs = in->ReadInt32();
    for (uint32_t i = 0; i < num_locs; ++i)
    {
        StrUtil::ReadCStr(buf, in, sizeof(buf));
        uint32_t flags = in->ReadInt32();
        rb.AddLocation(buf, i, flags | RTTI::kLoc_Generated);
    }
    uint32_t num_types = in->ReadInt32();
    for (uint32_t i = 0; i < num_types; ++i)
    {
        StrUtil::ReadCStr(buf, in, sizeof(buf));
        uint32_t loc_id = in->ReadInt32();
        uint32_t parent_id = in->ReadInt32();
        uint32_t flags = in->ReadInt32();
        uint32_t size = in->ReadInt32();
        rb.AddType(buf, i, loc_id, parent_id, flags | RTTI::kType_Generated, size);
        // Read managed ptrs offsets and generate pseudo-fields for these
        uint32_t num_offs = in->ReadInt32();
        for (uint32_t oi = 0; oi < num_offs; ++oi)
        {
            uint32_t off = in->ReadInt32();
            rb.AddField(i, "", off, 0u, RTTI::kField_ManagedPtr | RTTI::kType_Generated, 0u);
        }
    }

    r_data.GenRTTI = std::move(rb.Finalize());
    return HSaveError::None();
}

HSaveError WritePluginData(Stream *out)
{
    WritePluginSaveData(out);
    return HSaveError::None();
}

HSaveError ReadPluginData(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams& /*pp*/, RestoredData& /*r_data*/)
{
    ReadPluginSaveData(in, static_cast<PluginSvgVersion>(cmp_ver), cmp_size);
    return HSaveError::None();
}


// TODO: move elsewhere later
// FIXME: currently not passed into the dynamic object deserializer
enum ManagedPoolSavegameVersion
{
    kManagedPoolSvgVersion_Initial = 0,
    kManagedPoolSvgVersion_400     = 4000000 // typeinfo for dynamic objects
};

// Description of a supported game state serialization component
struct ComponentHandler
{
    String             Name;    // internal component's ID
    int32_t            Version; // current version to write and the highest supported version
    int32_t            LowestVersion; // lowest supported version that the engine can read
    SaveCmpSelection   Selection; // flag mask corresponding to this component
    HSaveError       (*Serialize)  (Stream*);
    HSaveError       (*Unserialize)(Stream*, int32_t cmp_ver, soff_t cmp_size, const PreservedParams&, RestoredData&);
    HSaveError       (*Prescan)    (Stream*, int32_t cmp_ver, soff_t cmp_size, const PreservedParams&, RestoredData&);
};

// Array of supported components
ComponentHandler ComponentHandlers[] =
{
    // NOTE: the new format values should now be defined as AGS version
    // at which a change was introduced, represented as NN,NN,NN,NN.
    {
        "Game State",
        kGSSvgVersion_400_18,
        kGSSvgVersion_400,
        kSaveCmp_GameState,
        WriteGameState,
        ReadGameState,
        nullptr
    },
    {
        "Audio",
        kAudioSvgVersion_36130,
        kAudioSvgVersion_Initial,
        kSaveCmp_Audio,
        WriteAudio,
        ReadAudio,
        PrescanAudio
    },
    {
        "Characters",
        kCharSvgVersion_400_18,
        kCharSvgVersion_400,
        kSaveCmp_Characters,
        WriteCharacters,
        ReadCharacters,
        PrescanCharacters
    },
    {
        "Dialogs",
        kDialogSvgVersion_40008,
        kDialogSvgVersion_Initial,
        kSaveCmp_Dialogs,
        WriteDialogs,
        ReadDialogs,
        PrescanDialogs
    },
    {
        "GUI",
        kGuiSvgVersion_40018,
        kGuiSvgVersion_Initial,
        kSaveCmp_GUI,
        WriteGUI,
        ReadGUI,
        PrescanGUI
    },
    {
        "Inventory Items",
        0,
        0,
        kSaveCmp_InvItems,
        WriteInventory,
        ReadInventory,
        PrescanInventory
    },
    {
        "Mouse Cursors",
        kCursorSvgVersion_36016,
        kCursorSvgVersion_Initial,
        kSaveCmp_Cursors,
        WriteMouseCursors,
        ReadMouseCursors,
        PrescanMouseCursors
    },
    {
        "Views",
        kViewSvgVersion_4000018,
        kViewSvgVersion_Initial,
        kSaveCmp_Views,
        WriteViews,
        ReadViews,
        PrescanViews
    },
    {
        "Dynamic Sprites",
        0,
        0,
        kSaveCmp_DynamicSprites,
        WriteDynamicSprites,
        ReadDynamicSprites,
        nullptr
    },
    // Alternate "Dynamic Sprites" handler in case only object-owned sprites are serialized
    {
        "Dynamic Sprites",
        0,
        0,
        kSaveCmp_ObjectSprites,
        WriteObjectSprites,
        ReadObjectSprites,
        nullptr
    },
    {
        "Overlays",
        kOverSvgVersion_40018,
        kOverSvgVersion_Initial,
        kSaveCmp_Overlays,
        WriteOverlays,
        ReadOverlays,
        nullptr
    },
    {
        "Dynamic Surfaces",
        0,
        0,
        kSaveCmp_DynamicSprites, // share flag with "Dynamic Sprites"
        WriteDynamicSurfaces,
        ReadDynamicSurfaces,
        nullptr
    },
    {
        "Script Modules",
        kScriptModulesSvgVersion_36200,
        kScriptModulesSvgVersion_Initial,
        kSaveCmp_Scripts,
        WriteScriptModules,
        ReadScriptModules,
        PrescanScriptModules
    },
    {
        "Room States",
        kRoomStatSvgVersion_40018,
        kRoomStatSvgVersion_40003,
        kSaveCmp_Rooms,
        WriteRoomStates,
        ReadRoomStates,
        nullptr
    },
    {
        "Loaded Room State",
        kRoomStatSvgVersion_40018, // must correspond to "Room States"
        kRoomStatSvgVersion_40003,
        kSaveCmp_ThisRoom,
        WriteThisRoom,
        ReadThisRoom,
        PrescanThisRoom
    },
    {
        "Move Lists",
        kMoveSvgVersion_40016,
        kMoveSvgVersion_400,
        (SaveCmpSelection)(kSaveCmp_Characters | kSaveCmp_ThisRoom), // must go along with characters and room objects
        WriteMoveLists,
        ReadMoveLists,
        PrescanMoveLists
    },
    {
        "Managed Pool",
        kManagedPoolSvgVersion_400,
        kManagedPoolSvgVersion_Initial,
        kSaveCmp_Scripts, // must go along with scripts
        WriteManagedPool,
        ReadManagedPool,
        nullptr
    },
    {
        "RTTI",
        4000000,
        4000000,
        kSaveCmp_Scripts, // must go along with scripts
        WriteRTTI,
        ReadRTTI,
        nullptr
    },
    {
        "Plugin Data",
        kPluginSvgVersion_36115,
        kPluginSvgVersion_Initial,
        kSaveCmp_Plugins,
        WritePluginData,
        ReadPluginData,
        nullptr
    },
    { nullptr, 0, 0, kSaveCmp_None, nullptr, nullptr, nullptr } // end of array
};


typedef std::multimap<String, ComponentHandler> HandlersMap;
void GenerateHandlersMap(HandlersMap &map)
{
    map.clear();
    for (int i = 0; !ComponentHandlers[i].Name.IsEmpty(); ++i)
        map.insert(std::make_pair(ComponentHandlers[i].Name, ComponentHandlers[i]));
}

// A helper struct to pass to (de)serialization handlers
struct SvgCmpReadHelper
{
    SavegameVersion        Version; // general savegame version
    // Flag mask, instructing which components to read (others shall be skipped)
    SaveCmpSelection       ComponentSelection = kSaveCmp_All;
    const PreservedParams &PP;      // previous game state kept for reference
    RestoredData          &RData;   // temporary storage for loaded data, that
                                    // will be applied after loading is done
    // The map of serialization handlers, one per supported component type ID
    HandlersMap            Handlers;

    SvgCmpReadHelper(SavegameVersion svg_version, SaveCmpSelection select_cmp,
        const PreservedParams &pp, RestoredData &r_data)
        : Version(svg_version)
        , ComponentSelection(select_cmp)
        , PP(pp)
        , RData(r_data)
    {
    }
};

enum ComponentFlags
{
    kSvgCmp_Deflate = 0x0001 // compress using Deflate algorithm
};

// The basic information about deserialized component, used for debugging purposes
struct ComponentInfo
{
    String      Name;           // internal component's ID
    uint32_t    TagOffset = 0;  // offset at which an opening tag is located [not serialized]
    uint32_t    HeaderSize = 0; // component header size (meta data)
    uint32_t    Flags = 0;      // ComponentFlags
    int32_t     Version = -1;   // data format version
    uint32_t    DataOffset = 0; // offset at which component data begins [not serialized]
    uint32_t    DataSize = 0u;  // expected size of component data
    uint32_t    UncompressedDataSize = 0u; // uncompressed data size
    uint32_t    Checksum = 0u;  // checksum of the uncompressed data (only if compressed)

    ComponentInfo() = default;
};

HSaveError ReadComponent(Stream *in, SvgCmpReadHelper &hlp, ComponentInfo &info)
{
    const bool has_fileformat = (hlp.Version >= kSvgVersion_363 && hlp.Version < kSvgVersion_399)
        || (hlp.Version >= kSvgVersion_400_18);

    // Read component info
    info = ComponentInfo();
    info.TagOffset = in->GetPosition();
    if (!ReadFormatTag(in, info.Name, true))
        return new SavegameError(kSvgErr_ComponentOpeningTagFormat);
    if (has_fileformat)
    {
        info.HeaderSize = in->ReadInt32();
        info.Flags = in->ReadInt32();
        info.Version = in->ReadInt32();
        info.DataSize = in->ReadInt32();
        info.UncompressedDataSize = in->ReadInt32();
        info.Checksum = in->ReadInt32();
    }
    else if (hlp.Version >= kSvgVersion_ComponentsEx)
    {
        info.Version = in->ReadInt32();
        info.DataSize = in->ReadInt32();
        in->ReadInt32(); // reserved
    }
    else
    {
        info.Version = in->ReadInt32();
        info.DataSize = in->ReadInt32();
    }
    // Assume that component data begins right after the header
    info.DataOffset = in->GetPosition();

    // Find component's handler(s)
    const ComponentHandler *handler = nullptr;
    auto it_hdr = hlp.Handlers.equal_range(info.Name);
    const bool found_any = it_hdr.first != it_hdr.second;
    if (!found_any)
        return new SavegameError(kSvgErr_UnsupportedComponent);

    // Find any first handler, that is not disabled by ComponentSelection
    for (auto it = it_hdr.first; handler == nullptr && it != it_hdr.second; ++it)
    {
        if ((it->second.Selection & hlp.ComponentSelection) != 0)
        {
            handler = &it->second;
        }
    }

    const bool prescan = (hlp.RData.Result.RestoreFlags & kSaveRestore_Prescan) != 0;
    auto pfn_read = prescan ? handler->Prescan : handler->Unserialize;

    // If a handler is chosen, and has Unserialize method, then try reading the data
    if (handler && pfn_read)
    {
        if (info.Version > handler->Version || info.Version < handler->LowestVersion)
            return new SavegameError(kSvgErr_UnsupportedComponentVersion, String::FromFormat("Saved version: %d, supported: %d - %d", info.Version, handler->LowestVersion, handler->Version));

        if ((info.Flags & kSvgCmp_Deflate) != 0)
        {
            auto deflate_s = std::make_unique<DeflateStream>(in->ReleaseStreamBase(), info.DataOffset, info.DataOffset + info.DataSize);
            auto deflate_in = std::make_unique<Stream>(std::move(deflate_s));

            HSaveError err = pfn_read(deflate_in.get(), info.Version, info.UncompressedDataSize, hlp.PP, hlp.RData);
            if (!err)
                return err;

            if (prescan)
                deflate_in->Seek(info.UncompressedDataSize, kSeekBegin);

            // FIXME: this is very ugly, maybe may be fixed by changing stream base storage to shared ptr?
            deflate_s.reset(dynamic_cast<DeflateStream*>(deflate_in->ReleaseStreamBase().release()));
            uint32_t uncomp_data_sz = deflate_s->GetProcessedInput();
            if (uncomp_data_sz != info.UncompressedDataSize)
                return new SavegameError(kSvgErr_ComponentUncompressedSizeMismatch, String::FromFormat("Expected: %zu, actual: %zu", info.UncompressedDataSize, uncomp_data_sz));
            // TODO: test checksum too?

            in->AttachStreamBase(deflate_s->ReleaseStreamBase());
        }
        else
        {
            HSaveError err = pfn_read(in, info.Version, info.DataSize, hlp.PP, hlp.RData);
            if (!err)
                return err;

            if (prescan)
                in->Seek(info.DataOffset + info.DataSize, kSeekBegin);
        }
    }
    // Else, skip the data
    else
    {
        in->Seek(info.DataSize);
    }

    // Test that we have reached an expected position in stream
    if (in->GetPosition() - info.DataOffset != info.DataSize)
        return new SavegameError(kSvgErr_ComponentSizeMismatch, String::FromFormat("Expected: %jd, actual: %jd",
            static_cast<intmax_t>(info.DataSize), static_cast<intmax_t>(in->GetPosition() - info.DataOffset)));
    if (!AssertFormatTag(in, info.Name, false))
        return new SavegameError(kSvgErr_ComponentClosingTagFormat);
    return HSaveError::None();
}

HSaveError ReadAllImpl(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp,
    const PreservedParams &pp, RestoredData &r_data)
{
    // Prepare a helper struct we will be passing to the block reading proc
    SvgCmpReadHelper hlp(svg_version, select_cmp, pp, r_data);
    GenerateHandlersMap(hlp.Handlers);

    size_t idx = 0;
    if (!AssertFormatTag(in, ComponentListTag, true))
        return new SavegameError(kSvgErr_ComponentListOpeningTagFormat);
    do
    {
        // Look out for the end of the component list:
        // this is the only way how this function ends with success
        soff_t off = in->GetPosition();
        if (AssertFormatTag(in, ComponentListTag, false))
            return HSaveError::None();
        // If the list's end was not detected, then seek back and continue reading
        in->Seek(off, kSeekBegin);

        ComponentInfo info;
        HSaveError err = ReadComponent(in, hlp, info);
        if (!err)
        {
            return new SavegameError(kSvgErr_ComponentUnserialization,
                String::FromFormat("(#%d) %s, version %i, at offset %u.",
                idx, info.Name.IsEmpty() ? "unknown" : info.Name.GetCStr(), info.Version, info.TagOffset),
                err);
        }
        idx++;
    }
    while (!in->EOS());
    return new SavegameError(kSvgErr_ComponentListClosingTagMissing);
}

HSaveError ReadAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp,
    const PreservedParams &pp, RestoredData &r_data)
{
    return ReadAllImpl(in, svg_version, select_cmp, pp, r_data);
}

HSaveError PrescanAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp,
    const PreservedParams &pp, RestoredData &r_data)
{
    r_data.Result.RestoreFlags = (SaveRestorationFlags)(r_data.Result.RestoreFlags
        | kSaveRestore_Prescan);
    return ReadAllImpl(in, svg_version, select_cmp, pp, r_data);
}

HSaveError WriteComponent(Stream *out, ComponentHandler &hdlr, bool compress)
{
    uint32_t flags = kSvgCmp_Deflate * compress;

    WriteFormatTag(out, hdlr.Name, true);
    soff_t header_pos = out->GetPosition();
    out->WriteInt32(0); // header size placeholder
    out->WriteInt32(flags); // flags
    out->WriteInt32(hdlr.Version);
    soff_t data_sz_pos = out->GetPosition();
    out->WriteInt32(0); // component size placeholder
    out->WriteInt32(0); // uncompressed size
    out->WriteInt32(0); // checksum

    soff_t data_begin_pos = out->GetPosition();

    uint32_t uncomp_data_sz = 0u;
    if (compress)
    {
        auto deflate_s = std::make_unique<DeflateStream>(out->ReleaseStreamBase(), kStream_Write);
        auto deflate_out = std::make_unique<Stream>(std::move(deflate_s));
        HSaveError err = hdlr.Serialize(deflate_out.get());
        if (!err)
            return err;

        // FIXME: this is very ugly, maybe may be fixed by changing stream base storage to shared ptr?
        deflate_s.reset(dynamic_cast<DeflateStream *>(deflate_out->ReleaseStreamBase().release()));
        deflate_s->Finalize();
        uncomp_data_sz = deflate_s->GetProcessedInput();
        out->AttachStreamBase(deflate_s->ReleaseStreamBase());
    }
    else
    {
        HSaveError err = hdlr.Serialize(out);
        if (!err)
            return err;
    }

    soff_t data_end_pos = out->GetPosition();

    out->Seek(header_pos, kSeekBegin);
    out->WriteInt32(data_begin_pos - header_pos);
    out->Seek(data_sz_pos, kSeekBegin);
    out->WriteInt32(data_end_pos - data_begin_pos); // size of serialized component data
    out->WriteInt32(compress ? uncomp_data_sz : (data_end_pos - data_begin_pos)); // uncompressed size
    out->WriteInt32(0); // checksum (?)
    out->Seek(data_end_pos, kSeekBegin);
    WriteFormatTag(out, hdlr.Name, false);
    return HSaveError::None();
}

HSaveError WriteAllCommon(Stream *out, SaveCmpSelection select_cmp, bool compress)
{
    WriteFormatTag(out, ComponentListTag, true);
    for (int type = 0; !ComponentHandlers[type].Name.IsEmpty(); ++type)
    {
        if ((ComponentHandlers[type].Selection & select_cmp) == 0)
            continue; // skip this component

        HSaveError err = WriteComponent(out, ComponentHandlers[type], compress);
        if (!err)
        {
            return new SavegameError(kSvgErr_ComponentSerialization,
                String::FromFormat("Component: (#%d) %s", type, ComponentHandlers[type].Name.GetCStr()),
                err);
        }
    }
    WriteFormatTag(out, ComponentListTag, false);
    return HSaveError::None();
}

} // namespace SavegameBlocks
} // namespace Engine
} // namespace AGS

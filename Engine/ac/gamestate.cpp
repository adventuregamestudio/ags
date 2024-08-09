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
#include <algorithm>
#include "ac/draw.h"
#include "ac/game_version.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/timer.h"
#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/scriptsystem.h"
#include "ac/dynobj/scriptviewport.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "game/customproperties.h"
#include "game/roomstruct.h"
#include "game/savegame_internal.h"
#include "main/engine.h"
#include "media/audio/audio_system.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern RoomStruct thisroom;
extern CharacterInfo *playerchar;
extern ScriptSystem scsystem;

GamePlayState::GamePlayState()
{
}

bool GamePlayState::IsAutoRoomViewport() const
{
    return _isAutoRoomViewport;
}

void GamePlayState::SetAutoRoomViewport(bool on)
{
    _isAutoRoomViewport = on;
}

void GamePlayState::SetMainViewport(const Rect &viewport)
{
    _mainViewport = viewport;
    Mouse::UpdateGraphicArea();
    scsystem.viewport_width = _mainViewport.GetWidth();
    scsystem.viewport_height = _mainViewport.GetHeight();
    _mainViewportHasChanged = true;
}

const Rect &GamePlayState::GetMainViewport() const
{
    return _mainViewport;
}

const Rect &GamePlayState::GetUIViewport() const
{
    return _uiViewport;
}

SpriteTransform GamePlayState::GetGlobalTransform(bool full_frame_rend) const
{
    // NOTE: shake_screen is not applied to the sprite batches,
    // but only as a final render factor (optimization)
    // TODO: also add global flip to the same transform, instead of passing separately?
    return SpriteTransform(_mainViewport.Left, _mainViewport.Top +
        shake_screen_yoff * static_cast<int>(full_frame_rend));
}

PViewport GamePlayState::GetRoomViewport(int index) const
{
    return _roomViewports[index];
}

const std::vector<PViewport> &GamePlayState::GetRoomViewportsZOrdered() const
{
    return _roomViewportsSorted;
}

PViewport GamePlayState::GetRoomViewportAt(int x, int y) const
{
    // We iterate backwards, because in AGS low z-order means bottom
    for (auto it = _roomViewportsSorted.rbegin(); it != _roomViewportsSorted.rend(); ++it)
        if ((*it)->IsVisible() && (*it)->GetRect().IsInside(x, y))
            return *it;
    return nullptr;
}

Rect GamePlayState::GetRoomViewportAbs(int index) const
{
    return Rect::MoveBy(_roomViewports[index]->GetRect(), _mainViewport.Left, _mainViewport.Top);
}

void GamePlayState::SetUIViewport(const Rect &viewport)
{
    _uiViewport = viewport;
}

static bool ViewportZOrder(const PViewport e1, const PViewport e2)
{
    return e1->GetZOrder() < e2->GetZOrder();
}

void GamePlayState::UpdateViewports()
{
    if (_mainViewportHasChanged)
    {
        on_mainviewport_changed();
        _mainViewportHasChanged = false;
    }
    if (_roomViewportZOrderChanged)
    {
        auto old_sort = _roomViewportsSorted;
        _roomViewportsSorted = _roomViewports;
        std::sort(_roomViewportsSorted.begin(), _roomViewportsSorted.end(), ViewportZOrder);
        for (size_t i = 0; i < _roomViewportsSorted.size(); ++i)
        {
            if (i >= old_sort.size() || _roomViewportsSorted[i] != old_sort[i])
                _roomViewportsSorted[i]->SetChangedVisible();
        }
        _roomViewportZOrderChanged = false;
    }
    size_t vp_changed = SIZE_MAX;
    for (size_t i = _roomViewportsSorted.size(); i-- > 0;)
    {
        auto vp = _roomViewportsSorted[i];
        if (vp->HasChangedSize() || vp->HasChangedPosition() || vp->HasChangedVisible())
        {
            vp_changed = i;
            on_roomviewport_changed(vp.get());
            vp->ClearChangedFlags();
        }
    }
    if (vp_changed != SIZE_MAX)
        detect_roomviewport_overlaps(vp_changed);
    for (auto cam : _roomCameras)
    {
        if (cam->HasChangedSize() || cam->HasChangedPosition())
        {
            on_roomcamera_changed(cam.get());
            cam->ClearChangedFlags();
        }
    }
}

void GamePlayState::InvalidateViewportZOrder()
{
    _roomViewportZOrderChanged = true;
}

PCamera GamePlayState::GetRoomCamera(int index) const
{
    return _roomCameras[index];
}

void GamePlayState::UpdateRoomCameras()
{
    for (size_t i = 0; i < _roomCameras.size(); ++i)
        UpdateRoomCamera(i);
}

void GamePlayState::UpdateRoomCamera(int index)
{
    auto cam = _roomCameras[index];
    const Rect &rc = cam->GetRect();
    const Size real_room_sz = Size(thisroom.Width, thisroom.Height);
    if ((real_room_sz.Width > rc.GetWidth()) || (real_room_sz.Height > rc.GetHeight()))
    {
        // TODO: split out into Camera Behavior
        if (!cam->IsLocked() && playerchar->is_enabled())
        {
            int x = playerchar->x - rc.GetWidth() / 2;
            int y = playerchar->y - rc.GetHeight() / 2;
            cam->SetAt(x, y);
        }
    }
}

Point GamePlayState::RoomToScreen(int roomx, int roomy)
{
    return _roomViewports[0]->RoomToScreen(roomx, roomy, false).first;
}

int GamePlayState::RoomToScreenX(int roomx)
{
    return _roomViewports[0]->RoomToScreen(roomx, 0, false).first.X;
}

int GamePlayState::RoomToScreenY(int roomy)
{
    return _roomViewports[0]->RoomToScreen(0, roomy, false).first.Y;
}

VpPoint GamePlayState::ScreenToRoomImpl(int scrx, int scry, int view_index, bool clip_viewport)
{
    PViewport view;
    if (view_index < 0)
    {
        view = GetRoomViewportAt(scrx, scry);
        if (!view)
        {
            if (clip_viewport)
                return std::make_pair(Point(), -1);
            view = _roomViewports[0]; // use primary viewport
        }
    }
    else
    {
        view = _roomViewports[view_index];
    }
    return view->ScreenToRoom(scrx, scry, clip_viewport);
}

VpPoint GamePlayState::ScreenToRoom(int scrx, int scry, bool restrict)
{
    if (game.options[OPT_BASESCRIPTAPI] >= kScriptAPI_v3507)
        return ScreenToRoomImpl(scrx, scry, -1, restrict);
    return ScreenToRoomImpl(scrx, scry, 0, false);
}

void GamePlayState::CreatePrimaryViewportAndCamera()
{
    if (_roomViewports.size() == 0)
    {
        play.CreateRoomViewport();
        play.RegisterRoomViewport(0);
    }
    if (_roomCameras.size() == 0)
    {
        play.CreateRoomCamera();
        play.RegisterRoomCamera(0);
    }
    _roomViewports[0]->LinkCamera(_roomCameras[0]);
    _roomCameras[0]->LinkToViewport(_roomViewports[0]);
}

PViewport GamePlayState::CreateRoomViewport()
{
    int index = (int)_roomViewports.size();
    PViewport viewport(new Viewport());
    viewport->SetID(index);
    viewport->SetRect(_mainViewport);
    _roomViewports.push_back(viewport);
    _scViewportHandles.push_back(0);
    _roomViewportsSorted.push_back(viewport);
    _roomViewportZOrderChanged = true;
    on_roomviewport_created(index);
    return viewport;
}

ScriptViewport *GamePlayState::RegisterRoomViewport(int index, int32_t handle)
{
    if (index < 0 || (size_t)index >= _roomViewports.size())
        return nullptr;
    auto scview = new ScriptViewport(index);
    if (handle == 0)
    {
        handle = ccRegisterManagedObjectAndRef(scview, scview); // one ref for GamePlayState
    }
    else
    {
        ccRegisterUnserializedObject(handle, scview, scview);
    }
    _scViewportHandles[index] = handle; // save handle for us
    return scview;
}

void GamePlayState::DeleteRoomViewport(int index)
{
    if (index < 0 || (size_t)index >= _roomViewports.size())
        return;
    auto handle = _scViewportHandles[index];
    auto scobj = (ScriptViewport*)ccGetObjectAddressFromHandle(handle);
    if (scobj)
    {
        scobj->Invalidate();
        ccReleaseObjectReference(handle);
    }
    auto cam = _roomViewports[index]->GetCamera();
    if (cam)
        cam->UnlinkFromViewport(index);
    _roomViewports.erase(_roomViewports.begin() + index);
    _scViewportHandles.erase(_scViewportHandles.begin() + index);
    for (size_t i = index; i < _roomViewports.size(); ++i)
    {
        _roomViewports[i]->SetID(i);
        handle = _scViewportHandles[index];
        scobj = (ScriptViewport*)ccGetObjectAddressFromHandle(handle);
        if (scobj)
            scobj->SetID(i);
    }
    for (size_t i = 0; i < _roomViewportsSorted.size(); ++i)
    {
        if (_roomViewportsSorted[i]->GetID() == index)
        {
            _roomViewportsSorted.erase(_roomViewportsSorted.begin() + i);
            break;
        }
    }
    on_roomviewport_deleted(index);
}

int GamePlayState::GetRoomViewportCount() const
{
    return (int)_roomViewports.size();
}

PCamera GamePlayState::CreateRoomCamera()
{
    int index = (int)_roomCameras.size();
    PCamera camera(new Camera());
    camera->SetID(index);
    camera->SetAt(0, 0);
    camera->SetSize(_mainViewport.GetSize());
    _scCameraHandles.push_back(0);
    _roomCameras.push_back(camera);
    return camera;
}

ScriptCamera *GamePlayState::RegisterRoomCamera(int index, int32_t handle)
{
    if (index < 0 || (size_t)index >= _roomCameras.size())
        return nullptr;
    auto sccamera = new ScriptCamera(index);
    if (handle == 0)
    {
        handle = ccRegisterManagedObjectAndRef(sccamera, sccamera); // one ref for GamePlayState
    }
    else
    {
        ccRegisterUnserializedObject(handle, sccamera, sccamera);
    }
    _scCameraHandles[index] = handle;
    return sccamera;
}

void GamePlayState::DeleteRoomCamera(int index)
{
    if (index < 0 || (size_t)index >= _roomCameras.size())
        return;
    auto handle = _scCameraHandles[index];
    auto scobj = (ScriptCamera*)ccGetObjectAddressFromHandle(handle);
    if (scobj)
    {
        scobj->Invalidate();
        ccReleaseObjectReference(handle);
    }
    for (auto& viewref : _roomCameras[index]->GetLinkedViewports())
    {
        auto view = viewref.lock();
        if (view)
            view->LinkCamera(nullptr);
    }
    _roomCameras.erase(_roomCameras.begin() + index);
    _scCameraHandles.erase(_scCameraHandles.begin() + index);
    for (size_t i = index; i < _roomCameras.size(); ++i)
    {
        _roomCameras[i]->SetID(i);
        handle = _scCameraHandles[index];
        scobj = (ScriptCamera*)ccGetObjectAddressFromHandle(handle);
        if (scobj)
            scobj->SetID(i);
    }
}

int GamePlayState::GetRoomCameraCount() const
{
    return (int)_roomCameras.size();
}

ScriptViewport *GamePlayState::GetScriptViewport(int index)
{
    if (index < 0 || (size_t)index >= _roomViewports.size())
        return nullptr;
    return (ScriptViewport*)ccGetObjectAddressFromHandle(_scViewportHandles[index]);
}

ScriptCamera *GamePlayState::GetScriptCamera(int index)
{
    if (index < 0 || (size_t)index >= _roomCameras.size())
        return nullptr;
    return (ScriptCamera*)ccGetObjectAddressFromHandle(_scCameraHandles[index]);
}

bool GamePlayState::IsIgnoringInput() const
{
    return AGS_Clock::now() < _ignoreUserInputUntilTime;
}

void GamePlayState::SetIgnoreInput(int timeout_ms)
{
    if (AGS_Clock::now() + std::chrono::milliseconds(timeout_ms) > _ignoreUserInputUntilTime)
        _ignoreUserInputUntilTime = AGS_Clock::now() + std::chrono::milliseconds(timeout_ms);
}

void GamePlayState::ClearIgnoreInput()
{
    _ignoreUserInputUntilTime = AGS_Clock::now();
}

void GamePlayState::SetWaitSkipResult(int how, int data)
{
    wait_counter = 0;
    wait_skipped_by = how;
    wait_skipped_by_data = data;
}

int GamePlayState::GetWaitSkipResult() const
{ // NOTE: we remove timer flag to make timeout reason = 0
    return ((wait_skipped_by & ~SKIP_AUTOTIMER) << SKIP_RESULT_TYPE_SHIFT)
        | (wait_skipped_by_data & SKIP_RESULT_DATA_MASK);
}

bool GamePlayState::IsBlockingVoiceSpeech() const
{
    return speech_has_voice && speech_voice_blocking;
}

bool GamePlayState::IsNonBlockingVoiceSpeech() const
{
    return speech_has_voice && !speech_voice_blocking;
}

bool GamePlayState::ShouldPlayVoiceSpeech() const
{
    return !play.fast_forward &&
        (play.speech_mode != kSpeech_TextOnly) && (play.voice_avail);
}

void GamePlayState::ReadFromSavegame(Stream *in, GameDataVersion data_ver, GameStateSvgVersion svg_ver, RestoredData &r_data)
{
    in->ReadInt32(); // [DEPRECATED]
    usedmode = in->ReadInt32();
    disabled_user_interface = in->ReadInt32();
    gscript_timer = in->ReadInt32();
    debug_mode = in->ReadInt32();
    in->Seek(sizeof(int32_t) * LEGACY_MAXGLOBALVARS);
    messagetime = in->ReadInt32();
    usedinv = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();
    text_speed = in->ReadInt32();
    sierra_inv_color = in->ReadInt32();
    talkanim_speed = in->ReadInt32();
    inv_item_wid = in->ReadInt32();
    inv_item_hit = in->ReadInt32();
    speech_text_shadow = in->ReadInt32();
    swap_portrait_side = in->ReadInt32();
    speech_textwindow_gui = in->ReadInt32();
    follow_change_room_timer = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    skip_display = in->ReadInt32();
    no_multiloop_repeat = in->ReadInt32();
    roomscript_finished = in->ReadInt32();
    used_inv_on = in->ReadInt32();
    no_textbg_when_voice = in->ReadInt32();
    max_dialogoption_width = in->ReadInt32();
    no_hicolor_fadein = in->ReadInt32();
    bgspeech_game_speed = in->ReadInt32();
    bgspeech_stay_on_display = in->ReadInt32();
    unfactor_speech_from_textlength = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    speech_music_drop = in->ReadInt32();
    in_cutscene = in->ReadInt32();
    fast_forward = in->ReadInt32();
    room_width = in->ReadInt32();
    room_height = in->ReadInt32();
    game_speed_modifier = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    takeover_data = in->ReadInt32();
    replay_hotkey_unused = in->ReadInt32();
    dialog_options_x = in->ReadInt32();
    dialog_options_y = in->ReadInt32();
    narrator_speech = in->ReadInt32();
    in->ReadInt32();// [DEPRECATED]
    lipsync_speed = in->ReadInt32();
    close_mouth_speech_time = in->ReadInt32();
    disable_antialiasing = in->ReadInt32();
    text_speed_modifier = in->ReadInt32();
    if (svg_ver < kGSSvgVersion_350)
        text_align = ConvertLegacyScriptAlignment((LegacyScriptAlignment)in->ReadInt32());
    else
        text_align = (HorAlignment)in->ReadInt32();
    speech_bubble_width = in->ReadInt32();
    min_dialogoption_width = in->ReadInt32();
    disable_dialog_parser = in->ReadInt32();
    anim_background_speed = in->ReadInt32();  // the setting for this room
    top_bar_backcolor= in->ReadInt32();
    top_bar_textcolor = in->ReadInt32();
    top_bar_bordercolor = in->ReadInt32();
    top_bar_borderwidth = in->ReadInt32();
    top_bar_ypos = in->ReadInt32();
    screenshot_width = in->ReadInt32();
    screenshot_height = in->ReadInt32();
    top_bar_font = in->ReadInt32();
    if (svg_ver < kGSSvgVersion_350)
        speech_text_align = ConvertLegacyScriptAlignment((LegacyScriptAlignment)in->ReadInt32());
    else
        speech_text_align = (HorAlignment)in->ReadInt32();
    auto_use_walkto_points = in->ReadInt32();
    inventory_greys_out = in->ReadInt32();
    skip_speech_specific_key = in->ReadInt32();
    abort_key = in->ReadInt32();
    fade_to_red = in->ReadInt32();
    fade_to_green = in->ReadInt32();
    fade_to_blue = in->ReadInt32();
    show_single_dialog_option = in->ReadInt32();
    keep_screen_during_instant_transition = in->ReadInt32();
    read_dialog_option_colour = in->ReadInt32();
    stop_dialog_at_end = in->ReadInt32();
    speech_portrait_placement = in->ReadInt32();
    speech_portrait_x = in->ReadInt32();
    speech_portrait_y = in->ReadInt32();
    speech_display_post_time_ms = in->ReadInt32();
    dialog_options_highlight_color = in->ReadInt32();
    randseed = in->ReadInt32();    // random seed
    player_on_region = in->ReadInt32();    // player's current region
    check_interaction_only = in->ReadInt32();
    bg_frame = in->ReadInt32();
    bg_anim_delay = in->ReadInt32();  // for animating backgrounds
    in->ReadInt32(); // [DEPRECATED]
    wait_counter = in->ReadInt16();
    mboundx1 = in->ReadInt16();
    mboundx2 = in->ReadInt16();
    mboundy1 = in->ReadInt16();
    mboundy2 = in->ReadInt16();
    fade_effect = in->ReadInt32();
    bg_frame_locked = in->ReadInt32();
    in->Seek(sizeof(int32_t) * LEGACY_MAXGSVALUES);
    in->ReadInt32();// [DEPRECATED]
    in->ReadInt32();
    in->ReadInt32();
    audio_master_volume = in->ReadInt32();
    in->Read(walkable_areas_on, MAX_WALK_AREAS);
    screen_flipped = in->ReadInt16();
    if (svg_ver < kGSSvgVersion_350_10)
    {
        short offsets_locked = in->ReadInt16();
        if (offsets_locked != 0)
            r_data.Camera0_Flags = kSvgCamPosLocked;
    }
    entered_at_x = in->ReadInt32();
    entered_at_y = in->ReadInt32();
    entered_edge = in->ReadInt32();
    speech_mode = (SpeechMode)in->ReadInt32();
    speech_skip_style = in->ReadInt32();
    in->ReadArrayOfInt32(script_timers, MAX_TIMERS);
    in->ReadInt32();// [DEPRECATED]
    speech_volume = in->ReadInt32();
    normal_font = in->ReadInt32();
    speech_font = in->ReadInt32();
    key_skip_wait = in->ReadInt8();
    swap_portrait_lastchar = in->ReadInt32();
    separate_music_lib = in->ReadInt32() != 0;
    in_conversation = in->ReadInt32();
    screen_tint = in->ReadInt32();
    num_parsed_words = in->ReadInt32();
    in->ReadArrayOfInt16( parsed_words, MAX_PARSED_WORDS);
    in->Read( bad_parsed_word, 100);
    raw_color = in->ReadInt32();
    in->Seek(LEGACY_MAXSAVEGAMES * sizeof(int16_t));// [DEPRECATED]
    mouse_cursor_hidden = in->ReadInt32();
    in->ReadInt32();// [DEPRECATED]
    in->ReadInt32();
    in->ReadInt32();
    shakesc_delay = in->ReadInt32();
    shakesc_amount = in->ReadInt32();
    shakesc_length = in->ReadInt32();
    rtint_red = in->ReadInt32();
    rtint_green = in->ReadInt32();
    rtint_blue = in->ReadInt32();
    rtint_level = in->ReadInt32();
    rtint_light = in->ReadInt32();
    rtint_enabled = in->ReadBool();
    in->ReadInt32();// [DEPRECATED]
    skip_until_char_stops = in->ReadInt32();
    get_loc_name_last_time = in->ReadInt32();
    get_loc_name_save_cursor = in->ReadInt32();
    restore_cursor_mode_to = in->ReadInt32();
    restore_cursor_image_to = in->ReadInt32();
    in->ReadInt16(); // legacy music queue
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
        in->ReadInt16();
    new_music_queue_size = in->ReadInt16();
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        new_music_queue[i].ReadFromSavegame(in);
    }

    crossfading_out_channel = in->ReadInt16();
    crossfade_step = in->ReadInt16();
    crossfade_out_volume_per_step = in->ReadInt16();
    crossfade_initial_volume_out = in->ReadInt16();
    crossfading_in_channel = in->ReadInt16();
    crossfade_in_volume_per_step = in->ReadInt16();
    crossfade_final_volume_in = in->ReadInt16();

    in->Seek(50);// [DEPRECATED]
    in->Seek(50);// [DEPRECATED]
    in->Seek(LEGACY_MAXGLOBALSTRINGS * MAX_MAXSTRLEN);
    in->Read(lastParserEntry, MAX_MAXSTRLEN);
    if ((svg_ver < kGSSvgVersion_361_14) || (svg_ver >= kGSSvgVersion_400 && svg_ver < kGSSvgVersion_400_03))
        game_name.ReadCount(in, LEGACY_GAMENAMELENGTH);
    else
        game_name = StrUtil::ReadString(in);
    ground_level_areas_disabled = in->ReadInt32();
    next_screen_transition = in->ReadInt32();
    in->ReadInt32(); // gamma_adjustment -- do not apply gamma level from savegame
    temporarily_turned_off_character = in->ReadInt16();
    in->ReadInt16(); // [DEPRECATED]
    uint32_t num_do_once_tokens = static_cast<uint32_t>(in->ReadInt32());
    for (size_t i = 0; i < num_do_once_tokens; ++i)
    {
        do_once_tokens.insert(StrUtil::ReadString(in));
    }
    text_min_display_time_ms = in->ReadInt32();
    ignore_user_input_after_text_timeout_ms = in->ReadInt32();
    if (svg_ver < kGSSvgVersion_350_9)
    {
        in->ReadInt32(); // ignore_user_input_until_time -- do not apply from savegame
    }
    else // (svg_ver >= kGSSvgVersion_350_9)
    {
        int voice_speech_flags = in->ReadInt32();
        speech_has_voice = voice_speech_flags != 0;
        speech_voice_blocking = (voice_speech_flags & 0x02) != 0;
    }
    if (svg_ver >= kGSSvgVersion_400_03)
    {
        face_dir_ratio = in->ReadFloat32();
        // reserve few more 32-bit values (for a total of 10)
        in->Seek(sizeof(int32_t) * 9);
    }
}

void GamePlayState::WriteForSavegame(Stream *out) const
{
    // NOTE: following parameters are never saved:
    // recording, playback, gamestep, screen_is_faded_out, room_changes
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(usedmode);
    out->WriteInt32(disabled_user_interface);
    out->WriteInt32(gscript_timer);
    out->WriteInt32(debug_mode);
    out->WriteByteCount(0, sizeof(int32_t) * LEGACY_MAXGLOBALVARS);
    out->WriteInt32(messagetime);
    out->WriteInt32(usedinv);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(text_speed);
    out->WriteInt32(sierra_inv_color);
    out->WriteInt32(talkanim_speed);
    out->WriteInt32(inv_item_wid);
    out->WriteInt32(inv_item_hit);
    out->WriteInt32(speech_text_shadow);
    out->WriteInt32(swap_portrait_side);
    out->WriteInt32(speech_textwindow_gui);
    out->WriteInt32(follow_change_room_timer);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(skip_display);
    out->WriteInt32(no_multiloop_repeat);
    out->WriteInt32(roomscript_finished);
    out->WriteInt32(used_inv_on);
    out->WriteInt32(no_textbg_when_voice);
    out->WriteInt32(max_dialogoption_width);
    out->WriteInt32(no_hicolor_fadein);
    out->WriteInt32(bgspeech_game_speed);
    out->WriteInt32(bgspeech_stay_on_display);
    out->WriteInt32(unfactor_speech_from_textlength);
    out->WriteInt32(0);// [DEPRECATED]
    out->WriteInt32(speech_music_drop);
    out->WriteInt32(in_cutscene);
    out->WriteInt32(fast_forward);
    out->WriteInt32(room_width);
    out->WriteInt32(room_height);
    out->WriteInt32(game_speed_modifier);
    out->WriteInt32(0);// [DEPRECATED]
    out->WriteInt32(takeover_data);
    out->WriteInt32(replay_hotkey_unused);         // StartRecording: not supported
    out->WriteInt32(dialog_options_x);
    out->WriteInt32(dialog_options_y);
    out->WriteInt32(narrator_speech);
    out->WriteInt32(0);// [DEPRECATED]
    out->WriteInt32(lipsync_speed);
    out->WriteInt32(close_mouth_speech_time);
    out->WriteInt32(disable_antialiasing);
    out->WriteInt32(text_speed_modifier);
    out->WriteInt32(text_align);
    out->WriteInt32(speech_bubble_width);
    out->WriteInt32(min_dialogoption_width);
    out->WriteInt32(disable_dialog_parser);
    out->WriteInt32(anim_background_speed);  // the setting for this room
    out->WriteInt32(top_bar_backcolor);
    out->WriteInt32(top_bar_textcolor);
    out->WriteInt32(top_bar_bordercolor);
    out->WriteInt32(top_bar_borderwidth);
    out->WriteInt32(top_bar_ypos);
    out->WriteInt32(screenshot_width);
    out->WriteInt32(screenshot_height);
    out->WriteInt32(top_bar_font);
    out->WriteInt32(speech_text_align);
    out->WriteInt32(auto_use_walkto_points);
    out->WriteInt32(inventory_greys_out);
    out->WriteInt32(skip_speech_specific_key);
    out->WriteInt32(abort_key);
    out->WriteInt32(fade_to_red);
    out->WriteInt32(fade_to_green);
    out->WriteInt32(fade_to_blue);
    out->WriteInt32(show_single_dialog_option);
    out->WriteInt32(keep_screen_during_instant_transition);
    out->WriteInt32(read_dialog_option_colour);
    out->WriteInt32(stop_dialog_at_end);
    out->WriteInt32(speech_portrait_placement);
    out->WriteInt32(speech_portrait_x);
    out->WriteInt32(speech_portrait_y);
    out->WriteInt32(speech_display_post_time_ms);
    out->WriteInt32(dialog_options_highlight_color);
    // ** up to here is referenced in the script "game." object
    out->WriteInt32(randseed);    // random seed
    out->WriteInt32( player_on_region);    // player's current region
    out->WriteInt32( check_interaction_only);
    out->WriteInt32( bg_frame);
    out->WriteInt32( bg_anim_delay);  // for animating backgrounds
    out->WriteInt32( 0);// [DEPRECATED]
    out->WriteInt16(wait_counter);
    out->WriteInt16(mboundx1);
    out->WriteInt16(mboundx2);
    out->WriteInt16(mboundy1);
    out->WriteInt16(mboundy2);
    out->WriteInt32( fade_effect);
    out->WriteInt32( bg_frame_locked);
    out->WriteByteCount(0, sizeof(int32_t) * LEGACY_MAXGSVALUES);
    out->WriteInt32( 0);// [DEPRECATED]
    out->WriteInt32( 0);
    out->WriteInt32( 0);
    out->WriteInt32( audio_master_volume);
    out->Write(walkable_areas_on, MAX_WALK_AREAS);
    out->WriteInt16( screen_flipped);
    out->WriteInt32( entered_at_x);
    out->WriteInt32( entered_at_y);
    out->WriteInt32( entered_edge);
    out->WriteInt32( speech_mode);
    out->WriteInt32( speech_skip_style);
    out->WriteArrayOfInt32(script_timers, MAX_TIMERS);
    out->WriteInt32( 0);// [DEPRECATED]
    out->WriteInt32( speech_volume);
    out->WriteInt32( normal_font);
    out->WriteInt32( speech_font);
    out->WriteInt8( key_skip_wait);
    out->WriteInt32( swap_portrait_lastchar);
    out->WriteInt32( separate_music_lib ? 1 : 0);
    out->WriteInt32( in_conversation);
    out->WriteInt32( screen_tint);
    out->WriteInt32( num_parsed_words);
    out->WriteArrayOfInt16( parsed_words, MAX_PARSED_WORDS);
    out->Write( bad_parsed_word, 100);
    out->WriteInt32( raw_color);
    out->WriteByteCount(0, LEGACY_MAXSAVEGAMES * sizeof(int16_t));// [DEPRECATED]
    out->WriteInt32( mouse_cursor_hidden);
    out->WriteInt32( 0);// [DEPRECATED]
    out->WriteInt32( 0);
    out->WriteInt32( 0);
    out->WriteInt32( shakesc_delay);
    out->WriteInt32( shakesc_amount);
    out->WriteInt32( shakesc_length);
    out->WriteInt32( rtint_red);
    out->WriteInt32( rtint_green);
    out->WriteInt32( rtint_blue);
    out->WriteInt32( rtint_level);
    out->WriteInt32( rtint_light);
    out->WriteBool(rtint_enabled);
    out->WriteInt32( 0);// [DEPRECATED]
    out->WriteInt32( skip_until_char_stops);
    out->WriteInt32( get_loc_name_last_time);
    out->WriteInt32( get_loc_name_save_cursor);
    out->WriteInt32( restore_cursor_mode_to);
    out->WriteInt32( restore_cursor_image_to);
    out->WriteInt16( 0 ); // legacy music queue
    out->WriteByteCount( 0, sizeof(int16_t) * MAX_QUEUED_MUSIC);
    out->WriteInt16(new_music_queue_size);
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        new_music_queue[i].WriteToSavegame(out);
    }

    out->WriteInt16( crossfading_out_channel);
    out->WriteInt16( crossfade_step);
    out->WriteInt16( crossfade_out_volume_per_step);
    out->WriteInt16( crossfade_initial_volume_out);
    out->WriteInt16( crossfading_in_channel);
    out->WriteInt16( crossfade_in_volume_per_step);
    out->WriteInt16( crossfade_final_volume_in);

    out->WriteByteCount(0, 50);// [DEPRECATED]
    out->WriteByteCount(0, 50);// [DEPRECATED]
    out->WriteByteCount(0, LEGACY_MAXGLOBALSTRINGS * MAX_MAXSTRLEN);
    out->Write(lastParserEntry, MAX_MAXSTRLEN);
    StrUtil::WriteString(game_name, out);
    out->WriteInt32( ground_level_areas_disabled);
    out->WriteInt32( next_screen_transition);
    out->WriteInt32( gamma_adjustment);
    out->WriteInt16(temporarily_turned_off_character);
    out->WriteInt16(0); // [DEPRECATED]
    out->WriteInt32(static_cast<uint32_t>(do_once_tokens.size()));
    for (const auto &token : do_once_tokens)
    {
        StrUtil::WriteString(token, out);
    }
    out->WriteInt32( text_min_display_time_ms);
    out->WriteInt32( ignore_user_input_after_text_timeout_ms);

    int voice_speech_flags = speech_has_voice ? 0x01 : 0;
    if (speech_voice_blocking)
        voice_speech_flags |= 0x02;
    out->WriteInt32(voice_speech_flags);

    // --- kGSSvgVersion_400_03
    out->WriteFloat32(face_dir_ratio);
    // reserve few more 32-bit values (for a total of 10)
    out->WriteByteCount(0, sizeof(int32_t) * 9);
}

void GamePlayState::FreeProperties()
{
    for (auto &p : audioclipProps)
        p.clear();
    for (auto &p : charProps)
        p.clear();
    for (auto &p : dialogProps)
        p.clear();
    for (auto &p : guiProps)
        p.clear();
    for (auto &p : invProps)
        p.clear();
}

void GamePlayState::FreeViewportsAndCameras()
{
    _roomViewports.clear();
    _roomViewportsSorted.clear();
    for (auto handle : _scViewportHandles)
    {
        auto scview = (ScriptViewport*)ccGetObjectAddressFromHandle(handle);
        if (scview)
        {
            scview->Invalidate();
            ccReleaseObjectReference(handle);
        }
    }
    _scViewportHandles.clear();
    _roomCameras.clear();
    for (auto handle : _scCameraHandles)
    {
        auto sccam = (ScriptCamera*)ccGetObjectAddressFromHandle(handle);
        if (sccam)
        {
            sccam->Invalidate();
            ccReleaseObjectReference(handle);
        }
    }
    _scCameraHandles.clear();
}

// Converts legacy alignment type used in script API
HorAlignment ConvertLegacyScriptAlignment(LegacyScriptAlignment align)
{
    switch (align)
    {
    case kLegacyScAlignLeft: return kHAlignLeft;
    case kLegacyScAlignCentre: return kHAlignCenter;
    case kLegacyScAlignRight: return kHAlignRight;
    }
    return kHAlignNone;
}

// Reads legacy alignment type from the value set in script depending on the
// current Script API level. This is made to make it possible to change
// Alignment constants in the Script API and still support old version.
HorAlignment ReadScriptAlignment(int32_t align)
{
    return game.options[OPT_BASESCRIPTAPI] < kScriptAPI_v350 ?
        ConvertLegacyScriptAlignment((LegacyScriptAlignment)align) :
        (HorAlignment)align;
}

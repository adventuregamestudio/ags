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
#ifndef __AC_GAMESTATE_H
#define __AC_GAMESTATE_H

#include <memory>
#include <vector>
#include <unordered_set>
#include "ac/characterinfo.h"
#include "ac/characterextras.h"
#include "ac/keycode.h"
#include "ac/movelist.h"
#include "ac/runtime_defines.h"
#include "ac/speech.h"
#include "ac/timer.h"
#include "game/roomstruct.h"
#include "game/viewport.h"
#include "gfx/graphicsdriver.h"
#include "media/audio/queuedaudioitem.h"
#include "util/geometry.h"
#include "util/string_types.h"
#include "util/string.h"

// Forward declaration
namespace AGS
{
    namespace Common
    {
        class Bitmap; class Stream;
        typedef std::shared_ptr<Bitmap> PBitmap;
    }
    namespace Engine { struct RestoredData; }
}
using namespace AGS; // FIXME later
struct ScriptViewport;
struct ScriptCamera;
struct ScriptOverlay;

// GameState struct's runtime save format
enum GameStateSvgVersion
{
    kGSSvgVersion_Initial   = 0,
    kGSSvgVersion_350       = 1,
    kGSSvgVersion_350_9     = 2,
    kGSSvgVersion_350_10    = 3,
    kGSSvgVersion_400       = 4000000,
};


// Runtime game state
struct GameState {
    // TODO: this is left purely to load older save version, revise later
    static const int LEGACY_MAXGLOBALVARS = 50;
    static const int LEGACY_MAXGSVALUES = 500;
    static const int LEGACY_MAXGLOBALSTRINGS = 51;

    int  usedmode = 0;              // set by ProcessClick to last cursor mode used
    int  disabled_user_interface = 0;  // >0 while in cutscene/etc
    int  gscript_timer = 0;         // obsolete
    int  debug_mode = 0;            // whether we're in debug mode
    int  messagetime = 0;           // time left for auto-remove messages
    int  usedinv = 0;               // inventory item last used
    int  text_speed = 0;            // how quickly text is removed
    int  sierra_inv_color = 0;      // background used to paint defualt inv window
    int  talkanim_speed = 0;        // animation speed of talking anims
    int  inv_item_wid,inv_item_hit = 0;  // set by SetInvDimensions
    int  speech_text_shadow = 0;         // colour of outline fonts (default black)
    int  swap_portrait_side = 0;         // sierra-style speech swap sides
    int  speech_textwindow_gui = 0;      // textwindow used for sierra-style speech
    int  follow_change_room_timer = 0;   // delay before moving following characters into new room
    int  skip_display = 0;          // how the user can skip normal Display windows
    int  no_multiloop_repeat = 0;   // for backwards compatibility
    int  roomscript_finished = 0;   // on_call finished in room
    int  used_inv_on = 0;           // inv item they clicked on
    int  no_textbg_when_voice = 0;  // no textwindow bgrnd when voice speech is used
    int  max_dialogoption_width = 0; // max width of dialog options text window
    int  no_hicolor_fadein = 0;     // fade out but instant in for hi-color
    int  bgspeech_game_speed = 0;   // is background speech relative to game speed
    int  bgspeech_stay_on_display = 0; // whether to remove bg speech when DisplaySpeech is used
    int  unfactor_speech_from_textlength = 0; // remove "&10" when calculating time for text to stay
    int  speech_music_drop = 0;     // how much to drop music volume by when speech is played
    int  in_cutscene = 0;           // we are between a StartCutscene and EndCutscene
    int  fast_forward = 0;          // player has elected to skip cutscene
    int  room_width = 0;            // width of current room
    int  room_height = 0;           // height of current room
    int  game_speed_modifier = 0;
    int  takeover_data = 0;         // value passed to RunAGSGame in previous game
    int  replay_hotkey_unused = 0;  // (UNUSED!) StartRecording: not supported
    int  dialog_options_x = 0;
    int  dialog_options_y = 0;
    int  narrator_speech = 0;
    int  lipsync_speed = 0;
    int  close_mouth_speech_time = 0; // stop speech animation at (messagetime - close_mouth_speech_time)
                                    // (this is designed to work in text-only mode)
    int  disable_antialiasing = 0;
    int  text_speed_modifier = 0;
    HorAlignment text_align = kHAlignNone;
    int  speech_bubble_width = 0;
    int  min_dialogoption_width = 0;
    int  disable_dialog_parser = 0;
    int  anim_background_speed = 0; // the setting for this room
    int  top_bar_backcolor = 0;
    int  top_bar_textcolor = 0;
    int  top_bar_bordercolor = 0;
    int  top_bar_borderwidth = 0;
    int  top_bar_ypos = 0;
    int  screenshot_width = 0;
    int  screenshot_height = 0;
    int  top_bar_font = 0;
    HorAlignment speech_text_align = kHAlignNone;
    int  auto_use_walkto_points = 0;
    int  inventory_greys_out = 0;
    int  skip_speech_specific_key = 0;
    int  abort_key = 0;
    int  fade_to_red = 0;
    int  fade_to_green = 0;
    int  fade_to_blue = 0;
    int  show_single_dialog_option = 0;
    int  keep_screen_during_instant_transition = 0;
    int  read_dialog_option_colour = 0;
    int  stop_dialog_at_end = 0;
    int  speech_portrait_placement = 0; // speech portrait placement mode (automatic/custom)
    int  speech_portrait_x = 0;     // a speech portrait x offset from corresponding screen side
    int  speech_portrait_y = 0;     // a speech portrait y offset 
    int  speech_display_post_time_ms = 0; // keep speech text/portrait on screen after text/voice has finished playing;
                                    // no speech animation is supposed to be played at this time
    int  dialog_options_highlight_color = 0; // The colour used for highlighted (hovered over) text in dialog options
    long  randseed = 0;             // random seed
    int   player_on_region = 0;     // player's current region
    int   screen_is_faded_out = 0;  // the screen is currently black
    int   check_interaction_only = 0;
    int   bg_frame,bg_anim_delay = 0; // for animating backgrounds
    short wait_counter = 0;
    char  wait_skipped_by = 0;      // tells how last blocking wait was skipped [not serialized]
    int   wait_skipped_by_data = 0; // extended data telling how last blocking wait was skipped [not serialized]
    short mboundx1 = 0;
    short mboundx2 = 0;
    short mboundy1 = 0;
    short mboundy2 = 0;
    int   fade_effect = 0;
    int   bg_frame_locked = 0;
    int   audio_master_volume = 0; // in 0-100
    char  walkable_areas_on[MAX_WALK_AREAS + 1]{};
    short screen_flipped = 0;
    int   entered_at_x = 0;
    int   entered_at_y = 0;
    int   entered_edge = 0;
    bool  voice_avail = false;      // whether voice-over is available
    SpeechMode speech_mode = kSpeech_TextOnly; // speech mode (text, voice, or both)
    int   cant_skip_speech = 0;
    int   script_timers[MAX_TIMERS]{};
    int   speech_volume = 0; // in 0-255 !!
    int   normal_font = 0;
    int   speech_font = 0;
    char  key_skip_wait = 0;
    int   swap_portrait_lastchar = 0;
    int   swap_portrait_lastlastchar = 0;
    bool  separate_music_lib = false;
    int   in_conversation = 0;
    int   screen_tint = 0;
    int   num_parsed_words = 0;
    short parsed_words[MAX_PARSED_WORDS]{};
    char  bad_parsed_word[100]{};
    int   raw_color = 0;
    int   raw_modified[MAX_ROOM_BGFRAMES]{};
    Common::PBitmap raw_drawing_surface;
    short filenumbers[MAXSAVEGAMES]{}; // [OBSOLETE]
    int   room_changes = 0;
    int   mouse_cursor_hidden = 0;
    unsigned long shakesc_delay = 0;  // unsigned long to match loopcounter
    int   shakesc_amount = 0;
    int   shakesc_length = 0;
    int   rtint_red = 0;
    int   rtint_green = 0;
    int   rtint_blue = 0;
    int   rtint_level = 0;
    int   rtint_light = 0;
    bool  rtint_enabled = false;
    int   skip_until_char_stops = 0;
    int   get_loc_name_last_time = 0;
    int   get_loc_name_save_cursor = 0;
    int   restore_cursor_mode_to = 0;
    int   restore_cursor_image_to = 0;
    short crossfading_out_channel = 0;
    short crossfade_step = 0;
    short crossfade_out_volume_per_step = 0;
    short crossfade_initial_volume_out = 0;
    short crossfading_in_channel = 0;
    short crossfade_in_volume_per_step = 0;
    short crossfade_final_volume_in = 0;
    short new_music_queue_size = 0;
    QueuedAudioItem new_music_queue[MAX_QUEUED_MUSIC]{};
    char  lastParserEntry[MAX_MAXSTRLEN]{};
    char  game_name[100]{};
    int   ground_level_areas_disabled = 0;
    int   next_screen_transition = 0;
    int   gamma_adjustment = 0;
    short temporarily_turned_off_character = 0;  // Hide Player Charactr ticked
    std::vector<int> gui_draw_order; // used only for hit detection now
    std::unordered_set<AGS::Common::String> do_once_tokens;
    int   text_min_display_time_ms = 0;
    int   ignore_user_input_after_text_timeout_ms = 0;
    int   default_audio_type_volumes[MAX_AUDIO_TYPES]{};

    // Dynamic custom property values for characters and items
    std::vector<AGS::Common::StringIMap> charProps;
    AGS::Common::StringIMap invProps[MAX_INV];

    // Dynamic speech state
    //
    // Tells whether there is a voice-over played during current speech
    bool  speech_has_voice = false;
    // Tells whether the voice was played in blocking mode;
    // atm blocking speech handles itself, and we only need to finalize
    // non-blocking voice speech during game update; speech refactor would be
    // required to get rid of this rule.
    bool  speech_voice_blocking = false;
    // Tells whether character speech stays on screen not animated for additional time
    bool  speech_in_post_state = false;

    // Special overlays
    //
    // Is there a QFG4-style dialog overlay on screen (contains overlay ID)
    int  complete_overlay_on = 0;
    // Is there a blocking text overlay on screen (contains overlay ID)
    int  text_overlay_on = 0;
    // Script overlay handles, because we must return same script objects
    // whenever user script queries for them.
    // Blocking speech overlay managed handle
    int  speech_text_schandle = 0;
    // Speech portrait overlay managed handle
    int  speech_face_schandle = 0;

    // y offset of the shaking screen
    int shake_screen_yoff = 0;

    // CHECKME: we might consider hiding these in draw unit, but then,
    // because the drawing system still has standalone parts (GUIs...)
    // there still may be cases when we may require these accessed elsewhere.
    // Sprite modified flag, used to detect dynamic sprite modifications
    std::vector<bool> spritemodified;
    // Which sprites were modified since the recent drawing pass
    std::vector<int> spritemodifiedlist;


    GameState();

    //
    // Viewport and camera control.
    // Viewports are positioned in game screen coordinates, related to the "game size",
    // while cameras are positioned in room coordinates.
    //
    // Returns main (game's) viewport position on screen, this is the overall game view
    const Rect &GetMainViewport() const;
    // Returns UI viewport position on screen, within the main viewport
    const Rect &GetUIViewport() const;
    // Returns SpriteTransform corresponding to the global screen offsets
    AGS::Engine::SpriteTransform GetGlobalTransform(bool full_frame_rend) const;
    // Tells if the room viewport should be adjusted automatically each time a new room is loaded
    bool IsAutoRoomViewport() const;
    // Returns Room viewport object by it's main index
    PViewport  GetRoomViewport(int index) const;
    // Returns Room viewport object by index in z-order
    const std::vector<PViewport> &GetRoomViewportsZOrdered() const;
    // Finds room viewport at the given screen coordinates; returns nullptr if non found
    PViewport  GetRoomViewportAt(int x, int y) const;
    // Returns Room viewport position in absolute coordinates (with main viewport offset);
    // this is a helper function, meant for peculiar cases.
    Rect       GetRoomViewportAbs(int index) const;
    // Sets if the room viewport should be adjusted automatically each time a new room is loaded
    void SetAutoRoomViewport(bool on);
    // Main viewport defines the location of all things drawn and interactable on the game screen.
    // Other viewports are defined relative to the main viewports.
    void SetMainViewport(const Rect &viewport);
    // UI viewport is a formal dummy viewport for GUI and Overlays (like speech).
    void SetUIViewport(const Rect &viewport);
    // Applies all the pending changes to viewports and cameras;
    // NOTE: this function may be slow, thus recommended to be called only once
    // and during the main game update.
    void UpdateViewports();
    // Notifies game state that viewports need z-order resorting upon next update.
    void InvalidateViewportZOrder();
    // Returns room camera object chosen by index
    PCamera GetRoomCamera(int index) const;
    // Runs cameras behaviors
    void UpdateRoomCameras();
    // Converts room coordinates to the game screen coordinates through the room viewport
    // This group of functions always tries to pass a point through the **primary** room viewport
    // TODO: also support using arbitrary viewport (for multiple viewports).
    Point RoomToScreen(int roomx, int roomy);
    int  RoomToScreenX(int roomx);
    int  RoomToScreenY(int roomy);
    // Converts game screen coordinates to the room coordinates through the room viewport.
    // Tries to find if there is any viewport at the given coords.
    // If "clip_viewport" parameter is true, then not finding a viewport results in failure,
    // if it is false, proceeds converting through the primary viewport.
    VpPoint ScreenToRoom(int scrx, int scry, bool clip_viewport = true);

    // Makes sure primary viewport and camera are created and linked together
    void CreatePrimaryViewportAndCamera();
    // Creates new room viewport
    PViewport CreateRoomViewport();
    // Register camera in the managed system; optionally links to existing handle
    ScriptViewport *RegisterRoomViewport(int index, int32_t handle = 0);
    // Deletes existing room viewport
    void DeleteRoomViewport(int index);
    // Get number of room viewports
    int GetRoomViewportCount() const;
    // Creates new room camera
    PCamera CreateRoomCamera();
    // Register camera in the managed system; optionally links to existing handle
    ScriptCamera *RegisterRoomCamera(int index, int32_t handle = 0);
    // Deletes existing room camera
    void DeleteRoomCamera(int index);
    // Get number of room cameras
    int GetRoomCameraCount() const;
    // Gets script viewport reference; does NOT increment refcount
    // because script interpreter does this when acquiring managed pointer.
    ScriptViewport *GetScriptViewport(int index);
    // Gets script camera reference; does NOT increment refcount
    // because script interpreter does this when acquiring managed pointer.
    ScriptCamera *GetScriptCamera(int index);

    //
    // User input management
    //
    // Tells if game should ignore user input right now. Note that some of the parent states
    // may not ignore it at the same time, such as cutscene state, which may still be skipped
    // with a key press or a mouse button.
    bool IsIgnoringInput() const;
    // Sets ignore input state, for the given time; if there's one already, chooses max timeout
    void SetIgnoreInput(int timeout_ms);
    // Clears ignore input state
    void ClearIgnoreInput();

    // Set how the last blocking wait was skipped
    void SetWaitSkipResult(int how, int data = 0);
    void SetWaitKeySkip(const KeyInput &kp)
    {
        SetWaitSkipResult(SKIP_KEYPRESS, AGSKeyToScriptKey(kp.Key) | kp.Mod);
    }
    // Returns the information about how the latest blocking wait was skipped.
    // The information is packed into int32 value like this:
    // | 0xFF       | 0xFF    | 0xF      | 0xFFF                     |
    // | eInputType | eKeyMod | reserved | eKeyCode, MouseButton etc |
    int GetWaitSkipResult() const;

    //
    // Voice speech management
    //
    // Tells if there's a blocking voice speech playing right now
    bool IsBlockingVoiceSpeech() const;
    // Tells whether we have to finalize voice speech when stopping or reusing the channel
    bool IsNonBlockingVoiceSpeech() const;
    // Speech helpers
    bool ShouldPlayVoiceSpeech() const;

    //
    // Serialization
    //
    void ReadFromSavegame(Common::Stream *in, GameDataVersion data_ver, GameStateSvgVersion svg_ver, AGS::Engine::RestoredData &r_data);
    void WriteForSavegame(Common::Stream *out) const;
    // This is required for freeing only particular parts when restoring the game;
    // FIXME: investigate and refactor to be able to simply reset whole object
    void FreeProperties();
    void FreeViewportsAndCameras();

private:
    VpPoint ScreenToRoomImpl(int scrx, int scry, int view_index, bool clip_viewport);
    void UpdateRoomCamera(int index);

    // Defines if the room viewport should be adjusted to the room size automatically.
    bool _isAutoRoomViewport = true;
    // Main viewport defines the rectangle of the drawn and interactable area;
    // in the most basic case it will be equal to the game size.
    Rect _mainViewport;
    // UI viewport defines the render and interaction rectangle of game's UI,
    // within the main game viewport.
    Rect _uiViewport;
    // Room viewports define place on screen where the room camera's
    // contents are drawn.
    std::vector<PViewport> _roomViewports;
    // Vector of viewports sorted in z-order.
    std::vector<PViewport> _roomViewportsSorted;
    // Cameras defines the position of a "looking eye" inside the room.
    std::vector<PCamera> _roomCameras;
    // We keep handles to the script refs to viewports and cameras, so that we
    // could address them and invalidate as the actual object gets destroyed.
    std::vector<int32_t> _scViewportHandles;
    std::vector<int32_t> _scCameraHandles;

    // Tells that the main viewport's position has changed since last game update
    bool  _mainViewportHasChanged = false;
    // Tells that room viewports need z-order resort
    bool  _roomViewportZOrderChanged = false;

    AGS_Clock::time_point _ignoreUserInputUntilTime{};
};

// Converts legacy alignment type used in script API
HorAlignment ConvertLegacyScriptAlignment(LegacyScriptAlignment align);
// Reads legacy alignment type from the value set in script depending on the
// current Script API level. This is made to make it possible to change
// Alignment constants in the Script API and still support old version.
HorAlignment ReadScriptAlignment(int32_t align);

extern GameState play;
extern std::vector<CharacterExtras> charextra;
extern std::vector<MoveList> mls;

#endif // __AC_GAMESTATE_H

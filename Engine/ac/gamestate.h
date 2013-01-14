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

#include "ac/runtime_defines.h"
#include "media/audio/queuedaudioitem.h"
#include "util/file.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// Adding to this might need to modify AGSDEFNS.SH and AGSPLUGIN.H
struct GameState {
    int  score;      // player's current score
    int  usedmode;   // set by ProcessClick to last cursor mode used
    int  disabled_user_interface;  // >0 while in cutscene/etc
    int  gscript_timer;    // obsolete
    int  debug_mode;       // whether we're in debug mode
    int  globalvars[MAXGLOBALVARS];  // obsolete
    int  messagetime;      // time left for auto-remove messages
    int  usedinv;          // inventory item last used
    int  inv_top,inv_numdisp,obsolete_inv_numorder,inv_numinline;
    int  text_speed;       // how quickly text is removed
    int  sierra_inv_color; // background used to paint defualt inv window
    int  talkanim_speed;   // animation speed of talking anims
    int  inv_item_wid,inv_item_hit;  // set by SetInvDimensions
    int  speech_text_shadow;         // colour of outline fonts (default black)
    int  swap_portrait_side;         // sierra-style speech swap sides
    int  speech_textwindow_gui;      // textwindow used for sierra-style speech
    int  follow_change_room_timer;   // delay before moving following characters into new room
    int  totalscore;           // maximum possible score
    int  skip_display;         // how the user can skip normal Display windows
    int  no_multiloop_repeat;  // for backwards compatibility
    int  roomscript_finished;  // on_call finished in room
    int  used_inv_on;          // inv item they clicked on
    int  no_textbg_when_voice; // no textwindow bgrnd when voice speech is used
    int  max_dialogoption_width; // max width of dialog options text window
    int  no_hicolor_fadein;      // fade out but instant in for hi-color
    int  bgspeech_game_speed;    // is background speech relative to game speed
    int  bgspeech_stay_on_display; // whether to remove bg speech when DisplaySpeech is used
    int  unfactor_speech_from_textlength; // remove "&10" when calculating time for text to stay
    int  mp3_loop_before_end;    // loop this time before end of track (ms)
    int  speech_music_drop;      // how much to drop music volume by when speech is played
    int  in_cutscene;            // we are between a StartCutscene and EndCutscene
    int  fast_forward;           // player has elected to skip cutscene
    int  room_width;      // width of current room (320-res co-ordinates)
    int  room_height;     // height of current room (320-res co-ordinates)
    int  game_speed_modifier;
    int  score_sound;
    int  takeover_data;  // value passed to RunAGSGame in previous game
    int  replay_hotkey;
    int  dialog_options_x;
    int  dialog_options_y;
    int  narrator_speech;
    int  ambient_sounds_persist;
    int  lipsync_speed;
    int  close_mouth_speech_time;
    int  disable_antialiasing;
    int  text_speed_modifier;
    int  text_align;
    int  speech_bubble_width;
    int  min_dialogoption_width;
    int  disable_dialog_parser;
    int  anim_background_speed;  // the setting for this room
    int  top_bar_backcolor;
    int  top_bar_textcolor;
    int  top_bar_bordercolor;
    int  top_bar_borderwidth;
    int  top_bar_ypos;
    int  screenshot_width;
    int  screenshot_height;
    int  top_bar_font;
    int  speech_text_align;
    int  auto_use_walkto_points;
    int  inventory_greys_out;
    int  skip_speech_specific_key;
    int  abort_key;
    int  fade_to_red;
    int  fade_to_green;
    int  fade_to_blue;
    int  show_single_dialog_option;
    int  keep_screen_during_instant_transition;
    int  read_dialog_option_colour;
    int  stop_dialog_at_end;
    int  reserved[10];  // make sure if a future version adds a var, it doesn't mess anything up
    // ** up to here is referenced in the script "game." object
    int   recording;   // user is recording their moves
    int   playback;    // playing back recording
    short gamestep;    // step number for matching recordings
    long  randseed;    // random seed
    int   player_on_region;    // player's current region
    int   screen_is_faded_out; // the screen is currently black
    int   check_interaction_only;
    int   bg_frame,bg_anim_delay;  // for animating backgrounds
    int   music_vol_was;  // before the volume drop
    short wait_counter;
    short mboundx1,mboundx2,mboundy1,mboundy2;
    int   fade_effect;
    int   bg_frame_locked;
    int   globalscriptvars[MAXGSVALUES];
    int   cur_music_number,music_repeat;
    int   music_master_volume;
    int   digital_master_volume;
    char  walkable_areas_on[MAX_WALK_AREAS+1];
    short screen_flipped;
    short offsets_locked;
    int   entered_at_x,entered_at_y, entered_edge;
    int   want_speech;
    int   cant_skip_speech;
    int   script_timers[MAX_TIMERS];
    int   sound_volume,speech_volume;
    int   normal_font, speech_font;
    char  key_skip_wait;
    int   swap_portrait_lastchar;
    int   seperate_music_lib;
    int   in_conversation;
    int   screen_tint;
    int   num_parsed_words;
    short parsed_words[MAX_PARSED_WORDS];
    char  bad_parsed_word[100];
    int   raw_color;
    int   raw_modified[MAX_BSCENE];
    short filenumbers[MAXSAVEGAMES];
    int   room_changes;
    int   mouse_cursor_hidden;
    int   silent_midi;
    int   silent_midi_channel;
    int   current_music_repeating;  // remember what the loop flag was when this music started
    unsigned long shakesc_delay;  // unsigned long to match loopcounter
    int   shakesc_amount, shakesc_length;
    int   rtint_red, rtint_green, rtint_blue, rtint_level, rtint_light;
    int   end_cutscene_music;
    int   skip_until_char_stops;
    int   get_loc_name_last_time;
    int   get_loc_name_save_cursor;
    int   restore_cursor_mode_to;
    int   restore_cursor_image_to;
    short music_queue_size;
    short music_queue[MAX_QUEUED_MUSIC];
    short new_music_queue_size;
    short crossfading_out_channel;
    short crossfade_step;
    short crossfade_out_volume_per_step;
    short crossfade_initial_volume_out;
    short crossfading_in_channel;
    short crossfade_in_volume_per_step;
    short crossfade_final_volume_in;
    QueuedAudioItem new_music_queue[MAX_QUEUED_MUSIC];
    char  takeover_from[50];
    char  playmp3file_name[PLAYMP3FILE_MAX_FILENAME_LEN];
    char  globalstrings[MAXGLOBALSTRINGS][MAX_MAXSTRLEN];
    char  lastParserEntry[MAX_MAXSTRLEN];
    char  game_name[100];
    int   ground_level_areas_disabled;
    int   next_screen_transition;
    int   gamma_adjustment;
    short temporarily_turned_off_character;  // Hide Player Charactr ticked
    short inv_backwards_compatibility;
    int  *gui_draw_order;
    char**do_once_tokens;
    int   num_do_once_tokens;
    int   text_min_display_time_ms;
    int   ignore_user_input_after_text_timeout_ms;
    unsigned long ignore_user_input_until_time;
    int   default_audio_type_volumes[MAX_AUDIO_TYPES];

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
};

#endif // __AC_GAMESTATE_H

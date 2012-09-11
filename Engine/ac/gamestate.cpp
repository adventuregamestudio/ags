
#include "util/wgt2allg.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "util/string_utils.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

//
// [IKM] What must be kept in mind: in previous versions of AGS
// this struct was read and written as-is (read/write-ing object)
// It did not have virtual functions (did not have functions at all
// actually), so I believe there's no need to skip 4 bytes for
// vtable pointer.
// On other hand we should read/write even pointers to arrays
// (or at least emulate that), although that could make no sense.
//

extern GameSetupStruct game;

void GameState::ReadFromFile(CDataStream *in)
{
    char padding[3] = {0,0,0}; // to align data

    score = in->ReadInt32();
    usedmode = in->ReadInt32();
    disabled_user_interface = in->ReadInt32();
    gscript_timer = in->ReadInt32();
    debug_mode = in->ReadInt32();
    in->ReadArrayOfInt32(globalvars, MAXGLOBALVARS);
    messagetime = in->ReadInt32();
    usedinv = in->ReadInt32();
    inv_top = in->ReadInt32();
    inv_numdisp = in->ReadInt32();
    obsolete_inv_numorder = in->ReadInt32();
    inv_numinline = in->ReadInt32();
    text_speed = in->ReadInt32();
    sierra_inv_color = in->ReadInt32();
    talkanim_speed = in->ReadInt32();
    inv_item_wid = in->ReadInt32();
    inv_item_hit = in->ReadInt32();
    speech_text_shadow = in->ReadInt32();
    swap_portrait_side = in->ReadInt32();
    speech_textwindow_gui = in->ReadInt32();
    follow_change_room_timer = in->ReadInt32();
    totalscore = in->ReadInt32();
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
    mp3_loop_before_end = in->ReadInt32();
    speech_music_drop = in->ReadInt32();
    in_cutscene = in->ReadInt32();
    fast_forward = in->ReadInt32();
    room_width = in->ReadInt32();
    room_height = in->ReadInt32();
    game_speed_modifier = in->ReadInt32();
    score_sound = in->ReadInt32();
    takeover_data = in->ReadInt32();
    replay_hotkey = in->ReadInt32();
    dialog_options_x = in->ReadInt32();
    dialog_options_y = in->ReadInt32();
    narrator_speech = in->ReadInt32();
    ambient_sounds_persist = in->ReadInt32();
    lipsync_speed = in->ReadInt32();
    close_mouth_speech_time = in->ReadInt32();
    disable_antialiasing = in->ReadInt32();
    text_speed_modifier = in->ReadInt32();
    text_align = in->ReadInt32();
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
    speech_text_align = in->ReadInt32();
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
    in->ReadArrayOfInt32(reserved, 10);
    // ** up to here is referenced in the script "game." object
    recording = in->ReadInt32();   // user is recording their moves
    playback = in->ReadInt32();    // playing back recording
    gamestep = in->ReadInt16();    // step number for matching recordings
    in->Read(&padding, 2); // <-- padding
    randseed = in->ReadInt32();    // random seed
    player_on_region = in->ReadInt32();    // player's current region
    screen_is_faded_out = in->ReadInt32(); // the screen is currently black
    check_interaction_only = in->ReadInt32();
    bg_frame = in->ReadInt32();
    bg_anim_delay = in->ReadInt32();  // for animating backgrounds
    music_vol_was = in->ReadInt32();  // before the volume drop
    wait_counter = in->ReadInt16();
    mboundx1 = in->ReadInt16();
    mboundx2 = in->ReadInt16();
    mboundy1 = in->ReadInt16();
    mboundy2 = in->ReadInt16();
    in->Read(&padding, 2); // <-- padding
    fade_effect = in->ReadInt32();
    bg_frame_locked = in->ReadInt32();
    in->ReadArrayOfInt32(globalscriptvars, MAXGSVALUES);
    cur_music_number = in->ReadInt32();
    music_repeat = in->ReadInt32();
    music_master_volume = in->ReadInt32();
    digital_master_volume = in->ReadInt32();
    in->Read(walkable_areas_on, MAX_WALK_AREAS+1);
    screen_flipped = in->ReadInt16();
    offsets_locked = in->ReadInt16();
    entered_at_x = in->ReadInt32();
    entered_at_y = in->ReadInt32();
    entered_edge = in->ReadInt32();
    want_speech = in->ReadInt32();
    cant_skip_speech = in->ReadInt32();
    in->ReadArrayOfInt32(script_timers, MAX_TIMERS);
    sound_volume = in->ReadInt32();
    speech_volume = in->ReadInt32();
    normal_font = in->ReadInt32();
    speech_font = in->ReadInt32();
    key_skip_wait = in->ReadInt8();
    in->Read(&padding, 3); // <-- padding
    swap_portrait_lastchar = in->ReadInt32();
    seperate_music_lib = in->ReadInt32();
    in_conversation = in->ReadInt32();
    screen_tint = in->ReadInt32();
    num_parsed_words = in->ReadInt32();
    in->ReadArrayOfInt16( parsed_words, MAX_PARSED_WORDS);
    in->Read(&padding, 2); // <-- padding
    in->Read( bad_parsed_word, 100);
    raw_color = in->ReadInt32();
    in->ReadArrayOfInt32( raw_modified, MAX_BSCENE);
    in->ReadArrayOfInt16( filenumbers, MAXSAVEGAMES);
    room_changes = in->ReadInt32();
    mouse_cursor_hidden = in->ReadInt32();
    silent_midi = in->ReadInt32();
    silent_midi_channel = in->ReadInt32();
    current_music_repeating = in->ReadInt32();
    shakesc_delay = in->ReadInt32();
    shakesc_amount = in->ReadInt32();
    shakesc_length = in->ReadInt32();
    rtint_red = in->ReadInt32();
    rtint_green = in->ReadInt32();
    rtint_blue = in->ReadInt32();
    rtint_level = in->ReadInt32();
    rtint_light = in->ReadInt32();
    end_cutscene_music = in->ReadInt32();
    skip_until_char_stops = in->ReadInt32();
    get_loc_name_last_time = in->ReadInt32();
    get_loc_name_save_cursor = in->ReadInt32();
    restore_cursor_mode_to = in->ReadInt32();
    restore_cursor_image_to = in->ReadInt32();
    music_queue_size = in->ReadInt16();
    in->Read(&padding, 2); // <-- padding
    in->ReadArrayOfInt16( music_queue, MAX_QUEUED_MUSIC);
    new_music_queue_size = in->ReadInt16();
    crossfading_out_channel = in->ReadInt16();
    crossfade_step = in->ReadInt16();
    crossfade_out_volume_per_step = in->ReadInt16();
    crossfade_initial_volume_out = in->ReadInt16();
    crossfading_in_channel = in->ReadInt16();
    crossfade_in_volume_per_step = in->ReadInt16();
    crossfade_final_volume_in = in->ReadInt16();

    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        new_music_queue[i].ReadFromFile(in);
    }

    in->Read(takeover_from, 50);
    in->Read(playmp3file_name, PLAYMP3FILE_MAX_FILENAME_LEN);
    in->Read(globalstrings, MAXGLOBALSTRINGS * MAX_MAXSTRLEN);
    in->Read(lastParserEntry, MAX_MAXSTRLEN);
    in->Read(game_name, 100);
    ground_level_areas_disabled = in->ReadInt32();
    next_screen_transition = in->ReadInt32();
    gamma_adjustment = in->ReadInt32();
    temporarily_turned_off_character = in->ReadInt16();
    inv_backwards_compatibility = in->ReadInt16();
    gui_draw_order = (int*)in->ReadInt32();
    do_once_tokens = (char**)in->ReadInt32();
    num_do_once_tokens = in->ReadInt32();
    text_min_display_time_ms = in->ReadInt32();
    ignore_user_input_after_text_timeout_ms = in->ReadInt32();
    ignore_user_input_until_time = in->ReadInt32();
    in->ReadArrayOfInt32(default_audio_type_volumes, MAX_AUDIO_TYPES);
}

void GameState::WriteToFile(CDataStream *out)
{
    //-------------------------------------------------
    // [IKM] 2012-07-02 : on padding.
    //
    // Basically we may need to add 1-3 bytes after writing any data which size
    // value is not a multiple of 4 (x86 architecture word size)
    //
    // Example:
    //   write short (2 bytes)
    //   write short (2 bytes)
    //   write short (2 bytes)
    //   write int32 (4 bytes) <--- we need 2 more bytes BEFORE this one to make data properly aligned
    // or
    //   write array of 31 chars
    //   write int32 (4 bytes) <--- we need 1 more byte BEFORE this one
    //
    // Why do we need all this? For backwards compatibility.
    // Originally AGS saves most structs plainly as a single data piece, like
    //
    //  fwrite(&play,sizeof(GameState),1,ooo);
    //
    // which aligns data on its own. Here we have to do that manually.
    //
    //-------------------------------------------------
    char padding[3] = {0,0,0}; // to align data

    out->WriteInt32(score);
    out->WriteInt32(usedmode);
    out->WriteInt32(disabled_user_interface);
    out->WriteInt32(gscript_timer);
    out->WriteInt32(debug_mode);
    out->WriteArrayOfInt32(globalvars, MAXGLOBALVARS);
    out->WriteInt32(messagetime);
    out->WriteInt32(usedinv);
    out->WriteInt32(inv_top);
    out->WriteInt32(inv_numdisp);
    out->WriteInt32(obsolete_inv_numorder);
    out->WriteInt32(inv_numinline);
    out->WriteInt32(text_speed);
    out->WriteInt32(sierra_inv_color);
    out->WriteInt32(talkanim_speed);
    out->WriteInt32(inv_item_wid);
    out->WriteInt32(inv_item_hit);
    out->WriteInt32(speech_text_shadow);
    out->WriteInt32(swap_portrait_side);
    out->WriteInt32(speech_textwindow_gui);
    out->WriteInt32(follow_change_room_timer);
    out->WriteInt32(totalscore);
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
    out->WriteInt32(mp3_loop_before_end);
    out->WriteInt32(speech_music_drop);
    out->WriteInt32(in_cutscene);
    out->WriteInt32(fast_forward);
    out->WriteInt32(room_width);
    out->WriteInt32(room_height);
    out->WriteInt32(game_speed_modifier);
    out->WriteInt32(score_sound);
    out->WriteInt32(takeover_data);
    out->WriteInt32(replay_hotkey);
    out->WriteInt32(dialog_options_x);
    out->WriteInt32(dialog_options_y);
    out->WriteInt32(narrator_speech);
    out->WriteInt32(ambient_sounds_persist);
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
    out->WriteArrayOfInt32(reserved, 10);
    // ** up to here is referenced in the script "game." object
    out->WriteInt32( recording);   // user is recording their moves
    out->WriteInt32( playback);    // playing back recording
    out->WriteInt16(gamestep);    // step number for matching recordings
    out->Write(&padding, 2); // <-- padding
    out->WriteInt32(randseed);    // random seed
    out->WriteInt32( player_on_region);    // player's current region
    out->WriteInt32( screen_is_faded_out); // the screen is currently black
    out->WriteInt32( check_interaction_only);
    out->WriteInt32( bg_frame);
    out->WriteInt32( bg_anim_delay);  // for animating backgrounds
    out->WriteInt32( music_vol_was);  // before the volume drop
    out->WriteInt16(wait_counter);
    out->WriteInt16(mboundx1);
    out->WriteInt16(mboundx2);
    out->WriteInt16(mboundy1);
    out->WriteInt16(mboundy2);
    out->Write(&padding, 2); // <-- padding
    out->WriteInt32( fade_effect);
    out->WriteInt32( bg_frame_locked);
    out->WriteArrayOfInt32(globalscriptvars, MAXGSVALUES);
    out->WriteInt32( cur_music_number);
    out->WriteInt32( music_repeat);
    out->WriteInt32( music_master_volume);
    out->WriteInt32( digital_master_volume);
    out->Write(walkable_areas_on, MAX_WALK_AREAS+1);
    out->WriteInt16( screen_flipped);
    out->WriteInt16( offsets_locked);
    out->WriteInt32( entered_at_x);
    out->WriteInt32( entered_at_y);
    out->WriteInt32( entered_edge);
    out->WriteInt32( want_speech);
    out->WriteInt32( cant_skip_speech);
    out->WriteArrayOfInt32(script_timers, MAX_TIMERS);
    out->WriteInt32( sound_volume);
    out->WriteInt32( speech_volume);
    out->WriteInt32( normal_font);
    out->WriteInt32( speech_font);
    out->WriteInt8( key_skip_wait);
    out->Write(&padding, 3); // <-- padding
    out->WriteInt32( swap_portrait_lastchar);
    out->WriteInt32( seperate_music_lib);
    out->WriteInt32( in_conversation);
    out->WriteInt32( screen_tint);
    out->WriteInt32( num_parsed_words);
    out->WriteArrayOfInt16( parsed_words, MAX_PARSED_WORDS);
    out->Write(&padding, 2); // <-- padding
    out->Write( bad_parsed_word, 100);
    out->WriteInt32( raw_color);
    out->WriteArrayOfInt32( raw_modified, MAX_BSCENE);
    out->WriteArrayOfInt16( filenumbers, MAXSAVEGAMES);
    out->WriteInt32( room_changes);
    out->WriteInt32( mouse_cursor_hidden);
    out->WriteInt32( silent_midi);
    out->WriteInt32( silent_midi_channel);
    out->WriteInt32( current_music_repeating);
    out->WriteInt32( shakesc_delay);
    out->WriteInt32( shakesc_amount);
    out->WriteInt32( shakesc_length);
    out->WriteInt32( rtint_red);
    out->WriteInt32( rtint_green);
    out->WriteInt32( rtint_blue);
    out->WriteInt32( rtint_level);
    out->WriteInt32( rtint_light);
    out->WriteInt32( end_cutscene_music);
    out->WriteInt32( skip_until_char_stops);
    out->WriteInt32( get_loc_name_last_time);
    out->WriteInt32( get_loc_name_save_cursor);
    out->WriteInt32( restore_cursor_mode_to);
    out->WriteInt32( restore_cursor_image_to);
    out->WriteInt16( music_queue_size);
    out->Write(&padding, 2); // <-- padding
    out->WriteArrayOfInt16( music_queue, MAX_QUEUED_MUSIC);
    out->WriteInt16( new_music_queue_size);
    out->WriteInt16( crossfading_out_channel);
    out->WriteInt16( crossfade_step);
    out->WriteInt16( crossfade_out_volume_per_step);
    out->WriteInt16( crossfade_initial_volume_out);
    out->WriteInt16( crossfading_in_channel);
    out->WriteInt16( crossfade_in_volume_per_step);
    out->WriteInt16( crossfade_final_volume_in);

    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        new_music_queue[i].WriteToFile(out);
    }

    out->Write(takeover_from, 50);
    out->Write(playmp3file_name, PLAYMP3FILE_MAX_FILENAME_LEN);
    out->Write(globalstrings, MAXGLOBALSTRINGS * MAX_MAXSTRLEN);
    out->Write(lastParserEntry, MAX_MAXSTRLEN);
    out->Write(game_name, 100);
    out->WriteInt32( ground_level_areas_disabled);
    out->WriteInt32( next_screen_transition);
    out->WriteInt32( gamma_adjustment);
    out->WriteInt16(temporarily_turned_off_character);
    out->WriteInt16(inv_backwards_compatibility);
    out->WriteInt32((int32)gui_draw_order);
    out->WriteInt32((int32)do_once_tokens);
    out->WriteInt32( num_do_once_tokens);
    out->WriteInt32( text_min_display_time_ms);
    out->WriteInt32( ignore_user_input_after_text_timeout_ms);
    out->WriteInt32( ignore_user_input_until_time);
    out->WriteArrayOfInt32(default_audio_type_volumes, MAX_AUDIO_TYPES);
}

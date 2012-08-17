
#include "util/wgt2allg.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "util/string_utils.h"

//
// [IKM] What must be kept in mind: in previous versions of AGS
// this struct was read and written as-is (fread/fwrite-ing object)
// It did not have virtual functions (did not have functions at all
// actually), so I believe there's no need to skip 4 bytes for
// vtable pointer.
// On other hand we should read/write even pointers to arrays
// (or at least emulate that), although that could make no sense.
//

extern GameSetupStruct game;

void GameState::ReadFromFile(FILE *f)
{
    char padding[3] = {0,0,0}; // to align data

    score = getw(f);
    usedmode = getw(f);
    disabled_user_interface = getw(f);
    gscript_timer = getw(f);
    debug_mode = getw(f);
    fread(globalvars, sizeof(int32), MAXGLOBALVARS, f);
    messagetime = getw(f);
    usedinv = getw(f);
    inv_top = getw(f);
    inv_numdisp = getw(f);
    obsolete_inv_numorder = getw(f);
    inv_numinline = getw(f);
    text_speed = getw(f);
    sierra_inv_color = getw(f);
    talkanim_speed = getw(f);
    inv_item_wid = getw(f);
    inv_item_hit = getw(f);
    speech_text_shadow = getw(f);
    swap_portrait_side = getw(f);
    speech_textwindow_gui = getw(f);
    follow_change_room_timer = getw(f);
    totalscore = getw(f);
    skip_display = getw(f);
    no_multiloop_repeat = getw(f);
    roomscript_finished = getw(f);
    used_inv_on = getw(f);
    no_textbg_when_voice = getw(f);
    max_dialogoption_width = getw(f);
    no_hicolor_fadein = getw(f);
    bgspeech_game_speed = getw(f);
    bgspeech_stay_on_display = getw(f);
    unfactor_speech_from_textlength = getw(f);
    mp3_loop_before_end = getw(f);
    speech_music_drop = getw(f);
    in_cutscene = getw(f);
    fast_forward = getw(f);
    room_width = getw(f);
    room_height = getw(f);
    game_speed_modifier = getw(f);
    score_sound = getw(f);
    takeover_data = getw(f);
    replay_hotkey = getw(f);
    dialog_options_x = getw(f);
    dialog_options_y = getw(f);
    narrator_speech = getw(f);
    ambient_sounds_persist = getw(f);
    lipsync_speed = getw(f);
    close_mouth_speech_time = getw(f);
    disable_antialiasing = getw(f);
    text_speed_modifier = getw(f);
    text_align = getw(f);
    speech_bubble_width = getw(f);
    min_dialogoption_width = getw(f);
    disable_dialog_parser = getw(f);
    anim_background_speed = getw(f);  // the setting for this room
    top_bar_backcolor= getw(f);
    top_bar_textcolor = getw(f);
    top_bar_bordercolor = getw(f);
    top_bar_borderwidth = getw(f);
    top_bar_ypos = getw(f);
    screenshot_width = getw(f);
    screenshot_height = getw(f);
    top_bar_font = getw(f);
    speech_text_align = getw(f);
    auto_use_walkto_points = getw(f);
    inventory_greys_out = getw(f);
    skip_speech_specific_key = getw(f);
    abort_key = getw(f);
    fade_to_red = getw(f);
    fade_to_green = getw(f);
    fade_to_blue = getw(f);
    show_single_dialog_option = getw(f);
    keep_screen_during_instant_transition = getw(f);
    read_dialog_option_colour = getw(f);
    stop_dialog_at_end = getw(f);
    fread(reserved, sizeof(int32), 10, f);
    // ** up to here is referenced in the script "game." object
    recording = getw(f);   // user is recording their moves
    playback = getw(f);    // playing back recording
    gamestep = getshort(f);    // step number for matching recordings
    fread(&padding, sizeof(char), 2, f); // <-- padding
    randseed = getw(f);    // random seed
    player_on_region = getw(f);    // player's current region
    screen_is_faded_out = getw(f); // the screen is currently black
    check_interaction_only = getw(f);
    bg_frame = getw(f);
    bg_anim_delay = getw(f);  // for animating backgrounds
    music_vol_was = getw(f);  // before the volume drop
    wait_counter = getshort(f);
    mboundx1 = getshort(f);
    mboundx2 = getshort(f);
    mboundy1 = getshort(f);
    mboundy2 = getshort(f);
    fread(&padding, sizeof(char), 2, f); // <-- padding
    fade_effect = getw(f);
    bg_frame_locked = getw(f);
    fread(globalscriptvars, sizeof(int32), MAXGSVALUES, f);
    cur_music_number = getw(f);
    music_repeat = getw(f);
    music_master_volume = getw(f);
    digital_master_volume = getw(f);
    fread(walkable_areas_on, sizeof(char), MAX_WALK_AREAS+1, f);
    screen_flipped = getshort(f);
    offsets_locked = getshort(f);
    entered_at_x = getw(f);
    entered_at_y = getw(f);
    entered_edge = getw(f);
    want_speech = getw(f);
    cant_skip_speech = getw(f);
    fread(script_timers, sizeof(int32), MAX_TIMERS, f);
    sound_volume = getw(f);
    speech_volume = getw(f);
    normal_font = getw(f);
    speech_font = getw(f);
    key_skip_wait = fgetc(f);
    fread(&padding, sizeof(char), 3, f); // <-- padding
    swap_portrait_lastchar = getw(f);
    seperate_music_lib = getw(f);
    in_conversation = getw(f);
    screen_tint = getw(f);
    num_parsed_words = getw(f);
    fread( parsed_words, sizeof(short), MAX_PARSED_WORDS, f);
    fread(&padding, sizeof(char), 2, f); // <-- padding
    fread( bad_parsed_word, sizeof(char), 100, f);
    raw_color = getw(f);
    fread( raw_modified, sizeof(int32), MAX_BSCENE, f);
    fread( filenumbers, sizeof(short), MAXSAVEGAMES, f);
    room_changes = getw(f);
    mouse_cursor_hidden = getw(f);
    silent_midi = getw(f);
    silent_midi_channel = getw(f);
    current_music_repeating = getw(f);
    shakesc_delay = getw(f);
    shakesc_amount = getw(f);
    shakesc_length = getw(f);
    rtint_red = getw(f);
    rtint_green = getw(f);
    rtint_blue = getw(f);
    rtint_level = getw(f);
    rtint_light = getw(f);
    end_cutscene_music = getw(f);
    skip_until_char_stops = getw(f);
    get_loc_name_last_time = getw(f);
    get_loc_name_save_cursor = getw(f);
    restore_cursor_mode_to = getw(f);
    restore_cursor_image_to = getw(f);
    music_queue_size = getshort(f);
    fread(&padding, sizeof(char), 2, f); // <-- padding
    fread( music_queue, sizeof(short), MAX_QUEUED_MUSIC, f);
    new_music_queue_size = getshort(f);
    crossfading_out_channel = getshort(f);
    crossfade_step = getshort(f);
    crossfade_out_volume_per_step = getshort(f);
    crossfade_initial_volume_out = getshort(f);
    crossfading_in_channel = getshort(f);
    crossfade_in_volume_per_step = getshort(f);
    crossfade_final_volume_in = getshort(f);

    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        new_music_queue->ReadFromFile(f);
    }

    fread(takeover_from, sizeof(char), 50, f);
    fread(playmp3file_name, sizeof(char), PLAYMP3FILE_MAX_FILENAME_LEN, f);
    fread(globalstrings, sizeof(char), MAXGLOBALSTRINGS * MAX_MAXSTRLEN, f);
    fread(lastParserEntry, sizeof(char), MAX_MAXSTRLEN, f);
    fread(game_name, sizeof(char), 100, f);
    ground_level_areas_disabled = getw(f);
    next_screen_transition = getw(f);
    gamma_adjustment = getw(f);
    temporarily_turned_off_character = getshort(f);
    inv_backwards_compatibility = getshort(f);
    gui_draw_order = (int*)getw(f);
    do_once_tokens = (char**)getw(f);
    num_do_once_tokens = getw(f);
    text_min_display_time_ms = getw(f);
    ignore_user_input_after_text_timeout_ms = getw(f);
    ignore_user_input_until_time = getw(f);
    fread(default_audio_type_volumes, sizeof(int32), MAX_AUDIO_TYPES, f);
}

void GameState::WriteToFile(FILE *f)
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

    putw(score, f);
    putw(usedmode, f);
    putw(disabled_user_interface, f);
    putw(gscript_timer, f);
    putw(debug_mode, f);
    fwrite(globalvars, sizeof(int32), MAXGLOBALVARS, f);
    putw(messagetime, f);
    putw(usedinv, f);
    putw(inv_top, f);
    putw(inv_numdisp, f);
    putw(obsolete_inv_numorder, f);
    putw(inv_numinline, f);
    putw(text_speed, f);
    putw(sierra_inv_color, f);
    putw(talkanim_speed, f);
    putw(inv_item_wid, f);
    putw(inv_item_hit, f);
    putw(speech_text_shadow, f);
    putw(swap_portrait_side, f);
    putw(speech_textwindow_gui, f);
    putw(follow_change_room_timer, f);
    putw(totalscore, f);
    putw(skip_display, f);
    putw(no_multiloop_repeat, f);
    putw(roomscript_finished, f);
    putw(used_inv_on, f);
    putw(no_textbg_when_voice, f);
    putw(max_dialogoption_width, f);
    putw(no_hicolor_fadein, f);
    putw(bgspeech_game_speed, f);
    putw(bgspeech_stay_on_display, f);
    putw(unfactor_speech_from_textlength, f);
    putw(mp3_loop_before_end, f);
    putw(speech_music_drop, f);
    putw(in_cutscene, f);
    putw(fast_forward, f);
    putw(room_width, f);
    putw(room_height, f);
    putw(game_speed_modifier, f);
    putw(score_sound, f);
    putw(takeover_data, f);
    putw(replay_hotkey, f);
    putw(dialog_options_x, f);
    putw(dialog_options_y, f);
    putw(narrator_speech, f);
    putw(ambient_sounds_persist, f);
    putw(lipsync_speed, f);
    putw(close_mouth_speech_time, f);
    putw(disable_antialiasing, f);
    putw(text_speed_modifier, f);
    putw(text_align, f);
    putw(speech_bubble_width, f);
    putw(min_dialogoption_width, f);
    putw(disable_dialog_parser, f);
    putw(anim_background_speed, f);  // the setting for this room
    putw(top_bar_backcolor, f);
    putw(top_bar_textcolor, f);
    putw(top_bar_bordercolor, f);
    putw(top_bar_borderwidth, f);
    putw(top_bar_ypos, f);
    putw(screenshot_width, f);
    putw(screenshot_height, f);
    putw(top_bar_font, f);
    putw(speech_text_align, f);
    putw(auto_use_walkto_points, f);
    putw(inventory_greys_out, f);
    putw(skip_speech_specific_key, f);
    putw(abort_key, f);
    putw(fade_to_red, f);
    putw(fade_to_green, f);
    putw(fade_to_blue, f);
    putw(show_single_dialog_option, f);
    putw(keep_screen_during_instant_transition, f);
    putw(read_dialog_option_colour, f);
    putw(stop_dialog_at_end, f);
    fwrite(reserved, sizeof(int32), 10, f);
    // ** up to here is referenced in the script "game." object
    putw( recording, f);   // user is recording their moves
    putw( playback, f);    // playing back recording
    putshort(gamestep, f);    // step number for matching recordings
    fwrite(&padding, sizeof(char), 2, f); // <-- padding
    putw(randseed, f);    // random seed
    putw( player_on_region, f);    // player's current region
    putw( screen_is_faded_out, f); // the screen is currently black
    putw( check_interaction_only, f);
    putw( bg_frame, f);
    putw( bg_anim_delay, f);  // for animating backgrounds
    putw( music_vol_was, f);  // before the volume drop
    putshort(wait_counter, f);
    putshort(mboundx1, f);
    putshort(mboundx2, f);
    putshort(mboundy1, f);
    putshort(mboundy2, f);
    fwrite(&padding, sizeof(char), 2, f); // <-- padding
    putw( fade_effect, f);
    putw( bg_frame_locked, f);
    fwrite(globalscriptvars, sizeof(int32), MAXGSVALUES, f);
    putw( cur_music_number, f);
    putw( music_repeat, f);
    putw( music_master_volume, f);
    putw( digital_master_volume, f);
    fwrite(walkable_areas_on, sizeof(char), MAX_WALK_AREAS+1, f);
    putshort( screen_flipped, f);
    putshort( offsets_locked, f);
    putw( entered_at_x, f);
    putw( entered_at_y, f);
    putw( entered_edge, f);
    putw( want_speech, f);
    putw( cant_skip_speech, f);
    fwrite(script_timers, sizeof(int32), MAX_TIMERS, f);
    putw( sound_volume, f);
    putw( speech_volume, f);
    putw( normal_font, f);
    putw( speech_font, f);
    fputc( key_skip_wait, f);
    fwrite(&padding, sizeof(char), 3, f); // <-- padding
    putw( swap_portrait_lastchar, f);
    putw( seperate_music_lib, f);
    putw( in_conversation, f);
    putw( screen_tint, f);
    putw( num_parsed_words, f);
    fwrite( parsed_words, sizeof(short), MAX_PARSED_WORDS, f);
    fwrite(&padding, sizeof(char), 2, f); // <-- padding
    fwrite( bad_parsed_word, sizeof(char), 100, f);
    putw( raw_color, f);
    fwrite( raw_modified, sizeof(int32), MAX_BSCENE, f);
    fwrite( filenumbers, sizeof(short), MAXSAVEGAMES, f);
    putw( room_changes, f);
    putw( mouse_cursor_hidden, f);
    putw( silent_midi, f);
    putw( silent_midi_channel, f);
    putw( current_music_repeating, f);
    putw( shakesc_delay, f);
    putw( shakesc_amount, f);
    putw( shakesc_length, f);
    putw( rtint_red, f);
    putw( rtint_green, f);
    putw( rtint_blue, f);
    putw( rtint_level, f);
    putw( rtint_light, f);
    putw( end_cutscene_music, f);
    putw( skip_until_char_stops, f);
    putw( get_loc_name_last_time, f);
    putw( get_loc_name_save_cursor, f);
    putw( restore_cursor_mode_to, f);
    putw( restore_cursor_image_to, f);
    putshort( music_queue_size, f);
    fwrite(&padding, sizeof(char), 2, f); // <-- padding
    fwrite( music_queue, sizeof(short), MAX_QUEUED_MUSIC, f);
    putshort( new_music_queue_size, f);
    putshort( crossfading_out_channel, f);
    putshort( crossfade_step, f);
    putshort( crossfade_out_volume_per_step, f);
    putshort( crossfade_initial_volume_out, f);
    putshort( crossfading_in_channel, f);
    putshort( crossfade_in_volume_per_step, f);
    putshort( crossfade_final_volume_in, f);

    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        new_music_queue->WriteToFile(f);
    }

    fwrite(takeover_from, sizeof(char), 50, f);
    fwrite(playmp3file_name, sizeof(char), PLAYMP3FILE_MAX_FILENAME_LEN, f);
    fwrite(globalstrings, sizeof(char), MAXGLOBALSTRINGS * MAX_MAXSTRLEN, f);
    fwrite(lastParserEntry, sizeof(char), MAX_MAXSTRLEN, f);
    fwrite(game_name, sizeof(char), 100, f);
    putw( ground_level_areas_disabled, f);
    putw( next_screen_transition, f);
    putw( gamma_adjustment, f);
    putshort(temporarily_turned_off_character, f);
    putshort(inv_backwards_compatibility, f);
    putw((int32)gui_draw_order, f);
    putw((int32)do_once_tokens, f);
    putw( num_do_once_tokens, f);
    putw( text_min_display_time_ms, f);
    putw( ignore_user_input_after_text_timeout_ms, f);
    putw( ignore_user_input_until_time, f);
    fwrite(default_audio_type_volumes, sizeof(int32), MAX_AUDIO_TYPES, f);
}

#include "ac/statobj/staticgame.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "script/cc_error.h"

extern GameState play;

StaticGame GameStaticManager;

int32_t StaticGame::ReadInt32(const char *address, intptr_t offset)
{
    int index = offset / sizeof(int32_t);
    if (index >= 5 && index < 5 + MAXGLOBALVARS)
        return play.globalvars[index - 5];

    switch (index)
    {
    case 0: return play.score;
    case 1: return play.usedmode;
    case 2: return play.disabled_user_interface;
    case 3: return play.gscript_timer;
    case 4: return play.debug_mode;
        // 5 -> 54: play.globalvars
    case 55: return play.messagetime;
    case 56: return play.usedinv;
    case 57: return play.inv_top;
    case 58: return play.inv_numdisp;
    case 59: return play.obsolete_inv_numorder;
    case 60: return play.inv_numinline;
    case 61: return play.text_speed;
    case 62: return play.sierra_inv_color;
    case 63: return play.talkanim_speed;
    case 64: return play.inv_item_wid;
    case 65: return play.inv_item_hit;
    case 66: return play.speech_text_shadow;
    case 67: return play.swap_portrait_side;
    case 68: return play.speech_textwindow_gui;
    case 69: return play.follow_change_room_timer;
    case 70: return play.totalscore;
    case 71: return play.skip_display;
    case 72: return play.no_multiloop_repeat;
    case 73: return play.roomscript_finished;
    case 74: return play.used_inv_on;
    case 75: return play.no_textbg_when_voice;
    case 76: return play.max_dialogoption_width;
    case 77: return play.no_hicolor_fadein;
    case 78: return play.bgspeech_game_speed;
    case 79: return play.bgspeech_stay_on_display;
    case 80: return play.unfactor_speech_from_textlength;
    case 81: return play.mp3_loop_before_end;
    case 82: return play.speech_music_drop;
    case 83: return play.in_cutscene;
    case 84: return play.fast_forward;
    case 85: return play.room_width;
    case 86: return play.room_height;
    case 87: return play.game_speed_modifier;
    case 88: return play.score_sound;
    case 89: return play.takeover_data;
    case 90: return play.replay_hotkey;
    case 91: return play.dialog_options_x;
    case 92: return play.dialog_options_y;
    case 93: return play.narrator_speech;
    case 94: return play.ambient_sounds_persist;
    case 95: return play.lipsync_speed;
    case 96: return play.close_mouth_speech_time;
    case 97: return play.disable_antialiasing;
    case 98: return play.text_speed_modifier;
    case 99: return play.text_align;
    case 100: return play.speech_bubble_width;
    case 101: return play.min_dialogoption_width;
    case 102: return play.disable_dialog_parser;
    case 103: return play.anim_background_speed;
    case 104: return play.top_bar_backcolor;
    case 105: return play.top_bar_textcolor;
    case 106: return play.top_bar_bordercolor;
    case 107: return play.top_bar_borderwidth;
    case 108: return play.top_bar_ypos;
    case 109: return play.screenshot_width;
    case 110: return play.screenshot_height;
    case 111: return play.top_bar_font;
    case 112: return play.speech_text_align;
    case 113: return play.auto_use_walkto_points;
    case 114: return play.inventory_greys_out;
    case 115: return play.skip_speech_specific_key;
    case 116: return play.abort_key;
    case 117: return play.fade_to_red;
    case 118: return play.fade_to_green;
    case 119: return play.fade_to_blue;
    case 120: return play.show_single_dialog_option;
    case 121: return play.keep_screen_during_instant_transition;
    case 122: return play.read_dialog_option_colour;
    case 123: return play.stop_dialog_at_end;
    case 124: return play.speech_portrait_placement;
    case 125: return play.speech_portrait_x;
    case 126: return play.speech_portrait_y;
    case 127: return play.speech_display_post_time_ms;
    case 128: return play.dialog_options_highlight_color;
    }
    cc_error("StaticGame: unsupported variable offset %d", offset);
    return 0;
}

void StaticGame::WriteInt32(const char *address, intptr_t offset, int32_t val)
{
    int index = offset / sizeof(int32_t);
    if (index >= 5 && index < 5 + MAXGLOBALVARS)
    {
        play.globalvars[index - 5] = val;
        return;
    }

    switch (index)
    {
    case 0:  play.score = val; break;
    case 1:  play.usedmode = val; break;
    case 2:  play.disabled_user_interface = val; break;
    case 3:  play.gscript_timer = val; break;
    case 4:  set_debug_mode(val != 0); break; // play.debug_mode
        // 5 -> 54: play.globalvars
    case 55:  play.messagetime = val; break;
    case 56:  play.usedinv = val; break;
    case 57:  play.inv_top = val; break;
    case 58:  play.inv_numdisp = val; break;
    case 59:  play.obsolete_inv_numorder = val; break;
    case 60:  play.inv_numinline = val; break;
    case 61:  play.text_speed = val; break;
    case 62:  play.sierra_inv_color = val; break;
    case 63:  play.talkanim_speed = val; break;
    case 64:  play.inv_item_wid = val; break;
    case 65:  play.inv_item_hit = val; break;
    case 66:  play.speech_text_shadow = val; break;
    case 67:  play.swap_portrait_side = val; break;
    case 68:  play.speech_textwindow_gui = val; break;
    case 69:  play.follow_change_room_timer = val; break;
    case 70:  play.totalscore = val; break;
    case 71:  play.skip_display = val; break;
    case 72:  play.no_multiloop_repeat = val; break;
    case 73:  play.roomscript_finished = val; break;
    case 74:  play.used_inv_on = val; break;
    case 75:  play.no_textbg_when_voice = val; break;
    case 76:  play.max_dialogoption_width = val; break;
    case 77:  play.no_hicolor_fadein = val; break;
    case 78:  play.bgspeech_game_speed = val; break;
    case 79:  play.bgspeech_stay_on_display = val; break;
    case 80:  play.unfactor_speech_from_textlength = val; break;
    case 81:  play.mp3_loop_before_end = val; break;
    case 82:  play.speech_music_drop = val; break;
    case 83:
    case 84:
    case 85:
    case 86:
        cc_error("StaticGame: attempt to write readonly variable at offset %d", offset);
        break;
    case 87:  play.game_speed_modifier = val; break;
    case 88:  play.score_sound = val; break;
    case 89:  play.takeover_data = val; break;
    case 90:  play.replay_hotkey = val; break;
    case 91:  play.dialog_options_x = val; break;
    case 92:  play.dialog_options_y = val; break;
    case 93:  play.narrator_speech = val; break;
    case 94:  play.ambient_sounds_persist = val; break;
    case 95:  play.lipsync_speed = val; break;
    case 96:  play.close_mouth_speech_time = val; break;
    case 97:  play.disable_antialiasing = val; break;
    case 98:  play.text_speed_modifier = val; break;
    case 99:  play.text_align = val; break;
    case 100:  play.speech_bubble_width = val; break;
    case 101:  play.min_dialogoption_width = val; break;
    case 102:  play.disable_dialog_parser = val; break;
    case 103:  play.anim_background_speed = val; break;
    case 104:  play.top_bar_backcolor = val; break;
    case 105:  play.top_bar_textcolor = val; break;
    case 106:  play.top_bar_bordercolor = val; break;
    case 107:  play.top_bar_borderwidth = val; break;
    case 108:  play.top_bar_ypos = val; break;
    case 109:  play.screenshot_width = val; break;
    case 110:  play.screenshot_height = val; break;
    case 111:  play.top_bar_font = val; break;
    case 112:  play.speech_text_align = val; break;
    case 113:  play.auto_use_walkto_points = val; break;
    case 114:  play.inventory_greys_out = val; break;
    case 115:  play.skip_speech_specific_key = val; break;
    case 116:  play.abort_key = val; break;
    case 117:
    case 118:
    case 119:
        cc_error("StaticGame: attempt to write readonly variable at offset %d", offset);
        break;
    case 120:  play.show_single_dialog_option = val; break;
    case 121:  play.keep_screen_during_instant_transition = val; break;
    case 122:  play.read_dialog_option_colour = val; break;
    case 123:  play.stop_dialog_at_end = val; break;
    case 124:  play.speech_portrait_placement = val; break;
    case 125:  play.speech_portrait_x = val; break;
    case 126:  play.speech_portrait_y = val; break;
    case 127:  play.speech_display_post_time_ms = val; break;
    case 128:  play.dialog_options_highlight_color = val; break;
    default:
        cc_error("StaticGame: unsupported variable offset %d", offset);
    }
}

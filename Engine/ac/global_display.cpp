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
#include <cstdio>
#include <stdarg.h>
#include "ac/common.h"
#include "ac/character.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_screen.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/speech.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "game/roomstruct.h"
#include "main/game_run.h"

using namespace AGS::Common;

extern RoomStruct thisroom;
extern GameSetupStruct game;

void DisplayAtYImpl(int ypos, const char *texx, const TopBarSettings *topbar, bool as_speech);

void Display(const char*texx, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    vsnprintf(displbuf, sizeof(displbuf), get_translation(texx), ap);
    va_end(ap);
    DisplayAtY (-1, displbuf);
}

void DisplaySimple(const char *text)
{
    DisplayAtY (-1, text);
}

void DisplayMB(const char *text)
{
    DisplayAtYImpl(-1, text, nullptr, false);
}

void DisplayTopBar(int ypos, int ttexcol, int backcol, const char *title, const char *text)
{
    // FIXME: refactor source_text_length and get rid of this ugly hack!
    const int real_text_sourcelen = source_text_length;
    source_text_length = real_text_sourcelen;

    if (ypos > 0)
        play.top_bar_ypos = ypos;
    if (ttexcol > 0)
        play.top_bar_textcolor = ttexcol;
    if (backcol > 0)
        play.top_bar_backcolor = backcol;

    int font;
    // they want to customize the font
    if (play.top_bar_font >= 0)
        font = play.top_bar_font;
    else
        font = FONT_NORMAL;
    int height = get_font_height_outlined(font)
        + data_to_game_coord(play.top_bar_borderwidth) * 2 + get_fixed_pixel_size(1);

    const TopBarSettings topbar(get_translation(title), FONT_NORMAL, height);

    // DisplaySpeech normally sets this up, but since we're not going via it...
    if (play.speech_skip_style & SKIP_AUTOTIMER)
        play.messagetime = GetTextDisplayTime(text);

    DisplayAtYImpl(play.top_bar_ypos, text, &topbar, game.options[OPT_ALWAYSSPCH] != 0);
}

// Display a room/global message in the bar
void DisplayMessageBar(int ypos, int ttexcol, int backcol, const char *title, int msgnum) {
    char msgbufr[3001];
    get_message_text(msgnum, msgbufr);
    DisplayTopBar(ypos, ttexcol, backcol, title, msgbufr);
}

void DisplayMessageImpl(int msnum, int aschar, int ypos) {
    char msgbufr[3001];
    if (msnum>=500) {
        get_message_text (msnum, msgbufr);
        if (aschar > 0)
            DisplaySpeech(msgbufr, aschar);
        else
            DisplayAtY(ypos, msgbufr);
        return;
    }

    if (aschar > 0) {
        quit("!DisplayMessage: data column specified a character for local\n"
            "message; use the message editor to select the character for room\n"
            "messages.\n");
    }

    int repeatloop=1;
    while (repeatloop) {
        get_message_text (msnum, msgbufr);

        if (thisroom.MessageInfos[msnum].DisplayAs > 0) {
            DisplaySpeech(msgbufr, thisroom.MessageInfos[msnum].DisplayAs - 1);
        }
        else {
            // time out automatically if they have set that
            const SkipSpeechStyle old_skip_display = play.skip_display;
            if (thisroom.MessageInfos[msnum].Flags & MSG_TIMELIMIT)
                play.skip_display = play.skip_timed_display;

            DisplayAtY(ypos, msgbufr);

            play.skip_display = old_skip_display;
        }
        if (thisroom.MessageInfos[msnum].Flags & MSG_DISPLAYNEXT) {
            msnum++;
            repeatloop=1;
        }
        else
            repeatloop=0;
    }
}

void DisplayMessageAtY(int msnum, int ypos) {
    DisplayMessageImpl(msnum, -1, ypos);
}

void DisplayMessage(int msnum) {
    DisplayMessageAtY(msnum, -1);
}

void DisplayAt(int xxp,int yyp,int widd, const char* text) {
    if (play.screen_is_faded_out > 0)
        debug_script_warn("Warning: blocking Display call during fade-out.");

    data_to_game_coords(&xxp, &yyp);
    widd = data_to_game_coord(widd);

    if (widd<1) widd=play.GetUIViewport().GetWidth()/2;
    if (xxp<0) xxp=play.GetUIViewport().GetWidth()/2-widd/2;
    display_at(xxp, yyp, widd, text, nullptr);
}

void DisplayAtYImpl(int ypos, const char *texx, const TopBarSettings *topbar, bool as_speech) {
    const Rect &ui_view = play.GetUIViewport();
    if ((ypos < -1) || (ypos >= ui_view.GetHeight()))
        quitprintf("!DisplayAtY: invalid Y co-ordinate supplied (used: %d; valid: 0..%d)", ypos, ui_view.GetHeight());
    if (play.screen_is_faded_out > 0)
        debug_script_warn("Warning: blocking Display call during fade-out.");

    // Display("") ... a bit of a stupid thing to do, so ignore it
    if (texx[0] == 0)
        return;

    if (ypos > 0)
        ypos = data_to_game_coord(ypos);

    if (as_speech)
        DisplaySpeechAt(-1, (ypos > 0) ? game_to_data_coord(ypos) : ypos, -1, game.playercharacter, texx);
    else { 
        // Normal "Display" in text box

        if (is_screen_dirty()) {
            // erase any previous DisplaySpeech
            play.disabled_user_interface ++;
            UpdateGameOnce();
            play.disabled_user_interface --;
        }

        display_at(-1, ypos, ui_view.GetWidth() / 2 + ui_view.GetWidth() / 4, get_translation(texx), topbar);
    }
}

void DisplayAtY(int ypos, const char *texx) {
    DisplayAtYImpl(ypos, texx, nullptr, game.options[OPT_ALWAYSSPCH] != 0);
}

void SetSpeechStyle (int newstyle) {
    if ((newstyle < 0) || (newstyle > 3))
        quit("!SetSpeechStyle: must use a SPEECH_* constant as parameter");
    game.options[OPT_SPEECHTYPE] = newstyle;
}

void SetSkipSpeech (SkipSpeechStyle newval) {
    if ((newval < kSkipSpeechFirst) || (newval > kSkipSpeechLast))
        quit("!SetSkipSpeech: invalid skip mode specified");

    debug_script_log("SkipSpeech style set to %d", newval);
    if (usetup.Access.SpeechSkipStyle == kSkipSpeechNone)
        play.speech_skip_style = user_to_internal_skip_speech((SkipSpeechStyle)newval);
}

SkipSpeechStyle GetSkipSpeech()
{
    return internal_skip_speech_to_user(play.speech_skip_style);
}

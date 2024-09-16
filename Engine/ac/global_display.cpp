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
        + play.top_bar_borderwidth * 2 + 1;

    const TopBarSettings topbar(get_translation(title), FONT_NORMAL, height);

    // DisplaySpeech normally sets this up, but since we're not going via it...
    if (play.speech_skip_style & SKIP_AUTOTIMER)
        play.messagetime = GetTextDisplayTime(text);

    DisplayAtYImpl(play.top_bar_ypos, text, &topbar, game.options[OPT_ALWAYSSPCH] != 0);
}

void DisplayAt(int xxp,int yyp,int widd, const char* text) {
    if (play.screen_is_faded_out > 0)
        debug_script_warn("Warning: blocking Display call during fade-out.");

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

    if (as_speech)
        DisplaySpeechAt(-1, ypos, -1, game.playercharacter, texx); // CLNUP if ypos <0  it used ypos without scaling, weird
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

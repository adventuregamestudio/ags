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
#include <cstdio>
#include <stdarg.h>
#include "ac/common.h"
#include "ac/character.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_screen.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/speech.h"
#include "ac/string.h"
#include "ac/topbarsettings.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "game/roomstruct.h"
#include "main/game_run.h"

using namespace AGS::Common;

extern TopBarSettings topBar;
extern GameState play;
extern RoomStruct thisroom;
extern int display_message_aschar;
extern GameSetupStruct game;

void DisplayAtYImpl(int ypos, const char *texx, bool as_speech);

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
    DisplayAtYImpl(-1, text, false);
}

void DisplayTopBar(int ypos, int ttexcol, int backcol, const char *title, const char *text)
{
    // FIXME: refactor source_text_length and get rid of this ugly hack!
    const int real_text_sourcelen = source_text_length;
    snprintf(topBar.text, sizeof(topBar.text), "%s", get_translation(title));
    source_text_length = real_text_sourcelen;

    if (ypos > 0)
        play.top_bar_ypos = ypos;
    if (ttexcol > 0)
        play.top_bar_textcolor = ttexcol;
    if (backcol > 0)
        play.top_bar_backcolor = backcol;

    topBar.wantIt = 1;
    topBar.font = FONT_NORMAL;
    topBar.height = get_font_height_outlined(topBar.font);
    topBar.height += play.top_bar_borderwidth * 2 + 1;

    // they want to customize the font
    if (play.top_bar_font >= 0)
        topBar.font = play.top_bar_font;

    // DisplaySpeech normally sets this up, but since we're not going via it...
    if (play.cant_skip_speech & SKIP_AUTOTIMER)
        play.messagetime = GetTextDisplayTime(text);

    DisplayAtY(play.top_bar_ypos, text);
}

// Display a room/global message in the bar
void DisplayMessageBar(int ypos, int ttexcol, int backcol, const char *title, int msgnum) {
    char msgbufr[3001];
    get_message_text(msgnum, msgbufr);
    DisplayTopBar(ypos, ttexcol, backcol, title, msgbufr);
}

void DisplayMessageAtY(int msnum, int ypos) {
    char msgbufr[3001];
    if (msnum>=500) {
        get_message_text (msnum, msgbufr);
        if (display_message_aschar > 0)
            DisplaySpeech(msgbufr, display_message_aschar);
        else
            DisplayAtY(ypos, msgbufr);
        display_message_aschar=0;
        return;
    }
}

void DisplayMessage(int msnum) {
    DisplayMessageAtY (msnum, -1);
}

void DisplayAt(int xxp,int yyp,int widd, const char* text) {
    if (widd<1) widd=play.GetUIViewport().GetWidth()/2;
    if (xxp<0) xxp=play.GetUIViewport().GetWidth()/2-widd/2;
    _display_at(xxp, yyp, widd, text, DISPLAYTEXT_MESSAGEBOX, 0, 0, 0, false);
}

void DisplayAtYImpl(int ypos, const char *texx, bool as_speech) {
    const Rect &ui_view = play.GetUIViewport();
    if ((ypos < -1) || (ypos >= ui_view.GetHeight()))
        quitprintf("!DisplayAtY: invalid Y co-ordinate supplied (used: %d; valid: 0..%d)", ypos, ui_view.GetHeight());

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

        _display_at(-1, ypos, ui_view.GetWidth() / 2 + ui_view.GetWidth() / 4,
            get_translation(texx), DISPLAYTEXT_MESSAGEBOX, 0, 0, 0, false);
    }
}

void DisplayAtY(int ypos, const char *texx) {
    DisplayAtYImpl(ypos, texx, game.options[OPT_ALWAYSSPCH] != 0);
}

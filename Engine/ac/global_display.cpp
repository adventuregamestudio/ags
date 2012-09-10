
#include <stdarg.h>
#include "util/wgt2allg.h"
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
#include "ac/roomstruct.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/topbarsettings.h"
#include "debug/debug_log.h"
#include "main/game_run.h"

extern TopBarSettings topBar;
extern GameState play;
extern roomstruct thisroom;
extern int display_message_aschar;
extern int scrnwid,scrnhit;
extern GameSetupStruct game;
extern int screen_is_dirty;

void Display(char*texx, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf, get_translation(texx), ap);
    va_end(ap);
    DisplayAtY (-1, displbuf);
}

void DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char*texx, ...) {

    strcpy(topBar.text, get_translation(title));

    char displbuf[3001];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf, get_translation(texx), ap);
    va_end(ap);

    if (ypos > 0)
        play.top_bar_ypos = ypos;
    if (ttexcol > 0)
        play.top_bar_textcolor = ttexcol;
    if (backcol > 0)
        play.top_bar_backcolor = backcol;

    topBar.wantIt = 1;
    topBar.font = FONT_NORMAL;
    topBar.height = wgetfontheight(topBar.font);
    topBar.height += multiply_up_coordinate(play.top_bar_borderwidth) * 2 + get_fixed_pixel_size(1);

    // they want to customize the font
    if (play.top_bar_font >= 0)
        topBar.font = play.top_bar_font;

    // DisplaySpeech normally sets this up, but since we're not going via it...
    if (play.cant_skip_speech & SKIP_AUTOTIMER)
        play.messagetime = GetTextDisplayTime(texx);

    DisplayAtY(play.top_bar_ypos, displbuf);
}

// Display a room/global message in the bar
void DisplayMessageBar(int ypos, int ttexcol, int backcol, char *title, int msgnum) {
    char msgbufr[3001];
    get_message_text(msgnum, msgbufr);
    DisplayTopBar(ypos, ttexcol, backcol, title, "%s", msgbufr);
}

void DisplayMessageAtY(int msnum, int ypos) {
    char msgbufr[3001];
    if (msnum>=500) { //quit("global message requseted, nto yet supported");
        get_message_text (msnum, msgbufr);
        if (display_message_aschar > 0)
            DisplaySpeech(msgbufr, display_message_aschar);
        else
            DisplayAtY(ypos, msgbufr);
        display_message_aschar=0;
        return;
    }

    if (display_message_aschar > 0) {
        display_message_aschar=0;
        quit("!DisplayMessage: data column specified a character for local\n"
            "message; use the message editor to select the character for room\n"
            "messages.\n");
    }

    int repeatloop=1;
    while (repeatloop) {
        get_message_text (msnum, msgbufr);

        if (thisroom.msgi[msnum].displayas>0) {
            DisplaySpeech(msgbufr, thisroom.msgi[msnum].displayas - 1);
        }
        else {
            // time out automatically if they have set that
            int oldGameSkipDisp = play.skip_display;
            if (thisroom.msgi[msnum].flags & MSG_TIMELIMIT)
                play.skip_display = 0;

            DisplayAtY(ypos, msgbufr);

            play.skip_display = oldGameSkipDisp;
        }
        if (thisroom.msgi[msnum].flags & MSG_DISPLAYNEXT) {
            msnum++;
            repeatloop=1;
        }
        else
            repeatloop=0;
    }

}

void DisplayMessage(int msnum) {
    DisplayMessageAtY (msnum, -1);
}

void DisplayAt(int xxp,int yyp,int widd,char*texx, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf, get_translation(texx), ap);
    va_end(ap);

    multiply_up_coordinates(&xxp, &yyp);
    widd = multiply_up_coordinate(widd);

    if (widd<1) widd=scrnwid/2;
    if (xxp<0) xxp=scrnwid/2-widd/2;
    _display_at(xxp,yyp,widd,displbuf,1,0, 0, 0, false);
}

void DisplayAtY (int ypos, char *texx) {
    if ((ypos < -1) || (ypos >= GetMaxScreenHeight()))
        quitprintf("!DisplayAtY: invalid Y co-ordinate supplied (used: %d; valid: 0..%d)", ypos, GetMaxScreenHeight());

    // Display("") ... a bit of a stupid thing to do, so ignore it
    if (texx[0] == 0)
        return;

    if (ypos > 0)
        ypos = multiply_up_coordinate(ypos);

    if (game.options[OPT_ALWAYSSPCH])
        DisplaySpeechAt(-1, (ypos > 0) ? divide_down_coordinate(ypos) : ypos, -1, game.playercharacter, texx);
    else { 
        // Normal "Display" in text box

        if (screen_is_dirty) {
            // erase any previous DisplaySpeech
            play.disabled_user_interface ++;
            mainloop();
            play.disabled_user_interface --;
        }

        _display_at(-1,ypos,scrnwid/2+scrnwid/4,get_translation(texx),1,0, 0, 0, false);
    }
}

void SetSpeechStyle (int newstyle) {
    if ((newstyle < 0) || (newstyle > 3))
        quit("!SetSpeechStyle: must use a SPEECH_* constant as parameter");
    game.options[OPT_SPEECHTYPE] = newstyle;
}

// 0 = click mouse or key to skip
// 1 = key only
// 2 = can't skip at all
// 3 = only on keypress, no auto timer
// 4 = mouseclick only
void SetSkipSpeech (int newval) {
    if ((newval < 0) || (newval > 4))
        quit("!SetSkipSpeech: invalid skip mode specified (0-4)");

    DEBUG_CONSOLE("SkipSpeech style set to %d", newval);
    play.cant_skip_speech = user_to_internal_skip_speech(newval);
}

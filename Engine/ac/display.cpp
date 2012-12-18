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

#include "util/wgt2allg.h"
#include "ac/display.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_game.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/record.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "ac/topbarsettings.h"
#include "debug/debug_log.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern GameState play;
extern GameSetupStruct game;
extern GUIMain *guis;
extern int longestline;
extern int scrnwid,scrnhit;
extern Bitmap *virtual_screen;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern volatile int timerloop;
extern AGSPlatformDriver *platform;
extern volatile unsigned long globalTimerCounter;
extern int time_between_timers;
extern int offsetx, offsety;
extern int frames_per_second;
extern int loops_per_character;
extern IAGSFontRenderer* fontRenderers[MAX_FONTS];
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern DynamicArray<GUIButton> guibuts;

int display_message_aschar=0;
char *heightTestString = "ZHwypgfjqhkilIK";


TopBarSettings topBar;
// draw_text_window: draws the normal or custom text window
// create a new bitmap the size of the window before calling, and
//   point abuf to it
// returns text start x & y pos in parameters
Bitmap *screenop = NULL;
int wantFreeScreenop = 0;
int texthit;

// Pass yy = -1 to find Y co-ord automatically
// allowShrink = 0 for none, 1 for leftwards, 2 for rightwards
// pass blocking=2 to create permanent overlay
int _display_main(int xx,int yy,int wii,char*todis,int blocking,int usingfont,int asspch, int isThought, int allowShrink, bool overlayPositionFixed) 
{
    bool alphaChannel = false;
    ensure_text_valid_for_font(todis, usingfont);
    break_up_text_into_lines(wii-6,usingfont,todis);
    texthit = wgetfontheight(usingfont);

    // AGS 2.x: If the screen is faded out, fade in again when displaying a message box.
    if (!asspch && (loaded_game_file_version <= kGameVersion_272))
        play.screen_is_faded_out = 0;

    // if it's a normal message box and the game was being skipped,
    // ensure that the screen is up to date before the message box
    // is drawn on top of it
    if ((play.skip_until_char_stops >= 0) && (blocking == 1))
        render_graphics();

    EndSkippingUntilCharStops();

    if (topBar.wantIt) {
        // ensure that the window is wide enough to display
        // any top bar text
        int topBarWid = wgettextwidth_compensate(topBar.text, topBar.font);
        topBarWid += multiply_up_coordinate(play.top_bar_borderwidth + 2) * 2;
        if (longestline < topBarWid)
            longestline = topBarWid;
        // the top bar should behave like DisplaySpeech wrt blocking
        blocking = 0;
    }

    if (asspch > 0) {
        // update the all_buttons_disabled variable in advance
        // of the adjust_x/y_for_guis calls
        play.disabled_user_interface++;
        update_gui_disabled_status();
        play.disabled_user_interface--;
    }

    if (xx == OVR_AUTOPLACE) ;
    // centre text in middle of screen
    else if (yy<0) yy=(scrnhit/2-(numlines*texthit)/2)-3;
    // speech, so it wants to be above the character's head
    else if (asspch > 0) {
        yy-=numlines*texthit;
        if (yy < 5) yy=5;
        yy = adjust_y_for_guis (yy);
    }

    if (longestline < wii - get_fixed_pixel_size(6)) {
        // shrink the width of the dialog box to fit the text
        int oldWid = wii;
        //if ((asspch >= 0) || (allowShrink > 0))
        // If it's not speech, or a shrink is allowed, then shrink it
        if ((asspch == 0) || (allowShrink > 0))
            wii = longestline + get_fixed_pixel_size(6);

        // shift the dialog box right to align it, if necessary
        if ((allowShrink == 2) && (xx >= 0))
            xx += (oldWid - wii);
    }

    if (xx<-1) { 
        xx=(-xx)-wii/2;
        if (xx < 0)
            xx = 0;

        xx = adjust_x_for_guis (xx, yy);

        if (xx + wii >= scrnwid)
            xx = (scrnwid - wii) - 5;
    }
    else if (xx<0) xx=scrnwid/2-wii/2;

    int ee, extraHeight = get_fixed_pixel_size(6);
    wtextcolor(15);
    if (blocking < 2)
        remove_screen_overlay(OVER_TEXTMSG);

    screenop = BitmapHelper::CreateBitmap((wii > 0) ? wii : 2, numlines*texthit + extraHeight, final_col_dep);
    wsetscreen(screenop);
    screenop->Clear(screenop->GetMaskColor());

    // inform draw_text_window to free the old bitmap
    wantFreeScreenop = 1;

    if ((strlen (todis) < 1) || (strcmp (todis, "  ") == 0) || (wii == 0)) ;
    // if it's an empty speech line, don't draw anything
    else if (asspch) { //wtextcolor(12);
        int ttxleft = 0, ttxtop = get_fixed_pixel_size(3), oriwid = wii - 6;
        int usingGui = -1, drawBackground = 0;

        if ((asspch < 0) && (game.options[OPT_SPEECHTYPE] >= 2)) {
            usingGui = play.speech_textwindow_gui;
            drawBackground = 1;
        }
        else if ((isThought) && (game.options[OPT_THOUGHTGUI] > 0)) {
            usingGui = game.options[OPT_THOUGHTGUI];
            // make it treat it as drawing inside a window now
            if (asspch > 0)
                asspch = -asspch;
            drawBackground = 1;
        }

        if (drawBackground)
            draw_text_window_and_bar(&ttxleft, &ttxtop, &xx, &yy, &wii, 0, usingGui);
        else if ((ShouldAntiAliasText()) && (final_col_dep >= 24))
            alphaChannel = true;

        for (ee=0;ee<numlines;ee++) {
            //int ttxp=wii/2 - wgettextwidth_compensate(lines[ee], usingfont)/2;
            int ttyp=ttxtop+ee*texthit;
            // asspch < 0 means that it's inside a text box so don't
            // centre the text
            if (asspch < 0) {
                if ((usingGui >= 0) && 
                    ((game.options[OPT_SPEECHTYPE] >= 2) || (isThought)))
                    wtextcolor(guis[usingGui].fgcol);
                else
                    wtextcolor(-asspch);

                wouttext_aligned(ttxleft, ttyp, oriwid, usingfont, lines[ee], play.text_align);
            }
            else {
                wtextcolor(asspch);
                //wouttext_outline(ttxp,ttyp,usingfont,lines[ee]);
                wouttext_aligned(ttxleft, ttyp, wii, usingfont, lines[ee], play.speech_text_align);
            }
        }
    }
    else {
        int xoffs,yoffs, oriwid = wii - 6;
        draw_text_window_and_bar(&xoffs,&yoffs,&xx,&yy,&wii);

        if (game.options[OPT_TWCUSTOM] > 0)
        {
            alphaChannel = guis[game.options[OPT_TWCUSTOM]].is_alpha();
        }

        adjust_y_coordinate_for_text(&yoffs, usingfont);

        for (ee=0;ee<numlines;ee++)
            wouttext_aligned (xoffs, yoffs + ee * texthit, oriwid, usingfont, lines[ee], play.text_align);
    }

    wantFreeScreenop = 0;

    int ovrtype = OVER_TEXTMSG;
    if (blocking == 2) ovrtype=OVER_CUSTOM;
    else if (blocking >= OVER_CUSTOM) ovrtype=blocking;

    int nse = add_screen_overlay(xx, yy, ovrtype, screenop, alphaChannel);

    wsetscreen(virtual_screen);
    if (blocking>=2) {
        return screenover[nse].type;
    }

    if (blocking) {
        if (play.fast_forward) {
            remove_screen_overlay(OVER_TEXTMSG);
            play.messagetime=-1;
            return 0;
        }

        /*    wputblock(xx,yy,screenop,1);
        remove_screen_overlay(OVER_TEXTMSG);*/

        if (!play.mouse_cursor_hidden)
            domouse(1);
        // play.skip_display has same values as SetSkipSpeech:
        // 0 = click mouse or key to skip
        // 1 = key only
        // 2 = can't skip at all
        // 3 = only on keypress, no auto timer
        // 4 = mouse only
        int countdown = GetTextDisplayTime (todis);
        int skip_setting = user_to_internal_skip_speech(play.skip_display);
        while (1) {
            timerloop = 0;
            NEXT_ITERATION();
            /*      if (!play.mouse_cursor_hidden)
            domouse(0);
            write_screen();*/

            render_graphics();

            update_polled_stuff_and_crossfade();
            if (mgetbutton()>NONE) {
                // If we're allowed, skip with mouse
                if (skip_setting & SKIP_MOUSECLICK)
                    break;
            }
            if (kbhit()) {
                // discard keypress, and don't leave extended keys over
                int kp = getch();
                if (kp == 0) getch();

                // let them press ESC to skip the cutscene
                check_skip_cutscene_keypress (kp);
                if (play.fast_forward)
                    break;

                if (skip_setting & SKIP_KEYPRESS)
                    break;
            }
            while ((timerloop == 0) && (play.fast_forward == 0)) {
                update_polled_stuff_if_runtime();
                platform->YieldCPU();
            }
            countdown--;

            if (channels[SCHAN_SPEECH] != NULL) {
                // extend life of text if the voice hasn't finished yet
                if ((!rec_isSpeechFinished()) && (play.fast_forward == 0)) {
                    if (countdown <= 1)
                        countdown = 1;
                }
                else  // if the voice has finished, remove the speech
                    countdown = 0;
            }

            if ((countdown < 1) && (skip_setting & SKIP_AUTOTIMER))
            {
                play.ignore_user_input_until_time = globalTimerCounter + (play.ignore_user_input_after_text_timeout_ms / time_between_timers);
                break;
            }
            // if skipping cutscene, don't get stuck on No Auto Remove
            // text boxes
            if ((countdown < 1) && (play.fast_forward))
                break;
        }
        if (!play.mouse_cursor_hidden)
            domouse(2);
        remove_screen_overlay(OVER_TEXTMSG);

        construct_virtual_screen(true);
    }
    else {
        // if the speech does not time out, but we are skipping a cutscene,
        // allow it to time out
        if ((play.messagetime < 0) && (play.fast_forward))
            play.messagetime = 2;

        if (!overlayPositionFixed)
        {
            screenover[nse].positionRelativeToScreen = false;
            screenover[nse].x += offsetx;
            screenover[nse].y += offsety;
        }

        do_main_cycle(UNTIL_NOOVERLAY,0);
    }

    play.messagetime=-1;
    return 0;
}

void _display_at(int xx,int yy,int wii,char*todis,int blocking,int asspch, int isThought, int allowShrink, bool overlayPositionFixed) {
    int usingfont=FONT_NORMAL;
    if (asspch) usingfont=FONT_SPEECH;
    int needStopSpeech = 0;

    EndSkippingUntilCharStops();

    if (todis[0]=='&') {
        // auto-speech
        int igr=atoi(&todis[1]);
        while ((todis[0]!=' ') & (todis[0]!=0)) todis++;
        if (todis[0]==' ') todis++;
        if (igr <= 0)
            quit("Display: auto-voice symbol '&' not followed by valid integer");
        if (play_speech(play.narrator_speech,igr)) {
            // if Voice Only, then blank out the text
            if (play.want_speech == 2)
                todis = "  ";
        }
        needStopSpeech = 1;
    }
    _display_main(xx,yy,wii,todis,blocking,usingfont,asspch, isThought, allowShrink, overlayPositionFixed);

    if (needStopSpeech)
        stop_speech();
}

int   source_text_length = -1;

int GetTextDisplayTime (const char *text, int canberel) {
    int uselen = strlen(text);

    int fpstimer = frames_per_second;

    // if it's background speech, make it stay relative to game speed
    if ((canberel == 1) && (play.bgspeech_game_speed == 1))
        fpstimer = 40;

    if (source_text_length >= 0) {
        // sync to length of original text, to make sure any animations
        // and music sync up correctly
        uselen = source_text_length;
        source_text_length = -1;
    }
    else {
        if ((text[0] == '&') && (play.unfactor_speech_from_textlength != 0)) {
            // if there's an "&12 text" type line, remove "&12 " from the source
            // length
            int j = 0;
            while ((text[j] != ' ') && (text[j] != 0))
                j++;
            j++;
            uselen -= j;
        }

    }

    if (uselen <= 0)
        return 0;

    if (play.text_speed + play.text_speed_modifier <= 0)
        quit("!Text speed is zero; unable to display text. Check your game.text_speed settings.");

    // Store how many game loops per character of text
    // This is calculated using a hard-coded 15 for the text speed,
    // so that it's always the same no matter how fast the user
    // can read.
    loops_per_character = (((uselen/play.lipsync_speed)+1) * fpstimer) / uselen;

    int textDisplayTimeInMS = ((uselen / (play.text_speed + play.text_speed_modifier)) + 1) * 1000;
    if (textDisplayTimeInMS < play.text_min_display_time_ms)
        textDisplayTimeInMS = play.text_min_display_time_ms;

    return (textDisplayTimeInMS * fpstimer) / 1000;
}

bool ShouldAntiAliasText() {
    return (game.options[OPT_ANTIALIASFONTS] != 0);
}

void wouttext_outline(int xxp, int yyp, int usingfont, char*texx) {
    int otextc=textcol;

    if (game.fontoutline[usingfont] >= 0) {
        wtextcolor(play.speech_text_shadow);
        // MACPORT FIX 9/6/5: cast
        wouttextxy(xxp, yyp, (int)game.fontoutline[usingfont], texx);
    }
    else if (game.fontoutline[usingfont] == FONT_OUTLINE_AUTO) {
        wtextcolor(play.speech_text_shadow);

        int outlineDist = 1;

        if ((game.options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[usingfont]->SupportsExtendedCharacters(usingfont))) {
            // if it's a scaled up SCI font, move the outline out more
            outlineDist = get_fixed_pixel_size(1);
        }

        // move the text over so that it's still within the bounding rect
        xxp += outlineDist;
        yyp += outlineDist;

        wouttextxy(xxp - outlineDist, yyp, usingfont, texx);
        wouttextxy(xxp + outlineDist, yyp, usingfont, texx);
        wouttextxy(xxp, yyp + outlineDist, usingfont, texx);
        wouttextxy(xxp, yyp - outlineDist, usingfont, texx);
        wouttextxy(xxp - outlineDist, yyp - outlineDist, usingfont, texx);
        wouttextxy(xxp - outlineDist, yyp + outlineDist, usingfont, texx);
        wouttextxy(xxp + outlineDist, yyp + outlineDist, usingfont, texx);
        wouttextxy(xxp + outlineDist, yyp - outlineDist, usingfont, texx);
    }

    textcol = otextc;
    wouttextxy(xxp, yyp, usingfont, texx);
}

void wouttext_aligned (int usexp, int yy, int oriwid, int usingfont, const char *text, int align) {

    if (align == SCALIGN_CENTRE)
        usexp = usexp + (oriwid / 2) - (wgettextwidth_compensate(text, usingfont) / 2);
    else if (align == SCALIGN_RIGHT)
        usexp = usexp + (oriwid - wgettextwidth_compensate(text, usingfont));

    wouttext_outline(usexp, yy, usingfont, (char *)text);
}

int wgetfontheight(int font) {
    int htof = wgettextheight(heightTestString, font);

    // automatic outline fonts are 2 pixels taller
    if (game.fontoutline[font] == FONT_OUTLINE_AUTO) {
        // scaled up SCI font, push outline further out
        if ((game.options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
            htof += get_fixed_pixel_size(2);
        // otherwise, just push outline by 1 pixel
        else
            htof += 2;
    }

    return htof;
}

int wgettextwidth_compensate(const char *tex, int font) {
    int wdof = wgettextwidth(tex, font);

    if (game.fontoutline[font] == FONT_OUTLINE_AUTO) {
        // scaled up SCI font, push outline further out
        if ((game.options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
            wdof += get_fixed_pixel_size(2);
        // otherwise, just push outline by 1 pixel
        else
            wdof += get_fixed_pixel_size(1);
    }

    return wdof;
}

void do_corner(int sprn,int xx1,int yy1,int typx,int typy) {
    if (sprn<0) return;
    Bitmap *thisone = spriteset[sprn];
    if (thisone == NULL)
        thisone = spriteset[0];

    put_sprite_256(xx1+typx*spritewidth[sprn],yy1+typy*spriteheight[sprn],thisone);
    //  wputblock(xx1+typx*spritewidth[sprn],yy1+typy*spriteheight[sprn],thisone,1);
}

int get_but_pic(GUIMain*guo,int indx) {
    return guibuts[guo->objrefptr[indx] & 0x000ffff].pic;
}

void draw_button_background(int xx1,int yy1,int xx2,int yy2,GUIMain*iep) {
    if (iep==NULL) {  // standard window
        abuf->FillRect(Rect(xx1,yy1,xx2,yy2),get_col8_lookup(15));
        abuf->DrawRect(Rect(xx1,yy1,xx2,yy2),get_col8_lookup(16));
        /*    wsetcolor(opts.tws.backcol); abuf->FillRect(Rect(xx1,yy1,xx2,yy2);
        wsetcolor(opts.tws.textcol); abuf->DrawRect(Rect(xx1+1,yy1+1,xx2-1,yy2-1);*/
    }
    else {
        if (loaded_game_file_version < kGameVersion_262) // < 2.62
        {
            // Color 0 wrongly shows as transparent instead of black
            // From the changelog of 2.62:
            //  - Fixed text windows getting a black background if colour 0 was
            //    specified, rather than being transparent.
            if (iep->bgcol == 0)
                iep->bgcol = 16;
        }

        if (iep->bgcol >= 0) wsetcolor(iep->bgcol);
        else wsetcolor(0); // black backrgnd behind picture

        if (iep->bgcol > 0)
            abuf->FillRect(Rect(xx1,yy1,xx2,yy2), currentcolor);

        int leftRightWidth = spritewidth[get_but_pic(iep,4)];
        int topBottomHeight = spriteheight[get_but_pic(iep,6)];
        if (iep->bgpic>0) {
            if ((loaded_game_file_version <= kGameVersion_272) // 2.xx
                && (spriteset[iep->bgpic]->GetWidth() == 1)
                && (spriteset[iep->bgpic]->GetHeight() == 1) 
                && (*((unsigned int*)spriteset[iep->bgpic]->GetData()) == 0x00FF00FF))
            {
                // Don't draw fully transparent dummy GUI backgrounds
            }
            else
            {
                // offset the background image and clip it so that it is drawn
                // such that the border graphics can have a transparent outside
                // edge
                int bgoffsx = xx1 - leftRightWidth / 2;
                int bgoffsy = yy1 - topBottomHeight / 2;
                abuf->SetClip(Rect(bgoffsx, bgoffsy, xx2 + leftRightWidth / 2, yy2 + topBottomHeight / 2));
                int bgfinishx = xx2;
                int bgfinishy = yy2;
                int bgoffsyStart = bgoffsy;
                while (bgoffsx <= bgfinishx)
                {
                    bgoffsy = bgoffsyStart;
                    while (bgoffsy <= bgfinishy)
                    {
                        wputblock(bgoffsx, bgoffsy, spriteset[iep->bgpic], 0);
                        bgoffsy += spriteheight[iep->bgpic];
                    }
                    bgoffsx += spritewidth[iep->bgpic];
                }
                // return to normal clipping rectangle
                abuf->SetClip(Rect(0, 0, abuf->GetWidth() - 1, abuf->GetHeight() - 1));
            }
        }
        int uu;
        for (uu=yy1;uu <= yy2;uu+=spriteheight[get_but_pic(iep,4)]) {
            do_corner(get_but_pic(iep,4),xx1,uu,-1,0);   // left side
            do_corner(get_but_pic(iep,5),xx2+1,uu,0,0);  // right side
        }
        for (uu=xx1;uu <= xx2;uu+=spritewidth[get_but_pic(iep,6)]) {
            do_corner(get_but_pic(iep,6),uu,yy1,0,-1);  // top side
            do_corner(get_but_pic(iep,7),uu,yy2+1,0,0); // bottom side
        }
        do_corner(get_but_pic(iep,0),xx1,yy1,-1,-1);  // top left
        do_corner(get_but_pic(iep,1),xx1,yy2+1,-1,0);  // bottom left
        do_corner(get_but_pic(iep,2),xx2+1,yy1,0,-1);  //  top right
        do_corner(get_but_pic(iep,3),xx2+1,yy2+1,0,0);  // bottom right
    }
}

// Calculate the width that the left and right border of the textwindow
// GUI take up
int get_textwindow_border_width (int twgui) {
    if (twgui < 0)
        return 0;

    if (!guis[twgui].is_textwindow())
        quit("!GUI set as text window but is not actually a text window GUI");

    int borwid = spritewidth[get_but_pic(&guis[twgui], 4)] + 
        spritewidth[get_but_pic(&guis[twgui], 5)];

    return borwid;
}

// get the hegiht of the text window's top border
int get_textwindow_top_border_height (int twgui) {
    if (twgui < 0)
        return 0;

    if (!guis[twgui].is_textwindow())
        quit("!GUI set as text window but is not actually a text window GUI");

    return spriteheight[get_but_pic(&guis[twgui], 6)];
}

void draw_text_window(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight, int ifnum) {
    if (ifnum < 0)
        ifnum = game.options[OPT_TWCUSTOM];

    if (ifnum <= 0) {
        if (ovrheight)
            quit("!Cannot use QFG4 style options without custom text window");
        draw_button_background(0,0,abuf->GetWidth() - 1,abuf->GetHeight() - 1,NULL);
        wtextcolor(16);
        xins[0]=3;
        yins[0]=3;
    }
    else {
        if (ifnum >= game.numgui)
            quitprintf("!Invalid GUI %d specified as text window (total GUIs: %d)", ifnum, game.numgui);
        if (!guis[ifnum].is_textwindow())
            quit("!GUI set as text window but is not actually a text window GUI");

        int tbnum = get_but_pic(&guis[ifnum], 0);

        wii[0] += get_textwindow_border_width (ifnum);
        xx[0]-=spritewidth[tbnum];
        yy[0]-=spriteheight[tbnum];
        if (ovrheight == 0)
            ovrheight = numlines*texthit;

        if ((wantFreeScreenop > 0) && (screenop != NULL))
            delete screenop;
        screenop = BitmapHelper::CreateBitmap(wii[0],ovrheight+6+spriteheight[tbnum]*2,final_col_dep);
        screenop->Clear(screenop->GetMaskColor());
        wsetscreen(screenop);
        int xoffs=spritewidth[tbnum],yoffs=spriteheight[tbnum];
        draw_button_background(xoffs,yoffs,(abuf->GetWidth() - xoffs) - 1,(abuf->GetHeight() - yoffs) - 1,&guis[ifnum]);
        wtextcolor(guis[ifnum].fgcol);
        xins[0]=xoffs+3;
        yins[0]=yoffs+3;
    }

}

void draw_text_window_and_bar(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight, int ifnum) {

    draw_text_window(xins, yins, xx, yy, wii, ovrheight, ifnum);

    if ((topBar.wantIt) && (screenop != NULL)) {
        // top bar on the dialog window with character's name
        // create an enlarged window, then free the old one
        Bitmap *newScreenop = BitmapHelper::CreateBitmap(screenop->GetWidth(), screenop->GetHeight() + topBar.height, final_col_dep);
        newScreenop->Blit(screenop, 0, 0, 0, topBar.height, screenop->GetWidth(), screenop->GetHeight());
        delete screenop;
        screenop = newScreenop;
        wsetscreen(screenop);

        // draw the top bar
        screenop->FillRect(Rect(0, 0, screenop->GetWidth() - 1, topBar.height - 1), get_col8_lookup(play.top_bar_backcolor));
        if (play.top_bar_backcolor != play.top_bar_bordercolor) {
            // draw the border
            for (int j = 0; j < multiply_up_coordinate(play.top_bar_borderwidth); j++)
                screenop->DrawRect(Rect(j, j, screenop->GetWidth() - (j + 1), topBar.height - (j + 1)), get_col8_lookup(play.top_bar_bordercolor));
        }

        int textcolwas = textcol;
        // draw the text
        int textx = (screenop->GetWidth() / 2) - wgettextwidth_compensate(topBar.text, topBar.font) / 2;
        wtextcolor(play.top_bar_textcolor);
        wouttext_outline(textx, play.top_bar_borderwidth + get_fixed_pixel_size(1), topBar.font, topBar.text);
        // restore the current text colour
        textcol = textcolwas;

        // don't draw it next time
        topBar.wantIt = 0;
        // adjust the text Y position
        yins[0] += topBar.height;
    }
    else if (topBar.wantIt)
        topBar.wantIt = 0;
}

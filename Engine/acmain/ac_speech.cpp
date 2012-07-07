
#include <stdio.h>
#include "wgt2allg.h"
#include "ac/ac_view.h"
#include "ac/gamesetupstruct.h"
#include "ac/gui.h"
#include "ac/global_game.h"
#include "ac/screenoverlay.h"
#include "ac/viewframe.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_speech.h"
#include "acmain/ac_commonheaders.h"
#include "acmain/ac_lipsync.h"
#include "gui/guimain.h"
#include "media/audio/audio.h"
#include "media/audio/sound.h"
#include "sprcache.h"

extern GameSetupStruct game;
extern GUIMain*guis;
extern ViewStruct*views;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int is_text_overlay;
extern int said_speech_line;
extern int said_text;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int our_eip,displayed_room;


// Sierra-style speech settings
int face_talking=-1,facetalkview=0,facetalkwait=0,facetalkframe=0;
int facetalkloop=0, facetalkrepeat = 0, facetalkAllowBlink = 1;
int facetalkBlinkLoop = 0;
CharacterInfo *facetalkchar = NULL;



void SetTextWindowGUI (int guinum) {
    if ((guinum < -1) | (guinum >= game.numgui))
        quit("!SetTextWindowGUI: invalid GUI number");

    if (guinum < 0) ;  // disable it
    else if (!guis[guinum].is_textwindow())
        quit("!SetTextWindowGUI: specified GUI is not a text window");

    if (play.speech_textwindow_gui == game.options[OPT_TWCUSTOM])
        play.speech_textwindow_gui = guinum;
    game.options[OPT_TWCUSTOM] = guinum;
}



int DisplaySpeechBackground(int charid,char*speel) {
    // remove any previous background speech for this character
    int cc;
    for (cc = 0; cc < numscreenover; cc++) {
        if (screenover[cc].bgSpeechForChar == charid) {
            remove_screen_overlay_index(cc);
            cc--;
        }
    }

    int ovrl=CreateTextOverlay(OVR_AUTOPLACE,charid,scrnwid/2,FONT_SPEECH,
        -game.chars[charid].talkcolor, get_translation(speel));

    int scid = find_overlay_of_type(ovrl);
    screenover[scid].bgSpeechForChar = charid;
    screenover[scid].timeout = GetTextDisplayTime(speel, 1);
    return ovrl;
}




int play_speech(int charid,int sndid) {
    stop_and_destroy_channel (SCHAN_SPEECH);

    // don't play speech if we're skipping a cutscene
    if (play.fast_forward)
        return 0;
    if ((play.want_speech < 1) || (speech_file == NULL))
        return 0;

    SOUNDCLIP *speechmp3;
    /*  char finame[40]="~SPEECH.VOX~NARR";
    if (charid >= 0)
    strncpy(&finame[12],game.chars[charid].scrname,4);*/

    char finame[40] = "~";
    strcat(finame, get_filename(speech_file));
    strcat(finame, "~");

    if (charid >= 0) {
        // append the first 4 characters of the script name to the filename
        char theScriptName[5];
        if (game.chars[charid].scrname[0] == 'c')
            strncpy(theScriptName, &game.chars[charid].scrname[1], 4);
        else
            strncpy(theScriptName, game.chars[charid].scrname, 4);
        theScriptName[4] = 0;
        strcat(finame, theScriptName);
    }
    else
        strcat(finame, "NARR");

    // append the speech number
    sprintf(&finame[strlen(finame)],"%d",sndid);

    int ii;  // Compare the base file name to the .pam file name
    char *basefnptr = strchr (&finame[4], '~') + 1;
    curLipLine = -1;  // See if we have voice lip sync for this line
    curLipLinePhenome = -1;
    for (ii = 0; ii < numLipLines; ii++) {
        if (stricmp(splipsync[ii].filename, basefnptr) == 0) {
            curLipLine = ii;
            break;
        }
    }
    // if the lip-sync is being used for voice sync, disable
    // the text-related lipsync
    if (numLipLines > 0)
        game.options[OPT_LIPSYNCTEXT] = 0;

    strcat (finame, ".WAV");
    speechmp3 = my_load_wave (finame, play.speech_volume, 0);

    if (speechmp3 == NULL) {
        strcpy (&finame[strlen(finame)-3], "ogg");
        speechmp3 = my_load_ogg (finame, play.speech_volume);
    }

    if (speechmp3 == NULL) {
        strcpy (&finame[strlen(finame)-3], "mp3");
        speechmp3 = my_load_mp3 (finame, play.speech_volume);
    }

    if (speechmp3 != NULL) {
        if (speechmp3->play() == 0)
            speechmp3 = NULL;
    }

    if (speechmp3 == NULL) {
        debug_log ("Speech load failure: '%s'",finame);
        curLipLine = -1;
        return 0;
    }

    channels[SCHAN_SPEECH] = speechmp3;
    play.music_vol_was = play.music_master_volume;

    // Negative value means set exactly; positive means drop that amount
    if (play.speech_music_drop < 0)
        play.music_master_volume = -play.speech_music_drop;
    else
        play.music_master_volume -= play.speech_music_drop;

    apply_volume_drop_modifier(true);
    update_music_volume();
    update_music_at = 0;
    mvolcounter = 0;

    update_ambient_sound_vol();

    // change Sierra w/bgrnd  to Sierra without background when voice
    // is available (for Tierra)
    if ((game.options[OPT_SPEECHTYPE] == 2) && (play.no_textbg_when_voice > 0)) {
        game.options[OPT_SPEECHTYPE] = 1;
        play.no_textbg_when_voice = 2;
    }

    return 1;
}

void stop_speech() {
    if (channels[SCHAN_SPEECH] != NULL) {
        play.music_master_volume = play.music_vol_was;
        // update the music in a bit (fixes two speeches follow each other
        // and music going up-then-down)
        update_music_at = 20;
        mvolcounter = 1;
        stop_and_destroy_channel (SCHAN_SPEECH);
        curLipLine = -1;

        if (play.no_textbg_when_voice == 2) {
            // set back to Sierra w/bgrnd
            play.no_textbg_when_voice = 1;
            game.options[OPT_SPEECHTYPE] = 2;
        }
    }
}
void SetSpeechVolume(int newvol) {
    if ((newvol<0) | (newvol>255))
        quit("!SetSpeechVolume: invalid volume - must be from 0-255");

    if (channels[SCHAN_SPEECH])
        channels[SCHAN_SPEECH]->set_volume (newvol);

    play.speech_volume = newvol;
}

void SetSpeechFont (int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.numfonts))
        quit("!SetSpeechFont: invalid font number.");
    play.speech_font = fontnum;
}
void SetNormalFont (int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.numfonts))
        quit("!SetNormalFont: invalid font number.");
    play.normal_font = fontnum;
}

void __scr_play_speech(int who, int which) {
    // *** implement this - needs to call stop_speech as well
    // to reset the volume
    quit("PlaySpeech not yet implemented");
}

// 0 = text only
// 1 = voice & text
// 2 = voice only
void SetVoiceMode (int newmod) {
    if ((newmod < 0) | (newmod > 2))
        quit("!SetVoiceMode: invalid mode number (must be 0,1,2)");
    // If speech is turned off, store the mode anyway in case the
    // user adds the VOX file later
    if (play.want_speech < 0)
        play.want_speech = (-newmod) - 1;
    else
        play.want_speech = newmod;
}

int IsVoxAvailable() {
    if (play.want_speech < 0)
        return 0;
    return 1;
}

int IsMusicVoxAvailable () {
    return play.seperate_music_lib;
}


void SetSpeechStyle (int newstyle) {
    if ((newstyle < 0) || (newstyle > 3))
        quit("!SetSpeechStyle: must use a SPEECH_* constant as parameter");
    game.options[OPT_SPEECHTYPE] = newstyle;
}


void _displayspeech(char*texx, int aschar, int xx, int yy, int widd, int isThought) {
    if (!is_valid_character(aschar))
        quit("!DisplaySpeech: invalid character");

    CharacterInfo *speakingChar = &game.chars[aschar];
    if ((speakingChar->view < 0) || (speakingChar->view >= game.numviews))
        quit("!DisplaySpeech: character has invalid view");

    if (is_text_overlay > 0)
        quit("!DisplaySpeech: speech was already displayed (nested DisplaySpeech, perhaps room script and global script conflict?)");

    EndSkippingUntilCharStops();

    said_speech_line = 1;

    int aa;
    if (play.bgspeech_stay_on_display == 0) {
        // remove any background speech
        for (aa=0;aa<numscreenover;aa++) {
            if (screenover[aa].timeout > 0) {
                remove_screen_overlay(screenover[aa].type);
                aa--;
            }
        }
    }
    said_text = 1;

    // the strings are pre-translated
    //texx = get_translation(texx);
    our_eip=150;

    int isPause = 1;
    // if the message is all .'s, don't display anything
    for (aa = 0; texx[aa] != 0; aa++) {
        if (texx[aa] != '.') {
            isPause = 0;
            break;
        }
    }

    play.messagetime = GetTextDisplayTime(texx);

    if (isPause) {
        if (update_music_at > 0)
            update_music_at += play.messagetime;
        do_main_cycle(UNTIL_INTISNEG,(int)&play.messagetime);
        return;
    }

    int textcol = speakingChar->talkcolor;

    // if it's 0, it won't be recognised as speech
    if (textcol == 0)
        textcol = 16;

    int allowShrink = 0;
    int bwidth = widd;
    if (bwidth < 0)
        bwidth = scrnwid/2 + scrnwid/4;

    our_eip=151;

    int useview = speakingChar->talkview;
    if (isThought) {
        useview = speakingChar->thinkview;
        // view 0 is not valid for think views
        if (useview == 0)
            useview = -1;
        // speech bubble can shrink to fit
        allowShrink = 1;
        if (speakingChar->room != displayed_room) {
            // not in room, centre it
            xx = -1;
            yy = -1;
        }
    }

    if (useview >= game.numviews)
        quitprintf("!Character.Say: attempted to use view %d for animation, but it does not exist", useview + 1);

    int tdxp = xx,tdyp = yy;
    int oldview=-1, oldloop = -1;
    int ovr_type = 0;

    text_lips_offset = 0;
    text_lips_text = texx;

    block closeupface=NULL;
    if (texx[0]=='&') {
        // auto-speech
        int igr=atoi(&texx[1]);
        while ((texx[0]!=' ') & (texx[0]!=0)) texx++;
        if (texx[0]==' ') texx++;
        if (igr <= 0)
            quit("DisplaySpeech: auto-voice symbol '&' not followed by valid integer");

        text_lips_text = texx;

        if (play_speech(aschar,igr)) {
            if (play.want_speech == 2)
                texx = "  ";  // speech only, no text.
        }
    }
    if (game.options[OPT_SPEECHTYPE] == 3)
        remove_screen_overlay(OVER_COMPLETE);
    our_eip=1500;

    if (game.options[OPT_SPEECHTYPE] == 0)
        allowShrink = 1;

    if (speakingChar->idleleft < 0)  {
        // if idle anim in progress for the character, stop it
        ReleaseCharacterView(aschar);
        //    speakingChar->idleleft = speakingChar->idletime;
    }

    bool overlayPositionFixed = false;
    int charFrameWas = 0;
    int viewWasLocked = 0;
    if (speakingChar->flags & CHF_FIXVIEW)
        viewWasLocked = 1;

    /*if ((speakingChar->room == displayed_room) ||
    ((useview >= 0) && (game.options[OPT_SPEECHTYPE] > 0)) ) {*/

    if (speakingChar->room == displayed_room) {
        // If the character is in this room, go for it - otherwise
        // run the "else" clause which  does text in the middle of
        // the screen.
        our_eip=1501;
        if (tdxp < 0)
            tdxp = multiply_up_coordinate(speakingChar->x) - offsetx;
        if (tdxp < 2)
            tdxp=2;

        if (speakingChar->walking)
            StopMoving(aschar);

        // save the frame we need to go back to
        // if they were moving, this will be 0 (because we just called
        // StopMoving); otherwise, it might be a specific animation 
        // frame which we should return to
        if (viewWasLocked)
            charFrameWas = speakingChar->frame;

        // if the current loop doesn't exist in talking view, use loop 0
        if (speakingChar->loop >= views[speakingChar->view].numLoops)
            speakingChar->loop = 0;

        if ((speakingChar->view < 0) || 
            (speakingChar->loop >= views[speakingChar->view].numLoops) ||
            (views[speakingChar->view].loops[speakingChar->loop].numFrames < 1))
        {
            quitprintf("Unable to display speech because the character %s has an invalid view frame (View %d, loop %d, frame %d)", speakingChar->scrname, speakingChar->view + 1, speakingChar->loop, speakingChar->frame);
        }

        our_eip=1504;

        if (tdyp < 0) 
        {
            int sppic = views[speakingChar->view].loops[speakingChar->loop].frames[0].pic;
            tdyp = multiply_up_coordinate(speakingChar->get_effective_y()) - offsety - get_fixed_pixel_size(5);
            if (charextra[aschar].height < 1)
                tdyp -= spriteheight[sppic];
            else
                tdyp -= charextra[aschar].height;
            // if it's a thought, lift it a bit further up
            if (isThought)  
                tdyp -= get_fixed_pixel_size(10);
        }

        our_eip=1505;
        if (tdyp < 5)
            tdyp=5;

        tdxp=-tdxp;  // tell it to centre it
        our_eip=152;

        if ((useview >= 0) && (game.options[OPT_SPEECHTYPE] > 0)) {
            // Sierra-style close-up portrait

            if (play.swap_portrait_lastchar != aschar) {
                // if the portraits are set to Alternate, OR they are
                // set to Left but swap_portrait has been set to 1 (the old
                // method for enabling it), then swap them round
                if ((game.options[OPT_PORTRAITSIDE] == PORTRAIT_ALTERNATE) ||
                    ((game.options[OPT_PORTRAITSIDE] == 0) &&
                    (play.swap_portrait_side > 0))) {

                        if (play.swap_portrait_side == 2)
                            play.swap_portrait_side = 1;
                        else
                            play.swap_portrait_side = 2;
                }

                if (game.options[OPT_PORTRAITSIDE] == PORTRAIT_XPOSITION) {
                    // Portrait side based on character X-positions
                    if (play.swap_portrait_lastchar < 0) {
                        // no previous character been spoken to
                        // therefore, find another character in this room
                        // that it could be
                        for (int ce = 0; ce < game.numcharacters; ce++) {
                            if ((game.chars[ce].room == speakingChar->room) &&
                                (game.chars[ce].on == 1) &&
                                (ce != aschar)) {
                                    play.swap_portrait_lastchar = ce;
                                    break;
                            }
                        }
                    }

                    if (play.swap_portrait_lastchar >= 0) {
                        // if this character is right of the one before, put the
                        // portrait on the right
                        if (speakingChar->x > game.chars[play.swap_portrait_lastchar].x)
                            play.swap_portrait_side = -1;
                        else
                            play.swap_portrait_side = 0;
                    }
                }

                play.swap_portrait_lastchar = aschar;
            }

            // Determine whether to display the portrait on the left or right
            int portrait_on_right = 0;

            if (game.options[OPT_SPEECHTYPE] == 3) 
            { }  // always on left with QFG-style speech
            else if ((play.swap_portrait_side == 1) ||
                (play.swap_portrait_side == -1) ||
                (game.options[OPT_PORTRAITSIDE] == PORTRAIT_RIGHT))
                portrait_on_right = 1;


            int bigx=0,bigy=0,kk;
            ViewStruct*viptr=&views[useview];
            for (kk = 0; kk < viptr->loops[0].numFrames; kk++) 
            {
                int tw = spritewidth[viptr->loops[0].frames[kk].pic];
                if (tw > bigx) bigx=tw;
                tw = spriteheight[viptr->loops[0].frames[kk].pic];
                if (tw > bigy) bigy=tw;
            }

            // if they accidentally used a large full-screen image as the sierra-style
            // talk view, correct it
            if ((game.options[OPT_SPEECHTYPE] != 3) && (bigx > scrnwid - get_fixed_pixel_size(50)))
                bigx = scrnwid - get_fixed_pixel_size(50);

            if (widd > 0)
                bwidth = widd - bigx;

            our_eip=153;
            int draw_yp = 0, ovr_yp = get_fixed_pixel_size(20);
            if (game.options[OPT_SPEECHTYPE] == 3) {
                // QFG4-style whole screen picture
                closeupface = create_bitmap_ex(bitmap_color_depth(spriteset[viptr->loops[0].frames[0].pic]), scrnwid, scrnhit);
                clear_to_color(closeupface, 0);
                draw_yp = scrnhit/2 - spriteheight[viptr->loops[0].frames[0].pic]/2;
                bigx = scrnwid/2 - get_fixed_pixel_size(20);
                ovr_type = OVER_COMPLETE;
                ovr_yp = 0;
                tdyp = -1;  // center vertically
            }
            else {
                // KQ6-style close-up face picture
                if (yy < 0)
                    ovr_yp = adjust_y_for_guis (ovr_yp);
                else
                    ovr_yp = yy;

                closeupface = create_bitmap_ex(bitmap_color_depth(spriteset[viptr->loops[0].frames[0].pic]),bigx+1,bigy+1);
                clear_to_color(closeupface,bitmap_mask_color(closeupface));
                ovr_type = OVER_PICTURE;

                if (yy < 0)
                    tdyp = ovr_yp + get_textwindow_top_border_height(play.speech_textwindow_gui);
            }
            //draw_sprite(closeupface,spriteset[viptr->frames[0][0].pic],0,draw_yp);
            DrawViewFrame(closeupface, &viptr->loops[0].frames[0], 0, draw_yp);

            int overlay_x = get_fixed_pixel_size(10);

            if (xx < 0) {
                tdxp = get_fixed_pixel_size(16) + bigx + get_textwindow_border_width(play.speech_textwindow_gui) / 2;

                int maxWidth = (scrnwid - tdxp) - get_fixed_pixel_size(5) - 
                    get_textwindow_border_width (play.speech_textwindow_gui) / 2;

                if (bwidth > maxWidth)
                    bwidth = maxWidth;
            }
            else {
                tdxp = xx + bigx + get_fixed_pixel_size(8);
                overlay_x = xx;
            }

            // allow the text box to be shrunk to fit the text
            allowShrink = 1;

            // if the portrait's on the right, swap it round
            if (portrait_on_right) {
                if ((xx < 0) || (widd < 0)) {
                    overlay_x = (scrnwid - bigx) - get_fixed_pixel_size(5);
                    tdxp = get_fixed_pixel_size(9);
                }
                else {
                    overlay_x = (xx + widd - bigx) - get_fixed_pixel_size(5);
                    tdxp = xx;
                }
                tdxp += get_textwindow_border_width(play.speech_textwindow_gui) / 2;
                allowShrink = 2;
            }
            if (game.options[OPT_SPEECHTYPE] == 3)
                overlay_x = 0;
            face_talking=add_screen_overlay(overlay_x,ovr_yp,ovr_type,closeupface);
            facetalkframe = 0;
            facetalkwait = viptr->loops[0].frames[0].speed + GetCharacterSpeechAnimationDelay(speakingChar);
            facetalkloop = 0;
            facetalkview = useview;
            facetalkrepeat = (isThought) ? 0 : 1;
            facetalkBlinkLoop = 0;
            facetalkAllowBlink = 1;
            if ((isThought) && (speakingChar->flags & CHF_NOBLINKANDTHINK))
                facetalkAllowBlink = 0;
            facetalkchar = &game.chars[aschar];
            if (facetalkchar->blinktimer < 0)
                facetalkchar->blinktimer = facetalkchar->blinkinterval;
            textcol=-textcol;
            overlayPositionFixed = true;
        }
        else if (useview >= 0) {
            // Lucasarts-style speech
            our_eip=154;

            oldview = speakingChar->view;
            oldloop = speakingChar->loop;
            speakingChar->animating = 1 | (GetCharacterSpeechAnimationDelay(speakingChar) << 8);
            // only repeat if speech, not thought
            if (!isThought)
                speakingChar->animating |= CHANIM_REPEAT;

            speakingChar->view = useview;
            speakingChar->frame=0;
            speakingChar->flags|=CHF_FIXVIEW;

            if (speakingChar->loop >= views[speakingChar->view].numLoops)
            {
                // current character loop is outside the normal talking directions
                speakingChar->loop = 0;
            }

            facetalkBlinkLoop = speakingChar->loop;

            if ((speakingChar->loop >= views[speakingChar->view].numLoops) ||
                (views[speakingChar->view].loops[speakingChar->loop].numFrames < 1))
            {
                quitprintf("!Unable to display speech because the character %s has an invalid speech view (View %d, loop %d, frame %d)", speakingChar->scrname, speakingChar->view + 1, speakingChar->loop, speakingChar->frame);
            }

            // set up the speed of the first frame
            speakingChar->wait = GetCharacterSpeechAnimationDelay(speakingChar) + 
                views[speakingChar->view].loops[speakingChar->loop].frames[0].speed;

            if (widd < 0) {
                bwidth = scrnwid/2 + scrnwid/6;
                // If they are close to the screen edge, make the text narrower
                int relx = multiply_up_coordinate(speakingChar->x) - offsetx;
                if ((relx < scrnwid / 4) || (relx > scrnwid - (scrnwid / 4)))
                    bwidth -= scrnwid / 5;
            }
            /*   this causes the text to bob up and down as they talk
            tdxp = OVR_AUTOPLACE;
            tdyp = aschar;*/
            if (!isThought)  // set up the lip sync if not thinking
                char_speaking = aschar;

        }
    }
    else
        allowShrink = 1;

    // it wants the centred position, so make it so
    if ((xx >= 0) && (tdxp < 0))
        tdxp -= widd / 2;

    // if they used DisplaySpeechAt, then use the supplied width
    if ((widd > 0) && (isThought == 0))
        allowShrink = 0;

    our_eip=155;
    _display_at(tdxp,tdyp,bwidth,texx,0,textcol, isThought, allowShrink, overlayPositionFixed);
    our_eip=156;
    if ((play.in_conversation > 0) && (game.options[OPT_SPEECHTYPE] == 3))
        closeupface = NULL;
    if (closeupface!=NULL)
        remove_screen_overlay(ovr_type);
    screen_is_dirty = 1;
    face_talking = -1;
    facetalkchar = NULL;
    our_eip=157;
    if (oldview>=0) {
        speakingChar->flags &= ~CHF_FIXVIEW;
        if (viewWasLocked)
            speakingChar->flags |= CHF_FIXVIEW;
        speakingChar->view=oldview;

        // Don't reset the loop in 2.x games
        if (loaded_game_file_version > 32)
            speakingChar->loop = oldloop;

        speakingChar->animating=0;
        speakingChar->frame = charFrameWas;
        speakingChar->wait=0;
        speakingChar->idleleft = speakingChar->idletime;
        // restart the idle animation straight away
        charextra[aschar].process_idle_this_time = 1;
    }
    char_speaking = -1;
    stop_speech();
}

int get_character_currently_talking() {
    if ((face_talking >= 0) && (facetalkrepeat))
        return facetalkchar->index_id;
    else if (char_speaking >= 0)
        return char_speaking;

    return -1;
}




void DisplaySpeech(char*texx, int aschar) {
    _displayspeech (texx, aschar, -1, -1, -1, 0);
}

// **** THIS IS UNDOCUMENTED BECAUSE IT DOESN'T WORK PROPERLY
// **** AT 640x400 AND DOESN'T USE THE RIGHT SPEECH STYLE
void DisplaySpeechAt (int xx, int yy, int wii, int aschar, char*spch) {
    multiply_up_coordinates(&xx, &yy);
    wii = multiply_up_coordinate(wii);
    _displayspeech (get_translation(spch), aschar, xx, yy, wii, 0);
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


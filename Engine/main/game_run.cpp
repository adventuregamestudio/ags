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

//
// Game loop
//

#include "ac/common.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_debug.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_region.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/record.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/roomstruct.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "main/mainheader.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/soundclip.h"
#include "plugin/agsplugin.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "ac/spritecache.h"

using namespace AGS::Common;

extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;
extern int mouse_on_iface;   // mouse cursor is over this interface
extern int ifacepopped;
extern int is_text_overlay;
extern volatile char want_exit, abort_engine;
extern int proper_exit,our_eip;
extern int displayed_room, starting_room, in_new_room, new_room_was;
extern GameSetupStruct game;
extern roomstruct thisroom;
extern int game_paused;
extern int getloctype_index;
extern int in_enters_screen,done_es_error;
extern int in_leaves_screen;
extern int inside_script,in_graph_script;
extern int no_blocking_functions;
extern CharacterInfo*playerchar;
extern GameState play;
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern int cur_mode;
extern RoomObject*objs;
extern int replay_start_this_time;
extern char noWalkBehindsAtAll;
extern RoomStatus*croom;
extern CharacterExtras *charextra;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int offsetx, offsety;
extern unsigned int loopcounter,lastcounter;
extern volatile int timerloop;
extern int cur_mode,cur_cursor;

// Checks if user interface should remain disabled for now
int ShouldStayInWaitMode();

int numEventsAtStartOfFunction;
long t1;  // timer for FPS // ... 't1'... how very appropriate.. :)

long user_disabled_for=0,user_disabled_data=0,user_disabled_data2=0;
int user_disabled_data3=0;

int restrict_until=0;

void ProperExit()
{
    want_exit = 0;
    proper_exit = 1;
    quit("||exit!");
}

void game_loop_check_problems_at_start()
{
    if ((in_enters_screen != 0) & (displayed_room == starting_room))
        quit("!A text script run in the Player Enters Screen event caused the\n"
        "screen to be updated. If you need to use Wait(), do so in After Fadein");
    if ((in_enters_screen != 0) && (done_es_error == 0)) {
        debug_script_warn("Wait() was used in Player Enters Screen - use Enters Screen After Fadein instead");
        done_es_error = 1;
    }
    if (no_blocking_functions)
        quit("!A blocking function was called from within a non-blocking event such as " REP_EXEC_ALWAYS_NAME);
}

void game_loop_check_new_room()
{
    if (in_new_room == 0) {
        // Run the room and game script repeatedly_execute
        run_function_on_non_blocking_thread(&repExecAlways);
        setevent(EV_TEXTSCRIPT,TS_REPEAT);
        setevent(EV_RUNEVBLOCK,EVB_ROOM,0,6);  
    }
    // run this immediately to make sure it gets done before fade-in
    // (player enters screen)
    check_new_room ();
}

void game_loop_do_late_update()
{
    if (in_new_room == 0)
    {
        // Run the room and game script late_repeatedly_execute
        run_function_on_non_blocking_thread(&lateRepExecAlways);
    }
}

int game_loop_check_ground_level_interactions()
{
    if ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0) {
        // check if he's standing on a hotspot
        int hotspotThere = get_hotspot_at(playerchar->x, playerchar->y);
        // run Stands on Hotspot event
        setevent(EV_RUNEVBLOCK, EVB_HOTSPOT, hotspotThere, 0);

        // check current region
        int onRegion = GetRegionAt (playerchar->x, playerchar->y);
        int inRoom = displayed_room;

        if (onRegion != play.player_on_region) {
            // we need to save this and set play.player_on_region
            // now, so it's correct going into RunRegionInteraction
            int oldRegion = play.player_on_region;

            play.player_on_region = onRegion;
            // Walks Off last region
            if (oldRegion > 0)
                RunRegionInteraction (oldRegion, 2);
            // Walks Onto new region
            if (onRegion > 0)
                RunRegionInteraction (onRegion, 1);
        }
        if (play.player_on_region > 0)   // player stands on region
            RunRegionInteraction (play.player_on_region, 0);

        // one of the region interactions sent us to another room
        if (inRoom != displayed_room) {
            check_new_room();
        }

        // if in a Wait loop which is no longer valid (probably
        // because the Region interaction did a NewRoom), abort
        // the rest of the loop
        if ((restrict_until) && (!ShouldStayInWaitMode())) {
            // cancel the Rep Exec and Stands on Hotspot events that
            // we just added -- otherwise the event queue gets huge
            numevents = numEventsAtStartOfFunction;
            return 0;
        }
    } // end if checking ground level interactions

    return RETURN_CONTINUE;
}

void lock_mouse_on_click()
{
    if (usetup.mouse_auto_lock && scsystem.windowed)
        Mouse::TryLockToWindow();
}

void toggle_mouse_lock()
{
    if (scsystem.windowed)
    {
        if (Mouse::IsLockedToWindow())
            Mouse::UnlockFromWindow();
        else
            Mouse::TryLockToWindow();
    }
}

// Returns current key modifiers;
// NOTE: annoyingly enough, on Windows (not sure about other platforms)
// Allegro API's 'key_shifts' variable seem to be always one step behind real
// situation: if first modifier gets pressed, 'key_shifts' will be zero,
// when second modifier gets pressed it will only contain first one, and so on.
int get_active_shifts()
{
    int shifts = 0;
    if (key[KEY_LSHIFT] || key[KEY_RSHIFT])
        shifts |= KB_SHIFT_FLAG;
    if (key[KEY_LCONTROL] || key[KEY_RCONTROL])
        shifts |= KB_CTRL_FLAG;
    if (key[KEY_ALT] || key[KEY_ALTGR])
        shifts |= KB_ALT_FLAG;
    return shifts;
}

// Special flags to OR saved shift flags with:
// Shifts key combination already fired (wait until full shifts release)
#define KEY_SHIFTS_FIRED      0x80000000

// check_controls: checks mouse & keyboard interface
void check_controls() {
    our_eip = 1007;
    NEXT_ITERATION();

    int aa,mongu=-1;
    
    mongu = gui_on_mouse_move();

    mouse_on_iface=mongu;
    if ((ifacepopped>=0) && (mousey>=guis[ifacepopped].Y+guis[ifacepopped].Height))
        remove_popup_interface(ifacepopped);

    // check mouse clicks on GUIs
    static int wasbutdown=0,wasongui=0;

    if ((wasbutdown>0) && (misbuttondown(wasbutdown-1))) {
        gui_on_mouse_hold(wasongui, wasbutdown);
    }
    else if ((wasbutdown>0) && (!misbuttondown(wasbutdown-1))) {
        gui_on_mouse_up(wasongui, wasbutdown);
        wasbutdown=0;
    }

    aa=mgetbutton();
    if (aa>NONE) {
        lock_mouse_on_click();

        if ((play.in_cutscene == 3) || (play.in_cutscene == 4))
            start_skipping_cutscene();
        if ((play.in_cutscene == 5) && (aa == RIGHT))
            start_skipping_cutscene();

        if (play.fast_forward) { }
        else if ((play.wait_counter > 0) && (play.key_skip_wait > 1))
            play.wait_counter = -1;
        else if (is_text_overlay > 0) {
            if (play.cant_skip_speech & SKIP_MOUSECLICK)
                remove_screen_overlay(OVER_TEXTMSG);
        }
        else if (!IsInterfaceEnabled()) ;  // blocking cutscene, ignore mouse
        else if (pl_run_plugin_hooks(AGSE_MOUSECLICK, aa+1)) {
            // plugin took the click
            debug_script_log("Plugin handled mouse button %d", aa+1);
        }
        else if (mongu>=0) {
            if (wasbutdown==0) {
                gui_on_mouse_down(mongu, aa+1);
            }            
            wasongui=mongu;
            wasbutdown=aa+1;
        }
        else setevent(EV_TEXTSCRIPT,TS_MCLICK,aa+1);
        //    else RunTextScriptIParam(gameinst,"on_mouse_click",aa+1);
    }
    aa = check_mouse_wheel();
    if (aa !=0)
        lock_mouse_on_click();
    if (aa < 0)
        setevent (EV_TEXTSCRIPT, TS_MCLICK, 9);
    else if (aa > 0)
        setevent (EV_TEXTSCRIPT, TS_MCLICK, 8);

    // check keypresses
    static int old_key_shifts = 0; // for saving shift modes

    int kbhit_res = kbhit();
    // First, check shifts
    const int act_shifts = get_active_shifts();
    // If shifts combination have already triggered an action, then do nothing
    // until new shifts are empty, in which case reset saved shifts
    if (old_key_shifts & KEY_SHIFTS_FIRED)
    {
        if (act_shifts == 0)
            old_key_shifts = 0;
    }
    else
    {
        // If any non-shift key is pressed, add fired flag to indicate that
        // this is no longer a pure shifts key combination
        if (kbhit_res)
        {
            old_key_shifts = act_shifts | KEY_SHIFTS_FIRED;
        }
        // If all the previously registered shifts are still pressed,
        // then simply resave new shift state.
        else if ((old_key_shifts & act_shifts) == old_key_shifts)
        {
            old_key_shifts = act_shifts;
        }
        // Otherwise some of the shifts were released, then run key combo action
        // and set KEY_COMBO_FIRED flag to prevent multiple execution
        else if (old_key_shifts)
        {
            // Toggle mouse lock on Ctrl + Alt
            if (old_key_shifts == (KB_ALT_FLAG | KB_CTRL_FLAG))
                toggle_mouse_lock();
            old_key_shifts |= KEY_SHIFTS_FIRED;
        }
    }
    
    if (kbhit_res) {
        // in case they press the finish-recording button, make sure we know
        int was_playing = play.playback;

        int kgn = getch();
        if (kgn==0) kgn=getch()+AGS_EXT_KEY_SHIFT;
        //    if (kgn==367) restart_game();
        //    if (kgn==2) Display("numover: %d character movesped: %d, animspd: %d",numscreenover,playerchar->walkspeed,playerchar->animspeed);
        //    if (kgn==2) CreateTextOverlay(50,60,170,FONT_SPEECH,14,"This is a test screen overlay which shouldn't disappear");
        //    if (kgn==2) { Display("Crashing"); strcpy(NULL, NULL); }
        //    if (kgn == 2) FaceLocation (game.playercharacter, playerchar->x + 1, playerchar->y);
        //if (kgn == 2) SetCharacterIdle (game.playercharacter, 5, 0);
        //if (kgn == 2) Display("Some for?ign text");
        //if (kgn == 2) do_conversation(5);

        // LAlt or RAlt + Enter
        // NOTE: for some reason LAlt + Enter produces same code as F9
        if (act_shifts == KB_ALT_FLAG && ((kgn == 367 && !key[KEY_F9]) || kgn == 13))
        {
            engine_try_switch_windowed_gfxmode();
            return;
        }

        if (kgn == play.replay_hotkey) {
            // start/stop recording
            if (play.recording)
                stop_recording();
            else if ((play.playback) || (was_playing))
                ;  // do nothing (we got the replay of the stop key)
            else
                replay_start_this_time = 1;
        }

        check_skip_cutscene_keypress (kgn);

        if (play.fast_forward) { }
        else if (pl_run_plugin_hooks(AGSE_KEYPRESS, kgn)) {
            // plugin took the keypress
            debug_script_log("Keypress code %d taken by plugin", kgn);
        }
        else if ((kgn == '`') && (play.debug_mode > 0)) {
            // debug console
            display_console = !display_console;
        }
        else if ((is_text_overlay > 0) &&
            (play.cant_skip_speech & SKIP_KEYPRESS) &&
            (kgn != 434)) {
                // 434 = F12, allow through for screenshot of text
                // (though atm with one script at a time that won't work)
                // only allow a key to remove the overlay if the icon bar isn't up
                if (IsGamePaused() == 0) {
                    // check if it requires a specific keypress
                    if ((play.skip_speech_specific_key > 0) &&
                        (kgn != play.skip_speech_specific_key)) { }
                    else
                        remove_screen_overlay(OVER_TEXTMSG);
                }
        }
        else if ((play.wait_counter > 0) && (play.key_skip_wait > 0)) {
            play.wait_counter = -1;
            debug_script_log("Keypress code %d ignored - in Wait", kgn);
        }
        else if ((kgn == 5) && (display_fps == 2)) {
            // if --fps paramter is used, Ctrl+E will max out frame rate
            SetGameSpeed(1000);
            display_fps = 2;
        }
        else if ((kgn == 4) && (play.debug_mode > 0)) {
            // ctrl+D - show info
            char infobuf[900];
            int ff;
            // MACPORT FIX 9/6/5: added last %s
            sprintf(infobuf,"In room %d %s[Player at %d, %d (view %d, loop %d, frame %d)%s%s%s",
                displayed_room, (noWalkBehindsAtAll ? "(has no walk-behinds)" : ""), playerchar->x,playerchar->y,
                playerchar->view + 1, playerchar->loop,playerchar->frame,
                (IsGamePaused() == 0) ? "" : "[Game paused.",
                (play.ground_level_areas_disabled == 0) ? "" : "[Ground areas disabled.",
                (IsInterfaceEnabled() == 0) ? "[Game in Wait state" : "");
            for (ff=0;ff<croom->numobj;ff++) {
                if (ff >= 8) break; // buffer not big enough for more than 7
                sprintf(&infobuf[strlen(infobuf)],
                    "[Object %d: (%d,%d) size (%d x %d) on:%d moving:%s animating:%d slot:%d trnsp:%d clkble:%d",
                    ff, objs[ff].x, objs[ff].y,
                    (spriteset[objs[ff].num] != NULL) ? spritewidth[objs[ff].num] : 0,
                    (spriteset[objs[ff].num] != NULL) ? spriteheight[objs[ff].num] : 0,
                    objs[ff].on,
                    (objs[ff].moving > 0) ? "yes" : "no", objs[ff].cycling,
                    objs[ff].num, objs[ff].transparent,
                    ((objs[ff].flags & OBJF_NOINTERACT) != 0) ? 0 : 1 );
            }
            Display(infobuf);
            int chd = game.playercharacter;
            char bigbuffer[STD_BUFFER_SIZE] = "CHARACTERS IN THIS ROOM:[";
            for (ff = 0; ff < game.numcharacters; ff++) {
                if (game.chars[ff].room != displayed_room) continue;
                if (strlen(bigbuffer) > 430) {
                    strcat(bigbuffer, "and more...");
                    Display(bigbuffer);
                    strcpy(bigbuffer, "CHARACTERS IN THIS ROOM (cont'd):[");
                }
                chd = ff;
                sprintf(&bigbuffer[strlen(bigbuffer)], 
                    "%s (view/loop/frm:%d,%d,%d  x/y/z:%d,%d,%d  idleview:%d,time:%d,left:%d walk:%d anim:%d follow:%d flags:%X wait:%d zoom:%d)[",
                    game.chars[chd].scrname, game.chars[chd].view+1, game.chars[chd].loop, game.chars[chd].frame,
                    game.chars[chd].x, game.chars[chd].y, game.chars[chd].z,
                    game.chars[chd].idleview, game.chars[chd].idletime, game.chars[chd].idleleft,
                    game.chars[chd].walking, game.chars[chd].animating, game.chars[chd].following,
                    game.chars[chd].flags, game.chars[chd].wait, charextra[chd].zoom);
            }
            Display(bigbuffer);

        }
        /*    else if (kgn == 21) {
        play.debug_mode++;
        script_debug(5,0);
        play.debug_mode--;
        }*/
        else if ((kgn == 22 + AGS_EXT_KEY_SHIFT && (key[KEY_LCONTROL] || key[KEY_RCONTROL]) ) &&
            (play.wait_counter < 1) && (is_text_overlay == 0) && (restrict_until == 0))
        {
            // make sure we can't interrupt a Wait()
            // and desync the music to cutscene
            play.debug_mode++;
            script_debug (1,0);
            play.debug_mode--;
        }
        else if (inside_script) {
            // Don't queue up another keypress if it can't be run instantly
            debug_script_log("Keypress %d ignored (game blocked)", kgn);
        }
        else {
            int keywasprocessed = 0;
            // determine if a GUI Text Box should steal the click
            // it should do if a displayable character (32-255) is
            // pressed, but exclude control characters (<32) and
            // extended keys (eg. up/down arrow; 256+)
            if ( ((kgn >= 32) && (kgn != '[') && (kgn < 256)) || (kgn == 13) || (kgn == 8) ) {
                int uu,ww;
                for (uu=0;uu<game.numgui;uu++) {
                    if (!guis[uu].IsVisible()) continue;
                    for (ww=0;ww<guis[uu].ControlCount;ww++) {
                        // not a text box, ignore it
                        if ((guis[uu].CtrlRefs[ww] >> 16)!=kGUITextBox)
                            continue;
                        GUITextBox*guitex=(GUITextBox*)guis[uu].Controls[ww];
                        // if the text box is disabled, it cannot except keypresses
                        if ((guitex->IsDisabled()) || (!guitex->IsVisible()))
                            continue;
                        guitex->KeyPress(kgn);
                        if (guitex->activated) {
                            guitex->activated = 0;
                            setevent(EV_IFACECLICK, uu, ww, 1);
                        }
                        keywasprocessed = 1;
                    }
                }
            }
            if (!keywasprocessed) {
                kgn = GetKeyForKeyPressCb(kgn);
                debug_script_log("Running on_key_press keycode %d", kgn);
                setevent(EV_TEXTSCRIPT,TS_KEYPRESS,kgn);
            }
        }
        //    RunTextScriptIParam(gameinst,"on_key_press",kgn);
    }
}  // end check_controls

void check_room_edges(int numevents_was)
{
    if ((IsInterfaceEnabled()) && (IsGamePaused() == 0) &&
        (in_new_room == 0) && (new_room_was == 0)) {
            // Only allow walking off edges if not in wait mode, and
            // if not in Player Enters Screen (allow walking in from off-screen)
            int edgesActivated[4] = {0, 0, 0, 0};
            // Only do it if nothing else has happened (eg. mouseclick)
            if ((numevents == numevents_was) &&
                ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0)) {

                    if (playerchar->x <= thisroom.left)
                        edgesActivated[0] = 1;
                    else if (playerchar->x >= thisroom.right)
                        edgesActivated[1] = 1;
                    if (playerchar->y >= thisroom.bottom)
                        edgesActivated[2] = 1;
                    else if (playerchar->y <= thisroom.top)
                        edgesActivated[3] = 1;

                    if ((play.entered_edge >= 0) && (play.entered_edge <= 3)) {
                        // once the player is no longer outside the edge, forget the stored edge
                        if (edgesActivated[play.entered_edge] == 0)
                            play.entered_edge = -10;
                        // if we are walking in from off-screen, don't activate edges
                        else
                            edgesActivated[play.entered_edge] = 0;
                    }

                    for (int ii = 0; ii < 4; ii++) {
                        if (edgesActivated[ii])
                            setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, ii);
                    }
            }
    }
    our_eip = 1008;

}

void game_loop_check_controls(bool checkControls)
{
    // don't let the player do anything before the screen fades in
    if ((in_new_room == 0) && (checkControls)) {
        int inRoom = displayed_room;
        int numevents_was = numevents;
        check_controls();
        check_room_edges(numevents_was);
        // If an inventory interaction changed the room
        if (inRoom != displayed_room)
            check_new_room();
    }
}

void game_loop_do_update()
{
    if (debug_flags & DBG_NOUPDATE) ;
    else if (game_paused==0) update_stuff();
}

void game_loop_update_animated_buttons()
{
    // update animating GUI buttons
    // this bit isn't in update_stuff because it always needs to
    // happen, even when the game is paused
    for (int aa = 0; aa < numAnimButs; aa++) {
        if (UpdateAnimatingButton(aa)) {
            StopButtonAnimation(aa);
            aa--;
        } 
    }
}

void game_loop_do_render_and_check_mouse(IDriverDependantBitmap *extraBitmap, int extraX, int extraY)
// [IKM] ...and some coffee, please :)
{
    if (!play.fast_forward) {
        int mwasatx=mousex,mwasaty=mousey;

        // Only do this if we are not skipping a cutscene
        render_graphics(extraBitmap, extraX, extraY);

        // Check Mouse Moves Over Hotspot event
        static int offsetxWas = -100, offsetyWas = -100;

        if (((mwasatx!=mousex) || (mwasaty!=mousey) ||
            (offsetxWas != offsetx) || (offsetyWas != offsety)) &&
            (displayed_room >= 0)) 
        {
            // mouse moves over hotspot
            if (__GetLocationType(divide_down_coordinate(mousex), divide_down_coordinate(mousey), 1) == LOCTYPE_HOTSPOT) {
                int onhs = getloctype_index;

                setevent(EV_RUNEVBLOCK,EVB_HOTSPOT,onhs,6); 
            }
        }

        offsetxWas = offsetx;
        offsetyWas = offsety;

#ifdef MAC_VERSION
        // take a breather after the heavy work
        // cuts down on CPU usage and reduces the fan noise
        rest(2);
#endif
    }
}

void game_loop_update_events()
{
    new_room_was = in_new_room;
    if (in_new_room>0)
        setevent(EV_FADEIN,0,0,0);
    in_new_room=0;
    update_events();
    if ((new_room_was > 0) && (in_new_room == 0)) {
        // if in a new room, and the room wasn't just changed again in update_events,
        // then queue the Enters Screen scripts
        // run these next time round, when it's faded in
        if (new_room_was==2)  // first time enters screen
            setevent(EV_RUNEVBLOCK,EVB_ROOM,0,4);
        if (new_room_was!=3)   // enters screen after fadein
            setevent(EV_RUNEVBLOCK,EVB_ROOM,0,7);
    }
}

void game_loop_update_background_animation()
{
    if (play.bg_anim_delay > 0) play.bg_anim_delay--;
    else if (play.bg_frame_locked) ;
    else {
        play.bg_anim_delay = play.anim_background_speed;
        play.bg_frame++;
        if (play.bg_frame >= thisroom.num_bscenes)
            play.bg_frame=0;
        if (thisroom.num_bscenes >= 2) {
            // get the new frame's palette
            on_background_frame_change();
        }
    }
}

void game_loop_update_loop_counter()
{
    loopcounter++;

    if (play.wait_counter > 0) play.wait_counter--;
    if (play.shakesc_length > 0) play.shakesc_length--;

    if (loopcounter % 5 == 0)
    {
        update_ambient_sound_vol();
        update_directional_sound_vol();
    }
}

void game_loop_check_replay_record()
{
    if (replay_start_this_time) {
        replay_start_this_time = 0;
        start_replay_record();
    }
}

void game_loop_update_fps()
{
    if (time(NULL) != t1) {
        t1 = time(NULL);
        fps = loopcounter - lastcounter;
        lastcounter = loopcounter;
    }
}

void PollUntilNextFrame()
{
    // make sure we poll, cos a low framerate (eg 5 fps) could stutter
    // mp3 music
    while (timerloop == 0 && play.fast_forward == 0) {
        update_polled_stuff_if_runtime();
        platform->YieldCPU();
    }
}

void UpdateGameOnce(bool checkControls, IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {

    int res;

    update_mp3();

    numEventsAtStartOfFunction = numevents;

    if (want_exit) {
        ProperExit();
    }

    ccNotifyScriptStillAlive ();
    our_eip=1;
    timerloop=0;

    game_loop_check_problems_at_start();

    // if we're not fading in, don't count the fadeouts
    if ((play.no_hicolor_fadein) && (game.options[OPT_FADETYPE] == FADE_NORMAL))
        play.screen_is_faded_out = 0;

    our_eip = 1014;

    update_gui_disabled_status();

    our_eip = 1004;

    game_loop_check_new_room();

    our_eip = 1005;

    res = game_loop_check_ground_level_interactions();
    if (res != RETURN_CONTINUE) {
        return;
    }

    mouse_on_iface=-1;

    check_debug_keys();

    game_loop_check_controls(checkControls);

    our_eip=2;

    game_loop_do_update();

    game_loop_update_animated_buttons();

    game_loop_do_late_update();

    update_polled_audio_and_crossfade();

    game_loop_do_render_and_check_mouse(extraBitmap, extraX, extraY);

    our_eip=6;

    game_loop_update_events();

    our_eip=7;

    //    if (mgetbutton()>NONE) break;
    update_polled_stuff_if_runtime();

    game_loop_update_background_animation();

    game_loop_update_loop_counter();

    game_loop_check_replay_record();

    // Immediately start the next frame if we are skipping a cutscene
    if (play.fast_forward)
        return;

    our_eip=72;

    game_loop_update_fps();

    PollUntilNextFrame();
}

void UpdateMouseOverLocation()
{
    // Call GetLocationName - it will internally force a GUI refresh
    // if the result it returns has changed from last time
    char tempo[STD_BUFFER_SIZE];
    GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);

    if ((play.get_loc_name_save_cursor >= 0) &&
        (play.get_loc_name_save_cursor != play.get_loc_name_last_time) &&
        (mouse_on_iface < 0) && (ifacepopped < 0)) {
            // we have saved the cursor, but the mouse location has changed
            // and it's time to restore it
            play.get_loc_name_save_cursor = -1;
            set_cursor_mode(play.restore_cursor_mode_to);

            if (cur_mode == play.restore_cursor_mode_to)
            {
                // make sure it changed -- the new mode might have been disabled
                // in which case don't change the image
                set_mouse_cursor(play.restore_cursor_image_to);
            }
            debug_script_log("Restore mouse to mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
    }
}

// Checks if user interface should remain disabled for now
int ShouldStayInWaitMode() {
    if (restrict_until == 0)
        quit("end_wait_loop called but game not in loop_until state");
    int retval = restrict_until;

    if (restrict_until==UNTIL_MOVEEND) {
        short*wkptr=(short*)user_disabled_data;
        if (wkptr[0]<1) retval=0;
    }
    else if (restrict_until==UNTIL_CHARIS0) {
        char*chptr=(char*)user_disabled_data;
        if (chptr[0]==0) retval=0;
    }
    else if (restrict_until==UNTIL_NEGATIVE) {
        short*wkptr=(short*)user_disabled_data;
        if (wkptr[0]<0) retval=0;
    }
    else if (restrict_until==UNTIL_INTISNEG) {
        int*wkptr=(int*)user_disabled_data;
        if (wkptr[0]<0) retval=0;
    }
    else if (restrict_until==UNTIL_NOOVERLAY) {
        if (is_text_overlay < 1) retval=0;
    }
    else if (restrict_until==UNTIL_INTIS0) {
        int*wkptr=(int*)user_disabled_data;
        if (wkptr[0]<0) retval=0;
    }
    else if (restrict_until==UNTIL_SHORTIS0) {
        short*wkptr=(short*)user_disabled_data;
        if (wkptr[0]==0) retval=0;
    }
    else quit("loop_until: unknown until event");

    return retval;
}

int UpdateWaitMode()
{
    if (restrict_until==0) ;
    else {
        restrict_until = ShouldStayInWaitMode();
        our_eip = 77;

        if (restrict_until==0) {
            set_default_cursor();
            guis_need_update = 1;
            play.disabled_user_interface--;
            /*      if (user_disabled_for==FOR_ANIMATION)
            run_animation((FullAnimation*)user_disabled_data2,user_disabled_data3);
            else*/ if (user_disabled_for==FOR_EXITLOOP) {
                user_disabled_for=0; return -1; }
            else if (user_disabled_for==FOR_SCRIPT) {
                quit("err: for_script obsolete (v2.1 and earlier only)");
            }
            else
                quit("Unknown user_disabled_for in end restrict_until");

            user_disabled_for=0;
        }
    }

    return RETURN_CONTINUE;
}

// Run single game iteration; calls UpdateGameOnce() internally
int GameTick()
{
    if (displayed_room < 0)
        quit("!A blocking function was called before the first room has been loaded");

    UpdateGameOnce(true);
    UpdateMouseOverLocation();

    our_eip=76;

    int res = UpdateWaitMode();
    if (res != RETURN_CONTINUE) {
        return res;
    }

    our_eip = 78;
    return 0;
}

void SetupLoopParameters(int untilwhat,long udata,int mousestuff) {
    play.disabled_user_interface++;
    guis_need_update = 1;
    // Only change the mouse cursor if it hasn't been specifically changed first
    // (or if it's speech, always change it)
    if (((cur_cursor == cur_mode) || (untilwhat == UNTIL_NOOVERLAY)) &&
        (cur_mode != CURS_WAIT))
        set_mouse_cursor(CURS_WAIT);

    restrict_until=untilwhat;
    user_disabled_data=udata;
    user_disabled_for=FOR_EXITLOOP;
    return;
}

// This function is called from lot of various functions
// in the game core, character, room object etc
void GameLoopUntilEvent(int untilwhat,long daaa) {
  // blocking cutscene - end skipping
  EndSkippingUntilCharStops();

  // this function can get called in a nested context, so
  // remember the state of these vars in case a higher level
  // call needs them
  int cached_restrict_until = restrict_until;
  int cached_user_disabled_data = user_disabled_data;
  int cached_user_disabled_for = user_disabled_for;

  SetupLoopParameters(untilwhat,daaa,0);
  while (GameTick()==0) ;

  restrict_until = cached_restrict_until;
  user_disabled_data = cached_user_disabled_data;
  user_disabled_for = cached_user_disabled_for;
}

// for external modules to call
void NextIteration() {
    NEXT_ITERATION();
}

extern unsigned int load_new_game;
void RunGameUntilAborted()
{
    while (!abort_engine) {
        GameTick();

        if (load_new_game) {
            RunAGSGame (NULL, load_new_game, 0);
            load_new_game = 0;
        }
    }
}

void update_polled_stuff_if_runtime()
{
    if (want_exit) {
        want_exit = 0;
        quit("||exit!");
    }

    if (!psp_audio_multithreaded)
        update_polled_mp3();

    if (editor_debugging_initialized)
        check_for_messages_from_editor();
}

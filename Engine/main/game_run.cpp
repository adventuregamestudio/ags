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
#include "ac/global_character.h"
#include "ac/global_debug.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_region.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/record.h"
#include "ac/room.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "main/mainheader.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/soundclip.h"
#include "plugin/agsplugin.h"
#include "script/script.h"
#include "ac/spritecache.h"

using AGS::Common::GuiTextBox;

extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;
extern int mouse_on_iface;   // mouse cursor is over this interface
extern int ifacepopped;
extern int is_text_overlay;
extern volatile char want_exit, abort_engine;
extern int want_quit;
extern int proper_exit,our_eip;
extern int displayed_room, starting_room, in_new_room, new_room_was;
extern int game_paused;
extern int getloctype_index;
extern int in_enters_screen,done_es_error;
extern int in_leaves_screen;
extern int inside_script,in_graph_script;
extern int no_blocking_functions;
extern CharacterInfo*playerchar;
extern int is_complete_overlay;
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern int cur_mode;
extern int replay_start_this_time;
extern char noWalkBehindsAtAll;
extern CharacterExtras *charextra;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int offsetx, offsety;
extern unsigned int loopcounter,lastcounter;
extern volatile int timerloop;
extern int cur_mode,cur_cursor;


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

void ProperQuit()
{
    want_quit = 0;
    QuitGame(1);
}

void game_loop_check_problems_at_start()
{
    if ((in_enters_screen != 0) & (displayed_room == starting_room))
        quit("!A text script run in the Player Enters Screen event caused the\n"
        "screen to be updated. If you need to use Wait(), do so in After Fadein");
    if ((in_enters_screen != 0) && (done_es_error == 0)) {
        debug_log("Wait() was used in Player Enters Screen - use Enters Screen After Fadein instead");
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

int game_loop_check_ground_level_interactions()
{
    if ((play.GroundLevelAreasDisabled & GLED_INTERACTION) == 0) {
        // check if he's standing on a hotspot
        int hotspotThere = get_hotspot_at(playerchar->x, playerchar->y);
        // run Stands on Hotspot event
        setevent(EV_RUNEVBLOCK, EVB_HOTSPOT, hotspotThere, 0);

        // check current region
        int onRegion = GetRegionAt (playerchar->x, playerchar->y);
        int inRoom = displayed_room;

        if (onRegion != play.PlayerOnRegionIndex) {
            // we need to save this and set play.PlayerOnRegionIndex
            // now, so it's correct going into RunRegionInteraction
            int oldRegion = play.PlayerOnRegionIndex;

            play.PlayerOnRegionIndex = onRegion;
            // Walks Off last region
            if (oldRegion > 0)
                RunRegionInteraction (oldRegion, 2);
            // Walks Onto new region
            if (onRegion > 0)
                RunRegionInteraction (onRegion, 1);
        }
        if (play.PlayerOnRegionIndex > 0)   // player stands on region
            RunRegionInteraction (play.PlayerOnRegionIndex, 0);

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

// check_controls: checks mouse & keyboard interface
void check_controls() {
    int numevents_was = numevents;
    our_eip = 1007;
    NEXT_ITERATION();

    int aa,mongu=-1;
    // If all GUIs are off, skip the loop
    if ((game.Options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0)) ;
    else {
        // Scan for mouse-y-pos GUIs, and pop one up if appropriate
        // Also work out the mouse-over GUI while we're at it
        int ll;
        for (ll = 0; ll < game.GuiCount;ll++) {
            aa = play.GuiDrawOrder[ll];
            if (guis[aa].IsMouseOnGui()) mongu=aa;

            if (guis[aa].PopupStyle!=Common::kGuiPopupMouseY) continue;
            if (is_complete_overlay>0) break;  // interfaces disabled
            //    if (play.DisabledUserInterface>0) break;
            if (ifacepopped==aa) continue;
            if (guis[aa].IsConcealed()) continue;
            // Don't allow it to be popped up while skipping cutscene
            if (play.FastForwardCutscene) continue;

            if (mousey < guis[aa].PopupAtMouseY) {
                set_mouse_cursor(CURS_ARROW);
                guis[aa].SetVisibility(Common::kGuiVisibility_On); guis_need_update = 1;
                ifacepopped=aa; PauseGame();
                break;
            }
        }
    }

    mouse_on_iface=mongu;
    if ((ifacepopped>=0) && (mousey>=guis[ifacepopped].GetY()+guis[ifacepopped].GetHeight()))
        remove_popup_interface(ifacepopped);

    // check mouse clicks on GUIs
    static int wasbutdown=0,wasongui=0;

    if ((wasbutdown>0) && (misbuttondown(wasbutdown-1))) {
        for (aa=0;aa<guis[wasongui].ControlCount;aa++) {
            if (!guis[wasongui].Controls[aa]->IsActivated) continue;
            if (guis[wasongui].GetControlType(aa)!=Common::kGuiSlider) continue;
            // GUI Slider repeatedly activates while being dragged
            guis[wasongui].Controls[aa]->IsActivated=0;
            setevent(EV_IFACECLICK, wasongui, aa, wasbutdown);
            break;
        }
    }
    else if ((wasbutdown>0) && (!misbuttondown(wasbutdown-1))) {
        guis[wasongui].OnMouseButtonUp();
        int whichbut=wasbutdown;
        wasbutdown=0;

        for (aa=0;aa<guis[wasongui].ControlCount;aa++) {
            if (!guis[wasongui].Controls[aa]->IsActivated) continue;
            guis[wasongui].Controls[aa]->IsActivated=0;
            if (!IsInterfaceEnabled()) break;

            int cttype=guis[wasongui].GetControlType(aa);
            if ((cttype == Common::kGuiButton) || (cttype == Common::kGuiSlider) || (cttype == Common::kGuiListBox)) {
                setevent(EV_IFACECLICK, wasongui, aa, whichbut);
            }
            else if (cttype == Common::kGuiInvWindow) {
                mouse_ifacebut_xoffs=mousex-(guis[wasongui].Controls[aa]->GetX())-guis[wasongui].GetX();
                mouse_ifacebut_yoffs=mousey-(guis[wasongui].Controls[aa]->GetY())-guis[wasongui].GetY();
                int iit=offset_over_inv((GuiInvWindow*)guis[wasongui].Controls[aa]);
                if (iit>=0) {
                    evblocknum=iit;
                    play.ClickedInvItemIndex = iit;
                    if (game.Options[OPT_HANDLEINVCLICKS]) {
                        // Let the script handle the click
                        // LEFTINV is 5, RIGHTINV is 6
                        setevent(EV_TEXTSCRIPT,TS_MCLICK, whichbut + 4);
                    }
                    else if (whichbut==2)  // right-click is always Look
                        run_event_block_inv(iit, 0);
                    else if (cur_mode == MODE_HAND)
                        SetActiveInventory(iit);
                    else
                        RunInventoryInteraction (iit, cur_mode);
                    evblocknum=-1;
                }
            }
            else quit("clicked on unknown control type");
            if (guis[wasongui].PopupStyle==Common::kGuiPopupMouseY)
                remove_popup_interface(wasongui);
            break;
        }

        run_on_event(GE_GUI_MOUSEUP, RuntimeScriptValue().SetInt32(wasongui));
    }

    aa=mgetbutton();
    if (aa>NONE) {
        if ((play.IsInCutscene == 3) || (play.IsInCutscene == 4))
            start_skipping_cutscene();
        if ((play.IsInCutscene == 5) && (aa == RIGHT))
            start_skipping_cutscene();

        if (play.FastForwardCutscene) { }
        else if ((play.WaitCounter > 0) && (play.SkipWaitMode > 1))
            play.WaitCounter = -1;
        else if (is_text_overlay > 0) {
            if (play.SkipSpeechMode & SKIP_MOUSECLICK)
                remove_screen_overlay(OVER_TEXTMSG);
        }
        else if (!IsInterfaceEnabled()) ;  // blocking cutscene, ignore mouse
        else if (platform->RunPluginHooks(AGSE_MOUSECLICK, aa+1)) {
            // plugin took the click
            DEBUG_CONSOLE("Plugin handled mouse button %d", aa+1);
        }
        else if (mongu>=0) {
            if (wasbutdown==0) {
                DEBUG_CONSOLE("Mouse click over GUI %d", mongu);
                guis[mongu].OnMouseButtonDown();
                // run GUI click handler if not on any control
                if ((guis[mongu].MouseDownControl < 0) && (guis[mongu].OnClickHandler[0] != 0))
                    setevent(EV_IFACECLICK, mongu, -1, aa + 1);

                run_on_event(GE_GUI_MOUSEDOWN, RuntimeScriptValue().SetInt32(mongu));
            }
            wasongui=mongu;
            wasbutdown=aa+1;
        }
        else setevent(EV_TEXTSCRIPT,TS_MCLICK,aa+1);
        //    else RunTextScriptIParam(gameinst,"on_mouse_click",aa+1);
    }
    aa = check_mouse_wheel();
    if (aa < 0)
        setevent (EV_TEXTSCRIPT, TS_MCLICK, 9);
    else if (aa > 0)
        setevent (EV_TEXTSCRIPT, TS_MCLICK, 8);

    // check keypresses
    if (kbhit()) {
        // in case they press the finish-recording button, make sure we know
        int was_playing = play.IsPlayback;

        int kgn = getch();
        if (kgn==0) kgn=getch()+300;
        //    if (kgn==367) restart_game();
        //    if (kgn==2) Display("numover: %d character movesped: %d, animspd: %d",numscreenover,playerchar->walkspeed,playerchar->animspeed);
        //    if (kgn==2) CreateTextOverlay(50,60,170,FONT_SPEECH,14,"This is a test screen overlay which shouldn't disappear");
        //    if (kgn==2) { Display("Crashing"); strcpy(NULL, NULL); }
        //    if (kgn == 2) FaceLocation (game.PlayerCharacterIndex, playerchar->x + 1, playerchar->y);
        //if (kgn == 2) SetCharacterIdle (game.PlayerCharacterIndex, 5, 0);
        //if (kgn == 2) Display("Some for?ign text");
        //if (kgn == 2) do_conversation(5);

        if (kgn == play.ReplayHotkey) {
            // start/stop recording
            if (play.IsRecording)
                stop_recording();
            else if ((play.IsPlayback) || (was_playing))
                ;  // do nothing (we got the replay of the stop key)
            else
                replay_start_this_time = 1;
        }

        check_skip_cutscene_keypress (kgn);

        if (play.FastForwardCutscene) { }
        else if (platform->RunPluginHooks(AGSE_KEYPRESS, kgn)) {
            // plugin took the keypress
            DEBUG_CONSOLE("Keypress code %d taken by plugin", kgn);
        }
        else if ((kgn == '`') && (play.DebugMode > 0)) {
            // debug console
            display_console = !display_console;
        }
        else if ((is_text_overlay > 0) &&
            (play.SkipSpeechMode & SKIP_KEYPRESS) &&
            (kgn != 434)) {
                // 434 = F12, allow through for screenshot of text
                // (though atm with one script at a time that won't work)
                // only allow a key to remove the overlay if the icon bar isn't up
                if (IsGamePaused() == 0) {
                    // check if it requires a specific keypress
                    if ((play.SpeechSkipKey > 0) &&
                        (kgn != play.SpeechSkipKey)) { }
                    else
                        remove_screen_overlay(OVER_TEXTMSG);
                }
        }
        else if ((play.WaitCounter > 0) && (play.SkipWaitMode > 0)) {
            play.WaitCounter = -1;
            DEBUG_CONSOLE("Keypress code %d ignored - in Wait", kgn);
        }
        else if ((kgn == 5) && (display_fps == 2)) {
            // if --fps paramter is used, Ctrl+E will max out frame rate
            SetGameSpeed(1000);
            display_fps = 2;
        }
        else if ((kgn == 4) && (play.DebugMode > 0)) {
            // ctrl+D - show info
            char infobuf[900];
            int ff;
            // MACPORT FIX 9/6/5: added last %s
            sprintf(infobuf,"In room %d %s[Player at %d, %d (view %d, loop %d, frame %d)%s%s%s",
                displayed_room, (noWalkBehindsAtAll ? "(has no walk-behinds)" : ""), playerchar->x,playerchar->y,
                playerchar->view + 1, playerchar->loop,playerchar->frame,
                (IsGamePaused() == 0) ? "" : "[Game paused.",
                (play.GroundLevelAreasDisabled == 0) ? "" : "[Ground areas disabled.",
                (IsInterfaceEnabled() == 0) ? "[Game in Wait state" : "");
            for (ff=0;ff<croom->ObjectCount;ff++) {
                if (ff >= 8) break; // buffer not big enough for more than 7
                sprintf(&infobuf[strlen(infobuf)],
                    "[Object %d: (%d,%d) size (%d x %d) on:%d moving:%s animating:%d slot:%d trnsp:%d clkble:%d",
                    ff, objs[ff].X, objs[ff].Y,
                    (spriteset[objs[ff].SpriteIndex] != NULL) ? spritewidth[objs[ff].SpriteIndex] : 0,
                    (spriteset[objs[ff].SpriteIndex] != NULL) ? spriteheight[objs[ff].SpriteIndex] : 0,
                    objs[ff].IsOn,
                    (objs[ff].Moving > 0) ? "yes" : "no", objs[ff].Cycling,
                    objs[ff].SpriteIndex, objs[ff].Transparency,
                    ((objs[ff].Flags & OBJF_NOINTERACT) != 0) ? 0 : 1 );
            }
            Display(infobuf);
            int chd = game.PlayerCharacterIndex;
            char bigbuffer[STD_BUFFER_SIZE] = "CHARACTERS IN THIS ROOM:[";
            for (ff = 0; ff < game.CharacterCount; ff++) {
                if (game.Characters[ff].room != displayed_room) continue;
                if (strlen(bigbuffer) > 430) {
                    strcat(bigbuffer, "and more...");
                    Display(bigbuffer);
                    strcpy(bigbuffer, "CHARACTERS IN THIS ROOM (cont'd):[");
                }
                chd = ff;
                sprintf(&bigbuffer[strlen(bigbuffer)], 
                    "%s (view/loop/frm:%d,%d,%d  x/y/z:%d,%d,%d  idleview:%d,time:%d,left:%d walk:%d anim:%d follow:%d flags:%X wait:%d zoom:%d)[",
                    game.Characters[chd].scrname, game.Characters[chd].view+1, game.Characters[chd].loop, game.Characters[chd].frame,
                    game.Characters[chd].x, game.Characters[chd].y, game.Characters[chd].z,
                    game.Characters[chd].idleview, game.Characters[chd].idletime, game.Characters[chd].idleleft,
                    game.Characters[chd].walking, game.Characters[chd].animating, game.Characters[chd].following,
                    game.Characters[chd].flags, game.Characters[chd].wait, charextra[chd].zoom);
            }
            Display(bigbuffer);

        }
        /*    else if (kgn == 21) {
        play.DebugMode++;
        script_debug(5,0);
        play.DebugMode--;
        }*/
        else if ((kgn == 22) && (play.WaitCounter < 1) && (is_text_overlay == 0) && (restrict_until == 0)) {
            // make sure we can't interrupt a Wait()
            // and desync the music to cutscene
            play.DebugMode++;
            script_debug (1,0);
            play.DebugMode--;
        }
        else if (inside_script) {
            // Don't queue up another keypress if it can't be run instantly
            DEBUG_CONSOLE("Keypress %d ignored (game blocked)", kgn);
        }
        else {
            int keywasprocessed = 0;
            // determine if a GUI Text Box should steal the click
            // it should do if a displayable character (32-255) is
            // pressed, but exclude control characters (<32) and
            // extended keys (eg. up/down arrow; 256+)
            if ( ((kgn >= 32) && (kgn != '[') && (kgn < 256)) || (kgn == 13) || (kgn == 8) ) {
                int uu,ww;
                for (uu=0;uu<game.GuiCount;uu++) {
                    if (!guis[uu].IsVisible()) continue;
                    for (ww=0;ww<guis[uu].ControlCount;ww++) {
                        // not a text box, ignore it
                        if ((guis[uu].ControlRefs[ww] >> 16)!=Common::kGuiTextBox)
                            continue;
                        GuiTextBox*guitex=(GuiTextBox*)guis[uu].Controls[ww];
                        // if the text box is disabled, it cannot except keypresses
                        if ((guitex->IsDisabled()) || (!guitex->IsVisible()))
                            continue;
                        guitex->OnKeyPress(kgn);
                        if (guitex->IsActivated) {
                            guitex->IsActivated = 0;
                            setevent(EV_IFACECLICK, uu, ww, 1);
                        }
                        keywasprocessed = 1;
                    }
                }
            }
            if (!keywasprocessed) {
                if ((kgn>='a') & (kgn<='z')) kgn-=32;
                DEBUG_CONSOLE("Running on_key_press keycode %d", kgn);
                setevent(EV_TEXTSCRIPT,TS_KEYPRESS,kgn);
            }
        }
        //    RunTextScriptIParam(gameinst,"on_key_press",kgn);
    }

    if ((IsInterfaceEnabled()) && (IsGamePaused() == 0) &&
        (in_new_room == 0) && (new_room_was == 0)) {
            // Only allow walking off edges if not in wait mode, and
            // if not in Player Enters Screen (allow walking in from off-screen)
            int edgesActivated[4] = {0, 0, 0, 0};
            // Only do it if nothing else has happened (eg. mouseclick)
            if ((numevents == numevents_was) &&
                ((play.GroundLevelAreasDisabled & GLED_INTERACTION) == 0)) {

                    if (playerchar->x <= thisroom.Edges.Left)
                        edgesActivated[0] = 1;
                    else if (playerchar->x >= thisroom.Edges.Right)
                        edgesActivated[1] = 1;
                    if (playerchar->y >= thisroom.Edges.Bottom)
                        edgesActivated[2] = 1;
                    else if (playerchar->y <= thisroom.Edges.Top)
                        edgesActivated[3] = 1;

                    if ((play.CharacterEnterRoomAtEdge >= 0) && (play.CharacterEnterRoomAtEdge <= 3)) {
                        // once the player is no longer outside the edge, forget the stored edge
                        if (edgesActivated[play.CharacterEnterRoomAtEdge] == 0)
                            play.CharacterEnterRoomAtEdge = -10;
                        // if we are walking in from off-screen, don't activate edges
                        else
                            edgesActivated[play.CharacterEnterRoomAtEdge] = 0;
                    }

                    for (int ii = 0; ii < 4; ii++) {
                        if (edgesActivated[ii])
                            setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, ii);
                    }
            }
    }
    our_eip = 1008;

}  // end check_controls

void game_loop_check_controls(bool checkControls)
{
    // don't let the player do anything before the screen fades in
    if ((in_new_room == 0) && (checkControls)) {
        int inRoom = displayed_room;
        check_controls();
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
    if (!play.FastForwardCutscene) {
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
    if (play.RoomBkgAnimDelay > 0) play.RoomBkgAnimDelay--;
    else if (play.RoomBkgFrameLocked) ;
    else {
        play.RoomBkgAnimDelay = play.RoomBkgAnimSpeed;
        play.RoomBkgFrameIndex++;
        if (play.RoomBkgFrameIndex >= thisroom.BkgSceneCount)
            play.RoomBkgFrameIndex=0;
        if (thisroom.BkgSceneCount >= 2) {
            // get the new frame's palette
            on_background_frame_change();
        }
    }
}

void game_loop_update_loop_counter()
{
    loopcounter++;

    if (play.WaitCounter > 0) play.WaitCounter--;
    if (play.ShakeScreenLength > 0) play.ShakeScreenLength--;

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

    if (play.FastForwardCutscene)
        return;
}

void game_loop_update_fps()
{
    if (time(NULL) != t1) {
        t1 = time(NULL);
        fps = loopcounter - lastcounter;
        lastcounter = loopcounter;
    }
}

void game_loop_poll_stuff_once_more()
{
    // make sure we poll, cos a low framerate (eg 5 fps) could stutter
    // mp3 music
    while (timerloop == 0) {
        update_polled_stuff_if_runtime();
        platform->YieldCPU();
    }
}

void SetupLoopParameters(int untilwhat,long udata,int mousestuff) {
    play.DisabledUserInterface++;
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

//=============================================================================

void UpdateGameOnce(bool checkControls, IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {

    int res;

    UPDATE_MP3

    numEventsAtStartOfFunction = numevents;

    if (want_exit) {
        ProperExit();
    }

    ccNotifyScriptStillAlive ();
    our_eip=1;
    timerloop=0;

    if (want_quit) {
        ProperQuit();
    }

    game_loop_check_problems_at_start();

    // if we're not fading in, don't count the fadeouts
    if ((play.NoHicolorFadeIn) && (game.Options[OPT_FADETYPE] == FADE_NORMAL))
        play.ScreenIsFadedOut = 0;

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

    update_polled_stuff_and_crossfade();

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
    if (play.FastForwardCutscene)
      return;

    our_eip=72;

    game_loop_update_fps();

    game_loop_poll_stuff_once_more();
}

void UpdateMouseOverLocation()
{
    // Call GetLocationName - it will internally force a GUI refresh
    // if the result it returns has changed from last time
    char tempo[STD_BUFFER_SIZE];
    GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);

    if ((play.GetLocationNameSaveCursor >= 0) &&
        (play.GetLocationNameSaveCursor != play.GetLocationNameLastTime) &&
        (mouse_on_iface < 0) && (ifacepopped < 0)) {
            // we have saved the cursor, but the mouse location has changed
            // and it's time to restore it
            play.GetLocationNameSaveCursor = -1;
            set_cursor_mode(play.RestoreCursorModeTo);

            if (cur_mode == play.RestoreCursorModeTo)
            {
                // make sure it changed -- the new mode might have been disabled
                // in which case don't change the image
                set_mouse_cursor(play.RestoreCursorImageTo);
            }
            DEBUG_CONSOLE("Restore mouse to mode %d cursor %d", play.RestoreCursorModeTo, play.RestoreCursorImageTo);
    }
}

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
            play.DisabledUserInterface--;
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

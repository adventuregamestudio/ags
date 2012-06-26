/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

//
// Game loop
//

#include "ac/hotspot.h"
#include "main/mainheader.h"
#include "main/game_run.h"

int numEventsAtStartOfFunction;
long t1;  // timer for FPS // ... 't1'... how very appropriate.. :)

int user_disabled_for=0,user_disabled_data=0,user_disabled_data2=0;
int user_disabled_data3=0;



void game_loop_check_want_exit()
{
    if (want_exit) {
        want_exit = 0;
        proper_exit = 1;
        quit("||exit!");
        /*#ifdef WINDOWS_VERSION
        // the Quit thread is running now, so exit this main one.
        ExitThread (1);
        #endif*/
    }
}

void game_loop_check_want_quit()
{
    if (want_quit) {
        want_quit = 0;
        QuitGame(1);
    }
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

    // if we're not fading in, don't count the fadeouts
    if ((play.no_hicolor_fadein) && (game.options[OPT_FADETYPE] == FADE_NORMAL))
        play.screen_is_faded_out = 0;
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
        if ((restrict_until) && (!wait_loop_still_valid())) {
            // cancel the Rep Exec and Stands on Hotspot events that
            // we just added -- otherwise the event queue gets huge
            numevents = numEventsAtStartOfFunction;
            return 0;
        }
    } // end if checking ground level interactions

    return RETURN_CONTINUE;
}

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

    if (play.fast_forward)
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

void mainloop(bool checkControls, IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {
    
    int res;

    UPDATE_MP3

    numEventsAtStartOfFunction = numevents;

    game_loop_check_want_exit();

    ccNotifyScriptStillAlive ();
    our_eip=1;
    timerloop=0;

    game_loop_check_problems_at_start();

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

    our_eip=72;

    game_loop_update_fps();

    game_loop_poll_stuff_once_more();
}

void game_loop_process_mouse_over_location()
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
            DEBUG_CONSOLE("Restore mouse to mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
    }
}

int wait_loop_still_valid() {
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

int game_loop_process_wait_until()
{
    if (restrict_until==0) ;
    else {
        restrict_until = wait_loop_still_valid();
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

int main_game_loop() {

    if (displayed_room < 0)
        quit("!A blocking function was called before the first room has been loaded");

    mainloop(true);

    game_loop_process_mouse_over_location();

    our_eip=76;

    int res = game_loop_process_wait_until();
    if (res != RETURN_CONTINUE) {
        return res;
    }
    
    our_eip = 78;
    return 0;
}


void main_loop_until(int untilwhat,int udata,int mousestuff) {
    play.disabled_user_interface++;
    guis_need_update = 1;
    // Only change the mouse cursor if it hasn't been specifically changed first
    // (or if it's speech, always change it)
    if (((cur_cursor == cur_mode) || (untilwhat == UNTIL_NOOVERLAY)) &&
        (cur_mode != CURS_WAIT))
        set_mouse_cursor(CURS_WAIT);

    restrict_until=untilwhat;
    user_disabled_data=udata;
    return;
}

// This function is called from lot of various functions
// in the game core, character, room object etc
void do_main_cycle(int untilwhat,int daaa) {
  // blocking cutscene - end skipping
  EndSkippingUntilCharStops();

  // this function can get called in a nested context, so
  // remember the state of these vars in case a higher level
  // call needs them
  int cached_restrict_until = restrict_until;
  int cached_user_disabled_data = user_disabled_data;
  int cached_user_disabled_for = user_disabled_for;

  main_loop_until(untilwhat,daaa,0);
  user_disabled_for=FOR_EXITLOOP;
  while (main_game_loop()==0) ;

  restrict_until = cached_restrict_until;
  user_disabled_data = cached_user_disabled_data;
  user_disabled_for = cached_user_disabled_for;
}

// for external modules to call
void next_iteration() {
    NEXT_ITERATION();
}

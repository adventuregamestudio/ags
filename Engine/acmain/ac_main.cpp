/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#define USE_CLIB
#include <stdio.h>
#include "wgt2allg.h"
#include "ali3d.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_main.h"
#include "acmain/ac_commonheaders.h"
#include "acmain/ac_controls.h"
#include "acchars/ac_charhelpers.h"
#include "acmain/ac_lipsync.h"
#include "acmain/ac_math.h"
#include "acmain/ac_cdplayer.h"
#include "cs/cs_runtime.h"
#include "acmain/ac_animatingguibutton.h"
#include "acgui/ac_guibutton.h"
#include "acgfx/ac_gfxfilters.h"
#include "acrun/ac_executingscript.h"
#include "acmain/ac_transition.h"
#include "cs/cs_utils.h"
#include "mousew32.h"
#include "acmain/ac_display.h"
#include "acmain/ac_file.h"
#include "routefnd.h"
#include "misc.h"
#include "cs/cc_error.h"
#include "acfont/ac_agsfontrenderer.h"
#include "acmain/ac_display.h"
#include "media/audio/audio.h"
#include "media/audio/soundcache.h"


#ifdef WINDOWS_VERSION
#include <shlwapi.h>
//#include <crtdbg.h>
//#include "winalleg.h"

int wArgc;
LPWSTR *wArgv;
#else
// ???
#endif

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <process.h>
#endif

// **** GLOBALS ****
char *music_file;
char *speech_file;
WCHAR directoryPathBuffer[MAX_PATH];


#if defined(MAC_VERSION) || (defined(LINUX_VERSION) && !defined(PSP_VERSION))
#include <pthread.h>
pthread_t soundthread;
#endif

#if defined(ANDROID_VERSION)
#include <sys/stat.h>
#include <android/log.h>

extern "C" void android_render();
extern "C" void selectLatestSavegame();
extern bool psp_load_latest_savegame;
#endif

#ifdef MAC_VERSION
char dataDirectory[512];
char appDirectory[512];
extern "C"
{
    int osx_sys_question(const char *msg, const char *but1, const char *but2);
}
#endif

// How is this actually used??
// We need COLOR_DEPTH_24 to allow it to load the preload PCX in hi-col
BEGIN_COLOR_DEPTH_LIST
    COLOR_DEPTH_8
    COLOR_DEPTH_15
    COLOR_DEPTH_16
    COLOR_DEPTH_24
    COLOR_DEPTH_32
END_COLOR_DEPTH_LIST


extern "C" HWND allegro_wnd;


// for external modules to call
void next_iteration() {
    NEXT_ITERATION();
}


void precache_view(int view) 
{
    if (view < 0) 
        return;

    for (int i = 0; i < views[view].numLoops; i++) {
        for (int j = 0; j < views[view].loops[i].numFrames; j++)
            spriteset.precache (views[view].loops[i].frames[j].pic);
    }
}




#if defined(PSP_VERSION)
// PSP: Workaround for sound stuttering. Do sound updates in its own thread.
int update_mp3_thread(SceSize args, void *argp)
{
  while (update_mp3_thread_running)
  {
    UPDATE_MP3_THREAD
    sceKernelDelayThread(1000 * 50);
  }
  return 0;
}
#elif (defined(LINUX_VERSION) && !defined(PSP_VERSION)) || defined(MAC_VERSION)
void* update_mp3_thread(void* arg)
{
  while (update_mp3_thread_running)
  {
    UPDATE_MP3_THREAD
    usleep(1000 * 50);
  }
  pthread_exit(NULL);
}
#elif defined(WINDOWS_VERSION)
DWORD WINAPI update_mp3_thread(LPVOID lpParam)
{
  while (update_mp3_thread_running)
  {
    UPDATE_MP3_THREAD
    Sleep(50);
  }
  return 0;
}
#endif


int user_disabled_for=0,user_disabled_data=0,user_disabled_data2=0;
int user_disabled_data3=0;


// check and abort game if the script is currently
// inside the rep_exec_always function
void can_run_delayed_command() {
  if (no_blocking_functions)
    quit("!This command cannot be used within non-blocking events such as " REP_EXEC_ALWAYS_NAME);
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



// Sierra-style speech settings
int face_talking=-1,facetalkview=0,facetalkwait=0,facetalkframe=0;
int facetalkloop=0, facetalkrepeat = 0, facetalkAllowBlink = 1;
int facetalkBlinkLoop = 0;
CharacterInfo *facetalkchar = NULL;



// update_stuff: moves and animates objects, executes repeat scripts, and
// the like.
void update_stuff() {
  int aa;
  our_eip = 20;

  if (play.gscript_timer > 0) play.gscript_timer--;
  for (aa=0;aa<MAX_TIMERS;aa++) {
    if (play.script_timers[aa] > 1) play.script_timers[aa]--;
    }
  // update graphics for object if cycling view
  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on != 1) continue;
    if (objs[aa].moving>0) {
      do_movelist_move(&objs[aa].moving,&objs[aa].x,&objs[aa].y);
      }
    if (objs[aa].cycling==0) continue;
    if (objs[aa].view<0) continue;
    if (objs[aa].wait>0) { objs[aa].wait--; continue; }

    if (objs[aa].cycling >= ANIM_BACKWARDS) {
      // animate backwards
      objs[aa].frame--;
      if (objs[aa].frame < 0) {
        if ((objs[aa].loop > 0) && 
           (views[objs[aa].view].loops[objs[aa].loop - 1].RunNextLoop())) 
        {
          // If it's a Go-to-next-loop on the previous one, then go back
          objs[aa].loop --;
          objs[aa].frame = views[objs[aa].view].loops[objs[aa].loop].numFrames - 1;
        }
        else if (objs[aa].cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it on the first frame
          objs[aa].cycling = 0;
          objs[aa].frame = 0;
        }
        else { // repeating animation
          objs[aa].frame = views[objs[aa].view].loops[objs[aa].loop].numFrames - 1;
        }
      }
    }
    else {  // Animate forwards
      objs[aa].frame++;
      if (objs[aa].frame >= views[objs[aa].view].loops[objs[aa].loop].numFrames) {
        // go to next loop thing
        if (views[objs[aa].view].loops[objs[aa].loop].RunNextLoop()) {
          if (objs[aa].loop+1 >= views[objs[aa].view].numLoops)
            quit("!Last loop in a view requested to move to next loop");
          objs[aa].loop++;
          objs[aa].frame=0;
        }
        else if (objs[aa].cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it on the last frame
          objs[aa].cycling=0;
          objs[aa].frame--;
          }
        else {
          if (play.no_multiloop_repeat == 0) {
            // multi-loop anims, go back to start of it
            while ((objs[aa].loop > 0) && 
              (views[objs[aa].view].loops[objs[aa].loop - 1].RunNextLoop()))
              objs[aa].loop --;
          }
          if (objs[aa].cycling % ANIM_BACKWARDS == ANIM_ONCERESET)
            objs[aa].cycling=0;
          objs[aa].frame=0;
        }
      }
    }  // end if forwards

    ViewFrame*vfptr=&views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame];
    objs[aa].num = vfptr->pic;

    if (objs[aa].cycling == 0)
      continue;

    objs[aa].wait=vfptr->speed+objs[aa].overall_speed;
    CheckViewFrame (objs[aa].view, objs[aa].loop, objs[aa].frame);
  }
  our_eip = 21;

  // shadow areas
  int onwalkarea = get_walkable_area_at_character (game.playercharacter);
  if (onwalkarea<0) ;
  else if (playerchar->flags & CHF_FIXVIEW) ;
  else { onwalkarea=thisroom.shadinginfo[onwalkarea];
    if (onwalkarea>0) playerchar->view=onwalkarea-1;
    else if (thisroom.options[ST_MANVIEW]==0) playerchar->view=playerchar->defview;
    else playerchar->view=thisroom.options[ST_MANVIEW]-1;
  }
  our_eip = 22;

  #define MAX_SHEEP 30
  int numSheep = 0;
  int followingAsSheep[MAX_SHEEP];

  // move & animate characters
  for (aa=0;aa<game.numcharacters;aa++) {
    if (game.chars[aa].on != 1) continue;
    CharacterInfo*chi=&game.chars[aa];
    
    // walking
    if (chi->walking >= TURNING_AROUND) {
      // Currently rotating to correct direction
      if (chi->walkwait > 0) chi->walkwait--;
      else {
        // Work out which direction is next
        int wantloop = find_looporder_index(chi->loop) + 1;
        // going anti-clockwise, take one before instead
        if (chi->walking >= TURNING_BACKWARDS)
          wantloop -= 2;
        while (1) {
          if (wantloop >= 8)
            wantloop = 0;
          if (wantloop < 0)
            wantloop = 7;
          if ((turnlooporder[wantloop] >= views[chi->view].numLoops) ||
              (views[chi->view].loops[turnlooporder[wantloop]].numFrames < 1) ||
              ((turnlooporder[wantloop] >= 4) && ((chi->flags & CHF_NODIAGONAL)!=0))) {
            if (chi->walking >= TURNING_BACKWARDS)
              wantloop--;
            else
              wantloop++;
          }
          else break;
        }
        chi->loop = turnlooporder[wantloop];
        chi->walking -= TURNING_AROUND;
        // if still turning, wait for next frame
        if (chi->walking % TURNING_BACKWARDS >= TURNING_AROUND)
          chi->walkwait = chi->animspeed;
        else
          chi->walking = chi->walking % TURNING_BACKWARDS;
        charextra[aa].animwait = 0;
      }
      continue;
    }
    // Make sure it doesn't flash up a blue cup
    if (chi->view < 0) ;
    else if (chi->loop >= views[chi->view].numLoops)
      chi->loop = 0;

    int doing_nothing = 1;

    if ((chi->walking > 0) && (chi->room == displayed_room))
    {
      if (chi->walkwait > 0) chi->walkwait--;
      else 
      {
        chi->flags &= ~CHF_AWAITINGMOVE;

        // Move the character
        int numSteps = wantMoveNow(aa, chi);

        if ((numSteps) && (charextra[aa].xwas != INVALID_X)) {
          // if the zoom level changed mid-move, the walkcounter
          // might not have come round properly - so sort it out
          chi->x = charextra[aa].xwas;
          chi->y = charextra[aa].ywas;
          charextra[aa].xwas = INVALID_X;
        }

        int oldxp = chi->x, oldyp = chi->y;

        for (int ff = 0; ff < abs(numSteps); ff++) {
          if (doNextCharMoveStep (aa, chi))
            break;
          if ((chi->walking == 0) || (chi->walking >= TURNING_AROUND))
            break;
        }

        if (numSteps < 0) {
          // very small scaling, intersperse the movement
          // to stop it being jumpy
          charextra[aa].xwas = chi->x;
          charextra[aa].ywas = chi->y;
          chi->x = ((chi->x) - oldxp) / 2 + oldxp;
          chi->y = ((chi->y) - oldyp) / 2 + oldyp;
        }
        else if (numSteps > 0)
          charextra[aa].xwas = INVALID_X;

        if ((chi->flags & CHF_ANTIGLIDE) == 0)
          chi->walkwaitcounter++;
      }

      if (chi->loop >= views[chi->view].numLoops)
        quitprintf("Unable to render character %d (%s) because loop %d does not exist in view %d", chi->index_id, chi->name, chi->loop, chi->view + 1);

      // check don't overflow loop
      int framesInLoop = views[chi->view].loops[chi->loop].numFrames;
      if (chi->frame > framesInLoop)
      {
        chi->frame = 1;

        if (framesInLoop < 2)
          chi->frame = 0;

        if (framesInLoop < 1)
          quitprintf("Unable to render character %d (%s) because there are no frames in loop %d", chi->index_id, chi->name, chi->loop);
      }

      if (chi->walking<1) {
        charextra[aa].process_idle_this_time = 1;
        doing_nothing=1;
        chi->walkwait=0;
        charextra[aa].animwait = 0;
        // use standing pic
        StopMoving(aa);
        chi->frame = 0;
        CheckViewFrameForCharacter(chi);
      }
      else if (charextra[aa].animwait > 0) charextra[aa].animwait--;
      else {
        if (chi->flags & CHF_ANTIGLIDE)
          chi->walkwaitcounter++;

        if ((chi->flags & CHF_MOVENOTWALK) == 0)
        {
          chi->frame++;
          if (chi->frame >= views[chi->view].loops[chi->loop].numFrames)
          {
            // end of loop, so loop back round skipping the standing frame
            chi->frame = 1;

            if (views[chi->view].loops[chi->loop].numFrames < 2)
              chi->frame = 0;
          }

          charextra[aa].animwait = views[chi->view].loops[chi->loop].frames[chi->frame].speed + chi->animspeed;

          if (chi->flags & CHF_ANTIGLIDE)
            chi->walkwait = charextra[aa].animwait;
          else
            chi->walkwait = 0;

          CheckViewFrameForCharacter(chi);
        }
      }
      doing_nothing = 0;
    }
    // not moving, but animating
    // idleleft is <0 while idle view is playing (.animating is 0)
    if (((chi->animating != 0) || (chi->idleleft < 0)) &&
        ((chi->walking == 0) || ((chi->flags & CHF_MOVENOTWALK) != 0)) &&
        (chi->room == displayed_room)) 
    {
      doing_nothing = 0;
      // idle anim doesn't count as doing something
      if (chi->idleleft < 0)
        doing_nothing = 1;

      if (chi->wait>0) chi->wait--;
      else if ((char_speaking == aa) && (game.options[OPT_LIPSYNCTEXT] != 0)) {
        // currently talking with lip-sync speech
        int fraa = chi->frame;
        chi->wait = update_lip_sync (chi->view, chi->loop, &fraa) - 1;
        // closed mouth at end of sentence
        if ((play.messagetime >= 0) && (play.messagetime < play.close_mouth_speech_time))
          chi->frame = 0;

        if (chi->frame != fraa) {
          chi->frame = fraa;
          CheckViewFrameForCharacter(chi);
        }
        
        continue;
      }
      else {
        int oldframe = chi->frame;
        if (chi->animating & CHANIM_BACKWARDS) {
          chi->frame--;
          if (chi->frame < 0) {
            // if the previous loop is a Run Next Loop one, go back to it
            if ((chi->loop > 0) && 
              (views[chi->view].loops[chi->loop - 1].RunNextLoop())) {

              chi->loop --;
              chi->frame = views[chi->view].loops[chi->loop].numFrames - 1;
            }
            else if (chi->animating & CHANIM_REPEAT) {

              chi->frame = views[chi->view].loops[chi->loop].numFrames - 1;

              while (views[chi->view].loops[chi->loop].RunNextLoop()) {
                chi->loop++;
                chi->frame = views[chi->view].loops[chi->loop].numFrames - 1;
              }
            }
            else {
              chi->frame++;
              chi->animating = 0;
            }
          }
        }
        else
          chi->frame++;

        if ((aa == char_speaking) &&
            (channels[SCHAN_SPEECH] == NULL) &&
            (play.close_mouth_speech_time > 0) &&
            (play.messagetime < play.close_mouth_speech_time)) {
          // finished talking - stop animation
          chi->animating = 0;
          chi->frame = 0;
        }

        if (chi->frame >= views[chi->view].loops[chi->loop].numFrames) {
          
          if (views[chi->view].loops[chi->loop].RunNextLoop()) 
          {
            if (chi->loop+1 >= views[chi->view].numLoops)
              quit("!Animating character tried to overrun last loop in view");
            chi->loop++;
            chi->frame=0;
          }
          else if ((chi->animating & CHANIM_REPEAT)==0) {
            chi->animating=0;
            chi->frame--;
            // end of idle anim
            if (chi->idleleft < 0) {
              // constant anim, reset (need this cos animating==0)
              if (chi->idletime == 0)
                chi->frame = 0;
              // one-off anim, stop
              else {
                ReleaseCharacterView(aa);
                chi->idleleft=chi->idletime;
              }
            }
          }
          else {
            chi->frame=0;
            // if it's a multi-loop animation, go back to start
            if (play.no_multiloop_repeat == 0) {
              while ((chi->loop > 0) && 
                  (views[chi->view].loops[chi->loop - 1].RunNextLoop()))
                chi->loop--;
            }
          }
        }
        chi->wait = views[chi->view].loops[chi->loop].frames[chi->frame].speed;
        // idle anim doesn't have speed stored cos animating==0
        if (chi->idleleft < 0)
          chi->wait += chi->animspeed+5;
        else 
          chi->wait += (chi->animating >> 8) & 0x00ff;

        if (chi->frame != oldframe)
          CheckViewFrameForCharacter(chi);
      }
    }

    if ((chi->following >= 0) && (chi->followinfo == FOLLOW_ALWAYSONTOP)) {
      // an always-on-top follow
      if (numSheep >= MAX_SHEEP)
        quit("too many sheep");
      followingAsSheep[numSheep] = aa;
      numSheep++;
    }
    // not moving, but should be following another character
    else if ((chi->following >= 0) && (doing_nothing == 1)) {
      short distaway=(chi->followinfo >> 8) & 0x00ff;
      // no character in this room
      if ((game.chars[chi->following].on == 0) || (chi->on == 0)) ;
      else if (chi->room < 0) {
        chi->room ++;
        if (chi->room == 0) {
          // appear in the new room
          chi->room = game.chars[chi->following].room;
          chi->x = play.entered_at_x;
          chi->y = play.entered_at_y;
        }
      }
      // wait a bit, so we're not constantly walking
      else if (Random(100) < (chi->followinfo & 0x00ff)) ;
      // the followed character has changed room
      else if ((chi->room != game.chars[chi->following].room)
            && (game.chars[chi->following].on == 0))
        ;  // do nothing if the player isn't visible
      else if (chi->room != game.chars[chi->following].room) {
        chi->prevroom = chi->room;
        chi->room = game.chars[chi->following].room;

        if (chi->room == displayed_room) {
          // only move to the room-entered position if coming into
          // the current room
          if (play.entered_at_x > (thisroom.width - 8)) {
            chi->x = thisroom.width+8;
            chi->y = play.entered_at_y;
            }
          else if (play.entered_at_x < 8) {
            chi->x = -8;
            chi->y = play.entered_at_y;
            }
          else if (play.entered_at_y > (thisroom.height - 8)) {
            chi->y = thisroom.height+8;
            chi->x = play.entered_at_x;
            }
          else if (play.entered_at_y < thisroom.top+8) {
            chi->y = thisroom.top+1;
            chi->x = play.entered_at_x;
            }
          else {
            // not at one of the edges
            // delay for a few seconds to let the player move
            chi->room = -play.follow_change_room_timer;
          }
          if (chi->room >= 0) {
            walk_character(aa,play.entered_at_x,play.entered_at_y,1, true);
            doing_nothing = 0;
          }
        }
      }
      else if (chi->room != displayed_room) {
        // if the characetr is following another character and
        // neither is in the current room, don't try to move
      }
      else if ((abs(game.chars[chi->following].x - chi->x) > distaway+30) |
        (abs(game.chars[chi->following].y - chi->y) > distaway+30) |
        ((chi->followinfo & 0x00ff) == 0)) {
        // in same room
        int goxoffs=(Random(50)-25);
        // make sure he's not standing on top of the other man
        if (goxoffs < 0) goxoffs-=distaway;
        else goxoffs+=distaway;
        walk_character(aa,game.chars[chi->following].x + goxoffs,
          game.chars[chi->following].y + (Random(50)-25),0, true);
        doing_nothing = 0;
      }
    }

    // no idle animation, so skip this bit
    if (chi->idleview < 1) ;
    // currently playing idle anim
    else if (chi->idleleft < 0) ;
    // not in the current room
    else if (chi->room != displayed_room) ;
    // they are moving or animating (or the view is locked), so 
    // reset idle timeout
    else if ((doing_nothing == 0) || ((chi->flags & CHF_FIXVIEW) != 0))
      chi->idleleft = chi->idletime;
    // count idle time
    else if ((loopcounter%40==0) || (charextra[aa].process_idle_this_time == 1)) {
      chi->idleleft--;
      if (chi->idleleft == -1) {
        int useloop=chi->loop;
        DEBUG_CONSOLE("%s: Now idle (view %d)", chi->scrname, chi->idleview+1);
        SetCharacterView(aa,chi->idleview+1);
        // SetCharView resets it to 0
        chi->idleleft = -2;
        int maxLoops = views[chi->idleview].numLoops;
        // if the char is set to "no diagonal loops", don't try
        // to use diagonal idle loops either
        if ((maxLoops > 4) && (useDiagonal(chi)))
          maxLoops = 4;
        // If it's not a "swimming"-type idleanim, choose a random loop
        // if there arent enough loops to do the current one.
        if ((chi->idletime > 0) && (useloop >= maxLoops)) {
          do {
            useloop = rand() % maxLoops;
          // don't select a loop which is a continuation of a previous one
          } while ((useloop > 0) && (views[chi->idleview].loops[useloop-1].RunNextLoop()));
        }
        // Normal idle anim - just reset to loop 0 if not enough to
        // use the current one
        else if (useloop >= maxLoops)
          useloop = 0;

        animate_character(chi,useloop,
          chi->animspeed+5,(chi->idletime == 0) ? 1 : 0, 1);

        // don't set Animating while the idle anim plays
        chi->animating = 0;
      }
    }  // end do idle animation

    charextra[aa].process_idle_this_time = 0;
  }

  // update location of all following_exactly characters
  for (aa = 0; aa < numSheep; aa++) {
    CharacterInfo *chi = &game.chars[followingAsSheep[aa]];

    chi->x = game.chars[chi->following].x;
    chi->y = game.chars[chi->following].y;
    chi->z = game.chars[chi->following].z;
    chi->room = game.chars[chi->following].room;
    chi->prevroom = game.chars[chi->following].prevroom;

    int usebase = game.chars[chi->following].get_baseline();

    if (chi->flags & CHF_BEHINDSHEPHERD)
      chi->baseline = usebase - 1;
    else
      chi->baseline = usebase + 1;
  }

  our_eip = 23;

  // update overlay timers
  for (aa=0;aa<numscreenover;aa++) {
    if (screenover[aa].timeout > 0) {
      screenover[aa].timeout--;
      if (screenover[aa].timeout == 0)
        remove_screen_overlay(screenover[aa].type);
    }
  }

  // determine if speech text should be removed
  if (play.messagetime>=0) {
    play.messagetime--;
    // extend life of text if the voice hasn't finished yet
    if (channels[SCHAN_SPEECH] != NULL) {
      if ((!rec_isSpeechFinished()) && (play.fast_forward == 0)) {
      //if ((!channels[SCHAN_SPEECH]->done) && (play.fast_forward == 0)) {
        if (play.messagetime <= 1)
          play.messagetime = 1;
      }
      else  // if the voice has finished, remove the speech
        play.messagetime = 0;
    }

    if (play.messagetime < 1) 
    {
      if (play.fast_forward > 0)
      {
        remove_screen_overlay(OVER_TEXTMSG);
      }
      else if (play.cant_skip_speech & SKIP_AUTOTIMER)
      {
        remove_screen_overlay(OVER_TEXTMSG);
        play.ignore_user_input_until_time = globalTimerCounter + (play.ignore_user_input_after_text_timeout_ms / time_between_timers);
      }
    }
  }
  our_eip = 24;

  // update sierra-style speech
  if ((face_talking >= 0) && (play.fast_forward == 0)) 
{
    int updatedFrame = 0;

    if ((facetalkchar->blinkview > 0) && (facetalkAllowBlink)) {
      if (facetalkchar->blinktimer > 0) {
        // countdown to playing blink anim
        facetalkchar->blinktimer--;
        if (facetalkchar->blinktimer == 0) {
          facetalkchar->blinkframe = 0;
          facetalkchar->blinktimer = -1;
          updatedFrame = 2;
        }
      }
      else if (facetalkchar->blinktimer < 0) {
        // currently playing blink anim
        if (facetalkchar->blinktimer < ( (0 - 6) - views[facetalkchar->blinkview].loops[facetalkBlinkLoop].frames[facetalkchar->blinkframe].speed)) {
          // time to advance to next frame
          facetalkchar->blinktimer = -1;
          facetalkchar->blinkframe++;
          updatedFrame = 2;
          if (facetalkchar->blinkframe >= views[facetalkchar->blinkview].loops[facetalkBlinkLoop].numFrames) 
          {
            facetalkchar->blinkframe = 0;
            facetalkchar->blinktimer = facetalkchar->blinkinterval;
          }
        }
        else
          facetalkchar->blinktimer--;
      }

    }

    if (curLipLine >= 0) {
      // check voice lip sync
      int spchOffs = channels[SCHAN_SPEECH]->get_pos_ms ();
      if (curLipLinePhenome >= splipsync[curLipLine].numPhenomes) {
        // the lip-sync has finished, so just stay idle
      }
      else 
      {
        while ((curLipLinePhenome < splipsync[curLipLine].numPhenomes) &&
          ((curLipLinePhenome < 0) || (spchOffs >= splipsync[curLipLine].endtimeoffs[curLipLinePhenome])))
        {
          curLipLinePhenome ++;
          if (curLipLinePhenome >= splipsync[curLipLine].numPhenomes)
            facetalkframe = game.default_lipsync_frame;
          else
            facetalkframe = splipsync[curLipLine].frame[curLipLinePhenome];

          if (facetalkframe >= views[facetalkview].loops[facetalkloop].numFrames)
            facetalkframe = 0;

          updatedFrame |= 1;
        }
      }
    }
    else if (facetalkwait>0) facetalkwait--;
    // don't animate if the speech has finished
    else if ((play.messagetime < 1) && (facetalkframe == 0) && (play.close_mouth_speech_time > 0))
      ;
    else {
      // Close mouth at end of sentence
      if ((play.messagetime < play.close_mouth_speech_time) &&
          (play.close_mouth_speech_time > 0)) {
        facetalkframe = 0;
        facetalkwait = play.messagetime;
      }
      else if ((game.options[OPT_LIPSYNCTEXT]) && (facetalkrepeat > 0)) {
        // lip-sync speech (and not a thought)
        facetalkwait = update_lip_sync (facetalkview, facetalkloop, &facetalkframe);
        // It is actually displayed for facetalkwait+1 loops
        // (because when it's 1, it gets --'d then wait for next time)
        facetalkwait --;
      }
      else {
        // normal non-lip-sync
        facetalkframe++;
        if ((facetalkframe >= views[facetalkview].loops[facetalkloop].numFrames) ||
            ((play.messagetime < 1) && (play.close_mouth_speech_time > 0))) {

          if ((facetalkframe >= views[facetalkview].loops[facetalkloop].numFrames) &&
              (views[facetalkview].loops[facetalkloop].RunNextLoop())) 
          {
            facetalkloop++;
          }
          else 
          {
            facetalkloop = 0;
          }
          facetalkframe = 0;
          if (!facetalkrepeat)
            facetalkwait = 999999;
        }
        if ((facetalkframe != 0) || (facetalkrepeat == 1))
          facetalkwait = views[facetalkview].loops[facetalkloop].frames[facetalkframe].speed + GetCharacterSpeechAnimationDelay(facetalkchar);
      }
      updatedFrame |= 1;
    }

    // is_text_overlay might be 0 if it was only just destroyed this loop
    if ((updatedFrame) && (is_text_overlay > 0)) {

      if (updatedFrame & 1)
        CheckViewFrame (facetalkview, facetalkloop, facetalkframe);
      if (updatedFrame & 2)
        CheckViewFrame (facetalkchar->blinkview, facetalkBlinkLoop, facetalkchar->blinkframe);

      int yPos = 0;
      int thisPic = views[facetalkview].loops[facetalkloop].frames[facetalkframe].pic;
      
      if (game.options[OPT_SPEECHTYPE] == 3) {
        // QFG4-style fullscreen dialog
        yPos = (screenover[face_talking].pic->h / 2) - (spriteheight[thisPic] / 2);
        clear_to_color(screenover[face_talking].pic, 0);
      }
      else {
        clear_to_color(screenover[face_talking].pic, bitmap_mask_color(screenover[face_talking].pic));
      }

      DrawViewFrame(screenover[face_talking].pic, &views[facetalkview].loops[facetalkloop].frames[facetalkframe], 0, yPos);
//      draw_sprite(screenover[face_talking].pic, spriteset[thisPic], 0, yPos);

      if ((facetalkchar->blinkview > 0) && (facetalkchar->blinktimer < 0)) {
        // draw the blinking sprite on top
        DrawViewFrame(screenover[face_talking].pic,
            &views[facetalkchar->blinkview].loops[facetalkBlinkLoop].frames[facetalkchar->blinkframe],
            0, yPos);

/*        draw_sprite(screenover[face_talking].pic,
          spriteset[views[facetalkchar->blinkview].frames[facetalkloop][facetalkchar->blinkframe].pic],
          0, yPos);*/
      }

      gfxDriver->UpdateDDBFromBitmap(screenover[face_talking].bmp, screenover[face_talking].pic, false);
    }  // end if updatedFrame
  }

  our_eip = 25;
}



const char *get_engine_version() {
    return ACI_VERSION_TEXT;
}

void atexit_handler() {
    if (proper_exit==0) {
        sprintf(pexbuf,"\nError: the program has exited without requesting it.\n"
            "Program pointer: %+03d  (write this number down), ACI version " ACI_VERSION_TEXT "\n"
            "If you see a list of numbers above, please write them down and contact\n"
            "Chris Jones. Otherwise, note down any other information displayed.\n",
            our_eip);
        platform->DisplayAlert(pexbuf);
    }

    if (!(music_file == NULL))
        free(music_file);

    if (!(speech_file == NULL))
        free(speech_file);

    // Deliberately commented out, because chances are game_file_name
    // was not allocated on the heap, it points to argv[0] or
    // the gamefilenamebuf memory
    // It will get freed by the system anyway, leaving it in can
    // cause a crash on exit
    /*if (!(game_file_name == NULL))
    free(game_file_name);*/
}





char return_to_roomedit[30] = "\0";
char return_to_room[150] = "\0";
// quit - exits the engine, shutting down everything gracefully
// The parameter is the message to print. If this message begins with
// an '!' character, then it is printed as a "contact game author" error.
// If it begins with a '|' then it is treated as a "thanks for playing" type
// message. If it begins with anything else, it is treated as an internal
// error.
// "!|" is a special code used to mean that the player has aborted (Alt+X)
void quit(char*quitmsg) {
    int i;
    // Need to copy it in case it's from a plugin (since we're
    // about to free plugins)
    char qmsgbufr[STD_BUFFER_SIZE];
    strncpy(qmsgbufr, quitmsg, STD_BUFFER_SIZE);
    qmsgbufr[STD_BUFFER_SIZE - 1] = 0;
    char *qmsg = &qmsgbufr[0];

    bool handledErrorInEditor = false;

    if (editor_debugging_initialized)
    {
        if ((qmsg[0] == '!') && (qmsg[1] != '|'))
        {
            handledErrorInEditor = send_exception_to_editor(&qmsg[1]);
        }
        send_message_to_editor("EXIT");
        editor_debugger->Shutdown();
    }

    our_eip = 9900;
    stop_recording();

    if (need_to_stop_cd)
        cd_manager(3,0);

    our_eip = 9020;
    ccUnregisterAllObjects();

    our_eip = 9019;
    platform->AboutToQuitGame();

    our_eip = 9016;
    platform->ShutdownPlugins();

    if ((qmsg[0] == '|') && (check_dynamic_sprites_at_exit) && 
        (game.options[OPT_DEBUGMODE] != 0)) {
            // game exiting normally -- make sure the dynamic sprites
            // have been deleted
            for (i = 1; i < spriteset.elements; i++) {
                if (game.spriteflags[i] & SPF_DYNAMICALLOC)
                    debug_log("Dynamic sprite %d was never deleted", i);
            }
    }

    // allegro_exit assumes screen is correct
    if (_old_screen)
        screen = _old_screen;

    platform->FinishedUsingGraphicsMode();

    if (use_cdplayer)
        platform->ShutdownCDPlayer();

    our_eip = 9917;
    game.options[OPT_CROSSFADEMUSIC] = 0;
    stopmusic();
#ifndef PSP_NO_MOD_PLAYBACK
    if (opts.mod_player)
        remove_mod_player();
#endif

    // Quit the sound thread.
    update_mp3_thread_running = false;

    remove_sound();
    our_eip = 9901;

    char alertis[1500]="\0";
    if (qmsg[0]=='|') ; //qmsg++;
    else if (qmsg[0]=='!') { 
        qmsg++;

        if (qmsg[0] == '|')
            strcpy (alertis, "Abort key pressed.\n\n");
        else if (qmsg[0] == '?') {
            strcpy(alertis, "A fatal error has been generated by the script using the AbortGame function. Please contact the game author for support.\n\n");
            qmsg++;
        }
        else
            strcpy(alertis,"An error has occurred. Please contact the game author for support, as this "
            "is likely to be a scripting error and not a bug in AGS.\n"
            "(ACI version " ACI_VERSION_TEXT ")\n\n");

        strcat (alertis, get_cur_script(5) );

        if (qmsg[0] != '|')
            strcat(alertis,"\nError: ");
        else
            qmsg = "";
    }
    else if (qmsg[0] == '%') {
        qmsg++;

        sprintf(alertis, "A warning has been generated. This is not normally fatal, but you have selected "
            "to treat warnings as errors.\n"
            "(ACI version " ACI_VERSION_TEXT ")\n\n%s\n", get_cur_script(5));
    }
    else strcpy(alertis,"An internal error has occurred. Please note down the following information.\n"
        "If the problem persists, post the details on the AGS Technical Forum.\n"
        "(ACI version " ACI_VERSION_TEXT ")\n"
        "\nError: ");

    shutdown_font_renderer();
    our_eip = 9902;

    // close graphics mode (Win) or return to text mode (DOS)
    if (_sub_screen) {
        destroy_bitmap(_sub_screen);
        _sub_screen = NULL;
    }

    our_eip = 9907;

    close_translation();

    our_eip = 9908;

    // Release the display mode (and anything dependant on the window)
    if (gfxDriver != NULL)
        gfxDriver->UnInit();

    // Tell Allegro that we are no longer in graphics mode
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

    // successful exit displays no messages (because Windoze closes the dos-box
    // if it is empty).
    if (qmsg[0]=='|') ;
    else if (!handledErrorInEditor)
    {
        // Display the message (at this point the window still exists)
        sprintf(pexbuf,"%s\n",qmsg);
        strcat(alertis,pexbuf);
        platform->DisplayAlert(alertis);
    }

    // remove the game window
    allegro_exit();

    if (gfxDriver != NULL)
    {
        delete gfxDriver;
        gfxDriver = NULL;
    }

    platform->PostAllegroExit();

    our_eip = 9903;

    // wipe all the interaction structs so they don't de-alloc the children twice
    memset (&roomstats[0], 0, sizeof(RoomStatus) * MAX_ROOMS);
    memset (&troom, 0, sizeof(RoomStatus));

    /*  _CrtMemState memstart;
    _CrtMemCheckpoint(&memstart);
    _CrtMemDumpStatistics( &memstart );*/

    /*  // print the FPS if there wasn't an error
    if ((play.debug_mode!=0) && (qmsg[0]=='|'))
    printf("Last cycle fps: %d\n",fps);*/
    al_ffblk	dfb;
    int	dun = al_findfirst("~ac*.tmp",&dfb,FA_SEARCH);
    while (!dun) {
        unlink(dfb.name);
        dun = al_findnext(&dfb);
    }
    al_findclose (&dfb);

    proper_exit=1;

    write_log_debug("***** ENGINE HAS SHUTDOWN");

    our_eip = 9904;
    exit(EXIT_NORMAL);
}

extern "C" {
    void quit_c(char*msg) {
        quit(msg);
    }
}


int do_movelist_move(short*mlnum,int*xx,int*yy) {
  int need_to_fix_sprite=0;
  if (mlnum[0]<1) quit("movelist_move: attempted to move on a non-exist movelist");
  MoveList*cmls; cmls=&mls[mlnum[0]];
  fixed xpermove=cmls->xpermove[cmls->onstage],ypermove=cmls->ypermove[cmls->onstage];

  short targetx=short((cmls->pos[cmls->onstage+1] >> 16) & 0x00ffff);
  short targety=short(cmls->pos[cmls->onstage+1] & 0x00ffff);
  int xps=xx[0],yps=yy[0];
  if (cmls->doneflag & 1) {
    // if the X-movement has finished, and the Y-per-move is < 1, finish
    // This can cause jump at the end, but without it the character will
    // walk on the spot for a while if the Y-per-move is for example 0.2
//    if ((ypermove & 0xfffff000) == 0) cmls->doneflag|=2;
//    int ypmm=(ypermove >> 16) & 0x0000ffff;

    // NEW 2.15 SR-1 plan: if X-movement has finished, and Y-per-move is < 1,
    // allow it to finish more easily by moving target zone

    int adjAmnt = 3;
    // 2.70: if the X permove is also <=1, don't do the skipping
    if (((xpermove & 0xffff0000) == 0xffff0000) ||
        ((xpermove & 0xffff0000) == 0x00000000))
      adjAmnt = 2;

    // 2.61 RC1: correct this to work with > -1 as well as < 1
    if (ypermove == 0) { }
    // Y per move is < 1, so finish the move
    else if ((ypermove & 0xffff0000) == 0)
      targety -= adjAmnt;
    // Y per move is -1 exactly, don't snap to finish
    else if (ypermove == 0xffff0000) { }
    // Y per move is > -1, so finish the move
    else if ((ypermove & 0xffff0000) == 0xffff0000)
      targety += adjAmnt;
  }
  else xps=cmls->fromx+(int)(fixtof(xpermove)*(float)cmls->onpart);

  if (cmls->doneflag & 2) {
    // Y-movement has finished

    int adjAmnt = 3;

    // if the Y permove is also <=1, don't skip as far
    if (((ypermove & 0xffff0000) == 0xffff0000) ||
        ((ypermove & 0xffff0000) == 0x00000000))
      adjAmnt = 2;

    if (xpermove == 0) { }
    // Y per move is < 1, so finish the move
    else if ((xpermove & 0xffff0000) == 0)
      targetx -= adjAmnt;
    // X per move is -1 exactly, don't snap to finish
    else if (xpermove == 0xffff0000) { }
    // X per move is > -1, so finish the move
    else if ((xpermove & 0xffff0000) == 0xffff0000)
      targetx += adjAmnt;

/*    int xpmm=(xpermove >> 16) & 0x0000ffff;
//    if ((xpmm==0) | (xpmm==0xffff)) cmls->doneflag|=1;
    if (xpmm==0) cmls->doneflag|=1;*/
    }
  else yps=cmls->fromy+(int)(fixtof(ypermove)*(float)cmls->onpart);
  // check if finished horizontal movement
  if (((xpermove > 0) && (xps >= targetx)) ||
      ((xpermove < 0) && (xps <= targetx))) {
    cmls->doneflag|=1;
    xps = targetx;
    // if the Y is almost there too, finish it
    // this is new in v2.40
    // removed in 2.70
    /*if (abs(yps - targety) <= 2)
      yps = targety;*/
  }
  else if (xpermove == 0)
    cmls->doneflag|=1;
  // check if finished vertical movement
  if ((ypermove > 0) & (yps>=targety)) {
    cmls->doneflag|=2;
    yps = targety;
  }
  else if ((ypermove < 0) & (yps<=targety)) {
    cmls->doneflag|=2;
    yps = targety;
  }
  else if (ypermove == 0)
    cmls->doneflag|=2;

  if ((cmls->doneflag & 0x03)==3) {
    // this stage is done, go on to the next stage
    // signed shorts to ensure that numbers like -20 do not become 65515
    cmls->fromx=(signed short)((cmls->pos[cmls->onstage+1] >> 16) & 0x000ffff);
    cmls->fromy=(signed short)(cmls->pos[cmls->onstage+1] & 0x000ffff);
    if ((cmls->fromx > 65000) || (cmls->fromy > 65000))
      quit("do_movelist: int to short rounding error");

    cmls->onstage++; cmls->onpart=-1; cmls->doneflag&=0xf0;
    cmls->lastx=-1;
    if (cmls->onstage < cmls->numstage) {
      xps=cmls->fromx; yps=cmls->fromy; }
    if (cmls->onstage>=cmls->numstage-1) {  // last stage is just dest pos
      cmls->numstage=0;
      mlnum[0]=0;
      need_to_fix_sprite=1;
      }
    else need_to_fix_sprite=2;
    }
  cmls->onpart++;
  xx[0]=xps; yy[0]=yps;
  return need_to_fix_sprite;
  }


int check_write_access() {

  if (platform->GetDiskFreeSpaceMB() < 2)
    return 0;

  our_eip = -1895;

  // The Save Game Dir is the only place that we should write to
  char tempPath[MAX_PATH];
  sprintf(tempPath, "%s""tmptest.tmp", saveGameDirectory);
  FILE *yy = fopen(tempPath, "wb");
  if (yy == NULL)
    return 0;

  our_eip = -1896;

  fwrite("just to test the drive free space", 30, 1, yy);
  fclose(yy);

  our_eip = -1897;

  if (unlink(tempPath))
    return 0;

  return 1;
}


long t1;  // timer for FPS // ... 't1'... how very appropriate.. :)


void mainloop(bool checkControls, IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {
  UPDATE_MP3

  int numEventsAtStartOfFunction = numevents;

  if (want_exit) {
    want_exit = 0;
    proper_exit = 1;
    quit("||exit!");
/*#ifdef WINDOWS_VERSION
    // the Quit thread is running now, so exit this main one.
    ExitThread (1);
#endif*/
  }
  ccNotifyScriptStillAlive ();
  our_eip=1;
  timerloop=0;
  if (want_quit) {
    want_quit = 0;
    QuitGame(1);
    }
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

  our_eip = 1014;
  update_gui_disabled_status();

  our_eip = 1004;
  if (in_new_room == 0) {
    // Run the room and game script repeatedly_execute
    run_function_on_non_blocking_thread(&repExecAlways);
    setevent(EV_TEXTSCRIPT,TS_REPEAT);
    setevent(EV_RUNEVBLOCK,EVB_ROOM,0,6);  
  }
  // run this immediately to make sure it gets done before fade-in
  // (player enters screen)
  check_new_room ();
  
  our_eip = 1005;

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
      return;
    }
  } // end if checking ground level interactions

  mouse_on_iface=-1;

  check_debug_keys();

  // don't let the player do anything before the screen fades in
  if ((in_new_room == 0) && (checkControls)) {
    int inRoom = displayed_room;
    check_controls();
    // If an inventory interaction changed the room
    if (inRoom != displayed_room)
      check_new_room();
  }
  our_eip=2;
  if (debug_flags & DBG_NOUPDATE) ;
  else if (game_paused==0) update_stuff();

  // update animating GUI buttons
  // this bit isn't in update_stuff because it always needs to
  // happen, even when the game is paused
  for (int aa = 0; aa < numAnimButs; aa++) {
    if (UpdateAnimatingButton(aa)) {
      StopButtonAnimation(aa);
      aa--;
    } 
  }

  update_polled_stuff_and_crossfade();

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
  our_eip=6;
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
  our_eip=7;
//    if (mgetbutton()>NONE) break;
  update_polled_stuff_if_runtime();
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
  loopcounter++;

  if (play.wait_counter > 0) play.wait_counter--;
  if (play.shakesc_length > 0) play.shakesc_length--;

  if (loopcounter % 5 == 0)
  {
    update_ambient_sound_vol();
    update_directional_sound_vol();
  }

  if (replay_start_this_time) {
    replay_start_this_time = 0;
    start_replay_record();
  }

  if (play.fast_forward)
    return;

  our_eip=72;
  if (time(NULL) != t1) {
    t1 = time(NULL);
    fps = loopcounter - lastcounter;
    lastcounter = loopcounter;
  }

  // make sure we poll, cos a low framerate (eg 5 fps) could stutter
  // mp3 music
  while (timerloop == 0) {
    update_polled_stuff_if_runtime();
    platform->YieldCPU();
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

int main_game_loop() {
  if (displayed_room < 0)
    quit("!A blocking function was called before the first room has been loaded");

  mainloop(true);
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

  our_eip=76;
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
  our_eip = 78;
  return 0;
}

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


#if defined(WINDOWS_VERSION)
#include <new.h>
char tempmsg[100];
char*printfworkingspace;
int malloc_fail_handler(size_t amountwanted) {
  free(printfworkingspace);
  sprintf(tempmsg,"Out of memory: failed to allocate %ld bytes (at PP=%d)",amountwanted, our_eip);
  quit(tempmsg);
  return 0;
}
#endif

// set to 0 once successful
int working_gfx_mode_status = -1;
int debug_15bit_mode = 0, debug_24bit_mode = 0;
int convert_16bit_bgr = 0;


int init_gfx_mode(int wid,int hit,int cdep) {

  // a mode has already been initialized, so abort
  if (working_gfx_mode_status == 0) return 0;

  if (debug_15bit_mode)
    cdep = 15;
  else if (debug_24bit_mode)
    cdep = 24;

  platform->WriteDebugString("Attempt to switch gfx mode to %d x %d (%d-bit)", wid, hit, cdep);

  if (usetup.refresh >= 50)
    request_refresh_rate(usetup.refresh);

  final_scrn_wid = wid;
  final_scrn_hit = hit;
  final_col_dep = cdep;

  if (game.color_depth == 1) {
    final_col_dep = 8;
  }
  else {
    set_color_depth(cdep);
  }

  working_gfx_mode_status = (gfxDriver->Init(wid, hit, final_col_dep, usetup.windowed > 0, &timerloop) ? 0 : -1);

  if (working_gfx_mode_status == 0) 
    platform->WriteDebugString("Succeeded. Using gfx mode %d x %d (%d-bit)", wid, hit, final_col_dep);
  else
    platform->WriteDebugString("Failed, resolution not supported");

  if ((working_gfx_mode_status < 0) && (usetup.windowed > 0) && (editor_debugging_enabled == 0)) {
    usetup.windowed ++;
    if (usetup.windowed > 2) usetup.windowed = 0;
    return init_gfx_mode(wid,hit,cdep);
  }
  return working_gfx_mode_status;    
}

void winclosehook() {
  want_exit = 1;
  abort_engine = 1;
  check_dynamic_sprites_at_exit = 0;
/*  while (want_exit == 1)
    yield_timeslice();
  / *if (want_quit == 0)
    want_quit = 1;
  else* / quit("|game aborted");
*/
}

void init_game_settings() {
  int ee;

  for (ee=0;ee<256;ee++) {
    if (game.paluses[ee]!=PAL_BACKGROUND)
      palette[ee]=game.defpal[ee];
  }

  if (game.options[OPT_NOSCALEFNT]) wtext_multiply=1;

  for (ee = 0; ee < game.numcursors; ee++) 
  {
    // The cursor graphics are assigned to mousecurs[] and so cannot
    // be removed from memory
    if (game.mcurs[ee].pic >= 0)
      spriteset.precache (game.mcurs[ee].pic);

    // just in case they typed an invalid view number in the editor
    if (game.mcurs[ee].view >= game.numviews)
      game.mcurs[ee].view = -1;

    if (game.mcurs[ee].view >= 0)
      precache_view (game.mcurs[ee].view);
  }
  // may as well preload the character gfx
  if (playerchar->view >= 0)
    precache_view (playerchar->view);

  for (ee = 0; ee < MAX_INIT_SPR; ee++)
    objcache[ee].image = NULL;

/*  dummygui.guiId = -1;
  dummyguicontrol.guin = -1;
  dummyguicontrol.objn = -1;*/

  our_eip=-6;
//  game.chars[0].talkview=4;
  //init_language_text(game.langcodes[0]);

  for (ee = 0; ee < MAX_INIT_SPR; ee++) {
    scrObj[ee].id = ee;
    scrObj[ee].obj = NULL;
  }

  for (ee=0;ee<game.numcharacters;ee++) {
    memset(&game.chars[ee].inv[0],0,MAX_INV*sizeof(short));
    game.chars[ee].activeinv=-1;
    game.chars[ee].following=-1;
    game.chars[ee].followinfo=97 | (10 << 8);
    game.chars[ee].idletime=20;  // can be overridden later with SetIdle or summink
    game.chars[ee].idleleft=game.chars[ee].idletime;
    game.chars[ee].transparency = 0;
    game.chars[ee].baseline = -1;
    game.chars[ee].walkwaitcounter = 0;
    game.chars[ee].z = 0;
    charextra[ee].xwas = INVALID_X;
    charextra[ee].zoom = 100;
    if (game.chars[ee].view >= 0) {
      // set initial loop to 0
      game.chars[ee].loop = 0;
      // or to 1 if they don't have up/down frames
      if (views[game.chars[ee].view].loops[0].numFrames < 1)
        game.chars[ee].loop = 1;
    }
    charextra[ee].process_idle_this_time = 0;
    charextra[ee].invorder_count = 0;
    charextra[ee].slow_move_counter = 0;
    charextra[ee].animwait = 0;
  }
  // multiply up gui positions
  guibg = (block*)malloc(sizeof(block) * game.numgui);
  guibgbmp = (IDriverDependantBitmap**)malloc(sizeof(IDriverDependantBitmap*) * game.numgui);
  for (ee=0;ee<game.numgui;ee++) {
    guibgbmp[ee] = NULL;
    GUIMain*cgp=&guis[ee];
    guibg[ee] = create_bitmap_ex (final_col_dep, cgp->wid, cgp->hit);
    guibg[ee] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[ee]);
  }

  our_eip=-5;
  for (ee=0;ee<game.numinvitems;ee++) {
    if (game.invinfo[ee].flags & IFLG_STARTWITH) playerchar->inv[ee]=1;
    else playerchar->inv[ee]=0;
  }
  play.score=0;
  play.sierra_inv_color=7;
  play.talkanim_speed = 5;
  play.inv_item_wid = 40;
  play.inv_item_hit = 22;
  play.messagetime=-1;
  play.disabled_user_interface=0;
  play.gscript_timer=-1;
  play.debug_mode=game.options[OPT_DEBUGMODE];
  play.inv_top=0;
  play.inv_numdisp=0;
  play.obsolete_inv_numorder=0;
  play.text_speed=15;
  play.text_min_display_time_ms = 1000;
  play.ignore_user_input_after_text_timeout_ms = 500;
  play.ignore_user_input_until_time = 0;
  play.lipsync_speed = 15;
  play.close_mouth_speech_time = 10;
  play.disable_antialiasing = 0;
  play.rtint_level = 0;
  play.rtint_light = 255;
  play.text_speed_modifier = 0;
  play.text_align = SCALIGN_LEFT;
  // Make the default alignment to the right with right-to-left text
  if (game.options[OPT_RIGHTLEFTWRITE])
    play.text_align = SCALIGN_RIGHT;

  play.speech_bubble_width = get_fixed_pixel_size(100);
  play.bg_frame=0;
  play.bg_frame_locked=0;
  play.bg_anim_delay=0;
  play.anim_background_speed = 0;
  play.silent_midi = 0;
  play.current_music_repeating = 0;
  play.skip_until_char_stops = -1;
  play.get_loc_name_last_time = -1;
  play.get_loc_name_save_cursor = -1;
  play.restore_cursor_mode_to = -1;
  play.restore_cursor_image_to = -1;
  play.ground_level_areas_disabled = 0;
  play.next_screen_transition = -1;
  play.temporarily_turned_off_character = -1;
  play.inv_backwards_compatibility = 0;
  play.gamma_adjustment = 100;
  play.num_do_once_tokens = 0;
  play.do_once_tokens = NULL;
  play.music_queue_size = 0;
  play.shakesc_length = 0;
  play.wait_counter=0;
  play.key_skip_wait = 0;
  play.cur_music_number=-1;
  play.music_repeat=1;
  play.music_master_volume=160;
  play.digital_master_volume = 100;
  play.screen_flipped=0;
  play.offsets_locked=0;
  play.cant_skip_speech = user_to_internal_skip_speech(game.options[OPT_NOSKIPTEXT]);
  play.sound_volume = 255;
  play.speech_volume = 255;
  play.normal_font = 0;
  play.speech_font = 1;
  play.speech_text_shadow = 16;
  play.screen_tint = -1;
  play.bad_parsed_word[0] = 0;
  play.swap_portrait_side = 0;
  play.swap_portrait_lastchar = -1;
  play.in_conversation = 0;
  play.skip_display = 3;
  play.no_multiloop_repeat = 0;
  play.in_cutscene = 0;
  play.fast_forward = 0;
  play.totalscore = game.totalscore;
  play.roomscript_finished = 0;
  play.no_textbg_when_voice = 0;
  play.max_dialogoption_width = get_fixed_pixel_size(180);
  play.no_hicolor_fadein = 0;
  play.bgspeech_game_speed = 0;
  play.bgspeech_stay_on_display = 0;
  play.unfactor_speech_from_textlength = 0;
  play.mp3_loop_before_end = 70;
  play.speech_music_drop = 60;
  play.room_changes = 0;
  play.check_interaction_only = 0;
  play.replay_hotkey = 318;  // Alt+R
  play.dialog_options_x = 0;
  play.dialog_options_y = 0;
  play.min_dialogoption_width = 0;
  play.disable_dialog_parser = 0;
  play.ambient_sounds_persist = 0;
  play.screen_is_faded_out = 0;
  play.player_on_region = 0;
  play.top_bar_backcolor = 8;
  play.top_bar_textcolor = 16;
  play.top_bar_bordercolor = 8;
  play.top_bar_borderwidth = 1;
  play.top_bar_ypos = 25;
  play.top_bar_font = -1;
  play.screenshot_width = 160;
  play.screenshot_height = 100;
  play.speech_text_align = SCALIGN_CENTRE;
  play.auto_use_walkto_points = 1;
  play.inventory_greys_out = 0;
  play.skip_speech_specific_key = 0;
  play.abort_key = 324;  // Alt+X
  play.fade_to_red = 0;
  play.fade_to_green = 0;
  play.fade_to_blue = 0;
  play.show_single_dialog_option = 0;
  play.keep_screen_during_instant_transition = 0;
  play.read_dialog_option_colour = -1;
  play.narrator_speech = game.playercharacter;
  play.crossfading_out_channel = 0;
  play.speech_textwindow_gui = game.options[OPT_TWCUSTOM];
  if (play.speech_textwindow_gui == 0)
    play.speech_textwindow_gui = -1;
  strcpy(play.game_name, game.gamename);
  play.lastParserEntry[0] = 0;
  play.follow_change_room_timer = 150;
  for (ee = 0; ee < MAX_BSCENE; ee++) 
    play.raw_modified[ee] = 0;
  play.game_speed_modifier = 0;
  if (debug_flags & DBG_DEBUGMODE)
    play.debug_mode = 1;
  gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);

  memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
  memset(&play.script_timers[0],0,MAX_TIMERS * sizeof(int));
  memset(&play.default_audio_type_volumes[0], -1, MAX_AUDIO_TYPES * sizeof(int));

  // reset graphical script vars (they're still used by some games)
  for (ee = 0; ee < MAXGLOBALVARS; ee++) 
    play.globalvars[ee] = 0;

  for (ee = 0; ee < MAXGLOBALSTRINGS; ee++)
    play.globalstrings[ee][0] = 0;

  for (ee = 0; ee < MAX_SOUND_CHANNELS; ee++)
    last_sound_played[ee] = -1;

  if (usetup.translation)
    init_translation (usetup.translation);

  update_invorder();
  displayed_room = -10;
}

char filetouse[MAX_PATH] = "nofile";

// Replace the filename part of complete path WASGV with INIFIL
void INIgetdirec(char *wasgv, char *inifil) {
  int u = strlen(wasgv) - 1;

  for (u = strlen(wasgv) - 1; u >= 0; u--) {
    if ((wasgv[u] == '\\') || (wasgv[u] == '/')) {
      memcpy(&wasgv[u + 1], inifil, strlen(inifil) + 1);
      break;
    }
  }

  if (u <= 0) {
    // no slashes - either the path is just "f:acwin.exe"
    if (strchr(wasgv, ':') != NULL)
      memcpy(strchr(wasgv, ':') + 1, inifil, strlen(inifil) + 1);
    // or it's just "acwin.exe" (unlikely)
    else
      strcpy(wasgv, inifil);
  }

}

char *INIreaditem(const char *sectn, const char *entry) {
  FILE *fin = fopen(filetouse, "rt");
  if (fin == NULL)
    return NULL;

  char templine[200];
  char wantsect[100];
  sprintf (wantsect, "[%s]", sectn);

  while (!feof(fin)) {
    fgets (templine, 199, fin);
    // find the section
    if (strnicmp (wantsect, templine, strlen(wantsect)) == 0) {
      while (!feof(fin)) {
        // we're in the right section, find the entry
        fgets (templine, 199, fin);
        if (templine[0] == '[')
          break;
        if (feof(fin))
          break;
        // Strip CRLF
        char *lastchar = &templine[strlen(templine) -1];
        while(*lastchar == '\r' || *lastchar == '\n') {
          *lastchar = 0;
          lastchar--;
        }
        // Have we found the entry?
        if (strnicmp (templine, entry, strlen(entry)) == 0) {
          char *pptr = &templine[strlen(entry)];
          while ((pptr[0] == ' ') || (pptr[0] == '\t'))
            pptr++;
          if (pptr[0] == '=') {
            pptr++;
            while ((pptr[0] == ' ') || (pptr[0] == '\t'))
              pptr++;
            char *toret = (char*)malloc (strlen(pptr) + 5);
            strcpy (toret, pptr);
            fclose (fin);
            return toret;
          }
        }
      }
    }
  }
  fclose (fin);
  return NULL;
}

int INIreadint (const char *sectn, const char *item, int errornosect = 1) {
  char *tempstr = INIreaditem (sectn, item);
  if (tempstr == NULL)
    return -1;

  int toret = atoi(tempstr);
  free (tempstr);
  return toret;
}


//char datname[80]="ac.clb";
char ac_conf_file_defname[MAX_PATH] = "acsetup.cfg";
char *ac_config_file = &ac_conf_file_defname[0];
char conffilebuf[512];


#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
int psp_video_framedrop = 1;
int psp_audio_enabled = 1;
int psp_midi_enabled = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;

int psp_midi_preload_patches = 0;
int psp_audio_cachesize = 10;
char psp_game_file_name[] = "ac2game.dat";
int psp_gfx_smooth_sprites = 1;
char psp_translation[] = "default";
#endif


void read_config_file(char *argv0) {

  // Try current directory for config first; else try exe dir
  strcpy (ac_conf_file_defname, "acsetup.cfg");
  ac_config_file = &ac_conf_file_defname[0];
  FILE *ppp = fopen(ac_config_file, "rb");
  if (ppp == NULL) {

    strcpy(conffilebuf,argv0);
    
/*    for (int ee=0;ee<(int)strlen(conffilebuf);ee++) {
      if (conffilebuf[ee]=='/') conffilebuf[ee]='\\';
    }*/
    fix_filename_case(conffilebuf);
    fix_filename_slashes(conffilebuf);
    
    INIgetdirec(conffilebuf,ac_config_file);
//    printf("Using config: '%s'\n",conffilebuf);
    ac_config_file=&conffilebuf[0];
  }
  else {
    fclose(ppp);
    // put the full path, or it gets written back to the Windows folder
    _getcwd (ac_config_file, 255);
    strcat (ac_config_file, "\\acsetup.cfg");
    fix_filename_case(ac_config_file);
    fix_filename_slashes(ac_config_file);
  }

  // set default dir if no config file
  usetup.data_files_dir = ".";
  usetup.translation = NULL;
  usetup.main_data_filename = "ac2game.dat";
#ifdef WINDOWS_VERSION
  usetup.digicard = DIGI_DIRECTAMX(0);
#endif

  // Don't read in the standard config file if disabled.
  if (psp_ignore_acsetup_cfg_file)
  {
    usetup.gfxDriverID = "DX5";
    usetup.enable_antialiasing = psp_gfx_smooth_sprites;
    usetup.translation = psp_translation;
    return;
  }

  ppp=fopen(ac_config_file,"rt");
  if (ppp!=NULL) {
    strcpy(filetouse,ac_config_file);
    fclose(ppp);
#ifndef WINDOWS_VERSION
    usetup.digicard=INIreadint("sound","digiid");
    usetup.midicard=INIreadint("sound","midiid");
#else
    int idx = INIreadint("sound","digiwinindx", 0);
    if (idx == 0)
      idx = DIGI_DIRECTAMX(0);
    else if (idx == 1)
      idx = DIGI_WAVOUTID(0);
    else if (idx == 2)
      idx = DIGI_NONE;
    else if (idx == 3) 
      idx = DIGI_DIRECTX(0);
    else 
      idx = DIGI_AUTODETECT;
    usetup.digicard = idx;

    idx = INIreadint("sound","midiwinindx", 0);
    if (idx == 1)
      idx = MIDI_NONE;
    else if (idx == 2)
      idx = MIDI_WIN32MAPPER;
    else
      idx = MIDI_AUTODETECT;
    usetup.midicard = idx;

    if (usetup.digicard < 0)
      usetup.digicard = DIGI_AUTODETECT;
    if (usetup.midicard < 0)
      usetup.midicard = MIDI_AUTODETECT;
#endif

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
    usetup.windowed = INIreadint("misc","windowed");
    if (usetup.windowed < 0)
      usetup.windowed = 0;
#endif

    usetup.refresh = INIreadint ("misc", "refresh", 0);
    usetup.enable_antialiasing = INIreadint ("misc", "antialias", 0);
    usetup.force_hicolor_mode = INIreadint("misc", "notruecolor", 0);
    usetup.enable_side_borders = INIreadint("misc", "sideborders", 0);

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
    // PSP: Letterboxing is not useful on the PSP.
    force_letterbox = 0;
#else
    force_letterbox = INIreadint ("misc", "forceletterbox", 0);
#endif

    if (usetup.enable_antialiasing < 0)
      usetup.enable_antialiasing = 0;
    if (usetup.force_hicolor_mode < 0)
      usetup.force_hicolor_mode = 0;
    if (usetup.enable_side_borders < 0)
      usetup.enable_side_borders = 1;

    // This option is backwards (usevox is 0 if no_speech_pack)
    usetup.no_speech_pack = INIreadint ("sound", "usespeech", 0);
    if (usetup.no_speech_pack == 0)
      usetup.no_speech_pack = 1;
    else
      usetup.no_speech_pack = 0;

    usetup.data_files_dir = INIreaditem("misc","datadir");
    if (usetup.data_files_dir == NULL)
      usetup.data_files_dir = ".";
    // strip any trailing slash
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
    if (usetup.data_files_dir[strlen(usetup.data_files_dir)-1] == '/')
      usetup.data_files_dir[strlen(usetup.data_files_dir)-1] = 0;
#else
    if ((strlen(usetup.data_files_dir) < 4) && (usetup.data_files_dir[1] == ':'))
    { }  // if the path is just  d:\  don't strip the slash
    else if (usetup.data_files_dir[strlen(usetup.data_files_dir)-1] == '\\')
      usetup.data_files_dir[strlen(usetup.data_files_dir)-1] = 0;
#endif

    usetup.main_data_filename = INIreaditem ("misc", "datafile");
    if (usetup.main_data_filename == NULL)
      usetup.main_data_filename = "ac2game.dat";

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
    // PSP: No graphic filters are available.
    usetup.gfxFilterID = NULL;
#else
    usetup.gfxFilterID = INIreaditem("misc", "gfxfilter");
#endif

#if defined(IOS_VERSION) || defined(PSP_VERSION) || defined(ANDROID_VERSION)
    usetup.gfxDriverID = "DX5";
#else
    usetup.gfxDriverID = INIreaditem("misc", "gfxdriver");
#endif

    usetup.translation = INIreaditem ("language", "translation");

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
    // PSP: Don't let the setup determine the cache size as it is always too big.
    int tempint = INIreadint ("misc", "cachemax");
    if (tempint > 0)
      spriteset.maxCacheSize = tempint * 1024;
#endif

    char *repfile = INIreaditem ("misc", "replay");
    if (repfile != NULL) {
      strcpy (replayfile, repfile);
      free (repfile);
      play.playback = 1;
    }
    else
      play.playback = 0;

  }

  if (usetup.gfxDriverID == NULL)
    usetup.gfxDriverID = "DX5";

}

void start_game() {
  set_cursor_mode(MODE_WALK);
  filter->SetMousePosition(160,100);
  newmusic(0);

  our_eip = -42;

  for (int kk = 0; kk < numScriptModules; kk++)
    run_text_script(moduleInst[kk], "game_start");

  run_text_script(gameinst,"game_start");

  our_eip = -43;

  SetRestartPoint();

  our_eip=-3;

  if (displayed_room < 0) {
    current_fade_out_effect();
    load_new_room(playerchar->room,playerchar);
    // load_new_room updates it, but it should be -1 in the first room
    playerchar->prevroom = -1;
  }

  first_room_initialization();
}

void initialize_start_and_play_game(int override_start_room, const char *loadSaveGameOnStartup)
{
  try { // BEGIN try for ALI3DEXception
  
  set_cursor_mode (MODE_WALK);

  if (convert_16bit_bgr) {
    // Disable text as speech while displaying the warning message
    // This happens if the user's graphics card does BGR order 16-bit colour
    int oldalways = game.options[OPT_ALWAYSSPCH];
    game.options[OPT_ALWAYSSPCH] = 0;
    // PSP: This is normal. Don't show a warning.
    //Display ("WARNING: AGS has detected that you have an incompatible graphics card for this game. You may experience colour problems during the game. Try running the game with \"--15bit\" command line parameter and see if that helps.[[Click the mouse to continue.");
    game.options[OPT_ALWAYSSPCH] = oldalways;
  }

  srand (play.randseed);
  play.gamestep = 0;
  if (override_start_room)
    playerchar->room = override_start_room;

  write_log_debug("Checking replay status");

  if (play.recording) {
    start_recording();
  }
  else if (play.playback) {
    FILE *in = fopen(replayfile, "rb");
    if (in != NULL) {
      char buffer [100];
      fread (buffer, 12, 1, in);
      buffer[12] = 0;
      if (strcmp (buffer, "AGSRecording") != 0) {
        Display("ERROR: Invalid recorded data file");
        play.playback = 0;
      }
      else {
        fgetstring_limit (buffer, in, 12);
        if (buffer[0] != '2') 
          quit("!Replay file is from an old version of AGS");
        if (strcmp (buffer, "2.55.553") < 0)
          quit("!Replay file was recorded with an older incompatible version");

        if (strcmp (buffer, ACI_VERSION_TEXT)) {
          // Disable text as speech while displaying the warning message
          // This happens if the user's graphics card does BGR order 16-bit colour
          int oldalways = game.options[OPT_ALWAYSSPCH];
          game.options[OPT_ALWAYSSPCH] = 0;
          play.playback = 0;
          Display("Warning! replay is from a different version of AGS (%s) - it may not work properly.", buffer);
          play.playback = 1;
          srand (play.randseed);
          play.gamestep = 0;
          game.options[OPT_ALWAYSSPCH] = oldalways;
        }

        int replayver = getw(in);

        if ((replayver < 1) || (replayver > 3))
          quit("!Unsupported Replay file version");

        if (replayver >= 2) {
          fgetstring_limit (buffer, in, 99);
          int uid = getw (in);
          if ((strcmp (buffer, game.gamename) != 0) || (uid != game.uniqueid)) {
            char msg[150];
            sprintf (msg, "!This replay is meant for the game '%s' and will not work correctly with this game.", buffer);
            quit (msg);
          }
          // skip the total time
          getw (in);
          // replay description, maybe we'll use this later
          fgetstring_limit (buffer, in, 99);
        }

        play.randseed = getw(in);
        int flen = filelength(fileno(in)) - ftell (in);
        if (replayver >= 3) {
          flen = getw(in) * sizeof(short);
        }
        recordbuffer = (short*)malloc (flen);
        fread (recordbuffer, flen, 1, in);
        srand (play.randseed);
        recbuffersize = flen / sizeof(short);
        recsize = 0;
        disable_mgetgraphpos = 1;
        replay_time = 0;
        replay_last_second = loopcounter;
        if (replayver >= 3) {
          int issave = getw(in);
          if (issave) {
            if (restore_game_data (in, replayfile))
              quit("!Error running replay... could be incorrect game version");
            replay_last_second = loopcounter;
          }
        }
        fclose (in);
      }
    }
    else // file not found
      play.playback = 0;
  }

  write_log_debug("Engine initialization complete");
  write_log_debug("Starting game");
  
  if (editor_debugging_enabled)
  {
    SetMultitasking(1);
    if (init_editor_debugging())
    {
      timerloop = 0;
      while (timerloop < 20)
      {
        // pick up any breakpoints in game_start
        check_for_messages_from_editor();
      }

      ccSetDebugHook(scriptDebugHook);
    }
  }

  if (loadSaveGameOnStartup != NULL)
  {
    int saveGameNumber = 1000;
    const char *sgName = strstr(loadSaveGameOnStartup, "agssave.");
    if (sgName != NULL)
    {
      sscanf(sgName, "agssave.%03d", &saveGameNumber);
    }
    current_fade_out_effect();
    int loadGameErrorCode = do_game_load(loadSaveGameOnStartup, saveGameNumber, NULL, NULL);
    if (loadGameErrorCode)
    {
      quitprintf("Unable to resume the save game. Try starting the game over. (Error: %s)", load_game_errors[-loadGameErrorCode]);
    }
  }

  // only start if replay playback hasn't loaded a game
  if (displayed_room < 0)
    start_game();

  while (!abort_engine) {
    main_game_loop();

    if (load_new_game) {
      RunAGSGame (NULL, load_new_game, 0);
      load_new_game = 0;
    }
  }

  } catch (Ali3DException gfxException)
  {
    quit((char*)gfxException._message);
  }

}

int initialize_graphics_filter(const char *filterID, int width, int height, int colDepth)
{
  int idx = 0;
  GFXFilter **filterList;

  if (stricmp(usetup.gfxDriverID, "D3D9") == 0)
  {
    filterList = get_d3d_gfx_filter_list(false);
  }
  else
  {
    filterList = get_allegro_gfx_filter_list(false);
  }
  
  // by default, select No Filter
  filter = filterList[0];

  GFXFilter *thisFilter = filterList[idx];
  while (thisFilter != NULL) {

    if ((filterID != NULL) &&
        (strcmp(thisFilter->GetFilterID(), filterID) == 0))
      filter = thisFilter;
    else if (idx > 0)
      delete thisFilter;

    idx++;
    thisFilter = filterList[idx];
  }

  const char *filterError = filter->Initialize(width, height, colDepth);
  if (filterError != NULL) {
    proper_exit = 1;
    platform->DisplayAlert("Unable to initialize the graphics filter. It returned the following error:\n'%s'\n\nTry running Setup and selecting a different graphics filter.", filterError);
    return -1;
  }

  return 0;
}

int try_widescreen_bordered_graphics_mode_if_appropriate(int initasx, int initasy, int firstDepth)
{
  if (working_gfx_mode_status == 0) return 0;
  if (usetup.enable_side_borders == 0)
  {
    platform->WriteDebugString("Widescreen side borders: disabled in Setup");
    return 1;
  }
  if (usetup.windowed > 0)
  {
    platform->WriteDebugString("Widescreen side borders: disabled (windowed mode)");
    return 1;
  }

  int failed = 1;
  int desktopWidth, desktopHeight;
  if (get_desktop_resolution(&desktopWidth, &desktopHeight) == 0)
  {
    int gameHeight = initasy;

    int screenRatio = (desktopWidth * 1000) / desktopHeight;
    int gameRatio = (initasx * 1000) / gameHeight;
    // 1250 = 1280x1024 
    // 1333 = 640x480, 800x600, 1024x768, 1152x864, 1280x960
    // 1600 = 640x400, 960x600, 1280x800, 1680x1050
    // 1666 = 1280x768

    platform->WriteDebugString("Widescreen side borders: game resolution: %d x %d; desktop resolution: %d x %d", initasx, gameHeight, desktopWidth, desktopHeight);

    if ((screenRatio > 1500) && (gameRatio < 1500))
    {
      int tryWidth = (initasx * screenRatio) / gameRatio;
      int supportedRes = gfxDriver->FindSupportedResolutionWidth(tryWidth, gameHeight, firstDepth, 110);
      if (supportedRes > 0)
      {
        tryWidth = supportedRes;
        platform->WriteDebugString("Widescreen side borders: enabled, attempting resolution %d x %d", tryWidth, gameHeight);
      }
      else
      {
        platform->WriteDebugString("Widescreen side borders: gfx card does not support suitable resolution. will attempt %d x %d anyway", tryWidth, gameHeight);
      }
      failed = init_gfx_mode(tryWidth, gameHeight, firstDepth);
    }
    else
    {
      platform->WriteDebugString("Widescreen side borders: disabled (not necessary, game and desktop aspect ratios match)", initasx, gameHeight, desktopWidth, desktopHeight);
    }
  }
  else 
  {
    platform->WriteDebugString("Widescreen side borders: disabled (unable to obtain desktop resolution)");
  }
  return failed;
}

int switch_to_graphics_mode(int initasx, int initasy, int scrnwid, int scrnhit, int firstDepth, int secondDepth) 
{
  int failed;
  int initasyLetterbox = (initasy * 12) / 10;

  // first of all, try 16-bit normal then letterboxed
  if (game.options[OPT_LETTERBOX] == 0) 
  {
    failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, firstDepth);
    failed = init_gfx_mode(initasx,initasy, firstDepth);
  }
  failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, firstDepth);
  failed = init_gfx_mode(initasx, initasyLetterbox, firstDepth);

  if (secondDepth != firstDepth) {
    // now, try 15-bit normal then letterboxed
    if (game.options[OPT_LETTERBOX] == 0) 
    {
      failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, secondDepth);
      failed = init_gfx_mode(initasx,initasy, secondDepth);
    }
    failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, secondDepth);
    failed = init_gfx_mode(initasx, initasyLetterbox, secondDepth);
  }

  if ((scrnwid != initasx) || (scrnhit != initasy))
  {
    // now, try the original resolution at 16 then 15 bit
    failed = init_gfx_mode(scrnwid,scrnhit,firstDepth);
    failed = init_gfx_mode(scrnwid,scrnhit, secondDepth);
  }
  
  if (failed)
    return -1;

  return 0;
}

void CreateBlankImage()
{
  // this is the first time that we try to use the graphics driver,
  // so it's the most likey place for a crash
  try
  {
    BITMAP *blank = create_bitmap_ex(final_col_dep, 16, 16);
    blank = gfxDriver->ConvertBitmapToSupportedColourDepth(blank);
    clear(blank);
    blankImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
    blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
    destroy_bitmap(blank);
  }
  catch (Ali3DException gfxException)
  {
    quit((char*)gfxException._message);
  }

}

void show_preload () {
  // ** Do the preload graphic if available
  color temppal[256];
  block splashsc = load_pcx("preload.pcx",temppal);
  if (splashsc != NULL) {
    if (bitmap_color_depth(splashsc) == 8)
      wsetpalette(0,255,temppal);
    block tsc = create_bitmap_ex(bitmap_color_depth(screen),splashsc->w,splashsc->h);
    blit(splashsc,tsc,0,0,0,0,tsc->w,tsc->h);
    clear(screen);
    stretch_sprite(screen, tsc, 0, 0, scrnwid,scrnhit);

    gfxDriver->ClearDrawList();

    if (!gfxDriver->UsesMemoryBackBuffer())
    {
      IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(screen, false, true);
      gfxDriver->DrawSprite(0, 0, ddb);
      render_to_screen(screen, 0, 0);
      gfxDriver->DestroyDDB(ddb);
    }
    else
      render_to_screen(screen, 0, 0);

    wfreeblock(splashsc);
    wfreeblock(tsc);
    platform->Delay(500);
  }
}

void change_to_directory_of_file(LPCWSTR fileName)
{
  WCHAR wcbuffer[MAX_PATH];
  StrCpyW(wcbuffer, fileName);

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
    if (strrchr(wcbuffer, '/') != NULL) {
      strrchr(wcbuffer, '/')[0] = 0;
      chdir(wcbuffer);
    }
#else
  LPWSTR backSlashAt = StrRChrW(wcbuffer, NULL, L'\\');
  if (backSlashAt != NULL) {
      wcbuffer[wcslen(wcbuffer) - wcslen(backSlashAt)] = L'\0';
      SetCurrentDirectoryW(wcbuffer);
    }
#endif
}


// Startup flags, set from parameters to engine
int datafile_argv=0, change_to_game_dir = 0, force_window = 0;
int override_start_room = 0, force_16bit = 0;
bool justRegisterGame = false;
bool justUnRegisterGame = false;
const char *loadSaveGameOnStartup = NULL;
#ifndef WINDOWS_VERSION
char **global_argv = 0;
#endif

void initialise_game_file_name()
{
#ifdef WINDOWS_VERSION
  WCHAR buffer[MAX_PATH];
  LPCWSTR dataFilePath = wArgv[datafile_argv];
  // Hack for Windows in case there are unicode chars in the path.
  // The normal argv[] array has ????? instead of the unicode chars
  // and fails, so instead we manually get the short file name, which
  // is always using ANSI chars.
  if (wcschr(dataFilePath, '\\') == NULL)
  {
    GetCurrentDirectoryW(MAX_PATH, buffer);
    wcscat(buffer, L"\\");
    wcscat(buffer, dataFilePath);
    dataFilePath = &buffer[0];
  }
  if (GetShortPathNameW(dataFilePath, directoryPathBuffer, MAX_PATH) == 0)
  {
    platform->DisplayAlert("Unable to determine startup path: GetShortPathNameW failed. The specified game file might be missing.");
    game_file_name = NULL;
    return;
  }
  game_file_name = (char*)malloc(MAX_PATH);
  WideCharToMultiByte(CP_ACP, 0, directoryPathBuffer, -1, game_file_name, MAX_PATH, NULL, NULL);
#elif defined(PSP_VERSION) || defined(ANDROID_VERSION) || defined(IOS_VERSION)
  game_file_name = psp_game_file_name;
#else
  game_file_name = (char*)malloc(MAX_PATH);
  strcpy(game_file_name, get_filename(global_argv[datafile_argv]));
#endif
}

extern "C" int  cfopenpriority;

int main(int argc,char*argv[]) { 
  our_eip = -999;
  cfopenpriority=2;
  int ee;
  play.recording = 0;
  play.playback = 0;
  play.takeover_data = 0;

  platform = AGSPlatformDriver::GetDriver();

#ifdef WINDOWS_VERSION
  wArgv = CommandLineToArgvW(GetCommandLineW(), &wArgc);
  if (wArgv == NULL)
  {
    platform->DisplayAlert("CommandLineToArgvW failed, unable to start the game.");
    return 9;
  }
#else
  global_argv = argv;
#endif

  print_welcome_text(AC_VERSION_TEXT,ACI_VERSION_TEXT);
  if ((argc>1) && (argv[1][1]=='?'))
    return 0;

  write_log_debug("***** ENGINE STARTUP");

#if defined(WINDOWS_VERSION)
  _set_new_handler(malloc_fail_handler);
  _set_new_mode(1);
  printfworkingspace=(char*)malloc(7000);
#endif
  debug_flags=0;

  for (ee=1;ee<argc;ee++) {
    if (argv[ee][1]=='?') return 0;
    if (stricmp(argv[ee],"-shelllaunch") == 0)
      change_to_game_dir = 1;
    else if (stricmp(argv[ee],"-updatereg") == 0)
      debug_flags |= DBG_REGONLY;
    else if (stricmp(argv[ee],"-windowed") == 0)
      force_window = 1;
    else if (stricmp(argv[ee],"-fullscreen") == 0)
      force_window = 2;
    else if (stricmp(argv[ee],"-hicolor") == 0)
      force_16bit = 1;
    else if (stricmp(argv[ee],"-letterbox") == 0)
      force_letterbox = 1;
    else if (stricmp(argv[ee],"-record") == 0)
      play.recording = 1;
    else if (stricmp(argv[ee],"-playback") == 0)
      play.playback = 1;
#ifdef _DEBUG
    else if ((stricmp(argv[ee],"--startr") == 0) && (ee < argc-1)) {
      override_start_room = atoi(argv[ee+1]);
      ee++;
    }
#endif
    else if ((stricmp(argv[ee],"--testre") == 0) && (ee < argc-2)) {
      strcpy(return_to_roomedit, argv[ee+1]);
      strcpy(return_to_room, argv[ee+2]);
      ee+=2;
    }
    else if (stricmp(argv[ee],"--15bit")==0) debug_15bit_mode = 1;
    else if (stricmp(argv[ee],"--24bit")==0) debug_24bit_mode = 1;
    else if (stricmp(argv[ee],"--fps")==0) display_fps = 2;
    else if (stricmp(argv[ee],"--test")==0) debug_flags|=DBG_DEBUGMODE;
    else if (stricmp(argv[ee],"-noiface")==0) debug_flags|=DBG_NOIFACE;
    else if (stricmp(argv[ee],"-nosprdisp")==0) debug_flags|=DBG_NODRAWSPRITES;
    else if (stricmp(argv[ee],"-nospr")==0) debug_flags|=DBG_NOOBJECTS;
    else if (stricmp(argv[ee],"-noupdate")==0) debug_flags|=DBG_NOUPDATE;
    else if (stricmp(argv[ee],"-nosound")==0) debug_flags|=DBG_NOSFX;
    else if (stricmp(argv[ee],"-nomusic")==0) debug_flags|=DBG_NOMUSIC;
    else if (stricmp(argv[ee],"-noscript")==0) debug_flags|=DBG_NOSCRIPT;
    else if (stricmp(argv[ee],"-novideo")==0) debug_flags|=DBG_NOVIDEO;
    else if (stricmp(argv[ee],"-noexceptionhandler")==0) usetup.disable_exception_handling = 1;
    else if (stricmp(argv[ee],"-dbgscript")==0) debug_flags|=DBG_DBGSCRIPT;
    else if (stricmp(argv[ee],"-registergame") == 0)
    {
      justRegisterGame = true;
    }
    else if (stricmp(argv[ee],"-unregistergame") == 0)
    {
      justUnRegisterGame = true;
    }
    else if ((stricmp(argv[ee],"-loadsavedgame") == 0) && (argc > ee + 1))
    {
      loadSaveGameOnStartup = argv[ee + 1];
      ee++;
    }
    else if ((stricmp(argv[ee],"--enabledebugger") == 0) && (argc > ee + 1))
    {
      strcpy(editor_debugger_instance_token, argv[ee + 1]);
      editor_debugging_enabled = 1;
      force_window = 1;
      ee++;
    }
    else if (stricmp(argv[ee],"--takeover")==0) {
      if (argc < ee+2)
        break;
      play.takeover_data = atoi (argv[ee + 1]);
      strncpy (play.takeover_from, argv[ee + 2], 49);
      play.takeover_from[49] = 0;
      ee += 2;
    }
    else if (argv[ee][0]!='-') datafile_argv=ee;
  }
  

#ifdef _DEBUG
  /* logfile=fopen("g:\\ags.log","at");
   //_CrtSetReportHook( OurReportingFunction );
    int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    //tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF;

    tmpDbgFlag = (tmpDbgFlag & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF | _CRTDBG_DELAY_FREE_MEM_DF;

    _CrtSetDbgFlag(tmpDbgFlag);

/*
//  _CrtMemState memstart,memnow;
  _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
  _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_WNDW );
  _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_WNDW );
/*
//   _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
//   _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
//   _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );

//  _CrtMemCheckpoint(&memstart);
//  _CrtMemDumpStatistics( &memstart );*/
#endif

  if ((loadSaveGameOnStartup != NULL) && (argv[0] != NULL))
  {
    // When launched by double-clicking a save game file, the curdir will
    // be the save game folder unless we correct it
    change_to_directory_of_file(wArgv[0]);
  }
 
#ifdef MAC_VERSION
  getcwd(appDirectory, 512);
#endif
 
  //if (change_to_game_dir == 1)  {
  if (datafile_argv > 0) {
    // If launched by double-clicking .AGS file, change to that
    // folder; else change to this exe's folder
    change_to_directory_of_file(wArgv[datafile_argv]);
  }

#ifdef MAC_VERSION
  getcwd(dataDirectory, 512);
#endif
 
  // Update shell associations and exit
  if (debug_flags & DBG_REGONLY)
    exit(0);

#ifndef USE_CUSTOM_EXCEPTION_HANDLER
  usetup.disable_exception_handling = 1;
#endif

  if (usetup.disable_exception_handling)
  {
    initialize_engine(argc, argv);
    platform->PostAllegroExit();
  }
  else
  {
    return initialize_engine_with_exception_handling(argc, argv);
  }
}

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
extern int psp_gfx_smoothing;
extern int psp_gfx_scaling;
extern int psp_gfx_renderer;
extern int psp_gfx_super_sampling;
#endif

void create_gfx_driver() 
{
#ifdef WINDOWS_VERSION
  if (stricmp(usetup.gfxDriverID, "D3D9") == 0)
    gfxDriver = GetD3DGraphicsDriver(filter);
  else
#endif
  {
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
    if ((psp_gfx_renderer > 0) && (game.color_depth != 1))
      gfxDriver = GetOGLGraphicsDriver(filter);
    else
#endif
      gfxDriver = GetSoftwareGraphicsDriver(filter);
  }

  gfxDriver->SetCallbackOnInit(GfxDriverOnInitCallback);
  gfxDriver->SetTintMethod(TintReColourise);
}


extern "C" int csetlib(char *namm, char *passw);

int use_extra_sound_offset = 0;

int initialize_engine(int argc,char*argv[])
{
  FILE*ppp;
  int ee;

  write_log_debug("Reading config file");

  our_eip = -200;
  read_config_file(argv[0]);

  set_uformat(U_ASCII);

  write_log_debug("Initializing allegro");

  our_eip = -199;
  // Initialize allegro
#ifdef WINDOWS_VERSION
  if (install_allegro(SYSTEM_AUTODETECT,&myerrno,atexit)) {
    platform->DisplayAlert("Unable to initialize graphics subsystem. Make sure you have DirectX 5 or above installed.");
#else
  if (install_allegro(SYSTEM_AUTODETECT, &myerrno, atexit)) {
    platform->DisplayAlert("Unknown error initializing graphics subsystem.");
#endif
    return EXIT_NORMAL;
  }

  write_log_debug("Setting up window");

  our_eip = -198;
#if (ALLEGRO_DATE > 19990103)
  set_window_title("Adventure Game Studio");
#if (ALLEGRO_DATE > 20021115)
  set_close_button_callback (winclosehook);
#else
  set_window_close_hook (winclosehook);
#endif

  our_eip = -197;
#endif

  platform->SetGameWindowIcon();

  our_eip = -196;

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
  // check if Setup needs to be run instead
  if (argc>1) {
    if (stricmp(argv[1],"--setup")==0) { 
      write_log_debug("Running Setup");

      if (!platform->RunSetup())
        return EXIT_NORMAL;

#ifndef WINDOWS_VERSION
#define _spawnl spawnl
#define _P_OVERLAY P_OVERLAY
#endif
      // Just re-reading the config file seems to cause a caching
      // problem on Win9x, so let's restart the process.
      allegro_exit();
      char quotedpath[255];
      sprintf (quotedpath, "\"%s\"", argv[0]);
      _spawnl (_P_OVERLAY, argv[0], quotedpath, NULL);
      //read_config_file(argv[0]);
    }
  }
#endif

  // Force to run in a window, override the config file
  if (force_window == 1)
    usetup.windowed = 1;
  else if (force_window == 2)
    usetup.windowed = 0;
  
  our_eip = -195;

  write_log_debug("Initializing game data");

  // initialize the data file
  initialise_game_file_name();
  if (game_file_name == NULL) return EXIT_NORMAL;

  int errcod = csetlib(game_file_name,"");  // assume it's appended to exe

  our_eip = -194;
//  char gamefilenamebuf[200];
  if ((errcod!=0) && (change_to_game_dir == 0)) {
    // it's not, so look for the file
    
    game_file_name = ci_find_file(usetup.data_files_dir, usetup.main_data_filename);

#if !defined(WINDOWS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION) && !defined(IOS_VERSION)
    // Search the exe files for the game data
    if ((game_file_name == NULL) || (access(game_file_name, F_OK) != 0))
    {
      DIR* fd = NULL;
      struct dirent* entry = NULL;
      version_info_t version_info;

      if ((fd = opendir(".")))
      {
        while ((entry = readdir(fd)))
        {
          // Exclude the setup program
          if (stricmp(entry->d_name, "winsetup.exe") == 0)
            continue;

          // Filename must be >= 4 chars long
          int length = strlen(entry->d_name);
          if (length < 4)
            continue;
  
          if (stricmp(&(entry->d_name[length - 4]), ".exe") == 0)
          {
            if (!getVersionInformation(entry->d_name, &version_info))
              continue;
            if (strcmp(version_info.internal_name, "acwin") == 0)
            {
              game_file_name = (char*)malloc(strlen(entry->d_name) + 1);
              strcpy(game_file_name, entry->d_name);
              break;
            }
          }
        }
        closedir(fd);
      }
    }
#endif

    errcod=csetlib(game_file_name,"");
    if (errcod) {
      //sprintf(gamefilenamebuf,"%s\\ac2game.ags",usetup.data_files_dir);
      free(game_file_name);
      game_file_name = ci_find_file(usetup.data_files_dir, "ac2game.ags");
      
      errcod = csetlib(game_file_name,"");
    }
  }
  else {
    // set the data filename to the EXE name
    
    usetup.main_data_filename = get_filename(game_file_name);

    if (((strchr(game_file_name, '/') != NULL) ||
         (strchr(game_file_name, '\\') != NULL)) &&
         (stricmp(usetup.data_files_dir, ".") == 0)) {
      // there is a path in the game file name (and the user
      // has not specified another one)
      // save the path, so that it can load the VOX files, etc
      usetup.data_files_dir = (char*)malloc(strlen(game_file_name) + 1);
      strcpy(usetup.data_files_dir, game_file_name);
    
      if (strrchr(usetup.data_files_dir, '/') != NULL)
        strrchr(usetup.data_files_dir, '/')[0] = 0;
      else if (strrchr(usetup.data_files_dir, '\\') != NULL)
        strrchr(usetup.data_files_dir, '\\')[0] = 0;
      else {
        platform->DisplayAlert("Error processing game file name: slash but no slash");
        return EXIT_NORMAL;
      }
    }

  }

  our_eip = -193;

  if (errcod!=0) {  // there's a problem
    if (errcod==-1) {  // file not found
      char emsg[STD_BUFFER_SIZE];
      sprintf (emsg,
        "You must create and save a game first in the AGS Editor before you can use "
        "this engine.\n\n"
        "If you have just downloaded AGS, you are probably running the wrong executable.\n"
        "Run AGSEditor.exe to launch the editor.\n\n"
        "(Unable to find '%s')\n", argv[datafile_argv]);
      platform->DisplayAlert(emsg);
      }
    else if (errcod==-4)
      platform->DisplayAlert("ERROR: Too many files in data file.");
    else platform->DisplayAlert("ERROR: The file is corrupt. Make sure you have the correct version of the\n"
        "editor, and that this really is an AGS game.\n");
    return EXIT_NORMAL;
  }

  our_eip = -192;

  write_log_debug("Initializing TTF renderer");

  init_font_renderer();

  our_eip = -188;

  write_log_debug("Initializing mouse");

#ifdef _DEBUG
  // Quantify fails with the mouse for some reason
  minstalled();
#else
  if (minstalled()==0) {
    platform->DisplayAlert(platform->GetNoMouseErrorString());
    return EXIT_NORMAL;
  }
#endif // DEBUG
  our_eip = -187;

  write_log_debug("Checking memory");

  char*memcheck=(char*)malloc(4000000);
  if (memcheck==NULL) {
    platform->DisplayAlert("There is not enough memory available to run this game. You need 4 Mb free\n"
      "extended memory to run the game.\n"
      "If you are running from Windows, check the 'DPMI memory' setting on the DOS box\n"
      "properties.\n");
    return EXIT_NORMAL;
    }
  free(memcheck);
  unlink (replayTempFile);

  write_log_debug("Initializing rooms");

  roomstats=(RoomStatus*)calloc(sizeof(RoomStatus),MAX_ROOMS);
  for (ee=0;ee<MAX_ROOMS;ee++) {
    roomstats[ee].beenhere=0;
    roomstats[ee].numobj=0;
    roomstats[ee].tsdatasize=0;
    roomstats[ee].tsdata=NULL;
    }
  play.want_speech=-2;

  our_eip = -186;
  if (usetup.no_speech_pack == 0) {
    /* Can't just use fopen here, since we need to change the filename
        so that pack functions, etc. will have the right case later */
    speech_file = ci_find_file(usetup.data_files_dir, "speech.vox");
    
    ppp = ci_fopen(speech_file, "rb");

    if (ppp == NULL)
    {
      // In case they're running in debug, check Compiled folder
      free(speech_file);
      speech_file = ci_find_file("Compiled", "speech.vox");
      ppp = ci_fopen(speech_file, "rb");
    }
    
    if (ppp!=NULL) {
      fclose(ppp);

      write_log_debug("Initializing speech vox");

      //if (csetlib(useloc,"")!=0) {
      if (csetlib(speech_file,"")!=0) {
        platform->DisplayAlert("Unable to initialize speech sample file - check for corruption and that\nit belongs to this game.\n");
        return EXIT_NORMAL;
      }
      FILE *speechsync = clibfopen("syncdata.dat", "rb");
      if (speechsync != NULL) {
        // this game has voice lip sync
        if (getw(speechsync) != 4)
        { 
          // Don't display this warning.
          // platform->DisplayAlert("Unknown speech lip sync format (might be from older or newer version); lip sync disabled");
        }
        else {
          numLipLines = getw(speechsync);
          splipsync = (SpeechLipSyncLine*)malloc (sizeof(SpeechLipSyncLine) * numLipLines);
          for (ee = 0; ee < numLipLines; ee++)
          {
            splipsync[ee].numPhenomes = getshort(speechsync);
            fread(splipsync[ee].filename, 1, 14, speechsync);
            splipsync[ee].endtimeoffs = (int*)malloc(splipsync[ee].numPhenomes * sizeof(int));
            fread(splipsync[ee].endtimeoffs, sizeof(int), splipsync[ee].numPhenomes, speechsync);
            splipsync[ee].frame = (short*)malloc(splipsync[ee].numPhenomes * sizeof(short));
            fread(splipsync[ee].frame, sizeof(short), splipsync[ee].numPhenomes, speechsync);
          }
        }
        fclose (speechsync);
      }
      csetlib(game_file_name,"");
      platform->WriteConsole("Speech sample file found and initialized.\n");
      play.want_speech=1;
    }
  }

  our_eip = -185;
  play.seperate_music_lib = 0;

  /* Can't just use fopen here, since we need to change the filename
      so that pack functions, etc. will have the right case later */
  music_file = ci_find_file(usetup.data_files_dir, "audio.vox");

  /* Don't need to use ci_fopen here, because we've used ci_find_file to get
      the case insensitive matched filename already */
  // Use ci_fopen anyway because it can handle NULL filenames.
  ppp = ci_fopen(music_file, "rb");
  
  if (ppp == NULL)
  {
    // In case they're running in debug, check Compiled folder
    free(music_file);
    music_file = ci_find_file("Compiled", "audio.vox");
    ppp = ci_fopen(music_file, "rb");
  }

  if (ppp!=NULL) {
    fclose(ppp);

    write_log_debug("Initializing audio vox");

    //if (csetlib(useloc,"")!=0) {
    if (csetlib(music_file,"")!=0) {
      platform->DisplayAlert("Unable to initialize music library - check for corruption and that\nit belongs to this game.\n");
      return EXIT_NORMAL;
    }
    csetlib(game_file_name,"");
    platform->WriteConsole("Audio vox found and initialized.\n");
    play.seperate_music_lib = 1;
  }

  our_eip = -184;

#ifdef ALLEGRO_KEYBOARD_HANDLER
  write_log_debug("Initializing keyboard");

  install_keyboard();
#endif

  our_eip = -183;

  write_log_debug("Install timer");

  platform->WriteConsole("Checking sound inits.\n");
  if (opts.mod_player) reserve_voices(16,-1);
  // maybe this line will solve the sound volume?
  install_timer();
#if ALLEGRO_DATE > 19991010
  set_volume_per_voice(1);
#endif

  our_eip = -182;

#ifdef WINDOWS_VERSION
  // don't let it use the hardware mixer verion, crashes some systems
  //if ((usetup.digicard == DIGI_AUTODETECT) || (usetup.digicard == DIGI_DIRECTX(0)))
//    usetup.digicard = DIGI_DIRECTAMX(0);

  if (usetup.digicard == DIGI_DIRECTX(0)) {
    // DirectX mixer seems to buffer an extra sample itself
    use_extra_sound_offset = 1;
  }

  // if the user clicked away to another app while we were
  // loading, DirectSound will fail to initialize. There doesn't
  // seem to be a solution to force us back to the foreground,
  // because we have no actual visible window at this time

#endif

  write_log_debug("Initialize sound drivers");

  // PSP: Disable sound by config file.
  if (!psp_audio_enabled)
  {
    usetup.digicard = DIGI_NONE;
    usetup.midicard = MIDI_NONE;
  }

  if (!psp_midi_enabled)
    usetup.midicard = MIDI_NONE;

  if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
    reserve_voices(-1,-1);
    opts.mod_player=0;
    opts.mp3_player=0;
    if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
      if ((usetup.digicard != DIGI_NONE) && (usetup.midicard != MIDI_NONE)) {
        // only flag an error if they wanted a sound card
        platform->DisplayAlert("\nUnable to initialize your audio hardware.\n"
          "[Problem: %s]\n",allegro_error);
      }
      reserve_voices(0,0);
      install_sound(DIGI_NONE, MIDI_NONE, NULL);
      usetup.digicard = DIGI_NONE;
      usetup.midicard = MIDI_NONE;
    }
  }

  our_eip = -181;

  if (usetup.digicard == DIGI_NONE) {
    // disable speech and music if no digital sound
    // therefore the MIDI soundtrack will be used if present,
    // and the voice mode should not go to Voice Only
    play.want_speech = -2;
    play.seperate_music_lib = 0;
  }

  //set_volume(255,-1);
  if ((debug_flags & (~DBG_DEBUGMODE)) >0) {
    platform->DisplayAlert("Engine debugging enabled.\n"
     "\nNOTE: You have selected to enable one or more engine debugging options.\n"
     "These options cause many parts of the game to behave abnormally, and you\n"
     "may not see the game as you are used to it. The point is to test whether\n"
     "the engine passes a point where it is crashing on you normally.\n"
     "[Debug flags enabled: 0x%02X]\n"
     "Press a key to continue.\n",debug_flags);
    }

  our_eip = -10;

  write_log_debug("Install exit handler");

  atexit(atexit_handler);
  unlink("warnings.log");
  play.randseed = time(NULL);
  srand (play.randseed);

  write_log_debug("Initialize path finder library");

  init_pathfinder();

  write_log_debug("Initialize gfx");

  platform->InitialiseAbufAtStartup();

  LOCK_VARIABLE(timerloop);
  LOCK_FUNCTION(dj_timer_handler);
  set_game_speed(40);

  our_eip=-20;
  //thisroom.allocall();
  our_eip=-19;
  //setup_sierra_interface();   // take this out later

  write_log_debug("Load game data");

  our_eip=-17;
  if ((ee=load_game_file())!=0) {
    proper_exit=1;
    platform->FinishedUsingGraphicsMode();

    if (ee==-1)
      platform->DisplayAlert("Main game file not found. This may be from a different AGS version, or the file may have got corrupted.\n");
    else if (ee==-2)
      platform->DisplayAlert("Invalid file format. The file may be corrupt, or from a different\n"
        "version of AGS.\nThis engine can only run games made with AGS 2.5 or later.\n");
    else if (ee==-3)
      platform->DisplayAlert("Script link failed: %s\n",ccErrorString);
    return EXIT_NORMAL;
  }

  if (justRegisterGame) 
  {
    platform->RegisterGameWithGameExplorer();
    proper_exit = 1;
    return EXIT_NORMAL;
  }

  if (justUnRegisterGame) 
  {
    platform->UnRegisterGameWithGameExplorer();
    proper_exit = 1;
    return EXIT_NORMAL;
  }

  //platform->DisplayAlert("loaded game");
  our_eip=-91;
#if (ALLEGRO_DATE > 19990103)
  set_window_title(game.gamename);
#endif

  write_log_debug(game.gamename);

  our_eip = -189;

  if (file_exists("Compiled", FA_ARCH | FA_DIREC, NULL))
  {
    // running in debugger
    use_compiled_folder_as_current_dir = 1;
    // don't redirect to the game exe folder (_Debug)
    usetup.data_files_dir = ".";
  }

  if (game.saveGameFolderName[0] != 0)
  {
    char newDirBuffer[MAX_PATH];
    sprintf(newDirBuffer, "$MYDOCS$/%s", game.saveGameFolderName);
    Game_SetSaveGameDirectory(newDirBuffer);
  }
  else if (use_compiled_folder_as_current_dir)
  {
    Game_SetSaveGameDirectory("Compiled");
  }
  our_eip = -178;

  write_log_debug("Checking for disk space");

  //init_language_text("en");
  if (check_write_access()==0) {
#if defined(IOS_VERSION)
    platform->DisplayAlert("Unable to write to the current directory. Make sure write permissions are"
    " set for the game directory.\n");
#else
    platform->DisplayAlert("Unable to write to the current directory. Do not run this game off a\n"
    "network or CD-ROM drive. Also check drive free space (you need 1 Mb free).\n");
#endif
    proper_exit = 1;
    return EXIT_NORMAL; 
  }

  if (fontRenderers[0] == NULL) 
  {
    platform->DisplayAlert("No fonts found. If you're trying to run the game from the Debug directory, this is not supported. Use the Build EXE command to create an executable in the Compiled folder.");
    proper_exit = 1;
    return EXIT_NORMAL;
  }

  our_eip = -179;

#ifndef PSP_NO_MOD_PLAYBACK
  if (game.options[OPT_NOMODMUSIC])
    opts.mod_player = 0;

  if (opts.mod_player) {
    write_log_debug("Initializing MOD/XM player");

    if (init_mod_player(NUM_MOD_DIGI_VOICES) < 0) {
      platform->DisplayAlert("Warning: install_mod: MOD player failed to initialize.");
      opts.mod_player=0;
    }
  }
#else
  opts.mod_player = 0;
  write_log_debug("Compiled without MOD/XM player");
#endif

  write_log_debug("Initializing screen settings");

  // default shifts for how we store the sprite data

#if defined(PSP_VERSION)
  // PSP: Switch b<>r for 15/16 bit.
  _rgb_r_shift_32 = 16;
  _rgb_g_shift_32 = 8;
  _rgb_b_shift_32 = 0;
  _rgb_b_shift_16 = 11;
  _rgb_g_shift_16 = 5;
  _rgb_r_shift_16 = 0;
  _rgb_b_shift_15 = 10;
  _rgb_g_shift_15 = 5;
  _rgb_r_shift_15 = 0;
#else
  _rgb_r_shift_32 = 16;
  _rgb_g_shift_32 = 8;
  _rgb_b_shift_32 = 0;
  _rgb_r_shift_16 = 11;
  _rgb_g_shift_16 = 5;
  _rgb_b_shift_16 = 0;
  _rgb_r_shift_15 = 10;
  _rgb_g_shift_15 = 5;
  _rgb_b_shift_15 = 0;
#endif

  usetup.base_width = 320;
  usetup.base_height = 200;
  
  if (game.default_resolution >= 5)
  {
    if (game.default_resolution >= 6)
    {
      // 1024x768
      usetup.base_width = 512;
      usetup.base_height = 384;
    }
    else
    {
      // 800x600
      usetup.base_width = 400;
      usetup.base_height = 300;
    }
    // don't allow letterbox mode
    game.options[OPT_LETTERBOX] = 0;
    force_letterbox = 0;
    scrnwid = usetup.base_width * 2;
    scrnhit = usetup.base_height * 2;
    wtext_multiply = 2;
  }
  else if ((game.default_resolution == 4) ||
           (game.default_resolution == 3))
  {
    scrnwid = 640;
    scrnhit = 400;
    wtext_multiply = 2;
  }
  else if ((game.default_resolution == 2) ||
           (game.default_resolution == 1))
  {
    scrnwid = 320;
    scrnhit = 200;
    wtext_multiply = 1;
  }
  else
  {
    scrnwid = usetup.base_width;
    scrnhit = usetup.base_height;
    wtext_multiply = 1;
  }

  usetup.textheight = wgetfontheight(0) + 1;

  vesa_xres=scrnwid; vesa_yres=scrnhit;
  //scrnwto=scrnwid-1; scrnhto=scrnhit-1;
  current_screen_resolution_multiplier = scrnwid / BASEWIDTH;

  if ((game.default_resolution > 2) &&
      (game.options[OPT_NATIVECOORDINATES]))
  {
    usetup.base_width *= 2;
    usetup.base_height *= 2;
  }

  int initasx=scrnwid,initasy=scrnhit;
  if (scrnwid==960) { initasx=1024; initasy=768; }

  // save this setting so we only do 640x480 full-screen if they want it
  usetup.want_letterbox = game.options[OPT_LETTERBOX];

  if (force_letterbox > 0)
    game.options[OPT_LETTERBOX] = 1;

  // PSP: Don't letterbox a 320x200 screen.
  if ((game.default_resolution != 2) && (game.default_resolution != 4))
    force_letterbox = usetup.want_letterbox = game.options[OPT_LETTERBOX] = 0;		

  // don't allow them to force a 256-col game to hi-color
  if (game.color_depth < 2)
    usetup.force_hicolor_mode = 0;

  int firstDepth = 8, secondDepth = 8;
  if ((game.color_depth == 2) || (force_16bit) || (usetup.force_hicolor_mode)) {
    firstDepth = 16;
    secondDepth = 15;
  }
  else if (game.color_depth > 2) {
    firstDepth = 32;
    secondDepth = 24;
  }

  adjust_sizes_for_resolution(loaded_game_file_version);

  write_log_debug("Init gfx filters");

  if (initialize_graphics_filter(usetup.gfxFilterID, initasx, initasy, firstDepth))
  {
    return EXIT_NORMAL;
  }
  
  write_log_debug("Init gfx driver");

  create_gfx_driver();

  write_log_debug("Switching to graphics mode");

  if (switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
  {
    bool errorAndExit = true;

    if (((usetup.gfxFilterID == NULL) || 
      (stricmp(usetup.gfxFilterID, "None") == 0)) &&
      (scrnwid == 320))
    {
      // If the game is 320x200 and no filter is being used, try using a 2x
      // filter automatically since many gfx drivers don't suport 320x200.
      write_log_debug("320x200 not supported, trying with 2x filter");
      delete filter;

      if (initialize_graphics_filter("StdScale2", initasx, initasy, firstDepth)) 
      {
        return EXIT_NORMAL;
      }

      create_gfx_driver();

      if (!switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
      {
        errorAndExit = false;
      }

    }
    
    if (errorAndExit)
    {
      proper_exit=1;
      platform->FinishedUsingGraphicsMode();
  
      // make sure the error message displays the true resolution
      if (game.options[OPT_LETTERBOX])
        initasy = (initasy * 12) / 10;

      if (filter != NULL)
        filter->GetRealResolution(&initasx, &initasy);

      platform->DisplayAlert("There was a problem initializing graphics mode %d x %d (%d-bit).\n"
       "(Problem: '%s')\n"
       "Try to correct the problem, or seek help from the AGS homepage.\n"
       "\nPossible causes:\n* your graphics card drivers do not support this resolution. "
       "Run the game setup program and try the other resolution.\n"
       "* the graphics driver you have selected does not work. Try switching between Direct3D and DirectDraw.\n"
       "* the graphics filter you have selected does not work. Try another filter.",
       initasx, initasy, firstDepth, allegro_error);
      return EXIT_NORMAL;
    }
  }
  
  //screen = _filter->ScreenInitialized(screen, final_scrn_wid, final_scrn_hit);
  _old_screen = screen;

  if (gfxDriver->HasAcceleratedStretchAndFlip()) 
  {
    walkBehindMethod = DrawAsSeparateSprite;

    CreateBlankImage();
  }

  write_log_debug("Preparing graphics mode screen");

  if ((final_scrn_hit != scrnhit) || (final_scrn_wid != scrnwid)) {
    initasx = final_scrn_wid;
    initasy = final_scrn_hit;
    clear(_old_screen);
    screen = create_sub_bitmap(_old_screen, initasx / 2 - scrnwid / 2, initasy/2-scrnhit/2, scrnwid, scrnhit);
    _sub_screen=screen;

    scrnhit = screen->h;
    vesa_yres = screen->h;
    scrnwid = screen->w;
    vesa_xres = screen->w;
    gfxDriver->SetMemoryBackBuffer(screen);

    platform->WriteDebugString("Screen resolution: %d x %d; game resolution %d x %d", _old_screen->w, _old_screen->h, scrnwid, scrnhit);
  }


  // Most cards do 5-6-5 RGB, which is the format the files are saved in
  // Some do 5-6-5 BGR, or  6-5-5 RGB, in which case convert the gfx
  if ((final_col_dep == 16) && ((_rgb_b_shift_16 != 0) || (_rgb_r_shift_16 != 11))) {
    convert_16bit_bgr = 1;
    if (_rgb_r_shift_16 == 10) {
      // some very old graphics cards lie about being 16-bit when they
      // are in fact 15-bit ... get around this
      _places_r = 3;
      _places_g = 3;
    }
  }
  if (final_col_dep > 16) {
    // when we're using 32-bit colour, it converts hi-color images
    // the wrong way round - so fix that

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
    _rgb_b_shift_16 = 0;
    _rgb_g_shift_16 = 5;
    _rgb_r_shift_16 = 11;

    _rgb_b_shift_15 = 0;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 10;

    _rgb_r_shift_32 = 0;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 16;
#else
    _rgb_r_shift_16 = 11;
    _rgb_g_shift_16 = 5;
    _rgb_b_shift_16 = 0;
#endif
  }
  else if (final_col_dep == 16) {
    // ensure that any 32-bit graphics displayed are converted
    // properly to the current depth
#if defined(PSP_VERSION)
    _rgb_r_shift_32 = 0;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 16;

    _rgb_b_shift_15 = 0;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 10;
#else
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
#endif
  }
  else if (final_col_dep < 16) {
    // ensure that any 32-bit graphics displayed are converted
    // properly to the current depth
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
    _rgb_r_shift_32 = 0;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 16;

    _rgb_b_shift_15 = 0;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 10;
#else
    _rgb_r_shift_32 = 16;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 0;
#endif
  }

  platform->PostAllegroInit((usetup.windowed > 0) ? true : false);

  gfxDriver->SetCallbackForPolling(update_polled_stuff_if_runtime);
  gfxDriver->SetCallbackToDrawScreen(draw_screen_callback);
  gfxDriver->SetCallbackForNullSprite(GfxDriverNullSpriteCallback);

  write_log_debug("Initializing colour conversion");

  set_color_conversion(COLORCONV_MOST | COLORCONV_EXPAND_256 | COLORCONV_REDUCE_16_TO_15);

  SetMultitasking(0);

  write_log_debug("Check for preload image");

  show_preload ();

  write_log_debug("Initialize sprites");

  if (spriteset.initFile ("acsprset.spr")) 
  {
    platform->FinishedUsingGraphicsMode();
    allegro_exit();
    proper_exit=1;
    platform->DisplayAlert("Could not load sprite set file ACSPRSET.SPR\n"
      "This means that the file is missing or there is not enough free\n"
      "system memory to load the file.\n");
    return EXIT_NORMAL;
  }

  write_log_debug("Set up screen");

  virtual_screen=create_bitmap_ex(final_col_dep,scrnwid,scrnhit);
  clear(virtual_screen);
  gfxDriver->SetMemoryBackBuffer(virtual_screen);
//  ignore_mouseoff_bitmap = virtual_screen;
  abuf=screen;
  our_eip=-7;

  for (ee = 0; ee < MAX_INIT_SPR + game.numcharacters; ee++)
    actsps[ee] = NULL;

  write_log_debug("Initialize game settings");

  init_game_settings();

  write_log_debug("Prepare to start game");

  scsystem.width = final_scrn_wid;
  scsystem.height = final_scrn_hit;
  scsystem.coldepth = final_col_dep;
  scsystem.windowed = 0;
  scsystem.vsync = 0;
  scsystem.viewport_width = divide_down_coordinate(scrnwid);
  scsystem.viewport_height = divide_down_coordinate(scrnhit);
  strcpy(scsystem.aci_version, ACI_VERSION_TEXT);
  scsystem.os = platform->GetSystemOSID();

  if (usetup.windowed)
    scsystem.windowed = 1;

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
  filter->SetMouseArea(0, 0, scrnwid-1, scrnhit-1);
#else
  filter->SetMouseArea(0,0,BASEWIDTH-1,BASEHEIGHT-1);
#endif
//  mloadwcursor("mouse.spr");
  //mousecurs[0]=spriteset[2054];
  currentcursor=0;
  our_eip=-4;
  mousey=100;  // stop icon bar popping up
  init_invalid_regions(final_scrn_hit);
  wsetscreen(virtual_screen);
  our_eip = -41;

  gfxDriver->SetRenderOffset(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));

  // PSP: Initialize the sound cache.
  clear_sound_cache();

  // Create sound update thread. This is a workaround for sound stuttering.
  if (psp_audio_multithreaded)
  {
#if defined(PSP_VERSION)
    update_mp3_thread_running = true;
    SceUID thid = sceKernelCreateThread("update_mp3_thread", update_mp3_thread, 0x20, 0xFA0, THREAD_ATTR_USER, 0);
    if (thid > -1)
      thid = sceKernelStartThread(thid, 0, 0);
    else
    {
      update_mp3_thread_running = false;
      psp_audio_multithreaded = 0;
    }
#elif (defined(LINUX_VERSION) && !defined(PSP_VERSION)) || defined(MAC_VERSION)
    update_mp3_thread_running = true;
    if (pthread_create(&soundthread, NULL, update_mp3_thread, NULL) != 0)
    {
      update_mp3_thread_running = false;
      psp_audio_multithreaded = 0;
    }
#elif defined(WINDOWS_VERSION)
    update_mp3_thread_running = true;
    if (CreateThread(NULL, 0, update_mp3_thread, NULL, 0, NULL) == NULL)
    {
      update_mp3_thread_running = false;
      psp_audio_multithreaded = 0;
    }
#endif
  }

#if defined(ANDROID_VERSION)
  if (psp_load_latest_savegame)
    selectLatestSavegame();
#endif

  initialize_start_and_play_game(override_start_room, loadSaveGameOnStartup);

  update_mp3_thread_running = false;

  quit("|bye!");
  return 0;
}

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
END_OF_MAIN()
#endif

// in ac_minidump
extern int CustomExceptionHandler (LPEXCEPTION_POINTERS exinfo);
extern EXCEPTION_RECORD excinfo;
extern int miniDumpResultCode;

int initialize_engine_with_exception_handling(int argc,char*argv[])
{
  write_log_debug("Installing exception handler");

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
  __try 
  {
#endif

    return initialize_engine(argc, argv);

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
  }
  __except (CustomExceptionHandler ( GetExceptionInformation() )) 
  {
    strcpy (tempmsg, "");
    sprintf (printfworkingspace, "An exception 0x%X occurred in ACWIN.EXE at EIP = 0x%08X %s; program pointer is %+d, ACI version " ACI_VERSION_TEXT ", gtags (%d,%d)\n\n"
      "AGS cannot continue, this exception was fatal. Please note down the numbers above, remember what you were doing at the time and post the details on the AGS Technical Forum.\n\n%s\n\n"
      "Most versions of Windows allow you to press Ctrl+C now to copy this entire message to the clipboard for easy reporting.\n\n%s (code %d)",
      excinfo.ExceptionCode, excinfo.ExceptionAddress, tempmsg, our_eip, eip_guinum, eip_guiobj, get_cur_script(5),
      (miniDumpResultCode == 0) ? "An error file CrashInfo.dmp has been created. You may be asked to upload this file when reporting this problem on the AGS Forums." : 
      "Unable to create an error dump file.", miniDumpResultCode);
    MessageBoxA(allegro_wnd, printfworkingspace, "Illegal exception", MB_ICONSTOP | MB_OK);
    proper_exit = 1;
  }
  return EXIT_CRASH;
#endif
}

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
// Game update procedure
//

#include <math.h>
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/draw.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/lipsync.h"
#include "ac/overlay.h"
#include "ac/sys_events.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "main/update.h"
#include "ac/screenoverlay.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "media/audio/audio_system.h"
#include "ac/timer.h"
#include "main/game_run.h"
#include "ac/movelist.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern RoomStatus*croom;
extern GameSetupStruct game;
extern GameState play;
extern RoomStruct thisroom;
extern RoomObject*objs;
extern std::vector<ViewStruct> views;
extern int our_eip;
extern CharacterInfo*playerchar;
extern CharacterInfo *facetalkchar;
extern int face_talking,facetalkview,facetalkwait,facetalkframe;
extern int facetalkloop, facetalkrepeat, facetalkAllowBlink;
extern int facetalkBlinkLoop;
extern bool facetalk_qfg4_override_placement_x, facetalk_qfg4_override_placement_y;
extern SpeechLipSyncLine *splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;
extern IGraphicsDriver *gfxDriver;

// specific implementation for do_movelist_move, until we understand better the algorithm and what precision is required
bool movelist_float_equals(float a, float b) {
  return fabs(a - b) < 0.0001;
}

// Receives and modifies movelist_id and entity coordinates
// Returns whether to there's a need to update the sprite due to a change in direction
int do_movelist_move(short &movelist_id, int &xx, int &yy) {
  // TODO: make sense of contradicting comments
  int need_to_fix_sprite=0;
  if (movelist_id<1) quit("movelist_move: attempted to move on a non-existent movelist");
  MoveList* cmls = &mls[movelist_id];
  float xpermove=cmls->xpermove[cmls->onstage],ypermove=cmls->ypermove[cmls->onstage];

  int targetx = cmls->pos[cmls->onstage+1].X;
  int targety = cmls->pos[cmls->onstage+1].Y;
  int xps=xx, yps=yy; // the new position that will be assigned to xx and yy
  if (cmls->doneflag & kMoveListDone_X) {
    // if the X-movement has finished, and the Y-per-move is < 1, finish
    // This can cause jump at the end, but without it the character will
    // walk on the spot for a while if the Y-per-move is for example 0.2
//    if ((ypermove & 0xfffff000) == 0) cmls->doneflag|=2;
//    int ypmm=(ypermove >> 16) & 0x0000ffff;

    // NEW 2.15 SR-1 plan: if X-movement has finished, and Y-per-move is < 1,
    // allow it to finish more easily by moving target zone

    int adjAmnt = 3;
    // 2.70: if the X permove is also <=1, don't do the skipping
    if ((trunc(xpermove) == -1) ||
        (trunc(xpermove) == 0) )
      adjAmnt = 2;

    // 2.61 RC1: correct this to work with > -1 as well as < 1
    if (movelist_float_equals(ypermove, 0)) { }
    // Y per move is < 1, so finish the move
    else if (trunc(ypermove) == 0)
      targety -= adjAmnt;
    // Y per move is -1 exactly, don't snap to finish
    else if (movelist_float_equals(ypermove, -1)) { }
    // Y per move is > -1, so finish the move
    else if (trunc(ypermove) == -1)
      targety += adjAmnt;
  }
  else xps=cmls->fromx+(int)(xpermove*(float)cmls->onpart);

  if (cmls->doneflag & kMoveListDone_Y) {
    // Y-movement has finished

    int adjAmnt = 3;

    // if the Y permove is also <=1, don't skip as far
    if ((trunc(ypermove) == -1) ||
        (trunc(ypermove) == 0))
      adjAmnt = 2;

    if (movelist_float_equals(xpermove, 0)) { }
    // Y per move is < 1, so finish the move
    else if (trunc(xpermove) == 0)
      targetx -= adjAmnt;
    // X per move is -1 exactly, don't snap to finish
    else if (movelist_float_equals(xpermove, -1)) { }
    // X per move is > -1, so finish the move
    else if (trunc(xpermove) == -1)
      targetx += adjAmnt;

/*    int xpmm=(xpermove >> 16) & 0x0000ffff;
//    if ((xpmm==0) | (xpmm==0xffff)) cmls->doneflag|=1;
    if (xpmm==0) cmls->doneflag|=1;*/
  }
  else yps=cmls->fromy+(int)(ypermove*(float)cmls->onpart);

  // check if finished horizontal movement
  if (((xpermove > 0) && (xps >= targetx)) ||
      ((xpermove < 0) && (xps <= targetx))) {
    cmls->doneflag |= kMoveListDone_X;
    xps = targetx;
    // if the Y is almost there too, finish it
    // this is new in v2.40
    // removed in 2.70
    /*if (abs(yps - targety) <= 2)
      yps = targety;*/
  }
  else if (xpermove == 0)
    cmls->doneflag |= kMoveListDone_X;

  // check if finished vertical movement
  if ((ypermove > 0) & (yps>=targety)) {
    cmls->doneflag |= kMoveListDone_Y;
    yps = targety;
  }
  else if ((ypermove < 0) & (yps<=targety)) {
    cmls->doneflag |= kMoveListDone_Y;
    yps = targety;
  }
  else if (ypermove == 0)
    cmls->doneflag |= kMoveListDone_Y;

  if ((cmls->doneflag & kMoveListDone_XY) == kMoveListDone_XY) {
    // this stage is done, go on to the next stage
    cmls->fromx=cmls->pos[cmls->onstage+1].X;
    cmls->fromy=cmls->pos[cmls->onstage+1].Y;

    cmls->onstage++;
    cmls->onpart = -1;
    cmls->doneflag = 0; // this used to clear only the lower 4 bits, but no upper bit was ever assigned in the first place
    cmls->lastx = -1;
    if (cmls->onstage < cmls->numstage) {
      xps=cmls->fromx; yps=cmls->fromy;
    }
    if (cmls->onstage>=cmls->numstage-1) {  // last stage is just dest pos
      cmls->numstage = 0;
      movelist_id = 0; // movelist 0 means "not moving/walking"
      need_to_fix_sprite = 1; // WARNING: value 1 is not used anywhere, could be a mistake
    }
    else need_to_fix_sprite = 2; // used to request a sprite direction update
  }
  cmls->onpart++;
  xx = xps;
  yy = yps;
  return need_to_fix_sprite;
}


void update_script_timers()
{
  if (play.gscript_timer > 0) play.gscript_timer--;
  for (int aa=0;aa<MAX_TIMERS;aa++) {
    if (play.script_timers[aa] > 1) play.script_timers[aa]--;
    }
}

void update_cycling_views()
{
	// update graphics for object if cycling view
  for (uint32_t i = 0; i < croom->numobj; ++i)
  {
      objs[i].UpdateCyclingView(i);
  }
}

// Updates the view of the player character
void update_player_view()
{
    if (playerchar->flags & CHF_FIXVIEW)
        return; // view is locked

    int onwalkarea = get_walkable_area_at_character(game.playercharacter);
    if (onwalkarea < 0)
        return; // error?

    int areaview = thisroom.WalkAreas[onwalkarea].PlayerView;
    if (areaview > 0)
        playerchar->view = areaview - 1; // convert to 0-based id
    else if (thisroom.Options.PlayerView > 0) 
        playerchar->view = thisroom.Options.PlayerView - 1; // convert to 0-based id
    else
        playerchar->view = playerchar->defview;
}

void update_character_move_and_anim(std::vector<int> &followingAsSheep)
{
	// move & animate characters
  for (int aa=0;aa<game.numcharacters;aa++) {
    if (game.chars[aa].on != 1) continue;

    CharacterInfo*chi    = &game.chars[aa];
	CharacterExtras*chex = &charextra[aa];

	chi->UpdateMoveAndAnim(aa, chex, followingAsSheep);
  }
}

void update_following_exactly_characters(const std::vector<int> &followingAsSheep)
{
	// update location of all following_exactly characters
  for (size_t i = 0; i < followingAsSheep.size(); ++i) {
    CharacterInfo *chi = &game.chars[followingAsSheep[i]];

	chi->UpdateFollowingExactlyCharacter();
  }
}

void update_overlay_timers()
{
	// update overlay timers
  for (size_t i = 0; i < screenover.size();) {
    if (screenover[i].timeout > 0) {
      screenover[i].timeout--;
      if (screenover[i].timeout == 0)
      {
        remove_screen_overlay_index(i);
        continue;
      }
    }
    i++;
  }
}

void update_speech_and_messages()
{
  bool is_voice_playing = false;
  if (play.speech_has_voice)
  {
      auto *ch = AudioChans::GetChannel(SCHAN_SPEECH);
      is_voice_playing = ch && ch->is_ready();
  }
  // determine if speech text should be removed
  if (play.messagetime>=0) {
    play.messagetime--;
    // extend life of text if the voice hasn't finished yet
    if (play.speech_has_voice && !play.speech_in_post_state) {
      if ((is_voice_playing) && (play.fast_forward == 0)) {
        if (play.messagetime <= 1)
          play.messagetime = 1;
      }
      else  // if the voice has finished, remove the speech
        play.messagetime = 0;
    }

    if (play.messagetime < 1 && play.speech_display_post_time_ms > 0 &&
        play.fast_forward == 0)
    {
        if (!play.speech_in_post_state)
        {
            play.messagetime = ::lround(play.speech_display_post_time_ms * get_current_fps() / 1000.0f);
        }
        play.speech_in_post_state = !play.speech_in_post_state;
    }

    if (play.messagetime < 1) 
    {
      if (play.fast_forward > 0)
      {
        remove_screen_overlay(play.text_overlay_on);
        play.SetWaitSkipResult(SKIP_AUTOTIMER);
      }
      else if (play.cant_skip_speech & SKIP_AUTOTIMER)
      {
        remove_screen_overlay(play.text_overlay_on);
        play.SetWaitSkipResult(SKIP_AUTOTIMER);
        play.SetIgnoreInput(play.ignore_user_input_after_text_timeout_ms);
      }
    }
  }
}

// update sierra-style speech
void update_sierra_speech()
{
  int voice_pos_ms = -1;
  if (play.speech_has_voice)
  {
      auto *ch = AudioChans::GetChannel(SCHAN_SPEECH);
      voice_pos_ms = ch ? ch->get_pos_ms() : -1;
  }
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
      if (curLipLinePhoneme >= splipsync[curLipLine].numPhonemes) {
        // the lip-sync has finished, so just stay idle
      }
      else 
      {
        while ((curLipLinePhoneme < splipsync[curLipLine].numPhonemes) &&
          ((curLipLinePhoneme < 0) || (voice_pos_ms >= splipsync[curLipLine].endtimeoffs[curLipLinePhoneme])))
        {
          curLipLinePhoneme ++;
          if (curLipLinePhoneme >= splipsync[curLipLine].numPhonemes)
            facetalkframe = game.default_lipsync_frame;
          else
            facetalkframe = splipsync[curLipLine].frame[curLipLinePhoneme];

          if (facetalkframe >= views[facetalkview].loops[facetalkloop].numFrames)
            facetalkframe = 0;

          updatedFrame |= 1;
        }
      }
    }
    else if (facetalkwait>0) facetalkwait--;
    // don't animate if the speech has finished
    else if ((play.messagetime < 1) && (facetalkframe == 0) &&
             // if play.close_mouth_speech_time = 0, this means animation should play till
             // the speech ends; but this should not work in voice mode, and also if the
             // speech is in the "post" state
             (play.speech_has_voice || play.speech_in_post_state || play.close_mouth_speech_time > 0))
      ;
    else {
      // Close mouth at end of sentence: if speech has entered the "post" state,
      // or if this is a text only mode and close_mouth_speech_time is set
      if (play.speech_in_post_state ||
          (!play.speech_has_voice &&
          (play.messagetime < play.close_mouth_speech_time) &&
          (play.close_mouth_speech_time > 0))) {
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
            (!play.speech_has_voice && (play.messagetime < 1) && (play.close_mouth_speech_time > 0))) {

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
    if ((updatedFrame) && (play.text_overlay_on > 0)) {

      if (updatedFrame & 1)
        CheckViewFrame (facetalkview, facetalkloop, facetalkframe);
      if (updatedFrame & 2)
        CheckViewFrame (facetalkchar->blinkview, facetalkBlinkLoop, facetalkchar->blinkframe);

      int thisPic = views[facetalkview].loops[facetalkloop].frames[facetalkframe].pic;      
      int view_frame_x = 0;
      int view_frame_y = 0;

      Bitmap *frame_pic = screenover[face_talking].GetImage();
      if (game.options[OPT_SPEECHTYPE] == 3) {
        // QFG4-style fullscreen dialog
        if (facetalk_qfg4_override_placement_x)
        {
          view_frame_x = play.speech_portrait_x;
        }
        if (facetalk_qfg4_override_placement_y)
        {
          view_frame_y = play.speech_portrait_y;
        }
        else
        {
          view_frame_y = (frame_pic->GetHeight() / 2) - (game.SpriteInfos[thisPic].Height / 2);
        }
        frame_pic->Clear(0);
      }
      else {
        frame_pic->ClearTransparent();
      }

      const ViewFrame *face_vf = &views[facetalkview].loops[facetalkloop].frames[facetalkframe];
      DrawViewFrame(frame_pic, face_vf, view_frame_x, view_frame_y);

      if ((facetalkchar->blinkview > 0) && (facetalkchar->blinktimer < 0)) {
        ViewFrame *blink_vf = &views[facetalkchar->blinkview].loops[facetalkBlinkLoop].frames[facetalkchar->blinkframe];
        // draw the blinking sprite on top
        DrawViewFrame(frame_pic, blink_vf, view_frame_x, view_frame_y);
      }

      screenover[face_talking].MarkChanged();
    }  // end if updatedFrame
  }
}

// update_stuff: moves and animates objects, executes repeat scripts, and
// the like.
void update_stuff() {
  
  our_eip = 20;

  update_script_timers();

  update_cycling_views();

  our_eip = 21;

  update_player_view();
  
  our_eip = 22;

  std::vector<int> followingAsSheep;

  update_character_move_and_anim(followingAsSheep);

  update_following_exactly_characters(followingAsSheep);

  our_eip = 23;

  update_overlay_timers();

  update_speech_and_messages();

  our_eip = 24;

  update_sierra_speech();

  our_eip = 25;
}

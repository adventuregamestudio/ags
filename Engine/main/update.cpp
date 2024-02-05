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

//
// Game update procedure
//

#include <math.h>
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/lipsync.h"
#include "ac/movelist.h"
#include "ac/overlay.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/sys_events.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/timer.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/audio_system.h"


using namespace AGS::Common;
using namespace AGS::Engine;

extern RoomStatus*croom;
extern GameSetupStruct game;
extern GameState play;
extern RoomStruct thisroom;
extern RoomObject*objs;
extern std::vector<ViewStruct> views;
extern CharacterInfo*playerchar;
extern CharacterInfo *facetalkchar;
extern int face_talking,facetalkview,facetalkwait,facetalkframe;
extern int facetalkloop, facetalkrepeat, facetalkAllowBlink;
extern int facetalkBlinkLoop;
extern bool facetalk_qfg4_override_placement_x, facetalk_qfg4_override_placement_y;
extern std::vector<SpeechLipSyncLine> splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;
extern IGraphicsDriver *gfxDriver;


// Optionally fixes target position, when one axis is left to move along.
// This is done only for backwards compatibility now.
// Uses generic parameters.
static void movelist_handle_targetfix(const fixed xpermove, const fixed ypermove, int &targety)
{
    // Old comment about ancient behavior:
    // if the X-movement has finished, and the Y-per-move is < 1, finish
    // This can cause jump at the end, but without it the character will
    // walk on the spot for a while if the Y-per-move is for example 0.2
    //    if ((ypermove & 0xfffff000) == 0) cmls.doneflag|=2;
    //    int ypmm=(ypermove >> 16) & 0x0000ffff;

    // NEW 2.15 SR-1 plan: if X-movement has finished, and Y-per-move is < 1,
    // allow it to finish more easily by moving target zone
    // NOTE: interesting fact: this fix was also done for the strictly vertical
    // move, probably because of the logical mistake in condition.

    int tfix = 3;
    // 2.70: if the X permove is also <=1, don't skip as far
    if (((xpermove & 0xffff0000) == 0xffff0000) ||
        ((xpermove & 0xffff0000) == 0x00000000))
        tfix = 2;

    // 2.61 RC1: correct this to work with > -1 as well as < 1
    if (ypermove == 0) {}
    // Y per move is < 1, so finish the move
    else if ((ypermove & 0xffff0000) == 0)
        targety -= tfix;
    // Y per move is -1 exactly, don't snap to finish
    else if (ypermove == 0xffff0000) {}
    // Y per move is > -1, so finish the move
    else if ((ypermove & 0xffff0000) == 0xffff0000)
        targety += tfix;
}

// Handle remaining move along a single axis; uses generic parameters.
static void movelist_handle_remainer(const fixed xpermove, const fixed ypermove,
    const int xdistance, const float step_length, fixed &fin_ymove, float &fin_from_part)
{
    // Walk along the remaining axis with the full walking speed
    assert(xpermove != 0 && ypermove != 0 && step_length >= 0.f);
    fin_ymove = ypermove > 0 ? ftofix(step_length) : -ftofix(step_length);
    fin_from_part = (float)xdistance / fixtof(xpermove);
    assert(fin_from_part >= 0);
}

// Handle remaining move fixup, but only if necessary
static void movelist_handle_remainer(MoveList &m)
{
    assert(m.numstage > 0);
    const fixed xpermove = m.xpermove[m.onstage];
    const fixed ypermove = m.ypermove[m.onstage];
    const Point target = m.pos[m.onstage + 1];
    // Apply remainer to movelists where LONGER axis was completed, and SHORTER remains
    if ((xpermove != 0) && (ypermove != 0))
    {
        if ((m.doneflag & kMoveListDone_XY) == kMoveListDone_X && (abs(ypermove) < abs(xpermove)))
            movelist_handle_remainer(xpermove, ypermove, (target.X - m.from.X),
                m.GetStepLength(), m.fin_move, m.fin_from_part);
        else if ((m.doneflag & kMoveListDone_XY) == kMoveListDone_Y && (abs(xpermove) < abs(ypermove)))
            movelist_handle_remainer(ypermove, xpermove, (target.Y - m.from.Y),
                m.GetStepLength(), m.fin_move, m.fin_from_part);
    }
}

// Test if move completed, returns if just completed
static bool movelist_handle_donemove(const uint8_t testflag, const fixed xpermove, const int targetx, uint8_t &doneflag, int &xps)
{
    if ((doneflag & testflag) != 0)
        return false; // already done before

    if (((xpermove > 0) && (xps >= targetx)) || ((xpermove < 0) && (xps <= targetx)))
    {
        doneflag |= testflag;
        xps = targetx; // snap to the target (in case run over)
        // Comment about old engine behavior:
        // if the Y is almost there too, finish it
        // this is new in v2.40
        // removed in 2.70
        /*if (abs(yps - targety) <= 2)
            yps = targety;*/
    }
    else if (xpermove == 0)
    {
        doneflag |= testflag;
    }
    return (doneflag & testflag) != 0;
}

int do_movelist_move(short &mslot, int &pos_x, int &pos_y)
{
    // TODO: find out why movelist 0 is not being used
    assert(mslot >= 1);
    if (mslot < 1)
        return 0;

    int need_to_fix_sprite = 0; // TODO: find out what this value means and refactor
    MoveList &cmls = mls[mslot];
    const fixed xpermove = cmls.xpermove[cmls.onstage];
    const fixed ypermove = cmls.ypermove[cmls.onstage];
    const fixed fin_move = cmls.fin_move;
    const float main_onpart = (cmls.fin_from_part > 0.f) ? cmls.fin_from_part : cmls.onpart;
    const float fin_onpart = cmls.onpart - main_onpart;
    Point target = cmls.pos[cmls.onstage + 1];
    int xps = pos_x, yps = pos_y;

    // Old-style optional move target fixup
    if (loaded_game_file_version < kGameVersion_361)
    {
        if ((ypermove != 0) && (cmls.doneflag & kMoveListDone_X) != 0)
        { // X-move has finished, handle the Y-move remainer
            movelist_handle_targetfix(xpermove, ypermove, target.Y);
        }
        else if ((xpermove != 0) && (cmls.doneflag & kMoveListDone_Y) != 0)
        { // Y-move has finished, handle the X-move remainer
            movelist_handle_targetfix(xpermove, ypermove, target.Y);
        }
    }

    // Calculate next positions, as required
    if ((cmls.doneflag & kMoveListDone_X) == 0)
    {
        xps = cmls.from.X + (int)(fixtof(xpermove) * main_onpart) +
          (int)(fixtof(fin_move) * fin_onpart);
    }
    if ((cmls.doneflag & kMoveListDone_Y) == 0)
    {
        yps = cmls.from.Y + (int)(fixtof(ypermove) * main_onpart) +
          (int)(fixtof(fin_move) * fin_onpart);
    }

    // Check if finished either horizontal or vertical movement;
    // if any was finished just now, then also handle remainer fixup
    bool done_now = movelist_handle_donemove(kMoveListDone_X, xpermove, target.X, cmls.doneflag, xps);
    done_now     |= movelist_handle_donemove(kMoveListDone_Y, ypermove, target.Y, cmls.doneflag, yps);
    if (done_now)
        movelist_handle_remainer(cmls);

    // Handle end of move stage
    if ((cmls.doneflag & kMoveListDone_XY) == kMoveListDone_XY)
    {
        // this stage is done, go on to the next stage
        cmls.from = cmls.pos[cmls.onstage + 1];
        cmls.onstage++;
        cmls.onpart = -1.f;
        cmls.fin_from_part = 0.f;
        cmls.fin_move = 0;
        cmls.doneflag = 0;
        if (cmls.onstage < cmls.numstage)
        {
            xps = cmls.from.X;
            yps = cmls.from.Y;
        }

        if (cmls.onstage >= cmls.numstage - 1)
        {  // last stage is just dest pos
            cmls.numstage=0;
            mslot = 0;
            need_to_fix_sprite = 1; // TODO: find out what this means
        }
        else
        {
            need_to_fix_sprite = 2; // TODO: find out what this means
        }
    }

    // Make a step along the current vector and return
    cmls.onpart += 1.f;
    pos_x = xps;
    pos_y = yps;
    return need_to_fix_sprite;
}

void restore_movelists()
{
    // Recalculate move remainer fixups, where necessary
    for (auto &m : mls)
    {
        if (m.numstage > 0)
            movelist_handle_remainer(m);
    }
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
  auto &overs = get_overlays();
  for (auto &over : overs)
  {
    if (over.timeout > 0) {
      over.timeout--;
      if (over.timeout == 0)
      {
        remove_screen_overlay(over.type);
      }
    }
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

    // Enter speech post-state: optionally increase final waiting time
    if (!play.speech_in_post_state && (play.fast_forward == 0) && (play.messagetime < 1))
    {
        play.speech_in_post_state = true;
        if (play.speech_display_post_time_ms > 0)
        {
            play.messagetime = ::lround(play.speech_display_post_time_ms * get_game_fps() / 1000.0f);
        }
    }

    if (play.messagetime < 1) 
    {
      if (play.fast_forward > 0)
      {
        remove_screen_overlay(play.text_overlay_on);
        play.SetWaitSkipResult(SKIP_AUTOTIMER);
      }
      else if (play.speech_skip_style & SKIP_AUTOTIMER)
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

      const auto &talking_chex = charextra[facetalkchar->index_id];
      const int frame_vol = talking_chex.GetFrameSoundVolume(facetalkchar);
      if (updatedFrame & 1)
        CheckViewFrame(facetalkview, facetalkloop, facetalkframe, frame_vol);
      if (updatedFrame & 2)
        CheckViewFrame(facetalkchar->blinkview, facetalkBlinkLoop, facetalkchar->blinkframe, frame_vol);

      int thisPic = views[facetalkview].loops[facetalkloop].frames[facetalkframe].pic;      
      int view_frame_x = 0;
      int view_frame_y = 0;

      auto *face_over = get_overlay(face_talking);
      assert(face_over != nullptr);
      Bitmap *frame_pic = spriteset[face_over->GetSpriteNum()];
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
      bool face_has_alpha = (game.SpriteInfos[face_vf->pic].Flags & SPF_ALPHACHANNEL) != 0;
      DrawViewFrame(frame_pic, face_vf, view_frame_x, view_frame_y);

      if ((facetalkchar->blinkview > 0) && (facetalkchar->blinktimer < 0)) {
        ViewFrame *blink_vf = &views[facetalkchar->blinkview].loops[facetalkBlinkLoop].frames[facetalkchar->blinkframe];
        face_has_alpha |= (game.SpriteInfos[blink_vf->pic].Flags & SPF_ALPHACHANNEL) != 0;
        // draw the blinking sprite on top
        DrawViewFrame(frame_pic, blink_vf, view_frame_x, view_frame_y, face_has_alpha);
      }

      face_over->SetAlphaChannel(face_has_alpha);
      // Make sure overlay texture will get updated on screen
      game_sprite_updated(face_over->GetSpriteNum());
      face_over->MarkChanged();
    }  // end if updatedFrame
  }
}

// update_stuff: moves and animates objects, executes repeat scripts, and
// the like.
void update_stuff() {

  set_our_eip(20);

  update_script_timers();

  update_cycling_views();

  set_our_eip(21);

  update_player_view();

  set_our_eip(22);

  std::vector<int> followingAsSheep;

  update_character_move_and_anim(followingAsSheep);

  update_following_exactly_characters(followingAsSheep);

  set_our_eip(23);

  update_overlay_timers();

  update_speech_and_messages();

  set_our_eip(24);

  update_sierra_speech();

  set_our_eip(25);
}

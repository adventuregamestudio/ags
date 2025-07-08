//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RoomStatus*croom;
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


// Receives and modifies movelist_id and entity coordinates
// Returns whether to there's a need to update the sprite due to a change in direction
int do_movelist_move(short &mslot, int &pos_x, int &pos_y)
{
    assert(mslot > 0);
    if (mslot < 1)
        return 0;

    MoveList &cmls = *get_movelist(mslot);
    // TODO: find out what this value means and refactor
    int need_to_fix_sprite = 0;
    const int old_stage = cmls.GetStage();
    if (cmls.Forward())
    {
        pos_x = cmls.GetCurrentPos().X;
        pos_y = cmls.GetCurrentPos().Y;
        if (cmls.GetStage() != old_stage)
            need_to_fix_sprite = 2; // used to request a sprite direction update
    }
    else
    {
        pos_x = cmls.GetCurrentPos().X;
        pos_y = cmls.GetCurrentPos().Y;
        // reset MoveList, and tell moving object to stop
        remove_movelist(mslot);
        mslot = 0; // movelist 0 means "not moving/walking"
    }
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
    if (!game.chars[aa].is_enabled()) continue;

    CharacterInfo*chi    = &game.chars[aa];
	CharacterExtras*chex = &charextra[aa];

	UpdateCharacterMoveAndAnim(chi, chex, followingAsSheep);
  }
}

void update_following_exactly_characters(const std::vector<int> &followingAsSheep)
{
	// update location of all following_exactly characters
  for (size_t i = 0; i < followingAsSheep.size(); ++i) {
    CharacterInfo *chi = &game.chars[followingAsSheep[i]];

	UpdateFollowingExactlyCharacter(chi);
  }
}

void update_overlay_timers()
{
    // update overlay timers and positions
    auto &overs = get_overlays();
    for (auto &over : overs)
    {
        if (over.timeout > 0)
        {
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

bool has_voice_lipsync()
{
    return play.speech_has_voice && (curLipLine >= 0);
}

int update_voice_lipsync(int frame)
{
    if (!has_voice_lipsync())
        return frame; // no change

    auto *ch = AudioChans::GetChannel(SCHAN_SPEECH);
    const int voice_pos_ms = ch ? ch->get_pos_ms() : -1;
    if (voice_pos_ms < 0)
        return frame; // no change

    // check voice lip sync
    if (curLipLinePhoneme >= splipsync[curLipLine].numPhonemes) {
        // the lip-sync has finished, so just stay idle
        return frame;
    }
    else
    {
        while ((curLipLinePhoneme < splipsync[curLipLine].numPhonemes) &&
               ((curLipLinePhoneme < 0) || (voice_pos_ms >= splipsync[curLipLine].endtimeoffs[curLipLinePhoneme])))
        {
            curLipLinePhoneme++;
            if (curLipLinePhoneme >= splipsync[curLipLine].numPhonemes)
                frame = game.default_lipsync_frame;
            else
                frame = splipsync[curLipLine].frame[curLipLinePhoneme];

            if (frame >= views[facetalkview].loops[facetalkloop].numFrames)
                frame = 0;
        }
        return frame;
    }
}

void update_sierra_speech()
{
  // Update sierra-style portrait (if active)
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

    const int old_facetalkframe = facetalkframe;
    if (has_voice_lipsync()) {
        facetalkframe = update_voice_lipsync(facetalkframe);
    }
    else if (facetalkwait > 0) { facetalkwait--; }
    // don't animate if the speech has finished
    else if ((play.messagetime < 1) && (facetalkframe == 0) &&
             // if play.close_mouth_speech_time = 0, this means animation should play till
             // the speech ends; but this should not work in voice mode, and also if the
             // speech is in the "post" state
             (play.speech_has_voice || play.speech_in_post_state || play.close_mouth_speech_time > 0))
    { }
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
          facetalkwait = views[facetalkview].loops[facetalkloop].frames[facetalkframe].speed + Character_GetSpeechAnimationDelay(facetalkchar);
      }
    }

    if (facetalkframe != old_facetalkframe) {
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
      if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4) {
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

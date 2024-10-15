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
#include "ac/characterinfo.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_game.h" // GetGameSpeed
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/math.h"
#include "ac/object.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "main/update.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;

extern std::vector<ViewStruct> views;
extern GameSetupStruct game;
extern int displayed_room;
extern RoomStruct thisroom;
extern unsigned int loopcounter;
extern int char_speaking_anim;

#define Random __Rand

int CharacterInfo::get_baseline() const {
    if (baseline < 1)
        return y;
    return baseline;
}

int CharacterInfo::get_blocking_top() const {
    if (blocking_height > 0)
        return y - blocking_height / 2;
    return y - 2;
}

int CharacterInfo::get_blocking_bottom() const {
    // the blocking_bottom should be 1 less than the top + height
    // since the code does <= checks on it rather than < checks
    if (blocking_height > 0)
        return (y + (blocking_height + 1) / 2) - 1;
    return y + 3;
}

// Tests if frame reached or exceeded the loop, and resets frame back
// to either 1st walking frame if character is walking, or frame 0 otherwise
// (or if there's less than 2 frames in the current loop).
static void ResetFrameIfAtEnd(CharacterInfo *chi)
{
    const int frames_in_loop = views[chi->view].loops[chi->loop].numFrames;
    if (chi->frame >= frames_in_loop)
    {
        chi->frame = (chi->walking > 0 && frames_in_loop > 1) ? 1 : 0;
    }
}

// Fixups loop and frame values, in case any of them are set to a value out of the valid range
static void FixupCharacterLoopAndFrame(CharacterInfo *chi)
{
    // If current loop property exceeds number of loops,
    // or if selected loop has no frames, then try select any first loop that has frames.
    // NOTE: although this may seem like a weird solution to a problem,
    // we do so for backwards compatibility; this approximately emulates older games behavior.
    const int view = chi->view;
    if (view >= 0 &&
        (chi->loop >= views[view].numLoops || views[view].loops[chi->loop].numFrames == 0))
    {
        int loop = chi->loop;
        for (loop = 0;
            (loop < views[view].numLoops) && (views[view].loops[loop].numFrames == 0);
            ++loop);
        if (loop == views[view].numLoops) // view has no frames?!
        {
            quitprintf("!Character %s is assigned view %d that has no frames!", chi->scrname.GetCStr(), view);
            loop = 0;
        }
        chi->loop = loop;
    }

    ResetFrameIfAtEnd(chi); // test, in case new loop has less frames
}

void UpdateCharacterMoveAndAnim(CharacterInfo *chi, CharacterExtras *chex, std::vector<int> &followingAsSheep)
{
	if (!chi->is_enabled())
        return;
    
	// Update turning
    bool is_turning = UpdateCharacterTurning(chi, chex);
    // Fixup character's loop prior to any further logic updates
    FixupCharacterLoopAndFrame(chi);
    if (is_turning)
		return; // still turning, break

    int doing_nothing = 1;
	UpdateCharacterMoving(chi, chex, doing_nothing);

	// Update animating
	if (UpdateCharacterAnimating(chi, chex, doing_nothing))
		return; // further update blocked by active animation

	UpdateCharacterFollower(chi, followingAsSheep, doing_nothing);

	UpdateCharacterIdle(chi, chex, doing_nothing);

    chex->process_idle_this_time = 0;
}

void UpdateFollowingExactlyCharacter(CharacterInfo *chi)
{
    const auto &following = game.chars[chi->following];
    chi->x = following.x;
    chi->y = following.y;
    chi->z = following.z;
    chi->room = following.room;
    chi->prevroom = following.prevroom;

    const int usebase = following.get_baseline();
    if (chi->flags & CHF_BEHINDSHEPHERD)
      chi->baseline = usebase - 1;
    else
      chi->baseline = usebase + 1;
}

bool UpdateCharacterTurning(CharacterInfo *chi, CharacterExtras *chex)
{
    if (chi->walking >= TURNING_AROUND) {
      const int view = chi->view;
      // Currently rotating to correct direction
      if (chi->walkwait > 0) {
        chi->walkwait--;
      }
      else {
        // Work out which direction is next
        int wantloop = find_looporder_index(chi->loop) + 1;
        // going anti-clockwise, take one before instead
        if (chi->walking >= TURNING_BACKWARDS)
          wantloop -= 2;
        while (1) {
          if (wantloop >= 8)
            wantloop = 0;
          else if (wantloop < 0)
            wantloop = 7;

          if ((turnlooporder[wantloop] >= views[view].numLoops) ||
              (views[view].loops[turnlooporder[wantloop]].numFrames < 1) ||
              ((turnlooporder[wantloop] >= 4) && ((chi->flags & CHF_NODIAGONAL)!=0))) {
            if (chi->walking >= TURNING_BACKWARDS)
              wantloop--;
            else
              wantloop++;
          }
          else {
              break;
          }
        }
        chi->loop = turnlooporder[wantloop];
        chi->walking -= TURNING_AROUND;
        // if still turning, wait for next frame
        if (chi->walking % TURNING_BACKWARDS >= TURNING_AROUND)
          chi->walkwait = chi->animspeed;
        else
          chi->walking = chi->walking % TURNING_BACKWARDS;
        chex->animwait = 0;
      }

	  return true;
    }

	return false;
}

void UpdateCharacterMoving(CharacterInfo *chi, CharacterExtras *chex, int &doing_nothing)
{
	if ((chi->walking > 0) && (chi->room == displayed_room))
    {
      if (chi->walkwait > 0)
      {
          chi->walkwait--;
      }
      else 
      {
        chi->flags &= ~CHF_AWAITINGMOVE;

        // Move the character
        int numSteps = wantMoveNow(chi, chex);

        if ((numSteps) && (chex->xwas != INVALID_X)) {
          // if the zoom level changed mid-move, the walkcounter
          // might not have come round properly - so sort it out
          chi->x = chex->xwas;
          chi->y = chex->ywas;
          chex->xwas = INVALID_X;
        }

        const int oldxp = chi->x, oldyp = chi->y;

        for (int ff = 0; ff < abs(numSteps); ff++) {
          if (doNextCharMoveStep(chi, chex))
            break;
          if ((chi->walking == 0) || (chi->walking >= TURNING_AROUND))
            break;
        }

        if (numSteps < 0) {
          // very small scaling, intersperse the movement
          // to stop it being jumpy
          chex->xwas = chi->x;
          chex->ywas = chi->y;
          chi->x = ((chi->x) - oldxp) / 2 + oldxp;
          chi->y = ((chi->y) - oldyp) / 2 + oldyp;
        }
        else if (numSteps > 0)
          chex->xwas = INVALID_X;

        if ((chi->flags & CHF_ANTIGLIDE) == 0)
          chi->walkwaitcounter++;
      }

      // Fixup character's loop, it may be changed when making a walk-move
      FixupCharacterLoopAndFrame(chi);

      doing_nothing = 0; // still walking?

      const int view = chi->view, loop = chi->loop;
      if (chi->walking < 1) {
        // Finished walking, stop and reset state
        chex->process_idle_this_time = 1;
        doing_nothing=1;
        chi->walkwait=0;
        const bool was_walk_anim = (chi->flags & CHF_MOVENOTWALK) == 0;
        Character_StopMoving(chi);
        // CHECKME: there's possibly a flaw in game logic design here, as StopMoving also resets the frame,
        // except it does not reset animwait, nor calls CheckViewFrame()
        if (was_walk_anim) {
            // use standing pic
            chex->animwait = 0;
            chi->frame = 0;
            chex->CheckViewFrame(chi);
        }
      }
      else if (chex->animwait > 0) {
          chex->animwait--;
      } else {
        if (chi->flags & CHF_ANTIGLIDE)
          chi->walkwaitcounter++;

        if ((chi->flags & CHF_MOVENOTWALK) == 0)
        {
          chi->frame++;
          ResetFrameIfAtEnd(chi); // roll back if exceeded loop

          chex->animwait = views[view].loops[loop].frames[chi->frame].speed + chi->animspeed;

          if (chi->flags & CHF_ANTIGLIDE)
            chi->walkwait = chex->animwait;
          else
            chi->walkwait = 0;

          chex->CheckViewFrame(chi);
        }
      }
    }
}

bool UpdateCharacterAnimating(CharacterInfo *chi, CharacterExtras *chex, int &doing_nothing)
{
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

      const int view = chi->view;
      if (chi->wait > 0) {
          chi->wait--;
      }
      else if ((char_speaking_anim == chi->index_id) && (game.options[OPT_LIPSYNCTEXT] != 0)) {
        // currently talking with lip-sync speech
        int fraa = chi->frame;
        chi->wait = update_lip_sync(view, chi->loop, &fraa) - 1;
        // closed mouth at end of sentence
        // NOTE: standard lip-sync is synchronized with text timer, not voice file
        if (play.speech_in_post_state ||
            ((play.messagetime >= 0) && (play.messagetime < play.close_mouth_speech_time)))
          chi->frame = 0;

        if (chi->frame != fraa) {
          chi->frame = fraa;
          chex->CheckViewFrame(chi);
        }

		return true;
      }
      else {
        // Normal view animation
        const int oldframe = chi->frame;

        bool done_anim = false;
        if ((chi->index_id == char_speaking_anim) &&
            (play.speech_in_post_state ||
            ((!play.speech_has_voice) &&
                (play.close_mouth_speech_time > 0) &&
                (play.messagetime < play.close_mouth_speech_time)))) {
            // finished talking - stop animation
            done_anim = true;
            chi->frame = 0;
        } else {
            if (!CycleViewAnim(view, chi->loop, chi->frame, chi->get_anim_forwards(), chi->get_anim_repeat())) {
                done_anim = true; // finished animating
                // end of idle anim
                if (chi->idleleft < 0) {
                    // constant anim, reset (need this cos animating==0)
                    if (chi->idletime == 0)
                        chi->frame = 0;
                    // one-off anim, stop
                    else {
                        ReleaseCharacterView(chi->index_id);
                        chi->idleleft = chi->idletime;
                    }
                }
            }
        }

        chi->wait = views[view].loops[chi->loop].frames[chi->frame].speed;
        // idle anim doesn't have speed stored cos animating==0 (TODO: investigate why?)
        if (chi->idleleft < 0)
          chi->wait += chi->idle_anim_speed;
        else 
          chi->wait += chi->get_anim_delay();

        if (chi->frame != oldframe)
          chex->CheckViewFrame(chi);

        if (done_anim)
          stop_character_anim(chi);
      }
    }

	return false;
}

void UpdateCharacterFollower(CharacterInfo *chi, std::vector<int> &followingAsSheep, int &doing_nothing)
{
	if ((chi->following >= 0) && (chi->followinfo == FOLLOW_ALWAYSONTOP)) {
      // an always-on-top follow
      followingAsSheep.push_back(chi->index_id);
    }
    // not moving, but should be following another character
    else if ((chi->following >= 0) && (doing_nothing == 1)) {
      short distaway=(chi->followinfo >> 8) & 0x00ff;
      // no character in this room
      if (!game.chars[chi->following].is_enabled() || !chi->is_enabled()) ;
      else if (chi->room < 0) {
        chi->room ++; // CHECKME: what the heck is this for ???
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
            && (!game.chars[chi->following].is_enabled()))
        ;  // do nothing if the player isn't visible
      else if (chi->room != game.chars[chi->following].room) {
        chi->prevroom = chi->room;
        chi->room = game.chars[chi->following].room;

        if (chi->room == displayed_room) {
          // only move to the room-entered position if coming into
          // the current room
          if (play.entered_at_x > (thisroom.Width - 8)) {
            chi->x = thisroom.Width+8;
            chi->y = play.entered_at_y;
            }
          else if (play.entered_at_x < 8) {
            chi->x = -8;
            chi->y = play.entered_at_y;
            }
          else if (play.entered_at_y > (thisroom.Height - 8)) {
            chi->y = thisroom.Height+8;
            chi->x = play.entered_at_x;
            }
          else if (play.entered_at_y < thisroom.Edges.Top+8) {
            chi->y = thisroom.Edges.Top+1;
            chi->x = play.entered_at_x;
            }
          else {
            // not at one of the edges
            // delay for a few seconds to let the player move
            chi->room = -play.follow_change_room_timer;
          }
          if (chi->room >= 0) {
            move_character(chi, play.entered_at_x, play.entered_at_y, true /* ignwall */, true /* walk anim */);
            doing_nothing = 0;
          }
        }
      }
      else if (chi->room != displayed_room) {
        // if the characetr is following another character and
        // neither is in the current room, don't try to move
      }
      else if ((abs(game.chars[chi->following].x - chi->x) > distaway+30) ||
        (abs(game.chars[chi->following].y - chi->y) > distaway+30) ||
        ((chi->followinfo & 0x00ff) == 0)) {
        // in same room
        int goxoffs=(Random(50)-25);
        // make sure he's not standing on top of the other man
        if (goxoffs < 0) goxoffs-=distaway;
        else goxoffs+=distaway;
        move_character(chi, game.chars[chi->following].x + goxoffs,
          game.chars[chi->following].y + (Random(50)-25), false /* ignwall */, true /* walk anim */);
        doing_nothing = 0;
      }
    }
}

void UpdateCharacterIdle(CharacterInfo *chi, CharacterExtras *chex, int &doing_nothing)
{
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
    else if ((loopcounter % GetGameSpeed()==0) || (chex->process_idle_this_time == 1)) {
      chi->idleleft--;
      if (chi->idleleft == -1) {
        int useloop=chi->loop;
        debug_script_log("%s: Now idle (view %d)", chi->scrname.GetCStr(), chi->idleview+1);
		Character_LockView(chi, chi->idleview+1);
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

        animate_character(chi, useloop, chi->idle_anim_speed, (chi->idletime == 0) ? 1 : 0, 1);

        // don't set Animating while the idle anim plays (TODO: investigate why?)
        chi->animating = 0;
      }
    }  // end do idle animation
}

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
extern GameState play;
extern int char_speaking;
extern RoomStruct thisroom;
extern unsigned int loopcounter;

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

void CharacterInfo::UpdateMoveAndAnim(int &char_index, CharacterExtras *chex, std::vector<int> &followingAsSheep)
{
	if (on != 1) return;
    
	// check turning
	if (update_character_turning(chex))
		return; // still turning, break
    
    // Fixup character's view when possible
    if (view >= 0 &&
        (loop >= views[view].numLoops || views[view].loops[loop].numFrames == 0))
    {
        for (loop = 0;
            (loop < views[view].numLoops) && (views[view].loops[loop].numFrames == 0);
            ++loop);
        if (loop == views[view].numLoops) // view has no frames?!
        { // amazingly enough there are old games that allow this to happen...
            if (loaded_game_file_version >= kGameVersion_300)
                quitprintf("!Character %s is assigned view %d that has no frames!", scrname, view);
            loop = 0;
        }
    }

    int doing_nothing = 1;

	update_character_moving(char_index, chex, doing_nothing);

	// Update animating
	if (update_character_animating(char_index, doing_nothing))
		return; // further update blocked by active animation

	update_character_follower(char_index, followingAsSheep, doing_nothing);

	update_character_idle(chex, doing_nothing);

    chex->process_idle_this_time = 0;
}

void CharacterInfo::UpdateFollowingExactlyCharacter()
{
	x = game.chars[following].x;
    y = game.chars[following].y;
    z = game.chars[following].z;
    room = game.chars[following].room;
    prevroom = game.chars[following].prevroom;

    int usebase = game.chars[following].get_baseline();

    if (flags & CHF_BEHINDSHEPHERD)
      baseline = usebase - 1;
    else
      baseline = usebase + 1;
}

bool CharacterInfo::update_character_turning(CharacterExtras *chex)
{
    if (walking >= TURNING_AROUND) {
      // Currently rotating to correct direction
      if (walkwait > 0) walkwait--;
      else {
        // Work out which direction is next
        int wantloop = find_looporder_index(loop) + 1;
        // going anti-clockwise, take one before instead
        if (walking >= TURNING_BACKWARDS)
          wantloop -= 2;
        while (1) {
          if (wantloop >= 8)
            wantloop = 0;
          if (wantloop < 0)
            wantloop = 7;
          if ((turnlooporder[wantloop] >= views[view].numLoops) ||
              (views[view].loops[turnlooporder[wantloop]].numFrames < 1) ||
              ((turnlooporder[wantloop] >= 4) && ((flags & CHF_NODIAGONAL)!=0))) {
            if (walking >= TURNING_BACKWARDS)
              wantloop--;
            else
              wantloop++;
          }
          else break;
        }
        loop = turnlooporder[wantloop];
        if (frame >= views[view].loops[loop].numFrames) frame = 0; // AVD: make sure the loop always has a valid frame
        walking -= TURNING_AROUND;
        // if still turning, wait for next frame
        if (walking % TURNING_BACKWARDS >= TURNING_AROUND)
          walkwait = animspeed;
        else
          walking = walking % TURNING_BACKWARDS;
        chex->animwait = 0;
      }
	  return true;
    }

	return false;
}

void CharacterInfo::update_character_moving(int &char_index, CharacterExtras *chex, int &doing_nothing)
{
	if ((walking > 0) && (room == displayed_room))
    {
      if (walkwait > 0) walkwait--;
      else 
      {
        flags &= ~CHF_AWAITINGMOVE;

        // Move the character
        int numSteps = wantMoveNow(this, chex);

        if ((numSteps) && (chex->xwas != INVALID_X)) {
          // if the zoom level changed mid-move, the walkcounter
          // might not have come round properly - so sort it out
          x = chex->xwas;
          y = chex->ywas;
          chex->xwas = INVALID_X;
        }

        int oldxp = x, oldyp = y;

        for (int ff = 0; ff < abs(numSteps); ff++) {
          if (doNextCharMoveStep (this, char_index, chex))
            break;
          if ((walking == 0) || (walking >= TURNING_AROUND))
            break;
        }

        if (numSteps < 0) {
          // very small scaling, intersperse the movement
          // to stop it being jumpy
          chex->xwas = x;
          chex->ywas = y;
          x = ((x) - oldxp) / 2 + oldxp;
          y = ((y) - oldyp) / 2 + oldyp;
        }
        else if (numSteps > 0)
          chex->xwas = INVALID_X;

        if ((flags & CHF_ANTIGLIDE) == 0)
          walkwaitcounter++;
      }

      if (loop >= views[view].numLoops)
        quitprintf("Unable to render character %d (%s) because loop %d does not exist in view %d",
            index_id, scrname, loop, view + 1);

      // check don't overflow loop
      int framesInLoop = views[view].loops[loop].numFrames;
      if (frame > framesInLoop)
      {
        frame = 1;

        if (framesInLoop < 2)
          frame = 0;

        if (framesInLoop < 1)
          quitprintf("Unable to render character %d (%s) because there are no frames in loop %d",
              index_id, scrname, loop);
      }

      doing_nothing = 0; // still walking?

      if (walking < 1) {
        // Finished walking, stop and reset state
        chex->process_idle_this_time = 1;
        doing_nothing=1;
        walkwait=0;
        Character_StopMoving(this);
        if ((flags & CHF_MOVENOTWALK) == 0) {
            // use standing pic
            chex->animwait = 0;
            frame = 0;
            chex->CheckViewFrame(this);
        }
      }
      else if (chex->animwait > 0) {
          chex->animwait--;
      } else {
        if (flags & CHF_ANTIGLIDE)
          walkwaitcounter++;

        if ((flags & CHF_MOVENOTWALK) == 0)
        {
          frame++;
          if (frame >= views[view].loops[loop].numFrames)
          {
            // end of loop, so loop back round skipping the standing frame
            frame = 1;

            if (views[view].loops[loop].numFrames < 2)
              frame = 0;
          }

          chex->animwait = views[view].loops[loop].frames[frame].speed + animspeed;

          if (flags & CHF_ANTIGLIDE)
            walkwait = chex->animwait;
          else
            walkwait = 0;

          chex->CheckViewFrame(this);
        }
      }
    }
}

bool CharacterInfo::update_character_animating(int &aa, int &doing_nothing)
{
    auto &chex = charextra[index_id];

	// not moving, but animating
    // idleleft is <0 while idle view is playing (.animating is 0)
    if (((animating != 0) || (idleleft < 0)) &&
        ((walking == 0) || ((flags & CHF_MOVENOTWALK) != 0)) &&
        (room == displayed_room)) 
    {
      doing_nothing = 0;
      // idle anim doesn't count as doing something
      if (idleleft < 0)
        doing_nothing = 1;

      if (wait>0) wait--;
      else if ((char_speaking == aa) && (game.options[OPT_LIPSYNCTEXT] != 0)) {
        // currently talking with lip-sync speech
        int fraa = frame;
        wait = update_lip_sync (view, loop, &fraa) - 1;
        // closed mouth at end of sentence
        // NOTE: standard lip-sync is synchronized with text timer, not voice file
        if (play.speech_in_post_state ||
            ((play.messagetime >= 0) && (play.messagetime < play.close_mouth_speech_time)))
          frame = 0;

        if (frame != fraa) {
          frame = fraa;
          chex.CheckViewFrame(this);
        }

		return true;
      }
      else {
        // Normal view animation
        const int oldframe = frame;

        bool done_anim = false;
        if ((aa == char_speaking) &&
            (play.speech_in_post_state ||
            ((!play.speech_has_voice) &&
                (play.close_mouth_speech_time > 0) &&
                (play.messagetime < play.close_mouth_speech_time)))) {
            // finished talking - stop animation
            done_anim = true;
            frame = 0;
        } else {
            if (!CycleViewAnim(view, loop, frame, get_anim_forwards(), get_anim_repeat())) {
                done_anim = true; // finished animating
                // end of idle anim
                if (idleleft < 0) {
                    // constant anim, reset (need this cos animating==0)
                    if (idletime == 0)
                        frame = 0;
                    // one-off anim, stop
                    else {
                        ReleaseCharacterView(aa);
                        idleleft = idletime;
                    }
                }
            }
        }

        wait = views[view].loops[loop].frames[frame].speed;
        // idle anim doesn't have speed stored cos animating==0 (TODO: investigate why?)
        if (idleleft < 0)
          wait += idle_anim_speed;
        else 
          wait += get_anim_delay();

        if (frame != oldframe)
          chex.CheckViewFrame(this);

        if (done_anim)
          stop_character_anim(this);
      }
    }

	return false;
}

void CharacterInfo::update_character_follower(int &aa, std::vector<int> &followingAsSheep, int &doing_nothing)
{
	if ((following >= 0) && (followinfo == FOLLOW_ALWAYSONTOP)) {
      // an always-on-top follow
      followingAsSheep.push_back(aa);
    }
    // not moving, but should be following another character
    else if ((following >= 0) && (doing_nothing == 1)) {
      short distaway=(followinfo >> 8) & 0x00ff;
      // no character in this room
      if ((game.chars[following].on == 0) || (on == 0)) ;
      else if (room < 0) {
        room ++;
        if (room == 0) {
          // appear in the new room
          room = game.chars[following].room;
          x = play.entered_at_x;
          y = play.entered_at_y;
        }
      }
      // wait a bit, so we're not constantly walking
      else if (Random(100) < (followinfo & 0x00ff)) ;
      // the followed character has changed room
      else if ((room != game.chars[following].room)
            && (game.chars[following].on == 0))
        ;  // do nothing if the player isn't visible
      else if (room != game.chars[following].room) {
        prevroom = room;
        room = game.chars[following].room;

        if (room == displayed_room) {
          // only move to the room-entered position if coming into
          // the current room
          if (play.entered_at_x > (thisroom.Width - 8)) {
            x = thisroom.Width+8;
            y = play.entered_at_y;
            }
          else if (play.entered_at_x < 8) {
            x = -8;
            y = play.entered_at_y;
            }
          else if (play.entered_at_y > (thisroom.Height - 8)) {
            y = thisroom.Height+8;
            x = play.entered_at_x;
            }
          else if (play.entered_at_y < thisroom.Edges.Top+8) {
            y = thisroom.Edges.Top+1;
            x = play.entered_at_x;
            }
          else {
            // not at one of the edges
            // delay for a few seconds to let the player move
            room = -play.follow_change_room_timer;
          }
          if (room >= 0) {
            walk_character(aa,play.entered_at_x,play.entered_at_y,1, true);
            doing_nothing = 0;
          }
        }
      }
      else if (room != displayed_room) {
        // if the characetr is following another character and
        // neither is in the current room, don't try to move
      }
      else if ((abs(game.chars[following].x - x) > distaway+30) |
        (abs(game.chars[following].y - y) > distaway+30) |
        ((followinfo & 0x00ff) == 0)) {
        // in same room
        int goxoffs=(Random(50)-25);
        // make sure he's not standing on top of the other man
        if (goxoffs < 0) goxoffs-=distaway;
        else goxoffs+=distaway;
        walk_character(aa,game.chars[following].x + goxoffs,
          game.chars[following].y + (Random(50)-25),0, true);
        doing_nothing = 0;
      }
    }
}

void CharacterInfo::update_character_idle(CharacterExtras *chex, int &doing_nothing)
{
	// no idle animation, so skip this bit
    if (idleview < 1) ;
    // currently playing idle anim
    else if (idleleft < 0) ;
    // not in the current room
    else if (room != displayed_room) ;
    // they are moving or animating (or the view is locked), so 
    // reset idle timeout
    else if ((doing_nothing == 0) || ((flags & CHF_FIXVIEW) != 0))
      idleleft = idletime;
    // count idle time
    else if ((loopcounter % GetGameSpeed()==0) || (chex->process_idle_this_time == 1)) {
      idleleft--;
      if (idleleft == -1) {
        int useloop=loop;
        debug_script_log("%s: Now idle (view %d)", scrname, idleview+1);
		Character_LockView(this, idleview+1);
        // SetCharView resets it to 0
        idleleft = -2;
        int maxLoops = views[idleview].numLoops;
        // if the char is set to "no diagonal loops", don't try
        // to use diagonal idle loops either
        if ((maxLoops > 4) && (useDiagonal(this)))
          maxLoops = 4;
        // If it's not a "swimming"-type idleanim, choose a random loop
        // if there arent enough loops to do the current one.
        if ((idletime > 0) && (useloop >= maxLoops)) {
          do {
            useloop = rand() % maxLoops;
          // don't select a loop which is a continuation of a previous one
          } while ((useloop > 0) && (views[idleview].loops[useloop-1].RunNextLoop()));
        }
        // Normal idle anim - just reset to loop 0 if not enough to
        // use the current one
        else if (useloop >= maxLoops)
          useloop = 0;

        animate_character(this, useloop, idle_anim_speed, (idletime == 0) ? 1 : 0, 1);

        // don't set Animating while the idle anim plays (TODO: investigate why?)
        animating = 0;
      }
    }  // end do idle animation
}

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
// Game update procedure
//

#include "main/mainheader.h"
#include "main/update.h"


#define MAX_SHEEP 30	// sheep == follower


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


void update_script_timers()
{
  if (play.gscript_timer > 0) play.gscript_timer--;
  for (int aa=0;aa<MAX_TIMERS;aa++) {
    if (play.script_timers[aa] > 1) play.script_timers[aa]--;
    }
}

void update_cycle_view_forwards(int &aa)
{
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
}

void update_cycle_view_backwards(int &aa)
{
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

void update_cycling_views()
{
	// update graphics for object if cycling view
  for (int aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on != 1) continue;
    if (objs[aa].moving>0) {
      do_movelist_move(&objs[aa].moving,&objs[aa].x,&objs[aa].y);
      }
    if (objs[aa].cycling==0) continue;
    if (objs[aa].view<0) continue;
    if (objs[aa].wait>0) { objs[aa].wait--; continue; }

    if (objs[aa].cycling >= ANIM_BACKWARDS) {

      update_cycle_view_backwards(aa);

    }
    else {  // Animate forwards
      
	  update_cycle_view_forwards(aa);

    }  // end if forwards

    ViewFrame*vfptr=&views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame];
    objs[aa].num = vfptr->pic;

    if (objs[aa].cycling == 0)
      continue;

    objs[aa].wait=vfptr->speed+objs[aa].overall_speed;
    CheckViewFrame (objs[aa].view, objs[aa].loop, objs[aa].frame);
  }
}

void update_shadow_areas()
{
	// shadow areas
  int onwalkarea = get_walkable_area_at_character (game.playercharacter);
  if (onwalkarea<0) ;
  else if (playerchar->flags & CHF_FIXVIEW) ;
  else { onwalkarea=thisroom.shadinginfo[onwalkarea];
    if (onwalkarea>0) playerchar->view=onwalkarea-1;
    else if (thisroom.options[ST_MANVIEW]==0) playerchar->view=playerchar->defview;
    else playerchar->view=thisroom.options[ST_MANVIEW]-1;
  }
}

int update_character_walking(CharacterInfo*chi, int &aa)
{
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
	  return RETURN_CONTINUE;
      //continue;
    }

	return 0;
}

void update_character_moving(CharacterInfo*chi, int &aa, int &doing_nothing)
{
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
}

int update_character_animating(CharacterInfo*chi, int &aa, int &doing_nothing)
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
        
        //continue;
		return RETURN_CONTINUE;
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

	return 0;
}

void update_character_follower(CharacterInfo*chi, int &aa, int &numSheep, int *followingAsSheep, int &doing_nothing)
{
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
}

void update_character_idle(CharacterInfo*chi, int &aa, int &doing_nothing)
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
}

void update_character_move_and_anim(int &numSheep, int *followingAsSheep)
{
	int res;

	// move & animate characters
  for (int aa=0;aa<game.numcharacters;aa++) {
    if (game.chars[aa].on != 1) continue;
    CharacterInfo*chi=&game.chars[aa];
    
	// walking
	res = update_character_walking(chi, aa);
	if (res == RETURN_CONTINUE) { // [IKM] now, this is one of those places...
		continue;				  //  must be careful not to screw things up
	}
    
    // Make sure it doesn't flash up a blue cup
    if (chi->view < 0) ;
    else if (chi->loop >= views[chi->view].numLoops)
      chi->loop = 0;

    int doing_nothing = 1;

	update_character_moving(chi, aa, doing_nothing);

	res = update_character_animating(chi, aa, doing_nothing);
	if (res == RETURN_CONTINUE) { // [IKM] now, this is one of those places...
		continue;				  //  must be careful not to screw things up
	}    

	update_character_follower(chi, aa, numSheep, followingAsSheep, doing_nothing);

	update_character_idle(chi, aa, doing_nothing);

    charextra[aa].process_idle_this_time = 0;
  }
}

void update_following_exactly_characters(int &numSheep, int *followingAsSheep)
{
	// update location of all following_exactly characters
  for (int aa = 0; aa < numSheep; aa++) {
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
}

void update_overlay_timers()
{
	// update overlay timers
  for (int aa=0;aa<numscreenover;aa++) {
    if (screenover[aa].timeout > 0) {
      screenover[aa].timeout--;
      if (screenover[aa].timeout == 0)
        remove_screen_overlay(screenover[aa].type);
    }
  }
}

void update_speech_and_messages()
{
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
}

void update_sierra_speech()
{
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
}

// update_stuff: moves and animates objects, executes repeat scripts, and
// the like.
void update_stuff() {
  
  our_eip = 20;

  update_script_timers();

  update_cycling_views();

  our_eip = 21;

  update_shadow_areas();
  
  our_eip = 22;

  int numSheep = 0;
  int followingAsSheep[MAX_SHEEP];

  update_character_move_and_anim(numSheep, followingAsSheep);

  update_following_exactly_characters(numSheep, followingAsSheep);

  our_eip = 23;

  update_overlay_timers();

  update_speech_and_messages();

  our_eip = 24;

  update_sierra_speech();

  our_eip = 25;
}

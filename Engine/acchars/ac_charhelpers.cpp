/*
  AGS Character functions

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#include <stdio.h>
#include "wgt2allg.h"
#include "acchars/ac_charhelpers.h"
//#include "acruntim.h"
#include "acmain/ac_commonheaders.h"
#include "routefnd.h"
#include <math.h>

extern int loaded_game_file_version;

// order of loops to turn character in circle from down to down
int turnlooporder[8] = {0, 6, 1, 7, 3, 5, 2, 4};

void StopMoving(int chaa) {

    Character_StopMoving(&game.chars[chaa]);
}

void ReleaseCharacterView(int chat) {
    if (!is_valid_character(chat))
        quit("!ReleaseCahracterView: invalid character supplied");

    Character_UnlockView(&game.chars[chat]);
}

void walk_character(int chac,int tox,int toy,int ignwal, bool autoWalkAnims) {
    CharacterInfo*chin=&game.chars[chac];
    if (chin->room!=displayed_room)
        quit("!MoveCharacter: character not in current room");

    chin->flags &= ~CHF_MOVENOTWALK;

    int toxPassedIn = tox, toyPassedIn = toy;
    int charX = convert_to_low_res(chin->x);
    int charY = convert_to_low_res(chin->y);
    tox = convert_to_low_res(tox);
    toy = convert_to_low_res(toy);

    if ((tox == charX) && (toy == charY)) {
        StopMoving(chac);
        DEBUG_CONSOLE("%s already at destination, not moving", chin->scrname);
        return;
    }

    if ((chin->animating) && (autoWalkAnims))
        chin->animating = 0;

    if (chin->idleleft < 0) {
        ReleaseCharacterView(chac);
        chin->idleleft=chin->idletime;
    }
    // stop them to make sure they're on a walkable area
    // but save their frame first so that if they're already
    // moving it looks smoother
    int oldframe = chin->frame;
    int waitWas = 0, animWaitWas = 0;
    // if they are currently walking, save the current Wait
    if (chin->walking)
    {
        waitWas = chin->walkwait;
        animWaitWas = charextra[chac].animwait;
    }

    StopMoving (chac);
    chin->frame = oldframe;
    // use toxPassedIn cached variable so the hi-res co-ordinates
    // are still displayed as such
    DEBUG_CONSOLE("%s: Start move to %d,%d", chin->scrname, toxPassedIn, toyPassedIn);

    int move_speed_x = chin->walkspeed;
    int move_speed_y = chin->walkspeed;

    if (chin->walkspeed_y != UNIFORM_WALK_SPEED)
        move_speed_y = chin->walkspeed_y;

    if ((move_speed_x == 0) && (move_speed_y == 0)) {
        debug_log("Warning: MoveCharacter called for '%s' with walk speed 0", chin->name);
    }

    set_route_move_speed(move_speed_x, move_speed_y);
    set_color_depth(8);
    int mslot=find_route(charX, charY, tox, toy, prepare_walkable_areas(chac), chac+CHMLSOFFS, 1, ignwal);
    set_color_depth(final_col_dep);
    if (mslot>0) {
        chin->walking = mslot;
        mls[mslot].direct = ignwal;

        if ((game.options[OPT_NATIVECOORDINATES] != 0) &&
            (game.default_resolution > 2))
        {
            convert_move_path_to_high_res(&mls[mslot]);
        }
        // cancel any pending waits on current animations
        // or if they were already moving, keep the current wait - 
        // this prevents a glitch if MoveCharacter is called when they
        // are already moving
        if (autoWalkAnims)
        {
            chin->walkwait = waitWas;
            charextra[chac].animwait = animWaitWas;

            if (mls[mslot].pos[0] != mls[mslot].pos[1]) {
                fix_player_sprite(&mls[mslot],chin);
            }
        }
        else
            chin->flags |= CHF_MOVENOTWALK;
    }
    else if (autoWalkAnims) // pathfinder couldn't get a route, stand them still
        chin->frame = 0;
}

int find_looporder_index (int curloop) {
    int rr;
    for (rr = 0; rr < 8; rr++) {
        if (turnlooporder[rr] == curloop)
            return rr;
    }
    return 0;
}

// returns 0 to use diagonal, 1 to not
int useDiagonal (CharacterInfo *char1) {
    if ((views[char1->view].numLoops < 8) || ((char1->flags & CHF_NODIAGONAL)!=0))
        return 1;
    // If they have just provided standing frames for loops 4-7, to
    // provide smoother turning
    if (views[char1->view].loops[4].numFrames < 2)
        return 2;
    return 0;
}

// returns 1 normally, or 0 if they only have horizontal animations
int hasUpDownLoops(CharacterInfo *char1) {
    // if no loops in the Down animation
    // or no loops in the Up animation
    if ((views[char1->view].loops[0].numFrames < 1) ||
        (views[char1->view].numLoops < 4) ||
        (views[char1->view].loops[3].numFrames < 1))
    {
        return 0;
    }

    return 1;
}

void start_character_turning (CharacterInfo *chinf, int useloop, int no_diagonal) {
    // work out how far round they have to turn 
    int fromidx = find_looporder_index (chinf->loop);
    int toidx = find_looporder_index (useloop);
    //Display("Curloop: %d, needloop: %d",chinf->loop, useloop);
    int ii, go_anticlock = 0;
    // work out whether anticlockwise is quicker or not
    if ((toidx > fromidx) && ((toidx - fromidx) > 4))
        go_anticlock = 1;
    if ((toidx < fromidx) && ((fromidx - toidx) < 4))
        go_anticlock = 1;
    // strip any current turning_around stages
    chinf->walking = chinf->walking % TURNING_AROUND;
    if (go_anticlock)
        chinf->walking += TURNING_BACKWARDS;
    else
        go_anticlock = -1;

    // Allow the diagonal frames just for turning
    if (no_diagonal == 2)
        no_diagonal = 0;

    for (ii = fromidx; ii != toidx; ii -= go_anticlock) {
        if (ii < 0)
            ii = 7;
        if (ii >= 8)
            ii = 0;
        if (ii == toidx)
            break;
        if ((turnlooporder[ii] >= 4) && (no_diagonal > 0))
            continue;
        if (views[chinf->view].loops[turnlooporder[ii]].numFrames < 1)
            continue;
        if (turnlooporder[ii] < views[chinf->view].numLoops)
            chinf->walking += TURNING_AROUND;
    }

}



// loops: 0=down, 1=left, 2=right, 3=up, 4=down-right, 5=up-right,
// 6=down-left, 7=up-left
void fix_player_sprite(MoveList*cmls,CharacterInfo*chinf) {
    int view_num=chinf->view;
    int want_horiz=1,useloop = 1,no_diagonal=0;
    fixed xpmove = cmls->xpermove[cmls->onstage];
    fixed ypmove = cmls->ypermove[cmls->onstage];

    // if not moving, do nothing
    if ((xpmove == 0) && (ypmove == 0))
        return;

    no_diagonal = useDiagonal (chinf);

    // Different logic for 2.x.
    if (loaded_game_file_version <= 32)
    {
        bool can_right = ((views[chinf->view].numLoops >= 3) && (views[chinf->view].loops[2].numFrames > 0));
        bool can_left = ((views[chinf->view].numLoops >= 2) && (views[chinf->view].loops[1].numFrames > 0));

        if (abs(ypmove) < abs(xpmove))
        {
            if (!can_left && !can_right)
                useloop = 0;
            else if (can_right && (xpmove >= 0)) {
                useloop=2;
                CHECK_DIAGONAL(ypmove, xpmove, 5, 4)
            }
            else if (can_left && (xpmove < 0)) {
                useloop=1;
                CHECK_DIAGONAL(ypmove, xpmove,7,6)
            }
        }
        else
        {
            if (ypmove>=0) {
                useloop=0;
                CHECK_DIAGONAL(xpmove ,ypmove ,6,4)
            }
            else if (ypmove<0) {
                useloop=3;
                CHECK_DIAGONAL(xpmove, ypmove,7,5)
            }
        }
    }
    else
    {
        if (hasUpDownLoops(chinf) == 0)
            want_horiz = 1;
        else if (abs(ypmove) > abs(xpmove))
            want_horiz = 0;

        if ((want_horiz==1) && (xpmove > 0)) {
            // right
            useloop=2;
            // diagonal up-right/down-right
            CHECK_DIAGONAL(ypmove,xpmove,5,4)
        }
        else if ((want_horiz==1) && (xpmove <= 0)) {
            // left
            useloop=1;
            // diagonal up-left/down-left
            CHECK_DIAGONAL(ypmove,xpmove,7,6)
        }
        else if (ypmove < 0) {
            // up
            useloop=3;
            // diagonal up-left/up-right
            CHECK_DIAGONAL(xpmove,ypmove,7,5)
        }
        else {
            // down
            useloop=0;
            // diagonal down-left/down-right
            CHECK_DIAGONAL(xpmove,ypmove,6,4)
        }
    }

    if ((game.options[OPT_ROTATECHARS] == 0) || ((chinf->flags & CHF_NOTURNING) != 0)) {
        chinf->loop = useloop;
        return;
    }
    if ((chinf->loop > 3) && ((chinf->flags & CHF_NODIAGONAL)!=0)) {
        // They've just been playing an animation with an extended loop number,
        // so don't try and rotate using it
        chinf->loop = useloop;
        return;
    }
    if ((chinf->loop >= views[chinf->view].numLoops) ||
        (views[chinf->view].loops[chinf->loop].numFrames < 1) ||
        (hasUpDownLoops(chinf) == 0)) {
            // Character is not currently on a valid loop, so don't try to rotate
            // eg. left/right only view, but current loop 0
            chinf->loop = useloop;
            return;
    }
    start_character_turning (chinf, useloop, no_diagonal);
}

// Check whether two characters have walked into each other
int has_hit_another_character(int sourceChar) {

    // if the character who's moving doesn't block, don't bother checking
    if (game.chars[sourceChar].flags & CHF_NOBLOCKING)
        return -1;

    for (int ww = 0; ww < game.numcharacters; ww++) {
        if (game.chars[ww].on != 1) continue;
        if (game.chars[ww].room != displayed_room) continue;
        if (ww == sourceChar) continue;
        if (game.chars[ww].flags & CHF_NOBLOCKING) continue;

        if (is_char_on_another (sourceChar, ww, NULL, NULL)) {
            // we are now overlapping character 'ww'
            if ((game.chars[ww].walking) && 
                ((game.chars[ww].flags & CHF_AWAITINGMOVE) == 0))
                return ww;
        }

    }
    return -1;
}

// Does the next move from the character's movelist.
// Returns 1 if they are now waiting for another char to move,
// otherwise returns 0
int doNextCharMoveStep (int aa, CharacterInfo *chi) {
    int ntf=0, xwas = chi->x, ywas = chi->y;

    if (do_movelist_move(&chi->walking,&chi->x,&chi->y) == 2) 
    {
        if ((chi->flags & CHF_MOVENOTWALK) == 0)
            fix_player_sprite(&mls[chi->walking], chi);
    }

    ntf = has_hit_another_character(aa);
    if (ntf >= 0) {
        chi->walkwait = 30;
        if (game.chars[ntf].walkspeed < 5)
            chi->walkwait += (5 - game.chars[ntf].walkspeed) * 5;
        // we are now waiting for the other char to move, so
        // make sure he doesn't stop for us too

        chi->flags |= CHF_AWAITINGMOVE;

        if ((chi->flags & CHF_MOVENOTWALK) == 0)
        {
            chi->frame = 0;
            charextra[aa].animwait = chi->walkwait;
        }

        if ((chi->walking < 1) || (chi->walking >= TURNING_AROUND)) ;
        else if (mls[chi->walking].onpart > 0) {
            mls[chi->walking].onpart --;
            chi->x = xwas;
            chi->y = ywas;
        }
        DEBUG_CONSOLE("%s: Bumped into %s, waiting for them to move", chi->scrname, game.chars[ntf].scrname);
        return 1;
    }
    return 0;
}

extern int engineNeedsAsInt;

int find_nearest_walkable_area_within(int *xx, int *yy, int range, int step)
{
    int ex, ey, nearest = 99999, thisis, nearx = 0, neary = 0;
    int startx = 0, starty = 14;
    int roomWidthLowRes = convert_to_low_res(thisroom.width);
    int roomHeightLowRes = convert_to_low_res(thisroom.height);
    int xwidth = roomWidthLowRes, yheight = roomHeightLowRes;

    int xLowRes = convert_to_low_res(xx[0]);
    int yLowRes = convert_to_low_res(yy[0]);
    int rightEdge = convert_to_low_res(thisroom.right);
    int leftEdge = convert_to_low_res(thisroom.left);
    int topEdge = convert_to_low_res(thisroom.top);
    int bottomEdge = convert_to_low_res(thisroom.bottom);

    // tweak because people forget to move the edges sometimes
    // if the player is already over the edge, ignore it
    if (xLowRes >= rightEdge) rightEdge = roomWidthLowRes;
    if (xLowRes <= leftEdge) leftEdge = 0;
    if (yLowRes >= bottomEdge) bottomEdge = roomHeightLowRes;
    if (yLowRes <= topEdge) topEdge = 0;

    if (range > 0) 
    {
        startx = xLowRes - range;
        starty = yLowRes - range;
        xwidth = startx + range * 2;
        yheight = starty + range * 2;
        if (startx < 0) startx = 0;
        if (starty < 10) starty = 10;
        if (xwidth > roomWidthLowRes) xwidth = roomWidthLowRes;
        if (yheight > roomHeightLowRes) yheight = roomHeightLowRes;
    }

    for (ex = startx; ex < xwidth; ex += step) {
        for (ey = starty; ey < yheight; ey += step) {
            // non-walkalbe, so don't go here
            if (getpixel(thisroom.walls,ex,ey) == 0) continue;
            // off a screen edge, don't move them there
            if ((ex <= leftEdge) || (ex >= rightEdge) ||
                (ey <= topEdge) || (ey >= bottomEdge))
                continue;
            // otherwise, calculate distance from target
            thisis=(int) ::sqrt((double)((ex - xLowRes) * (ex - xLowRes) + (ey - yLowRes) * (ey - yLowRes)));
            if (thisis<nearest) { nearest=thisis; nearx=ex; neary=ey; }
        }
    }
    if (nearest < 90000) 
    {
        xx[0] = convert_back_to_high_res(nearx);
        yy[0] = convert_back_to_high_res(neary);
        return 1;
    }

    return 0;
}

void find_nearest_walkable_area (int *xx, int *yy) {


    int pixValue = getpixel(thisroom.walls, convert_to_low_res(xx[0]), convert_to_low_res(yy[0]));
    // only fix this code if the game was built with 2.61 or above
    if (pixValue == 0 || (engineNeedsAsInt >=261 && pixValue < 1))
    {
        // First, check every 2 pixels within immediate area
        if (!find_nearest_walkable_area_within(xx, yy, 20, 2))
        {
            // If not, check whole screen at 5 pixel intervals
            find_nearest_walkable_area_within(xx, yy, -1, 5);
        }
    }

}

void MoveToWalkableArea(int charid) {
    if (!is_valid_character(charid))
        quit("!MoveToWalkableArea: invalid character specified");

    Character_PlaceOnWalkableArea(&game.chars[charid]);
}

void FaceLocation(int cha, int xx, int yy) {
    if (!is_valid_character(cha))
        quit("!FaceLocation: Invalid character specified");

    Character_FaceLocation(&game.chars[cha], xx, yy, BLOCKING);
}

void FaceCharacter(int cha,int toface) {
    if (!is_valid_character(cha))
        quit("!FaceCharacter: Invalid character specified");
    if (!is_valid_character(toface)) 
        quit("!FaceCharacter: invalid character specified");

    Character_FaceCharacter(&game.chars[cha], &game.chars[toface], BLOCKING);
}

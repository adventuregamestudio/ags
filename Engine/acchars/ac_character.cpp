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
#include "acchars/ac_character.h"
#include "acrun/ac_inventoryitem.h"
#include "ac/ac_common.h"           // quit()
//#include "acruntim.h"
#include "acchars/ac_charhelpers.h"
#include "acrun/ac_runninggame.h"
#include "acmain/ac_event.h"
#include "acgui/ac_guimain.h"
#include "routefnd.h"
#include "acmain/ac_main.h"
#include "acmain/ac_room.h"
#include "acdebug/ac_debug.h"
#include "acrun/ac_scriptobject.h"
#include "acmain/ac_collision.h"
#include "acmain/ac_mouse.h"
#include "acmain/ac_strings.h"
#include "acmain/ac_translation.h"
#include "acmain/ac_message.h"
#include "acmain/ac_speech.h"
#include "acmain/ac_overlay.h"
#include "acmain/ac_location.h"
#include "acmain/ac_cutscene.h"
#include "acmain/ac_inventory.h"
#include "acmain/ac_walkablearea.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_string.h"
#include "acmain/ac_viewframe.h"
#include "acmain/ac_object.h"

extern int loaded_game_file_version;

// **** CHARACTER: FUNCTIONS ****

CharacterExtras *charextra;
CharacterInfo*playerchar;
long _sc_PlayerCharPtr = 0;


void Character_AddInventory(CharacterInfo *chaa, ScriptInvItem *invi, int addIndex) {
    int ee;

    if (invi == NULL)
        quit("!AddInventoryToCharacter: invalid invnetory number");

    int inum = invi->id;

    if (chaa->inv[inum] >= 32000)
        quit("!AddInventory: cannot carry more than 32000 of one inventory item");

    chaa->inv[inum]++;

    int charid = chaa->index_id;

    if (game.options[OPT_DUPLICATEINV] == 0) {
        // Ensure it is only in the list once
        for (ee = 0; ee < charextra[charid].invorder_count; ee++) {
            if (charextra[charid].invorder[ee] == inum) {
                // They already have the item, so don't add it to the list
                if (chaa == playerchar)
                    run_on_event (GE_ADD_INV, inum);
                return;
            }
        }
    }
    if (charextra[charid].invorder_count >= MAX_INVORDER)
        quit("!Too many inventory items added, max 500 display at one time");

    if ((addIndex == SCR_NO_VALUE) ||
        (addIndex >= charextra[charid].invorder_count) ||
        (addIndex < 0)) {
            // add new item at end of list
            charextra[charid].invorder[charextra[charid].invorder_count] = inum;
    }
    else {
        // insert new item at index
        for (ee = charextra[charid].invorder_count - 1; ee >= addIndex; ee--)
            charextra[charid].invorder[ee + 1] = charextra[charid].invorder[ee];

        charextra[charid].invorder[addIndex] = inum;
    }
    charextra[charid].invorder_count++;
    guis_need_update = 1;
    if (chaa == playerchar)
        run_on_event (GE_ADD_INV, inum);

}

void Character_AddWaypoint(CharacterInfo *chaa, int x, int y) {

    if (chaa->room != displayed_room)
        quit("!MoveCharacterPath: specified character not in current room");

    // not already walking, so just do a normal move
    if (chaa->walking <= 0) {
        Character_Walk(chaa, x, y, IN_BACKGROUND, ANYWHERE);
        return;
    }

    MoveList *cmls = &mls[chaa->walking % TURNING_AROUND];
    if (cmls->numstage >= MAXNEEDSTAGES)
        quit("!MoveCharacterPath: move is too complex, cannot add any further paths");

    cmls->pos[cmls->numstage] = (x << 16) + y;
    // They're already walking there anyway
    if (cmls->pos[cmls->numstage] == cmls->pos[cmls->numstage - 1])
        return;

    calculate_move_stage (cmls, cmls->numstage-1);
    cmls->numstage ++;

}

void Character_Animate(CharacterInfo *chaa, int loop, int delay, int repeat, int blocking, int direction) {

    if (direction == FORWARDS)
        direction = 0;
    else if (direction == BACKWARDS)
        direction = 1;
    else
        quit("!Character.Animate: Invalid DIRECTION parameter");

    animate_character(chaa, loop, delay, repeat, 0, direction);

    if ((blocking == BLOCKING) || (blocking == 1))
        do_main_cycle(UNTIL_SHORTIS0,(int)&chaa->animating);
    else if ((blocking != IN_BACKGROUND) && (blocking != 0))
        quit("!Character.Animate: Invalid BLOCKING parameter");
}

void Character_ChangeRoomAutoPosition(CharacterInfo *chaa, int room, int newPos) 
{
    if (chaa->index_id != game.playercharacter) 
    {
        quit("!Character.ChangeRoomAutoPosition can only be used with the player character.");
    }

    new_room_pos = newPos;

    if (new_room_pos == 0) {
        // auto place on other side of screen
        if (chaa->x <= thisroom.left + 10)
            new_room_pos = 2000;
        else if (chaa->x >= thisroom.right - 10)
            new_room_pos = 1000;
        else if (chaa->y <= thisroom.top + 10)
            new_room_pos = 3000;
        else if (chaa->y >= thisroom.bottom - 10)
            new_room_pos = 4000;

        if (new_room_pos < 3000)
            new_room_pos += chaa->y;
        else
            new_room_pos += chaa->x;
    }
    NewRoom(room);
}

void Character_ChangeRoom(CharacterInfo *chaa, int room, int x, int y) {

    if (chaa->index_id != game.playercharacter) {
        // NewRoomNPC
        if ((x != SCR_NO_VALUE) && (y != SCR_NO_VALUE)) {
            chaa->x = x;
            chaa->y = y;
        }
        chaa->prevroom = chaa->room;
        chaa->room = room;

        DEBUG_CONSOLE("%s moved to room %d, location %d,%d",
            chaa->scrname, room, chaa->x, chaa->y);

        return;
    }

    if ((x != SCR_NO_VALUE) && (y != SCR_NO_VALUE)) {
        new_room_pos = 0;

        if (loaded_game_file_version <= 32)
        {
            // Set position immediately on 2.x.
            chaa->x = x;
            chaa->y = y;
        }
        else
        {
            // don't check X or Y bounds, so that they can do a
            // walk-in animation if they want
            new_room_x = x;
            new_room_y = y;
        }
    }

    NewRoom(room);
}

void FindReasonableLoopForCharacter(CharacterInfo *chap) {

    if (chap->loop >= views[chap->view].numLoops)
        chap->loop=0;
    if (views[chap->view].numLoops < 1)
        quitprintf("!View %d does not have any loops", chap->view + 1);

    // if the current loop has no frames, find one that does
    if (views[chap->view].loops[chap->loop].numFrames < 1) 
    {
        for (int i = 0; i < views[chap->view].numLoops; i++) 
        {
            if (views[chap->view].loops[i].numFrames > 0) {
                chap->loop = i;
                break;
            }
        }
    }

}

void Character_ChangeView(CharacterInfo *chap, int vii) {
    vii--;

    if ((vii < 0) || (vii >= game.numviews))
        quit("!ChangeCharacterView: invalid view number specified");

    // if animating, but not idle view, give warning message
    if ((chap->flags & CHF_FIXVIEW) && (chap->idleleft >= 0))
        debug_log("Warning: ChangeCharacterView was used while the view was fixed - call ReleaseCharView first");

    DEBUG_CONSOLE("%s: Change view to %d", chap->scrname, vii+1);
    chap->defview = vii;
    chap->view = vii;
    chap->animating = 0;
    chap->frame = 0;
    chap->wait = 0;
    chap->walkwait = 0;
    charextra[chap->index_id].animwait = 0;
    FindReasonableLoopForCharacter(chap);
}

void Character_FaceCharacter(CharacterInfo *char1, CharacterInfo *char2, int blockingStyle) {
    if (char2 == NULL) 
        quit("!FaceCharacter: invalid character specified");

    if (char1->room != char2->room)
        quit("!FaceCharacter: characters are in different rooms");

    Character_FaceLocation(char1, char2->x, char2->y, blockingStyle);
}

void Character_FaceLocation(CharacterInfo *char1, int xx, int yy, int blockingStyle) {
    DEBUG_CONSOLE("%s: Face location %d,%d", char1->scrname, xx, yy);

    int diffrx = xx - char1->x;
    int diffry = yy - char1->y;
    int useloop = 1, wanthoriz=0, no_diagonal = 0;
    int highestLoopForTurning = 3;

    if ((diffrx == 0) && (diffry == 0)) {
        // FaceLocation called on their current position - do nothing
        return;
    }

    no_diagonal = useDiagonal (char1);

    if (no_diagonal != 1) {
        highestLoopForTurning = 7;
    }

    // Use a different logic on 2.x. This fixes some edge cases where
    // FaceLocation() is used to select a specific loop.
    if (loaded_game_file_version <= 32)
    {
        bool can_right = ((views[char1->view].numLoops >= 3) && (views[char1->view].loops[2].numFrames > 0));
        bool can_left = ((views[char1->view].numLoops >= 2) && (views[char1->view].loops[1].numFrames > 0));

        if (abs(diffry) < abs(diffrx))
        {
            if (!can_left && !can_right)
                useloop = 0;
            else if (can_right && (diffrx >= 0)) {
                useloop=2;
                CHECK_DIAGONAL(diffry, diffrx, 5, 4)
            }
            else if (can_left && (diffrx < 0)) {
                useloop=1;
                CHECK_DIAGONAL(diffry, diffrx,7,6)
            }
        }
        else
        {
            if (diffry>=0) {
                useloop=0;
                CHECK_DIAGONAL(diffrx ,diffry ,6,4)
            }
            else if (diffry<0) {
                useloop=3;
                CHECK_DIAGONAL(diffrx, diffry,7,5)
            }
        }
    }
    else
    {
        if (hasUpDownLoops(char1) == 0)
            wanthoriz = 1;
        else if (abs(diffry) < abs(diffrx))
            wanthoriz = 1;

        if ((wanthoriz==1) && (diffrx > 0)) {
            useloop=2;
            CHECK_DIAGONAL(diffry, diffrx, 5, 4)
        }
        else if ((wanthoriz==1) && (diffrx <= 0)) {
            useloop=1;
            CHECK_DIAGONAL(diffry, diffrx,7,6)
        }
        else if (diffry>0) {
            useloop=0;
            CHECK_DIAGONAL(diffrx ,diffry ,6,4)
        }
        else if (diffry<0) {
            useloop=3;
            CHECK_DIAGONAL(diffrx, diffry,7,5)
        }
    }

    if ((game.options[OPT_TURNTOFACELOC] != 0) &&
        (useloop != char1->loop) &&
        (char1->loop <= highestLoopForTurning) &&
        (in_enters_screen == 0)) {
            // Turn to face new direction
            Character_StopMoving(char1);
            if (char1->on == 1) {
                // only do the turning if the character is not hidden
                // (otherwise do_main_cycle will never return)
                start_character_turning (char1, useloop, no_diagonal);

                if ((blockingStyle == BLOCKING) || (blockingStyle == 1))
                    do_main_cycle(UNTIL_MOVEEND,(int)&char1->walking);
            }
            else
                char1->loop = useloop;
    }
    else
        char1->loop=useloop;

    char1->frame=0;
}

void Character_FaceObject(CharacterInfo *char1, ScriptObject *obj, int blockingStyle) {
    if (obj == NULL) 
        quit("!FaceObject: invalid object specified");


    Character_FaceLocation(char1, obj->obj->x, obj->obj->y, blockingStyle);
}

void Character_FollowCharacter(CharacterInfo *chaa, CharacterInfo *tofollow, int distaway, int eagerness) {

    if ((eagerness < 0) || (eagerness > 250))
        quit("!FollowCharacterEx: invalid eagerness: must be 0-250");

    if ((chaa->index_id == game.playercharacter) && (tofollow != NULL) && 
        (tofollow->room != chaa->room))
        quit("!FollowCharacterEx: you cannot tell the player character to follow a character in another room");

    if (tofollow != NULL) {
        DEBUG_CONSOLE("%s: Start following %s (dist %d, eager %d)", chaa->scrname, tofollow->scrname, distaway, eagerness);
    }
    else {
        DEBUG_CONSOLE("%s: Stop following other character", chaa->scrname);
    }

    if ((chaa->following >= 0) &&
        (chaa->followinfo == FOLLOW_ALWAYSONTOP)) {
            // if this character was following always-on-top, its baseline will
            // have been changed, so release it.
            chaa->baseline = -1;
    }

    if (tofollow == NULL)
        chaa->following = -1;
    else
        chaa->following = tofollow->index_id;

    chaa->followinfo=(distaway << 8) | eagerness;

    chaa->flags &= ~CHF_BEHINDSHEPHERD;

    // special case for Always On Other Character
    if (distaway == FOLLOW_ALWAYSONTOP) {
        chaa->followinfo = FOLLOW_ALWAYSONTOP;
        if (eagerness == 1)
            chaa->flags |= CHF_BEHINDSHEPHERD;
    }

    if (chaa->animating & CHANIM_REPEAT)
        debug_log("Warning: FollowCharacter called but the sheep is currently animating looped. It may never start to follow.");

}

int Character_IsCollidingWithChar(CharacterInfo *char1, CharacterInfo *char2) {
    if (char2 == NULL)
        quit("!AreCharactersColliding: invalid char2");

    if (char1->room != char2->room) return 0; // not colliding

    if ((char1->y > char2->y - 5) && (char1->y < char2->y + 5)) ;
    else return 0;

    int w1 = divide_down_coordinate(GetCharacterWidth(char1->index_id));
    int w2 = divide_down_coordinate(GetCharacterWidth(char2->index_id));

    int xps1=char1->x - w1/2;
    int xps2=char2->x - w2/2;

    if ((xps1 >= xps2 - w1) & (xps1 <= xps2 + w2)) return 1;
    return 0;
}

int Character_IsCollidingWithObject(CharacterInfo *chin, ScriptObject *objid) {
    if (objid == NULL)
        quit("!AreCharObjColliding: invalid object number");

    if (chin->room != displayed_room)
        return 0;
    if (objs[objid->id].on != 1)
        return 0;

    block checkblk = GetObjectImage(objid->id, NULL);
    int objWidth = checkblk->w;
    int objHeight = checkblk->h;
    int o1x = objs[objid->id].x;
    int o1y = objs[objid->id].y - divide_down_coordinate(objHeight);

    block charpic = GetCharacterImage(chin->index_id, NULL);

    int charWidth = charpic->w;
    int charHeight = charpic->h;
    int o2x = chin->x - divide_down_coordinate(charWidth) / 2;
    int o2y = chin->get_effective_y() - 5;  // only check feet

    if ((o2x >= o1x - divide_down_coordinate(charWidth)) &&
        (o2x <= o1x + divide_down_coordinate(objWidth)) &&
        (o2y >= o1y - 8) &&
        (o2y <= o1y + divide_down_coordinate(objHeight))) {
            // the character's feet are on the object
            if (game.options[OPT_PIXPERFECT] == 0)
                return 1;
            // check if they're on a transparent bit of the object
            int stxp = multiply_up_coordinate(o2x - o1x);
            int styp = multiply_up_coordinate(o2y - o1y);
            int maskcol = bitmap_mask_color (checkblk);
            int maskcolc = bitmap_mask_color (charpic);
            int thispix, thispixc;
            // check each pixel of the object along the char's feet
            for (int i = 0; i < charWidth; i += get_fixed_pixel_size(1)) {
                for (int j = 0; j < get_fixed_pixel_size(6); j += get_fixed_pixel_size(1)) {
                    thispix = my_getpixel(checkblk, i + stxp, j + styp);
                    thispixc = my_getpixel(charpic, i, j + (charHeight - get_fixed_pixel_size(5)));

                    if ((thispix != -1) && (thispix != maskcol) &&
                        (thispixc != -1) && (thispixc != maskcolc))
                        return 1;
                }
            }

    }
    return 0;
}

void Character_LockView(CharacterInfo *chap, int vii) {

    if ((vii < 1) || (vii > game.numviews)) {
        char buffer[150];
        sprintf (buffer, "!SetCharacterView: invalid view number (You said %d, max is %d)", vii, game.numviews);
        quit(buffer);
    }
    vii--;

    DEBUG_CONSOLE("%s: View locked to %d", chap->scrname, vii+1);
    if (chap->idleleft < 0) {
        Character_UnlockView(chap);
        chap->idleleft = chap->idletime;
    }
    Character_StopMoving(chap);
    chap->view=vii;
    chap->animating=0;
    FindReasonableLoopForCharacter(chap);
    chap->frame=0;
    chap->wait=0;
    chap->flags|=CHF_FIXVIEW;
    chap->pic_xoffs = 0;
    chap->pic_yoffs = 0;
}


void Character_LockViewAligned(CharacterInfo *chap, int vii, int loop, int align) {
    if (chap->view < 0)
        quit("!SetCharacterLoop: character has invalid old view number");

    int sppic = views[chap->view].loops[chap->loop].frames[chap->frame].pic;
    int leftSide = multiply_up_coordinate(chap->x) - spritewidth[sppic] / 2;

    Character_LockView(chap, vii);

    if ((loop < 0) || (loop >= views[chap->view].numLoops))
        quit("!SetCharacterViewEx: invalid loop specified");

    chap->loop = loop;
    chap->frame = 0;
    int newpic = views[chap->view].loops[chap->loop].frames[chap->frame].pic;
    int newLeft = multiply_up_coordinate(chap->x) - spritewidth[newpic] / 2;
    int xdiff = 0;

    if (align == SCALIGN_LEFT)
        xdiff = leftSide - newLeft;
    else if (align == SCALIGN_CENTRE)
        xdiff = 0;
    else if (align == SCALIGN_RIGHT)
        xdiff = (leftSide + spritewidth[sppic]) - (newLeft + spritewidth[newpic]);
    else
        quit("!SetCharacterViewEx: invalid alignment type specified");

    chap->pic_xoffs = xdiff;
    chap->pic_yoffs = 0;
}

void Character_LockViewFrame(CharacterInfo *chaa, int view, int loop, int frame) {

    Character_LockView(chaa, view);

    view--;
    if ((loop < 0) || (loop >= views[view].numLoops))
        quit("!SetCharacterFrame: invalid loop specified");
    if ((frame < 0) || (frame >= views[view].loops[loop].numFrames))
        quit("!SetCharacterFrame: invalid frame specified");

    chaa->loop = loop;
    chaa->frame = frame;
}

void Character_LockViewOffset(CharacterInfo *chap, int vii, int xoffs, int yoffs) {
    Character_LockView(chap, vii);

    if ((current_screen_resolution_multiplier == 1) && (game.default_resolution >= 3)) {
        // running a 640x400 game at 320x200, adjust
        xoffs /= 2;
        yoffs /= 2;
    }
    else if ((current_screen_resolution_multiplier > 1) && (game.default_resolution <= 2)) {
        // running a 320x200 game at 640x400, adjust
        xoffs *= 2;
        yoffs *= 2;
    }

    chap->pic_xoffs = xoffs;
    chap->pic_yoffs = yoffs;
}

void Character_LoseInventory(CharacterInfo *chap, ScriptInvItem *invi) {

    if (invi == NULL)
        quit("!LoseInventoryFromCharacter: invalid invnetory number");

    int inum = invi->id;

    if (chap->inv[inum] > 0)
        chap->inv[inum]--;

    if ((chap->activeinv == inum) & (chap->inv[inum] < 1)) {
        chap->activeinv = -1;
        if ((chap == playerchar) && (GetCursorMode() == MODE_USE))
            set_cursor_mode(0);
    }

    int charid = chap->index_id;

    if ((chap->inv[inum] == 0) || (game.options[OPT_DUPLICATEINV] > 0)) {
        int xx,tt;
        for (xx = 0; xx < charextra[charid].invorder_count; xx++) {
            if (charextra[charid].invorder[xx] == inum) {
                charextra[charid].invorder_count--;
                for (tt = xx; tt < charextra[charid].invorder_count; tt++)
                    charextra[charid].invorder[tt] = charextra[charid].invorder[tt+1];
                break;
            }
        }
    }
    guis_need_update = 1;

    if (chap == playerchar)
        run_on_event (GE_LOSE_INV, inum);
}

void Character_PlaceOnWalkableArea(CharacterInfo *chap) 
{
    if (displayed_room < 0)
        quit("!Character.PlaceOnWalkableArea: no room is currently loaded");

    find_nearest_walkable_area(&chap->x, &chap->y);
}

void Character_RemoveTint(CharacterInfo *chaa) {

    if (chaa->flags & CHF_HASTINT) {
        DEBUG_CONSOLE("Un-tint %s", chaa->scrname);
        chaa->flags &= ~CHF_HASTINT;
    }
    else {
        debug_log("Character.RemoveTint called but character was not tinted");
    }
}

int Character_GetHasExplicitTint(CharacterInfo *chaa) {

    if (chaa->flags & CHF_HASTINT)
        return 1;

    return 0;
}

void Character_Say(CharacterInfo *chaa, const char *texx, ...) {

    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf,get_translation(texx),ap);
    va_end(ap);

    _DisplaySpeechCore(chaa->index_id, displbuf);

}

void Character_SayAt(CharacterInfo *chaa, int x, int y, int width, const char *texx) {

    DisplaySpeechAt(x, y, width, chaa->index_id, (char*)texx);
}

ScriptOverlay* Character_SayBackground(CharacterInfo *chaa, const char *texx) {

    int ovltype = DisplaySpeechBackground(chaa->index_id, (char*)texx);
    int ovri = find_overlay_of_type(ovltype);
    if (ovri<0)
        quit("!SayBackground internal error: no overlay");

    // Convert the overlay ID to an Overlay object
    ScriptOverlay *scOver = new ScriptOverlay();
    scOver->overlayId = ovltype;
    scOver->borderHeight = 0;
    scOver->borderWidth = 0;
    scOver->isBackgroundSpeech = 1;
    int handl = ccRegisterManagedObject(scOver, scOver);
    screenover[ovri].associatedOverlayHandle = handl;

    return scOver;
}

void Character_SetAsPlayer(CharacterInfo *chaa) {

    // set to same character, so ignore
    if (game.playercharacter == chaa->index_id)
        return;

    setup_player_character(chaa->index_id);

    //update_invorder();

    DEBUG_CONSOLE("%s is new player character", playerchar->scrname);

    // Within game_start, return now
    if (displayed_room < 0)
        return;

    // Ignore invalid room numbers for the character and just place him in
    // the current room for 2.x. Following script calls to NewRoom() will
    // make sure this still works as intended.
    if ((loaded_game_file_version <= 32) && (playerchar->room < 0))
        playerchar->room = displayed_room;

    if (displayed_room != playerchar->room)
        NewRoom(playerchar->room);
    else   // make sure it doesn't run the region interactions
        play.player_on_region = GetRegionAt (playerchar->x, playerchar->y);

    if ((playerchar->activeinv >= 0) && (playerchar->inv[playerchar->activeinv] < 1))
        playerchar->activeinv = -1;

    // They had inv selected, so change the cursor
    if (cur_mode == MODE_USE) {
        if (playerchar->activeinv < 0)
            SetNextCursor ();
        else
            SetActiveInventory (playerchar->activeinv);
    }

}


void Character_SetIdleView(CharacterInfo *chaa, int iview, int itime) {

    if (iview == 1) 
        quit("!SetCharacterIdle: view 1 cannot be used as an idle view, sorry.");

    // if an idle anim is currently playing, release it
    if (chaa->idleleft < 0)
        Character_UnlockView(chaa);

    chaa->idleview = iview - 1;
    // make sure they don't appear idle while idle anim is disabled
    if (iview < 1)
        itime = 10;
    chaa->idletime = itime;
    chaa->idleleft = itime;

    // if not currently animating, reset the wait counter
    if ((chaa->animating == 0) && (chaa->walking == 0))
        chaa->wait = 0;

    if (iview >= 1) {
        DEBUG_CONSOLE("Set %s idle view to %d (time %d)", chaa->scrname, iview, itime);
    }
    else {
        DEBUG_CONSOLE("%s idle view disabled", chaa->scrname);
    }
    if (chaa->flags & CHF_FIXVIEW) {
        debug_log("SetCharacterIdle called while character view locked with SetCharacterView; idle ignored");
        DEBUG_CONSOLE("View locked, idle will not kick in until Released");
    }
    // if they switch to a swimming animation, kick it off immediately
    if (itime == 0)
        charextra[chaa->index_id].process_idle_this_time = 1;

}

void Character_SetOption(CharacterInfo *chaa, int flag, int yesorno) {

    if ((yesorno < 0) || (yesorno > 1))
        quit("!SetCharacterProperty: last parameter must be 0 or 1");

    if (flag & CHF_MANUALSCALING) {
        // backwards compatibility fix
        Character_SetIgnoreScaling(chaa, yesorno);
    }
    else {
        chaa->flags &= ~flag;
        if (yesorno)
            chaa->flags |= flag;
    }

}

void Character_SetSpeed(CharacterInfo *chaa, int xspeed, int yspeed) {

    if ((xspeed == 0) || (xspeed > 50) || (yspeed == 0) || (yspeed > 50))
        quit("!SetCharacterSpeedEx: invalid speed value");
    if (chaa->walking)
        quit("!SetCharacterSpeedEx: cannot change speed while walking");

    chaa->walkspeed = xspeed;

    if (yspeed == xspeed) 
        chaa->walkspeed_y = UNIFORM_WALK_SPEED;
    else
        chaa->walkspeed_y = yspeed;
}


void Character_StopMoving(CharacterInfo *charp) {

    int chaa = charp->index_id;
    if (chaa == play.skip_until_char_stops)
        EndSkippingUntilCharStops();

    if (charextra[chaa].xwas != INVALID_X) {
        charp->x = charextra[chaa].xwas;
        charp->y = charextra[chaa].ywas;
        charextra[chaa].xwas = INVALID_X;
    }
    if ((charp->walking > 0) && (charp->walking < TURNING_AROUND)) {
        // if it's not a MoveCharDirect, make sure they end up on a walkable area
        if ((mls[charp->walking].direct == 0) && (charp->room == displayed_room))
            Character_PlaceOnWalkableArea(charp);

        DEBUG_CONSOLE("%s: stop moving", charp->scrname);

        charp->idleleft = charp->idletime;
        // restart the idle animation straight away
        charextra[chaa].process_idle_this_time = 1;
    }
    if (charp->walking) {
        // If the character is currently moving, stop them and reset their frame
        charp->walking = 0;
        if ((charp->flags & CHF_MOVENOTWALK) == 0)
            charp->frame = 0;
    }
}

void Character_Tint(CharacterInfo *chaa, int red, int green, int blue, int opacity, int luminance) {
    if ((red < 0) || (green < 0) || (blue < 0) ||
        (red > 255) || (green > 255) || (blue > 255) ||
        (opacity < 0) || (opacity > 100) ||
        (luminance < 0) || (luminance > 100))
        quit("!Character.Tint: invalid parameter. R,G,B must be 0-255, opacity & luminance 0-100");

    DEBUG_CONSOLE("Set %s tint RGB(%d,%d,%d) %d%%", chaa->scrname, red, green, blue, opacity);

    charextra[chaa->index_id].tint_r = red;
    charextra[chaa->index_id].tint_g = green;
    charextra[chaa->index_id].tint_b = blue;
    charextra[chaa->index_id].tint_level = opacity;
    charextra[chaa->index_id].tint_light = (luminance * 25) / 10;
    chaa->flags |= CHF_HASTINT;
}

void Character_Think(CharacterInfo *chaa, const char *texx, ...) {

    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf,get_translation(texx),ap);
    va_end(ap);

    _DisplayThoughtCore(chaa->index_id, displbuf);
}

void Character_UnlockView(CharacterInfo *chaa) {
    if (chaa->flags & CHF_FIXVIEW) {
        DEBUG_CONSOLE("%s: Released view back to default", chaa->scrname);
    }
    chaa->flags &= ~CHF_FIXVIEW;
    chaa->view = chaa->defview;
    chaa->frame = 0;
    Character_StopMoving(chaa);
    if (chaa->view >= 0) {
        int maxloop = views[chaa->view].numLoops;
        if (((chaa->flags & CHF_NODIAGONAL)!=0) && (maxloop > 4))
            maxloop = 4;
        FindReasonableLoopForCharacter(chaa);
    }
    chaa->animating = 0;
    chaa->idleleft = chaa->idletime;
    chaa->pic_xoffs = 0;
    chaa->pic_yoffs = 0;
    // restart the idle animation straight away
    charextra[chaa->index_id].process_idle_this_time = 1;

}

void walk_or_move_character(CharacterInfo *chaa, int x, int y, int blocking, int direct, bool isWalk)
{
    if (chaa->on != 1)
        quit("!MoveCharacterBlocking: character is turned off and cannot be moved");

    if ((direct == ANYWHERE) || (direct == 1))
        walk_character(chaa->index_id, x, y, 1, isWalk);
    else if ((direct == WALKABLE_AREAS) || (direct == 0))
        walk_character(chaa->index_id, x, y, 0, isWalk);
    else
        quit("!Character.Walk: Direct must be ANYWHERE or WALKABLE_AREAS");

    if ((blocking == BLOCKING) || (blocking == 1))
        do_main_cycle(UNTIL_MOVEEND,(int)&chaa->walking);
    else if ((blocking != IN_BACKGROUND) && (blocking != 0))
        quit("!Character.Walk: Blocking must be BLOCKING or IN_BACKGRUOND");

}

void Character_Walk(CharacterInfo *chaa, int x, int y, int blocking, int direct) 
{
    walk_or_move_character(chaa, x, y, blocking, direct, true);
}

void Character_Move(CharacterInfo *chaa, int x, int y, int blocking, int direct) 
{
    walk_or_move_character(chaa, x, y, blocking, direct, false);
}

void Character_WalkStraight(CharacterInfo *chaa, int xx, int yy, int blocking) {

    if (chaa->room != displayed_room)
        quit("!MoveCharacterStraight: specified character not in current room");

    Character_StopMoving(chaa);
    int movetox = xx, movetoy = yy;

    wallscreen = prepare_walkable_areas(chaa->index_id);

    int fromXLowres = convert_to_low_res(chaa->x);
    int fromYLowres = convert_to_low_res(chaa->y);
    int toXLowres = convert_to_low_res(xx);
    int toYLowres = convert_to_low_res(yy);

    if (!can_see_from(fromXLowres, fromYLowres, toXLowres, toYLowres)) {
        movetox = convert_back_to_high_res(lastcx);
        movetoy = convert_back_to_high_res(lastcy);
    }

    walk_character(chaa->index_id, movetox, movetoy, 1, true);

    if ((blocking == BLOCKING) || (blocking == 1))
        do_main_cycle(UNTIL_MOVEEND,(int)&chaa->walking);
    else if ((blocking != IN_BACKGROUND) && (blocking != 0))
        quit("!Character.Walk: Blocking must be BLOCKING or IN_BACKGRUOND");

}


// **** CHARACTER: PROPERTIES ****


ScriptInvItem* Character_GetActiveInventory(CharacterInfo *chaa) {

    if (chaa->activeinv <= 0)
        return NULL;

    return &scrInv[chaa->activeinv];
}

void Character_SetActiveInventory(CharacterInfo *chaa, ScriptInvItem* iit) {
    guis_need_update = 1;

    if (iit == NULL) {
        chaa->activeinv = -1;

        if (chaa->index_id == game.playercharacter) {

            if (GetCursorMode()==MODE_USE)
                set_cursor_mode(0);
        }
        return;
    }

    if (chaa->inv[iit->id] < 1)
        quit("!SetActiveInventory: character doesn't have any of that inventory");

    chaa->activeinv = iit->id;

    if (chaa->index_id == game.playercharacter) {
        // if it's the player character, update mouse cursor
        update_inv_cursor(iit->id);
        set_cursor_mode(MODE_USE);
    }
}

int Character_GetAnimating(CharacterInfo *chaa) {
    if (chaa->animating)
        return 1;
    return 0;
}

int Character_GetAnimationSpeed(CharacterInfo *chaa) {
    return chaa->animspeed;
}

void Character_SetAnimationSpeed(CharacterInfo *chaa, int newval) {

    chaa->animspeed = newval;
}

int Character_GetBaseline(CharacterInfo *chaa) {

    if (chaa->baseline < 1)
        return 0;

    return chaa->baseline;
}

void Character_SetBaseline(CharacterInfo *chaa, int basel) {

    chaa->baseline = basel;
}

int Character_GetBlinkInterval(CharacterInfo *chaa) {

    return chaa->blinkinterval;
}

void Character_SetBlinkInterval(CharacterInfo *chaa, int interval) {

    if (interval < 0)
        quit("!SetCharacterBlinkView: invalid blink interval");

    chaa->blinkinterval = interval;

    if (chaa->blinktimer > 0)
        chaa->blinktimer = chaa->blinkinterval;
}

int Character_GetBlinkView(CharacterInfo *chaa) {

    return chaa->blinkview + 1;
}

void Character_SetBlinkView(CharacterInfo *chaa, int vii) {

    if (((vii < 2) || (vii > game.numviews)) && (vii != -1))
        quit("!SetCharacterBlinkView: invalid view number");

    chaa->blinkview = vii - 1;
}

int Character_GetBlinkWhileThinking(CharacterInfo *chaa) {
    if (chaa->flags & CHF_NOBLINKANDTHINK)
        return 0;
    return 1;
}

void Character_SetBlinkWhileThinking(CharacterInfo *chaa, int yesOrNo) {
    chaa->flags &= ~CHF_NOBLINKANDTHINK;
    if (yesOrNo == 0)
        chaa->flags |= CHF_NOBLINKANDTHINK;
}

int Character_GetBlockingHeight(CharacterInfo *chaa) {

    return chaa->blocking_height;
}

void Character_SetBlockingHeight(CharacterInfo *chaa, int hit) {

    chaa->blocking_height = hit;
}

int Character_GetBlockingWidth(CharacterInfo *chaa) {

    return chaa->blocking_width;
}

void Character_SetBlockingWidth(CharacterInfo *chaa, int wid) {

    chaa->blocking_width = wid;
}

int Character_GetDiagonalWalking(CharacterInfo *chaa) {

    if (chaa->flags & CHF_NODIAGONAL)
        return 0;
    return 1;  
}

void Character_SetDiagonalWalking(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_NODIAGONAL;
    if (!yesorno)
        chaa->flags |= CHF_NODIAGONAL;
}

int Character_GetClickable(CharacterInfo *chaa) {

    if (chaa->flags & CHF_NOINTERACT)
        return 0;
    return 1;
}

void Character_SetClickable(CharacterInfo *chaa, int clik) {

    chaa->flags &= ~CHF_NOINTERACT;
    // if they don't want it clickable, set the relevant bit
    if (clik == 0)
        chaa->flags |= CHF_NOINTERACT;
}

int Character_GetID(CharacterInfo *chaa) {

    return chaa->index_id;

}

int Character_GetFrame(CharacterInfo *chaa) {
    return chaa->frame;
}

void Character_SetFrame(CharacterInfo *chaa, int newval) {
    chaa->frame = newval;
}

int Character_GetIdleView(CharacterInfo *chaa) {

    if (chaa->idleview < 1)
        return -1;

    return chaa->idleview + 1;
}

int Character_GetIInventoryQuantity(CharacterInfo *chaa, int index) {
    if ((index < 1) || (index >= game.numinvitems))
        quitprintf("!Character.InventoryQuantity: invalid inventory index %d", index);

    return chaa->inv[index];
}

int Character_HasInventory(CharacterInfo *chaa, ScriptInvItem *invi)
{
    if (invi == NULL)
        quit("!Character.HasInventory: NULL inventory item supplied");

    return (chaa->inv[invi->id] > 0) ? 1 : 0;
}

void Character_SetIInventoryQuantity(CharacterInfo *chaa, int index, int quant) {
    if ((index < 1) || (index >= game.numinvitems))
        quitprintf("!Character.InventoryQuantity: invalid inventory index %d", index);

    if ((quant < 0) || (quant > 32000))
        quitprintf("!Character.InventoryQuantity: invalid quantity %d", quant);

    chaa->inv[index] = quant;
}

int Character_GetIgnoreLighting(CharacterInfo *chaa) {

    if (chaa->flags & CHF_NOLIGHTING)
        return 1;
    return 0;
}

void Character_SetIgnoreLighting(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_NOLIGHTING;
    if (yesorno)
        chaa->flags |= CHF_NOLIGHTING;
}

int Character_GetIgnoreScaling(CharacterInfo *chaa) {

    if (chaa->flags & CHF_MANUALSCALING)
        return 1;
    return 0;  
}

void Character_SetIgnoreScaling(CharacterInfo *chaa, int yesorno) {

    if (yesorno) {
        // when setting IgnoreScaling to 1, should reset zoom level
        // like it used to in pre-2.71
        charextra[chaa->index_id].zoom = 100;
    }
    Character_SetManualScaling(chaa, yesorno);
}

void Character_SetManualScaling(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_MANUALSCALING;
    if (yesorno)
        chaa->flags |= CHF_MANUALSCALING;
}

int Character_GetIgnoreWalkbehinds(CharacterInfo *chaa) {

    if (chaa->flags & CHF_NOWALKBEHINDS)
        return 1;
    return 0;
}

void Character_SetIgnoreWalkbehinds(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_NOWALKBEHINDS;
    if (yesorno)
        chaa->flags |= CHF_NOWALKBEHINDS;
}

int Character_GetMovementLinkedToAnimation(CharacterInfo *chaa) {

    if (chaa->flags & CHF_ANTIGLIDE)
        return 1;
    return 0;
}

void Character_SetMovementLinkedToAnimation(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_ANTIGLIDE;
    if (yesorno)
        chaa->flags |= CHF_ANTIGLIDE;
}

int Character_GetLoop(CharacterInfo *chaa) {
    return chaa->loop;
}

void Character_SetLoop(CharacterInfo *chaa, int newval) {
    if ((newval < 0) || (newval >= views[chaa->view].numLoops))
        quit("!Character.Loop: invalid loop number for this view");

    chaa->loop = newval;

    if (chaa->frame >= views[chaa->view].loops[chaa->loop].numFrames)
        chaa->frame = 0;
}

int Character_GetMoving(CharacterInfo *chaa) {
    if (chaa->walking)
        return 1;
    return 0;
}

const char* Character_GetName(CharacterInfo *chaa) {
    return CreateNewScriptString(chaa->name);
}

void Character_SetName(CharacterInfo *chaa, const char *newName) {
    strncpy(chaa->name, newName, 40);
    chaa->name[39] = 0;
}

int Character_GetNormalView(CharacterInfo *chaa) {
    return chaa->defview + 1;
}

int Character_GetPreviousRoom(CharacterInfo *chaa) {
    return chaa->prevroom;
}

int Character_GetRoom(CharacterInfo *chaa) {
    return chaa->room;
}


int Character_GetScaleMoveSpeed(CharacterInfo *chaa) {

    if (chaa->flags & CHF_SCALEMOVESPEED)
        return 1;
    return 0;  
}

void Character_SetScaleMoveSpeed(CharacterInfo *chaa, int yesorno) {

    if ((yesorno < 0) || (yesorno > 1))
        quit("Character.ScaleMoveSpeed: value must be true or false (1 or 0)");

    chaa->flags &= ~CHF_SCALEMOVESPEED;
    if (yesorno)
        chaa->flags |= CHF_SCALEMOVESPEED;
}

int Character_GetScaleVolume(CharacterInfo *chaa) {

    if (chaa->flags & CHF_SCALEVOLUME)
        return 1;
    return 0;  
}

void Character_SetScaleVolume(CharacterInfo *chaa, int yesorno) {

    if ((yesorno < 0) || (yesorno > 1))
        quit("Character.ScaleVolume: value must be true or false (1 or 0)");

    chaa->flags &= ~CHF_SCALEVOLUME;
    if (yesorno)
        chaa->flags |= CHF_SCALEVOLUME;
}

int Character_GetScaling(CharacterInfo *chaa) {
    return charextra[chaa->index_id].zoom;
}

void Character_SetScaling(CharacterInfo *chaa, int zoomlevel) {

    if ((chaa->flags & CHF_MANUALSCALING) == 0)
        quit("!Character.Scaling: cannot set property unless ManualScaling is enabled");
    if ((zoomlevel < 5) || (zoomlevel > 200))
        quit("!Character.Scaling: scaling level must be between 5 and 200%");

    charextra[chaa->index_id].zoom = zoomlevel;
}

int Character_GetSolid(CharacterInfo *chaa) {

    if (chaa->flags & CHF_NOBLOCKING)
        return 0;
    return 1;
}

void Character_SetSolid(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_NOBLOCKING;
    if (!yesorno)
        chaa->flags |= CHF_NOBLOCKING;
}

int Character_GetSpeaking(CharacterInfo *chaa) {
    if (get_character_currently_talking() == chaa->index_id)
        return 1;

    return 0;
}

int Character_GetSpeechColor(CharacterInfo *chaa) {

    return chaa->talkcolor;
}

void Character_SetSpeechColor(CharacterInfo *chaa, int ncol) {

    chaa->talkcolor = ncol;
}

int GetCharacterSpeechAnimationDelay(CharacterInfo *cha)
{
    if (game.options[OPT_OLDTALKANIMSPD])
    {
        // The talkanim property only applies to Lucasarts style speech.
        // Sierra style speech has a fixed delay of 5.
        if (game.options[OPT_SPEECHTYPE] == 0)
            return play.talkanim_speed;
        else
            return 5;
    }
    else
        return cha->speech_anim_speed;
}

void Character_SetSpeechAnimationDelay(CharacterInfo *chaa, int newDelay) 
{
    if (game.options[OPT_OLDTALKANIMSPD])
        quit("!Character.SpeechAnimationDelay cannot be set when legacy speech animation speed is enabled");

    chaa->speech_anim_speed = newDelay;
}

int Character_GetSpeechView(CharacterInfo *chaa) {

    return chaa->talkview + 1;
}

void Character_SetSpeechView(CharacterInfo *chaa, int vii) {
    if (vii == -1) {
        chaa->talkview = -1;
        return;
    }

    if ((vii < 1) || (vii > game.numviews))
        quit("!SetCharacterSpeechView: invalid view number");

    chaa->talkview = vii - 1;
}

int Character_GetThinkView(CharacterInfo *chaa) {

    return chaa->thinkview + 1;
}

void Character_SetThinkView(CharacterInfo *chaa, int vii) {
    if (((vii < 2) || (vii > game.numviews)) && (vii != -1))
        quit("!SetCharacterThinkView: invalid view number");

    chaa->thinkview = vii - 1;
}

int Character_GetTransparency(CharacterInfo *chaa) {

    if (chaa->transparency == 0)
        return 0;
    if (chaa->transparency == 255)
        return 100;

    return 100 - ((chaa->transparency * 10) / 25);
}

void Character_SetTransparency(CharacterInfo *chaa, int trans) {

    if ((trans < 0) || (trans > 100))
        quit("!SetCharTransparent: transparency value must be between 0 and 100");

    if (trans == 0)
        chaa->transparency=0;
    else if (trans == 100)
        chaa->transparency = 255;
    else
        chaa->transparency = ((100-trans) * 25) / 10;
}

int Character_GetTurnBeforeWalking(CharacterInfo *chaa) {

    if (chaa->flags & CHF_NOTURNING)
        return 0;
    return 1;  
}

void Character_SetTurnBeforeWalking(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_NOTURNING;
    if (!yesorno)
        chaa->flags |= CHF_NOTURNING;
}

int Character_GetView(CharacterInfo *chaa) {
    return chaa->view + 1;
}

int Character_GetWalkSpeedX(CharacterInfo *chaa) {
    return chaa->walkspeed;
}

int Character_GetWalkSpeedY(CharacterInfo *chaa) {
    if (chaa->walkspeed_y != UNIFORM_WALK_SPEED)
        return chaa->walkspeed_y;

    return chaa->walkspeed;
}

int Character_GetX(CharacterInfo *chaa) {
    return chaa->x;
}

void Character_SetX(CharacterInfo *chaa, int newval) {
    chaa->x = newval;
}

int Character_GetY(CharacterInfo *chaa) {
    return chaa->y;
}

void Character_SetY(CharacterInfo *chaa, int newval) {
    chaa->y = newval;
}

int Character_GetZ(CharacterInfo *chaa) {
    return chaa->z;
}

void Character_SetZ(CharacterInfo *chaa, int newval) {
    chaa->z = newval;
}





#include "acmain/ac_maindefines.h"



int is_valid_character(int newchar) {
    if ((newchar < 0) || (newchar >= game.numcharacters)) return 0;
    return 1;
}




void SetCharacterIdle(int who, int iview, int itime) {
    if (!is_valid_character(who))
        quit("!SetCharacterIdle: Invalid character specified");

    Character_SetIdleView(&game.chars[who], iview, itime);
}



int GetCharacterWidth(int ww) {
    CharacterInfo *char1 = &game.chars[ww];

    if (charextra[ww].width < 1)
    {
        if ((char1->view < 0) ||
            (char1->loop >= views[char1->view].numLoops) ||
            (char1->frame >= views[char1->view].loops[char1->loop].numFrames))
        {
            debug_log("GetCharacterWidth: Character %s has invalid frame: view %d, loop %d, frame %d", char1->scrname, char1->view + 1, char1->loop, char1->frame);
            return multiply_up_coordinate(4);
        }

        return spritewidth[views[char1->view].loops[char1->loop].frames[char1->frame].pic];
    }
    else 
        return charextra[ww].width;
}

int GetCharacterHeight(int charid) {
    CharacterInfo *char1 = &game.chars[charid];

    if (charextra[charid].height < 1)
    {
        if ((char1->view < 0) ||
            (char1->loop >= views[char1->view].numLoops) ||
            (char1->frame >= views[char1->view].loops[char1->loop].numFrames))
        {
            debug_log("GetCharacterHeight: Character %s has invalid frame: view %d, loop %d, frame %d", char1->scrname, char1->view + 1, char1->loop, char1->frame);
            return multiply_up_coordinate(2);
        }

        return spriteheight[views[char1->view].loops[char1->loop].frames[char1->frame].pic];
    }
    else
        return charextra[charid].height;
}



int wantMoveNow (int chnum, CharacterInfo *chi) {
    // check most likely case first
    if ((charextra[chnum].zoom == 100) || ((chi->flags & CHF_SCALEMOVESPEED) == 0))
        return 1;

    // the % checks don't work when the counter is negative, so once
    // it wraps round, correct it
    while (chi->walkwaitcounter < 0) {
        chi->walkwaitcounter += 12000;
    }

    // scaling 170-200%, move 175% speed
    if (charextra[chnum].zoom >= 170) {
        if ((chi->walkwaitcounter % 4) >= 1)
            return 2;
        else
            return 1;
    }
    // scaling 140-170%, move 150% speed
    else if (charextra[chnum].zoom >= 140) {
        if ((chi->walkwaitcounter % 2) == 1)
            return 2;
        else
            return 1;
    }
    // scaling 115-140%, move 125% speed
    else if (charextra[chnum].zoom >= 115) {
        if ((chi->walkwaitcounter % 4) >= 3)
            return 2;
        else
            return 1;
    }
    // scaling 80-120%, normal speed
    else if (charextra[chnum].zoom >= 80)
        return 1;
    // scaling 60-80%, move 75% speed
    if (charextra[chnum].zoom >= 60) {
        if ((chi->walkwaitcounter % 4) >= 1)
            return 1;
    }
    // scaling 30-60%, move 50% speed
    else if (charextra[chnum].zoom >= 30) {
        if ((chi->walkwaitcounter % 2) == 1)
            return -1;
        else if (charextra[chnum].xwas != INVALID_X) {
            // move the second half of the movement to make it smoother
            chi->x = charextra[chnum].xwas;
            chi->y = charextra[chnum].ywas;
            charextra[chnum].xwas = INVALID_X;
        }
    }
    // scaling 0-30%, move 25% speed
    else {
        if ((chi->walkwaitcounter % 4) >= 3)
            return -1;
        if (((chi->walkwaitcounter % 4) == 1) && (charextra[chnum].xwas != INVALID_X)) {
            // move the second half of the movement to make it smoother
            chi->x = charextra[chnum].xwas;
            chi->y = charextra[chnum].ywas;
            charextra[chnum].xwas = INVALID_X;
        }

    }

    return 0;
}


void setup_player_character(int charid) {
    game.playercharacter = charid;
    playerchar = &game.chars[charid];
    _sc_PlayerCharPtr = ccGetObjectHandleFromAddress((char*)playerchar);
}


void SetCharacterBaseline (int obn, int basel) {
    if (!is_valid_character(obn)) quit("!SetCharacterBaseline: invalid object number specified");

    Character_SetBaseline(&game.chars[obn], basel);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetCharacterTransparency(int obn,int trans) {
    if (!is_valid_character(obn))
        quit("!SetCharTransparent: invalid character number specified");

    Character_SetTransparency(&game.chars[obn], trans);
}

void scAnimateCharacter (int chh, int loopn, int sppd, int rept) {
    if (!is_valid_character(chh))
        quit("AnimateCharacter: invalid character");

    animate_character(&game.chars[chh], loopn, sppd, rept);
}

void AnimateCharacterEx(int chh, int loopn, int sppd, int rept, int direction, int blocking) {
    if ((direction < 0) || (direction > 1))
        quit("!AnimateCharacterEx: invalid direction");
    if (!is_valid_character(chh))
        quit("AnimateCharacter: invalid character");

    if (direction)
        direction = BACKWARDS;
    else
        direction = FORWARDS;

    if (blocking)
        blocking = BLOCKING;
    else
        blocking = IN_BACKGROUND;

    Character_Animate(&game.chars[chh], loopn, sppd, rept, blocking, direction);

}

void animate_character(CharacterInfo *chap, int loopn,int sppd,int rept, int noidleoverride, int direction) {

    if ((chap->view < 0) || (chap->view > game.numviews)) {
        quitprintf("!AnimateCharacter: you need to set the view number first\n"
            "(trying to animate '%s' using loop %d. View is currently %d).",chap->name,loopn,chap->view+1);
    }
    DEBUG_CONSOLE("%s: Start anim view %d loop %d, spd %d, repeat %d", chap->scrname, chap->view+1, loopn, sppd, rept);
    if ((chap->idleleft < 0) && (noidleoverride == 0)) {
        // if idle view in progress for the character (and this is not the
        // "start idle animation" animate_character call), stop the idle anim
        Character_UnlockView(chap);
        chap->idleleft=chap->idletime;
    }
    if ((loopn < 0) || (loopn >= views[chap->view].numLoops))
        quit("!AnimateCharacter: invalid loop number specified");
    Character_StopMoving(chap);
    chap->animating=1;
    if (rept) chap->animating |= CHANIM_REPEAT;
    if (direction) chap->animating |= CHANIM_BACKWARDS;

    chap->animating|=((sppd << 8) & 0xff00);
    chap->loop=loopn;

    if (direction) {
        chap->frame = views[chap->view].loops[loopn].numFrames - 1;
    }
    else
        chap->frame=0;

    chap->wait = sppd + views[chap->view].loops[loopn].frames[chap->frame].speed;
    CheckViewFrameForCharacter(chap);
}


void SetPlayerCharacter(int newchar) {
    if (!is_valid_character(newchar))
        quit("!SetPlayerCharacter: Invalid character specified");

    Character_SetAsPlayer(&game.chars[newchar]);
}

void FollowCharacterEx(int who, int tofollow, int distaway, int eagerness) {
    if (!is_valid_character(who))
        quit("!FollowCharacter: Invalid character specified");
    CharacterInfo *chtofollow;
    if (tofollow == -1)
        chtofollow = NULL;
    else if (!is_valid_character(tofollow))
        quit("!FollowCharacterEx: invalid character to follow");
    else
        chtofollow = &game.chars[tofollow];

    Character_FollowCharacter(&game.chars[who], chtofollow, distaway, eagerness);
}

void FollowCharacter(int who, int tofollow) {
    FollowCharacterEx(who,tofollow,10,97);
}

void SetCharacterIgnoreLight (int who, int yesorno) {
    if (!is_valid_character(who))
        quit("!SetCharacterIgnoreLight: Invalid character specified");

    Character_SetIgnoreLighting(&game.chars[who], yesorno);
}




void MoveCharacter(int cc,int xx,int yy) {
    walk_character(cc,xx,yy,0, true);
}
void MoveCharacterDirect(int cc,int xx, int yy) {
    walk_character(cc,xx,yy,1, true);
}
void MoveCharacterStraight(int cc,int xx, int yy) {
    if (!is_valid_character(cc))
        quit("!MoveCharacterStraight: invalid character specified");

    Character_WalkStraight(&game.chars[cc], xx, yy, IN_BACKGROUND);
}

// Append to character path
void MoveCharacterPath (int chac, int tox, int toy) {
    if (!is_valid_character(chac))
        quit("!MoveCharacterPath: invalid character specified");

    Character_AddWaypoint(&game.chars[chac], tox, toy);
}


int GetPlayerCharacter() {
    return game.playercharacter;
}

void SetCharacterSpeedEx(int chaa, int xspeed, int yspeed) {
    if (!is_valid_character(chaa))
        quit("!SetCharacterSpeedEx: invalid character");

    Character_SetSpeed(&game.chars[chaa], xspeed, yspeed);

}

void SetCharacterSpeed(int chaa,int nspeed) {
    SetCharacterSpeedEx(chaa, nspeed, nspeed);
}

void SetTalkingColor(int chaa,int ncol) {
    if (!is_valid_character(chaa)) quit("!SetTalkingColor: invalid character");

    Character_SetSpeechColor(&game.chars[chaa], ncol);
}

void SetCharacterSpeechView (int chaa, int vii) {
    if (!is_valid_character(chaa))
        quit("!SetCharacterSpeechView: invalid character specified");

    Character_SetSpeechView(&game.chars[chaa], vii);
}

void SetCharacterBlinkView (int chaa, int vii, int intrv) {
    if (!is_valid_character(chaa))
        quit("!SetCharacterBlinkView: invalid character specified");

    Character_SetBlinkView(&game.chars[chaa], vii);
    Character_SetBlinkInterval(&game.chars[chaa], intrv);
}

void SetCharacterView(int chaa,int vii) {
    if (!is_valid_character(chaa))
        quit("!SetCharacterView: invalid character specified");

    Character_LockView(&game.chars[chaa], vii);
}

void SetCharacterFrame(int chaa, int view, int loop, int frame) {

    Character_LockViewFrame(&game.chars[chaa], view, loop, frame);
}

// similar to SetCharView, but aligns the frame to make it line up
void SetCharacterViewEx (int chaa, int vii, int loop, int align) {

    Character_LockViewAligned(&game.chars[chaa], vii, loop, align);
}

void SetCharacterViewOffset (int chaa, int vii, int xoffs, int yoffs) {

    Character_LockViewOffset(&game.chars[chaa], vii, xoffs, yoffs);
}


void ChangeCharacterView(int chaa,int vii) {
    if (!is_valid_character(chaa))
        quit("!ChangeCharacterView: invalid character specified");

    Character_ChangeView(&game.chars[chaa], vii);
}

void SetCharacterClickable (int cha, int clik) {
    if (!is_valid_character(cha))
        quit("!SetCharacterClickable: Invalid character specified");
    // make the character clicklabe (reset "No interaction" bit)
    game.chars[cha].flags&=~CHF_NOINTERACT;
    // if they don't want it clickable, set the relevant bit
    if (clik == 0)
        game.chars[cha].flags|=CHF_NOINTERACT;
}

void SetCharacterIgnoreWalkbehinds (int cha, int clik) {
    if (!is_valid_character(cha))
        quit("!SetCharacterIgnoreWalkbehinds: Invalid character specified");

    Character_SetIgnoreWalkbehinds(&game.chars[cha], clik);
}


void MoveCharacterToObject(int chaa,int obbj) {
    // invalid object, do nothing
    // this allows MoveCharacterToObject(EGO, GetObjectAt(...));
    if (!is_valid_object(obbj))
        return;

    walk_character(chaa,objs[obbj].x+5,objs[obbj].y+6,0, true);
    do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
}

void MoveCharacterToHotspot(int chaa,int hotsp) {
    if ((hotsp<0) || (hotsp>=MAX_HOTSPOTS))
        quit("!MovecharacterToHotspot: invalid hotspot");
    if (thisroom.hswalkto[hotsp].x<1) return;
    walk_character(chaa,thisroom.hswalkto[hotsp].x,thisroom.hswalkto[hotsp].y,0, true);
    do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
}

void MoveCharacterBlocking(int chaa,int xx,int yy,int direct) {
    if (!is_valid_character (chaa))
        quit("!MoveCharacterBlocking: invalid character");

    // check if they try to move the player when Hide Player Char is
    // ticked -- otherwise this will hang the game
    if (game.chars[chaa].on != 1)
        quit("!MoveCharacterBlocking: character is turned off (is Hide Player Character selected?) and cannot be moved");

    if (direct)
        MoveCharacterDirect(chaa,xx,yy);
    else
        MoveCharacter(chaa,xx,yy);
    do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
}


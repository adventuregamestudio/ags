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
// AGS Character functions
//
//=============================================================================

#include "util/wgt2allg.h"
#include "ac/character.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/roomstruct.h"
#include "ac/view.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/global_object.h"
#include "ac/global_region.h"
#include "ac/global_room.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/lipsync.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/overlay.h"
#include "ac/path.h"
#include "ac/properties.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "gui/guimain.h"
#include "ac/route_finder.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "main/update.h"
#include "ac/spritecache.h"
#include "util/string_utils.h"
#include <math.h>
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "platform/base/override_defines.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern GameSetupStruct game;
extern int displayed_room,starting_room;
extern roomstruct thisroom;
extern MoveList *mls;
extern int new_room_pos;
extern int new_room_x, new_room_y;
extern GameState play;
extern ViewStruct*views;
extern RoomObject*objs;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern ScriptInvItem scrInv[MAX_INV];
extern SpriteCache spriteset;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern Bitmap *walkable_areas_temp;
extern IGraphicsDriver *gfxDriver;
extern Bitmap **actsps;
extern int source_text_length;
extern int offsetx, offsety;
extern int is_text_overlay;
extern int said_speech_line;
extern int numscreenover;
extern int said_text;
extern int our_eip;
extern int update_music_at;
extern int scrnwid,scrnhit;
extern int current_screen_resolution_multiplier;
extern int cur_mode;
extern int screen_is_dirty;

//--------------------------------

#define CHECK_DIAGONAL(maindir,othdir,codea,codeb) \
    if (no_diagonal) ;\
  else if (abs(maindir) > abs(othdir) / 2) {\
  if (maindir < 0) useloop=codea;\
    else useloop=codeb;\
}


CharacterExtras *charextra;
CharacterInfo*playerchar;
long _sc_PlayerCharPtr = 0;
int char_lowest_yp;

// Sierra-style speech settings
int face_talking=-1,facetalkview=0,facetalkwait=0,facetalkframe=0;
int facetalkloop=0, facetalkrepeat = 0, facetalkAllowBlink = 1;
int facetalkBlinkLoop = 0;
CharacterInfo *facetalkchar = NULL;

// lip-sync speech settings
int loops_per_character, text_lips_offset, char_speaking = -1;
char *text_lips_text = NULL;
SpeechLipSyncLine *splipsync = NULL;
int numLipLines = 0, curLipLine = -1, curLipLinePhenome = 0;

// **** CHARACTER: FUNCTIONS ****

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
        do_main_cycle(UNTIL_SHORTIS0,(long)&chaa->animating);
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
                    do_main_cycle(UNTIL_MOVEEND,(long)&char1->walking);
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

    Character_FaceLocation(char1, objs[obj->id].x, objs[obj->id].y, blockingStyle);
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

    Bitmap *checkblk = GetObjectImage(objid->id, NULL);
    int objWidth = checkblk->GetWidth();
    int objHeight = checkblk->GetHeight();
    int o1x = objs[objid->id].x;
    int o1y = objs[objid->id].y - divide_down_coordinate(objHeight);

    Bitmap *charpic = GetCharacterImage(chin->index_id, NULL);

    int charWidth = charpic->GetWidth();
    int charHeight = charpic->GetHeight();
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
            int maskcol = checkblk->GetMaskColor ();
            int maskcolc = charpic->GetMaskColor ();
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
        do_main_cycle(UNTIL_MOVEEND,(long)&chaa->walking);
    else if ((blocking != IN_BACKGROUND) && (blocking != 0))
        quit("!Character.Walk: Blocking must be BLOCKING or IN_BACKGRUOND");

}

void Character_RunInteraction(CharacterInfo *chaa, int mood) {

    RunCharacterInteraction(chaa->index_id, mood);
}



// **** CHARACTER: PROPERTIES ****

int Character_GetProperty(CharacterInfo *chaa, const char *property) {

    return get_int_property(&game.charProps[chaa->index_id], property);

}
void Character_GetPropertyText(CharacterInfo *chaa, const char *property, char *bufer) {
    get_text_property(&game.charProps[chaa->index_id], property, bufer);
}
const char* Character_GetTextProperty(CharacterInfo *chaa, const char *property) {
    return get_text_property_dynamic_string(&game.charProps[chaa->index_id], property);
}

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

extern int char_speaking;

int Character_GetSpeakingFrame(CharacterInfo *chaa) {

    if ((face_talking >= 0) && (facetalkrepeat))
    {
        if (facetalkchar->index_id == chaa->index_id)
        {
            return facetalkframe;
        }
    }
    else if (char_speaking >= 0)
    {
        if (char_speaking == chaa->index_id)
        {
            return chaa->frame;
        }
    }

    quit("!Character.SpeakingFrame: character is not currently speaking");
    return -1;
}

//=============================================================================

// order of loops to turn character in circle from down to down
int turnlooporder[8] = {0, 6, 1, 7, 3, 5, 2, 4};

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

    // if the character who's moving doesn't Bitmap *, don't bother checking
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
int doNextCharMoveStep (CharacterInfo *chi, int &char_index, CharacterExtras *chex) {
    int ntf=0, xwas = chi->x, ywas = chi->y;

    if (do_movelist_move(&chi->walking,&chi->x,&chi->y) == 2) 
    {
        if ((chi->flags & CHF_MOVENOTWALK) == 0)
            fix_player_sprite(&mls[chi->walking], chi);
    }

    ntf = has_hit_another_character(char_index);
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
            chex->animwait = chi->walkwait;
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
            if (thisroom.walls->GetPixel(ex,ey) == 0) continue;
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


    int pixValue = thisroom.walls->GetPixel(convert_to_low_res(xx[0]), convert_to_low_res(yy[0]));
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
        do_main_cycle(UNTIL_MOVEEND,(long)&chaa->walking);
    else if ((blocking != IN_BACKGROUND) && (blocking != 0))
        quit("!Character.Walk: Blocking must be BLOCKING or IN_BACKGRUOND");

}

int is_valid_character(int newchar) {
    if ((newchar < 0) || (newchar >= game.numcharacters)) return 0;
    return 1;
}

int wantMoveNow (CharacterInfo *chi, CharacterExtras *chex) {
    // check most likely case first
    if ((chex->zoom == 100) || ((chi->flags & CHF_SCALEMOVESPEED) == 0))
        return 1;

    // the % checks don't work when the counter is negative, so once
    // it wraps round, correct it
    while (chi->walkwaitcounter < 0) {
        chi->walkwaitcounter += 12000;
    }

    // scaling 170-200%, move 175% speed
    if (chex->zoom >= 170) {
        if ((chi->walkwaitcounter % 4) >= 1)
            return 2;
        else
            return 1;
    }
    // scaling 140-170%, move 150% speed
    else if (chex->zoom >= 140) {
        if ((chi->walkwaitcounter % 2) == 1)
            return 2;
        else
            return 1;
    }
    // scaling 115-140%, move 125% speed
    else if (chex->zoom >= 115) {
        if ((chi->walkwaitcounter % 4) >= 3)
            return 2;
        else
            return 1;
    }
    // scaling 80-120%, normal speed
    else if (chex->zoom >= 80)
        return 1;
    // scaling 60-80%, move 75% speed
    if (chex->zoom >= 60) {
        if ((chi->walkwaitcounter % 4) >= 1)
            return 1;
    }
    // scaling 30-60%, move 50% speed
    else if (chex->zoom >= 30) {
        if ((chi->walkwaitcounter % 2) == 1)
            return -1;
        else if (chex->xwas != INVALID_X) {
            // move the second half of the movement to make it smoother
            chi->x = chex->xwas;
            chi->y = chex->ywas;
            chex->xwas = INVALID_X;
        }
    }
    // scaling 0-30%, move 25% speed
    else {
        if ((chi->walkwaitcounter % 4) >= 3)
            return -1;
        if (((chi->walkwaitcounter % 4) == 1) && (chex->xwas != INVALID_X)) {
            // move the second half of the movement to make it smoother
            chi->x = chex->xwas;
            chi->y = chex->ywas;
            chex->xwas = INVALID_X;
        }

    }

    return 0;
}

void setup_player_character(int charid) {
    game.playercharacter = charid;
    playerchar = &game.chars[charid];
    _sc_PlayerCharPtr = ccGetObjectHandleFromAddress((char*)playerchar);
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

void CheckViewFrameForCharacter(CharacterInfo *chi) {

    int soundVolumeWas = play.sound_volume;

    if (chi->flags & CHF_SCALEVOLUME) {
        // adjust the sound volume using the character's zoom level
        int zoom_level = charextra[chi->index_id].zoom;
        if (zoom_level == 0)
            zoom_level = 100;

        play.sound_volume = (play.sound_volume * zoom_level) / 100;

        if (play.sound_volume < 0)
            play.sound_volume = 0;
        if (play.sound_volume > 255)
            play.sound_volume = 255;
    }

    CheckViewFrame(chi->view, chi->loop, chi->frame);

    play.sound_volume = soundVolumeWas;
}

Bitmap *GetCharacterImage(int charid, int *isFlipped) 
{
    if (!gfxDriver->HasAcceleratedStretchAndFlip())
    {
        if (actsps[charid + MAX_INIT_SPR] != NULL) 
        {
            // the actsps image is pre-flipped, so no longer register the image as such
            if (isFlipped)
                *isFlipped = 0;
            return actsps[charid + MAX_INIT_SPR];
        }
    }
    CharacterInfo*chin=&game.chars[charid];
    int sppic = views[chin->view].loops[chin->loop].frames[chin->frame].pic;
    return spriteset[sppic];
}

CharacterInfo *GetCharacterAtLocation(int xx, int yy) {
    int hsnum = GetCharacterAt(xx, yy);
    if (hsnum < 0)
        return NULL;
    return &game.chars[hsnum];
}

extern int char_lowest_yp, obj_lowest_yp;

int is_pos_on_character(int xx,int yy) {
    int cc,sppic,lowestyp=0,lowestwas=-1;
    for (cc=0;cc<game.numcharacters;cc++) {
        if (game.chars[cc].room!=displayed_room) continue;
        if (game.chars[cc].on==0) continue;
        if (game.chars[cc].flags & CHF_NOINTERACT) continue;
        if (game.chars[cc].view < 0) continue;
        CharacterInfo*chin=&game.chars[cc];

        if ((chin->view < 0) || 
            (chin->loop >= views[chin->view].numLoops) ||
            (chin->frame >= views[chin->view].loops[chin->loop].numFrames))
        {
            continue;
        }

        sppic=views[chin->view].loops[chin->loop].frames[chin->frame].pic;
        int usewid = charextra[cc].width;
        int usehit = charextra[cc].height;
        if (usewid==0) usewid=spritewidth[sppic];
        if (usehit==0) usehit=spriteheight[sppic];
        int xxx = chin->x - divide_down_coordinate(usewid) / 2;
        int yyy = chin->get_effective_y() - divide_down_coordinate(usehit);

        int mirrored = views[chin->view].loops[chin->loop].frames[chin->frame].flags & VFLG_FLIPSPRITE;
        Bitmap *theImage = GetCharacterImage(cc, &mirrored);

        if (is_pos_in_sprite(xx,yy,xxx,yyy, theImage,
            divide_down_coordinate(usewid),
            divide_down_coordinate(usehit), mirrored) == FALSE)
            continue;

        int use_base = chin->get_baseline();
        if (use_base < lowestyp) continue;
        lowestyp=use_base;
        lowestwas=cc;
    }
    char_lowest_yp = lowestyp;
    return lowestwas;
}

void get_char_blocking_rect(int charid, int *x1, int *y1, int *width, int *y2) {
    CharacterInfo *char1 = &game.chars[charid];
    int cwidth, fromx;

    if (char1->blocking_width < 1)
        cwidth = divide_down_coordinate(GetCharacterWidth(charid)) - 4;
    else
        cwidth = char1->blocking_width;

    fromx = char1->x - cwidth/2;
    if (fromx < 0) {
        cwidth += fromx;
        fromx = 0;
    }
    if (fromx + cwidth >= convert_back_to_high_res(walkable_areas_temp->GetWidth()))
        cwidth = convert_back_to_high_res(walkable_areas_temp->GetWidth()) - fromx;

    if (x1)
        *x1 = fromx;
    if (width)
        *width = cwidth;
    if (y1)
        *y1 = char1->get_blocking_top();
    if (y2)
        *y2 = char1->get_blocking_bottom();
}

// Check whether the source char has walked onto character ww
int is_char_on_another (int sourceChar, int ww, int*fromxptr, int*cwidptr) {

    int fromx, cwidth;
    int y1, y2;
    get_char_blocking_rect(ww, &fromx, &y1, &cwidth, &y2);

    if (fromxptr)
        fromxptr[0] = fromx;
    if (cwidptr)
        cwidptr[0] = cwidth;

    // if the character trying to move is already on top of
    // this char somehow, allow them through
    if ((sourceChar >= 0) &&
        // x/width are left and width co-ords, so they need >= and <
        (game.chars[sourceChar].x >= fromx) &&
        (game.chars[sourceChar].x < fromx + cwidth) &&
        // y1/y2 are the top/bottom co-ords, so they need >= / <=
        (game.chars[sourceChar].y >= y1 ) &&
        (game.chars[sourceChar].y <= y2 ))
        return 1;

    return 0;
}

int my_getpixel(Bitmap *blk, int x, int y) {
    if ((x < 0) || (y < 0) || (x >= blk->GetWidth()) || (y >= blk->GetHeight()))
        return -1;

    // strip the alpha channel
	// TODO: is there a way to do this vtable thing with Bitmap?
	BITMAP *al_bmp = (BITMAP*)blk->GetBitmapObject();
    return al_bmp->vtable->getpixel(al_bmp, x, y) & 0x00ffffff;
}

int check_click_on_character(int xx,int yy,int mood) {
    int lowestwas=is_pos_on_character(xx,yy);
    if (lowestwas>=0) {
        RunCharacterInteraction (lowestwas, mood);
        return 1;
    }
    return 0;
}

void _DisplaySpeechCore(int chid, char *displbuf) {
    if (displbuf[0] == 0) {
        // no text, just update the current character who's speaking
        // this allows the portrait side to be switched with an empty
        // speech line
        play.swap_portrait_lastchar = chid;
        return;
    }

    // adjust timing of text (so that DisplaySpeech("%s", str) pauses
    // for the length of the string not 2 frames)
    if ((int)strlen(displbuf) > source_text_length + 3)
        source_text_length = strlen(displbuf);

    DisplaySpeech(displbuf, chid);
}

void _DisplayThoughtCore(int chid, const char *displbuf) {
    // adjust timing of text (so that DisplayThought("%s", str) pauses
    // for the length of the string not 2 frames)
    if ((int)strlen(displbuf) > source_text_length + 3)
        source_text_length = strlen(displbuf);

    int xpp = -1, ypp = -1, width = -1;

    if ((game.options[OPT_SPEECHTYPE] == 0) || (game.chars[chid].thinkview <= 0)) {
        // lucasarts-style, so we want a speech bubble actually above
        // their head (or if they have no think anim in Sierra-style)
        width = multiply_up_coordinate(play.speech_bubble_width);
        xpp = (multiply_up_coordinate(game.chars[chid].x) - offsetx) - width / 2;
        if (xpp < 0)
            xpp = 0;
        // -1 will automatically put it above the char's head
        ypp = -1;
    }

    _displayspeech ((char*)displbuf, chid, xpp, ypp, width, 1);
}

int user_to_internal_skip_speech(int userval) {
    // 0 = click mouse or key to skip
    if (userval == 0)
        return SKIP_AUTOTIMER | SKIP_KEYPRESS | SKIP_MOUSECLICK;
    // 1 = key only
    else if (userval == 1)
        return SKIP_AUTOTIMER | SKIP_KEYPRESS;
    // 2 = can't skip at all
    else if (userval == 2)
        return SKIP_AUTOTIMER;
    // 3 = only on keypress, no auto timer
    else if (userval == 3)
        return SKIP_KEYPRESS | SKIP_MOUSECLICK;
    // 4 = mouse only
    else if (userval == 4)
        return SKIP_AUTOTIMER | SKIP_MOUSECLICK;
    else
        quit("user_to_internal_skip_speech: unknown userval");

    return 0;
}


void _displayspeech(char*texx, int aschar, int xx, int yy, int widd, int isThought) {
    if (!is_valid_character(aschar))
        quit("!DisplaySpeech: invalid character");

    CharacterInfo *speakingChar = &game.chars[aschar];
    if ((speakingChar->view < 0) || (speakingChar->view >= game.numviews))
        quit("!DisplaySpeech: character has invalid view");

    if (is_text_overlay > 0)
        quit("!DisplaySpeech: speech was already displayed (nested DisplaySpeech, perhaps room script and global script conflict?)");

    EndSkippingUntilCharStops();

    said_speech_line = 1;

    int aa;
    if (play.bgspeech_stay_on_display == 0) {
        // remove any background speech
        for (aa=0;aa<numscreenover;aa++) {
            if (screenover[aa].timeout > 0) {
                remove_screen_overlay(screenover[aa].type);
                aa--;
            }
        }
    }
    said_text = 1;

    // the strings are pre-translated
    //texx = get_translation(texx);
    our_eip=150;

    int isPause = 1;
    // if the message is all .'s, don't display anything
    for (aa = 0; texx[aa] != 0; aa++) {
        if (texx[aa] != '.') {
            isPause = 0;
            break;
        }
    }

    play.messagetime = GetTextDisplayTime(texx);

    if (isPause) {
        if (update_music_at > 0)
            update_music_at += play.messagetime;
        do_main_cycle(UNTIL_INTISNEG,(long)&play.messagetime);
        return;
    }

    int textcol = speakingChar->talkcolor;

    // if it's 0, it won't be recognised as speech
    if (textcol == 0)
        textcol = 16;

    int allowShrink = 0;
    int bwidth = widd;
    if (bwidth < 0)
        bwidth = scrnwid/2 + scrnwid/4;

    our_eip=151;

    int useview = speakingChar->talkview;
    if (isThought) {
        useview = speakingChar->thinkview;
        // view 0 is not valid for think views
        if (useview == 0)
            useview = -1;
        // speech bubble can shrink to fit
        allowShrink = 1;
        if (speakingChar->room != displayed_room) {
            // not in room, centre it
            xx = -1;
            yy = -1;
        }
    }

    if (useview >= game.numviews)
        quitprintf("!Character.Say: attempted to use view %d for animation, but it does not exist", useview + 1);

    int tdxp = xx,tdyp = yy;
    int oldview=-1, oldloop = -1;
    int ovr_type = 0;

    text_lips_offset = 0;
    text_lips_text = texx;

    Bitmap *closeupface=NULL;
    if (texx[0]=='&') {
        // auto-speech
        int igr=atoi(&texx[1]);
        while ((texx[0]!=' ') & (texx[0]!=0)) texx++;
        if (texx[0]==' ') texx++;
        if (igr <= 0)
            quit("DisplaySpeech: auto-voice symbol '&' not followed by valid integer");

        text_lips_text = texx;

        if (play_speech(aschar,igr)) {
            if (play.want_speech == 2)
                texx = "  ";  // speech only, no text.
        }
    }
    if (game.options[OPT_SPEECHTYPE] == 3)
        remove_screen_overlay(OVER_COMPLETE);
    our_eip=1500;

    if (game.options[OPT_SPEECHTYPE] == 0)
        allowShrink = 1;

    if (speakingChar->idleleft < 0)  {
        // if idle anim in progress for the character, stop it
        ReleaseCharacterView(aschar);
        //    speakingChar->idleleft = speakingChar->idletime;
    }

    bool overlayPositionFixed = false;
    int charFrameWas = 0;
    int viewWasLocked = 0;
    if (speakingChar->flags & CHF_FIXVIEW)
        viewWasLocked = 1;

    /*if ((speakingChar->room == displayed_room) ||
    ((useview >= 0) && (game.options[OPT_SPEECHTYPE] > 0)) ) {*/

    if (speakingChar->room == displayed_room) {
        // If the character is in this room, go for it - otherwise
        // run the "else" clause which  does text in the middle of
        // the screen.
        our_eip=1501;
        if (tdxp < 0)
            tdxp = multiply_up_coordinate(speakingChar->x) - offsetx;
        if (tdxp < 2)
            tdxp=2;

        if (speakingChar->walking)
            StopMoving(aschar);

        // save the frame we need to go back to
        // if they were moving, this will be 0 (because we just called
        // StopMoving); otherwise, it might be a specific animation 
        // frame which we should return to
        if (viewWasLocked)
            charFrameWas = speakingChar->frame;

        // if the current loop doesn't exist in talking view, use loop 0
        if (speakingChar->loop >= views[speakingChar->view].numLoops)
            speakingChar->loop = 0;

        if ((speakingChar->view < 0) || 
            (speakingChar->loop >= views[speakingChar->view].numLoops) ||
            (views[speakingChar->view].loops[speakingChar->loop].numFrames < 1))
        {
            quitprintf("Unable to display speech because the character %s has an invalid view frame (View %d, loop %d, frame %d)", speakingChar->scrname, speakingChar->view + 1, speakingChar->loop, speakingChar->frame);
        }

        our_eip=1504;

        if (tdyp < 0) 
        {
            int sppic = views[speakingChar->view].loops[speakingChar->loop].frames[0].pic;
            tdyp = multiply_up_coordinate(speakingChar->get_effective_y()) - offsety - get_fixed_pixel_size(5);
            if (charextra[aschar].height < 1)
                tdyp -= spriteheight[sppic];
            else
                tdyp -= charextra[aschar].height;
            // if it's a thought, lift it a bit further up
            if (isThought)  
                tdyp -= get_fixed_pixel_size(10);
        }

        our_eip=1505;
        if (tdyp < 5)
            tdyp=5;

        tdxp=-tdxp;  // tell it to centre it
        our_eip=152;

        if ((useview >= 0) && (game.options[OPT_SPEECHTYPE] > 0)) {
            // Sierra-style close-up portrait

            if (play.swap_portrait_lastchar != aschar) {
                // if the portraits are set to Alternate, OR they are
                // set to Left but swap_portrait has been set to 1 (the old
                // method for enabling it), then swap them round
                if ((game.options[OPT_PORTRAITSIDE] == PORTRAIT_ALTERNATE) ||
                    ((game.options[OPT_PORTRAITSIDE] == 0) &&
                    (play.swap_portrait_side > 0))) {

                        if (play.swap_portrait_side == 2)
                            play.swap_portrait_side = 1;
                        else
                            play.swap_portrait_side = 2;
                }

                if (game.options[OPT_PORTRAITSIDE] == PORTRAIT_XPOSITION) {
                    // Portrait side based on character X-positions
                    if (play.swap_portrait_lastchar < 0) {
                        // no previous character been spoken to
                        // therefore, find another character in this room
                        // that it could be
                        for (int ce = 0; ce < game.numcharacters; ce++) {
                            if ((game.chars[ce].room == speakingChar->room) &&
                                (game.chars[ce].on == 1) &&
                                (ce != aschar)) {
                                    play.swap_portrait_lastchar = ce;
                                    break;
                            }
                        }
                    }

                    if (play.swap_portrait_lastchar >= 0) {
                        // if this character is right of the one before, put the
                        // portrait on the right
                        if (speakingChar->x > game.chars[play.swap_portrait_lastchar].x)
                            play.swap_portrait_side = -1;
                        else
                            play.swap_portrait_side = 0;
                    }
                }

                play.swap_portrait_lastchar = aschar;
            }

            // Determine whether to display the portrait on the left or right
            int portrait_on_right = 0;

            if (game.options[OPT_SPEECHTYPE] == 3) 
            { }  // always on left with QFG-style speech
            else if ((play.swap_portrait_side == 1) ||
                (play.swap_portrait_side == -1) ||
                (game.options[OPT_PORTRAITSIDE] == PORTRAIT_RIGHT))
                portrait_on_right = 1;


            int bigx=0,bigy=0,kk;
            ViewStruct*viptr=&views[useview];
            for (kk = 0; kk < viptr->loops[0].numFrames; kk++) 
            {
                int tw = spritewidth[viptr->loops[0].frames[kk].pic];
                if (tw > bigx) bigx=tw;
                tw = spriteheight[viptr->loops[0].frames[kk].pic];
                if (tw > bigy) bigy=tw;
            }

            // if they accidentally used a large full-screen image as the sierra-style
            // talk view, correct it
            if ((game.options[OPT_SPEECHTYPE] != 3) && (bigx > scrnwid - get_fixed_pixel_size(50)))
                bigx = scrnwid - get_fixed_pixel_size(50);

            if (widd > 0)
                bwidth = widd - bigx;

            our_eip=153;
            int draw_yp = 0, ovr_yp = get_fixed_pixel_size(20);
            if (game.options[OPT_SPEECHTYPE] == 3) {
                // QFG4-style whole screen picture
                closeupface = BitmapHelper::CreateBitmap(scrnwid, scrnhit, spriteset[viptr->loops[0].frames[0].pic]->GetColorDepth());
                closeupface->Clear(0);
                draw_yp = scrnhit/2 - spriteheight[viptr->loops[0].frames[0].pic]/2;
                bigx = scrnwid/2 - get_fixed_pixel_size(20);
                ovr_type = OVER_COMPLETE;
                ovr_yp = 0;
                tdyp = -1;  // center vertically
            }
            else {
                // KQ6-style close-up face picture
                if (yy < 0)
                    ovr_yp = adjust_y_for_guis (ovr_yp);
                else
                    ovr_yp = yy;

                closeupface = BitmapHelper::CreateBitmap(bigx+1,bigy+1,spriteset[viptr->loops[0].frames[0].pic]->GetColorDepth());
                closeupface->Clear(closeupface->GetMaskColor());
                ovr_type = OVER_PICTURE;

                if (yy < 0)
                    tdyp = ovr_yp + get_textwindow_top_border_height(play.speech_textwindow_gui);
            }
            //->Blit(closeupface,spriteset[viptr->frames[0][0].pic],0,draw_yp);
            DrawViewFrame(closeupface, &viptr->loops[0].frames[0], 0, draw_yp);

            int overlay_x = get_fixed_pixel_size(10);

            if (xx < 0) {
                tdxp = get_fixed_pixel_size(16) + bigx + get_textwindow_border_width(play.speech_textwindow_gui) / 2;

                int maxWidth = (scrnwid - tdxp) - get_fixed_pixel_size(5) - 
                    get_textwindow_border_width (play.speech_textwindow_gui) / 2;

                if (bwidth > maxWidth)
                    bwidth = maxWidth;
            }
            else {
                tdxp = xx + bigx + get_fixed_pixel_size(8);
                overlay_x = xx;
            }

            // allow the text box to be shrunk to fit the text
            allowShrink = 1;

            // if the portrait's on the right, swap it round
            if (portrait_on_right) {
                if ((xx < 0) || (widd < 0)) {
                    overlay_x = (scrnwid - bigx) - get_fixed_pixel_size(5);
                    tdxp = get_fixed_pixel_size(9);
                }
                else {
                    overlay_x = (xx + widd - bigx) - get_fixed_pixel_size(5);
                    tdxp = xx;
                }
                tdxp += get_textwindow_border_width(play.speech_textwindow_gui) / 2;
                allowShrink = 2;
            }
            if (game.options[OPT_SPEECHTYPE] == 3)
                overlay_x = 0;
            face_talking=add_screen_overlay(overlay_x,ovr_yp,ovr_type,closeupface);
            facetalkframe = 0;
            facetalkwait = viptr->loops[0].frames[0].speed + GetCharacterSpeechAnimationDelay(speakingChar);
            facetalkloop = 0;
            facetalkview = useview;
            facetalkrepeat = (isThought) ? 0 : 1;
            facetalkBlinkLoop = 0;
            facetalkAllowBlink = 1;
            if ((isThought) && (speakingChar->flags & CHF_NOBLINKANDTHINK))
                facetalkAllowBlink = 0;
            facetalkchar = &game.chars[aschar];
            if (facetalkchar->blinktimer < 0)
                facetalkchar->blinktimer = facetalkchar->blinkinterval;
            textcol=-textcol;
            overlayPositionFixed = true;
        }
        else if (useview >= 0) {
            // Lucasarts-style speech
            our_eip=154;

            oldview = speakingChar->view;
            oldloop = speakingChar->loop;
            speakingChar->animating = 1 | (GetCharacterSpeechAnimationDelay(speakingChar) << 8);
            // only repeat if speech, not thought
            if (!isThought)
                speakingChar->animating |= CHANIM_REPEAT;

            speakingChar->view = useview;
            speakingChar->frame=0;
            speakingChar->flags|=CHF_FIXVIEW;

            if (speakingChar->loop >= views[speakingChar->view].numLoops)
            {
                // current character loop is outside the normal talking directions
                speakingChar->loop = 0;
            }

            facetalkBlinkLoop = speakingChar->loop;

            if ((speakingChar->loop >= views[speakingChar->view].numLoops) ||
                (views[speakingChar->view].loops[speakingChar->loop].numFrames < 1))
            {
                quitprintf("!Unable to display speech because the character %s has an invalid speech view (View %d, loop %d, frame %d)", speakingChar->scrname, speakingChar->view + 1, speakingChar->loop, speakingChar->frame);
            }

            // set up the speed of the first frame
            speakingChar->wait = GetCharacterSpeechAnimationDelay(speakingChar) + 
                views[speakingChar->view].loops[speakingChar->loop].frames[0].speed;

            if (widd < 0) {
                bwidth = scrnwid/2 + scrnwid/6;
                // If they are close to the screen edge, make the text narrower
                int relx = multiply_up_coordinate(speakingChar->x) - offsetx;
                if ((relx < scrnwid / 4) || (relx > scrnwid - (scrnwid / 4)))
                    bwidth -= scrnwid / 5;
            }
            /*   this causes the text to bob up and down as they talk
            tdxp = OVR_AUTOPLACE;
            tdyp = aschar;*/
            if (!isThought)  // set up the lip sync if not thinking
                char_speaking = aschar;

        }
    }
    else
        allowShrink = 1;

    // it wants the centred position, so make it so
    if ((xx >= 0) && (tdxp < 0))
        tdxp -= widd / 2;

    // if they used DisplaySpeechAt, then use the supplied width
    if ((widd > 0) && (isThought == 0))
        allowShrink = 0;

    our_eip=155;
    _display_at(tdxp,tdyp,bwidth,texx,0,textcol, isThought, allowShrink, overlayPositionFixed);
    our_eip=156;
    if ((play.in_conversation > 0) && (game.options[OPT_SPEECHTYPE] == 3))
        closeupface = NULL;
    if (closeupface!=NULL)
        remove_screen_overlay(ovr_type);
    screen_is_dirty = 1;
    face_talking = -1;
    facetalkchar = NULL;
    our_eip=157;
    if (oldview>=0) {
        speakingChar->flags &= ~CHF_FIXVIEW;
        if (viewWasLocked)
            speakingChar->flags |= CHF_FIXVIEW;
        speakingChar->view=oldview;

        // Don't reset the loop in 2.x games
        if (loaded_game_file_version > 32)
            speakingChar->loop = oldloop;

        speakingChar->animating=0;
        speakingChar->frame = charFrameWas;
        speakingChar->wait=0;
        speakingChar->idleleft = speakingChar->idletime;
        // restart the idle animation straight away
        charextra[aschar].process_idle_this_time = 1;
    }
    char_speaking = -1;
    stop_speech();
}

int get_character_currently_talking() {
    if ((face_talking >= 0) && (facetalkrepeat))
        return facetalkchar->index_id;
    else if (char_speaking >= 0)
        return char_speaking;

    return -1;
}

void DisplaySpeech(char*texx, int aschar) {
    _displayspeech (texx, aschar, -1, -1, -1, 0);
}

// Calculate which frame of the loop to use for this character of
// speech
int GetLipSyncFrame (char *curtex, int *stroffs) {
    /*char *frameletters[MAXLIPSYNCFRAMES] =
    {"./,/ ", "A", "O", "F/V", "D/N/G/L/R", "B/P/M",
    "Y/H/K/Q/C", "I/T/E/X/th", "U/W", "S/Z/J/ch", NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};*/

    int bestfit_len = 0, bestfit = game.default_lipsync_frame;
    for (int aa = 0; aa < MAXLIPSYNCFRAMES; aa++) {
        char *tptr = game.lipSyncFrameLetters[aa];
        while (tptr[0] != 0) {
            int lenthisbit = strlen(tptr);
            if (strchr(tptr, '/'))
                lenthisbit = strchr(tptr, '/') - tptr;

            if ((strnicmp (curtex, tptr, lenthisbit) == 0) && (lenthisbit > bestfit_len)) {
                bestfit = aa;
                bestfit_len = lenthisbit;
            }
            tptr += lenthisbit;
            while (tptr[0] == '/')
                tptr++;
        }
    }
    // If it's an unknown character, use the default frame
    if (bestfit_len == 0)
        bestfit_len = 1;
    *stroffs += bestfit_len;
    return bestfit;
}

int update_lip_sync(int talkview, int talkloop, int *talkframeptr) {
    int talkframe = talkframeptr[0];
    int talkwait = 0;

    // lip-sync speech
    char *nowsaying = &text_lips_text[text_lips_offset];
    // if it's an apostraphe, skip it (we'll, I'll, etc)
    if (nowsaying[0] == '\'') {
        text_lips_offset++;
        nowsaying++;
    }

    if (text_lips_offset >= (int)strlen(text_lips_text))
        talkframe = 0;
    else {
        talkframe = GetLipSyncFrame (nowsaying, &text_lips_offset);
        if (talkframe >= views[talkview].loops[talkloop].numFrames)
            talkframe = 0;
    }

    talkwait = loops_per_character + views[talkview].loops[talkloop].frames[talkframe].speed;

    talkframeptr[0] = talkframe;
    return talkwait;
}

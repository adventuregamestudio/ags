//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// AGS Character functions
//
//=============================================================================
#include <cstdio>
#include "ac/character.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_room.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/lipsync.h"
#include "ac/mouse.h"
#include "ac/movelist.h"
#include "ac/object.h"
#include "ac/overlay.h"
#include "ac/properties.h"
#include "ac/region.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/route_finder.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/view.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "ac/dynobj/scriptmotionpath.h"
#include "ac/dynobj/scriptshader.h"
#include "debug/debug_log.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "main/update.h"
#include "util/string_compat.h"
#include <math.h>
#include "gfx/graphicsdriver.h"
#include "script/runtimescriptvalue.h"
#include "script/script.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/cc_inventory.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptuserobject.h"
#include "script/script_runtime.h"
#include "gfx/gfx_def.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int displayed_room;
extern RoomStruct thisroom;
extern RoomStatus *croom;
extern std::vector<ViewStruct> views;
extern RoomObject*objs;
extern ScriptInvItem scrInv[MAX_INV];
extern SpriteCache spriteset;
extern Bitmap *walkable_areas_temp;
extern IGraphicsDriver *gfxDriver;
extern int said_speech_line;
extern int said_text;
extern CCCharacter ccDynamicCharacter;
extern CCInventory ccDynamicInv;

//--------------------------------


CharacterInfo*playerchar;
int32_t _sc_PlayerCharPtr = 0;
int char_lowest_yp;

// TODO: move all these to either GamePlayState or some kind of a Speech struct
// Indexes of currently speaking and thinking character
int char_speaking = -1, char_speaking_anim = -1, char_thinking = -1;
// Sierra-style speech settings
int face_talking=-1,facetalkview=0,facetalkwait=0,facetalkframe=0;
int facetalkloop=0, facetalkrepeat = 0, facetalkAllowBlink = 1;
int facetalkBlinkLoop = 0;
CharacterInfo *facetalkchar = nullptr;
// Do override default portrait position during QFG4-style speech overlay update
bool facetalk_qfg4_override_placement_x = false;
bool facetalk_qfg4_override_placement_y = false;

// lip-sync speech settings
int loops_per_character, text_lips_offset;
const char *text_lips_text = nullptr;
std::vector<SpeechLipSyncLine> splipsync;
int numLipLines = 0, curLipLine = -1, curLipLinePhoneme = 0;

// **** CHARACTER: FUNCTIONS ****

bool is_valid_character(int char_id)
{
    return ((char_id >= 0) && (char_id < game.numcharacters));
}

// Checks if character is currently playing idle anim, and reset it
void stop_character_idling(CharacterInfo *chi)
{
    if (chi->is_idling())
    {
        Character_UnlockView(chi);
        chi->idleleft = chi->idletime;
    }
}

// Resets idling timer, and marks for immediate update (in case its persistent idling)
void reset_character_idling_time(CharacterInfo *chi)
{
    chi->idleleft = chi->idletime;
    charextra[chi->index_id].process_idle_this_time = 1;
}

bool AssertCharacter(const char *apiname, int char_id)
{
    if ((char_id >= 0) && (char_id < game.numcharacters))
        return true;
    debug_script_warn("%s: invalid character id %d (range is 0..%d)", apiname, char_id, game.numcharacters - 1);
    return false;
}

void Character_AddInventory(CharacterInfo *chaa, ScriptInvItem *invi, int addIndex) {
    int ee;

    if (invi == nullptr)
        quit("!AddInventoryToCharacter: invalid inventory number");

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
                    run_on_event(kScriptEvent_InventoryAdd, inum);
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
    GUIE::MarkInventoryForUpdate(charid, charid == game.playercharacter);
    if (chaa == playerchar)
        run_on_event(kScriptEvent_InventoryAdd, inum);
}

void Character_AddWaypoint(CharacterInfo *chaa, int x, int y) {

    if (chaa->room != displayed_room)
        quitprintf("!MoveCharacterPath: character %s is not in current room %d (it is in room %d)",
            chaa->scrname.GetCStr(), displayed_room, chaa->room);

    // not already walking, so just do a normal move
    if (chaa->walking <= 0) {
        Character_Walk(chaa, x, y, IN_BACKGROUND, ANYWHERE);
        return;
    }

    MoveList &cmls = *get_movelist(chaa->get_movelist_id());

    // They're already walking there anyway
    const Point &last_pos = cmls.GetLastPos();
    if (last_pos == Point(x, y))
        return;

    int move_speed_x, move_speed_y;
    chaa->get_effective_walkspeeds(move_speed_x, move_speed_y);
    if ((move_speed_x == 0) && (move_speed_y == 0))
    {
        debug_script_warn("Character::AddWaypoint: called for '%s' with walk speed 0", chaa->scrname.GetCStr());
    }

    Pathfinding::AddWaypointDirect(cmls, x, y, move_speed_x, move_speed_y, kMoveStage_Direct);
}

void Character_Animate(CharacterInfo *chaa, int loop, int delay, int repeat,
    int blocking, int direction, int sframe, int volume)
{
    // If idle view in progress for the character, stop the idle anim;
    // do this prior to the loop check, as the view may switch back to defview here
    stop_character_idling(chaa);

    ValidateViewAnimVLF("Character.Animate", chaa->scrname.GetCStr(), chaa->view, loop, sframe);
    ValidateViewAnimParams("Character.Animate", chaa->scrname.GetCStr(), blocking, repeat, direction);

    animate_character(chaa, loop, delay, repeat, direction, sframe, volume);

    if (blocking != 0)
        GameLoopUntilViewAnimEnd(&charextra[chaa->index_id].anim);
}

void Character_Animate5(CharacterInfo *chaa, int loop, int delay, int repeat, int blocking, int direction) {
    Character_Animate(chaa, loop, delay, repeat, blocking, direction, 0 /* first frame */, 100 /* full volume */);
}

void Character_Animate6(CharacterInfo *chaa, int loop, int delay, int repeat, int blocking, int direction, int sframe) {
    Character_Animate(chaa, loop, delay, repeat, blocking, direction, sframe, 100 /* full volume */);
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
        if (chaa->x <= thisroom.Edges.Left + 10)
            new_room_pos = 2000;
        else if (chaa->x >= thisroom.Edges.Right - 10)
            new_room_pos = 1000;
        else if (chaa->y <= thisroom.Edges.Top + 10)
            new_room_pos = 3000;
        else if (chaa->y >= thisroom.Edges.Bottom - 10)
            new_room_pos = 4000;

        if (new_room_pos < 3000)
            new_room_pos += chaa->y;
        else
            new_room_pos += chaa->x;
    }
    NewRoom(room);
}

void Character_ChangeRoom(CharacterInfo *chaa, int room, int x, int y) {
    Character_ChangeRoomSetLoop(chaa, room, x, y, SCR_NO_VALUE);
}

void Character_ChangeRoomSetLoop(CharacterInfo *chaa, int room, int x, int y, int direction) {

    if (chaa->index_id != game.playercharacter) {
        // NewRoomNPC
        if ((x != SCR_NO_VALUE) && (y != SCR_NO_VALUE)) {
            chaa->x = x;
            chaa->y = y;
			if (direction != SCR_NO_VALUE && direction>=0) chaa->loop = direction;
        }
        chaa->prevroom = chaa->room;
        chaa->room = room;

		debug_script_log("%s moved to room %d, location %d,%d, loop %d",
			chaa->scrname.GetCStr(), room, chaa->x, chaa->y, chaa->loop);

        return;
    }

    if ((x != SCR_NO_VALUE) && (y != SCR_NO_VALUE)) {
        // We cannot set character position right away,
        // because room switch will occur only after the script end,
        // and character position may be still changing meanwhile.
        new_room_pos = 0;
        // don't check X or Y bounds, so that they can do a
        // walk-in animation if they want
        new_room_x = x;
        new_room_y = y;
        if (direction != SCR_NO_VALUE) new_room_loop = direction;
    }

    NewRoom(room);
}


void Character_ChangeView(CharacterInfo *chap, int vii) {
    vii--;

    if ((vii < 0) || (vii >= game.numviews))
        quit("!ChangeCharacterView: invalid view number specified");

    // if animating, but not idle view, give warning message
    if ((chap->flags & CHF_FIXVIEW) && (chap->idleleft >= 0))
        debug_script_warn("Warning: ChangeCharacterView was used while the view was fixed - call ReleaseCharView first");

    // if the idle animation is playing we should release the view
    stop_character_idling(chap);

    debug_script_log("%s: Change view to %d", chap->scrname.GetCStr(), vii+1);
    chap->defview = vii;
    chap->view = vii;
    stop_character_anim(chap);
    chap->frame = 0;
    chap->wait = 0;
    chap->walkwait = 0;
    charextra[chap->index_id].animwait = 0;
    FindReasonableLoopForCharacter(chap);
}

enum DirectionalLoop
{
    kDirLoop_Down      = 0,
    kDirLoop_Left      = 1,
    kDirLoop_Right     = 2,
    kDirLoop_Up        = 3,
    kDirLoop_DownRight = 4,
    kDirLoop_UpRight   = 5,
    kDirLoop_DownLeft  = 6,
    kDirLoop_UpLeft    = 7,

    kDirLoop_Default        = kDirLoop_Down,
    kDirLoop_LastOrthogonal = kDirLoop_Up,
    kDirLoop_Last           = kDirLoop_UpLeft,
};

// Internal direction-facing functions

float GetFaceDirRatio(CharacterInfo *chinfo)
{
    CharacterExtras &chex = charextra[chinfo->index_id];
    if (chex.face_dir_ratio != 0.f)
        return chex.face_dir_ratio;
    // TODO: cache current area in CharacterExtras somewhere during early char update
    int onarea = get_walkable_area_at_location(chinfo->x, chinfo->y);
    if (onarea > 0 && thisroom.WalkAreas[onarea].FaceDirectionRatio != 0.f)
        return thisroom.WalkAreas[onarea].FaceDirectionRatio;
    if (croom->face_dir_ratio != 0.f)
        return croom->face_dir_ratio;
    if (play.face_dir_ratio != 0.f)
        return play.face_dir_ratio;
    return 1.f;
}

DirectionalLoop GetDirectionalLoop(CharacterInfo *chinfo, float x_diff, float y_diff, bool move_dir_fw = true)
{
    DirectionalLoop next_loop = kDirLoop_Left; // NOTE: default loop was Left for some reason

    if (!move_dir_fw)
    {
        x_diff = -x_diff;
        y_diff = -y_diff;
    }

    // TODO: cache this in CharacterExtras for bit more performance
    float dir_ratio = GetFaceDirRatio(chinfo);
    assert(dir_ratio != 0.f);
    y_diff *= dir_ratio; // dir ratio is a y/x relation

    const ViewStruct &chview  = views[chinfo->view];
    const bool has_down_loop  = ((chview.numLoops > kDirLoop_Down)  && (chview.loops[kDirLoop_Down].numFrames > 0));
    const bool has_up_loop    = ((chview.numLoops > kDirLoop_Up)    && (chview.loops[kDirLoop_Up].numFrames > 0));
    // NOTE: 3.+ games required left & right loops to be present at all times
	const bool has_left_loop  = true; // ((chview.numLoops > kDirLoop_Left)  && (chview.loops[kDirLoop_Left].numFrames > 0));
	const bool has_right_loop = true; // ((chview.numLoops > kDirLoop_Right) && (chview.loops[kDirLoop_Right].numFrames > 0));
    const bool has_diagonal_loops = useDiagonal(chinfo) == 0; // NOTE: useDiagonal returns 0 for "true"

	const bool want_horizontal = (abs(y_diff) < abs(x_diff)) || (!has_down_loop || !has_up_loop);

    if (want_horizontal)
    {
        const bool want_diagonal = has_diagonal_loops && (abs(y_diff) > abs(x_diff) / 2);
        if (!has_left_loop && !has_right_loop)
        {
            next_loop = kDirLoop_Down;
        }
        else if (has_right_loop && (x_diff > 0))
        {
            next_loop = want_diagonal ? (y_diff < 0 ? kDirLoop_UpRight : kDirLoop_DownRight) :
                kDirLoop_Right;
        }
        else if (has_left_loop && (x_diff <= 0))
        {
            next_loop = want_diagonal ? (y_diff < 0 ? kDirLoop_UpLeft : kDirLoop_DownLeft) :
                kDirLoop_Left;
        }
    }
    else
    {
        const bool want_diagonal = has_diagonal_loops && (abs(x_diff) > abs(y_diff) / 2);
        if (y_diff > 0 || !has_up_loop)
        {
            next_loop = want_diagonal ? (x_diff < 0 ? kDirLoop_DownLeft : kDirLoop_DownRight) :
                kDirLoop_Down;
        }
        else
        {
            next_loop = want_diagonal ? (x_diff < 0 ? kDirLoop_UpLeft : kDirLoop_UpRight) :
                kDirLoop_Up;
        }
    }
    return next_loop;
}

void FaceDirectionalLoop(CharacterInfo *char1, int direction, int blockingStyle)
{
    stop_character_idling(char1);

    // Change facing only if the desired direction is different
    if (direction != char1->loop)
    {
        if ((game.options[OPT_CHARTURNWHENFACE] != 0) && ((char1->flags & CHF_TURNWHENFACE) != 0) &&
            (!in_enters_screen))
        {
            const int no_diagonal = useDiagonal (char1);
            const int highestLoopForTurning = no_diagonal != 1 ? kDirLoop_Last : kDirLoop_LastOrthogonal;
            if ((char1->loop <= highestLoopForTurning))
            {
                // Turn to face new direction
                Character_StopMoving(char1);
                if (char1->is_enabled())
                {
                    // only do the turning if the character is not hidden
                    // (otherwise GameLoopUntilNotMoving will never return)
                    start_character_turning (char1, direction, no_diagonal);

                    if ((blockingStyle == BLOCKING) || (blockingStyle == 1))
                        GameLoopUntilFlagUnset(&charextra[char1->index_id].flags, kCharf_TurningMask);
                }
                else
                    char1->loop = direction;
            }
            else
                char1->loop = direction;
        }
        else
            char1->loop = direction;
    }

    char1->frame = 0;
    reset_character_idling_time(char1);
}

void FaceLocationXY(CharacterInfo *char1, int xx, int yy, int blockingStyle)
{
    debug_script_log("%s: Face location %d,%d", char1->scrname.GetCStr(), xx, yy);

    const int diffrx = xx - char1->x;
    const int diffry = yy - char1->y;

    if ((diffrx == 0) && (diffry == 0)) {
        // FaceLocation called on their current position - do nothing
        return;
    }

    FaceDirectionalLoop(char1, GetDirectionalLoop(char1, diffrx, diffry), blockingStyle);
}

// External direction-facing functions with validation

void Character_FaceDirection(CharacterInfo *char1, int direction, int blockingStyle)
{
    if (char1 == nullptr)
        quit("!FaceDirection: invalid character specified");

    if (direction != SCR_NO_VALUE)
    {
        if (direction < 0 || direction > kDirLoop_Last)
            quit("!FaceDirection: invalid direction specified");

        FaceDirectionalLoop(char1, direction, blockingStyle);
    }
}

void Character_FaceLocation(CharacterInfo *char1, int xx, int yy, int blockingStyle)
{
    if (char1 == nullptr)
        quit("!FaceLocation: invalid character specified");

    FaceLocationXY(char1, xx, yy, blockingStyle);
}

void Character_FaceObject(CharacterInfo *char1, ScriptObject *obj, int blockingStyle) {
    if (obj == nullptr) 
        quit("!FaceObject: invalid object specified");

    FaceLocationXY(char1, objs[obj->id].x, objs[obj->id].y, blockingStyle);
}

void Character_FaceCharacter(CharacterInfo *char1, CharacterInfo *char2, int blockingStyle) {
    if (char2 == nullptr) 
        quit("!FaceCharacter: invalid character specified");

    if (char1->room != char2->room)
        quitprintf("!FaceCharacter: characters %s and %s are in different rooms (room %d and room %d respectively)",
            char1->scrname.GetCStr(), char2->scrname.GetCStr(), char1->room, char2->room);

    FaceLocationXY(char1, char2->x, char2->y, blockingStyle);
}

void Character_FollowCharacter(CharacterInfo *chaa, CharacterInfo *tofollow, int distaway, int eagerness) {

    // FOLLOW_ALWAYSONTOP constant limits distaway to 32766
    if ((distaway < 0) || (distaway > FOLLOW_ALWAYSONTOP))
        quitprintf("!FollowCharacterEx: invalid distance: must be 0-%d or FOLLOW_EXACTLY", FOLLOW_ALWAYSONTOP - 1);
    if ((eagerness < 0) || (eagerness > 250))
        quit("!FollowCharacterEx: invalid eagerness: must be 0-250");

    if ((chaa->index_id == game.playercharacter) && (tofollow != nullptr) && 
        (tofollow->room != chaa->room))
        quitprintf("!FollowCharacterEx: you cannot tell the player character %s, who is in room %d, to follow a character %s who is in another room %d",
            chaa->scrname.GetCStr(), chaa->room, tofollow->scrname.GetCStr(), tofollow->room);

    if (tofollow != nullptr)   
    {
        debug_script_log("%s: Start following %s (dist %d, eager %d)", chaa->scrname.GetCStr(), tofollow->scrname.GetCStr(), distaway, eagerness);
    }
    else
    {
        debug_script_log("%s: Stop following other character", chaa->scrname.GetCStr());
    }

    CharacterExtras *chex = &charextra[chaa->index_id];
    if ((chex->following >= 0) &&
        (chex->follow_dist == FOLLOW_ALWAYSONTOP))
    {
            // if this character was following always-on-top, its baseline will
            // have been changed, so release it.
            chaa->baseline = -1;
    }

    chex->SetFollowing(chaa, tofollow ? tofollow->index_id : -1, distaway, eagerness, (eagerness == 1));

    if (chex->IsAnimatingRepeatedly())
        debug_script_warn("Warning: FollowCharacter called but the sheep is currently animating looped. It may never start to follow.");
}

CharacterInfo* Character_GetFollowing(CharacterInfo* chaa)
{
    if (charextra[chaa->index_id].following < 0)
        return nullptr;

    return &game.chars[charextra[chaa->index_id].following];
}

int GetCharacterWidth(int charid) {
    CharacterInfo *char1 = &game.chars[charid];

    if (charextra[charid].width < 1)
    {
        if ((char1->view < 0) ||
            (char1->loop >= views[char1->view].numLoops) ||
            (char1->frame >= views[char1->view].loops[char1->loop].numFrames))
        {
            debug_script_warn("GetCharacterWidth: Character %s has invalid frame: view %d, loop %d, frame %d",
                              char1->scrname.GetCStr(), char1->view + 1, char1->loop, char1->frame);
            return 4;
        }

        return game.SpriteInfos[views[char1->view].loops[char1->loop].frames[char1->frame].pic].Width;
    }
    else
        return charextra[charid].width;
}

int GetCharacterHeight(int charid) {
    CharacterInfo *char1 = &game.chars[charid];

    if (charextra[charid].height < 1)
    {
        if ((char1->view < 0) ||
            (char1->loop >= views[char1->view].numLoops) ||
            (char1->frame >= views[char1->view].loops[char1->loop].numFrames))
        {
            debug_script_warn("GetCharacterHeight: Character %s has invalid frame: view %d, loop %d, frame %d",
                              char1->scrname.GetCStr(), char1->view + 1, char1->loop, char1->frame);
            return 2;
        }

        return game.SpriteInfos[views[char1->view].loops[char1->loop].frames[char1->frame].pic].Height;
    }
    else
        return charextra[charid].height;
}

int Character_IsCollidingWithChar(CharacterInfo *char1, CharacterInfo *char2) {
    if (char2 == nullptr)
        quit("!AreCharactersColliding: invalid char2");

    if (char1->room != char2->room) return 0; // not colliding

    if ((char1->y > char2->y - 5) && (char1->y < char2->y + 5)) ;
    else return 0;

    int w1 = GetCharacterWidth(char1->index_id);
    int w2 = GetCharacterWidth(char2->index_id);

    int xps1=char1->x - w1/2;
    int xps2=char2->x - w2/2;

    if ((xps1 >= xps2 - w1) & (xps1 <= xps2 + w2)) return 1;
    return 0;
}

int Character_IsCollidingWithObject(CharacterInfo *chin, ScriptObject *objid) {
    if (objid == nullptr)
        quit("!AreCharObjColliding: invalid object number");

    if (chin->room != displayed_room)
        return 0;
    if (!objs[objid->id].is_enabled())
        return 0;

    // TODO: use GraphicSpace and proper transformed coords?

    Bitmap *checkblk = GetObjectImage(objid->id);
    int objWidth = checkblk->GetWidth();
    int objHeight = checkblk->GetHeight();
    int o1x = objs[objid->id].x;
    int o1y = objs[objid->id].y - objHeight;

    Bitmap *charpic = GetCharacterImage(chin->index_id);

    int charWidth = charpic->GetWidth();
    int charHeight = charpic->GetHeight();
    int o2x = chin->x - charWidth / 2;
    int o2y = charextra[chin->index_id].GetEffectiveY(chin) - 5;  // only check feet

    if ((o2x >= o1x - charWidth) &&
        (o2x <= o1x + objWidth) &&
        (o2y >= o1y - 8) &&
        (o2y <= o1y + objHeight)) {
            // the character's feet are on the object
            if (game.options[OPT_PIXPERFECT] == 0)
                return 1;
            // check if they're on a transparent bit of the object
            int stxp = o2x - o1x;
            int styp = o2y - o1y;
            int maskcol = checkblk->GetMaskColor ();
            int maskcolc = charpic->GetMaskColor ();
            int thispix, thispixc;
            // check each pixel of the object along the char's feet
            for (int i = 0; i < charWidth; i += 1) {
                for (int j = 0; j < 6; j += 1) {
                    thispix = my_getpixel(checkblk, i + stxp, j + styp);
                    thispixc = my_getpixel(charpic, i, j + (charHeight - 5));

                    if ((thispix != -1) && (thispix != maskcol) &&
                        (thispixc != -1) && (thispixc != maskcolc))
                        return 1;
                }
            }

    }
    return 0;
}

bool Character_IsInteractionAvailable(CharacterInfo *cchar, int mood) {

    play.check_interaction_only = 1;
    Character_RunInteraction(cchar, mood);
    int ciwas = play.check_interaction_only;
    play.check_interaction_only = 0;
    return (ciwas == 2);
}

void Character_LockViewImpl(CharacterInfo *chap, const char *api_name,
    int view, int loop, int frame,
    const Pointf &anchor, const Point &offset, bool stop_moving)
{
    view--; // convert to 0-based
    AssertView(api_name, chap->scrname.GetCStr(), view);
    AssertViewHasLoops(api_name, chap->scrname.GetCStr(), view);

    if (loop >= 0)
        AssertLoop(api_name, chap->scrname.GetCStr(), chap->view, loop);
    else
        loop = 0;
    if (frame >= 0)
        AssertFrame(api_name, chap->scrname.GetCStr(), view - 1, loop, frame);
    else
        frame = 0;

    stop_character_idling(chap);
    if (stop_moving)
    {
        Character_StopMoving(chap);
    }
    stop_character_anim(chap);
    charextra[chap->index_id].SetLockedView(chap, view, loop, frame, anchor, offset);
    FindReasonableLoopForCharacter(chap);
    debug_script_log("%s: View locked to %d", chap->scrname.GetCStr(), view + 1);
}

void Character_LockViewEx(CharacterInfo *chap, int vii, int stopMoving)
{
    Character_LockViewImpl(chap, "Character.LockView", vii, -1, -1,
        CharacterInfo::GetDefaultSpriteAnchor(), Point(), stopMoving != 0);
}

void Character_LockView(CharacterInfo *chap, int vii)
{
    Character_LockViewEx(chap, vii, STOP_MOVING);
}

void Character_LockViewAlignedEx(CharacterInfo *chap, int vii, int loop, int align, int stopMoving)
{
    if ((align & kMAlignAny) == 0)
    {
        debug_script_warn("Character.LockViewAligned: invalid alignment type specified");
    }

    Character_LockViewImpl(chap, "Character.LockViewAligned", vii, loop, -1,
        GfxDef::GetGraphicAnchorFromAlignment(static_cast<FrameAlignment>(align)), Point(),
        stopMoving != 0);
}

void Character_LockViewAnchored(CharacterInfo *chaa, int view, float x_anchor, float y_anchor, int x_off, int y_off, int stop_moving)
{
    Character_LockViewImpl(chaa, "Character.LockViewAnchored", view, -1, -1,
        Pointf(x_anchor, y_anchor), Point(x_off, y_off),
        stop_moving != 0);
}

void Character_LockViewFrameEx(CharacterInfo *chaa, int view, int loop, int frame, int stopMoving)
{
    Character_LockViewImpl(chaa, "Character.LockViewFrame", view, loop, frame,
        CharacterInfo::GetDefaultSpriteAnchor(), Point(),
        stopMoving != 0);
}

void Character_LockViewOffsetEx(CharacterInfo *chap, int vii, int xoffs, int yoffs, int stopMoving)
{
    Character_LockViewImpl(chap, "Character.LockViewOffset", vii, -1, -1,
        CharacterInfo::GetDefaultSpriteAnchor(), Point(xoffs, yoffs),
        stopMoving != 0);
}

void Character_LoseInventory(CharacterInfo *chap, ScriptInvItem *invi) {

    if (invi == nullptr)
        quit("!LoseInventoryFromCharacter: invalid inventory number");

    int inum = invi->id;

    if (chap->inv[inum] > 0)
        chap->inv[inum]--;

    if ((chap->activeinv == inum) & (chap->inv[inum] < 1)) {
        chap->activeinv = -1;
        if ((chap == playerchar) && (is_current_cursor_mode(kCursorRole_UseInv)))
            set_cursor_mode(0); // change to the first enabled mode
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
    GUIE::MarkInventoryForUpdate(charid, charid == game.playercharacter);

    if (chap == playerchar)
        run_on_event(kScriptEvent_InventoryLose, inum);
}

void Character_PlaceOnWalkableArea(CharacterInfo *chap) 
{
    if (displayed_room < 0)
        quit("!Character.PlaceOnWalkableArea: no room is currently loaded");

    Point dst;
    if (FindNearestWalkableAreaForCharacter(Point(chap->x, chap->y), dst))
    {
        chap->x = dst.X;
        chap->y = dst.Y;
    }
}

void Character_RemoveTint(CharacterInfo *chaa) {

    if (chaa->flags & (CHF_HASTINT | CHF_HASLIGHT)) {
        debug_script_log("Un-tint %s", chaa->scrname.GetCStr());
        chaa->flags &= ~(CHF_HASTINT | CHF_HASLIGHT);
    }
    else {
        debug_script_warn("Character.RemoveTint called but character was not tinted");
    }
}

int Character_GetHasExplicitTint_Old(CharacterInfo *ch)
{
    return ch->has_explicit_tint() || ch->has_explicit_light();
}

int Character_GetHasExplicitTint(CharacterInfo *ch)
{
    return ch->has_explicit_tint();
}

void Character_Say(CharacterInfo *chaa, const char *text) {
    DisplaySpeechCore(chaa->index_id, text);
}

void Character_SayAt(CharacterInfo *chaa, int x, int y, int width, const char *texx) {

    DisplaySpeechAt(x, y, width, chaa->index_id, texx);
}

ScriptOverlay* Character_SayBackground(CharacterInfo *chaa, const char *texx) {

    int ovltype = DisplaySpeechBackground(chaa->index_id, texx);
    auto *over = get_overlay(ovltype);
    if (!over)
        quit("!SayBackground internal error: no overlay");
    // Create script object with an internal ref, keep at least until internal timeout
    ScriptOverlay *scover = over->CreateScriptObject();
    ccAddObjectReference(over->GetScriptHandle());
    return scover;
}

// [DEPRECATED] still used by Character_SetAsPlayer
void SetActiveInventory(int iit) {

    ScriptInvItem *tosend = nullptr;
    if ((iit > 0) && (iit < game.numinvitems))
        tosend = &scrInv[iit];
    else if (iit != -1)
        quitprintf("!SetActiveInventory: invalid inventory number %d", iit);

    Character_SetActiveInventory(playerchar, tosend);
}

// CLNUP check the use of SetActiveInventory
void Character_SetAsPlayer(CharacterInfo *chaa) {

    // Set to same character, so ignore.
    if ( game.playercharacter == chaa->index_id )
        return;

    setup_player_character(chaa->index_id);
    GUIE::MarkInventoryForUpdate(game.playercharacter, true);
    debug_script_log("%s is new player character", playerchar->scrname.GetCStr());

    // Within game_start, return now
    if (displayed_room < 0)
        return;

    if (displayed_room != playerchar->room)
        NewRoom(playerchar->room);
    else   // make sure it doesn't run the region interactions
        play.player_on_region = GetRegionIDAtRoom(playerchar->x, playerchar->y);

    if ((playerchar->activeinv >= 0) && (playerchar->inv[playerchar->activeinv] < 1))
        playerchar->activeinv = -1;

    // They had inv selected, so change the cursor
    if (is_current_cursor_mode(kCursorRole_UseInv)) {
        if (playerchar->activeinv < 0)
            Mouse_SetNextCursor ();
        else
            SetActiveInventory(playerchar->activeinv);
    }
}

void Character_SetIdleView(CharacterInfo *chaa, int iview, int itime) {

    if (iview == 1) 
        quit("!SetCharacterIdle: view 1 cannot be used as an idle view, sorry.");

    CharacterExtras *chex = &charextra[chaa->index_id];

    // if an idle anim is currently playing, release it
    stop_character_idling(chaa);

    chaa->idleview = iview - 1;
    // make sure they don't appear idle while idle anim is disabled
    if (iview < 1)
        itime = 10;
    chaa->idletime = itime;
    chaa->idleleft = itime;

    // if not currently animating, reset the wait counter
    if ((!chex->IsAnimating()) && (!chaa->is_moving()))
        chaa->wait = 0;

    if (iview >= 1) {
        debug_script_log("Set %s idle view to %d (time %d)", chaa->scrname.GetCStr(), iview, itime);
    }
    else {
        debug_script_log("%s idle view disabled", chaa->scrname.GetCStr());
    }
    if (chaa->flags & CHF_FIXVIEW) {
        debug_script_warn("SetCharacterIdle called while character view locked with SetCharacterView; idle ignored");
        debug_script_log("View locked, idle will not kick in until Released");
    }
    // if they switch to a swimming animation, kick it off immediately
    if (itime == 0)
        charextra[chaa->index_id].process_idle_this_time = 1;
}

bool Character_GetHasExplicitLight(CharacterInfo *ch)
{
    return ch->has_explicit_light();
}

int Character_GetLightLevel(CharacterInfo *ch)
{
    return ch->has_explicit_light() ? charextra[ch->index_id].tint_light : 0;
}

void Character_SetLightLevel(CharacterInfo *chaa, int light_level)
{
    light_level = Math::Clamp(light_level, -100, 100);

    charextra[chaa->index_id].tint_light = light_level;
    chaa->flags &= ~CHF_HASTINT;
    chaa->flags |= CHF_HASLIGHT;
}

int Character_GetTintRed(CharacterInfo *ch)
{
    return ch->has_explicit_tint() ? charextra[ch->index_id].tint_r : 0;
}

int Character_GetTintGreen(CharacterInfo *ch)
{
    return ch->has_explicit_tint() ? charextra[ch->index_id].tint_g : 0;
}

int Character_GetTintBlue(CharacterInfo *ch)
{
    return ch->has_explicit_tint() ? charextra[ch->index_id].tint_b : 0;
}

int Character_GetTintSaturation(CharacterInfo *ch)
{
    return ch->has_explicit_tint() ? charextra[ch->index_id].tint_level : 0;
}

int Character_GetTintLuminance(CharacterInfo *ch)
{
    return ch->has_explicit_tint() ? GfxDef::Value250ToValue100(charextra[ch->index_id].tint_light) : 0;
}

void Character_SetOption(CharacterInfo *chaa, int flag, int yesorno) {

    if ((yesorno < 0) || (yesorno > 1))
        quit("!SetCharacterProperty: last parameter must be 0 or 1");

    if (flag & CHF_MANUALSCALING) {
        Character_SetManualScaling(chaa, yesorno);
    }
    else {
        chaa->flags &= ~flag;
        if (yesorno)
            chaa->flags |= flag;
    }

}

void Character_SetSpeed(CharacterInfo *chaa, int xspeed, int yspeed) {

    if ((xspeed == 0) || (yspeed == 0))
        quit("!SetCharacterSpeedEx: invalid speed value");

    xspeed = Math::Clamp(xspeed, (int)INT16_MIN, (int)INT16_MAX);
    yspeed = Math::Clamp(yspeed, (int)INT16_MIN, (int)INT16_MAX);

    uint16_t old_speedx = chaa->walkspeed;
    uint16_t old_speedy = ((chaa->walkspeed_y == UNIFORM_WALK_SPEED) ? chaa->walkspeed : chaa->walkspeed_y);

    chaa->walkspeed = xspeed;
    if (yspeed == xspeed) 
        chaa->walkspeed_y = UNIFORM_WALK_SPEED;
    else
        chaa->walkspeed_y = yspeed;

    if (chaa->is_moving() && (old_speedx != xspeed || old_speedy != yspeed))
    {
        Pathfinding::RecalculateMoveSpeeds(*get_movelist(chaa->get_movelist_id()), old_speedx, old_speedy, xspeed, yspeed);
    }
}

void Character_StopMoving(CharacterInfo *chi)
{
    Character_StopMovingEx(chi, chi->is_moving() && (!get_movelist(chi->get_movelist_id())->IsStageDirect()));
}

void Character_StopMovingEx(CharacterInfo *chi, bool force_walkable_area)
{
    // If we have a movelist reference attached, then invalidate and dec refcount
    if (charextra[chi->index_id].movelist_handle > 0)
    {
        release_script_movelist(charextra[chi->index_id].movelist_handle);
        charextra[chi->index_id].movelist_handle = 0;
    }

    int chid = chi->index_id;
    if (chid == play.skip_until_char_stops)
        EndSkippingUntilCharStops();

    CharacterExtras &chex = charextra[chid];
    // Fixup position to the last saved one (CHECKME: find out what this means)
    if (chex.xwas != INVALID_X)
    {
        chi->x = chex.xwas;
        chi->y = chex.ywas;
        chex.xwas = INVALID_X;
    }

    // If requested by a caller, then validate the character position,
    // ensuring that it does not end on a non-walkable area and gets stuck.
    // Because the pathfinder is not ideal, and there are various sources of
    // inaccuracy in the walking logic, in rare cases the character might
    // pass through or even finish on a non-walkable pixel.
    if (force_walkable_area && (chi->room == displayed_room))
    {
        // TODO: don't use PlaceOnWalkable, as that may result in moving character
        // into the random direction.
        // For finishing a walk clamp to a precalculated final destination instead
        // (CHECKME: is it guaranteed valid?).
        // For when interrupting a move, consider writing a "backtracing"
        // function that runs along MoveList's current stage back, looking for
        // the first pixel on a walkable area.
        Character_PlaceOnWalkableArea(chi);
    }

    debug_script_log("%s: stop moving", chi->scrname.GetCStr());

    // If the character is currently moving, stop them and reset their state
    if (chi->walking > 0)
    {
        // Switch character state from walking to standing
    remove_movelist(chi->get_movelist_id());
        chi->walking = 0;
        // If the character was animating a walk, then reset their frame to standing
        if (chi->is_moving_walkanim())
            chi->frame = 0;
        // Restart idle timer
        reset_character_idling_time(chi);
    }

    // Always clear the move-related flags (for safety)
    // NOTE: I recall there was a potential case when this flag could remain after Move...
    chi->flags &= ~CHF_MOVENOTWALK;
}

void Character_Tint(CharacterInfo *chaa, int red, int green, int blue, int opacity, int luminance) {
    if ((red < 0) || (green < 0) || (blue < 0) ||
        (red > 255) || (green > 255) || (blue > 255) ||
        (opacity < 0) || (opacity > 100) ||
        (luminance < 0) || (luminance > 100))
    {
        debug_script_warn("Character.Tint: invalid parameter(s). R,G,B must be 0-255 (passed: %d,%d,%d), opacity & luminance 0-100 (passed: %d,%d)",
            red, green, blue, opacity, luminance);
        return;
    }

    debug_script_log("Set %s tint RGB(%d,%d,%d) %d%%", chaa->scrname.GetCStr(), red, green, blue, opacity);

    charextra[chaa->index_id].tint_r = red;
    charextra[chaa->index_id].tint_g = green;
    charextra[chaa->index_id].tint_b = blue;
    charextra[chaa->index_id].tint_level = opacity;
    charextra[chaa->index_id].tint_light = GfxDef::Value100ToValue250(luminance);
    chaa->flags &= ~CHF_HASLIGHT;
    chaa->flags |= CHF_HASTINT;
}

void Character_Think(CharacterInfo *chaa, const char *text) {
    DisplayThoughtCore(chaa->index_id, text);
}

void Character_UnlockView(CharacterInfo *chaa) {
    Character_UnlockViewEx(chaa, STOP_MOVING);
}

void Character_UnlockViewEx(CharacterInfo *chaa, int stopMoving) {
    if (chaa->flags & CHF_FIXVIEW) {
        debug_script_log("%s: Released view back to default", chaa->scrname.GetCStr());
    }
    charextra[chaa->index_id].SetUnlockedView(chaa);
    if (stopMoving != KEEP_MOVING)
    {
        Character_StopMoving(chaa);
    }
    if (chaa->view >= 0) {
        int maxloop = views[chaa->view].numLoops;
        if (((chaa->flags & CHF_NODIAGONAL)!=0) && (maxloop > 4))
            maxloop = 4;
        FindReasonableLoopForCharacter(chaa);
    }
    stop_character_anim(chaa);
    // Restart idle timer
    reset_character_idling_time(chaa);
}

// Tests if the given character is permitted to start a move in the room
bool ValidateCharForMove(CharacterInfo *chaa, const char *api_name)
{
    if (chaa->room != displayed_room)
    {
        debug_script_warn("%s: specified character %s not in current room (is in %d, current room %d)",
            api_name, chaa->scrname.GetCStr(), chaa->room, displayed_room);
        return false;
    }
    if (!chaa->is_enabled())
    {
        debug_script_warn("%s: character %s is turned off and cannot be moved", api_name, chaa->scrname.GetCStr());
        return false;
    }
    return true;
}

// Character_DoMove converts and validates script parameters, and calls corresponding internal character move function
static void Character_DoMove(CharacterInfo *chaa, const char *api_name,
    void *path_arr, int x, int y, bool use_path, bool walk_straight,
    int blocking, int ignwal, bool walk_anim, int repeat = kAnimFlow_Once, int direction = FORWARDS)
{
    if (!ValidateCharForMove(chaa, api_name))
        return;

    ValidateMoveParams(api_name, chaa->scrname.GetCStr(), blocking, ignwal);
    ValidateAnimParams(api_name, chaa->scrname.GetCStr(), repeat, direction);

    if (use_path)
    {
        std::vector<Point> path;
        if (!path_arr || !ScriptStructHelpers::ResolveArrayOfPoints(path_arr, path))
        {
            debug_script_warn("%s: path is null, or failed to resolve array of points", api_name);
            Character_StopMoving(chaa);
            return;
        }
        move_character(chaa, path, walk_anim,
            RunPathParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction)));
    }
    else if (walk_straight)
    {
        move_character_straight(chaa, x, y, walk_anim);
    }
    else
    {
        move_character(chaa, x, y, ignwal != 0, walk_anim);
    }

    if (blocking)
        GameLoopUntilNotMoving(&chaa->walking);
}

void Character_Walk(CharacterInfo *chaa, int x, int y, int blocking, int ignwal) 
{
    Character_DoMove(chaa, "Character.Walk", nullptr, x, y, false /* no path */, false /* not straight */, blocking, ignwal, true /* walk anim */);
}

void Character_Move(CharacterInfo *chaa, int x, int y, int blocking, int ignwal) 
{
    Character_DoMove(chaa, "Character.Move", nullptr, x, y, false /* no path */, false /* not straight */, blocking, ignwal, false /* no anim */);
}

void Character_WalkStraight(CharacterInfo *chaa, int xx, int yy, int blocking)
{
    Character_DoMove(chaa, "Character.WalkStraight", nullptr, xx, yy, false /* no path */, true /* straight */, blocking, WALKABLE_AREAS, true /* walk anim */);
}

void Character_MoveStraight(CharacterInfo *chaa, int xx, int yy, int blocking)
{
    Character_DoMove(chaa, "Character.MoveStraight", nullptr, xx, yy, false /* no path */, true /* straight */, blocking, WALKABLE_AREAS, false /* no anim */);
}

void Character_WalkPath(CharacterInfo *chaa, void *path_arr, int blocking, int repeat, int direction)
{
    Character_DoMove(chaa, "Character.WalkPath", path_arr, 0, 0, true /* use path */, false /* not straight */, blocking, ANYWHERE, true /* walk anim */, repeat, direction);
}

void Character_MovePath(CharacterInfo *chaa, void *path_arr, int blocking, int repeat, int direction)
{
    Character_DoMove(chaa, "Character.WalkPath", path_arr, 0, 0, true /* use path */, false /* not straight */, blocking, ANYWHERE, false /* no anim */, repeat, direction);
}

bool Character_RunFrameEvent(CharacterInfo *chaa, int view, int loop, int frame)
{
    view--; // convert to internal 0-based view ID
    AssertFrame("Character.RunFrameEvent", chaa->scrname.GetCStr(), view, loop, frame);
    return charextra[chaa->index_id].RunFrameEvent(&game.chars[chaa->index_id], view, loop, frame);
}

void Character_RunInteraction(CharacterInfo *chaa, int mood)
{
    if (!AssertCursorValidForEvent("Character.RunInteraction", mood))
        return;

    // Cursor mode must match the event index in "Interactions" table,
    // except when it does not have a "event" flag
    // TODO: do we need extra flag telling if to trigger "any click" for this mode?
    const int evnt = (game.mcurs[mood].flags & MCF_EVENT) != 0 ? mood : -1;
    const int anyclick_evt = kCharacterEvent_AnyClick;

    // For USE verb: remember active inventory
    if (game.HasCursorRole(mood, kCursorRole_UseInv))
    {
        play.usedinv = playerchar->activeinv;
    }

    const auto obj_evt = ObjectEvent(kScTypeGame, LOCTYPE_CHAR, chaa->index_id,
                                     RuntimeScriptValue().SetScriptObject(chaa, &ccDynamicCharacter), mood);
    if ((evnt >= 0) &&
        run_event_script(obj_evt, &game.chars[chaa->index_id].interactions, evnt,
                         &game.chars[chaa->index_id].GetEvents(), anyclick_evt, true /* do unhandled event */) < 0)
        return; // game state changed, don't do "any click"
    // any click on char
    run_event_script(obj_evt, &game.chars[chaa->index_id].GetEvents(), anyclick_evt);
}



// **** CHARACTER: PROPERTIES ****

int Character_GetProperty(CharacterInfo *chaa, const char *property)
{
    if (!AssertCharacter("Character.GetProperty", chaa->index_id))
        return 0;
    return get_int_property(game.charProps[chaa->index_id], play.charProps[chaa->index_id], property);
}

void Character_GetPropertyText(CharacterInfo *chaa, const char *property, char *bufer)
{
    if (!AssertCharacter("Character.GetPropertyText", chaa->index_id))
        return;
    get_text_property(game.charProps[chaa->index_id], play.charProps[chaa->index_id], property, bufer);
}

const char* Character_GetTextProperty(CharacterInfo *chaa, const char *property)
{
    if (!AssertCharacter("Character.GetTextProperty", chaa->index_id))
        return nullptr;
    return get_text_property_dynamic_string(game.charProps[chaa->index_id], play.charProps[chaa->index_id], property);
}

bool Character_SetProperty(CharacterInfo *chaa, const char *property, int value)
{
    if (!AssertCharacter("Character.SetProperty", chaa->index_id))
        return false;
    return set_int_property(play.charProps[chaa->index_id], property, value);
}

bool Character_SetTextProperty(CharacterInfo *chaa, const char *property, const char *value)
{
    if (!AssertCharacter("Character.SetTextProperty", chaa->index_id))
        return false;
    return set_text_property(play.charProps[chaa->index_id], property, value);
}

ScriptInvItem* Character_GetActiveInventory(CharacterInfo *chaa) {

    if (chaa->activeinv <= 0)
        return nullptr;

    return &scrInv[chaa->activeinv];
}

void Character_SetActiveInventory(CharacterInfo *chaa, ScriptInvItem* iit) {
    if (iit == nullptr) {
        chaa->activeinv = -1;

        if (chaa->index_id == game.playercharacter) {

            if (is_current_cursor_mode(kCursorRole_UseInv))
                set_cursor_mode(0); // change to the first enabled mode
        }
        GUIE::MarkInventoryForUpdate(chaa->index_id, chaa->index_id == game.playercharacter);
        return;
    }

    if (chaa->inv[iit->id] < 1)
    {
        debug_script_warn("SetActiveInventory: character doesn't have any of that inventory");
        return;
    }

    chaa->activeinv = iit->id;

    if (chaa->index_id == game.playercharacter) {
        // if it's the player character, update mouse cursor
        update_inv_cursor(iit->id);
        set_cursor_mode_with_role(kCursorRole_UseInv, -1);
    }
    GUIE::MarkInventoryForUpdate(chaa->index_id, chaa->index_id == game.playercharacter);
}

int Character_GetAnimating(CharacterInfo *chaa) {
    return charextra[chaa->index_id].IsAnimating() ? 1 : 0;
}

int Character_GetAnimationSpeed(CharacterInfo *chaa) {
    return chaa->animspeed;
}

void Character_SetAnimationSpeed(CharacterInfo *chaa, int newval) {

    chaa->animspeed = newval;
}

int Character_GetAnimationVolume(CharacterInfo *chaa) {
    return charextra[chaa->index_id].anim_volume;
}

void Character_SetAnimationVolume(CharacterInfo *chaa, int newval) {

    charextra[chaa->index_id].anim_volume = Math::Clamp(newval, 0, 100);
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

int Character_GetBlockingHeight(CharacterInfo *chaa)
{
    return chaa->blocking_height;
}

void Character_SetBlockingHeight(CharacterInfo *chaa, int hit) {

    chaa->blocking_height = static_cast<int16_t>(hit);
}

int Character_GetBlockingWidth(CharacterInfo *chaa)
{
    return chaa->blocking_width;
}

void Character_SetBlockingWidth(CharacterInfo *chaa, int wid)
{
    chaa->blocking_width = static_cast<int16_t>(wid);
}

int Character_GetBlockingRectX(CharacterInfo *chaa)
{
    return game.chars[chaa->index_id].blocking_x;
}

void Character_SetBlockingRectX(CharacterInfo *chaa, int x)
{
    game.chars[chaa->index_id].blocking_x = x;
}

int Character_GetBlockingRectY(CharacterInfo *chaa)
{
    return game.chars[chaa->index_id].blocking_y;
}

void Character_SetBlockingRectY(CharacterInfo *chaa, int y)
{
    game.chars[chaa->index_id].blocking_y = y;
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

const char *Character_GetScriptName(CharacterInfo *chin)
{
    return CreateNewScriptString(game.chars[chin->index_id].scrname);
}

bool Character_GetEnabled(CharacterInfo *chaa) {
    return chaa->is_enabled();
}

void Character_SetEnabled(CharacterInfo *chaa, bool newval) {
    chaa->set_enabled(newval);
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
    if (invi == nullptr)
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

// [DEPRECATED]
int Character_GetIgnoreLighting(CharacterInfo *chaa) {

    if (chaa->flags & CHF_NOLIGHTING)
        return 1;
    return 0;
}

// [DEPRECATED]
void Character_SetIgnoreLighting(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_NOLIGHTING;
    if (yesorno)
        chaa->flags |= CHF_NOLIGHTING;
}

int Character_GetManualScaling(CharacterInfo *chaa) {

    return (chaa->flags & CHF_MANUALSCALING) ? 1 : 0;
}

void Character_SetManualScaling(CharacterInfo *chaa, int yesorno) {

    chaa->flags &= ~CHF_MANUALSCALING;
    if (yesorno)
        chaa->flags |= CHF_MANUALSCALING;
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
    AssertLoop("Character.Loop", chaa->scrname.GetCStr(), chaa->view, newval);

    chaa->loop = newval;
    if (chaa->frame >= views[chaa->view].loops[chaa->loop].numFrames)
        chaa->frame = 0;
}

int Character_GetMoving(CharacterInfo *chaa) {
    if (chaa->walking)
        return 1;
    return 0;
}

int Character_GetDestinationX(CharacterInfo *chaa)
{
    if (chaa->get_movelist_id() > 0)
    {
        MoveList *cmls = get_movelist(chaa->get_movelist_id());
        return cmls->GetLastPos().X;
    }
    else
    {
        return chaa->x;
    }
}

int Character_GetDestinationY(CharacterInfo *chaa)
{
    if (chaa->get_movelist_id() > 0)
    {
        MoveList *cmls = get_movelist(chaa->get_movelist_id());
        return cmls->GetLastPos().Y;
    }
    else
    {
        return chaa->y;
    }
}

const char* Character_GetName(CharacterInfo *chaa) {
    return CreateNewScriptString(game.chars[chaa->index_id].name.GetCStr());
}

void Character_SetName(CharacterInfo *chaa, const char *newName) {
    chaa->name = newName;
    // Fill legacy name fields, for compatibility with old scripts and plugins
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
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
    {
        debug_script_warn("Character.Scaling: cannot set property unless ManualScaling is enabled");
        return;
    }
    int zoom_fixed = Math::Clamp(zoomlevel, 1, (int)(INT16_MAX)); // CharacterExtras.zoom is int16
    if (zoomlevel != zoom_fixed)
        debug_script_warn("Character.Scaling: scaling level must be between 1 and %d%%, asked for: %d",
                        (int)(INT16_MAX), zoomlevel);

    charextra[chaa->index_id].zoom = zoom_fixed;
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

int Character_GetSpeechAnimationDelay(CharacterInfo *chaa)
{
    if (game.options[OPT_GLOBALTALKANIMSPD] != 0)
        return play.talkanim_speed;
    else
        return chaa->speech_anim_speed;
}

void Character_SetSpeechAnimationDelay(CharacterInfo *chaa, int newDelay)
{
    if (game.options[OPT_GLOBALTALKANIMSPD] != 0)
    {
        debug_script_warn("Character.SpeechAnimationDelay cannot be set when global speech animation speed is enabled");
        return;
    }

    chaa->speech_anim_speed = newDelay;
}

int Character_GetIdleAnimationDelay(CharacterInfo *chaa)
{
    return chaa->idle_anim_speed;
}

void Character_SetIdleAnimationDelay(CharacterInfo *chaa, int newDelay)
{
    chaa->idle_anim_speed = newDelay;
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

bool Character_GetThinking(CharacterInfo *chaa)
{
    return char_thinking == chaa->index_id;
}

int Character_GetThinkingFrame(CharacterInfo *chaa)
{
    if (char_thinking == chaa->index_id)
        return chaa->thinkview > 0 ? chaa->frame : -1;

    debug_script_warn("Character.ThinkingFrame: character is not currently thinking");
    return -1;
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

    return GfxDef::LegacyTrans255ToTrans100(chaa->transparency);
}

void Character_SetTransparency(CharacterInfo *chaa, int trans) {

    if ((trans < 0) || (trans > 100))
        quit("!SetCharTransparent: transparency value must be between 0 and 100");

    chaa->transparency = GfxDef::Trans100ToLegacyTrans255(trans);
}

bool Character_GetVisible(CharacterInfo *chaa) {
    return chaa->is_visible();
}

void Character_SetVisible(CharacterInfo *chaa, bool newval) {
    chaa->set_visible(newval);
}

int Character_GetBlendMode(CharacterInfo *chaa) {
    return charextra[chaa->index_id].blend_mode;
}

void Character_SetBlendMode(CharacterInfo *chaa, int blend_mode) {
    charextra[chaa->index_id].blend_mode = ValidateBlendMode("Character.BlendMode", chaa->scrname.GetCStr(), blend_mode);
}

float Character_GetGraphicAnchorX(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].spr_anchor.X;
}

void Character_SetGraphicAnchorX(CharacterInfo *chaa, float x)
{
    charextra[chaa->index_id].spr_anchor.X = Math::Clamp(x, 0.f, 1.f);
    charextra[chaa->index_id].UpdateEffectiveValues(chaa);
}

float Character_GetGraphicAnchorY(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].spr_anchor.Y;
}

void Character_SetGraphicAnchorY(CharacterInfo *chaa, float y)
{
    charextra[chaa->index_id].spr_anchor.Y = Math::Clamp(y, 0.f, 1.f);
    charextra[chaa->index_id].UpdateEffectiveValues(chaa);
}

int Character_GetGraphicOffsetX(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].spr_offset.X;
}

void Character_SetGraphicOffsetX(CharacterInfo *chaa, int x)
{
    charextra[chaa->index_id].spr_offset.X = x;
    charextra[chaa->index_id].UpdateEffectiveValues(chaa);
}

int Character_GetGraphicOffsetY(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].spr_offset.Y;
}

void Character_SetGraphicOffsetY(CharacterInfo *chaa, int y)
{
    charextra[chaa->index_id].spr_offset.Y = y;
    charextra[chaa->index_id].UpdateEffectiveValues(chaa);
}

ScriptShaderInstance *Character_GetShader(CharacterInfo *chaa)
{
    return static_cast<ScriptShaderInstance *>(ccGetObjectAddressFromHandle(charextra[chaa->index_id].shader_handle));
}

void Character_SetShader(CharacterInfo *chaa, ScriptShaderInstance *shader_inst)
{
    auto &chex = charextra[chaa->index_id];
    chex.shader_id = shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID;
    chex.shader_handle = ccReplaceObjectHandle(chex.shader_handle, shader_inst);
}

float Character_GetRotation(CharacterInfo *chaa) {
    return charextra[chaa->index_id].rotation;
}

void Character_SetRotation(CharacterInfo *chaa, float degrees) {
    charextra[chaa->index_id].rotation = Math::ClampAngle360(degrees);
    charextra[chaa->index_id].UpdateGraphicSpace(chaa);
}

float Character_GetFaceDirectionRatio(CharacterInfo *chaa) {
    return charextra[chaa->index_id].face_dir_ratio;
}

void Character_SetFaceDirectionRatio(CharacterInfo *chaa, float ratio) {
    charextra[chaa->index_id].face_dir_ratio = ratio;
}

int Character_GetTurnBeforeWalking(CharacterInfo *chaa) {
    // NOTE: this flag has inverse meaning
    return ((chaa->flags & CHF_NOTURNWHENWALK) != 0) ? 0 : 1;
}

void Character_SetTurnBeforeWalking(CharacterInfo *chaa, int on) {
    // NOTE: this flag has inverse meaning
    if (on)
        chaa->flags &= ~CHF_NOTURNWHENWALK;
    else
        chaa->flags |= CHF_NOTURNWHENWALK;
}

int Character_GetTurnWhenFacing(CharacterInfo *chaa) {
    return ((chaa->flags & CHF_TURNWHENFACE) != 0) ? 1 : 0;
}

void Character_SetTurnWhenFacing(CharacterInfo *chaa, int on) {
    if (on)
        chaa->flags |= CHF_TURNWHENFACE;
    else
        chaa->flags &= ~CHF_TURNWHENFACE;
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
    charextra[chaa->index_id].UpdateGraphicSpace(chaa);
}

int Character_GetY(CharacterInfo *chaa) {
    return chaa->y;
}

void Character_SetY(CharacterInfo *chaa, int newval) {
    chaa->y = newval;
    charextra[chaa->index_id].UpdateGraphicSpace(chaa);
}

int Character_GetZ(CharacterInfo *chaa) {
    return chaa->z;
}

void Character_SetZ(CharacterInfo *chaa, int newval) {
    chaa->z = newval;
    charextra[chaa->index_id].UpdateGraphicSpace(chaa);
}

int Character_GetSpeakingFrame(CharacterInfo *chaa) {

    if ((face_talking >= 0) && (facetalkrepeat))
    {
        if (facetalkchar->index_id == chaa->index_id)
        {
            return facetalkframe;
        }
    }
    else if (char_speaking_anim >= 0)
    {
        if (char_speaking_anim == chaa->index_id)
        {
            return chaa->frame;
        }
    }

    debug_script_warn("Character.SpeakingFrame: character is not playing a speech animation");
    return -1;
}

bool Character_GetUseRegionTint(CharacterInfo *chaa)
{
    return (chaa->flags & CHF_NOLIGHTING) == 0;
}

void Character_SetUseRegionTint(CharacterInfo *chaa, int yesorno)
{
    chaa->flags &= ~CHF_NOLIGHTING;
    if (!yesorno)
        chaa->flags |= CHF_NOLIGHTING;
}

float Character_GetViewAnchorX(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].GetEffectiveGraphicAnchor().X;
}

float Character_GetViewAnchorY(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].GetEffectiveGraphicAnchor().Y;
}

int Character_GetViewOffsetX(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].GetEffectiveGraphicOffset().X;
}

int Character_GetViewOffsetY(CharacterInfo *chaa)
{
    return charextra[chaa->index_id].GetEffectiveGraphicOffset().Y;
}

ScriptMotionPath *Character_GetMotionPath(CharacterInfo *ch)
{
    const int mslot = ch->get_movelist_id();
    if (mslot <= 0)
        return nullptr;

    CharacterExtras &chex = charextra[ch->index_id];
    if (chex.movelist_handle > 0)
    {
        return static_cast<ScriptMotionPath *>(ccGetObjectAddressFromHandle(chex.movelist_handle));
    }

    auto sc_path = ScriptMotionPath::Create(mslot);
    ccAddObjectReference(sc_path.Handle);
    chex.movelist_handle = sc_path.Handle;
    return static_cast<ScriptMotionPath *>(sc_path.Obj);
}

//=============================================================================

// order of loops to turn character in circle from down to down
const int turnlooporder[8] = {0, 6, 1, 7, 3, 5, 2, 4};

// Core character move implementation:
// uses a provided path or searches for a path to a given destination;
// starts a move or walk (with automatic animation).
void move_character_impl(CharacterInfo *chin, const std::vector<Point> *path, int tox, int toy, bool ignwal, bool walk_anim,
    const RunPathParams &run_params)
{
    assert(run_params.Flow >= kAnimFlow_First && run_params.Flow <= kAnimFlow_Last);
    assert(run_params.Direction >= kAnimDirForward && run_params.Direction <= kAnimDirBackward);
    const int chac = chin->index_id;
    if (!ValidateCharForMove(chin, "MoveCharacter"))
        return;

    // Stop custom animation always (but idling will be stopped only if pathfinding was a success, below)
    CharacterExtras *chex = &charextra[chin->index_id];
    if (chex->IsAnimating() && walk_anim)
        stop_character_anim(chin);

    // If using a path, then update starting and destination parameters,
    // and jump the character to the path's start
    if (path)
    {
        chin->x = run_params.IsForward() ? path->front().X : path->back().X;
        chin->y = run_params.IsForward() ? path->front().Y : path->back().Y;
        tox = run_params.IsForward() ? path->back().X : path->front().X;
        toy = run_params.IsForward() ? path->back().Y : path->front().Y;
    }

    // If has path, then test if it's empty, or has 2 stages where start is identical to end;
    // If no path, then test if destination is identical to the current pos
    if (path && ((path->size() < 2) || (path->size() == 2) && ((*path)[0] == (*path)[1])) ||
        !path && (tox == chin->x) && (toy == chin->y))
    {
        Character_StopMoving(chin);
        debug_script_log("MoveCharacter: %s move path is empty, or is already at destination, not moving", chin->scrname.GetCStr());
        return;
    }

    // By default wait 1 frame before starting to move, to let animation advance once
    int walk_wait_init = 1;
    int anim_wait_init = 0;
    float step_frac_init = 0.f;
    int oldframe = chin->frame;
    // If character was currently walking, then save current "wait" timers
    // and animation frame, and apply later after new walking begins,
    // in order to make it look smoother.
    if (chin->walking)
    {
        walk_wait_init = chin->walkwait;
        anim_wait_init = charextra[chac].animwait;
        const auto &movelist = *get_movelist(chin->get_movelist_id());
        if (movelist.GetStageProgress() > 0.f)
            step_frac_init = movelist.GetPixelUnitFraction();
    }

    Character_StopMoving(chin);
    chin->frame = oldframe;
    debug_script_log("%s: Start move to %d,%d", chin->scrname.GetCStr(), tox, toy);

    int move_speed_x, move_speed_y;
    chin->get_effective_walkspeeds(move_speed_x, move_speed_y);
    if ((move_speed_x == 0) && (move_speed_y == 0))
    {
        debug_script_warn("MoveCharacter: called for '%s' with walk speed 0", chin->scrname.GetCStr());
    }

    MoveList new_mlist;
    bool path_result = false;
    if (path)
    {
        path_result = Pathfinding::CalculateMoveList(new_mlist, *path, move_speed_x, move_speed_y,
            ignwal ? kMoveStage_Direct : 0, run_params);
    }
    else
    {
        MaskRouteFinder *pathfind = get_room_pathfinder();
        pathfind->SetWalkableArea(prepare_walkable_areas(chac), thisroom.MaskResolution);
        path_result = Pathfinding::FindRoute(new_mlist, pathfind, chin->x, chin->y, tox, toy,
            move_speed_x, move_speed_y, false, ignwal, run_params);
    }

    // If path available, then add movelist
    const int movelist = path_result ? add_movelist(std::move(new_mlist)) : 0;
    assert(!path_result || movelist > 0);

    // If successful, then start moving
    if (movelist > 0)
    {
        // Stop idling state (if character was in one)
        stop_character_idling(chin);

        // Setup new walk state
        chin->walking = movelist;
        MoveList &mlist = *get_movelist(chin->walking);
        // NOTE: unfortunately, some old game scripts might break because of smooth walk transition
        if (step_frac_init > 0.f && (loaded_game_file_version >= kGameVersion_361))
        {
            mlist.SetPixelUnitFraction(step_frac_init);
        }

        // Cancel any pending waits on current animations
        // or if they were already moving, keep the current wait - 
        // this prevents a glitch if MoveCharacter is called when they
        // are already moving
        if (walk_anim)
        {
            chin->walkwait = walk_wait_init;
            charextra[chac].animwait = anim_wait_init;
            fix_player_sprite(chin, mlist);
        }
        else
        {
            chin->flags |= CHF_MOVENOTWALK;
        }
    }
    else
    {
        // Pathfinder couldn't get a route, stand them still
        if (walk_anim && !chin->is_idling())
            chin->frame = 0;
    }
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

void start_character_turning(CharacterInfo *chinf, int useloop, int no_diagonal)
{
    CharacterExtras &chex = charextra[chinf->index_id];
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
    if (go_anticlock == 0)
        go_anticlock = -1;

    // Allow the diagonal frames just for turning
    if (no_diagonal == 2)
        no_diagonal = 0;

    int turns = 0;
    for (ii = fromidx; ii != toidx; ii -= go_anticlock)
    {
        // Wrap the loop order into range [0-7]
        if (ii < 0)
            ii = 7;
        if (ii >= 8)
            ii = 0;
        if (ii == toidx)
            break;
        if ((turnlooporder[ii] >= 4) && (no_diagonal > 0))
            continue; // there are no diagonal loops
        if (turnlooporder[ii] >= views[chinf->view].numLoops)
            continue; // no such loop
        if (views[chinf->view].loops[turnlooporder[ii]].numFrames < 1)
            continue; // no frames in such loop
        turns++;
    }

    if (turns > 0)
    {
        chex.SetTurning(true, go_anticlock > 0, turns);
    }
}

void fix_player_sprite(CharacterInfo *chinf, const MoveList &cmls) {
    const float xpmove = cmls.GetCurrentSpeed().X;
    const float ypmove = cmls.GetCurrentSpeed().Y;

    // if not moving, do nothing
    if ((xpmove == 0.f) && (ypmove == 0.f))
        return;

    const int useloop = GetDirectionalLoop(chinf, xpmove, ypmove, cmls.GetRunParams().IsForward());

    if ((game.options[OPT_CHARTURNWHENWALK] == 0) || ((chinf->flags & CHF_NOTURNWHENWALK) != 0)) {
        chinf->loop = useloop;
        return;
    }
    if ((chinf->loop > kDirLoop_LastOrthogonal) && ((chinf->flags & CHF_NODIAGONAL)!=0)) {
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
    const int no_diagonal = useDiagonal (chinf);
    start_character_turning (chinf, useloop, no_diagonal);
}

// Check whether two characters have walked into each other
int has_hit_another_character(int sourceChar) {

    // if the character who's moving doesn't block, don't bother checking
    if (game.chars[sourceChar].flags & CHF_NOBLOCKING)
        return -1;

    for (int ww = 0; ww < game.numcharacters; ww++) {
        if (!game.chars[ww].is_enabled()) continue;
        if (game.chars[ww].room != displayed_room) continue;
        if (ww == sourceChar) continue;
        if (game.chars[ww].flags & CHF_NOBLOCKING) continue;

        if (get_char_blocking_rect(ww).IsInside(game.chars[sourceChar].x, game.chars[sourceChar].y))
        {
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
int doNextCharMoveStep(CharacterInfo *chi, CharacterExtras *chex)
{
    assert(chi->walking > 0);
    if (chi->walking <= 0)
        return 0;

    int ntf=0, xwas = chi->x, ywas = chi->y;
    MoveList &mlist = *get_movelist(chi->get_movelist_id());

    if (do_movelist_move(chi->walking, chi->x, chi->y) == 2) 
    {
        if (chi->is_moving_walkanim())
            fix_player_sprite(chi, mlist);
    }

    ntf = has_hit_another_character(chi->index_id);
    if (ntf >= 0)
    {
        chi->walkwait = 30;
        if (game.chars[ntf].walkspeed < 5)
            chi->walkwait += (5 - game.chars[ntf].walkspeed) * 5;
        // we are now waiting for the other char to move, so
        // make sure he doesn't stop for us too

        chi->flags |= CHF_AWAITINGMOVE;

        if (chi->is_moving_walkanim())
        {
            chi->frame = 0;
            chex->animwait = chi->walkwait;
        }

        if ((chi->walking < 1) || (chex->turns > 0)) { /* skip */ }
        else if (mlist.GetStageProgress() > 0.f)
        {
            mlist.Backward();
            chi->x = xwas;
            chi->y = ywas;
        }

        debug_script_log("%s: Bumped into %s, waiting for them to move",
            chi->scrname.GetCStr(), game.chars[ntf].scrname.GetCStr());
        return 1;
    }
    return 0;
}

bool is_char_walking_ndirect(CharacterInfo *chi)
{
    return chi->is_moving() &&
        (!get_movelist(chi->get_movelist_id())->IsStageDirect());
}

bool FindNearestWalkableAreaForCharacter(const Point &src, Point &dst)
{
    const Point at_pt = Point(room_to_mask_coord(src.X), room_to_mask_coord(src.Y));
    const Bitmap *mask = thisroom.WalkAreaMask.get();

    int pix_value = 0;
    if (mask->IsOnBitmap(at_pt.X, at_pt.Y))
    {
        pix_value = thisroom.WalkAreaMask->GetPixel(at_pt.X, at_pt.Y);
    }

    // Is already on a walkable area?
    if (pix_value > 0)
    {
        dst = src;
        return true;
    }

    Rect limits = RectWH(mask->GetSize());
    // Adjust scan area to match historical behavior of Character.PlaceOnWalkableArea()
    // Characters that are within Room Edges cannot be moved outside of them
    int right_edge = room_to_mask_coord(thisroom.Edges.Right);
    int left_edge = room_to_mask_coord(thisroom.Edges.Left);
    int top_edge = room_to_mask_coord(thisroom.Edges.Top);
    int bottom_edge = room_to_mask_coord(thisroom.Edges.Bottom);

    // Original comment sais that edges are ignored if the character is outside,
    // because "people forget to move the edges sometimes".
    if (at_pt.X <= left_edge) left_edge = 0;
    if (at_pt.X >= right_edge) right_edge = limits.Right;
    if (at_pt.Y <= top_edge) top_edge = 0;
    if (at_pt.Y >= bottom_edge) bottom_edge = limits.Bottom;

    // Ancient hack, where the scan area is restricted by 14 topmost pixels;
    // this is *likely* the historical size of a Sierra-style Statusbar GUI.
    if (loaded_game_file_version < kGameVersion_362)
    {
        limits.Top = std::max(limits.Top, 14);
    }

    if (!Pathfinding::FindNearestWalkablePoint(thisroom.WalkAreaMask.get(), at_pt, dst, limits, 0, 1))
        return false;
    dst = Point(mask_to_room_coord(dst.X), mask_to_room_coord(dst.Y));
    return true;
}

void FindReasonableLoopForCharacter(CharacterInfo *chap) {

    if (chap->loop >= views[chap->view].numLoops)
        chap->loop=kDirLoop_Default;
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

void move_character(CharacterInfo *chaa, int tox, int toy, bool ignwal, bool walk_anim)
{
    move_character_impl(chaa, nullptr, tox, toy, ignwal, walk_anim, RunPathParams::DefaultOnce());
}

void move_character(CharacterInfo *chaa, const std::vector<Point> &path, bool walk_anim, const RunPathParams &run_params)
{
    move_character_impl(chaa, &path, 0, 0, true /* ignore walls */, walk_anim, run_params);
}

void move_character_straight(CharacterInfo *chaa, int x, int y, bool walk_anim)
{
    MaskRouteFinder *pathfind = get_room_pathfinder();
    pathfind->SetWalkableArea(prepare_walkable_areas(chaa->index_id), thisroom.MaskResolution);
    if (!pathfind->IsWalkableAt(chaa->x, chaa->y))
    {
        Character_StopMoving(chaa);
        debug_script_log("MoveCharacterStraight: %s (%d,%d) is not on a walkable area, not moving", chaa->scrname.GetCStr(), chaa->x, chaa->y);
        return;
    }

    int movetox = x, movetoy = y;
    int lastcx = chaa->x, lastcy = chaa->y;
    if (!pathfind->CanSeeFrom(chaa->x, chaa->y, x, y, &lastcx, &lastcy))
    {
        movetox = lastcx;
        movetoy = lastcy;
    }

    std::vector<Point> path = { {chaa->x, chaa->y}, {movetox, movetoy} };
    move_character_impl(chaa, &path, movetox, movetoy, false /* walkable areas */, walk_anim, RunPathParams::DefaultOnce());
}

void walk_character(CharacterInfo *chaa, int tox, int toy, bool ignwal)
{
    move_character(chaa, tox, toy, ignwal, true /* animate */);
}

void walk_character_straight(CharacterInfo *chaa, int x, int y)
{
    move_character_straight(chaa, x, y, true /* animate */);
}

void MoveCharacterToHotspot(int chaa, int hotsp)
{
    if ((hotsp < 0) || (hotsp >= MAX_ROOM_HOTSPOTS))
        quit("!MovecharacterToHotspot: invalid hotspot");
    if (thisroom.Hotspots[hotsp].WalkTo.X < 1)
        return;

    walk_character(&game.chars[chaa], thisroom.Hotspots[hotsp].WalkTo.X, thisroom.Hotspots[hotsp].WalkTo.Y, false /* walkable areas */);
    GameLoopUntilNotMoving(&game.chars[chaa].walking);
}

int wantMoveNow (CharacterInfo *chi, CharacterExtras *chex) {
    // check most likely case first
    if ((chex->zoom == 100) || ((chi->flags & CHF_SCALEMOVESPEED) == 0))
        return 1;

    // the % checks don't work when the counter is negative, so once
    // it wraps round, correct it
    // FIXME: can this be solved by making walkwaitcounter unsigned instead??
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
            return -1;
        else if (chex->xwas != INVALID_X) {
            // move the second half of the movement to make it smoother
            chi->x = chex->xwas;
            chi->y = chex->ywas;
            chex->xwas = INVALID_X;
        }
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
    _sc_PlayerCharPtr = ccGetObjectHandleFromAddress(playerchar);
}

// Animate character internal implementation;
// this function may be called by the game logic too, so we assume
// the arguments must be correct, and do not fix them up as we do for API functions.
void animate_character(CharacterInfo *chap, int loopn, int sppd, int rept,
    int direction, int sframe, int volume)
{
    if ((chap->view < 0) || (chap->view > game.numviews) ||
        (loopn < 0) || (loopn >= views[chap->view].numLoops))
    {
        quitprintf("!AnimateCharacter: invalid view and/or loop\n"
            "(trying to animate '%s' using view %d (range is 1..%d) and loop %d (view has %d loops)).",
            chap->scrname.GetCStr(), chap->view + 1, game.numviews, loopn, views[chap->view].numLoops);
    }
    // NOTE: there's always frame 0 allocated for safety
    sframe = std::max(0, std::min(sframe, views[chap->view].loops[loopn].numFrames - 1));
    debug_script_log("%s: Start anim view %d loop %d, spd %d, repeat %d, frame: %d",
        chap->scrname.GetCStr(), chap->view+1, loopn, sppd, rept, sframe);

    Character_StopMoving(chap);

    CharacterExtras *chex = &charextra[chap->index_id];
    chex->SetAnimating(static_cast<AnimFlowStyle>(rept), static_cast<AnimFlowDirection>(direction), sppd, Math::Clamp(volume, 0, 100));
    chap->loop = loopn;
    chap->frame = SetFirstAnimFrame(chap->view, loopn, sframe, static_cast<AnimFlowDirection>(direction));
    chap->wait = sppd + views[chap->view].loops[loopn].frames[chap->frame].speed;

    charextra[chap->index_id].CheckViewFrame(chap);
}

void stop_character_anim(CharacterInfo *chap)
{
    charextra[chap->index_id].ResetAnimating();
}

Bitmap *GetCharacterImage(int charid, bool *is_original)
{
    // NOTE: the cached image will only be present in software render mode
    Bitmap *actsp = get_cached_character_image(charid);
    if (is_original)
        *is_original = !actsp; // no cached means we use original sprite
    if (actsp)
        return actsp;

    CharacterInfo*chin=&game.chars[charid];
    int sppic = views[chin->view].loops[chin->loop].frames[chin->frame].pic;
    return spriteset[sppic];
}

Bitmap *GetCharacterSourceImage(int charid)
{
    CharacterInfo*chin = &game.chars[charid];
    int sppic = views[chin->view].loops[chin->loop].frames[chin->frame].pic;
    return spriteset[sppic];
}

CharacterInfo *GetCharacterAtScreen(int xx, int yy) {
    VpPoint vpt = play.ScreenToRoom(xx, yy);
    if (vpt.second < 0)
        return nullptr;
    int chnum = is_pos_on_character(vpt.first.X, vpt.first.Y);
    if (chnum < 0)
        return nullptr;
    return &game.chars[chnum];
}

CharacterInfo *GetCharacterAtRoom(int x, int y)
{
    int hsnum = is_pos_on_character(x, y);
    if (hsnum < 0)
        return nullptr;
    return &game.chars[hsnum];
}

extern int char_lowest_yp, obj_lowest_yp;

void update_character_scale(int charid)
{
    // Test for valid view and loop
    CharacterInfo &chin = game.chars[charid];
    if (!chin.is_enabled() || chin.room != displayed_room)
        return; // not enabled, or in a different room

    CharacterExtras &chex = charextra[charid];
    if (chin.view < 0)
    {
        quitprintf("!The character '%s' was turned on in the current room (room %d) but has not been assigned a view number.",
            chin.scrname.GetCStr(), displayed_room);
    }
    if (chin.loop >= views[chin.view].numLoops)
    {
        quitprintf("!The character '%s' could not be displayed because there was no loop %d of view %d.",
            chin.scrname.GetCStr(), chin.loop, chin.view + 1);
    }
    // If frame is too high -- fallback to the frame 0;
    // there's always at least 1 dummy frame at index 0
    if (chin.frame >= views[chin.view].loops[chin.loop].numFrames)
    {
        chin.frame = 0;
    }

    const ViewFrame &vf = views[chin.view].loops[chin.loop].frames[chin.frame];
    const int pic = vf.pic;
    int zoom, zoom_offs, scale_width, scale_height;
    update_object_scale(zoom, scale_width, scale_height,
        chin.x, chin.y, pic,
        chex.zoom, (chin.flags & CHF_MANUALSCALING) == 0);
    zoom_offs = (game.options[OPT_SCALECHAROFFSETS] != 0) ? zoom : 100;

    // Save calculated properties and recalc GS
    chex.zoom = zoom;
    chex.spr_width = game.SpriteInfos[pic].Width;
    chex.spr_height = game.SpriteInfos[pic].Height;
    chex.frame_xoff = vf.xoffs;
    chex.frame_yoff = vf.yoffs;
    chex.width = scale_width;
    chex.height = scale_height;
    chex.zoom_offs = zoom_offs;
    chex.UpdateGraphicSpace(&chin);
}

int is_pos_on_character(int xx,int yy) {
    int cc,sppic,lowestyp=0,lowestwas=-1;
    for (cc=0;cc<game.numcharacters;cc++) {
        if (game.chars[cc].room!=displayed_room) continue;
        if (!game.chars[cc].is_displayed()) continue; // disabled or not visible
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
        int usewid = game.SpriteInfos[sppic].Width;
        int usehit = game.SpriteInfos[sppic].Height;
        // TODO: support mirrored transformation in GraphicSpace
        SpriteTransformFlags sprite_flags = views[chin->view].loops[chin->loop].frames[chin->frame].flags;
        Bitmap *theImage = GetCharacterSourceImage(cc);
        // Convert to local object coordinates
        Point local = charextra[cc].GetGraphicSpace().WorldToLocal(xx, yy);
        if (is_pos_in_sprite(local.X, local.Y, 0, 0, theImage,
                usewid, usehit, sprite_flags) == FALSE)
            continue;

        int use_base = chin->get_baseline();
        if (use_base < lowestyp) continue;
        lowestyp=use_base;
        lowestwas=cc;
    }
    char_lowest_yp = lowestyp;
    return lowestwas;
}

Rect get_char_blocking_rect(int charid)
{
    CharacterInfo *chi = &game.chars[charid];

    int width;
    if (chi->blocking_width < 1)
        width = GetCharacterWidth(charid) - 4;
    else
        width = chi->blocking_width;

    int x = chi->x - width / 2 + game.chars[chi->index_id].blocking_x;
    int y1 = chi->get_blocking_top() + game.chars[chi->index_id].blocking_y;
    int y2 = chi->get_blocking_bottom() + game.chars[chi->index_id].blocking_y;

    return Rect(x, y1, x + width - 1, y2);
}

int my_getpixel(Bitmap *blk, int x, int y) {
    if ((x < 0) || (y < 0) || (x >= blk->GetWidth()) || (y >= blk->GetHeight()))
        return -1;

    // strip the alpha channel
    // TODO: is there a way to do this vtable thing with Bitmap?
    BITMAP *al_bmp = (BITMAP*)blk->GetAllegroBitmap();
    return al_bmp->vtable->getpixel(al_bmp, x, y) & 0x00ffffff;
}

int check_click_on_character(int xx,int yy,int mood) {
    int lowestwas=is_pos_on_character(xx,yy);
    if (lowestwas>=0) {
        Character_RunInteraction(&game.chars[lowestwas], mood);
        return 1;
    }
    return 0;
}

void DisplaySpeechCore(int chid, const char *displbuf) {
    if (displbuf[0] == 0) {
        // no text, just update the current character who's speaking
        // this allows the portrait side to be switched with an empty
        // speech line
        play.swap_portrait_lastchar = chid;
        return;
    }

    // adjust timing of text (so that DisplaySpeech("%s", str) pauses
    // for the length of the string not 2 frames)
    int len = (int)strlen(displbuf);
    if (len > source_text_length + 3)
        source_text_length = len;

    DisplaySpeech(displbuf, chid);
}

void DisplayThoughtCore(int chid, const char *displbuf) {
    // adjust timing of text (so that DisplayThought("%s", str) pauses
    // for the length of the string not 2 frames)
    int len = (int)strlen(displbuf);
    if (len > source_text_length + 3)
        source_text_length = len;

    int width = -1;
    if ((game.options[OPT_SPEECHTYPE] == kSpeechStyle_LucasArts) || (game.chars[chid].thinkview <= 0)) {
        // lucasarts-style, so we want a speech bubble actually above
        // their head (or if they have no think anim in Sierra-style)
        width = play.speech_bubble_width;
    }

    display_speech(displbuf, chid, -1, -1, width, true /*auto-pos*/, true /* is thought */);
}

void display_speech(const char *texx, int aschar, int xx, int yy, int widd, bool auto_position, bool is_thought)
{
    if (!is_valid_character(aschar))
        quit("!DisplaySpeech: invalid character");

    CharacterInfo *speakingChar = &game.chars[aschar];
    if ((speakingChar->view < 0) || (speakingChar->view >= game.numviews))
        quit("!DisplaySpeech: character has invalid view");

    if (play.screen_is_faded_out > 0)
        debug_script_warn("Warning: blocking Say call during fade-out.");
    if (play.text_overlay_on > 0)
    {
        debug_script_warn("DisplaySpeech: speech was already displayed (nested DisplaySpeech, perhaps room script and global script conflict?)");
        return;
    }

    EndSkippingUntilCharStops();

    said_speech_line = 1;

    if (play.bgspeech_stay_on_display == 0)
    {
        // remove any background speech
        auto &overs = get_overlays();
        for (auto &over : overs)
        {
            if (over.GetTimeout() > 0)
                remove_screen_overlay(over.GetID());
        }
    }
    said_text = 1;

    set_our_eip(150);

    int isPause = 1;
    // if the message is all .'s, don't display anything
    for (size_t aa = 0; texx[aa] != 0; aa++) {
        if (texx[aa] != '.') {
            isPause = 0;
            break;
        }
    }

    play.messagetime = GetTextDisplayTime(texx);
    play.speech_in_post_state = false;

    if (isPause) {
        postpone_scheduled_music_update_by(std::chrono::milliseconds(play.messagetime * 1000 / frames_per_second));
        // Set a post-state right away, as we only need to wait for a messagetime timer
        play.speech_in_post_state = true;
        GameLoopUntilValueIsNegative(&play.messagetime);
        post_display_cleanup();
        return;
    }

    DisplayTextStyle disp_style = kDisplayTextStyle_Overchar;
    // If the character is in this room, then default to aligning the speech
    // to the character position; but if it's not then center the speech on screen
    // NOTE: clamping the freely positioned speech (see SayAt) to the screen width
    // is a historical behavior. It's not known why but it was not clamped to height;
    // this has to be kept in mind if backwards compatibility matters.
    DisplayTextPosition disp_pos = (DisplayTextPosition)
        ((auto_position ?
            get_textpos_from_scriptcoords(xx, yy, (speakingChar->room == displayed_room)) :
            kDisplayTextPos_Normal)
        | kDisplayTextPos_ClampToScreenWidth);
    const color_t text_color = speakingChar->talkcolor;

    DisplayTextShrink allow_shrink = kDisplayTextShrink_None;
    bool align_hcenter = false; // whether to align text by centering over position

    const Rect ui_view = play.GetUIViewport();
    int bwidth = widd;
    if (bwidth < 0)
        bwidth = ui_view.GetWidth()/2 + ui_view.GetWidth()/4;

    set_our_eip(151);

    int useview = speakingChar->talkview;
    if (is_thought)
    {
        useview = speakingChar->thinkview;
        // view 0 is not valid for think views
        if (useview == 0)
            useview = -1;
        // speech bubble can shrink to fit
        allow_shrink = kDisplayTextShrink_Left;
    }

    if (useview >= game.numviews)
        quitprintf("!Character.Say: attempted to use view %d for animation, but it does not exist", useview + 1);

    if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
        remove_screen_overlay(OVER_COMPLETE);
    set_our_eip(1500);

    if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_LucasArts)
        allow_shrink = kDisplayTextShrink_Left;

    // If has a valid speech view, and idle anim in progress for the character, then stop it
    if (useview >= 0)
    {
        stop_character_idling(speakingChar);
    }

    // TODO: consider turning certain speech styles into autoplaced overlays
    // in the future; but that would require a large refactor of all the coordinate
    // calculations below, and inside display_main.
    int tdxp = xx, tdyp = yy;
    int oldview=-1, oldloop = -1;
    int ovr_type = 0;
    text_lips_offset = 0;
    text_lips_text = texx;
    std::unique_ptr<Bitmap> closeupface;
    int charFrameWas = 0;
    int viewWasLocked = 0;
    if (speakingChar->flags & CHF_FIXVIEW)
        viewWasLocked = 1;
    char_speaking = speakingChar->index_id;

    // Start voice-over, if requested by the tokens in speech text
    try_auto_play_speech(texx, texx, aschar);

    if (speakingChar->room == displayed_room) {
        // If the character is in this room, go for it - otherwise
        // run the "else" clause which  does text in the middle of
        // the screen.
        set_our_eip(1501);

        if (speakingChar->walking)
            Character_StopMoving(speakingChar);

        // save the frame we need to go back to
        // if they were moving, this will be 0 (because we just called
        // StopMoving); otherwise, it might be a specific animation 
        // frame which we should return to
        if (viewWasLocked)
            charFrameWas = speakingChar->frame;

        if ((speakingChar->view < 0) || views[speakingChar->view].numLoops == 0)
            quitprintf("!Character %s current view %d is invalid, or has no loops.",
                speakingChar->scrname.GetCStr(), speakingChar->view + 1);
        // If current view is missing a loop - use loop 0
        if (speakingChar->loop >= views[speakingChar->view].numLoops)
        {
            debug_script_warn("WARNING: Character %s current view %d does not have necessary loop %d; switching to loop 0.",
                speakingChar->scrname.GetCStr(), speakingChar->view + 1, speakingChar->loop);
            speakingChar->loop = 0;
        }

        set_our_eip(1504);

        // Calculate speech position based on character's position on screen
        // (an assumption here, but this must be the case if it's LA-style speech)
        auto view = FindNearestViewport(aschar);
        if (tdxp < 0)
            tdxp = view->RoomToScreen(speakingChar->x, 0).first.X;
        if (tdxp < 2)
            tdxp = 2;
        // tell it to align it by center
        align_hcenter = auto_position && (xx < 0);

        if (tdyp < 0)
        {
            int sppic = views[speakingChar->view].loops[speakingChar->loop].frames[0].pic;
            int height = (charextra[aschar].height < 1) ? game.SpriteInfos[sppic].Height : charextra[aschar].height;
            tdyp = view->RoomToScreen(0, charextra[aschar].GetEffectiveY(speakingChar) - height).first.Y
                - 5;
            if (is_thought) // if it's a thought, lift it a bit further up
                tdyp -= 10;
        }
        if (tdyp < 5)
            tdyp = 5;

        set_our_eip(152);

        if ((game.options[OPT_SPEECHTYPE] > kSpeechStyle_LucasArts)
            && (useview >= 0) && (views[useview].numLoops > 0))
        {
            // Sierra-style close-up portrait
            disp_pos = kDisplayTextPos_Normal;

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
                        // No previous character been spoken to
                        // therefore, assume it's the player
                        if (game.playercharacter != aschar &&
                            game.chars[game.playercharacter].room == speakingChar->room &&
                            game.chars[game.playercharacter].is_enabled())
                        {
                            play.swap_portrait_lastchar = game.playercharacter;
                        }
                        else
                        {
                            // The player's not here. Find another character in this room
                            // that it could be
                            for (int ce = 0; ce < game.numcharacters; ce++)
                            {
                                if ((game.chars[ce].room == speakingChar->room) &&
                                    (game.chars[ce].is_enabled()) &&
                                    (ce != aschar))
                                {
                                    play.swap_portrait_lastchar = ce;
                                    break;
                                }
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
                play.swap_portrait_lastlastchar = play.swap_portrait_lastchar;
                play.swap_portrait_lastchar = aschar;
            }
            else
                // If the portrait side is based on the character's X position and the same character is
                // speaking, compare against the previous *previous* character to see where the speech should be
                if (game.options[OPT_PORTRAITSIDE] == PORTRAIT_XPOSITION && play.swap_portrait_lastlastchar >= 0) {
                    if (speakingChar->x > game.chars[play.swap_portrait_lastlastchar].x)
                        play.swap_portrait_side = -1;
                    else
                        play.swap_portrait_side = 0;
                }

            // Determine whether to display the portrait on the left or right
            int portrait_on_right = 0;

            if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
            { }  // always on left with QFG-style speech
            else if ((play.swap_portrait_side == 1) ||
                (play.swap_portrait_side == -1) ||
                (game.options[OPT_PORTRAITSIDE] == PORTRAIT_RIGHT))
                portrait_on_right = 1;


            int bigx=0,bigy=0,kk;
            ViewStruct*viptr=&views[useview];
            for (kk = 0; kk < viptr->loops[0].numFrames; kk++) 
            {
                int tw = game.SpriteInfos[viptr->loops[0].frames[kk].pic].Width;
                if (tw > bigx) bigx=tw;
                tw = game.SpriteInfos[viptr->loops[0].frames[kk].pic].Height;
                if (tw > bigy) bigy=tw;
            }

            // if they accidentally used a large full-screen image as the sierra-style
            // talk view, correct it
            if ((game.options[OPT_SPEECHTYPE] != kSpeechStyle_QFG4) && (bigx > ui_view.GetWidth() - 50))
                bigx = ui_view.GetWidth() - 50;

            if (widd > 0)
                bwidth = widd - bigx;

            set_our_eip(153);
            int ovr_yp = 20;
            int view_frame_x = 0;
            int view_frame_y = 0;
            facetalk_qfg4_override_placement_x = false;
            facetalk_qfg4_override_placement_y = false;

            if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4) {
                // QFG4-style whole screen picture
                closeupface.reset(BitmapHelper::CreateBitmap(ui_view.GetWidth(), ui_view.GetHeight()));
                closeupface->Clear(0);
                if (xx < 0 && play.speech_portrait_placement)
                {
                    facetalk_qfg4_override_placement_x = true;
                    view_frame_x = play.speech_portrait_x;
                }
                if (yy < 0 && play.speech_portrait_placement)
                {
                    facetalk_qfg4_override_placement_y = true;
                    view_frame_y = play.speech_portrait_y;
                }
                else
                {
                    view_frame_y = ui_view.GetHeight()/2 - game.SpriteInfos[viptr->loops[0].frames[0].pic].Height/2;
                }
                bigx = ui_view.GetWidth()/2 - 20;
                ovr_type = OVER_COMPLETE;
                ovr_yp = 0;
                tdyp = -1;  // center vertically
            }
            else {
                // KQ6-style close-up face picture
                if (yy < 0 && play.speech_portrait_placement)
                {
                    ovr_yp = play.speech_portrait_y;
                }
                else if (yy < 0)
                    ovr_yp = adjust_y_for_guis(ovr_yp, true /* displayspeech is always blocking */);
                else
                    ovr_yp = yy;

                closeupface.reset(BitmapHelper::CreateTransparentBitmap(bigx + 1, bigy + 1));
                ovr_type = OVER_PICTURE;

                if (yy < 0)
                    tdyp = ovr_yp + get_textwindow_top_border_height(play.speech_textwindow_gui);
            }
            const ViewFrame *vf = &viptr->loops[0].frames[0];
            DrawViewFrame(closeupface.get(), vf, view_frame_x, view_frame_y);

            int overlay_x = 10;
            if (xx < 0) {
                tdxp = bigx + get_textwindow_border_width(play.speech_textwindow_gui) / 2;
                if (play.speech_portrait_placement)
                {
                    overlay_x = play.speech_portrait_x;
                    tdxp += overlay_x + 6;
                }
                else
                {
                    tdxp += 16;
                }

                int maxWidth = (ui_view.GetWidth() - tdxp) - 5 -
                    get_textwindow_border_width (play.speech_textwindow_gui) / 2;

                if (bwidth > maxWidth)
                    bwidth = maxWidth;
            }
            else {
                tdxp = xx + bigx + 8;
                overlay_x = xx;
            }

            // allow the text box to be shrunk to fit the text
            allow_shrink = kDisplayTextShrink_Left;

            // if the portrait's on the right, swap it round
            if (portrait_on_right) {
                if ((xx < 0) || (widd < 0)) {
                    tdxp = 9;
                    if (play.speech_portrait_placement)
                    {
                        overlay_x = (ui_view.GetWidth() - bigx) - play.speech_portrait_x;
                        int maxWidth = overlay_x - tdxp - 9 - 
                            get_textwindow_border_width (play.speech_textwindow_gui) / 2;
                        if (bwidth > maxWidth)
                            bwidth = maxWidth;
                    }
                    else
                    {
                        overlay_x = (ui_view.GetWidth() - bigx) - 5;
                    }
                }
                else {
                    overlay_x = (xx + widd - bigx) - 5;
                    tdxp = xx;
                }
                tdxp += get_textwindow_border_width(play.speech_textwindow_gui) / 2;
                allow_shrink = kDisplayTextShrink_Right;
            }
            if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
                overlay_x = 0;
            face_talking = add_screen_overlay(false, overlay_x, ovr_yp, ovr_type, std::move(closeupface));
            facetalkview = useview;
            facetalkloop = 0;
            facetalkframe = 0;
            facetalkwait = viptr->loops[0].frames[0].speed + Character_GetSpeechAnimationDelay(speakingChar);
            facetalkrepeat = (is_thought) ? 0 : 1;
            facetalkBlinkLoop = 0;
            facetalkAllowBlink = 1;
            if ((is_thought) && (speakingChar->flags & CHF_NOBLINKANDTHINK))
                facetalkAllowBlink = 0;
            facetalkchar = &game.chars[aschar];
            if (facetalkchar->blinktimer < 0)
                facetalkchar->blinktimer = facetalkchar->blinkinterval;
            disp_style = kDisplayTextStyle_TextWindow;
            // Process the first portrait view frame
            const int frame_vol = charextra[facetalkchar->index_id].GetFrameSoundVolume(facetalkchar);
            CheckViewFrame(facetalkview, facetalkloop, facetalkframe, frame_vol);
        }
        else if (useview >= 0) {
            // Lucasarts-style speech
            set_our_eip(154);

            oldview = speakingChar->view;
            oldloop = speakingChar->loop;

            charextra[speakingChar->index_id].SetAnimating(
                is_thought ? kAnimFlow_Once : kAnimFlow_Repeat, // only repeat if speech, not thought
                kAnimDirForward, // always forwards
                Character_GetSpeechAnimationDelay(speakingChar));

            speakingChar->view = useview;
            speakingChar->frame=0;
            speakingChar->flags|=CHF_FIXVIEW;

            if ((speakingChar->view < 0) || views[speakingChar->view].numLoops == 0)
                quitprintf("!Character %s speech view %d is invalid, or has no loops.",
                    speakingChar->scrname.GetCStr(), speakingChar->view + 1);
            // If speech view is missing a loop - use loop 0
            if (speakingChar->loop >= views[speakingChar->view].numLoops)
            {
                debug_script_warn("WARNING: Character %s speech view %d does not have necessary loop %d; switching to loop 0.",
                    speakingChar->scrname.GetCStr(), speakingChar->view + 1, speakingChar->loop);
                speakingChar->loop = 0;
            }

            facetalkBlinkLoop = speakingChar->loop;

            // set up the speed of the first frame
            speakingChar->wait = Character_GetSpeechAnimationDelay(speakingChar) +
                views[speakingChar->view].loops[speakingChar->loop].frames[0].speed;

            if (widd < 0) {
                bwidth = ui_view.GetWidth()/2 + ui_view.GetWidth()/6;
                // If they are close to the screen edge, make the text narrower
                int relx = play.RoomToScreenX(speakingChar->x);
                if ((relx < ui_view.GetWidth() / 4) || (relx > ui_view.GetWidth() - (ui_view.GetWidth() / 4)))
                    bwidth -= ui_view.GetWidth() / 5;
            }
            if (!is_thought)  // set up the lip sync if not thinking
                char_speaking_anim = aschar;
        }
    }
    else
    {
        // If the character is in another room, then center the speech on screen
        allow_shrink = kDisplayTextShrink_Left;
    }

    // If initial argument was NOT requiring a autoposition,
    // but further calculation set it to be centered, then make it so here
    // (NOTE: this assumes that a valid width is also passed)
    if ((xx >= 0) && align_hcenter)
        tdxp -= widd / 2;

    // if they used DisplaySpeechAt, then use the supplied width
    if ((widd > 0) && (!is_thought))
        allow_shrink = kDisplayTextShrink_None;

    if (is_thought)
        char_thinking = aschar;

    set_our_eip(155);
    display_main(tdxp, tdyp, bwidth, texx, nullptr, kDisplayText_Speech, 0 /* no overid */,
        DisplayTextLooks(disp_style, disp_pos, allow_shrink, is_thought), FONT_SPEECH, text_color, -1 /* don't autoplace */);
    set_our_eip(156);
    if (face_talking >= 0)
        remove_screen_overlay(face_talking);
    face_talking = -1;
    facetalkchar = nullptr;
    set_our_eip(157);
    if (oldview>=0) {
        speakingChar->flags &= ~CHF_FIXVIEW;
        if (viewWasLocked)
            speakingChar->flags |= CHF_FIXVIEW;
        speakingChar->view=oldview;
        stop_character_anim(speakingChar);
        speakingChar->frame = charFrameWas;
        speakingChar->wait=0;
        // Restart idle timer
        reset_character_idling_time(speakingChar);
    }
    char_speaking = -1;
    char_speaking_anim = -1;
    char_thinking = -1;
    // Stop any blocking voice-over, if was started by this function
    if (play.IsBlockingVoiceSpeech())
        stop_voice_speech();
}

int get_character_currently_talking() {
    if ((face_talking >= 0) && (facetalkrepeat))
        return facetalkchar->index_id;
    else if (char_speaking >= 0)
        return char_speaking;

    return -1;
}

void DisplaySpeech(const char *texx, int aschar)
{
    display_speech(texx, aschar, -1, -1, -1, true /*auto-pos*/, false /* not thought */);
}

// Calculate which frame of the loop to use for this character of
// speech
int GetLipSyncFrame (const char *curtex, int *stroffs) {
    /*char *frameletters[MAXLIPSYNCFRAMES] =
    {"./,/ ", "A", "O", "F/V", "D/N/G/L/R", "B/P/M",
    "Y/H/K/Q/C", "I/T/E/X/th", "U/W", "S/Z/J/ch", NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};*/

    size_t bestfit_len = 0;
    int bestfit = game.default_lipsync_frame;
    for (int aa = 0; aa < MAXLIPSYNCFRAMES; aa++) {
        char *tptr = game.lipSyncFrameLetters[aa];
        while (tptr[0] != 0) {
            size_t lenthisbit = strlen(tptr);
            if (strchr(tptr, '/'))
                lenthisbit = strchr(tptr, '/') - tptr;

            if ((ags_strnicmp (curtex, tptr, lenthisbit) == 0) && (lenthisbit > bestfit_len)) {
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

// Updates text-based lipsync
int update_lip_sync(int talkview, int talkloop, int *talkframeptr) {
    int talkframe = talkframeptr[0];
    int talkwait = 0;

    // lip-sync speech
    const char *nowsaying = &text_lips_text[text_lips_offset];
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

void restore_characters()
{
    for (int i = 0; i < game.numcharacters; ++i)
    {
        charextra[i].zoom_offs = (game.options[OPT_SCALECHAROFFSETS] != 0) ?
            charextra[i].zoom : 100;
    }
}

Rect GetCharacterRoomBBox(int charid, bool use_frame_0)
{
    int width, height;
    const CharacterExtras& chex = charextra[charid];
    const CharacterInfo& chin = game.chars[charid];
    int frame = use_frame_0 ? 0 : chin.frame;
    int pic = views[chin.view].loops[chin.loop].frames[frame].pic;
    scale_sprite_size(pic, chex.zoom, &width, &height);
    return RectWH(chin.x - width / 2, chin.y - height, width, height);
}

PViewport FindNearestViewport(int charid)
{
    Rect bbox = GetCharacterRoomBBox(charid, true);
    float min_dist = -1.f;
    PViewport nearest_view;
    for (int i = 0; i < play.GetRoomViewportCount(); ++i)
    {
        auto view = play.GetRoomViewport(i);
        if (!view->IsVisible())
            continue;
        auto cam = view->GetCamera();
        if (!cam)
            continue;
        Rect camr = cam->GetRect();
        float dist = DistanceBetween(bbox, camr);
        if (dist == 0.f)
            return view;
        if (min_dist < 0.f || dist < min_dist)
        {
            min_dist = dist;
            nearest_view = view;
        }
    }
    return nearest_view ? nearest_view : play.GetRoomViewport(0);
}

void UpdateInventory() {
    for (int cc = 0; cc < game.numcharacters; cc++) {
        charextra[cc].invorder_count = 0;
        int ff, howmany;
        // Iterate through all inv items, adding them once (or multiple
        // times if requested) to the list.
        for (ff = 0; ff < game.numinvitems; ff++) {
            howmany = game.chars[cc].inv[ff];
            if ((game.options[OPT_DUPLICATEINV] == 0) && (howmany > 1))
                howmany = 1;

            for (int ts = 0; ts < howmany; ts++) {
                if (charextra[cc].invorder_count >= MAX_INVORDER)
                    quit("!Too many inventory items to display: 500 max");

                charextra[cc].invorder[charextra[cc].invorder_count] = ff;
                charextra[cc].invorder_count++;
            }
        }
    }
    GUIE::MarkInventoryForUpdate(game.playercharacter, true);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"

CharacterInfo *Character_GetByName(const char *name)
{
    return static_cast<CharacterInfo*>(ccGetScriptObjectAddress(name, ccDynamicCharacter.GetType()));
}


RuntimeScriptValue Sc_Character_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(CharacterInfo, ccDynamicCharacter, Character_GetByName, const char);
}

// void | CharacterInfo *chaa, ScriptInvItem *invi, int addIndex
RuntimeScriptValue Sc_Character_AddInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(CharacterInfo, Character_AddInventory, ScriptInvItem);
}

// void | CharacterInfo *chaa, int x, int y
RuntimeScriptValue Sc_Character_AddWaypoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_AddWaypoint);
}

// void | CharacterInfo *chaa, int loop, int delay, int repeat, int blocking, int direction
RuntimeScriptValue Sc_Character_Animate5(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(CharacterInfo, Character_Animate5);
}

RuntimeScriptValue Sc_Character_Animate6(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(CharacterInfo, Character_Animate6);
}

RuntimeScriptValue Sc_Character_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT7(CharacterInfo, Character_Animate);
}

// void | CharacterInfo *chaa, int room, int x, int y
RuntimeScriptValue Sc_Character_ChangeRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_ChangeRoom);
}

RuntimeScriptValue Sc_Character_ChangeRoomSetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_ChangeRoomSetLoop);
}

// void | CharacterInfo *chaa, int room, int newPos
RuntimeScriptValue Sc_Character_ChangeRoomAutoPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_ChangeRoomAutoPosition);
}

// void | CharacterInfo *chap, int vii
RuntimeScriptValue Sc_Character_ChangeView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_ChangeView);
}

// void | CharacterInfo *char1, CharacterInfo *char2, int blockingStyle
RuntimeScriptValue Sc_Character_FaceCharacter(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(CharacterInfo, Character_FaceCharacter, CharacterInfo);
}

// void | CharacterInfo *char1, int direction, int blockingStyle
RuntimeScriptValue Sc_Character_FaceDirection(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_FaceDirection);
}

// void | CharacterInfo *char1, int xx, int yy, int blockingStyle
RuntimeScriptValue Sc_Character_FaceLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_FaceLocation);
}

// void | CharacterInfo *char1, ScriptObject *obj, int blockingStyle
RuntimeScriptValue Sc_Character_FaceObject(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(CharacterInfo, Character_FaceObject, ScriptObject);
}

// void | CharacterInfo *chaa, CharacterInfo *tofollow, int distaway, int eagerness
RuntimeScriptValue Sc_Character_FollowCharacter(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT2(CharacterInfo, Character_FollowCharacter, CharacterInfo);
}

// CharacterInfo * | CharacterInfo *chaa
RuntimeScriptValue Sc_Character_GetFollowing(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, CharacterInfo, ccDynamicCharacter, Character_GetFollowing);
}

// int (CharacterInfo *chaa, const char *property)
RuntimeScriptValue Sc_Character_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_GetProperty, const char);
}

// void (CharacterInfo *chaa, const char *property, char *bufer)
RuntimeScriptValue Sc_Character_GetPropertyText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ2(CharacterInfo, Character_GetPropertyText, const char, char);
}

// const char* (CharacterInfo *chaa, const char *property)
RuntimeScriptValue Sc_Character_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(CharacterInfo, const char, myScriptStringImpl, Character_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Character_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(CharacterInfo, Character_SetProperty, const char);
}

RuntimeScriptValue Sc_Character_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(CharacterInfo, Character_SetTextProperty, const char, const char);
}

// int (CharacterInfo *chaa, ScriptInvItem *invi)
RuntimeScriptValue Sc_Character_HasInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_HasInventory, ScriptInvItem);
}

// int (CharacterInfo *char1, CharacterInfo *char2)
RuntimeScriptValue Sc_Character_IsCollidingWithChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_IsCollidingWithChar, CharacterInfo);
}

// int (CharacterInfo *chin, ScriptObject *objid)
RuntimeScriptValue Sc_Character_IsCollidingWithObject(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_IsCollidingWithObject, ScriptObject);
}

RuntimeScriptValue Sc_Character_IsInteractionAvailable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_PINT(CharacterInfo, Character_IsInteractionAvailable);
}

// void (CharacterInfo *chap, int vii)
RuntimeScriptValue Sc_Character_LockView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_LockView);
}

// void (CharacterInfo *chap, int vii, int stopMoving)
RuntimeScriptValue Sc_Character_LockViewEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_LockViewEx);
}

RuntimeScriptValue Sc_Character_LockViewAlignedEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_LockViewAlignedEx);
}

RuntimeScriptValue Sc_Character_LockViewAnchored(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(Character_LockViewAnchored, 1);
    Character_LockViewAnchored((CharacterInfo*)self, params[0].IValue, params[1].FValue, params[2].FValue, params[3].IValue, params[4].IValue, params[5].IValue);
    return RuntimeScriptValue((int32_t)0);
}

// void (CharacterInfo *chaa, int view, int loop, int frame, int stopMoving)
RuntimeScriptValue Sc_Character_LockViewFrameEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_LockViewFrameEx);
}

// void (CharacterInfo *chap, int vii, int xoffs, int yoffs, int stopMoving)
RuntimeScriptValue Sc_Character_LockViewOffsetEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_LockViewOffsetEx);
}

// void (CharacterInfo *chap, ScriptInvItem *invi)
RuntimeScriptValue Sc_Character_LoseInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_LoseInventory, ScriptInvItem);
}

// void (CharacterInfo *chaa, int x, int y, int blocking, int direct) 
RuntimeScriptValue Sc_Character_Move(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_Move);
}

RuntimeScriptValue Sc_Character_MoveStraight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_MoveStraight);
}

RuntimeScriptValue Sc_Character_MovePath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT3(CharacterInfo, Character_MovePath, void);
}

// void (CharacterInfo *chap) 
RuntimeScriptValue Sc_Character_PlaceOnWalkableArea(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_PlaceOnWalkableArea);
}

// void (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_RemoveTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_RemoveTint);
}

RuntimeScriptValue Sc_Character_RunFrameEvent(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_RunFrameEvent);
}

// void (CharacterInfo *chaa, int mood)
RuntimeScriptValue Sc_Character_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_RunInteraction);
}

// void (CharacterInfo *chaa, const char *texx, ...)
RuntimeScriptValue Sc_Character_Say(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_Say, 1);
    Character_Say((CharacterInfo*)self, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_Character_SayAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_SayAt, 4);
    Character_SayAt((CharacterInfo*)self, params[0].IValue, params[1].IValue, params[2].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_Character_SayBackground(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_SayBackground, 1);
    auto *ret_obj = Character_SayBackground((CharacterInfo*)self, scsf_buffer);
    return RuntimeScriptValue().SetScriptObject(ret_obj, ret_obj);
}

RuntimeScriptValue Sc_Character_SetAsPlayer(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_SetAsPlayer);
}

// void (CharacterInfo *chaa, int iview, int itime)
RuntimeScriptValue Sc_Character_SetIdleView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_SetIdleView);
}

RuntimeScriptValue Sc_Character_GetHasExplicitLight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetHasExplicitLight);
}

RuntimeScriptValue Sc_Character_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetLightLevel);
}

RuntimeScriptValue Sc_Character_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetLightLevel);
}

RuntimeScriptValue Sc_Character_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintBlue);
}

RuntimeScriptValue Sc_Character_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintGreen);
}

RuntimeScriptValue Sc_Character_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintRed);
}

RuntimeScriptValue Sc_Character_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintSaturation);
}

RuntimeScriptValue Sc_Character_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintLuminance);
}

// void (CharacterInfo *chaa, int xspeed, int yspeed)
RuntimeScriptValue Sc_Character_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_SetSpeed);
}

// void (CharacterInfo *charp)
RuntimeScriptValue Sc_Character_StopMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_StopMoving);
}

// void (CharacterInfo *chaa, const char *texx, ...)
RuntimeScriptValue Sc_Character_Think(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_Think, 1);
    Character_Think((CharacterInfo*)self, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

//void (CharacterInfo *chaa, int red, int green, int blue, int opacity, int luminance)
RuntimeScriptValue Sc_Character_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(CharacterInfo, Character_Tint);
}

// void (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_UnlockView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_UnlockView);
}

// void (CharacterInfo *chaa, int stopMoving)
RuntimeScriptValue Sc_Character_UnlockViewEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_UnlockViewEx);
}

// void (CharacterInfo *chaa, int x, int y, int blocking, int direct)
RuntimeScriptValue Sc_Character_Walk(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_Walk);
}

// void (CharacterInfo *chaa, int xx, int yy, int blocking)
RuntimeScriptValue Sc_Character_WalkStraight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_WalkStraight);
}

RuntimeScriptValue Sc_Character_WalkPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT3(CharacterInfo, Character_WalkPath, void);
}

RuntimeScriptValue Sc_GetCharacterAtRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(CharacterInfo, ccDynamicCharacter, GetCharacterAtRoom);
}

// CharacterInfo *(int xx, int yy)
RuntimeScriptValue Sc_GetCharacterAtScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(CharacterInfo, ccDynamicCharacter, GetCharacterAtScreen);
}

// ScriptInvItem* (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetActiveInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, ScriptInvItem, ccDynamicInv, Character_GetActiveInventory);
}

// void (CharacterInfo *chaa, ScriptInvItem* iit)
RuntimeScriptValue Sc_Character_SetActiveInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_SetActiveInventory, ScriptInvItem);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAnimating);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetAnimationSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAnimationSpeed);
}

// void (CharacterInfo *chaa, int newval)
RuntimeScriptValue Sc_Character_SetAnimationSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetAnimationSpeed);
}

RuntimeScriptValue Sc_Character_GetAnimationVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAnimationVolume);
}

RuntimeScriptValue Sc_Character_SetAnimationVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetAnimationVolume);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBaseline);
}

// void (CharacterInfo *chaa, int basel)
RuntimeScriptValue Sc_Character_SetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBaseline);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetBlinkInterval(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlinkInterval);
}

// void (CharacterInfo *chaa, int interval)
RuntimeScriptValue Sc_Character_SetBlinkInterval(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlinkInterval);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetBlinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlinkView);
}

// void (CharacterInfo *chaa, int vii)
RuntimeScriptValue Sc_Character_SetBlinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlinkView);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetBlinkWhileThinking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlinkWhileThinking);
}

// void (CharacterInfo *chaa, int yesOrNo)
RuntimeScriptValue Sc_Character_SetBlinkWhileThinking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlinkWhileThinking);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingHeight);
}

// void (CharacterInfo *chaa, int hit)
RuntimeScriptValue Sc_Character_SetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingHeight);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingWidth);
}

// void (CharacterInfo *chaa, int wid)
RuntimeScriptValue Sc_Character_SetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingWidth);
}

RuntimeScriptValue Sc_Character_GetBlockingRectX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingRectX);
}

RuntimeScriptValue Sc_Character_SetBlockingRectX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingRectX);
}

RuntimeScriptValue Sc_Character_GetBlockingRectY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingRectY);
}

RuntimeScriptValue Sc_Character_SetBlockingRectY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingRectY);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetClickable);
}

// void (CharacterInfo *chaa, int clik)
RuntimeScriptValue Sc_Character_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetClickable);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetDiagonalWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetDiagonalWalking);
}

// void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetDiagonalWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetDiagonalWalking);
}

RuntimeScriptValue Sc_Character_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetEnabled);
}

RuntimeScriptValue Sc_Character_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(CharacterInfo, Character_SetEnabled);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetFrame);
}

// void (CharacterInfo *chaa, int newval)
RuntimeScriptValue Sc_Character_SetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetFrame);
}

RuntimeScriptValue Sc_Character_GetHasExplicitTint_Old(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetHasExplicitTint_Old);
}

RuntimeScriptValue Sc_Character_GetHasExplicitTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetHasExplicitTint);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetID);
}

RuntimeScriptValue Sc_Character_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, const char, myScriptStringImpl, Character_GetScriptName);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetIdleView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIdleView);
}

// int (CharacterInfo *chaa, int index)
RuntimeScriptValue Sc_Character_GetIInventoryQuantity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(CharacterInfo, Character_GetIInventoryQuantity);
}

// void (CharacterInfo *chaa, int index, int quant)
RuntimeScriptValue Sc_Character_SetIInventoryQuantity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_SetIInventoryQuantity);
}

// [DEPRECATED] int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetIgnoreLighting(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIgnoreLighting);
}

// [DEPRECATED] void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetIgnoreLighting(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetIgnoreLighting);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetLoop);
}

// void (CharacterInfo *chaa, int newval)
RuntimeScriptValue Sc_Character_SetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetLoop);
}

RuntimeScriptValue Sc_Character_GetManualScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetManualScaling);
}

RuntimeScriptValue Sc_Character_SetManualScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetManualScaling);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetMovementLinkedToAnimation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetMovementLinkedToAnimation);
}

// void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetMovementLinkedToAnimation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetMovementLinkedToAnimation);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetMoving);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetDestinationX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetDestinationX);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetDestinationY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetDestinationY);
}

// const char* (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, const char, myScriptStringImpl, Character_GetName);
}

// void (CharacterInfo *chaa, const char *newName)
RuntimeScriptValue Sc_Character_SetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_SetName, const char);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetNormalView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetNormalView);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetPreviousRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetPreviousRoom);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetRoom);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetScaleMoveSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetScaleMoveSpeed);
}

// void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetScaleMoveSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetScaleMoveSpeed);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetScaleVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetScaleVolume);
}

// void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetScaleVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetScaleVolume);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetScaling);
}

// void (CharacterInfo *chaa, int zoomlevel)
RuntimeScriptValue Sc_Character_SetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetScaling);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSolid);
}

// void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSolid);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetSpeaking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeaking);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetSpeakingFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeakingFrame);
}

// int (CharacterInfo *cha)
RuntimeScriptValue Sc_Character_GetSpeechAnimationDelay(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeechAnimationDelay);
}

// void (CharacterInfo *chaa, int newDelay)
RuntimeScriptValue Sc_Character_SetSpeechAnimationDelay(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Character_GetIdleAnimationDelay(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIdleAnimationDelay);
}

// void (CharacterInfo *chaa, int newDelay)
RuntimeScriptValue Sc_Character_SetIdleAnimationDelay(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetIdleAnimationDelay);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetSpeechColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeechColor);
}

// void (CharacterInfo *chaa, int ncol)
RuntimeScriptValue Sc_Character_SetSpeechColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSpeechColor);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetSpeechView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeechView);
}

// void (CharacterInfo *chaa, int vii)
RuntimeScriptValue Sc_Character_SetSpeechView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSpeechView);
}

RuntimeScriptValue Sc_Character_GetThinking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetThinking);
}

RuntimeScriptValue Sc_Character_GetThinkingFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetThinkingFrame);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetThinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetThinkView);
}

// void (CharacterInfo *chaa, int vii)
RuntimeScriptValue Sc_Character_SetThinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetThinkView);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTransparency);
}

// void (CharacterInfo *chaa, int trans)
RuntimeScriptValue Sc_Character_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetTransparency);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetTurnBeforeWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTurnBeforeWalking);
}

// void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetTurnBeforeWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetTurnBeforeWalking);
}

RuntimeScriptValue Sc_Character_GetTurnWhenFacing (void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTurnWhenFacing );
}

RuntimeScriptValue Sc_Character_SetTurnWhenFacing (void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetTurnWhenFacing);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetView);
}

RuntimeScriptValue Sc_Character_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetVisible);
}

RuntimeScriptValue Sc_Character_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(CharacterInfo, Character_SetVisible);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetWalkSpeedX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetWalkSpeedX);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetWalkSpeedY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetWalkSpeedY);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetX);
}

// void (CharacterInfo *chaa, int newval)
RuntimeScriptValue Sc_Character_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetX);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetY);
}

// void (CharacterInfo *chaa, int newval)
RuntimeScriptValue Sc_Character_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetY);
}

// int (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetZ(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetZ);
}

// void (CharacterInfo *chaa, int newval)
RuntimeScriptValue Sc_Character_SetZ(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetZ);
}

RuntimeScriptValue Sc_Character_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlendMode);
}

RuntimeScriptValue Sc_Character_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlendMode);
}

RuntimeScriptValue Sc_Character_GetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetGraphicAnchorX);
}

RuntimeScriptValue Sc_Character_SetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetGraphicAnchorX);
}

RuntimeScriptValue Sc_Character_GetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetGraphicAnchorY);
}

RuntimeScriptValue Sc_Character_SetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetGraphicAnchorY);
}

RuntimeScriptValue Sc_Character_GetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetGraphicOffsetX);
}

RuntimeScriptValue Sc_Character_SetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetGraphicOffsetX);
}

RuntimeScriptValue Sc_Character_GetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetGraphicOffsetY);
}

RuntimeScriptValue Sc_Character_SetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetGraphicOffsetY);
}

RuntimeScriptValue Sc_Character_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(CharacterInfo, ScriptShaderInstance, Character_GetShader);
}

RuntimeScriptValue Sc_Character_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_SetShader, ScriptShaderInstance);
}

// bool (CharacterInfo *chaa)
RuntimeScriptValue Sc_Character_GetUseRegionTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetUseRegionTint);
}

// void (CharacterInfo *chaa, int yesorno)
RuntimeScriptValue Sc_Character_SetUseRegionTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetUseRegionTint);
}

RuntimeScriptValue Sc_Character_GetViewAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetViewAnchorX);
}

RuntimeScriptValue Sc_Character_GetViewAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetViewAnchorY);
}

RuntimeScriptValue Sc_Character_GetViewOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetViewOffsetX);
}

RuntimeScriptValue Sc_Character_GetViewOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetViewOffsetY);
}

RuntimeScriptValue Sc_Character_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetRotation);
}

RuntimeScriptValue Sc_Character_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetRotation);
}

RuntimeScriptValue Sc_Character_GetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Character_SetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Character_GetMotionPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(CharacterInfo, ScriptMotionPath, Character_GetMotionPath);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

void ScPl_Character_Say(CharacterInfo *chaa, const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    Character_Say(chaa, scsf_buffer);
}

void ScPl_Character_SayAt(CharacterInfo *chaa, int x, int y, int width, const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    Character_SayAt(chaa, x, y, width, scsf_buffer);
}

ScriptOverlay *ScPl_Character_SayBackground(CharacterInfo *chaa, const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    return Character_SayBackground(chaa, scsf_buffer);
}

void ScPl_Character_Think(CharacterInfo *chaa, const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    Character_Think(chaa, scsf_buffer);
}

void RegisterCharacterAPI(ScriptAPIVersion /*base_api*/, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister character_api[] = {
        { "Character::GetAtRoomXY^2",             API_FN_PAIR(GetCharacterAtRoom) },
        { "Character::GetAtScreenXY^2",           API_FN_PAIR(GetCharacterAtScreen) },
        { "Character::GetByName",                 API_FN_PAIR(Character_GetByName) },

        { "Character::AddInventory^2",            API_FN_PAIR(Character_AddInventory) },
        { "Character::AddWaypoint^2",             API_FN_PAIR(Character_AddWaypoint) },
        { "Character::Animate^5",                 API_FN_PAIR(Character_Animate5) },
        { "Character::Animate^6",                 API_FN_PAIR(Character_Animate6) },
        { "Character::Animate^7",                 API_FN_PAIR(Character_Animate) },
        { "Character::ChangeRoom^3",              API_FN_PAIR(Character_ChangeRoom) },
        { "Character::ChangeRoom^4",              API_FN_PAIR(Character_ChangeRoomSetLoop) },
        { "Character::ChangeRoomAutoPosition^2",  API_FN_PAIR(Character_ChangeRoomAutoPosition) },
        { "Character::ChangeView^1",              API_FN_PAIR(Character_ChangeView) },
        { "Character::FaceCharacter^2",           API_FN_PAIR(Character_FaceCharacter) },
        { "Character::FaceDirection^2",           API_FN_PAIR(Character_FaceDirection) },
        { "Character::FaceLocation^3",            API_FN_PAIR(Character_FaceLocation) },
        { "Character::FaceObject^2",              API_FN_PAIR(Character_FaceObject) },
        { "Character::FollowCharacter^3",         API_FN_PAIR(Character_FollowCharacter) },
        { "Character::GetProperty^1",             API_FN_PAIR(Character_GetProperty) },
        { "Character::GetPropertyText^2",         API_FN_PAIR(Character_GetPropertyText) },
        { "Character::GetTextProperty^1",         API_FN_PAIR(Character_GetTextProperty) },
        { "Character::SetProperty^2",             API_FN_PAIR(Character_SetProperty) },
        { "Character::SetTextProperty^2",         API_FN_PAIR(Character_SetTextProperty) },
        { "Character::HasInventory^1",            API_FN_PAIR(Character_HasInventory) },
        { "Character::IsCollidingWithChar^1",     API_FN_PAIR(Character_IsCollidingWithChar) },
        { "Character::IsCollidingWithObject^1",   API_FN_PAIR(Character_IsCollidingWithObject) },
        { "Character::IsInteractionAvailable^1",  API_FN_PAIR(Character_IsInteractionAvailable) },
        { "Character::LockView^1",                API_FN_PAIR(Character_LockView) },
        { "Character::LockView^2",                API_FN_PAIR(Character_LockViewEx) },
        { "Character::LockViewAligned^4",         API_FN_PAIR(Character_LockViewAlignedEx) },
        { "Character::LockViewAnchored^6",        API_FN_PAIR(Character_LockViewAnchored) },
        { "Character::LockViewFrame^4",           API_FN_PAIR(Character_LockViewFrameEx) },
        { "Character::LockViewOffset^4",          API_FN_PAIR(Character_LockViewOffsetEx) },
        { "Character::LoseInventory^1",           API_FN_PAIR(Character_LoseInventory) },
        { "Character::Move^4",                    API_FN_PAIR(Character_Move) },
        { "Character::MovePath^4",                API_FN_PAIR(Character_MovePath) },
        { "Character::MoveStraight^3",            API_FN_PAIR(Character_MoveStraight) },
        { "Character::PlaceOnWalkableArea^0",     API_FN_PAIR(Character_PlaceOnWalkableArea) },
        { "Character::RemoveTint^0",              API_FN_PAIR(Character_RemoveTint) },
        { "Character::RunFrameEvent^3",           API_FN_PAIR(Character_RunFrameEvent) },
        { "Character::RunInteraction^1",          API_FN_PAIR(Character_RunInteraction) },
        { "Character::Say^101",                   Sc_Character_Say, ScPl_Character_Say },
        // old non-variadic variants
        { "Character::SayAt^4",                   API_FN_PAIR(Character_SayAt) },
        { "Character::SayBackground^1",           API_FN_PAIR(Character_SayBackground) },
        // newer variadic variants
        { "Character::SayAt^104",                 Sc_Character_SayAt, ScPl_Character_SayAt },
        { "Character::SayBackground^101",         Sc_Character_SayBackground, ScPl_Character_SayBackground },
        { "Character::SetAsPlayer^0",             API_FN_PAIR(Character_SetAsPlayer) },
        { "Character::SetIdleView^2",             API_FN_PAIR(Character_SetIdleView) },
        { "Character::SetLightLevel^1",           API_FN_PAIR(Character_SetLightLevel) },
        { "Character::SetWalkSpeed^2",            API_FN_PAIR(Character_SetSpeed) },
        { "Character::StopMoving^0",              API_FN_PAIR(Character_StopMoving) },
        { "Character::Think^101",                 Sc_Character_Think, ScPl_Character_Think },
        { "Character::Tint^5",                    API_FN_PAIR(Character_Tint) },
        { "Character::UnlockView^0",              API_FN_PAIR(Character_UnlockView) },
        { "Character::UnlockView^1",              API_FN_PAIR(Character_UnlockViewEx) },
        { "Character::Walk^4",                    API_FN_PAIR(Character_Walk) },
        { "Character::WalkPath^4",                API_FN_PAIR(Character_WalkPath) },
        { "Character::WalkStraight^3",            API_FN_PAIR(Character_WalkStraight) },
        
        { "Character::get_ActiveInventory",       API_FN_PAIR(Character_GetActiveInventory) },
        { "Character::set_ActiveInventory",       API_FN_PAIR(Character_SetActiveInventory) },
        { "Character::get_Animating",             API_FN_PAIR(Character_GetAnimating) },
        { "Character::get_AnimationSpeed",        API_FN_PAIR(Character_GetAnimationSpeed) },
        { "Character::set_AnimationSpeed",        API_FN_PAIR(Character_SetAnimationSpeed) },
        { "Character::get_AnimationVolume",       API_FN_PAIR(Character_GetAnimationVolume) },
        { "Character::set_AnimationVolume",       API_FN_PAIR(Character_SetAnimationVolume) },
        { "Character::get_Baseline",              API_FN_PAIR(Character_GetBaseline) },
        { "Character::set_Baseline",              API_FN_PAIR(Character_SetBaseline) },
        { "Character::get_BlinkInterval",         API_FN_PAIR(Character_GetBlinkInterval) },
        { "Character::set_BlinkInterval",         API_FN_PAIR(Character_SetBlinkInterval) },
        { "Character::get_BlinkView",             API_FN_PAIR(Character_GetBlinkView) },
        { "Character::set_BlinkView",             API_FN_PAIR(Character_SetBlinkView) },
        { "Character::get_BlinkWhileThinking",    API_FN_PAIR(Character_GetBlinkWhileThinking) },
        { "Character::set_BlinkWhileThinking",    API_FN_PAIR(Character_SetBlinkWhileThinking) },
        { "Character::get_BlockingHeight",        API_FN_PAIR(Character_GetBlockingHeight) },
        { "Character::set_BlockingHeight",        API_FN_PAIR(Character_SetBlockingHeight) },
        { "Character::get_BlockingWidth",         API_FN_PAIR(Character_GetBlockingWidth) },
        { "Character::set_BlockingWidth",         API_FN_PAIR(Character_SetBlockingWidth) },
        { "Character::get_BlockingRectX",         API_FN_PAIR(Character_GetBlockingRectX) },
        { "Character::set_BlockingRectX",         API_FN_PAIR(Character_SetBlockingRectX) },
        { "Character::get_BlockingRectY",         API_FN_PAIR(Character_GetBlockingRectY) },
        { "Character::set_BlockingRectY",         API_FN_PAIR(Character_SetBlockingRectY) },
        { "Character::get_Clickable",             API_FN_PAIR(Character_GetClickable) },
        { "Character::set_Clickable",             API_FN_PAIR(Character_SetClickable) },
        { "Character::get_DestinationX",          API_FN_PAIR(Character_GetDestinationX) },
        { "Character::get_DestinationY",          API_FN_PAIR(Character_GetDestinationY) },
        { "Character::get_DiagonalLoops",         API_FN_PAIR(Character_GetDiagonalWalking) },
        { "Character::set_DiagonalLoops",         API_FN_PAIR(Character_SetDiagonalWalking) },
        { "Character::get_Enabled",               API_FN_PAIR(Character_GetEnabled) },
        { "Character::set_Enabled",               API_FN_PAIR(Character_SetEnabled) },
        { "Character::get_Following",             API_FN_PAIR(Character_GetFollowing) },
        { "Character::get_Frame",                 API_FN_PAIR(Character_GetFrame) },
        { "Character::set_Frame",                 API_FN_PAIR(Character_SetFrame) },
        { "Character::get_ID",                    API_FN_PAIR(Character_GetID) },
        { "Character::get_IdleView",              API_FN_PAIR(Character_GetIdleView) },
        { "Character::get_IdleAnimationDelay",    API_FN_PAIR(Character_GetIdleAnimationDelay) },
        { "Character::set_IdleAnimationDelay",    API_FN_PAIR(Character_SetIdleAnimationDelay) },
        { "Character::geti_InventoryQuantity",    API_FN_PAIR(Character_GetIInventoryQuantity) },
        { "Character::seti_InventoryQuantity",    API_FN_PAIR(Character_SetIInventoryQuantity) },
        { "Character::get_IgnoreLighting",        API_FN_PAIR(Character_GetIgnoreLighting) },
        { "Character::set_IgnoreLighting",        API_FN_PAIR(Character_SetIgnoreLighting) },
        { "Character::get_Loop",                  API_FN_PAIR(Character_GetLoop) },
        { "Character::set_Loop",                  API_FN_PAIR(Character_SetLoop) },
        { "Character::get_ManualScaling",         API_FN_PAIR(Character_GetManualScaling) },
        { "Character::set_ManualScaling",         API_FN_PAIR(Character_SetManualScaling) },
        { "Character::get_MovementLinkedToAnimation",API_FN_PAIR(Character_GetMovementLinkedToAnimation) },
        { "Character::set_MovementLinkedToAnimation",API_FN_PAIR(Character_SetMovementLinkedToAnimation) },
        { "Character::get_Moving",                API_FN_PAIR(Character_GetMoving) },
        { "Character::get_Name",                  API_FN_PAIR(Character_GetName) },
        { "Character::set_Name",                  API_FN_PAIR(Character_SetName) },
        { "Character::get_NormalView",            API_FN_PAIR(Character_GetNormalView) },
        { "Character::get_PreviousRoom",          API_FN_PAIR(Character_GetPreviousRoom) },
        { "Character::get_Room",                  API_FN_PAIR(Character_GetRoom) },
        { "Character::get_ScaleMoveSpeed",        API_FN_PAIR(Character_GetScaleMoveSpeed) },
        { "Character::set_ScaleMoveSpeed",        API_FN_PAIR(Character_SetScaleMoveSpeed) },
        { "Character::get_ScaleVolume",           API_FN_PAIR(Character_GetScaleVolume) },
        { "Character::set_ScaleVolume",           API_FN_PAIR(Character_SetScaleVolume) },
        { "Character::get_Scaling",               API_FN_PAIR(Character_GetScaling) },
        { "Character::set_Scaling",               API_FN_PAIR(Character_SetScaling) },
        { "Character::get_ScriptName",            API_FN_PAIR(Character_GetScriptName) },
        { "Character::get_Solid",                 API_FN_PAIR(Character_GetSolid) },
        { "Character::set_Solid",                 API_FN_PAIR(Character_SetSolid) },
        { "Character::get_Speaking",              API_FN_PAIR(Character_GetSpeaking) },
        { "Character::get_SpeakingFrame",         API_FN_PAIR(Character_GetSpeakingFrame) },
        { "Character::get_SpeechAnimationDelay",  API_FN_PAIR(Character_GetSpeechAnimationDelay) },
        { "Character::set_SpeechAnimationDelay",  API_FN_PAIR(Character_SetSpeechAnimationDelay) },
        { "Character::get_SpeechColor",           API_FN_PAIR(Character_GetSpeechColor) },
        { "Character::set_SpeechColor",           API_FN_PAIR(Character_SetSpeechColor) },
        { "Character::get_SpeechView",            API_FN_PAIR(Character_GetSpeechView) },
        { "Character::set_SpeechView",            API_FN_PAIR(Character_SetSpeechView) },
        { "Character::get_Thinking",              API_FN_PAIR(Character_GetThinking) },
        { "Character::get_ThinkingFrame",         API_FN_PAIR(Character_GetThinkingFrame) },
        { "Character::get_ThinkView",             API_FN_PAIR(Character_GetThinkView) },
        { "Character::set_ThinkView",             API_FN_PAIR(Character_SetThinkView) },
        { "Character::get_Transparency",          API_FN_PAIR(Character_GetTransparency) },
        { "Character::set_Transparency",          API_FN_PAIR(Character_SetTransparency) },
        { "Character::get_TurnBeforeWalking",     API_FN_PAIR(Character_GetTurnBeforeWalking) },
        { "Character::set_TurnBeforeWalking",     API_FN_PAIR(Character_SetTurnBeforeWalking) },
        { "Character::get_TurnWhenFacing",        API_FN_PAIR(Character_GetTurnWhenFacing ) },
        { "Character::set_TurnWhenFacing",        API_FN_PAIR(Character_SetTurnWhenFacing ) },
        { "Character::get_View",                  API_FN_PAIR(Character_GetView) },
        { "Character::get_WalkSpeedX",            API_FN_PAIR(Character_GetWalkSpeedX) },
        { "Character::get_WalkSpeedY",            API_FN_PAIR(Character_GetWalkSpeedY) },
        { "Character::get_X",                     API_FN_PAIR(Character_GetX) },
        { "Character::set_X",                     API_FN_PAIR(Character_SetX) },
        { "Character::get_x",                     API_FN_PAIR(Character_GetX) },
        { "Character::set_x",                     API_FN_PAIR(Character_SetX) },
        { "Character::get_Y",                     API_FN_PAIR(Character_GetY) },
        { "Character::set_Y",                     API_FN_PAIR(Character_SetY) },
        { "Character::get_y",                     API_FN_PAIR(Character_GetY) },
        { "Character::set_y",                     API_FN_PAIR(Character_SetY) },
        { "Character::get_Z",                     API_FN_PAIR(Character_GetZ) },
        { "Character::set_Z",                     API_FN_PAIR(Character_SetZ) },
        { "Character::get_z",                     API_FN_PAIR(Character_GetZ) },
        { "Character::set_z",                     API_FN_PAIR(Character_SetZ) },
        { "Character::get_HasExplicitLight",      API_FN_PAIR(Character_GetHasExplicitLight) },
        { "Character::get_HasExplicitTint",       API_FN_PAIR(Character_GetHasExplicitTint) },
        { "Character::get_LightLevel",            API_FN_PAIR(Character_GetLightLevel) },
        { "Character::get_TintBlue",              API_FN_PAIR(Character_GetTintBlue) },
        { "Character::get_TintGreen",             API_FN_PAIR(Character_GetTintGreen) },
        { "Character::get_TintRed",               API_FN_PAIR(Character_GetTintRed) },
        { "Character::get_TintSaturation",        API_FN_PAIR(Character_GetTintSaturation) },
        { "Character::get_TintLuminance",         API_FN_PAIR(Character_GetTintLuminance) },
        { "Character::get_Visible",               API_FN_PAIR(Character_GetVisible) },
        { "Character::set_Visible",               API_FN_PAIR(Character_SetVisible) },

        { "Character::get_BlendMode",             API_FN_PAIR(Character_GetBlendMode) },
        { "Character::set_BlendMode",             API_FN_PAIR(Character_SetBlendMode) },
        { "Character::get_GraphicAnchorX",        API_FN_PAIR(Character_GetGraphicAnchorX) },
        { "Character::set_GraphicAnchorX",        API_FN_PAIR(Character_SetGraphicAnchorX) },
        { "Character::get_GraphicAnchorY",        API_FN_PAIR(Character_GetGraphicAnchorY) },
        { "Character::set_GraphicAnchorY",        API_FN_PAIR(Character_SetGraphicAnchorY) },
        { "Character::get_GraphicOffsetX",        API_FN_PAIR(Character_GetGraphicOffsetX) },
        { "Character::set_GraphicOffsetX",        API_FN_PAIR(Character_SetGraphicOffsetX) },
        { "Character::get_GraphicOffsetY",        API_FN_PAIR(Character_GetGraphicOffsetY) },
        { "Character::set_GraphicOffsetY",        API_FN_PAIR(Character_SetGraphicOffsetY) },
        { "Character::get_GraphicRotation",       API_FN_PAIR(Character_GetRotation) },
        { "Character::set_GraphicRotation",       API_FN_PAIR(Character_SetRotation) },
        { "Character::get_UseRegionTint",         API_FN_PAIR(Character_GetUseRegionTint) },
        { "Character::set_UseRegionTint",         API_FN_PAIR(Character_SetUseRegionTint) },
        { "Character::get_ViewAnchorX",           API_FN_PAIR(Character_GetViewAnchorX) },
        { "Character::get_ViewAnchorY",           API_FN_PAIR(Character_GetViewAnchorY) },
        { "Character::get_ViewOffsetX",           API_FN_PAIR(Character_GetViewOffsetX) },
        { "Character::get_ViewOffsetY",           API_FN_PAIR(Character_GetViewOffsetY) },

        { "Character::get_FaceDirectionRatio",    API_FN_PAIR(Character_GetFaceDirectionRatio) },
        { "Character::set_FaceDirectionRatio",    API_FN_PAIR(Character_SetFaceDirectionRatio) },
        { "Character::get_MotionPath",            API_FN_PAIR(Character_GetMotionPath) },

        { "Character::get_Shader",                API_FN_PAIR(Character_GetShader) },
        { "Character::set_Shader",                API_FN_PAIR(Character_SetShader) },
    };

    ccAddExternalFunctions(character_api);
}

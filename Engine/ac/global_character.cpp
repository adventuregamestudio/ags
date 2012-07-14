/*
  AGS Character functions

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#include "ac/global_character.h"
#include "wgt2allg.h"
#include "ac/common.h"
#include "ac/view.h"
#include "ac/character.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_overlay.h"
#include "ac/global_translation.h"
#include "ac/object.h"
#include "ac/overlay.h"
#include "ac/properties.h"
#include "ac/roomstruct.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "debug/debug.h"
#include "main/game_run.h"
#include "script/script.h"


extern GameSetupStruct game;
extern ViewStruct*views;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern RoomObject*objs;
extern roomstruct thisroom;
extern GameState play;
extern ScriptObject scrObj[MAX_INIT_SPR];
extern ScriptInvItem scrInv[MAX_INV];
extern int offsetx, offsety;
extern int guis_need_update;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int numscreenover;
extern int scrnwid,scrnhit;

// defined in character unit
extern CharacterExtras *charextra;
extern CharacterInfo*playerchar;
extern long _sc_PlayerCharPtr;
extern CharacterInfo*playerchar;


void StopMoving(int chaa) {

    Character_StopMoving(&game.chars[chaa]);
}

void ReleaseCharacterView(int chat) {
    if (!is_valid_character(chat))
        quit("!ReleaseCahracterView: invalid character supplied");

    Character_UnlockView(&game.chars[chat]);
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

void RunCharacterInteraction (int cc, int mood) {
    if (!is_valid_character(cc))
        quit("!RunCharacterInteraction: invalid character");

    int passon=-1,cdata=-1;
    if (mood==MODE_LOOK) passon=0;
    else if (mood==MODE_HAND) passon=1;
    else if (mood==MODE_TALK) passon=2;
    else if (mood==MODE_USE) { passon=3;
    cdata=playerchar->activeinv;
    play.usedinv=cdata;
    }
    else if (mood==MODE_PICKUP) passon = 5;
    else if (mood==MODE_CUSTOM1) passon = 6;
    else if (mood==MODE_CUSTOM2) passon = 7;

    evblockbasename="character%d"; evblocknum=cc;
    if (game.charScripts != NULL) 
    {
        if (passon>=0)
            run_interaction_script(game.charScripts[cc], passon, 4, (passon == 3));
        run_interaction_script(game.charScripts[cc], 4);  // any click on char
    }
    else 
    {
        if (passon>=0)
            run_interaction_event(game.intrChar[cc],passon, 4, (passon == 3));
        run_interaction_event(game.intrChar[cc],4);  // any click on char
    }
}

int AreCharObjColliding(int charid,int objid) {
    if (!is_valid_character(charid))
        quit("!AreCharObjColliding: invalid character");
    if (!is_valid_object(objid))
        quit("!AreCharObjColliding: invalid object number");

    return Character_IsCollidingWithObject(&game.chars[charid], &scrObj[objid]);
}

int AreCharactersColliding(int cchar1,int cchar2) {
    if (!is_valid_character(cchar1))
        quit("!AreCharactersColliding: invalid char1");
    if (!is_valid_character(cchar2))
        quit("!AreCharactersColliding: invalid char2");

    return Character_IsCollidingWithChar(&game.chars[cchar1], &game.chars[cchar2]);
}

int GetCharacterProperty (int cha, const char *property) {
    if (!is_valid_character(cha))
        quit("!GetCharacterProperty: invalid character");
    return get_int_property (&game.charProps[cha], property);
}

void SetCharacterProperty (int who, int flag, int yesorno) {
    if (!is_valid_character(who))
        quit("!SetCharacterProperty: Invalid character specified");

    Character_SetOption(&game.chars[who], flag, yesorno);
}

void GetCharacterPropertyText (int item, const char *property, char *bufer) {
    get_text_property (&game.charProps[item], property, bufer);
}

int GetCharacterAt (int xx, int yy) {
    xx += divide_down_coordinate(offsetx);
    yy += divide_down_coordinate(offsety);
    return is_pos_on_character(xx,yy);
}

void SetActiveInventory(int iit) {

    ScriptInvItem *tosend = NULL;
    if ((iit > 0) && (iit < game.numinvitems))
        tosend = &scrInv[iit];
    else if (iit != -1)
        quitprintf("!SetActiveInventory: invalid inventory number %d", iit);

    Character_SetActiveInventory(playerchar, tosend);
}

void update_invorder() {
    for (int cc = 0; cc < game.numcharacters; cc++) {
        charextra[cc].invorder_count = 0;
        int ff, howmany;
        // Iterate through all inv items, adding them once (or multiple
        // times if requested) to the list.
        for (ff=0;ff < game.numinvitems;ff++) {
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
    // backwards compatibility
    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;

    guis_need_update = 1;
}

void add_inventory(int inum) {
    if ((inum < 0) || (inum >= MAX_INV))
        quit("!AddInventory: invalid inventory number");

    Character_AddInventory(playerchar, &scrInv[inum], SCR_NO_VALUE);

    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
}

void lose_inventory(int inum) {
    if ((inum < 0) || (inum >= MAX_INV))
        quit("!LoseInventory: invalid inventory number");

    Character_LoseInventory(playerchar, &scrInv[inum]);

    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
}

void AddInventoryToCharacter(int charid, int inum) {
    if (!is_valid_character(charid))
        quit("!AddInventoryToCharacter: invalid character specified");
    if ((inum < 1) || (inum >= game.numinvitems))
        quit("!AddInventory: invalid inv item specified");

    Character_AddInventory(&game.chars[charid], &scrInv[inum], SCR_NO_VALUE);
}

void LoseInventoryFromCharacter(int charid, int inum) {
    if (!is_valid_character(charid))
        quit("!LoseInventoryFromCharacter: invalid character specified");
    if ((inum < 1) || (inum >= game.numinvitems))
        quit("!AddInventory: invalid inv item specified");

    Character_LoseInventory(&game.chars[charid], &scrInv[inum]);
}

void DisplayThought(int chid, const char*texx, ...) {
    if ((chid < 0) || (chid >= game.numcharacters))
        quit("!DisplayThought: invalid character specified");

    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf,get_translation(texx),ap);
    va_end(ap);

    _DisplayThoughtCore(chid, displbuf);
}

void __sc_displayspeech(int chid,char*texx, ...) {
    if ((chid<0) || (chid>=game.numcharacters))
        quit("!DisplaySpeech: invalid character specified");

    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf, get_translation(texx), ap);
    va_end(ap);

    _DisplaySpeechCore(chid, displbuf);

}

// **** THIS IS UNDOCUMENTED BECAUSE IT DOESN'T WORK PROPERLY
// **** AT 640x400 AND DOESN'T USE THE RIGHT SPEECH STYLE
void DisplaySpeechAt (int xx, int yy, int wii, int aschar, char*spch) {
    multiply_up_coordinates(&xx, &yy);
    wii = multiply_up_coordinate(wii);
    _displayspeech (get_translation(spch), aschar, xx, yy, wii, 0);
}

int DisplaySpeechBackground(int charid,char*speel) {
    // remove any previous background speech for this character
    int cc;
    for (cc = 0; cc < numscreenover; cc++) {
        if (screenover[cc].bgSpeechForChar == charid) {
            remove_screen_overlay_index(cc);
            cc--;
        }
    }

    int ovrl=CreateTextOverlay(OVR_AUTOPLACE,charid,scrnwid/2,FONT_SPEECH,
        -game.chars[charid].talkcolor, get_translation(speel));

    int scid = find_overlay_of_type(ovrl);
    screenover[scid].bgSpeechForChar = charid;
    screenover[scid].timeout = GetTextDisplayTime(speel, 1);
    return ovrl;
}

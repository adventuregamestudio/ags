//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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

#include "ac/global_character.h"
#include "ac/common.h"
#include "ac/view.h"
#include "ac/character.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/object.h"
#include "ac/overlay.h"
#include "ac/properties.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "ac/dynobj/cc_character.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "main/game_run.h"
#include "script/script.h"

using namespace AGS::Common;


extern GameSetupStruct game;
extern std::vector<ViewStruct> views;
extern RoomObject*objs;
extern RoomStruct thisroom;
extern GameState play;
extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern ScriptInvItem scrInv[MAX_INV];
extern CCCharacter ccDynamicCharacter;

// defined in character unit
extern CharacterInfo*playerchar;
extern int32_t _sc_PlayerCharPtr;
extern CharacterInfo*playerchar;


void StopMoving(int chaa) {

    Character_StopMoving(&game.chars[chaa]);
}

void ReleaseCharacterView(int chat) {
    if (!is_valid_character(chat))
        quit("!ReleaseCahracterView: invalid character supplied");

    Character_UnlockView(&game.chars[chat]);
}

int GetCharacterWidth(int ww) {
    CharacterInfo *char1 = &game.chars[ww];

    if (charextra[ww].width < 1)
    {
        if ((char1->view < 0) ||
            (char1->loop >= views[char1->view].numLoops) ||
            (char1->frame >= views[char1->view].loops[char1->loop].numFrames))
        {
            debug_script_warn("GetCharacterWidth: Character %s has invalid frame: view %d, loop %d, frame %d", char1->scrname, char1->view + 1, char1->loop, char1->frame);
            return 4;
        }

        return game.SpriteInfos[views[char1->view].loops[char1->loop].frames[char1->frame].pic].Width;
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
            debug_script_warn("GetCharacterHeight: Character %s has invalid frame: view %d, loop %d, frame %d", char1->scrname, char1->view + 1, char1->loop, char1->frame);
            return 2;
        }

        return game.SpriteInfos[views[char1->view].loops[char1->loop].frames[char1->frame].pic].Height;
    }
    else
        return charextra[charid].height;
}

int GetPlayerCharacter() {
    return game.playercharacter;
}

void SetCharacterFrame(int chaa, int view, int loop, int frame) {

    Character_LockViewFrame(&game.chars[chaa], view, loop, frame);
}

void MoveCharacterToHotspot(int chaa,int hotsp) {
    if ((hotsp<0) || (hotsp>=MAX_ROOM_HOTSPOTS))
        quit("!MovecharacterToHotspot: invalid hotspot");
    if (thisroom.Hotspots[hotsp].WalkTo.X<1) return;
    walk_character(chaa,thisroom.Hotspots[hotsp].WalkTo.X,thisroom.Hotspots[hotsp].WalkTo.Y,0, true);

    GameLoopUntilNotMoving(&game.chars[chaa].walking);
}

int GetCharacterSpeechAnimationDelay(CharacterInfo *cha)
{
	if (game.options[OPT_GLOBALTALKANIMSPD] != 0)
		return play.talkanim_speed;
    else
        return cha->speech_anim_speed;
}

void RunCharacterInteraction (int cc, int mood) {
    if (!is_valid_character(cc))
        quit("!RunCharacterInteraction: invalid character");

    // convert cursor mode to event index (in character event table)
    // TODO: probably move this conversion table elsewhere? should be a global info
    int evnt;
    switch (mood)
    {
    case MODE_LOOK: evnt = 0; break;
    case MODE_HAND: evnt = 1; break;
    case MODE_TALK: evnt = 2; break;
    case MODE_USE: evnt = 3; break;
    case MODE_PICKUP: evnt = 5; break;
    case MODE_CUSTOM1: evnt = 6; break;
    case MODE_CUSTOM2: evnt = 7; break;
    default: evnt = -1; break;
    }
    const int anyclick_evt = 4; // TODO: make global constant (character any-click evt)

    // For USE verb: remember active inventory
    if (mood == MODE_USE)
    {
        play.usedinv = playerchar->activeinv;
    }

    const auto obj_evt = ObjectEvent("character%d", cc,
        RuntimeScriptValue().SetScriptObject(&game.chars[cc], &ccDynamicCharacter), mood);
        if ((evnt >= 0) &&
                run_interaction_script(obj_evt, game.charScripts[cc].get(), evnt, anyclick_evt) < 0)
            return; // game state changed, don't do "any click"
        run_interaction_script(obj_evt, game.charScripts[cc].get(), anyclick_evt);  // any click on char
}

int GetCharIDAtScreen(int xx, int yy) {
    VpPoint vpt = play.ScreenToRoom(xx, yy);
    if (vpt.second < 0)
        return -1;
    return is_pos_on_character(vpt.first.X, vpt.first.Y);
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
    GUI::MarkInventoryForUpdate(game.playercharacter, true);
}

// CLNUP still used by run_dialog_script and run_interaction_commandlist, investigate if we could just use Character_AddInventory
void add_inventory(int inum) {
    if ((inum < 0) || (inum >= MAX_INV))
        quit("!AddInventory: invalid inventory number");

    Character_AddInventory(playerchar, &scrInv[inum], SCR_NO_VALUE);
}

// CLNUP still used by run_dialog_script and run_interaction_commandlist, investigate if we could just use Character_LoseInventory
void lose_inventory(int inum) {
    if ((inum < 0) || (inum >= MAX_INV))
        quit("!LoseInventory: invalid inventory number");

    Character_LoseInventory(playerchar, &scrInv[inum]);
}

// CLNUP investigate if I can just removed the following comments
// **** THIS IS UNDOCUMENTED BECAUSE IT DOESN'T WORK PROPERLY
// **** AT 640x400 AND DOESN'T USE THE RIGHT SPEECH STYLE
void DisplaySpeechAt (int xx, int yy, int wii, int aschar, const char*spch) {
    _displayspeech (get_translation(spch), aschar, xx, yy, wii, 0);
}

// [DEPRECATED] left only for use in Display, replace/merge with modern function
static int CreateTextOverlay(int xx, int yy, int wii, int fontid, int text_color, const char* text, int disp_type) {
    int allowShrink = 0;

    if (xx == OVR_AUTOPLACE) // allow DisplaySpeechBackground to be shrunk
        allowShrink = 1;

    auto *over = Overlay_CreateTextCore(false, xx, yy, wii, fontid, text_color, text, disp_type, allowShrink);
    return over ? over->type : 0;
}

// [DEPRECATED] but still used by Character_SayBackground, might merge since there are no other instances
int DisplaySpeechBackground(int charid, const char*speel) {
    // remove any previous background speech for this character
    // TODO: have a map character -> bg speech over?
    const auto &overs = get_overlays();
    for (size_t i = 0; i < overs.size(); ++i)
    {
        if (overs[i].bgSpeechForChar == charid)
        {
            remove_screen_overlay(i);
            break;
        }
    }

    int ovrl=CreateTextOverlay(OVR_AUTOPLACE,charid,play.GetUIViewport().GetWidth()/2,FONT_SPEECH,
        -game.chars[charid].talkcolor, get_translation(speel), DISPLAYTEXT_NORMALOVERLAY);

    auto *over = get_overlay(ovrl);
    over->bgSpeechForChar = charid;
    over->timeout = GetTextDisplayTime(speel, 1);
    return ovrl;
}

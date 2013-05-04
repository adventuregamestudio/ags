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

#include "ac/global_room.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/movelist.h"
#include "ac/properties.h"
#include "ac/room.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "script/script.h"

using AGS::Engine::RoomState;

extern CharacterInfo*playerchar;
extern int displayed_room;
extern int in_enters_screen;
extern int in_leaves_screen;
extern int in_inv_screen, inv_screen_newroom;
extern int gs_to_newroom;

void SetAmbientTint (int red, int green, int blue, int opacity, int luminance) {
    if ((red < 0) || (green < 0) || (blue < 0) ||
        (red > 255) || (green > 255) || (blue > 255) ||
        (opacity < 0) || (opacity > 100) ||
        (luminance < 0) || (luminance > 100))
        quit("!SetTint: invalid parameter. R,G,B must be 0-255, opacity & luminance 0-100");

    DEBUG_CONSOLE("Set ambient tint RGB(%d,%d,%d) %d%%", red, green, blue, opacity);

    play.RoomTintRed = red;
    play.RoomTintGreen = green;
    play.RoomTintBlue = blue;
    play.RoomTintLevel = opacity;
    play.RoomTintLight = (luminance * 25) / 10;
}

void NewRoom(int nrnum) {
    if (nrnum < 0)
        quitprintf("!NewRoom: room change requested to invalid room number %d.", nrnum);

    if (displayed_room < 0) {
        // called from game_start; change the room where the game will start
        playerchar->room = nrnum;
        return;
    }


    DEBUG_CONSOLE("Room change requested to room %d", nrnum);
    EndSkippingUntilCharStops();

    can_run_delayed_command();

    if (play.StopDialogAtEnd != DIALOG_NONE) {
        if (play.StopDialogAtEnd == DIALOG_RUNNING)
            play.StopDialogAtEnd = DIALOG_NEWROOM + nrnum;
        else
            quit("!NewRoom: two NewRoom/RunDialog/StopDialog requests within dialog");
        return;
    }

    if (in_leaves_screen >= 0) {
        // NewRoom called from the Player Leaves Screen event -- just
        // change which room it will go to
        in_leaves_screen = nrnum;
    }
    else if (in_enters_screen) {
        setevent(EV_NEWROOM,nrnum);
        return;
    }
    else if (in_inv_screen) {
        inv_screen_newroom = nrnum;
        return;
    }
    else if ((inside_script==0) & (in_graph_script==0)) {
        new_room(nrnum,playerchar);
        return;
    }
    else if (inside_script) {
        curscript->queue_action(ePSANewRoom, nrnum, "NewRoom");
        // we might be within a MoveCharacterBlocking -- the room
        // change should abort it
        if ((playerchar->walking > 0) && (playerchar->walking < TURNING_AROUND)) {
            // nasty hack - make sure it doesn't move the character
            // to a walkable area
            CharMoveLists[playerchar->walking].direct = 1;
            StopMoving(game.PlayerCharacterIndex);
        }
    }
    else if (in_graph_script)
        gs_to_newroom = nrnum;
}


void NewRoomEx(int nrnum,int newx,int newy) {

    Character_ChangeRoom(playerchar, nrnum, newx, newy);

}

void NewRoomNPC(int charid, int nrnum, int newx, int newy) {
    if (!is_valid_character(charid))
        quit("!NewRoomNPC: invalid character");
    if (charid == game.PlayerCharacterIndex)
        quit("!NewRoomNPC: use NewRoomEx with the player character");

    Character_ChangeRoom(&game.Characters[charid], nrnum, newx, newy);
}

void ResetRoom(int nrnum) {
    if (nrnum == displayed_room)
        quit("!ResetRoom: cannot reset current room");
    if ((nrnum<0) | (nrnum>=MAX_ROOMS))
        quit("!ResetRoom: invalid room number");

    if (AGS::Engine::IsRoomStateValid(nrnum))
    {
        RoomState* roomstat = AGS::Engine::GetRoomState(nrnum);
        if (roomstat->BeenHere)
        {
            roomstat->ScriptData.Free();
            roomstat->ScriptDataSize = 0;
        }
        roomstat->BeenHere = 0;
    }

    DEBUG_CONSOLE("Room %d reset to original state", nrnum);
}

int HasPlayerBeenInRoom(int roomnum) {
    if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
        return 0;
    if (AGS::Engine::IsRoomStateValid(roomnum))
        return AGS::Engine::GetRoomState(roomnum)->BeenHere;
    else
        return 0;
}

void CallRoomScript (int value) {
    can_run_delayed_command();

    if (!inside_script)
        quit("!CallRoomScript: not inside a script???");

    play.RoomScriptFinished = 0;
    RuntimeScriptValue rval_null;
    curscript->run_another("$on_call", RuntimeScriptValue().SetInt32(value), rval_null /*0*/);
}

int HasBeenToRoom (int roomnum) {
    if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
        quit("!HasBeenToRoom: invalid room number specified");

    if (AGS::Engine::IsRoomStateValid(roomnum))
        return AGS::Engine::GetRoomState(roomnum)->BeenHere;
    else
        return 0;
}

int GetRoomProperty (const char *property) {
    return get_int_property (&thisroom.Properties, property);
}

void GetRoomPropertyText (const char *property, char *bufer) {
    get_text_property (&thisroom.Properties, property, bufer);
}

void SetBackgroundFrame(int frnum) {
    if ((frnum<-1) | (frnum>=thisroom.BkgSceneCount))
        quit("!SetBackgrondFrame: invalid frame number specified");
    if (frnum<0) {
        play.RoomBkgFrameLocked=0;
        return;
    }

    play.RoomBkgFrameLocked = 1;

    if (frnum == play.RoomBkgFrameIndex)
    {
        // already on this frame, do nothing
        return;
    }

    play.RoomBkgFrameIndex = frnum;
    on_background_frame_change ();
}

int GetBackgroundFrame() {
    return play.RoomBkgFrameIndex;
}

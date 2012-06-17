/*
  AGS Runtime header

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/
#ifndef __AC_RUNNINGGAME_H
#define __AC_RUNNINGGAME_H

#include "acrun/ac_gamestate.h"
#include "acrun/ac_gamesetup.h"
#include "ac/ac_roomstruct.h"
#include "ac/ac_gamesetupstruct.h"
#include "acrun/ac_roomstatus.h"
#include "ac/ac_move.h"
#include "ac/ac_view.h"
#include "acrun/ac_roomobject.h"
#include "acrun/ac_charactercache.h"
#include "acrun/ac_objectcache.h"
#include "acrun/ac_scriptobject.h"
#include "sprcache.h"

extern GameState play;
extern GameSetup usetup;
extern roomstruct thisroom;
extern GameSetupStruct game;
extern RoomStatus *roomstats;
extern RoomStatus troom;    // used for non-saveable rooms, eg. intro

extern volatile int switching_away_from_game;
extern volatile char want_exit, abort_engine;
extern int loaded_game_file_version;

extern int frames_per_second;

extern int displayed_room,starting_room;

extern MoveList *mls;

extern int in_new_room, new_room_was;  // 1 in new room, 2 first time in new room, 3 loading saved game
extern int new_room_pos;
extern int new_room_x, new_room_y;

extern ViewStruct*views;

extern RoomObject*objs;
extern RoomStatus*croom;

extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];

extern CharacterCache *charcache;
extern ObjectCache objcache[MAX_INIT_SPR];

extern ScriptObject scrObj[MAX_INIT_SPR];
extern ScriptGUI *scrGui;
extern ScriptHotspot scrHotspot[MAX_HOTSPOTS];
extern ScriptRegion scrRegion[MAX_REGIONS];
extern ScriptInvItem scrInv[MAX_INV];
extern ScriptDialog scrDialog[MAX_DIALOG];

extern SpriteCache spriteset;

#endif // __AC_RUNNINGGAME_H
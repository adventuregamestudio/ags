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
#include "ac/ac_room.h"
#include "ac/ac_gamesetupstruct.h"
#include "acrun/ac_roomstatus.h"

extern GameState play;
extern GameSetup usetup;
extern roomstruct thisroom;
extern GameSetupStruct game;
extern RoomStatus *roomstats;
extern RoomStatus troom;    // used for non-saveable rooms, eg. intro

extern volatile int switching_away_from_game;
extern volatile char want_exit, abort_engine;
extern int loaded_game_file_version;

extern int frames_per_second=40;

#endif // __AC_RUNNINGGAME_H
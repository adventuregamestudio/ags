
#include <stdio.h>
#include "wgt2allg.h"
#include "acrun/ac_runninggame.h"

GameState play;
GameSetup usetup;
roomstruct thisroom;
GameSetupStruct game;
RoomStatus *roomstats;
RoomStatus troom;    // used for non-saveable rooms, eg. intro

volatile int switching_away_from_game = 0;
volatile char want_exit = 0, abort_engine = 0;
int loaded_game_file_version = 0;

int frames_per_second=40;

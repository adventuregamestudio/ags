
#include <stdio.h>
#include "wgt2allg.h"
#include "acrun/ac_runninggame.h"

GameState play;
GameSetup usetup;
//roomstruct thisroom;
GameSetupStruct game;
RoomStatus *roomstats;
RoomStatus troom;    // used for non-saveable rooms, eg. intro

volatile int switching_away_from_game = 0;
volatile char want_exit = 0, abort_engine = 0;
int loaded_game_file_version = 0;

int frames_per_second=40;

int displayed_room=-10,starting_room = -1;

MoveList *mls = NULL;

int in_new_room=0, new_room_was = 0;  // 1 in new room, 2 first time in new room, 3 loading saved game
int new_room_pos=0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;

ViewStruct*views=NULL;

RoomObject*objs;
RoomStatus*croom=NULL;

int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];

CharacterCache *charcache = NULL;
ObjectCache objcache[MAX_INIT_SPR];

ScriptObject scrObj[MAX_INIT_SPR];
ScriptGUI *scrGui = NULL;
ScriptHotspot scrHotspot[MAX_HOTSPOTS];
ScriptRegion scrRegion[MAX_REGIONS];
ScriptInvItem scrInv[MAX_INV];
ScriptDialog scrDialog[MAX_DIALOG];

//block spriteset[MAX_SPRITES+1];
//SpriteCache spriteset (MAX_SPRITES+1);
// initially size 1, this will be increased by the initFile function
SpriteCache spriteset(1);

int proper_exit=0,our_eip=0;

GUIMain*guis=NULL;





CCGUIObject ccDynamicGUIObject;
CCCharacter ccDynamicCharacter;
CCHotspot   ccDynamicHotspot;
CCRegion    ccDynamicRegion;
CCInventory ccDynamicInv;
CCGUI       ccDynamicGUI;
CCObject    ccDynamicObject;
CCDialog    ccDynamicDialog;
ScriptString myScriptStringImpl;





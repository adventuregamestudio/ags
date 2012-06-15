
#include "acmain/ac_maindefines.h"


ScriptDrawingSurface* Room_GetDrawingSurfaceForBackground(int backgroundNumber)
{
    if (displayed_room < 0)
        quit("!Room.GetDrawingSurfaceForBackground: no room is currently loaded");

    if (backgroundNumber == SCR_NO_VALUE)
    {
        backgroundNumber = play.bg_frame;
    }

    if ((backgroundNumber < 0) || (backgroundNumber >= thisroom.num_bscenes))
        quit("!Room.GetDrawingSurfaceForBackground: invalid background number specified");


    ScriptDrawingSurface *surface = new ScriptDrawingSurface();
    surface->roomBackgroundNumber = backgroundNumber;
    ccRegisterManagedObject(surface, surface);
    return surface;
}


int Room_GetObjectCount() {
  return croom->numobj;
}

int Room_GetWidth() {
  return thisroom.width;
}

int Room_GetHeight() {
  return thisroom.height;
}

int Room_GetColorDepth() {
  return bitmap_color_depth(thisroom.ebscene[0]);
}

int Room_GetLeftEdge() {
  return thisroom.left;
}

int Room_GetRightEdge() {
  return thisroom.right;
}

int Room_GetTopEdge() {
  return thisroom.top;
}

int Room_GetBottomEdge() {
  return thisroom.bottom;
}

int Room_GetMusicOnLoad() {
  return thisroom.options[ST_TUNE];
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

  if (play.stop_dialog_at_end != DIALOG_NONE) {
    if (play.stop_dialog_at_end == DIALOG_RUNNING)
      play.stop_dialog_at_end = DIALOG_NEWROOM + nrnum;
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
      mls[playerchar->walking].direct = 1;
      StopMoving(game.playercharacter);
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
  if (charid == game.playercharacter)
    quit("!NewRoomNPC: use NewRoomEx with the player character");

  Character_ChangeRoom(&game.chars[charid], nrnum, newx, newy);
}

void ResetRoom(int nrnum) {
  if (nrnum == displayed_room)
    quit("!ResetRoom: cannot reset current room");
  if ((nrnum<0) | (nrnum>=MAX_ROOMS))
    quit("!ResetRoom: invalid room number");
  if (roomstats[nrnum].beenhere) {
    if (roomstats[nrnum].tsdata!=NULL)
      free(roomstats[nrnum].tsdata);
    roomstats[nrnum].tsdata=NULL;
    roomstats[nrnum].tsdatasize=0;
    }
  roomstats[nrnum].beenhere=0;
  DEBUG_CONSOLE("Room %d reset to original state", nrnum);
}

int HasPlayerBeenInRoom(int roomnum) {
  if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
    return 0;
  return roomstats[roomnum].beenhere;
}



void CallRoomScript (int value) {
  can_run_delayed_command();

  if (!inside_script)
    quit("!CallRoomScript: not inside a script???");

  play.roomscript_finished = 0;
  curscript->run_another("$on_call", value, 0);
}


int HasBeenToRoom (int roomnum) {
  if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
    quit("!HasBeenToRoom: invalid room number specified");

  if (roomstats[roomnum].beenhere)
    return 1;
  return 0;
}

int find_highest_room_entered() {
  int qq,fndas=-1;
  for (qq=0;qq<MAX_ROOMS;qq++) {
    if (roomstats[qq].beenhere!=0) fndas=qq;
  }
  // This is actually legal - they might start in room 400 and save
  //if (fndas<0) quit("find_highest_room: been in no rooms?");
  return fndas;
}


void first_room_initialization() {
  starting_room = displayed_room;
  t1 = time(NULL);
  lastcounter=0;
  loopcounter=0;
  mouse_z_was = mouse_z;
}

void check_new_room() {
  // if they're in a new room, run Player Enters Screen and on_event(ENTER_ROOM)
  if ((in_new_room>0) & (in_new_room!=3)) {
    EventHappened evh;
    evh.type = EV_RUNEVBLOCK;
    evh.data1 = EVB_ROOM;
    evh.data2 = 0;
    evh.data3 = 5;
    evh.player=game.playercharacter;
    // make sure that any script calls don't re-call enters screen
    int newroom_was = in_new_room;
    in_new_room = 0;
    play.disabled_user_interface ++;
    process_event(&evh);
    play.disabled_user_interface --;
    in_new_room = newroom_was;
//    setevent(EV_RUNEVBLOCK,EVB_ROOM,0,5);
  }
}

void compile_room_script() {
  ccError = 0;

  roominst = ccCreateInstance(thisroom.compiled_script);

  if ((ccError!=0) || (roominst==NULL)) {
   char thiserror[400];
   sprintf(thiserror, "Unable to create local script: %s", ccErrorString);
   quit(thiserror);
  }

  roominstFork = ccForkInstance(roominst);
  if (roominstFork == NULL)
    quitprintf("Unable to create forked room instance: %s", ccErrorString);

  repExecAlways.roomHasFunction = true;
  getDialogOptionsDimensionsFunc.roomHasFunction = true;
}

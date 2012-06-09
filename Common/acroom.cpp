//
// Implementation from acroom.h that was previously put under CROOM_NOFUNCTIONS
// macro control
//


// MACPORT FIX: endian support
#include "bigend.h"
#include "misc.h"
#define WGT2ALLEGRO_NOFUNCTIONS
#define USE_CLIB
#include "wgt2allg.h"
#include "acroom_func.h"

extern "C" {
	extern FILE *clibfopen(char *, char *);
}


char *croom_h_copyright = "ChrisRoom v2.00 - CRM reader/writer copyright (c) 1995, 1998, 1999 by Chris Jones.";
char *game_file_sig = "Adventure Creator Game File v2";

block backups[5];


/*long cloadcompfile(FILE*outpt,block tobesaved,color*pal,long poot=0) {
  fseek(outpt,poot,SEEK_SET);
  int widt,hit,hh;
  for (hh=0;hh<4;hh++) *tobesaved++=fgetc(outpt);
  tobesaved-=4;
  widt=*tobesaved++;  widt+=(*tobesaved++)*256;
  hit=*tobesaved++; hit+=(*tobesaved++)*256;
  unsigned char* ress=(unsigned char*)malloc(widt+1);
  for (int ww=0;ww<hit;ww++) {
    cunpackbitl(ress,widt,outpt);
    for (int ss=0;ss<widt;ss++)  (*tobesaved++)=ress[ss];
    }
  for (ww=0;ww<256;ww++) {
    pal[ww].r=fgetc(outpt);
    pal[ww].g=fgetc(outpt);
    pal[ww].b=fgetc(outpt);
    }
  poot=ftell(outpt); free(ress); tobesaved-=(widt*hit+4);
  return poot;
  }*/


// ** SCHEMA LOAD/SAVE FUNCTIONS
void CustomPropertySchema::Serialize (FILE *outto) {
  putw (1, outto);  // version 1 at present
  putw (numProps, outto);
  for (int jj = 0; jj < numProps; jj++) {
    fputstring (propName[jj], outto);
    fputstring (propDesc[jj], outto);
    fputstring (defaultValue[jj], outto);
    putw (propType[jj], outto);
  }

}

int CustomPropertySchema::UnSerialize (FILE *infrom) {
  if (getw(infrom) != 1)
    return -1;
  numProps = getw(infrom);
  for (int kk = 0; kk < numProps; kk++) {
    this->resetProperty (kk);
    fgetstring_limit (propName[kk], infrom, 20);
    fgetstring_limit (propDesc[kk], infrom, 100);
    fgetstring_limit (defaultValue[kk], infrom, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    propType[kk] = getw(infrom);
  }

  return 0;
}

// ** OBJECT PROPERTIES LOAD/SAVE FUNCTIONS
void CustomProperties::Serialize (FILE *outto) {
  putw (1, outto);
  putw (numProps, outto);
  for (int ee = 0; ee < numProps; ee++) {
    fputstring (propName[ee], outto);
    fputstring (propVal[ee], outto);
  }
}

int CustomProperties::UnSerialize (FILE *infrom) {
  if (getw(infrom) != 1)
    return -1;
  numProps = getw(infrom);
  for (int ee = 0; ee < numProps; ee++) {
    propName[ee] = (char*)malloc(200);
    propVal[ee] = (char*)malloc(MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    fgetstring_limit (propName[ee], infrom, 200);
    fgetstring_limit (propVal[ee], infrom, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
  }
  return 0;
}

int in_interaction_editor = 0;

void WordsDictionary::sort () {
  int aa, bb;
  for (aa = 0; aa < num_words; aa++) {
    for (bb = aa + 1; bb < num_words; bb++) {
      if (((wordnum[aa] == wordnum[bb]) && (stricmp(word[aa], word[bb]) > 0))
          || (wordnum[aa] > wordnum[bb])) {
        short temp = wordnum[aa];
        char tempst[30];

        wordnum[aa] = wordnum[bb];
        wordnum[bb] = temp;
        strcpy(tempst, word[aa]);
        strcpy(word[aa], word[bb]);
        strcpy(word[bb], tempst);
        bb = aa;
      }
    }
  }
}

int WordsDictionary::find_index (const char*wrem) {
  int aa;
  for (aa = 0; aa < num_words; aa++) {
    if (stricmp (wrem, word[aa]) == 0)
      return aa;
  }
  return -1;
}


// {name, flags, numArgs, {argTypes}, {argNames}, description, textscript}
ActionTypes actions[NUM_ACTION_TYPES] = {
  {"Do nothing", 0, 0, {NULL}, {NULL}, "Does nothing.", ""},
  {"Run script", AFLG_RUNSCRIPT, 0, {NULL}, {NULL}, "Runs a text script. Click the 'Edit Script' button to modify the script.", ""},
  {"Game - Add score on first execution", 0, 1, {ARG_TYPE_INT}, {"Points to add"},
      "Gives the player $$1 extra points the first time this action is run.",
      ""},
  {"Game - Add score", 0, 1, {ARG_TYPE_INT}, {"Points to add"},
      "Gives the player $$1 extra points every time this action is run.",
      "GiveScore($$1);"},
  {"Game - Display a message", AFLG_MESSAGE, 1, {ARG_TYPE_MSG}, {"Message number"},
      "Displays message $$1 to the player.",
      "DisplayMessage($$1);"},
  {"Game - Play music", 0, 1, {ARG_TYPE_INT}, {"Music number"},
      "Changes the current background music to MUSIC$$1.MP3, WAV, MID or MOD",
      "PlayMusic($$1);"},
  {"Game - Stop music", 0, 0, {NULL}, {NULL},
      "Stops the currently playing background music.",
      "StopMusic();"},
  {"Game - Play sound effect", 0, 1, {ARG_TYPE_INT}, {"Sound number"},
      "Plays SOUND$$1.MP3 or SOUND$$1.WAV",
      "PlaySound($$1);"},
  {"Game - Play Flic animation", 0, 2, {ARG_TYPE_INT, ARG_TYPE_BOOL}, {"Flic number", "Player can skip"},
      "Plays FLIC$$1.FLC or FLIC$$1.FLI",
      "PlayFlic($$1, $$2);"},
  {"Game - Run dialog", 0, 1, {ARG_TYPE_INT}, {"Dialog topic number"},
      "Starts a conversation using dialog topic $$1.",
      "dialog[$$1].Start();"},
  {"Game - Enable dialog option", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Dialog topic number", "Option number"},
      "Enables dialog option $$2 in topic $$1 to be visible to the player.",
      "dialog[$$1].SetOptionState($$2, eOptionOn);"},
  {"Game - Disable dialog option", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Dialog topic number", "Option number"},
      "Stops dialog option $$2 in topic $$1 from being visible to the player.",
      "dialog[$$1].SetOptionState($$2, eOptionOff);"},
  {"Player - Go to a different room", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"New room number", "Edge+offset value"},
      "Takes the player to room $$1, optionally choosing position using old-style Data column value $$2",
      "player.ChangeRoom($$1);"},
  {"Player - Give the player an inventory item", 0, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Adds inventory item $$1 to the player character's inventory.",
      "player.AddInventory(inventory[$$1]);"},
  {"Object - Move object", 0, 5, {ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_BOOL},
      {"Object number", "Destination X location", "Destination Y location", "Move speed", "Wait for move to finish"},
      "Starts object $$1 moving towards ($$2, $$3), at speed $$4.",
      "object[$$1].Move($$2, $$3, $$4);"},
  {"Object - Remove an object from the room", 0, 1, {ARG_TYPE_INT}, {"Object number"},
      "Switches room object $$1 off so the player cannot see or interact with it.",
      "object[$$1].Visible = false;"},
  {"Object - Switch an object back on", 0, 1, {ARG_TYPE_INT}, {"Object number"},
      "Switches room object $$1 on, so that the player can see and interact with it.",
      "object[$$1].Visible = true;"},
  {"Object - Set object view number", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Object number", "New view number"},
      "Changes object $$1's view number to $$2",
      "object[$$1].SetView($$2);"},
  {"Object - Start object animating", 0, 4, {ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_BOOL},
      {"Object number", "Loop number", "Speed", "Repeat"},
      "Starts object $$1 animating, using loop $$2 of its current view, and animating at speed $$3.",
      "object[$$1].Animate($$2, $$3);"},
  {"Character - Move character", 0, 4, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_BOOL},
      {"Character", "Destination X location", "Destination Y location", "Wait for move to finish"},
      "Starts character $$1 moving towards ($$2, $$3).",
      "character[$$1].Walk($$2, $$3);"},
  {"Conditional - If inventory item was used", AFLG_COND | AFLG_INVCHECK, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Performs child actions if the player just used inventory item $$1 on this interaction.",
      "if (player.ActiveInventory == inventory[$$1]) {"},
  {"Conditional - If the player has an inventory item", AFLG_COND, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Performs child actions if the player character has inventory item $$1 in their current inventory.",
      "if (player.InventoryQuantity[$$1] > 0) {"},
  {"Conditional - If a character is moving", AFLG_COND, 1, {ARG_TYPE_CHAR}, {"Character number"},
      "Performs child actions if character $$1 is currently moving",
      "if (character[$$1].Moving) {"},
  {"Conditional - If a variable is set to a certain value", AFLG_COND, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Variable", "Value"},
      "Performs child actions if $$1 == $$2",
      "if (GetGraphicalVariable(\"$$1\") == $$2) { "},
  {"Character - Stop character walking", 0, 1, {ARG_TYPE_CHAR}, {"Character"},
      "Immediately stops character $$1 from moving.",
      "character[$$1].StopMoving();"},
  {"Player - Go to a different room (at specific co-ordinates)", 0, 3, {ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT},
      {"New room number", "X co-ordinate", "Y co-ordinate"},
      "Takes the player to room $$1, and places him at ($$2, $$3)",
      "player.ChangeRoom($$1, $$2, $$3);"},
  {"Character - Move NPC to different room", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_INT},
      {"Character", "New room number"},
      "Places non-player character $$1 into room $$2.",
      "character[$$1].ChangeRoom($$2);"},
  {"Character - Set character view", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_INT},
      {"Character", "View number"},
      "Locks character $$1's view to view $$2, in preparation for doing an animation. Use 'Release Character View' afterwards to release it.",
      "character[$$1].LockView($$2);"},
  {"Character - Release character view", 0, 1, {ARG_TYPE_CHAR}, {"Character"},
      "Reverts character $$1's view back to its normal view and enables standard engine processing.",
      "character[$$1].UnlockView();"},
  {"Character - Follow another character", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_CHAR},
      {"Character", "Follow Character"},
      "Makes character $$1 follow $$2 around the screen.",
      "character[$$1].FollowCharacter($$2);"},
  {"Character - Stop following", 0, 1, {ARG_TYPE_CHAR}, {"Character"},
      "Stops character $$1 following any other characters.",
      "character[$$1].FollowCharacter(null);"},
  {"Room - Disable hotspot", 0, 1, {ARG_TYPE_INT}, {"Hotspot number"},
      "Disables hotspot $$1 in the current room.",
      "hotspot[$$1].Enabled = false;"},
  {"Room - Enable hotspot", 0, 1, {ARG_TYPE_INT}, {"Hotspot number"},
      "Re-enables hotspot $$1 in the current room.",
      "hotspot[$$1].Enabled = true;"},
  {"Game - Set variable value", 0, 2, {ARG_TYPE_VARONLY, ARG_TYPE_INT}, {"Variable", "New value"},
      "Sets variable $$1 to have the value $$2",
      "SetGraphicalVariable(\"$$1\", $$2);"},
  {"Character - Run animation loop", 0, 3, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "Loop number", "Speed"},
      "Runs character $$1 through loop $$2 of its current view, animating at speed $$3. Waits for animation to finish before continuing.",
      ""},  
  {"Character - Quick animation", 0, 4, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "View number", "Loop number", "Speed"},
      "Does SetCharacterView($$1, $$2), AnimateCharacter($$1, $$3, $$4), ReleaseCharacterView($$1) in order.",
      ""},
  {"Character - Set idle animation", 0, 3, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "View number", "Delay"},
      "Sets character $$1 to use view $$2 as its idle animation, with a timeout of $$3 seconds of inactivity before the animation is played.",
      "character[$$1].SetIdleView($$2, $$3);"},
  {"Character - Disable idle animation", 0, 1, {ARG_TYPE_CHAR},
      {"Character"},
      "Disables character $$1's idle animation, so it will no longer be played.",
      "character[$$1].SetIdleView(-1, -1);"},
  {"Player - Remove an item from the inventory", 0, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Removes inventory item $$1 from the player character's inventory.",
      "player.LoseInventory(inventory[$$1]);"},
  {"Game - Show GUI", 0, 1, {ARG_TYPE_INT}, {"GUI number"},
      "Switches on GUI number $$1 so the player can see it.",
      "gui[$$1].Visible = true;"},
  {"Game - Hide GUI", 0, 1, {ARG_TYPE_INT}, {"GUI number"},
      "Switches off GUI number $$1 so the player can no longer see it.",
      "gui[$$1].Visible = false;"},
  {"Stop running more commands", 0, 0, {NULL}, {NULL},
      "Stops running the interaction list at this point. Useful at the end of a block of actions inside a conditional.",
      "return;"},
  {"Character - Face location", 0, 3, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "X co-ordinate", "Y co-ordinate"},
      "Turns character $$1 so that they are facing the room co-ordinates ($$2, $$3).",
      "character[$$1].FaceLocation($$2, $$3);"},
  {"Game - Pause command processor for a set time", 0, 1, {ARG_TYPE_INT},
      {"Loops to wait"},
      "Stops processing actions here and lets the game continue execution for $$1 game loops (default 40 per second) before continuing with the next command.",
      "Wait($$1);"},
  {"Character - Change character view", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_INT},
      {"Character", "New view number"},
      "Changes character $$1's normal walking view to View $$2 permanently, until you call this command again.",
      "character[$$1].ChangeView($$2);"},
  {"Conditional - If the player character is", AFLG_COND, 1, {ARG_TYPE_CHAR},
      {"Character"},
      "Performs child actions if the player character is currently $$1. Useful in games where the player can control multiple characters.",
      "if (player.ID == $$1) {"},
  {"Conditional - If mouse cursor mode is", AFLG_COND, 1, {ARG_TYPE_INT},
      {"Mouse cursor"},
      "Performs child actions if the current cursor mode is mode $$1 (from the Cursors pane).",
      "if (mouse.Mode == $$1) {"},
  {"Conditional - If the player has been in room", AFLG_COND, 1, {ARG_TYPE_INT},
      {"Room number"},
      "Performs child actions if the player has been to room $$1 during the game.",
	  "if (HasPlayerBeenInRoom($$1)) {"}
};
InteractionVariable globalvars[MAX_GLOBAL_VARIABLES] = {{"Global 1", 0, 0}};
int numGlobalVars = 1;

void serialize_command_list (NewInteractionCommandList *nicl, FILE*ooo) {
  if (nicl == NULL)
    return;
  putw (nicl->numCommands, ooo);
  putw (nicl->timesRun, ooo);
#ifndef ALLEGRO_BIG_ENDIAN
  fwrite (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands, ooo);
#else
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
    nicl->command[iteratorCount].WriteToFile(ooo);
  }
#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL)
      serialize_command_list (nicl->command[k].get_child_list(), ooo);
  }
}

void serialize_new_interaction (NewInteraction *nint, FILE*ooo) {
  int a;

  putw (1, ooo);  // Version
  putw (nint->numEvents, ooo);
  fwrite (&nint->eventTypes[0], sizeof(int), nint->numEvents, ooo);
  for (a = 0; a < nint->numEvents; a++)
    putw ((int)nint->response[a], ooo);

  for (a = 0; a < nint->numEvents; a++) {
    if (nint->response[a] != NULL)
      serialize_command_list (nint->response[a], ooo);
  }
}

NewInteractionCommandList *deserialize_command_list (FILE *ooo) {
  NewInteractionCommandList *nicl = new NewInteractionCommandList;
  nicl->numCommands = getw(ooo);
  nicl->timesRun = getw(ooo);
#ifndef ALLEGRO_BIG_ENDIAN
  fread (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands, ooo);
#else
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
    nicl->command[iteratorCount].ReadFromFile(ooo);
  }
#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL) {
      nicl->command[k].children = deserialize_command_list (ooo);
    }
    nicl->command[k].parent = nicl;
  }
  return nicl;
}

NewInteraction *nitemp;
NewInteraction *deserialize_new_interaction (FILE *ooo) {
  int a;

  if (getw(ooo) != 1)
    return NULL;
  nitemp = new NewInteraction;
  nitemp->numEvents = getw(ooo);
  if (nitemp->numEvents > MAX_NEWINTERACTION_EVENTS) {
    quit("Error: this interaction was saved with a newer version of AGS");
    return NULL;
  }
  fread (&nitemp->eventTypes[0], sizeof(int), nitemp->numEvents, ooo);
  //fread (&nitemp->response[0], sizeof(void*), nitemp->numEvents, ooo);
  for (a = 0; a < nitemp->numEvents; a++)
    nitemp->response[a] = (NewInteractionCommandList*)getw(ooo);

  for (a = 0; a < nitemp->numEvents; a++) {
    if (nitemp->response[a] != NULL)
      nitemp->response[a] = deserialize_command_list (ooo);
    nitemp->timesRun[a] = 0;
  }
  return nitemp;
}

void NewInteractionCommandList::reset () {
  int j;
  for (j = 0; j < numCommands; j++) {
    if (command[j].children != NULL) {
      // using this Reset crashes it for some reason
      //command[j].reset ();
      command[j].get_child_list()->reset();
      delete command[j].children;
      command[j].children = NULL;
    }
    command[j].remove();
  }
  numCommands = 0;
  timesRun = 0;
}

int ff;
/*void roomstruct::allocall() {
  // These all get recreated when a room is loaded anyway
  walls = create_bitmap_ex(8, 320, 200);
  object = create_bitmap_ex(8, 320, 200);
  lookat = create_bitmap_ex(8, 320, 200);
  bscene = create_bitmap_ex(8, 320, 200);
  shading = create_bitmap_ex(8, 320, 200);

  if (shading == NULL)
    quit("roomstruct::allocall: out of memory");

 //  printf("Before %ld\n",farcoreleft());
  for (ff=0;ff<5;ff++) { //backups[ff]=wnewblock(0,0,319,199);
    backups[ff]=wallocblock(320,200);
//    printf("%d ",ff); if (kbhit()) break;
    if (backups[ff]==NULL) quit("ROOM.C, AllocMem: Out of memory"); }
  walls=::backups[0];  // this is because blocks in a struct don't work
  object=::backups[1]; // properly
  lookat=::backups[2];
  bscene=::backups[3];
  shading=::backups[4];
//  printf("After %ld\n",farcoreleft());
  }
*/
void roomstruct::freemessage() {
  for (int f = 0; f < nummes; f++) {
    if (message[f] != NULL)
      free(message[f]);
  }
}

/*void roomstruct::freeall() {
//  for (int f=0;f<4;f++) wfreeblock(::backups[f]);
  wfreeblock(walls);
  wfreeblock(lookat);
  wfreeblock(ebscene[0]);
  wfreeblock(object);
  
  if (shading != NULL)
    wfreeblock(shading);

  freemessage();
}*/

/*void roomstruct::freeall() { wfreeblock(walls); wfreeblock(bscene);
  wfreeblock(object); wfreeblock(lookat);
  for (int f=0;f<nummes;f++) if (message[f]!=NULL) free(message[f]); }*/



int _acroom_bpp = 1;  // bytes per pixel of currently loading room

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(BITMAP*bmpt) {
  if (bitmap_color_depth(bmpt) == 15)
    return 2;

  return bitmap_color_depth(bmpt) / 8;
}



#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)
char *lztempfnm = "~aclzw.tmp";

long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe) {
  FILE  *ooo, *iii;
  long  fll, toret, gobacto;

  ooo = ci_fopen(lztempfnm, "wb");
  putw(bmpp->w * bmp_bpp(bmpp), ooo);
  putw(bmpp->h, ooo);
  fwrite(&bmpp->line[0][0], bmpp->w * bmp_bpp(bmpp), bmpp->h, ooo);
  fclose(ooo);

  iii = ci_fopen(fnn, "r+b");
  fseek(iii, offe, SEEK_SET);

  ooo = ci_fopen(lztempfnm, "rb");
  fll = filelength(fileno(ooo));
  fwrite(&pall[0], sizeof(color), 256, iii);
  fwrite(&fll, 4, 1, iii);
  gobacto = ftell(iii);

  // reserve space for compressed size
  fwrite(&fll, 4, 1, iii);
  lzwcompress(ooo, iii);
  toret = ftell(iii);
  fseek(iii, gobacto, SEEK_SET);
  fll = (toret - gobacto) - 4;
  fwrite(&fll, 4, 1, iii);      // write compressed size
  fclose(ooo);
  fclose(iii);
  unlink(lztempfnm);

  return toret;
}

BITMAP *recalced;
/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff) {
  recalced=bmm;
  FILE*iii=clibfopen(fnn,"rb");
  fseek(iii,ooff,SEEK_SET);*/

long load_lzw(FILE *iii, BITMAP *bmm, color *pall) {
  long          uncompsiz, *loptr;
  unsigned char *membuffer;
  int           arin;

  recalced = bmm;
  // MACPORT FIX (HACK REALLY)
  fread(&pall[0], 1, sizeof(color)*256, iii);
  fread(&maxsize, 4, 1, iii);
  fread(&uncompsiz,4,1,iii);

  uncompsiz += ftell(iii);
  outbytes = 0; putbytes = 0;

  update_polled_stuff_if_runtime();
  membuffer = lzwexpand_to_mem(iii);
  update_polled_stuff_if_runtime();

  loptr = (long *)&membuffer[0];
  membuffer += 8;
#ifdef ALLEGRO_BIG_ENDIAN
  loptr[0] = __int_swap_endian(loptr[0]);
  loptr[1] = __int_swap_endian(loptr[1]);
  int bitmapNumPixels = loptr[0]*loptr[1]/_acroom_bpp;
  switch (_acroom_bpp) // bytes per pixel!
  {
    case 1:
    {
      // all done
      break;
    }
    case 2:
    {
      short *sp = (short *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        sp[i] = __short_swap_endian(sp[i]);
      }
      // all done
      break;
    }
    case 4:
    {
      int *ip = (int *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        ip[i] = __int_swap_endian(ip[i]);
      }
      // all done
      break;
    }
  }
#endif // ALLEGRO_BIG_ENDIAN

  if (bmm!=NULL)
    destroy_bitmap(bmm);

  update_polled_stuff_if_runtime();

  bmm = create_bitmap_ex(_acroom_bpp * 8, (loptr[0] / _acroom_bpp), loptr[1]);
  if (bmm == NULL)
    quit("!load_room: not enough memory to load room background");

  update_polled_stuff_if_runtime();

  acquire_bitmap (bmm);
  recalced = bmm;

  for (arin = 0; arin < loptr[1]; arin++)
    memcpy(&bmm->line[arin][0], &membuffer[arin * loptr[0]], loptr[0]);

  release_bitmap (bmm);

  update_polled_stuff_if_runtime();

  free(membuffer-8);

  if (ftell(iii) != uncompsiz)
    fseek(iii, uncompsiz, SEEK_SET);

  update_polled_stuff_if_runtime();

  return uncompsiz;
}

long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo) {
  unsigned char *wgtbl = (unsigned char *)malloc(bmpp->w * bmpp->h + 4);
  short         *sss = (short *)wgtbl;
  long          toret;

  sss[0] = bmpp->w;
  sss[1] = bmpp->h;

  memcpy(&wgtbl[4], &bmpp->line[0][0], bmpp->w * bmpp->h);

  toret = csavecompressed(fnn, wgtbl, pall, ooo);
  free(wgtbl);
  return toret;
}

long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo) {
  short widd,hitt;
  int   ii;

  BITMAP *bim = *bimpp;
  if (bim != NULL)
    destroy_bitmap(bim);

  fread(&widd,2,1,fpp);
  fread(&hitt,2,1,fpp);
  bim = create_bitmap_ex(8, widd, hitt);
  if (bim == NULL)
    quit("!load_room: not enough memory to decompress masks");
  *bimpp = bim;

  for (ii = 0; ii < hitt; ii++) {
    cunpackbitl(&bim->line[ii][0], widd, fpp);
    if (ii % 20 == 0)
      update_polled_stuff_if_runtime();
  }

  fseek(fpp, 768, SEEK_CUR);  // skip palette

  return ftell(fpp);
}
#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER



char *passwencstring = "Avis Durgan";


void decrypt_text(char*toenc) {
  int adx = 0;

  while (1) {
    toenc[0] -= passwencstring[adx];
    if (toenc[0] == 0)
      break;

    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void read_string_decrypt(FILE *ooo, char *sss) {
  int newlen = getw(ooo);
  if ((newlen < 0) || (newlen > 5000000))
    quit("ReadString: file is corrupt");

  // MACPORT FIX: swap as usual
  fread(sss, sizeof(char), newlen, ooo);
  sss[newlen] = 0;
  decrypt_text(sss);
}

void read_dictionary (WordsDictionary *dict, FILE *writeto) {
  int ii;

  dict->allocate_memory(getw(writeto));
  for (ii = 0; ii < dict->num_words; ii++) {
    read_string_decrypt (writeto, dict->word[ii]);
    fread(&dict->wordnum[ii], sizeof(short), 1, writeto);
  }
}

void freadmissout(short *pptr, FILE *opty) {
  fread(&pptr[0], 2, 5, opty);
  fread(&pptr[7], 2, NUM_CONDIT - 7, opty);
  pptr[5] = pptr[6] = 0;
}

void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr) {
  evpt->list[evpt->numcmd] = evnt;
  evpt->respond[evpt->numcmd] = whatac;
  evpt->respondval[evpt->numcmd] = val1;
  evpt->data[evpt->numcmd] = data;
  evpt->score[evpt->numcmd] = scorr;
  evpt->numcmd++;
}

int usesmisccond = 0;

void deserialize_interaction_scripts(FILE *iii, InteractionScripts *scripts)
{
  int numEvents = getw(iii);
  if (numEvents > MAX_NEWINTERACTION_EVENTS)
    quit("Too many interaction script events");
  scripts->numEvents = numEvents;

  char buffer[200];
  for (int i = 0; i < numEvents; i++)
  {
    fgetstring_limit(buffer, iii, sizeof(buffer));
    scripts->scriptFuncNames[i] = new char[strlen(buffer) + 1];
    strcpy(scripts->scriptFuncNames[i], buffer);
  }
}

void load_main_block(roomstruct *rstruc, char *files, FILE *opty, room_file_header rfh) {
  int   f, gsmod, NUMREAD;
  char  buffre[3000];
  long  tesl;

  usesmisccond = 0;
  rstruc->width = 320;
  rstruc->height = 200;
  rstruc->resolution = 1;
  rstruc->numwalkareas = 0;
  rstruc->numhotspots = 0;

  memset(&rstruc->shadinginfo[0], 0, sizeof(short) * 16);
  memset(&rstruc->sprs[0], 0, sizeof(sprstruc) * MAX_INIT_SPR);
  memset(&rstruc->objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
  memset(&rstruc->objectFlags[0], 0, sizeof(short) * MAX_INIT_SPR);
  memset(&rstruc->hswalkto[0], 0, sizeof(_Point) * MAX_HOTSPOTS);
  memset(&rstruc->walk_area_zoom[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
  memset(&rstruc->walk_area_light[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));

  for (f = 0; f < MAX_HOTSPOTS; f++) {
    rstruc->hotspotScriptNames[f][0] = 0;
	if (rstruc->hotspotnames[f] != NULL)
		free(rstruc->hotspotnames[f]);

	rstruc->hotspotnames[f] = (char*)malloc(20);
    sprintf(rstruc->hotspotnames[f], "Hotspot %d", f);

    if (f == 0)
      strcpy(rstruc->hotspotnames[f], "No hotspot");
  }

/*  memset(&rstruc->hscond[0], 0, sizeof(EventBlock) * MAX_HOTSPOTS);
  memset(&rstruc->objcond[0], 0, sizeof(EventBlock) * MAX_INIT_SPR);
  memset(&rstruc->misccond, 0, sizeof(EventBlock));*/

  if (rfh.version >= 12)
    _acroom_bpp = getw(opty);
  else
    _acroom_bpp = 1;

  if (_acroom_bpp < 1)
    _acroom_bpp = 1;

  rstruc->bytes_per_pixel = _acroom_bpp;
  fread(&rstruc->numobj, 2, 1, opty);
  if (rstruc->numobj > MAX_OBJ)
    quit("!room newer than this version - too many walk-behinds");

  NUMREAD = NUM_CONDIT;
  fread(&rstruc->objyval[0], 2, rstruc->numobj, opty);

  gsmod = 0;

  fread(&rstruc->numhotspots, sizeof(int), 1, opty);
  if (rstruc->numhotspots == 0)
    rstruc->numhotspots = 20;
	if (rstruc->numhotspots > MAX_HOTSPOTS)
		quit("room has too many hotspots: need newer version of AGS?");

#ifndef ALLEGRO_BIG_ENDIAN
    fread(&rstruc->hswalkto[0], sizeof(_Point), rstruc->numhotspots, opty);
#else
    // Points are a pair of shorts
    fread(&rstruc->hswalkto[0], sizeof(short), 2*rstruc->numhotspots, opty);
#endif

	for (f = 0; f < rstruc->numhotspots; f++)
	{
		free(rstruc->hotspotnames[f]);
		if (rfh.version >= 28)
		{
			fgetstring_limit(buffre, opty, 2999);
			rstruc->hotspotnames[f] = (char*)malloc(strlen(buffre) + 1);
			strcpy(rstruc->hotspotnames[f], buffre);
		}
		else
		{
			rstruc->hotspotnames[f] = (char*)malloc(30);
			fread(rstruc->hotspotnames[f], 30, 1, opty);
		}
	}

  if (rfh.version >= 24)
    fread(&rstruc->hotspotScriptNames[0], MAX_SCRIPT_NAME_LEN, rstruc->numhotspots, opty);
    
  fread(&rstruc->numwalkareas, 4, 1, opty);
  // MACPORT FIX: read polypoints
#ifndef ALLEGRO_BIG_ENDIAN
  fread(&rstruc->wallpoints[0], sizeof(PolyPoints), rstruc->numwalkareas, opty);
#else
  for (int iteratorCount = 0; iteratorCount < rstruc->numwalkareas; ++iteratorCount)
  {
    rstruc->wallpoints[iteratorCount].ReadFromFile(opty);
  }
#endif
  
  update_polled_stuff_if_runtime();

  fread(&rstruc->top, 2, 1, opty);
  fread(&rstruc->bottom, 2, 1, opty);
  fread(&rstruc->left, 2, 1, opty);
  fread(&rstruc->right, 2, 1, opty);

  fread(&rstruc->numsprs, 2, 1, opty);
  // MACPORT FIX: read sprstrucs
#ifndef ALLEGRO_BIG_ENDIAN
  fread(&rstruc->sprs[0], sizeof(sprstruc), rstruc->numsprs, opty);
#else
  for (int iteratorCount = 0; iteratorCount < rstruc->numsprs; ++iteratorCount)
  {
    rstruc->sprs[iteratorCount].ReadFromFile(opty);
  }
#endif

  if (rfh.version >= 19) {
    rstruc->numLocalVars = getw(opty);
    if (rstruc->numLocalVars > 0) {
      rstruc->localvars = (InteractionVariable*)malloc (sizeof(InteractionVariable) * rstruc->numLocalVars);
#ifndef ALLEGRO_BIG_ENDIAN
      fread (&rstruc->localvars[0], sizeof(InteractionVariable), rstruc->numLocalVars, opty);
#else
      for (int iteratorCount = 0; iteratorCount < rstruc->numLocalVars; ++iteratorCount)
      {
        rstruc->localvars[iteratorCount].ReadFromFile(opty);
      }
#endif
    }
  }
  
  rstruc->numRegions = 0;

  if (rfh.version >= 15) {
    if ((rstruc->numhotspots > MAX_HOTSPOTS) || (rstruc->numsprs > MAX_INIT_SPR))
      quit("load_room: room file created with newer version (too many hotspots/objects)");

    // free all of the old interactions
    for (f = 0; f < MAX_HOTSPOTS; f++) {
      if (rstruc->intrHotspot[f] != NULL) {
        delete rstruc->intrHotspot[f];
        rstruc->intrHotspot[f] = NULL;
      }

	  if (rfh.version < 26) 
	  {
		  if (f < rstruc->numhotspots)
			rstruc->intrHotspot[f] = deserialize_new_interaction (opty);
		  else
			rstruc->intrHotspot[f] = new NewInteraction();
	  }
    }

    for (f = 0; f < MAX_INIT_SPR; f++) {
      if (rstruc->intrObject[f] != NULL) {
        delete rstruc->intrObject[f];
        rstruc->intrObject[f] = NULL;
      }

	  if (rfh.version < 26) 
	  {
		  if (f < rstruc->numsprs)
			rstruc->intrObject[f] = deserialize_new_interaction (opty);
		  else
			rstruc->intrObject[f] = new NewInteraction();
	  }
    }

	if (rfh.version < 26) 
	{
	    delete rstruc->intrRoom;
		rstruc->intrRoom = deserialize_new_interaction (opty);
	}

    for (f = 0; f < MAX_REGIONS; f++) {
      if (rstruc->intrRegion[f] != NULL)
        delete rstruc->intrRegion[f];
      rstruc->intrRegion[f] = new NewInteraction();
    }

    if (rfh.version >= 21) {
      rstruc->numRegions = getw (opty);
      if (rstruc->numRegions > MAX_REGIONS)
        quit("load_room: needs newer version of AGS - too many regions");

  	  if (rfh.version < 26) 
	  {
        for (f = 0; f < rstruc->numRegions; f++) {
          delete rstruc->intrRegion[f];
          rstruc->intrRegion[f] = deserialize_new_interaction (opty);
		}
      }
    }

	if (rfh.version >= 26)
	{
	  rstruc->hotspotScripts = new InteractionScripts*[rstruc->numhotspots];
	  rstruc->objectScripts = new InteractionScripts*[rstruc->numsprs];
      rstruc->regionScripts = new InteractionScripts*[rstruc->numRegions];
	  rstruc->roomScripts = new InteractionScripts();
	  deserialize_interaction_scripts(opty, rstruc->roomScripts);
	  int bb;
      for (bb = 0; bb < rstruc->numhotspots; bb++) {
        rstruc->hotspotScripts[bb] = new InteractionScripts();
        deserialize_interaction_scripts(opty, rstruc->hotspotScripts[bb]);
      }
      for (bb = 0; bb < rstruc->numsprs; bb++) {
        rstruc->objectScripts[bb] = new InteractionScripts();
        deserialize_interaction_scripts(opty, rstruc->objectScripts[bb]);
      }
	  for (bb = 0; bb < rstruc->numRegions; bb++) {
        rstruc->regionScripts[bb] = new InteractionScripts();
        deserialize_interaction_scripts(opty, rstruc->regionScripts[bb]);
      }

	}
  }

  if (rfh.version >= 9) {
    fread(&rstruc->objbaseline[0], sizeof(int), rstruc->numsprs, opty);
    fread(&rstruc->width, 2, 1, opty);
    fread(&rstruc->height, 2, 1, opty); 
  }

  if (rfh.version >= 23)
    fread(&rstruc->objectFlags[0], sizeof(short), rstruc->numsprs, opty);

  if (rfh.version >= 11)
    fread(&rstruc->resolution, 2, 1, opty);

  int num_walk_areas = MAX_WALK_AREAS;
  if (rfh.version >= 14)
    num_walk_areas = getw (opty);
    
  if (num_walk_areas > MAX_WALK_AREAS + 1)
    quit("load_room: Too many walkable areas, need newer version");

  if (rfh.version >= 10)
    fread(&rstruc->walk_area_zoom[0], sizeof(short), num_walk_areas, opty);

  if (rfh.version >= 13)
    fread(&rstruc->walk_area_light[0], sizeof(short), num_walk_areas, opty);

  if (rfh.version >= 18) {
    fread(&rstruc->walk_area_zoom2[0], sizeof(short), num_walk_areas, opty);
    fread(&rstruc->walk_area_top[0], sizeof(short), num_walk_areas, opty);
    fread(&rstruc->walk_area_bottom[0], sizeof(short), num_walk_areas, opty);

    for (f = 0; f < num_walk_areas; f++) {
      // if they set a contiuously scaled area where the top
      // and bottom zoom levels are identical, set it as a normal
      // scaled area
      if (rstruc->walk_area_zoom[f] == rstruc->walk_area_zoom2[f])
        rstruc->walk_area_zoom2[f] = NOT_VECTOR_SCALED;
    }
  }

  fread(&rstruc->password[0], 11, 1, opty);
  fread(&rstruc->options[0], 10, 1, opty);
  fread(&rstruc->nummes, 2, 1, opty);

  if (rfh.version >= 25)
    rstruc->gameId = getw(opty);

  if (rfh.version >= 3)
#ifndef ALLEGRO_BIG_ENDIAN
    fread(&rstruc->msgi[0], sizeof(MessageInfo), rstruc->nummes, opty);
#else
  {
    for (int iteratorCount = 0; iteratorCount < rstruc->nummes; ++iteratorCount)
    {
      rstruc->msgi[iteratorCount].ReadFromFile(opty);
    }
  }
#endif
  else
    memset(&rstruc->msgi[0], 0, sizeof(MessageInfo) * MAXMESS);

  for (f = 0;f < rstruc->nummes; f++) {
    if (rfh.version >= 22)
      read_string_decrypt(opty, buffre);
    else
      fgetstring_limit(buffre, opty, 2999);

    rstruc->message[f] = (char *)malloc(strlen(buffre) + 2);
    strcpy(rstruc->message[f], buffre);

    if (buffre[strlen(buffre)-1] == (char)200) {
      rstruc->message[f][strlen(buffre)-1] = 0;
      rstruc->msgi[f].flags |= MSG_DISPLAYNEXT;
    }
  }

  rstruc->numanims = 0;
  if (rfh.version >= 6) {
    fread(&rstruc->numanims, 2, 1, opty);

    if (rstruc->numanims > 0)
      fseek (opty, sizeof(FullAnimation) * rstruc->numanims, SEEK_CUR);
//      fread(&rstruc->anims[0], sizeof(FullAnimation), rstruc->numanims, opty);
  }
  else {
    rstruc->numanims = 0;
    memset(&rstruc->anims[0], 0, sizeof(FullAnimation) * MAXANIMS);
  }

  if ((rfh.version >= 4) && (rfh.version < 16)) {
    load_script_configuration(opty);
    load_graphical_scripts(opty, rstruc);
  }

  if (rfh.version >= 8)
    fread(&rstruc->shadinginfo[0], sizeof(short), 16, opty);

  if (rfh.version >= 21) {
    fread (&rstruc->regionLightLevel[0], sizeof(short), rstruc->numRegions, opty);
    fread (&rstruc->regionTintLevel[0], sizeof(int), rstruc->numRegions, opty);
  }

  update_polled_stuff_if_runtime();

  if (rfh.version >= 5) {
    tesl = load_lzw(opty, rstruc->ebscene[0], rstruc->pal);
    rstruc->ebscene[0] = recalced;
  }
  else
    tesl = loadcompressed_allegro(opty, &rstruc->ebscene[0], rstruc->pal, ftell(opty));

  if ((rstruc->ebscene[0]->w > 320) & (rfh.version < 11))
    rstruc->resolution = 2;

  update_polled_stuff_if_runtime();
  if (rfh.version >= 21)
    tesl = loadcompressed_allegro(opty, &rstruc->regions, rstruc->pal, tesl);
  else if (rfh.version >= 8) {
    tesl = loadcompressed_allegro(opty, &rstruc->regions, rstruc->pal, tesl);
    // an old version - clear the 'shadow' area into a blank regions bmp
    wfreeblock (rstruc->regions);
    rstruc->regions = NULL;
  }

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(opty, &rstruc->walls, rstruc->pal, tesl);

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(opty, &rstruc->object, rstruc->pal, tesl);

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(opty, &rstruc->lookat, rstruc->pal, tesl);

  if (rfh.version < 21) {
    // Old version - copy walkable areas to Regions
    if (rstruc->regions == NULL)
      rstruc->regions = create_bitmap_ex (8, rstruc->walls->w, rstruc->walls->h);
    clear (rstruc->regions);

    blit (rstruc->walls, rstruc->regions, 0, 0, 0, 0, rstruc->regions->w, rstruc->regions->h);
    for (f = 0; f <= 15; f++) {
      rstruc->regionLightLevel[f] = rstruc->walk_area_light[f];
      rstruc->regionTintLevel[f] = 0;
    }
  }

  if (rfh.version < 9) {
    for (f = 0; f < 11; f++)
      rstruc->password[f] += 60;
  }
  else {
    for (f = 0; f < 11; f++)
      rstruc->password[f] += passwencstring[f];
  }
}

void load_room(char *files, roomstruct *rstruc, bool gameIsHighRes) {
  FILE              *opty;
  room_file_header  rfh;
  int i;

  rstruc->freemessage();
  if (rstruc->scripts != NULL) {
    free(rstruc->scripts);
    rstruc->scripts = NULL;
  }

  if (rstruc->compiled_script != NULL)
    ccFreeScript(rstruc->compiled_script);

  rstruc->compiled_script = NULL;
  if (rstruc->num_bscenes > 1) {
    int ff;

    for (ff = 1; ff < rstruc->num_bscenes; ff++) {
      wfreeblock(rstruc->ebscene[ff]);
      rstruc->ebscene[ff] = NULL;
    }
    update_polled_stuff_if_runtime();
  }

  rstruc->num_bscenes = 1;
  rstruc->bscene_anim_speed = 5;
  memset (&rstruc->objectnames[0][0], 0, MAX_INIT_SPR * MAXOBJNAMELEN);
  memset (&rstruc->objectscriptnames[0][0], 0, MAX_INIT_SPR * MAX_SCRIPT_NAME_LEN);
  memset (&rstruc->regionLightLevel[0], 0, sizeof(short) * MAX_REGIONS);
  memset (&rstruc->regionTintLevel[0], 0, sizeof(int) * MAX_REGIONS);

  for (i = 0; i <= MAX_WALK_AREAS; i++) {
    rstruc->walk_area_zoom2[i] = NOT_VECTOR_SCALED;
    rstruc->walk_area_top[i] = -1;
    rstruc->walk_area_bottom[i] = -1;
  }

  for (i = 0; i < rstruc->numhotspots; i++)
    rstruc->hsProps[i].reset();
  for (i = 0; i < rstruc->numsprs; i++)
    rstruc->objProps[i].reset();
  rstruc->roomProps.reset();

  if (rstruc->roomScripts != NULL) 
  {
	  delete rstruc->roomScripts;
	  rstruc->roomScripts = NULL;
  }
  if (rstruc->hotspotScripts != NULL)
  {
	  for (i = 0; i < rstruc->lastLoadNumHotspots; i++)
	  {
		  delete rstruc->hotspotScripts[i];
	  }
	  delete rstruc->hotspotScripts;
	  rstruc->hotspotScripts = NULL;
  }
  if (rstruc->objectScripts != NULL)
  {
	  for (i = 0; i < rstruc->lastLoadNumObjects; i++)
	  {
		  delete rstruc->objectScripts[i];
	  }
	  delete rstruc->objectScripts;
	  rstruc->objectScripts = NULL;
  }
  if (rstruc->regionScripts != NULL)
  {
	  for (i = 0; i < rstruc->lastLoadNumRegions; i++)
	  {
		  delete rstruc->regionScripts[i];
	  }
	  delete rstruc->regionScripts;
	  rstruc->regionScripts = NULL;
  }

  if (rstruc->localvars != NULL)
    free (rstruc->localvars);
  rstruc->localvars = NULL;
  rstruc->numLocalVars = 0;

  memset(&rstruc->ebpalShared[0], 0, MAX_BSCENE);

  update_polled_stuff_if_runtime();

  opty = clibfopen(files, "rb");
  if (opty == NULL) {
    char errbuffr[500];
    sprintf(errbuffr,"Load_room: Unable to load the room file '%s'\n"
      "Make sure that you saved the room to the correct folder (it should be\n"
      "in your game's sub-folder of the AGS directory).\n"
      "Also check that the player character's starting room is set correctly.\n",files);
    quit(errbuffr);
  }
  update_polled_stuff_if_runtime();  // it can take a while to load the file sometimes

#ifndef ALLEGRO_BIG_ENDIAN
  fread(&rfh, sizeof(rfh), 1, opty);
#else
  rfh.ReadFromFile(opty);
#endif
  //fclose(opty);
  rstruc->wasversion = rfh.version;
#ifdef THIS_IS_THE_ENGINE
  if ((rstruc->wasversion < 17) | (rstruc->wasversion > ROOM_FILE_VERSION))
#else
  if ((rstruc->wasversion < 15) || (rstruc->wasversion > ROOM_FILE_VERSION))
#endif
  {
    fclose(opty);
    quit("Load_Room: Bad packed file. Either the file requires a newer or older version of\n"
      "this program or the file is corrupt.\n");
  }

  int   thisblock = 0;
  long  bloklen;
  FILE *optywas;

  while (thisblock != BLOCKTYPE_EOF) {
    update_polled_stuff_if_runtime();
    thisblock = fgetc(opty);

    if (thisblock == BLOCKTYPE_EOF)
      break;

    fread(&bloklen, 4, 1, opty);
    bloklen += ftell(opty);  // make it the new position for after block read
    optywas = opty;

    if (thisblock == BLOCKTYPE_MAIN)
      load_main_block(rstruc, files, opty, rfh);
    else if (thisblock == BLOCKTYPE_SCRIPT) {
      long  lee;
      int   hh;

      fread(&lee, 4, 1, opty);
      rstruc->scripts = (char *)malloc(lee + 5);
      // MACPORT FIX: swap
      fread(rstruc->scripts, sizeof(char), lee, opty);
      rstruc->scripts[lee] = 0;

      for (hh = 0; hh < lee; hh++)
        rstruc->scripts[hh] += passwencstring[hh % 11];
    }
    else if (thisblock == BLOCKTYPE_COMPSCRIPT3) {
      rstruc->compiled_script = fread_script(opty);
      if (rstruc->compiled_script == NULL)
        quit("Load_room: Script load failed; need newer version?");
    }
    else if ((thisblock == BLOCKTYPE_COMPSCRIPT) || (thisblock == BLOCKTYPE_COMPSCRIPT2))
#ifdef LOADROOM_ALLOWOLD
      rstruc->compiled_script = NULL;
#else
      quit("Load_room: old room format. Please upgrade the room.");
#endif
    else if (thisblock == BLOCKTYPE_OBJECTNAMES) {
      if (fgetc(opty) != rstruc->numsprs)
        quit("Load_room: inconsistent blocks for object names");

      fread(&rstruc->objectnames[0][0], MAXOBJNAMELEN, rstruc->numsprs, opty);
    }
    else if (thisblock == BLOCKTYPE_OBJECTSCRIPTNAMES) {
      if (fgetc(opty) != rstruc->numsprs)
        quit("Load_room: inconsistent blocks for object script names");

      fread(&rstruc->objectscriptnames[0][0], MAX_SCRIPT_NAME_LEN, rstruc->numsprs, opty);
    }
    else if (thisblock == BLOCKTYPE_ANIMBKGRND) {
      int   ct;
      long  fpos;

      rstruc->num_bscenes = fgetc(opty);
      rstruc->bscene_anim_speed = fgetc(opty);

      if (rfh.version >= 20)
        fread(&rstruc->ebpalShared[0], 1, rstruc->num_bscenes, opty);
      else
        memset (&rstruc->ebpalShared[0], 0, rstruc->num_bscenes);

      fpos = ftell(opty);
//        fclose(opty);

      for (ct = 1; ct < rstruc->num_bscenes; ct++) {
        update_polled_stuff_if_runtime();
//          fpos = load_lzw(files,rstruc->ebscene[ct],rstruc->pal,fpos);
        fpos = load_lzw(opty, rstruc->ebscene[ct], rstruc->bpalettes[ct]);
        rstruc->ebscene[ct] = recalced;
      }
//        opty = clibfopen(files, "rb");
//        fseek(opty, fpos, SEEK_SET);
    }
    else if (thisblock == BLOCKTYPE_PROPERTIES) {
      // Read custom properties
      if (getw(opty) != 1)
        quit("LoadRoom: unknown Custom Properties block encounreted");

      int errors = 0, gg;

      if (rstruc->roomProps.UnSerialize (opty))
        quit("LoadRoom: error reading custom properties block");

      for (gg = 0; gg < rstruc->numhotspots; gg++)
        errors += rstruc->hsProps[gg].UnSerialize (opty);
      for (gg = 0; gg < rstruc->numsprs; gg++)
        errors += rstruc->objProps[gg].UnSerialize (opty);

      if (errors > 0)
        quit("LoadRoom: errors encountered reading custom props");
    }
    else if (thisblock == -1)
    {
      fclose(opty);
      quit("LoadRoom: unexpected end of file while loading room");
      return;
    }
    else {
      char  tempbfr[90];
      sprintf(tempbfr, "LoadRoom: unknown block type %d encountered in '%s'", thisblock, files);
      quit(tempbfr);
    }

    // The ftell call below has caused crashes
    if (ftell(opty) != bloklen)
      fseek(opty, bloklen, SEEK_SET);
  }

  // sync bpalettes[0] with room.pal
  memcpy (&rstruc->bpalettes[0][0], &rstruc->pal[0], sizeof(color) * 256);

  fclose(opty);

  if ((rfh.version < 29) && (gameIsHighRes))
  {
	  // Pre-3.0.3, multiply up co-ordinates
      // If you change this, also change convert_room_coordinates_to_low_res
      // function in the engine
	  int f;
	  for (f = 0; f < rstruc->numsprs; f++)
	  {
		  rstruc->sprs[f].x *= 2;
		  rstruc->sprs[f].y *= 2;
		  if (rstruc->objbaseline[f] > 0)
		  {
			  rstruc->objbaseline[f] *= 2;
		  }
	  }

	  for (f = 0; f < rstruc->numhotspots; f++)
	  {
		  rstruc->hswalkto[f].x *= 2;
		  rstruc->hswalkto[f].y *= 2;
	  }

	  for (f = 0; f < rstruc->numobj; f++)
	  {
		  rstruc->objyval[f] *= 2;
	  }

	  rstruc->left *= 2;
	  rstruc->top *= 2;
	  rstruc->bottom *= 2;
	  rstruc->right *= 2;
	  rstruc->width *= 2;
	  rstruc->height *= 2;
  }

}




void ViewStruct::Initialize(int loopCount)
{
  numLoops = loopCount;
  if (numLoops > 0)
  {
    loops = (ViewLoopNew*)calloc(numLoops, sizeof(ViewLoopNew));
  }
}

void ViewStruct::Dispose()
{
  if (numLoops > 0)
  {
    free(loops);
    numLoops = 0;
  }
}

void ViewStruct::WriteToFile(FILE *ooo)
{
  putshort(numLoops, ooo);
  for (int i = 0; i < numLoops; i++)
  {
    loops[i].WriteToFile(ooo);
  }
}

void ViewStruct::ReadFromFile(FILE *iii)
{
#ifdef ALLEGRO_BIG_ENDIAN
  Initialize(__getshort__bigendian(fp));
#else
  Initialize(getshort(iii));
#endif

  for (int i = 0; i < numLoops; i++)
  {
    loops[i].ReadFromFile(iii);
  }
}

void ViewLoopNew::Initialize(int frameCount)
{
  numFrames = frameCount;
  flags = 0;
  frames = (ViewFrame*)calloc(numFrames + 1, sizeof(ViewFrame));
}

void ViewLoopNew::Dispose()
{
  if (frames != NULL)
  {
    free(frames);
    frames = NULL;
    numFrames = 0;
  }
}

void ViewLoopNew::WriteToFile(FILE *ooo)
{
  fwrite(&numFrames, sizeof(short), 1, ooo);
  fwrite(&flags, sizeof(int), 1, ooo);
  fwrite(frames, sizeof(ViewFrame), numFrames, ooo);
}


void ViewLoopNew::ReadFromFile(FILE *iii)
{
#ifdef ALLEGRO_BIG_ENDIAN

  STEVE PLEASE VALIDATE THAT THIS CODE IS OK

  Initialize(__getshort__bigendian(fp));
  flags = getw(iii);

  for (int i = 0; i < numFrames; ++i)
  {
    frames[i].ReadFromFile(iii);
  }

#else

  Initialize(getshort(iii));
  flags = getw(iii);

  fread(frames, sizeof(ViewFrame), numFrames, iii);

#endif

  // an extra frame is allocated in memory to prevent
  // crashes with empty loops -- set its picture to teh BLUE CUP!!
  frames[numFrames].pic = 0;
}





int CharacterInfo::get_effective_y() {
  return y - z;
}
int CharacterInfo::get_baseline() {
  if (baseline < 1)
    return y;
  return baseline;
}
int CharacterInfo::get_blocking_top() {
  if (blocking_height > 0)
    return y - blocking_height / 2;
  return y - 2;
}
int CharacterInfo::get_blocking_bottom() {
  // the blocking_bottom should be 1 less than the top + height
  // since the code does <= checks on it rather than < checks
  if (blocking_height > 0)
    return (y + (blocking_height + 1) / 2) - 1;
  return y + 3;
}



void ConvertOldCharacterToNew (OldCharacterInfo *oci, CharacterInfo *ci) {
  COPY_CHAR_VAR (defview);
  COPY_CHAR_VAR (talkview);
  COPY_CHAR_VAR (view);
  COPY_CHAR_VAR (room);
  COPY_CHAR_VAR (prevroom);
  COPY_CHAR_VAR (x);
  COPY_CHAR_VAR (y);
  COPY_CHAR_VAR (wait);
  COPY_CHAR_VAR (flags);
  COPY_CHAR_VAR (following);
  COPY_CHAR_VAR (followinfo);
  COPY_CHAR_VAR (idleview);
  COPY_CHAR_VAR (idletime);
  COPY_CHAR_VAR (idleleft);
  COPY_CHAR_VAR (transparency);
  COPY_CHAR_VAR (baseline);
  COPY_CHAR_VAR (activeinv);
  COPY_CHAR_VAR (loop);
  COPY_CHAR_VAR (frame);
  COPY_CHAR_VAR (walking);
  COPY_CHAR_VAR (animating);
  COPY_CHAR_VAR (walkspeed);
  COPY_CHAR_VAR (animspeed);
  COPY_CHAR_VAR (actx);
  COPY_CHAR_VAR (acty);
  COPY_CHAR_VAR (on);
  strcpy (ci->name, oci->name);
  strcpy (ci->scrname, oci->scrname);
  memcpy (&ci->inv[0], &oci->inv[0], sizeof(short) * 100);
  // move the talking colour into the struct and remove from flags
  ci->talkcolor = (oci->flags & OCHF_SPEECHCOL) >> OCHF_SPEECHCOLSHIFT;
  ci->flags = ci->flags & (~OCHF_SPEECHCOL);
}

void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss) {
  int i;
  strcpy (gss->gamename, ogss->gamename);
  for (i = 0; i < 20; i++)
    gss->options[i] = ogss->options[i];
  memcpy (&gss->paluses[0], &ogss->paluses[0], 256);
  memcpy (&gss->defpal[0],  &ogss->defpal[0],  256 * sizeof(color));
  gss->numviews = ogss->numviews;
  gss->numcharacters = ogss->numcharacters;
  gss->playercharacter = ogss->playercharacter;
  gss->totalscore = ogss->totalscore;
  gss->numinvitems = ogss->numinvitems;
  gss->numdialog = ogss->numdialog;
  gss->numdlgmessage = ogss->numdlgmessage;
  gss->numfonts = ogss->numfonts;
  gss->color_depth = ogss->color_depth;
  gss->target_win = ogss->target_win;
  gss->dialog_bullet = ogss->dialog_bullet;
  gss->hotdot = ogss->hotdot;
  gss->hotdotouter = ogss->hotdotouter;
  gss->uniqueid = ogss->uniqueid;
  gss->numgui = ogss->numgui;
  memcpy (&gss->fontflags[0], &ogss->fontflags[0], 10);
  memcpy (&gss->fontoutline[0], &ogss->fontoutline[0], 10);
  memcpy (&gss->spriteflags[0], &ogss->spriteflags[0], 6000);
  memcpy (&gss->invinfo[0], &ogss->invinfo[0], 100 * sizeof(InventoryItemInfo));
  memcpy (&gss->mcurs[0], &ogss->mcurs[0], 10 * sizeof(MouseCursor));
  for (i = 0; i < MAXGLOBALMES; i++)
    gss->messages[i] = ogss->messages[i];
  gss->dict = ogss->dict;
  gss->globalscript = ogss->globalscript;
  gss->chars = NULL; //ogss->chars;
  gss->compiled_script = ogss->compiled_script;
  gss->numcursors = 10;
}

void Convert272ViewsToNew (int numof, ViewStruct272 *oldv, ViewStruct *newv) {
  
  for (int a = 0; a < numof; a++) {
    newv[a].Initialize(oldv[a].numloops);
    
    for (int b = 0; b < oldv[a].numloops; b++) 
    {
      newv[a].loops[b].Initialize(oldv[a].numframes[b]);

      if ((oldv[a].numframes[b] > 0) &&
          (oldv[a].frames[b][oldv[a].numframes[b] - 1].pic == -1))
      {
        newv[a].loops[b].flags = LOOPFLAG_RUNNEXTLOOP;
        newv[a].loops[b].numFrames--;
      }
      else
        newv[a].loops[b].flags = 0;

      for (int c = 0; c < newv[a].loops[b].numFrames; c++)
        newv[a].loops[b].frames[c] = oldv[a].frames[b][c];
    }
  }
}


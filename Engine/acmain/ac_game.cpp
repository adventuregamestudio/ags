#define USE_CLIB
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_game.h"
#include "acmain/ac_commonheaders.h"
#include "gui/dialog.h"
#include "acmain/ac_conversation.h"
#include "gui/guilabel.h"
#include "cs/cs_utils.h"
#include "ac/file.h"
#include "script/script_runtime.h"
#include "acmain/ac_lipsync.h"
#include "ac/dynobj/scriptviewframe.h"
#include "plugin/agsplugin.h"
#include "gui/animatingguibutton.h"
#include "acmain/ac_richgamemedia.h"
#include "ac/dynobj/cc_serializer.h"
#include "cs/cc_error.h"
#include "gfx/gfxfilter.h"
#include "media/audio/audio.h"
#include "ac/audioclip.h"
#include "ac/global_audio.h"
#include "ac/system_audio.h"
#include "script/exports.h"
#include "ac/gui.h"

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <sys/stat.h>                      //mkdir
#endif


extern "C" int csetlib(char *namm, char *passw);


extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;

extern int ifacepopped;  // currently displayed pop-up GUI (-1 if none)
extern int mouse_on_iface;   // mouse cursor is over this interface
extern int mouse_on_iface_button;
extern int mouse_pushed_iface;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;



char saveGameDirectory[260] = "./";
int want_quit = 0;

const char* sgnametemplate = "agssave.%03d";
char saveGameSuffix[MAX_SG_EXT_LENGTH + 1];

int game_paused=0;
extern char* game_file_name;



int engineNeedsAsInt = 100;
char pexbuf[STD_BUFFER_SIZE];

unsigned int load_new_game = 0;
int load_new_game_restore = -1;


#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
extern int psp_gfx_smoothing;
extern int psp_gfx_scaling;
extern int psp_gfx_renderer;
extern int psp_gfx_super_sampling;
#endif



void GiveScore(int amnt) 
{
    guis_need_update = 1;
    play.score += amnt;

    if ((amnt > 0) && (play.score_sound >= 0))
        play_audio_clip_by_index(play.score_sound);

    run_on_event (GE_GOT_SCORE, amnt);
}



const char *load_game_errors[9] =
  {"No error","File not found","Not an AGS save game",
  "Invalid save game version","Saved with different interpreter",
  "Saved under a different game", "Resolution mismatch",
  "Colour depth mismatch", ""};

void restart_game() {
  can_run_delayed_command();
  if (inside_script) {
    curscript->queue_action(ePSARestartGame, 0, "RestartGame");
    return;
  }
  int errcod;
  if ((errcod = load_game(RESTART_POINT_SAVE_GAME_NUMBER, NULL, NULL))!=0)
    quitprintf("unable to restart game (error:%s)", load_game_errors[-errcod]);

}



void set_game_speed(int fps) {
    frames_per_second = fps;
    time_between_timers = 1000 / fps;
    install_int_ex(dj_timer_handler,MSEC_TO_TIMER(time_between_timers));
}

extern int cbuttfont;
extern int acdialog_font;

extern char buffer2[60];
int oldmouse;
void setup_for_dialog() {
  cbuttfont = play.normal_font;
  acdialog_font = play.normal_font;
  wsetscreen(virtual_screen);
  if (!play.mouse_cursor_hidden)
    domouse(1);
  oldmouse=cur_cursor; set_mouse_cursor(CURS_ARROW);
}
void restore_after_dialog() {
  set_mouse_cursor(oldmouse);
  if (!play.mouse_cursor_hidden)
    domouse(2);
  construct_virtual_screen(true);
}

void RestoreGameSlot(int slnum) {
  if (displayed_room < 0)
    quit("!RestoreGameSlot: a game cannot be restored from within game_start");

  can_run_delayed_command();
  if (inside_script) {
    curscript->queue_action(ePSARestoreGame, slnum, "RestoreGameSlot");
    return;
  }
  load_game(slnum, NULL, NULL);
}

void get_save_game_path(int slotNum, char *buffer) {
  strcpy(buffer, saveGameDirectory);
  sprintf(&buffer[strlen(buffer)], sgnametemplate, slotNum);
  strcat(buffer, saveGameSuffix);
}

void DeleteSaveSlot (int slnum) {
  char nametouse[260];
  get_save_game_path(slnum, nametouse);
  unlink (nametouse);
  if ((slnum >= 1) && (slnum <= MAXSAVEGAMES)) {
    char thisname[260];
    for (int i = MAXSAVEGAMES; i > slnum; i--) {
      get_save_game_path(i, thisname);
      FILE *fin = fopen (thisname, "rb");
      if (fin != NULL) {
        fclose (fin);
        // Rename the highest save game to fill in the gap
        rename (thisname, nametouse);
        break;
      }
    }

  }
}

int Game_SetSaveGameDirectory(const char *newFolder) {

  // don't allow them to go to another folder
  if ((newFolder[0] == '/') || (newFolder[0] == '\\') ||
    (newFolder[0] == ' ') ||
    ((newFolder[0] != 0) && (newFolder[1] == ':')))
    return 0;

  char newSaveGameDir[260];
  platform->ReplaceSpecialPaths(newFolder, newSaveGameDir);
  fix_filename_slashes(newSaveGameDir);

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
  mkdir(newSaveGameDir, 0);
#else
  mkdir(newSaveGameDir);
#endif

  put_backslash(newSaveGameDir);

  char newFolderTempFile[260];
  strcpy(newFolderTempFile, newSaveGameDir);
  strcat(newFolderTempFile, "agstmp.tmp");

  FILE *testTemp = fopen(newFolderTempFile, "wb");
  if (testTemp == NULL) {
    return 0;
  }
  fclose(testTemp);
  unlink(newFolderTempFile);

  // copy the Restart Game file, if applicable
  char restartGamePath[260];
  sprintf(restartGamePath, "%s""agssave.%d%s", saveGameDirectory, RESTART_POINT_SAVE_GAME_NUMBER, saveGameSuffix);
  FILE *restartGameFile = fopen(restartGamePath, "rb");
  if (restartGameFile != NULL) {
    long fileSize = filelength(fileno(restartGameFile));
    char *mbuffer = (char*)malloc(fileSize);
    fread(mbuffer, fileSize, 1, restartGameFile);
    fclose(restartGameFile);

    sprintf(restartGamePath, "%s""agssave.%d%s", newSaveGameDir, RESTART_POINT_SAVE_GAME_NUMBER, saveGameSuffix);
    restartGameFile = fopen(restartGamePath, "wb");
    fwrite(mbuffer, fileSize, 1, restartGameFile);
    fclose(restartGameFile);
    free(mbuffer);
  }

  strcpy(saveGameDirectory, newSaveGameDir);
  return 1;
}

int GetSaveSlotDescription(int slnum,char*desbuf) {
  VALIDATE_STRING(desbuf);
  if (load_game(slnum, desbuf, NULL) == 0)
    return 1;
  sprintf(desbuf,"INVALID SLOT %d", slnum);
  return 0;
}

const char* Game_GetSaveSlotDescription(int slnum) {
  char buffer[STD_BUFFER_SIZE];
  if (load_game(slnum, buffer, NULL) == 0)
    return CreateNewScriptString(buffer);
  return NULL;
}

int LoadSaveSlotScreenshot(int slnum, int width, int height) {
  int gotSlot;
  multiply_up_coordinates(&width, &height);

  if (load_game(slnum, NULL, &gotSlot) != 0)
    return 0;

  if (gotSlot == 0)
    return 0;

  if ((spritewidth[gotSlot] == width) && (spriteheight[gotSlot] == height))
    return gotSlot;

  // resize the sprite to the requested size
  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[gotSlot]), width, height);

  stretch_blit(spriteset[gotSlot], newPic,
               0, 0, spritewidth[gotSlot], spriteheight[gotSlot],
               0, 0, width, height);

  update_polled_stuff_if_runtime();

  // replace the bitmap in the sprite set
  free_dynamic_sprite(gotSlot);
  add_dynamic_sprite(gotSlot, newPic);

  return gotSlot;
}


int load_game_and_print_error(int toload) {
    int ecret = load_game(toload, NULL, NULL);
    if (ecret < 0) {
        // disable speech in case there are dynamic graphics that
        // have been freed
        int oldalways = game.options[OPT_ALWAYSSPCH];
        game.options[OPT_ALWAYSSPCH] = 0;
        Display("Unable to load game (error: %s).",load_game_errors[-ecret]);
        game.options[OPT_ALWAYSSPCH] = oldalways;
    }
    return ecret;
}

void restore_game_dialog() {
    can_run_delayed_command();
    if (thisroom.options[ST_SAVELOAD] == 1) {
        DisplayMessage (983);
        return;
    }
    if (inside_script) {
        curscript->queue_action(ePSARestoreGameDialog, 0, "RestoreGameDialog");
        return;
    }
    setup_for_dialog();
    int toload=loadgamedialog();
    restore_after_dialog();
    if (toload>=0) {
        load_game_and_print_error(toload);
    }
}

void save_game_dialog() {
    if (thisroom.options[ST_SAVELOAD] == 1) {
        DisplayMessage (983);
        return;
    }
    if (inside_script) {
        curscript->queue_action(ePSASaveGameDialog, 0, "SaveGameDialog");
        return;
    }
    setup_for_dialog();
    int toload=savegamedialog();
    restore_after_dialog();
    if (toload>=0)
        save_game(toload,buffer2);
}


void PauseGame() {
    game_paused++;
    DEBUG_CONSOLE("Game paused");
}
void UnPauseGame() {
    if (game_paused > 0)
        game_paused--;
    DEBUG_CONSOLE("Game UnPaused, pause level now %d", game_paused);
}


int IsGamePaused() {
    if (game_paused>0) return 1;
    return 0;
}


void setup_sierra_interface() {
    int rr;
    game.numgui =0;
    for (rr=0;rr<42;rr++) game.paluses[rr]=PAL_GAMEWIDE;
    for (rr=42;rr<256;rr++) game.paluses[rr]=PAL_BACKGROUND;
}


void free_do_once_tokens() 
{
    for (int i = 0; i < play.num_do_once_tokens; i++)
    {
        free(play.do_once_tokens[i]);
    }
    if (play.do_once_tokens != NULL) 
    {
        free(play.do_once_tokens);
        play.do_once_tokens = NULL;
    }
    play.num_do_once_tokens = 0;
}


// Free all the memory associated with the game
void unload_game_file() {
    int bb, ee;

    for (bb = 0; bb < game.numcharacters; bb++) {
        if (game.charScripts != NULL)
            delete game.charScripts[bb];

        if (game.intrChar != NULL)
        {
            if (game.intrChar[bb] != NULL)
                delete game.intrChar[bb];
            game.intrChar[bb] = NULL;
        }
        free(characterScriptObjNames[bb]);
        game.charProps[bb].reset();
    }
    if (game.intrChar != NULL)
    {
        free(game.intrChar);
        game.intrChar = NULL;
    }
    free(characterScriptObjNames);
    free(charextra);
    free(mls);
    free(actsps);
    free(actspsbmp);
    free(actspswb);
    free(actspswbbmp);
    free(actspswbcache);
    free(game.charProps);

    for (bb = 1; bb < game.numinvitems; bb++) {
        if (game.invScripts != NULL)
            delete game.invScripts[bb];
        if (game.intrInv[bb] != NULL)
            delete game.intrInv[bb];
        game.intrInv[bb] = NULL;
    }

    if (game.charScripts != NULL)
    {
        delete game.charScripts;
        delete game.invScripts;
        game.charScripts = NULL;
        game.invScripts = NULL;
    }

    if (game.dict != NULL) {
        game.dict->free_memory();
        free (game.dict);
        game.dict = NULL;
    }

    if ((gameinst != NULL) && (gameinst->pc != 0))
        quit("Error: unload_game called while script still running");
    //ccAbortAndDestroyInstance (gameinst);
    else {
        ccFreeInstance(gameinstFork);
        ccFreeInstance(gameinst);
        gameinstFork = NULL;
        gameinst = NULL;
    }

    ccFreeScript (gamescript);
    gamescript = NULL;

    if ((dialogScriptsInst != NULL) && (dialogScriptsInst->pc != 0))
        quit("Error: unload_game called while dialog script still running");
    else if (dialogScriptsInst != NULL)
    {
        ccFreeInstance(dialogScriptsInst);
        dialogScriptsInst = NULL;
    }

    if (dialogScriptsScript != NULL)
    {
        ccFreeScript(dialogScriptsScript);
        dialogScriptsScript = NULL;
    }

    for (ee = 0; ee < numScriptModules; ee++) {
        ccFreeInstance(moduleInstFork[ee]);
        ccFreeInstance(moduleInst[ee]);
        ccFreeScript(scriptModules[ee]);
    }
    numScriptModules = 0;

    if (game.audioClipCount > 0)
    {
        free(game.audioClips);
        game.audioClipCount = 0;
        free(game.audioClipTypes);
        game.audioClipTypeCount = 0;
    }

    free(game.viewNames[0]);
    free(game.viewNames);
    free (views);
    views = NULL;

    free (game.chars);
    game.chars = NULL;

    free (charcache);
    charcache = NULL;

    if (splipsync != NULL)
    {
        for (ee = 0; ee < numLipLines; ee++)
        {
            free(splipsync[ee].endtimeoffs);
            free(splipsync[ee].frame);
        }
        free(splipsync);
        splipsync = NULL;
        numLipLines = 0;
        curLipLine = -1;
    }

    for (ee=0;ee < MAXGLOBALMES;ee++) {
        if (game.messages[ee]==NULL) continue;
        free (game.messages[ee]);
        game.messages[ee] = NULL;
    }

    for (ee = 0; ee < game.roomCount; ee++)
    {
        free(game.roomNames[ee]);
    }
    if (game.roomCount > 0)
    {
        free(game.roomNames);
        free(game.roomNumbers);
        game.roomCount = 0;
    }

    for (ee=0;ee<game.numdialog;ee++) {
        if (dialog[ee].optionscripts!=NULL)
            free (dialog[ee].optionscripts);
        dialog[ee].optionscripts = NULL;
    }
    free (dialog);
    dialog = NULL;

    for (ee = 0; ee < game.numgui; ee++) {
        free (guibg[ee]);
        guibg[ee] = NULL;
        free(guiScriptObjNames[ee]);
    }

    free(guiScriptObjNames);
    free(guibg);
    free (guis);
    guis = NULL;
    free(scrGui);

    platform->ShutdownPlugins();
    ccRemoveAllSymbols();
    ccUnregisterAllObjects();

    for (ee=0;ee<game.numfonts;ee++)
        wfreefont(ee);

    free_do_once_tokens();
    free(play.gui_draw_order);

    free (roomstats);
    roomstats=(RoomStatus*)calloc(sizeof(RoomStatus),MAX_ROOMS);
    for (ee=0;ee<MAX_ROOMS;ee++) {
        roomstats[ee].beenhere=0;
        roomstats[ee].numobj=0;
        roomstats[ee].tsdatasize=0;
        roomstats[ee].tsdata=NULL;
    }

}




void SetGlobalInt(int index,int valu) {
    if ((index<0) | (index>=MAXGSVALUES))
        quit("!SetGlobalInt: invalid index");

    if (play.globalscriptvars[index] != valu) {
        DEBUG_CONSOLE("GlobalInt %d set to %d", index, valu);
    }

    play.globalscriptvars[index]=valu;
}


int GetGlobalInt(int index) {
    if ((index<0) | (index>=MAXGSVALUES))
        quit("!GetGlobalInt: invalid index");
    return play.globalscriptvars[index];
}

void SetGlobalString (int index, char *newval) {
    if ((index<0) | (index >= MAXGLOBALSTRINGS))
        quit("!SetGlobalString: invalid index");
    DEBUG_CONSOLE("GlobalString %d set to '%s'", index, newval);
    strncpy(play.globalstrings[index], newval, MAX_MAXSTRLEN);
    // truncate it to 200 chars, to be sure
    play.globalstrings[index][MAX_MAXSTRLEN - 1] = 0;
}

void GetGlobalString (int index, char *strval) {
    if ((index<0) | (index >= MAXGLOBALSTRINGS))
        quit("!GetGlobalString: invalid index");
    strcpy (strval, play.globalstrings[index]);
}

const char* Game_GetGlobalStrings(int index) {
    if ((index < 0) || (index >= MAXGLOBALSTRINGS))
        quit("!Game.GlobalStrings: invalid index");

    return CreateNewScriptString(play.globalstrings[index]);
}



char gamefilenamebuf[200];
#define RAGMODE_PRESERVEGLOBALINT 1
#define RAGMODE_LOADNOW 0x8000000  // just to make sure it's non-zero
int RunAGSGame (char *newgame, unsigned int mode, int data) {

    can_run_delayed_command();

    int AllowedModes = RAGMODE_PRESERVEGLOBALINT | RAGMODE_LOADNOW;

    if ((mode & (~AllowedModes)) != 0)
        quit("!RunAGSGame: mode value unknown");

    if (use_compiled_folder_as_current_dir || editor_debugging_enabled)
    {
        quit("!RunAGSGame cannot be used while running the game from within the AGS Editor. You must build the game EXE and run it from there to use this function.");
    }

    if ((mode & RAGMODE_LOADNOW) == 0) {
        // need to copy, since the script gets destroyed
        get_current_dir_path(gamefilenamebuf, newgame);
        game_file_name = &gamefilenamebuf[0];
        usetup.main_data_filename = game_file_name;
        play.takeover_data = data;
        load_new_game_restore = -1;

        if (inside_script) {
            curscript->queue_action(ePSARunAGSGame, mode | RAGMODE_LOADNOW, "RunAGSGame");
            ccAbortInstance (ccGetCurrentInstance ());
        }
        else
            load_new_game = mode | RAGMODE_LOADNOW;

        return 0;
    }

    int result, ee;

    unload_old_room();
    displayed_room = -10;

    unload_game_file();

    if (csetlib(game_file_name,""))
        quitprintf("!RunAGSGame: unable to load new game file '%s'", game_file_name);

    clear(abuf);
    show_preload();

    if ((result = load_game_file ()) != 0) {
        quitprintf("!RunAGSGame: error %d loading new game file", result);
    }

    spriteset.reset();
    if (spriteset.initFile ("acsprset.spr"))
        quit("!RunAGSGame: error loading new sprites");

    if ((mode & RAGMODE_PRESERVEGLOBALINT) == 0) {
        // reset GlobalInts
        for (ee = 0; ee < MAXGSVALUES; ee++)
            play.globalscriptvars[ee] = 0;  
    }

    init_game_settings();
    play.screen_is_faded_out = 1;

    if (load_new_game_restore >= 0) {
        load_game (load_new_game_restore, NULL, NULL);
        load_new_game_restore = -1;
    }
    else
        start_game();

    return 0;
}



// ** GetGameParameter replacement functions

int Game_GetInventoryItemCount() {
  // because of the dummy item 0, this is always one higher than it should be
  return game.numinvitems - 1;
}

int Game_GetFontCount() {
  return game.numfonts;
}

int Game_GetMouseCursorCount() {
  return game.numcursors;
}

int Game_GetCharacterCount() {
  return game.numcharacters;
}

int Game_GetGUICount() {
  return game.numgui;
}

int Game_GetViewCount() {
  return game.numviews;
}

int Game_GetUseNativeCoordinates()
{
  if (game.options[OPT_NATIVECOORDINATES] != 0)
  {
    return 1;
  }
  return 0;
}

int Game_GetSpriteWidth(int spriteNum) {
  if ((spriteNum < 0) || (spriteNum >= MAX_SPRITES))
    return 0;

  if (!spriteset.doesSpriteExist(spriteNum))
    return 0;

  return divide_down_coordinate(spritewidth[spriteNum]);
}

int Game_GetSpriteHeight(int spriteNum) {
  if ((spriteNum < 0) || (spriteNum >= MAX_SPRITES))
    return 0;

  if (!spriteset.doesSpriteExist(spriteNum))
    return 0;

  return divide_down_coordinate(spriteheight[spriteNum]);
}

int Game_GetLoopCountForView(int viewNumber) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");

  return views[viewNumber - 1].numLoops;
}

int Game_GetRunNextSettingForLoop(int viewNumber, int loopNumber) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");
  if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
    quit("!GetGameParameter: invalid loop specified");

  return (views[viewNumber - 1].loops[loopNumber].RunNextLoop()) ? 1 : 0;
}

int Game_GetFrameCountForLoop(int viewNumber, int loopNumber) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");
  if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
    quit("!GetGameParameter: invalid loop specified");

  return views[viewNumber - 1].loops[loopNumber].numFrames;
}

ScriptViewFrame* Game_GetViewFrame(int viewNumber, int loopNumber, int frame) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");
  if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
    quit("!GetGameParameter: invalid loop specified");
  if ((frame < 0) || (frame >= views[viewNumber - 1].loops[loopNumber].numFrames))
    quit("!GetGameParameter: invalid frame specified");

  ScriptViewFrame *sdt = new ScriptViewFrame(viewNumber - 1, loopNumber, frame);
  ccRegisterManagedObject(sdt, sdt);
  return sdt;
}



int Game_DoOnceOnly(const char *token) 
{
  if (strlen(token) > 199)
    quit("!Game.DoOnceOnly: token length cannot be more than 200 chars");

  for (int i = 0; i < play.num_do_once_tokens; i++)
  {
    if (strcmp(play.do_once_tokens[i], token) == 0)
    {
      return 0;
    }
  }
  play.do_once_tokens = (char**)realloc(play.do_once_tokens, sizeof(char*) * (play.num_do_once_tokens + 1));
  play.do_once_tokens[play.num_do_once_tokens] = (char*)malloc(strlen(token) + 1);
  strcpy(play.do_once_tokens[play.num_do_once_tokens], token);
  play.num_do_once_tokens++;
  return 1;
}



#define GP_SPRITEWIDTH   1
#define GP_SPRITEHEIGHT  2
#define GP_NUMLOOPS      3
#define GP_NUMFRAMES     4
#define GP_ISRUNNEXTLOOP 5
#define GP_FRAMESPEED    6
#define GP_FRAMEIMAGE    7
#define GP_FRAMESOUND    8
#define GP_NUMGUIS       9
#define GP_NUMOBJECTS    10
#define GP_NUMCHARACTERS 11
#define GP_NUMINVITEMS   12
#define GP_ISFRAMEFLIPPED 13

int GetGameParameter (int parm, int data1, int data2, int data3) {
  switch (parm) {
   case GP_SPRITEWIDTH:
     return Game_GetSpriteWidth(data1);
   case GP_SPRITEHEIGHT:
     return Game_GetSpriteHeight(data1);
   case GP_NUMLOOPS:
     return Game_GetLoopCountForView(data1);
   case GP_NUMFRAMES:
     return Game_GetFrameCountForLoop(data1, data2);
   case GP_FRAMESPEED:
   case GP_FRAMEIMAGE:
   case GP_FRAMESOUND:
   case GP_ISFRAMEFLIPPED:
     {
     if ((data1 < 1) || (data1 > game.numviews))
       quit("!GetGameParameter: invalid view specified");
     if ((data2 < 0) || (data2 >= views[data1 - 1].numLoops))
       quit("!GetGameParameter: invalid loop specified");
     if ((data3 < 0) || (data3 >= views[data1 - 1].loops[data2].numFrames))
       quit("!GetGameParameter: invalid frame specified");

     ViewFrame *pvf = &views[data1 - 1].loops[data2].frames[data3];

     if (parm == GP_FRAMESPEED)
       return pvf->speed;
     else if (parm == GP_FRAMEIMAGE)
       return pvf->pic;
     else if (parm == GP_FRAMESOUND)
       return get_old_style_number_for_sound(pvf->sound);
     else if (parm == GP_ISFRAMEFLIPPED)
       return (pvf->flags & VFLG_FLIPSPRITE) ? 1 : 0;
     else
       quit("GetGameParameter internal error");
     }
   case GP_ISRUNNEXTLOOP:
     return Game_GetRunNextSettingForLoop(data1, data2);
   case GP_NUMGUIS:
     return game.numgui;
   case GP_NUMOBJECTS:
     return croom->numobj;
   case GP_NUMCHARACTERS:
     return game.numcharacters;
   case GP_NUMINVITEMS:
     return game.numinvitems;
   default:
     quit("!GetGameParameter: unknown parameter specified");
  }
  return 0;
}

int Game_GetTextReadingSpeed()
{
  return play.text_speed;
}

void Game_SetTextReadingSpeed(int newTextSpeed)
{
  if (newTextSpeed < 1)
    quitprintf("!Game.TextReadingSpeed: %d is an invalid speed", newTextSpeed);

  play.text_speed = newTextSpeed;
}

int Game_GetMinimumTextDisplayTimeMs()
{
  return play.text_min_display_time_ms;
}

void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime)
{
  play.text_min_display_time_ms = newTextMinTime;
}

int Game_GetIgnoreUserInputAfterTextTimeoutMs()
{
  return play.ignore_user_input_after_text_timeout_ms;
}

void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs)
{
  play.ignore_user_input_after_text_timeout_ms = newValueMs;
}

const char *Game_GetFileName() {
  return CreateNewScriptString(usetup.main_data_filename);
}

const char *Game_GetName() {
  return CreateNewScriptString(play.game_name);
}

void Game_SetName(const char *newName) {
  strncpy(play.game_name, newName, 99);
  play.game_name[99] = 0;

#if (ALLEGRO_DATE > 19990103)
  set_window_title(play.game_name);
#endif
}




void QuitGame(int dialog) {
  if (dialog) {
    int rcode;
    setup_for_dialog();
    rcode=quitdialog();
    restore_after_dialog();
    if (rcode==0) return;
    }
  quit("|You have exited.");
  }




void SetRestartPoint() {
  save_game(RESTART_POINT_SAVE_GAME_NUMBER, "Restart Game Auto-Save");
}



void SetGameSpeed(int newspd) {
  // if Ctrl+E has been used to max out frame rate, lock it there
  if ((frames_per_second == 1000) && (display_fps == 2))
    return;

  newspd += play.game_speed_modifier;
  if (newspd>1000) newspd=1000;
  if (newspd<10) newspd=10;
  set_game_speed(newspd);
  DEBUG_CONSOLE("Game speed set to %d", newspd);
}

int GetGameSpeed() {
  return frames_per_second - play.game_speed_modifier;
}

int SetGameOption (int opt, int setting) {
  if (((opt < 1) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    quit("!SetGameOption: invalid option specified");

  if (opt == OPT_ANTIGLIDE)
  {
    for (int i = 0; i < game.numcharacters; i++)
    {
      if (setting)
        game.chars[i].flags |= CHF_ANTIGLIDE;
      else
        game.chars[i].flags &= ~CHF_ANTIGLIDE;
    }
  }

  if ((opt == OPT_CROSSFADEMUSIC) && (game.audioClipTypeCount > AUDIOTYPE_LEGACY_MUSIC))
  {
    // legacy compatibility -- changing crossfade speed here also
    // updates the new audio clip type style
    game.audioClipTypes[AUDIOTYPE_LEGACY_MUSIC].crossfadeSpeed = setting;
  }
  
  int oldval = game.options[opt];
  game.options[opt] = setting;

  if (opt == OPT_DUPLICATEINV)
    update_invorder();
  else if (opt == OPT_DISABLEOFF)
    gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);
  else if (opt == OPT_PORTRAITSIDE) {
    if (setting == 0)  // set back to Left
      play.swap_portrait_side = 0;
  }

  return oldval;
}

int GetGameOption (int opt) {
  if (((opt < 1) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    quit("!GetGameOption: invalid option specified");

  return game.options[opt];
}




// save game functions
#define SGVERSION 8
char*sgsig="Adventure Game Studio saved game";
int sgsiglen=32;



void serialize_bitmap(block thispic, FILE*ooo) {
  if (thispic != NULL) {
    putw(thispic->w,ooo);
    putw(thispic->h,ooo);
    putw(bitmap_color_depth(thispic),ooo);
    for (int cc=0;cc<thispic->h;cc++)
      fwrite(&thispic->line[cc][0],thispic->w,bitmap_color_depth(thispic)/8,ooo);
    }
  }

// Some people have been having crashes with the save game list,
// so make sure the game name is valid
void safeguard_string (unsigned char *descript) {
  int it;
  for (it = 0; it < 50; it++) {
    if ((descript[it] < 1) || (descript[it] > 127))
      break;
  }
  if (descript[it] != 0)
    descript[it] = 0;
}

// On Windows we could just use IIDFromString but this is platform-independant
void convert_guid_from_text_to_binary(const char *guidText, unsigned char *buffer) 
{
  guidText++; // skip {
  for (int bytesDone = 0; bytesDone < 16; bytesDone++)
  {
    if (*guidText == '-')
      guidText++;

    char tempString[3];
    tempString[0] = guidText[0];
    tempString[1] = guidText[1];
    tempString[2] = 0;
    int thisByte = 0;
    sscanf(tempString, "%X", &thisByte);

    buffer[bytesDone] = thisByte;
    guidText += 2;
  }

  // Swap bytes to give correct GUID order
  unsigned char temp;
  temp = buffer[0]; buffer[0] = buffer[3]; buffer[3] = temp;
  temp = buffer[1]; buffer[1] = buffer[2]; buffer[2] = temp;
  temp = buffer[4]; buffer[4] = buffer[5]; buffer[5] = temp;
  temp = buffer[6]; buffer[6] = buffer[7]; buffer[7] = temp;
}

block read_serialized_bitmap(FILE* ooo) {
  block thispic;
  int picwid = getw(ooo);
  int pichit = getw(ooo);
  int piccoldep = getw(ooo);
  thispic = create_bitmap_ex(piccoldep,picwid,pichit);
  if (thispic == NULL)
    return NULL;
  for (int vv=0; vv < pichit; vv++)
    fread(&thispic->line[vv][0], picwid, piccoldep/8, ooo);
  return thispic;
}




long write_screen_shot_for_vista(FILE *ooo, block screenshot) 
{
  long fileSize = 0;
  char tempFileName[MAX_PATH];
  sprintf(tempFileName, "%s""_tmpscht.bmp", saveGameDirectory);
  
  save_bitmap(tempFileName, screenshot, palette);

  update_polled_stuff_if_runtime();
  
  if (exists(tempFileName))
  {
    fileSize = file_size(tempFileName);
    char *buffer = (char*)malloc(fileSize);

    FILE *input = fopen(tempFileName, "rb");
    fread(buffer, fileSize, 1, input);
    fclose(input);
    unlink(tempFileName);

    fwrite(buffer, fileSize, 1, ooo);
    free(buffer);
  }
  return fileSize;
}


void save_game_screenshot(FILE *ooo, block &screenshot)
{
    // store the screenshot at the start to make it easily accesible
    putw((screenshot == NULL) ? 0 : 1, ooo);

    if (screenshot)
        serialize_bitmap(screenshot, ooo);
}

void save_game_header(FILE *ooo)
{
    fputstring(ACI_VERSION_TEXT, ooo);
    fputstring(usetup.main_data_filename, ooo);
}

void save_game_head_dynamic_values(FILE *ooo)
{
    putw(scrnhit,ooo);
    putw(final_col_dep, ooo);
    putw(frames_per_second,ooo);
    putw(cur_mode,ooo);
    putw(cur_cursor,ooo);
    putw(offsetx,ooo); putw(offsety,ooo);
    putw(loopcounter,ooo);
}

void save_game_spriteset(FILE *ooo)
{
    putw(spriteset.elements, ooo);
    for (int bb = 1; bb < spriteset.elements; bb++) {
        if (game.spriteflags[bb] & SPF_DYNAMICALLOC) {
            putw(bb, ooo);
            fputc(game.spriteflags[bb], ooo);
            serialize_bitmap(spriteset[bb], ooo);
        }
    }
    // end of dynamic sprite list
    putw(0, ooo);
}

void save_game_scripts(FILE *ooo)
{
    // write the data segment of the global script
    int gdatasize=gameinst->globaldatasize;
    putw(gdatasize,ooo);
    ccFlattenGlobalData (gameinst);
    // MACPORT FIX: just in case gdatasize is 2 or 4, don't want to swap endian
    fwrite(&gameinst->globaldata[0], 1, gdatasize, ooo);
    ccUnFlattenGlobalData (gameinst);
    // write the script modules data segments
    putw(numScriptModules, ooo);
    for (int bb = 0; bb < numScriptModules; bb++) {
        int glsize = moduleInst[bb]->globaldatasize;
        putw(glsize, ooo);
        if (glsize > 0) {
            ccFlattenGlobalData(moduleInst[bb]);
            fwrite(&moduleInst[bb]->globaldata[0], 1, glsize, ooo);
            ccUnFlattenGlobalData(moduleInst[bb]);
        }
    }
}

void save_game_room_state(FILE *ooo)
{
    putw(displayed_room, ooo);
    if (displayed_room >= 0) {
        // update the current room script's data segment copy
        if (roominst!=NULL)
            save_room_data_segment();

        // Update the saved interaction variable values
        for (ff = 0; ff < thisroom.numLocalVars; ff++)
            croom->interactionVariableValues[ff] = thisroom.localvars[ff].value;
    }

    // write the room state for all the rooms the player has been in
    for (int bb = 0; bb < MAX_ROOMS; bb++) {
        if (roomstats[bb].beenhere) {
            fputc (1, ooo);
            fwrite(&roomstats[bb],sizeof(RoomStatus),1,ooo);
            if (roomstats[bb].tsdatasize>0)
                fwrite(&roomstats[bb].tsdata[0], 1, roomstats[bb].tsdatasize, ooo);
        }
        else
            fputc (0, ooo);
    }
}

void save_game_play_ex_data(FILE *ooo)
{
    for (int bb = 0; bb < play.num_do_once_tokens; bb++)
    {
        fputstring(play.do_once_tokens[bb], ooo);
    }
    fwrite(&play.gui_draw_order[0], sizeof(int), game.numgui, ooo);
}

void save_game_movelist(FILE *ooo)
{
    fwrite(&mls[0],sizeof(MoveList), game.numcharacters + MAX_INIT_SPR + 1, ooo);
}

void save_game_charextras(FILE *ooo)
{
    fwrite(&charextra[0],sizeof(CharacterExtras),game.numcharacters,ooo);
}

void save_game_palette(FILE *ooo)
{
    fwrite(&palette[0],sizeof(color),256,ooo);
}

void save_game_dialogs(FILE *ooo)
{
    for (int bb=0;bb<game.numdialog;bb++)
        fwrite(&dialog[bb].optionflags[0],sizeof(int),MAXTOPICOPTIONS,ooo);
}

// [IKM] yea, okay this is just plain silly name :)
void save_game_more_dynamic_values(FILE *ooo)
{
    putw(mouse_on_iface,ooo);
    putw(mouse_on_iface_button,ooo);
    putw(mouse_pushed_iface,ooo);
    putw (ifacepopped, ooo);
    putw(game_paused,ooo);
    //putw(mi.trk,ooo);
}

void save_game_gui(FILE *ooo)
{
    write_gui(ooo,guis,&game);
    putw(numAnimButs, ooo);
    fwrite(&animbuts[0], sizeof(AnimatingGUIButton), numAnimButs, ooo);
}

void save_game_audiocliptypes(FILE *ooo)
{
    putw(game.audioClipTypeCount, ooo);
    fwrite(&game.audioClipTypes[0], sizeof(AudioClipType), game.audioClipTypeCount, ooo);
}

void save_game_thisroom(FILE *ooo)
{
    fwrite(&thisroom.regionLightLevel[0],sizeof(short), MAX_REGIONS,ooo);
    fwrite(&thisroom.regionTintLevel[0],sizeof(int), MAX_REGIONS,ooo);
    fwrite(&thisroom.walk_area_zoom[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);
    fwrite(&thisroom.walk_area_zoom2[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);
}

void save_game_ambientsounds(FILE *ooo)
{
    fwrite (&ambient[0], sizeof(AmbientSound), MAX_SOUND_CHANNELS, ooo);
}

void save_game_overlays(FILE *ooo)
{
    putw(numscreenover,ooo);
    fwrite(&screenover[0],sizeof(ScreenOverlay),numscreenover,ooo);
    for (int bb=0;bb<numscreenover;bb++) {
        serialize_bitmap (screenover[bb].pic, ooo);
    }
}

void save_game_dynamic_surfaces(FILE *ooo)
{
    for (int bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
    {
        if (dynamicallyCreatedSurfaces[bb] == NULL)
        {
            fputc(0, ooo);
        }
        else
        {
            fputc(1, ooo);
            serialize_bitmap(dynamicallyCreatedSurfaces[bb], ooo);
        }
    }
}

void save_game_displayed_room_status(FILE *ooo)
{
    if (displayed_room >= 0) {

        for (int bb = 0; bb < MAX_BSCENE; bb++) {
            if (play.raw_modified[bb])
                serialize_bitmap (thisroom.ebscene[bb], ooo);
        }

        putw ((raw_saved_screen == NULL) ? 0 : 1, ooo);
        if (raw_saved_screen)
            serialize_bitmap (raw_saved_screen, ooo);

        // save the current troom, in case they save in room 600 or whatever
        fwrite(&troom,sizeof(RoomStatus),1,ooo);
        if (troom.tsdatasize>0)
            fwrite(&troom.tsdata[0],troom.tsdatasize,1,ooo);

    }
}

void save_game_globalvars(FILE *ooo)
{
    putw (numGlobalVars, ooo);
    fwrite (&globalvars[0], sizeof(InteractionVariable), numGlobalVars, ooo);
}

void save_game_views(FILE *ooo)
{
    putw (game.numviews, ooo);
    for (int bb = 0; bb < game.numviews; bb++) {
        for (int cc = 0; cc < views[bb].numLoops; cc++) {
            for (int dd = 0; dd < views[bb].loops[cc].numFrames; dd++)
            {
                putw(views[bb].loops[cc].frames[dd].sound, ooo);
                putw(views[bb].loops[cc].frames[dd].pic, ooo);
            }
        }
    }
}

void save_game_audioclips_and_crossfade(FILE *ooo)
{
    putw(game.audioClipCount, ooo);
    for (int bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        if ((channels[bb] != NULL) && (channels[bb]->done == 0) && (channels[bb]->sourceClip != NULL))
        {
            putw(((ScriptAudioClip*)channels[bb]->sourceClip)->id, ooo);
            putw(channels[bb]->get_pos(), ooo);
            putw(channels[bb]->priority, ooo);
            putw(channels[bb]->repeat ? 1 : 0, ooo);
            putw(channels[bb]->vol, ooo);
            putw(channels[bb]->panning, ooo);
            putw(channels[bb]->volAsPercentage, ooo);
            putw(channels[bb]->panningAsPercentage, ooo);
        }
        else
        {
            putw(-1, ooo);
        }
    }
    putw(crossFading, ooo);
    putw(crossFadeVolumePerStep, ooo);
    putw(crossFadeStep, ooo);
    putw(crossFadeVolumeAtStart, ooo);
}

#define MAGICNUMBER 0xbeefcafe
// Write the save game position to the file
void save_game_data (FILE *ooo, block screenshot) {

  platform->RunPluginHooks(AGSE_PRESAVEGAME, 0);
  putw(SGVERSION,ooo);

  save_game_screenshot(ooo, screenshot);
  save_game_header(ooo);
  save_game_head_dynamic_values(ooo);
  save_game_spriteset(ooo);
  save_game_scripts(ooo);
  save_game_room_state(ooo);

  update_polled_stuff_if_runtime();

  if (play.cur_music_number >= 0) {
      if (IsMusicPlaying() == 0)
          play.cur_music_number = -1;
  }

  //----------------------------------------------------------------
  play.WriteToFile(ooo);
  //fwrite(&play,sizeof(GameState),1,ooo);

  save_game_play_ex_data(ooo);
  //----------------------------------------------------------------

  save_game_movelist(ooo);

  ((GameSetupStructBase*)&game)->WriteToFile(ooo);
  //fwrite(&game,sizeof(GameSetupStructBase),1,ooo);

  //----------------------------------------------------------------
  game.WriteForSaveGame(ooo);
  //fwrite(&game.invinfo[0], sizeof(InventoryItemInfo), game.numinvitems, ooo);
  //fwrite(&game.mcurs[0], sizeof(MouseCursor), game.numcursors, ooo);

  //if (game.invScripts == NULL)
  //{
  //  for (bb = 0; bb < game.numinvitems; bb++)
  //    fwrite (&game.intrInv[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo);
  //  for (bb = 0; bb < game.numcharacters; bb++)
  //    fwrite (&game.intrChar[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo); 
  //}

  //fwrite (&game.options[0], sizeof(int), OPT_HIGHESTOPTION+1, ooo);
  //fputc (game.options[OPT_LIPSYNCTEXT], ooo);

  //fwrite(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,ooo);
  //----------------------------------------------------------------

  save_game_charextras(ooo);
  save_game_palette(ooo);
  save_game_dialogs(ooo);
  save_game_more_dynamic_values(ooo);
  save_game_gui(ooo);
  save_game_audiocliptypes(ooo);
  save_game_thisroom(ooo);
  save_game_ambientsounds(ooo);
  save_game_overlays(ooo);

  update_polled_stuff_if_runtime();

  save_game_dynamic_surfaces(ooo);

  update_polled_stuff_if_runtime();

  save_game_displayed_room_status(ooo);
  save_game_globalvars(ooo);
  save_game_views(ooo);

  putw (MAGICNUMBER+1, ooo);

  save_game_audioclips_and_crossfade(ooo);  

  platform->RunPluginHooks(AGSE_SAVEGAME, (int)ooo);
  putw (MAGICNUMBER, ooo);  // to verify the plugins

  // save the room music volume
  putw(thisroom.options[ST_VOLUME], ooo);

  ccSerializeAllObjects(ooo);

  putw(current_music_type, ooo);

  update_polled_stuff_if_runtime();
}

void create_savegame_screenshot(block &screenShot)
{
    if (game.options[OPT_SAVESCREENSHOT]) {
        int usewid = multiply_up_coordinate(play.screenshot_width);
        int usehit = multiply_up_coordinate(play.screenshot_height);
        if (usewid > virtual_screen->w)
            usewid = virtual_screen->w;
        if (usehit > virtual_screen->h)
            usehit = virtual_screen->h;

        if ((play.screenshot_width < 16) || (play.screenshot_height < 16))
            quit("!Invalid game.screenshot_width/height, must be from 16x16 to screen res");

        if (gfxDriver->UsesMemoryBackBuffer())
        {
            screenShot = create_bitmap_ex(bitmap_color_depth(virtual_screen), usewid, usehit);

            stretch_blit(virtual_screen, screenShot, 0, 0,
                virtual_screen->w, virtual_screen->h, 0, 0,
                screenShot->w, screenShot->h);
        }
        else
        {
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
            int color_depth = (psp_gfx_renderer > 0) ? 32 : final_col_dep;
#else
            int color_depth = final_col_dep;
#endif
            block tempBlock = create_bitmap_ex(color_depth, virtual_screen->w, virtual_screen->h);
            gfxDriver->GetCopyOfScreenIntoBitmap(tempBlock);

            screenShot = create_bitmap_ex(color_depth, usewid, usehit);
            stretch_blit(tempBlock, screenShot, 0, 0,
                tempBlock->w, tempBlock->h, 0, 0,
                screenShot->w, screenShot->h);

            destroy_bitmap(tempBlock);
        }
    }
}

void save_game(int slotn, const char*descript) {
  
  // dont allow save in rep_exec_always, because we dont save
  // the state of blocked scripts
  can_run_delayed_command();

  if (inside_script) {
    strcpy(curscript->postScriptSaveSlotDescription[curscript->queue_action(ePSASaveGame, slotn, "SaveGameSlot")], descript);
    return;
  }

  if (platform->GetDiskFreeSpaceMB() < 2) {
    Display("ERROR: There is not enough disk space free to save the game. Clear some disk space and try again.");
    return;
  }

  VALIDATE_STRING(descript);
  char nametouse[260];
  get_save_game_path(slotn, nametouse);

  FILE *ooo = fopen(nametouse, "wb");
  if (ooo == NULL)
    quit("save_game: unable to open savegame file for writing");

  // Initialize and write Vista header
  RICH_GAME_MEDIA_HEADER vistaHeader;
  memset(&vistaHeader, 0, sizeof(RICH_GAME_MEDIA_HEADER));
  memcpy(&vistaHeader.dwMagicNumber, RM_MAGICNUMBER, sizeof(long));
  vistaHeader.dwHeaderVersion = 1;
  vistaHeader.dwHeaderSize = sizeof(RICH_GAME_MEDIA_HEADER);
  vistaHeader.dwThumbnailOffsetHigherDword = 0;
  vistaHeader.dwThumbnailOffsetLowerDword = 0;
  vistaHeader.dwThumbnailSize = 0;
  convert_guid_from_text_to_binary(game.guid, &vistaHeader.guidGameId[0]);
  uconvert(game.gamename, U_ASCII, (char*)&vistaHeader.szGameName[0], U_UNICODE, RM_MAXLENGTH);
  uconvert(descript, U_ASCII, (char*)&vistaHeader.szSaveName[0], U_UNICODE, RM_MAXLENGTH);
  vistaHeader.szLevelName[0] = 0;
  vistaHeader.szComments[0] = 0;

  //===================================================================
  // Started writing to the file here

  // Extended savegame info for Win Vista and higher
  fwrite(&vistaHeader, sizeof(RICH_GAME_MEDIA_HEADER), 1, ooo);

  // Savegame signature
  fwrite(sgsig,sgsiglen,1,ooo);

  safeguard_string ((unsigned char*)descript);

  fputstring((char*)descript,ooo);

  block screenShot = NULL;

  // Screenshot
  create_savegame_screenshot(screenShot);

  update_polled_stuff_if_runtime();

  // Actual dynamic game data is saved here
  save_game_data(ooo, screenShot);

  // End writing to the file here
  //===================================================================

  if (screenShot != NULL)
  {
    long screenShotOffset = ftell(ooo) - sizeof(RICH_GAME_MEDIA_HEADER);
    long screenShotSize = write_screen_shot_for_vista(ooo, screenShot);
    fclose(ooo);

    update_polled_stuff_if_runtime();

    ooo = fopen(nametouse, "r+b");
    fseek(ooo, 12, SEEK_SET);
    putw(screenShotOffset, ooo);
    fseek(ooo, 4, SEEK_CUR);
    putw(screenShotSize, ooo);
  }

  if (screenShot != NULL)
    free(screenShot);

  fclose(ooo);
}

char rbuffer[200];

void restore_game_screenshot(FILE *ooo)
{
    int isScreen = getw(ooo);
    if (isScreen) {
        // skip the screenshot
        wfreeblock(read_serialized_bitmap(ooo));
    }
}

int restore_game_header(FILE *ooo)
{
    fgetstring_limit(rbuffer, ooo, 200);
    int vercmp = strcmp(rbuffer, ACI_VERSION_TEXT);
    if ((vercmp > 0) || (strcmp(rbuffer, LOWEST_SGVER_COMPAT) < 0) ||
        (strlen(rbuffer) > strlen(LOWEST_SGVER_COMPAT))) {
            fclose(ooo);
            return -4;
    }
    fgetstring_limit (rbuffer, ooo, 180);
    rbuffer[180] = 0;
    if (stricmp (rbuffer, usetup.main_data_filename)) {
        fclose(ooo);
        return -5;
    }

    return 0;
}

int restore_game_head_dynamic_values(FILE *ooo, int &sg_cur_mode, int &sg_cur_cursor)
{
    int gamescrnhit = getw(ooo);
    // a 320x240 game, they saved in a 320x200 room but try to restore
    // from within a 320x240 room, make it work
    if (final_scrn_hit == (gamescrnhit * 12) / 10)
        gamescrnhit = scrnhit;
    // they saved in a 320x240 room but try to restore from a 320x200
    // room, fix it
    else if (gamescrnhit == final_scrn_hit)
        gamescrnhit = scrnhit;

    if (gamescrnhit != scrnhit) {
        Display("This game was saved with the interpreter running at a different "
            "resolution. It cannot be restored.");
        fclose(ooo);
        return -6;
    }

    if (getw(ooo) != final_col_dep) {
        Display("This game was saved with the engine running at a different colour depth. It cannot be restored.");
        fclose(ooo);
        return -7;
    }

    unload_old_room();

    remove_screen_overlay(-1);
    is_complete_overlay=0; is_text_overlay=0;
    set_game_speed(getw(ooo));
    sg_cur_mode=getw(ooo);
    sg_cur_cursor=getw(ooo);
    offsetx = getw(ooo);
    offsety = getw(ooo);
    loopcounter = getw(ooo);

    return 0;
}

void restore_game_spriteset(FILE *ooo)
{
    for (int bb = 1; bb < spriteset.elements; bb++) {
        if (game.spriteflags[bb] & SPF_DYNAMICALLOC) {
            // do this early, so that it changing guibuts doesn't
            // affect the restored data
            free_dynamic_sprite(bb);
        }
    }
    // ensure the sprite set is at least as large as it was
    // when the game was saved
    spriteset.enlargeTo(getw(ooo));
    // get serialized dynamic sprites
    int sprnum = getw(ooo);
    while (sprnum) {
        unsigned char spriteflag = fgetc(ooo);
        add_dynamic_sprite(sprnum, read_serialized_bitmap(ooo));
        game.spriteflags[sprnum] = spriteflag;
        sprnum = getw(ooo);
    }
}

void restore_game_clean_gfx()
{
    for (int vv = 0; vv < game.numgui; vv++) {
        if (guibg[vv])
            wfreeblock (guibg[vv]);
        guibg[vv] = NULL;

        if (guibgbmp[vv])
            gfxDriver->DestroyDDB(guibgbmp[vv]);
        guibgbmp[vv] = NULL;
    }
}

void restore_game_clean_scripts()
{
    ccFreeInstance(gameinstFork);
    ccFreeInstance(gameinst);
    gameinstFork = NULL;
    gameinst = NULL;
    for (int vv = 0; vv < numScriptModules; vv++) {
        ccFreeInstance(moduleInstFork[vv]);
        ccFreeInstance(moduleInst[vv]);
        moduleInst[vv] = NULL;
    }

    if (dialogScriptsInst != NULL)
    {
        ccFreeInstance(dialogScriptsInst);
        dialogScriptsInst = NULL;
    }
}

void restore_game_scripts(FILE *ooo, int &gdatasize, char **newglobaldatabuffer, 
                          char **scriptModuleDataBuffers, int *scriptModuleDataSize)
{
    // read the global script data segment
    gdatasize = getw(ooo);
    *newglobaldatabuffer = (char*)malloc(gdatasize);
    fread(newglobaldatabuffer, sizeof(char), gdatasize, ooo);
    //fread(&gameinst->globaldata[0],gdatasize,1,ooo);
    //ccUnFlattenGlobalData (gameinst);

    

    if (getw(ooo) != numScriptModules)
        quit("wrong script module count; cannot restore game");
    for (int vv = 0; vv < numScriptModules; vv++) {
        scriptModuleDataSize[vv] = getw(ooo);
        scriptModuleDataBuffers[vv] = (char*)malloc(scriptModuleDataSize[vv]);
        fread(&scriptModuleDataBuffers[vv][0], sizeof(char), scriptModuleDataSize[vv], ooo);
    }
}

void restore_game_room_state(FILE *ooo, const char *nametouse)
{
    int vv;

    displayed_room = getw(ooo);

    // now the rooms
    for (vv=0;vv<MAX_ROOMS;vv++) {
        if (roomstats[vv].tsdata==NULL) ;
        else if (roomstats[vv].tsdatasize>0) {
            free(roomstats[vv].tsdata);
            roomstats[vv].tsdatasize=0; roomstats[vv].tsdata=NULL;
        }
        roomstats[vv].beenhere=0;
    }
    long gobackto=ftell(ooo);
    fclose(ooo);
    ooo=fopen(nametouse,"rb");
    fseek(ooo,gobackto,SEEK_SET);

    // read the room state for all the rooms the player has been in
    for (vv=0;vv<MAX_ROOMS;vv++) {
        if ((roomstats[vv].tsdatasize>0) & (roomstats[vv].tsdata!=NULL))
            free(roomstats[vv].tsdata);
        roomstats[vv].tsdatasize=0;
        roomstats[vv].tsdata=NULL;
        roomstats[vv].beenhere = fgetc (ooo);

        if (roomstats[vv].beenhere) {
            fread(&roomstats[vv],sizeof(RoomStatus),1,ooo);
            if (roomstats[vv].tsdatasize>0) {
                roomstats[vv].tsdata=(char*)malloc(roomstats[vv].tsdatasize+8);
                fread(&roomstats[vv].tsdata[0],roomstats[vv].tsdatasize,1,ooo);
            }
        }
    }

    /*  for (vv=0;vv<MAX_ROOMS;vv++) {
    if ((roomstats[vv].tsdatasize>0) & (roomstats[vv].tsdata!=NULL))
    free(roomstats[vv].tsdata);
    roomstats[vv].tsdatasize=0;
    roomstats[vv].tsdata=NULL;
    }
    int numtoread=getw(ooo);
    if ((numtoread < 0) | (numtoread>MAX_ROOMS)) {
    sprintf(rbuffer,"Save game has invalid value for rooms_entered: %d",numtoread);
    quit(rbuffer);
    }
    fread(&roomstats[0],sizeof(RoomStatus),numtoread,ooo);
    for (vv=0;vv<numtoread;vv++) {
    if (roomstats[vv].tsdatasize>0) {
    roomstats[vv].tsdata=(char*)malloc(roomstats[vv].tsdatasize+5);
    fread(&roomstats[vv].tsdata[0],roomstats[vv].tsdatasize,1,ooo);
    }
    else roomstats[vv].tsdata=NULL;
    }*/
}

void restore_game_play(FILE *ooo)
{
    int speech_was = play.want_speech, musicvox = play.seperate_music_lib;
    // preserve the replay settings
    int playback_was = play.playback, recording_was = play.recording;
    int gamestep_was = play.gamestep;
    int screenfadedout_was = play.screen_is_faded_out;
    int roomchanges_was = play.room_changes;
    // make sure the pointer is preserved
    int *gui_draw_order_was = play.gui_draw_order;

    free_do_once_tokens();

    //fread (&play, 76, 4, ooo);
    //fread (((char*)&play) + 78*4, sizeof(GameState) - 78*4, 1, ooo);

    //fread(&play,sizeof(GameState),1,ooo);
    play.ReadFromFile(ooo);

    // Preserve whether the music vox is available
    play.seperate_music_lib = musicvox;
    // If they had the vox when they saved it, but they don't now
    if ((speech_was < 0) && (play.want_speech >= 0))
        play.want_speech = (-play.want_speech) - 1;
    // If they didn't have the vox before, but now they do
    else if ((speech_was >= 0) && (play.want_speech < 0))
        play.want_speech = (-play.want_speech) - 1;

    play.screen_is_faded_out = screenfadedout_was;
    play.playback = playback_was;
    play.recording = recording_was;
    play.gamestep = gamestep_was;
    play.room_changes = roomchanges_was;
    play.gui_draw_order = gui_draw_order_was;

    if (play.num_do_once_tokens > 0)
    {
        play.do_once_tokens = (char**)malloc(sizeof(char*) * play.num_do_once_tokens);
        for (int bb = 0; bb < play.num_do_once_tokens; bb++)
        {
            fgetstring_limit(rbuffer, ooo, 200);
            play.do_once_tokens[bb] = (char*)malloc(strlen(rbuffer) + 1);
            strcpy(play.do_once_tokens[bb], rbuffer);
        }
    }

    fread(&play.gui_draw_order[0], sizeof(int), game.numgui, ooo);
}

void restore_game_movelist(FILE *ooo)
{
    fread(&mls[0],sizeof(MoveList), game.numcharacters + MAX_INIT_SPR + 1, ooo);
}

void restore_game_charextras(FILE *ooo)
{
    fread(&charextra[0],sizeof(CharacterExtras),game.numcharacters,ooo);
}

void restore_game_palette(FILE *ooo)
{
    fread(&palette[0],sizeof(color),256,ooo);
}

void restore_game_dialogs(FILE *ooo)
{
    for (int vv=0;vv<game.numdialog;vv++)
        fread(&dialog[vv].optionflags[0],sizeof(int),MAXTOPICOPTIONS,ooo);
}

void restore_game_more_dynamic_values(FILE *ooo)
{
    mouse_on_iface=getw(ooo);
    mouse_on_iface_button=getw(ooo);
    mouse_pushed_iface=getw(ooo);
    ifacepopped = getw(ooo);
    game_paused=getw(ooo);
}

void restore_game_gui(FILE *ooo, int numGuisWas)
{
    int vv;
    for (vv = 0; vv < game.numgui; vv++)
        unexport_gui_controls(vv);

    read_gui(ooo,guis,&game);

    if (numGuisWas != game.numgui)
        quit("!Restore_Game: Game has changed (GUIs), unable to restore position");

    for (vv = 0; vv < game.numgui; vv++)
        export_gui_controls(vv);

    numAnimButs = getw(ooo);
    fread(&animbuts[0], sizeof(AnimatingGUIButton), numAnimButs, ooo);
}

void restore_game_audiocliptypes(FILE *ooo)
{
    if (getw(ooo) != game.audioClipTypeCount)
        quit("!Restore_Game: game has changed (audio types), unable to restore");

    fread(&game.audioClipTypes[0], sizeof(AudioClipType), game.audioClipTypeCount, ooo);
}

void restore_game_thisroom(FILE *ooo, short *saved_light_levels, int *saved_tint_levels,
                           short *saved_zoom_levels1, short *saved_zoom_levels2)
{
    fread(&saved_light_levels[0], sizeof(short), MAX_REGIONS, ooo);
    fread(&saved_tint_levels[0], sizeof(int), MAX_REGIONS, ooo);
    fread(&saved_zoom_levels1[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);
    fread(&saved_zoom_levels2[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);
}

void restore_game_ambientsounds(FILE *ooo, int crossfadeInChannelWas, int crossfadeOutChannelWas,
                                int *doAmbient)
{
    int bb;
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        stop_and_destroy_channel_ex(bb, false);
    }

    play.crossfading_in_channel = crossfadeInChannelWas;
    play.crossfading_out_channel = crossfadeOutChannelWas;

    fread(&ambient[0], sizeof(AmbientSound), MAX_SOUND_CHANNELS, ooo);

    for (bb = 1; bb < MAX_SOUND_CHANNELS; bb++) {
        if (ambient[bb].channel == 0)
            doAmbient[bb] = 0;
        else {
            doAmbient[bb] = ambient[bb].num;
            ambient[bb].channel = 0;
        }
    }
}

void restore_game_overlays(FILE *ooo)
{
    numscreenover = getw(ooo);
    fread(&screenover[0],sizeof(ScreenOverlay),numscreenover,ooo);
    for (int bb=0;bb<numscreenover;bb++) {
        if (screenover[bb].pic != NULL)
        {
            screenover[bb].pic = read_serialized_bitmap(ooo);
            screenover[bb].bmp = gfxDriver->CreateDDBFromBitmap(screenover[bb].pic, false);
        }
    }
}

void restore_game_dynamic_surfaces(FILE *ooo, block *dynamicallyCreatedSurfacesFromSaveGame)
{
    // load into a temp array since ccUnserialiseObjects will destroy
    // it otherwise
    for (int bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
    {
        if (fgetc(ooo) == 0)
        {
            dynamicallyCreatedSurfacesFromSaveGame[bb] = NULL;
        }
        else
        {
            dynamicallyCreatedSurfacesFromSaveGame[bb] = read_serialized_bitmap(ooo);
        }
    }
}

void restore_game_displayed_room_status(FILE *ooo, block *newbscene)
{
    int bb;
    for (bb = 0; bb < MAX_BSCENE; bb++)
        newbscene[bb] = NULL;

    if (displayed_room >= 0) {

        for (bb = 0; bb < MAX_BSCENE; bb++) {
            newbscene[bb] = NULL;
            if (play.raw_modified[bb]) {
                newbscene[bb] = read_serialized_bitmap (ooo);
            }
        }
        bb = getw(ooo);
        if (raw_saved_screen != NULL) {
            wfreeblock(raw_saved_screen);
            raw_saved_screen = NULL;
        }
        if (bb)
            raw_saved_screen = read_serialized_bitmap(ooo);

        if (troom.tsdata != NULL)
            free (troom.tsdata);
        // get the current troom, in case they save in room 600 or whatever
        fread(&troom,sizeof(RoomStatus),1,ooo);
        if (troom.tsdatasize > 0) {
            troom.tsdata=(char*)malloc(troom.tsdatasize+5);
            fread(&troom.tsdata[0],troom.tsdatasize,1,ooo);
        }
        else
            troom.tsdata = NULL;

    }
}

void restore_game_globalvars(FILE *ooo)
{
    if (getw (ooo) != numGlobalVars) 
        quit("!Game has been modified since save; unable to restore game (GM01)");

    fread (&globalvars[0], sizeof(InteractionVariable), numGlobalVars, ooo);
}

void restore_game_views(FILE *ooo)
{
    if (getw(ooo) != game.numviews)
        quit("!Game has been modified since save; unable to restore (GV02)");

    for (int bb = 0; bb < game.numviews; bb++) {
        for (int cc = 0; cc < views[bb].numLoops; cc++) {
            for (int dd = 0; dd < views[bb].loops[cc].numFrames; dd++)
            {
                views[bb].loops[cc].frames[dd].sound = getw(ooo);
                views[bb].loops[cc].frames[dd].pic = getw(ooo);
            }
        }
    }
}

void restore_game_audioclips_and_crossfade(FILE *ooo, int crossfadeInChannelWas, int crossfadeOutChannelWas)
{
    int bb;

    if (getw(ooo) != game.audioClipCount)
        quit("Game has changed: different audio clip count");

    play.crossfading_in_channel = 0;
    play.crossfading_out_channel = 0;
    int channelPositions[MAX_SOUND_CHANNELS + 1];
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        channelPositions[bb] = 0;
        int audioClipIndex = getw(ooo);
        if (audioClipIndex >= 0)
        {
            if (audioClipIndex >= game.audioClipCount)
                quit("save game error: invalid audio clip index");

            channelPositions[bb] = getw(ooo);
            if (channelPositions[bb] < 0) channelPositions[bb] = 0;
            int priority = getw(ooo);
            int repeat = getw(ooo);
            int vol = getw(ooo);
            int pan = getw(ooo);
            int volAsPercent = getw(ooo);
            int panAsPercent = getw(ooo);
            play_audio_clip_on_channel(bb, &game.audioClips[audioClipIndex], priority, repeat, channelPositions[bb]);
            if (channels[bb] != NULL)
            {
                channels[bb]->set_panning(pan);
                channels[bb]->set_volume(vol);
                channels[bb]->panningAsPercentage = panAsPercent;
                channels[bb]->volAsPercentage = volAsPercent;
            }
        }
    }
    if ((crossfadeInChannelWas > 0) && (channels[crossfadeInChannelWas] != NULL))
        play.crossfading_in_channel = crossfadeInChannelWas;
    if ((crossfadeOutChannelWas > 0) && (channels[crossfadeOutChannelWas] != NULL))
        play.crossfading_out_channel = crossfadeOutChannelWas;

    // If there were synced audio tracks, the time taken to load in the
    // different channels will have thrown them out of sync, so re-time it
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        if ((channelPositions[bb] > 0) && (channels[bb] != NULL) && (channels[bb]->done == 0))
        {
            channels[bb]->seek(channelPositions[bb]);
        }
    }
    crossFading = getw(ooo);
    crossFadeVolumePerStep = getw(ooo);
    crossFadeStep = getw(ooo);
    crossFadeVolumeAtStart = getw(ooo);
}

int restore_game_data (FILE *ooo, const char *nametouse) {

    int bb, vv;

  if (getw(ooo)!=SGVERSION) {
    fclose(ooo);
    return -3;
  }

  restore_game_screenshot(ooo);
  
  int res = restore_game_header(ooo);
  if (res != 0) {
      return res;
  }

  int sg_cur_mode = 0, sg_cur_cursor = 0;
  res = restore_game_head_dynamic_values(ooo, /*out*/ sg_cur_mode, sg_cur_cursor);
  if (res != 0) {
      return res;
  }
  
  restore_game_spriteset(ooo); 

  clear_music_cache();
  restore_game_clean_gfx();
  
  update_polled_stuff_if_runtime();

  restore_game_clean_scripts();

  update_polled_stuff_if_runtime();

  int gdatasize = 0;
  char*newglobaldatabuffer;
  char *scriptModuleDataBuffers[MAX_SCRIPT_MODULES];
  int scriptModuleDataSize[MAX_SCRIPT_MODULES];
  restore_game_scripts(ooo, /*out*/ gdatasize,&newglobaldatabuffer, scriptModuleDataBuffers, scriptModuleDataSize);
  restore_game_room_state(ooo, nametouse);
  
  restore_game_play(ooo);
  
  restore_game_movelist(ooo);

  // save pointer members before reading
  char* gswas=game.globalscript;
  ccScript* compsc=game.compiled_script;
  CharacterInfo* chwas=game.chars;
  WordsDictionary *olddict = game.dict;
  char* mesbk[MAXGLOBALMES];
  int numchwas = game.numcharacters;
  for (vv=0;vv<MAXGLOBALMES;vv++) mesbk[vv]=game.messages[vv];
  int numdiwas = game.numdialog, numinvwas = game.numinvitems;
  int numviewswas = game.numviews;
  int numGuisWas = game.numgui;

  //fread(&game,sizeof(GameSetupStructBase),1,ooo);
  GameSetupStructBase *gameBase = (GameSetupStructBase *) &game;
  gameBase->ReadFromFile(ooo);

  if (game.numdialog!=numdiwas)
      quit("!Restore_Game: Game has changed (dlg), unable to restore");
  if ((numchwas != game.numcharacters) || (numinvwas != game.numinvitems))
      quit("!Restore_Game: Game has changed (inv), unable to restore position");
  if (game.numviews != numviewswas)
      quit("!Restore_Game: Game has changed (views), unable to restore position");

  game.ReadFromSaveGame(ooo, gswas, compsc, chwas, olddict, mesbk); 
  //
  //fread(&game.invinfo[0], sizeof(InventoryItemInfo), game.numinvitems, ooo);
  //fread(&game.mcurs[0], sizeof(MouseCursor), game.numcursors, ooo);
  //
  //if (game.invScripts == NULL)
  //{
  //  for (bb = 0; bb < game.numinvitems; bb++)
  //    fread (&game.intrInv[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo);
  //  for (bb = 0; bb < game.numcharacters; bb++)
  //    fread (&game.intrChar[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo);
  //}
  //
  //// restore pointer members
  //game.globalscript=gswas;
  //game.compiled_script=compsc;
  //game.chars=chwas;
  //game.dict = olddict;
  //for (vv=0;vv<MAXGLOBALMES;vv++) game.messages[vv]=mesbk[vv];
  //
  //fread(&game.options[0], sizeof(int), OPT_HIGHESTOPTION+1, ooo);
  //game.options[OPT_LIPSYNCTEXT] = fgetc(ooo);
  //
  //fread(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,ooo);
  //
  
  restore_game_charextras(ooo);
  if (roominst!=NULL) {  // so it doesn't overwrite the tsdata
    ccFreeInstance(roominstFork);
    ccFreeInstance(roominst); 
    roominstFork = NULL;
    roominst=NULL;
  }
  restore_game_palette(ooo);
  restore_game_dialogs(ooo);
  restore_game_more_dynamic_values(ooo);

  restore_game_gui(ooo, numGuisWas);
  restore_game_audiocliptypes(ooo);

  short saved_light_levels[MAX_REGIONS];
  int   saved_tint_levels[MAX_REGIONS];
  short saved_zoom_levels1[MAX_WALK_AREAS + 1];
  short saved_zoom_levels2[MAX_WALK_AREAS + 1];
  restore_game_thisroom(ooo, saved_light_levels, saved_tint_levels, saved_zoom_levels1, saved_zoom_levels2);

  int crossfadeInChannelWas = play.crossfading_in_channel;
  int crossfadeOutChannelWas = play.crossfading_out_channel;
  int doAmbient[MAX_SOUND_CHANNELS];
  restore_game_ambientsounds(ooo, crossfadeInChannelWas, crossfadeOutChannelWas, doAmbient);
  restore_game_overlays(ooo);

  update_polled_stuff_if_runtime();

  block dynamicallyCreatedSurfacesFromSaveGame[MAX_DYNAMIC_SURFACES];
  restore_game_dynamic_surfaces(ooo, dynamicallyCreatedSurfacesFromSaveGame);

  update_polled_stuff_if_runtime();

  block newbscene[MAX_BSCENE];
  restore_game_displayed_room_status(ooo, newbscene);
  restore_game_globalvars(ooo);
  restore_game_views(ooo);

  if (getw(ooo) != MAGICNUMBER+1)
    quit("!Game has been modified since save; unable to restore (GV03)");

  restore_game_audioclips_and_crossfade(ooo, crossfadeInChannelWas, crossfadeOutChannelWas);

  recache_queued_clips_after_loading_save_game();

  platform->RunPluginHooks(AGSE_RESTOREGAME, (int)ooo);
  if (getw(ooo) != (unsigned)MAGICNUMBER)
    quit("!One of the game plugins did not restore its game data correctly.");

  // save the new room music vol for later use
  int newRoomVol = getw(ooo);

  if (ccUnserializeAllObjects(ooo, &ccUnserializer))
    quitprintf("LoadGame: Error during deserialization: %s", ccErrorString);

  // preserve legacy music type setting
  current_music_type = getw(ooo);

  fclose(ooo);

  // restore these to the ones retrieved from the save game
  for (bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
  {
    dynamicallyCreatedSurfaces[bb] = dynamicallyCreatedSurfacesFromSaveGame[bb];
  }

  if (create_global_script())
    quitprintf("Unable to recreate global script: %s", ccErrorString);

  if (gameinst->globaldatasize != gdatasize)
    quit("!Restore_game: Global script changed, cannot restore game");

  // read the global data into the newly created script
  memcpy(&gameinst->globaldata[0], newglobaldatabuffer, gdatasize);
  free(newglobaldatabuffer);
  ccUnFlattenGlobalData(gameinst);

  // restore the script module data
  for (bb = 0; bb < numScriptModules; bb++) {
    if (scriptModuleDataSize[bb] != moduleInst[bb]->globaldatasize)
      quit("!Restore Game: script module global data changed, unable to restore");
    memcpy(&moduleInst[bb]->globaldata[0], scriptModuleDataBuffers[bb], scriptModuleDataSize[bb]);
    free(scriptModuleDataBuffers[bb]);
    ccUnFlattenGlobalData(moduleInst[bb]);
  }
  

  setup_player_character(game.playercharacter);

  int gstimer=play.gscript_timer;
  int oldx1 = play.mboundx1, oldx2 = play.mboundx2;
  int oldy1 = play.mboundy1, oldy2 = play.mboundy2;
  int musicWasRepeating = play.current_music_repeating;
  int newms = play.cur_music_number;

  // disable the queue momentarily
  int queuedMusicSize = play.music_queue_size;
  play.music_queue_size = 0;

  update_polled_stuff_if_runtime();

  if (displayed_room >= 0)
    load_new_room(displayed_room,NULL);//&game.chars[game.playercharacter]);

  update_polled_stuff_if_runtime();

  play.gscript_timer=gstimer;

  // restore the correct room volume (they might have modified
  // it with SetMusicVolume)
  thisroom.options[ST_VOLUME] = newRoomVol;

  filter->SetMouseLimit(oldx1,oldy1,oldx2,oldy2);
  
  set_cursor_mode(sg_cur_mode);
  set_mouse_cursor(sg_cur_cursor);
  if (sg_cur_mode == MODE_USE)
    SetActiveInventory (playerchar->activeinv);
  // ensure that the current cursor is locked
  spriteset.precache(game.mcurs[sg_cur_cursor].pic);

#if (ALLEGRO_DATE > 19990103)
  set_window_title(play.game_name);
#endif

  update_polled_stuff_if_runtime();

  if (displayed_room >= 0) {

    for (bb = 0; bb < MAX_BSCENE; bb++) {
      if (newbscene[bb]) {
        wfreeblock(thisroom.ebscene[bb]);
        thisroom.ebscene[bb] = newbscene[bb];
      }
    }

    in_new_room=3;  // don't run "enters screen" events
    // now that room has loaded, copy saved light levels in
    memcpy(&thisroom.regionLightLevel[0],&saved_light_levels[0],sizeof(short)*MAX_REGIONS);
    memcpy(&thisroom.regionTintLevel[0],&saved_tint_levels[0],sizeof(int)*MAX_REGIONS);
    generate_light_table();

    memcpy(&thisroom.walk_area_zoom[0], &saved_zoom_levels1[0], sizeof(short) * (MAX_WALK_AREAS + 1));
    memcpy(&thisroom.walk_area_zoom2[0], &saved_zoom_levels2[0], sizeof(short) * (MAX_WALK_AREAS + 1));

    on_background_frame_change();

  }

  gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);
/*
  play_sound(-1);
  
  stopmusic();
  // use the repeat setting when the current track was started
  int musicRepeatSetting = play.music_repeat;
  SetMusicRepeat(musicWasRepeating);
  if (newms>=0) {
    // restart the background music
    if (newms == 1000)
      PlayMP3File (play.playmp3file_name);
    else {
      play.cur_music_number=2000;  // make sure it gets played
      newmusic(newms);
    }
  }
  SetMusicRepeat(musicRepeatSetting);
  if (play.silent_midi)
    PlaySilentMIDI (play.silent_midi);
  SeekMIDIPosition(midipos);
  //SeekMODPattern (modtrack);
  //SeekMP3PosMillis (mp3mpos);

  if (musicpos > 0) {
    // For some reason, in Prodigal after this Seek line is called
    // it can cause the next update_polled_stuff to crash;
    // must be some sort of bug in AllegroMP3
    if ((crossFading > 0) && (channels[crossFading] != NULL))
      channels[crossFading]->seek(musicpos);
    else if (channels[SCHAN_MUSIC] != NULL)
      channels[SCHAN_MUSIC]->seek(musicpos);
  }*/

  // restore the queue now that the music is playing
  play.music_queue_size = queuedMusicSize;
  
  if (play.digital_master_volume >= 0)
    System_SetVolume(play.digital_master_volume);

  for (vv = 1; vv < MAX_SOUND_CHANNELS; vv++) {
    if (doAmbient[vv])
      PlayAmbientSound(vv, doAmbient[vv], ambient[vv].vol, ambient[vv].x, ambient[vv].y);
  }

  for (vv = 0; vv < game.numgui; vv++) {
    guibg[vv] = create_bitmap_ex (final_col_dep, guis[vv].wid, guis[vv].hit);
    guibg[vv] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[vv]);
  }

  if (gfxDriver->SupportsGammaControl())
    gfxDriver->SetGamma(play.gamma_adjustment);

  guis_need_update = 1;

  play.ignore_user_input_until_time = 0;
  update_polled_stuff_if_runtime();

  platform->RunPluginHooks(AGSE_POSTRESTOREGAME, 0);

  if (displayed_room < 0) {
    // the restart point, no room was loaded
    load_new_room(playerchar->room, playerchar);
    playerchar->prevroom = -1;

    first_room_initialization();
  }

  if ((play.music_queue_size > 0) && (cachedQueuedMusic == NULL)) {
    cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
  }

  return 0;
}

int gameHasBeenRestored = 0;

int do_game_load(const char *nametouse, int slotNumber, char *descrp, int *wantShot)
{
  gameHasBeenRestored++;

  FILE*ooo=fopen(nametouse,"rb");
  if (ooo==NULL)
    return -1;

  // skip Vista header
  fseek(ooo, sizeof(RICH_GAME_MEDIA_HEADER), SEEK_SET);

  fread(rbuffer,sgsiglen,1,ooo);
  rbuffer[sgsiglen]=0;
  if (strcmp(rbuffer,sgsig)!=0) {
    // not a save game
    fclose(ooo);
    return -2; 
  }
  int oldeip = our_eip;
  our_eip = 2050;

  fgetstring_limit(rbuffer,ooo, 180);
  rbuffer[180] = 0;
  safeguard_string ((unsigned char*)rbuffer);

  if (descrp!=NULL) {
    // just want slot description, so return
    strcpy(descrp,rbuffer);
    fclose (ooo);
    our_eip = oldeip;
    return 0;
  }

  if (wantShot != NULL) {
    // just want the screenshot
    if (getw(ooo)!=SGVERSION) {
      fclose(ooo);
      return -3;
    }
    int isScreen = getw(ooo);
    *wantShot = 0;

    if (isScreen) {
      int gotSlot = spriteset.findFreeSlot();
      // load the screenshot
      block redin = read_serialized_bitmap(ooo);
      if (gotSlot > 0) {
        // add it into the sprite set
        add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(redin));

        *wantShot = gotSlot;
      }
      else
      {
        destroy_bitmap(redin);
      }
    }
    fclose (ooo);
    our_eip = oldeip;
    return 0;
  }

  our_eip = 2051;

  // do the actual restore
  int ress = restore_game_data(ooo, nametouse);

  our_eip = oldeip;

  if (ress == -5) {
    // saved in different game
    RunAGSGame (rbuffer, 0, 0);
    load_new_game_restore = slotNumber;
    return 0;
  }

  if (ress)
    return ress;

  run_on_event (GE_RESTORE_GAME, slotNumber);

  // ensure keyboard buffer is clean
  // use the raw versions rather than the rec_ versions so we don't
  // interfere with the replay sync
  while (keypressed()) readkey();

  return 0;
}

int load_game(int slotn, char*descrp, int *wantShot) {
  char nametouse[260];
  get_save_game_path(slotn, nametouse);

  return do_game_load(nametouse, slotn, descrp, wantShot);
}




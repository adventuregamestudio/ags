
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_script.h"
#include "acmain/ac_commonheaders.h"
#include "cs/cc_error.h"
#include "cs/cc_options.h"
#include "cs/cs_runtime.h"
#include "acrun/ac_executingscript.h"
#include "acmain/ac_conversation.h"
#include "acgfx/ac_gfxfilters.h"
#include "acdialog/ac_cscidialog.h"
#include "acmain/ac_variable.h"
#include "acaudio/ac_audio.h"
#include "acaudio/ac_music.h"
#include "acaudio/ac_sound.h"
#include "acmain/ac_video.h"
#include "acchars/ac_charhelpers.h"
#include "acmain/ac_wait.h"
#include "acmain/ac_hotspot.h"
#include "acmain/ac_customproperties.h"
#include "acmain/ac_file.h"
#include "acmain/ac_inventoryitem.h"
#include "acmain/ac_guicontrol.h"
#include "acmain/ac_guilabel.h"
#include "acmain/ac_guibutton.h"
#include "acmain/ac_guislider.h"
#include "acmain/ac_guilistbox.h"
#include "acmain/ac_guitextbox.h"
#include "acmain/ac_guiinvwindow.h"
#include "acmain/ac_controls.h"
#include "acmain/ac_math.h"
#include "acmain/ac_drawingsurface.h"
#include "acmain/ac_inputbox.h"
#include "acmain/ac_system.h"
#include "acmain/ac_parser.h"
#include "acmain/ac_cdplayer.h"
#include "acmain/ac_transition.h"
#include "acmain/ac_display.h"
#include "acmain/ac_plugin.h"



int num_scripts=0;
int post_script_cleanup_stack = 0;

int inside_script=0,in_graph_script=0;
int no_blocking_functions = 0; // set to 1 while in rep_Exec_always

NonBlockingScriptFunction repExecAlways(REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction getDialogOptionsDimensionsFunc("dialog_options_get_dimensions", 1);
NonBlockingScriptFunction renderDialogOptionsFunc("dialog_options_render", 1);
NonBlockingScriptFunction getDialogOptionUnderCursorFunc("dialog_options_get_active", 1);
NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc("dialog_options_mouse_click", 2);

ScriptSystem scsystem;

ccScript *scriptModules[MAX_SCRIPT_MODULES];
ccInstance *moduleInst[MAX_SCRIPT_MODULES];
ccInstance *moduleInstFork[MAX_SCRIPT_MODULES];
char *moduleRepExecAddr[MAX_SCRIPT_MODULES];
int numScriptModules = 0;

char **characterScriptObjNames = NULL;
char objectScriptObjNames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN + 5];
char **guiScriptObjNames = NULL;


char bname[40],bne[40];
char* make_ts_func_name(char*base,int iii,int subd) {
    sprintf(bname,base,iii);
    sprintf(bne,"%s_%c",bname,subd+'a');
    return &bne[0];
}




char scfunctionname[30];
int prepare_text_script(ccInstance*sci,char**tsname) {
  ccError=0;
  if (sci==NULL) return -1;
  if (ccGetSymbolAddr(sci,tsname[0]) == NULL) {
    strcpy (ccErrorString, "no such function in script");
    return -2;
  }
  if (sci->pc!=0) {
    strcpy(ccErrorString,"script is already in execution");
    return -3;
  }
  scripts[num_scripts].init();
  scripts[num_scripts].inst = sci;
/*  char tempb[300];
  sprintf(tempb,"Creating script instance for '%s' room %d",tsname[0],displayed_room);
  write_log(tempb);*/
  if (sci->pc != 0) {
//    write_log("Forking instance");
    scripts[num_scripts].inst = ccForkInstance(sci);
    if (scripts[num_scripts].inst == NULL)
      quit("unable to fork instance for secondary script");
    scripts[num_scripts].forked = 1;
    }
  curscript = &scripts[num_scripts];
  num_scripts++;
  if (num_scripts >= MAX_SCRIPT_AT_ONCE)
    quit("too many nested text script instances created");
  // in case script_run_another is the function name, take a backup
  strcpy(scfunctionname,tsname[0]);
  tsname[0]=&scfunctionname[0];
  update_script_mouse_coords();
  inside_script++;
//  aborted_ip=0;
//  abort_executor=0;
  return 0;
  }

void cancel_all_scripts() {
  int aa;

  for (aa = 0; aa < num_scripts; aa++) {
    if (scripts[aa].forked)
      ccAbortAndDestroyInstance(scripts[aa].inst);
    else
      ccAbortInstance(scripts[aa].inst);
    scripts[aa].numanother = 0;
    }
  num_scripts = 0;
/*  if (gameinst!=NULL) ccAbortInstance(gameinst);
  if (roominst!=NULL) ccAbortInstance(roominst);*/
  }

void post_script_cleanup() {
  // should do any post-script stuff here, like go to new room
  if (ccError) quit(ccErrorString);
  ExecutingScript copyof = scripts[num_scripts-1];
//  write_log("Instance finished.");
  if (scripts[num_scripts-1].forked)
    ccFreeInstance(scripts[num_scripts-1].inst);
  num_scripts--;
  inside_script--;

  if (num_scripts > 0)
    curscript = &scripts[num_scripts-1];
  else {
    curscript = NULL;
  }
//  if (abort_executor) user_disabled_data2=aborted_ip;

  int old_room_number = displayed_room;

  // run the queued post-script actions
  for (int ii = 0; ii < copyof.numPostScriptActions; ii++) {
    int thisData = copyof.postScriptActionData[ii];

    switch (copyof.postScriptActions[ii]) {
    case ePSANewRoom:
      // only change rooms when all scripts are done
      if (num_scripts == 0) {
        new_room(thisData, playerchar);
        // don't allow any pending room scripts from the old room
        // in run_another to be executed
        return;
      }
      else
        curscript->queue_action(ePSANewRoom, thisData, "NewRoom");
      break;
    case ePSAInvScreen:
      invscreen();
      break;
    case ePSARestoreGame:
      cancel_all_scripts();
      load_game_and_print_error(thisData);
      return;
    case ePSARestoreGameDialog:
      restore_game_dialog();
      return;
    case ePSARunAGSGame:
      cancel_all_scripts();
      load_new_game = thisData;
      return;
    case ePSARunDialog:
      do_conversation(thisData);
      break;
    case ePSARestartGame:
      cancel_all_scripts();
      restart_game();
      return;
    case ePSASaveGame:
      save_game(thisData, copyof.postScriptSaveSlotDescription[ii]);
      break;
    case ePSASaveGameDialog:
      save_game_dialog();
      break;
    default:
      quitprintf("undefined post script action found: %d", copyof.postScriptActions[ii]);
    }
    // if the room changed in a conversation, for example, abort
    if (old_room_number != displayed_room) {
      return;
    }
  }


  int jj;
  for (jj = 0; jj < copyof.numanother; jj++) {
    old_room_number = displayed_room;
    char runnext[40];
    strcpy(runnext,copyof.script_run_another[jj]);
    copyof.script_run_another[jj][0]=0;
    if (runnext[0]=='#')
      run_text_script_2iparam(gameinst,&runnext[1],copyof.run_another_p1[jj],copyof.run_another_p2[jj]);
    else if (runnext[0]=='!')
      run_text_script_iparam(gameinst,&runnext[1],copyof.run_another_p1[jj]);
    else if (runnext[0]=='|')
      run_text_script(roominst,&runnext[1]);
    else if (runnext[0]=='%')
      run_text_script_2iparam(roominst, &runnext[1], copyof.run_another_p1[jj], copyof.run_another_p2[jj]);
    else if (runnext[0]=='$') {
      run_text_script_iparam(roominst,&runnext[1],copyof.run_another_p1[jj]);
      play.roomscript_finished = 1;
    }
    else
      run_text_script(gameinst,runnext);

    // if they've changed rooms, cancel any further pending scripts
    if ((displayed_room != old_room_number) || (load_new_game))
      break;
  }
  copyof.numanother = 0;

}



void quit_with_script_error(const char *functionName)
{
    quitprintf("%sError running function '%s':\n%s", (ccErrorIsUserError ? "!" : ""), functionName, ccErrorString);
}

void _do_run_script_func_cant_block(ccInstance *forkedinst, NonBlockingScriptFunction* funcToRun, bool *hasTheFunc) {
    if (!hasTheFunc[0])
        return;

    no_blocking_functions++;
    int result;

    if (funcToRun->numParameters == 0)
        result = ccCallInstance(forkedinst, (char*)funcToRun->functionName, 0);
    else if (funcToRun->numParameters == 1)
        result = ccCallInstance(forkedinst, (char*)funcToRun->functionName, 1, funcToRun->param1);
    else if (funcToRun->numParameters == 2)
        result = ccCallInstance(forkedinst, (char*)funcToRun->functionName, 2, funcToRun->param1, funcToRun->param2);
    else
        quit("_do_run_script_func_cant_block called with too many parameters");

    if (result == -2) {
        // the function doens't exist, so don't try and run it again
        hasTheFunc[0] = false;
    }
    else if ((result != 0) && (result != 100)) {
        quit_with_script_error(funcToRun->functionName);
    }
    else
    {
        funcToRun->atLeastOneImplementationExists = true;
    }
    // this might be nested, so don't disrupt blocked scripts
    ccErrorString[0] = 0;
    ccError = 0;
    no_blocking_functions--;
}

void run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun) {

    update_script_mouse_coords();

    int room_changes_was = play.room_changes;
    funcToRun->atLeastOneImplementationExists = false;

    // run modules
    // modules need a forkedinst for this to work
    for (int kk = 0; kk < numScriptModules; kk++) {
        _do_run_script_func_cant_block(moduleInstFork[kk], funcToRun, &funcToRun->moduleHasFunction[kk]);

        if (room_changes_was != play.room_changes)
            return;
    }

    _do_run_script_func_cant_block(gameinstFork, funcToRun, &funcToRun->globalScriptHasFunction);

    if (room_changes_was != play.room_changes)
        return;

    _do_run_script_func_cant_block(roominstFork, funcToRun, &funcToRun->roomHasFunction);
}

int run_script_function_if_exist(ccInstance*sci,char*tsname,int numParam, int iparam, int iparam2, int iparam3) {
    int oldRestoreCount = gameHasBeenRestored;
    // First, save the current ccError state
    // This is necessary because we might be attempting
    // to run Script B, while Script A is still running in the
    // background.
    // If CallInstance here has an error, it would otherwise
    // also abort Script A because ccError is a global variable.
    int cachedCcError = ccError;
    ccError = 0;

    int toret = prepare_text_script(sci,&tsname);
    if (toret) {
        ccError = cachedCcError;
        return -18;
    }

    // Clear the error message
    ccErrorString[0] = 0;

    if (numParam == 0) 
        toret = ccCallInstance(curscript->inst,tsname,numParam);
    else if (numParam == 1)
        toret = ccCallInstance(curscript->inst,tsname,numParam, iparam);
    else if (numParam == 2)
        toret = ccCallInstance(curscript->inst,tsname,numParam,iparam, iparam2);
    else if (numParam == 3)
        toret = ccCallInstance(curscript->inst,tsname,numParam,iparam, iparam2, iparam3);
    else
        quit("Too many parameters to run_script_function_if_exist");

    // 100 is if Aborted (eg. because we are LoadAGSGame'ing)
    if ((toret != 0) && (toret != -2) && (toret != 100)) {
        quit_with_script_error(tsname);
    }

    post_script_cleanup_stack++;

    if (post_script_cleanup_stack > 50)
        quitprintf("!post_script_cleanup call stack exceeded: possible recursive function call? running %s", tsname);

    post_script_cleanup();

    post_script_cleanup_stack--;

    // restore cached error state
    ccError = cachedCcError;

    // if the game has been restored, ensure that any further scripts are not run
    if ((oldRestoreCount != gameHasBeenRestored) && (eventClaimed == EVENT_INPROGRESS))
        eventClaimed = EVENT_CLAIMED;

    return toret;
}

int run_text_script(ccInstance*sci,char*tsname) {
    if (strcmp(tsname, REP_EXEC_NAME) == 0) {
        // run module rep_execs
        int room_changes_was = play.room_changes;
        int restore_game_count_was = gameHasBeenRestored;

        for (int kk = 0; kk < numScriptModules; kk++) {
            if (moduleRepExecAddr[kk] != NULL)
                run_script_function_if_exist(moduleInst[kk], tsname, 0, 0, 0);

            if ((room_changes_was != play.room_changes) ||
                (restore_game_count_was != gameHasBeenRestored))
                return 0;
        }
    }

    int toret = run_script_function_if_exist(sci, tsname, 0, 0, 0);
    if ((toret == -18) && (sci == roominst)) {
        // functions in room script must exist
        quitprintf("prepare_script: error %d (%s) trying to run '%s'   (Room %d)",toret,ccErrorString,tsname, displayed_room);
    }
    return toret;
}


int run_text_script_iparam(ccInstance*sci,char*tsname,int iparam) {
    if ((strcmp(tsname, "on_key_press") == 0) || (strcmp(tsname, "on_mouse_click") == 0)) {
        bool eventWasClaimed;
        int toret = run_claimable_event(tsname, true, 1, iparam, 0, &eventWasClaimed);

        if (eventWasClaimed)
            return toret;
    }

    return run_script_function_if_exist(sci, tsname, 1, iparam, 0);
}

int run_text_script_2iparam(ccInstance*sci,char*tsname,int iparam,int param2) {
    if (strcmp(tsname, "on_event") == 0) {
        bool eventWasClaimed;
        int toret = run_claimable_event(tsname, true, 2, iparam, param2, &eventWasClaimed);

        if (eventWasClaimed)
            return toret;
    }

    // response to a button click, better update guis
    if (strnicmp(tsname, "interface_click", 15) == 0)
        guis_need_update = 1;

    int toret = run_script_function_if_exist(sci, tsname, 2, iparam, param2);

    // tsname is no longer valid, because run_script_function_if_exist might
    // have restored a save game and freed the memory. Therefore don't 
    // attempt any strcmp's here
    tsname = NULL;

    return toret;
}


void get_script_name(ccInstance *rinst, char *curScrName) {
    if (rinst == NULL)
        strcpy (curScrName, "Not in a script");
    else if (rinst->instanceof == gamescript)
        strcpy (curScrName, "Global script");
    else if (rinst->instanceof == thisroom.compiled_script)
        sprintf (curScrName, "Room %d script", displayed_room);
    else
        strcpy (curScrName, "Unknown script");
}


int create_global_script() {
    ccSetOption(SCOPT_AUTOIMPORT, 1);
    for (int kk = 0; kk < numScriptModules; kk++) {
        moduleInst[kk] = ccCreateInstance(scriptModules[kk]);
        if (moduleInst[kk] == NULL)
            return -3;
        // create a forked instance for rep_exec_always
        moduleInstFork[kk] = ccForkInstance(moduleInst[kk]);
        if (moduleInstFork[kk] == NULL)
            return -3;

        moduleRepExecAddr[kk] = ccGetSymbolAddr(moduleInst[kk], REP_EXEC_NAME);
    }
    gameinst = ccCreateInstance(gamescript);
    if (gameinst == NULL)
        return -3;
    // create a forked instance for rep_exec_always
    gameinstFork = ccForkInstance(gameinst);
    if (gameinstFork == NULL)
        return -3;

    if (dialogScriptsScript != NULL)
    {
        dialogScriptsInst = ccCreateInstance(dialogScriptsScript);
        if (dialogScriptsInst == NULL)
            return -3;
    }

    ccSetOption(SCOPT_AUTOIMPORT, 0);
    return 0;
}



void script_debug(int cmdd,int dataa) {
  if (play.debug_mode==0) return;
  int rr;
  if (cmdd==0) {
    for (rr=1;rr<game.numinvitems;rr++)
      playerchar->inv[rr]=1;
    update_invorder();
//    Display("invorder decided there are %d items[display %d",play.inv_numorder,play.inv_numdisp);
    }
  else if (cmdd==1) {
    char toDisplay[STD_BUFFER_SIZE];
    const char *filterName = filter->GetVersionBoxText();
    sprintf(toDisplay,"Adventure Game Studio run-time engine[ACI version " ACI_VERSION_TEXT
      "[Running %d x %d at %d-bit %s[GFX: %s[%s" "Sprite cache size: %ld KB (limit %ld KB; %ld locked)",
      final_scrn_wid,final_scrn_hit,final_col_dep, (convert_16bit_bgr) ? "BGR" : "",
      gfxDriver->GetDriverName(), filterName,
      spriteset.cachesize / 1024, spriteset.maxCacheSize / 1024, spriteset.lockedSize / 1024);
    if (play.seperate_music_lib)
      strcat(toDisplay,"[AUDIO.VOX enabled");
    if (play.want_speech >= 1)
      strcat(toDisplay,"[SPEECH.VOX enabled");
    if (transtree != NULL) {
      strcat(toDisplay,"[Using translation ");
      strcat(toDisplay, transFileName);
    }
    if (opts.mod_player == 0)
      strcat(toDisplay,"[(mod/xm player discarded)");
    Display(toDisplay);
//    Display("shftR: %d  shftG: %d  shftB: %d", _rgb_r_shift_16, _rgb_g_shift_16, _rgb_b_shift_16);
//    Display("Remaining memory: %d kb",_go32_dpmi_remaining_virtual_memory()/1024);
//Display("Play char bcd: %d",bitmap_color_depth(spriteset[views[playerchar->view].frames[playerchar->loop][playerchar->frame].pic]));
    }
  else if (cmdd==2) 
  {  // show walkable areas from here
    block tempw=create_bitmap(thisroom.walls->w,thisroom.walls->h);
    blit(prepare_walkable_areas(-1),tempw,0,0,0,0,tempw->w,tempw->h);
    block stretched = create_bitmap(scrnwid, scrnhit);
    stretch_sprite(stretched, tempw, -offsetx, -offsety, get_fixed_pixel_size(tempw->w), get_fixed_pixel_size(tempw->h));

    IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(stretched, false, true);
    render_graphics(ddb, 0, 0);

    destroy_bitmap(tempw);
    destroy_bitmap(stretched);
    gfxDriver->DestroyDDB(ddb);
    while (!kbhit()) ;
    getch();
    invalidate_screen();
  }
  else if (cmdd==3) 
  {
    int goToRoom = -1;
    if (game.roomCount == 0)
    {
      char inroomtex[80];
      sprintf(inroomtex, "!Enter new room: (in room %d)", displayed_room);
      setup_for_dialog();
      goToRoom = enternumberwindow(inroomtex);
      restore_after_dialog();
    }
    else
    {
      setup_for_dialog();
      goToRoom = roomSelectorWindow(displayed_room, game.roomCount, game.roomNumbers, game.roomNames);
      restore_after_dialog();
    }
    if (goToRoom >= 0) 
      NewRoom(goToRoom);
  }
  else if (cmdd == 4) {
    if (display_fps != 2)
      display_fps = dataa;
  }
  else if (cmdd == 5) {
    if (dataa == 0) dataa = game.playercharacter;
    if (game.chars[dataa].walking < 1) {
      Display("Not currently moving.");
      return;
    }
    block tempw=create_bitmap(thisroom.walls->w,thisroom.walls->h);
    int mlsnum = game.chars[dataa].walking;
    if (game.chars[dataa].walking >= TURNING_AROUND)
      mlsnum %= TURNING_AROUND;
    MoveList*cmls = &mls[mlsnum];
    clear_to_color(tempw, bitmap_mask_color(tempw));
    for (int i = 0; i < cmls->numstage-1; i++) {
      short srcx=short((cmls->pos[i] >> 16) & 0x00ffff);
      short srcy=short(cmls->pos[i] & 0x00ffff);
      short targetx=short((cmls->pos[i+1] >> 16) & 0x00ffff);
      short targety=short(cmls->pos[i+1] & 0x00ffff);
      line (tempw, srcx, srcy, targetx, targety, get_col8_lookup(i+1));
    }
    stretch_sprite(screen, tempw, -offsetx, -offsety, multiply_up_coordinate(tempw->w), multiply_up_coordinate(tempw->h));
    render_to_screen(screen, 0, 0);
    wfreeblock(tempw);
    while (!kbhit()) ;
    getch();
  }
  else if (cmdd == 99)
    ccSetOption(SCOPT_DEBUGRUN, dataa);
  else quit("!Debug: unknown command code");
}



void script_SetTimer(int tnum,int timeout) {
  if ((tnum < 1) || (tnum >= MAX_TIMERS))
    quit("!StartTimer: invalid timer number");
  play.script_timers[tnum] = timeout;
  }





int get_nivalue (NewInteractionCommandList *nic, int idx, int parm) {
  if (nic->command[idx].data[parm].valType == VALTYPE_VARIABLE) {
    // return the value of the variable
    return get_interaction_variable(nic->command[idx].data[parm].val)->value;
  }
  return nic->command[idx].data[parm].val;
}

#define IPARAM1 get_nivalue(nicl, i, 0)
#define IPARAM2 get_nivalue(nicl, i, 1)
#define IPARAM3 get_nivalue(nicl, i, 2)
#define IPARAM4 get_nivalue(nicl, i, 3)
#define IPARAM5 get_nivalue(nicl, i, 4)


struct TempEip {
    int oldval;
    TempEip (int newval) {
        oldval = our_eip;
        our_eip = newval;
    }
    ~TempEip () { our_eip = oldval; }
};


// the 'cmdsrun' parameter counts how many commands are run.
// if a 'Inv Item Was Used' check does not pass, it doesn't count
// so cmdsrun remains 0 if no inventory items matched
int run_interaction_commandlist (NewInteractionCommandList *nicl, int *timesrun, int*cmdsrun) {
  int i;

  if (nicl == NULL)
    return -1;

  for (i = 0; i < nicl->numCommands; i++) {
    cmdsrun[0] ++;
    int room_was = play.room_changes;
    
    switch (nicl->command[i].type) {
      case 0:  // Do nothing
        break;
      case 1:  // Run script
        { 
        TempEip tempip(4001);
        UPDATE_MP3
        if ((strstr(evblockbasename,"character")!=0) || (strstr(evblockbasename,"inventory")!=0)) {
          // Character or Inventory (global script)
          char *torun = make_ts_func_name(evblockbasename,evblocknum,nicl->command[i].data[0].val);
          // we are already inside the mouseclick event of the script, can't nest calls
          if (inside_script) 
            curscript->run_another (torun, 0, 0);
          else run_text_script(gameinst,torun);
          }
        else {
          // Other (room script)
          if (inside_script) {
            char funcName[60];
            strcpy(funcName,"|");
            strcat(funcName,make_ts_func_name(evblockbasename,evblocknum,nicl->command[i].data[0].val));
            curscript->run_another (funcName, 0, 0);
            }
          else
            run_text_script(roominst,make_ts_func_name(evblockbasename,evblocknum,nicl->command[i].data[0].val));
          }
        UPDATE_MP3
        break;
      }
      case 2:  // Add score (first time)
        if (timesrun[0] > 0)
          break;
        timesrun[0] ++;
      case 3:  // Add score
        GiveScore (IPARAM1);
        break;
      case 4:  // Display Message
/*        if (comprdata<0)
          display_message_aschar=evb->data[ss];*/
        DisplayMessage(IPARAM1);
        break;
      case 5:  // Play Music
        PlayMusicResetQueue(IPARAM1);
        break;
      case 6:  // Stop Music
        stopmusic ();
        break;
      case 7:  // Play Sound
        play_sound (IPARAM1);
        break;
      case 8:  // Play Flic
        play_flc_file(IPARAM1, IPARAM2);
        break;
      case 9:  // Run Dialog
		{ int room_was = play.room_changes;
        RunDialog(IPARAM1);
		// if they changed room within the dialog script,
		// the interaction command list is no longer valid
        if (room_was != play.room_changes)
          return -1;
		}
        break;
      case 10: // Enable Dialog Option
        SetDialogOption (IPARAM1, IPARAM2, 1);
        break;
      case 11: // Disable Dialog Option
        SetDialogOption (IPARAM1, IPARAM2, 0);
        break;
      case 12: // Go To Screen
        Character_ChangeRoomAutoPosition(playerchar, IPARAM1, IPARAM2);
        return -1;
      case 13: // Add Inventory
        add_inventory (IPARAM1);
        break;
      case 14: // Move Object
        MoveObject (IPARAM1, IPARAM2, IPARAM3, IPARAM4);
        // if they want to wait until finished, do so
        if (IPARAM5)
          do_main_cycle(UNTIL_MOVEEND,(int)&objs[IPARAM1].moving);
        break;
      case 15: // Object Off
        ObjectOff (IPARAM1);
        break;
      case 16: // Object On
        ObjectOn (IPARAM1);
        break;
      case 17: // Set Object View
        SetObjectView (IPARAM1, IPARAM2);
        break;
      case 18: // Animate Object
        AnimateObject (IPARAM1, IPARAM2, IPARAM3, IPARAM4);
        break;
      case 19: // Move Character
        if (IPARAM4)
          MoveCharacterBlocking (IPARAM1, IPARAM2, IPARAM3, 0);
        else
          MoveCharacter (IPARAM1, IPARAM2, IPARAM3);
        break;
      case 20: // If Inventory Item was used
        if (play.usedinv == IPARAM1) {
          if (game.options[OPT_NOLOSEINV] == 0)
            lose_inventory (play.usedinv);
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        }
        else
          cmdsrun[0] --;
        break;
      case 21: // if player has inventory item
        if (playerchar->inv[IPARAM1] > 0)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 22: // if a character is moving
        if (game.chars[IPARAM1].walking)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 23: // if two variables are equal
        if (IPARAM1 == IPARAM2)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 24: // Stop character walking
        StopMoving (IPARAM1);
        break;
      case 25: // Go to screen at specific co-ordinates
        NewRoomEx (IPARAM1, IPARAM2, IPARAM3);
        return -1;
      case 26: // Move NPC to different room
        if (!is_valid_character(IPARAM1))
          quit("!Move NPC to different room: invalid character specified");
        game.chars[IPARAM1].room = IPARAM2;
        break;
      case 27: // Set character view
        SetCharacterView (IPARAM1, IPARAM2);
        break;
      case 28: // Release character view
        ReleaseCharacterView (IPARAM1);
        break;
      case 29: // Follow character
        FollowCharacter (IPARAM1, IPARAM2);
        break;
      case 30: // Stop following
        FollowCharacter (IPARAM1, -1);
        break;
      case 31: // Disable hotspot
        DisableHotspot (IPARAM1);
        break;
      case 32: // Enable hotspot
        EnableHotspot (IPARAM1);
        break;
      case 33: // Set variable value
        get_interaction_variable(nicl->command[i].data[0].val)->value = IPARAM2;
        break;
      case 34: // Run animation
        scAnimateCharacter(IPARAM1, IPARAM2, IPARAM3, 0);
        do_main_cycle(UNTIL_SHORTIS0,(int)&game.chars[IPARAM1].animating);
        break;
      case 35: // Quick animation
        SetCharacterView (IPARAM1, IPARAM2);
        scAnimateCharacter(IPARAM1, IPARAM3, IPARAM4, 0);
        do_main_cycle(UNTIL_SHORTIS0,(int)&game.chars[IPARAM1].animating);
        ReleaseCharacterView (IPARAM1);
        break;
      case 36: // Set idle animation
        SetCharacterIdle (IPARAM1, IPARAM2, IPARAM3);
        break;
      case 37: // Disable idle animation
        SetCharacterIdle (IPARAM1, -1, -1);
        break;
      case 38: // Lose inventory item
        lose_inventory (IPARAM1);
        break;
      case 39: // Show GUI
        InterfaceOn (IPARAM1);
        break;
      case 40: // Hide GUI
        InterfaceOff (IPARAM1);
        break;
      case 41: // Stop running more commands
        return -1;
      case 42: // Face location
        FaceLocation (IPARAM1, IPARAM2, IPARAM3);
        break;
      case 43: // Pause command processor
        scrWait (IPARAM1);
        break;
      case 44: // Change character view
        ChangeCharacterView (IPARAM1, IPARAM2);
        break;
      case 45: // If player character is
        if (GetPlayerCharacter() == IPARAM1)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 46: // if cursor mode is
        if (GetCursorMode() == IPARAM1)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 47: // if player has been to room
        if (HasBeenToRoom(IPARAM1))
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      default:
        quit("unknown new interaction command");
        break;
    }

    // if the room changed within the action, nicl is no longer valid
    if (room_was != play.room_changes)
      return -1;
  }
  return 0;

}

void run_unhandled_event (int evnt) {

  if (play.check_interaction_only)
    return;

  int evtype=0;
  if (strnicmp(evblockbasename,"hotspot",7)==0) evtype=1;
  else if (strnicmp(evblockbasename,"object",6)==0) evtype=2;
  else if (strnicmp(evblockbasename,"character",9)==0) evtype=3;
  else if (strnicmp(evblockbasename,"inventory",9)==0) evtype=5;
  else if (strnicmp(evblockbasename,"region",6)==0)
    return;  // no unhandled_events for regions

  // clicked Hotspot 0, so change the type code
  if ((evtype == 1) & (evblocknum == 0) & (evnt != 0) & (evnt != 5) & (evnt != 6))
    evtype = 4;
  if ((evtype==1) & ((evnt==0) | (evnt==5) | (evnt==6)))
    ;  // character stands on hotspot, mouse moves over hotspot, any click
  else if ((evtype==2) & (evnt==4)) ;  // any click on object
  else if ((evtype==3) & (evnt==4)) ;  // any click on character
  else if (evtype > 0) {
    can_run_delayed_command();

    if (inside_script)
      curscript->run_another ("#unhandled_event", evtype, evnt);
    else
      run_text_script_2iparam(gameinst,"unhandled_event",evtype,evnt);
  }

}

// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_event (NewInteraction *nint, int evnt, int chkAny, int isInv) {

  if ((nint->response[evnt] == NULL) || (nint->response[evnt]->numCommands == 0)) {
    // no response defined for this event
    // If there is a response for "Any Click", then abort now so as to
    // run that instead
    if (chkAny < 0) ;
    else if ((nint->response[chkAny] != NULL) && (nint->response[chkAny]->numCommands > 0))
      return 0;

    // Otherwise, run unhandled_event
    run_unhandled_event(evnt);
    
    return 0;
  }

  if (play.check_interaction_only) {
    play.check_interaction_only = 2;
    return -1;
  }

  int cmdsrun = 0, retval = 0;
  // Right, so there were some commands defined in response to the event.
  retval = run_interaction_commandlist (nint->response[evnt], &nint->timesRun[evnt], &cmdsrun);

  // An inventory interaction, but the wrong item was used
  if ((isInv) && (cmdsrun == 0))
    run_unhandled_event (evnt);

  return retval;
}

// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_script(InteractionScripts *nint, int evnt, int chkAny, int isInv) {

  if ((nint->scriptFuncNames[evnt] == NULL) || (nint->scriptFuncNames[evnt][0] == 0)) {
    // no response defined for this event
    // If there is a response for "Any Click", then abort now so as to
    // run that instead
    if (chkAny < 0) ;
    else if ((nint->scriptFuncNames[chkAny] != NULL) && (nint->scriptFuncNames[chkAny][0] != 0))
      return 0;

    // Otherwise, run unhandled_event
    run_unhandled_event(evnt);
    
    return 0;
  }

  if (play.check_interaction_only) {
    play.check_interaction_only = 2;
    return -1;
  }

  int room_was = play.room_changes;

  UPDATE_MP3
  if ((strstr(evblockbasename,"character")!=0) || (strstr(evblockbasename,"inventory")!=0)) {
    // Character or Inventory (global script)
    if (inside_script) 
      curscript->run_another (nint->scriptFuncNames[evnt], 0, 0);
    else run_text_script(gameinst, nint->scriptFuncNames[evnt]);
    }
  else {
    // Other (room script)
    if (inside_script) {
      char funcName[60];
      sprintf(funcName, "|%s", nint->scriptFuncNames[evnt]);
      curscript->run_another (funcName, 0, 0);
    }
    else
      run_text_script(roominst, nint->scriptFuncNames[evnt]);
  }
  UPDATE_MP3

  int retval = 0;
  // if the room changed within the action
  if (room_was != play.room_changes)
    retval = -1;

  return retval;
}

int run_dialog_request (int parmtr) {
  play.stop_dialog_at_end = DIALOG_RUNNING;
  run_text_script_iparam(gameinst, "dialog_request", parmtr);

  if (play.stop_dialog_at_end == DIALOG_STOP) {
    play.stop_dialog_at_end = DIALOG_NONE;
    return -2;
  }
  if (play.stop_dialog_at_end >= DIALOG_NEWTOPIC) {
    int tval = play.stop_dialog_at_end - DIALOG_NEWTOPIC;
    play.stop_dialog_at_end = DIALOG_NONE;
    return tval;
  }
  if (play.stop_dialog_at_end >= DIALOG_NEWROOM) {
    int roomnum = play.stop_dialog_at_end - DIALOG_NEWROOM;
    play.stop_dialog_at_end = DIALOG_NONE;
    NewRoom(roomnum);
    return -2;
  }
  play.stop_dialog_at_end = DIALOG_NONE;
  return -1;
}


//char*ac_default_header=NULL,*temphdr=NULL;
char ac_default_header[15000]; // this is not used anywhere (?)
char temphdr[10000];

void setup_exports(char*expfrom) {
  char namof[30]="\0"; temphdr[0]=0;
  while (expfrom[0]!=0) {
    expfrom=strstr(expfrom,"function ");
    if (expfrom==NULL) break;
    if (expfrom[-1]!=10) { expfrom++; continue; }
    expfrom+=9;
    int iid=0;
    while (expfrom[iid]!='(') { namof[iid]=expfrom[iid]; iid++; }
    namof[iid]=0;
    strcat(temphdr,"export ");
    strcat(temphdr,namof);
    strcat(temphdr,";\r\n");
    }
  int aa;
  for (aa=0;aa<game.numcharacters-1;aa++) {
    if (game.chars[aa].scrname[0]==0) continue;
    strcat(temphdr,"#define ");
    strcat(temphdr,game.chars[aa].scrname);
    strcat(temphdr," ");
    char*ptro=&temphdr[strlen(temphdr)];
    sprintf(ptro,"%d\r\n",aa);
    }
  }

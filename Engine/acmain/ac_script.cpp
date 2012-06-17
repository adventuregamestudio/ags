
#include "acmain/ac_maindefines.h"
#include "acmain/ac_script.h"


int inside_script=0,in_graph_script=0;
int no_blocking_functions = 0; // set to 1 while in rep_Exec_always

NonBlockingScriptFunction repExecAlways(REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction getDialogOptionsDimensionsFunc("dialog_options_get_dimensions", 1);
NonBlockingScriptFunction renderDialogOptionsFunc("dialog_options_render", 1);
NonBlockingScriptFunction getDialogOptionUnderCursorFunc("dialog_options_get_active", 1);
NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc("dialog_options_mouse_click", 2);


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





#define scAdd_External_Symbol ccAddExternalSymbol
void setup_script_exports() {
  // the ^5 after the function name is the number of params
  // this is to allow an extra parameter to be added in a later
  // version without screwing up the stack in previous versions
  // (just export both the ^5 and the ^6 as seperate funcs)
  register_audio_script_functions();
  scAdd_External_Symbol("Character::AddInventory^2",(void *)Character_AddInventory);
  scAdd_External_Symbol("Character::AddWaypoint^2",(void *)Character_AddWaypoint);
  scAdd_External_Symbol("Character::Animate^5",(void *)Character_Animate);
  scAdd_External_Symbol("Character::ChangeRoom^3",(void *)Character_ChangeRoom);
  scAdd_External_Symbol("Character::ChangeRoomAutoPosition^2",(void *)Character_ChangeRoomAutoPosition);
  scAdd_External_Symbol("Character::ChangeView^1",(void *)Character_ChangeView);
  scAdd_External_Symbol("Character::FaceCharacter^2",(void *)Character_FaceCharacter);
  scAdd_External_Symbol("Character::FaceLocation^3",(void *)Character_FaceLocation);
  scAdd_External_Symbol("Character::FaceObject^2",(void *)Character_FaceObject);
  scAdd_External_Symbol("Character::FollowCharacter^3",(void *)Character_FollowCharacter);
  scAdd_External_Symbol("Character::GetProperty^1",(void *)Character_GetProperty);
  scAdd_External_Symbol("Character::GetPropertyText^2",(void *)Character_GetPropertyText);
  scAdd_External_Symbol("Character::GetTextProperty^1",(void *)Character_GetTextProperty);
  scAdd_External_Symbol("Character::HasInventory^1",(void *)Character_HasInventory);
  scAdd_External_Symbol("Character::IsCollidingWithChar^1",(void *)Character_IsCollidingWithChar);
  scAdd_External_Symbol("Character::IsCollidingWithObject^1",(void *)Character_IsCollidingWithObject);
  scAdd_External_Symbol("Character::LockView^1",(void *)Character_LockView);
  scAdd_External_Symbol("Character::LockViewAligned^3",(void *)Character_LockViewAligned);
  scAdd_External_Symbol("Character::LockViewFrame^3",(void *)Character_LockViewFrame);
  scAdd_External_Symbol("Character::LockViewOffset^3",(void *)Character_LockViewOffset);
  scAdd_External_Symbol("Character::LoseInventory^1",(void *)Character_LoseInventory);
  scAdd_External_Symbol("Character::Move^4",(void *)Character_Move);
  scAdd_External_Symbol("Character::PlaceOnWalkableArea^0",(void *)Character_PlaceOnWalkableArea);
  scAdd_External_Symbol("Character::RemoveTint^0",(void *)Character_RemoveTint);
  scAdd_External_Symbol("Character::RunInteraction^1",(void *)Character_RunInteraction);
  scAdd_External_Symbol("Character::Say^101",(void *)Character_Say);
  scAdd_External_Symbol("Character::SayAt^4",(void *)Character_SayAt);
  scAdd_External_Symbol("Character::SayBackground^1",(void *)Character_SayBackground);
  scAdd_External_Symbol("Character::SetAsPlayer^0",(void *)Character_SetAsPlayer);
  scAdd_External_Symbol("Character::SetIdleView^2",(void *)Character_SetIdleView);
  //scAdd_External_Symbol("Character::SetOption^2",(void *)Character_SetOption);
  scAdd_External_Symbol("Character::SetWalkSpeed^2",(void *)Character_SetSpeed);
  scAdd_External_Symbol("Character::StopMoving^0",(void *)Character_StopMoving);
  scAdd_External_Symbol("Character::Think^101",(void *)Character_Think);
  scAdd_External_Symbol("Character::Tint^5",(void *)Character_Tint);
  scAdd_External_Symbol("Character::UnlockView^0",(void *)Character_UnlockView);
  scAdd_External_Symbol("Character::Walk^4",(void *)Character_Walk);
  scAdd_External_Symbol("Character::WalkStraight^3",(void *)Character_WalkStraight);

  // static
  scAdd_External_Symbol("Character::GetAtScreenXY^2", (void *)GetCharacterAtLocation);

  scAdd_External_Symbol("Character::get_ActiveInventory",(void *)Character_GetActiveInventory);
  scAdd_External_Symbol("Character::set_ActiveInventory",(void *)Character_SetActiveInventory);
  scAdd_External_Symbol("Character::get_Animating", (void *)Character_GetAnimating);
  scAdd_External_Symbol("Character::get_AnimationSpeed", (void *)Character_GetAnimationSpeed);
  scAdd_External_Symbol("Character::set_AnimationSpeed", (void *)Character_SetAnimationSpeed);
  scAdd_External_Symbol("Character::get_Baseline",(void *)Character_GetBaseline);
  scAdd_External_Symbol("Character::set_Baseline",(void *)Character_SetBaseline);
  scAdd_External_Symbol("Character::get_BlinkInterval",(void *)Character_GetBlinkInterval);
  scAdd_External_Symbol("Character::set_BlinkInterval",(void *)Character_SetBlinkInterval);
  scAdd_External_Symbol("Character::get_BlinkView",(void *)Character_GetBlinkView);
  scAdd_External_Symbol("Character::set_BlinkView",(void *)Character_SetBlinkView);
  scAdd_External_Symbol("Character::get_BlinkWhileThinking",(void *)Character_GetBlinkWhileThinking);
  scAdd_External_Symbol("Character::set_BlinkWhileThinking",(void *)Character_SetBlinkWhileThinking);
  scAdd_External_Symbol("Character::get_BlockingHeight",(void *)Character_GetBlockingHeight);
  scAdd_External_Symbol("Character::set_BlockingHeight",(void *)Character_SetBlockingHeight);
  scAdd_External_Symbol("Character::get_BlockingWidth",(void *)Character_GetBlockingWidth);
  scAdd_External_Symbol("Character::set_BlockingWidth",(void *)Character_SetBlockingWidth);
  scAdd_External_Symbol("Character::get_Clickable",(void *)Character_GetClickable);
  scAdd_External_Symbol("Character::set_Clickable",(void *)Character_SetClickable);
  scAdd_External_Symbol("Character::get_DiagonalLoops", (void *)Character_GetDiagonalWalking);
  scAdd_External_Symbol("Character::set_DiagonalLoops", (void *)Character_SetDiagonalWalking);
  scAdd_External_Symbol("Character::get_Frame", (void *)Character_GetFrame);
  scAdd_External_Symbol("Character::set_Frame", (void *)Character_SetFrame);
  scAdd_External_Symbol("Character::get_HasExplicitTint", (void *)Character_GetHasExplicitTint);
  scAdd_External_Symbol("Character::get_ID", (void *)Character_GetID);
  scAdd_External_Symbol("Character::get_IdleView", (void *)Character_GetIdleView);
  scAdd_External_Symbol("Character::geti_InventoryQuantity", (void *)Character_GetIInventoryQuantity);
  scAdd_External_Symbol("Character::seti_InventoryQuantity", (void *)Character_SetIInventoryQuantity);
  scAdd_External_Symbol("Character::get_IgnoreLighting",(void *)Character_GetIgnoreLighting);
  scAdd_External_Symbol("Character::set_IgnoreLighting",(void *)Character_SetIgnoreLighting);
  scAdd_External_Symbol("Character::get_IgnoreScaling", (void *)Character_GetIgnoreScaling);
  scAdd_External_Symbol("Character::set_IgnoreScaling", (void *)Character_SetIgnoreScaling);
  scAdd_External_Symbol("Character::get_IgnoreWalkbehinds",(void *)Character_GetIgnoreWalkbehinds);
  scAdd_External_Symbol("Character::set_IgnoreWalkbehinds",(void *)Character_SetIgnoreWalkbehinds);
  scAdd_External_Symbol("Character::get_Loop", (void *)Character_GetLoop);
  scAdd_External_Symbol("Character::set_Loop", (void *)Character_SetLoop);
  scAdd_External_Symbol("Character::get_ManualScaling", (void *)Character_GetIgnoreScaling);
  scAdd_External_Symbol("Character::set_ManualScaling", (void *)Character_SetManualScaling);
  scAdd_External_Symbol("Character::get_MovementLinkedToAnimation",(void *)Character_GetMovementLinkedToAnimation);
  scAdd_External_Symbol("Character::set_MovementLinkedToAnimation",(void *)Character_SetMovementLinkedToAnimation);
  scAdd_External_Symbol("Character::get_Moving", (void *)Character_GetMoving);
  scAdd_External_Symbol("Character::get_Name", (void *)Character_GetName);
  scAdd_External_Symbol("Character::set_Name", (void *)Character_SetName);
  scAdd_External_Symbol("Character::get_NormalView",(void *)Character_GetNormalView);
  scAdd_External_Symbol("Character::get_PreviousRoom",(void *)Character_GetPreviousRoom);
  scAdd_External_Symbol("Character::get_Room",(void *)Character_GetRoom);
  scAdd_External_Symbol("Character::get_ScaleMoveSpeed", (void *)Character_GetScaleMoveSpeed);
  scAdd_External_Symbol("Character::set_ScaleMoveSpeed", (void *)Character_SetScaleMoveSpeed);
  scAdd_External_Symbol("Character::get_ScaleVolume", (void *)Character_GetScaleVolume);
  scAdd_External_Symbol("Character::set_ScaleVolume", (void *)Character_SetScaleVolume);
  scAdd_External_Symbol("Character::get_Scaling", (void *)Character_GetScaling);
  scAdd_External_Symbol("Character::set_Scaling", (void *)Character_SetScaling);
  scAdd_External_Symbol("Character::get_Solid", (void *)Character_GetSolid);
  scAdd_External_Symbol("Character::set_Solid", (void *)Character_SetSolid);
  scAdd_External_Symbol("Character::get_Speaking", (void *)Character_GetSpeaking);
  scAdd_External_Symbol("Character::get_SpeakingFrame", (void *)Character_GetSpeakingFrame);
  scAdd_External_Symbol("Character::get_SpeechAnimationDelay",(void *)GetCharacterSpeechAnimationDelay);
  scAdd_External_Symbol("Character::set_SpeechAnimationDelay",(void *)Character_SetSpeechAnimationDelay);
  scAdd_External_Symbol("Character::get_SpeechColor",(void *)Character_GetSpeechColor);
  scAdd_External_Symbol("Character::set_SpeechColor",(void *)Character_SetSpeechColor);
  scAdd_External_Symbol("Character::get_SpeechView",(void *)Character_GetSpeechView);
  scAdd_External_Symbol("Character::set_SpeechView",(void *)Character_SetSpeechView);
  scAdd_External_Symbol("Character::get_ThinkView",(void *)Character_GetThinkView);
  scAdd_External_Symbol("Character::set_ThinkView",(void *)Character_SetThinkView);
  scAdd_External_Symbol("Character::get_Transparency",(void *)Character_GetTransparency);
  scAdd_External_Symbol("Character::set_Transparency",(void *)Character_SetTransparency);
  scAdd_External_Symbol("Character::get_TurnBeforeWalking", (void *)Character_GetTurnBeforeWalking);
  scAdd_External_Symbol("Character::set_TurnBeforeWalking", (void *)Character_SetTurnBeforeWalking);
  scAdd_External_Symbol("Character::get_View", (void *)Character_GetView);
  scAdd_External_Symbol("Character::get_WalkSpeedX", (void *)Character_GetWalkSpeedX);
  scAdd_External_Symbol("Character::get_WalkSpeedY", (void *)Character_GetWalkSpeedY);
  scAdd_External_Symbol("Character::get_X", (void *)Character_GetX);
  scAdd_External_Symbol("Character::set_X", (void *)Character_SetX);
  scAdd_External_Symbol("Character::get_x", (void *)Character_GetX);
  scAdd_External_Symbol("Character::set_x", (void *)Character_SetX);
  scAdd_External_Symbol("Character::get_Y", (void *)Character_GetY);
  scAdd_External_Symbol("Character::set_Y", (void *)Character_SetY);
  scAdd_External_Symbol("Character::get_y", (void *)Character_GetY);
  scAdd_External_Symbol("Character::set_y", (void *)Character_SetY);
  scAdd_External_Symbol("Character::get_Z", (void *)Character_GetZ);
  scAdd_External_Symbol("Character::set_Z", (void *)Character_SetZ);
  scAdd_External_Symbol("Character::get_z", (void *)Character_GetZ);
  scAdd_External_Symbol("Character::set_z", (void *)Character_SetZ);

  scAdd_External_Symbol("Object::Animate^5", (void *)Object_Animate);
  scAdd_External_Symbol("Object::IsCollidingWithObject^1", (void *)Object_IsCollidingWithObject);
  scAdd_External_Symbol("Object::GetName^1", (void *)Object_GetName);
  scAdd_External_Symbol("Object::GetProperty^1", (void *)Object_GetProperty);
  scAdd_External_Symbol("Object::GetPropertyText^2", (void *)Object_GetPropertyText);
  scAdd_External_Symbol("Object::GetTextProperty^1",(void *)Object_GetTextProperty);
  scAdd_External_Symbol("Object::MergeIntoBackground^0", (void *)Object_MergeIntoBackground);
  scAdd_External_Symbol("Object::Move^5", (void *)Object_Move);
  scAdd_External_Symbol("Object::RemoveTint^0", (void *)Object_RemoveTint);
  scAdd_External_Symbol("Object::RunInteraction^1", (void *)Object_RunInteraction);
  scAdd_External_Symbol("Object::SetPosition^2", (void *)Object_SetPosition);
  scAdd_External_Symbol("Object::SetView^3", (void *)Object_SetView);
  scAdd_External_Symbol("Object::StopAnimating^0", (void *)Object_StopAnimating);
  scAdd_External_Symbol("Object::StopMoving^0", (void *)Object_StopMoving);
  scAdd_External_Symbol("Object::Tint^5", (void *)Object_Tint);

  // static
  scAdd_External_Symbol("Object::GetAtScreenXY^2", (void *)GetObjectAtLocation);

  scAdd_External_Symbol("Object::get_Animating", (void *)Object_GetAnimating);
  scAdd_External_Symbol("Object::get_Baseline", (void *)Object_GetBaseline);
  scAdd_External_Symbol("Object::set_Baseline", (void *)Object_SetBaseline);
  scAdd_External_Symbol("Object::get_BlockingHeight",(void *)Object_GetBlockingHeight);
  scAdd_External_Symbol("Object::set_BlockingHeight",(void *)Object_SetBlockingHeight);
  scAdd_External_Symbol("Object::get_BlockingWidth",(void *)Object_GetBlockingWidth);
  scAdd_External_Symbol("Object::set_BlockingWidth",(void *)Object_SetBlockingWidth);
  scAdd_External_Symbol("Object::get_Clickable", (void *)Object_GetClickable);
  scAdd_External_Symbol("Object::set_Clickable", (void *)Object_SetClickable);
  scAdd_External_Symbol("Object::get_Frame", (void *)Object_GetFrame);
  scAdd_External_Symbol("Object::get_Graphic", (void *)Object_GetGraphic);
  scAdd_External_Symbol("Object::set_Graphic", (void *)Object_SetGraphic);
  scAdd_External_Symbol("Object::get_ID", (void *)Object_GetID);
  scAdd_External_Symbol("Object::get_IgnoreScaling", (void *)Object_GetIgnoreScaling);
  scAdd_External_Symbol("Object::set_IgnoreScaling", (void *)Object_SetIgnoreScaling);
  scAdd_External_Symbol("Object::get_IgnoreWalkbehinds", (void *)Object_GetIgnoreWalkbehinds);
  scAdd_External_Symbol("Object::set_IgnoreWalkbehinds", (void *)Object_SetIgnoreWalkbehinds);
  scAdd_External_Symbol("Object::get_Loop", (void *)Object_GetLoop);
  scAdd_External_Symbol("Object::get_Moving", (void *)Object_GetMoving);
  scAdd_External_Symbol("Object::get_Name", (void *)Object_GetName_New);
  scAdd_External_Symbol("Object::get_Solid", (void *)Object_GetSolid);
  scAdd_External_Symbol("Object::set_Solid", (void *)Object_SetSolid);
  scAdd_External_Symbol("Object::get_Transparency", (void *)Object_GetTransparency);
  scAdd_External_Symbol("Object::set_Transparency", (void *)Object_SetTransparency);
  scAdd_External_Symbol("Object::get_View", (void *)Object_GetView);
  scAdd_External_Symbol("Object::get_Visible", (void *)Object_GetVisible);
  scAdd_External_Symbol("Object::set_Visible", (void *)Object_SetVisible);
  scAdd_External_Symbol("Object::get_X", (void *)Object_GetX);
  scAdd_External_Symbol("Object::set_X", (void *)Object_SetX);
  scAdd_External_Symbol("Object::get_Y", (void *)Object_GetY);
  scAdd_External_Symbol("Object::set_Y", (void *)Object_SetY);

  scAdd_External_Symbol("Dialog::get_ID", (void *)Dialog_GetID);
  scAdd_External_Symbol("Dialog::get_OptionCount", (void *)Dialog_GetOptionCount);
  scAdd_External_Symbol("Dialog::get_ShowTextParser", (void *)Dialog_GetShowTextParser);
  scAdd_External_Symbol("Dialog::DisplayOptions^1", (void *)Dialog_DisplayOptions);
  scAdd_External_Symbol("Dialog::GetOptionState^1", (void *)Dialog_GetOptionState);
  scAdd_External_Symbol("Dialog::GetOptionText^1", (void *)Dialog_GetOptionText);
  scAdd_External_Symbol("Dialog::HasOptionBeenChosen^1", (void *)Dialog_HasOptionBeenChosen);
  scAdd_External_Symbol("Dialog::SetOptionState^2", (void *)Dialog_SetOptionState);
  scAdd_External_Symbol("Dialog::Start^0", (void *)Dialog_Start);

  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ActiveOptionID", (void *)DialogOptionsRendering_GetActiveOptionID);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ActiveOptionID", (void *)DialogOptionsRendering_SetActiveOptionID);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_DialogToRender", (void *)DialogOptionsRendering_GetDialogToRender);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Height", (void *)DialogOptionsRendering_GetHeight);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Height", (void *)DialogOptionsRendering_SetHeight);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxX", (void *)DialogOptionsRendering_GetParserTextboxX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxX", (void *)DialogOptionsRendering_SetParserTextboxX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxY", (void *)DialogOptionsRendering_GetParserTextboxY);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxY", (void *)DialogOptionsRendering_SetParserTextboxY);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxWidth", (void *)DialogOptionsRendering_GetParserTextboxWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxWidth", (void *)DialogOptionsRendering_SetParserTextboxWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Surface", (void *)DialogOptionsRendering_GetSurface);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Width", (void *)DialogOptionsRendering_GetWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Width", (void *)DialogOptionsRendering_SetWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_X", (void *)DialogOptionsRendering_GetX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_X", (void *)DialogOptionsRendering_SetX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Y", (void *)DialogOptionsRendering_GetY);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Y", (void *)DialogOptionsRendering_SetY);

  scAdd_External_Symbol("File::Delete^1",(void *)File_Delete);
  scAdd_External_Symbol("File::Exists^1",(void *)File_Exists);
  scAdd_External_Symbol("File::Open^2",(void *)sc_OpenFile);
  scAdd_External_Symbol("File::Close^0", (void *)File_Close);
  scAdd_External_Symbol("File::ReadInt^0", (void *)File_ReadInt);
  scAdd_External_Symbol("File::ReadRawChar^0", (void *)File_ReadRawChar);
  scAdd_External_Symbol("File::ReadRawInt^0", (void *)File_ReadRawInt);
  scAdd_External_Symbol("File::ReadRawLine^1", (void *)File_ReadRawLine);
  scAdd_External_Symbol("File::ReadRawLineBack^0", (void *)File_ReadRawLineBack);
  scAdd_External_Symbol("File::ReadString^1", (void *)File_ReadString);
  scAdd_External_Symbol("File::ReadStringBack^0", (void *)File_ReadStringBack);
  scAdd_External_Symbol("File::WriteInt^1", (void *)File_WriteInt);
  scAdd_External_Symbol("File::WriteRawChar^1", (void *)File_WriteRawChar);
  scAdd_External_Symbol("File::WriteRawLine^1", (void *)File_WriteRawLine);
  scAdd_External_Symbol("File::WriteString^1", (void *)File_WriteString);
  scAdd_External_Symbol("File::get_EOF", (void *)File_GetEOF);
  scAdd_External_Symbol("File::get_Error", (void *)File_GetError);

  scAdd_External_Symbol("Overlay::CreateGraphical^4", (void *)Overlay_CreateGraphical);
  scAdd_External_Symbol("Overlay::CreateTextual^106", (void *)Overlay_CreateTextual);
  scAdd_External_Symbol("Overlay::SetText^104", (void *)Overlay_SetText);
  scAdd_External_Symbol("Overlay::Remove^0", (void *)Overlay_Remove);
  scAdd_External_Symbol("Overlay::get_Valid", (void *)Overlay_GetValid);
  scAdd_External_Symbol("Overlay::get_X", (void *)Overlay_GetX);
  scAdd_External_Symbol("Overlay::set_X", (void *)Overlay_SetX);
  scAdd_External_Symbol("Overlay::get_Y", (void *)Overlay_GetY);
  scAdd_External_Symbol("Overlay::set_Y", (void *)Overlay_SetY);

  scAdd_External_Symbol("InventoryItem::GetAtScreenXY^2", (void *)GetInvAtLocation);
  scAdd_External_Symbol("InventoryItem::IsInteractionAvailable^1", (void *)InventoryItem_CheckInteractionAvailable);
  scAdd_External_Symbol("InventoryItem::GetName^1", (void *)InventoryItem_GetName);
  scAdd_External_Symbol("InventoryItem::GetProperty^1", (void *)InventoryItem_GetProperty);
  scAdd_External_Symbol("InventoryItem::GetPropertyText^2", (void *)InventoryItem_GetPropertyText);
  scAdd_External_Symbol("InventoryItem::GetTextProperty^1",(void *)InventoryItem_GetTextProperty);
  scAdd_External_Symbol("InventoryItem::RunInteraction^1", (void *)InventoryItem_RunInteraction);
  scAdd_External_Symbol("InventoryItem::SetName^1", (void *)InventoryItem_SetName);
  scAdd_External_Symbol("InventoryItem::get_CursorGraphic", (void *)InventoryItem_GetCursorGraphic);
  scAdd_External_Symbol("InventoryItem::set_CursorGraphic", (void *)InventoryItem_SetCursorGraphic);
  scAdd_External_Symbol("InventoryItem::get_Graphic", (void *)InventoryItem_GetGraphic);
  scAdd_External_Symbol("InventoryItem::set_Graphic", (void *)InventoryItem_SetGraphic);
  scAdd_External_Symbol("InventoryItem::get_ID", (void *)InventoryItem_GetID);
  scAdd_External_Symbol("InventoryItem::get_Name", (void *)InventoryItem_GetName_New);
  scAdd_External_Symbol("InventoryItem::set_Name", (void *)InventoryItem_SetName);

  scAdd_External_Symbol("GUI::Centre^0", (void *)GUI_Centre);
  scAdd_External_Symbol("GUI::GetAtScreenXY^2", (void *)GetGUIAtLocation);
  scAdd_External_Symbol("GUI::SetPosition^2", (void *)GUI_SetPosition);
  scAdd_External_Symbol("GUI::SetSize^2", (void *)GUI_SetSize);
  scAdd_External_Symbol("GUI::get_BackgroundGraphic", (void *)GUI_GetBackgroundGraphic);
  scAdd_External_Symbol("GUI::set_BackgroundGraphic", (void *)GUI_SetBackgroundGraphic);
  scAdd_External_Symbol("GUI::get_Clickable", (void *)GUI_GetClickable);
  scAdd_External_Symbol("GUI::set_Clickable", (void *)GUI_SetClickable);
  scAdd_External_Symbol("GUI::get_ControlCount", (void *)GUI_GetControlCount);
  scAdd_External_Symbol("GUI::geti_Controls", (void *)GUI_GetiControls);
  scAdd_External_Symbol("GUI::get_Height", (void *)GUI_GetHeight);
  scAdd_External_Symbol("GUI::set_Height", (void *)GUI_SetHeight);
  scAdd_External_Symbol("GUI::get_ID", (void *)GUI_GetID);
  scAdd_External_Symbol("GUI::get_Transparency", (void *)GUI_GetTransparency);
  scAdd_External_Symbol("GUI::set_Transparency", (void *)GUI_SetTransparency);
  scAdd_External_Symbol("GUI::get_Visible", (void *)GUI_GetVisible);
  scAdd_External_Symbol("GUI::set_Visible", (void *)GUI_SetVisible);
  scAdd_External_Symbol("GUI::get_Width", (void *)GUI_GetWidth);
  scAdd_External_Symbol("GUI::set_Width", (void *)GUI_SetWidth);
  scAdd_External_Symbol("GUI::get_X", (void *)GUI_GetX);
  scAdd_External_Symbol("GUI::set_X", (void *)GUI_SetX);
  scAdd_External_Symbol("GUI::get_Y", (void *)GUI_GetY);
  scAdd_External_Symbol("GUI::set_Y", (void *)GUI_SetY);
  scAdd_External_Symbol("GUI::get_ZOrder", (void *)GUI_GetZOrder);
  scAdd_External_Symbol("GUI::set_ZOrder", (void *)GUI_SetZOrder);

  scAdd_External_Symbol("GUIControl::BringToFront^0", (void *)GUIControl_BringToFront);
  scAdd_External_Symbol("GUIControl::GetAtScreenXY^2", (void *)GetGUIControlAtLocation);
  scAdd_External_Symbol("GUIControl::SendToBack^0", (void *)GUIControl_SendToBack);
  scAdd_External_Symbol("GUIControl::SetPosition^2", (void *)GUIControl_SetPosition);
  scAdd_External_Symbol("GUIControl::SetSize^2", (void *)GUIControl_SetSize);
  scAdd_External_Symbol("GUIControl::get_AsButton", (void *)GUIControl_GetAsButton);
  scAdd_External_Symbol("GUIControl::get_AsInvWindow", (void *)GUIControl_GetAsInvWindow);
  scAdd_External_Symbol("GUIControl::get_AsLabel", (void *)GUIControl_GetAsLabel);
  scAdd_External_Symbol("GUIControl::get_AsListBox", (void *)GUIControl_GetAsListBox);
  scAdd_External_Symbol("GUIControl::get_AsSlider", (void *)GUIControl_GetAsSlider);
  scAdd_External_Symbol("GUIControl::get_AsTextBox", (void *)GUIControl_GetAsTextBox);
  scAdd_External_Symbol("GUIControl::get_Clickable", (void *)GUIControl_GetClickable);
  scAdd_External_Symbol("GUIControl::set_Clickable", (void *)GUIControl_SetClickable);
  scAdd_External_Symbol("GUIControl::get_Enabled", (void *)GUIControl_GetEnabled);
  scAdd_External_Symbol("GUIControl::set_Enabled", (void *)GUIControl_SetEnabled);
  scAdd_External_Symbol("GUIControl::get_Height", (void *)GUIControl_GetHeight);
  scAdd_External_Symbol("GUIControl::set_Height", (void *)GUIControl_SetHeight);
  scAdd_External_Symbol("GUIControl::get_ID", (void *)GUIControl_GetID);
  scAdd_External_Symbol("GUIControl::get_OwningGUI", (void *)GUIControl_GetOwningGUI);
  scAdd_External_Symbol("GUIControl::get_Visible", (void *)GUIControl_GetVisible);
  scAdd_External_Symbol("GUIControl::set_Visible", (void *)GUIControl_SetVisible);
  scAdd_External_Symbol("GUIControl::get_Width", (void *)GUIControl_GetWidth);
  scAdd_External_Symbol("GUIControl::set_Width", (void *)GUIControl_SetWidth);
  scAdd_External_Symbol("GUIControl::get_X", (void *)GUIControl_GetX);
  scAdd_External_Symbol("GUIControl::set_X", (void *)GUIControl_SetX);
  scAdd_External_Symbol("GUIControl::get_Y", (void *)GUIControl_GetY);
  scAdd_External_Symbol("GUIControl::set_Y", (void *)GUIControl_SetY);

  scAdd_External_Symbol("Label::GetText^1", (void *)Label_GetText);
  scAdd_External_Symbol("Label::SetText^1", (void *)Label_SetText);
  scAdd_External_Symbol("Label::get_Font", (void *)Label_GetFont);
  scAdd_External_Symbol("Label::set_Font", (void *)Label_SetFont);
  scAdd_External_Symbol("Label::get_Text", (void *)Label_GetText_New);
  scAdd_External_Symbol("Label::set_Text", (void *)Label_SetText);
  scAdd_External_Symbol("Label::get_TextColor", (void *)Label_GetColor);
  scAdd_External_Symbol("Label::set_TextColor", (void *)Label_SetColor);

  scAdd_External_Symbol("Button::Animate^4", (void *)Button_Animate);
  scAdd_External_Symbol("Button::GetText^1", (void *)Button_GetText);
  scAdd_External_Symbol("Button::SetText^1", (void *)Button_SetText);
  scAdd_External_Symbol("Button::get_ClipImage", (void *)Button_GetClipImage);
  scAdd_External_Symbol("Button::set_ClipImage", (void *)Button_SetClipImage);
  scAdd_External_Symbol("Button::get_Font", (void *)Button_GetFont);
  scAdd_External_Symbol("Button::set_Font", (void *)Button_SetFont);
  scAdd_External_Symbol("Button::get_Graphic", (void *)Button_GetGraphic);
  scAdd_External_Symbol("Button::get_MouseOverGraphic", (void *)Button_GetMouseOverGraphic);
  scAdd_External_Symbol("Button::set_MouseOverGraphic", (void *)Button_SetMouseOverGraphic);
  scAdd_External_Symbol("Button::get_NormalGraphic", (void *)Button_GetNormalGraphic);
  scAdd_External_Symbol("Button::set_NormalGraphic", (void *)Button_SetNormalGraphic);
  scAdd_External_Symbol("Button::get_PushedGraphic", (void *)Button_GetPushedGraphic);
  scAdd_External_Symbol("Button::set_PushedGraphic", (void *)Button_SetPushedGraphic);
  scAdd_External_Symbol("Button::get_Text", (void *)Button_GetText_New);
  scAdd_External_Symbol("Button::set_Text", (void *)Button_SetText);
  scAdd_External_Symbol("Button::get_TextColor", (void *)Button_GetTextColor);
  scAdd_External_Symbol("Button::set_TextColor", (void *)Button_SetTextColor);

  scAdd_External_Symbol("Slider::get_BackgroundGraphic", (void *)Slider_GetBackgroundGraphic);
  scAdd_External_Symbol("Slider::set_BackgroundGraphic", (void *)Slider_SetBackgroundGraphic);
  scAdd_External_Symbol("Slider::get_HandleGraphic", (void *)Slider_GetHandleGraphic);
  scAdd_External_Symbol("Slider::set_HandleGraphic", (void *)Slider_SetHandleGraphic);
  scAdd_External_Symbol("Slider::get_HandleOffset", (void *)Slider_GetHandleOffset);
  scAdd_External_Symbol("Slider::set_HandleOffset", (void *)Slider_SetHandleOffset);
  scAdd_External_Symbol("Slider::get_Max", (void *)Slider_GetMax);
  scAdd_External_Symbol("Slider::set_Max", (void *)Slider_SetMax);
  scAdd_External_Symbol("Slider::get_Min", (void *)Slider_GetMin);
  scAdd_External_Symbol("Slider::set_Min", (void *)Slider_SetMin);
  scAdd_External_Symbol("Slider::get_Value", (void *)Slider_GetValue);
  scAdd_External_Symbol("Slider::set_Value", (void *)Slider_SetValue);

  scAdd_External_Symbol("TextBox::GetText^1", (void *)TextBox_GetText);
  scAdd_External_Symbol("TextBox::SetText^1", (void *)TextBox_SetText);
  scAdd_External_Symbol("TextBox::get_Font", (void *)TextBox_GetFont);
  scAdd_External_Symbol("TextBox::set_Font", (void *)TextBox_SetFont);
  scAdd_External_Symbol("TextBox::get_Text", (void *)TextBox_GetText_New);
  scAdd_External_Symbol("TextBox::set_Text", (void *)TextBox_SetText);
  scAdd_External_Symbol("TextBox::get_TextColor", (void *)TextBox_GetTextColor);
  scAdd_External_Symbol("TextBox::set_TextColor", (void *)TextBox_SetTextColor);

  scAdd_External_Symbol("InvWindow::ScrollDown^0", (void *)InvWindow_ScrollDown);
  scAdd_External_Symbol("InvWindow::ScrollUp^0", (void *)InvWindow_ScrollUp);
  scAdd_External_Symbol("InvWindow::get_CharacterToUse", (void *)InvWindow_GetCharacterToUse);
  scAdd_External_Symbol("InvWindow::set_CharacterToUse", (void *)InvWindow_SetCharacterToUse);
  scAdd_External_Symbol("InvWindow::geti_ItemAtIndex", (void *)InvWindow_GetItemAtIndex);
  scAdd_External_Symbol("InvWindow::get_ItemCount", (void *)InvWindow_GetItemCount);
  scAdd_External_Symbol("InvWindow::get_ItemHeight", (void *)InvWindow_GetItemHeight);
  scAdd_External_Symbol("InvWindow::set_ItemHeight", (void *)InvWindow_SetItemHeight);
  scAdd_External_Symbol("InvWindow::get_ItemWidth", (void *)InvWindow_GetItemWidth);
  scAdd_External_Symbol("InvWindow::set_ItemWidth", (void *)InvWindow_SetItemWidth);
  scAdd_External_Symbol("InvWindow::get_ItemsPerRow", (void *)InvWindow_GetItemsPerRow);
  scAdd_External_Symbol("InvWindow::get_RowCount", (void *)InvWindow_GetRowCount);
  scAdd_External_Symbol("InvWindow::get_TopItem", (void *)InvWindow_GetTopItem);
  scAdd_External_Symbol("InvWindow::set_TopItem", (void *)InvWindow_SetTopItem);

  scAdd_External_Symbol("ListBox::AddItem^1", (void *)ListBox_AddItem);
  scAdd_External_Symbol("ListBox::Clear^0", (void *)ListBox_Clear);
  scAdd_External_Symbol("ListBox::FillDirList^1", (void *)ListBox_FillDirList);
  scAdd_External_Symbol("ListBox::FillSaveGameList^0", (void *)ListBox_FillSaveGameList);
  scAdd_External_Symbol("ListBox::GetItemAtLocation^2", (void *)ListBox_GetItemAtLocation);
  scAdd_External_Symbol("ListBox::GetItemText^2", (void *)ListBox_GetItemText);
  scAdd_External_Symbol("ListBox::InsertItemAt^2", (void *)ListBox_InsertItemAt);
  scAdd_External_Symbol("ListBox::RemoveItem^1", (void *)ListBox_RemoveItem);
  scAdd_External_Symbol("ListBox::ScrollDown^0", (void *)ListBox_ScrollDown);
  scAdd_External_Symbol("ListBox::ScrollUp^0", (void *)ListBox_ScrollUp);
  scAdd_External_Symbol("ListBox::SetItemText^2", (void *)ListBox_SetItemText);
  scAdd_External_Symbol("ListBox::get_Font", (void *)ListBox_GetFont);
  scAdd_External_Symbol("ListBox::set_Font", (void *)ListBox_SetFont);
  scAdd_External_Symbol("ListBox::get_HideBorder", (void *)ListBox_GetHideBorder);
  scAdd_External_Symbol("ListBox::set_HideBorder", (void *)ListBox_SetHideBorder);
  scAdd_External_Symbol("ListBox::get_HideScrollArrows", (void *)ListBox_GetHideScrollArrows);
  scAdd_External_Symbol("ListBox::set_HideScrollArrows", (void *)ListBox_SetHideScrollArrows);
  scAdd_External_Symbol("ListBox::get_ItemCount", (void *)ListBox_GetItemCount);
  scAdd_External_Symbol("ListBox::geti_Items", (void *)ListBox_GetItems);
  scAdd_External_Symbol("ListBox::seti_Items", (void *)ListBox_SetItemText);
  scAdd_External_Symbol("ListBox::get_RowCount", (void *)ListBox_GetRowCount);
  scAdd_External_Symbol("ListBox::geti_SaveGameSlots", (void *)ListBox_GetSaveGameSlots);
  scAdd_External_Symbol("ListBox::get_SelectedIndex", (void *)ListBox_GetSelectedIndex);
  scAdd_External_Symbol("ListBox::set_SelectedIndex", (void *)ListBox_SetSelectedIndex);
  scAdd_External_Symbol("ListBox::get_TopItem", (void *)ListBox_GetTopItem);
  scAdd_External_Symbol("ListBox::set_TopItem", (void *)ListBox_SetTopItem);

  scAdd_External_Symbol("Mouse::ChangeModeGraphic^2",(void *)ChangeCursorGraphic);
  scAdd_External_Symbol("Mouse::ChangeModeHotspot^3",(void *)ChangeCursorHotspot);
  scAdd_External_Symbol("Mouse::ChangeModeView^2",(void *)Mouse_ChangeModeView);
  scAdd_External_Symbol("Mouse::DisableMode^1",(void *)disable_cursor_mode);
  scAdd_External_Symbol("Mouse::EnableMode^1",(void *)enable_cursor_mode);
  scAdd_External_Symbol("Mouse::GetModeGraphic^1",(void *)Mouse_GetModeGraphic);
  scAdd_External_Symbol("Mouse::IsButtonDown^1",(void *)IsButtonDown);
  scAdd_External_Symbol("Mouse::SaveCursorUntilItLeaves^0",(void *)SaveCursorForLocationChange);
  scAdd_External_Symbol("Mouse::SelectNextMode^0", (void *)SetNextCursor);
  scAdd_External_Symbol("Mouse::SetBounds^4",(void *)SetMouseBounds);
  scAdd_External_Symbol("Mouse::SetPosition^2",(void *)SetMousePosition);
  scAdd_External_Symbol("Mouse::Update^0",(void *)RefreshMouse);
  scAdd_External_Symbol("Mouse::UseDefaultGraphic^0",(void *)set_default_cursor);
  scAdd_External_Symbol("Mouse::UseModeGraphic^1",(void *)set_mouse_cursor);
  scAdd_External_Symbol("Mouse::get_Mode",(void *)GetCursorMode);
  scAdd_External_Symbol("Mouse::set_Mode",(void *)set_cursor_mode);
  scAdd_External_Symbol("Mouse::get_Visible", (void *)Mouse_GetVisible);
  scAdd_External_Symbol("Mouse::set_Visible", (void *)Mouse_SetVisible);

  scAdd_External_Symbol("Maths::ArcCos^1", (void*)Math_ArcCos);
  scAdd_External_Symbol("Maths::ArcSin^1", (void*)Math_ArcSin);
  scAdd_External_Symbol("Maths::ArcTan^1", (void*)Math_ArcTan);
  scAdd_External_Symbol("Maths::ArcTan2^2", (void*)Math_ArcTan2);
  scAdd_External_Symbol("Maths::Cos^1", (void*)Math_Cos);
  scAdd_External_Symbol("Maths::Cosh^1", (void*)Math_Cosh);
  scAdd_External_Symbol("Maths::DegreesToRadians^1", (void*)Math_DegreesToRadians);
  scAdd_External_Symbol("Maths::Exp^1", (void*)Math_Exp);
  scAdd_External_Symbol("Maths::Log^1", (void*)Math_Log);
  scAdd_External_Symbol("Maths::Log10^1", (void*)Math_Log10);
  scAdd_External_Symbol("Maths::RadiansToDegrees^1", (void*)Math_RadiansToDegrees);
  scAdd_External_Symbol("Maths::RaiseToPower^2", (void*)Math_RaiseToPower);
  scAdd_External_Symbol("Maths::Sin^1", (void*)Math_Sin);
  scAdd_External_Symbol("Maths::Sinh^1", (void*)Math_Sinh);
  scAdd_External_Symbol("Maths::Sqrt^1", (void*)Math_Sqrt);
  scAdd_External_Symbol("Maths::Tan^1", (void*)Math_Tan);
  scAdd_External_Symbol("Maths::Tanh^1", (void*)Math_Tanh);
  scAdd_External_Symbol("Maths::get_Pi", (void*)Math_GetPi);

  scAdd_External_Symbol("Hotspot::GetAtScreenXY^2",(void *)GetHotspotAtLocation);
  scAdd_External_Symbol("Hotspot::GetName^1", (void*)Hotspot_GetName);
  scAdd_External_Symbol("Hotspot::GetProperty^1", (void*)Hotspot_GetProperty);
  scAdd_External_Symbol("Hotspot::GetPropertyText^2", (void*)Hotspot_GetPropertyText);
  scAdd_External_Symbol("Hotspot::GetTextProperty^1",(void *)Hotspot_GetTextProperty);
  scAdd_External_Symbol("Hotspot::RunInteraction^1", (void*)Hotspot_RunInteraction);
  scAdd_External_Symbol("Hotspot::get_Enabled", (void*)Hotspot_GetEnabled);
  scAdd_External_Symbol("Hotspot::set_Enabled", (void*)Hotspot_SetEnabled);
  scAdd_External_Symbol("Hotspot::get_ID", (void*)Hotspot_GetID);
  scAdd_External_Symbol("Hotspot::get_Name", (void*)Hotspot_GetName_New);
  scAdd_External_Symbol("Hotspot::get_WalkToX", (void*)Hotspot_GetWalkToX);
  scAdd_External_Symbol("Hotspot::get_WalkToY", (void*)Hotspot_GetWalkToY);

  scAdd_External_Symbol("Region::GetAtRoomXY^2",(void *)GetRegionAtLocation);
  scAdd_External_Symbol("Region::Tint^4", (void*)Region_Tint);
  scAdd_External_Symbol("Region::RunInteraction^1", (void*)Region_RunInteraction);
  scAdd_External_Symbol("Region::get_Enabled", (void*)Region_GetEnabled);
  scAdd_External_Symbol("Region::set_Enabled", (void*)Region_SetEnabled);
  scAdd_External_Symbol("Region::get_ID", (void*)Region_GetID);
  scAdd_External_Symbol("Region::get_LightLevel", (void*)Region_GetLightLevel);
  scAdd_External_Symbol("Region::set_LightLevel", (void*)Region_SetLightLevel);
  scAdd_External_Symbol("Region::get_TintEnabled", (void*)Region_GetTintEnabled);
  scAdd_External_Symbol("Region::get_TintBlue", (void*)Region_GetTintBlue);
  scAdd_External_Symbol("Region::get_TintGreen", (void*)Region_GetTintGreen);
  scAdd_External_Symbol("Region::get_TintRed", (void*)Region_GetTintRed);
  scAdd_External_Symbol("Region::get_TintSaturation", (void*)Region_GetTintSaturation);

  scAdd_External_Symbol("DateTime::get_Now", (void*)DateTime_Now);
  scAdd_External_Symbol("DateTime::get_DayOfMonth", (void*)DateTime_GetDayOfMonth);
  scAdd_External_Symbol("DateTime::get_Hour", (void*)DateTime_GetHour);
  scAdd_External_Symbol("DateTime::get_Minute", (void*)DateTime_GetMinute);
  scAdd_External_Symbol("DateTime::get_Month", (void*)DateTime_GetMonth);
  scAdd_External_Symbol("DateTime::get_RawTime", (void*)DateTime_GetRawTime);
  scAdd_External_Symbol("DateTime::get_Second", (void*)DateTime_GetSecond);
  scAdd_External_Symbol("DateTime::get_Year", (void*)DateTime_GetYear);

  scAdd_External_Symbol("DrawingSurface::Clear^1", (void *)DrawingSurface_Clear);
  scAdd_External_Symbol("DrawingSurface::CreateCopy^0", (void *)DrawingSurface_CreateCopy);
  scAdd_External_Symbol("DrawingSurface::DrawCircle^3", (void *)DrawingSurface_DrawCircle);
  scAdd_External_Symbol("DrawingSurface::DrawImage^6", (void *)DrawingSurface_DrawImage);
  scAdd_External_Symbol("DrawingSurface::DrawLine^5", (void *)DrawingSurface_DrawLine);
  scAdd_External_Symbol("DrawingSurface::DrawMessageWrapped^5", (void *)DrawingSurface_DrawMessageWrapped);
  scAdd_External_Symbol("DrawingSurface::DrawPixel^2", (void *)DrawingSurface_DrawPixel);
  scAdd_External_Symbol("DrawingSurface::DrawRectangle^4", (void *)DrawingSurface_DrawRectangle);
  scAdd_External_Symbol("DrawingSurface::DrawString^104", (void *)DrawingSurface_DrawString);
  scAdd_External_Symbol("DrawingSurface::DrawStringWrapped^6", (void *)DrawingSurface_DrawStringWrapped);
  scAdd_External_Symbol("DrawingSurface::DrawSurface^2", (void *)DrawingSurface_DrawSurface);
  scAdd_External_Symbol("DrawingSurface::DrawTriangle^6", (void *)DrawingSurface_DrawTriangle);
  scAdd_External_Symbol("DrawingSurface::GetPixel^2", (void *)DrawingSurface_GetPixel);
  scAdd_External_Symbol("DrawingSurface::Release^0", (void *)DrawingSurface_Release);
  scAdd_External_Symbol("DrawingSurface::get_DrawingColor", (void *)DrawingSurface_GetDrawingColor);
  scAdd_External_Symbol("DrawingSurface::set_DrawingColor", (void *)DrawingSurface_SetDrawingColor);
  scAdd_External_Symbol("DrawingSurface::get_Height", (void *)DrawingSurface_GetHeight);
  scAdd_External_Symbol("DrawingSurface::get_UseHighResCoordinates", (void *)DrawingSurface_GetUseHighResCoordinates);
  scAdd_External_Symbol("DrawingSurface::set_UseHighResCoordinates", (void *)DrawingSurface_SetUseHighResCoordinates);
  scAdd_External_Symbol("DrawingSurface::get_Width", (void *)DrawingSurface_GetWidth);

  scAdd_External_Symbol("DynamicSprite::ChangeCanvasSize^4", (void*)DynamicSprite_ChangeCanvasSize);
  scAdd_External_Symbol("DynamicSprite::CopyTransparencyMask^1", (void*)DynamicSprite_CopyTransparencyMask);
  scAdd_External_Symbol("DynamicSprite::Crop^4", (void*)DynamicSprite_Crop);
  scAdd_External_Symbol("DynamicSprite::Delete", (void*)DynamicSprite_Delete);
  scAdd_External_Symbol("DynamicSprite::Flip^1", (void*)DynamicSprite_Flip);
  scAdd_External_Symbol("DynamicSprite::GetDrawingSurface^0", (void*)DynamicSprite_GetDrawingSurface);
  scAdd_External_Symbol("DynamicSprite::Resize^2", (void*)DynamicSprite_Resize);
  scAdd_External_Symbol("DynamicSprite::Rotate^3", (void*)DynamicSprite_Rotate);
  scAdd_External_Symbol("DynamicSprite::SaveToFile^1", (void*)DynamicSprite_SaveToFile);
  scAdd_External_Symbol("DynamicSprite::Tint^5", (void*)DynamicSprite_Tint);
  scAdd_External_Symbol("DynamicSprite::get_ColorDepth", (void*)DynamicSprite_GetColorDepth);
  scAdd_External_Symbol("DynamicSprite::get_Graphic", (void*)DynamicSprite_GetGraphic);
  scAdd_External_Symbol("DynamicSprite::get_Height", (void*)DynamicSprite_GetHeight);
  scAdd_External_Symbol("DynamicSprite::get_Width", (void*)DynamicSprite_GetWidth);
  
  scAdd_External_Symbol("DynamicSprite::Create^3", (void*)DynamicSprite_Create);
  scAdd_External_Symbol("DynamicSprite::CreateFromBackground", (void*)DynamicSprite_CreateFromBackground);
  scAdd_External_Symbol("DynamicSprite::CreateFromDrawingSurface^5", (void*)DynamicSprite_CreateFromDrawingSurface);
  scAdd_External_Symbol("DynamicSprite::CreateFromExistingSprite^1", (void*)DynamicSprite_CreateFromExistingSprite_Old);
  scAdd_External_Symbol("DynamicSprite::CreateFromExistingSprite^2", (void*)DynamicSprite_CreateFromExistingSprite);
  scAdd_External_Symbol("DynamicSprite::CreateFromFile", (void*)DynamicSprite_CreateFromFile);
  scAdd_External_Symbol("DynamicSprite::CreateFromSaveGame", (void*)DynamicSprite_CreateFromSaveGame);
  scAdd_External_Symbol("DynamicSprite::CreateFromScreenShot", (void*)DynamicSprite_CreateFromScreenShot);

  scAdd_External_Symbol("String::IsNullOrEmpty^1", (void*)String_IsNullOrEmpty);
  scAdd_External_Symbol("String::Append^1", (void*)String_Append);
  scAdd_External_Symbol("String::AppendChar^1", (void*)String_AppendChar);
  scAdd_External_Symbol("String::CompareTo^2", (void*)String_CompareTo);
  scAdd_External_Symbol("String::Contains^1", (void*)StrContains);
  scAdd_External_Symbol("String::Copy^0", (void*)String_Copy);
  scAdd_External_Symbol("String::EndsWith^2", (void*)String_EndsWith);
  scAdd_External_Symbol("String::Format^101", (void*)String_Format);
  scAdd_External_Symbol("String::IndexOf^1", (void*)StrContains);
  scAdd_External_Symbol("String::LowerCase^0", (void*)String_LowerCase);
  scAdd_External_Symbol("String::Replace^3", (void*)String_Replace);
  scAdd_External_Symbol("String::ReplaceCharAt^2", (void*)String_ReplaceCharAt);
  scAdd_External_Symbol("String::StartsWith^2", (void*)String_StartsWith);
  scAdd_External_Symbol("String::Substring^2", (void*)String_Substring);
  scAdd_External_Symbol("String::Truncate^1", (void*)String_Truncate);
  scAdd_External_Symbol("String::UpperCase^0", (void*)String_UpperCase);
  scAdd_External_Symbol("String::get_AsFloat", (void*)StringToFloat);
  scAdd_External_Symbol("String::get_AsInt", (void*)StringToInt);
  scAdd_External_Symbol("String::geti_Chars", (void*)String_GetChars);
  scAdd_External_Symbol("String::get_Length", (void*)strlen);

  scAdd_External_Symbol("Game::ChangeTranslation^1", (void *)Game_ChangeTranslation);
  scAdd_External_Symbol("Game::DoOnceOnly^1", (void *)Game_DoOnceOnly);
  scAdd_External_Symbol("Game::GetColorFromRGB^3", (void *)Game_GetColorFromRGB);
  scAdd_External_Symbol("Game::GetFrameCountForLoop^2", (void *)Game_GetFrameCountForLoop);
  scAdd_External_Symbol("Game::GetLocationName^2",(void *)Game_GetLocationName);
  scAdd_External_Symbol("Game::GetLoopCountForView^1", (void *)Game_GetLoopCountForView);
  scAdd_External_Symbol("Game::GetMODPattern^0",(void *)Game_GetMODPattern);
  scAdd_External_Symbol("Game::GetRunNextSettingForLoop^2", (void *)Game_GetRunNextSettingForLoop);
  scAdd_External_Symbol("Game::GetSaveSlotDescription^1",(void *)Game_GetSaveSlotDescription);
  scAdd_External_Symbol("Game::GetViewFrame^3",(void *)Game_GetViewFrame);
  scAdd_External_Symbol("Game::InputBox^1",(void *)Game_InputBox);
  scAdd_External_Symbol("Game::SetSaveGameDirectory^1", (void *)Game_SetSaveGameDirectory);
  scAdd_External_Symbol("Game::StopSound^1", (void *)StopAllSounds);
  scAdd_External_Symbol("Game::get_CharacterCount", (void *)Game_GetCharacterCount);
  scAdd_External_Symbol("Game::get_DialogCount", (void *)Game_GetDialogCount);
  scAdd_External_Symbol("Game::get_FileName", (void *)Game_GetFileName);
  scAdd_External_Symbol("Game::get_FontCount", (void *)Game_GetFontCount);
  scAdd_External_Symbol("Game::geti_GlobalMessages",(void *)Game_GetGlobalMessages);
  scAdd_External_Symbol("Game::geti_GlobalStrings",(void *)Game_GetGlobalStrings);
  scAdd_External_Symbol("Game::seti_GlobalStrings",(void *)SetGlobalString);
  scAdd_External_Symbol("Game::get_GUICount", (void *)Game_GetGUICount);
  scAdd_External_Symbol("Game::get_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_GetIgnoreUserInputAfterTextTimeoutMs);
  scAdd_External_Symbol("Game::set_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_SetIgnoreUserInputAfterTextTimeoutMs);
  scAdd_External_Symbol("Game::get_InSkippableCutscene", (void *)Game_GetInSkippableCutscene);
  scAdd_External_Symbol("Game::get_InventoryItemCount", (void *)Game_GetInventoryItemCount);
  scAdd_External_Symbol("Game::get_MinimumTextDisplayTimeMs", (void *)Game_GetMinimumTextDisplayTimeMs);
  scAdd_External_Symbol("Game::set_MinimumTextDisplayTimeMs", (void *)Game_SetMinimumTextDisplayTimeMs);
  scAdd_External_Symbol("Game::get_MouseCursorCount", (void *)Game_GetMouseCursorCount);
  scAdd_External_Symbol("Game::get_Name", (void *)Game_GetName);
  scAdd_External_Symbol("Game::set_Name", (void *)Game_SetName);
  scAdd_External_Symbol("Game::get_NormalFont", (void *)Game_GetNormalFont);
  scAdd_External_Symbol("Game::set_NormalFont", (void *)SetNormalFont);
  scAdd_External_Symbol("Game::get_SkippingCutscene", (void *)Game_GetSkippingCutscene);
  scAdd_External_Symbol("Game::get_SpeechFont", (void *)Game_GetSpeechFont);
  scAdd_External_Symbol("Game::set_SpeechFont", (void *)SetSpeechFont);
  scAdd_External_Symbol("Game::geti_SpriteWidth", (void *)Game_GetSpriteWidth);
  scAdd_External_Symbol("Game::geti_SpriteHeight", (void *)Game_GetSpriteHeight);
  scAdd_External_Symbol("Game::get_TextReadingSpeed", (void *)Game_GetTextReadingSpeed);
  scAdd_External_Symbol("Game::set_TextReadingSpeed", (void *)Game_SetTextReadingSpeed);
  scAdd_External_Symbol("Game::get_TranslationFilename",(void *)Game_GetTranslationFilename);
  scAdd_External_Symbol("Game::get_UseNativeCoordinates", (void *)Game_GetUseNativeCoordinates);
  scAdd_External_Symbol("Game::get_ViewCount", (void *)Game_GetViewCount);

  scAdd_External_Symbol("System::get_CapsLock", (void *)System_GetCapsLock);
  scAdd_External_Symbol("System::get_ColorDepth", (void *)System_GetColorDepth);
  scAdd_External_Symbol("System::get_Gamma", (void *)System_GetGamma);
  scAdd_External_Symbol("System::set_Gamma", (void *)System_SetGamma);
  scAdd_External_Symbol("System::get_HardwareAcceleration", (void *)System_GetHardwareAcceleration);
  scAdd_External_Symbol("System::get_NumLock", (void *)System_GetNumLock);
  scAdd_External_Symbol("System::set_NumLock", (void *)System_SetNumLock);
  scAdd_External_Symbol("System::get_OperatingSystem", (void *)System_GetOS);
  scAdd_External_Symbol("System::get_ScreenHeight", (void *)System_GetScreenHeight);
  scAdd_External_Symbol("System::get_ScreenWidth", (void *)System_GetScreenWidth);
  scAdd_External_Symbol("System::get_ScrollLock", (void *)System_GetScrollLock);
  scAdd_External_Symbol("System::get_SupportsGammaControl", (void *)System_GetSupportsGammaControl);
  scAdd_External_Symbol("System::get_Version", (void *)System_GetVersion);
  scAdd_External_Symbol("SystemInfo::get_Version", (void *)System_GetVersion);
  scAdd_External_Symbol("System::get_ViewportHeight", (void *)System_GetViewportHeight);
  scAdd_External_Symbol("System::get_ViewportWidth", (void *)System_GetViewportWidth);
  scAdd_External_Symbol("System::get_Volume",(void *)System_GetVolume);
  scAdd_External_Symbol("System::set_Volume",(void *)System_SetVolume);
  scAdd_External_Symbol("System::get_VSync", (void *)System_GetVsync);
  scAdd_External_Symbol("System::set_VSync", (void *)System_SetVsync);
  scAdd_External_Symbol("System::get_Windowed", (void *)System_GetWindowed);

  scAdd_External_Symbol("Room::GetDrawingSurfaceForBackground^1", (void *)Room_GetDrawingSurfaceForBackground);
  scAdd_External_Symbol("Room::GetTextProperty^1",(void *)Room_GetTextProperty);
  scAdd_External_Symbol("Room::get_BottomEdge", (void *)Room_GetBottomEdge);
  scAdd_External_Symbol("Room::get_ColorDepth", (void *)Room_GetColorDepth);
  scAdd_External_Symbol("Room::get_Height", (void *)Room_GetHeight);
  scAdd_External_Symbol("Room::get_LeftEdge", (void *)Room_GetLeftEdge);
  scAdd_External_Symbol("Room::geti_Messages",(void *)Room_GetMessages);
  scAdd_External_Symbol("Room::get_MusicOnLoad", (void *)Room_GetMusicOnLoad);
  scAdd_External_Symbol("Room::get_ObjectCount", (void *)Room_GetObjectCount);
  scAdd_External_Symbol("Room::get_RightEdge", (void *)Room_GetRightEdge);
  scAdd_External_Symbol("Room::get_TopEdge", (void *)Room_GetTopEdge);
  scAdd_External_Symbol("Room::get_Width", (void *)Room_GetWidth);

  scAdd_External_Symbol("Parser::FindWordID^1",(void *)Parser_FindWordID);
  scAdd_External_Symbol("Parser::ParseText^1",(void *)ParseText);
  scAdd_External_Symbol("Parser::SaidUnknownWord^0",(void *)Parser_SaidUnknownWord);
  scAdd_External_Symbol("Parser::Said^1",(void *)Said);

  scAdd_External_Symbol("ViewFrame::get_Flipped", (void *)ViewFrame_GetFlipped);
  scAdd_External_Symbol("ViewFrame::get_Frame", (void *)ViewFrame_GetFrame);
  scAdd_External_Symbol("ViewFrame::get_Graphic", (void *)ViewFrame_GetGraphic);
  scAdd_External_Symbol("ViewFrame::set_Graphic", (void *)ViewFrame_SetGraphic);
  scAdd_External_Symbol("ViewFrame::get_LinkedAudio", (void *)ViewFrame_GetLinkedAudio);
  scAdd_External_Symbol("ViewFrame::set_LinkedAudio", (void *)ViewFrame_SetLinkedAudio);
  scAdd_External_Symbol("ViewFrame::get_Loop", (void *)ViewFrame_GetLoop);
  scAdd_External_Symbol("ViewFrame::get_Sound", (void *)ViewFrame_GetSound);
  scAdd_External_Symbol("ViewFrame::set_Sound", (void *)ViewFrame_SetSound);
  scAdd_External_Symbol("ViewFrame::get_Speed", (void *)ViewFrame_GetSpeed);
  scAdd_External_Symbol("ViewFrame::get_View", (void *)ViewFrame_GetView);
  
  scAdd_External_Symbol("AbortGame",(void *)_sc_AbortGame);
  scAdd_External_Symbol("AddInventory",(void *)add_inventory);
  scAdd_External_Symbol("AddInventoryToCharacter",(void *)AddInventoryToCharacter);
  scAdd_External_Symbol("AnimateButton",(void *)AnimateButton);
  scAdd_External_Symbol("AnimateCharacter",(void *)scAnimateCharacter);
  scAdd_External_Symbol("AnimateCharacterEx",(void *)AnimateCharacterEx);
  scAdd_External_Symbol("AnimateObject",(void *)AnimateObject);
  scAdd_External_Symbol("AnimateObjectEx",(void *)AnimateObjectEx);
  scAdd_External_Symbol("AreCharactersColliding",(void *)AreCharactersColliding);
  scAdd_External_Symbol("AreCharObjColliding",(void *)AreCharObjColliding);
  scAdd_External_Symbol("AreObjectsColliding",(void *)AreObjectsColliding);
  scAdd_External_Symbol("AreThingsOverlapping",(void *)AreThingsOverlapping);
  scAdd_External_Symbol("CallRoomScript",(void *)CallRoomScript);
  scAdd_External_Symbol("CDAudio",(void *)cd_manager);
  scAdd_External_Symbol("CentreGUI",(void *)CentreGUI);
  scAdd_External_Symbol("ChangeCharacterView",(void *)ChangeCharacterView);
  scAdd_External_Symbol("ChangeCursorGraphic",(void *)ChangeCursorGraphic);
  scAdd_External_Symbol("ChangeCursorHotspot",(void *)ChangeCursorHotspot);
  scAdd_External_Symbol("ClaimEvent",(void *)ClaimEvent);
  scAdd_External_Symbol("CreateGraphicOverlay",(void *)CreateGraphicOverlay);
  scAdd_External_Symbol("CreateTextOverlay",(void *)CreateTextOverlay);
  scAdd_External_Symbol("CyclePalette",(void *)CyclePalette);
  scAdd_External_Symbol("Debug",(void *)script_debug);
  scAdd_External_Symbol("DeleteSaveSlot",(void *)DeleteSaveSlot);
  scAdd_External_Symbol("DeleteSprite",(void *)free_dynamic_sprite);
  scAdd_External_Symbol("DisableCursorMode",(void *)disable_cursor_mode);
  scAdd_External_Symbol("DisableGroundLevelAreas",(void *)DisableGroundLevelAreas);
  scAdd_External_Symbol("DisableHotspot",(void *)DisableHotspot);
  scAdd_External_Symbol("DisableInterface",(void *)DisableInterface);
  scAdd_External_Symbol("DisableRegion",(void *)DisableRegion);
  scAdd_External_Symbol("Display",(void *)Display);
  scAdd_External_Symbol("DisplayAt",(void *)DisplayAt);
  scAdd_External_Symbol("DisplayAtY",(void *)DisplayAtY);
  scAdd_External_Symbol("DisplayMessage",(void *)DisplayMessage);
  scAdd_External_Symbol("DisplayMessageAtY",(void *)DisplayMessageAtY);
  scAdd_External_Symbol("DisplayMessageBar",(void *)DisplayMessageBar);
  scAdd_External_Symbol("DisplaySpeech",(void *)__sc_displayspeech);
  scAdd_External_Symbol("DisplaySpeechAt", (void *)DisplaySpeechAt);
  scAdd_External_Symbol("DisplaySpeechBackground",(void *)DisplaySpeechBackground);
  scAdd_External_Symbol("DisplayThought",(void *)DisplayThought);
  scAdd_External_Symbol("DisplayTopBar",(void *)DisplayTopBar);
  scAdd_External_Symbol("EnableCursorMode",(void *)enable_cursor_mode);
  scAdd_External_Symbol("EnableGroundLevelAreas",(void *)EnableGroundLevelAreas);
  scAdd_External_Symbol("EnableHotspot",(void *)EnableHotspot);
  scAdd_External_Symbol("EnableInterface",(void *)EnableInterface);
  scAdd_External_Symbol("EnableRegion",(void *)EnableRegion);
  scAdd_External_Symbol("EndCutscene", (void *)EndCutscene);
  scAdd_External_Symbol("FaceCharacter",(void *)FaceCharacter);
  scAdd_External_Symbol("FaceLocation",(void *)FaceLocation);
  scAdd_External_Symbol("FadeIn",(void *)FadeIn);
  scAdd_External_Symbol("FadeOut",(void *)my_fade_out);
  scAdd_External_Symbol("FileClose",(void *)FileClose);
  scAdd_External_Symbol("FileIsEOF",(void *)FileIsEOF);
  scAdd_External_Symbol("FileIsError",(void *)FileIsError);
  scAdd_External_Symbol("FileOpen",(void *)FileOpen);
  scAdd_External_Symbol("FileRead",(void *)FileRead);
  scAdd_External_Symbol("FileReadInt",(void *)FileReadInt);
  scAdd_External_Symbol("FileReadRawChar",(void *)FileReadRawChar);
  scAdd_External_Symbol("FileReadRawInt",(void *)FileReadRawInt);
  scAdd_External_Symbol("FileWrite",(void *)FileWrite);
  scAdd_External_Symbol("FileWriteInt",(void *)FileWriteInt);
  scAdd_External_Symbol("FileWriteRawChar",(void *)FileWriteRawChar);
  scAdd_External_Symbol("FileWriteRawLine", (void *)FileWriteRawLine);
  scAdd_External_Symbol("FindGUIID",(void *)FindGUIID);
  scAdd_External_Symbol("FlipScreen",(void *)FlipScreen);
  scAdd_External_Symbol("FloatToInt",(void *)FloatToInt);
  scAdd_External_Symbol("FollowCharacter",(void *)FollowCharacter);
  scAdd_External_Symbol("FollowCharacterEx",(void *)FollowCharacterEx);
  scAdd_External_Symbol("GetBackgroundFrame",(void *)GetBackgroundFrame);
  scAdd_External_Symbol("GetButtonPic",(void *)GetButtonPic);
  scAdd_External_Symbol("GetCharacterAt",(void *)GetCharacterAt);
  scAdd_External_Symbol("GetCharacterProperty",(void *)GetCharacterProperty);
  scAdd_External_Symbol("GetCharacterPropertyText",(void *)GetCharacterPropertyText);
  scAdd_External_Symbol("GetCurrentMusic",(void *)GetCurrentMusic);
  scAdd_External_Symbol("GetCursorMode",(void *)GetCursorMode);
  scAdd_External_Symbol("GetDialogOption",(void *)GetDialogOption);
  scAdd_External_Symbol("GetGameOption",(void *)GetGameOption);
  scAdd_External_Symbol("GetGameParameter",(void *)GetGameParameter);
  scAdd_External_Symbol("GetGameSpeed",(void *)GetGameSpeed);
  scAdd_External_Symbol("GetGlobalInt",(void *)GetGlobalInt);
  scAdd_External_Symbol("GetGlobalString",(void *)GetGlobalString);
  scAdd_External_Symbol("GetGraphicalVariable",(void *)GetGraphicalVariable);
  scAdd_External_Symbol("GetGUIAt", (void *)GetGUIAt);
  scAdd_External_Symbol("GetGUIObjectAt", (void *)GetGUIObjectAt);
  scAdd_External_Symbol("GetHotspotAt",(void *)GetHotspotAt);
  scAdd_External_Symbol("GetHotspotName",(void *)GetHotspotName);
  scAdd_External_Symbol("GetHotspotPointX",(void *)GetHotspotPointX);
  scAdd_External_Symbol("GetHotspotPointY",(void *)GetHotspotPointY);
  scAdd_External_Symbol("GetHotspotProperty",(void *)GetHotspotProperty);
  scAdd_External_Symbol("GetHotspotPropertyText",(void *)GetHotspotPropertyText);
  scAdd_External_Symbol("GetInvAt",(void *)GetInvAt);
  scAdd_External_Symbol("GetInvGraphic",(void *)GetInvGraphic);
  scAdd_External_Symbol("GetInvName",(void *)GetInvName);
  scAdd_External_Symbol("GetInvProperty",(void *)GetInvProperty);
  scAdd_External_Symbol("GetInvPropertyText",(void *)GetInvPropertyText);
  //scAdd_External_Symbol("GetLanguageString",(void *)GetLanguageString);
  scAdd_External_Symbol("GetLocationName",(void *)GetLocationName);
  scAdd_External_Symbol("GetLocationType",(void *)GetLocationType);
  scAdd_External_Symbol("GetMessageText", (void *)GetMessageText);
  scAdd_External_Symbol("GetMIDIPosition", (void *)GetMIDIPosition);
  scAdd_External_Symbol("GetMP3PosMillis", (void *)GetMP3PosMillis);
  scAdd_External_Symbol("GetObjectAt",(void *)GetObjectAt);
  scAdd_External_Symbol("GetObjectBaseline",(void *)GetObjectBaseline);
  scAdd_External_Symbol("GetObjectGraphic",(void *)GetObjectGraphic);
  scAdd_External_Symbol("GetObjectName",(void *)GetObjectName);
  scAdd_External_Symbol("GetObjectProperty",(void *)GetObjectProperty);
  scAdd_External_Symbol("GetObjectPropertyText",(void *)GetObjectPropertyText);
  scAdd_External_Symbol("GetObjectX",(void *)GetObjectX);
  scAdd_External_Symbol("GetObjectY",(void *)GetObjectY);
//  scAdd_External_Symbol("GetPalette",(void *)scGetPal);
  scAdd_External_Symbol("GetPlayerCharacter",(void *)GetPlayerCharacter);
  scAdd_External_Symbol("GetRawTime",(void *)GetRawTime);
  scAdd_External_Symbol("GetRegionAt",(void *)GetRegionAt);
  scAdd_External_Symbol("GetRoomProperty",(void *)GetRoomProperty);
  scAdd_External_Symbol("GetRoomPropertyText",(void *)GetRoomPropertyText);
  scAdd_External_Symbol("GetSaveSlotDescription",(void *)GetSaveSlotDescription);
  scAdd_External_Symbol("GetScalingAt",(void *)GetScalingAt);
  scAdd_External_Symbol("GetSliderValue",(void *)GetSliderValue);
  scAdd_External_Symbol("GetTextBoxText",(void *)GetTextBoxText);
  scAdd_External_Symbol("GetTextHeight",(void *)GetTextHeight);
  scAdd_External_Symbol("GetTextWidth",(void *)GetTextWidth);
  scAdd_External_Symbol("GetTime",(void *)sc_GetTime);
  scAdd_External_Symbol("GetTranslation", (void *)get_translation);
  scAdd_External_Symbol("GetTranslationName", (void *)GetTranslationName);
  scAdd_External_Symbol("GetViewportX",(void *)GetViewportX);
  scAdd_External_Symbol("GetViewportY",(void *)GetViewportY);
  scAdd_External_Symbol("GetWalkableAreaAt",(void *)GetWalkableAreaAt);
  scAdd_External_Symbol("GiveScore",(void *)GiveScore);
  scAdd_External_Symbol("HasPlayerBeenInRoom",(void *)HasPlayerBeenInRoom);
  scAdd_External_Symbol("HideMouseCursor",(void *)HideMouseCursor);
  scAdd_External_Symbol("InputBox",(void *)sc_inputbox);
  scAdd_External_Symbol("InterfaceOff",(void *)InterfaceOff);
  scAdd_External_Symbol("InterfaceOn",(void *)InterfaceOn);
  scAdd_External_Symbol("IntToFloat",(void *)IntToFloat);
  scAdd_External_Symbol("InventoryScreen",(void *)sc_invscreen);
  scAdd_External_Symbol("IsButtonDown",(void *)IsButtonDown);
  scAdd_External_Symbol("IsChannelPlaying",(void *)IsChannelPlaying);
  scAdd_External_Symbol("IsGamePaused",(void *)IsGamePaused);
  scAdd_External_Symbol("IsGUIOn", (void *)IsGUIOn);
  scAdd_External_Symbol("IsInteractionAvailable", (void *)IsInteractionAvailable);
  scAdd_External_Symbol("IsInventoryInteractionAvailable", (void *)IsInventoryInteractionAvailable);
  scAdd_External_Symbol("IsInterfaceEnabled", (void *)IsInterfaceEnabled);
  scAdd_External_Symbol("IsKeyPressed",(void *)IsKeyPressed);
  scAdd_External_Symbol("IsMusicPlaying",(void *)IsMusicPlaying);
  scAdd_External_Symbol("IsMusicVoxAvailable",(void *)IsMusicVoxAvailable);
  scAdd_External_Symbol("IsObjectAnimating",(void *)IsObjectAnimating);
  scAdd_External_Symbol("IsObjectMoving",(void *)IsObjectMoving);
  scAdd_External_Symbol("IsObjectOn",(void *)IsObjectOn);
  scAdd_External_Symbol("IsOverlayValid",(void *)IsOverlayValid);
  scAdd_External_Symbol("IsSoundPlaying",(void *)IsSoundPlaying);
  scAdd_External_Symbol("IsTimerExpired",(void *)IsTimerExpired);
  scAdd_External_Symbol("IsTranslationAvailable", (void *)IsTranslationAvailable);
  scAdd_External_Symbol("IsVoxAvailable",(void *)IsVoxAvailable);
  scAdd_External_Symbol("ListBoxAdd", (void *)ListBoxAdd);
  scAdd_External_Symbol("ListBoxClear", (void *)ListBoxClear);
  scAdd_External_Symbol("ListBoxDirList", (void *)ListBoxDirList);
  scAdd_External_Symbol("ListBoxGetItemText", (void *)ListBoxGetItemText);
  scAdd_External_Symbol("ListBoxGetNumItems", (void *)ListBoxGetNumItems);
  scAdd_External_Symbol("ListBoxGetSelected", (void *)ListBoxGetSelected);
  scAdd_External_Symbol("ListBoxRemove", (void *)ListBoxRemove);
  scAdd_External_Symbol("ListBoxSaveGameList", (void *)ListBoxSaveGameList);
  scAdd_External_Symbol("ListBoxSetSelected", (void *)ListBoxSetSelected);
  scAdd_External_Symbol("ListBoxSetTopItem", (void *)ListBoxSetTopItem);
  scAdd_External_Symbol("LoadImageFile",(void *)LoadImageFile);
  scAdd_External_Symbol("LoadSaveSlotScreenshot",(void *)LoadSaveSlotScreenshot);
  scAdd_External_Symbol("LoseInventory",(void *)lose_inventory);
  scAdd_External_Symbol("LoseInventoryFromCharacter",(void *)LoseInventoryFromCharacter);
  scAdd_External_Symbol("MergeObject",(void *)MergeObject);
  scAdd_External_Symbol("MoveCharacter",(void *)MoveCharacter);
  scAdd_External_Symbol("MoveCharacterBlocking",(void *)MoveCharacterBlocking);
  scAdd_External_Symbol("MoveCharacterDirect",(void *)MoveCharacterDirect);
  scAdd_External_Symbol("MoveCharacterPath",(void *)MoveCharacterPath);
  scAdd_External_Symbol("MoveCharacterStraight",(void *)MoveCharacterStraight);
  scAdd_External_Symbol("MoveCharacterToHotspot",(void *)MoveCharacterToHotspot);
  scAdd_External_Symbol("MoveCharacterToObject",(void *)MoveCharacterToObject);
  scAdd_External_Symbol("MoveObject",(void *)MoveObject);
  scAdd_External_Symbol("MoveObjectDirect",(void *)MoveObjectDirect);
  scAdd_External_Symbol("MoveOverlay",(void *)MoveOverlay);
  scAdd_External_Symbol("MoveToWalkableArea", (void *)MoveToWalkableArea);
  scAdd_External_Symbol("NewRoom",(void *)NewRoom);
  scAdd_External_Symbol("NewRoomEx",(void *)NewRoomEx);
  scAdd_External_Symbol("NewRoomNPC",(void *)NewRoomNPC);
  scAdd_External_Symbol("ObjectOff",(void *)ObjectOff);
  scAdd_External_Symbol("ObjectOn",(void *)ObjectOn);
  scAdd_External_Symbol("ParseText",(void *)ParseText);
  scAdd_External_Symbol("PauseGame",(void *)PauseGame);
  scAdd_External_Symbol("PlayAmbientSound",(void *)PlayAmbientSound);
  scAdd_External_Symbol("PlayFlic",(void *)play_flc_file);
  scAdd_External_Symbol("PlayMP3File",(void *)PlayMP3File);
  scAdd_External_Symbol("PlayMusic",(void *)PlayMusicResetQueue);
  scAdd_External_Symbol("PlayMusicQueued",(void *)PlayMusicQueued);
  scAdd_External_Symbol("PlaySilentMIDI",(void *)PlaySilentMIDI);
  scAdd_External_Symbol("PlaySound",(void *)play_sound);
  scAdd_External_Symbol("PlaySoundEx",(void *)PlaySoundEx);
  scAdd_External_Symbol("PlaySpeech",(void *)__scr_play_speech);
  scAdd_External_Symbol("PlayVideo",(void *)scrPlayVideo);
  scAdd_External_Symbol("ProcessClick",(void *)ProcessClick);
  scAdd_External_Symbol("QuitGame",(void *)QuitGame);
  scAdd_External_Symbol("Random",(void *)__Rand);
  scAdd_External_Symbol("RawClearScreen", (void *)RawClear);
  scAdd_External_Symbol("RawDrawCircle",(void *)RawDrawCircle);
  scAdd_External_Symbol("RawDrawFrameTransparent",(void *)RawDrawFrameTransparent);
  scAdd_External_Symbol("RawDrawImage", (void *)RawDrawImage);
  scAdd_External_Symbol("RawDrawImageOffset", (void *)RawDrawImageOffset);
  scAdd_External_Symbol("RawDrawImageResized", (void *)RawDrawImageResized);
  scAdd_External_Symbol("RawDrawImageTransparent", (void *)RawDrawImageTransparent);
  scAdd_External_Symbol("RawDrawLine", (void *)RawDrawLine);
  scAdd_External_Symbol("RawDrawRectangle", (void *)RawDrawRectangle);
  scAdd_External_Symbol("RawDrawTriangle", (void *)RawDrawTriangle);
  scAdd_External_Symbol("RawPrint", (void *)RawPrint);
  scAdd_External_Symbol("RawPrintMessageWrapped", (void *)RawPrintMessageWrapped);
  scAdd_External_Symbol("RawRestoreScreen", (void *)RawRestoreScreen);
  scAdd_External_Symbol("RawRestoreScreenTinted", (void *)RawRestoreScreenTinted);
  scAdd_External_Symbol("RawSaveScreen", (void *)RawSaveScreen);
  scAdd_External_Symbol("RawSetColor", (void *)RawSetColor);
  scAdd_External_Symbol("RawSetColorRGB", (void *)RawSetColorRGB);
  scAdd_External_Symbol("RefreshMouse",(void *)RefreshMouse);
  scAdd_External_Symbol("ReleaseCharacterView",(void *)ReleaseCharacterView);
  scAdd_External_Symbol("ReleaseViewport",(void *)ReleaseViewport);
  scAdd_External_Symbol("RemoveObjectTint",(void *)RemoveObjectTint);
  scAdd_External_Symbol("RemoveOverlay",(void *)RemoveOverlay);
  scAdd_External_Symbol("RemoveWalkableArea",(void *)RemoveWalkableArea);
  scAdd_External_Symbol("ResetRoom",(void *)ResetRoom);
  scAdd_External_Symbol("RestartGame",(void *)restart_game);
  scAdd_External_Symbol("RestoreGameDialog",(void *)restore_game_dialog);
  scAdd_External_Symbol("RestoreGameSlot",(void *)RestoreGameSlot);
  scAdd_External_Symbol("RestoreWalkableArea",(void *)RestoreWalkableArea);
  scAdd_External_Symbol("RunAGSGame", (void *)RunAGSGame);
  scAdd_External_Symbol("RunCharacterInteraction",(void *)RunCharacterInteraction);
  scAdd_External_Symbol("RunDialog",(void *)RunDialog);
  scAdd_External_Symbol("RunHotspotInteraction", (void *)RunHotspotInteraction);
  scAdd_External_Symbol("RunInventoryInteraction", (void *)RunInventoryInteraction);
  scAdd_External_Symbol("RunObjectInteraction", (void *)RunObjectInteraction);
  scAdd_External_Symbol("RunRegionInteraction", (void *)RunRegionInteraction);
  scAdd_External_Symbol("Said",(void *)Said);
  scAdd_External_Symbol("SaidUnknownWord",(void *)SaidUnknownWord);
  scAdd_External_Symbol("SaveCursorForLocationChange",(void *)SaveCursorForLocationChange);
  scAdd_External_Symbol("SaveGameDialog",(void *)save_game_dialog);
  scAdd_External_Symbol("SaveGameSlot",(void *)save_game);
  scAdd_External_Symbol("SaveScreenShot",(void *)SaveScreenShot);
  scAdd_External_Symbol("SeekMIDIPosition", (void *)SeekMIDIPosition);
  scAdd_External_Symbol("SeekMODPattern",(void *)SeekMODPattern);
  scAdd_External_Symbol("SeekMP3PosMillis", (void *)SeekMP3PosMillis);
  scAdd_External_Symbol("SetActiveInventory",(void *)SetActiveInventory);
  scAdd_External_Symbol("SetAmbientTint",(void *)SetAmbientTint);
  scAdd_External_Symbol("SetAreaLightLevel",(void *)SetAreaLightLevel);
  scAdd_External_Symbol("SetAreaScaling",(void *)SetAreaScaling);
  scAdd_External_Symbol("SetBackgroundFrame",(void *)SetBackgroundFrame);
  scAdd_External_Symbol("SetButtonPic",(void *)SetButtonPic);
  scAdd_External_Symbol("SetButtonText",(void *)SetButtonText);
  scAdd_External_Symbol("SetChannelVolume",(void *)SetChannelVolume);
  scAdd_External_Symbol("SetCharacterBaseline",(void *)SetCharacterBaseline);
  scAdd_External_Symbol("SetCharacterClickable",(void *)SetCharacterClickable);
  scAdd_External_Symbol("SetCharacterFrame",(void *)SetCharacterFrame);
  scAdd_External_Symbol("SetCharacterIdle",(void *)SetCharacterIdle);
  scAdd_External_Symbol("SetCharacterIgnoreLight",(void *)SetCharacterIgnoreLight);
  scAdd_External_Symbol("SetCharacterIgnoreWalkbehinds",(void *)SetCharacterIgnoreWalkbehinds);
  scAdd_External_Symbol("SetCharacterProperty",(void *)SetCharacterProperty);
  scAdd_External_Symbol("SetCharacterBlinkView",(void *)SetCharacterBlinkView);
  scAdd_External_Symbol("SetCharacterSpeechView",(void *)SetCharacterSpeechView);
  scAdd_External_Symbol("SetCharacterSpeed",(void *)SetCharacterSpeed);
  scAdd_External_Symbol("SetCharacterSpeedEx",(void *)SetCharacterSpeedEx);
  scAdd_External_Symbol("SetCharacterTransparency",(void *)SetCharacterTransparency);
  scAdd_External_Symbol("SetCharacterView",(void *)SetCharacterView);
  scAdd_External_Symbol("SetCharacterViewEx",(void *)SetCharacterViewEx);
  scAdd_External_Symbol("SetCharacterViewOffset",(void *)SetCharacterViewOffset);
  scAdd_External_Symbol("SetCursorMode",(void *)set_cursor_mode);
  scAdd_External_Symbol("SetDefaultCursor",(void *)set_default_cursor);
  scAdd_External_Symbol("SetDialogOption",(void *)SetDialogOption);
  scAdd_External_Symbol("SetDigitalMasterVolume",(void *)SetDigitalMasterVolume);
  scAdd_External_Symbol("SetFadeColor",(void *)SetFadeColor);
  scAdd_External_Symbol("SetFrameSound",(void *)SetFrameSound);
  scAdd_External_Symbol("SetGameOption",(void *)SetGameOption);
  scAdd_External_Symbol("SetGameSpeed",(void *)SetGameSpeed);
  scAdd_External_Symbol("SetGlobalInt",(void *)SetGlobalInt);
  scAdd_External_Symbol("SetGlobalString",(void *)SetGlobalString);
  scAdd_External_Symbol("SetGraphicalVariable",(void *)SetGraphicalVariable);
  scAdd_External_Symbol("SetGUIBackgroundPic", (void *)SetGUIBackgroundPic);
  scAdd_External_Symbol("SetGUIClickable", (void *)SetGUIClickable);
  scAdd_External_Symbol("SetGUIObjectEnabled",(void *)SetGUIObjectEnabled);
  scAdd_External_Symbol("SetGUIObjectPosition",(void *)SetGUIObjectPosition);
  scAdd_External_Symbol("SetGUIObjectSize",(void *)SetGUIObjectSize);
  scAdd_External_Symbol("SetGUIPosition",(void *)SetGUIPosition);
  scAdd_External_Symbol("SetGUISize",(void *)SetGUISize);
  scAdd_External_Symbol("SetGUITransparency", (void *)SetGUITransparency);
  scAdd_External_Symbol("SetGUIZOrder", (void *)SetGUIZOrder);
  scAdd_External_Symbol("SetInvItemName",(void *)SetInvItemName);
  scAdd_External_Symbol("SetInvItemPic",(void *)set_inv_item_pic);
  scAdd_External_Symbol("SetInvDimensions",(void *)SetInvDimensions);
  scAdd_External_Symbol("SetLabelColor",(void *)SetLabelColor);
  scAdd_External_Symbol("SetLabelFont",(void *)SetLabelFont);
  scAdd_External_Symbol("SetLabelText",(void *)SetLabelText);
  scAdd_External_Symbol("SetMouseBounds",(void *)SetMouseBounds);
  scAdd_External_Symbol("SetMouseCursor",(void *)set_mouse_cursor);
  scAdd_External_Symbol("SetMousePosition",(void *)SetMousePosition);
  scAdd_External_Symbol("SetMultitaskingMode",(void *)SetMultitasking);
  scAdd_External_Symbol("SetMusicMasterVolume",(void *)SetMusicMasterVolume);
  scAdd_External_Symbol("SetMusicRepeat",(void *)SetMusicRepeat);
  scAdd_External_Symbol("SetMusicVolume",(void *)SetMusicVolume);
  scAdd_External_Symbol("SetNextCursorMode", (void *)SetNextCursor);
  scAdd_External_Symbol("SetNextScreenTransition",(void *)SetNextScreenTransition);
  scAdd_External_Symbol("SetNormalFont", (void *)SetNormalFont);
  scAdd_External_Symbol("SetObjectBaseline",(void *)SetObjectBaseline);
  scAdd_External_Symbol("SetObjectClickable",(void *)SetObjectClickable);
  scAdd_External_Symbol("SetObjectFrame",(void *)SetObjectFrame);
  scAdd_External_Symbol("SetObjectGraphic",(void *)SetObjectGraphic);
  scAdd_External_Symbol("SetObjectIgnoreWalkbehinds",(void *)SetObjectIgnoreWalkbehinds);
  scAdd_External_Symbol("SetObjectPosition",(void *)SetObjectPosition);
  scAdd_External_Symbol("SetObjectTint",(void *)SetObjectTint);
  scAdd_External_Symbol("SetObjectTransparency",(void *)SetObjectTransparency);
  scAdd_External_Symbol("SetObjectView",(void *)SetObjectView);
//  scAdd_External_Symbol("SetPalette",scSetPal);
  scAdd_External_Symbol("SetPalRGB",(void *)SetPalRGB);
  scAdd_External_Symbol("SetPlayerCharacter",(void *)SetPlayerCharacter);
  scAdd_External_Symbol("SetRegionTint",(void *)SetRegionTint);
  scAdd_External_Symbol("SetRestartPoint",(void *)SetRestartPoint);
  scAdd_External_Symbol("SetScreenTransition",(void *)SetScreenTransition);
  scAdd_External_Symbol("SetSkipSpeech",(void *)SetSkipSpeech);
  scAdd_External_Symbol("SetSliderValue",(void *)SetSliderValue);
  scAdd_External_Symbol("SetSoundVolume",(void *)SetSoundVolume);
  scAdd_External_Symbol("SetSpeechFont", (void *)SetSpeechFont);
  scAdd_External_Symbol("SetSpeechStyle", (void *)SetSpeechStyle);
  scAdd_External_Symbol("SetSpeechVolume",(void *)SetSpeechVolume);
  scAdd_External_Symbol("SetTalkingColor",(void *)SetTalkingColor);
  scAdd_External_Symbol("SetTextBoxFont",(void *)SetTextBoxFont);
  scAdd_External_Symbol("SetTextBoxText",(void *)SetTextBoxText);
  scAdd_External_Symbol("SetTextOverlay",(void *)SetTextOverlay);
  scAdd_External_Symbol("SetTextWindowGUI",(void *)SetTextWindowGUI);
  scAdd_External_Symbol("SetTimer",(void *)script_SetTimer);
  scAdd_External_Symbol("SetViewport",(void *)SetViewport);
  scAdd_External_Symbol("SetVoiceMode",(void *)SetVoiceMode);
  scAdd_External_Symbol("SetWalkBehindBase",(void *)SetWalkBehindBase);
  scAdd_External_Symbol("ShakeScreen",(void *)ShakeScreen);
  scAdd_External_Symbol("ShakeScreenBackground",(void *)ShakeScreenBackground);
  scAdd_External_Symbol("ShowMouseCursor",(void *)ShowMouseCursor);
  scAdd_External_Symbol("SkipUntilCharacterStops",(void *)SkipUntilCharacterStops);
  scAdd_External_Symbol("StartCutscene", (void *)StartCutscene);
  scAdd_External_Symbol("StartRecording", (void *)scStartRecording);
  scAdd_External_Symbol("StopAmbientSound",(void *)StopAmbientSound);
  scAdd_External_Symbol("StopChannel",(void *)stop_and_destroy_channel);
  scAdd_External_Symbol("StopDialog",(void *)StopDialog);
  scAdd_External_Symbol("StopMoving",(void *)StopMoving);
  scAdd_External_Symbol("StopMusic", (void *)scr_StopMusic);
  scAdd_External_Symbol("StopObjectMoving",(void *)StopObjectMoving);
  scAdd_External_Symbol("StrCat",(void *)_sc_strcat);
  scAdd_External_Symbol("StrCaseComp",(void *)stricmp);
  scAdd_External_Symbol("StrComp",(void *)strcmp);
  scAdd_External_Symbol("StrContains",(void *)StrContains);
  scAdd_External_Symbol("StrCopy",(void *)_sc_strcpy);
  scAdd_External_Symbol("StrFormat",(void *)_sc_sprintf);
  scAdd_External_Symbol("StrGetCharAt", (void *)StrGetCharAt);
  scAdd_External_Symbol("StringToInt",(void *)StringToInt);
  scAdd_External_Symbol("StrLen",(void *)strlen);
  scAdd_External_Symbol("StrSetCharAt", (void *)StrSetCharAt);
  scAdd_External_Symbol("StrToLowerCase", (void *)_sc_strlower);
  scAdd_External_Symbol("StrToUpperCase", (void *)_sc_strupper);
  scAdd_External_Symbol("TintScreen",(void *)TintScreen);
  scAdd_External_Symbol("UnPauseGame",(void *)UnPauseGame);
  scAdd_External_Symbol("UpdateInventory", (void *)update_invorder);
  scAdd_External_Symbol("UpdatePalette",(void *)UpdatePalette);
  scAdd_External_Symbol("Wait",(void *)scrWait);
  scAdd_External_Symbol("WaitKey",(void *)WaitKey);
  scAdd_External_Symbol("WaitMouseKey",(void *)WaitMouseKey);
  scAdd_External_Symbol("game",&play);
  scAdd_External_Symbol("gs_globals",&play.globalvars[0]);
  scAdd_External_Symbol("mouse",&scmouse);
  scAdd_External_Symbol("palette",&palette[0]);
  scAdd_External_Symbol("system",&scsystem);
  scAdd_External_Symbol("savegameindex",&play.filenumbers[0]);


  // Stubs for plugin functions.

  // ags_shell.dll
  scAdd_External_Symbol("ShellExecute", (void*)ScriptStub_ShellExecute);

  // ags_snowrain.dll
  scAdd_External_Symbol("srSetSnowDriftRange",(void *)srSetSnowDriftRange);
  scAdd_External_Symbol("srSetSnowDriftSpeed",(void *)srSetSnowDriftSpeed);
  scAdd_External_Symbol("srSetSnowFallSpeed",(void *)srSetSnowFallSpeed);
  scAdd_External_Symbol("srChangeSnowAmount",(void *)srChangeSnowAmount);
  scAdd_External_Symbol("srSetSnowBaseline",(void *)srSetSnowBaseline);
  scAdd_External_Symbol("srSetSnowTransparency",(void *)srSetSnowTransparency);
  scAdd_External_Symbol("srSetSnowDefaultView",(void *)srSetSnowDefaultView);
  scAdd_External_Symbol("srSetSnowWindSpeed",(void *)srSetSnowWindSpeed);
  scAdd_External_Symbol("srSetSnowAmount",(void *)srSetSnowAmount);
  scAdd_External_Symbol("srSetSnowView",(void *)srSetSnowView);
  scAdd_External_Symbol("srChangeRainAmount",(void *)srChangeRainAmount);
  scAdd_External_Symbol("srSetRainView",(void *)srSetRainView);
  scAdd_External_Symbol("srSetRainDefaultView",(void *)srSetRainDefaultView);
  scAdd_External_Symbol("srSetRainTransparency",(void *)srSetRainTransparency);
  scAdd_External_Symbol("srSetRainWindSpeed",(void *)srSetRainWindSpeed);
  scAdd_External_Symbol("srSetRainBaseline",(void *)srSetRainBaseline);
  scAdd_External_Symbol("srSetRainAmount",(void *)srSetRainAmount);
  scAdd_External_Symbol("srSetRainFallSpeed",(void *)srSetRainFallSpeed);
  scAdd_External_Symbol("srSetWindSpeed",(void *)srSetWindSpeed);
  scAdd_External_Symbol("srSetBaseline",(void *)srSetBaseline);

  // agsjoy.dll
  scAdd_External_Symbol("JoystickCount",(void *)JoystickCount);
  scAdd_External_Symbol("Joystick::Open^1",(void *)Joystick_Open);
  scAdd_External_Symbol("Joystick::IsButtonDown^1",(void *)Joystick_IsButtonDown);
  scAdd_External_Symbol("Joystick::EnableEvents^1",(void *)Joystick_EnableEvents);
  scAdd_External_Symbol("Joystick::DisableEvents^0",(void *)Joystick_DisableEvents);
  scAdd_External_Symbol("Joystick::Click^1",(void *)Joystick_Click);
  scAdd_External_Symbol("Joystick::Valid^0",(void *)Joystick_Valid);
  scAdd_External_Symbol("Joystick::Unplugged^0",(void *)Joystick_Unplugged);

  // agsblend.dll
  scAdd_External_Symbol("DrawAlpha",(void *)DrawAlpha);
  scAdd_External_Symbol("GetAlpha",(void *)GetAlpha);
  scAdd_External_Symbol("PutAlpha",(void *)PutAlpha);
  scAdd_External_Symbol("Blur",(void *)Blur);
  scAdd_External_Symbol("HighPass",(void *)HighPass);
  scAdd_External_Symbol("DrawAdd",(void *)DrawAdd);

  // agsflashlight.dll
  scAdd_External_Symbol("SetFlashlightTint",(void *)SetFlashlightInt3);
  scAdd_External_Symbol("GetFlashlightTintRed",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightTintGreen",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightTintBlue",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightMinLightLevel",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightMaxLightLevel",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightDarkness",(void *)SetFlashlightInt1);
  scAdd_External_Symbol("GetFlashlightDarkness",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightDarknessSize",(void *)SetFlashlightInt1);
  scAdd_External_Symbol("GetFlashlightDarknessSize",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightBrightness",(void *)SetFlashlightInt1);
  scAdd_External_Symbol("GetFlashlightBrightness",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightBrightnessSize",(void *)SetFlashlightInt1);
  scAdd_External_Symbol("GetFlashlightBrightnessSize",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightPosition",(void *)SetFlashlightInt2);
  scAdd_External_Symbol("GetFlashlightPositionX",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightPositionY",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightFollowMouse",(void *)SetFlashlightInt1);
  scAdd_External_Symbol("GetFlashlightFollowMouse",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightFollowCharacter",(void *)SetFlashlightInt5);
  scAdd_External_Symbol("GetFlashlightFollowCharacter",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightCharacterDX",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightCharacterDY",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightCharacterHorz",(void *)GetFlashlightInt);
  scAdd_External_Symbol("GetFlashlightCharacterVert",(void *)GetFlashlightInt);
  scAdd_External_Symbol("SetFlashlightMask",(void *)SetFlashlightInt1);
  scAdd_External_Symbol("GetFlashlightMask",(void *)GetFlashlightInt);
}


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

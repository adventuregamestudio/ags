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

#include <string.h>
#include "script/script.h"
#include "ac/common.h"
#include "ac/roomstruct.h"
#include "ac/character.h"
#include "ac/dialog.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/global_dialog.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_hotspot.h"
#include "ac/global_object.h"
#include "ac/global_room.h"
#include "ac/global_video.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "script/cc_error.h"
#include "script/cc_options.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "script/script_runtime.h"
#include "util/string_utils.h"

extern GameSetupStruct game;
extern GameState play;
extern roomstruct thisroom;
extern int gameHasBeenRestored, displayed_room;
extern unsigned int load_new_game;
extern RoomObject*objs;
extern int our_eip;
extern CharacterInfo*playerchar;

ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
ExecutingScript*curscript = NULL;

ccScript* gamescript=NULL;
ccScript* dialogScriptsScript = NULL;
ccInstance *gameinst = NULL, *roominst = NULL;
ccInstance *dialogScriptsInst = NULL;
ccInstance *gameinstFork = NULL, *roominstFork = NULL;

int num_scripts=0;
int post_script_cleanup_stack = 0;

int inside_script=0,in_graph_script=0;
int no_blocking_functions = 0; // set to 1 while in rep_Exec_always

NonBlockingScriptFunction repExecAlways(REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction getDialogOptionsDimensionsFunc("dialog_options_get_dimensions", 1);
NonBlockingScriptFunction renderDialogOptionsFunc("dialog_options_render", 1);
NonBlockingScriptFunction getDialogOptionUnderCursorFunc("dialog_options_get_active", 1);
NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc("dialog_options_mouse_click", 2);
NonBlockingScriptFunction runDialogOptionKeyPressHandlerFunc("dialog_options_key_press", 2);
NonBlockingScriptFunction runDialogOptionRepExecFunc("dialog_options_repexec", 1);

ScriptSystem scsystem;

std::vector<ccScript *> scriptModules;
std::vector<ccInstance *> moduleInst;
std::vector<ccInstance *> moduleInstFork;
std::vector<RuntimeScriptValue> moduleRepExecAddr;
int numScriptModules = 0;

char **characterScriptObjNames = NULL;
char objectScriptObjNames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN + 5];
char **guiScriptObjNames = NULL;


int run_dialog_request (int parmtr) {
    play.stop_dialog_at_end = DIALOG_RUNNING;
    gameinst->RunTextScriptIParam("dialog_request", RuntimeScriptValue().SetInt32(parmtr));

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

void run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun) {

    update_script_mouse_coords();

    int room_changes_was = play.room_changes;
    funcToRun->atLeastOneImplementationExists = false;

    // run modules
    // modules need a forkedinst for this to work
    for (int kk = 0; kk < numScriptModules; kk++) {
        funcToRun->moduleHasFunction[kk] = moduleInstFork[kk]->DoRunScriptFuncCantBlock(funcToRun, funcToRun->moduleHasFunction[kk]);

        if (room_changes_was != play.room_changes)
            return;
    }

    funcToRun->globalScriptHasFunction = gameinstFork->DoRunScriptFuncCantBlock(funcToRun, funcToRun->globalScriptHasFunction);

    if (room_changes_was != play.room_changes)
        return;

    funcToRun->roomHasFunction = roominstFork->DoRunScriptFuncCantBlock(funcToRun, funcToRun->roomHasFunction);
}

//-----------------------------------------------------------
// [IKM] 2012-06-22
//
// run_interaction_event() and run_interaction_script()
// are *almost* identical, except for the first parameter
// type.
// May these types be made children of the same base?
//-----------------------------------------------------------


// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_event (Interaction *nint, int evnt, int chkAny, int isInv) {

    if (evnt >= nint->Events.size() ||
        (nint->Events[evnt].Response.get() == NULL) || (nint->Events[evnt].Response->Cmds.size() == 0)) {
        // no response defined for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if (chkAny < 0) ;
        else if (chkAny < nint->Events.size() &&
                (nint->Events[chkAny].Response.get() != NULL) && (nint->Events[chkAny].Response->Cmds.size() > 0))
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
    retval = run_interaction_commandlist (nint->Events[evnt].Response.get(), &nint->Events[evnt].TimesRun, &cmdsrun);

    // An inventory interaction, but the wrong item was used
    if ((isInv) && (cmdsrun == 0))
        run_unhandled_event (evnt);

    return retval;
}

// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_script(InteractionScripts *nint, int evnt, int chkAny, int isInv) {

    if ((nint->ScriptFuncNames[evnt] == NULL) || (nint->ScriptFuncNames[evnt][0] == 0)) {
        // no response defined for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if (chkAny < 0) ;
        else if ((nint->ScriptFuncNames[chkAny] != NULL) && (nint->ScriptFuncNames[chkAny][0] != 0))
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

    RuntimeScriptValue rval_null;

    update_mp3();
        if ((strstr(evblockbasename,"character")!=0) || (strstr(evblockbasename,"inventory")!=0)) {
            // Character or Inventory (global script)
            QueueScriptFunction(kScInstGame, nint->ScriptFuncNames[evnt]);
        }
        else {
            // Other (room script)
            QueueScriptFunction(kScInstRoom, nint->ScriptFuncNames[evnt]);
        }
        update_mp3();

            int retval = 0;
        // if the room changed within the action
        if (room_was != play.room_changes)
            retval = -1;

        return retval;
}

int create_global_script() {
    ccSetOption(SCOPT_AUTOIMPORT, 1);
    for (int kk = 0; kk < numScriptModules; kk++) {
        moduleInst[kk] = ccInstance::CreateFromScript(scriptModules[kk]);
        if (moduleInst[kk] == NULL)
            return -3;
        // create a forked instance for rep_exec_always
        moduleInstFork[kk] = moduleInst[kk]->Fork();
        if (moduleInstFork[kk] == NULL)
            return -3;

        moduleRepExecAddr[kk] = moduleInst[kk]->GetSymbolAddress(REP_EXEC_NAME);
    }
    gameinst = ccInstance::CreateFromScript(gamescript);
    if (gameinst == NULL)
        return -3;
    // create a forked instance for rep_exec_always
    gameinstFork = gameinst->Fork();
    if (gameinstFork == NULL)
        return -3;

    if (dialogScriptsScript != NULL)
    {
        dialogScriptsInst = ccInstance::CreateFromScript(dialogScriptsScript);
        if (dialogScriptsInst == NULL)
            return -3;
    }

    ccSetOption(SCOPT_AUTOIMPORT, 0);
    return 0;
}

void cancel_all_scripts() {
    int aa;

    for (aa = 0; aa < num_scripts; aa++) {
        if (scripts[aa].forked)
            scripts[aa].inst->AbortAndDestroy();
        else
            scripts[aa].inst->Abort();
        scripts[aa].numanother = 0;
    }
    num_scripts = 0;
    /*  if (gameinst!=NULL) ->Abort(gameinst);
    if (roominst!=NULL) ->Abort(roominst);*/
}

ccInstance *GetScriptInstanceByType(ScriptInstType sc_inst)
{
    if (sc_inst == kScInstGame)
        return gameinst;
    else if (sc_inst == kScInstRoom)
        return roominst;
    return NULL;
}

void QueueScriptFunction(ScriptInstType sc_inst, const char *fn_name, size_t param_count, const RuntimeScriptValue &p1, const RuntimeScriptValue &p2)
{
    if (inside_script)
        // queue the script for the run after current script is finished
        curscript->run_another (fn_name, sc_inst, param_count, p1, p2);
    else
        // if no script is currently running, run the requested script right away
        RunScriptFunction(sc_inst, fn_name, param_count, p1, p2);
}

void RunScriptFunction(ScriptInstType sc_inst, const char *fn_name, size_t param_count, const RuntimeScriptValue &p1, const RuntimeScriptValue &p2)
{
    ccInstance *inst = GetScriptInstanceByType(sc_inst);
    if (inst)
    {
        if (param_count == 2)
            inst->RunTextScript2IParam(fn_name, p1, p2);
        else if (param_count == 1)
            inst->RunTextScriptIParam(fn_name, p1);
        else if (param_count == 0)
            inst->RunTextScript(fn_name);
    }
}

//=============================================================================


char bname[MAX_FUNCTION_NAME_LEN+1],bne[MAX_FUNCTION_NAME_LEN+1];
char* make_ts_func_name(char*base,int iii,int subd) {
    snprintf(bname,MAX_FUNCTION_NAME_LEN,base,iii);
    snprintf(bne,MAX_FUNCTION_NAME_LEN,"%s_%c",bname,subd+'a');
    return &bne[0];
}

void post_script_cleanup() {
    // should do any post-script stuff here, like go to new room
    if (ccError) quit(ccErrorString);
    ExecutingScript copyof = scripts[num_scripts-1];
    //  write_log("Instance finished.");
    if (scripts[num_scripts-1].forked)
        delete scripts[num_scripts-1].inst;
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
        QueuedScript &script = copyof.ScFnQueue[jj];
        RunScriptFunction(script.Instance, script.FnName, script.ParamCount, script.Param1, script.Param2);
        if (script.Instance == kScInstRoom && script.ParamCount == 1)
        {
            // some bogus hack for "on_call" event handler
            play.roomscript_finished = 1;
        }

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

int get_nivalue (InteractionCommandList *nic, int idx, int parm) {
    if (nic->Cmds[idx].Data[parm].Type == AGS::Common::kInterValVariable) {
        // return the value of the variable
        return get_interaction_variable(nic->Cmds[idx].Data[parm].Value)->Value;
    }
    return nic->Cmds[idx].Data[parm].Value;
}

InteractionVariable *get_interaction_variable (int varindx) {

    if ((varindx >= LOCAL_VARIABLE_OFFSET) && (varindx < LOCAL_VARIABLE_OFFSET + thisroom.numLocalVars))
        return &thisroom.localvars[varindx - LOCAL_VARIABLE_OFFSET];

    if ((varindx < 0) || (varindx >= numGlobalVars))
        quit("!invalid interaction variable specified");

    return &globalvars[varindx];
}

InteractionVariable *FindGraphicalVariable(const char *varName) {
    int ii;
    for (ii = 0; ii < numGlobalVars; ii++) {
        if (stricmp (globalvars[ii].Name, varName) == 0)
            return &globalvars[ii];
    }
    for (ii = 0; ii < thisroom.numLocalVars; ii++) {
        if (stricmp (thisroom.localvars[ii].Name, varName) == 0)
            return &thisroom.localvars[ii];
    }
    return NULL;
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
int run_interaction_commandlist (InteractionCommandList *nicl, int *timesrun, int*cmdsrun) {
    size_t i;

    if (nicl == NULL)
        return -1;

    for (i = 0; i < nicl->Cmds.size(); i++) {
        cmdsrun[0] ++;
        int room_was = play.room_changes;

        switch (nicl->Cmds[i].Type) {
      case 0:  // Do nothing
          break;
      case 1:  // Run script
          { 
              TempEip tempip(4001);
              RuntimeScriptValue rval_null;
              update_mp3();
                  if ((strstr(evblockbasename,"character")!=0) || (strstr(evblockbasename,"inventory")!=0)) {
                      // Character or Inventory (global script)
                      const char *torun = make_ts_func_name(evblockbasename,evblocknum,nicl->Cmds[i].Data[0].Value);
                      // we are already inside the mouseclick event of the script, can't nest calls
                      QueueScriptFunction(kScInstGame, torun);
                  }
                  else {
                      // Other (room script)
                      const char *torun = make_ts_func_name(evblockbasename,evblocknum,nicl->Cmds[i].Data[0].Value);
                      QueueScriptFunction(kScInstRoom, torun);
                  }
                  update_mp3();
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
              GameLoopUntilEvent(UNTIL_MOVEEND,(long)&objs[IPARAM1].moving);
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
              if (run_interaction_commandlist (nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          }
          else
              cmdsrun[0] --;
          break;
      case 21: // if player has inventory item
          if (playerchar->inv[IPARAM1] > 0)
              if (run_interaction_commandlist (nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 22: // if a character is moving
          if (game.chars[IPARAM1].walking)
              if (run_interaction_commandlist (nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 23: // if two variables are equal
          if (IPARAM1 == IPARAM2)
              if (run_interaction_commandlist (nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
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
          get_interaction_variable(nicl->Cmds[i].Data[0].Value)->Value = IPARAM2;
          break;
      case 34: // Run animation
          scAnimateCharacter(IPARAM1, IPARAM2, IPARAM3, 0);
          GameLoopUntilEvent(UNTIL_SHORTIS0,(long)&game.chars[IPARAM1].animating);
          break;
      case 35: // Quick animation
          SetCharacterView (IPARAM1, IPARAM2);
          scAnimateCharacter(IPARAM1, IPARAM3, IPARAM4, 0);
          GameLoopUntilEvent(UNTIL_SHORTIS0,(long)&game.chars[IPARAM1].animating);
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
              if (run_interaction_commandlist (nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 46: // if cursor mode is
          if (GetCursorMode() == IPARAM1)
              if (run_interaction_commandlist (nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 47: // if player has been to room
          if (HasBeenToRoom(IPARAM1))
              if (run_interaction_commandlist (nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
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

// check and abort game if the script is currently
// inside the rep_exec_always function
void can_run_delayed_command() {
  if (no_blocking_functions)
    quit("!This command cannot be used within non-blocking events such as " REP_EXEC_ALWAYS_NAME);
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

        QueueScriptFunction(kScInstGame, "unhandled_event", 2, RuntimeScriptValue().SetInt32(evtype), RuntimeScriptValue().SetInt32(evnt));
    }
}


//
// [IKM] 2012-06-22: this does not seem to be used anywhere; obsolete code?
//
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

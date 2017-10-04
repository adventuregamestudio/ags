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
#include "media/video/video.h"
#include "script/script_runtime.h"
#include "util/string_utils.h"

extern GameSetupStruct game;
extern GameState play;
extern RoomStruct thisroom;
extern int gameHasBeenRestored, displayed_room;
extern unsigned int load_new_game;
extern RoomObject*objs;
extern int our_eip;
extern CharacterInfo*playerchar;

ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
ExecutingScript*curscript = NULL;

PScript gamescript;
PScript dialogScriptsScript;
ccInstance *gameinst = NULL, *roominst = NULL;
ccInstance *dialogScriptsInst = NULL;
ccInstance *gameinstFork = NULL, *roominstFork = NULL;

int num_scripts=0;
int post_script_cleanup_stack = 0;

int inside_script=0,in_graph_script=0;
int no_blocking_functions = 0; // set to 1 while in rep_Exec_always

NonBlockingScriptFunction repExecAlways(REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction lateRepExecAlways(LATE_REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction getDialogOptionsDimensionsFunc("dialog_options_get_dimensions", 1);
NonBlockingScriptFunction renderDialogOptionsFunc("dialog_options_render", 1);
NonBlockingScriptFunction getDialogOptionUnderCursorFunc("dialog_options_get_active", 1);
NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc("dialog_options_mouse_click", 2);
NonBlockingScriptFunction runDialogOptionKeyPressHandlerFunc("dialog_options_key_press", 2);
NonBlockingScriptFunction runDialogOptionRepExecFunc("dialog_options_repexec", 1);

ScriptSystem scsystem;

std::vector<PScript> scriptModules;
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


// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_script(InteractionScripts *nint, int evnt, int chkAny, int isInv) {

    if ((nint->ScriptFuncNames[evnt] == NULL) || (nint->ScriptFuncNames[evnt][0u] == 0)) {
        // no response defined for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if (chkAny < 0) ;
        else if ((nint->ScriptFuncNames[chkAny] != NULL) && (nint->ScriptFuncNames[chkAny][0u] != 0))
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
        try_restore_save(thisData);
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

InteractionVariable *FindGraphicalVariable(const char *varName) {
    int ii;
    for (ii = 0; ii < numGlobalVars; ii++) {
        if (stricmp (globalvars[ii].Name, varName) == 0)
            return &globalvars[ii];
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

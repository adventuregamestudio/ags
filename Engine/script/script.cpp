//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdio.h>
#include <string.h>
#include "script/script.h"
#include "ac/common.h"
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
#include "script/cc_common.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "script/script_runtime.h"
#include "util/string_compat.h"
#include "media/audio/audio_system.h"

extern GameSetupStruct game;
extern GameState play;
extern int gameHasBeenRestored, displayed_room;
extern unsigned int load_new_game;
extern RoomObject*objs;
extern int our_eip;
extern CharacterInfo*playerchar;

ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
ExecutingScript *curscript = nullptr; // non-owning ptr

PScript gamescript;
PScript dialogScriptsScript;
UInstance gameinst;
UInstance roominst;
UInstance dialogScriptsInst;
UInstance gameinstFork;
UInstance roominstFork;

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
NonBlockingScriptFunction runDialogOptionKeyPressHandlerFunc("dialog_options_key_press", 3);
NonBlockingScriptFunction runDialogOptionTextInputHandlerFunc("dialog_options_text_input", 2);
NonBlockingScriptFunction runDialogOptionRepExecFunc("dialog_options_repexec", 1);
NonBlockingScriptFunction runDialogOptionCloseFunc("dialog_options_close", 1);

ScriptSystem scsystem;

std::vector<PScript> scriptModules;
std::vector<UInstance> moduleInst;
std::vector<UInstance> moduleInstFork;
std::vector<RuntimeScriptValue> moduleRepExecAddr;
size_t numScriptModules = 0;


static bool DoRunScriptFuncCantBlock(ccInstance *sci, NonBlockingScriptFunction* funcToRun, bool hasTheFunc);


int run_dialog_request (int parmtr) {
    play.stop_dialog_at_end = DIALOG_RUNNING;
    RuntimeScriptValue params[]{ parmtr };
    RunScriptFunction(gameinst.get(), "dialog_request", 1, params);

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
    for (size_t i = 0; i < numScriptModules; ++i) {
        funcToRun->moduleHasFunction[i] = DoRunScriptFuncCantBlock(moduleInstFork[i].get(), funcToRun, funcToRun->moduleHasFunction[i]);

        if (room_changes_was != play.room_changes)
            return;
    }

    funcToRun->globalScriptHasFunction = DoRunScriptFuncCantBlock(gameinstFork.get(), funcToRun, funcToRun->globalScriptHasFunction);

    if (room_changes_was != play.room_changes)
        return;

    funcToRun->roomHasFunction = DoRunScriptFuncCantBlock(roominstFork.get(), funcToRun, funcToRun->roomHasFunction);
}


// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_script(const ObjectEvent &obj_evt, InteractionScripts *nint, int evnt, int chkAny) {

    if (evnt < 0 || static_cast<size_t>(evnt) >= nint->ScriptFuncNames.size() ||
            nint->ScriptFuncNames[evnt].IsEmpty()) {
        // no response defined for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if (chkAny < 0) ;
        else if (!nint->ScriptFuncNames[chkAny].IsEmpty())
            return 0;

        // Otherwise, run unhandled_event
        run_unhandled_event(obj_evt, evnt);
        return 0;
    }

    if (play.check_interaction_only) {
        play.check_interaction_only = 2; // CHECKME: wth is "2"?
        return -1;
    }

    const int room_was = play.room_changes;

    // TODO: find a way to generalize all the following hard-coded behavior

    // Character or Inventory require a global script call
    const ScriptInstType inst_type =
        (strstr(obj_evt.BlockName.GetCStr(), "character") != nullptr) ||
        (strstr(obj_evt.BlockName.GetCStr(), "inventory") != nullptr) ?
        kScInstGame : kScInstRoom;
    
    // Room events do not require additional params
    if ((strstr(obj_evt.BlockName.GetCStr(), "room") != nullptr)) {
        QueueScriptFunction(inst_type, nint->ScriptFuncNames[evnt].GetCStr());
    }
    // Regions only require 1 param - dynobj ref
    else if ((strstr(obj_evt.BlockName.GetCStr(), "region") != nullptr)) {
        QueueScriptFunction(inst_type, nint->ScriptFuncNames[evnt].GetCStr(), 1, &obj_evt.DynObj);
    }
    // Other types (characters, objects, invitems, hotspots) require
    // 2 params - dynobj ref and the interaction mode (aka verb)
    else {
        RuntimeScriptValue params[]{ obj_evt.DynObj, RuntimeScriptValue().SetInt32(obj_evt.Mode) };
        QueueScriptFunction(inst_type, nint->ScriptFuncNames[evnt].GetCStr(), 2, params);
    }

    // if the room changed within the action
    if (room_was != play.room_changes)
        return -1;
    return 0;
}

int create_global_script() {
    constexpr int kscript_create_error = -3;

    ccSetOption(SCOPT_AUTOIMPORT, 1);

    // NOTE: this function assumes that the module lists have their elements preallocated!

    std::vector<ccInstance*> all_insts; // gather all to resolve exports below
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        auto inst = ccInstance::CreateFromScript(scriptModules[i]);
        if (!inst)
            return kscript_create_error;
        moduleInst[i].reset(inst);
        all_insts.push_back(inst);
    }

    gameinst.reset(ccInstance::CreateFromScript(gamescript));
    if (!gameinst)
        return kscript_create_error;
    all_insts.push_back(gameinst.get());

    if (dialogScriptsScript)
    {
        dialogScriptsInst.reset(ccInstance::CreateFromScript(dialogScriptsScript));
        if (!dialogScriptsInst)
            return kscript_create_error;
        all_insts.push_back(dialogScriptsInst.get());
    }

    // Resolve the script imports after all the scripts have been loaded 
    for (auto &inst : all_insts)
    {
        if (!inst->ResolveScriptImports(inst->instanceof.get()))
            return kscript_create_error;
        if (!inst->ResolveImportFixups(inst->instanceof.get()))
            return kscript_create_error;
    }

    // Create the forks for 'repeatedly_execute_always' after resolving
    // because they copy their respective originals including the resolve information
    for (size_t module_idx = 0; module_idx < numScriptModules; module_idx++)
    {
        auto fork = moduleInst[module_idx]->Fork();
        if (!fork)
            return kscript_create_error;

        moduleInstFork[module_idx].reset(fork);
        moduleRepExecAddr[module_idx] = moduleInst[module_idx]->GetSymbolAddress(REP_EXEC_NAME);
    }

    gameinstFork.reset(gameinst->Fork());
    if (gameinstFork == nullptr)
        return kscript_create_error;

    ccSetOption(SCOPT_AUTOIMPORT, 0);
    return 0;
}

void cancel_all_scripts()
{
    for (int i = 0; i < num_scripts; ++i)
    {
        auto &sc = scripts[i];
        if (sc.inst)
        {
            (sc.forkedInst) ?
                sc.inst->AbortAndDestroy() :
                sc.inst->Abort();
        }
        sc.numanother = 0;
    }
    num_scripts = 0;
    // in case the script is running on non-blocking thread (rep-exec-always etc)
    auto inst = ccInstance::GetCurrentInstance();
    if (inst)
        inst->Abort();
}

ccInstance *GetScriptInstanceByType(ScriptInstType sc_inst)
{
    if (sc_inst == kScInstGame)
        return gameinst.get();
    else if (sc_inst == kScInstRoom)
        return roominst.get();
    return nullptr;
}

void QueueScriptFunction(ScriptInstType sc_inst, const char *fn_name, size_t param_count, const RuntimeScriptValue *params)
{
    if (inside_script)
        // queue the script for the run after current script is finished
        curscript->run_another(fn_name, sc_inst, param_count, params);
    else
        // if no script is currently running, run the requested script right away
        RunScriptFunctionAuto(sc_inst, fn_name, param_count, params);
}

static bool DoRunScriptFuncCantBlock(ccInstance *sci, NonBlockingScriptFunction* funcToRun, bool hasTheFunc)
{
    if (!hasTheFunc)
        return(false);

    no_blocking_functions++;
    int result = sci->CallScriptFunction(funcToRun->functionName, funcToRun->numParameters, funcToRun->params);

    if (result == -2) {
        // the function doens't exist, so don't try and run it again
        hasTheFunc = false;
    }
    else if ((result != 0) && (result != 100)) {
        quit_with_script_error(funcToRun->functionName);
    }
    else
    {
        funcToRun->atLeastOneImplementationExists = true;
    }
    // this might be nested, so don't disrupt blocked scripts
    cc_clear_error();
    no_blocking_functions--;
    return(hasTheFunc);
}

char scfunctionname[MAX_FUNCTION_NAME_LEN + 1];
static int PrepareTextScript(ccInstance *sci, const char**tsname)
{
    cc_clear_error();
    // FIXME: try to make it so this function is not called with NULL sci
    if (sci == nullptr) return -1;
    if (sci->GetSymbolAddress(tsname[0]).IsNull()) {
        cc_error("no such function in script");
        return -2;
    }
    if (sci->IsBeingRun()) {
        cc_error("script is already in execution");
        return -3;
    }
    ExecutingScript exscript;
    // CHECKME: this conditional block will never run, because
    // function would have quit earlier (deprecated functionality?)
    if (sci->IsBeingRun()) {
        auto fork = sci->Fork();
        if (!fork)
            quit("unable to fork instance for secondary script");
        exscript.forkedInst.reset(fork);
        exscript.inst = fork;
    } else {
        exscript.inst = sci;
    }
    scripts[num_scripts] = std::move(exscript);
    curscript = &scripts[num_scripts];
    num_scripts++;
    if (num_scripts >= MAX_SCRIPT_AT_ONCE)
        quit("too many nested text script instances created");
    // in case script_run_another is the function name, take a backup
    strncpy(scfunctionname, tsname[0], MAX_FUNCTION_NAME_LEN);
    tsname[0] = &scfunctionname[0];
    update_script_mouse_coords();
    inside_script++;
    return 0;
}

int RunScriptFunction(ccInstance *sci, const char *tsname, size_t numParam, const RuntimeScriptValue *params)
{
    int oldRestoreCount = gameHasBeenRestored;
    // TODO: research why this is really necessary, and refactor to avoid such hacks!
    // First, save the current ccError state
    // This is necessary because we might be attempting
    // to run Script B, while Script A is still running in the
    // background.
    // If CallInstance here has an error, it would otherwise
    // also abort Script A because ccError is a global variable.
    ScriptError cachedCcError = cc_get_error();

    cc_clear_error();
    int toret = PrepareTextScript(sci, &tsname);
    if (toret) {
        cc_error(cachedCcError);
        return -18;
    }

    cc_clear_error();
    toret = curscript->inst->CallScriptFunction(tsname, numParam, params);

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
    cc_error(cachedCcError);

    // if the game has been restored, ensure that any further scripts are not run
    if ((oldRestoreCount != gameHasBeenRestored) && (eventClaimed == EVENT_INPROGRESS))
        eventClaimed = EVENT_CLAIMED;

    return toret;
}

void RunScriptFunctionInModules(const char *tsname, size_t param_count, const RuntimeScriptValue *params)
{
    for (size_t i = 0; i < numScriptModules; ++i)
        RunScriptFunction(moduleInst[i].get(), tsname, param_count, params);
    RunScriptFunction(gameinst.get(), tsname, param_count, params);
}

int RunScriptFunctionInRoom(const char *tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // Some room callbacks are considered to be obligatory; for historical reasons these are
    // identified by having no parameters;
    // TODO: this is a hack, this should be defined either by function type, or as an arg
    const bool strict_room_event = (param_count == 0);
    int toret = RunScriptFunction(roominst.get(), tsname, param_count, params);
    // If it's a obligatory room event, and return code means missing function - error
    if (strict_room_event && (toret == -18))
        quitprintf("RunScriptFunction: error %d (%s) trying to run '%s'   (Room %d)",
            toret, cc_get_error().ErrorString.GetCStr(), tsname, displayed_room);
    return toret;
}

// Run non-claimable event in all script modules, except room, break if certain events occured
static int RunUnclaimableEvent(const char *tsname)
{
    const int room_changes_was = play.room_changes;
    const int restore_game_count_was = gameHasBeenRestored;
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        if (!moduleRepExecAddr[i].IsNull())
            RunScriptFunction(moduleInst[i].get(), tsname);
        // Break on room change or save restoration
        if ((room_changes_was != play.room_changes) ||
            (restore_game_count_was != gameHasBeenRestored))
            return 0;
    }
    return RunScriptFunction(gameinst.get(), tsname);
}

static int RunClaimableEvent(const char *tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // Run claimable event chain in script modules and room script
    bool eventWasClaimed;
    int toret = run_claimable_event(tsname, true, param_count, params, &eventWasClaimed);
    // Break on event claim
    if (eventWasClaimed)
        return toret;
    return RunScriptFunction(gameinst.get(), tsname, param_count, params);
}

int RunScriptFunctionAuto(ScriptInstType sc_inst, const char *tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // If told to use a room instance, then run only there
    if (sc_inst == kScInstRoom)
        return RunScriptFunctionInRoom(tsname, param_count, params);
    // Rep-exec is only run in script modules, but not room script
    // (because room script has its own callback, attached to event slot)
    if (strcmp(tsname, REP_EXEC_NAME) == 0)
    {
        return RunUnclaimableEvent(REP_EXEC_NAME);
    }
    // Claimable event is run in all the script modules and room script,
    // before running in the globalscript instance
    if ((strcmp(tsname, tsnames[kTS_KeyPress]) == 0) || (strcmp(tsname, tsnames[kTS_MouseClick]) == 0) ||
        (strcmp(tsname, tsnames[kTS_TextInput]) == 0) || (strcmp(tsname, "on_event") == 0))
    {
        return RunClaimableEvent(tsname, param_count, params);
    }
    // Else run on the single chosen script instance
    ccInstance *sci = GetScriptInstanceByType(sc_inst);
    if (!sci)
        return 0;
    return RunScriptFunction(sci, tsname, param_count, params);
}

void AllocScriptModules()
{
    // NOTE: this preallocation possibly required to safeguard some algorithms
    moduleInst.resize(numScriptModules);
    moduleInstFork.resize(numScriptModules);
    moduleRepExecAddr.resize(numScriptModules);
    repExecAlways.moduleHasFunction.resize(numScriptModules, true);
    lateRepExecAlways.moduleHasFunction.resize(numScriptModules, true);
    getDialogOptionsDimensionsFunc.moduleHasFunction.resize(numScriptModules, true);
    renderDialogOptionsFunc.moduleHasFunction.resize(numScriptModules, true);
    getDialogOptionUnderCursorFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionMouseClickHandlerFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionKeyPressHandlerFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionTextInputHandlerFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionRepExecFunc.moduleHasFunction.resize(numScriptModules, true);
    runDialogOptionCloseFunc.moduleHasFunction.resize(numScriptModules, true);
    for (auto &val : moduleRepExecAddr)
    {
        val.Invalidate();
    }
}

void FreeAllScriptInstances()
{
    FreeRoomScriptInstance();

    // NOTE: don't know why, but Forks must be deleted prior to primary inst,
    // or bad things will happen; TODO: investigate and make this less fragile
    gameinstFork.reset();
    gameinst.reset();
    dialogScriptsInst.reset();
    moduleInstFork.clear();
    moduleInst.clear();
}

void FreeRoomScriptInstance()
{
    // NOTE: don't know why, but Forks must be deleted prior to primary inst,
    // or bad things will happen; TODO: investigate and make this less fragile
    roominstFork.reset();
    roominst.reset();
}

void FreeGlobalScripts()
{
    numScriptModules = 0;

    gamescript.reset();
    scriptModules.clear();
    dialogScriptsScript.reset();

    repExecAlways.moduleHasFunction.clear();
    lateRepExecAlways.moduleHasFunction.clear();
    getDialogOptionsDimensionsFunc.moduleHasFunction.clear();
    renderDialogOptionsFunc.moduleHasFunction.clear();
    getDialogOptionUnderCursorFunc.moduleHasFunction.clear();
    runDialogOptionMouseClickHandlerFunc.moduleHasFunction.clear();
    runDialogOptionKeyPressHandlerFunc.moduleHasFunction.clear();
    runDialogOptionTextInputHandlerFunc.moduleHasFunction.clear();
    runDialogOptionRepExecFunc.moduleHasFunction.clear();
    runDialogOptionCloseFunc.moduleHasFunction.clear();
}

String GetScriptName(ccInstance *sci)
{
    // TODO: have script name a ccScript's member?
    // TODO: check script modules too?
    if (!sci)
        return "Not in a script";
    else if (sci->instanceof == gamescript)
        return "Global script";
    else if (sci->instanceof == thisroom.CompiledScript)
        return String::FromFormat("Room %d script", displayed_room);
    return "Unknown script";
}

//=============================================================================


char bname[MAX_FUNCTION_NAME_LEN+1],bne[MAX_FUNCTION_NAME_LEN+1];
char* make_ts_func_name(const char*base,int iii,int subd) {
    int err = snprintf(bname,MAX_FUNCTION_NAME_LEN,base,iii);
    if (err >= sizeof(bname))
      debug_script_warn("Function name length limit exceeded: %s (%d)", base, iii);
    err = snprintf(bne,MAX_FUNCTION_NAME_LEN,"%s_%c",bname,subd+'a');
    if (err >= sizeof(bne))
      debug_script_warn("Function name length limit exceeded: %s", bname);
    return &bne[0];
}

void post_script_cleanup() {
    // should do any post-script stuff here, like go to new room
    if (cc_has_error())
        quit(cc_get_error().ErrorString);

    ExecutingScript copyof;
    if (num_scripts > 0)
    { // save until the end of function
        copyof = std::move(scripts[num_scripts - 1]);
        copyof.forkedInst.reset(); // don't need it further
        num_scripts--;
    }
    inside_script--;

    if (num_scripts > 0)
        curscript = &scripts[num_scripts-1];
    else {
        curscript = nullptr;
    }
    //  if (abort_executor) user_disabled_data2=aborted_ip;

    int old_room_number = displayed_room;

    // FIXME: sync audio in case any screen changing or time-consuming post-script actions were scheduled
    if (copyof.numPostScriptActions > 0) {
        sync_audio_playback();
    }

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


    if (copyof.numPostScriptActions > 0) {
        sync_audio_playback();
    }

    for (int jj = 0; jj < copyof.numanother; jj++) {
        old_room_number = displayed_room;
        QueuedScript &script = copyof.ScFnQueue[jj];
        RunScriptFunctionAuto(script.Instance, script.FnName.GetCStr(), script.ParamCount, script.Params);
        if (script.Instance == kScInstRoom && script.ParamCount == 1)
        {
            // some bogus hack for "on_call" event handler
            play.roomscript_finished = 1;
        }

        // if they've changed rooms, cancel any further pending scripts
        if ((displayed_room != old_room_number) || (load_new_game))
            break;
    }

}

void quit_with_script_error(const char *functionName)
{
    // TODO: clean up the error reporting logic. Now engine will append call
    // stack info in quit_check_for_error_state() but only in case of explicit
    // script error ("!" type), and not in other case.
    const auto &error = cc_get_error();
    if (error.IsUserError)
        quitprintf("!Error running function '%s':\n%s", functionName, error.ErrorString.GetCStr());
    else
        quitprintf("Error running function '%s':\n%s\n\n%s", functionName,
            error.ErrorString.GetCStr(), error.CallStack.GetCStr());
}

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

void run_unhandled_event(const ObjectEvent &obj_evt, int evnt) {

    if (play.check_interaction_only)
        return;

    const char *evblockbasename = obj_evt.BlockName.GetCStr();
    const int evblocknum = obj_evt.BlockID;
    int evtype=0;

    if (ags_strnicmp(evblockbasename,"hotspot",7)==0) evtype=1;
    else if (ags_strnicmp(evblockbasename,"object",6)==0) evtype=2;
    else if (ags_strnicmp(evblockbasename,"character",9)==0) evtype=3;
    else if (ags_strnicmp(evblockbasename,"inventory",9)==0) evtype=5;
    else if (ags_strnicmp(evblockbasename,"region",6)==0)
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
        RuntimeScriptValue params[] = { evtype, evnt };
        QueueScriptFunction(kScInstGame, "unhandled_event", 2, params);
    }
}

bool get_script_position(ScriptPosition &script_pos)
{
    ccInstance *cur_instance = ccInstance::GetCurrentInstance();
    if (cur_instance)
    {
        cur_instance->GetScriptPosition(script_pos);
        return true;
    }
    return false;
}

String cc_format_error(const String &message)
{
    if (currentline > 0)
        return String::FromFormat("Error (line %d): %s", currentline, message.GetCStr());
    else
        return String::FromFormat("Error (line unknown): %s", message.GetCStr());
}

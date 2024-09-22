//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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
extern int gameHasBeenRestored, displayed_room;
extern unsigned int load_new_game;
extern RoomObject*objs;
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
NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc("dialog_options_mouse_click", 4);
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
    funcToRun->AtLeastOneImplementationExists = false;

    // run modules
    // modules need a forkedinst for this to work
    for (size_t i = 0; i < numScriptModules; ++i) {
        funcToRun->ModuleHasFunction[i] = DoRunScriptFuncCantBlock(moduleInstFork[i].get(), funcToRun, funcToRun->ModuleHasFunction[i]);

        if (room_changes_was != play.room_changes)
            return;
    }

    funcToRun->GlobalScriptHasFunction = DoRunScriptFuncCantBlock(gameinstFork.get(), funcToRun, funcToRun->GlobalScriptHasFunction);

    if (room_changes_was != play.room_changes)
        return;

    funcToRun->RoomHasFunction = DoRunScriptFuncCantBlock(roominstFork.get(), funcToRun, funcToRun->RoomHasFunction);
}

int run_interaction_event(const ObjectEvent &obj_evt, Interaction *nint, int evnt, int chkAny, bool isInv) {

    if (evnt < 0 || (size_t)evnt >= nint->Events.size() ||
        (nint->Events[evnt].Response.get() == nullptr) || (nint->Events[evnt].Response->Cmds.size() == 0)) {
        // no response defined for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if (chkAny < 0) ;
        else if ((size_t)chkAny < nint->Events.size() &&
                (nint->Events[chkAny].Response.get() != nullptr) && (nint->Events[chkAny].Response->Cmds.size() > 0))
            return 0;

        // Otherwise, run unhandled_event
        run_unhandled_event(obj_evt, evnt);

        return 0;
    }

    if (play.check_interaction_only) {
        play.check_interaction_only = 2;
        return -1;
    }

    int cmdsrun = 0, retval = 0;
    // Right, so there were some commands defined in response to the event.
    retval = run_interaction_commandlist(obj_evt, nint->Events[evnt].Response.get(), &nint->Events[evnt].TimesRun, &cmdsrun);

    // An inventory interaction, but the wrong item was used
    if ((isInv) && (cmdsrun == 0))
        run_unhandled_event(obj_evt, evnt);

    return retval;
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

    // Which script do we call: global script or room script?
    const ScriptType sc_type = obj_evt.ScType;
    QueueScriptFunction(sc_type, nint->ScriptFuncNames[evnt].GetCStr(), obj_evt.ParamCount, obj_evt.Params);

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
        if (sc.Inst)
        {
            (sc.ForkedInst) ?
                sc.Inst->AbortAndDestroy() :
                sc.Inst->Abort();
        }
        sc = {}; // FIXME: store in vector and erase?
    }
    num_scripts = 0;
    // in case the script is running on non-blocking thread (rep-exec-always etc)
    auto inst = ccInstance::GetCurrentInstance();
    if (inst)
        inst->Abort();
}

ccInstance *GetScriptInstanceByType(ScriptType sc_type)
{
    if (sc_type == kScTypeGame)
        return gameinst.get();
    else if (sc_type == kScTypeRoom)
        return roominst.get();
    return nullptr;
}

bool DoesScriptFunctionExist(ccInstance *sci, const String &fn_name)
{
    return sci->GetSymbolAddress(fn_name).Type == kScValCodePtr;
}

bool DoesScriptFunctionExistInModules(const String &fn_name)
{
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        if (DoesScriptFunctionExist(moduleInst[i].get(), fn_name))
            return true;
    }
    return DoesScriptFunctionExist(gameinst.get(), fn_name);
}

void QueueScriptFunction(ScriptType sc_type, const String &fn_name, size_t param_count, const RuntimeScriptValue *params)
{
    if (inside_script)
        // queue the script for the run after current script is finished
        curscript->RunAnother(sc_type, fn_name, param_count, params);
    else
        // if no script is currently running, run the requested script right away
        RunScriptFunctionAuto(sc_type, fn_name, param_count, params);
}

static bool DoRunScriptFuncCantBlock(ccInstance *sci, NonBlockingScriptFunction* funcToRun, bool hasTheFunc)
{
    if (!hasTheFunc)
        return(false);

    no_blocking_functions++;
    int result = sci->CallScriptFunction(funcToRun->FunctionName, funcToRun->ParamCount, funcToRun->Params);

    if (result == -2) {
        // the function doens't exist, so don't try and run it again
        hasTheFunc = false;
    }
    else if ((result != 0) && (result != 100)) {
        quit_with_script_error(funcToRun->FunctionName);
    }
    else
    {
        funcToRun->AtLeastOneImplementationExists = true;
    }
    // this might be nested, so don't disrupt blocked scripts
    cc_clear_error();
    no_blocking_functions--;
    return(hasTheFunc);
}

static RunScFuncResult PrepareTextScript(ccInstance *sci, const String &tsname)
{
    assert(sci);
    cc_clear_error();
    if (!DoesScriptFunctionExist(sci, tsname))
    {
        cc_error("no such function in script");
        return kScFnRes_NotFound;
    }
    if (sci->IsBeingRun())
    {
        cc_error("script is already in execution");
        return kScFnRes_ScriptBusy;
    }
    ExecutingScript exscript;
    // CHECKME: this conditional block will never run, because
    // function would have quit earlier (deprecated functionality?)
    if (sci->IsBeingRun())
    {
        auto fork = sci->Fork();
        if (!fork)
            quit("unable to fork instance for secondary script");
        exscript.ForkedInst.reset(fork);
        exscript.Inst = fork;
    }
    else
    {
        exscript.Inst = sci;
    }
    scripts[num_scripts] = std::move(exscript);
    curscript = &scripts[num_scripts];
    num_scripts++;
    if (num_scripts >= MAX_SCRIPT_AT_ONCE)
        quit("too many nested text script instances created");
    update_script_mouse_coords();
    inside_script++;
    return kScFnRes_Done;
}

RunScFuncResult RunScriptFunction(ccInstance *sci, const String &tsname, size_t numParam, const RuntimeScriptValue *params)
{
    assert(sci);
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
    const RunScFuncResult res = PrepareTextScript(sci, tsname);
    if (res != kScFnRes_Done)
    {
        cc_error(cachedCcError);
        return res;
    }

    cc_clear_error();
    const ccInstError inst_ret = curscript->Inst->CallScriptFunction(tsname, numParam, params);

    if ((inst_ret != kInstErr_None) && (inst_ret != kInstErr_FuncNotFound) && (inst_ret != kInstErr_Aborted))
    {
        quit_with_script_error(tsname);
    }

    post_script_cleanup_stack++;

    if (post_script_cleanup_stack > 50)
        quitprintf("!post_script_cleanup call stack exceeded: possible recursive function call? running %s", tsname.GetCStr());

    post_script_cleanup();

    post_script_cleanup_stack--;

    // restore cached error state
    cc_error(cachedCcError);

    // if the game has been restored, ensure that any further scripts are not run
    if ((oldRestoreCount != gameHasBeenRestored) && (eventClaimed == EVENT_INPROGRESS))
        eventClaimed = EVENT_CLAIMED;

    // Convert any instance exec error into RunScriptFunction result;
    // NOTE: only kInstErr_FuncNotFound and kInstErr_Aborted can reach here
    if (inst_ret == kInstErr_FuncNotFound)
        return kScFnRes_NotFound;
    return kScFnRes_Done;
}

void RunScriptFunctionInModules(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    for (size_t i = 0; i < numScriptModules; ++i)
        RunScriptFunction(moduleInst[i].get(), tsname, param_count, params);
    RunScriptFunction(gameinst.get(), tsname, param_count, params);
}

void RunScriptFunctionInRoom(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // Some room callbacks are considered to be obligatory; for historical reasons these are
    // identified by having no parameters;
    // FIXME: this is a hack, this should be defined either by function type, or as an arg
    const bool strict_room_event = (param_count == 0);
    const RunScFuncResult res = RunScriptFunction(roominst.get(), tsname, param_count, params);
    // If it's a obligatory room event, and return code means missing function - error
    if (strict_room_event && (res != kScFnRes_Done))
        quitprintf("RunScriptFunction: error %d (%s) trying to run '%s'   (Room %d)",
            res, cc_get_error().ErrorString.GetCStr(), tsname.GetCStr(), displayed_room);
}

// Run non-claimable event in all script modules, except room, break if certain events occured
static void RunUnclaimableEvent(const String &tsname)
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
            return;
    }
    RunScriptFunction(gameinst.get(), tsname);
}

static void RunClaimableEvent(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // Run claimable event chain in script modules and room script
    bool eventWasClaimed;
    run_claimable_event(tsname, true, param_count, params, &eventWasClaimed);
    // Break on event claim
    if (eventWasClaimed)
        return;
    RunScriptFunction(gameinst.get(), tsname, param_count, params);
}

void RunScriptFunctionAuto(ScriptType sc_type, const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // If told to use a room instance, then run only there
    if (sc_type == kScTypeRoom)
    {
        RunScriptFunctionInRoom(tsname, param_count, params);
        return;
    }
    // Rep-exec is only run in script modules, but not room script
    // (because room script has its own callback, attached to event slot)
    if (strcmp(tsname.GetCStr(), REP_EXEC_NAME) == 0)
    {
        RunUnclaimableEvent(REP_EXEC_NAME);
        return;
    }
    // Claimable event is run in all the script modules and room script,
    // before running in the globalscript instance
    // FIXME: make this condition a callback parameter?
    if ((strcmp(tsname.GetCStr(), ScriptEventCb[kTS_KeyPress].FnName) == 0) || (strcmp(tsname.GetCStr(), ScriptEventCb[kTS_MouseClick].FnName) == 0) ||
        (strcmp(tsname.GetCStr(), ScriptEventCb[kTS_TextInput].FnName) == 0) || (strcmp(tsname.GetCStr(), "on_event") == 0))
    {
        RunClaimableEvent(tsname, param_count, params);
        return;
    }
    // Else run on the single chosen script instance
    ccInstance *sci = GetScriptInstanceByType(sc_type);
    if (!sci)
        return;
    RunScriptFunction(sci, tsname, param_count, params);
}

void AllocScriptModules()
{
    // NOTE: this preallocation possibly required to safeguard some algorithms
    moduleInst.resize(numScriptModules);
    moduleInstFork.resize(numScriptModules);
    moduleRepExecAddr.resize(numScriptModules);
    repExecAlways.ModuleHasFunction.resize(numScriptModules, true);
    lateRepExecAlways.ModuleHasFunction.resize(numScriptModules, true);
    getDialogOptionsDimensionsFunc.ModuleHasFunction.resize(numScriptModules, true);
    renderDialogOptionsFunc.ModuleHasFunction.resize(numScriptModules, true);
    getDialogOptionUnderCursorFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionMouseClickHandlerFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionKeyPressHandlerFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionTextInputHandlerFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionRepExecFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionCloseFunc.ModuleHasFunction.resize(numScriptModules, true);
    for (auto &val : moduleRepExecAddr)
    {
        val.Invalidate();
    }
}

void FreeAllScriptInstances()
{
    ccInstance::FreeInstanceStack();
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

    repExecAlways.ModuleHasFunction.clear();
    lateRepExecAlways.ModuleHasFunction.clear();
    getDialogOptionsDimensionsFunc.ModuleHasFunction.clear();
    renderDialogOptionsFunc.ModuleHasFunction.clear();
    getDialogOptionUnderCursorFunc.ModuleHasFunction.clear();
    runDialogOptionMouseClickHandlerFunc.ModuleHasFunction.clear();
    runDialogOptionKeyPressHandlerFunc.ModuleHasFunction.clear();
    runDialogOptionTextInputHandlerFunc.ModuleHasFunction.clear();
    runDialogOptionRepExecFunc.ModuleHasFunction.clear();
    runDialogOptionCloseFunc.ModuleHasFunction.clear();
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

String make_interact_func_name(const String &base, int param, int subd)
{
    // This will result in a name like "baseX_Y",
    // where 'X' is defined by "param" and 'Y' defined by "subd"
    String fname = String::FromFormat(base.GetCStr(), param);
    fname.AppendFmt("_%c", 'a' + subd);
    return fname;
}

void post_script_cleanup() {
    // should do any post-script stuff here, like go to new room
    if (cc_has_error())
        quit(cc_get_error().ErrorString);

    ExecutingScript copyof;
    if (num_scripts > 0)
    { // save until the end of function
        copyof = std::move(scripts[num_scripts - 1]);
        copyof.ForkedInst.reset(); // don't need it further
        num_scripts--; // FIXME: store in vector and erase?
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
    if (copyof.PostScriptActions.size() > 0) {
        sync_audio_playback();
    }

    // run the queued post-script actions
    for (const auto &act : copyof.PostScriptActions)
    {
        int thisData = act.Data;

        switch (act.Type)
        {
        case ePSANewRoom:
            // only change rooms when all scripts are done
            if (num_scripts == 0) {
                new_room(thisData, playerchar);
                // don't allow any pending room scripts from the old room
                // in run_another to be executed
                return;
            }
            else
                curscript->QueueAction(PostScriptAction(ePSANewRoom, thisData, "NewRoom"));
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
            save_game(thisData, act.Description.GetCStr(), std::move(act.Image));
            break;
        case ePSASaveGameDialog:
            save_game_dialog();
            break;
        default:
            quitprintf("undefined post script action found: %d", act.Type);
        }

        // if the room changed in a conversation, for example, abort
        if (old_room_number != displayed_room) {
            return;
        }
    }


    if (copyof.PostScriptActions.size() > 0) {
        sync_audio_playback();
    }

    for (const auto &script : copyof.ScFnQueue) {
        old_room_number = displayed_room;
        RunScriptFunctionAuto(script.ScType, script.FnName.GetCStr(), script.ParamCount, script.Params);
        if (script.ScType == kScTypeRoom && script.ParamCount == 1)
        {
            // some bogus hack for "on_call" event handler
            play.roomscript_finished = 1;
        }

        // if they've changed rooms, cancel any further pending scripts
        if ((displayed_room != old_room_number) || (load_new_game))
            break;
    }

}

void quit_with_script_error(const String &fn_name)
{
    // TODO: clean up the error reporting logic. Now engine will append call
    // stack info in quit_check_for_error_state() but only in case of explicit
    // script error ("!" type), and not in other case.
    const auto &error = cc_get_error();
    if (error.IsUserError)
        quitprintf("!Error running function '%s':\n%s", fn_name.GetCStr(), error.ErrorString.GetCStr());
    else
        quitprintf("Error running function '%s':\n%s\n\n%s", fn_name.GetCStr(),
            error.ErrorString.GetCStr(), error.CallStack.GetCStr());
}

int get_nivalue (InteractionCommandList *nic, int idx, int parm) {
    if (nic->Cmds[idx].Data[parm].Type == AGS::Common::kInterValVariable) {
        // return the value of the variable
        return get_interaction_variable(nic->Cmds[idx].Data[parm].Value)->Value;
    }
    return nic->Cmds[idx].Data[parm].Value;
}

InteractionVariable *get_interaction_variable (int varindx) {

    if ((varindx >= LOCAL_INTER_VAR_OFFSET) && ((size_t)(varindx - LOCAL_INTER_VAR_OFFSET) < thisroom.LocalVariables.size()))
        return &thisroom.LocalVariables[varindx - LOCAL_INTER_VAR_OFFSET];

    if ((varindx < 0) || (varindx >= game.numIntrVars))
        quit("!invalid interaction variable specified");

    return &game.intrVars[varindx];
}

InteractionVariable *FindGraphicalVariable(const char *varName) {
    for (int i = 0; i < game.numIntrVars; ++i) {
        if (game.intrVars[i].Name.CompareNoCase(varName) == 0)
            return &game.intrVars[i];
    }
    for (size_t i = 0; i < thisroom.LocalVariables.size(); ++i) {
        if (thisroom.LocalVariables[i].Name.CompareNoCase(varName) == 0)
            return &thisroom.LocalVariables[i];
    }
    return nullptr;
}

#define IPARAM1 get_nivalue(nicl, i, 0)
#define IPARAM2 get_nivalue(nicl, i, 1)
#define IPARAM3 get_nivalue(nicl, i, 2)
#define IPARAM4 get_nivalue(nicl, i, 3)
#define IPARAM5 get_nivalue(nicl, i, 4)

struct TempEip {
    int oldval;
    TempEip (int newval) {
        oldval = get_our_eip();
        set_our_eip(newval);
    }
    ~TempEip () { set_our_eip(oldval); }
};

// the 'cmdsrun' parameter counts how many commands are run.
// if a 'Inv Item Was Used' check does not pass, it doesn't count
// so cmdsrun remains 0 if no inventory items matched
int run_interaction_commandlist(const ObjectEvent &obj_evt, InteractionCommandList *nicl, int *timesrun, int*cmdsrun) {
    if (nicl == nullptr)
        return -1;

    const char *evblockbasename = obj_evt.BlockName.GetCStr();
    const int evblocknum = obj_evt.BlockID;
    for (size_t i = 0; i < nicl->Cmds.size(); i++) {
        cmdsrun[0] ++;
        int room_was = play.room_changes;

        switch (nicl->Cmds[i].Type) {
      case 0:  // Do nothing
          break;
      case 1:  // Run script
          { 
              TempEip tempip(4001);
              RuntimeScriptValue rval_null;
                  if ((strstr(evblockbasename,"character")!=nullptr) || (strstr(evblockbasename,"inventory")!=nullptr)) {
                      // Character or Inventory (global script)
                      const String torun = make_interact_func_name(evblockbasename,evblocknum,nicl->Cmds[i].Data[0].Value);
                      // we are already inside the mouseclick event of the script, can't nest calls
                      QueueScriptFunction(kScTypeGame, torun);
                  }
                  else {
                      // Other (room script)
                      const String torun = make_interact_func_name(evblockbasename,evblocknum,nicl->Cmds[i].Data[0].Value);
                      QueueScriptFunction(kScTypeRoom, torun);
                  }
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
          PlayFlic(IPARAM1, IPARAM2);
          break;
      case 9:  // Run Dialog
          RunDialog(IPARAM1);
          // if they changed room within the dialog script,
          // the interaction command list is no longer valid
          if (room_was != play.room_changes)
              return -1;
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
              GameLoopUntilNotMoving(&objs[IPARAM1].moving);
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
          AnimateObject4(IPARAM1, IPARAM2, IPARAM3, IPARAM4);
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
              if (run_interaction_commandlist(obj_evt, nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          }
          else
              cmdsrun[0] --;
          break;
      case 21: // if player has inventory item
          if (playerchar->inv[IPARAM1] > 0)
              if (run_interaction_commandlist(obj_evt, nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 22: // if a character is moving
          if (game.chars[IPARAM1].walking)
              if (run_interaction_commandlist(obj_evt, nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 23: // if two variables are equal
          if (IPARAM1 == IPARAM2)
              if (run_interaction_commandlist(obj_evt, nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
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
          AnimateCharacter4(IPARAM1, IPARAM2, IPARAM3, 0);
          GameLoopUntilValueIsZero(&game.chars[IPARAM1].animating);
          break;
      case 35: // Quick animation
          SetCharacterView (IPARAM1, IPARAM2);
          AnimateCharacter4(IPARAM1, IPARAM3, IPARAM4, 0);
          GameLoopUntilValueIsZero(&game.chars[IPARAM1].animating);
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
              if (run_interaction_commandlist(obj_evt, nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 46: // if cursor mode is
          if (GetCursorMode() == IPARAM1)
              if (run_interaction_commandlist(obj_evt, nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
                  return -1;
          break;
      case 47: // if player has been to room
          if (HasBeenToRoom(IPARAM1))
              if (run_interaction_commandlist(obj_evt, nicl->Cmds[i].Children.get(), timesrun, cmdsrun))
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
        QueueScriptFunction(kScTypeGame, "unhandled_event", 2, params);
    }
}

ExecutingScript *get_executingscript()
{
    return curscript;
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

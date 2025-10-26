//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_room.h"
#include "ac/global_video.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "script/cc_common.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "plugin/plugin_engine.h"
#include "script/script_runtime.h"
#include "util/memory_compat.h"
#include "util/string_compat.h"


using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int gameHasBeenRestored, displayed_room;
extern unsigned int load_new_game;
extern RoomObject*objs;
extern CharacterInfo*playerchar;
extern bool logScriptTOC;

ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
ExecutingScript *curscript = nullptr; // non-owning ptr

PRuntimeScript gamescript;
PRuntimeScript dialogScriptsScript;
PRuntimeScript roomscript;

int num_scripts = 0; // number of ExecutingScript objects recorded
int post_script_cleanup_stack = 0;
int inside_script = 0;
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

std::vector<AGS::Engine::PRuntimeScript> scriptModules;
std::vector<RuntimeScriptValue> moduleRepExecAddr;
size_t numScriptModules = 0;

std::unique_ptr<ScriptExecutor> scriptExecutor;
std::unique_ptr<ScriptThread> scriptThreadMain;
std::unique_ptr<ScriptThread> scriptThreadNonBlocking;

// Run a script function on a non-blocking script thread
static bool DoRunScriptFuncCantBlock(const RuntimeScript *script, NonBlockingScriptFunction* funcToRun, bool hasTheFunc);


void run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun) {

    update_script_mouse_coords();

    int room_changes_was = play.room_changes;
    funcToRun->AtLeastOneImplementationExists = false;

    // run modules
    for (size_t i = 0; i < numScriptModules; ++i) {
        funcToRun->ModuleHasFunction[i] = DoRunScriptFuncCantBlock(scriptModules[i].get(), funcToRun, funcToRun->ModuleHasFunction[i]);

        if (room_changes_was != play.room_changes)
            return;
    }

    funcToRun->GlobalScriptHasFunction = DoRunScriptFuncCantBlock(gamescript.get(), funcToRun, funcToRun->GlobalScriptHasFunction);

    if (room_changes_was != play.room_changes)
        return;

    funcToRun->RoomHasFunction = DoRunScriptFuncCantBlock(roomscript.get(), funcToRun, funcToRun->RoomHasFunction);
}

// Reports a warning in case a requested event handler function was not run
// TODO: find a way to pass event's name too, but they are not clearly defined as strings inside the engine atm...
static void WarnEventFunctionNotFound(const ScriptFunctionRef &fn_ref, bool in_room)
{
    String message = String::FromFormat("WARNING: event script function '%s' not found",
                                        fn_ref.FuncName.GetCStr());
    if (in_room)
        message.AppendFmt(" (Room %d)", displayed_room);
    else
        message.AppendFmt(" (%s)", fn_ref.ModuleName.IsEmpty() ? "Global Script" : fn_ref.ModuleName.GetCStr());
    debug_script_warn("%s", message.GetCStr());
}

// Tests if the interaction handler is referencing an existing function in script.
// Writes down the result of the test for the future use.
static bool test_interaction_handler(ScriptEventsBase *handlers, int evnt, bool in_room)
{
    if (!handlers->HasHandler(evnt))
        return false;
    auto &handler = handlers->GetHandler(static_cast<size_t>(evnt));
    if (!handler.IsChecked())
    {
        if (in_room)
            handler.SetChecked(DoesScriptFunctionExist(roomscript.get(), handler.FunctionName));
        else
            handler.SetChecked(DoesScriptFunctionExistInModule(handlers->GetScriptModule(), handler.FunctionName));

        if (!handler.IsEnabled())
            WarnEventFunctionNotFound(ScriptFunctionRef(handlers->GetScriptModule(), handler.FunctionName), in_room);
    }
    return handler.IsEnabled();
}

// Runs new-style interaction event handler in script
int run_event_script(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int evnt,
                     ScriptEventsBase *chkany_handlers, int any_evt, bool do_unhandled_event)
{
    assert(handlers);
    if (!handlers)
        return 0;

    if (!test_interaction_handler(handlers, evnt, obj_evt.ScType == kScTypeRoom))
    {
        // No response enabled for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if ((chkany_handlers != nullptr) && test_interaction_handler(chkany_handlers, any_evt, obj_evt.ScType == kScTypeRoom))
            return 0;

        // Optionally, run unhandled_event
        if (do_unhandled_event)
            run_unhandled_event(obj_evt, evnt);
        return 0;
    }

    // FIXME: refactor this nonsense! check_interaction_only here is used as
    // both function argument and return value!
    if (play.check_interaction_only) {
        play.check_interaction_only = 2; // CHECKME: wth is "2"?
        return -1;
    }

    const int room_was = play.room_changes;

    QueueScriptFunction(obj_evt.ScType, ScriptFunctionRef(handlers->GetScriptModule(), handlers->GetHandler(evnt).FunctionName),
        obj_evt.ParamCount, obj_evt.Params);

    // if the room changed within the action
    if (room_was != play.room_changes)
        return -1;
    return 0;
}

int run_event_script(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int evnt, bool do_unhandled_event)
{
    return run_event_script(obj_evt, handlers, evnt, nullptr, -1, do_unhandled_event);
}

void SetupBuiltinTypeAliases()
{
    // Add aliases for the location "ags":
    // this is a reserved name for the engine API types
    RuntimeScript::AddGlobalTypeAliases("ags");
    // Add aliases for the locations corresponding to all the registered plugins
    std::vector<String> pl_names;
    pl_get_plugin_names(pl_names);
    for (const auto &name : pl_names)
        RuntimeScript::AddGlobalTypeAliases(name);
}

bool LinkGlobalScripts()
{
    ccSetOption(SCOPT_AUTOIMPORT, 1);

    // NOTE: this function assumes that the module lists have their elements preallocated!

    std::vector<RuntimeScript*> all_insts; // gather all to resolve exports below
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        all_insts.push_back(scriptModules[i].get());
    }

    all_insts.push_back(gamescript.get());

    if (dialogScriptsScript)
    {
        all_insts.push_back(dialogScriptsScript.get());
    }

    //
    // Script linking stage
    // Register all the script exports
    for (auto &inst : all_insts)
    {
        inst->RegisterExports(simp);
    }
    // Resolve all the script imports (these include both engine API and script exports)
    for (auto &inst : all_insts)
    {
        if (!inst->ResolveImports(simp))
            return false;
    }

    // Record addresses for 'repeatedly_execute'
    // TODO: find out why do we have to do that here
    for (size_t module_idx = 0; module_idx < numScriptModules; module_idx++)
    {
        moduleRepExecAddr[module_idx] = scriptModules[module_idx]->GetSymbolAddress(REP_EXEC_NAME);
    }

    ccSetOption(SCOPT_AUTOIMPORT, 0);

    // Register built-in types under simple aliases for dynamic cast feature
    // TODO: maybe find a better place to do this? should be done after JointRTTI is created though
    SetupBuiltinTypeAliases();

    // Optionally dump script's TOC into the log
    if (logScriptTOC)
    {
        for (const auto &inst : all_insts)
        {
            if (inst->GetTOC())
                Debug::Printf(PrintScriptTOC(*inst->GetTOC(), inst->GetScriptName().GetCStr()));
        }
    }

    return true;
}

void AbortAllScripts()
{
    scriptExecutor->Abort();
    num_scripts = 0;
}

RuntimeScript *GetScriptInstanceByType(ScriptType sc_type)
{
    if (sc_type == kScTypeGame)
        return gamescript.get();
    else if (sc_type == kScTypeRoom)
        return roomscript.get();
    return nullptr;
}

bool DoesScriptFunctionExist(const RuntimeScript *script, const String &fn_name)
{
    return script->GetSymbolAddress(fn_name).Type == kScValCodePtr;
}

bool DoesScriptFunctionExistInModules(const String &fn_name)
{
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        if (DoesScriptFunctionExist(scriptModules[i].get(), fn_name))
            return true;
    }
    return DoesScriptFunctionExist(gamescript.get(), fn_name);
}

bool DoesScriptFunctionExistInModule(const String &script_module, const String &fn_name)
{
    if (script_module.IsEmpty() || script_module == gamescript->GetScriptName())
        return DoesScriptFunctionExist(gamescript.get(), fn_name);

    for (size_t i = 0; i < numScriptModules; ++i)
    {
        if (script_module == scriptModules[i]->GetScriptName())
            return DoesScriptFunctionExist(scriptModules[i].get(), fn_name);
    }
    return false;
}

void QueueScriptFunction(ScriptType sc_type, const String &fn_name,
    size_t param_count, const RuntimeScriptValue *params)
{
    QueueScriptFunction(sc_type, ScriptFunctionRef(fn_name), param_count, params);
}

void QueueScriptFunction(ScriptType sc_type, const String &script_module, const ScriptEventHandler &handler,
                            size_t param_count, const RuntimeScriptValue *params)
{
    if (handler.IsEnabled())
        QueueScriptFunction(sc_type, ScriptFunctionRef(script_module, handler.FunctionName), param_count, params);
}

void QueueScriptFunction(ScriptType sc_type, const ScriptFunctionRef &fn_ref,
    size_t param_count, const RuntimeScriptValue *params)
{
    if (inside_script)
    {
        // queue the script for the run after current script is finished
        curscript->RunAnother(sc_type, fn_ref, param_count, params);
    }
    else
    {
        // if no script is currently running, run the requested script right away
        RunScriptFunctionAuto(sc_type, fn_ref, param_count, params);
    }
}

// Run a script function on a non-blocking script thread
static bool DoRunScriptFuncCantBlock(const RuntimeScript *script, NonBlockingScriptFunction* funcToRun, bool hasTheFunc)
{
    if (!hasTheFunc)
        return(false);

    no_blocking_functions++;
    const ScriptExecError result = scriptExecutor->Run(scriptThreadNonBlocking.get(),
        script, funcToRun->FunctionName, funcToRun->Params, funcToRun->ParamCount);

    if (result == kScExecErr_FuncNotFound)
    {
        // the function doens't exist, so don't try and run it again
        hasTheFunc = false;
    }
    else if ((result != kScExecErr_None) && (result != kScExecErr_Aborted))
    {
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

static RunScFuncResult PrepareTextScript(const RuntimeScript *script, const String &tsname)
{
    assert(script);
    cc_clear_error();
    if (!DoesScriptFunctionExist(script, tsname))
    {
        cc_error("no such function in script");
        return kScFnRes_NotFound;
    }
    // TODO: should be IsBusy instead?
    // need to figure out and possible adjust the script running rules
    if (scriptExecutor->IsRunning())
    {
        cc_error("script is already in execution");
        return kScFnRes_ScriptBusy;
    }
    ExecutingScript exscript;
    exscript.Script = script;
    scripts[num_scripts] = std::move(exscript);
    curscript = &scripts[num_scripts];
    num_scripts++;
    if (num_scripts >= MAX_SCRIPT_AT_ONCE)
        quit("too many nested text script instances created");
    update_script_mouse_coords();
    inside_script++;
    return kScFnRes_Done;
}

static void PostScriptProcessing(const String &tsname)
{
    post_script_cleanup_stack++;

    if (post_script_cleanup_stack > 50)
        quitprintf("!post_script_cleanup call stack exceeded: possible recursive function call? running %s", tsname.GetCStr());

    post_script_cleanup();

    post_script_cleanup_stack--;
}

RunScFuncResult RunScriptFunction(const RuntimeScript *script, const String &tsname, size_t numParam, const RuntimeScriptValue *params)
{
    assert(script);
    int oldRestoreCount = gameHasBeenRestored;
    // TODO: research why this is really necessary, and refactor to avoid such hacks!
    // First, save the current ccError state
    // This is necessary because we might be attempting
    // to run Script B, while Script A is still running in the
    // background.
    // If CallInstance here has an error, it would otherwise
    // also abort Script A because ccError is a global variable.
    ScriptError cachedCcError = cc_get_error();

    const RunScFuncResult res = PrepareTextScript(script, tsname);
    if (res != kScFnRes_Done)
    {
        if (res != kScFnRes_NotFound)
            quit_with_script_error(tsname);
        cc_error(cachedCcError); // restore cached error state
        return res;
    }

    const ScriptExecError inst_ret = scriptExecutor->Run(scriptThreadMain.get(),
        curscript->Script, tsname, params, numParam);
    if ((inst_ret != kScExecErr_None) && (inst_ret != kScExecErr_FuncNotFound) && (inst_ret != kScExecErr_Aborted))
    {
        quit_with_script_error(tsname);
    }

    // FIXME: here we save the return value of the current called function,
    // because anything scheduled for the post-script processing will be run by the same executor,
    // and on the same thread, and may replace "return value" field saved in a executor.
    // That's ugly. Should revise and redesign this later. BTW, ScriptExecutor keeping
    // last return value is also a hack, used currently only by dialogs. This should be done differently.
    const int ret_value = scriptExecutor->GetReturnValue();
    if (!scriptThreadMain->IsBusy())
    {
        PostScriptProcessing(tsname);
    }
    scriptExecutor->SetReturnValue(ret_value); // restore saved ret value

    // restore cached error state
    cc_error(cachedCcError);

    // if the game has been restored, ensure that any further scripts are not run
    if ((oldRestoreCount != gameHasBeenRestored) && (eventClaimed == EVENT_INPROGRESS))
        eventClaimed = EVENT_CLAIMED;

    // Convert any instance exec error into RunScriptFunction result;
    // NOTE: only kScExecErr_FuncNotFound and kScExecErr_Aborted can reach here
    if (inst_ret == kScExecErr_FuncNotFound)
        return kScFnRes_NotFound;
    return kScFnRes_Done;
}

bool RunScriptFunctionInModules(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    bool result = false;
    for (size_t i = 0; i < numScriptModules; ++i)
        result |= RunScriptFunction(scriptModules[i].get(), tsname, param_count, params) == kScFnRes_Done;
    result |= RunScriptFunction(gamescript.get(), tsname, param_count, params) == kScFnRes_Done;
    return result;
}

bool RunScriptFunctionInRoom(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    if (!roomscript)
        return false; // room is not loaded yet

    return RunScriptFunction(roomscript.get(), tsname, param_count, params) == kScFnRes_Done;
}

// Run non-claimable event in all script modules, *excluding* room;
// break if certain changes occured to the game state
static bool RunEventInModules(const String &tsname, size_t param_count, const RuntimeScriptValue *params,
    bool break_after_first)
{
    const int room_changes_was = play.room_changes;
    const int restore_game_count_was = gameHasBeenRestored;
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        const RunScFuncResult ret = RunScriptFunction(scriptModules[i].get(), tsname, param_count, params);
        if (ret != kScFnRes_NotFound)
        {
            // Break on room change or save restoration,
            // or if was told to break after the first found callback,
            // or if script execution error occured
            if ((ret != kScFnRes_Done) ||
                break_after_first ||
                (room_changes_was != play.room_changes) ||
                (restore_game_count_was != gameHasBeenRestored))
                return ret == kScFnRes_Done; // return if succeeded to run
        }
    }
    // Try global script last
    return RunScriptFunction(gamescript.get(), tsname, param_count, params) == kScFnRes_Done;
}

// Run non-claimable event in all script modules, *excluding* room;
// break if certain changes occured to the game state
static bool RunUnclaimableEvent(const String &tsname)
{
    return RunEventInModules(tsname, 0, nullptr, false);
}

// Run a single event callback, look for it in all script modules, *excluding* room;
// break after the first run callback, or in case of certain changes to the game state
static bool RunSingleEvent(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    return RunEventInModules(tsname, param_count, params, true);
}

// Run a single event callback in the specified script module;
// if the name is not provided, then tries to run it in global script.
static bool RunEventInModule(const ScriptFunctionRef &fn_ref, size_t param_count, const RuntimeScriptValue *params)
{
    if (!fn_ref.ModuleName.IsEmpty())
    {
        for (size_t i = 0; i < numScriptModules; ++i)
        {
            if (fn_ref.ModuleName.Compare(scriptModules[i]->GetScriptName()) == 0)
            {
                return RunScriptFunction(scriptModules[i].get(), fn_ref.FuncName, param_count, params) == kScFnRes_Done;
            }
        }
    }
    // Try global script last, for backwards compatibility
    return RunScriptFunction(gamescript.get(), fn_ref.FuncName, param_count, params) == kScFnRes_Done;
}

// Run claimable event in all script modules, *including* room;
// break if event was claimed by any of the run callbacks.
// CHECKME: should not this also break on room change / save restore, like RunUnclaimableEvent?
static bool RunClaimableEvent(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // Run claimable event chain in script modules and room script
    bool eventWasClaimed;
    run_claimable_event(tsname, true, param_count, params, &eventWasClaimed);
    // Break on event claim
    if (eventWasClaimed)
        return true; // suppose if claimed then some function ran successfully
    return RunScriptFunction(gamescript.get(), tsname, param_count, params) == kScFnRes_Done;
}

bool RunScriptFunctionAuto(ScriptType sc_type, const ScriptFunctionRef &fn_ref, size_t param_count, const RuntimeScriptValue *params)
{
    // If told to use a room instance, then run only there
    if (sc_type == kScTypeRoom)
    {
        return RunScriptFunctionInRoom(fn_ref.FuncName, param_count, params);
    }

    // TODO: optimize this function name mapping to propagation type
    const String &fn_name = fn_ref.FuncName;
    auto evt_cb = std::find_if(ScriptEventCb.begin(), ScriptEventCb.end(),
        [fn_name](const ScriptEventCallback &cb) { return cb.FnName == fn_name; });
    ScriptCallbackPropagation propagate = evt_cb != ScriptEventCb.end() ? evt_cb->CbPropagate : kScCallback_Direct;

    // Unclaimable event means it is run in all modules except room,
    // and cannot be stopped from propagating
    if (propagate == kScCallback_CommonUnClaimable)
    {
        return RunUnclaimableEvent(REP_EXEC_NAME);
    }
    // Claimable event is run in all the script modules and room script,
    // before running in the globalscript instance
    if (propagate == kScCallback_CommonClaimable)
    {
        return RunClaimableEvent(fn_name, param_count, params);
    }

    // Else run this event in script modules (except room) according to the function ref
    return RunEventInModule(fn_ref, param_count, params);
}

void InitScriptExec()
{
    scriptExecutor = std::make_unique<ScriptExecutor>();
    scriptThreadMain = std::make_unique<ScriptThread>("Main");
    scriptThreadNonBlocking = std::make_unique<ScriptThread>("Non-Blocking");

    ccSetScriptAliveTimer(1000 / 60u, 1000u, 150000u);
}

void ShutdownScriptExec()
{
    scriptExecutor = {};
    scriptThreadMain = {};
    scriptThreadNonBlocking = {};
}

void AllocScriptModules()
{
    // NOTE: this preallocation possibly required to safeguard some algorithms
    scriptModules.resize(numScriptModules);
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

void UnlinkAllScripts()
{
    if (roomscript)
        roomscript->UnRegisterExports(simp);
    if (gamescript)
        gamescript->UnRegisterExports(simp);
    if (dialogScriptsScript)
        dialogScriptsScript->UnRegisterExports(simp);
    for (auto &module : scriptModules)
        module->UnRegisterExports(simp);
}

void FreeRoomScript()
{
    if (roomscript)
        roomscript->UnRegisterExports(simp);
    roomscript = nullptr;
}

void FreeAllScripts()
{
    UnlinkAllScripts();

    roomscript = nullptr;
    gamescript = nullptr;
    dialogScriptsScript = nullptr;
    scriptModules.clear();
    numScriptModules = 0;

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

//=============================================================================

String make_interact_func_name(const String &base, int param, int subd)
{
    // This will result in a name like "baseX_Y",
    // where 'X' is defined by "param" and 'Y' defined by "subd"
    String fname = String::FromFormat(base.GetCStr(), param);
    fname.AppendFmt("_%c", 'a' + subd);
    return fname;
}

void post_script_cleanup()
{
    // should do any post-script stuff here, like go to new room
    if (cc_has_error())
        quit(cc_get_error().ErrorString);

    ExecutingScript copyof;
    if (num_scripts > 0)
    { // save until the end of function
        copyof = std::move(scripts[num_scripts - 1]);
        num_scripts--; // FIXME: store in vector and erase?
    }
    inside_script--;

    if (num_scripts > 0)
        curscript = &scripts[num_scripts-1];
    else {
        curscript = nullptr;
    }

    int old_room_number = displayed_room;

    // FIXME: sync audio in case any screen changing or time-consuming post-script actions were scheduled
    if (copyof.PostScriptActions.size() > 0) {
        sync_audio_playback();
    }

    // run the queued post-script actions
    for (const auto &act : copyof.PostScriptActions)
    {
        const int data1 = act.Data[0];
        const int data2 = act.Data[1];

        switch (act.Type)
        {
        case ePSANewRoom:
            // only change rooms when all scripts are done
            if (num_scripts == 0) {
                new_room(data1, playerchar);
                // don't allow any pending room scripts from the old room
                // in run_another to be executed
                return;
            }
            else
                curscript->QueueAction(PostScriptAction(ePSANewRoom, data1, "NewRoom"));
            break;
        case ePSARestoreGame:
            AbortAllScripts();
            try_restore_save(data1);
            return;
        case ePSARestoreGameDialog:
            do_restore_game_dialog(data1 & 0xFFFF, (data1 >> 16), data2);
            return;
        case ePSARunAGSGame:
            AbortAllScripts();
            load_new_game = data1;
            return;
        case ePSARunDialog:
            if (is_in_dialog())
            {
                set_dialog_result_goto(data1, data2);
            }
            else
            {
                do_conversation(data1, data2);
            }
            break;
        case ePSAStopDialog:
            if (is_dialog_executing_script())
            {
                set_dialog_result_stop();
            }
            else if (is_in_dialogoptions())
            {
                schedule_dialog_stop();
            }
            break;
        case ePSARestartGame:
            AbortAllScripts();
            restart_game();
            return;
        case ePSASaveGame:
            save_game(data1, act.Text, std::move(act.Image));
            break;
        case ePSASaveGameDialog:
            do_save_game_dialog(data1 & 0xFFFF, (data1 >> 16), data2);
            break;
        case ePSAScanSaves:
            prescan_save_slots(act.Data[0], act.Data[1], act.Data[2], act.Data[3], act.Data[4], act.Data[5]);
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

    for (const auto &script : copyof.ScFnQueue)
    {
        old_room_number = displayed_room;

        RunScriptFunctionAuto(script.ScType, script.Function, script.ParamCount, script.Params);

        // FIXME: this is some bogus hack for "on_call" event handler
        // don't use instance + param count, instead find a way to save actual callback name!
        if (script.ScType == kScTypeRoom && script.ParamCount == 1)
        {
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

struct TempEip {
    int oldval;
    TempEip (int newval) {
        oldval = get_our_eip();
        set_our_eip(newval);
    }
    ~TempEip () { set_our_eip(oldval); }
};

void can_run_delayed_command() {
  if (no_blocking_functions)
    quit("!This command cannot be used within non-blocking events such as " REP_EXEC_ALWAYS_NAME);
}

bool get_can_run_delayed_command()
{
    return no_blocking_functions == 0;
}

void run_unhandled_event(const ObjectEvent &obj_evt, int evnt)
{
    // FIXME: do not use a global variable here, pass as a function argument!
    if (play.check_interaction_only)
        return;

    // No "unhandled event" for walk mode
    if (evnt == MODE_WALK)
        return;

    int loc_type = obj_evt.ObjectTypeID;
    int obj_id = obj_evt.ObjectID;

    // Special case for Hotspot 0: change to "no location"
    if (loc_type == LOCTYPE_HOTSPOT && obj_id == 0)
        loc_type = LOCTYPE_NOTHING;

    can_run_delayed_command();
    RuntimeScriptValue params[] = { loc_type, evnt };
    QueueScriptFunction(kScTypeGame, "unhandled_event", 2, params);
}

ExecutingScript *get_executingscript()
{
    return curscript;
}

bool get_script_position(ScriptPosition &script_pos)
{
    if (scriptExecutor)
    {
        scriptExecutor->GetScriptPosition(script_pos);
        return true;
    }
    return false;
}

String cc_get_callstack(int max_lines)
{
    if (scriptExecutor && max_lines > 0)
        return scriptExecutor->FormatCallStack(max_lines);
    return {};
}

String cc_format_error(const String &message)
{
    if (currentline > 0)
        return String::FromFormat("Error (line %d): %s", currentline, message.GetCStr());
    else
        return String::FromFormat("Error (line unknown): %s", message.GetCStr());
}

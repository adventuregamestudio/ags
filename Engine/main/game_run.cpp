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

//
// Game loop
//

#include <limits>
#include <chrono>
#include <SDL.h>
#include "ac/button.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_debug.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_region.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/overlay.h"
#include "ac/spritecache.h"
#include "ac/sys_events.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "ac/walkbehind.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "gui/animatingguibutton.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin_evts.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/joystick.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern int mouse_on_iface;   // mouse cursor is over this interface
extern int ifacepopped;
extern volatile bool want_exit, abort_engine;
extern int proper_exit,our_eip;
extern int displayed_room, starting_room, in_new_room, new_room_was;
extern ScriptSystem scsystem;
extern GameSetupStruct game;
extern RoomStruct thisroom;
extern int game_paused;
extern int getloctype_index;
extern int in_enters_screen,done_es_error;
extern int in_leaves_screen;
extern int inside_script,in_graph_script;
extern int no_blocking_functions;
extern CharacterInfo*playerchar;
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern int cur_mode;
extern RoomObject*objs;
extern RoomStatus*croom;
extern SpriteCache spriteset;
extern int cur_mode,cur_cursor;
extern char check_dynamic_sprites_at_exit;

// Checks if user interface should remain disabled for now
static bool ShouldStayInWaitMode();

float fps = std::numeric_limits<float>::quiet_NaN();
static auto t1 = AGS_Clock::now();  // timer for FPS // ... 't1'... how very appropriate.. :)
unsigned int loopcounter=0;
static unsigned int lastcounter=0;

#define UNTIL_ANIMEND   1
#define UNTIL_MOVEEND   2
#define UNTIL_CHARIS0   3
#define UNTIL_NOOVERLAY 4
#define UNTIL_NEGATIVE  5
#define UNTIL_INTIS0    6
#define UNTIL_SHORTIS0  7
#define UNTIL_INTISNEG  8
#define UNTIL_ANIMBTNEND 9

// Following struct instructs the engine to run game loops until
// certain condition is not fullfilled.
struct RestrictUntil
{
    int type = 0; // type of condition, UNTIL_* constant
    int disabled_for = 0; // FOR_* constant
    // pointer to the test variable
    const void *data_ptr = nullptr;
    // other values used for a test, depend on type
    int data1 = 0;
    int data2 = 0;
} restrict_until;

static size_t numEventsAtStartOfFunction;

static void ProperExit()
{
    want_exit = false;
    proper_exit = 1;
    quit("||exit!");
}

static void game_loop_check_problems_at_start()
{
    if ((in_enters_screen != 0) & (displayed_room == starting_room))
        quit("!A text script run in the Player Enters Screen event caused the screen to be updated. If you need to use Wait(), do so in After Fadein");
    if ((in_enters_screen != 0) && (done_es_error == 0)) {
        debug_script_warn("Wait() was used in Player Enters Screen - use Enters Screen After Fadein instead");
        done_es_error = 1;
    }
    if (no_blocking_functions)
        quit("!A blocking function was called from within a non-blocking event such as " REP_EXEC_ALWAYS_NAME);
}

// Runs rep-exec
static void game_loop_do_early_script_update()
{
    if (in_new_room == 0) {
        // Run the room and game script repeatedly_execute
        run_function_on_non_blocking_thread(&repExecAlways);
        setevent(EV_TEXTSCRIPT, kTS_Repeat);
        setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, EVROM_REPEXEC);
    }
}

// Runs late-rep-exec
static void game_loop_do_late_script_update()
{
    if (in_new_room == 0)
    {
        // Run the room and game script late_repeatedly_execute
        run_function_on_non_blocking_thread(&lateRepExecAlways);
    }
}

static int game_loop_check_ground_level_interactions()
{
    if ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0) {
        // check if he's standing on a hotspot
        int hotspotThere = get_hotspot_at(playerchar->x, playerchar->y);
        // run Stands on Hotspot event
        setevent(EV_RUNEVBLOCK, EVB_HOTSPOT, hotspotThere, EVHOT_STANDSON);

        // check current region
        int onRegion = GetRegionIDAtRoom(playerchar->x, playerchar->y);
        int inRoom = displayed_room;

        if (onRegion != play.player_on_region) {
            // we need to save this and set play.player_on_region
            // now, so it's correct going into RunRegionInteraction
            int oldRegion = play.player_on_region;

            play.player_on_region = onRegion;
            // Walks Off last region
            if (oldRegion > 0)
                RunRegionInteraction (oldRegion, 2);
            // Walks Onto new region
            if (onRegion > 0)
                RunRegionInteraction (onRegion, 1);
        }
        if (play.player_on_region > 0)   // player stands on region
            RunRegionInteraction (play.player_on_region, 0);

        // one of the region interactions sent us to another room
        if (inRoom != displayed_room) {
            check_new_room();
        }

        // if in a Wait loop which is no longer valid (probably
        // because the Region interaction did a NewRoom), abort
        // the rest of the loop
        if ((restrict_until.type > 0) && (!ShouldStayInWaitMode())) {
            // cancel the Rep Exec and Stands on Hotspot events that
            // we just added -- otherwise the event queue gets huge
            events.resize(numEventsAtStartOfFunction);
            return 0;
        }
    } // end if checking ground level interactions

    return RETURN_CONTINUE;
}

static void lock_mouse_on_click()
{
    if (usetup.mouse_auto_lock && scsystem.windowed)
        Mouse::TryLockToWindow();
}

static void toggle_mouse_lock()
{
    if (scsystem.windowed)
    {
        if (Mouse::IsLockedToWindow())
            Mouse::UnlockFromWindow();
        else
            Mouse::TryLockToWindow();
    }
}

bool run_service_mb_controls(eAGSMouseButton &out_mbut)
{
    out_mbut = kMouseNone; // clear the output
    if (ags_inputevent_ready() != kInputMouse)
        return false; // there was no mouse event

    const SDL_Event mb_evt = ags_get_next_inputevent();
    if (mb_evt.type == SDL_MOUSEBUTTONDOWN)
    {
        out_mbut = sdl_mbut_to_ags_but(mb_evt.button.button);
        lock_mouse_on_click();
    }
    return out_mbut != kMouseNone;
}

static eAGSMouseButton wasbutdown = kMouseNone;
static int wasongui = 0;

// Runs default handling of mouse movement, button state, and wheel
static void check_mouse_state(int &was_mouse_on_iface)
{
    mouse_on_iface = gui_on_mouse_move();
    was_mouse_on_iface = mouse_on_iface;

    if ((ifacepopped>=0) && (mousey>=guis[ifacepopped].Y+guis[ifacepopped].Height))
        remove_popup_interface(ifacepopped);

    // check mouse clicks on GUIs
    if ((wasbutdown > kMouseNone) && (ags_misbuttondown(wasbutdown))) {
        gui_on_mouse_hold(wasongui, wasbutdown);
    }
    else if ((wasbutdown > kMouseNone) && (!ags_misbuttondown(wasbutdown))) {
        gui_on_mouse_up(wasongui, wasbutdown);
        wasbutdown = kMouseNone;
    }

    int mwheelz = ags_check_mouse_wheel();
    if (mwheelz < 0)
        setevent (EV_TEXTSCRIPT, kTS_MouseClick, 9);
    else if (mwheelz > 0)
        setevent (EV_TEXTSCRIPT, kTS_MouseClick, 8);
}

// Runs default mouse button handling
static void check_mouse_controls(const int was_mouse_on_iface)
{
    eAGSMouseButton mbut;
    if (run_service_mb_controls(mbut) && mbut > kMouseNone) {
        check_skip_cutscene_mclick(mbut);

        if (play.fast_forward || play.IsIgnoringInput()) { /* do nothing if skipping cutscene or input disabled */ }
        else if ((play.wait_counter != 0) && (play.key_skip_wait & SKIP_MOUSECLICK) != 0) {
            play.SetWaitSkipResult(SKIP_MOUSECLICK, mbut);
        }
        else if (play.text_overlay_on > 0) {
            if (play.cant_skip_speech & SKIP_MOUSECLICK)
            {
                remove_screen_overlay(play.text_overlay_on);
                play.SetWaitSkipResult(SKIP_MOUSECLICK, mbut);
            }
        }
        else if (!IsInterfaceEnabled()) ;  // blocking cutscene, ignore mouse
        else if (pl_run_plugin_hooks(AGSE_MOUSECLICK, mbut)) {
            // plugin took the click
            debug_script_log("Plugin handled mouse button %d", mbut);
        }
        else if (was_mouse_on_iface >= 0) {
            if (wasbutdown == kMouseNone) {
                gui_on_mouse_down(was_mouse_on_iface, mbut);
            }
            wasongui = was_mouse_on_iface;
            wasbutdown = mbut;
        }
        else setevent(EV_TEXTSCRIPT, kTS_MouseClick, mbut);
    }
}

// Runs service key controls, returns false if service key combinations were handled
// and no more processing required, otherwise returns true and provides current keycode and key shifts.
//
// * old_keyhandle mode is a backward compatible input handling mode, where
//   - lone mod keys are not passed further into the engine;
//   - key + mod combos are merged into one key code for the script callback.
bool run_service_key_controls(KeyInput &out_key)
{
    out_key = KeyInput(); // clear the output
    if (ags_inputevent_ready() != kInputKeyboard)
        return false; // there was no key event

    const bool old_keyhandle = (game.options[OPT_KEYHANDLEAPI] == 0);
    const SDL_Event key_evt = ags_get_next_inputevent();
    bool handled = false;

    // Following section is for testing for pushed and released mod-keys.
    // A bit of explanation: some service actions may require combination of
    // mod-keys, for example [Ctrl + Alt] toggles mouse lock in window.
    // Here comes a problem: other actions may also use [Ctrl + Alt] mods in
    // combination with a third key: e.g. [Ctrl + Alt + V] displays engine info.
    // For this reason we cannot simply test for pressed Ctrl and Alt here,
    // but we must wait until player *releases at least one mod key* of this combo,
    // while no third key was pressed.
    // In other words, such action should only trigger if:
    // * if combination of held down mod-keys was gathered,
    // * if no other key was pressed meanwhile,
    // * if at least one of those gathered mod-keys was released.
    //
    // TODO: maybe split this mod handling into sep procedure and make it easier to use (not that it's used alot)?
    int cur_mod = sys_modkeys;
    bool is_only_mod_key = false;
    switch (key_evt.type)
    {
    case SDL_KEYDOWN:
        is_only_mod_key = is_sdl_mod_key(key_evt.key.keysym);
        cur_mod |= make_sdl_merged_mod(make_sdl_mod_flag(key_evt.key.keysym));
        break;
    case SDL_KEYUP:
        is_only_mod_key = is_sdl_mod_key(key_evt.key.keysym);
        cur_mod &= ~make_sdl_merged_mod(make_sdl_mod_flag(key_evt.key.keysym));
        break;
    }
    
    // If mods combination have already triggered an action,
    // then do nothing until all the current mods are released
    if (!sys_modkeys_fired)
    {
        // If any non-mod key is pressed, add "fired" flag to indicate that
        // this is no longer a pure mod keys combination
        if ((sys_modkeys != 0) && !is_only_mod_key)
        {
            sys_modkeys_fired = true;
        }
        // If some of the previously pressed mods were released, then run key combo action
        // and set "fired" flag to prevent multiple execution
        else if ((sys_modkeys != 0) && ((sys_modkeys & cur_mod) != sys_modkeys))
        {
            // Toggle mouse lock on Ctrl + Alt
            if (sys_modkeys == (KMOD_CTRL | KMOD_ALT))
            {
                toggle_mouse_lock();
                handled = true;
            }
            sys_modkeys_fired = true;
        }
    }
    // Save new mod flags, keep or erase the "fired" flag,
    // depending on whether there are any mod keys still pressed
    sys_modkeys = cur_mod;
    sys_modkeys_fired = sys_modkeys_fired && (cur_mod != 0);

    // If mods are handled, or is in backward input mode, then stop here
    if (handled || (old_keyhandle && is_only_mod_key))
        return false;

    KeyInput ki = sdl_keyevt_to_ags_key(key_evt, old_keyhandle);
    if ((ki.Key == eAGSKeyCodeNone) && (ki.UChar == 0))
        return false; // should skip this key event

    // Use backward-compatible combined key for special controls,
    // because game variables may store old-style key + mod codes
    const eAGSKeyCode agskey = ki.CompatKey;
    // LAlt or RAlt + Enter/Return
    if ((ki.Mod & eAGSModAlt) && (agskey == eAGSKeyCodeReturn))
    {
        engine_try_switch_windowed_gfxmode();
        return false;
    }

    // Alt+X, abort (but only once game is loaded)
    if ((displayed_room >= 0) && (play.abort_key > 0) && (agskey == play.abort_key)) {
        Debug::Printf("Abort key pressed");
        check_dynamic_sprites_at_exit = 0;
        quit("!|");
    }

    if ((agskey == eAGSKeyCodeCtrlE) && (display_fps == kFPS_Forced)) {
        // if --fps paramter is used, Ctrl+E will max out frame rate
        setTimerFps(isTimerFpsMaxed() ? frames_per_second : 1000);
        return false;
    }

    if ((agskey == eAGSKeyCodeCtrlD) && (play.debug_mode > 0)) {
        // ctrl+D - show info
        char infobuf[900];
        sprintf(infobuf, "In room %d %s[Player at %d, %d (view %d, loop %d, frame %d)%s%s%s",
            displayed_room, (noWalkBehindsAtAll ? "(has no walk-behinds)" : ""), playerchar->x, playerchar->y,
            playerchar->view + 1, playerchar->loop, playerchar->frame,
            (IsGamePaused() == 0) ? "" : "[Game paused.",
            (play.ground_level_areas_disabled == 0) ? "" : "[Ground areas disabled.",
            (IsInterfaceEnabled() == 0) ? "[Game in Wait state" : "");
        for (uint32_t ff = 0; ff<croom->numobj; ff++) {
            if (ff >= 8) break; // buffer not big enough for more than 7
            sprintf(&infobuf[strlen(infobuf)],
                "[Object %d: (%d,%d) size (%d x %d) on:%d moving:%s animating:%d slot:%d trnsp:%d clkble:%d",
                ff, objs[ff].x, objs[ff].y,
                (spriteset.DoesSpriteExist(objs[ff].num) ? game.SpriteInfos[objs[ff].num].Width : 0),
                (spriteset.DoesSpriteExist(objs[ff].num) ? game.SpriteInfos[objs[ff].num].Height : 0),
                objs[ff].on,
                (objs[ff].moving > 0) ? "yes" : "no", objs[ff].cycling,
                objs[ff].num, objs[ff].transparent,
                ((objs[ff].flags & OBJF_NOINTERACT) != 0) ? 0 : 1);
        }
        DisplayMB(infobuf);
        int chd = game.playercharacter;
        char bigbuffer[STD_BUFFER_SIZE] = "CHARACTERS IN THIS ROOM:[";
        for (int ff = 0; ff < game.numcharacters; ff++) {
            if (game.chars[ff].room != displayed_room) continue;
            if (strlen(bigbuffer) > 430) {
                strcat(bigbuffer, "and more...");
                DisplayMB(bigbuffer);
                strcpy(bigbuffer, "CHARACTERS IN THIS ROOM (cont'd):[");
            }
            chd = ff;
            sprintf(&bigbuffer[strlen(bigbuffer)],
                "%s (view/loop/frm:%d,%d,%d  x/y/z:%d,%d,%d  idleview:%d,time:%d,left:%d walk:%d anim:%d follow:%d flags:%X wait:%d zoom:%d)[",
                game.chars[chd].scrname, game.chars[chd].view + 1, game.chars[chd].loop, game.chars[chd].frame,
                game.chars[chd].x, game.chars[chd].y, game.chars[chd].z,
                game.chars[chd].idleview, game.chars[chd].idletime, game.chars[chd].idleleft,
                game.chars[chd].walking, game.chars[chd].animating, game.chars[chd].following,
                game.chars[chd].flags, game.chars[chd].wait, charextra[chd].zoom);
        }
        DisplayMB(bigbuffer);
        return false;
    }

    if (((agskey == eAGSKeyCodeCtrlV) && (ki.Mod & eAGSModAlt) != 0)
        && (play.wait_counter < 1) && (play.text_overlay_on == 0) && (restrict_until.type == 0)) {
        // make sure we can't interrupt a Wait()
        // and desync the music to cutscene
        play.debug_mode++;
        script_debug(1, 0);
        play.debug_mode--;
        return false;
    }

    // No service operation triggered? return active keypress and mods to caller
    out_key = ki;
    return true;
}

// Runs default keyboard handling
static void check_keyboard_controls()
{
    const bool old_keyhandle = game.options[OPT_KEYHANDLEAPI] == 0;
    // First check for service engine's combinations (mouse lock, display mode switch, and so forth)
    KeyInput ki;
    if (!run_service_key_controls(ki)) {
        return;
    }
    // Use backward-compatible combined key for special controls,
    // because game variables may store old-style key + mod codes
    const eAGSKeyCode agskey = ki.CompatKey;
    // Then, check cutscene skip
    check_skip_cutscene_keypress(agskey);
    if (play.fast_forward) { 
        return; 
    }
    if (play.IsIgnoringInput()) {
        return;
    }
    // Now check for in-game controls
    if (pl_run_plugin_hooks(AGSE_KEYPRESS, agskey)) {
        // plugin took the keypress
        debug_script_log("Keypress code %d taken by plugin", agskey);
        return;
    }

    // skip speech if desired by Speech.SkipStyle
    if ((play.text_overlay_on > 0) && (play.cant_skip_speech & SKIP_KEYPRESS) &&
            !IsAGSServiceKey(ki.Key)) {
        // only allow a key to remove the overlay if the icon bar isn't up
        if (IsGamePaused() == 0) {
            // check if it requires a specific keypress
            if ((play.skip_speech_specific_key == 0) ||
                (agskey == play.skip_speech_specific_key))
            {
                remove_screen_overlay(play.text_overlay_on);
                play.SetWaitKeySkip(ki);
            }
        }

        return;
    }

    if ((play.wait_counter != 0) && (play.key_skip_wait & SKIP_KEYPRESS) &&
            !IsAGSServiceKey(ki.Key)) {
        play.SetWaitKeySkip(ki);
        return;
    }

    if (inside_script) {
        // Don't queue up another keypress if it can't be run instantly
        debug_script_log("Keypress %d ignored (game blocked)", agskey);
        return;
    }

    bool keywasprocessed = false;
    // Determine if a GUI Text Box should steal the click:
    // it should be either a printable character or one of the textbox control keys
    // TODO: instead of making a preliminary check, just let each gui control
    // test the key and OnKeyPress return if it was handled?
    if ((all_buttons_disabled < 0) &&
        ((ki.UChar > 0) || (agskey >= 32) && (agskey <= 255)) ||
         (agskey == eAGSKeyCodeReturn) || (agskey == eAGSKeyCodeBackspace)) {
        for (int guiIndex = 0; guiIndex < game.numgui; guiIndex++) {
            auto &gui = guis[guiIndex];

            if (!gui.IsDisplayed()) continue;

            for (int controlIndex = 0; controlIndex < gui.GetControlCount(); controlIndex++) {
                // not a text box, ignore it
                if (gui.GetControlType(controlIndex) != kGUITextBox) { continue; }

                auto *guitex = static_cast<GUITextBox*>(gui.GetControl(controlIndex));
                if (guitex == nullptr) { continue; }

                // if the text box is disabled, it cannot accept keypresses
                if (!guitex->IsEnabled()) { continue; }
                if (!guitex->IsVisible()) { continue; }

                keywasprocessed = true;

                guitex->OnKeyPress(ki);

                if (guitex->IsActivated) {
                    guitex->IsActivated = false;
                    setevent(EV_IFACECLICK, guiIndex, controlIndex, 1);
                }
            }
        }
    }

    if (keywasprocessed)
        return;

    // Built-in key-presses
    if ((usetup.key_save_game > 0) && (agskey == usetup.key_save_game)) {
        do_save_game_dialog();
        return;
    } else if ((usetup.key_restore_game > 0) && (agskey == usetup.key_restore_game)) {
        do_restore_game_dialog();
        return;
    }

    // Pass the key event to the script
    const int sckey = AGSKeyToScriptKey(ki.Key);
    const int sckeymod = ki.Mod;
    if (old_keyhandle || (ki.UChar == 0))
    {
        debug_script_log("Running on_key_press keycode %d, mod %d", sckey, sckeymod);
        setevent(EV_TEXTSCRIPT, kTS_KeyPress, sckey, sckeymod);
    }
    if (!old_keyhandle && (ki.UChar > 0))
    {
        debug_script_log("Running on_text_input char %s (%d)", ki.Text, ki.UChar);
        setevent(EV_TEXTSCRIPT, kTS_TextInput, ki.UChar);
    }
}

bool run_service_gamepad_controls(GamepadInput &out_key)
{
    out_key.JoystickID = -1;
    out_key.Type = eAGSGamepad_InputTypeNone;
    out_key.Button = eAGSGamepad_ButtonInvalid;
    out_key.Axis = eAGSGamepad_AxisInvalid;

    if (ags_inputevent_ready() != kInputGamepad)
        return false; // there was no gamepad event

    const auto gp_evt = ags_get_next_inputevent();
    switch (gp_evt.type)
    {
        case SDL_CONTROLLERBUTTONDOWN:
            out_key.JoystickID = gp_evt.cbutton.which;
            out_key.Type = eAGSGamepad_InputTypeButton;
            out_key.Button = Gamepad_Button_SDLtoAGS(static_cast<SDL_GameControllerButton>(gp_evt.cbutton.button));
            break;
    }
    return out_key.Button != eAGSGamepad_ButtonInvalid;
}

// Runs default gamepad handling
static void check_gamepad_controls()
{
    // First check for service engine's combinations (mouse lock, display mode switch, and so forth)
    GamepadInput gi;
    if (!run_service_gamepad_controls(gi)) {
        return;
    }
    eAGSGamepad_Button gbn = gi.Button;
    // Then, check cutscene skip
    check_skip_cutscene_gamepad(gbn);
    if (play.fast_forward) {
        return;
    }
    if (play.IsIgnoringInput()) {
        return;
    }

    // skip speech if desired by Speech.SkipStyle
    if ((play.text_overlay_on > 0) && (play.cant_skip_speech & SKIP_KEYPRESS)) {
        // only allow a key to remove the overlay if the icon bar isn't up
        if (IsGamePaused() == 0) {
            // check if it requires a specific keypress
            if ((play.skip_speech_specific_key > 0) &&
                (gbn != play.skip_speech_specific_key)) { }
            else
            {
                remove_screen_overlay(play.text_overlay_on);
                play.SetWaitSkipResult(SKIP_GAMEPAD, gbn);
            }
        }

        return;
    }

    if ((play.wait_counter != 0) && (play.key_skip_wait & SKIP_KEYPRESS) != 0) {
        play.SetWaitSkipResult(SKIP_GAMEPAD, gbn);
        return;
    }

    if (inside_script) {
        // Don't queue up another button press if it can't be run instantly
        debug_script_log("Gamepad button %d ignored (game blocked)", gbn);
        return;
    }
}

// check_controls: checks mouse & keyboard interface
static void check_controls() {
    our_eip = 1007;

    sys_evt_process_pending();

    // First handle mouse state, which does not depend on down/up events
    // (motion, wheel axis, etc)
    // FIXME: atm we must save the last mouse_on_iface value *locally* for use
    // further in check_mouse_controls, because there may be 1+ nested
    // check_controls() calls as a result of some triggered script callbacks,
    // during which some global vars like mouse_on_iface may change...
    // need to rewrite all this interface interaction ugliness!
    int was_mouse_on_iface;
    check_mouse_state(was_mouse_on_iface);

    // Handle all the buffered input events
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    {
        switch (type) {
            case kInputKeyboard:
                check_keyboard_controls();
                break;
            case kInputMouse:
                check_mouse_controls(was_mouse_on_iface);
                break;
            case kInputGamepad:
                check_gamepad_controls();
                break;
            default:
                break;
        }
    }

    ags_clear_input_buffer();
}

static void check_room_edges(size_t numevents_was)
{
    if ((IsInterfaceEnabled()) && (IsGamePaused() == 0) &&
        (in_new_room == 0) && (new_room_was == 0)) {
            // Only allow walking off edges if not in wait mode, and
            // if not in Player Enters Screen (allow walking in from off-screen)
            int edgesActivated[4] = {0, 0, 0, 0};
            // Only do it if nothing else has happened (eg. mouseclick)
            if ((events.size() == numevents_was) &&
                ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0)) {

                    if (playerchar->x <= thisroom.Edges.Left)
                        edgesActivated[0] = 1;
                    else if (playerchar->x >= thisroom.Edges.Right)
                        edgesActivated[1] = 1;
                    if (playerchar->y >= thisroom.Edges.Bottom)
                        edgesActivated[2] = 1;
                    else if (playerchar->y <= thisroom.Edges.Top)
                        edgesActivated[3] = 1;

                    if ((play.entered_edge >= 0) && (play.entered_edge <= 3)) {
                        // once the player is no longer outside the edge, forget the stored edge
                        if (edgesActivated[play.entered_edge] == 0)
                            play.entered_edge = -10;
                        // if we are walking in from off-screen, don't activate edges
                        else
                            edgesActivated[play.entered_edge] = 0;
                    }

                    for (int ii = 0; ii < 4; ii++) {
                        if (edgesActivated[ii])
                            setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, ii);
                    }
            }
    }
    our_eip = 1008;

}

static void game_loop_check_controls(bool checkControls)
{
    // don't let the player do anything before the screen fades in
    if ((in_new_room == 0) && (checkControls)) {
        int inRoom = displayed_room;
        size_t numevents_was = events.size();
        check_controls();
        check_room_edges(numevents_was);
        // If an inventory interaction changed the room
        if (inRoom != displayed_room)
            check_new_room();
    }
}

static void game_loop_do_update()
{
    if (debug_flags & DBG_NOUPDATE) ;
    else if (game_paused==0) update_stuff();
}

static void game_loop_update_animated_buttons()
{
    // update animating GUI buttons
    // this bit isn't in update_stuff because it always needs to
    // happen, even when the game is paused
    for (size_t i = 0; i < GetAnimatingButtonCount(); ++i) {
        if (!UpdateAnimatingButton(i)) {
            StopButtonAnimation(i);
            i--;
        }
    }
}

extern std::vector<ViewStruct> views;

static void update_objects_scale()
{
    for (uint32_t objid = 0; objid < croom->numobj; ++objid)
    {
        update_object_scale(objid);
    }

    for (uint32_t charid = 0; charid < game.numcharacters; ++charid)
    {
        update_character_scale(charid);
    }
}

// Updates GUI reaction to the cursor position change
// TODO: possibly may be merged with gui_on_mouse_move()
static void update_cursor_over_gui()
{
    if (((debug_flags & DBG_NOIFACE) != 0) || (displayed_room < 0))
        return; // GUI is disabled (debug flag) or room is not loaded
    if (!IsInterfaceEnabled())
        return; // interface is disabled (by script or blocking action)
    // Poll guis
    for (auto &gui : guis)
    {
        if (!gui.IsDisplayed()) continue; // not on screen
        if (!gui.IsClickable()) continue; // don't update non-clickable
        // Don't touch GUI if "GUIs Turn Off When Disabled"
        if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) &&
            (all_buttons_disabled >= 0) &&
            (gui.PopupStyle != kGUIPopupNoAutoRemove))
            continue;
        gui.Poll(mousex, mousey);
    }
}

extern int lastmx, lastmy;
extern int mouse_frame, mouse_delay;

static void update_cursor_view()
{
    // update animating mouse cursor
    if (game.mcurs[cur_cursor].view >= 0) {
        // only on mousemove, and it's not moving
        if (((game.mcurs[cur_cursor].flags & MCF_ANIMMOVE) != 0) &&
            (mousex == lastmx) && (mousey == lastmy));
        // only on hotspot, and it's not on one
        else if (((game.mcurs[cur_cursor].flags & MCF_HOTSPOT) != 0) &&
            (GetLocationType(mousex, mousey) == 0))
            set_new_cursor_graphic(game.mcurs[cur_cursor].pic);
        else if (mouse_delay>0) mouse_delay--;
        else {
            int viewnum = game.mcurs[cur_cursor].view;
            int loopnum = 0;
            if (loopnum >= views[viewnum].numLoops)
                quitprintf("An animating mouse cursor is using view %d which has no loops", viewnum + 1);
            if (views[viewnum].loops[loopnum].numFrames < 1)
                quitprintf("An animating mouse cursor is using view %d which has no frames in loop %d", viewnum + 1, loopnum);

            mouse_frame++;
            if (mouse_frame >= views[viewnum].loops[loopnum].numFrames)
                mouse_frame = 0;
            set_new_cursor_graphic(views[viewnum].loops[loopnum].frames[mouse_frame].pic);
            mouse_delay = views[viewnum].loops[loopnum].frames[mouse_frame].speed + game.mcurs[cur_cursor].animdelay;
            CheckViewFrame(viewnum, loopnum, mouse_frame);
        }
        lastmx = mousex; lastmy = mousey;
    }
}

static void update_cursor_over_location(int mwasatx, int mwasaty)
{
    if (play.fast_forward)
        return;
    if (displayed_room < 0)
        return;

    // Check Mouse Moves Over Hotspot event
    auto view = play.GetRoomViewportAt(mousex, mousey);
    auto cam = view ? view->GetCamera() : nullptr;
    if (!cam)
        return;

    // NOTE: all cameras are in same room right now, so their positions are in same coordinate system;
    // therefore we may use this as an indication that mouse is over different camera too.
    // TODO: do not use static variables!
    // TODO: if we support rotation then we also need to compare full transform!
    static int offsetxWas = -1000, offsetyWas = -1000;
    int offsetx = cam->GetRect().Left;
    int offsety = cam->GetRect().Top;

    if (((mwasatx!=mousex) || (mwasaty!=mousey) ||
        (offsetxWas != offsetx) || (offsetyWas != offsety))) 
    {
        // mouse moves over hotspot
        if (__GetLocationType(mousex, mousey, 1) == LOCTYPE_HOTSPOT) {
            int onhs = getloctype_index;

            setevent(EV_RUNEVBLOCK, EVB_HOTSPOT, onhs, EVHOT_MOUSEOVER);
        }
    }

    offsetxWas = offsetx;
    offsetyWas = offsety;
}

static void game_loop_update_events()
{
    new_room_was = in_new_room;
    if (in_new_room>0)
        setevent(EV_FADEIN,0,0,0);
    in_new_room=0;
    processallevents();
    if ((new_room_was > 0) && (in_new_room == 0)) {
        // if in a new room, and the room wasn't just changed again in update_events,
        // then queue the Enters Screen scripts
        // run these next time round, when it's faded in
        if (new_room_was==2)  // first time enters screen
            setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, EVROM_FIRSTENTER);
        if (new_room_was!=3)   // enters screen after fadein
            setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, EVROM_AFTERFADEIN);
    }
}

static void game_loop_update_background_animation()
{
    if (play.bg_anim_delay > 0) play.bg_anim_delay--;
    else if (play.bg_frame_locked) ;
    else {
        play.bg_anim_delay = play.anim_background_speed;
        play.bg_frame++;
        if ((size_t)play.bg_frame >= thisroom.BgFrameCount)
            play.bg_frame=0;
        if (thisroom.BgFrameCount >= 2) {
            // get the new frame's palette
            on_background_frame_change();
        }
    }
}

static void game_loop_update_loop_counter()
{
    loopcounter++;

    if (play.wait_counter > 0) play.wait_counter--;
    if (play.shakesc_length > 0) play.shakesc_length--;
}

static void game_loop_update_fps()
{
    auto t2 = AGS_Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    auto frames = loopcounter - lastcounter;

    if (duration >= std::chrono::milliseconds(1000) && frames > 0) {
        fps = 1000.0f * frames / duration.count();
        t1 = t2;
        lastcounter = loopcounter;
    }
}

float get_game_fps() {
    // if we have maxed out framerate then return the frame rate we're seeing instead
    // fps must be greater that 0 or some timings will take forever.
    if (isTimerFpsMaxed() && fps > 0.0f) {
        return fps;
    }
    return frames_per_second;
}

float get_real_fps() {
    return fps;
}

void set_loop_counter(unsigned int new_counter) {
    loopcounter = new_counter;
    t1 = AGS_Clock::now();
    lastcounter = loopcounter;
    fps = std::numeric_limits<float>::quiet_NaN();
}

void UpdateGameOnce(bool checkControls, IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {

    int res;

    sys_evt_process_pending();

    numEventsAtStartOfFunction = events.size();

    if (want_exit) {
        ProperExit();
    }

    ccNotifyScriptStillAlive ();
    our_eip=1;

    game_loop_check_problems_at_start();

    // if we're not fading in, don't count the fadeouts
    if ((play.no_hicolor_fadein) && (game.options[OPT_FADETYPE] == FADE_NORMAL))
        play.screen_is_faded_out = 0;

    our_eip = 1014;

    update_gui_disabled_status();

    our_eip = 1004;

    game_loop_do_early_script_update();
    // run this immediately to make sure it gets done before fade-in
    // (player enters screen)
    check_new_room();

    our_eip = 1005;

    res = game_loop_check_ground_level_interactions();
    if (res != RETURN_CONTINUE) {
        return;
    }

    mouse_on_iface=-1;

    check_debug_keys();

    // Handle player's input
    // remember old mouse pos, needed for update_cursor_over_location() later
    const int mwasatx = mousex, mwasaty = mousey;
    // update mouse position (mousex, mousey)
    ags_domouse();
    // update gui under mouse; this also updates gui control focus;
    // atm we must call this before "check_controls", because GUI interaction
    // relies on remembering which control was focused by the cursor prior
    update_cursor_over_gui();
    // handle actual input (keys, mouse, and so forth)
    game_loop_check_controls(checkControls);

    our_eip=2;

    // do the overall game state update
    game_loop_do_update();

    game_loop_update_animated_buttons();

    game_loop_do_late_script_update();

    // historically room object and character scaling was updated
    // right before the drawing
    update_objects_scale();
    update_cursor_over_location(mwasatx, mwasaty);
    update_cursor_view();

    update_audio_system_on_game_loop();

    // Only render if we are not skipping a cutscene
    if (!play.fast_forward)
        render_graphics(extraBitmap, extraX, extraY);

    our_eip=6;

    game_loop_update_events();

    our_eip=7;

    update_polled_stuff();

    game_loop_update_background_animation();

    game_loop_update_loop_counter();

    // Immediately start the next frame if we are skipping a cutscene
    if (play.fast_forward)
        return;

    our_eip=72;

    game_loop_update_fps();

    update_polled_stuff();

    WaitForNextFrame();
}

void UpdateGameAudioOnly()
{
    update_audio_system_on_game_loop();
    game_loop_update_loop_counter();
    game_loop_update_fps();
    WaitForNextFrame();
}

static void UpdateMouseOverLocation()
{
    // Call GetLocationName - it will internally force a GUI refresh
    // if the result it returns has changed from last time
    char tempo[STD_BUFFER_SIZE];
    GetLocationName(mousex, mousey, tempo);

    if ((play.get_loc_name_save_cursor >= 0) &&
        (play.get_loc_name_save_cursor != play.get_loc_name_last_time) &&
        (mouse_on_iface < 0) && (ifacepopped < 0)) {
            // we have saved the cursor, but the mouse location has changed
            // and it's time to restore it
            play.get_loc_name_save_cursor = -1;
            set_cursor_mode(play.restore_cursor_mode_to);

            if (cur_mode == play.restore_cursor_mode_to)
            {
                // make sure it changed -- the new mode might have been disabled
                // in which case don't change the image
                set_mouse_cursor(play.restore_cursor_image_to);
            }
            debug_script_log("Restore mouse to mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
    }
}

// Checks if user interface should remain disabled for now
static bool ShouldStayInWaitMode() {
    if (restrict_until.type == 0)
        quit("end_wait_loop called but game not in loop_until state");

    switch (restrict_until.type)
    {
    case UNTIL_MOVEEND:
    {
        short*wkptr = (short*)restrict_until.data_ptr;
        return !(wkptr[0] < 1);
    }
    case UNTIL_CHARIS0:
    {
        char*chptr = (char*)restrict_until.data_ptr;
        return !(chptr[0] == 0);
    }
    case UNTIL_NEGATIVE:
    {
        short*wkptr = (short*)restrict_until.data_ptr;
        return !(wkptr[0] < 0);
    }
    case UNTIL_INTISNEG:
    {
        int*wkptr = (int*)restrict_until.data_ptr;
        return !(wkptr[0] < 0);
    }
    case UNTIL_NOOVERLAY:
    {
        return !(play.text_overlay_on == 0);
    }
    case UNTIL_INTIS0:
    {
        int*wkptr = (int*)restrict_until.data_ptr;
        return !(wkptr[0] == 0);
    }
    case UNTIL_SHORTIS0:
    {
        short*wkptr = (short*)restrict_until.data_ptr;
        return !(wkptr[0] == 0);
    }
    case UNTIL_ANIMBTNEND:
    {  // still animating?
        return FindButtonAnimation(restrict_until.data1, restrict_until.data2) >= 0;
    }
    default:
        quit("loop_until: unknown until event");
    }

    return true; // should stay in wait
}

static int UpdateWaitMode()
{
    if (restrict_until.type == 0) { return RETURN_CONTINUE; }

    if (!ShouldStayInWaitMode())
        restrict_until.type = 0;
    our_eip = 77;

    if (restrict_until.type > 0) { return RETURN_CONTINUE; }

    auto was_disabled_for = restrict_until.disabled_for;

    set_default_cursor();
    // If GUI looks change when disabled, then mark all of them for redraw
    GUI::MarkAllGUIForUpdate(GUI::Options.DisabledStyle != kGuiDis_Unchanged, true);
    play.disabled_user_interface--;
    restrict_until.disabled_for = 0;

    switch (was_disabled_for) {
        // case FOR_ANIMATION:
        //     run_animation((FullAnimation*)user_disabled_data2,user_disabled_data3);
        //     break;
        case FOR_EXITLOOP:
            return -1;
        case FOR_SCRIPT:
            quit("err: for_script obsolete (v2.1 and earlier only)");
            break;
        default:
            quit("Unknown user_disabled_for in end restrict_until");
    }

    // we shouldn't get here.
    return RETURN_CONTINUE;
}

// Run single game iteration; calls UpdateGameOnce() internally
static int GameTick()
{
    if (displayed_room < 0)
        quit("!A blocking function was called before the first room has been loaded");

    UpdateGameOnce(true);
    UpdateMouseOverLocation();

    our_eip=76;

    int res = UpdateWaitMode();
    if (res == RETURN_CONTINUE) { return 0; } // continue looping 
    return res;
}

static void SetupLoopParameters(int untilwhat, const void* data_ptr = nullptr, int data1 = 0, int data2 = 0) {
    play.disabled_user_interface++;
    // If GUI looks change when disabled, then mark all of them for redraw
    GUI::MarkAllGUIForUpdate(GUI::Options.DisabledStyle != kGuiDis_Unchanged, true);

    // Only change the mouse cursor if it hasn't been specifically changed first
    // (or if it's speech, always change it)
    if (((cur_cursor == cur_mode) || (untilwhat == UNTIL_NOOVERLAY)) &&
        (cur_mode != CURS_WAIT))
        set_mouse_cursor(CURS_WAIT);

    restrict_until.type = untilwhat;
    restrict_until.data_ptr = data_ptr;
    restrict_until.data1 = data1;
    restrict_until.data2 = data2;
    restrict_until.disabled_for = FOR_EXITLOOP;
}

// This function is called from lot of various functions
// in the game core, character, room object etc
static void GameLoopUntilEvent(int untilwhat, const void* data_ptr = nullptr, int data1 = 0, int data2 = 0) {
  // blocking cutscene - end skipping
  EndSkippingUntilCharStops();

  // this function can get called in a nested context, so
  // remember the state of these vars in case a higher level
  // call needs them
  auto cached_restrict_until = restrict_until;

  SetupLoopParameters(untilwhat, data_ptr, data1, data2);
  while (GameTick()==0);

  our_eip = 78;

  restrict_until = cached_restrict_until;
}

void GameLoopUntilValueIsZero(const char *value) 
{
    GameLoopUntilEvent(UNTIL_CHARIS0, value);
}

void GameLoopUntilValueIsZero(const short *value) 
{
    GameLoopUntilEvent(UNTIL_SHORTIS0, value);
}

void GameLoopUntilValueIsZero(const int *value) 
{
    GameLoopUntilEvent(UNTIL_INTIS0, value);
}

void GameLoopUntilValueIsZeroOrLess(const short *value) 
{
    GameLoopUntilEvent(UNTIL_MOVEEND, value);
}

void GameLoopUntilValueIsNegative(const short *value) 
{
    GameLoopUntilEvent(UNTIL_NEGATIVE, value);
}

void GameLoopUntilValueIsNegative(const int *value) 
{
    GameLoopUntilEvent(UNTIL_INTISNEG, value);
}

void GameLoopUntilNotMoving(const short *move) 
{
    GameLoopUntilEvent(UNTIL_MOVEEND, move);
}

void GameLoopUntilNoOverlay() 
{
    GameLoopUntilEvent(UNTIL_NOOVERLAY);
}

void GameLoopUntilButAnimEnd(int guin, int objn)
{
    GameLoopUntilEvent(UNTIL_ANIMBTNEND, nullptr, guin, objn);
}


extern unsigned int load_new_game;
void RunGameUntilAborted()
{
    // skip ticks to account for time spent starting game.
    skipMissedTicks();

    while (!abort_engine) {
        GameTick();

        if (load_new_game) {
            RunAGSGame (nullptr, load_new_game, 0);
            load_new_game = 0;
        }
    }
}

void UpdateCursorAndDrawables()
{
    const int mwasatx = mousex, mwasaty = mousey;
    ags_domouse();
    update_cursor_over_gui();
    update_cursor_over_location(mwasatx, mwasaty);
    update_cursor_view();
    // TODO: following does not have to be called every frame while in a
    // fully blocking state (like Display() func), refactor to only call it
    // once the blocking state begins.
    update_objects_scale();
}

void update_polled_stuff()
{
    if (want_exit) {
        want_exit = false;
        quit("||exit!");
    }

    if (editor_debugging_initialized)
        check_for_messages_from_debugger();
}

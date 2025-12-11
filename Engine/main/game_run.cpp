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
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/overlay.h"
#include "ac/region.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/sys_events.h"
#include "ac/touch.h"
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
#include "media/video/video.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/joystick.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern int mouse_on_iface;   // mouse cursor is over this interface
extern int ifacepopped;
extern volatile bool want_exit, abort_engine;
extern int proper_exit;
extern int displayed_room;
extern EnterNewRoomState in_new_room, new_room_was;
extern ScriptSystem scsystem;
extern GameSetupStruct game;
extern RoomStruct thisroom;
extern int game_paused;
extern int getloctype_index;
extern bool in_enters_screen;
extern bool done_as_error;
extern int in_leaves_screen;
extern CharacterInfo*playerchar;
extern int cur_mode;
extern RoomObject*objs;
extern RoomStatus*croom;
extern SpriteCache spriteset;
extern int cur_mode,cur_cursor;
extern char check_dynamic_sprites_at_exit;

// Checks if wait mode should continue until condition is met
static bool ShouldStayInWaitMode();

struct GameTimer
{
    uint32_t LoopCounter = 0u;
    // Real FPS (calculated from time between frames)
    float FPS = std::numeric_limits<float>::quiet_NaN();
    // FPS syncing: it's done periodically, not every frame
    const uint32_t FPSSyncPeriodMs = 1000u;
    Clock::time_point LastFPSSyncTime = {};
    uint32_t LastFPSSyncCounter = 0u;
    // For runtime reference
    Clock::time_point StartTime = {};
    bool RuntimeStarted = false;
} static GameTimer;


// Kinds of conditions used when "waiting" for something
#define UNTIL_ANIMEND   1
#define UNTIL_MOVEEND   2
#define UNTIL_CHARIS0   3
#define UNTIL_NOOVERLAY 4
#define UNTIL_NEGATIVE  5
#define UNTIL_INTIS0    6
#define UNTIL_SHORTIS0  7
#define UNTIL_INTISNEG  8
#define UNTIL_ANIMBTNEND 9
#define UNTIL_FLAGUNSET 10
#define UNTIL_VIEWANIM  11
#define UNTIL_ANIMOVEREND 12

static void GameTick();

// Game state instructs the engine to run game loops until
// certain condition is not fullfilled.
// TODO: reimplement the end condition check using a function pointer?
class GameLoopUntilState : public GameState
{
public:
    GameLoopUntilState(int untilwhat, const void* data_ptr = nullptr, int data1 = 0, int data2 = 0)
        : _untilType(untilwhat)
        , _disabledFor(FOR_EXITLOOP)
        , _dataPtr(data_ptr)
        , _data1(data1)
        , _data2(data2)
    {
    }

    int GetUntilType() const { return _untilType; }
    int GetDisabledFor() const { return _disabledFor; }
    const void *GetDataPtr() const { return _dataPtr; }
    int GetData1() const { return _data1; }
    int GetData2() const { return _data2; }

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        assert(_disabledFor == FOR_EXITLOOP);
        // Only change the mouse cursor if it hasn't been specifically changed first
        // (or if it's speech, always change it)
        bool should_update_cursor =
            ((cur_cursor == cur_mode) || (_untilType == UNTIL_NOOVERLAY)) &&
            (!is_current_cursor_mode(kCursorRole_Wait));
        DisableInterfaceEx(should_update_cursor);
    }
    // End the state, release all resources
    void End() override
    {
        set_our_eip(77);
        EnableInterfaceEx(true /* update cursor */);

        switch (_disabledFor)
        {
        case FOR_EXITLOOP:
            break;
        // These other types are obsolete since at least v2.5
        // FOR_SCRIPT is for v2.1 and earlier.
        // case FOR_ANIMATION:
        //     run_animation((FullAnimation*)user_disabled_data2,user_disabled_data3);
        //     break;
        // case FOR_SCRIPT:
        //     break;
        default:
            quit("Unknown reason to disable user input in the Wait state.");
            break;
        }
    }
    // Draw the state
    void Draw() override
    {
    }
    // Update the state during a game tick
    bool Run() override
    {
        // If skipping cutscene and expecting user input: don't wait at all
        if (play.fast_forward && (play.wait_counter != 0) && ((play.key_skip_wait & ~SKIP_AUTOTIMER) != 0))
        {
            play.SetWaitSkipResult(SKIP_NONE);
            return false;
        }

        GameTick();
        return ShouldStayInWaitMode();
    }

private:
    int _untilType = 0; // type of condition, UNTIL_* constant
    int _disabledFor = 0; // FOR_* constant
    // pointer to the test variable
    const void *_dataPtr = nullptr;
    // other values used for a test, depend on type
    int _data1 = 0;
    int _data2 = 0;
};

// TODO: this is a global variable, because this state is checked during update;
// find a way to refactor this and not have it here.
std::unique_ptr<GameLoopUntilState> restrict_until;
bool restrict_until_in_script = false;

// Schedules a normal exit from the game
static void ProperExit()
{
    want_exit = false;
    proper_exit = 1;
    quit("||exit!");
}

// Checks for potential general problems at the start of the game update,
// that may lead to warnings or game aborts
static void GameUpdateCheckProblems()
{
    if (displayed_room < 0)
    {
        quit("!A blocking function was called before the first room has been loaded");
    }

    if ((in_enters_screen) && (!done_as_error))
    {
        debug_script_warn("Wait() was used in \"Before Fadein\" event; use \"After Fadein\" instead");
        done_as_error = true;
    }

    if (!get_can_run_delayed_command())
    {
        quit("!A blocking function was called from within a non-blocking event such as " REP_EXEC_ALWAYS_NAME);
    }

    // If we're not fading in, don't count the fadeouts
    // FIXME: find a better place for this fixup, probably should be done
    // after restoring a save, and after 'no_hicolor_fadein' or OPT_FADETYPE is changed?
    if ((play.no_hicolor_fadein) && (game.options[OPT_FADETYPE] == kScrTran_Fade))
        play.screen_is_faded_out = 0;
}

// Runs rep-exec-always immediately,
// schedules global and room's rep-exec events to be run during script event processing.
static void GameUpdateEarlyRepExec()
{
    if (in_new_room == 0)
    {
        // Run the room and game script repeatedly_execute
        run_function_on_non_blocking_thread(&repExecAlways);
        setevent(AGSEvent_Script(kTS_RepeatedlyExecute));
        setevent(AGSEvent_Object(kObjEventType_Room, 0, kRoomEvent_Repexec));
    }
}

// Runs late-rep-exec-always
static void GameUpdateLateRepExec()
{
    if (in_new_room == 0)
    {
        // Run the room and game script late_repeatedly_execute
        run_function_on_non_blocking_thread(&lateRepExecAlways);
    }
}

// Check room's "ground" triggers, such as regions and hotspots with
// "Stand On" event callbacks.
static void GameUpdateCheckGroundInteractions()
{
    // If ground interactions are disabled completely, then bail out
    if ((play.ground_level_areas_disabled & GLED_INTERACTION) != 0)
        return;

    // Do not check for ground interactions while in the waiting
    // (a blocking action, or a Wait call from the user script).
    // This is done because interaction event handlers are scheduled and are
    // only run after blocking action / wait is over.
    // Which may cause all kinds of unexpected and untimely effects.
    // NOTE: this condition was not present in the older versions of the
    // engine, but result was more or less same by accident, as the number
    // of scheduled callbacks was limited to a very small number.
    // (That was pretty unreliable though.)
    if (IsInBlockingAction() && (loaded_game_file_version >= kGameVersion_362))
    {
        // NOTE: if we do update play.player_on_region here, then player might
        // trigger "walk on/off region" after finishing blocking walk if
        // it was walking back and forth the region and with the last step
        // has crossed the region's border. CHECKME: should we do this...?
        return;
    }
    else
    {
        // check if he's standing on a hotspot
        int hotspotThere = get_hotspot_at(playerchar->x, playerchar->y);
        // run Stands on Hotspot event;
        // NOTE: this runs even for hotspot 0 (no hotspot), in case one has a script function attached
        setevent(AGSEvent_Object(kObjEventType_Hotspot, hotspotThere, kHotspotEvent_StandOn));

        // check current region
        int onRegion = GetRegionIDAtRoom(playerchar->x, playerchar->y);
        if (onRegion != play.player_on_region)
        {
            // we need to save this and set play.player_on_region
            // now, so it's correct going into RunRegionInteraction
            int oldRegion = play.player_on_region;

            play.player_on_region = onRegion;
            // Walks Off last region
            if (oldRegion > 0)
                setevent(AGSEvent_Object(kObjEventType_Region, oldRegion, kRegionEvent_WalkOff));
            // Walks Onto new region
            if (onRegion > 0)
                setevent(AGSEvent_Object(kObjEventType_Region, onRegion, kRegionEvent_WalkOn));
        }

        if (play.player_on_region > 0) // player stands on region
        {
            setevent(AGSEvent_Object(kObjEventType_Region, play.player_on_region, kRegionEvent_Standing));
        }
    }
}

static void lock_mouse_on_click()
{
    // Only update when in windowed mode, as always locked in fullscreen
    if (usetup.MouseAutoLock && scsystem.windowed != 0)
        Mouse::TryLockToWindow();
}

static void toggle_mouse_lock()
{
    // Only update when in windowed mode, as always locked in fullscreen
    if (scsystem.windowed)
    {
        if (Mouse::IsLockedToWindow())
            Mouse::UnlockFromWindow();
        else
            Mouse::TryLockToWindow();
    }
}

bool run_service_mb_controls(eAGSMouseButton &out_mbut, Point *out_mpos)
{
    out_mbut = kMouseNone; // clear the output
    if (out_mpos)
        *out_mpos = {};
    if (ags_inputevent_ready() != kInputMouse)
        return false; // there was no mouse event

    const SDL_Event mb_evt = ags_get_next_inputevent();
    if (mb_evt.type == SDL_MOUSEBUTTONDOWN)
    {
        out_mbut = sdl_mbut_to_ags_but(mb_evt.button.button);
        if (out_mpos)
            *out_mpos = Mouse::SysToGamePos(mb_evt.button.x, mb_evt.button.y);
        lock_mouse_on_click();
    }
    return out_mbut != kMouseNone;
}

static eAGSMouseButton wasbutdown = kMouseNone;
static int wasongui = 0;

// Runs default handling of mouse movement, button state, and wheel
static void check_mouse_state(int &was_mouse_on_iface)
{
    mouse_on_iface = gui_on_mouse_move(mousex, mousey);
    was_mouse_on_iface = mouse_on_iface;

    if ((ifacepopped>=0) && (mousey>=guis[ifacepopped].GetY()+guis[ifacepopped].GetHeight()))
        remove_popup_interface(ifacepopped);

    // check mouse clicks on GUIs
    if ((wasbutdown > kMouseNone) && (ags_misbuttondown(wasbutdown))) {
        gui_on_mouse_hold(wasongui, wasbutdown);
    }
    else if ((wasbutdown > kMouseNone) && (!ags_misbuttondown(wasbutdown))) {
        eAGSMouseButton mouse_btn_up = wasbutdown;
        wasbutdown = kMouseNone; // reset before event, avoid recursive call of "mouse up"
        gui_on_mouse_up(wasongui, mouse_btn_up, mousex, mousey);
    }

    int mwheelz = ags_check_mouse_wheel();
    if (mwheelz < 0)
        setevent(AGSEvent_Script(kTS_MouseClick, 9, mousex, mousey));
    else if (mwheelz > 0)
        setevent(AGSEvent_Script(kTS_MouseClick, 8, mousex, mousey));
}

// Runs default mouse button handling
static void check_mouse_controls(const int was_mouse_on_iface)
{
    eAGSMouseButton mbut;
    Point mpos;
    if (run_service_mb_controls(mbut, &mpos) && mbut > kMouseNone) {
        check_skip_cutscene_mclick(mbut);

        if (play.fast_forward || play.IsIgnoringInput()) { /* do nothing if skipping cutscene or input disabled */ }
        else if ((play.wait_counter != 0) && (play.key_skip_wait & SKIP_MOUSECLICK) != 0) {
            play.SetWaitSkipResult(SKIP_MOUSECLICK, mbut);
        }
        else if (play.text_overlay_on > 0) {
            if (play.speech_skip_style & SKIP_MOUSECLICK)
            {
                remove_screen_overlay(play.text_overlay_on);
                play.SetWaitSkipResult(SKIP_MOUSECLICK, mbut);
            }
        }
        else if (!IsInterfaceEnabled()) ;  // blocking cutscene, ignore mouse
        else if (pl_run_plugin_hooks(kPluginEvt_MouseClick, mbut)) {
            // plugin took the click
            debug_script_log("Plugin handled mouse button %d", mbut);
        }
        else if (was_mouse_on_iface >= 0) {
            if (wasbutdown == kMouseNone) {
                // FIXME: logically should pass recorded mpos.X, mpos.Y, but first must investigate
                // how that will affect other GUI processing (mouse up and motion)
                gui_on_mouse_down(was_mouse_on_iface, mbut, mousex, mousey);
            }
            wasongui = was_mouse_on_iface;
            wasbutdown = mbut;
        }
        else setevent(AGSEvent_Script(kTS_MouseClick, mbut, mpos.X, mpos.Y));
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
        // if --fps parameter is used, Ctrl+E will toggle maxed out frame rate
        setTimerFps(frames_per_second, !isTimerFpsMaxed());
        return false;
    }

    // FIXME: review this command! - practically inconvenient
    if ((agskey == eAGSKeyCodeCtrlD) && (play.debug_mode > 0)) {
        // ctrl+D - show info
        String buffer = String::FromFormat(
            "In room %d %s\nPlayer at %d, %d (view %d, loop %d, frame %d)%s%s%s",
            displayed_room, (noWalkBehindsAtAll ? "(has no walk-behinds)" : ""), playerchar->x, playerchar->y,
            playerchar->view + 1, playerchar->loop, playerchar->frame,
            (IsGamePaused() == 0) ? "" : "\nGame paused.",
            (play.ground_level_areas_disabled == 0) ? "" : "\nGround areas disabled.",
            (IsInterfaceEnabled() == 0) ? "\nGame in Wait state" : "");
        for (uint32_t ff = 0; ff<croom->numobj; ff++) {
            if (ff >= 8) break; // FIXME: measure graphical size instead?
            buffer.AppendFmt(
                "\nObject %d: (%d,%d) size (%d x %d) on:%d moving:%s animating:%s slot:%d trnsp:%d clkble:%d",
                ff, objs[ff].x, objs[ff].y,
                (spriteset.DoesSpriteExist(objs[ff].num) ? game.SpriteInfos[objs[ff].num].Width : 0),
                (spriteset.DoesSpriteExist(objs[ff].num) ? game.SpriteInfos[objs[ff].num].Height : 0),
                objs[ff].is_enabled(),
                objs[ff].is_moving() ? "yes" : "no", objs[ff].is_animating() ? "yes" : "no",
                objs[ff].num, objs[ff].transparent,
                ((objs[ff].flags & OBJF_NOINTERACT) != 0) ? 0 : 1);
        }
        DisplayMB(buffer.GetCStr());
        int chd = game.playercharacter;
        buffer = "CHARACTERS IN THIS ROOM:\n";
        for (int ff = 0; ff < game.numcharacters; ff++) {
            if (game.chars[ff].room != displayed_room) continue;
            if (buffer.GetLength() > 430) { // FIXME: why 430? measure graphical size instead?
                buffer.Append("and more...");
                DisplayMB(buffer.GetCStr());
                buffer = "CHARACTERS IN THIS ROOM (cont'd):\n";
            }
            chd = ff;
            buffer.AppendFmt(
                "%s (view/loop/frm:%d,%d,%d  x/y/z:%d,%d,%d  idleview:%d,time:%d,left:%d walk:%s anim:%s follow:%d flags:%X wait:%d zoom:%d)\n",
                game.chars[chd].scrname.GetCStr(), game.chars[chd].view + 1, game.chars[chd].loop, game.chars[chd].frame,
                game.chars[chd].x, game.chars[chd].y, game.chars[chd].z,
                game.chars[chd].idleview, game.chars[chd].idletime, game.chars[chd].idleleft,
                game.chars[chd].is_moving() ? "yes" : "no", charextra[chd].IsAnimating() ? "yes" : "no", charextra[chd].following,
                game.chars[chd].flags, game.chars[chd].wait, charextra[chd].zoom);
        }
        DisplayMB(buffer.GetCStr());
        return false;
    }

    if (((agskey == eAGSKeyCodeCtrlV) && (ki.Mod & eAGSModAlt) != 0)
        && (play.wait_counter < 1) && (play.text_overlay_on == 0) && (!restrict_until)) {
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
    if (pl_run_plugin_hooks(kPluginEvt_KeyPress, agskey)) {
        // plugin took the keypress
        debug_script_log("Keypress code %d taken by plugin", agskey);
        return;
    }

    // skip speech if desired by Speech.SkipStyle
    if ((play.text_overlay_on > 0) && (play.speech_skip_style & SKIP_KEYPRESS) &&
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

    if (is_inside_script()) {
        // Don't queue up another keypress if it can't be run instantly
        debug_script_log("Keypress %d ignored (game blocked)", agskey);
        return;
    }

    bool keywasprocessed = false;
    // Determine if a GUI Text Box should steal the click:
    // it should be either a printable character or one of the textbox control keys
    // TODO: instead of making a preliminary check, just let each gui control
    // test the key and OnKeyPress return if it was handled?
    if (GUI::IsEnabledState() &&
        ((ki.UChar > 0) || ((agskey >= 32) && (agskey <= 255)) ||
         (agskey == eAGSKeyCodeReturn) || (agskey == eAGSKeyCodeBackspace))) {
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

                guitex->OnKeyPress(ki);
                // Note that the TextBox always steals the key event here, regardless
                // of whether it had any meaning for control
                keywasprocessed = true;

                if (guitex->IsActivated()) {
                    guitex->SetActivated(false);
                    // FIXME: review this, are we abusing "mouse button" arg here in order to pass a different data?
                    setevent(AGSEvent_GUI(guiIndex, controlIndex, static_cast<eAGSMouseButton>(1)));
                }
            }
        }
    }

    if (keywasprocessed)
        return;

    // Built-in key-presses
    if ((usetup.Override.KeySaveGame > 0) && (agskey == usetup.Override.KeySaveGame)) {
        do_save_game_dialog(0, TOP_SAVESLOT - 1, play.normal_font); // ignore special slot 999
        return;
    } else if ((usetup.Override.KeyRestoreGame > 0) && (agskey == usetup.Override.KeyRestoreGame)) {
        do_restore_game_dialog(0, TOP_SAVESLOT - 1, play.normal_font); // ignore special slot 999
        return;
    }

    // Pass the key event to the script
    const int sckey = AGSKeyToScriptKey(ki.Key);
    const int sckeymod = ki.Mod;
    if (old_keyhandle || (ki.UChar == 0))
    {
        debug_script_log("Running on_key_press keycode %d, mod %d", sckey, sckeymod);
        setevent(AGSEvent_Script(kTS_KeyPress, sckey, sckeymod));
    }
    if (!old_keyhandle && (ki.UChar > 0))
    {
        debug_script_log("Running on_text_input char %s (%d)", ki.Text, ki.UChar);
        setevent(AGSEvent_Script(kTS_TextInput, ki.UChar));
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
    if ((play.text_overlay_on > 0) && (play.speech_skip_style & SKIP_KEYPRESS)) {
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

    if (is_inside_script()) {
        // Don't queue up another button press if it can't be run instantly
        debug_script_log("Gamepad button %d ignored (game blocked)", gbn);
        return;
    }
}

bool run_service_touch_controls(TouchInput &out_touch)
{
    out_touch = {}; // clear the output
    if (ags_inputevent_ready() != kInputTouch)
        return false; // there was no touch event

    const SDL_Event t_evt = ags_get_next_inputevent();
    if (t_evt.type == AGS_SDL_EVT_TOUCHDOWN || t_evt.type == AGS_SDL_EVT_TOUCHUP || t_evt.type == AGS_SDL_EVT_TOUCHMOTION)
    {
        const AGS_TouchPointerEventData *data = static_cast<AGS_TouchPointerEventData*>(t_evt.user.data1);
        out_touch.PointerID = data->PointerID;
        out_touch.Position = Mouse::SysToGamePos(data->Position);
        switch (t_evt.type)
        {
        case AGS_SDL_EVT_TOUCHDOWN: out_touch.Phase = TouchPhase::Down; break;
        case AGS_SDL_EVT_TOUCHUP: out_touch.Phase = TouchPhase::Up; break;
        case AGS_SDL_EVT_TOUCHMOTION: out_touch.Phase = TouchPhase::Motion; break;
        default: assert(false); break; // must not happen
        }
        ags_dispose_userevent(t_evt);
    }
    return out_touch.PointerID >= 0;
}

static void check_touch_controls()
{
    TouchInput ti;
    if (!run_service_touch_controls(ti))
        return;

    // TODO: check cutscene skip
    if (play.fast_forward)
        return;
    if (play.IsIgnoringInput())
        return;

    // Run script callbacks
    switch (ti.Phase)
    {
    case TouchPhase::Down:
        debug_script_log("Running on_pointer_down pointer id %d, pos %d,%d", ti.PointerID, ti.Position.X, ti.Position.Y);
        setevent(AGSEvent_Script(kTS_PointerDown, ti.PointerID, ti.Position.X, ti.Position.Y));
        break;
    case TouchPhase::Up:
        debug_script_log("Running on_pointer_up pointer id %d, pos %d,%d", ti.PointerID, ti.Position.X, ti.Position.Y);
        setevent(AGSEvent_Script(kTS_PointerUp, ti.PointerID, ti.Position.X, ti.Position.Y));
        break;
    case TouchPhase::Motion:
        debug_script_log("Running on_pointer_move pointer id %d, pos %d,%d", ti.PointerID, ti.Position.X, ti.Position.Y);
        setevent(AGSEvent_Script(kTS_PointerMove, ti.PointerID, ti.Position.X, ti.Position.Y));
        break;
    default:
        break;
    }
}

// Process player input
static void GameUpdateCheckControls()
{
    // don't let the player do anything before the screen fades in
    if (in_new_room != 0)
        return;

    set_our_eip(1007);

    sys_evt_process_pending();

    // First handle mouse state, which does not depend on down/up events
    // (motion, wheel axis, etc)
    // FIXME: atm we must save the last mouse_on_iface value *locally* for use
    // further in check_mouse_controls, because there may be 1+ nested
    // check_controls() calls as a result of some triggered script callbacks,
    // during which some global vars like mouse_on_iface may change...
    // need to rewrite all this interface interaction ugliness!
    int was_mouse_on_iface;
    check_mouse_state(was_mouse_on_iface); // NOTE: this also polls mousewheel

    // Handle all the buffered input events
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    {
        switch (type)
        {
            case kInputKeyboard:
                check_keyboard_controls();
                break;
            case kInputMouse:
                check_mouse_controls(was_mouse_on_iface);
                break;
            case kInputGamepad:
                check_gamepad_controls();
                break;
            case kInputTouch:
                check_touch_controls();
                break;
            default:
                ags_drop_next_inputevent();
                break;
        }
    }
}

// Check room edges triggers
static void GameUpdateCheckRoomEdges()
{
    if ((IsInterfaceEnabled()) && (IsGamePaused() == 0) && ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0)
        && (in_new_room == kEnterRoom_None) && (new_room_was == kEnterRoom_None))
    {
        // Only allow walking off edges if not in wait mode, and
        // if not in Player Enters Screen (allow walking in from off-screen)
        int edgesActivated[4] = {0, 0, 0, 0};
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

        for (int ii = 0; ii < 4; ii++)
        {
            if (edgesActivated[ii])
                setevent(AGSEvent_Object(kObjEventType_Room, 0, ii));
        }
    }
}

static void game_loop_update_background_animation()
{
    if (play.bg_anim_delay > 0) play.bg_anim_delay--;
    else if (play.bg_frame_locked);
    else {
        play.bg_anim_delay = play.anim_background_speed;
        play.bg_frame++;
        if ((size_t)play.bg_frame >= thisroom.BgFrameCount)
            play.bg_frame = 0;
        if (thisroom.BgFrameCount >= 2) {
            // get the new frame's palette
            on_background_frame_change();
        }
    }
}

static void game_loop_update_game_counters()
{
    if (play.wait_counter > 0) play.wait_counter--;
    if (play.shakesc_length > 0) play.shakesc_length--;
}

static void GameUpdateGameState()
{
    if ((debug_flags & DBG_NOUPDATE) == 0)
    {
        if (game_paused == 0)
        {
            update_game_objects();
            game_loop_update_background_animation();
        }
    }

    // Following global counters are updated during game pause too
    game_loop_update_game_counters();
}

// Updated objects that do not auto-pause when the game is paused
static void GameUpdatePersistentAnimations()
{
    // Update animating GUI buttons and overlays
    // The buttons are animated even if the game is paused
    for (size_t i = 0; i < GetAnimatingButtonCount(); ++i)
    {
        if (!UpdateAnimatingButton(i))
        {
            StopButtonAnimation(i);
            i--;
        }
    }

    UpdateOverlayAnimations();
}

extern std::vector<ViewStruct> views;

static void update_objects_scale()
{
    for (uint32_t objid = 0; objid < croom->numobj; ++objid)
    {
        update_object_scale(objid);
    }

    for (int charid = 0; charid < game.numcharacters; ++charid)
    {
        update_character_scale(charid);
    }
}

static void update_overlay_positions()
{
    auto &overs = get_overlays();
    for (auto &over : overs)
    {
        if (over.IsAutoPosition())
        {
            autoposition_overlay(over);
        }
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
        gui.Poll(mousex, mousey);
    }
}

extern int lastmx, lastmy;
extern int mouse_frame, mouse_delay;

static void update_cursor_view()
{
    // update animating mouse cursor
    const auto &mcur = game.mcurs[cur_cursor];
    if (mcur.view >= 0 && mcur.view < game.numviews)
    {
        // only on mousemove, and it's not moving
        if (((mcur.flags & MCF_ANIMMOVE) != 0) &&
            (mousex == lastmx) && (mousey == lastmy));
        // only on hotspot, and it's not on one
        else if (((mcur.flags & MCF_HOTSPOT) != 0) &&
            (GetLocationType(mousex, mousey) == 0))
            set_new_cursor_graphic(mcur.pic);
        else if (mouse_delay>0) mouse_delay--;
        // only animate if the loop 0 exists and has frames
        else if (views[mcur.view].numLoops > 0 && views[mcur.view].loops[0].numFrames > 0)
        {
            const int viewnum = mcur.view;
            const int loopnum = 0;
            mouse_frame++;
            if (mouse_frame >= views[viewnum].loops[loopnum].numFrames)
                mouse_frame = 0;
            set_new_cursor_graphic(views[viewnum].loops[loopnum].frames[mouse_frame].pic);
            mouse_delay = views[viewnum].loops[loopnum].frames[mouse_frame].speed + mcur.animdelay;
            CheckViewFrame(viewnum, loopnum, mouse_frame);
        }
        lastmx = mousex; lastmy = mousey;
    }
}

// Update the "saved cursor until it leaves location" state
static void UpdateSavedCursorOverLocation()
{
    // Call GetLocationName - it will internally force a GUI refresh
    // if the result it returns has changed from last time
    // CHECKME: this is also likely called in the main game update function,
    // so it may be not necessary here.
    GetLocationName(mousex, mousey);

    if ((play.get_loc_name_save_cursor >= 0) &&
        (play.get_loc_name_save_cursor != play.get_loc_name_last_time) &&
        (mouse_on_iface < 0) && (ifacepopped < 0)) {
        // we have saved the cursor, but the mouse location has changed
        // and it's time to restore it
        play.get_loc_name_save_cursor = kSavedLocType_Undefined;
        set_cursor_mode(play.restore_cursor_mode_to);

        if (cur_mode == play.restore_cursor_mode_to)
        {
            // make sure it changed -- the new mode might have been disabled
            // in which case don't change the image
            set_cursor_look(play.restore_cursor_image_to);
        }
        debug_script_log("Restore mouse to mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
    }
}

// Assign GUI context parameters, read from the game state.
// This will be required for GUI labels render.
static void UpdateGUIContext(int mwasatx, int mwasaty)
{
    if (play.fast_forward)
        return;
    if (displayed_room < 0)
        return;

    // It's easier to update these every game tick, this is simple data and copy is fast,
    // OTOH it may be inconvenient to have this in every place where this data changes.
    GUI::Context.GameTitle = play.game_name;

    if (playerchar->activeinv < 1 || playerchar->activeinv >= MAX_INV)
        GUI::Context.InventoryPic = -1;
    else
        GUI::Context.InventoryPic = game.invinfo[playerchar->activeinv].pic;

    // Update a location name for the GUI labels. We do this every game
    // tick, because this does not depend only on cursor position, but also
    // on game object positions, and game state changes, and we cannot track all of that here.
    //
    // While game is in Wait mode, or in room transition: set empty overhotspot text.
    if (!IsInterfaceEnabled() || in_room_transition)
        GUI::Context.Overhotspot = "";
    // Games prior to 3.6.0 had a slightly different order of updates, so use old cursor pos for them
    else if (loaded_game_file_version < kGameVersion_360_21)
        GUI::Context.Overhotspot = GetLocationName(mwasatx, mwasaty);
    else
        GUI::Context.Overhotspot = GetLocationName(mousex, mousey);
}

// Detect mouse move over hotspot, and run respective event if necessary
static void update_cursor_over_location(int mwasatx, int mwasaty)
{
    if (play.fast_forward)
        return;
    if (displayed_room < 0)
        return;

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
        if (__GetLocationType(mousex, mousey, 1) == LOCTYPE_HOTSPOT)
        {
            int onhs = getloctype_index;

            setevent(AGSEvent_Object(kObjEventType_Hotspot, onhs, kHotspotEvent_MouseOver));
        }
    }

    offsetxWas = offsetx;
    offsetyWas = offsety;
}

static void UpdateDrawableObjectStates(bool do_cursor, int mwasatx, int mwasaty)
{
    if (displayed_room < 0)
        return;

    // camera positions may be linked to a player character
    play.UpdateRoomCameras();

    update_objects_scale();
    // overlay positions may be linked to a certain character
    update_overlay_positions();
    if (do_cursor)
    {
        update_cursor_over_location(mwasatx, mwasaty);
        update_cursor_view();
    }
}

// Process all events scheduled during the last game update
static void GameUpdateProcessEvents()
{
    new_room_was = in_new_room;
    // If we're in the new room (after "room load" event), then schedule "fade in" event,
    // it will be processed right away
    if (in_new_room != kEnterRoom_None)
    {
        setevent({ kAGSEvent_FadeIn });
    }
    in_new_room = kEnterRoom_None;

    processallevents();

    if ((new_room_was != kEnterRoom_None) && (in_new_room == kEnterRoom_None))
    {
        // if in a new room, and the room wasn't just changed again in update_events,
        // then queue the Enters Screen scripts run these next time round, when it's faded in
        switch (new_room_was)
        {
        case kEnterRoom_FirstTime: // first time enters screen
            setevent(AGSEvent_Object(kObjEventType_Room, 0, kRoomEvent_FirstEnter));
            /* fall-through */
        case kEnterRoom_Normal: // enters screen after fadein
            setevent(AGSEvent_Object(kObjEventType_Room, 0, kRoomEvent_AfterFadein));
            break;
        case kEnterRoom_RestoredSave:
            in_room_transition = false; // room transition ends here
            break;
        }
    }
}

static void game_loop_update_loop_counter()
{
    GameTimer.LoopCounter++;
}

static void game_loop_update_fps()
{
    const auto now = Clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - GameTimer.LastFPSSyncTime);
    const auto frames = GameTimer.LoopCounter - GameTimer.LastFPSSyncCounter;

    if (duration >= std::chrono::milliseconds(GameTimer.FPSSyncPeriodMs) && frames > 0)
    {
        GameTimer.FPS = 1000.0f * frames / duration.count();
        GameTimer.LastFPSSyncTime = now;
        GameTimer.LastFPSSyncCounter = GameTimer.LoopCounter;
    }
}

float get_game_fps()
{
    // if we have maxed out framerate then return the frame rate we're seeing instead
    // fps must be greater that 0 or some timings will take forever.
    // TODO: review this, consider always returning fixed property here,
    // and use get_real_fps() whenever we need real time;
    // but must double check all situations where this function is used!
    if (isTimerFpsMaxed() && GameTimer.FPS > 0.0f)
    {
        return GameTimer.FPS;
    }
    return frames_per_second;
}

float get_real_fps()
{
    return GameTimer.FPS;
}

void set_loop_counter(uint32_t new_counter)
{
    GameTimer.LoopCounter = new_counter;
    GameTimer.LastFPSSyncCounter = GameTimer.LoopCounter;
    GameTimer.LastFPSSyncTime = Clock::now();
    GameTimer.FPS = std::numeric_limits<float>::quiet_NaN();
}

void increment_loop_counter()
{
    GameTimer.LoopCounter++;
    GameTimer.LastFPSSyncCounter = GameTimer.LoopCounter;
}

uint32_t get_loop_counter()
{
    return GameTimer.LoopCounter;
}

void set_runtime_start()
{
    GameTimer.StartTime = Clock::now();
    GameTimer.RuntimeStarted = true;
}

bool is_runtime_set()
{
    return GameTimer.RuntimeStarted;
}

uint32_t get_runtime_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - GameTimer.StartTime).count();
}

//
// UpdateGameOnce is a primary game update routine.
//
// Plan:
//  * System events (parse and record for further use down the routine).
//  * Check for exit request.
//  * Early bad state check
//  * New room check
//  * repeatedly_execute_always
//  * Game state general update
//  * Player input handling
//  * late_repeatedly_execute_always
//  * Process scheduled game events
//  * Sync video & audio, update drawable objects for render
//  * Rendering
//  * Loop counter increment
//  * Wait for next frame
//
void UpdateGameOnce(bool do_controls, AGS::Engine::IDriverDependantBitmap *extra_ddb, int extra_x, int extra_y)
{
    set_our_eip(1000);

    sys_evt_process_pending();

    if (want_exit)
    {
        ProperExit();
        return;
    }

    set_our_eip(1001);

    GameUpdateCheckProblems();

    set_our_eip(1002);

    // Check "breakpoint" key prior to running any script callbacks
    check_debug_keys();
    // Tell scripts that the game is alive
    ccNotifyScriptStillAlive();
    // Check if we just entered a new room, and run "before fade-in" script event
    check_new_room();

    set_our_eip(1003);

    // Run early rep-exec-always, and schedule rep-execs for later
    GameUpdateEarlyRepExec();

    set_our_eip(1004);

    // Do the overall game state update
    GameUpdateGameState();
    GameUpdatePersistentAnimations();
    // Check edges and ground interactions, after the game objects have updated their positions
    GameUpdateCheckRoomEdges();
    GameUpdateCheckGroundInteractions();

    set_our_eip(1005);

    // Handle player's input
    // remember old mouse pos, needed for update_cursor_over_location() later
    const int mwasatx = mousex, mwasaty = mousey;
    mouse_on_iface = -1; // FIXME: why is this here? move to a related update function!
    // update mouse position (mousex, mousey)
    ags_domouse();
    // update gui under mouse; this also updates gui control focus;
    // atm we must call this before "check_controls", because GUI interaction
    // relies on remembering which control was focused by the cursor prior
    update_gui_disabled_status();
    update_cursor_over_gui();
    UpdateSavedCursorOverLocation();
    // handle actual input (keys, mouse, and so forth)
    if (do_controls)
    {
        GameUpdateCheckControls();
    }

    set_our_eip(1006);

    // Run late rep-exec-always, after all the game was updated
    GameUpdateLateRepExec();
    // Then process all the accumulated events for this game tick
    GameUpdateProcessEvents();

    set_our_eip(1007);

    // Sync video and audio with the game logic
    update_video_system_on_game_loop();
    update_audio_system_on_game_loop();
    // Update drawable object states, which depend on their positions,
    // room region properties, or other object positions
    UpdateDrawableObjectStates(true /* cursor-related update */, mwasatx, mwasaty);

    set_our_eip(1008);

    // Record necessary values for the GUI rendering
    UpdateGUIContext(mwasatx, mwasaty);
    // Only render if we are not skipping a cutscene
    if (!play.fast_forward)
    {
        update_gui_disabled_status(); // in case they changed it in the late script update
        render_graphics(extra_ddb, extra_x, extra_y);
    }

    set_our_eip(1009);

    game_loop_update_loop_counter();
    update_polled_stuff();
    game_loop_update_fps();

    set_our_eip(1010);

    // Immediately start the next frame if we are skipping a cutscene
    if (play.fast_forward)
        return;

    set_our_eip(1011);

    WaitForNextFrame();

    set_our_eip(1012);
}

void UpdateGameAudioOnly()
{
    update_audio_system_on_game_loop();
    game_loop_update_loop_counter();
    game_loop_update_fps();
    WaitForNextFrame();
}

bool IsInBlockingAction()
{
    return restrict_until != nullptr;
}

bool IsInWaitRunFromScript()
{
    return restrict_until_in_script;
}

// Checks if wait mode should continue until condition is met
// FIXME: should be a private method of GameLoopUntilState,
// but is called elsewhere for some strange reason;
// investigate and move to GameLoopUntilState.
static bool ShouldStayInWaitMode()
{
    assert(restrict_until);
    if (!restrict_until)
        return false;

    switch (restrict_until->GetUntilType())
    {
    case UNTIL_MOVEEND:
    {
        short*wkptr = (short*)restrict_until->GetDataPtr();
        return !(wkptr[0] < 1);
    }
    case UNTIL_CHARIS0:
    {
        char*chptr = (char*)restrict_until->GetDataPtr();
        return !(chptr[0] == 0);
    }
    case UNTIL_NEGATIVE:
    {
        short*wkptr = (short*)restrict_until->GetDataPtr();
        return !(wkptr[0] < 0);
    }
    case UNTIL_INTISNEG:
    {
        int*wkptr = (int*)restrict_until->GetDataPtr();
        return !(wkptr[0] < 0);
    }
    case UNTIL_NOOVERLAY:
    {
        return !(play.text_overlay_on == 0);
    }
    case UNTIL_INTIS0:
    {
        int*wkptr = (int*)restrict_until->GetDataPtr();
        return !(wkptr[0] == 0);
    }
    case UNTIL_SHORTIS0:
    {
        short*wkptr = (short*)restrict_until->GetDataPtr();
        return !(wkptr[0] == 0);
    }
    case UNTIL_ANIMBTNEND:
    {  // still animating?
        return FindButtonAnimation(restrict_until->GetData1(), restrict_until->GetData2()) >= 0;
    }
    case UNTIL_FLAGUNSET:
    {
        const int *bitset = static_cast<const int*>(restrict_until->GetDataPtr());
        return ((*bitset) & restrict_until->GetData1()) != 0;
    }
    case UNTIL_VIEWANIM:
    {
        const ViewAnimateParams *anim = static_cast<const ViewAnimateParams*>(restrict_until->GetDataPtr());
        return anim->IsValid();
    }
    case UNTIL_ANIMOVEREND:
    {
        return IsOverlayAnimating(restrict_until->GetData1());
    }
    default:
        debug_script_warn("loop_until: unknown until event, aborting");
        return false;
    }
}

// Run single game iteration; calls UpdateGameOnce() internally
static void GameTick()
{
    UpdateGameOnce(true);
}

// This function is called from lot of various functions
// in the game core, character, room object etc
static void GameLoopUntilEvent(int untilwhat, const void* data_ptr = nullptr, int data1 = 0, int data2 = 0) {
    // blocking cutscene - end skipping
    EndSkippingUntilCharStops();

    // this function can get called in a nested context, so
    // remember the state of these vars in case a higher level
    // call needs them
    std::unique_ptr<GameLoopUntilState> cached_restrict_until = std::move(restrict_until);
    bool cached_restrict_until_in_script = restrict_until_in_script;

    restrict_until_in_script = is_inside_script();
    restrict_until.reset(new GameLoopUntilState(untilwhat, data_ptr, data1, data2));
    restrict_until->Begin();
    while (restrict_until->Run());
    restrict_until->End();

    set_our_eip(78);

    restrict_until = std::move(cached_restrict_until);
    restrict_until_in_script = cached_restrict_until_in_script;
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

void GameLoopUntilOverlayAnimEnd(int over_id)
{
    GameLoopUntilEvent(UNTIL_ANIMOVEREND, nullptr, over_id);
}

void GameLoopUntilFlagUnset(const int *flagset, int flagbit)
{
    GameLoopUntilEvent(UNTIL_FLAGUNSET, flagset, flagbit);
}

void GameLoopUntilViewAnimEnd(const ViewAnimateParams *anim)
{
    GameLoopUntilEvent(UNTIL_VIEWANIM, anim);
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
    // TODO: following does not have to be called every frame while in a
    // fully blocking state (like Display() func), refactor to only call it
    // once the blocking state begins.
    UpdateDrawableObjectStates(true /* cursor-related update */, mwasatx, mwasaty);
}

void SyncDrawablesState()
{
    UpdateDrawableObjectStates(false /* NO cursor-related update */, -1, -1);
}

void ShutGameWaitState()
{
    restrict_until = {};
    restrict_until_in_script = false;
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

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
#include "ac/global_game.h"
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include "core/platform.h"
#include "ac/audiocliptype.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_gui.h"
#include "ac/global_hotspot.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/path_helper.h"
#include "ac/sys_events.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/view.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "gui/guidialog.h"
#include "main/engine.h"
#include "main/game_start.h"
#include "main/game_run.h"
#include "main/graphics_mode.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "core/assetmanager.h"
#include "main/config.h"
#include "main/game_file.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string_utils.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern ExecutingScript*curscript;
extern int displayed_room;
extern int game_paused;
extern SpriteCache spriteset;
extern unsigned int load_new_game;
extern int load_new_game_restore;
extern GameSetupStruct game;
extern std::vector<ViewStruct> views;
extern RoomStatus*croom;
extern RoomStruct thisroom;
extern int getloctype_index;
extern IGraphicsDriver *gfxDriver;
extern RGB palette[256];


void AbortGame()
{
    // make sure scripts stop at the next step
    cancel_all_scripts();
}

void restart_game() {
    can_run_delayed_command();
    if (inside_script) {
        curscript->QueueAction(PostScriptAction(ePSARestartGame, 0, "RestartGame"));
        return;
    }
    try_restore_save(RESTART_POINT_SAVE_GAME_NUMBER);
}

void CopySaveSlot(int old_save, int new_save)
{
    if (old_save == new_save)
        return; // cannot copy into itself

    String old_filename = get_save_game_path(old_save);
    String new_filename = get_save_game_path(new_save);
    File::CopyFile(old_filename, new_filename, true);
}

void MoveSaveSlot(int old_save, int new_save)
{
    if (old_save == new_save)
        return; // cannot move into itself

    String old_filename = get_save_game_path(old_save);
    String new_filename = get_save_game_path(new_save);
    File::RenameFile(old_filename, new_filename);
}

void RestoreGameSlot(int slnum)
{
    if (displayed_room < 0)
        quit("!RestoreGameSlot: a game cannot be restored from within game_start");

    can_run_delayed_command();
    if (inside_script) {
        curscript->QueueAction(PostScriptAction(ePSARestoreGame, slnum, "RestoreGameSlot"));
        return;
    }
    try_restore_save(slnum);
}

void SaveGameSlot(int slotn, const char *descript, int spritenum)
{
    VALIDATE_STRING(descript);

    if (platform->GetDiskFreeSpaceMB(get_save_game_directory()) < 2)
    {
        Display("ERROR: There is not enough disk space free to save the game. Clear some disk space and try again.");
        return;
    }

    // dont allow save in rep_exec_always, because we dont save
    // the state of blocked scripts
    can_run_delayed_command();

    // Make a sprite copy, as save process may be scheduled and asynchronous (in theory)
    std::unique_ptr<Bitmap> image;
    if (spritenum >= 0)
        image.reset(BitmapHelper::CreateBitmapCopy(spriteset[spritenum]));

    if (inside_script)
    {
        curscript->QueueAction(PostScriptAction(ePSASaveGame, slotn, "SaveGameSlot", descript, std::move(image)));
        return;
    }

    save_game(slotn, descript, std::move(image));
}

void SaveGameSlot2(int slnum, const char *descript)
{
    SaveGameSlot(slnum, descript, -1);
}

void DeleteSaveSlot(int slnum)
{
    String save_filename = get_save_game_path(slnum);
    File::DeleteFile(save_filename);
}

void PauseGame() {
    game_paused++;
    debug_script_log("Game paused (%d)", game_paused);
}
void UnPauseGame() {
    if (game_paused > 0)
    {
        game_paused--;
        debug_script_log("Game un-paused (%d)", game_paused);
    }
}


int IsGamePaused() {
    if (game_paused>0) return 1;
    return 0;
}

bool GetSaveSlotDescription(int slnum, String &description) {
    if (read_savedgame_description(get_save_game_path(slnum), description))
        return true;
    description.Format("INVALID SLOT %d", slnum);
    return false;
}

// CLNUP check this, used by ListBox_FillSaveGameList, but Game_GetSaveSlotDescription exist too
int GetSaveSlotDescription(int slnum, char *desbuf) {
    VALIDATE_STRING(desbuf);
    String description;
    bool res = GetSaveSlotDescription(slnum, description);
    snprintf(desbuf, MAX_MAXSTRLEN, "%s", description.GetCStr());
    return res ? 1 : 0;
}

void FillSaveList(std::vector<SaveListItem> &saves, unsigned bot_index, unsigned top_index, size_t max_count)
{
    if (max_count == 0)
        return; // duh

    String svg_dir = get_save_game_directory();
    String svg_suff = get_save_game_suffix();
    String pattern = String::FromFormat("agssave.???%s", svg_suff.GetCStr());
    bot_index = std::min(999u, bot_index); // NOTE: slots are limited by 0..999 range
    top_index = std::min(999u, top_index);

    for (FindFile ff = FindFile::OpenFiles(svg_dir, pattern); !ff.AtEnd(); ff.Next())
    {
        String slotname = ff.Current();
        if (!svg_suff.IsEmpty())
            slotname.ClipRight(svg_suff.GetLength());
        int saveGameSlot = Path::GetFileExtension(slotname).ToInt();
        // only list games between .XXX to .YYY (to allow hidden slots for special perposes)
        if (saveGameSlot < 0 || static_cast<unsigned>(saveGameSlot) < bot_index
            || static_cast<unsigned>(saveGameSlot) > top_index)
            continue;
        String description;
        GetSaveSlotDescription(saveGameSlot, description);
        saves.push_back(SaveListItem(saveGameSlot, description, ff.GetEntry().Time));
        if (saves.size() >= max_count)
            break;
    }
}

int GetLastSaveSlot()
{
    std::vector<SaveListItem> saves;
    FillSaveList(saves, 0, RESTART_POINT_SAVE_GAME_NUMBER - 1);
    if (saves.size() == 0)
        return -1;
    std::sort(saves.rbegin(), saves.rend());
    return saves[0].Slot;
}

// TODO: refactor this method, and use same shared procedure at both normal stop/startup and in RunAGSGame
int RunAGSGame(const String &newgame, unsigned int mode, int data) {

    can_run_delayed_command();

    int AllowedModes = RAGMODE_LOADNOW;

    if ((mode & (~AllowedModes)) != 0)
        quit("!RunAGSGame: mode value unknown");

    if (editor_debugging_initialized)
    {
        quit("!RunAGSGame cannot be used while running the game from within the AGS Editor. You must build the game EXE and run it from there to use this function.");
    }

    if ((mode & RAGMODE_LOADNOW) == 0) {
        ResPaths.GamePak.Path = PathFromInstallDir(newgame);
        ResPaths.GamePak.Name = newgame;
        play.takeover_data = data;
        load_new_game_restore = -1;

        if (inside_script) {
            curscript->QueueAction(PostScriptAction(ePSARunAGSGame, mode | RAGMODE_LOADNOW, "RunAGSGame"));
            ccInstance::GetCurrentInstance()->Abort();
        }
        else
            load_new_game = mode | RAGMODE_LOADNOW;

        return 0;
    }

    unload_old_room();
    displayed_room = -10;

#if defined (AGS_AUTO_WRITE_USER_CONFIG)
    save_config_file(); // save current user config in case engine fails to run new game
#endif // AGS_AUTO_WRITE_USER_CONFIG
    unload_game();

    // Adjust config (NOTE: normally, RunAGSGame would need a redesign to allow separate config etc per each game)
    usetup.translation = ""; // reset to default, prevent from trying translation file of game A in game B

    AssetMgr->RemoveAllLibraries();

    // TODO: refactor and share same code with the startup!
    if (AssetMgr->AddLibrary(ResPaths.GamePak.Path) != Common::kAssetNoError)
        quitprintf("!RunAGSGame: unable to load new game file '%s'", ResPaths.GamePak.Path.GetCStr());
    engine_assign_assetpaths();

    show_preload();

    HError err = load_game_file();
    if (!err)
        quitprintf("!RunAGSGame: error loading new game file:\n%s", err->FullMessage().GetCStr());

    err = engine_init_sprites();
    if (!err)
        quitprintf("!RunAGSGame: error loading new sprites:\n%s", err->FullMessage().GetCStr());

    engine_init_game_settings();
    play.screen_is_faded_out = 1;

    if (load_new_game_restore >= 0) {
        try_restore_save(load_new_game_restore, true);
        load_new_game_restore = -1;
    }
    else
        start_game();

    return 0;
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
    newspd += play.game_speed_modifier;
    if (newspd>1000) newspd=1000;
    if (newspd<10) newspd=10;
    set_game_speed(newspd);
    debug_script_log("Game speed set to %d", newspd);
}

int GetGameSpeed() {
    // TODO: consider return strictly logical game fps;
    // need to investigate what would be consequence of this in "infinite FPS" mode
    return ::lround(get_game_fps()) - play.game_speed_modifier;
}

int SetGameOption (int opt, int newval) {
    if (((opt < OPT_DEBUGMODE) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    {
        debug_script_warn("SetGameOption: invalid option specified: %d", opt);
        return 0;
    }

    // Handle forbidden options
    const auto restricted_opts = GameSetupStructBase::GetRestrictedOptions();
    for (auto r_opt : restricted_opts)
    {
        if (r_opt == opt)
        {
            debug_script_warn("SetGameOption: option %d cannot be modified at runtime", opt);
            return game.options[opt];
        }
    }

    // Test if option already has this value
    if (game.options[opt] == newval)
        return game.options[opt];

    const int oldval = game.options[opt];
    game.options[opt] = newval;

    // Update the game in accordance to the new option value
    switch (opt)
    {
    case OPT_ANTIGLIDE:
        for (int i = 0; i < game.numcharacters; i++)
        {
            if (newval)
                game.chars[i].flags |= CHF_ANTIGLIDE;
            else
                game.chars[i].flags &= ~CHF_ANTIGLIDE;
        }
        break;
    case OPT_DISABLEOFF:
        GUI::Options.DisabledStyle = static_cast<GuiDisableStyle>(game.options[OPT_DISABLEOFF]);
        // If GUI was disabled at this time then also update it, as visual style could've changed
        if (play.disabled_user_interface > 0) { GUIE::MarkAllGUIForUpdate(true, false); }
        break;
    case OPT_ANTIALIASFONTS:
        adjust_fonts_for_render_mode(newval != 0);
        break;
    case OPT_RIGHTLEFTWRITE:
        GUIE::MarkForTranslationUpdate();
        break;
    case OPT_DUPLICATEINV:
        update_invorder();
        break;
    case OPT_PORTRAITSIDE:
        if (newval == 0)  // set back to Left
            play.swap_portrait_side = 0;
        break;
    default:
        break; // do nothing else
    }

    return oldval;
}

int GetGameOption (int opt) {
    if (((opt < OPT_DEBUGMODE) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    {
        debug_script_warn("GetGameOption: invalid option specified: %d", opt);
        return 0;
    }

    return game.options[opt];
}

void SkipUntilCharacterStops(int cc) {
    if (!is_valid_character(cc))
        quit("!SkipUntilCharacterStops: invalid character specified");
    if (game.chars[cc].room!=displayed_room)
        quitprintf("!SkipUntilCharacterStops: character %s is not in current room %d (it is in room %d)",
            game.chars[cc].scrname.GetCStr(), displayed_room, game.chars[cc].room);

    // if they are not currently moving, do nothing
    if (!game.chars[cc].walking)
        return;

    if (is_in_cutscene())
        quit("!SkipUntilCharacterStops: cannot be used within a cutscene");

    initialize_skippable_cutscene();
    play.fast_forward = 2;
    play.skip_until_char_stops = cc;
}

void EndSkippingUntilCharStops() {
    // not currently skipping, so ignore
    if (play.skip_until_char_stops < 0)
        return;

    stop_fast_forwarding();
    play.skip_until_char_stops = -1;
}

void StartCutscene (int skipwith) {
    static ScriptPosition last_cutscene_script_pos;

    if (is_in_cutscene()) {
        quitprintf("!StartCutscene: already in a cutscene; previous started in \"%s\", line %d",
            last_cutscene_script_pos.Section.GetCStr(), last_cutscene_script_pos.Line);
    }

    if ((skipwith < 1) || (skipwith > 6))
        quit("!StartCutscene: invalid argument, must be 1 to 5.");

    get_script_position(last_cutscene_script_pos);

    // make sure they can't be skipping and cutsceneing at the same time
    EndSkippingUntilCharStops();

    play.in_cutscene = skipwith;
    initialize_skippable_cutscene();
}

void SkipCutscene()
{
    if (is_in_cutscene())
        start_skipping_cutscene();
}

int EndCutscene () {
    if (!is_in_cutscene())
        quit("!EndCutscene: not in a cutscene");

    int retval = play.fast_forward;
    play.in_cutscene = 0;
    // Stop it fast-forwarding
    stop_fast_forwarding();

    // make sure that the screen redraws
    invalidate_screen();

    // Return whether the player skipped it
    return retval;
}

void ShowInputBox(const char*msg, char*bufr) {
    VALIDATE_STRING(bufr);
    ShowInputBoxImpl(msg, bufr, MAX_MAXSTRLEN);
}

void ShowInputBoxImpl(const char*msg, char *bufr, size_t buf_len) {
    setup_for_dialog();
    enterstringwindow(get_translation(msg), bufr, buf_len);
    restore_after_dialog();
}

// GetLocationType exported function - just call through
// to the main function with default 0
int GetLocationType(int xxx,int yyy) {
    return __GetLocationType(xxx, yyy, 0);
}

void SaveCursorForLocationChange() {
    // update the current location name
    char tempo[100];
    GetLocationName(mousex, mousey, tempo);

    if (play.get_loc_name_save_cursor != play.get_loc_name_last_time) {
        play.get_loc_name_save_cursor = play.get_loc_name_last_time;
        play.restore_cursor_mode_to = GetCursorMode();
        play.restore_cursor_image_to = GetMouseCursor();
        debug_script_log("Saving mouse: mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
    }
}

void GetLocationName(int xxx,int yyy,char*tempo)
{
    VALIDATE_STRING(tempo);
    tempo[0] = 0;

    if (displayed_room < 0)
        return; // no room loaded yet

    if (GetGUIAt(xxx, yyy) >= 0) {
        int mover = GetInvAt (xxx, yyy);
        if (mover > 0) {
            if (play.get_loc_name_last_time != 1000 + mover)
                GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
            play.get_loc_name_last_time = 1000 + mover;
            snprintf(tempo, MAX_MAXSTRLEN, "%s", get_translation(game.invinfo[mover].name.GetCStr()));
        }
        else if ((play.get_loc_name_last_time > 1000) && (play.get_loc_name_last_time < 1000 + MAX_INV)) {
            // no longer selecting an item
            GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
            play.get_loc_name_last_time = -1;
        }
        return;
    }

    int loctype = GetLocationType(xxx, yyy); // GetLocationType takes screen coords
    VpPoint vpt = play.ScreenToRoom(xxx, yyy);
    if (vpt.second < 0)
        return;
    xxx = vpt.first.X;
    yyy = vpt.first.Y;
    if ((xxx>=thisroom.Width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.Height))
        return;

    int onhs,aa;
    if (loctype == 0) {
        if (play.get_loc_name_last_time != 0) {
            play.get_loc_name_last_time = 0;
            GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
        }
        return;
    }

    // on character
    if (loctype == LOCTYPE_CHAR) {
        onhs = getloctype_index;
        snprintf(tempo, MAX_MAXSTRLEN, "%s", get_translation(game.chars[onhs].name.GetCStr()));
        if (play.get_loc_name_last_time != 2000+onhs)
            GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
        play.get_loc_name_last_time = 2000+onhs;
        return;
    }
    // on object
    if (loctype == LOCTYPE_OBJ) {
        aa = getloctype_index;
        snprintf(tempo, MAX_MAXSTRLEN, "%s", get_translation(croom->obj[aa].name.GetCStr()));
        if (play.get_loc_name_last_time != 3000+aa)
            GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
        play.get_loc_name_last_time = 3000+aa;
        return;
    }
    onhs = getloctype_index;
    if (onhs>0)
        snprintf(tempo, MAX_MAXSTRLEN, "%s", get_translation(croom->hotspot[onhs].Name.GetCStr()));
    if (play.get_loc_name_last_time != onhs)
        GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
    play.get_loc_name_last_time = onhs;
}

int IsKeyPressed (int keycode) {
    return ags_iskeydown(static_cast<eAGSKeyCode>(keycode));
}

int SaveScreenShot(const char*namm) {
    String svg_dir = get_save_game_directory();
    String ext = Path::GetFileExtension(namm);
    String filename;
    if (ext.IsEmpty())
    {
        ext = "bmp";
        filename = Path::MakePath(svg_dir, namm, "bmp");
    }
    else
    {
        filename = Path::ConcatPaths(svg_dir, namm);
    }

    std::unique_ptr<Stream> out(File::OpenFileCI(filename, kFile_CreateAlways, kStream_Write));
    if (!out)
        return 0;

    // NOTE: be aware that by the historical logic AGS makes a screenshot
    // of a "main viewport", that may be smaller in legacy "letterbox" mode.
    const Rect &viewport = play.GetMainViewport();
    std::unique_ptr<Bitmap> bmp(CopyScreenIntoBitmap(viewport.GetWidth(), viewport.GetHeight(), &viewport));
    return BitmapHelper::SaveBitmap(bmp.get(), palette, out.get(), ext) ? 1 : 0;
}

void SetMultitasking (int mode) {
    if ((mode < 0) | (mode > 1))
        quit("!SetMultitasking: invalid mode parameter");
    // Save requested setting
    usetup.multitasking = mode != 0;

    // Account for the override config option (must be checked first!)
    if ((usetup.override_multitasking >= 0) && (mode != usetup.override_multitasking))
    {
        Debug::Printf("SetMultitasking: overridden by user config: %d -> %d",
            mode, usetup.override_multitasking);
        mode = usetup.override_multitasking;
    }

    // Must run on background if debugger is connected
    if ((mode == 0) && (editor_debugging_initialized != 0))
    {
        Debug::Printf("SetMultitasking: overridden by the external debugger: %d -> 1", mode);
        mode = 1;
    }

    // Regardless, don't allow background running if exclusive full screen
    if ((mode == 1) && gfxDriver && gfxDriver->GetDisplayMode().IsRealFullscreen())
    {
        Debug::Printf("SetMultitasking: overridden by fullscreen: %d -> 0", mode);
        mode = 0;
    }

    // Install engine callbacks for switching in and out the window
    Debug::Printf(kDbgMsg_Info, "Multitasking mode set: %d", mode);
    if (mode == 0)
    {
        sys_set_background_mode(false);
        sys_evt_set_focus_callbacks(display_switch_in_resume, display_switch_out_suspend);
    }
    else
    {
        sys_set_background_mode(true);
        sys_evt_set_focus_callbacks(display_switch_in, display_switch_out);
    }
}

extern int getloctype_throughgui, getloctype_index;

void RoomProcessClick(int xx,int yy,int mood) {
    getloctype_throughgui = 1;
    int loctype = GetLocationType (xx, yy);
    VpPoint vpt = play.ScreenToRoom(xx, yy);
    if (vpt.second < 0)
        return;
    xx = vpt.first.X;
    yy = vpt.first.Y;

    if ((mood==MODE_WALK) && (game.options[OPT_NOWALKMODE]==0)) {
        int hsnum=get_hotspot_at(xx,yy);
        if (hsnum<1) ;
        else if (thisroom.Hotspots[hsnum].WalkTo.X<1) ;
        else if (play.auto_use_walkto_points == 0) ;
        else {
            xx=thisroom.Hotspots[hsnum].WalkTo.X;
            yy=thisroom.Hotspots[hsnum].WalkTo.Y;
            debug_script_log("Move to walk-to point hotspot %d", hsnum);
        }
        move_character(playerchar, xx, yy, false /* ignwal */, true /* walk anim */);
        return;
    }
    play.usedmode=mood;

    if (loctype == 0) {
        // click on nothing -> hotspot 0
        getloctype_index = 0;
        loctype = LOCTYPE_HOTSPOT;
    }

    if (loctype == LOCTYPE_CHAR) {
        if (check_click_on_character(xx,yy,mood)) return;
    }
    else if (loctype == LOCTYPE_OBJ) {
        if (check_click_on_object(xx,yy,mood)) return;
    }
    else if (loctype == LOCTYPE_HOTSPOT) 
        RunHotspotInteraction (getloctype_index, mood);
}

int IsInteractionAvailable (int xx,int yy,int mood) {
    getloctype_throughgui = 1;
    int loctype = GetLocationType (xx, yy);
    VpPoint vpt = play.ScreenToRoom(xx, yy);
    if (vpt.second < 0)
        return 0;
    xx = vpt.first.X;
    yy = vpt.first.Y;

    // You can always walk places
    if ((mood==MODE_WALK) && (game.options[OPT_NOWALKMODE]==0))
        return 1;

    play.check_interaction_only = 1;

    if (loctype == 0) {
        // click on nothing -> hotspot 0
        getloctype_index = 0;
        loctype = LOCTYPE_HOTSPOT;
    }

    if (loctype == LOCTYPE_CHAR) {
        check_click_on_character(xx,yy,mood);
    }
    else if (loctype == LOCTYPE_OBJ) {
        check_click_on_object(xx,yy,mood);
    }
    else if (loctype == LOCTYPE_HOTSPOT)
        RunHotspotInteraction (getloctype_index, mood);

    int ciwas = play.check_interaction_only;
    play.check_interaction_only = 0;

    if (ciwas == 2)
        return 1;

    return 0;
}

void SetSpeechFont (int fontnum) {
    fontnum = ValidateFontNumber("SetSpeechFont", fontnum);
    play.speech_font = fontnum;
}

void SetNormalFont (int fontnum) {
    fontnum = ValidateFontNumber("SetNormalFont", fontnum);
    play.normal_font = fontnum;
}

void _sc_AbortGame(const char* text) {
    char displbuf[STD_BUFFER_SIZE] = "!?";
    snprintf(&displbuf[2], STD_BUFFER_SIZE - 3, "%s", text);
    quit(displbuf);
}

int WaitImpl(int skip_type, int nloops)
{
    // if skipping cutscene and expecting user input: don't wait at all
    if (play.fast_forward && ((skip_type & ~SKIP_AUTOTIMER) != 0))
        return 0;

    play.wait_counter = nloops;
    play.wait_skipped_by = SKIP_NONE;
    play.wait_skipped_by_data = 0;
    play.key_skip_wait = skip_type;

    GameLoopUntilValueIsZero(&play.wait_counter);

    if (game.options[OPT_BASESCRIPTAPI] < kScriptAPI_v360)
    {
        // < 3.6.0 return 1 is skipped by user input, otherwise 0
        return ((play.wait_skipped_by & (SKIP_KEYPRESS | SKIP_MOUSECLICK)) != 0) ? 1 : 0;
    }
    // >= 3.6.0 return skip (input) type flags with keycode
    return play.GetWaitSkipResult();
}

void scrWait(int nloops) {
    WaitImpl(SKIP_AUTOTIMER, nloops);
}

int WaitKey(int nloops) {
    return WaitImpl(SKIP_KEYPRESS | SKIP_AUTOTIMER, nloops);
}

int WaitMouse(int nloops) {
    return WaitImpl(SKIP_MOUSECLICK | SKIP_AUTOTIMER, nloops);
}

int WaitMouseKey(int nloops) {
    return WaitImpl(SKIP_KEYPRESS | SKIP_MOUSECLICK | SKIP_AUTOTIMER, nloops);
}

int WaitInput(int input_flags, int nloops) {
    return WaitImpl((input_flags >> SKIP_RESULT_TYPE_SHIFT) | SKIP_AUTOTIMER, nloops);
}

void SkipWait() {
    play.wait_counter = 0;
}

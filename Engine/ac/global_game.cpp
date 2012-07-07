#define USE_CLIB
#include "ac/global_game.h"
#include "wgt2allg.h"
#include "ac/ac_common.h"
#include "ac/ac_view.h"
#include "acmain/ac_maindefines.h"
#include "ac/character.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/file.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/gui.h"
#include "ac/roomstatus.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_inventory.h"
#include "acmain/ac_mouse.h"
#include "acmain/ac_room.h"
#include "acmain/ac_translation.h"
#include "debug/debug.h"
#include "gui/guidialog.h"
#include "main/engine.h"
#include "main/game_start.h"
#include "media/audio/audio.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "sprcache.h"

extern int guis_need_update;
extern GameState play;
extern ExecutingScript*curscript;
extern const char *load_game_errors[9];
extern int displayed_room;
extern int game_paused;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int frames_per_second;
extern int time_between_timers;
extern char gamefilenamebuf[200];
extern char* game_file_name;
extern GameSetup usetup;
extern unsigned int load_new_game;
extern int load_new_game_restore;
extern GameSetupStruct game;
extern ViewStruct*views;
extern RoomStatus*croom;
extern int gui_disabled_style;
extern roomstruct thisroom;
extern int getloctype_index;

extern "C" int csetlib(char *namm, char *passw);

void GiveScore(int amnt) 
{
    guis_need_update = 1;
    play.score += amnt;

    if ((amnt > 0) && (play.score_sound >= 0))
        play_audio_clip_by_index(play.score_sound);

    run_on_event (GE_GOT_SCORE, amnt);
}

void restart_game() {
    can_run_delayed_command();
    if (inside_script) {
        curscript->queue_action(ePSARestartGame, 0, "RestartGame");
        return;
    }
    int errcod;
    if ((errcod = load_game(RESTART_POINT_SAVE_GAME_NUMBER, NULL, NULL))!=0)
        quitprintf("unable to restart game (error:%s)", load_game_errors[-errcod]);

}

void RestoreGameSlot(int slnum) {
    if (displayed_room < 0)
        quit("!RestoreGameSlot: a game cannot be restored from within game_start");

    can_run_delayed_command();
    if (inside_script) {
        curscript->queue_action(ePSARestoreGame, slnum, "RestoreGameSlot");
        return;
    }
    load_game(slnum, NULL, NULL);
}

void DeleteSaveSlot (int slnum) {
    char nametouse[260];
    get_save_game_path(slnum, nametouse);
    unlink (nametouse);
    if ((slnum >= 1) && (slnum <= MAXSAVEGAMES)) {
        char thisname[260];
        for (int i = MAXSAVEGAMES; i > slnum; i--) {
            get_save_game_path(i, thisname);
            FILE *fin = fopen (thisname, "rb");
            if (fin != NULL) {
                fclose (fin);
                // Rename the highest save game to fill in the gap
                rename (thisname, nametouse);
                break;
            }
        }

    }
}

void PauseGame() {
    game_paused++;
    DEBUG_CONSOLE("Game paused");
}
void UnPauseGame() {
    if (game_paused > 0)
        game_paused--;
    DEBUG_CONSOLE("Game UnPaused, pause level now %d", game_paused);
}


int IsGamePaused() {
    if (game_paused>0) return 1;
    return 0;
}

int GetSaveSlotDescription(int slnum,char*desbuf) {
    VALIDATE_STRING(desbuf);
    if (load_game(slnum, desbuf, NULL) == 0)
        return 1;
    sprintf(desbuf,"INVALID SLOT %d", slnum);
    return 0;
}

int LoadSaveSlotScreenshot(int slnum, int width, int height) {
    int gotSlot;
    multiply_up_coordinates(&width, &height);

    if (load_game(slnum, NULL, &gotSlot) != 0)
        return 0;

    if (gotSlot == 0)
        return 0;

    if ((spritewidth[gotSlot] == width) && (spriteheight[gotSlot] == height))
        return gotSlot;

    // resize the sprite to the requested size
    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[gotSlot]), width, height);

    stretch_blit(spriteset[gotSlot], newPic,
        0, 0, spritewidth[gotSlot], spriteheight[gotSlot],
        0, 0, width, height);

    update_polled_stuff_if_runtime();

    // replace the bitmap in the sprite set
    free_dynamic_sprite(gotSlot);
    add_dynamic_sprite(gotSlot, newPic);

    return gotSlot;
}

void SetGlobalInt(int index,int valu) {
    if ((index<0) | (index>=MAXGSVALUES))
        quit("!SetGlobalInt: invalid index");

    if (play.globalscriptvars[index] != valu) {
        DEBUG_CONSOLE("GlobalInt %d set to %d", index, valu);
    }

    play.globalscriptvars[index]=valu;
}


int GetGlobalInt(int index) {
    if ((index<0) | (index>=MAXGSVALUES))
        quit("!GetGlobalInt: invalid index");
    return play.globalscriptvars[index];
}

void SetGlobalString (int index, char *newval) {
    if ((index<0) | (index >= MAXGLOBALSTRINGS))
        quit("!SetGlobalString: invalid index");
    DEBUG_CONSOLE("GlobalString %d set to '%s'", index, newval);
    strncpy(play.globalstrings[index], newval, MAX_MAXSTRLEN);
    // truncate it to 200 chars, to be sure
    play.globalstrings[index][MAX_MAXSTRLEN - 1] = 0;
}

void GetGlobalString (int index, char *strval) {
    if ((index<0) | (index >= MAXGLOBALSTRINGS))
        quit("!GetGlobalString: invalid index");
    strcpy (strval, play.globalstrings[index]);
}

int RunAGSGame (char *newgame, unsigned int mode, int data) {

    can_run_delayed_command();

    int AllowedModes = RAGMODE_PRESERVEGLOBALINT | RAGMODE_LOADNOW;

    if ((mode & (~AllowedModes)) != 0)
        quit("!RunAGSGame: mode value unknown");

    if (use_compiled_folder_as_current_dir || editor_debugging_enabled)
    {
        quit("!RunAGSGame cannot be used while running the game from within the AGS Editor. You must build the game EXE and run it from there to use this function.");
    }

    if ((mode & RAGMODE_LOADNOW) == 0) {
        // need to copy, since the script gets destroyed
        get_current_dir_path(gamefilenamebuf, newgame);
        game_file_name = &gamefilenamebuf[0];
        usetup.main_data_filename = game_file_name;
        play.takeover_data = data;
        load_new_game_restore = -1;

        if (inside_script) {
            curscript->queue_action(ePSARunAGSGame, mode | RAGMODE_LOADNOW, "RunAGSGame");
            ccAbortInstance (ccGetCurrentInstance ());
        }
        else
            load_new_game = mode | RAGMODE_LOADNOW;

        return 0;
    }

    int result, ee;

    unload_old_room();
    displayed_room = -10;

    unload_game_file();

    if (csetlib(game_file_name,""))
        quitprintf("!RunAGSGame: unable to load new game file '%s'", game_file_name);

    clear(abuf);
    show_preload();

    if ((result = load_game_file ()) != 0) {
        quitprintf("!RunAGSGame: error %d loading new game file", result);
    }

    spriteset.reset();
    if (spriteset.initFile ("acsprset.spr"))
        quit("!RunAGSGame: error loading new sprites");

    if ((mode & RAGMODE_PRESERVEGLOBALINT) == 0) {
        // reset GlobalInts
        for (ee = 0; ee < MAXGSVALUES; ee++)
            play.globalscriptvars[ee] = 0;  
    }

    init_game_settings();
    play.screen_is_faded_out = 1;

    if (load_new_game_restore >= 0) {
        load_game (load_new_game_restore, NULL, NULL);
        load_new_game_restore = -1;
    }
    else
        start_game();

    return 0;
}

int GetGameParameter (int parm, int data1, int data2, int data3) {
    switch (parm) {
   case GP_SPRITEWIDTH:
       return Game_GetSpriteWidth(data1);
   case GP_SPRITEHEIGHT:
       return Game_GetSpriteHeight(data1);
   case GP_NUMLOOPS:
       return Game_GetLoopCountForView(data1);
   case GP_NUMFRAMES:
       return Game_GetFrameCountForLoop(data1, data2);
   case GP_FRAMESPEED:
   case GP_FRAMEIMAGE:
   case GP_FRAMESOUND:
   case GP_ISFRAMEFLIPPED:
       {
           if ((data1 < 1) || (data1 > game.numviews))
               quit("!GetGameParameter: invalid view specified");
           if ((data2 < 0) || (data2 >= views[data1 - 1].numLoops))
               quit("!GetGameParameter: invalid loop specified");
           if ((data3 < 0) || (data3 >= views[data1 - 1].loops[data2].numFrames))
               quit("!GetGameParameter: invalid frame specified");

           ViewFrame *pvf = &views[data1 - 1].loops[data2].frames[data3];

           if (parm == GP_FRAMESPEED)
               return pvf->speed;
           else if (parm == GP_FRAMEIMAGE)
               return pvf->pic;
           else if (parm == GP_FRAMESOUND)
               return get_old_style_number_for_sound(pvf->sound);
           else if (parm == GP_ISFRAMEFLIPPED)
               return (pvf->flags & VFLG_FLIPSPRITE) ? 1 : 0;
           else
               quit("GetGameParameter internal error");
       }
   case GP_ISRUNNEXTLOOP:
       return Game_GetRunNextSettingForLoop(data1, data2);
   case GP_NUMGUIS:
       return game.numgui;
   case GP_NUMOBJECTS:
       return croom->numobj;
   case GP_NUMCHARACTERS:
       return game.numcharacters;
   case GP_NUMINVITEMS:
       return game.numinvitems;
   default:
       quit("!GetGameParameter: unknown parameter specified");
    }
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
    // if Ctrl+E has been used to max out frame rate, lock it there
    if ((frames_per_second == 1000) && (display_fps == 2))
        return;

    newspd += play.game_speed_modifier;
    if (newspd>1000) newspd=1000;
    if (newspd<10) newspd=10;
    set_game_speed(newspd);
    DEBUG_CONSOLE("Game speed set to %d", newspd);
}

int GetGameSpeed() {
    return frames_per_second - play.game_speed_modifier;
}

int SetGameOption (int opt, int setting) {
    if (((opt < 1) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
        quit("!SetGameOption: invalid option specified");

    if (opt == OPT_ANTIGLIDE)
    {
        for (int i = 0; i < game.numcharacters; i++)
        {
            if (setting)
                game.chars[i].flags |= CHF_ANTIGLIDE;
            else
                game.chars[i].flags &= ~CHF_ANTIGLIDE;
        }
    }

    if ((opt == OPT_CROSSFADEMUSIC) && (game.audioClipTypeCount > AUDIOTYPE_LEGACY_MUSIC))
    {
        // legacy compatibility -- changing crossfade speed here also
        // updates the new audio clip type style
        game.audioClipTypes[AUDIOTYPE_LEGACY_MUSIC].crossfadeSpeed = setting;
    }

    int oldval = game.options[opt];
    game.options[opt] = setting;

    if (opt == OPT_DUPLICATEINV)
        update_invorder();
    else if (opt == OPT_DISABLEOFF)
        gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);
    else if (opt == OPT_PORTRAITSIDE) {
        if (setting == 0)  // set back to Left
            play.swap_portrait_side = 0;
    }

    return oldval;
}

int GetGameOption (int opt) {
    if (((opt < 1) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
        quit("!GetGameOption: invalid option specified");

    return game.options[opt];
}

void SkipUntilCharacterStops(int cc) {
    if (!is_valid_character(cc))
        quit("!SkipUntilCharacterStops: invalid character specified");
    if (game.chars[cc].room!=displayed_room)
        quit("!SkipUntilCharacterStops: specified character not in current room");

    // if they are not currently moving, do nothing
    if (!game.chars[cc].walking)
        return;

    if (play.in_cutscene)
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

// skipwith decides how it can be skipped:
// 1 = ESC only
// 2 = any key
// 3 = mouse button
// 4 = mouse button or any key
// 5 = right click or ESC only
void StartCutscene (int skipwith) {
    if (play.in_cutscene)
        quit("!StartCutscene: already in a cutscene");

    if ((skipwith < 1) || (skipwith > 5))
        quit("!StartCutscene: invalid argument, must be 1 to 5.");

    // make sure they can't be skipping and cutsceneing at the same time
    EndSkippingUntilCharStops();

    play.in_cutscene = skipwith;
    initialize_skippable_cutscene();
}

int EndCutscene () {
    if (play.in_cutscene == 0)
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

void sc_inputbox(const char*msg,char*bufr) {
    VALIDATE_STRING(bufr);
    setup_for_dialog();
    enterstringwindow(get_translation(msg),bufr);
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
    GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);

    if (play.get_loc_name_save_cursor != play.get_loc_name_last_time) {
        play.get_loc_name_save_cursor = play.get_loc_name_last_time;
        play.restore_cursor_mode_to = GetCursorMode();
        play.restore_cursor_image_to = GetMouseCursor();
        DEBUG_CONSOLE("Saving mouse: mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
    }
}

void GetLocationName(int xxx,int yyy,char*tempo) {
    if (displayed_room < 0)
        quit("!GetLocationName: no room has been loaded");

    VALIDATE_STRING(tempo);

    if (GetGUIAt(xxx, yyy) >= 0) {
        tempo[0]=0;
        int mover = GetInvAt (xxx, yyy);
        if (mover > 0) {
            if (play.get_loc_name_last_time != 1000 + mover)
                guis_need_update = 1;
            play.get_loc_name_last_time = 1000 + mover;
            strcpy(tempo,get_translation(game.invinfo[mover].name));
        }
        else if ((play.get_loc_name_last_time > 1000) && (play.get_loc_name_last_time < 1000 + MAX_INV)) {
            // no longer selecting an item
            guis_need_update = 1;
            play.get_loc_name_last_time = -1;
        }
        return;
    }
    int loctype = GetLocationType (xxx, yyy);
    xxx += divide_down_coordinate(offsetx); 
    yyy += divide_down_coordinate(offsety);
    tempo[0]=0;
    if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
        return;

    int onhs,aa;
    if (loctype == 0) {
        if (play.get_loc_name_last_time != 0) {
            play.get_loc_name_last_time = 0;
            guis_need_update = 1;
        }
        return;
    }

    // on character
    if (loctype == LOCTYPE_CHAR) {
        onhs = getloctype_index;
        strcpy(tempo,get_translation(game.chars[onhs].name));
        if (play.get_loc_name_last_time != 2000+onhs)
            guis_need_update = 1;
        play.get_loc_name_last_time = 2000+onhs;
        return;
    }
    // on object
    if (loctype == LOCTYPE_OBJ) {
        aa = getloctype_index;
        strcpy(tempo,get_translation(thisroom.objectnames[aa]));
        if (play.get_loc_name_last_time != 3000+aa)
            guis_need_update = 1;
        play.get_loc_name_last_time = 3000+aa;
        return;
    }
    onhs = getloctype_index;
    if (onhs>0) strcpy(tempo,get_translation(thisroom.hotspotnames[onhs]));
    if (play.get_loc_name_last_time != onhs)
        guis_need_update = 1;
    play.get_loc_name_last_time = onhs;
}

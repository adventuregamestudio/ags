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

#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/game.h"
#include "ac/common.h"
#include "ac/roomstruct.h"
#include "ac/view.h"
#include "ac/audiochannel.h"
#include "ac/character.h"
#include "ac/charactercache.h"
#include "ac/characterextras.h"
#include "ac/dialogtopic.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/file.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_object.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/lipsync.h"
#include "ac/mouse.h"
#include "ac/movelist.h"
#include "ac/objectcache.h"
#include "ac/overlay.h"
#include "ac/record.h"
#include "ac/region.h"
#include "ac/richgamemedia.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/timer.h"
#include "ac/translation.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "debug/out.h"
#include "script/cc_error.h"
#include "util/string_utils.h"
#include "debug/debug_log.h"
#include "gui/animatingguibutton.h"
#include "gui/guidialog.h"
#include "main/main.h"
#include "media/audio/audio.h"
#include "plugin/agsplugin.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/spritecache.h"
#include "util/filestream.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using AGS::Common::String;
using AGS::Common::DataStream;
using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

#if !defined (WINDOWS_VERSION)
#include <sys/stat.h>                      //mkdir
#endif

extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];
extern int time_between_timers;
extern Bitmap *virtual_screen;
extern int cur_mode,cur_cursor;
extern SpeechLipSyncLine *splipsync;
extern int numLipLines, curLipLine, curLipLinePhenome;

extern CharacterExtras *charextra;
extern DialogTopic *dialog;

extern int scrnwid,scrnhit;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;

extern int ifacepopped;  // currently displayed pop-up GUI (-1 if none)
extern int mouse_on_iface;   // mouse cursor is over this interface
extern int mouse_on_iface_button;
extern int mouse_pushed_iface;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;

extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;

extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int numscreenover;
extern int is_complete_overlay,is_text_overlay;

extern int psp_gfx_smoothing;
extern int psp_gfx_scaling;
extern int psp_gfx_renderer;
extern int psp_gfx_super_sampling;

extern int obj_lowest_yp, char_lowest_yp;

extern int actSpsCount;
extern Bitmap **actsps;
extern IDriverDependantBitmap* *actspsbmp;
// temporary cache of walk-behind for this actsps image
extern Bitmap **actspswb;
extern IDriverDependantBitmap* *actspswbbmp;
extern CachedActSpsData* actspswbcache;
extern Bitmap **guibg;
extern IDriverDependantBitmap **guibgbmp;
extern char transFileName[MAX_PATH];
extern color palette[256];
extern int offsetx,offsety;
extern unsigned int loopcounter;
extern Bitmap *raw_saved_screen;
extern Bitmap *dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
extern IGraphicsDriver *gfxDriver;

//=============================================================================
GameState play;
GameSetup usetup;
GameSetupStruct game;
RoomStatus troom;    // used for non-saveable rooms, eg. intro
RoomObject*objs;
RoomStatus*croom=NULL;
roomstruct thisroom;

volatile int switching_away_from_game = 0;
volatile char want_exit = 0, abort_engine = 0;
GameDataVersion loaded_game_file_version = kGameVersion_Undefined;
int frames_per_second=40;
int displayed_room=-10,starting_room = -1;
int in_new_room=0, new_room_was = 0;  // 1 in new room, 2 first time in new room, 3 loading saved game
int new_room_pos=0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;

//Bitmap *spriteset[MAX_SPRITES+1];
//SpriteCache spriteset (MAX_SPRITES+1);
// initially size 1, this will be increased by the initFile function
int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
SpriteCache spriteset(1);
int proper_exit=0,our_eip=0;

GUIMain*guis=NULL;

CCGUIObject ccDynamicGUIObject;
CCCharacter ccDynamicCharacter;
CCHotspot   ccDynamicHotspot;
CCRegion    ccDynamicRegion;
CCInventory ccDynamicInv;
CCGUI       ccDynamicGUI;
CCObject    ccDynamicObject;
CCDialog    ccDynamicDialog;
ScriptString myScriptStringImpl;
ScriptObject scrObj[MAX_INIT_SPR];
ScriptGUI    *scrGui = NULL;
ScriptHotspot scrHotspot[MAX_HOTSPOTS];
ScriptRegion scrRegion[MAX_REGIONS];
ScriptInvItem scrInv[MAX_INV];
ScriptDialog scrDialog[MAX_DIALOG];

ViewStruct*views=NULL;

CharacterCache *charcache = NULL;
ObjectCache objcache[MAX_INIT_SPR];

MoveList *mls = NULL;

//=============================================================================

char saveGameDirectory[260] = "./";
int want_quit = 0;

const char* sgnametemplate = "agssave.%03d";
char saveGameSuffix[MAX_SG_EXT_LENGTH + 1];

int game_paused=0;

int engineNeedsAsInt = 100;
char pexbuf[STD_BUFFER_SIZE];

unsigned int load_new_game = 0;
int load_new_game_restore = -1;

const char *load_game_errors[9] =
{"No error","File not found","Not an AGS save game",
"Invalid save game version","Saved with different interpreter",
"Saved under a different game", "Resolution mismatch",
"Colour depth mismatch", ""};

int getloctype_index = 0, getloctype_throughgui = 0;

//=============================================================================
// Audio
//=============================================================================

void Game_StopAudio(int audioType)
{
    if (((audioType < 0) || (audioType >= game.audioClipTypeCount)) && (audioType != SCR_NO_VALUE))
        quitprintf("!Game.StopAudio: invalid audio type %d", audioType);
    int aa;

    for (aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
    {
        if (audioType == SCR_NO_VALUE)
        {
            stop_or_fade_out_channel(aa);
        }
        else
        {
            ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
            if ((clip != NULL) && (clip->type == audioType))
                stop_or_fade_out_channel(aa);
        }
    }

    remove_clips_of_type_from_queue(audioType);
}

int Game_IsAudioPlaying(int audioType)
{
    if (((audioType < 0) || (audioType >= game.audioClipTypeCount)) && (audioType != SCR_NO_VALUE))
        quitprintf("!Game.IsAudioPlaying: invalid audio type %d", audioType);

    if (play.fast_forward)
        return 0;

    for (int aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
    {
        ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
        if (clip != NULL) 
        {
            if ((clip->type == audioType) || (audioType == SCR_NO_VALUE))
            {
                return 1;
            }
        }
    }
    return 0;
}

void Game_SetAudioTypeSpeechVolumeDrop(int audioType, int volumeDrop) 
{
    if ((audioType < 0) || (audioType >= game.audioClipTypeCount))
        quit("!Game.SetAudioTypeVolume: invalid audio type");

    game.audioClipTypes[audioType].volume_reduction_while_speech_playing = volumeDrop;
}

void Game_SetAudioTypeVolume(int audioType, int volume, int changeType)
{
    if ((volume < 0) || (volume > 100))
        quitprintf("!Game.SetAudioTypeVolume: volume %d is not between 0..100", volume);
    if ((audioType < 0) || (audioType >= game.audioClipTypeCount))
        quit("!Game.SetAudioTypeVolume: invalid audio type");
    int aa;

    if ((changeType == VOL_CHANGEEXISTING) ||
        (changeType == VOL_BOTH))
    {
        for (aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
        {
            ScriptAudioClip *clip = AudioChannel_GetPlayingClip(&scrAudioChannel[aa]);
            if ((clip != NULL) && (clip->type == audioType))
            {
                channels[aa]->set_volume((volume * 255) / 100);
                channels[aa]->volAsPercentage = volume;
            }
        }
    }

    if ((changeType == VOL_SETFUTUREDEFAULT) ||
        (changeType == VOL_BOTH))
    {
        play.default_audio_type_volumes[audioType] = volume;
    }

}

int Game_GetMODPattern() {
    if (current_music_type == MUS_MOD) {
        return channels[SCHAN_MUSIC]->get_pos();
    }
    return -1;
}

//=============================================================================
// ---
//=============================================================================

int Game_GetDialogCount()
{
  return game.numdialog;
}

void set_game_speed(int fps) {
    frames_per_second = fps;
    time_between_timers = 1000 / fps;
    install_int_ex(dj_timer_handler,MSEC_TO_TIMER(time_between_timers));
}

extern int cbuttfont;
extern int acdialog_font;

extern char buffer2[60];
int oldmouse;
void setup_for_dialog() {
    cbuttfont = play.normal_font;
    acdialog_font = play.normal_font;
    wsetscreen(virtual_screen);
    if (!play.mouse_cursor_hidden)
        domouse(1);
    oldmouse=cur_cursor; set_mouse_cursor(CURS_ARROW);
}
void restore_after_dialog() {
    set_mouse_cursor(oldmouse);
    if (!play.mouse_cursor_hidden)
        domouse(2);
    construct_virtual_screen(true);
}



String get_save_game_path(int slotNum) {
    String filename;
    filename.Format(sgnametemplate, slotNum);
    String path = saveGameDirectory;
    path.Append(filename);
    path.Append(saveGameSuffix);
    return path;
}



int Game_SetSaveGameDirectory(const char *newFolder) {

    // don't allow them to go to another folder
    if ((newFolder[0] == '/') || (newFolder[0] == '\\') ||
        (newFolder[0] == ' ') ||
        ((newFolder[0] != 0) && (newFolder[1] == ':')))
        return 0;

    char newSaveGameDir[260];
    platform->ReplaceSpecialPaths(newFolder, newSaveGameDir);
    fix_filename_slashes(newSaveGameDir);

#if defined (WINDOWS_VERSION)
    mkdir(newSaveGameDir);
#else
    mkdir(newSaveGameDir, 0);
#endif

    put_backslash(newSaveGameDir);

    char newFolderTempFile[260];
    strcpy(newFolderTempFile, newSaveGameDir);
    strcat(newFolderTempFile, "agstmp.tmp");

    if (!Common::File::TestCreateFile(newFolderTempFile)) {
        return 0;
    }

    // copy the Restart Game file, if applicable
    char restartGamePath[260];
    sprintf(restartGamePath, "%s""agssave.%d%s", saveGameDirectory, RESTART_POINT_SAVE_GAME_NUMBER, saveGameSuffix);
    DataStream *restartGameFile = Common::File::OpenFileRead(restartGamePath);
    if (restartGameFile != NULL) {
        long fileSize = restartGameFile->GetLength();
        char *mbuffer = (char*)malloc(fileSize);
        restartGameFile->Read(mbuffer, fileSize);
        delete restartGameFile;

        sprintf(restartGamePath, "%s""agssave.%d%s", newSaveGameDir, RESTART_POINT_SAVE_GAME_NUMBER, saveGameSuffix);
        restartGameFile = Common::File::CreateFile(restartGamePath);
        restartGameFile->Write(mbuffer, fileSize);
        delete restartGameFile;
        free(mbuffer);
    }

    strcpy(saveGameDirectory, newSaveGameDir);
    return 1;
}



const char* Game_GetSaveSlotDescription(int slnum) {
    String description;
    if (read_savedgame_description(get_save_game_path(slnum), description) == 0)
        return CreateNewScriptString(description);
    return NULL;
}




int load_game_and_print_error(int toload) {
    int ecret = load_game(toload);
    if (ecret < 0) {
        // disable speech in case there are dynamic graphics that
        // have been freed
        int oldalways = game.options[OPT_ALWAYSSPCH];
        game.options[OPT_ALWAYSSPCH] = 0;
        Display("Unable to load game (error: %s).",load_game_errors[-ecret]);
        game.options[OPT_ALWAYSSPCH] = oldalways;
    }
    return ecret;
}

void restore_game_dialog() {
    can_run_delayed_command();
    if (thisroom.options[ST_SAVELOAD] == 1) {
        DisplayMessage (983);
        return;
    }
    if (inside_script) {
        curscript->queue_action(ePSARestoreGameDialog, 0, "RestoreGameDialog");
        return;
    }
    setup_for_dialog();
    int toload=loadgamedialog();
    restore_after_dialog();
    if (toload>=0) {
        load_game_and_print_error(toload);
    }
}

void save_game_dialog() {
    if (thisroom.options[ST_SAVELOAD] == 1) {
        DisplayMessage (983);
        return;
    }
    if (inside_script) {
        curscript->queue_action(ePSASaveGameDialog, 0, "SaveGameDialog");
        return;
    }
    setup_for_dialog();
    int toload=savegamedialog();
    restore_after_dialog();
    if (toload>=0)
        save_game(toload,buffer2);
}





void setup_sierra_interface() {
    int rr;
    game.numgui =0;
    for (rr=0;rr<42;rr++) game.paluses[rr]=PAL_GAMEWIDE;
    for (rr=42;rr<256;rr++) game.paluses[rr]=PAL_BACKGROUND;
}


void free_do_once_tokens() 
{
    for (int i = 0; i < play.num_do_once_tokens; i++)
    {
        free(play.do_once_tokens[i]);
    }
    if (play.do_once_tokens != NULL) 
    {
        free(play.do_once_tokens);
        play.do_once_tokens = NULL;
    }
    play.num_do_once_tokens = 0;
}


// Free all the memory associated with the game
void unload_game_file() {
    int bb, ee;

    for (bb = 0; bb < game.numcharacters; bb++) {
        if (game.charScripts != NULL)
            delete game.charScripts[bb];

        if (game.intrChar != NULL)
        {
            if (game.intrChar[bb] != NULL)
                delete game.intrChar[bb];
            game.intrChar[bb] = NULL;
        }
        free(characterScriptObjNames[bb]);
        game.charProps[bb].reset();
    }
    if (game.intrChar != NULL)
    {
        free(game.intrChar);
        game.intrChar = NULL;
    }
    free(characterScriptObjNames);
    free(charextra);
    free(mls);
    free(actsps);
    free(actspsbmp);
    free(actspswb);
    free(actspswbbmp);
    free(actspswbcache);
    free(game.charProps);

    for (bb = 1; bb < game.numinvitems; bb++) {
        if (game.invScripts != NULL)
            delete game.invScripts[bb];
        if (game.intrInv[bb] != NULL)
            delete game.intrInv[bb];
        game.intrInv[bb] = NULL;
    }

    if (game.charScripts != NULL)
    {
        delete game.charScripts;
        delete game.invScripts;
        game.charScripts = NULL;
        game.invScripts = NULL;
    }

    if (game.dict != NULL) {
        game.dict->free_memory();
        free (game.dict);
        game.dict = NULL;
    }

    if ((gameinst != NULL) && (gameinst->pc != 0))
        quit("Error: unload_game called while script still running");
    //ccAbortAndDestroyInstance (gameinst);
    else {
        ccFreeInstance(gameinstFork);
        ccFreeInstance(gameinst);
        gameinstFork = NULL;
        gameinst = NULL;
    }

    ccFreeScript (gamescript);
    gamescript = NULL;

    if ((dialogScriptsInst != NULL) && (dialogScriptsInst->pc != 0))
        quit("Error: unload_game called while dialog script still running");
    else if (dialogScriptsInst != NULL)
    {
        ccFreeInstance(dialogScriptsInst);
        dialogScriptsInst = NULL;
    }

    if (dialogScriptsScript != NULL)
    {
        ccFreeScript(dialogScriptsScript);
        dialogScriptsScript = NULL;
    }

    for (ee = 0; ee < numScriptModules; ee++) {
        ccFreeInstance(moduleInstFork[ee]);
        ccFreeInstance(moduleInst[ee]);
        ccFreeScript(scriptModules[ee]);
    }
    numScriptModules = 0;

    if (game.audioClipCount > 0)
    {
        free(game.audioClips);
        game.audioClipCount = 0;
        free(game.audioClipTypes);
        game.audioClipTypeCount = 0;
    }

    free(game.viewNames[0]);
    free(game.viewNames);
    free (views);
    views = NULL;

    free (game.chars);
    game.chars = NULL;

    free (charcache);
    charcache = NULL;

    if (splipsync != NULL)
    {
        for (ee = 0; ee < numLipLines; ee++)
        {
            free(splipsync[ee].endtimeoffs);
            free(splipsync[ee].frame);
        }
        free(splipsync);
        splipsync = NULL;
        numLipLines = 0;
        curLipLine = -1;
    }

    for (ee=0;ee < MAXGLOBALMES;ee++) {
        if (game.messages[ee]==NULL) continue;
        free (game.messages[ee]);
        game.messages[ee] = NULL;
    }

    for (ee = 0; ee < game.roomCount; ee++)
    {
        free(game.roomNames[ee]);
    }
    if (game.roomCount > 0)
    {
        free(game.roomNames);
        free(game.roomNumbers);
        game.roomCount = 0;
    }

    for (ee=0;ee<game.numdialog;ee++) {
        if (dialog[ee].optionscripts!=NULL)
            free (dialog[ee].optionscripts);
        dialog[ee].optionscripts = NULL;
    }
    free (dialog);
    dialog = NULL;

    for (ee = 0; ee < game.numgui; ee++) {
        free (guibg[ee]);
        guibg[ee] = NULL;
        free(guiScriptObjNames[ee]);
    }

    free(guiScriptObjNames);
    free(guibg);
    free (guis);
    guis = NULL;
    free(scrGui);

    platform->ShutdownPlugins();
    ccRemoveAllSymbols();
    ccUnregisterAllObjects();

    for (ee=0;ee<game.numfonts;ee++)
        wfreefont(ee);

    free_do_once_tokens();
    free(play.gui_draw_order);

    resetRoomStatuses();

}






const char* Game_GetGlobalStrings(int index) {
    if ((index < 0) || (index >= MAXGLOBALSTRINGS))
        quit("!Game.GlobalStrings: invalid index");

    return CreateNewScriptString(play.globalstrings[index]);
}



char gamefilenamebuf[200];


// ** GetGameParameter replacement functions

int Game_GetInventoryItemCount() {
    // because of the dummy item 0, this is always one higher than it should be
    return game.numinvitems - 1;
}

int Game_GetFontCount() {
    return game.numfonts;
}

int Game_GetMouseCursorCount() {
    return game.numcursors;
}

int Game_GetCharacterCount() {
    return game.numcharacters;
}

int Game_GetGUICount() {
    return game.numgui;
}

int Game_GetViewCount() {
    return game.numviews;
}

int Game_GetUseNativeCoordinates()
{
    if (game.options[OPT_NATIVECOORDINATES] != 0)
    {
        return 1;
    }
    return 0;
}

int Game_GetSpriteWidth(int spriteNum) {
    if ((spriteNum < 0) || (spriteNum >= MAX_SPRITES))
        return 0;

    if (!spriteset.doesSpriteExist(spriteNum))
        return 0;

    return divide_down_coordinate(spritewidth[spriteNum]);
}

int Game_GetSpriteHeight(int spriteNum) {
    if ((spriteNum < 0) || (spriteNum >= MAX_SPRITES))
        return 0;

    if (!spriteset.doesSpriteExist(spriteNum))
        return 0;

    return divide_down_coordinate(spriteheight[spriteNum]);
}

int Game_GetLoopCountForView(int viewNumber) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");

    return views[viewNumber - 1].numLoops;
}

int Game_GetRunNextSettingForLoop(int viewNumber, int loopNumber) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");

    return (views[viewNumber - 1].loops[loopNumber].RunNextLoop()) ? 1 : 0;
}

int Game_GetFrameCountForLoop(int viewNumber, int loopNumber) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");

    return views[viewNumber - 1].loops[loopNumber].numFrames;
}

ScriptViewFrame* Game_GetViewFrame(int viewNumber, int loopNumber, int frame) {
    if ((viewNumber < 1) || (viewNumber > game.numviews))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");
    if ((frame < 0) || (frame >= views[viewNumber - 1].loops[loopNumber].numFrames))
        quit("!GetGameParameter: invalid frame specified");

    ScriptViewFrame *sdt = new ScriptViewFrame(viewNumber - 1, loopNumber, frame);
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

int Game_DoOnceOnly(const char *token) 
{
    if (strlen(token) > 199)
        quit("!Game.DoOnceOnly: token length cannot be more than 200 chars");

    for (int i = 0; i < play.num_do_once_tokens; i++)
    {
        if (strcmp(play.do_once_tokens[i], token) == 0)
        {
            return 0;
        }
    }
    play.do_once_tokens = (char**)realloc(play.do_once_tokens, sizeof(char*) * (play.num_do_once_tokens + 1));
    play.do_once_tokens[play.num_do_once_tokens] = (char*)malloc(strlen(token) + 1);
    strcpy(play.do_once_tokens[play.num_do_once_tokens], token);
    play.num_do_once_tokens++;
    return 1;
}

int Game_GetTextReadingSpeed()
{
    return play.text_speed;
}

void Game_SetTextReadingSpeed(int newTextSpeed)
{
    if (newTextSpeed < 1)
        quitprintf("!Game.TextReadingSpeed: %d is an invalid speed", newTextSpeed);

    play.text_speed = newTextSpeed;
}

int Game_GetMinimumTextDisplayTimeMs()
{
    return play.text_min_display_time_ms;
}

void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime)
{
    play.text_min_display_time_ms = newTextMinTime;
}

int Game_GetIgnoreUserInputAfterTextTimeoutMs()
{
    return play.ignore_user_input_after_text_timeout_ms;
}

void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs)
{
    play.ignore_user_input_after_text_timeout_ms = newValueMs;
}

const char *Game_GetFileName() {
    return CreateNewScriptString(usetup.main_data_filename);
}

const char *Game_GetName() {
    return CreateNewScriptString(play.game_name);
}

void Game_SetName(const char *newName) {
    strncpy(play.game_name, newName, 99);
    play.game_name[99] = 0;

#if (ALLEGRO_DATE > 19990103)
    set_window_title(play.game_name);
#endif
}

int Game_GetSkippingCutscene()
{
    if (play.fast_forward)
    {
        return 1;
    }
    return 0;
}

int Game_GetInSkippableCutscene()
{
    if (play.in_cutscene)
    {
        return 1;
    }
    return 0;
}

int Game_GetColorFromRGB(int red, int grn, int blu) {
    if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
        (blu < 0) || (blu > 255))
        quit("!GetColorFromRGB: colour values must be 0-255");

    if (game.color_depth == 1)
    {
        return makecol8(red, grn, blu);
    }

    int agscolor = ((blu >> 3) & 0x1f);
    agscolor += ((grn >> 2) & 0x3f) << 5;
    agscolor += ((red >> 3) & 0x1f) << 11;
    return agscolor;
}

const char* Game_InputBox(const char *msg) {
    char buffer[STD_BUFFER_SIZE];
    sc_inputbox(msg, buffer);
    return CreateNewScriptString(buffer);
}

const char* Game_GetLocationName(int x, int y) {
    char buffer[STD_BUFFER_SIZE];
    GetLocationName(x, y, buffer);
    return CreateNewScriptString(buffer);
}

const char* Game_GetGlobalMessages(int index) {
    if ((index < 500) || (index >= MAXGLOBALMES + 500)) {
        return NULL;
    }
    char buffer[STD_BUFFER_SIZE];
    buffer[0] = 0;
    replace_tokens(get_translation(get_global_message(index)), buffer, STD_BUFFER_SIZE);
    return CreateNewScriptString(buffer);
}

int Game_GetSpeechFont() {
    return play.speech_font;
}
int Game_GetNormalFont() {
    return play.normal_font;
}

const char* Game_GetTranslationFilename() {
    char buffer[STD_BUFFER_SIZE];
    GetTranslationName(buffer);
    return CreateNewScriptString(buffer);
}

int Game_ChangeTranslation(const char *newFilename)
{
    if ((newFilename == NULL) || (newFilename[0] == 0))
    {
        close_translation();
        strcpy(transFileName, "");
        return 1;
    }

    char oldTransFileName[MAX_PATH];
    strcpy(oldTransFileName, transFileName);

    if (!init_translation(newFilename))
    {
        strcpy(transFileName, oldTransFileName);
        return 0;
    }

    return 1;
}

//=============================================================================

// save game functions
#define SGVERSION 8
char*sgsig="Adventure Game Studio saved game";
int sgsiglen=32;

void serialize_bitmap(Common::Bitmap *thispic, DataStream *out) {
    if (thispic != NULL) {
        out->WriteInt32(thispic->GetWidth());
        out->WriteInt32(thispic->GetHeight());
        out->WriteInt32(thispic->GetColorDepth());
        for (int cc=0;cc<thispic->GetHeight();cc++)
        {
          switch (thispic->GetColorDepth())
          {
          case 8:
          // CHECKME: originally, AGS does not use real BPP here, but simply divides color depth by 8;
          // therefore 15-bit bitmaps are saved only partially? is this a bug? or?
          case 15:
            out->WriteArray(&thispic->GetScanLine(cc)[0], thispic->GetWidth(), 1);
            break;
          case 16:
            out->WriteArrayOfInt16((const int16_t*)&thispic->GetScanLine(cc)[0], thispic->GetWidth());
            break;
          case 32:
            out->WriteArrayOfInt32((const int32_t*)&thispic->GetScanLine(cc)[0], thispic->GetWidth());
            break;
          }
        }
    }
}

// Some people have been having crashes with the save game list,
// so make sure the game name is valid
void safeguard_string (unsigned char *descript) {
    int it;
    for (it = 0; it < 50; it++) {
        if ((descript[it] < 1) || (descript[it] > 127))
            break;
    }
    if (descript[it] != 0)
        descript[it] = 0;
}

// On Windows we could just use IIDFromString but this is platform-independant
void convert_guid_from_text_to_binary(const char *guidText, unsigned char *buffer) 
{
    guidText++; // skip {
    for (int bytesDone = 0; bytesDone < 16; bytesDone++)
    {
        if (*guidText == '-')
            guidText++;

        char tempString[3];
        tempString[0] = guidText[0];
        tempString[1] = guidText[1];
        tempString[2] = 0;
        int thisByte = 0;
        sscanf(tempString, "%X", &thisByte);

        buffer[bytesDone] = thisByte;
        guidText += 2;
    }

    // Swap bytes to give correct GUID order
    unsigned char temp;
    temp = buffer[0]; buffer[0] = buffer[3]; buffer[3] = temp;
    temp = buffer[1]; buffer[1] = buffer[2]; buffer[2] = temp;
    temp = buffer[4]; buffer[4] = buffer[5]; buffer[5] = temp;
    temp = buffer[6]; buffer[6] = buffer[7]; buffer[7] = temp;
}

Bitmap *read_serialized_bitmap(DataStream *in) {
    Bitmap *thispic;
    int picwid = in->ReadInt32();
    int pichit = in->ReadInt32();
    int piccoldep = in->ReadInt32();
    thispic = BitmapHelper::CreateBitmap(picwid,pichit,piccoldep);
    if (thispic == NULL)
        return NULL;
    for (int vv=0; vv < pichit; vv++)
    {
      switch (piccoldep)
      {
      case 8:
      // CHECKME: originally, AGS does not use real BPP here, but simply divides color depth by 8
      case 15:
        in->ReadArray(thispic->GetScanLineForWriting(vv), picwid, 1);
        break;
      case 16:
        in->ReadArrayOfInt16((int16_t*)thispic->GetScanLineForWriting(vv), picwid);
        break;
      case 32:
        in->ReadArrayOfInt32((int32_t*)thispic->GetScanLineForWriting(vv), picwid);
        break;
      }
    }

    return thispic;
}




long write_screen_shot_for_vista(DataStream *out, Bitmap *screenshot) 
{
    long fileSize = 0;
    char tempFileName[MAX_PATH];
    sprintf(tempFileName, "%s""_tmpscht.bmp", saveGameDirectory);

	BitmapHelper::SaveToFile(screenshot, tempFileName, palette);

    update_polled_stuff_if_runtime();

    if (exists(tempFileName))
    {
        fileSize = file_size(tempFileName);
        char *buffer = (char*)malloc(fileSize);

        DataStream *temp_in = Common::File::OpenFileRead(tempFileName);
        temp_in->Read(buffer, fileSize);
        delete temp_in;
        unlink(tempFileName);

        out->Write(buffer, fileSize);
        free(buffer);
    }
    return fileSize;
}


void save_game_screenshot(DataStream *out, Bitmap *screenshot)
{
    // store the screenshot at the start to make it easily accesible
    out->WriteInt32((screenshot == NULL) ? 0 : 1);

    if (screenshot)
        serialize_bitmap(screenshot, out);
}

void save_game_header(DataStream *out)
{
    fputstring(ACI_VERSION_TEXT, out);
    fputstring(usetup.main_data_filename, out);
}

void save_game_head_dynamic_values(DataStream *out)
{
    out->WriteInt32(scrnhit);
    out->WriteInt32(final_col_dep);
    out->WriteInt32(frames_per_second);
    out->WriteInt32(cur_mode);
    out->WriteInt32(cur_cursor);
    out->WriteInt32(offsetx); out->WriteInt32(offsety);
    out->WriteInt32(loopcounter);
}

void save_game_spriteset(DataStream *out)
{
    out->WriteInt32(spriteset.elements);
    for (int bb = 1; bb < spriteset.elements; bb++) {
        if (game.spriteflags[bb] & SPF_DYNAMICALLOC) {
            out->WriteInt32(bb);
            out->WriteInt8(game.spriteflags[bb]);
            serialize_bitmap(spriteset[bb], out);
        }
    }
    // end of dynamic sprite list
    out->WriteInt32(0);
}

void save_game_scripts(DataStream *out)
{
    // write the data segment of the global script
    int gdatasize=gameinst->globaldatasize;
    out->WriteInt32(gdatasize);
    ccFlattenGlobalData (gameinst);
    // MACPORT FIX: just in case gdatasize is 2 or 4, don't want to swap endian
    out->Write(&gameinst->globaldata[0], gdatasize);
    ccUnFlattenGlobalData (gameinst);
    // write the script modules data segments
    out->WriteInt32(numScriptModules);
    for (int bb = 0; bb < numScriptModules; bb++) {
        int glsize = moduleInst[bb]->globaldatasize;
        out->WriteInt32(glsize);
        if (glsize > 0) {
            ccFlattenGlobalData(moduleInst[bb]);
            out->Write(&moduleInst[bb]->globaldata[0], glsize);
            ccUnFlattenGlobalData(moduleInst[bb]);
        }
    }
}

void save_game_room_state(DataStream *out)
{
    out->WriteInt32(displayed_room);
    if (displayed_room >= 0) {
        // update the current room script's data segment copy
        if (roominst!=NULL)
            save_room_data_segment();

        // Update the saved interaction variable values
        for (int ff = 0; ff < thisroom.numLocalVars; ff++)
            croom->interactionVariableValues[ff] = thisroom.localvars[ff].value;
    }

    // write the room state for all the rooms the player has been in
    RoomStatus* roomstat;
    for (int bb = 0; bb < MAX_ROOMS; bb++) {
        if (isRoomStatusValid(bb))
        {
            roomstat = getRoomStatus(bb);
            if (roomstat->beenhere) {
                out->WriteInt8 (1);
                roomstat->WriteToFile(out);
                if (roomstat->tsdatasize>0)
                    out->Write(&roomstat->tsdata[0], roomstat->tsdatasize);
            }
            else
                out->WriteInt8 (0); // <--- [IKM] added by me, CHECKME if needed / works correctly
        }
        else
            out->WriteInt8 (0);
    }
}

void save_game_play_ex_data(DataStream *out)
{
    for (int bb = 0; bb < play.num_do_once_tokens; bb++)
    {
        fputstring(play.do_once_tokens[bb], out);
    }
    out->WriteArrayOfInt32(&play.gui_draw_order[0], game.numgui);
}

void save_game_movelist(DataStream *out)
{
    for (int i = 0; i < game.numcharacters + MAX_INIT_SPR + 1; ++i)
    {
        mls[i].WriteToFile(out);
    }
    //out->WriteArray(&mls[0],sizeof(MoveList), game.numcharacters + MAX_INIT_SPR + 1);
}

void save_game_charextras(DataStream *out)
{
    for (int i = 0; i < game.numcharacters; ++i)
    {
        charextra[i].WriteToFile(out);
    }
    //out->WriteArray(&charextra[0],sizeof(CharacterExtras),game.numcharacters);
}

void save_game_palette(DataStream *out)
{
    out->WriteArray(&palette[0],sizeof(color),256);
}

void save_game_dialogs(DataStream *out)
{
    for (int bb=0;bb<game.numdialog;bb++)
        out->WriteArrayOfInt32(&dialog[bb].optionflags[0],MAXTOPICOPTIONS);
}

// [IKM] yea, okay this is just plain silly name :)
void save_game_more_dynamic_values(DataStream *out)
{
    out->WriteInt32(mouse_on_iface);
    out->WriteInt32(mouse_on_iface_button);
    out->WriteInt32(mouse_pushed_iface);
    out->WriteInt32 (ifacepopped);
    out->WriteInt32(game_paused);
    //out->WriteInt32(mi.trk);
}

void save_game_gui(DataStream *out)
{
    write_gui(out,guis,&game);
    out->WriteInt32(numAnimButs);
    for (int i = 0; i < numAnimButs; ++i)
    {
        animbuts[i].WriteToFile(out);
    }
    int padding = get_padding(sizeof(AnimatingGUIButton) * numAnimButs);
    char pad_buf[3] = {0,0,0};
    out->Write(pad_buf, padding);
}

void save_game_audiocliptypes(DataStream *out)
{
    out->WriteInt32(game.audioClipTypeCount);
    for (int i = 0; i < game.audioClipTypeCount; ++i)
    {
        game.audioClipTypes[i].WriteToFile(out);
    }
    //out->WriteArray(&game.audioClipTypes[0], sizeof(AudioClipType), game.audioClipTypeCount);
}

void save_game_thisroom(DataStream *out)
{
    out->WriteArrayOfInt16(&thisroom.regionLightLevel[0],MAX_REGIONS);
    out->WriteArrayOfInt32(&thisroom.regionTintLevel[0],MAX_REGIONS);
    out->WriteArrayOfInt16(&thisroom.walk_area_zoom[0],MAX_WALK_AREAS + 1);
    out->WriteArrayOfInt16(&thisroom.walk_area_zoom2[0],MAX_WALK_AREAS + 1);
}

void save_game_ambientsounds(DataStream *out)
{
    for (int i = 0; i < MAX_SOUND_CHANNELS; ++i)
    {
        ambient[i].WriteToFile(out);
    }
    //out->WriteArray (&ambient[0], sizeof(AmbientSound), MAX_SOUND_CHANNELS);
}

void save_game_overlays(DataStream *out)
{
    out->WriteInt32(numscreenover);
    for (int i = 0; i < numscreenover; ++i)
    {
        screenover[i].WriteToFile(out);
    }
    //out->WriteArray(&screenover[0],sizeof(ScreenOverlay),numscreenover);
    for (int bb=0;bb<numscreenover;bb++) {
        serialize_bitmap (screenover[bb].pic, out);
    }
}

void save_game_dynamic_surfaces(DataStream *out)
{
    for (int bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
    {
        if (dynamicallyCreatedSurfaces[bb] == NULL)
        {
            out->WriteInt8(0);
        }
        else
        {
            out->WriteInt8(1);
            serialize_bitmap(dynamicallyCreatedSurfaces[bb], out);
        }
    }
}

void save_game_displayed_room_status(DataStream *out)
{
    if (displayed_room >= 0) {

        for (int bb = 0; bb < MAX_BSCENE; bb++) {
            if (play.raw_modified[bb])
                serialize_bitmap (thisroom.ebscene[bb], out);
        }

        out->WriteInt32 ((raw_saved_screen == NULL) ? 0 : 1);
        if (raw_saved_screen)
            serialize_bitmap (raw_saved_screen, out);

        // save the current troom, in case they save in room 600 or whatever
        troom.WriteToFile(out);
        //out->WriteArray(&troom,sizeof(RoomStatus),1);
        if (troom.tsdatasize>0)
            out->Write(&troom.tsdata[0],troom.tsdatasize);

    }
}

void save_game_globalvars(DataStream *out)
{
    out->WriteInt32 (numGlobalVars);
    for (int i = 0; i < numGlobalVars; ++i)
    {
        globalvars[i].WriteToFile(out);
    }
    //out->WriteArray (&globalvars[0], sizeof(InteractionVariable), numGlobalVars);
}

void save_game_views(DataStream *out)
{
    out->WriteInt32 (game.numviews);
    for (int bb = 0; bb < game.numviews; bb++) {
        for (int cc = 0; cc < views[bb].numLoops; cc++) {
            for (int dd = 0; dd < views[bb].loops[cc].numFrames; dd++)
            {
                out->WriteInt32(views[bb].loops[cc].frames[dd].sound);
                out->WriteInt32(views[bb].loops[cc].frames[dd].pic);
            }
        }
    }
}

void save_game_audioclips_and_crossfade(DataStream *out)
{
    out->WriteInt32(game.audioClipCount);
    for (int bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        if ((channels[bb] != NULL) && (channels[bb]->done == 0) && (channels[bb]->sourceClip != NULL))
        {
            out->WriteInt32(((ScriptAudioClip*)channels[bb]->sourceClip)->id);
            out->WriteInt32(channels[bb]->get_pos());
            out->WriteInt32(channels[bb]->priority);
            out->WriteInt32(channels[bb]->repeat ? 1 : 0);
            out->WriteInt32(channels[bb]->vol);
            out->WriteInt32(channels[bb]->panning);
            out->WriteInt32(channels[bb]->volAsPercentage);
            out->WriteInt32(channels[bb]->panningAsPercentage);
        }
        else
        {
            out->WriteInt32(-1);
        }
    }
    out->WriteInt32(crossFading);
    out->WriteInt32(crossFadeVolumePerStep);
    out->WriteInt32(crossFadeStep);
    out->WriteInt32(crossFadeVolumeAtStart);
}

#define MAGICNUMBER 0xbeefcafe
// Write the save game position to the file
void save_game_data (DataStream *out, Bitmap *screenshot) {

    platform->RunPluginHooks(AGSE_PRESAVEGAME, 0);
    out->WriteInt32(SGVERSION);

    save_game_screenshot(out, screenshot);
    save_game_header(out);
    save_game_head_dynamic_values(out);
    save_game_spriteset(out);
    save_game_scripts(out);
    save_game_room_state(out);

    update_polled_stuff_if_runtime();

    if (play.cur_music_number >= 0) {
        if (IsMusicPlaying() == 0)
            play.cur_music_number = -1;
    }

    //----------------------------------------------------------------
    play.WriteToFile(out);
    //out->WriteArray(&play,sizeof(GameState),1);

    save_game_play_ex_data(out);
    //----------------------------------------------------------------

    save_game_movelist(out);

    ((GameSetupStructBase*)&game)->WriteToFile(out);

    //----------------------------------------------------------------
    game.WriteForSaveGame(out);

    save_game_charextras(out);
    save_game_palette(out);
    save_game_dialogs(out);
    save_game_more_dynamic_values(out);
    save_game_gui(out);
    save_game_audiocliptypes(out);
    save_game_thisroom(out);
    save_game_ambientsounds(out);
    save_game_overlays(out);

    update_polled_stuff_if_runtime();

    save_game_dynamic_surfaces(out);

    update_polled_stuff_if_runtime();

    save_game_displayed_room_status(out);
    save_game_globalvars(out);
    save_game_views(out);

    out->WriteInt32 (MAGICNUMBER+1);

    save_game_audioclips_and_crossfade(out);  

    // [IKM] Plugins expect FILE pointer! // TODO something with this later...
    platform->RunPluginHooks(AGSE_SAVEGAME, (long)((Common::FileStream*)out)->GetHandle());
    out->WriteInt32 (MAGICNUMBER);  // to verify the plugins

    // save the room music volume
    out->WriteInt32(thisroom.options[ST_VOLUME]);

    ccSerializeAllObjects(out);

    out->WriteInt32(current_music_type);

    update_polled_stuff_if_runtime();
}

void create_savegame_screenshot(Bitmap *&screenShot)
{
    if (game.options[OPT_SAVESCREENSHOT]) {
        int usewid = multiply_up_coordinate(play.screenshot_width);
        int usehit = multiply_up_coordinate(play.screenshot_height);
        if (usewid > virtual_screen->GetWidth())
            usewid = virtual_screen->GetWidth();
        if (usehit > virtual_screen->GetHeight())
            usehit = virtual_screen->GetHeight();

        if ((play.screenshot_width < 16) || (play.screenshot_height < 16))
            quit("!Invalid game.screenshot_width/height, must be from 16x16 to screen res");

        if (gfxDriver->UsesMemoryBackBuffer())
        {
            screenShot = BitmapHelper::CreateBitmap(usewid, usehit, virtual_screen->GetColorDepth());

            screenShot->StretchBlt(virtual_screen,
				RectWH(0, 0, virtual_screen->GetWidth(), virtual_screen->GetHeight()),
				RectWH(0, 0, screenShot->GetWidth(), screenShot->GetHeight()));
        }
        else
        {
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
            int color_depth = (psp_gfx_renderer > 0) ? 32 : final_col_dep;
#else
            int color_depth = final_col_dep;
#endif
            Bitmap *tempBlock = BitmapHelper::CreateBitmap(virtual_screen->GetWidth(), virtual_screen->GetHeight(), color_depth);
            gfxDriver->GetCopyOfScreenIntoBitmap(tempBlock);

            screenShot = BitmapHelper::CreateBitmap(usewid, usehit, color_depth);
            screenShot->StretchBlt(tempBlock,
				RectWH(0, 0, tempBlock->GetWidth(), tempBlock->GetHeight()),
				RectWH(0, 0, screenShot->GetWidth(), screenShot->GetHeight()));

            delete tempBlock;
        }
    }
}

void save_game(int slotn, const char*descript) {

    // dont allow save in rep_exec_always, because we dont save
    // the state of blocked scripts
    can_run_delayed_command();

    if (inside_script) {
        strcpy(curscript->postScriptSaveSlotDescription[curscript->queue_action(ePSASaveGame, slotn, "SaveGameSlot")], descript);
        return;
    }

    if (platform->GetDiskFreeSpaceMB() < 2) {
        Display("ERROR: There is not enough disk space free to save the game. Clear some disk space and try again.");
        return;
    }

    VALIDATE_STRING(descript);
    String nametouse;
    nametouse = get_save_game_path(slotn);

    DataStream *out = Common::File::CreateFile(nametouse);
    if (out == NULL)
        quit("save_game: unable to open savegame file for writing");

    // Initialize and write Vista header
    RICH_GAME_MEDIA_HEADER vistaHeader;
    memset(&vistaHeader, 0, sizeof(RICH_GAME_MEDIA_HEADER));
    memcpy(&vistaHeader.dwMagicNumber, RM_MAGICNUMBER, sizeof(int));
    vistaHeader.dwHeaderVersion = 1;
    vistaHeader.dwHeaderSize = sizeof(RICH_GAME_MEDIA_HEADER);
    vistaHeader.dwThumbnailOffsetHigherDword = 0;
    vistaHeader.dwThumbnailOffsetLowerDword = 0;
    vistaHeader.dwThumbnailSize = 0;
    convert_guid_from_text_to_binary(game.guid, &vistaHeader.guidGameId[0]);
    uconvert(game.gamename, U_ASCII, (char*)&vistaHeader.szGameName[0], U_UNICODE, RM_MAXLENGTH);
    uconvert(descript, U_ASCII, (char*)&vistaHeader.szSaveName[0], U_UNICODE, RM_MAXLENGTH);
    vistaHeader.szLevelName[0] = 0;
    vistaHeader.szComments[0] = 0;

    //===================================================================
    // Started writing to the file here

    // Extended savegame info for Win Vista and higher
    vistaHeader.WriteToFile(out);

    // Savegame signature
    out->Write(sgsig,sgsiglen);

    safeguard_string ((unsigned char*)descript);

    fputstring((char*)descript, out);

    Bitmap *screenShot = NULL;

    // Screenshot
    create_savegame_screenshot(screenShot);

    update_polled_stuff_if_runtime();

    // Actual dynamic game data is saved here
    save_game_data(out, screenShot);

    // End writing to the file here
    //===================================================================

    if (screenShot != NULL)
    {
        int screenShotOffset = out->GetPosition() - sizeof(RICH_GAME_MEDIA_HEADER);
        int screenShotSize = write_screen_shot_for_vista(out, screenShot);
        delete out;

        update_polled_stuff_if_runtime();

        out = Common::File::OpenFile(nametouse, Common::kFile_Open, Common::kFile_ReadWrite);
        out->Seek(Common::kSeekBegin, 12);
        out->WriteInt32(screenShotOffset);
        out->Seek(Common::kSeekCurrent, 4);
        out->WriteInt32(screenShotSize);
    }

    if (screenShot != NULL)
        delete screenShot;

    delete out;
}

char rbuffer[200];

Bitmap *restore_game_screenshot(DataStream *in)
{
    int isScreen = in->ReadInt32();
    if (isScreen) {
        return read_serialized_bitmap(in);
    }
    return NULL;
}

int restore_game_header(DataStream *in)
{
    fgetstring_limit(rbuffer, in, 200);
    int vercmp = strcmp(rbuffer, ACI_VERSION_TEXT);
    if ((vercmp > 0) || (strcmp(rbuffer, LOWEST_SGVER_COMPAT) < 0) ||
        (strlen(rbuffer) > strlen(LOWEST_SGVER_COMPAT))) {
            return -4;
    }
    fgetstring_limit (rbuffer, in, 180);
    rbuffer[180] = 0;
    if (stricmp (rbuffer, usetup.main_data_filename)) {
        return -5;
    }

    return 0;
}

int restore_game_head_dynamic_values(DataStream *in, int &sg_cur_mode, int &sg_cur_cursor)
{
    int gamescrnhit = in->ReadInt32();
    // a 320x240 game, they saved in a 320x200 room but try to restore
    // from within a 320x240 room, make it work
    if (final_scrn_hit == (gamescrnhit * 12) / 10)
        gamescrnhit = scrnhit;
    // they saved in a 320x240 room but try to restore from a 320x200
    // room, fix it
    else if (gamescrnhit == final_scrn_hit)
        gamescrnhit = scrnhit;

    if (gamescrnhit != scrnhit) {
        Display("This game was saved with the interpreter running at a different "
            "resolution. It cannot be restored.");
        return -6;
    }

    if (in->ReadInt32() != final_col_dep) {
        Display("This game was saved with the engine running at a different colour depth. It cannot be restored.");
        return -7;
    }

    unload_old_room();

    remove_screen_overlay(-1);
    is_complete_overlay=0; is_text_overlay=0;
    set_game_speed(in->ReadInt32());
    sg_cur_mode=in->ReadInt32();
    sg_cur_cursor=in->ReadInt32();
    offsetx = in->ReadInt32();
    offsety = in->ReadInt32();
    loopcounter = in->ReadInt32();

    return 0;
}

void restore_game_spriteset(DataStream *in)
{
    for (int bb = 1; bb < spriteset.elements; bb++) {
        if (game.spriteflags[bb] & SPF_DYNAMICALLOC) {
            // do this early, so that it changing guibuts doesn't
            // affect the restored data
            free_dynamic_sprite(bb);
        }
    }
    // ensure the sprite set is at least as large as it was
    // when the game was saved
    spriteset.enlargeTo(in->ReadInt32());
    // get serialized dynamic sprites
    int sprnum = in->ReadInt32();
    while (sprnum) {
        unsigned char spriteflag = in->ReadByte();
        add_dynamic_sprite(sprnum, read_serialized_bitmap(in));
        game.spriteflags[sprnum] = spriteflag;
        sprnum = in->ReadInt32();
    }
}

void restore_game_clean_gfx()
{
    for (int vv = 0; vv < game.numgui; vv++) {
        delete guibg[vv];
        guibg[vv] = NULL;

        if (guibgbmp[vv])
            gfxDriver->DestroyDDB(guibgbmp[vv]);
        guibgbmp[vv] = NULL;
    }
}

void restore_game_clean_scripts()
{
    ccFreeInstance(gameinstFork);
    ccFreeInstance(gameinst);
    gameinstFork = NULL;
    gameinst = NULL;
    for (int vv = 0; vv < numScriptModules; vv++) {
        ccFreeInstance(moduleInstFork[vv]);
        ccFreeInstance(moduleInst[vv]);
        moduleInst[vv] = NULL;
    }

    if (dialogScriptsInst != NULL)
    {
        ccFreeInstance(dialogScriptsInst);
        dialogScriptsInst = NULL;
    }
}

void restore_game_scripts(DataStream *in, int &gdatasize, char **newglobaldatabuffer, 
                          char **scriptModuleDataBuffers, int *scriptModuleDataSize)
{
    // read the global script data segment
    gdatasize = in->ReadInt32();
    *newglobaldatabuffer = (char*)malloc(gdatasize);
    in->Read(*newglobaldatabuffer, gdatasize);
    //in->ReadArray(&gameinst->globaldata[0],gdatasize,1);
    //ccUnFlattenGlobalData (gameinst);



    if (in->ReadInt32() != numScriptModules)
        quit("wrong script module count; cannot restore game");
    for (int vv = 0; vv < numScriptModules; vv++) {
        scriptModuleDataSize[vv] = in->ReadInt32();
        scriptModuleDataBuffers[vv] = (char*)malloc(scriptModuleDataSize[vv]);
        in->Read(&scriptModuleDataBuffers[vv][0], scriptModuleDataSize[vv]);
    }
}

void restore_game_room_state(DataStream *in, const char *nametouse)
{
    int vv;

    displayed_room = in->ReadInt32();

    // now the rooms
    resetRoomStatuses();

    // JJS: What was the point in closing and reopening the file?
    /*
    long gobackto = in->GetPosition();
    delete in;
    in = Common::File::OpenFileRead(nametouse);
    in->Seek(Common::kSeekBegin, gobackto);
    */

    // read the room state for all the rooms the player has been in
    RoomStatus* roomstat;
    int beenhere;
    for (vv=0;vv<MAX_ROOMS;vv++)
    {
        beenhere = in->ReadByte();
        if (beenhere)
        {
            roomstat = getRoomStatus(vv);
            if ((roomstat->tsdatasize > 0) && (roomstat->tsdata != NULL))
                free(roomstat->tsdata);
            roomstat->tsdatasize = 0;
            roomstat->tsdata = NULL;
            roomstat->beenhere = beenhere;

            if (roomstat->beenhere)
            {
                roomstat->ReadFromFile(in);
                if (roomstat->tsdatasize > 0)
                {
                    roomstat->tsdata=(char*)malloc(roomstat->tsdatasize + 8);  // JJS: Why allocate 8 additional bytes?
                    in->Read(&roomstat->tsdata[0], roomstat->tsdatasize);
                }
            }
        }
    }
}

void restore_game_play(DataStream *in)
{
    int speech_was = play.want_speech, musicvox = play.seperate_music_lib;
    // preserve the replay settings
    int playback_was = play.playback, recording_was = play.recording;
    int gamestep_was = play.gamestep;
    int screenfadedout_was = play.screen_is_faded_out;
    int roomchanges_was = play.room_changes;
    // make sure the pointer is preserved
    int *gui_draw_order_was = play.gui_draw_order;

    free_do_once_tokens();

    //in->ReadArray (&play, 76, 4);
    //in->ReadArray (((char*)&play) + 78*4, sizeof(GameState) - 78*4, 1);

    //in->ReadArray(&play,sizeof(GameState),1);
    play.ReadFromFile(in);

    // Preserve whether the music vox is available
    play.seperate_music_lib = musicvox;
    // If they had the vox when they saved it, but they don't now
    if ((speech_was < 0) && (play.want_speech >= 0))
        play.want_speech = (-play.want_speech) - 1;
    // If they didn't have the vox before, but now they do
    else if ((speech_was >= 0) && (play.want_speech < 0))
        play.want_speech = (-play.want_speech) - 1;

    play.screen_is_faded_out = screenfadedout_was;
    play.playback = playback_was;
    play.recording = recording_was;
    play.gamestep = gamestep_was;
    play.room_changes = roomchanges_was;
    play.gui_draw_order = gui_draw_order_was;

    if (play.num_do_once_tokens > 0)
    {
        play.do_once_tokens = (char**)malloc(sizeof(char*) * play.num_do_once_tokens);
        for (int bb = 0; bb < play.num_do_once_tokens; bb++)
        {
            fgetstring_limit(rbuffer, in, 200);
            play.do_once_tokens[bb] = (char*)malloc(strlen(rbuffer) + 1);
            strcpy(play.do_once_tokens[bb], rbuffer);
        }
    }

    in->ReadArrayOfInt32(&play.gui_draw_order[0], game.numgui);
}

void restore_game_movelist(DataStream *in)
{
    for (int i = 0; i < game.numcharacters + MAX_INIT_SPR + 1; ++i)
    {
        mls[i].ReadFromFile(in);
    }
    //in->ReadArray(&mls[0],sizeof(MoveList), game.numcharacters + MAX_INIT_SPR + 1);
}

void restore_game_charextras(DataStream *in)
{
    for (int i = 0; i < game.numcharacters; ++i)
    {
        charextra[i].ReadFromFile(in);
    }
    //in->ReadArray(&charextra[0],sizeof(CharacterExtras),game.numcharacters);
}

void restore_game_palette(DataStream *in)
{
    in->ReadArray(&palette[0],sizeof(color),256);
}

void restore_game_dialogs(DataStream *in)
{
    for (int vv=0;vv<game.numdialog;vv++)
        in->ReadArrayOfInt32(&dialog[vv].optionflags[0],MAXTOPICOPTIONS);
}

void restore_game_more_dynamic_values(DataStream *in)
{
    mouse_on_iface=in->ReadInt32();
    mouse_on_iface_button=in->ReadInt32();
    mouse_pushed_iface=in->ReadInt32();
    ifacepopped = in->ReadInt32();
    game_paused=in->ReadInt32();
}

void restore_game_gui(DataStream *in, int numGuisWas)
{
    int vv;
    for (vv = 0; vv < game.numgui; vv++)
        unexport_gui_controls(vv);

    read_gui(in,guis,&game);

    if (numGuisWas != game.numgui)
        quit("!Restore_Game: Game has changed (GUIs), unable to restore position");

    for (vv = 0; vv < game.numgui; vv++)
        export_gui_controls(vv);

    numAnimButs = in->ReadInt32();
    for (int i = 0; i < numAnimButs; ++i)
    {
        animbuts[i].ReadFromFile(in);
    }
    int padding = get_padding(sizeof(AnimatingGUIButton) * numAnimButs);
    in->Seek(Common::kSeekCurrent, padding);
}

void restore_game_audiocliptypes(DataStream *in)
{
    if (in->ReadInt32() != game.audioClipTypeCount)
        quit("!Restore_Game: game has changed (audio types), unable to restore");

    for (int i = 0; i < game.audioClipTypeCount; ++i)
    {
        game.audioClipTypes[i].ReadFromFile(in);
    }
}

void restore_game_thisroom(DataStream *in, short *saved_light_levels, int *saved_tint_levels,
                           short *saved_zoom_levels1, short *saved_zoom_levels2)
{
    in->ReadArrayOfInt16(&saved_light_levels[0],MAX_REGIONS);
    in->ReadArrayOfInt32(&saved_tint_levels[0], MAX_REGIONS);
    in->ReadArrayOfInt16(&saved_zoom_levels1[0],MAX_WALK_AREAS + 1);
    in->ReadArrayOfInt16(&saved_zoom_levels2[0],MAX_WALK_AREAS + 1);
}

void restore_game_ambientsounds(DataStream *in, int crossfadeInChannelWas, int crossfadeOutChannelWas,
                                int *doAmbient)
{
    int bb;
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        stop_and_destroy_channel_ex(bb, false);
    }

    play.crossfading_in_channel = crossfadeInChannelWas;
    play.crossfading_out_channel = crossfadeOutChannelWas;

    for (int i = 0; i < MAX_SOUND_CHANNELS; ++i)
    {
        ambient[i].ReadFromFile(in);
    }
    //in->ReadArray(&ambient[0], sizeof(AmbientSound), MAX_SOUND_CHANNELS);

    for (bb = 1; bb < MAX_SOUND_CHANNELS; bb++) {
        if (ambient[bb].channel == 0)
            doAmbient[bb] = 0;
        else {
            doAmbient[bb] = ambient[bb].num;
            ambient[bb].channel = 0;
        }
    }
}

void restore_game_overlays(DataStream *in)
{
    numscreenover = in->ReadInt32();
    for (int i = 0; i < numscreenover; ++i)
    {
        screenover[i].ReadFromFile(in);
    }
    //in->ReadArray(&screenover[0],sizeof(ScreenOverlay),numscreenover);
    for (int bb=0;bb<numscreenover;bb++) {
        if (screenover[bb].pic != NULL)
        {
            screenover[bb].pic = read_serialized_bitmap(in);
            screenover[bb].bmp = gfxDriver->CreateDDBFromBitmap(screenover[bb].pic, false);
        }
    }
}

void restore_game_dynamic_surfaces(DataStream *in, Bitmap **dynamicallyCreatedSurfacesFromSaveGame)
{
    // load into a temp array since ccUnserialiseObjects will destroy
    // it otherwise
    for (int bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
    {
        if (in->ReadInt8() == 0)
        {
            dynamicallyCreatedSurfacesFromSaveGame[bb] = NULL;
        }
        else
        {
            dynamicallyCreatedSurfacesFromSaveGame[bb] = read_serialized_bitmap(in);
        }
    }
}

void restore_game_displayed_room_status(DataStream *in, Bitmap **newbscene)
{
    int bb;
    for (bb = 0; bb < MAX_BSCENE; bb++)
        newbscene[bb] = NULL;

    if (displayed_room >= 0) {

        for (bb = 0; bb < MAX_BSCENE; bb++) {
            newbscene[bb] = NULL;
            if (play.raw_modified[bb]) {
                newbscene[bb] = read_serialized_bitmap (in);
            }
        }
        bb = in->ReadInt32();

        delete raw_saved_screen;
        raw_saved_screen = NULL;

        if (bb)
            raw_saved_screen = read_serialized_bitmap(in);

        if (troom.tsdata != NULL)
            free (troom.tsdata);
        // get the current troom, in case they save in room 600 or whatever
        troom.ReadFromFile(in);
        //in->ReadArray(&troom,sizeof(RoomStatus),1);
        if (troom.tsdatasize > 0) {
            troom.tsdata=(char*)malloc(troom.tsdatasize+5);
            in->Read(&troom.tsdata[0],troom.tsdatasize);
        }
        else
            troom.tsdata = NULL;

    }
}

void restore_game_globalvars(DataStream *in)
{
    if (in->ReadInt32() != numGlobalVars) 
        quit("!Game has been modified since save; unable to restore game (GM01)");

    for (int i = 0; i < numGlobalVars; ++i)
    {
        globalvars[i].ReadFromFile(in);
    }
}

void restore_game_views(DataStream *in)
{
    if (in->ReadInt32() != game.numviews)
        quit("!Game has been modified since save; unable to restore (GV02)");

    for (int bb = 0; bb < game.numviews; bb++) {
        for (int cc = 0; cc < views[bb].numLoops; cc++) {
            for (int dd = 0; dd < views[bb].loops[cc].numFrames; dd++)
            {
                views[bb].loops[cc].frames[dd].sound = in->ReadInt32();
                views[bb].loops[cc].frames[dd].pic = in->ReadInt32();
            }
        }
    }
}

void restore_game_audioclips_and_crossfade(DataStream *in, int crossfadeInChannelWas, int crossfadeOutChannelWas)
{
    int bb;

    if (in->ReadInt32() != game.audioClipCount)
        quit("Game has changed: different audio clip count");

    play.crossfading_in_channel = 0;
    play.crossfading_out_channel = 0;
    int channelPositions[MAX_SOUND_CHANNELS + 1];
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        channelPositions[bb] = 0;
        int audioClipIndex = in->ReadInt32();
        if (audioClipIndex >= 0)
        {
            if (audioClipIndex >= game.audioClipCount)
                quit("save game error: invalid audio clip index");

            channelPositions[bb] = in->ReadInt32();
            if (channelPositions[bb] < 0) channelPositions[bb] = 0;
            int priority = in->ReadInt32();
            int repeat = in->ReadInt32();
            int vol = in->ReadInt32();
            int pan = in->ReadInt32();
            int volAsPercent = in->ReadInt32();
            int panAsPercent = in->ReadInt32();
            play_audio_clip_on_channel(bb, &game.audioClips[audioClipIndex], priority, repeat, channelPositions[bb]);
            if (channels[bb] != NULL)
            {
                channels[bb]->set_panning(pan);
                channels[bb]->set_volume(vol);
                channels[bb]->panningAsPercentage = panAsPercent;
                channels[bb]->volAsPercentage = volAsPercent;
            }
        }
    }
    if ((crossfadeInChannelWas > 0) && (channels[crossfadeInChannelWas] != NULL))
        play.crossfading_in_channel = crossfadeInChannelWas;
    if ((crossfadeOutChannelWas > 0) && (channels[crossfadeOutChannelWas] != NULL))
        play.crossfading_out_channel = crossfadeOutChannelWas;

    // If there were synced audio tracks, the time taken to load in the
    // different channels will have thrown them out of sync, so re-time it
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        if ((channelPositions[bb] > 0) && (channels[bb] != NULL) && (channels[bb]->done == 0))
        {
            channels[bb]->seek(channelPositions[bb]);
        }
    }
    crossFading = in->ReadInt32();
    crossFadeVolumePerStep = in->ReadInt32();
    crossFadeStep = in->ReadInt32();
    crossFadeVolumeAtStart = in->ReadInt32();
}

int restore_game_data (DataStream *in, const char *nametouse) {

    int bb, vv;

    int sg_cur_mode = 0, sg_cur_cursor = 0;
    int res = restore_game_head_dynamic_values(in, /*out*/ sg_cur_mode, sg_cur_cursor);
    if (res != 0) {
        return res;
    }

    restore_game_spriteset(in); 

    clear_music_cache();
    restore_game_clean_gfx();

    update_polled_stuff_if_runtime();

    restore_game_clean_scripts();

    update_polled_stuff_if_runtime();

    int gdatasize = 0;
    char*newglobaldatabuffer;
    char *scriptModuleDataBuffers[MAX_SCRIPT_MODULES];
    int scriptModuleDataSize[MAX_SCRIPT_MODULES];
    restore_game_scripts(in, /*out*/ gdatasize,&newglobaldatabuffer, scriptModuleDataBuffers, scriptModuleDataSize);
    restore_game_room_state(in, nametouse);

    restore_game_play(in);

    restore_game_movelist(in);

    // save pointer members before reading
    char* gswas=game.globalscript;
    ccScript* compsc=game.compiled_script;
    CharacterInfo* chwas=game.chars;
    WordsDictionary *olddict = game.dict;
    char* mesbk[MAXGLOBALMES];
    int numchwas = game.numcharacters;
    for (vv=0;vv<MAXGLOBALMES;vv++) mesbk[vv]=game.messages[vv];
    int numdiwas = game.numdialog, numinvwas = game.numinvitems;
    int numviewswas = game.numviews;
    int numGuisWas = game.numgui;

    //in->ReadArray(&game,sizeof(GameSetupStructBase),1);
    GameSetupStructBase *gameBase = (GameSetupStructBase *) &game;
    gameBase->ReadFromFile(in);

    if (game.numdialog!=numdiwas)
        quit("!Restore_Game: Game has changed (dlg), unable to restore");
    if ((numchwas != game.numcharacters) || (numinvwas != game.numinvitems))
        quit("!Restore_Game: Game has changed (inv), unable to restore position");
    if (game.numviews != numviewswas)
        quit("!Restore_Game: Game has changed (views), unable to restore position");

    game.ReadFromSaveGame(in, gswas, compsc, chwas, olddict, mesbk); 
    //
    //in->ReadArray(&game.invinfo[0], sizeof(InventoryItemInfo), game.numinvitems);
    //in->ReadArray(&game.mcurs[0], sizeof(MouseCursor), game.numcursors);
    //
    //if (game.invScripts == NULL)
    //{
    //  for (bb = 0; bb < game.numinvitems; bb++)
    //    in->ReadArray (&game.intrInv[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS);
    //  for (bb = 0; bb < game.numcharacters; bb++)
    //    in->ReadArray (&game.intrChar[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS);
    //}
    //
    //// restore pointer members
    //game.globalscript=gswas;
    //game.compiled_script=compsc;
    //game.chars=chwas;
    //game.dict = olddict;
    //for (vv=0;vv<MAXGLOBALMES;vv++) game.messages[vv]=mesbk[vv];
    //
    //in->ReadArray(&game.options[0], sizeof(int), OPT_HIGHESTOPTION+1);
    //game.options[OPT_LIPSYNCTEXT] = in->ReadByte();
    //
    //in->ReadArray(&game.chars[0],sizeof(CharacterInfo),game.numcharacters);
    //

    restore_game_charextras(in);
    if (roominst!=NULL) {  // so it doesn't overwrite the tsdata
        ccFreeInstance(roominstFork);
        ccFreeInstance(roominst); 
        roominstFork = NULL;
        roominst=NULL;
    }
    restore_game_palette(in);
    restore_game_dialogs(in);
    restore_game_more_dynamic_values(in);

    restore_game_gui(in, numGuisWas);
    restore_game_audiocliptypes(in);

    short saved_light_levels[MAX_REGIONS];
    int   saved_tint_levels[MAX_REGIONS];
    short saved_zoom_levels1[MAX_WALK_AREAS + 1];
    short saved_zoom_levels2[MAX_WALK_AREAS + 1];
    restore_game_thisroom(in, saved_light_levels, saved_tint_levels, saved_zoom_levels1, saved_zoom_levels2);

    int crossfadeInChannelWas = play.crossfading_in_channel;
    int crossfadeOutChannelWas = play.crossfading_out_channel;
    int doAmbient[MAX_SOUND_CHANNELS];
    restore_game_ambientsounds(in, crossfadeInChannelWas, crossfadeOutChannelWas, doAmbient);
    restore_game_overlays(in);

    update_polled_stuff_if_runtime();

    Bitmap *dynamicallyCreatedSurfacesFromSaveGame[MAX_DYNAMIC_SURFACES];
    restore_game_dynamic_surfaces(in, dynamicallyCreatedSurfacesFromSaveGame);

    update_polled_stuff_if_runtime();

    Bitmap *newbscene[MAX_BSCENE];
    restore_game_displayed_room_status(in, newbscene);
    restore_game_globalvars(in);
    restore_game_views(in);

    if (in->ReadInt32() != MAGICNUMBER+1)
        quit("!Game has been modified since save; unable to restore (GV03)");

    restore_game_audioclips_and_crossfade(in, crossfadeInChannelWas, crossfadeOutChannelWas);

    recache_queued_clips_after_loading_save_game();

    // [IKM] Plugins expect FILE pointer! // TODO something with this later
    platform->RunPluginHooks(AGSE_RESTOREGAME, (long)((Common::FileStream*)in)->GetHandle());
    if (in->ReadInt32() != (unsigned)MAGICNUMBER)
        quit("!One of the game plugins did not restore its game data correctly.");

    // save the new room music vol for later use
    int newRoomVol = in->ReadInt32();

    if (ccUnserializeAllObjects(in, &ccUnserializer))
        quitprintf("LoadGame: Error during deserialization: %s", ccErrorString);

    // preserve legacy music type setting
    current_music_type = in->ReadInt32();

    // restore these to the ones retrieved from the save game
    for (bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
    {
        dynamicallyCreatedSurfaces[bb] = dynamicallyCreatedSurfacesFromSaveGame[bb];
    }

    if (create_global_script())
        quitprintf("Unable to recreate global script: %s", ccErrorString);

    if (gameinst->globaldatasize != gdatasize)
        quit("!Restore_game: Global script changed, cannot restore game");

    // read the global data into the newly created script
    memcpy(&gameinst->globaldata[0], newglobaldatabuffer, gdatasize);
    free(newglobaldatabuffer);
    ccUnFlattenGlobalData(gameinst);

    // restore the script module data
    for (bb = 0; bb < numScriptModules; bb++) {
        if (scriptModuleDataSize[bb] != moduleInst[bb]->globaldatasize)
            quit("!Restore Game: script module global data changed, unable to restore");
        memcpy(&moduleInst[bb]->globaldata[0], scriptModuleDataBuffers[bb], scriptModuleDataSize[bb]);
        free(scriptModuleDataBuffers[bb]);
        ccUnFlattenGlobalData(moduleInst[bb]);
    }


    setup_player_character(game.playercharacter);

    int gstimer=play.gscript_timer;
    int oldx1 = play.mboundx1, oldx2 = play.mboundx2;
    int oldy1 = play.mboundy1, oldy2 = play.mboundy2;
    int musicWasRepeating = play.current_music_repeating;
    int newms = play.cur_music_number;

    // disable the queue momentarily
    int queuedMusicSize = play.music_queue_size;
    play.music_queue_size = 0;

    update_polled_stuff_if_runtime();

    if (displayed_room >= 0)
        load_new_room(displayed_room,NULL);//&game.chars[game.playercharacter]);

    update_polled_stuff_if_runtime();

    play.gscript_timer=gstimer;

    // restore the correct room volume (they might have modified
    // it with SetMusicVolume)
    thisroom.options[ST_VOLUME] = newRoomVol;

    filter->SetMouseLimit(oldx1,oldy1,oldx2,oldy2);

    set_cursor_mode(sg_cur_mode);
    set_mouse_cursor(sg_cur_cursor);
    if (sg_cur_mode == MODE_USE)
        SetActiveInventory (playerchar->activeinv);
    // ensure that the current cursor is locked
    spriteset.precache(game.mcurs[sg_cur_cursor].pic);

#if (ALLEGRO_DATE > 19990103)
    set_window_title(play.game_name);
#endif

    update_polled_stuff_if_runtime();

    if (displayed_room >= 0) {

        for (bb = 0; bb < MAX_BSCENE; bb++) {
            if (newbscene[bb]) {
                delete thisroom.ebscene[bb];
                thisroom.ebscene[bb] = newbscene[bb];
            }
        }

        in_new_room=3;  // don't run "enters screen" events
        // now that room has loaded, copy saved light levels in
        memcpy(&thisroom.regionLightLevel[0],&saved_light_levels[0],sizeof(short)*MAX_REGIONS);
        memcpy(&thisroom.regionTintLevel[0],&saved_tint_levels[0],sizeof(int)*MAX_REGIONS);
        generate_light_table();

        memcpy(&thisroom.walk_area_zoom[0], &saved_zoom_levels1[0], sizeof(short) * (MAX_WALK_AREAS + 1));
        memcpy(&thisroom.walk_area_zoom2[0], &saved_zoom_levels2[0], sizeof(short) * (MAX_WALK_AREAS + 1));

        on_background_frame_change();

    }

    gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);
    /*
    play_sound(-1);

    stopmusic();
    // use the repeat setting when the current track was started
    int musicRepeatSetting = play.music_repeat;
    SetMusicRepeat(musicWasRepeating);
    if (newms>=0) {
    // restart the background music
    if (newms == 1000)
    PlayMP3File (play.playmp3file_name);
    else {
    play.cur_music_number=2000;  // make sure it gets played
    newmusic(newms);
    }
    }
    SetMusicRepeat(musicRepeatSetting);
    if (play.silent_midi)
    PlaySilentMIDI (play.silent_midi);
    SeekMIDIPosition(midipos);
    //SeekMODPattern (modtrack);
    //SeekMP3PosMillis (mp3mpos);

    if (musicpos > 0) {
    // For some reason, in Prodigal after this Seek line is called
    // it can cause the next update_polled_stuff to crash;
    // must be some sort of bug in AllegroMP3
    if ((crossFading > 0) && (channels[crossFading] != NULL))
    channels[crossFading]->seek(musicpos);
    else if (channels[SCHAN_MUSIC] != NULL)
    channels[SCHAN_MUSIC]->seek(musicpos);
    }*/

    // restore the queue now that the music is playing
    play.music_queue_size = queuedMusicSize;

    if (play.digital_master_volume >= 0)
        System_SetVolume(play.digital_master_volume);

    for (vv = 1; vv < MAX_SOUND_CHANNELS; vv++) {
        if (doAmbient[vv])
            PlayAmbientSound(vv, doAmbient[vv], ambient[vv].vol, ambient[vv].x, ambient[vv].y);
    }

    for (vv = 0; vv < game.numgui; vv++) {
        guibg[vv] = BitmapHelper::CreateBitmap (guis[vv].wid, guis[vv].hit, final_col_dep);
        guibg[vv] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[vv]);
    }

    if (gfxDriver->SupportsGammaControl())
        gfxDriver->SetGamma(play.gamma_adjustment);

    guis_need_update = 1;

    play.ignore_user_input_until_time = 0;
    update_polled_stuff_if_runtime();

    platform->RunPluginHooks(AGSE_POSTRESTOREGAME, 0);

    if (displayed_room < 0) {
        // the restart point, no room was loaded
        load_new_room(playerchar->room, playerchar);
        playerchar->prevroom = -1;

        first_room_initialization();
    }

    if ((play.music_queue_size > 0) && (cachedQueuedMusic == NULL)) {
        cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
    }

    return 0;
}

int gameHasBeenRestored = 0;
int oldeip;

DataStream *open_savedgame(const char *savedgame, int &error_code)
{
    error_code = 0;
    DataStream *in = Common::File::OpenFileRead(savedgame);
    if (!in)
    {
        error_code = -1;
        return in;
    }

    // skip Vista header
    RICH_GAME_MEDIA_HEADER rich_media_header;
    rich_media_header.ReadFromFile(in);

    // check saved game signature
    in->Read(rbuffer, sgsiglen);
    rbuffer[sgsiglen] = 0;
    if (strcmp(rbuffer, sgsig) != 0) {
        // not a save game
        delete in;
        error_code = -2;
        return NULL;
    }

    int oldeip = our_eip;
    our_eip = 2050;

    // read description
    fgetstring_limit(rbuffer, in, 180);
    rbuffer[180] = 0;
    safeguard_string ((unsigned char*)rbuffer);

    // check saved game format version
    if (in->ReadInt32() != SGVERSION) {
        delete in;
        error_code = -3;
        return NULL;
    }

    return in;
}

int read_savedgame_description(const String &savedgame, String &description)
{
    int error_code;
    // yeah, I know what you think... this will be remade someday
    delete open_savedgame(savedgame, error_code);
    if (error_code == 0)
    {
        description = rbuffer;
        our_eip = oldeip;
    }
    return error_code;
}

int read_savedgame_screenshot(const String &savedgame, int &want_shot)
{
    want_shot = 0;

    int error_code;
    DataStream *in = open_savedgame(savedgame, error_code);
    if (!in)
    {
        return error_code;
    }

    Bitmap *screenshot = restore_game_screenshot(in);
    if (screenshot)
    {
        int slot = spriteset.findFreeSlot();
        if (slot > 0)
        {
            // add it into the sprite set
            add_dynamic_sprite(slot, gfxDriver->ConvertBitmapToSupportedColourDepth(screenshot));
            want_shot = slot;
        }
        else
        {
            delete screenshot;
        }
    }

    delete in;
    our_eip = oldeip;
    return 0;
}

int load_game(int slotNumber)
{
    return load_game(get_save_game_path(slotNumber), slotNumber);
}

int load_game(const Common::String &path, int slotNumber)
{
    gameHasBeenRestored++;

    int error_code;
    DataStream *in = open_savedgame(path, error_code);
    if (!in)
    {
        return error_code;
    }

    our_eip = 2051;

    delete restore_game_screenshot(in);  // [IKM] how very appropriate

    error_code = restore_game_header(in);

    // saved in different game
    if (error_code == -5) {
        // [IKM] 2012-11-26: this is a workaround, indeed.
        // Try to find wanted game's executable; if it does not exist,
        // continue loading savedgame in current game, and pray for the best
        get_current_dir_path(gamefilenamebuf, rbuffer);
        if (Common::File::TestReadFile(gamefilenamebuf))
        {
            delete in;
            RunAGSGame (rbuffer, 0, 0);
            load_new_game_restore = slotNumber;
            return 0;
        }
        Common::Out::FPrint("WARNING: the saved game '%s' references game file '%s', but it cannot be found in the current directory.", path.GetCStr(), gamefilenamebuf);
        Common::Out::FPrint("Trying to restore in the running game instead.");
    }
    else if (error_code != 0) {
        delete in;
        return error_code;
    }

    // do the actual restore
    error_code = restore_game_data(in, path);
    delete in;
    our_eip = oldeip;

    if (error_code)
    {
        return error_code;
    }

    run_on_event (GE_RESTORE_GAME, slotNumber);

    // ensure keyboard buffer is clean
    // use the raw versions rather than the rec_ versions so we don't
    // interfere with the replay sync
    while (keypressed()) readkey();
    return 0;
}

void start_skipping_cutscene () {
    play.fast_forward = 1;
    // if a drop-down icon bar is up, remove it as it will pause the game
    if (ifacepopped>=0)
        remove_popup_interface(ifacepopped);

    // if a text message is currently displayed, remove it
    if (is_text_overlay > 0)
        remove_screen_overlay(OVER_TEXTMSG);

}

void check_skip_cutscene_keypress (int kgn) {

    if ((play.in_cutscene > 0) && (play.in_cutscene != 3)) {
        if ((kgn != 27) && ((play.in_cutscene == 1) || (play.in_cutscene == 5)))
            ;
        else
            start_skipping_cutscene();
    }

}

// Helper functions used by StartCutscene/EndCutscene, but also
// by SkipUntilCharacterStops
void initialize_skippable_cutscene() {
    play.end_cutscene_music = -1;
}

void stop_fast_forwarding() {
    // when the skipping of a cutscene comes to an end, update things
    play.fast_forward = 0;
    setpal();
    if (play.end_cutscene_music >= 0)
        newmusic(play.end_cutscene_music);

    // Restore actual volume of sounds
    for (int aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
    {
        if ((channels[aa] != NULL) && (!channels[aa]->done) && 
            (channels[aa]->volAsPercentage == 0) &&
            (channels[aa]->originalVolAsPercentage > 0)) 
        {
            channels[aa]->volAsPercentage = channels[aa]->originalVolAsPercentage;
            channels[aa]->set_volume((channels[aa]->volAsPercentage * 255) / 100);
        }
    }

    update_music_volume();
}

// allowHotspot0 defines whether Hotspot 0 returns LOCTYPE_HOTSPOT
// or whether it returns 0
int __GetLocationType(int xxx,int yyy, int allowHotspot0) {
    getloctype_index = 0;
    // If it's not in ProcessClick, then return 0 when over a GUI
    if ((GetGUIAt(xxx, yyy) >= 0) && (getloctype_throughgui == 0))
        return 0;

    getloctype_throughgui = 0;

    xxx += divide_down_coordinate(offsetx);
    yyy += divide_down_coordinate(offsety);
    if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
        return 0;

    // check characters, objects and walkbehinds, work out which is
    // foremost visible to the player
    int charat = is_pos_on_character(xxx,yyy);
    int hsat = get_hotspot_at(xxx,yyy);
    int objat = GetObjectAt(xxx - divide_down_coordinate(offsetx), yyy - divide_down_coordinate(offsety));

    multiply_up_coordinates(&xxx, &yyy);

    int wbat = thisroom.object->GetPixel(xxx, yyy);

    if (wbat <= 0) wbat = 0;
    else wbat = croom->walkbehind_base[wbat];

    int winner = 0;
    // if it's an Ignore Walkbehinds object, then ignore the walkbehind
    if ((objat >= 0) && ((objs[objat].flags & OBJF_NOWALKBEHINDS) != 0))
        wbat = 0;
    if ((charat >= 0) && ((game.chars[charat].flags & CHF_NOWALKBEHINDS) != 0))
        wbat = 0;

    if ((charat >= 0) && (objat >= 0)) {
        if ((wbat > obj_lowest_yp) && (wbat > char_lowest_yp))
            winner = LOCTYPE_HOTSPOT;
        else if (obj_lowest_yp > char_lowest_yp)
            winner = LOCTYPE_OBJ;
        else
            winner = LOCTYPE_CHAR;
    }
    else if (charat >= 0) {
        if (wbat > char_lowest_yp)
            winner = LOCTYPE_HOTSPOT;
        else
            winner = LOCTYPE_CHAR;
    }
    else if (objat >= 0) {
        if (wbat > obj_lowest_yp)
            winner = LOCTYPE_HOTSPOT;
        else
            winner = LOCTYPE_OBJ;
    }

    if (winner == 0) {
        if (hsat >= 0)
            winner = LOCTYPE_HOTSPOT;
    }

    if ((winner == LOCTYPE_HOTSPOT) && (!allowHotspot0) && (hsat == 0))
        winner = 0;

    if (winner == LOCTYPE_HOTSPOT)
        getloctype_index = hsat;
    else if (winner == LOCTYPE_CHAR)
        getloctype_index = charat;
    else if (winner == LOCTYPE_OBJ)
        getloctype_index = objat;

    return winner;
}

void display_switch_out() {
    // this is only called if in SWITCH_PAUSE mode
    //debug_log("display_switch_out");

    switching_away_from_game++;

    platform->DisplaySwitchOut();

    // allow background running temporarily to halt the sound
    if (set_display_switch_mode(SWITCH_BACKGROUND) == -1)
        set_display_switch_mode(SWITCH_BACKAMNESIA);

    // stop the sound stuttering
    for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) {
        if ((channels[i] != NULL) && (channels[i]->done == 0)) {
            channels[i]->pause();
        }
    }

    rest(1000);

    // restore the callbacks
    SetMultitasking(0);

    switching_away_from_game--;
}

void display_switch_in() {
    for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) {
        if ((channels[i] != NULL) && (channels[i]->done == 0)) {
            channels[i]->resume();
        }
    }

    // This can cause a segfault on Linux
#if !defined (LINUX_VERSION)
    if (gfxDriver->UsesMemoryBackBuffer())  // make sure all borders are cleared
        gfxDriver->ClearRectangle(0, 0, final_scrn_wid - 1, final_scrn_hit - 1, NULL);
#endif

    platform->DisplaySwitchIn();
}

void replace_tokens(char*srcmes,char*destm, int maxlen) {
    int indxdest=0,indxsrc=0;
    char*srcp,*destp;
    while (srcmes[indxsrc]!=0) {
        srcp=&srcmes[indxsrc];
        destp=&destm[indxdest];
        if ((strncmp(srcp,"@IN",3)==0) | (strncmp(srcp,"@GI",3)==0)) {
            int tokentype=0;
            if (srcp[1]=='I') tokentype=1;
            else tokentype=2;
            int inx=atoi(&srcp[3]);
            srcp++;
            indxsrc+=2;
            while (srcp[0]!='@') {
                if (srcp[0]==0) quit("!Display: special token not terminated");
                srcp++;
                indxsrc++;
            }
            char tval[10];
            if (tokentype==1) {
                if ((inx<1) | (inx>=game.numinvitems))
                    quit("!Display: invalid inv item specified in @IN@");
                sprintf(tval,"%d",playerchar->inv[inx]);
            }
            else {
                if ((inx<0) | (inx>=MAXGSVALUES))
                    quit("!Display: invalid global int index speicifed in @GI@");
                sprintf(tval,"%d",GetGlobalInt(inx));
            }
            strcpy(destp,tval);
            indxdest+=strlen(tval);
        }
        else {
            destp[0]=srcp[0];
            indxdest++;
            indxsrc++;
        }
        if (indxdest >= maxlen - 3)
            break;
    }
    destm[indxdest]=0;
}

char *get_global_message (int msnum) {
    if (game.messages[msnum-500] == NULL)
        return "";
    return get_translation(game.messages[msnum-500]);
}

void get_message_text (int msnum, char *buffer, char giveErr) {
    int maxlen = 9999;
    if (!giveErr)
        maxlen = MAX_MAXSTRLEN;

    if (msnum>=500) { //quit("global message requseted, nto yet supported");

        if ((msnum >= MAXGLOBALMES + 500) || (game.messages[msnum-500]==NULL)) {
            if (giveErr)
                quit("!DisplayGlobalMessage: message does not exist");
            buffer[0] = 0;
            return;
        }
        buffer[0] = 0;
        replace_tokens(get_translation(game.messages[msnum-500]), buffer, maxlen);
        return;
    }
    else if (msnum >= thisroom.nummes) {
        if (giveErr)
            quit("!DisplayMessage: Invalid message number to display");
        buffer[0] = 0;
        return;
    }

    buffer[0]=0;
    replace_tokens(get_translation(thisroom.message[msnum]), buffer, maxlen);
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
        if (stricmp (globalvars[ii].name, varName) == 0)
            return &globalvars[ii];
    }
    for (ii = 0; ii < thisroom.numLocalVars; ii++) {
        if (stricmp (thisroom.localvars[ii].name, varName) == 0)
            return &thisroom.localvars[ii];
    }
    return NULL;
}

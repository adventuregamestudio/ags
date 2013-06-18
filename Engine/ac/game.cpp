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

#if !defined (WINDOWS_VERSION)
#include <sys/stat.h>                      //mkdir
#endif
#include "ac/common.h"
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
#include "ac/game.h"
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
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/timer.h"
#include "ac/translation.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "game/game_objects.h"
#include "game/savedgame.h"
#include "game/script_objects.h"
#include "gfx/ali3d.h"
#include "gui/animatingguibutton.h"
#include "gfx/graphicsdriver.h"
#include "gui/guidialog.h"
#include "main/game_file.h"
#include "main/main.h"
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "plugin/agsplugin.h"
#include "script/cc_error.h"
#include "script/runtimescriptvalue.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "util/alignedstream.h"
#include "util/filestream.h"
#include "util/math.h"
#include "util/string_utils.h"

using AGS::Common::AlignedStream;
using AGS::Common::Array;
using AGS::Common::ObjectArray;
using AGS::Common::String;
using AGS::Common::Stream;
using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;
namespace Math = AGS::Common::Math;

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

volatile int switching_away_from_game = 0;
volatile char want_exit = 0, abort_engine = 0;
GameDataVersion loaded_game_file_version = kGameVersion_Undefined;
int frames_per_second=40;
int displayed_room=-10,starting_room = -1;
int in_new_room=0, new_room_was = 0;  // 1 in new room, 2 first time in new room, 3 loading saved game
int new_room_pos=0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;

unsigned int loopcounter=0,lastcounter=0;

//Bitmap *spriteset[MAX_SPRITES+1];
//SpriteCache spriteset (MAX_SPRITES+1);
// initially size 1, this will be increased by the initFile function
int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
SpriteCache spriteset(1);
int proper_exit=0,our_eip=0;

GUIMain*guis=NULL;
ViewStruct*views=NULL;

//=============================================================================

char saveGameDirectory[260] = "./";
int want_quit = 0; // [IKM] This is never used

const char* sgnametemplate = "agssave.%03d";
char saveGameSuffix[MAX_SG_EXT_LENGTH + 1];

int game_paused=0;
char pexbuf[STD_BUFFER_SIZE];

unsigned int load_new_game = 0;
int load_new_game_restore = -1;

int getloctype_index = 0, getloctype_throughgui = 0;

//=============================================================================
// Audio
//=============================================================================

void Game_StopAudio(int audioType)
{
    if (((audioType < 0) || (audioType >= game.AudioClipTypeCount)) && (audioType != SCR_NO_VALUE))
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
    if (((audioType < 0) || (audioType >= game.AudioClipTypeCount)) && (audioType != SCR_NO_VALUE))
        quitprintf("!Game.IsAudioPlaying: invalid audio type %d", audioType);

    if (play.FastForwardCutscene)
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
    if ((audioType < 0) || (audioType >= game.AudioClipTypeCount))
        quit("!Game.SetAudioTypeVolume: invalid audio type");

    game.AudioClipTypes[audioType].volume_reduction_while_speech_playing = volumeDrop;
}

void Game_SetAudioTypeVolume(int audioType, int volume, int changeType)
{
    if ((volume < 0) || (volume > 100))
        quitprintf("!Game.SetAudioTypeVolume: volume %d is not between 0..100", volume);
    if ((audioType < 0) || (audioType >= game.AudioClipTypeCount))
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
        play.DefaultAudioTypeVolumes[audioType] = volume;
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
  return game.DialogCount;
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
    cbuttfont = play.NormalFont;
    acdialog_font = play.NormalFont;
    SetVirtualScreen(virtual_screen);
    if (!play.MouseCursorHidden)
        domouse(1);
    oldmouse=cur_cursor; set_mouse_cursor(CURS_ARROW);
}
void restore_after_dialog() {
    set_mouse_cursor(oldmouse);
    if (!play.MouseCursorHidden)
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
    mkdir(newSaveGameDir, 0755);
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
    Stream *restartGameFile = Common::File::OpenFileRead(restartGamePath);
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
    if (AGS::Engine::ReadSavedGameDescription(get_save_game_path(slnum), description) == 0)
    {
        return CreateNewScriptString(description);
    }
    return NULL;
}

void restore_game_dialog() {
    can_run_delayed_command();
    if (thisroom.Options[kRoomBaseOpt_SaveLoadDisabled] == 1) {
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
        load_game_or_quit(toload);
    }
}

void save_game_dialog() {
    if (thisroom.Options[kRoomBaseOpt_SaveLoadDisabled] == 1) {
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
    game.GuiCount =0;
    for (rr=0;rr<42;rr++) game.PaletteUses[rr]=PAL_GAMEWIDE;
    for (rr=42;rr<256;rr++) game.PaletteUses[rr]=PAL_BACKGROUND;
}


void free_do_once_tokens() 
{
    play.DoOnceTokens.Free();
    play.DoOnceTokenCount = 0;
}


// Free all the memory associated with the game
void unload_game_file() {
    int bb, ee;

    for (bb = 0; bb < game.CharacterCount; bb++) {
        if (!game.CharacterInteractionScripts.IsEmpty())
            delete game.CharacterInteractionScripts[bb];

        if (!game.CharacterInteractions.IsEmpty())
        {
            delete game.CharacterInteractions[bb];
            game.CharacterInteractions[bb] = NULL;
        }
        game.CharacterProperties[bb].reset();
    }
    game.CharacterInteractions.Free();
    characterScriptObjNames.Free();
    free(charextra);
    CharMoveLists.Free();
    ObjMoveLists.Free();
    CharActiveSprites.Free();
    ObjActiveSprites.Free();
    game.CharacterProperties.Free();

    for (bb = 1; bb < game.InvItemCount; bb++) {
        if (!game.InvItemInteractionScripts.IsEmpty())
            delete game.InvItemInteractionScripts[bb];

        if (!game.InvItemInteractions.IsEmpty())
        {
            delete game.InvItemInteractions[bb];
            game.InvItemInteractions[bb] = NULL;
        }
    }

    game.CharacterInteractionScripts.Free();
    game.InvItemInteractionScripts.Free();

    if (game.Dictionary != NULL) {
        game.Dictionary->free_memory();
        free (game.Dictionary);
        game.Dictionary = NULL;
    }

    if ((gameinst != NULL) && (gameinst->pc != 0))
        quit("Error: unload_game called while script still running");
    //->AbortAndDestroy (gameinst);
    else {
        delete gameinstFork;
        delete gameinst;
        gameinstFork = NULL;
        gameinst = NULL;
    }

    delete gamescript;
    gamescript = NULL;

    if ((dialogScriptsInst != NULL) && (dialogScriptsInst->pc != 0))
        quit("Error: unload_game called while dialog script still running");
    else if (dialogScriptsInst != NULL)
    {
        delete dialogScriptsInst;
        dialogScriptsInst = NULL;
    }

    if (dialogScriptsScript != NULL)
    {
        delete dialogScriptsScript;
        dialogScriptsScript = NULL;
    }

    for (ee = 0; ee < numScriptModules; ee++) {
        delete moduleInstFork[ee];
        delete moduleInst[ee];
        delete scriptModules[ee];
    }
    numScriptModules = 0;

    game.AudioClips.Free();
    game.AudioClipCount = 0;
    game.AudioClipTypes.Free();
    game.AudioClipTypeCount = 0;

    game.ViewNames.Free();
    free (views);
    views = NULL;

    game.Characters.Free();
    charcache.Free();

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

    game.GlobalMessages.Free();

    game.RoomNames.Free();
    game.RoomNumbers.Free();
    game.RoomCount = 0;

    for (ee=0;ee<game.DialogCount;ee++) {
        if (dialog[ee].optionscripts!=NULL)
            free (dialog[ee].optionscripts);
        dialog[ee].optionscripts = NULL;
    }
    free (dialog);
    dialog = NULL;

    for (ee = 0; ee < game.GuiCount; ee++) {
        free (guibg[ee]);
        guibg[ee] = NULL;
    }

    guiScriptObjNames.Free();
    free(guibg);
    free (guis);
    guis = NULL;
    scrGui.Free();

    platform->ShutdownPlugins();
    ccRemoveAllSymbols();
    ccUnregisterAllObjects();

    for (ee=0;ee<game.FontCount;ee++)
        wfreefont(ee);

    free_do_once_tokens();
    play.GuiDrawOrder.Free();

    ResetRoomStates();
}






const char* Game_GetGlobalStrings(int index) {
    if ((index < 0) || (index >= MAXGLOBALSTRINGS))
        quit("!Game.GlobalStrings: invalid index");

    return CreateNewScriptString(play.GlobalStrings[index]);
}



char gamefilenamebuf[200];


// ** GetGameParameter replacement functions

int Game_GetInventoryItemCount() {
    // because of the dummy item 0, this is always one higher than it should be
    return game.InvItemCount - 1;
}

int Game_GetFontCount() {
    return game.FontCount;
}

int Game_GetMouseCursorCount() {
    return game.MouseCursorCount;
}

int Game_GetCharacterCount() {
    return game.CharacterCount;
}

int Game_GetGUICount() {
    return game.GuiCount;
}

int Game_GetViewCount() {
    return game.ViewCount;
}

int Game_GetUseNativeCoordinates()
{
    if (game.Options[OPT_NATIVECOORDINATES] != 0)
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
    if ((viewNumber < 1) || (viewNumber > game.ViewCount))
        quit("!GetGameParameter: invalid view specified");

    return views[viewNumber - 1].numLoops;
}

int Game_GetRunNextSettingForLoop(int viewNumber, int loopNumber) {
    if ((viewNumber < 1) || (viewNumber > game.ViewCount))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");

    return (views[viewNumber - 1].loops[loopNumber].RunNextLoop()) ? 1 : 0;
}

int Game_GetFrameCountForLoop(int viewNumber, int loopNumber) {
    if ((viewNumber < 1) || (viewNumber > game.ViewCount))
        quit("!GetGameParameter: invalid view specified");
    if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
        quit("!GetGameParameter: invalid loop specified");

    return views[viewNumber - 1].loops[loopNumber].numFrames;
}

ScriptViewFrame* Game_GetViewFrame(int viewNumber, int loopNumber, int frame) {
    if ((viewNumber < 1) || (viewNumber > game.ViewCount))
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

    for (int i = 0; i < play.DoOnceTokenCount; i++)
    {
        if (strcmp(play.DoOnceTokens[i], token) == 0)
        {
            return 0;
        }
    }
    play.DoOnceTokens.Append(token);
    play.DoOnceTokenCount++;
    return 1;
}

int Game_GetTextReadingSpeed()
{
    return play.TextDisplaySpeed;
}

void Game_SetTextReadingSpeed(int newTextSpeed)
{
    if (newTextSpeed < 1)
        quitprintf("!Game.TextReadingSpeed: %d is an invalid speed", newTextSpeed);

    play.TextDisplaySpeed = newTextSpeed;
}

int Game_GetMinimumTextDisplayTimeMs()
{
    return play.DisplayTextMinTimeMs;
}

void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime)
{
    play.DisplayTextMinTimeMs = newTextMinTime;
}

int Game_GetIgnoreUserInputAfterTextTimeoutMs()
{
    return play.DisplayTextIgnoreUserInputDelayMs;
}

void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs)
{
    play.DisplayTextIgnoreUserInputDelayMs = newValueMs;
}

const char *Game_GetFileName() {
    return CreateNewScriptString(usetup.MainDataFilename);
}

const char *Game_GetName() {
    return CreateNewScriptString(play.GameName);
}

void Game_SetName(const char *newName) {
    play.GameName = newName;

#if (ALLEGRO_DATE > 19990103)
    set_window_title(play.GameName);
#endif
}

int Game_GetSkippingCutscene()
{
    if (play.FastForwardCutscene)
    {
        return 1;
    }
    return 0;
}

int Game_GetInSkippableCutscene()
{
    if (play.IsInCutscene)
    {
        return 1;
    }
    return 0;
}

int Game_GetColorFromRGB(int red, int grn, int blu) {
    if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
        (blu < 0) || (blu > 255))
        quit("!GetColorFromRGB: colour values must be 0-255");

    if (game.ColorDepth == 1)
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
    return play.SpeechFont;
}
int Game_GetNormalFont() {
    return play.NormalFont;
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

void serialize_bitmap(Common::Bitmap *thispic, Stream *out) {
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
void safeguard_string (String &descript) {
    int it;
    for (it = 0; it < 50; it++) {
        if ((descript[it] < 1) || (descript[it] > 127))
            break;
    }
    descript.TruncateToLeft(it);
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

Bitmap *read_serialized_bitmap(Stream *in) {
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

long write_screen_shot_for_vista(Stream *out, Bitmap *screenshot) 
{
    long fileSize = 0;
    char tempFileName[MAX_PATH];
    sprintf(tempFileName, "%s""_tmpscht.bmp", saveGameDirectory);

	screenshot->SaveToFile(tempFileName, palette);

    update_polled_stuff_if_runtime();

    if (exists(tempFileName))
    {
        fileSize = file_size(tempFileName);
        char *buffer = (char*)malloc(fileSize);

        Stream *temp_in = Common::File::OpenFileRead(tempFileName);
        temp_in->Read(buffer, fileSize);
        delete temp_in;
        unlink(tempFileName);

        out->Write(buffer, fileSize);
        free(buffer);
    }
    return fileSize;
}

void save_game_screenshot(Stream *out, Bitmap *screenshot)
{
    // store the screenshot at the start to make it easily accesible
    out->WriteInt32((screenshot == NULL) ? 0 : 1);

    if (screenshot)
        serialize_bitmap(screenshot, out);
}

#define MAGICNUMBER 0xbeefcafe

void create_savegame_screenshot(Bitmap *&screenShot)
{
    if (game.Options[OPT_SAVESCREENSHOT]) {
        int usewid = multiply_up_coordinate(play.ScreenshotWidth);
        int usehit = multiply_up_coordinate(play.ScreenshotHeight);
        if (usewid > virtual_screen->GetWidth())
            usewid = virtual_screen->GetWidth();
        if (usehit > virtual_screen->GetHeight())
            usehit = virtual_screen->GetHeight();

        if ((play.ScreenshotWidth < 16) || (play.ScreenshotHeight < 16))
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

    Stream *out = Common::File::CreateFile(nametouse);
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
    convert_guid_from_text_to_binary(game.Guid, &vistaHeader.guidGameId[0]);
    uconvert(game.GameName, U_ASCII, (char*)&vistaHeader.szGameName[0], U_UNICODE, RM_MAXLENGTH);
    uconvert(descript, U_ASCII, (char*)&vistaHeader.szSaveName[0], U_UNICODE, RM_MAXLENGTH);
    vistaHeader.szLevelName[0] = 0;
    vistaHeader.szComments[0] = 0;

    //===================================================================
    // Started writing to the file here

    // Extended savegame info for Win Vista and higher
    vistaHeader.WriteToFile(out);

    // Savegame signature
    out->Write(SavedGameOpening::SavedGameSignature, SavedGameOpening::SavedGameSigLength);
    out->WriteInt32(kSvgVersion_Current);

    String fixed_descript = descript;
    safeguard_string(fixed_descript);
    fixed_descript.Write(out);

    Bitmap *screenShot = NULL;

    // Screenshot
    create_savegame_screenshot(screenShot);

    update_polled_stuff_if_runtime();

    platform->RunPluginHooks(AGSE_PRESAVEGAME, 0);
    
    save_game_screenshot(out, screenShot);
    SavedGameInfo svg_info;
    svg_info.EngineName = "Adventure Game Studio run-time engine";
    svg_info.EngineVersion = EngineVersion.LongString;
    svg_info.GameTitle = game.GameName;
    svg_info.Guid = game.Guid;
    svg_info.MainDataFilename = usetup.MainDataFilename;
    svg_info.WriteToFile(out);

    // Actual dynamic game data is saved here
    AGS::Engine::SaveGameData(out);

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

Bitmap *restore_game_screenshot(Stream *in)
{
    int isScreen = in->ReadInt32();
    if (isScreen) {
        return read_serialized_bitmap(in);
    }
    return NULL;
}

AGS::Engine::SavedGameError restore_game_header(Stream *in, SavedGameVersion svg_version)
{
    if (svg_version <= kSvgVersion_321)
    {
        String version_string = String::FromStream(in, 200);
        AGS::Engine::Version requested_engine_version(version_string);
        if (requested_engine_version > EngineVersion ||
            requested_engine_version < SavedGameLowestBackwardCompatVersion)
        {
            // Engine version is either non-forward or non-backward compatible
            return kSvgErr_IncompatibleEngine;
        }
        fgetstring_limit (rbuffer, in, 180);
        rbuffer[180] = 0;
        if (stricmp (rbuffer, usetup.MainDataFilename))
        {
            return kSvgErr_DifferentMainDataFile;
        }
    }
    else
    {
        SavedGameInfo svg_info;
        svg_info.ReadFromFile(in);
#if defined AGS_CASE_SENSITIVE_FILESYSTEM
        if (svg_info.MainDataFilename.CompareNoCase(usetup.MainDataFilename))
#else
        if (svg_info.MainDataFilename.Compare(usetup.MainDataFilename))
#endif
        {
            return kSvgErr_DifferentMainDataFile;
        }
        if (svg_info.Guid.Compare(game.Guid))
        {
            return kSvgErr_GameGuidFailed;
        }
    }
    return kSvgErr_NoError;
}

AGS::Engine::SavedGameError restore_game_head_dynamic_values(Stream *in, int &sg_cur_mode, int &sg_cur_cursor)
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

    // CHECKME: this error might be obsolete, resolution is changed using scaling filters now,
    // and the game resolution stays the same all the time.
    if (gamescrnhit != scrnhit) {
        Display("This game was saved with the interpreter running at a different "
            "resolution. It cannot be restored.");
        return kSvgErr_LegacyDifferentResolution;
    }

    // CHECKME: is this error check still needed? There is an option to degrade
    // 32-bit game to 16-bit, although of dubious value.
    if (in->ReadInt32() != final_col_dep) {
        Display("This game was saved with the engine running at a different colour depth. It cannot be restored.");
        return kSvgErr_LegacyDifferentColorDepth;
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

    return kSvgErr_NoError;
}

void restore_game_spriteset(Stream *in)
{
    for (int bb = 1; bb < spriteset.elements; bb++) {
        if (game.SpriteFlags[bb] & SPF_DYNAMICALLOC) {
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
        game.SpriteFlags[sprnum] = spriteflag;
        sprnum = in->ReadInt32();
    }
}

void restore_game_clean_gfx()
{
    for (int vv = 0; vv < game.GuiCount; vv++) {
        delete guibg[vv];
        guibg[vv] = NULL;

        if (guibgbmp[vv])
            gfxDriver->DestroyDDB(guibgbmp[vv]);
        guibgbmp[vv] = NULL;
    }
}

void restore_game_clean_scripts()
{
    delete gameinstFork;
    delete gameinst;
    gameinstFork = NULL;
    gameinst = NULL;
    for (int vv = 0; vv < numScriptModules; vv++) {
        delete moduleInstFork[vv];
        delete moduleInst[vv];
        moduleInst[vv] = NULL;
    }

    if (dialogScriptsInst != NULL)
    {
        delete dialogScriptsInst;
        dialogScriptsInst = NULL;
    }
}

void restore_game_scripts(Stream *in, int &gdatasize, char **newglobaldatabuffer, 
                          char **scriptModuleDataBuffers, int *scriptModuleDataSize)
{
    // read the global script data segment
    gdatasize = in->ReadInt32();
    *newglobaldatabuffer = (char*)malloc(gdatasize);
    in->Read(*newglobaldatabuffer, gdatasize);
    if (in->ReadInt32() != numScriptModules)
        quit("wrong script module count; cannot restore game");
    for (int vv = 0; vv < numScriptModules; vv++) {
        scriptModuleDataSize[vv] = in->ReadInt32();
        scriptModuleDataBuffers[vv] = (char*)malloc(scriptModuleDataSize[vv]);
        in->Read(&scriptModuleDataBuffers[vv][0], scriptModuleDataSize[vv]);
    }
}

void ReadRoomStatus_Aligned(RoomState *roomstat, Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    roomstat->ReadFromFile_v321(&align_s);
}

void restore_game_room_state(Stream *in)
{
    int vv;

    displayed_room = in->ReadInt32();

    // now the rooms
    ResetRoomStates();

    // read the room state for all the rooms the player has been in
    RoomState* roomstat;
    int BeenHere;
    for (vv=0;vv<MAX_ROOMS;vv++)
    {
        BeenHere = in->ReadByte();
        if (BeenHere)
        {
            roomstat = GetRoomState(vv);
            ReadRoomStatus_Aligned(roomstat, in);
            if (roomstat->ScriptDataSize > 0)
            {
                roomstat->ScriptData.ReadRaw(in, roomstat->ScriptDataSize);
            }
        }
    }
}

void ReadGameState_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    play.ReadFromFile_v321(&align_s);
}

void restore_game_play(Stream *in)
{
    int speech_was = play.SpeechVoiceMode, musicvox = play.UseSeparateMusicLib;
    // preserve the replay settings
    int playback_was = play.IsPlayback, recording_was = play.IsRecording;
    int gamestep_was = play.GameStep;
    int screenfadedout_was = play.ScreenIsFadedOut;
    int roomchanges_was = play.RoomChangeCount;

    free_do_once_tokens();

    ReadGameState_Aligned(in);

    // Preserve whether the music vox is available
    play.UseSeparateMusicLib = musicvox;
    // If they had the vox when they saved it, but they don't now
    if ((speech_was < 0) && (play.SpeechVoiceMode >= 0))
        play.SpeechVoiceMode = (-play.SpeechVoiceMode) - 1;
    // If they didn't have the vox before, but now they do
    else if ((speech_was >= 0) && (play.SpeechVoiceMode < 0))
        play.SpeechVoiceMode = (-play.SpeechVoiceMode) - 1;

    play.ScreenIsFadedOut = screenfadedout_was;
    play.IsPlayback = playback_was;
    play.IsRecording = recording_was;
    play.GameStep = gamestep_was;
    play.RoomChangeCount = roomchanges_was;

    if (play.DoOnceTokenCount > 0)
    {
        play.DoOnceTokens.New(play.DoOnceTokenCount);
        for (int bb = 0; bb < play.DoOnceTokenCount; bb++)
        {
            fgetstring_limit(rbuffer, in, 200);
            play.DoOnceTokens[bb] = rbuffer;
        }
    }

    play.GuiDrawOrder.ReadRawOver(in, game.GuiCount);
}

void ReadMoveList_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < ObjMoveLists.GetCount(); ++i)
    {
        ObjMoveLists[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
    MoveList dummy_movlist;
    memset(&dummy_movlist, 0, sizeof(dummy_movlist));
    for (int i = ObjMoveLists.GetCount(); i < LEGACY_MOVELIST_CHARACTER_OFFSET; ++i)
    {
        dummy_movlist.ReadFromFile(&align_s);
        align_s.Reset();
    }
    for (int i = 0; i < game.CharacterCount; ++i)
    {
        CharMoveLists[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void ReadCharacterExtras_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < game.CharacterCount; ++i)
    {
        charextra[i].ReadFromFile_v321(&align_s);
        align_s.Reset();
    }
}

void restore_game_palette(Stream *in)
{
    in->ReadArray(&palette[0],sizeof(color),256);
}

void restore_game_dialogs(Stream *in)
{
    for (int vv=0;vv<game.DialogCount;vv++)
        in->ReadArrayOfInt32(&dialog[vv].optionflags[0],MAXTOPICOPTIONS);
}

void restore_game_more_dynamic_values(Stream *in)
{
    mouse_on_iface=in->ReadInt32();
    mouse_on_iface_button=in->ReadInt32();
    mouse_pushed_iface=in->ReadInt32();
    ifacepopped = in->ReadInt32();
    game_paused=in->ReadInt32();
}

void ReadAnimatedButtons_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < numAnimButs; ++i)
    {
        animbuts[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void restore_game_gui(Stream *in, int numGuisWas)
{
    int vv;
    for (vv = 0; vv < game.GuiCount; vv++)
        unexport_gui_controls(vv);

    read_gui(in,guis,&game);

    if (numGuisWas != game.GuiCount)
        quit("!Restore_Game: Game has changed (GUIs), unable to restore position");

    for (vv = 0; vv < game.GuiCount; vv++)
        export_gui_controls(vv);

    numAnimButs = in->ReadInt32();
    ReadAnimatedButtons_Aligned(in);
}

void restore_game_audiocliptypes(Stream *in)
{
    if (in->ReadInt32() != game.AudioClipTypeCount)
        quit("!Restore_Game: game has changed (audio types), unable to restore");

    for (int i = 0; i < game.AudioClipTypeCount; ++i)
    {
        game.AudioClipTypes[i].ReadFromFile(in);
    }
}

void restore_game_thisroom(Stream *in, short *saved_light_levels, int *saved_tint_levels,
                           short *saved_zoom_levels1, short *saved_zoom_levels2)
{
    in->ReadArrayOfInt16(&saved_light_levels[0],LEGACY_MAX_ROOM_REGIONS);
    in->ReadArrayOfInt32(&saved_tint_levels[0], LEGACY_MAX_ROOM_REGIONS);
    in->ReadArrayOfInt16(&saved_zoom_levels1[0],LEGACY_MAX_ROOM_WALKAREAS + 1);
    in->ReadArrayOfInt16(&saved_zoom_levels2[0],LEGACY_MAX_ROOM_WALKAREAS + 1);
}

void restore_game_ambientsounds(Stream *in, int crossfadeInChannelWas, int crossfadeOutChannelWas,
                                int *doAmbient)
{
    int bb;
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        stop_and_destroy_channel_ex(bb, false);
    }

    play.CrossfadingInChannel = crossfadeInChannelWas;
    play.CrossfadingOutChannel = crossfadeOutChannelWas;

    for (int i = 0; i < MAX_SOUND_CHANNELS; ++i)
    {
        ambient[i].ReadFromFile(in);
    }

    for (bb = 1; bb < MAX_SOUND_CHANNELS; bb++) {
        if (ambient[bb].channel == 0)
            doAmbient[bb] = 0;
        else {
            doAmbient[bb] = ambient[bb].num;
            ambient[bb].channel = 0;
        }
    }
}

void ReadOverlays_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < numscreenover; ++i)
    {
        screenover[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void restore_game_overlays(Stream *in)
{
    numscreenover = in->ReadInt32();
    ReadOverlays_Aligned(in);
    for (int bb=0;bb<numscreenover;bb++) {
        if (screenover[bb].pic != NULL)
        {
            screenover[bb].pic = read_serialized_bitmap(in);
            screenover[bb].bmp = gfxDriver->CreateDDBFromBitmap(screenover[bb].pic, false);
        }
    }
}

void restore_game_dynamic_surfaces(Stream *in, Bitmap **dynamicallyCreatedSurfacesFromSaveGame)
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

void restore_game_displayed_room_status(Stream *in, Bitmap **newbscene)
{
    int bb;
    for (bb = 0; bb < MAX_BSCENE; bb++)
        newbscene[bb] = NULL;

    if (displayed_room >= 0) {

        for (bb = 0; bb < MAX_BSCENE; bb++) {
            newbscene[bb] = NULL;
            if (play.RoomBkgWasModified[bb]) {
                newbscene[bb] = read_serialized_bitmap (in);
            }
        }
        bb = in->ReadInt32();

        delete raw_saved_screen;
        raw_saved_screen = NULL;

        if (bb)
            raw_saved_screen = read_serialized_bitmap(in);

        troom.ScriptData.Free();
        
        // get the current troom, in case they save in room 600 or whatever
        ReadRoomStatus_Aligned(&troom, in);
        
        if (troom.ScriptDataSize > 0) {
            troom.ScriptData.ReadRaw(in, troom.ScriptDataSize);
        }
    }
}

void restore_game_globalvars(Stream *in)
{
    if (in->ReadInt32() != numGlobalVars) 
        quit("!Game has been modified since save; unable to restore game (GM01)");

    for (int i = 0; i < numGlobalVars; ++i)
    {
        globalvars[i].ReadFromFile(in);
    }
}

void restore_game_views(Stream *in)
{
    if (in->ReadInt32() != game.ViewCount)
        quit("!Game has been modified since save; unable to restore (GV02)");

    for (int bb = 0; bb < game.ViewCount; bb++) {
        for (int cc = 0; cc < views[bb].numLoops; cc++) {
            for (int dd = 0; dd < views[bb].loops[cc].numFrames; dd++)
            {
                views[bb].loops[cc].frames[dd].sound = in->ReadInt32();
                views[bb].loops[cc].frames[dd].pic = in->ReadInt32();
            }
        }
    }
}

void restore_game_audioclips_and_crossfade(Stream *in, int crossfadeInChannelWas, int crossfadeOutChannelWas)
{
    int bb;

    if (in->ReadInt32() != game.AudioClipCount)
        quit("Game has changed: different audio clip count");

    play.CrossfadingInChannel = 0;
    play.CrossfadingOutChannel = 0;
    int channelPositions[MAX_SOUND_CHANNELS + 1];
    for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
    {
        channelPositions[bb] = 0;
        int audioClipIndex = in->ReadInt32();
        if (audioClipIndex >= 0)
        {
            if (audioClipIndex >= game.AudioClipCount)
                quit("save game error: invalid audio clip index");

            channelPositions[bb] = in->ReadInt32();
            if (channelPositions[bb] < 0) channelPositions[bb] = 0;
            int priority = in->ReadInt32();
            int repeat = in->ReadInt32();
            int vol = in->ReadInt32();
            int pan = in->ReadInt32();
            int volAsPercent = in->ReadInt32();
            int panAsPercent = in->ReadInt32();
            play_audio_clip_on_channel(bb, &game.AudioClips[audioClipIndex], priority, repeat, channelPositions[bb]);
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
        play.CrossfadingInChannel = crossfadeInChannelWas;
    if ((crossfadeOutChannelWas > 0) && (channels[crossfadeOutChannelWas] != NULL))
        play.CrossfadingOutChannel = crossfadeOutChannelWas;

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

void restore_game_before_managed_pool()
{
    // Allocate maximal allowed size in case we are loading the game saved by
    // the older engine version.
    // The script object arrays cannot be _enlarged_ while objects are
    // registered at this point, because that may cause reallocation and change
    // of their position in memory; this will require different way to store them
    // and/or managed pool redesign.
    // Arrays might be truncated when room is loaded though (which won't
    // reallocate objects in memory).
    ccUnregisterAllObjects();

    scrDialog.SetLength(MAX_DIALOG);
    scrHotspot.SetLength(LEGACY_MAX_ROOM_HOTSPOTS);
    scrInv.SetLength(MAX_INV);
    scrObj.SetLength(LEGACY_MAX_ROOM_OBJECTS);
    scrRegion.SetLength(LEGACY_MAX_ROOM_REGIONS);
}

void restore_game_after_managed_pool()
{
    // Now, unregister all extra game objects to clean the pool from unnecessary entries
    // in case the game was saved by an older engine version.
    for (int i = game.DialogCount; i < scrDialog.GetCount(); ++i)
    {
        ccUnRegisterManagedObject(&scrDialog[i]);
    }
    for (int i = game.InvItemCount; i < scrInv.GetCount(); ++i)
    {
        ccUnRegisterManagedObject(&scrInv[i]);
    }
    scrDialog.SetLength(game.DialogCount);
    scrInv.SetLength(game.InvItemCount);
    // NOTE: the room objects and regions will be cleaned at the current room load
}

AGS::Engine::SavedGameError restore_game_data (Stream *in) {

    int bb, vv;

    int sg_cur_mode = 0, sg_cur_cursor = 0;
    AGS::Engine::SavedGameError res = restore_game_head_dynamic_values(in, /*out*/ sg_cur_mode, sg_cur_cursor);
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
    restore_game_room_state(in);

    restore_game_play(in);

    // Unfortunately we do not know the exact number of objects in the current
    // room at this point, so we will read maximal movelist number allowed
    // by vanilla AGS. The array size will be corrected after room is loaded.
    ObjMoveLists.SetLength(LEGACY_MOVELIST_CHARACTER_OFFSET);
    ReadMoveList_Aligned(in);

    int numchwas = game.CharacterCount;
    int numdiwas = game.DialogCount;
    int numinvwas = game.InvItemCount;
    int numviewswas = game.ViewCount;
    int numGuisWas = game.GuiCount;

    ReadGameSetupStructBase_Aligned(in);

    if (game.DialogCount!=numdiwas)
        quit("!Restore_Game: Game has changed (dlg), unable to restore");
    if ((numchwas != game.CharacterCount) || (numinvwas != game.InvItemCount))
        quit("!Restore_Game: Game has changed (inv), unable to restore position");
    if (game.ViewCount != numviewswas)
        quit("!Restore_Game: Game has changed (views), unable to restore position");

    game.ReadFromSavedGame_v321(in);

    ReadCharacterExtras_Aligned(in);
    if (roominst!=NULL) {  // so it doesn't overwrite the ScriptData
        delete roominstFork;
        delete roominst; 
        roominstFork = NULL;
        roominst=NULL;
    }
    restore_game_palette(in);
    restore_game_dialogs(in);
    restore_game_more_dynamic_values(in);

    restore_game_gui(in, numGuisWas);
    restore_game_audiocliptypes(in);

    short saved_light_levels[LEGACY_MAX_ROOM_REGIONS];
    int   saved_tint_levels[LEGACY_MAX_ROOM_REGIONS];
    short saved_zoom_levels1[LEGACY_MAX_ROOM_WALKAREAS + 1];
    short saved_zoom_levels2[LEGACY_MAX_ROOM_WALKAREAS + 1];
    restore_game_thisroom(in, saved_light_levels, saved_tint_levels, saved_zoom_levels1, saved_zoom_levels2);

    int crossfadeInChannelWas = play.CrossfadingInChannel;
    int crossfadeOutChannelWas = play.CrossfadingOutChannel;
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

    restore_game_before_managed_pool();

    if (ccUnserializeAllObjects(in, &ccUnserializer))
        quitprintf("LoadGame: Error during deserialization: %s", ccErrorString);

    restore_game_after_managed_pool();

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

    // restore the script module data
    for (bb = 0; bb < numScriptModules; bb++) {
        if (scriptModuleDataSize[bb] != moduleInst[bb]->globaldatasize)
            quit("!Restore Game: script module global data changed, unable to restore");
        memcpy(&moduleInst[bb]->globaldata[0], scriptModuleDataBuffers[bb], scriptModuleDataSize[bb]);
        free(scriptModuleDataBuffers[bb]);
    }


    setup_player_character(game.PlayerCharacterIndex);

    int gstimer=play.GlobalScriptTimer;
    int oldx1 = play.MouseBoundLeft, oldx2 = play.MouseBoundRight;
    int oldy1 = play.MouseBoundTop, oldy2 = play.MouseBoundBottom;
    int musicWasRepeating = play.CurrentMusicLoopMode;
    int newms = play.CurrentMusicIndex;

    // disable the queue momentarily
    int queuedMusicSize = play.MusicQueueLength;
    play.MusicQueueLength = 0;

    update_polled_stuff_if_runtime();

    if (displayed_room >= 0)
        load_new_room(displayed_room,NULL);//&game.Characters[game.PlayerCharacterIndex]);

    update_polled_stuff_if_runtime();

    play.GlobalScriptTimer=gstimer;

    // restore the correct room volume (they might have modified
    // it with SetMusicVolume)
    thisroom.Options[kRoomBaseOpt_MusicVolume] = newRoomVol;

    filter->SetMouseLimit(oldx1,oldy1,oldx2,oldy2);

    set_cursor_mode(sg_cur_mode);
    set_mouse_cursor(sg_cur_cursor);
    if (sg_cur_mode == MODE_USE)
        SetActiveInventory (playerchar->activeinv);
    // ensure that the current cursor is locked
    spriteset.precache(game.MouseCursors[sg_cur_cursor].pic);

#if (ALLEGRO_DATE > 19990103)
    set_window_title(play.GameName);
#endif

    update_polled_stuff_if_runtime();

    if (displayed_room >= 0) {

        for (bb = 0; bb < MAX_BSCENE; bb++) {
            if (newbscene[bb]) {
                delete thisroom.Backgrounds[bb].Graphic;
                thisroom.Backgrounds[bb].Graphic = newbscene[bb];
            }
        }

        in_new_room=3;  // don't run "enters screen" events
        // now that room has loaded, copy saved light levels in
        for (int i = 0; i < thisroom.RegionCount; ++i)
        {
            thisroom.Regions[i].Light = saved_light_levels[i];
            thisroom.Regions[i].Tint = saved_tint_levels[i];
        }
        generate_light_table();

        for (int i = 0; i < thisroom.WalkAreaCount; ++i)
        {
            thisroom.WalkAreas[i].Zoom = saved_zoom_levels1[i];
            thisroom.WalkAreas[i].Zoom2 = saved_zoom_levels2[i];
        }

        on_background_frame_change();

    }

    gui_disabled_style = convert_gui_disabled_style(game.Options[OPT_DISABLEOFF]);
    /*
    play_sound(-1);

    stopmusic();
    // use the repeat setting when the current track was started
    int musicRepeatSetting = play.MusicLoopMode;
    SetMusicRepeat(musicWasRepeating);
    if (newms>=0) {
    // restart the background music
    if (newms == 1000)
    PlayMP3File (play.PlayMp3FileName);
    else {
    play.CurrentMusicIndex=2000;  // make sure it gets played
    newmusic(newms);
    }
    }
    SetMusicRepeat(musicRepeatSetting);
    if (play.SilentMidiIndex)
    PlaySilentMIDI (play.SilentMidiIndex);
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
    play.MusicQueueLength = queuedMusicSize;

    if (play.DigitalMasterVolume >= 0)
        System_SetVolume(play.DigitalMasterVolume);

    for (vv = 1; vv < MAX_SOUND_CHANNELS; vv++) {
        if (doAmbient[vv])
            PlayAmbientSound(vv, doAmbient[vv], ambient[vv].vol, ambient[vv].x, ambient[vv].y);
    }

    for (vv = 0; vv < game.GuiCount; vv++) {
        guibg[vv] = BitmapHelper::CreateBitmap (guis[vv].wid, guis[vv].hit, final_col_dep);
        guibg[vv] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[vv]);
    }

    if (gfxDriver->SupportsGammaControl())
        gfxDriver->SetGamma(play.GammaAdjustment);

    guis_need_update = 1;

    play.IgnoreUserInputUntilTime = 0;
    update_polled_stuff_if_runtime();

    platform->RunPluginHooks(AGSE_POSTRESTOREGAME, 0);

    if (displayed_room < 0) {
        // the restart point, no room was loaded
        load_new_room(playerchar->room, playerchar);
        playerchar->prevroom = -1;

        first_room_initialization();
    }

    if ((play.MusicQueueLength > 0) && (cachedQueuedMusic == NULL)) {
        cachedQueuedMusic = load_music_from_disk(play.MusicQueue[0], 0);
    }

    return kSvgErr_NoError;
}

int gameHasBeenRestored = 0;
int oldeip;

AGS::Engine::SavedGameError load_game(const Common::String &path, int slotNumber)
{
    gameHasBeenRestored++;

    AGS::Engine::SavedGameOpening opening;
    AGS::Engine::SavedGameError error_code = AGS::Engine::OpenSavedGame(path, opening);
    if (error_code != kSvgErr_NoError)
    {
        return error_code;
    }

    Stream *in = opening.InputStream;
    if (opening.Version <= kSvgVersion_321)
    {
        // skip description and format version
        for (size_t desc_idx = 0; (in->ReadByte() > 0) && (desc_idx <= LegacyDescriptionLength); ++desc_idx);
        in->ReadInt32();
    }
    else
    {
        // skip description
        while(in->ReadByte() > 0);
    }

    our_eip = 2051;

    // skip screenshot
    delete restore_game_screenshot(in);  // [IKM] how very appropriate
    error_code = restore_game_header(in, opening.Version);

    // saved in different game
    if (error_code == kSvgErr_DifferentMainDataFile) {
        // [IKM] 2012-11-26: this is a workaround, indeed.
        // Try to find wanted game's executable; if it does not exist,
        // continue loading savedgame in current game, and pray for the best
        get_current_dir_path(gamefilenamebuf, rbuffer);
        if (Common::File::TestReadFile(gamefilenamebuf))
        {
            RunAGSGame (rbuffer, 0, 0);
            load_new_game_restore = slotNumber;
            return kSvgErr_NoError;
        }
        Common::Out::FPrint("WARNING: the saved game '%s' references game file '%s', but it cannot be found in the current directory.", path.GetCStr(), gamefilenamebuf);
        Common::Out::FPrint("Trying to restore in the running game instead.");
    }
    else if (error_code != 0) {
        return error_code;
    }

    // do the actual restore
    if (opening.Version <= kSvgVersion_321)
    {
        error_code = restore_game_data(in);
    }
    else
    {
        error_code = AGS::Engine::RestoreGameData(in);
    }

    our_eip = oldeip;
    if (error_code)
    {
        return error_code;
    }

    run_on_event (GE_RESTORE_GAME, RuntimeScriptValue().SetInt32(slotNumber));

    // ensure keyboard buffer is clean
    // use the raw versions rather than the rec_ versions so we don't
    // interfere with the replay sync
    while (keypressed()) readkey();
    return kSvgErr_NoError;
}

void load_game_or_quit(int slotNumber)
{
    load_game_or_quit(get_save_game_path(slotNumber), slotNumber);
}

void load_game_or_quit(const Common::String &path, int slotNumber)
{
    AGS::Engine::SavedGameError error = load_game(path, slotNumber);
    if (error != AGS::Engine::kSvgErr_NoError)
    {
        quitprintf("Unable to restore game:\n%s", AGS::Engine::SavedGameErrorText[error]);
    }
}

void start_skipping_cutscene () {
    play.FastForwardCutscene = 1;
    // if a drop-down icon bar is up, remove it as it will pause the game
    if (ifacepopped>=0)
        remove_popup_interface(ifacepopped);

    // if a text message is currently displayed, remove it
    if (is_text_overlay > 0)
        remove_screen_overlay(OVER_TEXTMSG);

}

void check_skip_cutscene_keypress (int kgn) {

    if ((play.IsInCutscene > 0) && (play.IsInCutscene != 3)) {
        if ((kgn != 27) && ((play.IsInCutscene == 1) || (play.IsInCutscene == 5)))
            ;
        else
            start_skipping_cutscene();
    }

}

// Helper functions used by StartCutscene/EndCutscene, but also
// by SkipUntilCharacterStops
void initialize_skippable_cutscene() {
    play.PlayMusicAfterCustsceneSkip = -1;
}

void stop_fast_forwarding() {
    // when the skipping of a cutscene comes to an end, update things
    play.FastForwardCutscene = 0;
    setpal();
    if (play.PlayMusicAfterCustsceneSkip >= 0)
        newmusic(play.PlayMusicAfterCustsceneSkip);

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
    if ((xxx>=thisroom.Width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.Height))
        return 0;

    // check characters, objects and walkbehinds, work out which is
    // foremost visible to the player
    int charat = is_pos_on_character(xxx,yyy);
    int hsat = get_hotspot_at(xxx,yyy);
    int objat = GetObjectAt(xxx - divide_down_coordinate(offsetx), yyy - divide_down_coordinate(offsety));

    multiply_up_coordinates(&xxx, &yyy);

    int wbat = thisroom.WalkBehindMask->GetPixel(xxx, yyy);

    if (wbat <= 0) wbat = 0;
    else wbat = croom->WalkBehinds[wbat].Baseline;

    int winner = 0;
    // if it's an Ignore Walkbehinds object, then ignore the walkbehind
    if ((objat >= 0) && ((objs[objat].Flags & OBJF_NOWALKBEHINDS) != 0))
        wbat = 0;
    if ((charat >= 0) && ((game.Characters[charat].flags & CHF_NOWALKBEHINDS) != 0))
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
                if ((inx<1) | (inx>=game.InvItemCount))
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
    if (game.GlobalMessages[msnum-500] == NULL)
        return "";
    return get_translation(game.GlobalMessages[msnum-500]);
}

void get_message_text (int msnum, char *buffer, char giveErr) {
    int maxlen = 9999;
    if (!giveErr)
        maxlen = MAX_MAXSTRLEN;

    if (msnum>=500) { //quit("global message requseted, nto yet supported");

        if ((msnum >= MAXGLOBALMES + 500) || (game.GlobalMessages[msnum-500]==NULL)) {
            if (giveErr)
                quit("!DisplayGlobalMessage: message does not exist");
            buffer[0] = 0;
            return;
        }
        buffer[0] = 0;
        replace_tokens(get_translation(game.GlobalMessages[msnum-500]), buffer, maxlen);
        return;
    }
    else if (msnum >= thisroom.MessageCount) {
        if (giveErr)
            quit("!DisplayMessage: Invalid message number to display");
        buffer[0] = 0;
        return;
    }

    buffer[0]=0;
    replace_tokens(get_translation(thisroom.Messages[msnum]), buffer, maxlen);
}

InteractionVariable *get_interaction_variable (int varindx) {

    if ((varindx >= LOCAL_VARIABLE_OFFSET) && (varindx < LOCAL_VARIABLE_OFFSET + thisroom.LocalVariableCount))
        return &thisroom.LocalVariables[varindx - LOCAL_VARIABLE_OFFSET];

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
    for (ii = 0; ii < thisroom.LocalVariableCount; ii++) {
        if (stricmp (thisroom.LocalVariables[ii].name, varName) == 0)
            return &thisroom.LocalVariables[ii];
    }
    return NULL;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int  (int audioType);
RuntimeScriptValue Sc_Game_IsAudioPlaying(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_IsAudioPlaying);
}

// void (int audioType, int volumeDrop)
RuntimeScriptValue Sc_Game_SetAudioTypeSpeechVolumeDrop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Game_SetAudioTypeSpeechVolumeDrop);
}

// void (int audioType, int volume, int changeType)
RuntimeScriptValue Sc_Game_SetAudioTypeVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Game_SetAudioTypeVolume);
}

// void (int audioType)
RuntimeScriptValue Sc_Game_StopAudio(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_StopAudio);
}

// int (const char *newFilename)
RuntimeScriptValue Sc_Game_ChangeTranslation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_ChangeTranslation, const char);
}

// int (const char *token)
RuntimeScriptValue Sc_Game_DoOnceOnly(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_DoOnceOnly, const char);
}

// int (int red, int grn, int blu)
RuntimeScriptValue Sc_Game_GetColorFromRGB(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(Game_GetColorFromRGB);
}

// int (int viewNumber, int loopNumber)
RuntimeScriptValue Sc_Game_GetFrameCountForLoop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Game_GetFrameCountForLoop);
}

// const char* (int x, int y)
RuntimeScriptValue Sc_Game_GetLocationName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(const char, myScriptStringImpl, Game_GetLocationName);
}

// int (int viewNumber)
RuntimeScriptValue Sc_Game_GetLoopCountForView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetLoopCountForView);
}

// int ()
RuntimeScriptValue Sc_Game_GetMODPattern(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMODPattern);
}

// int (int viewNumber, int loopNumber)
RuntimeScriptValue Sc_Game_GetRunNextSettingForLoop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Game_GetRunNextSettingForLoop);
}

// const char* (int slnum)
RuntimeScriptValue Sc_Game_GetSaveSlotDescription(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetSaveSlotDescription);
}

// ScriptViewFrame* (int viewNumber, int loopNumber, int frame)
RuntimeScriptValue Sc_Game_GetViewFrame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptViewFrame, Game_GetViewFrame);
}

// const char* (const char *msg)
RuntimeScriptValue Sc_Game_InputBox(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, Game_InputBox, const char);
}

// int (const char *newFolder)
RuntimeScriptValue Sc_Game_SetSaveGameDirectory(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_SetSaveGameDirectory, const char);
}

// void (int evenAmbient);
RuntimeScriptValue Sc_StopAllSounds(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StopAllSounds);
}

// int ()
RuntimeScriptValue Sc_Game_GetCharacterCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetCharacterCount);
}

// int ()
RuntimeScriptValue Sc_Game_GetDialogCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetDialogCount);
}

// const char *()
RuntimeScriptValue Sc_Game_GetFileName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetFileName);
}

// int ()
RuntimeScriptValue Sc_Game_GetFontCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetFontCount);
}

// const char* (int index)
RuntimeScriptValue Sc_Game_GetGlobalMessages(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetGlobalMessages);
}

// const char* (int index)
RuntimeScriptValue Sc_Game_GetGlobalStrings(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetGlobalStrings);
}

// void  (int index, char *newval);
RuntimeScriptValue Sc_SetGlobalString(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(SetGlobalString, const char);
}

// int ()
RuntimeScriptValue Sc_Game_GetGUICount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetGUICount);
}

// int ()
RuntimeScriptValue Sc_Game_GetIgnoreUserInputAfterTextTimeoutMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetIgnoreUserInputAfterTextTimeoutMs);
}

// void (int newValueMs)
RuntimeScriptValue Sc_Game_SetIgnoreUserInputAfterTextTimeoutMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetIgnoreUserInputAfterTextTimeoutMs);
}

// int ()
RuntimeScriptValue Sc_Game_GetInSkippableCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInSkippableCutscene);
}

// int ()
RuntimeScriptValue Sc_Game_GetInventoryItemCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInventoryItemCount);
}

// int ()
RuntimeScriptValue Sc_Game_GetMinimumTextDisplayTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMinimumTextDisplayTimeMs);
}

// void (int newTextMinTime)
RuntimeScriptValue Sc_Game_SetMinimumTextDisplayTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetMinimumTextDisplayTimeMs);
}

// int ()
RuntimeScriptValue Sc_Game_GetMouseCursorCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMouseCursorCount);
}

// const char *()
RuntimeScriptValue Sc_Game_GetName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetName);
}

// void (const char *newName)
RuntimeScriptValue Sc_Game_SetName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Game_SetName, const char);
}

// int ()
RuntimeScriptValue Sc_Game_GetNormalFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetNormalFont);
}

// void  (int fontnum);
RuntimeScriptValue Sc_SetNormalFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetNormalFont);
}

// int ()
RuntimeScriptValue Sc_Game_GetSkippingCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetSkippingCutscene);
}

// int ()
RuntimeScriptValue Sc_Game_GetSpeechFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetSpeechFont);
}

// void  (int fontnum);
RuntimeScriptValue Sc_SetSpeechFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechFont);
}

// int (int spriteNum)
RuntimeScriptValue Sc_Game_GetSpriteWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetSpriteWidth);
}

// int (int spriteNum)
RuntimeScriptValue Sc_Game_GetSpriteHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetSpriteHeight);
}

// int ()
RuntimeScriptValue Sc_Game_GetTextReadingSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetTextReadingSpeed);
}

// void (int newTextSpeed)
RuntimeScriptValue Sc_Game_SetTextReadingSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetTextReadingSpeed);
}

// const char* ()
RuntimeScriptValue Sc_Game_GetTranslationFilename(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetTranslationFilename);
}

// int ()
RuntimeScriptValue Sc_Game_GetUseNativeCoordinates(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetUseNativeCoordinates);
}

// int ()
RuntimeScriptValue Sc_Game_GetViewCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetViewCount);
}


void RegisterGameAPI()
{
    ccAddExternalStaticFunction("Game::IsAudioPlaying^1",                       Sc_Game_IsAudioPlaying);
    ccAddExternalStaticFunction("Game::SetAudioTypeSpeechVolumeDrop^2",         Sc_Game_SetAudioTypeSpeechVolumeDrop);
    ccAddExternalStaticFunction("Game::SetAudioTypeVolume^3",                   Sc_Game_SetAudioTypeVolume);
    ccAddExternalStaticFunction("Game::StopAudio^1",                            Sc_Game_StopAudio);
    ccAddExternalStaticFunction("Game::ChangeTranslation^1",                    Sc_Game_ChangeTranslation);
    ccAddExternalStaticFunction("Game::DoOnceOnly^1",                           Sc_Game_DoOnceOnly);
    ccAddExternalStaticFunction("Game::GetColorFromRGB^3",                      Sc_Game_GetColorFromRGB);
    ccAddExternalStaticFunction("Game::GetFrameCountForLoop^2",                 Sc_Game_GetFrameCountForLoop);
    ccAddExternalStaticFunction("Game::GetLocationName^2",                      Sc_Game_GetLocationName);
    ccAddExternalStaticFunction("Game::GetLoopCountForView^1",                  Sc_Game_GetLoopCountForView);
    ccAddExternalStaticFunction("Game::GetMODPattern^0",                        Sc_Game_GetMODPattern);
    ccAddExternalStaticFunction("Game::GetRunNextSettingForLoop^2",             Sc_Game_GetRunNextSettingForLoop);
    ccAddExternalStaticFunction("Game::GetSaveSlotDescription^1",               Sc_Game_GetSaveSlotDescription);
    ccAddExternalStaticFunction("Game::GetViewFrame^3",                         Sc_Game_GetViewFrame);
    ccAddExternalStaticFunction("Game::InputBox^1",                             Sc_Game_InputBox);
    ccAddExternalStaticFunction("Game::SetSaveGameDirectory^1",                 Sc_Game_SetSaveGameDirectory);
    ccAddExternalStaticFunction("Game::StopSound^1",                            Sc_StopAllSounds);
    ccAddExternalStaticFunction("Game::get_CharacterCount",                     Sc_Game_GetCharacterCount);
    ccAddExternalStaticFunction("Game::get_DialogCount",                        Sc_Game_GetDialogCount);
    ccAddExternalStaticFunction("Game::get_FileName",                           Sc_Game_GetFileName);
    ccAddExternalStaticFunction("Game::get_FontCount",                          Sc_Game_GetFontCount);
    ccAddExternalStaticFunction("Game::geti_GlobalMessages",                    Sc_Game_GetGlobalMessages);
    ccAddExternalStaticFunction("Game::geti_GlobalStrings",                     Sc_Game_GetGlobalStrings);
    ccAddExternalStaticFunction("Game::seti_GlobalStrings",                     Sc_SetGlobalString);
    ccAddExternalStaticFunction("Game::get_GUICount",                           Sc_Game_GetGUICount);
    ccAddExternalStaticFunction("Game::get_IgnoreUserInputAfterTextTimeoutMs",  Sc_Game_GetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalStaticFunction("Game::set_IgnoreUserInputAfterTextTimeoutMs",  Sc_Game_SetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalStaticFunction("Game::get_InSkippableCutscene",                Sc_Game_GetInSkippableCutscene);
    ccAddExternalStaticFunction("Game::get_InventoryItemCount",                 Sc_Game_GetInventoryItemCount);
    ccAddExternalStaticFunction("Game::get_MinimumTextDisplayTimeMs",           Sc_Game_GetMinimumTextDisplayTimeMs);
    ccAddExternalStaticFunction("Game::set_MinimumTextDisplayTimeMs",           Sc_Game_SetMinimumTextDisplayTimeMs);
    ccAddExternalStaticFunction("Game::get_MouseCursorCount",                   Sc_Game_GetMouseCursorCount);
    ccAddExternalStaticFunction("Game::get_Name",                               Sc_Game_GetName);
    ccAddExternalStaticFunction("Game::set_Name",                               Sc_Game_SetName);
    ccAddExternalStaticFunction("Game::get_NormalFont",                         Sc_Game_GetNormalFont);
    ccAddExternalStaticFunction("Game::set_NormalFont",                         Sc_SetNormalFont);
    ccAddExternalStaticFunction("Game::get_SkippingCutscene",                   Sc_Game_GetSkippingCutscene);
    ccAddExternalStaticFunction("Game::get_SpeechFont",                         Sc_Game_GetSpeechFont);
    ccAddExternalStaticFunction("Game::set_SpeechFont",                         Sc_SetSpeechFont);
    ccAddExternalStaticFunction("Game::geti_SpriteWidth",                       Sc_Game_GetSpriteWidth);
    ccAddExternalStaticFunction("Game::geti_SpriteHeight",                      Sc_Game_GetSpriteHeight);
    ccAddExternalStaticFunction("Game::get_TextReadingSpeed",                   Sc_Game_GetTextReadingSpeed);
    ccAddExternalStaticFunction("Game::set_TextReadingSpeed",                   Sc_Game_SetTextReadingSpeed);
    ccAddExternalStaticFunction("Game::get_TranslationFilename",                Sc_Game_GetTranslationFilename);
    ccAddExternalStaticFunction("Game::get_UseNativeCoordinates",               Sc_Game_GetUseNativeCoordinates);
    ccAddExternalStaticFunction("Game::get_ViewCount",                          Sc_Game_GetViewCount);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Game::IsAudioPlaying^1",                       (void*)Game_IsAudioPlaying);
    ccAddExternalFunctionForPlugin("Game::SetAudioTypeSpeechVolumeDrop^2",         (void*)Game_SetAudioTypeSpeechVolumeDrop);
    ccAddExternalFunctionForPlugin("Game::SetAudioTypeVolume^3",                   (void*)Game_SetAudioTypeVolume);
    ccAddExternalFunctionForPlugin("Game::StopAudio^1",                            (void*)Game_StopAudio);
    ccAddExternalFunctionForPlugin("Game::ChangeTranslation^1",                    (void*)Game_ChangeTranslation);
    ccAddExternalFunctionForPlugin("Game::DoOnceOnly^1",                           (void*)Game_DoOnceOnly);
    ccAddExternalFunctionForPlugin("Game::GetColorFromRGB^3",                      (void*)Game_GetColorFromRGB);
    ccAddExternalFunctionForPlugin("Game::GetFrameCountForLoop^2",                 (void*)Game_GetFrameCountForLoop);
    ccAddExternalFunctionForPlugin("Game::GetLocationName^2",                      (void*)Game_GetLocationName);
    ccAddExternalFunctionForPlugin("Game::GetLoopCountForView^1",                  (void*)Game_GetLoopCountForView);
    ccAddExternalFunctionForPlugin("Game::GetMODPattern^0",                        (void*)Game_GetMODPattern);
    ccAddExternalFunctionForPlugin("Game::GetRunNextSettingForLoop^2",             (void*)Game_GetRunNextSettingForLoop);
    ccAddExternalFunctionForPlugin("Game::GetSaveSlotDescription^1",               (void*)Game_GetSaveSlotDescription);
    ccAddExternalFunctionForPlugin("Game::GetViewFrame^3",                         (void*)Game_GetViewFrame);
    ccAddExternalFunctionForPlugin("Game::InputBox^1",                             (void*)Game_InputBox);
    ccAddExternalFunctionForPlugin("Game::SetSaveGameDirectory^1",                 (void*)Game_SetSaveGameDirectory);
    ccAddExternalFunctionForPlugin("Game::StopSound^1",                            (void*)StopAllSounds);
    ccAddExternalFunctionForPlugin("Game::get_CharacterCount",                     (void*)Game_GetCharacterCount);
    ccAddExternalFunctionForPlugin("Game::get_DialogCount",                        (void*)Game_GetDialogCount);
    ccAddExternalFunctionForPlugin("Game::get_FileName",                           (void*)Game_GetFileName);
    ccAddExternalFunctionForPlugin("Game::get_FontCount",                          (void*)Game_GetFontCount);
    ccAddExternalFunctionForPlugin("Game::geti_GlobalMessages",                    (void*)Game_GetGlobalMessages);
    ccAddExternalFunctionForPlugin("Game::geti_GlobalStrings",                     (void*)Game_GetGlobalStrings);
    ccAddExternalFunctionForPlugin("Game::seti_GlobalStrings",                     (void*)SetGlobalString);
    ccAddExternalFunctionForPlugin("Game::get_GUICount",                           (void*)Game_GetGUICount);
    ccAddExternalFunctionForPlugin("Game::get_IgnoreUserInputAfterTextTimeoutMs",  (void*)Game_GetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalFunctionForPlugin("Game::set_IgnoreUserInputAfterTextTimeoutMs",  (void*)Game_SetIgnoreUserInputAfterTextTimeoutMs);
    ccAddExternalFunctionForPlugin("Game::get_InSkippableCutscene",                (void*)Game_GetInSkippableCutscene);
    ccAddExternalFunctionForPlugin("Game::get_InventoryItemCount",                 (void*)Game_GetInventoryItemCount);
    ccAddExternalFunctionForPlugin("Game::get_MinimumTextDisplayTimeMs",           (void*)Game_GetMinimumTextDisplayTimeMs);
    ccAddExternalFunctionForPlugin("Game::set_MinimumTextDisplayTimeMs",           (void*)Game_SetMinimumTextDisplayTimeMs);
    ccAddExternalFunctionForPlugin("Game::get_MouseCursorCount",                   (void*)Game_GetMouseCursorCount);
    ccAddExternalFunctionForPlugin("Game::get_Name",                               (void*)Game_GetName);
    ccAddExternalFunctionForPlugin("Game::set_Name",                               (void*)Game_SetName);
    ccAddExternalFunctionForPlugin("Game::get_NormalFont",                         (void*)Game_GetNormalFont);
    ccAddExternalFunctionForPlugin("Game::set_NormalFont",                         (void*)SetNormalFont);
    ccAddExternalFunctionForPlugin("Game::get_SkippingCutscene",                   (void*)Game_GetSkippingCutscene);
    ccAddExternalFunctionForPlugin("Game::get_SpeechFont",                         (void*)Game_GetSpeechFont);
    ccAddExternalFunctionForPlugin("Game::set_SpeechFont",                         (void*)SetSpeechFont);
    ccAddExternalFunctionForPlugin("Game::geti_SpriteWidth",                       (void*)Game_GetSpriteWidth);
    ccAddExternalFunctionForPlugin("Game::geti_SpriteHeight",                      (void*)Game_GetSpriteHeight);
    ccAddExternalFunctionForPlugin("Game::get_TextReadingSpeed",                   (void*)Game_GetTextReadingSpeed);
    ccAddExternalFunctionForPlugin("Game::set_TextReadingSpeed",                   (void*)Game_SetTextReadingSpeed);
    ccAddExternalFunctionForPlugin("Game::get_TranslationFilename",                (void*)Game_GetTranslationFilename);
    ccAddExternalFunctionForPlugin("Game::get_UseNativeCoordinates",               (void*)Game_GetUseNativeCoordinates);
    ccAddExternalFunctionForPlugin("Game::get_ViewCount",                          (void*)Game_GetViewCount);
}

void RegisterStaticObjects()
{
    ccAddExternalStaticObject("game",&play, &GlobalStaticManager);
	ccAddExternalStaticObject("gs_globals",&play.GlobalVars[0], &GlobalStaticManager);
	ccAddExternalStaticObject("mouse",&scmouse, &GlobalStaticManager);
	ccAddExternalStaticObject("palette",&palette[0], &GlobalStaticManager);
	ccAddExternalStaticObject("system",&scsystem, &GlobalStaticManager);
	ccAddExternalStaticObject("savegameindex",&play.SavedGameFileNumbers[0], &GlobalStaticManager);
}

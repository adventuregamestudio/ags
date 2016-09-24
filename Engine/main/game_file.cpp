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

//
// Game data file management
//

#include "main/mainheader.h"
#include "main/game_file.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/charactercache.h"
#include "ac/dialogtopic.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gamestructdefines.h"
#include "ac/gui.h"
#include "ac/viewframe.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "gui/guilabel.h"
#include "main/main.h"
#include "platform/base/agsplatformdriver.h"
#include "script/exports.h"
#include "script/script.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "gfx/blender.h"
#include "core/assetmanager.h"
#include "ac/statobj/agsstaticobject.h"
#include "ac/statobj/staticarray.h"
#include "util/alignedstream.h"
#include "ac/gamesetup.h"
#include "game/main_game_file.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern char saveGameSuffix[MAX_SG_EXT_LENGTH + 1];

// Old dialog support
extern unsigned char** old_dialog_scripts; // defined in ac_conversation
extern char** old_speech_lines;

extern int ifacepopped;

extern GameSetupStruct game;
extern ViewStruct*views;
extern DialogTopic *dialog;
extern CharacterCache *charcache;
extern MoveList *mls;

extern CCGUIObject ccDynamicGUIObject;
extern CCCharacter ccDynamicCharacter;
extern CCHotspot   ccDynamicHotspot;
extern CCRegion    ccDynamicRegion;
extern CCInventory ccDynamicInv;
extern CCGUI       ccDynamicGUI;
extern CCObject    ccDynamicObject;
extern CCDialog    ccDynamicDialog;
extern ScriptString myScriptStringImpl;
extern ScriptObject scrObj[MAX_INIT_SPR];
extern ScriptGUI    *scrGui;
extern ScriptHotspot scrHotspot[MAX_HOTSPOTS];
extern ScriptRegion scrRegion[MAX_REGIONS];
extern ScriptInvItem scrInv[MAX_INV];
extern ScriptDialog scrDialog[MAX_DIALOG];

extern ScriptDialogOptionsRendering ccDialogOptionsRendering;
extern ScriptDrawingSurface* dialogOptionsRenderingSurface;

extern int our_eip;
extern int game_paused;

extern AGSPlatformDriver *platform;
extern ccScript* gamescript;
extern ccScript* dialogScriptsScript;
extern std::vector<ccScript *> scriptModules;
extern std::vector<ccInstance *> moduleInst;
extern std::vector<ccInstance *> moduleInstFork;
extern std::vector<RuntimeScriptValue> moduleRepExecAddr;
extern int numScriptModules;
extern GameState play;
extern char **characterScriptObjNames;
extern char objectScriptObjNames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN + 5];
extern char **guiScriptObjNames;
extern int actSpsCount;
extern Bitmap **actsps;
extern IDriverDependantBitmap* *actspsbmp;
extern Bitmap **actspswb;
extern IDriverDependantBitmap* *actspswbbmp;
extern CachedActSpsData* actspswbcache;

extern AGSStaticObject GlobalStaticManager;

StaticArray StaticCharacterArray;
StaticArray StaticObjectArray;
StaticArray StaticGUIArray;
StaticArray StaticHotspotArray;
StaticArray StaticRegionArray;
StaticArray StaticInventoryArray;
StaticArray StaticDialogArray;

// TODO: get rid of this variable and use loaded_game_file_version instead
int psp_is_old_datafile = 0; // Set for 3.1.1 and 3.1.2 datafiles
String game_file_name;


// Called when the game file is opened for the first time (when preloading game data);
// it logs information on data version and reports first found errors, if any.
MainGameFileError game_file_first_open(MainGameSource &src)
{
    MainGameFileError err = OpenMainGameFile(src);
    if (err == kMGFErr_NoError ||
        err == kMGFErr_SignatureFailed ||
        err == kMGFErr_FormatVersionTooOld ||
        err == kMGFErr_FormatVersionNotSupported)
    {
        // Log data description for debugging
        Out::FPrint("Opened game data file: %s", src.Filename.GetCStr());
        Out::FPrint("Game data version: %d", src.DataVersion);
        Out::FPrint("Requested engine version: %s", src.EngineVersion.LongString.GetCStr());
    }
    // Quit in case of error
    if (err == kMGFErr_FormatVersionTooOld || err == kMGFErr_FormatVersionNotSupported)
    {
        platform->DisplayAlert("This game requires an incompatible version of AGS (%s). It cannot be run.",
            src.EngineVersion.LongString.GetCStr());
        return err;
    }
    else if (err != kMGFErr_NoError)
        return err;

    // If the game was compiled for higher version of the engine, and yet has
    // supported data format, we warn about potential incompatibilities and
    // proceed
    if (src.EngineVersion > EngineVersion)
        platform->DisplayAlert("This game suggests a different version of AGS (%s). It may not run correctly.",
        src.EngineVersion.LongString.GetCStr());
    return kMGFErr_NoError;
}

MainGameFileError game_file_read_dialog_script(Stream *in, GameDataVersion data_ver)
{
	if (data_ver > kGameVersion_310) // 3.1.1+ dialog script
    {
        dialogScriptsScript = ccScript::CreateFromStream(in);
        if (dialogScriptsScript == NULL)
            return kMGFErr_CreateDialogScriptFailed;
    }
    else // 2.x and < 3.1.1 dialog
    {
        dialogScriptsScript = NULL;
    }
    return kMGFErr_NoError;
}

MainGameFileError game_file_read_script_modules(Stream *in, GameDataVersion data_ver)
{
	if (data_ver >= kGameVersion_270) // 2.7.0+ script modules
    {
        numScriptModules = in->ReadInt32();
        scriptModules.resize(numScriptModules);
        moduleInst.resize(numScriptModules, NULL);
        moduleInstFork.resize(numScriptModules, NULL);
        moduleRepExecAddr.resize(numScriptModules);
        repExecAlways.moduleHasFunction.resize(numScriptModules, true);
        lateRepExecAlways.moduleHasFunction.resize(numScriptModules, true);
        getDialogOptionsDimensionsFunc.moduleHasFunction.resize(numScriptModules, true);
        renderDialogOptionsFunc.moduleHasFunction.resize(numScriptModules, true);
        getDialogOptionUnderCursorFunc.moduleHasFunction.resize(numScriptModules, true);
        runDialogOptionMouseClickHandlerFunc.moduleHasFunction.resize(numScriptModules, true);
        runDialogOptionKeyPressHandlerFunc.moduleHasFunction.resize(numScriptModules, true);
        runDialogOptionRepExecFunc.moduleHasFunction.resize(numScriptModules, true);
        for (int bb = 0; bb < numScriptModules; bb++) {
            scriptModules[bb] = ccScript::CreateFromStream(in);
            if (scriptModules[bb] == NULL)
                return kMGFErr_CreateScriptModuleFailed;
            moduleRepExecAddr[bb].Invalidate();
        }
    }
    else
    {
        numScriptModules = 0;
    }
    return kMGFErr_NoError;
}

void ReadViewStruct272_Aligned(ViewStruct272* oldv, Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int iteratorCount = 0; iteratorCount < game.numviews; ++iteratorCount)
    {
        oldv[iteratorCount].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void game_file_read_views(Stream *in, GameDataVersion data_ver)
{
	if (data_ver > kGameVersion_272) // 3.x views
    {
        for (int iteratorCount = 0; iteratorCount < game.numviews; ++iteratorCount)
        {
            views[iteratorCount].ReadFromFile(in);
        }
    }
    else // 2.x views
    {
        ViewStruct272* oldv = (ViewStruct272*)calloc(game.numviews, sizeof(ViewStruct272));
        ReadViewStruct272_Aligned(oldv, in);
        Convert272ViewsToNew(game.numviews, oldv, views);
        free(oldv);
    }
}

void set_default_glmsg (int msgnum, const char* val) {
    if (game.messages[msgnum-500] == NULL) {
        game.messages[msgnum-500] = (char*)malloc (strlen(val)+5);
        strcpy (game.messages[msgnum-500], val);
    }
}

void game_file_set_default_glmsg()
{
    set_default_glmsg (983, "Sorry, not now.");
    set_default_glmsg (984, "Restore");
    set_default_glmsg (985, "Cancel");
    set_default_glmsg (986, "Select a game to restore:");
    set_default_glmsg (987, "Save");
    set_default_glmsg (988, "Type a name to save as:");
    set_default_glmsg (989, "Replace");
    set_default_glmsg (990, "The save directory is full. You must replace an existing game:");
    set_default_glmsg (991, "Replace:");
    set_default_glmsg (992, "With:");
    set_default_glmsg (993, "Quit");
    set_default_glmsg (994, "Play");
    set_default_glmsg (995, "Are you sure you want to quit?");
}

void game_file_read_dialogs(Stream *in, GameDataVersion data_ver)
{
	dialog=(DialogTopic*)malloc(sizeof(DialogTopic)*game.numdialog+5);

    for (int iteratorCount = 0; iteratorCount < game.numdialog; ++iteratorCount)
    {
        dialog[iteratorCount].ReadFromFile(in);
    }

    if (data_ver <= kGameVersion_310) // Dialog script
    {
        old_dialog_scripts = (unsigned char**)malloc(game.numdialog * sizeof(unsigned char**));

        int i;
        for (int i = 0; i < game.numdialog; i++)
        {
            old_dialog_scripts[i] = (unsigned char*)malloc(dialog[i].codesize);
            in->Read(old_dialog_scripts[i], dialog[i].codesize);

            // Skip encrypted text script
            unsigned int script_size = in->ReadInt32();
            in->Seek(script_size);
        }

        // Read the dialog lines
        old_speech_lines = (char**)malloc(10000 * sizeof(char**));
        i = 0;

        if (data_ver <= kGameVersion_260)
        {
            // Plain text on <= 2.60
            char buffer[1000];
            bool end_reached = false;

            while (!end_reached)
            {
                char* nextchar = buffer;

                while (1)
                {
                    *nextchar = in->ReadInt8();
                    if (*nextchar == 0)
                        break;

                    if ((unsigned char)*nextchar == 0xEF)
                    {
                        end_reached = true;
                        in->Seek(-1);
                        break;
                    }

                    nextchar++;
                }

                if (end_reached)
                    break;

                old_speech_lines[i] = (char*)malloc(strlen(buffer) + 1);
                strcpy(old_speech_lines[i], buffer);
                i++;
            }
        }
        else
        {
            // Encrypted text on > 2.60
            while (1)
            {
                unsigned int newlen = in->ReadInt32();
                if (newlen == 0xCAFEBEEF)  // GUI magic
                {
                    in->Seek(-4);
                    break;
                }

                old_speech_lines[i] = (char*)malloc(newlen + 1);
                in->Read(old_speech_lines[i], newlen);
                old_speech_lines[i][newlen] = 0;
                decrypt_text(old_speech_lines[i]);

                i++;
            }
        }
        old_speech_lines = (char**)realloc(old_speech_lines, i * sizeof(char**));
    }
}

void game_file_read_gui(Stream *in)
{
	read_gui(in,guis,&game);

    for (int bb = 0; bb < numguilabels; bb++) {
        // labels are not clickable by default
        guilabels[bb].SetClickable(false);
    }

    play.gui_draw_order = (int*)calloc(game.numgui * sizeof(int), 1);
}

void game_file_set_score_sound(GameSetupStruct::GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever >= kGameVersion_320) {
        play.score_sound = read_data.score_sound;
    }
    else {
        play.score_sound = -1;
        if (game.options[OPT_SCORESOUND] > 0)
        {
            ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, game.options[OPT_SCORESOUND]);
            if (clip)
                play.score_sound = clip->id;
            else
                play.score_sound = -1;
        }
    }
}

void init_and_register_characters()
{
	characterScriptObjNames = (char**)malloc(sizeof(char*) * game.numcharacters);

    for (int ee=0;ee<game.numcharacters;ee++) {
        game.chars[ee].walking = 0;
        game.chars[ee].animating = 0;
        game.chars[ee].pic_xoffs = 0;
        game.chars[ee].pic_yoffs = 0;
        game.chars[ee].blinkinterval = 140;
        game.chars[ee].blinktimer = game.chars[ee].blinkinterval;
        game.chars[ee].index_id = ee;
        game.chars[ee].blocking_width = 0;
        game.chars[ee].blocking_height = 0;
        game.chars[ee].prevroom = -1;
        game.chars[ee].loop = 0;
        game.chars[ee].frame = 0;
        game.chars[ee].walkwait = -1;
        ccRegisterManagedObject(&game.chars[ee], &ccDynamicCharacter);

        // export the character's script object
        characterScriptObjNames[ee] = (char*)malloc(strlen(game.chars[ee].scrname) + 5);
        strcpy(characterScriptObjNames[ee], game.chars[ee].scrname);

        ccAddExternalDynamicObject(characterScriptObjNames[ee], &game.chars[ee], &ccDynamicCharacter);
    }
}

void init_and_register_hotspots()
{
	for (int ee = 0; ee < MAX_HOTSPOTS; ee++) {
        scrHotspot[ee].id = ee;
        scrHotspot[ee].reserved = 0;

        ccRegisterManagedObject(&scrHotspot[ee], &ccDynamicHotspot);
    }
}

void init_and_register_regions()
{
	for (int ee = 0; ee < MAX_REGIONS; ee++) {
        scrRegion[ee].id = ee;
        scrRegion[ee].reserved = 0;

        ccRegisterManagedObject(&scrRegion[ee], &ccDynamicRegion);
    }
}

void init_and_register_invitems()
{
	for (int ee = 0; ee < MAX_INV; ee++) {
        scrInv[ee].id = ee;
        scrInv[ee].reserved = 0;

        ccRegisterManagedObject(&scrInv[ee], &ccDynamicInv);

        if (game.invScriptNames[ee][0] != 0)
            ccAddExternalDynamicObject(game.invScriptNames[ee], &scrInv[ee], &ccDynamicInv);
    }
}

void init_and_register_dialogs()
{
	for (int ee = 0; ee < game.numdialog; ee++) {
        scrDialog[ee].id = ee;
        scrDialog[ee].reserved = 0;

        ccRegisterManagedObject(&scrDialog[ee], &ccDynamicDialog);

        if (game.dialogScriptNames[ee][0] != 0)
            ccAddExternalDynamicObject(game.dialogScriptNames[ee], &scrDialog[ee], &ccDynamicDialog);
    }
}

void init_and_register_guis()
{
	int ee;

	scrGui = (ScriptGUI*)malloc(sizeof(ScriptGUI) * game.numgui);
    for (ee = 0; ee < game.numgui; ee++) {
        // 64 bit: Using the id instead
        // scrGui[ee].gui = NULL;
        scrGui[ee].id = -1;
    }

    guiScriptObjNames = (char**)malloc(sizeof(char*) * game.numgui);

    for (ee=0;ee<game.numgui;ee++) {
        guis[ee].RebuildArray();
        if ((guis[ee].PopupStyle == kGUIPopupNone) || (guis[ee].PopupStyle == kGUIPopupNoAutoRemove))
            guis[ee].SetVisibility(kGUIVisibility_On);
        else
            guis[ee].SetVisibility(kGUIVisibility_Off);

        // export all the GUI's controls
        export_gui_controls(ee);

        // copy the script name to its own memory location
        // because ccAddExtSymbol only keeps a reference
        guiScriptObjNames[ee] = strdup(guis[ee].Name);

        // 64 bit: Using the id instead
        // scrGui[ee].gui = &guis[ee];
        scrGui[ee].id = ee;

        ccAddExternalDynamicObject(guiScriptObjNames[ee], &scrGui[ee], &ccDynamicGUI);
        ccRegisterManagedObject(&scrGui[ee], &ccDynamicGUI);
    }

    //ccRegisterManagedObject(&dummygui, NULL);
    //ccRegisterManagedObject(&dummyguicontrol, NULL);
}

void init_and_register_fonts()
{
	our_eip=-22;
    for (int ee=0;ee<game.numfonts;ee++) 
    {
        int fontsize = game.fontflags[ee] & FFLG_SIZEMASK;
        if (fontsize == 0)
            fontsize = 8;

        if ((game.options[OPT_NOSCALEFNT] == 0) && game.IsHiRes())
            fontsize *= 2;

        if (!wloadfont_size(ee, fontsize))
            quitprintf("Unable to load font %d, no renderer could load a matching file", ee);
    }
}

void init_and_register_game_objects()
{
	init_and_register_characters();
	init_and_register_hotspots();
	init_and_register_regions();
	init_and_register_invitems();
	init_and_register_dialogs();
	init_and_register_guis();
    init_and_register_fonts();    

    play.fade_effect=game.options[OPT_FADETYPE];

    our_eip=-21;

    for (int ee = 0; ee < MAX_INIT_SPR; ee++) {
        ccRegisterManagedObject(&scrObj[ee], &ccDynamicObject);
    }

    register_audio_script_objects();

    ccRegisterManagedObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);

    dialogOptionsRenderingSurface = new ScriptDrawingSurface();
    dialogOptionsRenderingSurface->isLinkedBitmapOnly = true;
    long dorsHandle = ccRegisterManagedObject(dialogOptionsRenderingSurface, dialogOptionsRenderingSurface);
    ccAddObjectReference(dorsHandle);

    StaticCharacterArray.Create(&ccDynamicCharacter, sizeof(CharacterInfo), sizeof(CharacterInfo));
    StaticObjectArray.Create(&ccDynamicObject, sizeof(ScriptObject), sizeof(ScriptObject));
    StaticGUIArray.Create(&ccDynamicGUI, sizeof(ScriptGUI), sizeof(ScriptGUI));
    StaticHotspotArray.Create(&ccDynamicHotspot, sizeof(ScriptHotspot), sizeof(ScriptHotspot));
    StaticRegionArray.Create(&ccDynamicRegion, sizeof(ScriptRegion), sizeof(ScriptRegion));
    StaticInventoryArray.Create(&ccDynamicInv, sizeof(ScriptInvItem), sizeof(ScriptInvItem));
    StaticDialogArray.Create(&ccDynamicDialog, sizeof(ScriptDialog), sizeof(ScriptDialog));

    ccAddExternalStaticArray("character",&game.chars[0], &StaticCharacterArray);
    setup_player_character(game.playercharacter);
    if (loaded_game_file_version >= kGameVersion_270) {
        ccAddExternalStaticObject("player", &_sc_PlayerCharPtr, &GlobalStaticManager);
    }
    ccAddExternalStaticArray("object",&scrObj[0], &StaticObjectArray);
    ccAddExternalStaticArray("gui",&scrGui[0], &StaticGUIArray);
    ccAddExternalStaticArray("hotspot",&scrHotspot[0], &StaticHotspotArray);
    ccAddExternalStaticArray("region",&scrRegion[0], &StaticRegionArray);
    ccAddExternalStaticArray("inventory",&scrInv[0], &StaticInventoryArray);
    ccAddExternalStaticArray("dialog", &scrDialog[0], &StaticDialogArray);
}

void ReadGameSetupStructBase_Aligned(Stream *in)
{
    GameSetupStructBase *gameBase = (GameSetupStructBase *) &game;
    AlignedStream align_s(in, Common::kAligned_Read);
    gameBase->ReadFromFile(&align_s);
}

void WriteGameSetupStructBase_Aligned(Stream *out)
{
    GameSetupStructBase *gameBase = (GameSetupStructBase *) &game;
    AlignedStream align_s(out, Common::kAligned_Write);
    gameBase->WriteToFile(&align_s);
}

void PreReadSaveFileInfo(Stream *in, GameDataVersion data_ver)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    game.ReadFromFile(&align_s);
    // Discard game messages we do not need here
    delete [] game.load_messages;
    game.load_messages = NULL;

    GameSetupStruct::GAME_STRUCT_READ_DATA read_data;
    read_data.filever        = data_ver;
    read_data.saveGameSuffix = saveGameSuffix;
    game.read_savegame_info(in, read_data);
}

void fixup_save_directory()
{
    // If the save game folder was not specified by game author, create one of
    // the game name, game GUID, or data file name, as a last resort
    if (!game.saveGameFolderName[0])
    {
        if (game.gamename[0])
            snprintf(game.saveGameFolderName, MAX_SG_FOLDER_LEN - 1, "%s", game.gamename);
        else if (game.guid[0])
            snprintf(game.saveGameFolderName, MAX_SG_FOLDER_LEN - 1, "%s", game.guid);
        else
            snprintf(game.saveGameFolderName, MAX_SG_FOLDER_LEN - 1, "AGS-Game-%d", game.uniqueid);
    }
    // Lastly, fixup folder name by removing any illegal characters
    FixupFilename(game.saveGameFolderName);
}

bool preload_game_data(String &err_str)
{
    MainGameSource src;
    MainGameFileError err = game_file_first_open(src);
    if (err != kMGFErr_NoError)
    {
        err_str = GetMainGameFileErrorText(err);
        return false;
    }
    // Read only the particular data we need for preliminary game analysis
    PreReadSaveFileInfo(src.InputStream.get(), src.DataVersion);
    fixup_save_directory();
    return true;
}

MainGameFileError load_game_file()
{
    game_paused = 0;  // reset the game paused flag
    ifacepopped = -1;

	//-----------------------------------------------------------
	// Start reading from file
	//-----------------------------------------------------------

    MainGameSource src;
    MainGameFileError err = OpenMainGameFile(src);
    if (err != kMGFErr_NoError)
        return err;
    Stream *in = src.InputStream.get();
    // Remember loaded data version
    const GameDataVersion data_ver = src.DataVersion;
    if (src.DataVersion < kGameVersion_321)
        psp_is_old_datafile = 1;

    our_eip=-18;

    setup_script_exports();

    our_eip=-16;

    game.charScripts = NULL;
    game.invScripts = NULL;
    memset(&game.spriteflags[0], 0, MAX_SPRITES);

    ReadGameSetupStructBase_Aligned(in);

    if (game.size.IsNull())
        return kMGFErr_InvalidNativeResolution;

    // The earlier versions of AGS provided support for "upscaling" low-res
    // games (320x200 and 320x240) to hi-res (640x400 and 640x480
    // respectively). The script API has means for detecting if the game is
    // running upscaled, and game developer could use this opportunity to setup
    // game accordingly (e.g. assign hi-res fonts, etc).
    // This feature is officially deprecated since 3.1.0, however the engine
    // itself still supports it, technically.
    // This overriding option re-enables "upscaling". It works ONLY for low-res
    // resolutions, such as 320x200 and 320x240.
    if (usetup.override_upscale)
    {
        if (game.GetDefaultResolution() == kGameResolution_320x200)
            game.SetDefaultResolution(kGameResolution_640x400);
        else if (game.GetDefaultResolution() == kGameResolution_320x240)
            game.SetDefaultResolution(kGameResolution_640x480);
    }

    if (data_ver < kGameVersion_312)
    {
        // Fix animation speed for old formats
		game.options[OPT_GLOBALTALKANIMSPD] = 5;
    }
    else if (data_ver < kGameVersion_330)
    {
        // Convert game option for 3.1.2 - 3.2 games
        game.options[OPT_GLOBALTALKANIMSPD] = game.options[OPT_GLOBALTALKANIMSPD] != 0 ? 5 : (-5 - 1);
    }

    // Define old dialog options API for pre-3.4.0.2 games
    if (data_ver < kGameVersion_340_2)
        game.options[OPT_DIALOGOPTIONSAPI] = -1;

    if (game.numfonts > MAX_FONTS)
        return kMGFErr_TooManyFonts;

    GameSetupStruct::GAME_STRUCT_READ_DATA read_data;
    read_data.filever        = data_ver;
    read_data.saveGameSuffix = saveGameSuffix;
    read_data.max_audio_types= MAX_AUDIO_TYPES;
    read_data.game_file_name = game_file_name;
    game.ReadFromFile_Part1(in, read_data);

    if (!game.load_compiled_script)
        return kMGFErr_NoGlobalScript;

    gamescript = ccScript::CreateFromStream(in);
    if (gamescript == NULL)
        return kMGFErr_CreateGlobalScriptFailed;
    err = game_file_read_dialog_script(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    err = game_file_read_script_modules(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    our_eip=-15;

    charextra = (CharacterExtras*)calloc(game.numcharacters, sizeof(CharacterExtras));
    mls = (MoveList*)calloc(game.numcharacters + MAX_INIT_SPR + 1, sizeof(MoveList));
    actSpsCount = game.numcharacters + MAX_INIT_SPR + 2;
    actsps = (Bitmap **)calloc(actSpsCount, sizeof(Bitmap *));
    actspsbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswb = (Bitmap **)calloc(actSpsCount, sizeof(Bitmap *));
    actspswbbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswbcache = (CachedActSpsData*)calloc(actSpsCount, sizeof(CachedActSpsData));
    game.charProps.resize(game.numcharacters);
    play.charProps.resize(game.numcharacters);

    allocate_memory_for_views(game.numviews);
    int iteratorCount = 0;

	game_file_read_views(in, data_ver);

    our_eip=-14;

    if (data_ver <= kGameVersion_251) // <= 2.1 skip unknown data
    {
        int count = in->ReadInt32();
        in->Seek(count * 0x204);
    }

    charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*game.numcharacters+5);
    //-----------------------------------------------------
    game.ReadFromFile_Part2(in, read_data);
    //-----------------------------------------------------

    game_file_set_default_glmsg();
    
    our_eip=-13;

	game_file_read_dialogs(in, data_ver);

	game_file_read_gui(in);

    if (data_ver >= kGameVersion_260) // >= 2.60
    {
        platform->ReadPluginsFromDisk(in);
    }

    //-----------------------------------------------------
    game.ReadFromFile_Part3(in, read_data);
    //-----------------------------------------------------

    game_file_set_score_sound(read_data);

	//-----------------------------------------------------------
	// Reading from file is finished here
	//-----------------------------------------------------------

    fixup_save_directory();

    update_gui_zorder();

    if (game.numfonts == 0)
        return kMGFErr_NoFonts; // old v2.00 version (?)

    our_eip=-11;

	init_and_register_game_objects();    

    our_eip = -23;

    platform->StartPlugins();

    our_eip = -24;

    ccSetScriptAliveTimer(150000);
    ccSetStringClassImpl(&myScriptStringImpl);

    if (create_global_script())
        return kMGFErr_ScriptLinkFailed;
    return kMGFErr_NoError;
}

bool load_game_file(String &err_str)
{
    MainGameFileError load_err = load_game_file();
    if (load_err != kMGFErr_NoError)
    {
        err_str = GetMainGameFileErrorText(load_err);
        return false;
    }
    return true;
}

void display_game_file_error(const String &err_str)
{
    platform->DisplayAlert(String::FromFormat("Loading game failed with error:\n%s.\n\nThe game package may be incomplete, corrupt or from unsupported version of AGS.",
        err_str.GetCStr()));
}

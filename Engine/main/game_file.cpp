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
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gamestructdefines.h"
#include "ac/gui.h"
#include "ac/viewframe.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "debug/debug_log.h"
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

using AGS::Common::AlignedStream;
using AGS::Common::Bitmap;
using AGS::Common::Stream;
using AGS::Common::String;

extern char saveGameSuffix[MAX_SG_EXT_LENGTH + 1];

// Old dialog support
extern unsigned char** old_dialog_scripts; // defined in ac_conversation
extern char** old_speech_lines;

extern DynamicArray<GUILabel> guilabels; // defined in ac_guilabel
extern int numguilabels;

extern int ifacepopped;

extern GameSetupStruct game;
extern ViewStruct*views;
extern DialogTopic *dialog;
extern GUIMain*guis;
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
extern ccScript *scriptModules[MAX_SCRIPT_MODULES];
extern ccInstance *moduleInst[MAX_SCRIPT_MODULES];
extern ccInstance *moduleInstFork[MAX_SCRIPT_MODULES];
extern RuntimeScriptValue moduleRepExecAddr[MAX_SCRIPT_MODULES];
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

GameDataVersion filever;
// PSP specific variables:
int psp_is_old_datafile = 0; // Set for 3.1.1 and 3.1.2 datafiles
String game_file_name;


Stream * game_file_open()
{
	Stream*in = Common::AssetManager::OpenAsset("game28.dta"); // 3.x data file name
    if (in==NULL) {
        in = Common::AssetManager::OpenAsset("ac2game.dta"); // 2.x data file name
    }

	return in;
}

int game_file_read_version(Stream *in)
{
	char teststr[31];

	teststr[30]=0;
    in->Read(&teststr[0],30);
    filever=(GameDataVersion)in->ReadInt32();

    if (filever < kGameVersion_321) {
        // Allow loading of 2.x+ datafiles
        if (filever < kGameVersion_250) // < 2.5.0
        {
            delete in;
            return -2;
        }
        psp_is_old_datafile = 1;
    }

	int engineverlen = in->ReadInt32();
    String version_string = String::FromStreamCount(in, engineverlen);
    AGS::Engine::Version requested_engine_version(version_string);

    if (filever > kGameVersion_Current) {
        platform->DisplayAlert("This game requires a newer version of AGS (%s). It cannot be run.",
            requested_engine_version.LongString.GetCStr());
        delete in;
        return -2;
    }

    if (requested_engine_version > EngineVersion)
        platform->DisplayAlert("This game requires a newer version of AGS (%s). It may not run correctly.",
        requested_engine_version.LongString.GetCStr());

    loaded_game_file_version = filever;

	return RETURN_CONTINUE;
}

void game_file_read_dialog_script(Stream *in)
{
	if (filever > kGameVersion_310) // 3.1.1+ dialog script
    {
        dialogScriptsScript = ccScript::CreateFromStream(in);
        if (dialogScriptsScript == NULL)
            quit("Dialog scripts load failed; need newer version?");
    }
    else // 2.x and < 3.1.1 dialog
    {
        dialogScriptsScript = NULL;
    }
}

void game_file_read_script_modules(Stream *in)
{
	if (filever >= kGameVersion_270) // 2.7.0+ script modules
    {
        numScriptModules = in->ReadInt32();
        if (numScriptModules > MAX_SCRIPT_MODULES)
            quit("too many script modules; need newer version?");

        for (int bb = 0; bb < numScriptModules; bb++) {
            scriptModules[bb] = ccScript::CreateFromStream(in);
            if (scriptModules[bb] == NULL)
                quit("Script module load failure; need newer version?");
            moduleInst[bb] = NULL;
            moduleInstFork[bb] = NULL;
            moduleRepExecAddr[bb].Invalidate();
        }
    }
    else
    {
        numScriptModules = 0;
    }
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

void game_file_read_views(Stream *in)
{
	if (filever > kGameVersion_272) // 3.x views
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

void game_file_read_dialogs(Stream *in)
{
	dialog=(DialogTopic*)malloc(sizeof(DialogTopic)*game.numdialog+5);

    for (int iteratorCount = 0; iteratorCount < game.numdialog; ++iteratorCount)
    {
        dialog[iteratorCount].ReadFromFile(in);
    }

    if (filever <= kGameVersion_300) // Dialog script
    {
        old_dialog_scripts = (unsigned char**)malloc(game.numdialog * sizeof(unsigned char**));

        int i;
        for (int i = 0; i < game.numdialog; i++)
        {
            old_dialog_scripts[i] = (unsigned char*)malloc(dialog[i].codesize);
            in->Read(old_dialog_scripts[i], dialog[i].codesize);

            // Skip encrypted text script
            unsigned int script_size = in->ReadInt32();
            in->Seek(Common::kSeekCurrent, script_size);
        }

        // Read the dialog lines
        old_speech_lines = (char**)malloc(10000 * sizeof(char**));
        i = 0;

        if (filever <= kGameVersion_260)
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
                        in->Seek(Common::kSeekCurrent, -1);
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
                    in->Seek(Common::kSeekCurrent, -4);
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
	read_gui(in,guis,&game, &guis);

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
        guis[ee].rebuild_array();
        if ((guis[ee].popup == POPUP_NONE) || (guis[ee].popup == POPUP_NOAUTOREM))
            guis[ee].on = 1;
        else
            guis[ee].on = 0;

        // export all the GUI's controls
        export_gui_controls(ee);

        // copy the script name to its own memory location
        // because ccAddExtSymbol only keeps a reference
        guiScriptObjNames[ee] = (char*)malloc(21);
        strcpy(guiScriptObjNames[ee], guis[ee].name);

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

        if ((game.options[OPT_NOSCALEFNT] == 0) && (game.default_resolution > 2))
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

int load_game_file() {

	int res;    

    game_paused = 0;  // reset the game paused flag
    ifacepopped = -1;

	//-----------------------------------------------------------
	// Start reading from file
	//-----------------------------------------------------------

    Stream *in = game_file_open();
	if (in==NULL)
		return -1;

    our_eip=-18;

    setup_script_exports();

    our_eip=-16;

	res = game_file_read_version(in);
	if (res != RETURN_CONTINUE) {
		return res;
	}

    game.charScripts = NULL;
    game.invScripts = NULL;
    memset(&game.spriteflags[0], 0, MAX_SPRITES);

    ReadGameSetupStructBase_Aligned(in);

    if (filever < kGameVersion_312)
    {
        // Fix animation speed for old formats
		game.options[OPT_GLOBALTALKANIMSPD] = 5;
    }
    else if (filever < kGameVersion_330)
    {
        // Convert game option for 3.1.2 - 3.2 games
        game.options[OPT_GLOBALTALKANIMSPD] = game.options[OPT_GLOBALTALKANIMSPD] != 0 ? 5 : (-5 - 1);
    }

    // 3.20: Fixed GUI AdditiveOpacity mode not working properly if you tried to have a non-alpha sprite on an alpha GUI
    if (loaded_game_file_version < kGameVersion_320)
    {
        // Force new style rendering for gui sprites with alpha channel
        game.options[OPT_NEWGUIALPHA] = kGuiAlphaRender_AdditiveOpacitySrcCopy;
    }

    if (game.numfonts > MAX_FONTS)
        quit("!This game requires a newer version of AGS. Too many fonts for this version to handle.");

    GameSetupStruct::GAME_STRUCT_READ_DATA read_data;
    read_data.filever        = filever;
    read_data.saveGameSuffix = saveGameSuffix;
    read_data.max_audio_types= MAX_AUDIO_TYPES;
    read_data.game_file_name = game_file_name;

    //-----------------------------------------------------
    game.ReadFromFile_Part1(in, read_data);
    //-----------------------------------------------------

    if (!game.load_compiled_script)
        quit("No global script in game; data load error");

    gamescript = ccScript::CreateFromStream(in);
    if (gamescript == NULL)
        quit("Global script load failed; need newer version?");

    game_file_read_dialog_script(in);

	game_file_read_script_modules(in);

    our_eip=-15;

    charextra = (CharacterExtras*)calloc(game.numcharacters, sizeof(CharacterExtras));
    mls = (MoveList*)calloc(game.numcharacters + MAX_INIT_SPR + 1, sizeof(MoveList));
    actSpsCount = game.numcharacters + MAX_INIT_SPR + 2;
    actsps = (Bitmap **)calloc(actSpsCount, sizeof(Bitmap *));
    actspsbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswb = (Bitmap **)calloc(actSpsCount, sizeof(Bitmap *));
    actspswbbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswbcache = (CachedActSpsData*)calloc(actSpsCount, sizeof(CachedActSpsData));
    game.charProps = (CustomProperties*)calloc(game.numcharacters, sizeof(CustomProperties));

    allocate_memory_for_views(game.numviews);
    int iteratorCount = 0;

	game_file_read_views(in);

    our_eip=-14;

    if (filever <= kGameVersion_251) // <= 2.1 skip unknown data
    {
        int count = in->ReadInt32();
        in->Seek(Common::kSeekCurrent, count * 0x204);
    }

    charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*game.numcharacters+5);
    //-----------------------------------------------------
    game.ReadFromFile_Part2(in, read_data);
    //-----------------------------------------------------

    game_file_set_default_glmsg();
    
    our_eip=-13;

	game_file_read_dialogs(in);

	game_file_read_gui(in);

    if (filever >= kGameVersion_260) // >= 2.60
    {
        platform->ReadPluginsFromDisk(in);
    }

    //-----------------------------------------------------
    game.ReadFromFile_Part3(in, read_data);
    //-----------------------------------------------------

    game_file_set_score_sound(read_data);

	delete in;

	//-----------------------------------------------------------
	// Reading from file is finished here
	//-----------------------------------------------------------

    update_gui_zorder();

    if (game.numfonts == 0)
        return -2;  // old v2.00 version

    our_eip=-11;

	init_and_register_game_objects();    

    our_eip = -23;

    platform->StartPlugins();

    our_eip = -24;

    ccSetScriptAliveTimer(150000);
    ccSetStringClassImpl(&myScriptStringImpl);

    if (create_global_script())
        return -3;

    return 0;
}

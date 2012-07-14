/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

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
#include "debug/debug.h"
#include "gui/guilabel.h"
#include "main/main.h"
#include "platform/agsplatformdriver.h"
#include "script/exports.h"
#include "script/script.h"

/*

Game data versions and changes:
-------------------------------

12 : 2.3 + 2.4

Versions above are incompatible at the moment.

19 : 2.5.1
22 : 2.5.5

Variable number of sprites.
24 : 2.5.6
25 : 2.6.0

Encrypted global messages and dialogs.
26 : 2.6.1
27 : 2.6.2

Script modules. Fixes bug in the inventory display.
31 : 2.7.0
32 : 2.7.2

Interactions are now scripts. The number for "not set" changed from 0 to -1 for
a lot of variables (views, sounds).
37 : 3.0 + 3.1.0

Dialogs are now scripts. New character animation speed.
39 : 3.1.1
40 : 3.1.2

Audio clips
41 : 3.2.0
42 : 3.2.1

*/

extern int engineNeedsAsInt; // defined in ac_game
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
extern char *moduleRepExecAddr[MAX_SCRIPT_MODULES];
extern int numScriptModules;
extern GameState play;
extern char **characterScriptObjNames;
extern char objectScriptObjNames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN + 5];
extern char **guiScriptObjNames;
extern int actSpsCount;
extern block *actsps;
extern IDriverDependantBitmap* *actspsbmp;
extern block *actspswb;
extern IDriverDependantBitmap* *actspswbbmp;
extern CachedActSpsData* actspswbcache;


int filever;
// PSP specific variables:
int psp_is_old_datafile = 0; // Set for 3.1.1 and 3.1.2 datafiles
char* game_file_name = NULL;


FILE * game_file_open()
{
	FILE*iii = clibfopen("game28.dta", "rb"); // 3.x data file name
    if (iii==NULL) {
        iii = clibfopen("ac2game.dta", "rb"); // 2.x data file name
    }

	return iii;
}

int game_file_read_version(FILE*iii)
{
	char teststr[31];

	teststr[30]=0;
    fread(&teststr[0],30,1,iii);
    filever=getw(iii);

    if (filever < 42) {
        // Allow loading of 2.x+ datafiles
        if (filever < 18) // < 2.5.0
        {
            fclose(iii);
            return -2;
        }
        psp_is_old_datafile = 1;
    }

	int engineverlen = getw(iii);
    char engineneeds[20];
    // MACPORT FIX 13/6/5: switch 'size' and 'nmemb' so it doesn't treat the string as an int
    fread(&engineneeds[0], sizeof(char), engineverlen, iii);
    engineneeds[engineverlen] = 0;

    if (filever > GAME_FILE_VERSION) {
        platform->DisplayAlert("This game requires a newer version of AGS (%s). It cannot be run.", engineneeds);
        fclose(iii);
        return -2;
    }

    if (strcmp (engineneeds, ACI_VERSION_TEXT) > 0)
        platform->DisplayAlert("This game requires a newer version of AGS (%s). It may not run correctly.", engineneeds);

    {
        int major, minor;
        sscanf(engineneeds, "%d.%d", &major, &minor);
        engineNeedsAsInt = 100*major + minor;
    }

    loaded_game_file_version = filever;

	return RETURN_CONTINUE;
}

void game_file_read_dialog_script(FILE*iii)
{
	if (filever > 37) // 3.1.1+ dialog script
    {
        dialogScriptsScript = fread_script(iii);
        if (dialogScriptsScript == NULL)
            quit("Dialog scripts load failed; need newer version?");
    }
    else // 2.x and < 3.1.1 dialog
    {
        dialogScriptsScript = NULL;
    }
}

void game_file_read_script_modules(FILE*iii)
{
	if (filever >= 31) // 2.7.0+ script modules
    {
        numScriptModules = getw(iii);
        if (numScriptModules > MAX_SCRIPT_MODULES)
            quit("too many script modules; need newer version?");

        for (int bb = 0; bb < numScriptModules; bb++) {
            scriptModules[bb] = fread_script(iii);
            if (scriptModules[bb] == NULL)
                quit("Script module load failure; need newer version?");
            moduleInst[bb] = NULL;
            moduleInstFork[bb] = NULL;
            moduleRepExecAddr[bb] = NULL;
        }
    }
    else
    {
        numScriptModules = 0;
    }
}

void game_file_read_views(FILE*iii)
{
	if (filever > 32) // 3.x views
    {
        for (int iteratorCount = 0; iteratorCount < game.numviews; ++iteratorCount)
        {
            views[iteratorCount].ReadFromFile(iii);
        }
    }
    else // 2.x views
    {
        ViewStruct272* oldv = (ViewStruct272*)calloc(game.numviews, sizeof(ViewStruct272));
        for (int iteratorCount = 0; iteratorCount < game.numviews; ++iteratorCount)
        {
            oldv[iteratorCount].ReadFromFile(iii);
        }
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

void game_file_read_dialogs(FILE*iii)
{
	dialog=(DialogTopic*)malloc(sizeof(DialogTopic)*game.numdialog+5);

//#ifdef ALLEGRO_BIG_ENDIAN
    for (int iteratorCount = 0; iteratorCount < game.numdialog; ++iteratorCount)
    {
        dialog[iteratorCount].ReadFromFile(iii);
    }
//#else
//    fread(&dialog[0],sizeof(DialogTopic),game.numdialog,iii);  
//#endif

    if (filever <= 37) // Dialog script
    {
        old_dialog_scripts = (unsigned char**)malloc(game.numdialog * sizeof(unsigned char**));

        int i;
        for (int i = 0; i < game.numdialog; i++)
        {
            old_dialog_scripts[i] = (unsigned char*)malloc(dialog[i].codesize);
            fread(old_dialog_scripts[i], dialog[i].codesize, 1, iii);

            // Skip encrypted text script
            unsigned int script_size = getw(iii);
            fseek(iii, script_size, SEEK_CUR);
        }

        // Read the dialog lines
        old_speech_lines = (char**)malloc(10000 * sizeof(char**));
        i = 0;

        if (filever <= 25)
        {
            // Plain text on <= 2.60
            char buffer[1000];
            bool end_reached = false;

            while (!end_reached)
            {
                char* nextchar = buffer;

                while (1)
                {
                    *nextchar = fgetc(iii);
                    if (*nextchar == 0)
                        break;

                    if ((unsigned char)*nextchar == 0xEF)
                    {
                        end_reached = true;
                        fseek(iii, -1, SEEK_CUR);
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
                unsigned int newlen = getw(iii);
                if (newlen == 0xCAFEBEEF)  // GUI magic
                {
                    fseek(iii, -4, SEEK_CUR);
                    break;
                }

                old_speech_lines[i] = (char*)malloc(newlen + 1);
                fread(old_speech_lines[i], newlen, 1, iii);
                old_speech_lines[i][newlen] = 0;
                decrypt_text(old_speech_lines[i]);

                i++;
            }
        }
        old_speech_lines = (char**)realloc(old_speech_lines, i * sizeof(char**));
    }
}

void game_file_read_gui(FILE*iii)
{
	read_gui(iii,guis,&game, &guis);

    for (int bb = 0; bb < numguilabels; bb++) {
        // labels are not clickable by default
        guilabels[bb].SetClickable(false);
    }

    play.gui_draw_order = (int*)calloc(game.numgui * sizeof(int), 1);
}

void game_file_set_score_sound(GameSetupStruct::GAME_STRUCT_READ_DATA &read_data)
{
    if (read_data.filever >= 41) {
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

        ccAddExternalSymbol(characterScriptObjNames[ee], &game.chars[ee]);
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
            ccAddExternalSymbol(game.invScriptNames[ee], &scrInv[ee]);
    }
}

void init_and_register_dialogs()
{
	for (int ee = 0; ee < game.numdialog; ee++) {
        scrDialog[ee].id = ee;
        scrDialog[ee].reserved = 0;

        ccRegisterManagedObject(&scrDialog[ee], &ccDynamicDialog);

        if (game.dialogScriptNames[ee][0] != 0)
            ccAddExternalSymbol(game.dialogScriptNames[ee], &scrDialog[ee]);
    }
}

void init_and_register_guis()
{
	int ee;

	scrGui = (ScriptGUI*)malloc(sizeof(ScriptGUI) * game.numgui);
    for (ee = 0; ee < game.numgui; ee++) {
        scrGui[ee].gui = NULL;
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

        scrGui[ee].gui = &guis[ee];
        scrGui[ee].id = ee;

        ccAddExternalSymbol(guiScriptObjNames[ee], &scrGui[ee]);
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

    wtexttransparent(TEXTFG);
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

    ccAddExternalSymbol("character",&game.chars[0]);
    setup_player_character(game.playercharacter);
    ccAddExternalSymbol("player", &_sc_PlayerCharPtr);
    ccAddExternalSymbol("object",&scrObj[0]);
    ccAddExternalSymbol("gui",&scrGui[0]);
    ccAddExternalSymbol("hotspot",&scrHotspot[0]);
    ccAddExternalSymbol("region",&scrRegion[0]);
    ccAddExternalSymbol("inventory",&scrInv[0]);
    ccAddExternalSymbol("dialog", &scrDialog[0]);
}

int load_game_file() {

	int res;    

    game_paused = 0;  // reset the game paused flag
    ifacepopped = -1;

	//-----------------------------------------------------------
	// Start reading from file
	//-----------------------------------------------------------

    FILE*iii = game_file_open();
	if (iii==NULL)
		return -1;

    our_eip=-18;

    setup_script_exports();

    our_eip=-16;

	res = game_file_read_version(iii);
	if (res != RETURN_CONTINUE) {
		return res;
	}

    game.charScripts = NULL;
    game.invScripts = NULL;
    memset(&game.spriteflags[0], 0, MAX_SPRITES);

//#ifdef ALLEGRO_BIG_ENDIAN
    GameSetupStructBase *gameBase = (GameSetupStructBase *) &game;
    gameBase->ReadFromFile(iii);
//#else
//    fread(&game, sizeof (GameSetupStructBase), 1, iii);
//#endif

    if (filever <= 37) // <= 3.1
    {
        // Fix animation speed for old formats
        game.options[OPT_OLDTALKANIMSPD] = 1;
    }

    if (game.numfonts > MAX_FONTS)
        quit("!This game requires a newer version of AGS. Too many fonts for this version to handle.");

    GameSetupStruct::GAME_STRUCT_READ_DATA read_data;
    read_data.filever        = filever;
    read_data.saveGameSuffix = saveGameSuffix;
    read_data.max_audio_types= MAX_AUDIO_TYPES;
    read_data.game_file_name = game_file_name;

    //-----------------------------------------------------
    game.ReadFromFile_Part1(iii, read_data);
    //-----------------------------------------------------

    if (game.compiled_script == NULL)
        quit("No global script in game; data load error");

    gamescript = fread_script(iii);
    if (gamescript == NULL)
        quit("Global script load failed; need newer version?");

    game_file_read_dialog_script(iii);

	game_file_read_script_modules(iii);

    our_eip=-15;

    charextra = (CharacterExtras*)calloc(game.numcharacters, sizeof(CharacterExtras));
    mls = (MoveList*)calloc(game.numcharacters + MAX_INIT_SPR + 1, sizeof(MoveList));
    actSpsCount = game.numcharacters + MAX_INIT_SPR + 2;
    actsps = (block*)calloc(actSpsCount, sizeof(block));
    actspsbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswb = (block*)calloc(actSpsCount, sizeof(block));
    actspswbbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
    actspswbcache = (CachedActSpsData*)calloc(actSpsCount, sizeof(CachedActSpsData));
    game.charProps = (CustomProperties*)calloc(game.numcharacters, sizeof(CustomProperties));

    allocate_memory_for_views(game.numviews);
    int iteratorCount = 0;

	game_file_read_views(iii);

    our_eip=-14;

    if (filever <= 19) // <= 2.1 skip unknown data
    {
        int count = getw(iii);
        fseek(iii, count * 0x204, SEEK_CUR);
    }

    charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*game.numcharacters+5);
    //-----------------------------------------------------
    game.ReadFromFile_Part2(iii, read_data);
    //-----------------------------------------------------

    game_file_set_default_glmsg();
    
    our_eip=-13;

	game_file_read_dialogs(iii);

	game_file_read_gui(iii);

    if (filever >= 25) // >= 2.60
    {
        platform->ReadPluginsFromDisk(iii);
    }

    //-----------------------------------------------------
    game.ReadFromFile_Part3(iii, read_data);
    //-----------------------------------------------------

    game_file_set_score_sound(read_data);

	fclose(iii);

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

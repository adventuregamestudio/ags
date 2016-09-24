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
#include "debug/debug_log.h"
#include "debug/out.h"
#include "gui/guilabel.h"
#include "main/main.h"
#include "platform/base/agsplatformdriver.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "gfx/blender.h"
#include "core/assetmanager.h"
#include "util/alignedstream.h"
#include "ac/gamesetup.h"
#include "game/main_game_file.h"
#include "game/game_init.h"
#include "plugin/agsplugin.h"
#include "script/script.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// Old dialog support (defined in ac/dialog)
extern std::vector< stdtr1compat::shared_ptr<unsigned char> > old_dialog_scripts;
extern std::vector<String> old_speech_lines;

extern int ifacepopped;

extern GameSetupStruct game;
extern ViewStruct*views;
extern DialogTopic *dialog;

extern int our_eip;

extern AGSPlatformDriver *platform;
extern int numScriptModules;

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
        dialogScriptsScript.reset(ccScript::CreateFromStream(in));
        if (dialogScriptsScript == NULL)
            return kMGFErr_CreateDialogScriptFailed;
    }
    else // 2.x and < 3.1.1 dialog
    {
        dialogScriptsScript.reset();
    }
    return kMGFErr_NoError;
}

MainGameFileError game_file_read_script_modules(Stream *in, GameDataVersion data_ver)
{
	if (data_ver >= kGameVersion_270) // 2.7.0+ script modules
    {
        numScriptModules = in->ReadInt32();
        scriptModules.resize(numScriptModules);
        for (int bb = 0; bb < numScriptModules; bb++) {
            scriptModules[bb].reset(ccScript::CreateFromStream(in));
            if (scriptModules[bb] == NULL)
                return kMGFErr_CreateScriptModuleFailed;
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

// Assigns default global message at given index
void set_default_glmsg (int msgnum, const char *val)
{
    // TODO: find out why the index should be lowered by 500
    // (or rather if we may pass correct index right away)
    msgnum -= 500;
    if (game.messages[msgnum] == NULL)
        game.messages[msgnum] = strdup(val);
}

// Sets up default global messages (these are used mainly in older games)
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
        old_dialog_scripts.resize(game.numdialog);

        int i;
        for (int i = 0; i < game.numdialog; i++)
        {
            old_dialog_scripts[i].reset(new unsigned char[dialog[i].codesize]);
            in->Read(old_dialog_scripts[i].get(), dialog[i].codesize);

            // Skip encrypted text script
            unsigned int script_size = in->ReadInt32();
            in->Seek(script_size);
        }

        // Read the dialog lines
        i = 0;
        // TODO: safer buffer reading
        char buffer[1000];
        if (data_ver <= kGameVersion_260)
        {
            // Plain text on <= 2.60
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

                old_speech_lines.push_back(buffer);
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

                newlen = Math::Min(newlen, sizeof(buffer) - 1);
                in->Read(buffer, newlen);
                buffer[newlen] = 0;
                decrypt_text(buffer);
                old_speech_lines.push_back(buffer);
                i++;
            }
        }
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

// Adjusts score clip id, depending on game data version
void game_file_set_score_sound(GameDataVersion data_ver)
{
    if (data_ver < kGameVersion_320)
    {
        game.scoreClipID = -1;
        if (game.options[OPT_SCORESOUND] > 0)
        {
            ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, game.options[OPT_SCORESOUND]);
            if (clip)
                game.scoreClipID = clip->id;
            else
                game.scoreClipID = -1;
        }
    }
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
    game.read_savegame_info(in, data_ver);
}

// Ensures that the game saves directory path is valid
void fixup_save_directory()
{
    // If the save game folder was not specified by game author, create one of
    // the game name, game GUID, or uniqueid, as a last resort
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

MainGameFileError load_game_file(Stream *in, GameDataVersion data_ver)
{
    game.charScripts = NULL;
    game.invScripts = NULL;
    memset(&game.spriteflags[0], 0, MAX_SPRITES);

    ReadGameSetupStructBase_Aligned(in);

    if (game.size.IsNull())
        return kMGFErr_InvalidNativeResolution;
    if (game.numfonts > MAX_FONTS)
        return kMGFErr_TooManyFonts;

    MainGameFileError err = game.ReadFromFile_Part1(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;

    if (!game.load_compiled_script)
        return kMGFErr_NoGlobalScript;
    gamescript.reset(ccScript::CreateFromStream(in));
    if (gamescript == NULL)
        return kMGFErr_CreateGlobalScriptFailed;
    err = game_file_read_dialog_script(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    err = game_file_read_script_modules(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    our_eip=-15;

    allocate_memory_for_views(game.numviews);
	game_file_read_views(in, data_ver);
    our_eip=-14;

    if (data_ver <= kGameVersion_251)
    {
        // skip unknown data
        int count = in->ReadInt32();
        in->Seek(count * 0x204);
    }

    err = game.ReadFromFile_Part2(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;

    our_eip=-13;
    game_file_read_dialogs(in, data_ver);
    game_file_read_gui(in);

    if (data_ver >= kGameVersion_260)
    {
        pl_read_plugins(in);
    }

    err = game.ReadFromFile_Part3(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;

    //
    // Convert and update old game settings.
    //
    game_file_set_score_sound(data_ver);
    game_file_set_default_glmsg();
    // Global talking animation speed
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
    // Old dialog options API for pre-3.4.0.2 games
    if (data_ver < kGameVersion_340_2)
        game.options[OPT_DIALOGOPTIONSAPI] = -1;
    fixup_save_directory();

    return kMGFErr_NoError;
}

bool load_game_file(String &err_str)
{
    MainGameSource src;
    MainGameFileError load_err = OpenMainGameFile(src);
    if (load_err == kMGFErr_NoError)
        load_err = load_game_file(src.InputStream.get(), src.DataVersion);
    if (load_err != kMGFErr_NoError)
    {
        err_str = GetMainGameFileErrorText(load_err);
        return false;
    }
    GameInitError init_err = InitGameState(src.DataVersion);
    if (init_err != kGameInitErr_NoError)
    {
        err_str = GetGameInitErrorText(init_err);
        return false;
    }
    return true;
}

void display_game_file_error(const String &err_str)
{
    platform->DisplayAlert(String::FromFormat("Loading game failed with error:\n%s.\n\nThe game package may be incomplete, corrupt or from unsupported version of AGS.",
        err_str.GetCStr()));
}

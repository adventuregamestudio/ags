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

#include "ac/dialogtopic.h"
#include "ac/gamesetupstruct.h"
#include "ac/view.h"
#include "core/asset.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/main_game_file.h"
#include "gui/guimain.h"
#include "script/cc_error.h"
#include "util/alignedstream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

const String MainGameSource::DefaultFilename_v3 = "game28.dta";
const String MainGameSource::DefaultFilename_v2 = "ac2game.dta";
const String MainGameSource::Signature = "Adventure Creator Game File v2";

MainGameSource::MainGameSource()
    : DataVersion(kGameVersion_Undefined)
{
}

String GetMainGameFileErrorText(MainGameFileError err)
{
    switch (err)
    {
    case kMGFErr_NoError:
        return "No error";
    case kMGFErr_FileNotFound:
        return "Main game file not found";
    case kMGFErr_NoStream:
        return "Failed to open input stream";
    case kMGFErr_SignatureFailed:
        return "Not an AGS main game file or unsupported format";
    case kMGFErr_FormatVersionTooOld:
        return "Format version is too old; this engine can only run games made with AGS 2.5 or later";
    case kMGFErr_FormatVersionNotSupported:
        return "Format version not supported";
    case kMGFErr_InvalidNativeResolution:
        return "Unable to determine native game resolution";
    case kMGFErr_TooManyFonts:
        return "Too many fonts for this engine to handle";
    case kMGFErr_TooManySprites:
        return "Too many sprites for this engine to handle";
    case kMGFErr_TooManyCursors:
        return "Too many cursors for this engine to handle";
    case kMGFErr_InvalidPropertySchema:
        return "load room: unable to deserialize properties schema";
    case kMGFErr_InvalidPropertyValues:
        return "Errors encountered when reading custom properties";
    case kMGFErr_NoGlobalScript:
        return "No global script in game";
    case kMGFErr_CreateGlobalScriptFailed:
        return String::FromFormat("Failed to load global script: %s", ccErrorString);
    case kMGFErr_CreateDialogScriptFailed:
        return String::FromFormat("Failed to load dialog script: %s", ccErrorString);
    case kMGFErr_CreateScriptModuleFailed:
        return String::FromFormat("Failed to load script module: %s", ccErrorString);
    case kMGFErr_PluginDataFmtNotSupported:
        return "Format version of plugin data is not supported";
    case kMGFErr_PluginDataSizeTooLarge:
        return "Plugin data size is too large";
    }
    return "Unknown error";
}

LoadedGameEntities::LoadedGameEntities(GameSetupStruct &game, DialogTopic *&dialogs, ViewStruct *&views)
    : Game(game)
    , Dialogs(dialogs)
    , Views(views)
{
}

bool IsMainGameLibrary(const String &filename)
{
    // We must not only detect if the given file is a correct AGS data library,
    // we also have to assure that this library contains main game asset.
    // Library may contain some optional data (digital audio, speech etc), but
    // that is not what we want.
    AssetLibInfo lib;
    if (AssetManager::ReadDataFileTOC(filename, lib) != kAssetNoError)
        return false;
    for (size_t i = 0; i < lib.AssetInfos.size(); ++i)
    {
        if (lib.AssetInfos[i].FileName.CompareNoCase(MainGameSource::DefaultFilename_v3) == 0 ||
            lib.AssetInfos[i].FileName.CompareNoCase(MainGameSource::DefaultFilename_v2) == 0)
        {
            return true;
        }
    }
    return false;
}

MainGameFileError OpenMainGameFile(MainGameSource &src)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to find and open main game file
    String filename = MainGameSource::DefaultFilename_v3;
    PStream in(AssetManager::OpenAsset(filename));
    if (!in)
    {
        filename = MainGameSource::DefaultFilename_v2;
        in = PStream(AssetManager::OpenAsset(filename));
    }
    if (!in)
        return kMGFErr_FileNotFound;
    src.Filename = filename;
    // Check data signature
    String data_sig = String::FromStreamCount(in.get(), MainGameSource::Signature.GetLength());
    if (data_sig.Compare(MainGameSource::Signature))
        return kMGFErr_SignatureFailed;
    // Read data format version and requested engine version
    src.DataVersion = (GameDataVersion)in->ReadInt32();
    if (src.DataVersion < kGameVersion_250)
        return kMGFErr_FormatVersionTooOld;
    String version_str = StrUtil::ReadString(in.get());
    src.EngineVersion.SetFromString(version_str);
    if (src.DataVersion > kGameVersion_Current)
        return kMGFErr_FormatVersionNotSupported;
    // Everything is fine, return opened stream
    src.InputStream = in;
    // Remember loaded game data version
    // NOTE: this global variable is embedded in the code too much to get
    // rid of it too easily; the easy way is to set it whenever the main
    // game file is opened.
    loaded_game_file_version = src.DataVersion;
    return kMGFErr_NoError;
}

MainGameFileError ReadDialogScript(PScript &dialog_script, Stream *in, GameDataVersion data_ver)
{
    if (data_ver > kGameVersion_310) // 3.1.1+ dialog script
    {
        dialog_script.reset(ccScript::CreateFromStream(in));
        if (dialog_script == NULL)
            return kMGFErr_CreateDialogScriptFailed;
    }
    else // 2.x and < 3.1.1 dialog
    {
        dialog_script.reset();
    }
    return kMGFErr_NoError;
}

MainGameFileError ReadScriptModules(std::vector<PScript> &sc_mods, Stream *in, GameDataVersion data_ver)
{
    if (data_ver >= kGameVersion_270) // 2.7.0+ script modules
    {
        int count = in->ReadInt32();
        sc_mods.resize(count);
        for (int i = 0; i < count; ++i)
        {
            sc_mods[i].reset(ccScript::CreateFromStream(in));
            if (sc_mods[i] == NULL)
                return kMGFErr_CreateScriptModuleFailed;
        }
    }
    else
    {
        sc_mods.resize(0);
    }
    return kMGFErr_NoError;
}

void ReadViewStruct272_Aligned(std::vector<ViewStruct272> &oldv, Stream *in, size_t count)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    oldv.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        oldv[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void ReadViews(GameSetupStruct &game, ViewStruct *&views, Stream *in, GameDataVersion data_ver)
{
    int count = game.numviews;
    views = (ViewStruct*)calloc(sizeof(ViewStruct) * count, 1);
    game.viewNames = (char**)malloc(sizeof(char*) * count);
    // NOTE: Only first index of viewNames actually allocs a string,
    // long enough to store all view names. Other slots store pointers
    // to substrings inside that large string.
    game.viewNames[0] = (char*)malloc(MAXVIEWNAMELENGTH * count);
    for (int i = 1; i < count; ++i)
    {
        game.viewNames[i] = game.viewNames[0] + (MAXVIEWNAMELENGTH * i);
    }

    if (data_ver > kGameVersion_272) // 3.x views
    {
        for (int i = 0; i < game.numviews; ++i)
        {
            views[i].ReadFromFile(in);
        }
    }
    else // 2.x views
    {
        std::vector<ViewStruct272> oldv;
        ReadViewStruct272_Aligned(oldv, in, count);
        Convert272ViewsToNew(oldv, views);
    }
}

void ReadDialogs(DialogTopic *&dialog,
                 std::vector< stdtr1compat::shared_ptr<unsigned char> > &old_dialog_scripts,
                 std::vector<String> &old_speech_lines,
                 Stream *in, GameDataVersion data_ver, int dlg_count)
{
    // TODO: I suspect +5 was a hacky way to "supress" memory access mistakes;
    // double check and remove if proved unnecessary
    dialog = (DialogTopic*)malloc(sizeof(DialogTopic) * dlg_count + 5);
    for (int i = 0; i < dlg_count; ++i)
    {
        dialog[i].ReadFromFile(in);
    }

    if (data_ver > kGameVersion_310)
        return;

    // read old dialog scripts
    old_dialog_scripts.resize(dlg_count);

    int i;
    for (int i = 0; i < dlg_count; i++)
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
            size_t newlen = in->ReadInt32();
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

MainGameFileError ReadPlugins(std::vector<PluginInfo> &infos, Stream *in)
{
    if (in->ReadInt32() != 1)
        return kMGFErr_PluginDataFmtNotSupported;

    int pl_count = in->ReadInt32();
    for (int i = 0; i < pl_count; ++i)
    {
        String name = String::FromStream(in);
        size_t datasize = in->ReadInt32();
        // just check for silly datasizes
        // TODO: replace with some constant, also why 10 MB limit?
        // and why is it different from the limit in editor?
        if (datasize > 10247680)
            return kMGFErr_PluginDataSizeTooLarge;

        PluginInfo info;
        info.Name = name;
        if (datasize > 0)
        {
            info.Data.reset(new char[datasize]);
            in->Read(info.Data.get(), datasize);
        }
        info.DataLen = datasize;
        infos.push_back(info);
    }
    return kMGFErr_NoError;
}

// Adjusts score clip id, depending on game data version
void AdjustScoreSound(GameSetupStruct &game, GameDataVersion data_ver)
{
    if (data_ver < kGameVersion_320)
    {
        game.scoreClipID = -1;
        if (game.options[OPT_SCORESOUND] > 0)
        {
            ScriptAudioClip* clip = GetAudioClipForOldStyleNumber(game, false, game.options[OPT_SCORESOUND]);
            if (clip)
                game.scoreClipID = clip->id;
            else
                game.scoreClipID = -1;
        }
    }
}

// Assigns default global message at given index
void SetDefaultGlmsg(GameSetupStruct &game, int msgnum, const char *val)
{
    // TODO: find out why the index should be lowered by 500
    // (or rather if we may pass correct index right away)
    msgnum -= 500;
    if (game.messages[msgnum] == NULL)
        game.messages[msgnum] = strdup(val);
}

// Sets up default global messages (these are used mainly in older games)
void SetDefaultGlobalMessages(GameSetupStruct &game)
{
    SetDefaultGlmsg(game, 983, "Sorry, not now.");
    SetDefaultGlmsg(game, 984, "Restore");
    SetDefaultGlmsg(game, 985, "Cancel");
    SetDefaultGlmsg(game, 986, "Select a game to restore:");
    SetDefaultGlmsg(game, 987, "Save");
    SetDefaultGlmsg(game, 988, "Type a name to save as:");
    SetDefaultGlmsg(game, 989, "Replace");
    SetDefaultGlmsg(game, 990, "The save directory is full. You must replace an existing game:");
    SetDefaultGlmsg(game, 991, "Replace:");
    SetDefaultGlmsg(game, 992, "With:");
    SetDefaultGlmsg(game, 993, "Quit");
    SetDefaultGlmsg(game, 994, "Play");
    SetDefaultGlmsg(game, 995, "Are you sure you want to quit?");
}

// Ensures that the game saves directory path is valid

MainGameFileError ReadGameData(LoadedGameEntities &ents, Stream *in, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;

    {
        AlignedStream align_s(in, Common::kAligned_Read);
        game.GameSetupStructBase::ReadFromFile(&align_s);
    }

    if (game.size.IsNull())
        return kMGFErr_InvalidNativeResolution;
    if (game.numfonts > MAX_FONTS)
        return kMGFErr_TooManyFonts;

    game.read_savegame_info(in, data_ver);
    game.read_font_flags(in);
    MainGameFileError err = game.read_sprite_flags(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    game.ReadInvInfo_Aligned(in);
    err = game.read_cursors(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    game.read_interaction_scripts(in, data_ver);
    game.read_words_dictionary(in);

    if (!game.load_compiled_script)
        return kMGFErr_NoGlobalScript;
    ents.GlobalScript.reset(ccScript::CreateFromStream(in));
    if (!ents.GlobalScript)
        return kMGFErr_CreateGlobalScriptFailed;
    err = ReadDialogScript(ents.DialogScript, in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    err = ReadScriptModules(ents.ScriptModules, in, data_ver);
    if (err != kMGFErr_NoError)
        return err;

    ReadViews(game, ents.Views, in, data_ver);

    if (data_ver <= kGameVersion_251)
    {
        // skip unknown data
        int count = in->ReadInt32();
        in->Seek(count * 0x204);
    }

    game.read_characters(in, data_ver);
    game.read_lipsync(in, data_ver);
    game.read_messages(in, data_ver);

    ReadDialogs(ents.Dialogs, ents.OldDialogScripts, ents.OldSpeechLines, in, data_ver, game.numdialog);
    read_gui(in, guis, &game);

    if (data_ver >= kGameVersion_260)
    {
        err = ReadPlugins(ents.PluginInfos, in);
        if (err != kMGFErr_NoError)
            return err;
    }

    err = game.read_customprops(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    err = game.read_audio(in, data_ver);
    if (err != kMGFErr_NoError)
        return err;
    game.read_room_names(in, data_ver);
    return kMGFErr_NoError;
}

MainGameFileError UpdateGameData(LoadedGameEntities &ents, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;
    AdjustScoreSound(game, data_ver);
    SetDefaultGlobalMessages(game);
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
    FixupSaveDirectory(game);
    return kMGFErr_NoError;
}

} // namespace Common
} // namespace AGS

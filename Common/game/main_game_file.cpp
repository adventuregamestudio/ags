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
#include "util/path.h"
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

String GetMainGameFileErrorText(MainGameFileErrorType err)
{
    switch (err)
    {
    case kMGFErr_NoError:
        return "No error.";
    case kMGFErr_FileNotFound:
        return "Main game file not found.";
    case kMGFErr_NoStream:
        return "Failed to open input stream.";
    case kMGFErr_SignatureFailed:
        return "Not an AGS main game file or unsupported format.";
    case kMGFErr_FormatVersionTooOld:
        return "Format version is too old; this engine can only run games made with AGS 3.4.1 or later";
    case kMGFErr_FormatVersionNotSupported:
        return "Format version not supported.";
    case kMGFErr_CapsNotSupported:
        return "Required engine caps are not supported.";
    case kMGFErr_InvalidNativeResolution:
        return "Unable to determine native game resolution.";
    case kMGFErr_TooManyFonts:
        return "Too many fonts for this engine to handle.";
    case kMGFErr_TooManySprites:
        return "Too many sprites for this engine to handle.";
    case kMGFErr_TooManyCursors:
        return "Too many cursors for this engine to handle.";
    case kMGFErr_InvalidPropertySchema:
        return "load room: unable to deserialize properties schema.";
    case kMGFErr_InvalidPropertyValues:
        return "Errors encountered when reading custom properties.";
    case kMGFErr_NoGlobalScript:
        return "No global script in game.";
    case kMGFErr_CreateGlobalScriptFailed:
        return String::FromFormat("Failed to load global script.");
    case kMGFErr_CreateDialogScriptFailed:
        return String::FromFormat("Failed to load dialog script.");
    case kMGFErr_CreateScriptModuleFailed:
        return String::FromFormat("Failed to load script module.");
    case kMGFErr_PluginDataFmtNotSupported:
        return "Format version of plugin data is not supported.";
    case kMGFErr_PluginDataSizeTooLarge:
        return "Plugin data size is too large.";
    }
    return "Unknown error.";
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

// Begins reading main game file from a generic stream
HGameFileError OpenMainGameFileBase(PStream &in, MainGameSource &src)
{
    if (!in)
        return HGameFileError::None();
    // Check data signature
    String data_sig = String::FromStreamCount(in.get(), MainGameSource::Signature.GetLength());
    if (data_sig.Compare(MainGameSource::Signature))
        return new MainGameFileError(kMGFErr_SignatureFailed);
    // Read data format version and requested engine version
    src.DataVersion = (GameDataVersion)in->ReadInt32();
    if (src.DataVersion < kGameVersion_341)
        return new MainGameFileError(kMGFErr_FormatVersionTooOld, String::FromFormat("Required format version: %d, supported %d - %d", src.DataVersion, kGameVersion_341, kGameVersion_Current));
    src.CompiledWith = StrUtil::ReadString(in.get());
    if (src.DataVersion > kGameVersion_Current)
        return new MainGameFileError(kMGFErr_FormatVersionNotSupported,
            String::FromFormat("Game was compiled with %s. Required format version: %d, supported %d - %d", src.CompiledWith.GetCStr(), src.DataVersion, kGameVersion_341, kGameVersion_Current));
    // Read required capabilities
    if (src.DataVersion >= kGameVersion_341)
    {
        size_t count = in->ReadInt32();
        for (size_t i = 0; i < count; ++i)
            src.Caps.insert(StrUtil::ReadString(in.get()));
    }
    // Everything is fine, return opened stream
    src.InputStream = in;
    // Remember loaded game data version
    // NOTE: this global variable is embedded in the code too much to get
    // rid of it too easily; the easy way is to set it whenever the main
    // game file is opened.
    loaded_game_file_version = src.DataVersion;
    return HGameFileError::None();
}

HGameFileError OpenMainGameFile(const String &filename, MainGameSource &src)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to open given file
    PStream in(File::OpenFileRead(filename));
    if (!in)
        return new MainGameFileError(kMGFErr_FileNotFound, String::FromFormat("Filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    return OpenMainGameFileBase(in, src);
}

HGameFileError OpenMainGameFileFromDefaultAsset(MainGameSource &src)
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
        return new MainGameFileError(kMGFErr_FileNotFound, String::FromFormat("Filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    return OpenMainGameFileBase(in, src);
}

HGameFileError ReadDialogScript(PScript &dialog_script, Stream *in, GameDataVersion data_ver)
{
    if (data_ver > kGameVersion_310) // 3.1.1+ dialog script
    {
        dialog_script.reset(ccScript::CreateFromStream(in));
        if (dialog_script == NULL)
            return new MainGameFileError(kMGFErr_CreateDialogScriptFailed, ccErrorString);
    }
    return HGameFileError::None();
}

HGameFileError ReadScriptModules(std::vector<PScript> &sc_mods, Stream *in, GameDataVersion data_ver)
{
    if (data_ver >= kGameVersion_270) // 2.7.0+ script modules
    {
        int count = in->ReadInt32();
        sc_mods.resize(count);
        for (int i = 0; i < count; ++i)
        {
            sc_mods[i].reset(ccScript::CreateFromStream(in));
            if (sc_mods[i] == NULL)
                return new MainGameFileError(kMGFErr_CreateScriptModuleFailed, ccErrorString);
        }
    }
    else
    {
        sc_mods.resize(0);
    }
    return HGameFileError::None();
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
        for (int i = 0; i < game.numviews; ++i)
        {
            views[i].ReadFromFile(in);
        }
}

// CLNUP remove the old dialogs reading stuff when possible
void ReadDialogs(DialogTopic *&dialog, Stream *in, GameDataVersion data_ver, int dlg_count)
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

    for (int i = 0; i < dlg_count; ++i)
    {
        // NOTE: originally this was read into dialog[i].optionscripts
        in->Seek(dialog[i].codesize);// CLNUP old dialog

        // Encrypted text script
        int script_text_len = in->ReadInt32();
        in->Seek(script_text_len);// CLNUP was read and decrypted into old_dialog_src[i]
        }

    // Read the dialog lines
    //
    // TODO: investigate this: these strings were read much simplier in the editor, see code:
    /*
        char stringbuffer[1000];
        for (bb=0;bb<thisgame.numdlgmessage;bb++) {
            if ((filever >= 26) && (encrypted))
                read_string_decrypt(iii, stringbuffer);
            else
                fgetstring(stringbuffer, iii);
        }
    */
    int i = 0;
    char buffer[1000];

        // Encrypted text on > 2.60
        while (1)
        {
            size_t newlen = in->ReadInt32();
            if (static_cast<int32_t>(newlen) == 0xCAFEBEEF)  // GUI magic
            {
                in->Seek(-4);
                break;
            }

            newlen = Math::Min(newlen, sizeof(buffer) - 1);
        in->Seek(newlen);// CLNUP old_speech_lines
            i++;
        }
}

HGameFileError ReadPlugins(std::vector<PluginInfo> &infos, Stream *in)
{
    int fmt_ver = in->ReadInt32();
    if (fmt_ver != 1)
        return new MainGameFileError(kMGFErr_PluginDataFmtNotSupported, String::FromFormat("Version: %d, supported: %d", fmt_ver, 1));

    int pl_count = in->ReadInt32();
    for (int i = 0; i < pl_count; ++i)
    {
        String name = String::FromStream(in);
        size_t datasize = in->ReadInt32();
        // just check for silly datasizes
        if (datasize > PLUGIN_SAVEBUFFERSIZE)
            return new MainGameFileError(kMGFErr_PluginDataSizeTooLarge, String::FromFormat("Required: %u, max: %u", datasize, PLUGIN_SAVEBUFFERSIZE));

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
    return HGameFileError::None();
}

// Create the missing audioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
void BuildAudioClipArray(GameSetupStruct &game, const AssetLibInfo &lib)
{
    char temp_name[30];
    int temp_number;
    char temp_extension[10];

    // TODO: this was done to simplify code transition; ideally we should be
    // working with GameSetupStruct's getters and setters here
    int &audioClipCount            = game.audioClipCount;
    ScriptAudioClip *&audioClips   = game.audioClips;

    size_t number_of_files = lib.AssetInfos.size();
    for (size_t i = 0; i < number_of_files; ++i)
    {
        if (sscanf(lib.AssetInfos[i].FileName, "%5s%d.%3s", temp_name, &temp_number, temp_extension) == 3)
        {
            if (stricmp(temp_name, "music") == 0)
            {
                sprintf(audioClips[audioClipCount].scriptName, "aMusic%d", temp_number);
                sprintf(audioClips[audioClipCount].fileName, "music%d.%s", temp_number, temp_extension);
                audioClips[audioClipCount].bundlingType = (stricmp(temp_extension, "mid") == 0) ? AUCL_BUNDLE_EXE : AUCL_BUNDLE_VOX;
                audioClips[audioClipCount].type = 2;
                audioClips[audioClipCount].defaultRepeat = 1;
            }
            else if (stricmp(temp_name, "sound") == 0)
            {
                sprintf(audioClips[audioClipCount].scriptName, "aSound%d", temp_number);
                sprintf(audioClips[audioClipCount].fileName, "sound%d.%s", temp_number, temp_extension);
                audioClips[audioClipCount].bundlingType = AUCL_BUNDLE_EXE;
                audioClips[audioClipCount].type = 3;
            }
            else
            {
                continue;
            }

            audioClips[audioClipCount].defaultVolume = 100;
            audioClips[audioClipCount].defaultPriority = 50;
            audioClips[audioClipCount].id = audioClipCount;

            if (stricmp(temp_extension, "mp3") == 0)
                audioClips[audioClipCount].fileType = eAudioFileMP3;
            else if (stricmp(temp_extension, "wav") == 0)
                audioClips[audioClipCount].fileType = eAudioFileWAV;
            else if (stricmp(temp_extension, "voc") == 0)
                audioClips[audioClipCount].fileType = eAudioFileVOC;
            else if (stricmp(temp_extension, "mid") == 0)
                audioClips[audioClipCount].fileType = eAudioFileMIDI;
            else if ((stricmp(temp_extension, "mod") == 0) || (stricmp(temp_extension, "xm") == 0)
                || (stricmp(temp_extension, "s3m") == 0) || (stricmp(temp_extension, "it") == 0))
                audioClips[audioClipCount].fileType = eAudioFileMOD;
            else if (stricmp(temp_extension, "ogg") == 0)
                audioClips[audioClipCount].fileType = eAudioFileOGG;

            audioClipCount++;
        }
    }
}

// Convert audio data to the current version
void UpgradeAudio(GameSetupStruct &game, GameDataVersion data_ver)
{
    if (data_ver >= kGameVersion_320)
        return;
}

// Convert character data to the current version
void UpgradeCharacters(GameSetupStruct &game, GameDataVersion data_ver)
{

}

void UpgradeMouseCursors(GameSetupStruct &game, GameDataVersion data_ver)
{

}

// Adjusts score clip id, depending on game data version
void AdjustScoreSound(GameSetupStruct &game, GameDataVersion data_ver)
{

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

// CLNUP not to remove yet, global messages may be deprecated but these are used by the engine
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
    SetDefaultGlmsg(game, 996, "You are carrying nothing.");
}

void FixupSaveDirectory(GameSetupStruct &game)
{
    // If the save game folder was not specified by game author, create one of
    // the game name, game GUID, or uniqueid, as a last resort
    if (!game.saveGameFolderName[0])
    {
        if (game.gamename[0])
            snprintf(game.saveGameFolderName, MAX_SG_FOLDER_LEN, "%s", game.gamename);
        else if (game.guid[0])
            snprintf(game.saveGameFolderName, MAX_SG_FOLDER_LEN, "%s", game.guid);
        else
            snprintf(game.saveGameFolderName, MAX_SG_FOLDER_LEN, "AGS-Game-%d", game.uniqueid);
    }
    // Lastly, fixup folder name by removing any illegal characters
    String s = Path::FixupSharedFilename(game.saveGameFolderName);
    snprintf(game.saveGameFolderName, MAX_SG_FOLDER_LEN, "%s", s.GetCStr());
}

HGameFileError ReadGameData(LoadedGameEntities &ents, Stream *in, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;

    {
        AlignedStream align_s(in, Common::kAligned_Read);
        game.GameSetupStructBase::ReadFromFile(&align_s);
    }

    if (game.size.IsNull())
        return new MainGameFileError(kMGFErr_InvalidNativeResolution);
    if (game.numfonts > MAX_FONTS)
        return new MainGameFileError(kMGFErr_TooManyFonts, String::FromFormat("Count: %d, max: %d", game.numfonts, MAX_FONTS));

    game.read_savegame_info(in, data_ver);
    game.read_font_flags(in, data_ver);
    HGameFileError err = game.read_sprite_flags(in, data_ver);
    if (!err)
        return err;
    game.ReadInvInfo_Aligned(in);
    err = game.read_cursors(in, data_ver);
    if (!err)
        return err;
    game.read_interaction_scripts(in, data_ver);
    game.read_words_dictionary(in);

    if (!game.load_compiled_script)
        return new MainGameFileError(kMGFErr_NoGlobalScript);
    ents.GlobalScript.reset(ccScript::CreateFromStream(in));
    if (!ents.GlobalScript)
        return new MainGameFileError(kMGFErr_CreateGlobalScriptFailed, ccErrorString);
    err = ReadDialogScript(ents.DialogScript, in, data_ver);
    if (!err)
        return err;
    err = ReadScriptModules(ents.ScriptModules, in, data_ver);
    if (!err)
        return err;

    ReadViews(game, ents.Views, in, data_ver);

    game.read_characters(in, data_ver);
    game.read_lipsync(in, data_ver);
    game.read_messages(in, data_ver);

    ReadDialogs(ents.Dialogs, in, data_ver, game.numdialog);
    GUI::ReadGUI(guis, in);
    game.numgui = guis.size();

    if (data_ver >= kGameVersion_260)
    {
        err = ReadPlugins(ents.PluginInfos, in);
        if (!err)
            return err;
    }

    err = game.read_customprops(in, data_ver);
    if (!err)
        return err;
    err = game.read_audio(in, data_ver);
    if (!err)
        return err;
    game.read_room_names(in, data_ver);
    return err;
}

HGameFileError UpdateGameData(LoadedGameEntities &ents, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;
    UpgradeAudio(game, data_ver);
    AdjustScoreSound(game, data_ver);
    UpgradeCharacters(game, data_ver);
    UpgradeMouseCursors(game, data_ver);
    SetDefaultGlobalMessages(game);
    // Old dialog options API for pre-3.4.0.2 games
    if (data_ver < kGameVersion_340_2)
        game.options[OPT_DIALOGOPTIONSAPI] = -1;
    FixupSaveDirectory(game);
    return HGameFileError::None();
}

} // namespace Common
} // namespace AGS

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <cstdio>
#include "ac/audiocliptype.h"
#include "ac/dialogtopic.h"
#include "ac/gamesetupstruct.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "core/asset.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/main_game_file.h"
#include "gui/guibutton.h"
#include "gui/guilabel.h"
#include "gui/guimain.h"
#include "script/cc_common.h"
#include "util/data_ext.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/string_utils.h"
#include "font/fonts.h"

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
    case kMGFErr_FileOpenFailed:
        return "Main game file not found or could not be opened.";
    case kMGFErr_SignatureFailed:
        return "Not an AGS main game file or unsupported format.";
    case kMGFErr_FormatVersionNotSupported:
        return "Format version not supported.";
    case kMGFErr_CapsNotSupported:
        return "The game requires extended capabilities which aren't supported by the engine.";
    case kMGFErr_InvalidNativeResolution:
        return "Unable to determine native game resolution.";
    case kMGFErr_TooManySprites:
        return "Too many sprites for this engine to handle.";
    case kMGFErr_InvalidPropertySchema:
        return "Failed to deserialize custom properties schema.";
    case kMGFErr_InvalidPropertyValues:
        return "Errors encountered when reading custom properties.";
    case kMGFErr_CreateGlobalScriptFailed:
        return "Failed to load global script.";
    case kMGFErr_CreateDialogScriptFailed:
        return "Failed to load dialog script.";
    case kMGFErr_CreateScriptModuleFailed:
        return "Failed to load script module.";
    case kMGFErr_GameEntityFailed:
        return "Failed to load one or more game entities.";
    case kMGFErr_PluginDataFmtNotSupported:
        return "Format version of plugin data is not supported.";
    case kMGFErr_PluginDataSizeTooLarge:
        return "Plugin data size is too large.";
    case kMGFErr_ExtListFailed:
        return "There was error reading game data extensions.";
    case kMGFErr_ExtUnknown:
        return "Unknown extension.";
    }
    return "Unknown error.";
}

LoadedGameEntities::LoadedGameEntities(GameSetupStruct &game)
    : Game(game)
    , SpriteCount(0)
{
}

LoadedGameEntities::~LoadedGameEntities() = default;

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

// Scans given directory for game data libraries, returns first found or none.
// Uses fn_testfile callback to test the file.
// Tracks files with standard AGS package names:
// - *.ags is a standart cross-platform file pattern for AGS games,
// - ac2game.dat is a legacy file name for very old games,
// - *.exe is a MS Win executable; it is included to this case because
//   users often run AGS ports with Windows versions of games.
String FindGameData(const String &path, std::function<bool(const String&)> fn_testfile)
{
    String test_file;
    Debug::Printf("Searching for game data in: %s", path.GetCStr());
    for (FindFile ff = FindFile::OpenFiles(path); !ff.AtEnd(); ff.Next())
    {
        test_file = ff.Current();
        if (test_file.CompareRightNoCase(".ags") == 0 ||
            test_file.CompareNoCase("ac2game.dat") == 0 ||
            test_file.CompareRightNoCase(".exe") == 0)
        {
            test_file = Path::ConcatPaths(path, test_file);
            if (fn_testfile(test_file))
            {
                Debug::Printf("Found game data pak: %s", test_file.GetCStr());
                return test_file;
            }
        }
    }
    return "";
}

String FindGameData(const String &path)
{
    return FindGameData(path, IsMainGameLibrary);
}

// Begins reading main game file from a generic stream
static HGameFileError OpenMainGameFileBase(Stream *in, MainGameSource &src)
{
    // Check data signature
    String data_sig = String::FromStreamCount(in, MainGameSource::Signature.GetLength());
    if (data_sig.Compare(MainGameSource::Signature))
        return new MainGameFileError(kMGFErr_SignatureFailed);
    // Read data format version and requested engine version
    src.DataVersion = (GameDataVersion)in->ReadInt32();
    src.CompiledWith = StrUtil::ReadString(in);
    if (src.DataVersion < kGameVersion_LowSupported || src.DataVersion > kGameVersion_Current)
        return new MainGameFileError(kMGFErr_FormatVersionNotSupported,
            String::FromFormat("Game was compiled with %s. Required format version: %d, supported %d - %d", src.CompiledWith.GetCStr(), src.DataVersion, kGameVersion_LowSupported, kGameVersion_Current));
    // Read required capabilities
    size_t count = in->ReadInt32();
    for (size_t i = 0; i < count; ++i)
        src.Caps.insert(StrUtil::ReadString(in));
    // Remember loaded game data version
    // NOTE: this global variable is embedded in the code too much to get
    // rid of it too easily; the easy way is to set it whenever the main
    // game file is opened.
    loaded_game_file_version = src.DataVersion;
    game_compiled_version.SetFromString(src.CompiledWith);
    return HGameFileError::None();
}

HGameFileError OpenMainGameFile(const String &filename, MainGameSource &src)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to open given file
    Stream *in = File::OpenFileRead(filename);
    if (!in)
        return new MainGameFileError(kMGFErr_FileOpenFailed, String::FromFormat("Tried filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    src.InputStream.reset(in);
    return OpenMainGameFileBase(in, src);
}

HGameFileError OpenMainGameFileFromDefaultAsset(MainGameSource &src, AssetManager *mgr)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to find and open main game file
    String filename = MainGameSource::DefaultFilename_v3;
    Stream *in = mgr->OpenAsset(filename);
    if (!in)
    {
        filename = MainGameSource::DefaultFilename_v2;
        in = mgr->OpenAsset(filename);
    }
    if (!in)
        return new MainGameFileError(kMGFErr_FileOpenFailed, String::FromFormat("Tried filenames: %s, %s.",
            MainGameSource::DefaultFilename_v3.GetCStr(), MainGameSource::DefaultFilename_v2.GetCStr()));
    src.Filename = filename;
    src.InputStream.reset(in);
    return OpenMainGameFileBase(in, src);
}

HGameFileError ReadDialogScript(PScript &dialog_script, Stream *in, GameDataVersion data_ver)
{
    dialog_script.reset(ccScript::CreateFromStream(in));
    if (dialog_script == nullptr)
        return new MainGameFileError(kMGFErr_CreateDialogScriptFailed, cc_get_error().ErrorString);
    return HGameFileError::None();
}

HGameFileError ReadScriptModules(std::vector<PScript> &sc_mods, Stream *in, GameDataVersion data_ver)
{
    int count = in->ReadInt32();
    sc_mods.resize(count);
    for (int i = 0; i < count; ++i)
    {
        sc_mods[i].reset(ccScript::CreateFromStream(in));
        if (sc_mods[i] == nullptr)
            return new MainGameFileError(kMGFErr_CreateScriptModuleFailed, cc_get_error().ErrorString);
    }
    return HGameFileError::None();
}

void ReadViews(GameSetupStruct &game, std::vector<ViewStruct> &views, Stream *in, GameDataVersion data_ver)
{
    views.resize(game.numviews);
        for (int i = 0; i < game.numviews; ++i)
        {
            views[i].ReadFromFile(in);
        }
}

void ReadDialogs(std::vector<DialogTopic> &dialog, Stream *in, GameDataVersion data_ver, int dlg_count)
{
    dialog.resize(dlg_count);
    for (int i = 0; i < dlg_count; ++i)
    {
        dialog[i].ReadFromFile(in);
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
            return new MainGameFileError(kMGFErr_PluginDataSizeTooLarge, String::FromFormat("Required: %zu, max: %zu", datasize, (size_t)PLUGIN_SAVEBUFFERSIZE));

        PluginInfo info;
        info.Name = name;
        if (datasize > 0)
        {
            info.Data.resize(datasize);
            in->Read(&info.Data.front(), datasize);
        }
        infos.push_back(info);
    }
    return HGameFileError::None();
}

// Create the missing audioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
void BuildAudioClipArray(const std::vector<String> &assets, std::vector<ScriptAudioClip> &audioclips)
{
    char temp_name[30];
    int temp_number;
    char temp_extension[10];

    // FIXME: use audio type constants instead of obscure numeric literals
    for (const String &asset : assets)
    {
        if (sscanf(asset.GetCStr(), "%5s%d.%3s", temp_name, &temp_number, temp_extension) != 3)
            continue;

        ScriptAudioClip clip;
        if (ags_stricmp(temp_extension, "mp3") == 0)
            clip.fileType = eAudioFileMP3;
        else if (ags_stricmp(temp_extension, "wav") == 0)
            clip.fileType = eAudioFileWAV;
        else if (ags_stricmp(temp_extension, "voc") == 0)
            clip.fileType = eAudioFileVOC;
        else if (ags_stricmp(temp_extension, "mid") == 0)
            clip.fileType = eAudioFileMIDI;
        else if ((ags_stricmp(temp_extension, "mod") == 0) || (ags_stricmp(temp_extension, "xm") == 0)
            || (ags_stricmp(temp_extension, "s3m") == 0) || (ags_stricmp(temp_extension, "it") == 0))
            clip.fileType = eAudioFileMOD;
        else if (ags_stricmp(temp_extension, "ogg") == 0)
            clip.fileType = eAudioFileOGG;
        else if (ags_stricmp(temp_extension, "flac") == 0)
            clip.fileType = eAudioFileFLAC;
        else
            continue;

        if (ags_stricmp(temp_name, "music") == 0)
        {
            clip.scriptName.Format("aMusic%d", temp_number);
            clip.fileName.Format("music%d.%s", temp_number, temp_extension);
            clip.bundlingType = (ags_stricmp(temp_extension, "mid") == 0) ? AUCL_BUNDLE_EXE : AUCL_BUNDLE_VOX;
            clip.type = 2;
            clip.defaultRepeat = 1;
        }
        else if (ags_stricmp(temp_name, "sound") == 0)
        {
            clip.scriptName.Format("aSound%d", temp_number);
            clip.fileName.Format("sound%d.%s", temp_number, temp_extension);
            clip.bundlingType = AUCL_BUNDLE_EXE;
            clip.type = 3;
            clip.defaultRepeat = 0;
        }
        else
        {
            continue;
        }

        clip.defaultVolume = 100;
        clip.defaultPriority = 50;
        clip.id = audioclips.size();
        audioclips.push_back(clip);
    }
}

void ApplySpriteData(GameSetupStruct &game, const LoadedGameEntities &ents, GameDataVersion data_ver)
{
    if (ents.SpriteCount == 0)
        return;

    // Apply sprite flags read from original format (sequential array)
    spriteset.EnlargeTo(ents.SpriteCount - 1);
    for (size_t i = 0; i < ents.SpriteCount; ++i)
    {
        game.SpriteInfos[i].Flags = ents.SpriteFlags[i];
    }
}

void UpgradeFonts(GameSetupStruct &game, GameDataVersion data_ver)
{
}

// Convert audio data to the current version
void UpgradeAudio(GameSetupStruct &game, LoadedGameEntities &ents, GameDataVersion data_ver)
{
}

// Convert character data to the current version
void UpgradeCharacters(GameSetupStruct &game, GameDataVersion data_ver)
{
}

void UpgradeGUI(GameSetupStruct &game, GameDataVersion data_ver)
{
    // Previously, Buttons and Labels had a fixed Translated behavior
    if (data_ver < kGameVersion_361)
    {
        for (auto &btn : guibuts)
            btn.SetTranslated(true); // always translated
        for (auto &lbl : guilabels)
            lbl.SetTranslated(true); // always translated
    }
}

void UpgradeMouseCursors(GameSetupStruct &game, GameDataVersion data_ver)
{
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

HGameFileError ReadSpriteFlags(LoadedGameEntities &ents, Stream *in, GameDataVersion data_ver)
{
    size_t sprcount = in->ReadInt32();
    if (sprcount > (size_t)SpriteCache::MAX_SPRITE_INDEX + 1)
        return new MainGameFileError(kMGFErr_TooManySprites, String::FromFormat("Count: %zu, max: %zu", sprcount, (size_t)SpriteCache::MAX_SPRITE_INDEX + 1));

    ents.SpriteCount = sprcount;
    ents.SpriteFlags.resize(sprcount);
    in->Read(ents.SpriteFlags.data(), sprcount);
    return HGameFileError::None();
}


// GameDataExtReader reads main game data's extension blocks
class GameDataExtReader : public DataExtReader
{
public:
    GameDataExtReader(LoadedGameEntities &ents, GameDataVersion data_ver, Stream *in)
        : DataExtReader(in, kDataExt_NumID8 | kDataExt_File64)
        , _ents(ents)
        , _dataVer(data_ver)
    {}

protected:
    HError ReadBlock(int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override;

    LoadedGameEntities &_ents;
    GameDataVersion _dataVer {};
};

HError GameDataExtReader::ReadBlock(int /*block_id*/, const String &ext_id,
    soff_t /*block_len*/, bool &read_next)
{
    read_next = true;
    // Add extensions here checking ext_id, which is an up to 16-chars name, for example:
    // if (ext_id.CompareNoCase("GUI_NEWPROPS") == 0)
    // {
    //     // read new gui properties
    // }
    if (ext_id.CompareNoCase("v360_fonts") == 0)
    {
        for (int i = 0; i < _ents.Game.numfonts; ++i)
        {
            // adjustable font outlines
            _ents.Game.fonts[i].AutoOutlineThickness = _in->ReadInt32();
            _ents.Game.fonts[i].AutoOutlineStyle =
                static_cast<enum FontInfo::AutoOutlineStyle>(_in->ReadInt32());
            // reserved
            _in->ReadInt32();
            _in->ReadInt32();
            _in->ReadInt32();
            _in->ReadInt32();
        }
        return HError::None();
    }
    else if (ext_id.CompareNoCase("v360_cursors") == 0)
    {
        for (int i = 0; i < _ents.Game.numcursors; ++i)
        {
            _ents.Game.mcurs[i].animdelay = _in->ReadInt32();
            // reserved
            _in->ReadInt32();
            _in->ReadInt32();
            _in->ReadInt32();
        }
        return HError::None();
    }
    // Early development version of "ags4"
    if (ext_id.CompareNoCase("ext_ags399") == 0)
    {
        // new character properties
        for (size_t i = 0; i < (size_t)_ents.Game.numcharacters; ++i)
        {
            _ents.CharEx[i].BlendMode = (BlendMode)_in->ReadInt32();
            // Reserved for colour options
            _in->Seek(sizeof(int32_t) * 3); // flags + tint rgbs + light level
            // Reserved for transform options (see brief list in savegame format)
            _in->Seek(sizeof(int32_t) * 11);
        }

        // new gui properties
        for (size_t i = 0; i < guis.size(); ++i)
        {
            guis[i].BlendMode = (BlendMode)_in->ReadInt32();
            // Reserved for colour options
            _in->Seek(sizeof(int32_t) * 3); // flags + tint rgbs + light level
            // Reserved for transform options (see list in savegame format)
            _in->Seek(sizeof(int32_t) * 11);
        }

        return HError::None();
    }
    return new MainGameFileError(kMGFErr_ExtUnknown, String::FromFormat("Type: %s", ext_id.GetCStr()));
}


HGameFileError ReadGameData(LoadedGameEntities &ents, Stream *in, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;

    //-------------------------------------------------------------------------
    // The classic data section.
    //-------------------------------------------------------------------------
    GameSetupStruct::SerializeInfo sinfo;
    game.GameSetupStructBase::ReadFromFile(in, data_ver, sinfo);

    Debug::Printf(kDbgMsg_Info, "Game title: '%s'", game.gamename);
    Debug::Printf(kDbgMsg_Info, "Game uid (old format): `%d`", game.uniqueid);
    Debug::Printf(kDbgMsg_Info, "Game guid: '%s'", game.guid);

    if (game.GetGameRes().IsNull())
        return new MainGameFileError(kMGFErr_InvalidNativeResolution);

    game.read_savegame_info(in, data_ver);
    game.read_font_infos(in, data_ver);
    HGameFileError err = ReadSpriteFlags(ents, in, data_ver);
    if (!err)
        return err;
    game.ReadInvInfo(in);
    err = game.read_cursors(in);
    if (!err)
        return err;
    game.read_interaction_scripts(in, data_ver);
    if (sinfo.HasWordsDict)
        game.read_words_dictionary(in);

    if (sinfo.HasCCScript)
    {
        ents.GlobalScript.reset(ccScript::CreateFromStream(in));
        if (!ents.GlobalScript)
            return new MainGameFileError(kMGFErr_CreateGlobalScriptFailed, cc_get_error().ErrorString);
        err = ReadDialogScript(ents.DialogScript, in, data_ver);
        if (!err)
            return err;
        err = ReadScriptModules(ents.ScriptModules, in, data_ver);
        if (!err)
            return err;
    }

    ReadViews(game, ents.Views, in, data_ver);

    game.read_characters(in);
    game.read_lipsync(in, data_ver);
    game.skip_messages(in, sinfo.HasMessages, data_ver);

    ReadDialogs(ents.Dialogs, in, data_ver, game.numdialog);
    HError err2 = GUI::ReadGUI(in);
    if (!err2)
        return new MainGameFileError(kMGFErr_GameEntityFailed, err2);
    game.numgui = guis.size();

    err = ReadPlugins(ents.PluginInfos, in);
    if (!err)
        return err;

    err = game.read_customprops(in, data_ver);
    if (!err)
        return err;
    err = game.read_audio(in, data_ver);
    if (!err)
        return err;
    game.read_room_names(in, data_ver);

    ents.CharEx.resize(game.numcharacters);
    //-------------------------------------------------------------------------
    // All the extended data, for AGS > 3.5.0.
    //-------------------------------------------------------------------------
    GameDataExtReader reader(ents, data_ver, in);
    HError ext_err = reader.Read();
    return ext_err ? HGameFileError::None() : new MainGameFileError(kMGFErr_ExtListFailed, ext_err);
}

HGameFileError UpdateGameData(LoadedGameEntities &ents, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;
    ApplySpriteData(game, ents, data_ver);
    UpgradeFonts(game, data_ver);
    UpgradeAudio(game, ents, data_ver);
    UpgradeCharacters(game, data_ver);
    UpgradeGUI(game, data_ver);
    UpgradeMouseCursors(game, data_ver);
    FixupSaveDirectory(game);
    return HGameFileError::None();
}

void PreReadGameData(GameSetupStruct &game, Stream *in, GameDataVersion data_ver)
{
    GameSetupStruct::SerializeInfo sinfo;
    game.ReadFromFile(in, data_ver, sinfo);
    game.read_savegame_info(in, data_ver);
}

} // namespace Common
} // namespace AGS

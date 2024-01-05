//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// This unit provides functions for reading main game file into appropriate
// data structures. Main game file contains general game data, such as global
// options, lists of static game entities and compiled scripts modules.
//
//=============================================================================

#ifndef __AGS_CN_GAME__MAINGAMEFILE_H
#define __AGS_CN_GAME__MAINGAMEFILE_H

#include <memory>
#include <set>
#include <vector>
#include "ac/game_version.h"
#include "game/plugininfo.h"
#include "script/cc_script.h"
#include "util/error.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/version.h"

struct GameSetupStruct;
struct DialogTopic;
struct ViewStruct;

namespace AGS
{
namespace Common
{

// Error codes for main game file reading
enum MainGameFileErrorType
{
    kMGFErr_NoError,
    kMGFErr_FileOpenFailed,
    kMGFErr_SignatureFailed,
    // separate error given for "too old" format to provide clarifying message
    kMGFErr_FormatVersionTooOld,
    kMGFErr_FormatVersionNotSupported,
    kMGFErr_CapsNotSupported,
    kMGFErr_InvalidNativeResolution,
    kMGFErr_TooManySprites,
    kMGFErr_InvalidPropertySchema,
    kMGFErr_InvalidPropertyValues,
    kMGFErr_CreateGlobalScriptFailed,
    kMGFErr_CreateDialogScriptFailed,
    kMGFErr_CreateScriptModuleFailed,
    kMGFErr_GameEntityFailed,
    kMGFErr_PluginDataFmtNotSupported,
    kMGFErr_PluginDataSizeTooLarge,
    kMGFErr_ExtListFailed,
    kMGFErr_ExtUnknown,
};

String GetMainGameFileErrorText(MainGameFileErrorType err);

typedef TypedCodeError<MainGameFileErrorType, GetMainGameFileErrorText> MainGameFileError;
typedef ErrorHandle<MainGameFileError> HGameFileError;
typedef std::unique_ptr<Stream> UStream;

// MainGameSource defines a successfully opened main game file
struct MainGameSource
{
    // Standart main game file names for 3.* and 2.* games respectively
    static const String DefaultFilename_v3;
    static const String DefaultFilename_v2;
    // Signature of the current game format
    static const String Signature;

    // Name of the asset file
    String              Filename;
    // Game file format version
    GameDataVersion     DataVersion;
    // Tool identifier (like version) this game was compiled with
    String              CompiledWith;
    // Extended engine capabilities required by the game; their primary use
    // currently is to let "alternate" game formats indicate themselves
    std::set<String>    Caps;
    // A ponter to the opened stream
    UStream             InputStream;

    MainGameSource();
};

// LoadedGameEntities is meant for keeping objects loaded from the game file.
// Because copying/assignment methods are not properly implemented for some
// of these objects yet, they have to be attached using references to be read
// directly. This is temporary solution that has to be resolved by the future
// code refactoring.
struct LoadedGameEntities
{
    GameSetupStruct        &Game;
    std::vector<DialogTopic> Dialogs;
    std::vector<ViewStruct> Views;
    PScript                 GlobalScript;
    PScript                 DialogScript;
    std::vector<PScript>    ScriptModules;
    std::vector<PluginInfo> PluginInfos;

    // Original sprite data (when it was read into const-sized arrays)
    size_t                  SpriteCount;
    std::vector<uint8_t>    SpriteFlags; // SPF_* flags

    // Old dialog support
    // legacy compiled dialog script of its own format,
    // requires separate interpreting
    std::vector<std::vector<uint8_t>> OldDialogScripts;
    // probably, actual dialog script sources kept within some older games
    std::vector<String>     OldDialogSources;
    // speech texts displayed during dialog
    std::vector<String>     OldSpeechLines;

    LoadedGameEntities(GameSetupStruct &game);
    ~LoadedGameEntities();
};

class AssetManager;

// Tells if the given path (library filename) contains main game file
bool               IsMainGameLibrary(const String &filename);
// Scans given directory path for a package containing main game data, returns first found or none.
String             FindGameData(const String &path);
// Scans given directory path for a package containing main game data,
// tests each one using provided callback, returns first found or none.
String             FindGameData(const String &path, std::function<bool(const String&)> fn_testfile);
// Opens main game file for reading from an arbitrary file
HGameFileError     OpenMainGameFile(const String &filename, MainGameSource &src);
// Opens main game file for reading using the current Asset Manager (uses default asset name)
HGameFileError     OpenMainGameFileFromDefaultAsset(MainGameSource &src, AssetManager *mgr);
// Reads game data, applies necessary conversions to match current format version
HGameFileError     ReadGameData(LoadedGameEntities &ents, Stream *in, GameDataVersion data_ver);
// Pre-reads the heading game data, just enough to identify the game and its special file locations
void               PreReadGameData(GameSetupStruct &game, Stream *in, GameDataVersion data_ver);
// Applies necessary updates, conversions and fixups to the loaded data
// making it compatible with current engine
HGameFileError     UpdateGameData(LoadedGameEntities &ents, GameDataVersion data_ver);
// Ensures that the game saves directory path is valid
void               FixupSaveDirectory(GameSetupStruct &game);
// Scans the Asset libraries for the old-style music and sound files and generate
// new-style audio clip array for the game
void               ScanOldStyleAudio(AssetManager *asset_mgr, GameSetupStruct &game, std::vector<ViewStruct> &views, GameDataVersion data_ver);
// Maps legacy sound numbers to real audio clips
void               RemapLegacySoundNums(GameSetupStruct &game, std::vector<ViewStruct> &views, GameDataVersion data_ver);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__MAINGAMEFILE_H

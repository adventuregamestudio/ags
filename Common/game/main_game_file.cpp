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
#include "util/file.h"
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
static HGameFileError OpenMainGameFileBase(MainGameSource &src)
{
    Stream *in = src.InputStream.get();
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
    auto in = File::OpenFileRead(filename);
    if (!in)
        return new MainGameFileError(kMGFErr_FileOpenFailed, String::FromFormat("Tried filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    src.InputStream = std::move(in);
    return OpenMainGameFileBase(src);
}

HGameFileError OpenMainGameFileFromDefaultAsset(MainGameSource &src, AssetManager *mgr)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to find and open main game file
    String filename = MainGameSource::DefaultFilename_v3;
    auto in = mgr->OpenAsset(filename);
    if (!in)
    {
        filename = MainGameSource::DefaultFilename_v2;
        in = mgr->OpenAsset(filename);
    }
    if (!in)
        return new MainGameFileError(kMGFErr_FileOpenFailed, String::FromFormat("Tried filenames: %s, %s.",
            MainGameSource::DefaultFilename_v3.GetCStr(), MainGameSource::DefaultFilename_v2.GetCStr()));
    src.Filename = filename;
    src.InputStream = std::move(in);
    return OpenMainGameFileBase(src);
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

void ApplySpriteData(GameSetupStruct &game, const LoadedGameEntities &ents, GameDataVersion data_ver)
{
    if (ents.SpriteCount == 0)
        return;

    // Apply sprite flags read from original format (sequential array)
    game.SpriteInfos.resize(ents.SpriteCount);
    for (size_t i = 0; i < ents.SpriteCount; ++i)
    {
        game.SpriteInfos[i].Flags = ents.SpriteFlags[i];
    }
}

// Lookup table for scaling 5 bit colors up to 8 bits,
// copied from Allegro 4 library, preventing an extra dependency.
static const uint8_t RGBScale5[32]
{
    0,   8,   16,  24,  33,  41,  49,  57,
    66,  74,  82,  90,  99,  107, 115, 123,
    132, 140, 148, 156, 165, 173, 181, 189,
    198, 206, 214, 222, 231, 239, 247, 255
};

// Lookup table for scaling 6 bit colors up to 8 bits,
// copied from Allegro 4 library, preventing an extra dependency.
static const uint8_t RGBScale6[64]
{
    0,   4,   8,   12,  16,  20,  24,  28,
    32,  36,  40,  44,  48,  52,  56,  60,
    65,  69,  73,  77,  81,  85,  89,  93,
    97,  101, 105, 109, 113, 117, 121, 125,
    130, 134, 138, 142, 146, 150, 154, 158,
    162, 166, 170, 174, 178, 182, 186, 190,
    195, 199, 203, 207, 211, 215, 219, 223,
    227, 231, 235, 239, 243, 247, 251, 255
};

// Remaps color number from legacy to new format:
// * palette index in 8-bit game,
// * encoded 32-bit A8R8G8B8 in 32-bit game.
static int RemapFromLegacyColourNumber(const GameSetupStruct &game, int color)
{
    if (game.color_depth == 1)
        return color; // keep palette index

    // Special 0-31 color numbers were always interpreted as palette indexes;
    // for them we compose a 32-bit xRGB from the palette entry
    if (color >= 0 && color < 32)
    {
        const RGB &rgb = game.defpal[color];
        return rgb.b | (rgb.g << 8) | (rgb.r << 16);
    }

    // The rest is a R5G6B5 color; we convert it to a proper 32-bit xRGB
    uint8_t red = RGBScale5[(color >> 11) & 0x1f];
    uint8_t green = RGBScale6[(color >> 5) & 0x3f];
    uint8_t blue = RGBScale5[(color) & 0x1f];
    return blue | (green << 8) | (red << 16);
}

void UpgradeGame(GameSetupStruct &game, GameDataVersion data_ver)
{
    if (data_ver < kGameVersion_362)
    {
        game.options[OPT_SAVESCREENSHOTLAYER] = UINT32_MAX; // all possible layers
    }
    // 32-bit color properties
    if (data_ver < kGameVersion_400_09)
    {
        game.hotdot = RemapFromLegacyColourNumber(game, game.hotdot);
        game.hotdotouter = RemapFromLegacyColourNumber(game, game.hotdotouter);
    }
}

void UpgradeFonts(GameSetupStruct &game, GameDataVersion data_ver)
{
    if (data_ver < kGameVersion_400_10)
    {
        for (size_t i = 0; i < game.fonts.size(); ++i)
        {
            // TODO: find a better way than relying on valid size?
            auto &fi = game.fonts[i];
            if (fi.Size > 0)
                fi.Filename.Format("agsfnt%d.ttf", i);
            else
                fi.Filename.Format("agsfnt%d.wfn", i);
        }
    }
}

// Convert audio data to the current version
void UpgradeAudio(GameSetupStruct &game, LoadedGameEntities &ents, GameDataVersion data_ver)
{
}

// Convert character data to the current version
void UpgradeCharacters(GameSetupStruct &game, GameDataVersion data_ver)
{
    const int char_count = game.numcharacters;
    auto &chars = game.chars;
    // < 3.6.2 characters always followed OPT_CHARTURNWHENFACE,
    // so they have to have TURNWHENFACE enabled
    if (data_ver < kGameVersion_362)
    {
        for (int i = 0; i < char_count; i++)
        {
            chars[i].flags |= CHF_TURNWHENFACE;
        }
    }

    // 32-bit color properties
    if (data_ver < kGameVersion_400_09)
    {
        for (int i = 0; i < char_count; i++)
        {
            chars[i].talkcolor = RemapFromLegacyColourNumber(game, chars[i].talkcolor);
        }
    }
}

void UpgradeGUI(GameSetupStruct &game, LoadedGameEntities &ents, GameDataVersion data_ver)
{
    // Previously, Buttons and Labels had a fixed Translated behavior
    if (data_ver < kGameVersion_361)
    {
        for (auto &btn : ents.GuiControls.Buttons)
            btn.SetTranslated(true); // always translated
        for (auto &lbl : ents.GuiControls.Labels)
            lbl.SetTranslated(true); // always translated
    }

    // 32-bit color properties
    if (data_ver < kGameVersion_400_09)
    {
        for (auto &gui : ents.Guis)
        {
            gui.BgColor = RemapFromLegacyColourNumber(game, gui.BgColor);
            gui.FgColor = RemapFromLegacyColourNumber(game, gui.FgColor);
        }

        for (auto &btn : ents.GuiControls.Buttons)
        {
            btn.TextColor = RemapFromLegacyColourNumber(game, btn.TextColor);
        }

        for (auto &lbl : ents.GuiControls.Labels)
        {
            lbl.TextColor = RemapFromLegacyColourNumber(game, lbl.TextColor);
        }

        for (auto &list : ents.GuiControls.ListBoxes)
        {
            list.TextColor = RemapFromLegacyColourNumber(game, list.TextColor);
            list.SelectedBgColor = RemapFromLegacyColourNumber(game, list.SelectedBgColor);
            list.SelectedTextColor = RemapFromLegacyColourNumber(game, list.SelectedTextColor);
        }

        for (auto &tbox : ents.GuiControls.TextBoxes)
        {
            tbox.TextColor = RemapFromLegacyColourNumber(game, tbox.TextColor);
        }
    }
}

void UpgradeMouseCursors(GameSetupStruct &game, GameDataVersion data_ver)
{
}

void FixupSaveDirectory(GameSetupStruct &game)
{
    // If the save game folder was not specified by game author, create one of
    // the game name, game GUID, or uniqueid, as a last resort
    if (game.saveGameFolderName.IsEmpty())
    {
        if (!game.gamename.IsEmpty())
            game.saveGameFolderName = game.gamename;
        else if (game.guid[0])
            game.saveGameFolderName = game.guid;
        else
            game.saveGameFolderName.Format("AGS-Game-%d", game.uniqueid);
    }
    // Lastly, fixup folder name by removing any illegal characters
    game.saveGameFolderName = Path::FixupSharedFilename(game.saveGameFolderName);
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
    GameDataExtReader(LoadedGameEntities &ents, GameDataVersion data_ver, std::unique_ptr<Stream> &&in)
        : DataExtReader(std::move(in), kDataExt_NumID8 | kDataExt_File64)
        , _ents(ents)
        , _dataVer(data_ver)
    {}

protected:
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override;
    HError ReadCustomProperties(Stream *in, const char *obj_type, size_t expect_obj_count, std::vector<StringIMap> &obj_values);

    LoadedGameEntities &_ents;
    GameDataVersion _dataVer {};
};

static HError ReadInteractionScriptModules(Stream *in, LoadedGameEntities &ents)
{
    // Updated InteractionEvents format, which specifies script module
    // for object interaction events
    size_t num_chars = in->ReadInt32();
    if (num_chars != ents.Game.chars.size())
        return new Error(String::FromFormat("Mismatching number of characters: read %zu expected %zu", num_chars, ents.Game.chars.size()));
    for (size_t i = 0; i < (size_t)ents.Game.numcharacters; ++i)
        ents.Game.charScripts[i] = InteractionEvents::CreateFromStream_v362(in);
    uint32_t num_invitems = in->ReadInt32();
    if (num_invitems != ents.Game.numinvitems)
        return new Error(String::FromFormat("Mismatching number of inventory items: read %zu expected %zu", num_invitems, (size_t)ents.Game.numinvitems));
    for (uint32_t i = 0; i < (uint32_t)ents.Game.numinvitems; ++i)
        ents.Game.invScripts[i] = InteractionEvents::CreateFromStream_v362(in);

    // Script module specification for GUI events
    uint32_t num_gui = in->ReadInt32();
    if (num_gui != ents.Game.numgui)
        return new Error(String::FromFormat("Mismatching number of GUI: read %zu expected %zu", num_gui, (size_t)ents.Game.numgui));
    for (size_t i = 0; i < (size_t)ents.Game.numgui; ++i)
        ents.Guis[i].ScriptModule = StrUtil::ReadString(in);
    return HError::None();
}

HError GameDataExtReader::ReadCustomProperties(Stream *in, const char *obj_type, size_t expect_obj_count, std::vector<StringIMap> &obj_values)
{
    size_t obj_count = in->ReadInt32();
    if (obj_count != expect_obj_count)
        return new Error(String::FromFormat("Mismatching number of %s: read %zu expected %zu", obj_type, obj_count, expect_obj_count));
    obj_values.resize(obj_count);
    int errors = 0;
    for (size_t i = 0; i < obj_count; ++i)
    {
        errors += Properties::ReadValues(obj_values[i], in);
    }
    if (errors > 0)
        return new MainGameFileError(kMGFErr_InvalidPropertyValues);
    return HError::None();
}

HError GameDataExtReader::ReadBlock(Stream *in, int /*block_id*/, const String &ext_id,
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
        for (FontInfo &finfo : _ents.Game.fonts)
        {
            // adjustable font outlines
            finfo.AutoOutlineThickness = in->ReadInt32();
            finfo.AutoOutlineStyle =
                static_cast<enum FontInfo::AutoOutlineStyle>(in->ReadInt32());
            // reserved
            in->ReadInt32();
            in->ReadInt32();
            in->ReadInt32();
            in->ReadInt32();
        }
    }
    else if (ext_id.CompareNoCase("v360_cursors") == 0)
    {
        for (MouseCursor &mcur : _ents.Game.mcurs)
        {
            mcur.animdelay = in->ReadInt32();
            // reserved
            in->ReadInt32();
            in->ReadInt32();
            in->ReadInt32();
        }
    }
    else if (ext_id.CompareNoCase("v361_objnames") == 0)
    {
        // Extended object names and script names:
        // for object types that had hard name length limits
        _ents.Game.gamename = StrUtil::ReadString(in);
        _ents.Game.saveGameFolderName = StrUtil::ReadString(in);
        size_t num_chars = in->ReadInt32();
        if (num_chars != _ents.Game.chars.size())
            return new Error(String::FromFormat("Mismatching number of characters: read %zu expected %zu", num_chars, _ents.Game.chars.size()));
        for (int i = 0; i < _ents.Game.numcharacters; ++i)
        {
            auto &chinfo = _ents.Game.chars[i];
            chinfo.scrname = StrUtil::ReadString(in);
            chinfo.name = StrUtil::ReadString(in);
        }
        size_t num_invitems = in->ReadInt32();
        if (num_invitems != _ents.Game.numinvitems)
            return new Error(String::FromFormat("Mismatching number of inventory items: read %zu expected %zu", num_invitems, (size_t)_ents.Game.numinvitems));
        for (int i = 0; i < _ents.Game.numinvitems; ++i)
        {
            _ents.Game.invinfo[i].name = StrUtil::ReadString(in);
        }
        size_t num_cursors = in->ReadInt32();
        if (num_cursors != _ents.Game.mcurs.size())
            return new Error(String::FromFormat("Mismatching number of cursors: read %zu expected %zu", num_cursors, _ents.Game.mcurs.size()));
        for (MouseCursor &mcur : _ents.Game.mcurs)
        {
            mcur.name = StrUtil::ReadString(in);
        }
        size_t num_clips = in->ReadInt32();
        if (num_clips != _ents.Game.audioClips.size())
            return new Error(String::FromFormat("Mismatching number of audio clips: read %zu expected %zu", num_clips, _ents.Game.audioClips.size()));
        for (ScriptAudioClip &clip : _ents.Game.audioClips)
        {
            clip.scriptName = StrUtil::ReadString(in);
            clip.fileName = StrUtil::ReadString(in);
        }
    }
    else if (ext_id.CompareNoCase("v362_interevents") == 0)
    {
        HError err = ReadInteractionScriptModules(in, _ents);
        if (!err)
            return err;
    }
    else if (ext_id.CompareNoCase("v362_interevent2") == 0)
    {
        // Explicit script module names
        // NOTE: that scripts may not be initialized at this time in case they are stored as separate
        // assets within the game package; we still read the names though to keep data format simpler
        String script_name = StrUtil::ReadString(in);
        if (_ents.GlobalScript)
            _ents.GlobalScript->SetScriptName(script_name.ToStdString());
        script_name = StrUtil::ReadString(in);
        if (_ents.DialogScript)
            _ents.DialogScript->SetScriptName(script_name.ToStdString());
        size_t module_count = in->ReadInt32();
        if (module_count != _ents.ScriptModules.size())
            return new Error(String::FromFormat("Mismatching number of script modules: read %zu expected %zu", module_count, _ents.ScriptModules.size()));
        for (size_t i = 0; i < module_count; ++i)
        {
            script_name = StrUtil::ReadString(in);
            if (_ents.ScriptModules[i])
                _ents.ScriptModules[i]->SetScriptName(script_name.ToStdString());
        }

        HError err = ReadInteractionScriptModules(in, _ents);
        if (!err)
            return err;
    }
    else if (ext_id.CompareNoCase("v362_guictrls") == 0)
    {
        size_t num_guibut = in->ReadInt32();
        if (num_guibut != _ents.GuiControls.Buttons.size())
            return new Error(String::FromFormat("Mismatching number of GUI buttons: read %zu expected %zu", num_guibut, _ents.GuiControls.Buttons.size()));
        for (GUIButton &but : _ents.GuiControls.Buttons)
        {
            // button padding
            but.TextPaddingHor = in->ReadInt32();
            but.TextPaddingVer = in->ReadInt32();
            in->ReadInt32(); // reserve 2 ints
            in->ReadInt32();
        }
    }
    // Early development version of "ags4"
    else if (ext_id.CompareNoCase("ext_ags399") == 0)
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
        for (size_t i = 0; i < _ents.Guis.size(); ++i)
        {
            _ents.Guis[i].BlendMode = (BlendMode)_in->ReadInt32();
            // Reserved for colour options
            _in->Seek(sizeof(int32_t) * 3); // flags + tint rgbs + light level
            // Reserved for transform options (see list in savegame format)
            _in->Seek(sizeof(int32_t) * 11);
        }
    }
    else if (ext_id.CompareNoCase("v400_gameopts") == 0)
    {
        _ents.Game.faceDirectionRatio = _in->ReadFloat32();
        // reserve few more 32-bit values (for a total of 10)
        _in->Seek(sizeof(int32_t) * 9);
    }
    else if (ext_id.CompareNoCase("v400_customprops") == 0)
    {
        auto &game = _ents.Game;
        game.audioclipProps.resize(game.audioClips.size());
        game.dialogProps.resize(game.numdialog);
        game.guiProps.resize(game.numgui);

        HError err = ReadCustomProperties(in, "audio clips", _ents.Game.audioClips.size(), game.audioclipProps);
        if (!err)
            return err;
        err = ReadCustomProperties(in, "dialogs", game.numdialog, game.dialogProps);
        if (!err)
            return err;
        err = ReadCustomProperties(in, "guis", game.numgui, game.guiProps);
        if (!err)
            return err;

        const char *guictrl_names[kGUIControlTypeNum] = { "", "gui buttons", "gui labels", "inventory windows", "sliders", "text boxes", "list boxes" };
        size_t guictrl_counts[kGUIControlTypeNum] = { 0, _ents.GuiControls.Buttons.size(), _ents.GuiControls.Labels.size(),
            _ents.GuiControls.InvWindows.size(), _ents.GuiControls.Sliders.size(), _ents.GuiControls.TextBoxes.size(), _ents.GuiControls.ListBoxes.size() };
        for (int i = kGUIButton; i < kGUIControlTypeNum; ++i)
        {
            err = ReadCustomProperties(in, guictrl_names[i], guictrl_counts[i], game.guicontrolProps[i]);
            if (!err)
                return err;
        }
    }
    else if (ext_id.CompareNoCase("v400_fontfiles") == 0)
    {
        size_t font_count = in->ReadInt32();
        if (font_count != _ents.Game.numfonts)
            return new Error(String::FromFormat("Mismatching number of fonts: read %zu expected %zu", font_count, (size_t)_ents.Game.numfonts));
        for (FontInfo &finfo : _ents.Game.fonts)
        {
            finfo.Filename = StrUtil::ReadString(in);
        }
    }
    else
    {
        return new MainGameFileError(kMGFErr_ExtUnknown, String::FromFormat("Type: %s", ext_id.GetCStr()));
    }
    return HError::None();
}


// Search and read only data belonging to the general game info
class GameDataExtPreloader : public GameDataExtReader
{
public:
    GameDataExtPreloader(LoadedGameEntities &ents, GameDataVersion data_ver, std::unique_ptr<Stream> &&in)
        : GameDataExtReader(ents, data_ver, std::move(in)) {}

protected:
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override;
};

HError GameDataExtPreloader::ReadBlock(Stream *in, int /*block_id*/, const String &ext_id,
    soff_t /*block_len*/, bool &read_next)
{
    // Try reading only data which belongs to the general game info
    read_next = true;
    if (ext_id.CompareNoCase("v361_objnames") == 0)
    {
        _ents.Game.gamename = StrUtil::ReadString(in);
        _ents.Game.saveGameFolderName = StrUtil::ReadString(in);
        read_next = false; // we're done
    }
    SkipBlock(); // prevent assertion trigger
    return HError::None();
}


HGameFileError ReadGameData(LoadedGameEntities &ents, std::unique_ptr<Stream> &&s_in, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;
    Stream *in = s_in.get(); // for convenience

    //-------------------------------------------------------------------------
    // The standard data section.
    //-------------------------------------------------------------------------
    GameSetupStruct::SerializeInfo sinfo;
    game.GameSetupStructBase::ReadFromFile(in, data_ver, sinfo);
    game.read_savegame_info(in, data_ver); // here we also read GUID in v3.* games

    Debug::Printf(kDbgMsg_Info, "Game title: '%s'", game.gamename.GetCStr());
    Debug::Printf(kDbgMsg_Info, "Game uid (old format): `%d`", game.uniqueid);
    Debug::Printf(kDbgMsg_Info, "Game guid: '%s'", game.guid);

    if (game.GetGameRes().IsNull())
        return new MainGameFileError(kMGFErr_InvalidNativeResolution);

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
    GUIRefCollection guictrl_refs(ents.GuiControls);
    HError err2 = GUI::ReadGUI(ents.Guis, guictrl_refs, in);
    if (!err2)
        return new MainGameFileError(kMGFErr_GameEntityFailed, err2);
    game.numgui = ents.Guis.size();

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
    GameDataExtReader reader(ents, data_ver, std::move(s_in));
    HError ext_err = reader.Read();
    return ext_err ? HGameFileError::None() : new MainGameFileError(kMGFErr_ExtListFailed, ext_err);
}

HGameFileError UpdateGameData(LoadedGameEntities &ents, GameDataVersion data_ver)
{
    GameSetupStruct &game = ents.Game;
    ApplySpriteData(game, ents, data_ver);
    UpgradeGame(game, data_ver);
    UpgradeFonts(game, data_ver);
    UpgradeAudio(game, ents, data_ver);
    UpgradeCharacters(game, data_ver);
    UpgradeGUI(game, ents, data_ver);
    UpgradeMouseCursors(game, data_ver);
    FixupSaveDirectory(game);
    return HGameFileError::None();
}

void PreReadGameData(GameSetupStruct &game, std::unique_ptr<Stream> &&s_in, GameDataVersion data_ver)
{
    Stream *in = s_in.get(); // for convenience
    GameSetupStruct::SerializeInfo sinfo;
    game.GameSetupStructBase::ReadFromFile(in, data_ver, sinfo);
    game.read_savegame_info(in, data_ver); // here we also read GUID in v3.* games

    // Check for particular expansions that might have data necessary
    // for "preload" purposes
    if (sinfo.ExtensionOffset == 0u)
        return; // either no extensions, or data version is too early

    in->Seek(sinfo.ExtensionOffset, kSeekBegin);
    LoadedGameEntities ents(game);
    GameDataExtPreloader reader(ents, data_ver, std::move(s_in));
    reader.Read();
}

} // namespace Common
} // namespace AGS

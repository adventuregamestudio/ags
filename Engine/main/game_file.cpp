//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

//
// Game data file management
//

#include "main/game_file.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/dialogtopic.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestructdefines.h"
#include "ac/gui.h"
#include "ac/viewframe.h"
#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "game/game_init.h"
#include "game/main_game_file.h"
#include "gfx/bitmap.h"
#include "gfx/blender.h"
#include "gui/guilabel.h"
#include "main/main.h"
#include "platform/base/agsplatformdriver.h"
#include "script/cc_common.h"
#include "script/script.h"
#include "util/stream.h"
#include "util/textstreamreader.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// Standard script names, used historically by AGS Editor
const String GlobalScriptObj = "GlobalScript.o";
const String DialogScriptObj = "__DialogScripts.o";
const String ScriptModulesList = "ScriptModules.lst";

extern GameSetupStruct game;
extern AGSPlatformDriver *platform;


// Test if engine supports extended capabilities required to run the game
bool test_game_caps(const std::set<String> &caps, std::set<String> &failed_caps)
{
    // Currently we support nothing special
    failed_caps = caps;
    return caps.size() == 0;
}

// Forms a simple list of capability names
String get_caps_list(const std::set<String> &caps)
{
    String caps_list;
    for (std::set<String>::const_iterator it = caps.begin(); it != caps.end(); ++it)
    {
        caps_list.Append("\n\t");
        caps_list.Append(*it);
    }
    return caps_list;
}

// Called when the game file is opened for the first time (when preloading game data);
// it logs information on data version and reports first found errors, if any.
HGameFileError game_file_first_open(MainGameSource &src)
{
    HGameFileError err = OpenMainGameFileFromDefaultAsset(src, AssetMgr.get());
    if (err ||
        err->Code() == kMGFErr_SignatureFailed ||
        err->Code() == kMGFErr_FormatVersionNotSupported)
    {
        // Log data description for debugging
        Debug::Printf(kDbgMsg_Info, "Opened game data file: %s", src.Filename.GetCStr());
        Debug::Printf(kDbgMsg_Info, "Game data version: %d", src.DataVersion);
        Debug::Printf(kDbgMsg_Info, "Compiled with: %s", src.CompiledWith.GetCStr());
        if (src.Caps.size() > 0)
        {
            String caps_list = get_caps_list(src.Caps);
            Debug::Printf(kDbgMsg_Info, "Requested engine caps: %s", caps_list.GetCStr());
        }
    }
    // Quit in case of error
    if (!err)
        return err;

    // Test the extended caps
    std::set<String> failed_caps;
    if (!test_game_caps(src.Caps, failed_caps))
    {
        String caps_list = get_caps_list(failed_caps);
        return new MainGameFileError(kMGFErr_CapsNotSupported, String::FromFormat("Missing engine caps: %s", caps_list.GetCStr()));
    }
    return HGameFileError::None();
}

HError preload_game_data()
{
    MainGameSource src;
    HGameFileError err = game_file_first_open(src);
    if (!err)
        return (HError)err;
    // Read only the particular data we need for preliminary game analysis
    PreReadGameData(game, std::move(src.InputStream), src.DataVersion, src.CompiledWith);
    FixupSaveDirectory(game);
    return HError::None();
}

// Tries to load a script asset and create a script object.
// If script was loaded successfully, then returns a new script object. Otherwise returns 'existing script'.
UScript TryLoadScriptAsset(UScript &&existing_script, const String &script_obj_name, bool is_requested)
{
    // Don't error here, as the script asset is optional
    auto in = AssetMgr->OpenAsset(script_obj_name);
    if (in)
    {
        UScript script(ccScript::CreateFromStream(Path::ReplaceExtension(script_obj_name, "asc").ToStdString(), in.get()));
        if (script)
        {
            Debug::Printf(kDbgMsg_Info, "Script module asset '%s' is found and loaded", script_obj_name.GetCStr());
            return script;
        }
        Debug::Printf(kDbgMsg_Error, "Failed to load a script module asset '%s':\n\t%s", script_obj_name.GetCStr(), cc_get_error().ErrorString.GetCStr());
        return std::move(existing_script);
    }
    if (is_requested)
        Debug::Printf(kDbgMsg_Error, "Script module asset '%s' is not present!", script_obj_name.GetCStr());
    return std::move(existing_script);
}

// Looks up for the game scripts available as separate assets.
// These are optional, so no error is raised if some of these are not found.
// For those that do exist, reads them and replaces any scripts of same kind
// in the already loaded game data.
void TryLoadGameScripts(LoadedGameEntities &ents)
{
    // Global script
    ents.GlobalScript = TryLoadScriptAsset(std::move(ents.GlobalScript), GlobalScriptObj, false);
    // Dialog script
    ents.DialogScript = TryLoadScriptAsset(std::move(ents.DialogScript), DialogScriptObj, false);
    // Script modules
    // First load a modules list
    std::vector<String> modules;
    auto in = AssetMgr->OpenAsset(ScriptModulesList);
    if (in)
    {
        TextStreamReader reader(std::move(in));
        while (!reader.EOS())
            modules.push_back(reader.ReadLine());
        Debug::Printf(kDbgMsg_Info, "Script modules list '%s' is found and loaded", ScriptModulesList.GetCStr());
    }
    if (modules.size() > ents.ScriptModules.size())
    {
        ents.ScriptModules.resize(modules.size());
        ents.ScriptModuleNames.resize(modules.size());
    }
    // Now run by the list and try loading everything
    for (size_t i = 0; i < modules.size(); ++i)
    {
        ents.ScriptModules[i] = TryLoadScriptAsset(std::move(ents.ScriptModules[i]), modules[i], true);
        ents.ScriptModuleNames[i] = ents.ScriptModules[i] ? ents.ScriptModules[i]->GetScriptName() : Path::ReplaceExtension(modules[i], "asc");
    }
}

HError load_game_file()
{
    MainGameSource src;
    LoadedGameEntities ents(game);
    HError err = (HError)OpenMainGameFileFromDefaultAsset(src, AssetMgr.get());
    if (!err)
        return err;
    err = (HError)ReadGameData(ents, std::move(src.InputStream), src.DataVersion, src.CompiledWith);
    if (!err)
        return err;

    //-------------------------------------------------------------------------
    // Data overrides: for compatibility mode and custom engine support
    // NOTE: this must be done before UpdateGameData, or certain adjustments
    // won't be applied correctly.
    if (src.DataVersion == kGameVersion_Undefined)
        return new GameInitError(kGameInitErr_UnknownDataVersion);
    loaded_game_file_version = src.DataVersion;

    // ...

    //-------------------------------------------------------------------------

    err = (HError)UpdateGameData(ents, src.DataVersion);
    if (!err)
        return err;
    TryLoadGameScripts(ents);
    err = (HError)InitGameState(ents, src.DataVersion);
    if (!err)
        return err;

    GUIE::MarkAllGUIForUpdate(true, true);
    return HError::None();
}

void display_game_file_error(HError err)
{
    platform->DisplayAlert("Loading game failed with error:\n%s\n\nThe game files may be incomplete, corrupt or from unsupported version of AGS.",
        err->FullMessage().GetCStr());
}

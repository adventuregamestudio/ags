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
#include "core/assetmanager.h"
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

extern int ifacepopped;

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
        err->Code() == kMGFErr_FormatVersionTooOld ||
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
    PreReadGameData(game, src.InputStream.get(), src.DataVersion);
    game.compiled_with = src.CompiledWith;
    FixupSaveDirectory(game);
    return HError::None();
}

static inline HError MakeScriptLoadError(const char *name)
{
    return new Error(String::FromFormat(
        "Failed to load a script module: %s", name),
        cc_get_error().ErrorString);
}

// Looks up for the game scripts available as separate assets.
// These are optional, so no error is raised if some of these are not found.
// For those that do exist, reads them and replaces any scripts of same kind
// in the already loaded game data.
HError LoadGameScripts(LoadedGameEntities &ents)
{
    // Global script
    std::unique_ptr<Stream> in(AssetMgr->OpenAsset("GlobalScript.o"));
    if (in)
    {
        PScript script(ccScript::CreateFromStream(in.get()));
        if (!script)
            return MakeScriptLoadError("GlobalScript.o");
        ents.GlobalScript = script;
    }
    // Dialog script
    in.reset(AssetMgr->OpenAsset("DialogScript.o"));
    if (in)
    {
        PScript script(ccScript::CreateFromStream(in.get()));
        if (!script)
            return MakeScriptLoadError("DialogScript.o");
        ents.DialogScript = script;
    }
    // Script modules
    // First load a modules list
    std::vector<String> modules;
    in.reset(AssetMgr->OpenAsset("ScriptModules.lst"));
    if (in)
    {
        TextStreamReader reader(in.get());
        in.release(); // TextStreamReader got it
        while (!reader.EOS())
            modules.push_back(reader.ReadLine());
    }
    if (modules.size() > ents.ScriptModules.size())
        ents.ScriptModules.resize(modules.size());
    // Now run by the list and try loading everything
    for (size_t i = 0; i < modules.size(); ++i)
    {
        in.reset(AssetMgr->OpenAsset(modules[i]));
        if (in)
        {
            PScript script(ccScript::CreateFromStream(in.get()));
            if (!script)
                return MakeScriptLoadError(modules[i].GetCStr());
            ents.ScriptModules[i] = script;
        }
    }
    return HError::None();
}

HError load_game_file()
{
    MainGameSource src;
    LoadedGameEntities ents(game);
    HError err = (HError)OpenMainGameFileFromDefaultAsset(src, AssetMgr.get());
    if (!err)
        return err;
    err = (HError)ReadGameData(ents, src.InputStream.get(), src.DataVersion);
    if (!err)
        return err;
    src.InputStream.reset();

    //-------------------------------------------------------------------------
    // Data overrides: for compatibility mode and custom engine support
    // NOTE: this must be done before UpdateGameData, or certain adjustments
    // won't be applied correctly.

    // Custom engine detection (ugly hack, depends on the known game GUIDs)
    if (strcmp(game.guid, "{d6795d1c-3cfe-49ec-90a1-85c313bfccaf}" /* Kathy Rain */ ) == 0 ||
        strcmp(game.guid, "{5833654f-6f0d-40d9-99e2-65c101c8544a}" /* Whispers of a Machine */ ) == 0)
    {
        game.options[OPT_CUSTOMENGINETAG] = CUSTOMENG_CLIFFTOP;
    }
    // Upscale mode -- for old games that supported it.
    if ((loaded_game_file_version < kGameVersion_310) && usetup.override_upscale)
    {
        if (game.GetResolutionType() == kGameResolution_320x200)
            game.SetGameResolution(kGameResolution_640x400);
        else if (game.GetResolutionType() == kGameResolution_320x240)
            game.SetGameResolution(kGameResolution_640x480);
    }
    if (game.options[OPT_CUSTOMENGINETAG] == CUSTOMENG_CLIFFTOP)
    {
        if (game.GetResolutionType() == kGameResolution_640x400)
            game.SetGameResolution(Size(640, 360));
    }

    err = (HError)UpdateGameData(ents, src.DataVersion);
    if (!err)
        return err;
    // Search the asset locations for old-style audio files and recreate clips array;
    // we do this separately after UpdateGameData, because this involves scanning enviroment.
    ScanOldStyleAudio(AssetMgr.get(), ents.Game, ents.Views, src.DataVersion);
    err = LoadGameScripts(ents);
    if (!err)
        return err;
    err = (HError)InitGameState(ents, src.DataVersion);
    if (!err)
        return err;
    return HError::None();
}

void display_game_file_error(HError err)
{
    platform->DisplayAlert("Loading game failed with error:\n%s.\n\nThe game files may be incomplete, corrupt or from unsupported version of AGS.",
        err->FullMessage().GetCStr());
}

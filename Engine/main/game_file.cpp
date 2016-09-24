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

extern int ifacepopped;

extern GameSetupStruct game;
extern ViewStruct*views;
extern DialogTopic *dialog;

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

void PreReadSaveFileInfo(Stream *in, GameDataVersion data_ver)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    game.ReadFromFile(&align_s);
    // Discard game messages we do not need here
    delete [] game.load_messages;
    game.load_messages = NULL;
    game.read_savegame_info(in, data_ver);
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
    FixupSaveDirectory(game);
    return true;
}

bool load_game_file(String &err_str)
{
    MainGameSource src;
    LoadedGameEntities ents(game, dialog, views);
    MainGameFileError load_err = OpenMainGameFile(src);
    if (load_err == kMGFErr_NoError)
    {
        load_err = ReadGameData(ents, src.InputStream.get(), src.DataVersion);
        if (load_err == kMGFErr_NoError)
            load_err = UpdateGameData(ents, src.DataVersion);
    }
    if (load_err != kMGFErr_NoError)
    {
        err_str = GetMainGameFileErrorText(load_err);
        return false;
    }
    GameInitError init_err = InitGameState(ents, src.DataVersion);
    if (init_err != kGameInitErr_NoError)
    {
        err_str = GetGameInitErrorText(init_err);
        return false;
    }
    return true;
}

void display_game_file_error(const String &err_str)
{
    platform->DisplayAlert(String::FromFormat("Loading game failed with error:\n%s.\n\nThe game files may be incomplete, corrupt or from unsupported version of AGS.",
        err_str.GetCStr()));
}

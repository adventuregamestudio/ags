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
// This unit provides functions for reading main game file into appropriate
// data structures. Main game file contains general game data, such as global
// options, lists of static game entities and compiled scripts modules.
//
//=============================================================================

#ifndef __AGS_CN_GAME__MAINGAMEFILE_H
#define __AGS_CN_GAME__MAINGAMEFILE_H

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include "ac/game_version.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/version.h"

namespace AGS
{
namespace Common
{

// Error codes for main game file reading
enum MainGameFileError
{
    kMGFErr_NoError,
    kMGFErr_FileNotFound,
    kMGFErr_NoStream,
    kMGFErr_SignatureFailed,
    // separate error given for "too old" format to provide clarifying message
    kMGFErr_FormatVersionTooOld,
    kMGFErr_FormatVersionNotSupported,
    kMGFErr_InvalidNativeResolution,
    kMGFErr_TooManyFonts,
    kMGFErr_TooManySprites,
    kMGFErr_TooManyCursors,
    kMGFErr_InvalidPropertySchema,
    kMGFErr_InvalidPropertyValues,
    kMGFErr_NoGlobalScript,
    kMGFErr_CreateGlobalScriptFailed,
    kMGFErr_CreateDialogScriptFailed,
    kMGFErr_CreateScriptModuleFailed
};

typedef stdtr1compat::shared_ptr<Stream> PStream;

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
    // Savegame format version
    GameDataVersion     DataVersion;
    // Engine version this game was intended for
    Version             EngineVersion;
    // A ponter to the opened stream
    PStream             InputStream;

    MainGameSource();
};

String             GetMainGameFileErrorText(MainGameFileError err);
// Tells if the given path (library filename) contains main game file
bool               IsMainGameLibrary(const String &filename);
// Opens main game file for reading
MainGameFileError  OpenMainGameFile(MainGameSource &src);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__MAINGAMEFILE_H

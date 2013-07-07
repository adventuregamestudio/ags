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
// 
//
//=============================================================================
#ifndef __AGS_EE_GAME__SAVEDGAME_H
#define __AGS_EE_GAME__SAVEDGAME_H

#include "util/string.h"

namespace AGS
{

namespace Common { class Stream; }

namespace Engine
{

using Common::Stream;
using Common::String;

//-----------------------------------------------------------------------------
// Saved game version history
//
// 8      last old style saved game format (of AGS 3.2.1)
// 9      first new style (chapters) format version (from 3.4)
//-----------------------------------------------------------------------------
enum SavedGameVersion
{
    kSvgVersion_Undefined = 0,
    kSvgVersion_321       = 8,
    kSvgVersion_340       = 9,
    kSvgVersion_Current   = kSvgVersion_340,
    kSvgVersion_LowestSupported = kSvgVersion_321
};

// TODO: move these to corresponding classes when they are ready
enum RuntimeGameVersion
{
    kRtGameVersion_340,
    kRtGameVersion_Current = kRtGameVersion_340
};
enum RuntimeAudioVersion
{
    kRtAudioVersion_340,
    kRtAudioVersion_Current = kRtAudioVersion_340
};
enum RuntimeCharacterVersion
{
    kRtCharacterVersion_340,
    kRtCharacterVersion_Current = kRtCharacterVersion_340
};
enum RuntimeDialogVersion
{
    kRtDialogVersion_340,
    kRtDialogVersion_Current = kRtDialogVersion_340
};
enum RuntimeInvItemVersion
{
    kRtInvItemVersion_340,
    kRtInvItemVersion_Current = kRtInvItemVersion_340
};
enum RuntimeMouseCursorVersion
{
    kRtMouseCursorVersion_340,
    kRtMouseCursorVersion_Current = kRtMouseCursorVersion_340
};
enum RuntimeViewVersion
{
    kRtViewVersion_340,
    kRtViewVersion_Current = kRtViewVersion_340
};
enum RuntimeDynamicSpriteVersion
{
    kRtDynamicSpriteVersion_340,
    kRtDynamicSpriteVersion_Current = kRtDynamicSpriteVersion_340
};
enum RuntimeOverlayVersion
{
    kRtOverlayVersion_340,
    kRtOverlayVersion_Current = kRtOverlayVersion_340
};
enum RuntimeDynamicSurfaceVersion
{
    kRtDynamicSurfaceVersion_340,
    kRtDynamicSurfaceVersion_Current = kRtDynamicSurfaceVersion_340
};
enum RuntimeScriptModuleVersion
{
    kRtScriptModuleVersion_340,
    kRtScriptModuleVersion_Current = kRtScriptModuleVersion_340
};
enum RuntimeRoomStateVersion
{
    kRtRoomStateVersion_340,
    kRtRoomStateVersion_Current = kRtRoomStateVersion_340
};
enum RuntimeRunningRoomStateVersion
{
    kRtRunningRoomStateVersion_340,
    kRtRunningRoomStateVersion_Current = kRtRunningRoomStateVersion_340
};
enum RuntimeManagedPoolVersion
{
    kRtManagedPoolVersion_340,
    kRtManagedPoolVersion_Current = kRtManagedPoolVersion_340
};
enum RuntimePluginStateVersion
{
    kRtPluginStateVersion_340,
    kRtPluginStateVersion_Current = kRtPluginStateVersion_340
};


enum SavedGameError
{
    kSvgErr_NoError,
    kSvgErr_FileNotFound,
    kSvgErr_SignatureFailed,
    kSvgErr_FormatVersionNotSupported,
    kSvgErr_IncompatibleEngine,
    kSvgErr_GameGuidFailed,
    kSvgErr_DifferentMainDataFile,
    kSvgErr_InconsistentFormat,
    kSvgErr_DataVersionNotSupported,
    kSvgErr_GameContentAssertionFailed,
    kSvgErr_LegacyDifferentResolution,
    kSvgErr_LegacyDifferentColorDepth,
    kSvgErr_GameObjectInitFailed,
    kSavedGameErrorNumber
};

struct SavedGameOpening
{
    static const char   *LegacySavedGameSignature;
    static const char   *SavedGameSignature;
    static const size_t SavedGameSigLength;

    // Name of file
    String Filename;
    // Saved game format version
    SavedGameVersion Version;

    // A ponter to opened stream
    Stream *InputStream;

    SavedGameOpening();
    ~SavedGameOpening();
    void Clear();
};

struct SavedGameInfo
{
    // Name of the engine that saved the game
    String EngineName;
    // Version of the engine that saved the game
    String EngineVersion;
    // Guid of the game which made this save
    String Guid;
    // Title of the game which made this save
    String GameTitle;
    // Name of the main data file used; this is needed to properly
    // load saves made by "minigames"
    String MainDataFilename;

    void ReadFromFile(Stream *in);
    void WriteToFile(Stream *out);
};

extern const size_t LegacyDescriptionLength;
extern const char *SavedGameErrorText[kSavedGameErrorNumber]; 

SavedGameError  OpenSavedGame(const String &filename, SavedGameOpening &opening);
SavedGameError  ReadSavedGameDescription(const Common::String &filename, Common::String &description);
SavedGameError  ReadSavedGameScreenshot(const Common::String &filename, int &sprite_slot);
SavedGameError  RestoreGameData(Stream *in);
void            SaveGameData(Stream *out);

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEDGAME_H

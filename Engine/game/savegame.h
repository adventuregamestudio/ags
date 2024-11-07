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

#ifndef __AGS_EE_GAME__SAVEGAME_H
#define __AGS_EE_GAME__SAVEGAME_H

#include <memory>
#include "ac/game_version.h"
#include "util/error.h"
#include "util/version.h"


namespace AGS
{

namespace Common { class Bitmap; class Stream; }

namespace Engine
{

using Common::Bitmap;
using Common::ErrorHandle;
using Common::TypedCodeError;
using Common::Stream;
using Common::String;
using Common::Version;

typedef std::shared_ptr<Stream> PStream;

//-----------------------------------------------------------------------------
// Savegame version history
//
// 8      last old style saved game format (of AGS 3.2.1)
// 9      first new style (self-descriptive block-based) format version
// Since 3.6.0: value is defined as AGS version represented as NN,NN,NN,NN.
//-----------------------------------------------------------------------------
enum SavegameVersion
{
    kSvgVersion_Undefined = 0,
    kSvgVersion_321       = 8,
    kSvgVersion_Components= 9,
    kSvgVersion_Cmp_64bit = 10,
    kSvgVersion_350_final = 11,
    kSvgVersion_350_final2= 12,
    kSvgVersion_351       = 13,
    kSvgVersion_360_beta  = 3060023,
    kSvgVersion_360_final = 3060041,
    kSvgVersion_361       = 3060115,
    kSvgVersion_362       = 3060200,
    kSvgVersion_Current   = kSvgVersion_362,
    kSvgVersion_LowestSupported = kSvgVersion_Components // change if support dropped
};

// Error codes for save restoration routine
enum SavegameErrorType
{
    kSvgErr_NoError,
    kSvgErr_FileOpenFailed,
    kSvgErr_SignatureFailed,
    kSvgErr_FormatVersionNotSupported,
    kSvgErr_IncompatibleEngine,
    kSvgErr_GameGuidMismatch,
    kSvgErr_ComponentListOpeningTagFormat,
    kSvgErr_ComponentListClosingTagMissing,
    kSvgErr_ComponentOpeningTagFormat,
    kSvgErr_ComponentClosingTagFormat,
    kSvgErr_ComponentSizeMismatch,
    kSvgErr_UnsupportedComponent,
    kSvgErr_ComponentSerialization,
    kSvgErr_ComponentUnserialization,
    kSvgErr_InconsistentFormat,
    kSvgErr_UnsupportedComponentVersion,
    kSvgErr_GameContentAssertion,
    kSvgErr_InconsistentData,
    kSvgErr_InconsistentPlugin,
    kSvgErr_DifferentColorDepth,
    kSvgErr_GameObjectInitFailed,
    kNumSavegameError
};

String GetSavegameErrorText(SavegameErrorType err);

typedef TypedCodeError<SavegameErrorType, GetSavegameErrorText> SavegameError;
typedef ErrorHandle<SavegameError> HSaveError;

// SavegameSource defines a successfully opened savegame stream
struct SavegameSource
{
    // Signature of the current savegame format
    static const String Signature;
    // Signature of the legacy savegame format
    static const String LegacySignature;

    // Name of the savefile
    String              Filename;
    // Savegame format version
    SavegameVersion     Version;
    // A ponter to the opened stream
    std::unique_ptr<Stream> InputStream;

    SavegameSource();
};

// Supported elements of savegame description;
// these may be used as flags to define valid fields
enum SavegameDescElem
{
    kSvgDesc_None       = 0,
    kSvgDesc_EnvInfo    = 0x0001,
    kSvgDesc_UserText   = 0x0002,
    kSvgDesc_UserImage  = 0x0004,
    kSvgDesc_All        = kSvgDesc_EnvInfo | kSvgDesc_UserText | kSvgDesc_UserImage
};

// SavegameDescription describes savegame with information about the enviroment
// it was created in, and custom data provided by user
struct SavegameDescription
{
    // Savegame's slot number
    int                 Slot = -1;
    // Name of the engine that saved the game
    String              EngineName;
    // Version of the engine that saved the game
    Version             EngineVersion;
    // Guid of the game which made this save
    String              GameGuid;
    // Legacy uniqueid of the game, for use in older games with no GUID
    int                 LegacyID = 0;
    // Title of the game which made this save
    String              GameTitle;
    // Name of the main data file used; this is needed to properly
    // load saves made by "minigames"
    String              MainDataFilename;
    // Game's main data version; should be checked early to know
    // if the save was made for the supported game format
    GameDataVersion     MainDataVersion = kGameVersion_Undefined;
    // Native color depth of the game; this is required to
    // properly restore dynamic graphics from the save
    int                 ColorDepth = 0;

    String              UserText;
    std::unique_ptr<Bitmap> UserImage;

    SavegameDescription() = default;
    SavegameDescription(SavegameDescription &&desc) = default;
    SavegameDescription(const SavegameDescription &desc);
};

// SaveCmpSelection flags tell which save components to restore, and which to skip.
// WARNING: There are interconnected components, and some objects have resources saved
// as a part of another component, for example: Scripts data stores handles to managed
// objects, and managed objects are saved as a part of "Managed Pool".
// Overlays store references to Dynamic Sprites.
// If Scripts are not restored, then managed objects will become dangling (require true GC!),
// if Overlays are not restored, then dynamic sprites exclusively owned by them may become
// dangling as well.
//
// Therefore, for now, treat this list as partially a formality. Not all of these flags
// may be safely disabled when saving/restoring, these ones must be always present.
//
// For that reason, we allow only few of these flags to be controlled from the script.
// See kSaveCmp_ScriptIgnoreMask.
enum SaveCmpSelection
{
    kSaveCmp_None           = 0,
    kSaveCmp_GameState      = 0x00000001,
    kSaveCmp_Audio          = 0x00000002,
    kSaveCmp_Characters     = 0x00000004,
    kSaveCmp_Dialogs        = 0x00000008,
    kSaveCmp_GUI            = 0x00000010,
    kSaveCmp_InvItems       = 0x00000020,
    kSaveCmp_Cursors        = 0x00000040,
    kSaveCmp_Views          = 0x00000080,
    kSaveCmp_DynamicSprites = 0x00000100,
    // [WARNING] not reliable to disable, requires a way to clean owned images
    kSaveCmp_Overlays       = 0x00000200,
    // [WARNING] not reliable to disable, requires a way to clean managed objs and their resources (at the very least)
    kSaveCmp_Scripts        = 0x00000400,
    // [WARNING] not reliable to disable, requires a way to clean managed objs and their resources (from room scripts)
    kSaveCmp_Rooms          = 0x00000800,
    // [WARNING] not reliable to disable, requires a way to clean managed objs and their resources (from this room script)
    kSaveCmp_ThisRoom       = 0x00001000,
    kSaveCmp_Plugins        = 0x00002000,
    // Special component flags, setup strictly by the engine
    kSaveCmp_ObjectSprites  = 0x80000000,
    kSaveCmp_All            = 0xFFFFFFFF,

    // Components, allowed to be ignored by script's request
    kSaveCmp_ScriptIgnoreMask =
          kSaveCmp_Audio
        | kSaveCmp_Dialogs
        | kSaveCmp_GUI
        | kSaveCmp_Cursors
        | kSaveCmp_Views
        | kSaveCmp_DynamicSprites
        | kSaveCmp_Plugins
};

struct RestoreGameStateOptions
{
    SavegameVersion SaveVersion = kSvgVersion_Undefined;
    SaveCmpSelection SelectedComponents = kSaveCmp_All;
    bool            IsGameClear = false;

    RestoreGameStateOptions() = default;
    RestoreGameStateOptions(SavegameVersion svg_ver, SaveCmpSelection select_cmp, bool game_clear)
        : SaveVersion(svg_ver), SelectedComponents(select_cmp), IsGameClear(game_clear)
    {}
};

// SaveRestoreFeedback - provide an optional instruction to the engine
// after trying to restore a save.
struct SaveRestoreFeedback
{
    bool             RetryWithClearGame = false;
    SaveCmpSelection RetryWithoutComponents = kSaveCmp_None;
};


// Opens savegame for reading; optionally reads description, if any is provided
HSaveError     OpenSavegame(const String &filename, SavegameSource &src,
                            SavegameDescription &desc, SavegameDescElem elems = kSvgDesc_All);
// Opens savegame and reads the savegame description
HSaveError     OpenSavegame(const String &filename, SavegameDescription &desc, SavegameDescElem elems = kSvgDesc_All);
// Reads the game data from the save stream and reinitializes game state;
// is_game_clear - tells whether the game is in clean default state
HSaveError     RestoreGameState(Stream *in, const SavegameDescription &desc, const RestoreGameStateOptions &options, SaveRestoreFeedback &feedback);
// Opens savegame for writing and puts in savegame description
std::unique_ptr<Stream> StartSavegame(const String &filename, const String &user_text, const Bitmap *user_image);
// Prepares game for saving state and writes game data into the save stream
void           SaveGameState(Stream *out, SaveCmpSelection select_cmp);

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAME_H

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

#ifndef __AGS_EE_GAME__SAVEGAME_H
#define __AGS_EE_GAME__SAVEGAME_H

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include "util/version.h"


namespace AGS
{

namespace Common { class Bitmap; class Stream; }

namespace Engine
{

using Common::Bitmap;
using Common::Stream;
using Common::String;
using Common::Version;

typedef stdtr1compat::shared_ptr<Stream> PStream;

//-----------------------------------------------------------------------------
// Savegame version history
//
// 8      last old style saved game format (of AGS 3.2.1)
// 9      first new style (self-descriptive block-based) format version
//-----------------------------------------------------------------------------
enum SavegameVersion
{
    kSvgVersion_Undefined = 0,
    kSvgVersion_321       = 8,
    kSvgVersion_Components= 9,
    kSvgVersion_Current   = kSvgVersion_Components,
    kSvgVersion_LowestSupported = kSvgVersion_321 // change if support dropped
};

// Error codes for save restoration routine
enum SavegameError
{
    kSvgErr_NoError,
    kSvgErr_FileNotFound,
    kSvgErr_NoStream,
    kSvgErr_SignatureFailed,
    kSvgErr_FormatVersionNotSupported,
    kSvgErr_IncompatibleEngine,
    kSvgErr_GameGuidMismatch,
    kSvgErr_ComponentOpeningTagFormat,
    kSvgErr_ComponentClosingTagFormat,
    kSvgErr_ComponentSizeMismatch,
    kSvgErr_UnsupportedComponent,
    kSvgErr_InconsistentFormat,
    kSvgErr_UnsupportedComponentVersion,
    kSvgErr_GameContentAssertion,
    kSvgErr_InconsistentData,
    kSvgErr_InconsistentPlugin,
    kSvgErr_DifferentColorDepth,
    kSvgErr_GameObjectInitFailed,
    kSvgErr_ExpectedEOS,
    kNumSavegameError
};

typedef std::auto_ptr<Stream> AStream;
typedef std::auto_ptr<Bitmap> ABitmap;

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
    // A pointer to the opened stream
    PStream             InputStream;

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
    // Name of the engine that saved the game
    String              EngineName;
    // Version of the engine that saved the game
    Version             EngineVersion;
    // Guid of the game which made this save
    String              GameGuid;
    // Title of the game which made this save
    String              GameTitle;
    // Name of the main data file used; this is needed to properly
    // load saves made by "minigames"
    String              MainDataFilename;
    // Color depth the engine was running in; this is required to
    // properly restore dynamic graphics from the save
    int                 ColorDepth;
    
    String              UserText;
    ABitmap             UserImage;

    SavegameDescription();
};


String         GetSavegameErrorText(SavegameError err);
// Opens savegame for reading; optionally reads description, if any is provided
SavegameError  OpenSavegame(const String &filename, SavegameSource &src,
                            SavegameDescription &desc, SavegameDescElem elems = kSvgDesc_All);
// Opens savegame and reads the savegame description
SavegameError  OpenSavegame(const String &filename, SavegameDescription &desc, SavegameDescElem elems = kSvgDesc_All);

// Reads the game data from the save stream and reinitializes game state
SavegameError  RestoreGameState(PStream in, SavegameVersion svg_version);

// Opens savegame for writing and puts in savegame description
PStream        StartSavegame(const String &filename, const String &user_text, const Bitmap *user_image);

// Prepares game for saving state and writes game data into the save stream
void           SaveGameState(PStream out);

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAME_H

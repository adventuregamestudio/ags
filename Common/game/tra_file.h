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
// This unit provides functions for reading compiled translation file.
//
//=============================================================================
#ifndef __AGS_CN_GAME_TRAFILE_H
#define __AGS_CN_GAME_TRAFILE_H

#include "util/error.h"
#include "util/stream.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{

enum TraFileErrorType
{
    kTraFileErr_NoError,
    kTraFileErr_SignatureFailed,
    kTraFileErr_FormatNotSupported,
    kTraFileErr_GameIDMismatch,
    kTraFileErr_UnexpectedEOF,
    kTraFileErr_UnknownBlockType,
    kTraFileErr_BlockDataOverlapping,
};

enum TraFileBlock
{
    kTraFblk_None       = 0,
    kTraFblk_Dict       = 1,
    kTraFblk_GameID     = 2,
    kTraFblk_TextOpts   = 3,
    // End of data tag
    kTraFile_EOF        = -1
};

String GetTraFileErrorText(TraFileErrorType err);
String GetTraBlockName(TraFileBlock id);

typedef TypedCodeError<TraFileErrorType, GetTraFileErrorText> TraFileError;


struct Translation
{
    // Game identifiers, for matching the translation file with the game
    int GameUid = 0;
    String GameName;
    // Translation dictionary in source/dest pairs
    StringMap Dict;
    // Localization parameters
    int NormalFont = -1; // replacement for normal font, or -1 for default
    int SpeechFont = -1; // replacement for speech font, or -1 for default
    int RightToLeft = -1; // r2l text mode (1, 2), or -1 for default
    StringMap StrOptions; // to store extended options with string values
};


// Parses translation data and tests whether it matches the given game
HError TestTraGameID(int game_uid, const String &game_name, Stream *in);
// Reads full translation data from the provided stream
HError ReadTraData(Translation &tra, Stream *in);
// Writes all translation data to the stream
void WriteTraData(const Translation &tra, Stream *out);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME_TRAFILE_H

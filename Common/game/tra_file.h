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
    kTraFblk_Dict       = 1,
    kTraFblk_GameID     = 2,
    kTraFblk_TextOpts   = 3,
    // End of data tag
    kTraFile_EOF        = -1
};

String GetTraFileErrorText(TraFileErrorType err);
String GetTraBlockName(TraFileBlock id);

typedef TypedCodeError<TraFileErrorType, GetTraFileErrorText> TraFileError;
typedef ErrorHandle<TraFileError> HTraFileError;


struct Translation
{
    // Game identifiers, for matching the translation file with the game
    int GameUid;
    String GameName;
    // Translation dictionary in source/dest pairs
    StringMap Dict;
    // Localization parameters
    int NormalFont = -1; // replacement for normal font, or -1 for default
    int SpeechFont = -1; // replacement for speech font, or -1 for default
    int RightToLeft = -1; // r2l text mode (0, 1), or -1 for default
};


// Parses translation data and tests whether it matches the given game
HTraFileError TestTraGameID(int game_uid, const String &game_name, Stream *in);
// Reads full translation data from the provided stream
HTraFileError ReadTraData(Translation &tra, Stream *in);
// Type of function that reads single trafile block and tells whether to continue reading
typedef std::function<HTraFileError(Stream *in, TraFileBlock block_id,
    soff_t block_len, bool &read_next)> PfnReadTraBlock;
// Parses tra file, passing each found block into callback; does not read any actual data itself
HTraFileError ReadTraData(PfnReadTraBlock reader, Stream *in);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME_TRAFILE_H

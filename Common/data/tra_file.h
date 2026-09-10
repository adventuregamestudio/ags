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
// This unit provides functions for reading and writing compiled translation
// file.
//
//=============================================================================
#ifndef __AGS_CN_GAME_TRAFILE_H
#define __AGS_CN_GAME_TRAFILE_H

#include "ac/gamestructdefines.h"
#include "ac/wordsdictionary.h"
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

enum TextDirectionMode
{
    kTextDirection_Default = -1,
    kTextDirection_LTR = 1,
    kTextDirection_RTL = 2
};

// Miscellaneous translation flags
enum TraOptionFlags
{
    // Translate Parser.Said input automatically
    kTraOpt_AutoTranslateSaid = 0x0001
};

// Flag-set specifies which of the font info's fields are valid
enum FontFieldFlags
{
    kFontField_None                     = 0,
    // Either Size or SizeMultiplier
    kFontField_Size                     = 0x0001,
    // Also assumes OutlineFont field
    kFontField_OutlineStyle             = 0x0002,
    kFontField_AutoOutlineStyle         = 0x0004,
    kFontField_AutoOutlineThickness     = 0x0008,
    kFontField_VerticalOffset           = 0x0010,
    kFontField_LineSpacing              = 0x0020,
    kFontField_CharacterSpacing         = 0x0040,
    kFontField_TTFMetricsFixup          = 0x0080,
    // Also assumes CustomHeight field
    kFontField_HeightDefinition         = 0x0100,
};

struct FontOverride
{
    FontOverride() = default;
    FontOverride(const FontInfo &finfo, FontFieldFlags fields)
        : Finfo(finfo), Fields(fields) {}

    FontInfo Finfo;
    FontFieldFlags Fields = kFontField_None;
};

struct Translation
{
    // Game identifiers, for matching the translation file with the game
    int GameUid = 0;
    String GameName;
    // Translation dictionary in source/dest pairs
    StringMap Dict;
    // Optional text parser's words dictionary (translation for text parser)
    WordsDictionary ParserDict;
    // Localization parameters
    String EncodingHint;
    String GameEncodingHint;
    String LanguageHint;
    int NormalFont = -1; // replacement for normal font, or -1 for default
    int SpeechFont = -1; // replacement for speech font, or -1 for default
    TextDirectionMode RightToLeft = kTextDirection_Default; // r2l text mode
    int OptFlags = 0; // misc translation options
    StringMap StrOptions; // to store extended options with string values
    std::unordered_map<int, FontOverride> FontOverrides;
};


// Parses translation data and tests whether it matches the given game
HError TestTraGameID(int game_uid, const String &game_name, std::unique_ptr<Stream> &&in);
// Reads full translation data from the provided stream
HError ReadTraData(Translation &tra, std::unique_ptr<Stream> &&in);
// Writes all translation data to the stream
void WriteTraData(const Translation &tra, std::unique_ptr<Stream> &&out);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME_TRAFILE_H

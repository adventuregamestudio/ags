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
#ifndef __AGS_TOOL_DATA__TRAUTILS_H
#define __AGS_TOOL_DATA__TRAUTILS_H

#include <memory>
#include <vector>
#include "data/tra_file.h"
#include "util/stream.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::HError;
using AGS::Common::Stream;
using AGS::Common::String;
using AGS::Common::Translation;

// Parses a TRS format and fills Translation data
// TODO: Translation is a final runtime data struct, but there have to be a
// separate struct for representing TRS contents, including list of annotations
// per trs item, user comments, etc. This would allow to save TRS back as-is.
HError ReadTRS(Translation &tra, std::unique_ptr<Stream> &&in);
// Generates a translation source file based on compiled Translation object.
HError WriteTRS(const Translation &tra, std::unique_ptr<Stream> &&out);
// Write a compiled translation file from Translation object.
HError WriteTRA(const Translation &tra, std::unique_ptr<Stream> &&out);

// A translated text of a translation source, as written in the source (escaped)
struct TranslationSourceEntry
{
    String Key;            // the game's text
    String Value;          // its translation
    bool Obsolete = false; // no longer in the game, kept in case it returns
    bool Fuzzy = false;    // marked by a translator for review
    int ParserWordID = -1; // the Text Parser word it translates, or -1
};

// A PO translation source as the editor reads it (AGS.Types.Translation): the options, and the
// translated texts in the order of the file
struct TranslationSource
{
    // Game identification, text options, encoding and language hints, font overrides;
    // the dictionaries are not used
    Translation Options;
    std::vector<int> FontOverrideOrder; // the overridden fonts, in the order of the file
    // A text found twice keeps its first place and the later translation, as in the editor
    std::vector<TranslationSourceEntry> Entries;
};

// Parses a PO translation source, as the editor does (Translation.LoadDataImpl)
HError ReadPO(TranslationSource &src, std::unique_ptr<Stream> &&in);
// Writes a PO translation source from a compiled translation, in the layout of the editor's PO
// (Translation.SaveDataImpl); the Text Parser's words get placeholder texts, the TRA has no game words
HError WritePO(const Translation &tra, std::unique_ptr<Stream> &&out);
// Writes a compiled translation from a PO source, as the editor does
// (TranslationsComponent.CompileTranslation)
HError WriteTRA(const TranslationSource &src, std::unique_ptr<Stream> &&out);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__TRAUTILS_H

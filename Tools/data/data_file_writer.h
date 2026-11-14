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
#ifndef __AGS_TOOL_DATA__DATAFILEWRITER_H
#define __AGS_TOOL_DATA__DATAFILEWRITER_H

#include <functional>
#include <memory>
#include <vector>
#include "util/error.h"
#include "util/stream.h"
#include "data/game_utils.h"

namespace AGS {
namespace DataFileWriter {

using AGS::Common::Stream;

// Auxiliary data that WriteGameData28 prepares once from the GameData,
// for the writing functions that need it
struct CompiledData
{
    // Views by ID, as the editor writes them: slot i holds the view with ID i + 1, or null;
    // the list runs up to the highest view ID
    std::vector<const DataUtil::ViewData *> ViewsByID;

    explicit CompiledData(const DataUtil::GameData &game);
};

void WriteCharacter(Stream *out, const DataUtil::GameData &game,
    const DataUtil::CharacterData &character, int index);
void WriteInventoryItem(Stream *out,
    const DataUtil::InventoryItemData &item);
void WriteCursor(Stream *out, const DataUtil::CursorData &cursor);
void WriteTextParserDictionary(const DataUtil::GameData &game, Stream *out);
void WriteView(Stream *out, const DataUtil::GameData &game,
    const DataUtil::ViewData &view);
void WriteAudioType(Stream *out, const DataUtil::AudioTypeData &type, int id);
void WriteAudioClip(Stream *out, const DataUtil::AudioClipData &clip,
    int index);
void WriteExt363Dialogs(Stream *out, const DataUtil::GameData &game);
void WritePropertySchemaBlock(Stream *out,
    const std::vector<DataUtil::CustomPropertySchemaItem> &schema);
void WritePropertyValues(Stream *out,
    const std::vector<DataUtil::CustomPropertyValue> &properties);
// Returns the code page of the game text encoding setting ("UTF-8" or "ANSI")
int ParseGameTextEncodingCodePage(const Common::String &value);

// Block-level writers used by WriteGameData28. These are exposed separately so
// that their count, ordering and reserved-slot framing may be unit-tested.
void WriteInventoryBlock(const DataUtil::GameData &game, Stream *out);
void WriteCursorBlock(const DataUtil::GameData &game, Stream *out);
void WriteViewsBlock(const DataUtil::GameData &game, const CompiledData &data, Stream *out);
void WriteCharactersBlock(const DataUtil::GameData &game, Stream *out);
void WriteGlobalMessagesBlock(const DataUtil::GameData &game, Stream *out);
void WriteCustomPropertiesBlock(const DataUtil::GameData &game, Stream *out);
void WriteAudioBlock(const DataUtil::GameData &game, Stream *out);
void WriteFontBlock(const DataUtil::GameData &game, Stream *out);
void WriteSpriteFlags(const DataUtil::GameData &game, Stream *out);
void WriteInteractionScriptsBlock(const DataUtil::GameData &game, Stream *out);
void WriteLipSyncBlock(const DataUtil::GameData &game, Stream *out);
void WriteSaveGameInfo(const DataUtil::GameData &game, Stream *out);
void WriteLegacyScriptNamesBlock(const DataUtil::GameData &game, const CompiledData &data, Stream *out);
void WritePluginsBlock(const DataUtil::GameData &game, Stream *out);
void WriteRoomNamesBlock(const DataUtil::GameData &game, Stream *out);
void WriteGuiBlock(const DataUtil::GameData &game, Stream *out);

void WriteExt360Fonts(Stream *out, const DataUtil::GameData &game);
void WriteExt360Cursors(Stream *out, const DataUtil::GameData &game);
void WriteExt361ObjNames(Stream *out, const DataUtil::GameData &game);
void WriteExt362Interactions(Stream *out, const DataUtil::GameData &game);
void WriteExt363GameInfo(Stream *out, const DataUtil::GameData &game);
void WriteGuiControlLooks363(Stream *out, const DataUtil::GUIControlData &control);
void WriteExt363GuiControls(Stream *out, const DataUtil::GameData &game);

using ExtensionWriter = std::function<void(Stream*, const DataUtil::GameData&)>;
void WriteExtension(Stream *out, const char *id,
    const DataUtil::GameData &game, ExtensionWriter writer);

// Serializes the legacy GameSetupStructBase block.
void WriteGameSetupStructBase(const DataUtil::GameData &game, const CompiledData &data, Stream *out,
    soff_t &ext_offset_pos);

} // namespace DataFileWriter

namespace DataUtil
{

using AGS::Common::HError;
using AGS::Common::Stream;

// Serializes the game data to the game28.dta format.
HError WriteGameData28(const GameData &game, std::unique_ptr<Stream> &&out);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__DATAFILEWRITER_H

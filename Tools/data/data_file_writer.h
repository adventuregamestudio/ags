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

#include <memory>
#include <vector>
#include "util/error.h"
#include "util/stream.h"
#include "data/game_utils.h"

namespace AGS {
namespace DataFileWriter {

using AGS::Common::Stream;

// Serializes the legacy GameSetupStructBase block.
void WriteGameSetupStructBase(const DataUtil::GameData &game, Stream *out,
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

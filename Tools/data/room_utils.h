//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__CRMUTIL_H
#define __AGS_TOOL_DATA__CRMUTIL_H

#include <vector>
#include "game/room_file.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::Stream;
using AGS::Common::String;
using AGS::Common::HError;
using AGS::Common::RoomFileBlock;

// Script names found in the room data
struct RoomScNames
{
    std::vector<String> ObjectNames;
    std::vector<String> HotspotNames;
};

// Following functions parse the binary room file and extract script names
// Reads only script names from the main room block, ignores other data
HError ReadFromMainBlock(RoomScNames &data, Stream *in, RoomFileVersion data_ver, soff_t block_len);
// Reads room object script names block
HError ReadObjScNamesBlock(RoomScNames &data, Stream *in, RoomFileVersion data_ver);
// A room reading callback of type PfnReadRoomBlock, meant to be passed into ReadRoomData();
// reads only blocks necessary for retrieving script names.
HError ReadRoomScNames(RoomScNames &data, Stream *in, RoomFileBlock block, const String &ext_id,
    soff_t block_len, RoomFileVersion data_ver);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__CRMUTIL_H

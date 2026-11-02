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
// Building a room file from the room sources of an AGS 4 project
// (Rooms/<n>/: data.xml, background and mask PNGs), the way the editor does
// it when it compiles a room: RoomsComponent loads and fixes up the room,
// then AGS.Native convert_room_to_native and save_room_file write it.
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__ROOM_BUILD_H
#define __AGS_TOOL_DATA__ROOM_BUILD_H

#include <utility>
#include <vector>
#include "data/room_utils.h"
#include "data/room_xml.h"
#include "game/roomdata.h"
#include "util/error.h"
#include "util/string.h"

namespace AGS
{
namespace AGF
{
class AGFReader;
}

namespace DataUtil
{

// What building a room needs from the game project
struct RoomBuildGame
{
    int UniqueID = 0;
    int DefaultRoomMaskResolution = 1;
    int ColorDepth = 32;      // in bits
    int TextCodePage = 65001; // the game text encoding
    // Cursor interaction events (the editor's InteractionSchema): cursor ID and event UID,
    // for every cursor that creates an event, in cursor order
    std::vector<std::pair<int, String>> InteractionEvents;
    std::vector<CustomPropertySchemaItem> PropertySchema;
};

// Reads the game settings that building a room needs from Game.agf
Common::HError ReadRoomBuildGame(AGF::AGFReader &reader, RoomBuildGame &game);

// Collects the names that the room script header declares
RoomScriptNames GetRoomScriptNames(const RoomXml &room);

// Fills room data from the room sources, without images and script. Applies the fixups the
// editor does before saving a room.
Common::HError ConvertRoomXml(RoomXml room, const RoomBuildGame &game, Common::RoomData &data);

// Loads the background images and area masks from the room directory into room data
Common::HError LoadRoomImages(const String &room_dir, const RoomXml &room, const RoomBuildGame &game,
                              Common::RoomData &data, std::vector<String> &warnings);

// Last steps before writing: calculates the walkable areas' top and bottom lines
void FinishRoomData(Common::RoomData &data);

// Converts a text property to the game encoding and resolves escapes, as the editor does
// (TextConverter.ConvertTextProperty)
String ConvertTextProperty(const String &utf8_text, int code_page);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__ROOM_BUILD_H

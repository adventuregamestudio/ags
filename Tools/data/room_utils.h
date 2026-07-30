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
#ifndef __AGS_TOOL_DATA__CRMUTIL_H
#define __AGS_TOOL_DATA__CRMUTIL_H

#include <vector>
#include "game/room_file.h"

namespace AGS
{
namespace DataUtil
{
using AGS::Common::String;

// Script names found in the room data
struct RoomScriptNames
{
    std::vector<String> ObjectNames;
    std::vector<String> HotspotNames;
};

// Generates room script header out of the room data;
// the header will contain room object declarations.
String MakeRoomScriptHeader(const RoomScriptNames &data);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__CRMUTIL_H

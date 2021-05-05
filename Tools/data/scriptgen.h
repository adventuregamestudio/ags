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
#ifndef __AGS_TOOL_DATA__SCRIPTGEN_H
#define __AGS_TOOL_DATA__SCRIPTGEN_H

#include "util/string.h"

namespace AGS
{

namespace DataUtil
{

using AGS::Common::String;
struct RoomScNames;

// Generates room script header out of the room data;
// the header will contain room object declarations.
String MakeRoomScriptHeader(const RoomScNames &data);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__SCRIPTGEN_H

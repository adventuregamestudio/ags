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
#ifndef __AGS_TOOL_DATA__SCRIPTGEN_H
#define __AGS_TOOL_DATA__SCRIPTGEN_H

#include "data/game_utils.h"
#include "util/string.h"

namespace AGS
{

namespace DataUtil
{

using AGS::Common::String;
struct RoomScNames;

// Generates game auto script header out of the game data;
// the header will contain game object and array declarations.
String MakeGameAutoScriptHeader(const GameRef &game);
// Generates a script header for the given list of variables;
// the header will contain variable import declarations.
String MakeVariablesScriptHeader(std::vector<Variable> &vars);
// Generates a script body for the given list of variables;
// the script will contain variable declarations and exports.
String MakeVariablesScriptBody(std::vector<Variable> &vars);
// Generates room script header out of the room data;
// the header will contain room object declarations.
String MakeRoomScriptHeader(const RoomScNames &data);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__SCRIPTGEN_H

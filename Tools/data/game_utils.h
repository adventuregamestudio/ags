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
#ifndef __AGS_TOOL_DATA__GAMEUTIL_H
#define __AGS_TOOL_DATA__GAMEUTIL_H

#include <vector>
#include "util/string.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::String;

// EntityRef is a parent struct for a game object data;
// contains common fields such as a numeric ID (aka index) and script name.
struct EntityRef
{
    String TypeName; // name of type, for the reference when necessary
    int ID = -1;
    String ScriptName;
};

// GUIRef contains only GUI data strictly necessary for generating scripts.
// NOTE: replace with full GUI struct later if appears necessary
struct GUIRef : EntityRef
{
    std::vector<EntityRef> Controls;
};

// GameRef contains only game data strictly necessary for generating scripts.
// NOTE: replace with full Game struct later if appears necessary
struct GameRef
{
    std::vector<EntityRef> AudioClips;
    std::vector<EntityRef> AudioTypes;
    std::vector<EntityRef> Characters;
    std::vector<EntityRef> Cursors;
    std::vector<EntityRef> Dialogs;
    std::vector<EntityRef> Fonts;
    std::vector<GUIRef>    GUI;
    std::vector<EntityRef> Inventory;
    std::vector<EntityRef> Views;
};

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__GAMEUTIL_H

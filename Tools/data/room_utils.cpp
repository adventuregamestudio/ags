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
#include "data/room_utils.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

//-----------------------------------------------------------------------------
// Room.ash
//-----------------------------------------------------------------------------

static void DeclareNames(String &header, const char *type_name, const std::vector<String> &names)
{
    for (const auto &name : names)
    {
        if (!name.IsEmpty())
            header.AppendFmt("import readonly %s *%s;\n", type_name, name.GetCStr());
    }
}

String MakeRoomScriptHeader(const RoomScriptNames &data)
{
    String header;
    DeclareNames(header, "Object", data.ObjectNames);
    DeclareNames(header, "Hotspot", data.HotspotNames);
    DeclareNames(header, "Region", data.RegionNames);
    DeclareNames(header, "WalkableArea", data.WalkableAreaNames);
    DeclareNames(header, "Walkbehind", data.WalkBehindNames);
    return header;
}

} // namespace DataUtil
} // namespace AGS

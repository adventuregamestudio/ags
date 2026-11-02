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
// Room sources of an AGS 4 project: Rooms/<n>/data.xml, as the editor saves
// it (AGS.Types Room, RoomObject, RoomHotspot, RoomWalkableArea,
// RoomWalkBehind and RoomRegion). The structs follow the XML elements and
// keep the editor's defaults; turning them into room file data is done in
// room_build.h.
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__ROOM_XML_H
#define __AGS_TOOL_DATA__ROOM_XML_H

#include <map>
#include <vector>
#include "data/game_utils.h"
#include "data/xml_fields.h"
#include "util/error.h"
#include "util/geometry.h"
#include "util/string.h"

namespace AGS
{
namespace DataUtil
{

// The oldest room format the editor reads (Room.FIRST_XML_VERSION)
extern const char *ROOM_XML_FIRST_VERSION;
// The newest room format these tools know: keep in step with Room.LATEST_XML_VERSION
extern const char *ROOM_XML_LATEST_VERSION;

// Script functions for the cursor interactions (AGS.Types Interactions)
struct InteractionsXml
{
    String ScriptModule;
    // <Event UID="Curs0002">function</Event>, by the cursor's event UID
    std::map<String, String> ByUID;
    // events that older editors wrote with an index only, by that index
    std::map<int, String> ByIndex;
};

struct RoomObjectXml
{
    int Image = 0;
    int BlendMode = 0; // AGS.Types BlendMode, same values as Common::BlendMode
    int Transparency = 0; // 0-100
    Pointf GraphicAnchor = Pointf(0.f, 1.f);
    Point GraphicOffset;
    bool Enabled = true;
    bool Visible = true;
    bool Clickable = true;
    int Baseline = 0;
    bool Solid = false;
    AGF::XmlRect BlockingRectangle;
    int StartX = 0;
    int StartY = 0;
    String DisplayName; // "Description" in older formats
    String ScriptName;  // "Name" in older formats
    bool UseRoomAreaScaling = false;
    bool UseRoomAreaLighting = false;
    std::vector<CustomPropertyValue> Properties;
    InteractionsXml Interactions;
    String OnAnyClick;
    String OnFrameEvent;
};

struct RoomHotspotXml
{
    String DisplayName; // "Description" in older formats
    String ScriptName;  // "Name" in older formats
    Point WalkToPoint;
    std::vector<CustomPropertyValue> Properties;
    InteractionsXml Interactions;
    String OnAnyClick;
    String OnWalkOn;
    String OnMouseMove;
};

struct RoomWalkableAreaXml
{
    String ScriptName;
    int AreaSpecificView = 0;
    float FaceDirectionRatio = 0.f;
    bool UseContinuousScaling = false;
    int ScalingLevel = 100;
    int MinScalingLevel = 100;
    int MaxScalingLevel = 100;
    std::vector<CustomPropertyValue> Properties;
};

struct RoomWalkBehindXml
{
    String ScriptName;
    int Baseline = 0;
};

struct RoomRegionXml
{
    String ScriptName;
    bool UseColourTint = false;
    int LightLevel = 100; // 0-200, 100 is the normal light (the editor shows it 100 units higher)
    int RedTint = 0;
    int GreenTint = 0;
    int BlueTint = 0;
    int TintSaturation = 50; // 1-100
    int TintLuminance = 100; // 0-100
    std::vector<CustomPropertyValue> Properties;
    String OnStanding;
    String OnWalksOnto;
    String OnWalksOff;
    // Obsolete: older formats kept the events here by index; reading moves them to the On... members
    InteractionsXml Interactions;
};

struct RoomXml
{
    String Version; // the data.xml format: the Version attribute of <Room>
    int Number = 0;
    String Description;
    int MaskResolution = 1;
    int BackgroundCount = 0;
    int GameID = 0;
    int ColorDepth = 0;
    int Width = 0;
    int Height = 0;
    int BackgroundAnimationDelay = 5;
    bool BackgroundAnimationEnabled = true;
    bool ShowPlayerCharacter = true;
    int PlayerCharacterView = 0;
    float FaceDirectionRatio = 0.f;
    int LeftEdgeX = 0;
    int RightEdgeX = 0;
    int TopEdgeY = 0;
    int BottomEdgeY = 0;
    std::vector<CustomPropertyValue> Properties;
    String OnLeaveLeft;
    String OnLeaveRight;
    String OnLeaveBottom;
    String OnLeaveTop;
    String OnLoad;
    String OnFirstTimeEnter;
    String OnAfterFadeIn;
    String OnLeave;
    String OnUnload;
    String OnRepExec;
    // Obsolete: older formats kept the events here by index; reading moves them to the On... members
    InteractionsXml Interactions;
    std::vector<RoomObjectXml> Objects;
    std::vector<RoomHotspotXml> Hotspots;
    std::vector<RoomWalkableAreaXml> WalkableAreas;
    std::vector<RoomWalkBehindXml> WalkBehinds;
    std::vector<RoomRegionXml> Regions;
};

// Reads room sources from a data.xml document, and upgrades values saved by older
// editors the way the editor does when it loads the room
Common::HError ReadRoomXml(const tinyxml2::XMLDocument &doc, RoomXml &room);
// Reads room sources from a data.xml file
Common::HError ReadRoomXmlFile(const String &filename, RoomXml &room);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__ROOM_XML_H

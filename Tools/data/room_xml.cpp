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
#include "data/room_xml.h"
#include <algorithm>
#include <cctype>
#include "util/math.h"
#include "util/string_types.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::AGF;
using tinyxml2::XMLElement;

namespace AGS
{
namespace DataUtil
{

const char *ROOM_XML_FIRST_VERSION = "3.99.99.01";
const char *ROOM_XML_LATEST_VERSION = "4.0.0.27";

// The editor reads at most this many items of each kind (Room.MAX_*)
static const size_t MAX_XML_OBJECTS = 256;
static const size_t MAX_XML_HOTSPOTS = 50;
static const size_t MAX_XML_WALKABLE_AREAS = 16;
static const size_t MAX_XML_WALK_BEHINDS = 16;
static const size_t MAX_XML_REGIONS = 16;

static const CstrArr<13> kBlendModeNames = {{"Normal", "Add", "Darken", "Lighten", "Multiply", "Screen", "Burn",
                                             "Subtract", "Exclusion", "Dodge", "Copy", "CopyRGB", "CopyAlpha"}};

//-----------------------------------------------------------------------------
// Value parsers, following the AGS.Types property setters
//-----------------------------------------------------------------------------

static bool ParseBlendMode(const char *text, int &value)
{
    return ParseXmlEnum(text, kBlendModeNames, value);
}

// Script names must be valid script identifiers (Utilities.ValidateScriptName)
static bool ParseScriptName(const char *text, String &value)
{
    for (const char *c = text; *c; ++c)
    {
        if ((c == text && std::isdigit(static_cast<unsigned char>(*c))) || !StrUtil::IsScriptWordChar(*c))
            return false;
    }
    value = text;
    return true;
}

// RoomObject.Image is never negative
static bool ParseImage(const char *text, int &value)
{
    if (!ParseXmlValue(text, value))
        return false;
    value = std::max(value, 0);
    return true;
}

// RoomObject.Transparency is clamped to 0-100
static bool ParseTransparency(const char *text, int &value)
{
    if (!ParseXmlValue(text, value))
        return false;
    value = Math::Clamp(value, 0, 100);
    return true;
}

// RoomRegion.TintSaturation cannot be 0: the editor refuses to load it
static bool ParseTintSaturation(const char *text, int &value)
{
    return ParseXmlValue(text, value) && value != 0;
}

// GraphicAnchor is "x, y" (GraphicAnchorTypeConverter): numbers that don't parse read as 0,
// and both are clamped to 0-1
static bool ParseGraphicAnchor(const char *text, Pointf &value)
{
    String s = text;
    s.Trim();
    if (s.IsEmpty())
    {
        value = Pointf(); // the converter gives null, which sets the default struct
        return true;
    }
    const auto parts = s.Split(',');
    if (parts.size() != 2)
        return false;
    float x = 0.f, y = 0.f;
    ParseXmlValue(parts[0].GetCStr(), x);
    ParseXmlValue(parts[1].GetCStr(), y);
    value = Pointf(Math::Clamp(x, 0.f, 1.f), Math::Clamp(y, 0.f, 1.f));
    return true;
}

//-----------------------------------------------------------------------------
// Readers for nested elements
//-----------------------------------------------------------------------------

static const XmlField<CustomPropertyValue> kCustomPropertyFields[] = {
    {"Name", XML_READ(&CustomPropertyValue::Name)},
    {"Value", XML_READ(&CustomPropertyValue::Value)},
};

// <Properties> with <CustomProperty> items; a malformed project may have duplicates,
// of which the editor keeps the first (names compare case-insensitively)
static HError ReadProperties(const XMLElement *elem, std::vector<CustomPropertyValue> &props)
{
    std::vector<CustomPropertyValue> items;
    HError err = ReadList(elem, "CustomProperty", items, kCustomPropertyFields);
    if (!err)
        return err;
    props.clear();
    for (auto &item : items)
    {
        const bool dup = std::any_of(props.begin(), props.end(), [&item](const CustomPropertyValue &p)
                                     { return p.Name.CompareNoCase(item.Name) == 0; });
        if (!dup)
            props.push_back(std::move(item));
    }
    return HError::None();
}

// <Interactions> with <Event Index= UID=>function</Event> items (Interactions.FromXml)
static HError ReadInteractions(const XMLElement *elem, InteractionsXml &inter)
{
    inter = {};
    for (const XMLElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if (std::strcmp(child->Name(), "Event") != 0)
        {
            if (std::strcmp(child->Name(), "ScriptModule") == 0)
                inter.ScriptModule = XmlDetail::ElementText(child);
            continue;
        }
        const int index = child->IntAttribute("Index", -1);
        const char *uid = child->Attribute("UID");
        const char *function = XmlDetail::ElementText(child);
        if (*function == 0)
            continue;
        if (uid && *uid)
            inter.ByUID[uid] = function;
        else if (index >= 0)
            inter.ByIndex[index] = function;
    }
    return HError::None();
}

//-----------------------------------------------------------------------------
// Tables
//-----------------------------------------------------------------------------

static const XmlField<RoomObjectXml> kRoomObjectFields[] = {
    {"Baseline", XML_READ(&RoomObjectXml::Baseline)},
    {"BlendMode", XML_READ_WITH(&RoomObjectXml::BlendMode, ParseBlendMode)},
    {"BlockingRectangle", XML_READ(&RoomObjectXml::BlockingRectangle)},
    {"Clickable", XML_READ(&RoomObjectXml::Clickable)},
    {"Description", XML_READ(&RoomObjectXml::DisplayName)}, // obsolete name, in rooms saved by older editors
    {"DisplayName", XML_READ(&RoomObjectXml::DisplayName)},
    {"Enabled", XML_READ(&RoomObjectXml::Enabled)},
    {"GraphicAnchor", XML_READ_WITH(&RoomObjectXml::GraphicAnchor, ParseGraphicAnchor)},
    {"GraphicOffset", XML_READ(&RoomObjectXml::GraphicOffset)},
    {"Image", XML_READ_WITH(&RoomObjectXml::Image, ParseImage)},
    {"Interactions", XML_READ_ELEMENT(&RoomObjectXml::Interactions, ReadInteractions)},
    {"Name", XML_READ_WITH(&RoomObjectXml::ScriptName, ParseScriptName)}, // obsolete name, in rooms saved by older editors
    {"OnAnyClick", XML_READ(&RoomObjectXml::OnAnyClick)},
    {"OnFrameEvent", XML_READ(&RoomObjectXml::OnFrameEvent)},
    {"Properties", XML_READ_ELEMENT(&RoomObjectXml::Properties, ReadProperties)},
    {"ScriptName", XML_READ_WITH(&RoomObjectXml::ScriptName, ParseScriptName)},
    {"Solid", XML_READ(&RoomObjectXml::Solid)},
    {"StartX", XML_READ(&RoomObjectXml::StartX)},
    {"StartY", XML_READ(&RoomObjectXml::StartY)},
    {"Transparency", XML_READ_WITH(&RoomObjectXml::Transparency, ParseTransparency)},
    {"UseRoomAreaLighting", XML_READ(&RoomObjectXml::UseRoomAreaLighting)},
    {"UseRoomAreaScaling", XML_READ(&RoomObjectXml::UseRoomAreaScaling)},
    {"Visible", XML_READ(&RoomObjectXml::Visible)},
};

static const XmlField<RoomHotspotXml> kRoomHotspotFields[] = {
    {"Description", XML_READ(&RoomHotspotXml::DisplayName)}, // obsolete name, in rooms saved by older editors
    {"DisplayName", XML_READ(&RoomHotspotXml::DisplayName)},
    {"Interactions", XML_READ_ELEMENT(&RoomHotspotXml::Interactions, ReadInteractions)},
    {"Name", XML_READ_WITH(&RoomHotspotXml::ScriptName, ParseScriptName)}, // obsolete name, in rooms saved by older editors
    {"OnAnyClick", XML_READ(&RoomHotspotXml::OnAnyClick)},
    {"OnMouseMove", XML_READ(&RoomHotspotXml::OnMouseMove)},
    {"OnWalkOn", XML_READ(&RoomHotspotXml::OnWalkOn)},
    {"Properties", XML_READ_ELEMENT(&RoomHotspotXml::Properties, ReadProperties)},
    {"ScriptName", XML_READ_WITH(&RoomHotspotXml::ScriptName, ParseScriptName)},
    {"WalkToPoint", XML_READ(&RoomHotspotXml::WalkToPoint)},
};

static const XmlField<RoomWalkableAreaXml> kRoomWalkableAreaFields[] = {
    {"AreaSpecificView", XML_READ(&RoomWalkableAreaXml::AreaSpecificView)},
    {"FaceDirectionRatio", XML_READ(&RoomWalkableAreaXml::FaceDirectionRatio)},
    {"MaxScalingLevel", XML_READ(&RoomWalkableAreaXml::MaxScalingLevel)},
    {"MinScalingLevel", XML_READ(&RoomWalkableAreaXml::MinScalingLevel)},
    {"Properties", XML_READ_ELEMENT(&RoomWalkableAreaXml::Properties, ReadProperties)},
    {"ScalingLevel", XML_READ(&RoomWalkableAreaXml::ScalingLevel)},
    {"ScriptName", XML_READ_WITH(&RoomWalkableAreaXml::ScriptName, ParseScriptName)},
    {"UseContinuousScaling", XML_READ(&RoomWalkableAreaXml::UseContinuousScaling)},
};

static const XmlField<RoomWalkBehindXml> kRoomWalkBehindFields[] = {
    {"Baseline", XML_READ(&RoomWalkBehindXml::Baseline)},
    {"ScriptName", XML_READ_WITH(&RoomWalkBehindXml::ScriptName, ParseScriptName)},
};

static const XmlField<RoomRegionXml> kRoomRegionFields[] = {
    {"BlueTint", XML_READ(&RoomRegionXml::BlueTint)},
    {"GreenTint", XML_READ(&RoomRegionXml::GreenTint)},
    {"Interactions", XML_READ_ELEMENT(&RoomRegionXml::Interactions, ReadInteractions)}, // old indexed events only
    {"LightLevel", XML_READ(&RoomRegionXml::LightLevel)},
    {"OnStanding", XML_READ(&RoomRegionXml::OnStanding)},
    {"OnWalksOff", XML_READ(&RoomRegionXml::OnWalksOff)},
    {"OnWalksOnto", XML_READ(&RoomRegionXml::OnWalksOnto)},
    {"Properties", XML_READ_ELEMENT(&RoomRegionXml::Properties, ReadProperties)},
    {"RedTint", XML_READ(&RoomRegionXml::RedTint)},
    {"ScriptName", XML_READ_WITH(&RoomRegionXml::ScriptName, ParseScriptName)},
    {"TintLuminance", XML_READ(&RoomRegionXml::TintLuminance)},
    {"TintSaturation", XML_READ_WITH(&RoomRegionXml::TintSaturation, ParseTintSaturation)},
    {"UseColourTint", XML_READ(&RoomRegionXml::UseColourTint)},
};

static HError ReadObjects(const XMLElement *elem, std::vector<RoomObjectXml> &items)
{
    return ReadList(elem, "RoomObject", items, kRoomObjectFields, MAX_XML_OBJECTS);
}

static HError ReadHotspots(const XMLElement *elem, std::vector<RoomHotspotXml> &items)
{
    return ReadList(elem, "RoomHotspot", items, kRoomHotspotFields, MAX_XML_HOTSPOTS);
}

static HError ReadWalkableAreas(const XMLElement *elem, std::vector<RoomWalkableAreaXml> &items)
{
    return ReadList(elem, "RoomWalkableArea", items, kRoomWalkableAreaFields, MAX_XML_WALKABLE_AREAS);
}

static HError ReadWalkBehinds(const XMLElement *elem, std::vector<RoomWalkBehindXml> &items)
{
    return ReadList(elem, "RoomWalkBehind", items, kRoomWalkBehindFields, MAX_XML_WALK_BEHINDS);
}

static HError ReadRegions(const XMLElement *elem, std::vector<RoomRegionXml> &items)
{
    return ReadList(elem, "RoomRegion", items, kRoomRegionFields, MAX_XML_REGIONS);
}

static const XmlField<RoomXml> kRoomFields[] = {
    {"BackgroundAnimationDelay", XML_READ(&RoomXml::BackgroundAnimationDelay)},
    {"BackgroundAnimationEnabled", XML_READ(&RoomXml::BackgroundAnimationEnabled)},
    {"BackgroundCount", XML_READ(&RoomXml::BackgroundCount)},
    {"BottomEdgeY", XML_READ(&RoomXml::BottomEdgeY)},
    {"ColorDepth", XML_READ(&RoomXml::ColorDepth)},
    {"Description", XML_READ(&RoomXml::Description)},
    {"FaceDirectionRatio", XML_READ(&RoomXml::FaceDirectionRatio)},
    {"GameID", XML_READ(&RoomXml::GameID)},
    {"Height", XML_READ(&RoomXml::Height)},
    {"Hotspots", XML_READ_ELEMENT(&RoomXml::Hotspots, ReadHotspots)},
    {"Interactions", XML_READ_ELEMENT(&RoomXml::Interactions, ReadInteractions)}, // old indexed events only
    {"LeftEdgeX", XML_READ(&RoomXml::LeftEdgeX)},
    {"MaskResolution", XML_READ(&RoomXml::MaskResolution)},
    {"Number", XML_READ(&RoomXml::Number)},
    {"Objects", XML_READ_ELEMENT(&RoomXml::Objects, ReadObjects)},
    {"OnAfterFadeIn", XML_READ(&RoomXml::OnAfterFadeIn)},
    {"OnFirstTimeEnter", XML_READ(&RoomXml::OnFirstTimeEnter)},
    {"OnLeave", XML_READ(&RoomXml::OnLeave)},
    {"OnLeaveBottom", XML_READ(&RoomXml::OnLeaveBottom)},
    {"OnLeaveLeft", XML_READ(&RoomXml::OnLeaveLeft)},
    {"OnLeaveRight", XML_READ(&RoomXml::OnLeaveRight)},
    {"OnLeaveTop", XML_READ(&RoomXml::OnLeaveTop)},
    {"OnLoad", XML_READ(&RoomXml::OnLoad)},
    {"OnRepExec", XML_READ(&RoomXml::OnRepExec)},
    {"OnUnload", XML_READ(&RoomXml::OnUnload)},
    {"PlayerCharacterView", XML_READ(&RoomXml::PlayerCharacterView)},
    {"Properties", XML_READ_ELEMENT(&RoomXml::Properties, ReadProperties)},
    {"Regions", XML_READ_ELEMENT(&RoomXml::Regions, ReadRegions)},
    {"RightEdgeX", XML_READ(&RoomXml::RightEdgeX)},
    {"ShowPlayerCharacter", XML_READ(&RoomXml::ShowPlayerCharacter)},
    {"TopEdgeY", XML_READ(&RoomXml::TopEdgeY)},
    {"WalkBehinds", XML_READ_ELEMENT(&RoomXml::WalkBehinds, ReadWalkBehinds)},
    {"WalkableAreas", XML_READ_ELEMENT(&RoomXml::WalkableAreas, ReadWalkableAreas)},
    {"Width", XML_READ(&RoomXml::Width)},
    // Obsolete properties that older editors wrote, and the editor skips
    {"Messages", nullptr},
    {"MusicVolumeAdjustment", nullptr},
    {"PlayMusicOnRoomLoad", nullptr},
    {"SaveLoadEnabled", nullptr},
};

//-----------------------------------------------------------------------------
// Upgrades of older formats, as the editor does them when it loads a room
//-----------------------------------------------------------------------------

static String IndexedFunction(const std::map<int, String> &functions, int index)
{
    auto it = functions.find(index);
    return it != functions.end() ? it->second : String();
}

// Old interaction lists were indexed by event; the new ones are indexed by cursor ID
// (Interactions.RemapLegacyFunctionIndexes)
static void RemapIndexedFunctions(std::map<int, String> &functions, const std::vector<int> &event_for_cursor)
{
    std::map<int, String> remapped;
    for (size_t cursor = 0; cursor < event_for_cursor.size(); ++cursor)
    {
        auto it = functions.find(event_for_cursor[cursor]);
        if (it != functions.end())
            remapped[static_cast<int>(cursor)] = it->second;
    }
    functions = std::move(remapped);
}

// RoomObject.ConvertOldInteractionEvents
static void ConvertOldEvents(RoomObjectXml &obj)
{
    auto &functions = obj.Interactions.ByIndex;
    if (functions.empty())
        return;
    obj.OnAnyClick = IndexedFunction(functions, 4);
    // Walk, Look, Interact, Talk, UseInv, PickUp, Pointer, Wait, Mode8, Mode9
    RemapIndexedFunctions(functions, {-1, 0, 1, 2, 3, 5, -1, -1, 6, 7});
}

// RoomHotspot.ConvertOldInteractionEvents
static void ConvertOldEvents(RoomHotspotXml &hotspot)
{
    auto &functions = hotspot.Interactions.ByIndex;
    if (functions.empty())
        return;
    hotspot.OnWalkOn = IndexedFunction(functions, 0);
    hotspot.OnAnyClick = IndexedFunction(functions, 5);
    hotspot.OnMouseMove = IndexedFunction(functions, 6);
    RemapIndexedFunctions(functions, {-1, 1, 2, 4, 3, 7, -1, -1, 8, 9});
}

// RoomRegion.LoadAndConvertInteractionEvents
static void ConvertOldEvents(RoomRegionXml &region)
{
    const auto &functions = region.Interactions.ByIndex;
    if (!functions.empty())
    {
        region.OnStanding = IndexedFunction(functions, 0);
        region.OnWalksOnto = IndexedFunction(functions, 1);
        region.OnWalksOff = IndexedFunction(functions, 2);
    }
    region.Interactions = {};
}

// Room.LoadAndConvertInteractionEvents
static void ConvertOldEvents(RoomXml &room)
{
    const auto &functions = room.Interactions.ByIndex;
    if (!functions.empty())
    {
        room.OnLeaveLeft = IndexedFunction(functions, 0);
        room.OnLeaveRight = IndexedFunction(functions, 1);
        room.OnLeaveBottom = IndexedFunction(functions, 2);
        room.OnLeaveTop = IndexedFunction(functions, 3);
        room.OnFirstTimeEnter = IndexedFunction(functions, 4);
        room.OnLoad = IndexedFunction(functions, 5);
        room.OnRepExec = IndexedFunction(functions, 6);
        room.OnAfterFadeIn = IndexedFunction(functions, 7);
        room.OnLeave = IndexedFunction(functions, 8);
        room.OnUnload = IndexedFunction(functions, 9);
    }
    room.Interactions = {};
}

static void UpgradeRoom(RoomXml &room)
{
    ConvertOldEvents(room);
    for (auto &obj : room.Objects)
        ConvertOldEvents(obj);
    for (auto &hotspot : room.Hotspots)
        ConvertOldEvents(hotspot);
    for (auto &region : room.Regions)
        ConvertOldEvents(region);
}

//-----------------------------------------------------------------------------
// Reading
//-----------------------------------------------------------------------------

HError ReadRoomXml(const tinyxml2::XMLDocument &doc, RoomXml &room)
{
    const XMLElement *root = doc.RootElement();
    if (!root || std::strcmp(root->Name(), "Room") != 0)
        return new Error("Not a room data file: <Room> element not found");
    const char *version = root->Attribute("Version");
    if (!version)
        version = ROOM_XML_FIRST_VERSION; // as the editor assumes
    if (CompareXmlVersions(version, ROOM_XML_FIRST_VERSION) < 0)
        return new Error(String::FromFormat("Room format %s is older than any AGS 4 room format", version));
    if (CompareXmlVersions(version, ROOM_XML_LATEST_VERSION) > 0)
        return new Error(String::FromFormat("Room format %s is newer than these tools support (%s). Update the tools.",
                                            version, ROOM_XML_LATEST_VERSION));

    room = {};
    room.Version = version;
    HError err = ReadFields(root, room, kRoomFields);
    if (!err)
        return err;
    UpgradeRoom(room);
    return HError::None();
}

HError ReadRoomXmlFile(const String &filename, RoomXml &room)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename.GetCStr()) != tinyxml2::XML_SUCCESS)
        return new Error(String::FromFormat("Failed to read %s: %s", filename.GetCStr(), doc.ErrorStr()));
    HError err = ReadRoomXml(doc, room);
    if (!err)
        return new Error(err, String::FromFormat("Failed to read %s", filename.GetCStr()));
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS

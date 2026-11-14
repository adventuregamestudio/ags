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
#include <cstdio>
#include <cstring>
#include "data/agfreader.h"
#include "game/roomdata.h"
#include "util/math.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::AGF;

namespace AGS
{
namespace DataUtil
{

const char *ROOM_XML_FIRST_VERSION = "3.99.99.01";
const char *ROOM_XML_LATEST_VERSION = "4.0.0.27";

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

// SerializeUtils.DeserializePropertiesFromXML: every child element must be a property
// of the class, or the editor refuses the file
template <size_t N>
static HError CheckProperties(DocElem elem, const char *const (&properties)[N])
{
    for (DocElem child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        bool known = false;
        for (const char *name : properties)
        {
            if (std::strcmp(child->Name(), name) == 0)
            {
                known = true;
                break;
            }
        }
        if (!known)
            return new Error(String::FromFormat("Unknown element <%s> in <%s> at line %d; this project may need a "
                                                "newer version of the tools",
                                                child->Name(), elem->Name(), child->GetLineNum()));
    }
    return HError::None();
}

// The constructors that take an XmlNode find their own element by the class name
static HError CheckElementName(DocElem elem, const char *class_name)
{
    if (std::strcmp(elem->Name(), class_name) != 0)
        return new Error(String::FromFormat("Unknown element <%s> at line %d, <%s> expected", elem->Name(),
                                            elem->GetLineNum(), class_name));
    return HError::None();
}

// Script names must be valid script identifiers (Utilities.ValidateScriptName)
static HError CheckScriptName(DocElem elem, const String &name)
{
    for (const char *c = name.GetCStr(); *c; ++c)
    {
        if ((c == name.GetCStr() && std::isdigit(static_cast<unsigned char>(*c))) || !StrUtil::IsScriptWordChar(*c))
            return new Error(String::FromFormat("Invalid script name \"%s\" in <%s> at line %d", name.GetCStr(),
                                                elem->Name(), elem->GetLineNum()));
    }
    return HError::None();
}

// Room.GetXmlChildren: the items of a list, at most max_limit of them
static std::vector<DocElem> GetXmlChildren(DocElem root, const char *list_name, size_t max_limit)
{
    std::vector<DocElem> items;
    DocElem list = root->FirstChildElement(list_name);
    for (DocElem item = list ? list->FirstChildElement() : nullptr; item && items.size() < max_limit;
         item = item->NextSiblingElement())
        items.push_back(item);
    return items;
}

// CustomProperties(XmlNode): <Properties> with <CustomProperty> items; a malformed project may
// have duplicates, of which the editor keeps the first (names compare case-insensitively)
static HError ReadProperties(DocElem elem, std::vector<CustomPropertyValue> &props)
{
    static const char *const kCustomPropertyElements[] = {"Name", "Value"};
    props.clear();
    DocElem list = elem->FirstChildElement("Properties");
    for (DocElem prop = list ? list->FirstChildElement() : nullptr; prop; prop = prop->NextSiblingElement())
    {
        HError err = CheckElementName(prop, "CustomProperty");
        if (!err)
            return err;
        err = CheckProperties(prop, kCustomPropertyElements);
        if (!err)
            return err;
        CustomPropertyValue value;
        value.Name = ValueParser::ReadString(prop, "Name");
        value.Value = ValueParser::ReadString(prop, "Value");
        const bool dup = std::any_of(props.begin(), props.end(), [&value](const CustomPropertyValue &p)
                                     { return p.Name.CompareNoCase(value.Name) == 0; });
        if (!dup)
            props.push_back(value);
    }
    return HError::None();
}

// Interactions.FromXml: <Interactions> with <Event Index= UID=>function</Event> items
static void ReadInteractions(DocElem elem, InteractionsXml &inter)
{
    inter = {};
    DocElem list = elem->FirstChildElement("Interactions");
    if (!list)
        return;
    inter.ScriptModule = ValueParser::ReadString(list, "ScriptModule");
    for (DocElem event = list->FirstChildElement("Event"); event; event = event->NextSiblingElement("Event"))
    {
        const int index = event->IntAttribute("Index", -1);
        const char *uid = event->Attribute("UID");
        const char *function = event->GetText();
        if (!function || !*function)
            continue;
        if (uid && *uid)
            inter.ByUID[uid] = function;
        else if (index >= 0)
            inter.ByIndex[index] = function;
    }
}

//-----------------------------------------------------------------------------
// Room items, in the order the editor writes their elements: SerializeUtils.SerializeToXML
// sorts the properties by name
//-----------------------------------------------------------------------------

// RoomObject(Room, XmlNode) and RoomObject.ToXml
static HError ReadRoomObject(DocElem elem, RoomObjectXml &obj)
{
    static const char *const kRoomObjectElements[] = {
        "Baseline", "BlendMode", "BlockingRectangle", "Clickable", "DisplayName", "Enabled", "GraphicAnchor",
        "GraphicOffset", "Image", "OnAnyClick", "OnFrameEvent", "Properties", "ScriptName", "Solid", "StartX",
        "StartY", "Transparency", "UseRoomAreaLighting", "UseRoomAreaScaling", "Visible", "Interactions",
        // obsolete names, in rooms saved by older editors
        "Description", "Name"};
    HError err = CheckElementName(elem, "RoomObject");
    if (!err)
        return err;
    err = CheckProperties(elem, kRoomObjectElements);
    if (!err)
        return err;

    obj.Baseline = ValueParser::ReadInt(elem, "Baseline");
    obj.BlendMode = ReadBlendMode(ValueParser::ReadString(elem, "BlendMode"));
    std::sscanf(ValueParser::ReadString(elem, "BlockingRectangle", "0,0,0,0"), "%d,%d,%d,%d", &obj.BlockingX,
                &obj.BlockingY, &obj.BlockingWidth, &obj.BlockingHeight);
    obj.Clickable = ValueParser::ReadBool(elem, "Clickable", true);
    obj.DisplayName = ValueParser::ReadString(elem, "DisplayName", ValueParser::ReadString(elem, "Description"));
    obj.Enabled = ValueParser::ReadBool(elem, "Enabled", true);
    obj.GraphicAnchor = ReadGraphicAnchor(ValueParser::ReadString(elem, "GraphicAnchor"), Pointf(0.f, 1.f));
    std::sscanf(ValueParser::ReadString(elem, "GraphicOffset", "0,0"), "%d,%d", &obj.GraphicOffset.X,
                &obj.GraphicOffset.Y);
    obj.Image = std::max(ValueParser::ReadInt(elem, "Image"), 0); // RoomObject.Image is never negative
    obj.OnAnyClick = ValueParser::ReadString(elem, "OnAnyClick");
    obj.OnFrameEvent = ValueParser::ReadString(elem, "OnFrameEvent");
    err = ReadProperties(elem, obj.Properties);
    if (!err)
        return err;
    obj.ScriptName = ValueParser::ReadString(elem, "ScriptName", ValueParser::ReadString(elem, "Name"));
    err = CheckScriptName(elem, obj.ScriptName);
    if (!err)
        return err;
    obj.Solid = ValueParser::ReadBool(elem, "Solid");
    obj.StartX = ValueParser::ReadInt(elem, "StartX");
    obj.StartY = ValueParser::ReadInt(elem, "StartY");
    // RoomObject.Transparency is clamped to 0-100
    obj.Transparency = Math::Clamp(ValueParser::ReadInt(elem, "Transparency"), 0, 100);
    obj.UseRoomAreaLighting = ValueParser::ReadBool(elem, "UseRoomAreaLighting");
    obj.UseRoomAreaScaling = ValueParser::ReadBool(elem, "UseRoomAreaScaling");
    obj.Visible = ValueParser::ReadBool(elem, "Visible", true);
    ReadInteractions(elem, obj.Interactions);
    return HError::None();
}

// RoomHotspot(Room, XmlNode) and RoomHotspot.ToXml
static HError ReadRoomHotspot(DocElem elem, RoomHotspotXml &hotspot)
{
    static const char *const kRoomHotspotElements[] = {
        "DisplayName", "OnAnyClick", "OnMouseMove", "OnWalkOn", "Properties", "ScriptName", "WalkToPoint",
        "Interactions",
        // obsolete names, in rooms saved by older editors
        "Description", "Name"};
    HError err = CheckElementName(elem, "RoomHotspot");
    if (!err)
        return err;
    err = CheckProperties(elem, kRoomHotspotElements);
    if (!err)
        return err;

    hotspot.DisplayName = ValueParser::ReadString(elem, "DisplayName", ValueParser::ReadString(elem, "Description"));
    hotspot.OnAnyClick = ValueParser::ReadString(elem, "OnAnyClick");
    hotspot.OnMouseMove = ValueParser::ReadString(elem, "OnMouseMove");
    hotspot.OnWalkOn = ValueParser::ReadString(elem, "OnWalkOn");
    err = ReadProperties(elem, hotspot.Properties);
    if (!err)
        return err;
    hotspot.ScriptName = ValueParser::ReadString(elem, "ScriptName", ValueParser::ReadString(elem, "Name"));
    err = CheckScriptName(elem, hotspot.ScriptName);
    if (!err)
        return err;
    std::sscanf(ValueParser::ReadString(elem, "WalkToPoint", "0,0"), "%d,%d", &hotspot.WalkToPoint.X,
                &hotspot.WalkToPoint.Y);
    ReadInteractions(elem, hotspot.Interactions);
    return HError::None();
}

// RoomWalkableArea(Room, XmlNode) and RoomWalkableArea.ToXml
static HError ReadRoomWalkableArea(DocElem elem, RoomWalkableAreaXml &area)
{
    static const char *const kRoomWalkableAreaElements[] = {
        "AreaSpecificView", "FaceDirectionRatio", "MaxScalingLevel", "MinScalingLevel", "Properties",
        "ScalingLevel", "ScriptName", "UseContinuousScaling"};
    HError err = CheckElementName(elem, "RoomWalkableArea");
    if (!err)
        return err;
    err = CheckProperties(elem, kRoomWalkableAreaElements);
    if (!err)
        return err;

    area.AreaSpecificView = ValueParser::ReadInt(elem, "AreaSpecificView");
    area.FaceDirectionRatio = ValueParser::ReadFloat(elem, "FaceDirectionRatio");
    area.MaxScalingLevel = ValueParser::ReadInt(elem, "MaxScalingLevel", 100);
    area.MinScalingLevel = ValueParser::ReadInt(elem, "MinScalingLevel", 100);
    err = ReadProperties(elem, area.Properties);
    if (!err)
        return err;
    area.ScalingLevel = ValueParser::ReadInt(elem, "ScalingLevel", 100);
    area.ScriptName = ValueParser::ReadString(elem, "ScriptName");
    err = CheckScriptName(elem, area.ScriptName);
    if (!err)
        return err;
    area.UseContinuousScaling = ValueParser::ReadBool(elem, "UseContinuousScaling");
    return HError::None();
}

// RoomWalkBehind(Room, XmlNode) and RoomWalkBehind.ToXml
static HError ReadRoomWalkBehind(DocElem elem, RoomWalkBehindXml &area)
{
    static const char *const kRoomWalkBehindElements[] = {"Baseline", "ScriptName"};
    HError err = CheckElementName(elem, "RoomWalkBehind");
    if (!err)
        return err;
    err = CheckProperties(elem, kRoomWalkBehindElements);
    if (!err)
        return err;

    area.Baseline = ValueParser::ReadInt(elem, "Baseline");
    area.ScriptName = ValueParser::ReadString(elem, "ScriptName");
    return CheckScriptName(elem, area.ScriptName);
}

// RoomRegion(Room, XmlNode) and RoomRegion.ToXml
static HError ReadRoomRegion(DocElem elem, RoomRegionXml &region)
{
    static const char *const kRoomRegionElements[] = {
        "BlueTint", "GreenTint", "LightLevel", "OnStanding", "OnWalksOff", "OnWalksOnto", "Properties",
        "RedTint", "ScriptName", "TintLuminance", "TintSaturation", "UseColourTint",
        // obsolete: old indexed events only
        "Interactions"};
    HError err = CheckElementName(elem, "RoomRegion");
    if (!err)
        return err;
    err = CheckProperties(elem, kRoomRegionElements);
    if (!err)
        return err;

    region.BlueTint = ValueParser::ReadInt(elem, "BlueTint");
    region.GreenTint = ValueParser::ReadInt(elem, "GreenTint");
    region.LightLevel = ValueParser::ReadInt(elem, "LightLevel", 100);
    region.OnStanding = ValueParser::ReadString(elem, "OnStanding");
    region.OnWalksOff = ValueParser::ReadString(elem, "OnWalksOff");
    region.OnWalksOnto = ValueParser::ReadString(elem, "OnWalksOnto");
    err = ReadProperties(elem, region.Properties);
    if (!err)
        return err;
    region.RedTint = ValueParser::ReadInt(elem, "RedTint");
    region.ScriptName = ValueParser::ReadString(elem, "ScriptName");
    err = CheckScriptName(elem, region.ScriptName);
    if (!err)
        return err;
    region.TintLuminance = ValueParser::ReadInt(elem, "TintLuminance", 100);
    region.TintSaturation = ValueParser::ReadInt(elem, "TintSaturation", 50);
    // RoomRegion.TintSaturation cannot be 0: the editor refuses to load it
    if (region.TintSaturation == 0)
        return new Error(String::FromFormat("Invalid value 0 of <TintSaturation> at line %d",
                                            elem->FirstChildElement("TintSaturation")->GetLineNum()));
    region.UseColourTint = ValueParser::ReadBool(elem, "UseColourTint");
    ReadInteractions(elem, region.Interactions);
    return HError::None();
}

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

// RoomObject.LoadAndConvertInteractionEvents
static void ConvertOldEvents(RoomObjectXml &obj)
{
    auto &functions = obj.Interactions.ByIndex;
    if (functions.empty())
        return;
    obj.OnAnyClick = IndexedFunction(functions, 4);
    // Walk, Look, Interact, Talk, UseInv, PickUp, Pointer, Wait, Mode8, Mode9
    RemapIndexedFunctions(functions, {-1, 0, 1, 2, 3, 5, -1, -1, 6, 7});
}

// RoomHotspot.LoadAndConvertInteractionEvents
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

// Room(XmlNode): UnloadedRoom(XmlNode) reads the room's own properties, then Room.LoadFromXml
// the lists; the elements are in the order of Room.ToXml
HError ReadRoomXml(tinyxml2::XMLDocument &doc, RoomXml &room)
{
    static const char *const kRoomElements[] = {
        "BackgroundAnimationDelay", "BackgroundAnimationEnabled", "BackgroundCount", "BottomEdgeY", "ColorDepth",
        "Description", "FaceDirectionRatio", "GameID", "Height", "LeftEdgeX", "MaskResolution", "Number",
        "OnAfterFadeIn", "OnFirstTimeEnter", "OnLeave", "OnLeaveBottom", "OnLeaveLeft", "OnLeaveRight", "OnLeaveTop",
        "OnLoad", "OnRepExec", "OnUnload", "PlayerCharacterView", "Properties", "RightEdgeX", "ShowPlayerCharacter",
        "TopEdgeY", "Width", "Objects", "Hotspots", "WalkableAreas", "WalkBehinds", "Regions",
        // obsolete: old indexed events only
        "Interactions",
        // obsolete properties that older editors wrote, and the editor skips
        "Messages", "MusicVolumeAdjustment", "PlayMusicOnRoomLoad", "SaveLoadEnabled"};

    DocElem root = doc.RootElement();
    if (!root || std::strcmp(root->Name(), "Room") != 0)
        return new Error("Not a room data file: <Room> element not found");
    // without a version the editor assumes the first one
    const char *version = ValueParser::ReadAttributeString(root, "Version", ROOM_XML_FIRST_VERSION);
    if (CompareXmlVersions(version, ROOM_XML_FIRST_VERSION) < 0)
        return new Error(String::FromFormat("Room format %s is older than any AGS 4 room format", version));
    if (CompareXmlVersions(version, ROOM_XML_LATEST_VERSION) > 0)
        return new Error(String::FromFormat("Room format %s is newer than these tools support (%s). Update the tools.",
                                            version, ROOM_XML_LATEST_VERSION));
    HError err = CheckProperties(root, kRoomElements);
    if (!err)
        return err;

    room = {};
    room.Version = version;
    room.BackgroundAnimationDelay = ValueParser::ReadInt(root, "BackgroundAnimationDelay", 5);
    room.BackgroundAnimationEnabled = ValueParser::ReadBool(root, "BackgroundAnimationEnabled", true);
    room.BackgroundCount = ValueParser::ReadInt(root, "BackgroundCount");
    room.BottomEdgeY = ValueParser::ReadInt(root, "BottomEdgeY");
    room.ColorDepth = ValueParser::ReadInt(root, "ColorDepth");
    room.Description = ValueParser::ReadString(root, "Description");
    room.FaceDirectionRatio = ValueParser::ReadFloat(root, "FaceDirectionRatio");
    room.GameID = ValueParser::ReadInt(root, "GameID");
    room.Height = ValueParser::ReadInt(root, "Height");
    room.LeftEdgeX = ValueParser::ReadInt(root, "LeftEdgeX");
    room.MaskResolution = ValueParser::ReadInt(root, "MaskResolution", 1);
    room.Number = ValueParser::ReadInt(root, "Number");
    room.OnAfterFadeIn = ValueParser::ReadString(root, "OnAfterFadeIn");
    room.OnFirstTimeEnter = ValueParser::ReadString(root, "OnFirstTimeEnter");
    room.OnLeave = ValueParser::ReadString(root, "OnLeave");
    room.OnLeaveBottom = ValueParser::ReadString(root, "OnLeaveBottom");
    room.OnLeaveLeft = ValueParser::ReadString(root, "OnLeaveLeft");
    room.OnLeaveRight = ValueParser::ReadString(root, "OnLeaveRight");
    room.OnLeaveTop = ValueParser::ReadString(root, "OnLeaveTop");
    room.OnLoad = ValueParser::ReadString(root, "OnLoad");
    room.OnRepExec = ValueParser::ReadString(root, "OnRepExec");
    room.OnUnload = ValueParser::ReadString(root, "OnUnload");
    room.PlayerCharacterView = ValueParser::ReadInt(root, "PlayerCharacterView");
    err = ReadProperties(root, room.Properties);
    if (!err)
        return err;
    room.RightEdgeX = ValueParser::ReadInt(root, "RightEdgeX");
    room.ShowPlayerCharacter = ValueParser::ReadBool(root, "ShowPlayerCharacter", true);
    room.TopEdgeY = ValueParser::ReadInt(root, "TopEdgeY");
    room.Width = ValueParser::ReadInt(root, "Width");
    ReadInteractions(root, room.Interactions);

    for (DocElem elem : GetXmlChildren(root, "Objects", MAX_ROOM_OBJECTS))
    {
        RoomObjectXml obj;
        err = ReadRoomObject(elem, obj);
        if (!err)
            return err;
        room.Objects.push_back(obj);
    }
    for (DocElem elem : GetXmlChildren(root, "Hotspots", MAX_ROOM_HOTSPOTS))
    {
        RoomHotspotXml hotspot;
        err = ReadRoomHotspot(elem, hotspot);
        if (!err)
            return err;
        room.Hotspots.push_back(hotspot);
    }
    for (DocElem elem : GetXmlChildren(root, "WalkableAreas", MAX_WALK_AREAS))
    {
        RoomWalkableAreaXml area;
        err = ReadRoomWalkableArea(elem, area);
        if (!err)
            return err;
        room.WalkableAreas.push_back(area);
    }
    for (DocElem elem : GetXmlChildren(root, "WalkBehinds", MAX_WALK_BEHINDS))
    {
        RoomWalkBehindXml area;
        err = ReadRoomWalkBehind(elem, area);
        if (!err)
            return err;
        room.WalkBehinds.push_back(area);
    }
    for (DocElem elem : GetXmlChildren(root, "Regions", MAX_ROOM_REGIONS))
    {
        RoomRegionXml region;
        err = ReadRoomRegion(elem, region);
        if (!err)
            return err;
        room.Regions.push_back(region);
    }

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

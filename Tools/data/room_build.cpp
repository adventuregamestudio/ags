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
#include "data/room_build.h"
#include <algorithm>
#include "ac/common_defines.h"
#include "data/agfreader.h"
#include "data/data_file_writer.h"
#include "data/sprite_utils.h"
#include "data/xml_fields.h"
#include "gfx/gfx_def.h"
#include "gfx/image_file.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::AGF;

namespace AGS
{
namespace DataUtil
{

//-----------------------------------------------------------------------------
// Game settings
//-----------------------------------------------------------------------------

// The parts of MouseCursor that make the interaction schema
struct CursorEventXml
{
    int ID = 0;
    bool CreateEvent = false;
    String EventUID;
};

static const XmlField<CursorEventXml> kCursorEventFields[] = {
    {"CreateEvent", XML_READ(&CursorEventXml::CreateEvent)},
    {"EventUID", XML_READ(&CursorEventXml::EventUID)},
    {"ID", XML_READ(&CursorEventXml::ID)},
};

HError ReadRoomBuildGame(AGFReader &reader, RoomBuildGame &game)
{
    game = {};
    GameSettings opts;
    ReadGameSettings(opts, reader.GetGameRoot());
    game.UniqueID = opts.UniqueID;
    game.DefaultRoomMaskResolution = opts.DefaultRoomMaskResolution;
    game.ColorDepth = static_cast<int>(opts.ColorDepth) * 8;
    game.TextCodePage = DataFileWriter::ParseGameTextEncodingCodePage(opts.GameTextEncoding);
    ReadCustomPropertySchema(game.PropertySchema, reader.GetGameRoot());

    // Tasks.SyncCursorsWithInteractionSchema
    Cursors cursors;
    std::vector<DocElem> cursor_elems;
    cursors.GetAll(reader.GetGameRoot(), cursor_elems);
    for (const auto *elem : cursor_elems)
    {
        CursorEventXml cursor;
        HError err = ReadFields(elem, cursor, kCursorEventFields, UnknownElements::Skip);
        if (!err)
            return err;
        if (cursor.CreateEvent)
            game.InteractionEvents.emplace_back(cursor.ID, cursor.EventUID);
    }
    return HError::None();
}

//-----------------------------------------------------------------------------
// Script names
//-----------------------------------------------------------------------------

RoomScriptNames GetRoomScriptNames(const RoomXml &room)
{
    RoomScriptNames names;
    for (const auto &obj : room.Objects)
        names.ObjectNames.push_back(obj.ScriptName);
    for (const auto &hotspot : room.Hotspots)
        names.HotspotNames.push_back(hotspot.ScriptName);
    for (const auto &region : room.Regions)
        names.RegionNames.push_back(region.ScriptName);
    for (const auto &area : room.WalkableAreas)
        names.WalkableAreaNames.push_back(area.ScriptName);
    for (const auto &area : room.WalkBehinds)
        names.WalkBehindNames.push_back(area.ScriptName);
    return names;
}

//-----------------------------------------------------------------------------
// Text
//-----------------------------------------------------------------------------

// Decodes one UTF-8 sequence; returns the code point, or -1 for a malformed sequence
static int DecodeUtf8(const char *&s)
{
    const unsigned char c = static_cast<unsigned char>(*s++);
    int extra, cp;
    if (c < 0x80)
        return c;
    else if ((c & 0xE0) == 0xC0)
        extra = 1, cp = c & 0x1F;
    else if ((c & 0xF0) == 0xE0)
        extra = 2, cp = c & 0x0F;
    else if ((c & 0xF8) == 0xF0)
        extra = 3, cp = c & 0x07;
    else
        return -1;
    for (; extra > 0; --extra)
    {
        const unsigned char cc = static_cast<unsigned char>(*s);
        if ((cc & 0xC0) != 0x80)
            return -1;
        cp = (cp << 6) | (cc & 0x3F);
        ++s;
    }
    return cp;
}

// Encodes UTF-8 text as Windows-1252; characters it doesn't have become '?', as with .NET
static String Utf8ToCp1252(const String &utf8)
{
    // Code points for the bytes 0x80-0x9F; 0 where the code page has no character
    static const int kHighChars[32] = {0x20AC, 0,      0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
                                       0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0,      0x017D, 0,
                                       0,      0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
                                       0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0,      0x017E, 0x0178};
    std::vector<char> out;
    for (const char *s = utf8.GetCStr(); *s;)
    {
        const int cp = DecodeUtf8(s);
        int byte = '?';
        if (cp >= 0 && (cp < 0x80 || (cp >= 0xA0 && cp <= 0xFF)))
            byte = cp;
        else if (cp >= 0x80 && cp <= 0x9F && kHighChars[cp - 0x80] == 0)
            byte = cp; // undefined in the code page: kept as is
        else
        {
            const int *found = std::find(std::begin(kHighChars), std::end(kHighChars), cp);
            if (cp > 0 && found != std::end(kHighChars))
                byte = 0x80 + static_cast<int>(found - std::begin(kHighChars));
        }
        out.push_back(static_cast<char>(byte));
    }
    return String(out.data(), out.size());
}

String ConvertTextProperty(const String &utf8_text, const int code_page)
{
    String str = (code_page == 65001) ? utf8_text : Utf8ToCp1252(utf8_text);
    // Escape backslashes before brackets, for '\[' support: Unescape deletes the '\'
    // of an unrecognized sequence
    str.Replace("\\[", "\\\\[");
    return StrUtil::Unescape(str);
}

//-----------------------------------------------------------------------------
// Room data
//-----------------------------------------------------------------------------

// Keeps only properties that the game's schema defines, with the schema's spelling of the name
// (CustomProperties.SyncWithSchema)
static void SyncWithSchema(std::vector<CustomPropertyValue> &props, const std::vector<CustomPropertySchemaItem> &schema)
{
    std::vector<CustomPropertyValue> synced;
    for (const auto &p : props)
    {
        auto def = std::find_if(schema.begin(), schema.end(), [&p](const CustomPropertySchemaItem &item)
                                { return item.Name.CompareNoCase(p.Name) == 0; });
        if (def != schema.end())
            synced.push_back({def->Name, p.Value});
    }
    props = std::move(synced);
}

// AGS.Native CompileCustomProperties
static void CompileProperties(const std::vector<CustomPropertyValue> &props, StringIMap &dst, int code_page)
{
    dst.clear();
    for (const auto &p : props)
        dst[p.Name] = ConvertTextProperty(p.Value, code_page);
}

// Writes the cursor interaction functions as a table indexed by cursor ID, keeping empty slots
// (AGS.Native convert_interaction_scripts_to_native)
static void ConvertInteractions(const InteractionsXml &inter, const RoomBuildGame &game, ScriptEventHandlers &dst)
{
    dst.SetScriptModule(inter.ScriptModule);
    // Functions of older formats are indexed by cursor ID; find them their event UID
    // (Interactions.ResolveIndexedFunctions)
    std::map<String, String> by_uid = inter.ByUID;
    for (const auto &fn : inter.ByIndex)
    {
        for (const auto &evt : game.InteractionEvents)
        {
            if (evt.first == fn.first)
            {
                by_uid.emplace(evt.second, fn.second);
                break;
            }
        }
    }
    if (by_uid.empty())
        return; // no assigned functions, nothing to write

    const auto &events = game.InteractionEvents;
    std::vector<ScriptEventHandler> handlers;
    for (int index = 0, evt = 0; static_cast<size_t>(evt) < events.size(); ++index)
    {
        if (events[evt].first < index)
        {
            ++evt; // out of order; the editor's cursors are always sorted by ID
            --index;
        }
        else if (events[evt].first == index)
        {
            auto fn = by_uid.find(events[evt].second);
            handlers.push_back(fn != by_uid.end() ? ScriptEventHandler(fn->second) : ScriptEventHandler());
            ++evt;
        }
        else
        {
            handlers.push_back(ScriptEventHandler()); // empty slot
        }
    }
    dst.SetHandlers(std::move(handlers));
}

// The fixups of RoomsComponent: SyncRoomWithGame when loading, PerformPreSaveChecks when saving
static HError FixupRoom(RoomXml &room, const RoomBuildGame &game)
{
    SyncWithSchema(room.Properties, game.PropertySchema);
    for (auto &hotspot : room.Hotspots)
        SyncWithSchema(hotspot.Properties, game.PropertySchema);
    for (auto &obj : room.Objects)
        SyncWithSchema(obj.Properties, game.PropertySchema);
    for (auto &region : room.Regions)
        SyncWithSchema(region.Properties, game.PropertySchema);

    for (auto &area : room.WalkableAreas)
    {
        if (area.UseContinuousScaling && area.MaxScalingLevel < area.MinScalingLevel)
            std::swap(area.MaxScalingLevel, area.MinScalingLevel);
    }

    if (room.GameID != game.UniqueID)
    {
        // A room taken from another game gets the game's default mask resolution; resizing
        // the masks is left to the editor
        if (room.MaskResolution != game.DefaultRoomMaskResolution)
            return new Error(String::FromFormat("Room %d comes from another game and has mask resolution 1:%d; the "
                                                "game uses 1:%d. Open and save the room in the editor first.",
                                                room.Number, room.MaskResolution, game.DefaultRoomMaskResolution));
        room.GameID = game.UniqueID;
    }
    return HError::None();
}

HError ConvertRoomXml(RoomXml room, const RoomBuildGame &game, RoomData &data)
{
    HError err = FixupRoom(room, game);
    if (!err)
        return err;

    // AGS.Native convert_room_to_native
    const int cp = game.TextCodePage;
    data = RoomData();
    data.Name = ConvertTextProperty(room.Description, cp);
    data.ScriptName = ""; // reserved
    data.MaskResolution = room.MaskResolution;
    data.GameID = room.GameID;
    data.Edges.Bottom = room.BottomEdgeY;
    data.Edges.Left = room.LeftEdgeX;
    data.Edges.Right = room.RightEdgeX;
    data.Edges.Top = room.TopEdgeY;
    data.BackgroundBPP = (room.ColorDepth + 7) / 8;
    data.Width = room.Width;
    data.Height = room.Height;
    data.BgAnimSpeed = room.BackgroundAnimationDelay;
    data.Options.Flags = 0;
    data.Options.PlayerCharOff = !room.ShowPlayerCharacter;
    data.Options.PlayerView = room.PlayerCharacterView;
    if (!room.BackgroundAnimationEnabled)
        data.Options.Flags |= kRoomFlag_BkgFrameLocked;
    data.Options.FaceDirectionRatio = room.FaceDirectionRatio;

    data.Objects.resize(room.Objects.size());
    for (size_t i = 0; i < room.Objects.size(); ++i)
    {
        const auto &obj = room.Objects[i];
        auto &dst = data.Objects[i];
        dst.ID = static_cast<int>(i);
        dst.ScriptName = obj.ScriptName;
        dst.Sprite = obj.Image;
        dst.Transparency = GfxDef::Trans100ToLegacyTrans255(obj.Transparency);
        dst.BlendMode = static_cast<BlendMode>(obj.BlendMode);
        dst.X = obj.StartX;
        dst.Y = obj.StartY;
        dst.Baseline = obj.Baseline;
        dst.Name = ConvertTextProperty(obj.DisplayName, cp);
        dst.Flags = (OBJF_ENABLED * obj.Enabled) | (OBJF_VISIBLE * obj.Visible) |
                    (OBJF_USEROOMSCALING * obj.UseRoomAreaScaling) | (OBJF_USEREGIONTINTS * obj.UseRoomAreaLighting) |
                    (OBJF_NOINTERACT * (!obj.Clickable)) | (OBJF_SOLID * obj.Solid);
        dst.BlockingRect = RectWH(obj.BlockingRectangle.X, obj.BlockingRectangle.Y, obj.BlockingRectangle.Width,
                                  obj.BlockingRectangle.Height);
        dst.GraphicAnchor = obj.GraphicAnchor;
        dst.GraphicOffset = obj.GraphicOffset;
        CompileProperties(obj.Properties, dst.Properties, cp);
        ConvertInteractions(obj.Interactions, game, dst.Interactions);
        dst.GetEvents().SetHandlers(StringMap{{"OnAnyClick", obj.OnAnyClick}, {"OnFrameEvent", obj.OnFrameEvent}});
    }

    data.HotspotCount = static_cast<uint32_t>(std::min<size_t>(room.Hotspots.size(), MAX_ROOM_HOTSPOTS));
    for (uint32_t i = 0; i < data.HotspotCount; ++i)
    {
        const auto &hotspot = room.Hotspots[i];
        auto &dst = data.Hotspots[i];
        dst.ID = static_cast<int>(i);
        dst.Name = ConvertTextProperty(hotspot.DisplayName, cp);
        dst.ScriptName = hotspot.ScriptName;
        dst.WalkTo = hotspot.WalkToPoint;
        CompileProperties(hotspot.Properties, dst.Properties, cp);
        ConvertInteractions(hotspot.Interactions, game, dst.Interactions);
        dst.GetEvents().SetHandlers(StringMap{
            {"OnAnyClick", hotspot.OnAnyClick}, {"OnMouseMove", hotspot.OnMouseMove}, {"OnWalkOn", hotspot.OnWalkOn}});
    }

    data.WalkAreaCount = static_cast<uint32_t>(std::min<size_t>(room.WalkableAreas.size(), MAX_WALK_AREAS));
    for (uint32_t i = 0; i < data.WalkAreaCount; ++i)
    {
        const auto &area = room.WalkableAreas[i];
        auto &dst = data.WalkAreas[i];
        dst.ID = static_cast<int>(i);
        dst.ScriptName = area.ScriptName;
        dst.PlayerView = area.AreaSpecificView;
        dst.FaceDirectionRatio = area.FaceDirectionRatio;
        if (area.UseContinuousScaling)
        {
            dst.ScalingFar = area.MinScalingLevel - 100;
            dst.ScalingNear = area.MaxScalingLevel - 100;
        }
        else
        {
            dst.ScalingFar = area.ScalingLevel - 100;
            dst.ScalingNear = NOT_VECTOR_SCALED;
        }
        CompileProperties(area.Properties, dst.Properties, cp);
    }

    data.WalkBehindCount = static_cast<uint32_t>(std::min<size_t>(room.WalkBehinds.size(), MAX_WALK_BEHINDS));
    for (uint32_t i = 0; i < data.WalkBehindCount; ++i)
    {
        data.WalkBehinds[i].ID = static_cast<int>(i);
        data.WalkBehinds[i].ScriptName = room.WalkBehinds[i].ScriptName;
        data.WalkBehinds[i].Baseline = room.WalkBehinds[i].Baseline;
    }

    data.RegionCount = static_cast<uint32_t>(std::min<size_t>(room.Regions.size(), MAX_ROOM_REGIONS));
    for (uint32_t i = 0; i < data.RegionCount; ++i)
    {
        const auto &region = room.Regions[i];
        auto &dst = data.Regions[i];
        dst.ID = static_cast<int>(i);
        dst.ScriptName = region.ScriptName;
        if (region.UseColourTint)
        {
            dst.Tint =
                region.RedTint | (region.GreenTint << 8) | (region.BlueTint << 16) | (region.TintSaturation << 24);
            dst.Light = (region.TintLuminance * 25) / 10;
        }
        else
        {
            dst.Tint = 0;
            dst.Light = region.LightLevel - 100;
        }
        CompileProperties(region.Properties, dst.Properties, cp);
        dst.GetEvents().SetHandlers(StringMap{
            {"OnStanding", region.OnStanding}, {"OnWalksOff", region.OnWalksOff}, {"OnWalksOnto", region.OnWalksOnto}});
    }

    CompileProperties(room.Properties, data.Properties, cp);
    data.GetEvents().SetHandlers(StringMap{
        {"OnAfterFadeIn", room.OnAfterFadeIn},
        {"OnFirstTimeEnter", room.OnFirstTimeEnter},
        {"OnLeave", room.OnLeave},
        {"OnLeaveBottom", room.OnLeaveBottom},
        {"OnLeaveLeft", room.OnLeaveLeft},
        {"OnLeaveRight", room.OnLeaveRight},
        {"OnLeaveTop", room.OnLeaveTop},
        {"OnLoad", room.OnLoad},
        {"OnRepExec", room.OnRepExec},
        {"OnUnload", room.OnUnload},
    });

    // Encoding hint
    data.StrOptions["textencoding"].Format("%d", cp);
    return HError::None();
}

//-----------------------------------------------------------------------------
// Images
//-----------------------------------------------------------------------------

static HError LoadImageFile(const String &filename, PixelBuffer &image, Palette &pal)
{
    if (!File::IsFile(filename))
        return new Error(String::FromFormat("File not found: %s", filename.GetCStr()));
    image = ImageFile::LoadImage(filename, nullptr, pal.data());
    if (!image)
        return new Error(String::FromFormat("Failed to read image %s", filename.GetCStr()));
    return HError::None();
}

// Loads an area mask: an 8-bit image whose pixel values are area numbers. Values that no area
// has are cleared to 0, as the editor does (RoomsComponent.ValidateMask).
static HError LoadMask(const String &room_dir, const char *name, int max_areas, PixelBuffer &mask,
                       std::vector<String> &warnings)
{
    const String filename = Path::ConcatPaths(room_dir, String::FromFormat("%s.png", name));
    Palette pal;
    HError err = LoadImageFile(filename, mask, pal);
    if (!err)
        return err;
    if (mask.GetFormat() != kPxFmt_Indexed8)
        return new Error(String::FromFormat("%s must be an 8-bit image (indexed palette)", filename.GetCStr()));

    bool invalid = false;
    for (int y = 0; y < mask.GetHeight(); ++y)
    {
        uint8_t *line = mask.GetLine(y);
        for (int x = 0; x < mask.GetWidth(); ++x)
        {
            if (line[x] >= max_areas)
            {
                line[x] = 0;
                invalid = true;
            }
        }
    }
    if (invalid)
        warnings.push_back(String::FromFormat("%s uses colours beyond the first %d palette entries; those pixels "
                                              "were cleared to no area",
                                              filename.GetCStr(), max_areas));
    return HError::None();
}

HError LoadRoomImages(const String &room_dir, const RoomXml &room, const RoomBuildGame &game, RoomData &data,
                      std::vector<String> &warnings)
{
    if (game.ColorDepth == 8)
        return new Error("Building rooms of 256-colour games is not supported by the tools yet (the backgrounds "
                         "need the game palette); build the game in the editor.");

    // Backgrounds: the editor uses the ones it finds (RoomsComponent.LoadImageCache)
    std::vector<PixelBuffer> backgrounds;
    for (int i = 0; i < room.BackgroundCount; ++i)
    {
        const String filename = Path::ConcatPaths(room_dir, String::FromFormat("background%d.png", i));
        if (i > 0 && !File::IsFile(filename))
            continue;
        PixelBuffer image, converted;
        Palette pal, game_pal;
        HError err = LoadImageFile(filename, image, pal);
        if (!err)
            return err;
        err = ConvertRoomBackgroundForGame(image, &pal, converted, game_pal, game.ColorDepth);
        if (!err)
            return new Error(err, String::FromFormat("Failed to convert %s", filename.GetCStr()));
        backgrounds.push_back(std::move(converted));
    }
    if (backgrounds.empty())
        return new Error(
            String::FromFormat("File not found: %s", Path::ConcatPaths(room_dir, "background0.png").GetCStr()));

    data.BgFrameCount = static_cast<uint32_t>(backgrounds.size());
    data.BgFrames.resize(data.BgFrameCount);
    for (size_t i = 0; i < backgrounds.size(); ++i)
        data.BgFrames[i].GraphicBuf = std::move(backgrounds[i]);
    // RoomStruct.PrepareForWriteToFile
    data.BackgroundBPP = data.BgFrames[0].GraphicBuf.GetBytesPerPixel();
    std::copy(std::begin(data.BgFrames[0].Palette), std::end(data.BgFrames[0].Palette), std::begin(data.Palette));

    // Masks (RoomAreaMaskType names, lowercase)
    HError err = LoadMask(room_dir, "hotspots", MAX_ROOM_HOTSPOTS, data.HotspotMaskBuf, warnings);
    if (err)
        err = LoadMask(room_dir, "walkbehinds", MAX_WALK_BEHINDS, data.WalkBehindMaskBuf, warnings);
    if (err)
        err = LoadMask(room_dir, "walkableareas", MAX_WALK_AREAS, data.WalkAreaMaskBuf, warnings);
    if (err)
        err = LoadMask(room_dir, "regions", MAX_ROOM_REGIONS, data.RegionMaskBuf, warnings);
    return err;
}

void FinishRoomData(RoomData &data)
{
    // AGS.Native calculate_walkable_areas
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        data.WalkAreas[i].Top = data.Height;
        data.WalkAreas[i].Bottom = 0;
    }
    const PixelBuffer &mask = data.WalkAreaMaskBuf;
    for (int x = 0; x < mask.GetWidth(); ++x)
    {
        for (int y = 0; y < mask.GetHeight(); ++y)
        {
            const int area = mask.GetLine(y)[x];
            if (area >= MAX_WALK_AREAS)
                continue;
            data.WalkAreas[area].Top = std::min(data.WalkAreas[area].Top, y);
            data.WalkAreas[area].Bottom = std::max(data.WalkAreas[area].Bottom, y);
        }
    }
}

} // namespace DataUtil
} // namespace AGS

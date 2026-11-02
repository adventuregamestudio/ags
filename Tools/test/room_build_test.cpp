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
#include <cstring>
#include "gtest/gtest.h"
#include "data/room_build.h"
#include "data/room_xml.h"
#include "gfx/gfx_def.h"
#include "gfx/image_file.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

namespace
{

HError ReadRoom(const char *xml, RoomXml &room)
{
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml) != tinyxml2::XML_SUCCESS)
        return new Error("XML parse error");
    return ReadRoomXml(doc, room);
}

// PixelBuffer does not clear its memory
PixelBuffer ClearBuffer(int width, int height, PixelFormat fmt)
{
    PixelBuffer buf(width, height, fmt);
    std::memset(buf.GetData(), 0, buf.GetDataSize());
    return buf;
}

RoomBuildGame MakeGame()
{
    RoomBuildGame game;
    game.UniqueID = 1234;
    game.DefaultRoomMaskResolution = 1;
    // cursors 1, 2 and 8 create events, as Look, Interact and Usermode1
    game.InteractionEvents = {{1, "Curs0002"}, {2, "Curs0003"}, {8, "Curs0009"}};
    CustomPropertySchemaItem prop;
    prop.Name = "Weight";
    game.PropertySchema.push_back(prop);
    return game;
}

} // namespace

TEST(RoomXml, ReadsOlderNames)
{
    // Rooms saved by older editors have Name and Description instead of ScriptName and DisplayName
    RoomXml room;
    HError err = ReadRoom("<Room Version=\"4.0.0.27\"><Number>3</Number><Width>320</Width><Objects><RoomObject>"
                          "<Name>oDoor</Name><Description>A door</Description><Transparency>40</Transparency>"
                          "<GraphicAnchor>0.5, 2</GraphicAnchor><BlendMode>Screen</BlendMode>"
                          "</RoomObject></Objects><Regions><RoomRegion><LightLevel>150</LightLevel>"
                          "<TintSaturation>50</TintSaturation><TintLuminance>100</TintLuminance></RoomRegion>"
                          "</Regions></Room>",
                          room);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(3, room.Number);
    ASSERT_EQ(1u, room.Objects.size());
    EXPECT_STREQ("oDoor", room.Objects[0].ScriptName.GetCStr());
    EXPECT_STREQ("A door", room.Objects[0].DisplayName.GetCStr());
    EXPECT_EQ(40, room.Objects[0].Transparency);
    EXPECT_FLOAT_EQ(0.5f, room.Objects[0].GraphicAnchor.X);
    EXPECT_FLOAT_EQ(1.f, room.Objects[0].GraphicAnchor.Y); // clamped
    EXPECT_EQ(5, room.Objects[0].BlendMode);
    ASSERT_EQ(1u, room.Regions.size());
    EXPECT_EQ(150, room.Regions[0].LightLevel);
    EXPECT_EQ(50, room.Regions[0].TintSaturation);
    EXPECT_EQ(100, room.Regions[0].TintLuminance);

    // The names the current editor writes
    RoomXml current;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.27\"><Objects><RoomObject><ScriptName>oDoor</ScriptName>"
                         "<DisplayName>A door</DisplayName><Transparency>140</Transparency></RoomObject></Objects>"
                         "</Room>",
                         current));
    EXPECT_STREQ("oDoor", current.Objects[0].ScriptName.GetCStr());
    EXPECT_STREQ("A door", current.Objects[0].DisplayName.GetCStr());
    EXPECT_EQ(100, current.Objects[0].Transparency); // clamped
}

TEST(RoomXml, RejectsWhatTheEditorRejects)
{
    RoomXml room;
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.27\"><Unheard>1</Unheard></Room>", room));
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.99\" />", room));
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.27\"><Objects><RoomObject><ScriptName>1st</ScriptName>"
                          "</RoomObject></Objects></Room>",
                          room));
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.27\"><Regions><RoomRegion><TintSaturation>0</TintSaturation>"
                          "</RoomRegion></Regions></Room>",
                          room));
    // obsolete properties are skipped, as the editor does
    EXPECT_TRUE(ReadRoom("<Room Version=\"4.0.0.27\"><SaveLoadEnabled>True</SaveLoadEnabled></Room>", room));
}

TEST(RoomXml, ConvertsOldIndexedEvents)
{
    // Old object interactions were indexed by event: 4 was "any click", 0 "look at"
    RoomXml room;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.27\"><Objects><RoomObject><Interactions>"
                         "<Event Index=\"0\">oLook</Event><Event Index=\"4\">oAny</Event></Interactions>"
                         "</RoomObject></Objects><Interactions><Event Index=\"6\">roomRep</Event></Interactions>"
                         "<OnRepExec>ignored</OnRepExec></Room>",
                         room));
    const auto &obj = room.Objects[0];
    EXPECT_STREQ("oAny", obj.OnAnyClick.GetCStr());
    ASSERT_EQ(1u, obj.Interactions.ByIndex.size());
    EXPECT_STREQ("oLook", obj.Interactions.ByIndex.at(1).GetCStr()); // now by cursor ID: Look is cursor 1
    EXPECT_STREQ("roomRep", room.OnRepExec.GetCStr());
}

TEST(RoomBuild, InteractionsAreIndexedByCursor)
{
    RoomXml room;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.27\"><GameID>1234</GameID><Objects><RoomObject><Interactions>"
                         "<Event Index=\"2\" UID=\"Curs0003\">oUse</Event>"
                         "<Event Index=\"8\" UID=\"Curs0009\">oMode8</Event>"
                         "<Event Index=\"5\" UID=\"Curs0006\">oStale</Event></Interactions>"
                         "<OnAnyClick>oAny</OnAnyClick></RoomObject></Objects><Hotspots><RoomHotspot /></Hotspots>"
                         "</Room>",
                         room));
    RoomData data;
    ASSERT_TRUE(ConvertRoomXml(room, MakeGame(), data));
    const auto &handlers = data.Objects[0].Interactions.GetHandlers();
    // slots 0..8, by cursor ID; functions of cursors without an event are dropped
    ASSERT_EQ(9u, handlers.size());
    EXPECT_STREQ("", handlers[1].FunctionName.GetCStr());
    EXPECT_STREQ("oUse", handlers[2].FunctionName.GetCStr());
    EXPECT_STREQ("", handlers[5].FunctionName.GetCStr());
    EXPECT_STREQ("oMode8", handlers[8].FunctionName.GetCStr());
    EXPECT_STREQ("oAny", data.Objects[0].GetEvents().GetHandler(kRoomObjectEvent_AnyClick).FunctionName.GetCStr());
    // no functions: no table at all
    EXPECT_EQ(0u, data.Hotspots[0].Interactions.GetHandlers().size());
    EXPECT_EQ(1u, data.HotspotCount);
}

TEST(RoomBuild, FixupsBeforeSaving)
{
    RoomXml room;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.27\"><GameID>1</GameID><Properties>"
                         "<CustomProperty><Name>weight</Name><Value>5</Value></CustomProperty>"
                         "<CustomProperty><Name>Gone</Name><Value>1</Value></CustomProperty></Properties>"
                         "<WalkableAreas><RoomWalkableArea><UseContinuousScaling>True</UseContinuousScaling>"
                         "<MinScalingLevel>150</MinScalingLevel><MaxScalingLevel>50</MaxScalingLevel>"
                         "</RoomWalkableArea></WalkableAreas></Room>",
                         room));
    RoomData data;
    ASSERT_TRUE(ConvertRoomXml(room, MakeGame(), data));
    EXPECT_EQ(1234, data.GameID);          // room taken from another game
    ASSERT_EQ(1u, data.Properties.size()); // only what the schema has, with its spelling
    EXPECT_STREQ("5", data.Properties["Weight"].GetCStr());
    EXPECT_EQ(-50, data.WalkAreas[0].ScalingFar); // min and max were swapped
    EXPECT_EQ(50, data.WalkAreas[0].ScalingNear);

    // another game's room with a different mask resolution needs the editor
    room.MaskResolution = 2;
    EXPECT_FALSE(ConvertRoomXml(room, MakeGame(), data));
}

TEST(RoomBuild, ValuesInEngineRanges)
{
    // as convert_room_to_native: transparency 0-100 to the legacy 0-255 transparency, region light
    // level 0-200 to -100..100, tint luminance 0-100 to 0-250
    RoomXml room;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.27\"><GameID>1234</GameID><Objects>"
                         "<RoomObject><Transparency>0</Transparency></RoomObject>"
                         "<RoomObject><Transparency>40</Transparency></RoomObject>"
                         "<RoomObject><Transparency>100</Transparency></RoomObject></Objects><Regions>"
                         "<RoomRegion><LightLevel>150</LightLevel></RoomRegion>"
                         "<RoomRegion><UseColourTint>True</UseColourTint><RedTint>10</RedTint><GreenTint>20</GreenTint>"
                         "<BlueTint>30</BlueTint><TintSaturation>40</TintSaturation><TintLuminance>60</TintLuminance>"
                         "</RoomRegion></Regions></Room>",
                         room));
    RoomData data;
    ASSERT_TRUE(ConvertRoomXml(room, MakeGame(), data));
    EXPECT_EQ(0, data.Objects[0].Transparency);
    EXPECT_EQ(GfxDef::Trans100ToLegacyTrans255(40), data.Objects[1].Transparency);
    EXPECT_EQ(255, data.Objects[2].Transparency);
    EXPECT_EQ(50, data.Regions[0].Light);
    EXPECT_EQ(0, data.Regions[0].Tint);
    EXPECT_EQ(150, data.Regions[1].Light);
    EXPECT_EQ(10 | (20 << 8) | (30 << 16) | (40 << 24), data.Regions[1].Tint);
}

TEST(RoomBuild, TextProperties)
{
    EXPECT_STREQ("caf\xC3\xA9 \xE2\x82\xAC", ConvertTextProperty("caf\xC3\xA9 \xE2\x82\xAC", 65001).GetCStr());
    EXPECT_STREQ("caf\xE9 \x80 ?", ConvertTextProperty("caf\xC3\xA9 \xE2\x82\xAC \xE2\x98\x83", 1252).GetCStr());
    EXPECT_STREQ("a\nb \\[c", ConvertTextProperty("a\\nb \\[c", 65001).GetCStr());
}

TEST(RoomBuild, ScriptHeader)
{
    RoomScriptNames names;
    names.ObjectNames = {"oDoor", ""};
    names.HotspotNames = {"", "hWall"};
    names.RegionNames = {"rDark"};
    names.WalkableAreaNames = {"wFloor"};
    names.WalkBehindNames = {"bTree"};
    EXPECT_STREQ("import readonly Object *oDoor;\n"
                 "import readonly Hotspot *hWall;\n"
                 "import readonly Region *rDark;\n"
                 "import readonly WalkableArea *wFloor;\n"
                 "import readonly Walkbehind *bTree;\n",
                 MakeRoomScriptHeader(names).GetCStr());
}

TEST(RoomBuild, WalkableAreaExtents)
{
    RoomData data;
    data.Height = 10;
    data.WalkAreaMaskBuf = ClearBuffer(4, 6, kPxFmt_Indexed8);
    data.WalkAreaMaskBuf.GetLine(2)[1] = 3;
    data.WalkAreaMaskBuf.GetLine(4)[3] = 3;
    data.WalkAreaMaskBuf.GetLine(5)[0] = 200; // not an area
    FinishRoomData(data);
    EXPECT_EQ(2, data.WalkAreas[3].Top);
    EXPECT_EQ(4, data.WalkAreas[3].Bottom);
    EXPECT_EQ(10, data.WalkAreas[5].Top); // no pixels: room height and 0
    EXPECT_EQ(0, data.WalkAreas[5].Bottom);
}

TEST(RoomBuild, LoadsImages)
{
    const String dir = Path::ConcatPaths(Directory::GetCurrentDirectory(), "room_build_test");
    Directory::CreateDirectory(dir);
    RGB pal[256] = {};
    PixelBuffer bg = ClearBuffer(4, 4, kPxFmt_A8R8G8B8);
    reinterpret_cast<uint32_t *>(bg.GetLine(0))[0] = 0x00123456; // fully transparent
    ASSERT_TRUE(ImageFile::SaveImage(bg, false, nullptr, Path::ConcatPaths(dir, "background0.png")));
    PixelBuffer mask = ClearBuffer(4, 4, kPxFmt_Indexed8);
    mask.GetLine(1)[1] = 20; // beyond the 16 regions
    for (const char *name : {"hotspots", "walkbehinds", "walkableareas", "regions"})
        ASSERT_TRUE(ImageFile::SaveImage(mask, false, pal, Path::ConcatPaths(dir, String::FromFormat("%s.png", name))));

    RoomXml room;
    room.BackgroundCount = 2; // background1.png is missing: the editor uses the ones it finds
    RoomData data;
    std::vector<String> warnings;
    HError err = LoadRoomImages(dir, room, MakeGame(), data, warnings);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(1u, data.BgFrameCount);
    EXPECT_EQ(4, data.BackgroundBPP);
    EXPECT_EQ(0xFF123456u, reinterpret_cast<const uint32_t *>(data.BgFrames[0].GraphicBuf.GetLine(0))[0]); // opaque
    EXPECT_EQ(20, data.HotspotMaskBuf.GetLine(1)[1]); // within the 50 hotspots
    EXPECT_EQ(0, data.RegionMaskBuf.GetLine(1)[1]);   // cleared
    EXPECT_EQ(3u, warnings.size());

    File::DeleteFile(Path::ConcatPaths(dir, "hotspots.png"));
    EXPECT_FALSE(LoadRoomImages(dir, room, MakeGame(), data, warnings));

    for (const char *name : {"background0", "walkbehinds", "walkableareas", "regions"})
        File::DeleteFile(Path::ConcatPaths(dir, String::FromFormat("%s.png", name)));
    Directory::DeleteDirectory(dir);
}

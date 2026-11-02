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
#include "gtest/gtest.h"
#include "data/xml_fields.h"
#include "util/string_types.h"

using namespace AGS;
using namespace AGS::AGF;
using AGS::Common::HError;
using AGS::Common::String;

namespace
{

struct Sample
{
    int Count = 7; // a default that a missing element must keep
    bool Enabled = false;
    float Ratio = 0.f;
    String Name;
    Point Where;
    XmlRect Area;
    int Mode = 0;
    std::vector<Sample> Children;
};

const AGS::Common::CstrArr<3> kModeNames = {{"Off", "Slow", "Fast"}};

bool ParseMode(const char *text, int &value)
{
    return ParseXmlEnum(text, kModeNames, value);
}

HError ReadChildren(const tinyxml2::XMLElement *elem, std::vector<Sample> &children);

const XmlField<Sample> kSampleFields[] = {
    {"Area", XML_READ(&Sample::Area)},
    {"Children", XML_READ_ELEMENT(&Sample::Children, ReadChildren)},
    {"Count", XML_READ(&Sample::Count)},
    {"Enabled", XML_READ(&Sample::Enabled)},
    {"Mode", XML_READ_WITH(&Sample::Mode, ParseMode)},
    {"Name", XML_READ(&Sample::Name)},
    {"Obsolete", nullptr},
    {"Ratio", XML_READ(&Sample::Ratio)},
    {"Where", XML_READ(&Sample::Where)},
};

HError ReadChildren(const tinyxml2::XMLElement *elem, std::vector<Sample> &children)
{
    return ReadList(elem, "Sample", children, kSampleFields, 2);
}

HError ReadSample(const char *xml, Sample &sample, UnknownElements unknown = UnknownElements::Fail)
{
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml) != tinyxml2::XML_SUCCESS)
        return new AGS::Common::Error("XML parse error");
    return ReadFields(doc.RootElement(), sample, kSampleFields, unknown);
}

} // namespace

TEST(XmlFields, ReadsEveryKindOfRow)
{
    Sample s;
    HError err = ReadSample("<Sample><Enabled>True</Enabled><Ratio>0.25</Ratio><Name>Ab c</Name>"
                            "<Where>3,-4</Where><Area>1,2,30,40</Area><Mode>Fast</Mode>"
                            "<Obsolete>whatever</Obsolete></Sample>",
                            s);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(7, s.Count); // missing element: default kept
    EXPECT_TRUE(s.Enabled);
    EXPECT_FLOAT_EQ(0.25f, s.Ratio);
    EXPECT_STREQ("Ab c", s.Name.GetCStr());
    EXPECT_EQ(3, s.Where.X);
    EXPECT_EQ(-4, s.Where.Y);
    EXPECT_EQ(1, s.Area.X);
    EXPECT_EQ(2, s.Area.Y);
    EXPECT_EQ(30, s.Area.Width);
    EXPECT_EQ(40, s.Area.Height);
    EXPECT_EQ(2, s.Mode);
}

TEST(XmlFields, ReadsNestedLists)
{
    Sample s;
    HError err = ReadSample("<Sample><Children><Sample><Count>1</Count></Sample><Sample><Count>2</Count></Sample>"
                            "<Sample><Count>3</Count></Sample></Children></Sample>",
                            s);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    ASSERT_EQ(2u, s.Children.size()); // the list keeps at most 2, the rest is dropped
    EXPECT_EQ(1, s.Children[0].Count);
    EXPECT_EQ(2, s.Children[1].Count);
}

TEST(XmlFields, UnknownElements)
{
    Sample s;
    HError err = ReadSample("<Sample><Count>5</Count><Surprise>1</Surprise></Sample>", s);
    ASSERT_FALSE(err);
    EXPECT_NE(nullptr, std::strstr(err->FullMessage().GetCStr(), "<Surprise>"));

    Sample skipped;
    EXPECT_TRUE(ReadSample("<Sample><Count>5</Count><Surprise>1</Surprise></Sample>", skipped, UnknownElements::Skip));
    EXPECT_EQ(5, skipped.Count);

    Sample in_list;
    EXPECT_FALSE(ReadSample("<Sample><Children><Other /></Children></Sample>", in_list));
}

TEST(XmlFields, InvalidValues)
{
    Sample s;
    EXPECT_FALSE(ReadSample("<Sample><Count>12x</Count></Sample>", s));
    EXPECT_FALSE(ReadSample("<Sample><Count /></Sample>", s));
    EXPECT_FALSE(ReadSample("<Sample><Enabled>yes</Enabled></Sample>", s));
    EXPECT_FALSE(ReadSample("<Sample><Where>3</Where></Sample>", s));
    EXPECT_FALSE(ReadSample("<Sample><Mode>Medium</Mode></Sample>", s));
    HError err = ReadSample("<Sample>\n<Count>99999999999</Count></Sample>", s);
    ASSERT_FALSE(err);
    EXPECT_NE(nullptr, std::strstr(err->FullMessage().GetCStr(), "line 2"));
}

TEST(XmlFields, ValueParsersFollowTheEditor)
{
    int i = 0;
    EXPECT_TRUE(ParseXmlValue(" -12 ", i));
    EXPECT_EQ(-12, i);
    EXPECT_TRUE(ParseXmlValue("0xFFA8A8A8", i)); // colors: all 32 bits, as Convert.ToInt32(s, 16)
    EXPECT_EQ(static_cast<int>(0xFFA8A8A8u), i);
    EXPECT_FALSE(ParseXmlValue("0x1FFFFFFFF", i));
    bool b = false;
    EXPECT_TRUE(ParseXmlValue("true", b)); // Convert.ToBoolean ignores case
    EXPECT_TRUE(b);
    EXPECT_TRUE(ParseXmlValue("FALSE", b));
    EXPECT_FALSE(b);
    int mode = -1;
    EXPECT_TRUE(ParseXmlEnum("1", kModeNames, mode)); // Enum.Parse takes numbers too
    EXPECT_EQ(1, mode);
    EXPECT_FALSE(ParseXmlEnum("fast", kModeNames, mode)); // but names are case-sensitive
    String str;
    EXPECT_TRUE(ParseXmlValue("", str));
    EXPECT_TRUE(str.IsEmpty());
}

TEST(XmlFields, CompareXmlVersions)
{
    EXPECT_EQ(0, CompareXmlVersions("4.0.0.34", "4.0.0.34"));
    EXPECT_LT(CompareXmlVersions("4.0.0.27", "4.0.0.34"), 0);
    EXPECT_GT(CompareXmlVersions("4.0.0.100", "4.0.0.34"), 0); // numbers, not text
    EXPECT_LT(CompareXmlVersions("3.99.99.01", "4.0.0.0"), 0);
    EXPECT_EQ(0, CompareXmlVersions("4.1", "4.1.0.0"));
}

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
#include <memory>
#include <vector>

#include "data/tra_utils.h"
#include "gtest/gtest.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

namespace
{

// A PO source with each of the cases ReadPO handles
const char *kPO = R"PO(# AGS TRANSLATION SOURCE FILE
# $GameID=1234
# $GameName=Game
# $NormalFont=2
# $SpeechFont=DEFAULT
# $TextDirection=RIGHT
# $Encoding=UTF-8
# $Language=it
# $AutoTranslateParserSaid=ON
# $Font1=Font3;Size=12;Outline=AUTO;AutoOutline=ROUND;AutoOutlineThickness=2;HeightDefinition=CUSTOM;CustomHeight=9;VerticalOffset=1;LineSpacing=4;CharacterSpacing=1;
# $Font0=File=x.ttf;SizeMultiplier=2;
msgid ""
msgstr ""
"Language: it\n"

msgid "Hello"
msgstr "Ciao"

#: GlobalScript.asc:10
msgid "Two "
"lines"
msgstr "Due "
"righe"

#, fuzzy
msgid "Unsure"
msgstr "Incerto"

#~ msgid "Gone"
#~ msgstr "Sparito"

msgctxt "OBSOLETE"
msgid "Old"
msgstr "Vecchio"

msgctxt "PARSERWORD:5"
msgid "look"
msgstr "guarda, osserva"

msgid "Apple"
msgid_plural "Apples"
msgstr[0] "Mela"
msgstr[1] "Mele"

msgid "Line\nbreak \"quoted\" \101"
msgstr "Caff\303\251\tbar"

msgid "Untranslated"
msgstr ""

msgid "Hello"
msgstr "Salve")PO";

std::unique_ptr<Stream> MakeReadStream(const std::vector<uint8_t> &data)
{
    return std::make_unique<Stream>(std::make_unique<VectorStream>(data));
}

Translation ReadTestPO()
{
    std::vector<uint8_t> data(kPO, kPO + strlen(kPO));
    Translation tra;
    EXPECT_TRUE(ReadPO(tra, MakeReadStream(data)));
    return tra;
}

Translation ReadTRA(const std::vector<uint8_t> &data)
{
    Translation tra;
    HError err = ReadTraData(tra, MakeReadStream(data));
    EXPECT_TRUE(err) << err->FullMessage().GetCStr();
    return tra;
}

std::vector<uint8_t> WriteTestTRA(const Translation &tra)
{
    std::vector<uint8_t> data;
    EXPECT_TRUE(WriteTRA(tra, std::make_unique<Stream>(std::make_unique<VectorStream>(data, kStream_Write))));
    return data;
}

std::vector<uint8_t> WriteTestPO(const Translation &tra)
{
    std::vector<uint8_t> data;
    EXPECT_TRUE(WritePO(tra, std::make_unique<Stream>(std::make_unique<VectorStream>(data, kStream_Write))));
    return data;
}

void ExpectSameTranslation(const Translation &tra, const Translation &again)
{
    EXPECT_EQ(tra.GameUid, again.GameUid);
    EXPECT_STREQ(tra.GameName.GetCStr(), again.GameName.GetCStr());
    EXPECT_EQ(tra.Dict, again.Dict);
    EXPECT_EQ(tra.ParserDict.GetWords(), again.ParserDict.GetWords());
    EXPECT_EQ(tra.NormalFont, again.NormalFont);
    EXPECT_EQ(tra.SpeechFont, again.SpeechFont);
    EXPECT_EQ(tra.RightToLeft, again.RightToLeft);
    EXPECT_EQ(tra.OptFlags, again.OptFlags);
    EXPECT_STREQ(tra.EncodingHint.GetCStr(), again.EncodingHint.GetCStr());
    EXPECT_STREQ(tra.LanguageHint.GetCStr(), again.LanguageHint.GetCStr());
    ASSERT_EQ(tra.FontOverrides.size(), again.FontOverrides.size());
    for (const auto &fover : tra.FontOverrides)
    {
        ASSERT_EQ(1u, again.FontOverrides.count(fover.first));
        const FontOverride &a = fover.second;
        const FontOverride &b = again.FontOverrides.at(fover.first);
        EXPECT_EQ(a.Fields, b.Fields);
        EXPECT_EQ(a.Finfo.FontID, b.Finfo.FontID);
        EXPECT_STREQ(a.Finfo.FileName.GetCStr(), b.Finfo.FileName.GetCStr());
        EXPECT_EQ(a.Finfo.Flags, b.Finfo.Flags);
        EXPECT_EQ(a.Finfo.Size, b.Finfo.Size);
        EXPECT_EQ(a.Finfo.SizeMultiplier, b.Finfo.SizeMultiplier);
        EXPECT_EQ(a.Finfo.Outline, b.Finfo.Outline);
        EXPECT_EQ(a.Finfo.AutoOutlineStyle, b.Finfo.AutoOutlineStyle);
        EXPECT_EQ(a.Finfo.AutoOutlineThickness, b.Finfo.AutoOutlineThickness);
        EXPECT_EQ(a.Finfo.CustomHeight, b.Finfo.CustomHeight);
        EXPECT_EQ(a.Finfo.YOffset, b.Finfo.YOffset);
        EXPECT_EQ(a.Finfo.LineSpacing, b.Finfo.LineSpacing);
        EXPECT_EQ(a.Finfo.CharacterSpacing, b.Finfo.CharacterSpacing);
    }
}

} // namespace

TEST(TraUtils, ReadPO)
{
    Translation tra = ReadTestPO();
    EXPECT_EQ(1234, tra.GameUid);
    EXPECT_STREQ("Game", tra.GameName.GetCStr());
    EXPECT_EQ(2, tra.NormalFont);
    EXPECT_EQ(-1, tra.SpeechFont);
    EXPECT_EQ(kTextDirection_RTL, tra.RightToLeft);
    EXPECT_STREQ("UTF-8", tra.EncodingHint.GetCStr());
    EXPECT_STREQ("it", tra.LanguageHint.GetCStr());
    EXPECT_EQ(kTraOpt_AutoTranslateSaid, tra.OptFlags);
    EXPECT_EQ(2u, tra.FontOverrides.size());

    // the header, the obsolete and the untranslated entries are left out,
    // and parser words go to their own dictionary
    EXPECT_EQ(5u, tra.Dict.size());
    // a text found again keeps its first translation, same as with TRS
    EXPECT_STREQ("Ciao", tra.Dict["Hello"].GetCStr());
    // strings split on several lines are joined
    EXPECT_STREQ("Due righe", tra.Dict["Two lines"].GetCStr());
    // flags do not matter
    EXPECT_STREQ("Incerto", tra.Dict["Unsure"].GetCStr());
    // only the first plural form
    EXPECT_STREQ("Mela", tra.Dict["Apple"].GetCStr());
    // the texts are unescaped
    EXPECT_STREQ("Caff\xC3\xA9\tbar", tra.Dict["Line\nbreak \"quoted\" A"].GetCStr());
    EXPECT_EQ(0u, tra.Dict.count("Gone"));
    EXPECT_EQ(0u, tra.Dict.count("Old"));
    EXPECT_EQ(0u, tra.Dict.count("Untranslated"));
    EXPECT_EQ(0u, tra.Dict.count("look"));
    const auto &words = tra.ParserDict.GetWords();
    ASSERT_EQ(2u, words.size());
    EXPECT_EQ(5, words.find("guarda")->second);
    EXPECT_EQ(5, words.find("osserva")->second);
}

TEST(TraUtils, POCompilesToTRA)
{
    const Translation tra = ReadTestPO();
    ExpectSameTranslation(tra, ReadTRA(WriteTestTRA(tra)));
}

TEST(TraUtils, WritePO)
{
    Translation tra = ReadTestPO();
    tra.Dict["Back\\slash \"quoted\"\r\n\x01 end"] = "Tab\there";
    // the lines end in "\r\n" on Windows
    String po;
    for (const uint8_t c : WriteTestPO(tra))
    {
        if (c != '\r')
            po.AppendChar(static_cast<char>(c));
    }
    // compared with "!=", as EXPECT_NE takes a reference to the undefined String::NoIndex
    EXPECT_TRUE(po.FindString("# $GameID=1234\n") != String::NoIndex);
    EXPECT_TRUE(po.FindString("# $NormalFont=2\n") != String::NoIndex);
    EXPECT_TRUE(po.FindString("# $TextDirection=RIGHT\n") != String::NoIndex);
    EXPECT_TRUE(po.FindString("# $Font0=File=x.ttf;SizeMultiplier=2;\n") != String::NoIndex);
    EXPECT_TRUE(po.FindString("\"Content-Type: text/plain; charset=UTF-8\\n\"\n") != String::NoIndex);
    // the texts are escaped
    EXPECT_TRUE(po.FindString("\nmsgid \"Back\\\\slash \\\"quoted\\\"\\r\\n\\001 end\"\nmsgstr \"Tab\\there\"\n") !=
                String::NoIndex);
    // one entry per parser word ID, with a placeholder text the PO reader keeps apart
    EXPECT_TRUE(po.FindString("\nmsgctxt \"PARSERWORD:5\"\nmsgid \"??? (word 5)\"\nmsgstr \"guarda, osserva\"\n") !=
                String::NoIndex);
}

TEST(TraUtils, WritePOCompilesToTheSameTranslation)
{
    // TRA -> PO -> TRA
    Translation tra = ReadTestPO();
    tra.Dict["Back\\slash \"quoted\"\r\n\x01 end"] = "Tab\there";
    tra = ReadTRA(WriteTestTRA(tra));

    Translation from_po;
    ASSERT_TRUE(ReadPO(from_po, MakeReadStream(WriteTestPO(tra))));
    ExpectSameTranslation(tra, from_po);
    ExpectSameTranslation(tra, ReadTRA(WriteTestTRA(from_po)));
}

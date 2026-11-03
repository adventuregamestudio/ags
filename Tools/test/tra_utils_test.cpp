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

// A PO source with each of the rules the editor follows (Translation.LoadDataImpl)
const char *kPO = R"PO(# AGS TRANSLATION SOURCE FILE
# $GameName=From the source
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

msgctxt "PARSERWORD:5"
msgid "look"
msgstr "guarda, osserva"

msgid "Apple"
msgid_plural "Apples"
msgstr[0] "Mela"
msgstr[1] "Mele"

msgid "Line\nbreak \"quoted\" \x41\101"
msgstr "Caff\xe9\tbar"

msgid "Untranslated"
msgstr ""

msgid "Hello"
msgstr "Salve")PO";

std::unique_ptr<Stream> MakeReadStream(const std::vector<uint8_t> &data)
{
    return std::make_unique<Stream>(std::make_unique<VectorStream>(data));
}

TranslationSource ReadTestPO()
{
    std::vector<uint8_t> data(kPO, kPO + strlen(kPO));
    TranslationSource src;
    EXPECT_TRUE(ReadPO(src, MakeReadStream(data)));
    return src;
}

std::vector<uint8_t> WriteTestTRA(TranslationSource &src)
{
    src.Options.GameUid = 1234;
    src.Options.GameName = "Game";
    std::vector<uint8_t> data;
    EXPECT_TRUE(WriteTRA(src, std::make_unique<Stream>(std::make_unique<VectorStream>(data, kStream_Write))));
    return data;
}

} // namespace

TEST(TraUtils, ReadPO)
{
    const TranslationSource src = ReadTestPO();
    EXPECT_EQ(2, src.Options.NormalFont);
    EXPECT_EQ(-1, src.Options.SpeechFont);
    EXPECT_EQ(kTextDirection_RTL, src.Options.RightToLeft);
    EXPECT_STREQ("UTF-8", src.Options.EncodingHint.GetCStr());
    EXPECT_STREQ("it", src.Options.LanguageHint.GetCStr());
    EXPECT_EQ(kTraOpt_AutoTranslateSaid, src.Options.OptFlags);

    // in the order of the file; the header is not an entry
    ASSERT_EQ(8u, src.Entries.size());
    // a text found again keeps its first place, with the later translation
    EXPECT_STREQ("Hello", src.Entries[0].Key.GetCStr());
    EXPECT_STREQ("Salve", src.Entries[0].Value.GetCStr());
    // strings split on several lines are joined
    EXPECT_STREQ("Two lines", src.Entries[1].Key.GetCStr());
    EXPECT_STREQ("Due righe", src.Entries[1].Value.GetCStr());
    EXPECT_TRUE(src.Entries[2].Fuzzy);
    EXPECT_TRUE(src.Entries[3].Obsolete);
    EXPECT_STREQ("Gone", src.Entries[3].Key.GetCStr());
    EXPECT_EQ(5, src.Entries[4].ParserWordID);
    // only the first plural form
    EXPECT_STREQ("Mela", src.Entries[5].Value.GetCStr());
    // the texts stay escaped until they are compiled
    EXPECT_STREQ("Line\\nbreak \\\"quoted\\\" \\x41\\101", src.Entries[6].Key.GetCStr());
    EXPECT_TRUE(src.Entries[7].Value.IsEmpty());
}

TEST(TraUtils, WriteTRAIsReadByTheEngine)
{
    TranslationSource src = ReadTestPO();
    const std::vector<uint8_t> data = WriteTestTRA(src);

    Translation tra;
    HError err = ReadTraData(tra, MakeReadStream(data));
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(1234, tra.GameUid);
    EXPECT_STREQ("Game", tra.GameName.GetCStr());
    // Fuzzy, obsolete and untranslated entries are left out, and parser words go to their own block
    EXPECT_EQ(4u, tra.Dict.size());
    EXPECT_STREQ("Salve", tra.Dict["Hello"].GetCStr());
    EXPECT_STREQ("Due righe", tra.Dict["Two lines"].GetCStr());
    EXPECT_STREQ("Mela", tra.Dict["Apple"].GetCStr());
    // escapes are resolved as the script compiler does, numeric ones in the translation's encoding
    EXPECT_STREQ("Caff\xC3\xA9\tbar", tra.Dict["Line\nbreak \"quoted\" AA"].GetCStr());
    EXPECT_EQ(0u, tra.Dict.count("Unsure"));
    EXPECT_EQ(0u, tra.Dict.count("Gone"));
    const auto &words = tra.ParserDict.GetWords();
    ASSERT_EQ(2u, words.size());
    EXPECT_EQ(5, words.find("guarda")->second);
    EXPECT_EQ(5, words.find("osserva")->second);
    EXPECT_EQ(2, tra.NormalFont);
    EXPECT_EQ(-1, tra.SpeechFont);
    EXPECT_EQ(kTextDirection_RTL, tra.RightToLeft);
    EXPECT_EQ(kTraOpt_AutoTranslateSaid, tra.OptFlags);
    EXPECT_STREQ("UTF-8", tra.StrOptions["encoding"].GetCStr());
    EXPECT_STREQ("it", tra.StrOptions["language"].GetCStr());
}

TEST(TraUtils, WriteTRAAsTheEditor)
{
    TranslationSource src = ReadTestPO();
    const std::vector<uint8_t> data = WriteTestTRA(src);

    // The signature, then the game ID block: its number and length, the game's ID, and its name
    // without a terminator, as the editor writes the texts
    const size_t sig_len = sizeof("AGSTranslation");
    ASSERT_GT(data.size(), sig_len + 20);
    EXPECT_EQ(0, memcmp(data.data(), "AGSTranslation", sig_len));
    auto int_at = [&data](size_t at)
    {
        int32_t v;
        memcpy(&v, data.data() + at, sizeof(v));
        return v;
    };
    EXPECT_EQ(kTraFblk_GameID, int_at(sig_len));
    EXPECT_EQ(1234, int_at(sig_len + 8));
    EXPECT_EQ(4, int_at(sig_len + 12)); // "Game"
    // the end of the block list, followed by a zero
    EXPECT_EQ(kTraFile_EOF, int_at(data.size() - 8));
    EXPECT_EQ(0, int_at(data.size() - 4));
}

namespace
{

Translation ReadTRA(const std::vector<uint8_t> &data)
{
    Translation tra;
    HError err = ReadTraData(tra, MakeReadStream(data));
    EXPECT_TRUE(err) << err->FullMessage().GetCStr();
    return tra;
}

// A compiled translation of the test PO, with texts that need escaping in a PO
Translation MakeTestTranslation()
{
    TranslationSource src = ReadTestPO();
    Translation tra = ReadTRA(WriteTestTRA(src));
    tra.Dict["Back\\slash \"quoted\"\r\n\x01 end"] = "Tab\there";
    return tra;
}

std::vector<uint8_t> WriteTestPO(const Translation &tra)
{
    std::vector<uint8_t> data;
    EXPECT_TRUE(WritePO(tra, std::make_unique<Stream>(std::make_unique<VectorStream>(data, kStream_Write))));
    return data;
}

} // namespace

TEST(TraUtils, WritePOAsTheEditor)
{
    // the lines end in "\r\n" on Windows
    String po;
    for (const uint8_t c : WriteTestPO(MakeTestTranslation()))
    {
        if (c != '\r')
            po.AppendChar(static_cast<char>(c));
    }
    // compared with "!=", as EXPECT_NE takes a reference to the undefined String::NoIndex
    EXPECT_TRUE(po.FindString("# $NormalFont=2\n") != String::NoIndex);
    EXPECT_TRUE(po.FindString("# $TextDirection=RIGHT\n") != String::NoIndex);
    EXPECT_TRUE(po.FindString("# $Font0=File=x.ttf;SizeMultiplier=2;\n") != String::NoIndex);
    EXPECT_TRUE(po.FindString("\"Content-Type: text/plain; charset=UTF-8\\n\"\n") != String::NoIndex);
    // the texts are escaped, and a "\r" is kept
    EXPECT_TRUE(po.FindString("\nmsgid \"Back\\\\slash \\\"quoted\\\"\\r\\n\\001 end\"\nmsgstr \"Tab\\there\"\n") !=
                String::NoIndex);
    // one entry per parser word ID, with a placeholder text the PO reader keeps apart
    EXPECT_TRUE(po.FindString("\nmsgctxt \"PARSERWORD:5\"\nmsgid \"??? (word 5)\"\nmsgstr \"guarda, osserva\"\n") !=
                String::NoIndex);
}

TEST(TraUtils, WritePOCompilesToTheSameTranslation)
{
    const Translation tra = MakeTestTranslation();
    TranslationSource src;
    ASSERT_TRUE(ReadPO(src, MakeReadStream(WriteTestPO(tra))));
    std::vector<uint8_t> tra_data;
    ASSERT_TRUE(WriteTRA(src, std::make_unique<Stream>(std::make_unique<VectorStream>(tra_data, kStream_Write))));
    const Translation again = ReadTRA(tra_data);

    EXPECT_EQ(tra.GameUid, again.GameUid);
    EXPECT_STREQ(tra.GameName.GetCStr(), again.GameName.GetCStr());
    EXPECT_EQ(tra.Dict, again.Dict);
    EXPECT_EQ(tra.ParserDict.GetWords(), again.ParserDict.GetWords());
    EXPECT_EQ(tra.NormalFont, again.NormalFont);
    EXPECT_EQ(tra.SpeechFont, again.SpeechFont);
    EXPECT_EQ(tra.RightToLeft, again.RightToLeft);
    EXPECT_EQ(tra.OptFlags, again.OptFlags);
    EXPECT_EQ(tra.StrOptions, again.StrOptions);
    ASSERT_EQ(2u, again.FontOverrides.size());
    for (const auto &fover : tra.FontOverrides)
    {
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

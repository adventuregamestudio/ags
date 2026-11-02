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
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <tinyxml2.h>
#include "gtest/gtest.h"
#include "data/agfreader.h"
#include "data/scriptgen.h"
#include "data/xml_fields.h"

using namespace AGS;

// Writes a minimal Game.agf with the given root attributes and tries to open it
static Common::HError OpenAgfWithRoot(const char *root_attributes)
{
    const char *path = "agfreader_test_version.agf";
    {
        std::ofstream f(path);
        f << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<AGSEditorDocument " << root_attributes
          << ">\n  <Game />\n</AGSEditorDocument>\n";
    }
    AGF::AGFReader reader;
    Common::HError err = reader.Open(path);
    reader.Close();
    std::remove(path);
    return err;
}

TEST(AGFReader, OpenAcceptsCurrentProjectFormat)
{
    Common::HError err = OpenAgfWithRoot("Version=\"4.0.0.33\" EditorVersion=\"4.00.00.33\"");
    EXPECT_TRUE(err) << (err ? "" : err->FullMessage().GetCStr());
}

TEST(AGFReader, OpenRejectsFormatsThatNeedAnUpgrade)
{
    // Legacy tag (Editor up to 4.0.0.19), and a version-string format older than the readable minimum:
    // both have to be opened and saved in the Editor first
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"3.0.3.2\" VersionIndex=\"26\""));
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"4.0.0.32\""));
}

TEST(AGFReader, OpenRejectsNewerFormats)
{
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"4.0.0.34\""));
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"4.1.0.0\""));
}

// Parses a single XML element and reads its script name with the given parser
static Common::String ReadScriptNameOf(AGF::EntityParser &parser, const char *xml)
{
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml) != tinyxml2::XML_SUCCESS)
        return "<parse error>";
    return parser.ReadScriptName(doc.RootElement());
}

TEST(AGFReader, EntityScriptNames)
{
    // These entities are saved with a <ScriptName> element since the AGS 4 project format;
    // the script header generator declares them by it
    AGF::Dialog dialog;
    AGF::GUIControl control;
    AGF::InventoryItem item;
    AGF::View view;
    EXPECT_STREQ("dDialog1",
                 ReadScriptNameOf(dialog, "<Dialog><ID>0</ID><ScriptName>dDialog1</ScriptName></Dialog>").GetCStr());
    EXPECT_STREQ(
        "Button2",
        ReadScriptNameOf(control, "<GUIButton><ID>1</ID><ScriptName>Button2</ScriptName></GUIButton>").GetCStr());
    EXPECT_STREQ("iKey", ReadScriptNameOf(item, "<InventoryItem><ID>1</ID><DisplayName /><ScriptName>iKey</ScriptName>"
                                                "</InventoryItem>")
                             .GetCStr());
    EXPECT_STREQ("VWALK", ReadScriptNameOf(view, "<View><ID>2</ID><ScriptName>VWALK</ScriptName></View>").GetCStr());
}

TEST(AGFReader, EnumReadSkipSpeech)
{
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrKeyboardOrTimer,
        AGF::ReadSkipSpeech("MouseOrKeyboardOrTimer"));
    EXPECT_EQ(DataUtil::kSkipSpeech_KeyboardOrTimer,
        AGF::ReadSkipSpeech("KeyboardOnly"));
    EXPECT_EQ(DataUtil::kSkipSpeech_TimerOnly,
        AGF::ReadSkipSpeech("TimerOnly"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrKeyboard,
        AGF::ReadSkipSpeech("MouseOrKeyboard"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrTimer,
        AGF::ReadSkipSpeech("MouseOnly"));
    EXPECT_EQ(DataUtil::kSkipSpeech_KeyboardOnly,
        AGF::ReadSkipSpeech("KeyboardOnlyStrict"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOnly,
        AGF::ReadSkipSpeech("MouseOnlyStrict"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrKeyboardOrTimer,
        AGF::ReadSkipSpeech("NotARealValue"));
}

TEST(AGFReader, EnumReadLipSyncType)
{
    EXPECT_EQ(DataUtil::kLipSync_None, AGF::ReadLipSyncType("None"));
    EXPECT_EQ(DataUtil::kLipSync_Text, AGF::ReadLipSyncType("Text"));
    EXPECT_EQ(DataUtil::kLipSync_PamelaVoiceFiles,
        AGF::ReadLipSyncType("PamelaVoiceFiles"));
    EXPECT_EQ(DataUtil::kLipSync_None,
        AGF::ReadLipSyncType("NotARealValue"));
}

// Parses the XML text; keeps the document for as long as its elements are used
static tinyxml2::XMLElement *ParseXml(tinyxml2::XMLDocument &doc, const char *xml)
{
    EXPECT_EQ(tinyxml2::XML_SUCCESS, doc.Parse(xml));
    return doc.RootElement();
}

TEST(AGFReader, TablesReportWhatTheyDoNotKnow)
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *elem = ParseXml(
        doc,
        "<Character><Enabled>False</Enabled><Visible>False</Visible><Brand>New</Brand><StartX>x</StartX></Character>");
    AGF::XmlProblems::Clear();
    DataUtil::CharacterData data;
    AGF::Character().ReadAllData(elem, data);

    // what is known gets read, what is not is reported and does not stop the rest
    EXPECT_FALSE(data.Enabled);
    EXPECT_FALSE(data.Visible);
    ASSERT_EQ(2u, AGF::XmlProblems::Get().size());
    EXPECT_NE(std::string::npos, std::string(AGF::XmlProblems::Get()[0].GetCStr()).find("<Brand>"));
    EXPECT_NE(std::string::npos, std::string(AGF::XmlProblems::Get()[1].GetCStr()).find("<StartX>"));
    AGF::XmlProblems::Clear();
}

TEST(AGFReader, GlobalVariablesMayBeArrays)
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *elem = ParseXml(
        doc, "<Game><GlobalVariables><Variables>"
             "<GlobalVariable><ArraySize>0</ArraySize><ArrayType>None</ArrayType><DefaultValue>True</DefaultValue>"
             "<Name>flag</Name><Type>bool</Type></GlobalVariable>"
             "<GlobalVariable><ArraySize>5</ArraySize><ArrayType>Array</ArrayType><DefaultValue></DefaultValue>"
             "<Name>list</Name><Type>int</Type></GlobalVariable>"
             "<GlobalVariable><ArraySize>0</ArraySize><ArrayType>DynamicArray</ArrayType><DefaultValue>x</DefaultValue>"
             "<Name>names</Name><Type>String</Type></GlobalVariable>"
             "</Variables></GlobalVariables></Game>");
    std::vector<DataUtil::Variable> vars;
    AGF::ReadGlobalVariables(vars, elem);
    ASSERT_EQ(3u, vars.size());

    // as the editor's GlobalVariablesComponent writes them
    EXPECT_STREQ("import bool flag;\nimport int list[5];\nimport String names[];\n",
                 DataUtil::MakeVariablesScriptHeader(vars).GetCStr());
    EXPECT_STREQ("bool flag = true;\nexport flag;\nint list[5];\nexport list;\nString names[];\nexport names;\n"
                 "function game_start() {\n}\n",
                 DataUtil::MakeVariablesScriptBody(vars).GetCStr());
}

// The drift guard: Tools/data/agf_elements.txt lists the XML elements that the editor writes
// for each class of AGS.Types (it is kept current by AgfElementsTests in the editor's tests). Each
// of them must have a row in a table of the reader, or the tools would ignore a project property.

static std::string FindElementsFile()
{
#ifdef AGF_ELEMENTS_FILE
    return AGF_ELEMENTS_FILE;
#else
    // from the working directory up to the checkout
    std::string dir = ".";
    for (int depth = 0; depth < 8; ++depth, dir += "/..")
    {
        std::string path = dir + "/Tools/data/agf_elements.txt";
        if (std::ifstream(path).good())
            return path;
    }
    return "Tools/data/agf_elements.txt";
#endif
}

TEST(AgfElements, TablesReadEverythingTheEditorWrites)
{
    std::ifstream list(FindElementsFile());
    ASSERT_TRUE(list.good()) << "cannot open " << FindElementsFile();

    size_t classes = 0;
    std::string line;
    while (std::getline(list, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        const size_t colon = line.find(':');
        ASSERT_NE(std::string::npos, colon) << line;
        const std::string class_name = line.substr(0, colon);
        ++classes;

        const auto &known = AGF::GetKnownElements();
        const auto table = std::find_if(known.begin(), known.end(),
                                        [&](const AGF::KnownElements &k) { return class_name == k.ClassName; });
        if (table == known.end())
        {
            ADD_FAILURE() << class_name << " has no table in agfreader.cpp (see GetKnownElements)";
            continue;
        }

        std::istringstream elements(line.substr(colon + 1));
        std::string element;
        while (elements >> element)
        {
            const bool found = std::any_of(table->Elements.begin(), table->Elements.end(),
                                           [&](const char *name) { return element == name; });
            EXPECT_TRUE(found) << "The editor writes <" << element << "> in " << class_name
                               << ", but its table has no row for it. Read it, or add a row without a reader "
                                  "(nullptr) if building a game does not need it.";
        }
    }
    EXPECT_GT(classes, 0u);
}

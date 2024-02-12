//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "util/inifile.h"
#include "util/ini_util.h"
#include "util/memorystream.h"

using namespace AGS::Common;


#if AGS_PLATFORM_OS_WINDOWS
#define ENDL "\r\n"
#else
#define ENDL "\n"
#endif

const char *IniFileText = ""
"global_item=global_value" ENDL
"[section1]" ENDL
"item1" ENDL
"//this is comment" ENDL
"item2=" ENDL
"item3=value" ENDL
"item4=another value" ENDL
"[this_section_should_be_deleted]" ENDL
"item1=value1" ENDL
"item2=value2" ENDL
";this is comment" ENDL
"[section3]" ENDL
"item_to_be_deleted=value" ENDL
"item_to_be_kept=another value" ENDL
"     [     section4     ]      " ENDL
"        item1     =     value  " ENDL;

const char *IniFileText2 = ""
"global_item=global_value" ENDL
"[section1]" ENDL
"item1=value1" ENDL
"//this is comment" ENDL
"item2=value2" ENDL
"item3=value3" ENDL
"new_item=new_value" ENDL
"[section3]" ENDL
"item_to_be_kept=another value" ENDL
"     [     section4     ]      " ENDL
"new_item1=new_value1" ENDL
"        item1     =     value  " ENDL
"new_item2=new_value2" ENDL
"[section5]" ENDL
"item5_1=value5_1" ENDL
"item5_2=value5_2" ENDL
"item5_3=value5_3" ENDL;


TEST(IniFile, ReadAndQuery) {
    // Storage buffer
    std::vector<uint8_t> membuf;

    //-------------------------------------------------------------------------
    // Write data
    std::unique_ptr<Stream> out(
        new VectorStream(membuf, kStream_Write));

    out->Write(IniFileText, strlen(IniFileText));
    out.reset();

    //-------------------------------------------------------------------------
    // Read data back
    IniFile ini;
    std::unique_ptr<Stream> in(
        new VectorStream(membuf));

    ini.Read(in.get());
    in.reset();

    //-----------------------------------------------------
    // Assertions

    // there are explicit sections and 1 implicit global one
    const size_t section_count = 5;
    ASSERT_TRUE(ini.GetSectionCount() == section_count);
    IniFile::ConstSectionIterator sec = ini.CBegin();

    ASSERT_TRUE(sec->GetItemCount() == 1u);
    IniFile::ConstItemIterator item = sec->CBegin();
    ASSERT_TRUE(item->GetKey() == "global_item");
    ASSERT_TRUE(item->GetValue() == "global_value");

    ++sec;
    ASSERT_TRUE(sec->GetName() == "section1");
    ASSERT_TRUE(sec->GetItemCount() == 5u);
    item = sec->CBegin();
    ASSERT_TRUE(item->GetKey() == "item1");
    ASSERT_TRUE(item->GetValue() == "");
    ++item;
    ASSERT_TRUE(item->GetLine() == "//this is comment");
    ++item;
    ASSERT_TRUE(item->GetKey() == "item2");
    ASSERT_TRUE(item->GetValue() == "");
    ++item;
    ASSERT_TRUE(item->GetKey() == "item3");
    ASSERT_TRUE(item->GetValue() == "value");
    ++item;
    ASSERT_TRUE(item->GetKey() == "item4");
    ASSERT_TRUE(item->GetValue() == "another value");

    ++sec;
    ASSERT_TRUE(sec->GetName() == "this_section_should_be_deleted");
    ASSERT_TRUE(sec->GetItemCount() == 3u);
    item = sec->CBegin();
    ASSERT_TRUE(item->GetKey() == "item1");
    ASSERT_TRUE(item->GetValue() == "value1");
    ++item;
    ASSERT_TRUE(item->GetKey() == "item2");
    ASSERT_TRUE(item->GetValue() == "value2");
    ++item;
    ASSERT_TRUE(item->GetLine() == ";this is comment");

    ++sec;
    ASSERT_TRUE(sec->GetName() == "section3");
    ASSERT_TRUE(sec->GetItemCount() == 2u);
    item = sec->CBegin();
    ASSERT_TRUE(item->GetKey() == "item_to_be_deleted");
    ASSERT_TRUE(item->GetValue() == "value");
    ++item;
    ASSERT_TRUE(item->GetKey() == "item_to_be_kept");
    ASSERT_TRUE(item->GetValue() == "another value");

    ++sec;
    ASSERT_TRUE(sec->GetName() == "section4");
    ASSERT_TRUE(sec->GetItemCount() == 1u);
    item = sec->CBegin();
    ASSERT_TRUE(item->GetKey() == "item1");
    ASSERT_TRUE(item->GetValue() == "value");
}

TEST(IniFile, ReadAndModify) {
    // Storage buffer
    std::vector<uint8_t> membuf;

    //-------------------------------------------------------------------------
    // Write data
    std::unique_ptr<Stream> out(
        new VectorStream(membuf, kStream_Write));

    out->Write(IniFileText, strlen(IniFileText));
    out.reset();

    //-------------------------------------------------------------------------
    // Read data back
    IniFile ini;
    std::unique_ptr<Stream> in(
        new VectorStream(membuf));

    ini.Read(in.get());
    in.reset();

    //-----------------------------------------------------
    // Assertions

    // there are explicit sections and 1 implicit global one
    const size_t section_count = 5u;
    // Modiying item values
    IniFile::SectionIterator sec = ini.Begin();
    ++sec;
    IniFile::ItemIterator item = sec->Begin();
    item->SetValue("value1");
    ++item; ++item;
    item->SetValue("value2");
    ++item;
    item->SetValue("value3");
    ++item;
    item->SetKey("new_item");
    item->SetValue("new_value");

    // Removing a section
    sec = ini.Begin(); ++sec; ++sec;
    ini.RemoveSection(sec);
    ASSERT_TRUE(ini.GetSectionCount() == section_count - 1u);

    // Removing an item
    sec = ini.Begin(); ++sec; ++sec;
    ASSERT_TRUE(sec->GetName() == "section3");
    item = sec->Begin();
    ASSERT_TRUE(item->GetKey() == "item_to_be_deleted");
    sec->EraseItem(item);

    // Inserting new items
    ++sec;
    ASSERT_TRUE(sec->GetName() == "section4");
    ini.InsertItem(sec, sec->Begin(), "new_item1", "new_value1");
    ini.InsertItem(sec, sec->End(), "new_item2", "new_value2");

    // Append new section
    sec = ini.InsertSection(ini.End(), "section5");
    ini.InsertItem(sec, sec->End(), "item5_1", "value5_1");
    ini.InsertItem(sec, sec->End(), "item5_2", "value5_2");
    ini.InsertItem(sec, sec->End(), "item5_3", "value5_3");

    //-------------------------------------------------------------------------
    // Write modified data
    membuf.resize(0);
    out.reset(new VectorStream(membuf, kStream_Write));
    ini.Write(out.get());
    out.reset();

    //-------------------------------------------------------------------------
    // Read modified data back and ASSERT_TRUE
    in.reset(new VectorStream(membuf));
    String ini_content;
    ini_content.ReadCount(in.get(), static_cast<size_t>(in->GetLength()));
    in.reset();

    ASSERT_TRUE(ini_content == IniFileText2);
}

TEST(IniFile, ReadKeyValueTree) {
    // Storage buffer
    std::vector<uint8_t> membuf;

    //-------------------------------------------------------------------------
    // Write data
    std::unique_ptr<Stream> out(
        new VectorStream(membuf, kStream_Write));

    out->Write(IniFileText2, strlen(IniFileText2));
    out.reset();

    //-------------------------------------------------------------------------
    // Read data back
    std::unique_ptr<Stream> in(
        new VectorStream(membuf));

    ConfigTree tree;
    IniUtil::Read(in.get(), tree);
    in.reset();

    //-----------------------------------------------------
    // Assertions

    ASSERT_TRUE(tree.size() == 5u);
    ASSERT_TRUE(tree.find("") != tree.end()); // global section
    ASSERT_TRUE(tree.find("section1") != tree.end());
    ASSERT_TRUE(tree.find("section3") != tree.end());
    ASSERT_TRUE(tree.find("section4") != tree.end());
    ASSERT_TRUE(tree.find("section5") != tree.end());
    StringOrderMap &sub_tree = tree[""];
    ASSERT_TRUE(sub_tree.size() == 1);
    ASSERT_TRUE(sub_tree.find("global_item") != sub_tree.end());
    ASSERT_TRUE(sub_tree["global_item"] == "global_value");
    sub_tree = tree["section1"];
    ASSERT_TRUE(sub_tree.size() == 4u);
    ASSERT_TRUE(sub_tree.find("item1") != sub_tree.end());
    ASSERT_TRUE(sub_tree.find("item2") != sub_tree.end());
    ASSERT_TRUE(sub_tree.find("item3") != sub_tree.end());
    ASSERT_TRUE(sub_tree.find("new_item") != sub_tree.end());
    ASSERT_TRUE(sub_tree["item1"] == "value1");
    ASSERT_TRUE(sub_tree["item2"] == "value2");
    ASSERT_TRUE(sub_tree["item3"] == "value3");
    ASSERT_TRUE(sub_tree["new_item"] == "new_value");
    sub_tree = tree["section3"];
    ASSERT_TRUE(sub_tree.size() == 1u);
    ASSERT_TRUE(sub_tree.find("item_to_be_kept") != sub_tree.end());
    ASSERT_TRUE(sub_tree["item_to_be_kept"] == "another value");
    sub_tree = tree["section4"];
    ASSERT_TRUE(sub_tree.size() == 3u);
    ASSERT_TRUE(sub_tree.find("new_item1") != sub_tree.end());
    ASSERT_TRUE(sub_tree.find("item1") != sub_tree.end());
    ASSERT_TRUE(sub_tree.find("new_item2") != sub_tree.end());
    ASSERT_TRUE(sub_tree["new_item1"] == "new_value1");
    ASSERT_TRUE(sub_tree["item1"] == "value");
    ASSERT_TRUE(sub_tree["new_item2"] == "new_value2");
    sub_tree = tree["section5"];
    ASSERT_TRUE(sub_tree.size() == 3u);
    ASSERT_TRUE(sub_tree.find("item5_1") != sub_tree.end());
    ASSERT_TRUE(sub_tree.find("item5_2") != sub_tree.end());
    ASSERT_TRUE(sub_tree.find("item5_3") != sub_tree.end());
    ASSERT_TRUE(sub_tree["item5_1"] == "value5_1");
    ASSERT_TRUE(sub_tree["item5_2"] == "value5_2");
    ASSERT_TRUE(sub_tree["item5_3"] == "value5_3");
}

TEST(IniFile, WriteKeyValueTree) {
    // Storage buffer
    std::vector<uint8_t> membuf;

    ConfigTree tree1;
    ConfigTree tree2;

    // construct the tree
    {
        StringOrderMap &audio_tree = tree1["audio"];
        audio_tree["volume"] = "100.0";
        audio_tree["driver"] = "midi";
        StringOrderMap &video_tree = tree1["video"];
        video_tree["gfx_mode"] = "standard mode";
        video_tree["gamma"] = "1.0";
        video_tree["vsync"] = "false";
    }

    // write and read back
    std::unique_ptr<Stream> out(
        new VectorStream(membuf, kStream_Write));
    IniUtil::Write(out.get(), tree1);
    std::unique_ptr<Stream> in(
        new VectorStream(membuf));
    IniUtil::Read(in.get(), tree2);

    // Assert, that tree2 has exactly same items as tree1
    ASSERT_TRUE(tree1 == tree2);
}

TEST(IniFile, MergeTreeWithFile) {
    // Storage buffer
    std::vector<uint8_t> membuf;

    //-------------------------------------------------------------------------
    // Write data
    std::unique_ptr<Stream> out(
        new VectorStream(membuf, kStream_Write));

    out->Write(IniFileText, strlen(IniFileText));
    out.reset();

    //-------------------------------------------------------------------------
    // Read data back
    IniFile ini;
    std::unique_ptr<Stream> in(
        new VectorStream(membuf));

    ini.Read(in.get());
    in.reset();

    //-------------------------------------------------------------------------
    // Modify the tree
    ConfigTree tree1;
    IniUtil::CopyIniToTree(ini, tree1);

    StringOrderMap &audio_tree = tree1["audio"];
    audio_tree["extra_option1"] = "extra value 1";
    audio_tree["extra_option2"] = "extra value 2";
    audio_tree["extra_option3"] = "extra value 3";
    StringOrderMap &video_tree = tree1["video"];
    video_tree["gfx_mode"] = "alternate mode";
    video_tree["gamma"] = "2.0";
    StringOrderMap &new_tree = tree1["other1"];
    new_tree["item1_1"] = "value1_1";
    new_tree["item1_2"] = "value1_2";
    new_tree["item1_3"] = "value1_3";
    StringOrderMap &new_tree2 = tree1["other2"];
    new_tree2["item2_1"] = "value2_1";
    new_tree2["item2_2"] = "value2_2";
    new_tree2["item2_3"] = "value2_3";

    // Merge with the previously written data and write
    IniUtil::Merge(ini, tree1);
    membuf.resize(0);
    out.reset(new VectorStream(membuf, kStream_Write));
    ini.Write(out.get());

    // Read merge result back into the second tree
    ConfigTree tree2;
    in.reset(new VectorStream(membuf));
    IniUtil::Read(in.get(), tree2);

    // Assert, that tree2 has all the items from tree1
    ASSERT_TRUE(tree1 == tree2);
}

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#ifdef _DEBUG

#include <string.h>
#include <algorithm>
#include "debug/assert.h"
#include "util/file.h"
#include "util/inifile.h"
#include "util/stream.h"

using namespace AGS::Common;

#if defined (WINDOWS_VERSION)
#define ENDL "\r\n"
#else
#define ENDL "\n"
#endif

const char *IniFileText = ""
"global_item=global_value"ENDL
"[section1]"ENDL
"item1"ENDL
"//this is comment"ENDL
"item2="ENDL
"item3=value"ENDL
"item4=another value"ENDL
"[this_section_should_be_deleted]"ENDL
"item1=value1"ENDL
"item2=value2"ENDL
";this is comment"ENDL
"[section3]"ENDL
"item_to_be_deleted=value"ENDL
"item_to_be_kept=another value"ENDL
"     [     section4     ]      "ENDL
"        item1     =     value  "ENDL;

const char *IniFileText2 = ""
"global_item=global_value"ENDL
"[section1]"ENDL
"item1=value1"ENDL
"//this is comment"ENDL
"item2=value2"ENDL
"item3=value3"ENDL
"new_item=new_value"ENDL
"[section3]"ENDL
"item_to_be_kept=another value"ENDL
"     [     section4     ]      "ENDL
"new_item1=new_value1"ENDL
"        item1     =     value  "ENDL
"new_item2=new_value2"ENDL
"[section5]"ENDL
"item5_1=value5_1"ENDL
"item5_2=value5_2"ENDL
"item5_3=value5_3"ENDL;


void Test_IniFile()
{
    Stream *fs = File::CreateFile("test.ini");
    fs->Write(IniFileText, strlen(IniFileText));
    delete fs;

    IniFile ini;
    fs = File::OpenFileRead("test.ini");
    ini.Read(fs);
    delete fs;

    // there are explicit sections and 1 implicit global one
    const int section_count = 5;
    // Test reading from the custom ini file
    {
        assert(ini.GetSectionCount() == section_count);
        IniFile::ConstSectionIterator sec = ini.CBegin();

        assert(sec->GetItemCount() == 1);
        IniFile::ConstItemIterator item = sec->CBegin();
        assert(item->GetKey() == "global_item");
        assert(item->GetValue() == "global_value");

        ++sec;
        assert(sec->GetName() == "section1");
        assert(sec->GetItemCount() == 5);
        item = sec->CBegin();
        assert(item->GetKey() == "item1");
        assert(item->GetValue() == "");
        ++item;
        assert(item->GetLine() == "//this is comment");
        ++item;
        assert(item->GetKey() == "item2");
        assert(item->GetValue() == "");
        ++item;
        assert(item->GetKey() == "item3");
        assert(item->GetValue() == "value");
        ++item;
        assert(item->GetKey() == "item4");
        assert(item->GetValue() == "another value");

        ++sec;
        assert(sec->GetName() == "this_section_should_be_deleted");
        assert(sec->GetItemCount() == 3);
        item = sec->CBegin();
        assert(item->GetKey() == "item1");
        assert(item->GetValue() == "value1");
        ++item;
        assert(item->GetKey() == "item2");
        assert(item->GetValue() == "value2");
        ++item;
        assert(item->GetLine() == ";this is comment");

        ++sec;
        assert(sec->GetName() == "section3");
        assert(sec->GetItemCount() == 2);
        item = sec->CBegin();
        assert(item->GetKey() == "item_to_be_deleted");
        assert(item->GetValue() == "value");
        ++item;
        assert(item->GetKey() == "item_to_be_kept");
        assert(item->GetValue() == "another value");

        ++sec;
        assert(sec->GetName() == "section4");
        assert(sec->GetItemCount() == 1);
        item = sec->CBegin();
        assert(item->GetKey() == "item1");
        assert(item->GetValue() == "value");
    }

    // Test altering INI data and saving to file
    {
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
        assert(ini.GetSectionCount() == section_count - 1);

        // Removing an item
        sec = ini.Begin(); ++sec; ++sec;
        assert(sec->GetName() == "section3");
        item = sec->Begin();
        assert(item->GetKey() == "item_to_be_deleted");
        sec->EraseItem(item);

        // Inserting new items
        ++sec;
        assert(sec->GetName() == "section4");
        ini.InsertItem(sec, sec->Begin(), "new_item1", "new_value1");
        ini.InsertItem(sec, sec->End(), "new_item2", "new_value2");

        // Append new section
        sec = ini.InsertSection(ini.End(), "section5");
        ini.InsertItem(sec, sec->End(), "item5_1","value5_1");
        ini.InsertItem(sec, sec->End(), "item5_2","value5_2");
        ini.InsertItem(sec, sec->End(), "item5_3","value5_3");

        fs = File::CreateFile("test.ini");
        ini.Write(fs);
        delete fs;

        fs = File::OpenFileRead("test.ini");
        String ini_content;
        ini_content.ReadCount(fs, fs->GetLength());
        
        assert(ini_content == IniFileText2);
    }

    File::DeleteFile("test.ini");
}

#endif // _DEBUG

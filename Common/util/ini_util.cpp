//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <cstdio>
#include <memory>
#include "util/file.h"
#include "util/ini_util.h"
#include "util/inifile.h"
#include "util/stream.h"
#include "util/string_utils.h"
#include "util/textstreamwriter.h"

namespace AGS
{
namespace Common
{

//-----------------------------------------------------------------------------
// ConfigReader
//-----------------------------------------------------------------------------

bool CfgReadItem(const ConfigTree &cfg, const String &sectn, const String &item, String &value)
{
    const auto sec_it = cfg.find(sectn);
    if (sec_it != cfg.end())
    {
        const auto item_it = sec_it->second.find(item);
        if (item_it != sec_it->second.end())
        {
            value = item_it->second;
            return true;
        }
    }
    return false;
}

int CfgReadInt(const ConfigTree &cfg, const String &sectn, const String &item, int def)
{
    String str;
    if (!CfgReadItem(cfg, sectn, item, str))
        return def;
    return StrUtil::StringToInt(str, def);
}

int CfgReadInt(const ConfigTree &cfg, const String &sectn, const String &item, int min, int max, int def)
{
    int val = CfgReadInt(cfg, sectn, item, def);
    if ((val < min) || (val > max))
        return def;
    return val;
}

int64_t CfgReadInt64(const ConfigTree &cfg, const String &sectn, const String &item, int64_t def)
{
    String str;
    if (!CfgReadItem(cfg, sectn, item, str))
        return def;
    return StrUtil::StringToInt64(str, def);
}

int64_t CfgReadInt64(const ConfigTree &cfg, const String &sectn, const String &item, int64_t min, int64_t max, int64_t def)
{
    int64_t val = CfgReadInt64(cfg, sectn, item, def);
    if ((val < min) || (val > max))
        return def;
    return val;
}

uint64_t CfgReadUInt64(const ConfigTree &cfg, const String &sectn, const String &item, uint64_t def)
{
    String str;
    if (!CfgReadItem(cfg, sectn, item, str))
        return def;
    return StrUtil::StringToInt64(str, def);
}

uint64_t CfgReadUInt64(const ConfigTree &cfg, const String &sectn, const String &item, uint64_t min, uint64_t max, uint64_t def)
{
    uint64_t val = CfgReadUInt64(cfg, sectn, item, def);
    if ((val < min) || (val > max))
        return def;
    return val;
}

float CfgReadFloat(const ConfigTree &cfg, const String &sectn, const String &item, float def)
{
    String str;
    if (!CfgReadItem(cfg, sectn, item, str))
        return def;
    return StrUtil::StringToFloat(str, def);
}

float CfgReadFloat(const ConfigTree &cfg, const String &sectn, const String &item, float min, float max, float def)
{
    float val = CfgReadFloat(cfg, sectn, item, def);
    if ((val < min) || (val > max))
        return def;
    return val;
}

String CfgReadString(const ConfigTree &cfg, const String &sectn, const String &item, const String &def)
{
    String str;
    if (!CfgReadItem(cfg, sectn, item, str))
        return def;
    return str;
}

void CfgReadString(char *cbuf, size_t buf_sz,
    const ConfigTree &cfg, const String &sectn, const String &item, const String &def)
{
    String str = CfgReadString(cfg, sectn, item, def);
    snprintf(cbuf, buf_sz, "%s", str.GetCStr());
}

String CfgFindKey(const ConfigTree &cfg, const String &sectn, const String &item, bool nocase)
{
    const auto sec_it = cfg.find(sectn);
    if (sec_it == cfg.end())
        return "";
    if (nocase)
    {
        for (auto item_it : sec_it->second)
        {
            if (item_it.first.CompareNoCase(item) == 0)
                return item_it.first;
        }
    }
    else
    {
        const auto item_it = sec_it->second.find(item);
        if (item_it != sec_it->second.end())
            return item_it->first;
    }
    return "";
}

//-----------------------------------------------------------------------------
// ConfigWriter
//-----------------------------------------------------------------------------

void CfgWriteInt(ConfigTree &cfg, const String &sectn, const String &item, int64_t value)
{
    cfg[sectn][item].Format("%lld", value);
}

void CfgWriteUInt(ConfigTree &cfg, const String &sectn, const String &item, uint64_t value)
{
    cfg[sectn][item].Format("%llu", value);
}

void CfgWriteFloat(ConfigTree &cfg, const String &sectn, const String &item, float value)
{
    cfg[sectn][item].Format("%f", value);
}

void CfgWriteFloat(ConfigTree &cfg, const String &sectn, const String &item, float value, unsigned precision)
{
    char fmt[10];
    snprintf(fmt, sizeof(fmt), "%%0.%df", precision);
    cfg[sectn][item].Format(fmt, value);
}

void CfgWriteString(ConfigTree &cfg, const String &sectn, const String &item, const String &value)
{
    cfg[sectn][item] = value;
}


//-----------------------------------------------------------------------------
// IniUtil
//-----------------------------------------------------------------------------

typedef std::unique_ptr<Stream>       UStream;
typedef StringOrderMap::const_iterator StrStrOIter;
typedef ConfigTree::const_iterator    ConfigNode;
typedef IniFile::SectionIterator      SectionIterator;
typedef IniFile::ConstSectionIterator CSectionIterator;
typedef IniFile::ItemIterator         ItemIterator;
typedef IniFile::ConstItemIterator    CItemIterator;


static bool ReadIni(const String &file, IniFile &ini)
{
    UStream fs(File::OpenFileRead(file));
    if (fs)
    {
        ini.Read(std::move(fs));
        return true;
    }
    return false;
}

void IniUtil::CopyIniToTree(const IniFile &ini, ConfigTree &tree)
{
    // Copy items into key-value tree
    for (CSectionIterator sec = ini.CBegin(); sec != ini.CEnd(); ++sec)
    {
        if (!sec->GetItemCount())
            continue; // skip empty sections
        StringOrderMap &subtree = tree[sec->GetName()];
        for (CItemIterator item = sec->CBegin(); item != sec->CEnd(); ++item)
        {
            if (!item->IsKeyValue())
                continue; // skip non key-value items
            subtree[item->GetKey()] = item->GetValue();
        }
    }
}

bool IniUtil::Read(const String &file, ConfigTree &tree)
{
    // Read ini content
    IniFile ini;
    if (!ReadIni(file, ini))
        return false;

    CopyIniToTree(ini, tree);
    return true;
}

void IniUtil::Read(std::unique_ptr<Stream> &&in, ConfigTree &tree)
{
    IniFile ini;
    ini.Read(std::move(in));
    CopyIniToTree(ini, tree);
}

void IniUtil::Write(std::unique_ptr<Stream> &&out, const ConfigTree &tree)
{
    TextStreamWriter writer(std::move(out));

    for (ConfigNode it_sec = tree.begin(); it_sec != tree.end(); ++it_sec)
    {
        const String &sec_key     = it_sec->first;
        const StringOrderMap &sec_tree = it_sec->second;

        if (!sec_tree.size())
            continue; // skip empty sections
        // write section name
        if (!sec_key.IsEmpty())
        {
            writer.WriteFormat("[%s]", sec_key.GetCStr());
            writer.WriteLineBreak();
        }
        // write all items
        for (StrStrOIter keyval = sec_tree.begin(); keyval != sec_tree.end(); ++keyval)
        {
            const String &item_key   = keyval->first;
            const String &item_value = keyval->second;

            writer.WriteFormat("%s = %s", item_key.GetCStr(), item_value.GetCStr());
            writer.WriteLineBreak();
        }
    }
}

void IniUtil::Write(const String &file, const ConfigTree &tree)
{
    UStream fs(File::CreateFile(file));
    if (!fs)
        return;
    IniUtil::Write(std::move(fs), tree);
}

void IniUtil::WriteToString(String &s, const ConfigTree &tree)
{
    for (ConfigNode it_sec = tree.begin(); it_sec != tree.end(); ++it_sec)
    {
        const String &sec_key = it_sec->first;
        const StringOrderMap &sec_tree = it_sec->second;
        if (!sec_tree.size())
            continue; // skip empty sections
        // write section name
        if (!sec_key.IsEmpty())
            s.Append(String::FromFormat("[%s]\n", sec_key.GetCStr()));
        // write all items
        for (StrStrOIter keyval = sec_tree.begin(); keyval != sec_tree.end(); ++keyval)
            s.Append(String::FromFormat("%s = %s\n", keyval->first.GetCStr(), keyval->second.GetCStr()));
    }
}

void IniUtil::Merge(IniFile &ini, const ConfigTree &tree)
{
    // Remember the sections we find in file, if some sections are not found,
    // they will be appended to the end of file.
    std::map<String, bool> sections_found;
    for (ConfigNode it = tree.begin(); it != tree.end(); ++it)
        sections_found[it->first] = false;

    // Merge existing sections
    for (SectionIterator sec = ini.Begin(); sec != ini.End(); ++sec)
    {
        if (!sec->GetItemCount())
            continue; // skip empty sections
        String secname = sec->GetName();
        ConfigNode tree_node = tree.find(secname);
        if (tree_node == tree.end())
            continue; // this section is not interesting for us

        // Remember the items we find in this section, if some items are not found,
        // they will be appended to the end of section.
        const StringOrderMap &subtree = tree_node->second;
        std::map<String, bool> items_found;
        for (StrStrOIter keyval = subtree.begin(); keyval != subtree.end(); ++keyval)
            items_found[keyval->first] = false;

        // Replace matching items
        for (ItemIterator item = sec->Begin(); item != sec->End(); ++item)
        {
            String key        = item->GetKey();
            StrStrOIter keyval = subtree.find(key);
            if (keyval == subtree.end())
                continue; // this item is not interesting for us

            String old_value = item->GetValue();
            String new_value = keyval->second;
            if (old_value != new_value)
                item->SetValue(new_value);
            items_found[key] = true;
        }

        // Append new items
        if (!sections_found[secname])
        {
            for (std::map<String, bool>::const_iterator item_f = items_found.begin(); item_f != items_found.end(); ++item_f)
            {
                if (item_f->second)
                    continue; // item was already found
                StrStrOIter keyval = subtree.find(item_f->first);
                ini.InsertItem(sec, sec->End(), keyval->first, keyval->second);
            }
            sections_found[secname] = true; // mark section as known
        }
    }

    // Add new sections
    for (std::map<String, bool>::const_iterator sec_f = sections_found.begin(); sec_f != sections_found.end(); ++sec_f)
    {
        if (sec_f->second)
            continue;
        SectionIterator sec = ini.InsertSection(ini.End(), sec_f->first);
        const StringOrderMap &subtree = tree.find(sec_f->first)->second;
        for (StrStrOIter keyval = subtree.begin(); keyval != subtree.end(); ++keyval)
            ini.InsertItem(sec, sec->End(), keyval->first, keyval->second);
    }
}

bool IniUtil::Merge(const String &file, const ConfigTree &tree)
{
    // Read ini content
    IniFile ini;
    ReadIni(file, ini); // NOTE: missing file is a valid case

    IniUtil::Merge(ini, tree);

    // Write the resulting set of lines
    UStream fs(File::CreateFile(file));
    if (!fs.get())
        return false;
    ini.Write(std::move(fs));
    return true;
}

} // namespace Common
} // namespace AGS

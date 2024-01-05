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
//
// Functions for working with the config key-value tree and exchanging data
// between key-value tree and INI file.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__INIUTIL_H
#define __AGS_CN_UTIL__INIUTIL_H

#include <map>
#include "util/string.h"

namespace AGS
{
namespace Common
{

typedef std::map<String, String>         StringOrderMap;
typedef std::map<String, StringOrderMap> ConfigTree;

//
// Helper functions for parsing values in a ConfigTree
bool    CfgReadItem(const ConfigTree &cfg, const String &sectn, const String &item, String &value);
int     CfgReadInt(const ConfigTree &cfg, const String &sectn, const String &item, int def = 0);
int     CfgReadInt(const ConfigTree &cfg, const String &sectn, const String &item, int min, int max, int def = 0);
inline bool CfgReadBoolInt(const ConfigTree &cfg, const String &sectn, const String &item, bool def = false)
            { return CfgReadInt(cfg, sectn, item, 0, 1, def) != 0; }
float   CfgReadFloat(const ConfigTree &cfg, const String &sectn, const String &item, float def = 0.f);
float   CfgReadFloat(const ConfigTree &cfg, const String &sectn, const String &item, float min, float max, float def = 0.f);
String  CfgReadString(const ConfigTree &cfg, const String &sectn, const String &item, const String &def = "");
// Specialized variant for reading into char buffer, for code compatibility
void    CfgReadString(char *cbuf, size_t buf_sz,
    const ConfigTree &cfg, const String &sectn, const String &item, const String &def = "");
// Looks up for a item key in a given section, returns actual key if one exists, or empty string otherwise,
// optionally compares item name in case-insensitive way.
// NOTE: this is a compatibility hack, in case we cannot enforce key case-sensitivity in some case.
String  CfgFindKey(const ConfigTree &cfg, const String &sectn, const String &item, bool nocase = false);

//
// Helper functions for writing values into a ConfigTree
void    CfgWriteInt(ConfigTree &cfg, const String &sectn, const String &item, int value);
inline void CfgWriteBoolInt(ConfigTree &cfg, const String &sectn, const String &item, bool value)
            { CfgWriteInt(cfg, sectn, item, static_cast<int>(value)); }
void    CfgWriteFloat(ConfigTree &cfg, const String &sectn, const String &item, float value);
void    CfgWriteFloat(ConfigTree &cfg, const String &sectn, const String &item, float value, unsigned precision);
void    CfgWriteString(ConfigTree &cfg, const String &sectn, const String &item, const String &value);


class IniFile;

// Utility functions that exchange data between ConfigTree and INI file.
namespace IniUtil
{
    // Copies the contents of an IniFile object to a key-value tree.
    // The pre-existing tree items, if any, are NOT erased.
    void CopyIniToTree(const IniFile &ini, ConfigTree &tree);
    // Parse the contents of given file as INI format and insert values
    // into the tree. The pre-existing tree items, if any, are NOT erased.
    // Returns FALSE if the file could not be opened.
    bool Read(const String &file, ConfigTree &tree);
    // Same as above, but reads from the provided stream.
    void Read(Stream *in, ConfigTree &tree);
    // Serialize given tree to the given file in INI text format.
    // The INI format suggests only one nested level (group - items).
    // The first level values are treated as a global section items.
    // The sub-nodes beyond 2nd level are ignored completely.
    void Write(const String &file, const ConfigTree &tree);
    // Same as above, but writes to the provided stream
    void Write(Stream *out, const ConfigTree &tree);
    // Serialize given tree to the string in INI text format.
    // TODO: implement proper memory/string stream compatible with base Stream
    // class and merge this with Write function.
    void WriteToString(String &s, const ConfigTree &tree);
    // Parse the contents of given source stream as INI format and merge
    // with values of the given tree while doing only minimal replaces;
    // write the result into destination stream.
    // If item already exists, only value is overwrited, if section exists,
    // new items are appended to the end of it; completely new sections are
    // appended to the end of text.
    // Returns FALSE if the file could not be opened for writing.
    bool Merge(const String &file, const ConfigTree &tree);
    // Similar to the above, but merges the key-value tree into the provided
    // IniFile object in memory.
    void Merge(IniFile &ini, const ConfigTree &tree);
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__INIUTIL_H

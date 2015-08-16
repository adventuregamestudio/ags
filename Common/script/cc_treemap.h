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
//
// 'C'-style script compiler
//
//=============================================================================

#ifndef __CC_TREEMAP_H
#define __CC_TREEMAP_H

#include <map>
#include <string>

// Mimics original interface but uses a std::map for storage
struct ccTreeMap {
    int findValue(const char *key);
    void addEntry(const char *ntx, int p_value);
    void clear();
    ~ccTreeMap();

private:
    std::map<std::string, int> storage;
};

#endif // __CC_TREEMAP_H
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

#include "cc_treemap.h"

int ccTreeMap::findValue(const char *key) {
	if (!key || strlen(key) <= 0) { return -1; }
    std::string cppkey(key);
    if (this->storage.count(cppkey) <= 0) { return -1; }
    return this->storage[cppkey];
}

void ccTreeMap::addEntry(const char* ntx, int p_value) {
    // don't add if it's an empty string or if it's already here
    if (!ntx || strlen(ntx) <= 0) { return; }

    this->storage[std::string(ntx)] = p_value;
}

void ccTreeMap::clear() {
    this->storage.clear();
}

ccTreeMap::~ccTreeMap() {
    this->storage.clear();
}
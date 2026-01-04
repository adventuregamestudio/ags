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
#ifndef __CC_MACROTABLE_H
#define __CC_MACROTABLE_H

#include <map>
#include "util/string.h"

typedef AGS::Common::String AGString;

struct MacroTable {
private:
    std::map<AGString,AGString> _macro_table;
public:
    bool contains(const AGString &name);
    AGString get_macro(const AGString &name) ;
    void add(const AGString &macroname, const AGString &value);
    void remove(AGString &macroname);
    void merge(MacroTable & macro_table);
    void clear();
};

#endif // __CC_MACROTABLE_H
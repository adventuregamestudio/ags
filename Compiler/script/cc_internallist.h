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
#ifndef __CC_INTERNALLIST_H
#define __CC_INTERNALLIST_H

#include <cstdint>

#define SCODE_META    (-2)   // meta tag follows
#define SCODE_INVALID (-1)   // this should never get added, so it's a fault
#define SMETA_LINENUM (1)
#define SMETA_END (2)

struct ccInternalList {
    int length;    // size of array, in ints
    int allocated; // memory allocated for array, in bytes
    int32_t *script;
    int pos;
    int lineAtEnd;
    int cancelCurrentLine;  // whether to set currentline=-10 if end reached

    void startread();
    int32_t peeknext();
    int32_t getnext();  // and update global current_line
    void write(int value);
    // write a meta symbol (ie. non-code thingy)
    void write_meta(int type,int param);
    void shutdown();
    void init();
    ~ccInternalList();
    ccInternalList();

private:
	bool isPosValid(int pos);
};

#endif // __CC_INTERNALLIST_H
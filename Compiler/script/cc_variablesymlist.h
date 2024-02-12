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
#ifndef __CC_VARIABLESYMLIST_H
#define __CC_VARIABLESYMLIST_H

struct VariableSymlist {
    int len;
    long *syml;

    void init(int pLen) {
        len = pLen;
        syml = (long*)malloc(sizeof(long) * len);
    }
    void destroy() {
        free(syml);
        syml = NULL;
    }
};
#define MAX_VARIABLE_PATH 10

#endif // __CC_VARIABLESYMLIST_H
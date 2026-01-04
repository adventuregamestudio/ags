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
#ifndef __CC_VARIABLESYMLIST_H
#define __CC_VARIABLESYMLIST_H

struct VariableSymlist {
    int len;
    int32_t *syml;

    void init(int pLen) {
        len = pLen;
        syml = (int32_t*)malloc(sizeof(int32_t) * len);
    }
    void destroy() {
        free(syml);
        syml = NULL;
    }
};
#define MAX_VARIABLE_PATH 10

#endif // __CC_VARIABLESYMLIST_H
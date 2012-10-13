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

#include <string.h>
#include <stdlib.h>
#include "ac/common.h"
#include "ac/tree_map.h"

TreeMap::TreeMap() {
    left = NULL;
    right = NULL;
    text = NULL;
    translation = NULL;
}

char* TreeMap::findValue (const char* key) {
    if (text == NULL)
        return NULL;

    if (strcmp(key, text) == 0)
        return translation;
    //debug_log("Compare: '%s' with '%s'", key, text);

    if (strcmp (key, text) < 0) {
        if (left == NULL)
            return NULL;
        return left->findValue (key);
    }
    else {
        if (right == NULL)
            return NULL;
        return right->findValue (key);
    }
}

void TreeMap::addText (const char* ntx, char *trans) {
    if ((ntx == NULL) || (ntx[0] == 0) ||
        ((text != NULL) && (strcmp(ntx, text) == 0)))
        // don't add if it's an empty string or if it's already here
        return;

    if (text == NULL) {
        text = (char*)malloc(strlen(ntx)+1);
        translation = (char*)malloc(strlen(trans)+1);
        if (translation == NULL)
            quit("load_translation: out of memory");
        strcpy(text, ntx);
        strcpy(translation, trans);
    }
    else if (strcmp(ntx, text) < 0) {
        // Earlier in alphabet, add to left
        if (left == NULL)
            left = new TreeMap();

        left->addText (ntx, trans);
    }
    else if (strcmp(ntx, text) > 0) {
        // Later in alphabet, add to right
        if (right == NULL)
            right = new TreeMap();

        right->addText (ntx, trans);
    }
}

void TreeMap::clear() {
    if (left) {
        left->clear();
        delete left;
    }
    if (right) {
        right->clear();
        delete right;
    }
    if (text)
        free(text);
    if (translation)
        free(translation);
    left = NULL;
    right = NULL;
    text = NULL;
    translation = NULL;
}

TreeMap::~TreeMap() {
    clear();
}

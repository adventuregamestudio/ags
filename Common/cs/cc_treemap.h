/*
** 'C'-style script compiler
** Copyright (C) 2000-2001, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __CC_TREEMAP_H
#define __CC_TREEMAP_H

struct ICompareStrings {
    virtual int compare(const char *left, const char *right) {
        return strcmp(left, right);
    }
};

// Binary tree structure for holding strings, allows fast access
struct ccTreeMap {
    ccTreeMap *left, *right;
    const char *text;
    int value;

    ccTreeMap();
    ccTreeMap *findNode(const char *key, ICompareStrings *comparer);
    int findValue(const char* key, ICompareStrings *comparer);
    int findValue(const char* key);
    void Clone(ccTreeMap *node);
    void removeNode();
    void removeEntry(const char *key);
    void addEntry(const char* ntx, int p_value);
    void destroyNonRecursive();
    void clear();
    ~ccTreeMap();
};

#endif // __CC_TREEMAP_H
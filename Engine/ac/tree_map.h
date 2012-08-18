
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__TREEMAP_H
#define __AGS_EE_AC__TREEMAP_H

// Binary tree structure for holding translations, allows fast
// access
struct TreeMap {
    TreeMap *left, *right;
    char *text;
    char *translation;

    TreeMap();
    char* findValue (const char* key);
    void addText (const char* ntx, char *trans);
    void clear();
    ~TreeMap();
};

#endif // __AGS_EE_AC__TREEMAP_H

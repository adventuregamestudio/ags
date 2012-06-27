#ifndef __AC_TRANSLATION_H
#define __AC_TRANSLATION_H

#include "ac/rundefines.h"

void close_translation ();
bool init_translation (const char *lang);
char *get_translation (const char *text);
int IsTranslationAvailable ();
int GetTranslationName (char* buffer);
const char* Game_GetTranslationFilename();
int Game_ChangeTranslation(const char *newFilename);

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

extern TreeMap *transtree;
extern char transFileName[MAX_PATH];


#endif // __AC_TRANSLATION_H

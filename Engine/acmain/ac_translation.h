#ifndef __AC_TRANSLATION_H
#define __AC_TRANSLATION_H

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

    TreeMap() {
        left = NULL;
        right = NULL;
        text = NULL;
        translation = NULL;
    }
    char* findValue (const char* key) {
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
    void addText (const char* ntx, char *trans) {
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
    void clear() {
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
    ~TreeMap() {
        clear();
    }
};

extern TreeMap *transtree = NULL;
extern char transFileName[MAX_PATH];


#endif // __AC_TRANSLATION_H
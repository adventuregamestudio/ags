
#include <stdlib.h>
#include <string.h>
#include "ac_wordsdictionary.h"

void WordsDictionary::allocate_memory(int wordCount)
{
    num_words = wordCount;
    if (num_words > 0)
    {
        word = (char**)malloc(wordCount * sizeof(char*));
        word[0] = (char*)malloc(wordCount * MAX_PARSER_WORD_LENGTH);
        wordnum = (short*)malloc(wordCount * sizeof(short));
        for (int i = 1; i < wordCount; i++)
        {
            word[i] = word[0] + MAX_PARSER_WORD_LENGTH * i;
        }
    }
}
void WordsDictionary::free_memory()
{
    if (num_words > 0)
    {
        free(word[0]);
        free(word);
        free(wordnum);
        word = NULL;
        wordnum = NULL;
        num_words = 0;
    }
}

void WordsDictionary::sort () {
    int aa, bb;
    for (aa = 0; aa < num_words; aa++) {
        for (bb = aa + 1; bb < num_words; bb++) {
            if (((wordnum[aa] == wordnum[bb]) && (stricmp(word[aa], word[bb]) > 0))
                || (wordnum[aa] > wordnum[bb])) {
                    short temp = wordnum[aa];
                    char tempst[30];

                    wordnum[aa] = wordnum[bb];
                    wordnum[bb] = temp;
                    strcpy(tempst, word[aa]);
                    strcpy(word[aa], word[bb]);
                    strcpy(word[bb], tempst);
                    bb = aa;
            }
        }
    }
}

int WordsDictionary::find_index (const char*wrem) {
    int aa;
    for (aa = 0; aa < num_words; aa++) {
        if (stricmp (wrem, word[aa]) == 0)
            return aa;
    }
    return -1;
}

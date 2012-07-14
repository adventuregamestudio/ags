
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac/wordsdictionary.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "platform/file.h"

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

char *passwencstring = "Avis Durgan";

void decrypt_text(char*toenc) {
  int adx = 0;

  while (1) {
    toenc[0] -= passwencstring[adx];
    if (toenc[0] == 0)
      break;

    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void read_string_decrypt(FILE *ooo, char *sss) {
  int newlen = getw(ooo);
  if ((newlen < 0) || (newlen > 5000000))
    quit("ReadString: file is corrupt");

  // MACPORT FIX: swap as usual
  fread(sss, sizeof(char), newlen, ooo);
  sss[newlen] = 0;
  decrypt_text(sss);
}

void read_dictionary (WordsDictionary *dict, FILE *writeto) {
  int ii;

  dict->allocate_memory(getw(writeto));
  for (ii = 0; ii < dict->num_words; ii++) {
    read_string_decrypt (writeto, dict->word[ii]);
    fread(&dict->wordnum[ii], sizeof(short), 1, writeto);
  }
}

void freadmissout(short *pptr, FILE *opty) {
  fread(&pptr[0], 2, 5, opty);
  fread(&pptr[7], 2, NUM_CONDIT - 7, opty);
  pptr[5] = pptr[6] = 0;
}

void encrypt_text(char *toenc) {
  int adx = 0, tobreak = 0;

  while (tobreak == 0) {
    if (toenc[0] == 0)
      tobreak = 1;

    toenc[0] += passwencstring[adx];
    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void write_string_encrypt(FILE *ooo, char *sss) {
  int stlent = (int)strlen(sss) + 1;

  putw(stlent, ooo);
  encrypt_text(sss);
  fwrite(sss, stlent, 1, ooo);
  decrypt_text(sss);
}

void write_dictionary (WordsDictionary *dict, FILE *writeto) {
  int ii;

  putw(dict->num_words, writeto);
  for (ii = 0; ii < dict->num_words; ii++) {
    write_string_encrypt (writeto, dict->word[ii]);
//#ifdef ALLEGRO_BIG_ENDIAN
    putshort(dict->wordnum[ii], writeto);//__putshort__lilendian(dict->wordnum[ii], writeto);
//#else
//    fwrite(&dict->wordnum[ii], sizeof(short), 1, writeto);
//#endif
  }
}

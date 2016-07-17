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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac/wordsdictionary.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "util/string_utils.h"
#include "util/stream.h"

using AGS::Common::Stream;

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

const char *passwencstring = "Avis Durgan";

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

void read_string_decrypt(Stream *in, char *sss) {
  int newlen = in->ReadInt32();
  if ((newlen < 0) || (newlen > 5000000))
    quit("ReadString: file is corrupt");

  // MACPORT FIX: swap as usual
  in->Read(sss, newlen);
  sss[newlen] = 0;
  decrypt_text(sss);
}

void read_dictionary (WordsDictionary *dict, Stream *out) {
  int ii;

  dict->allocate_memory(out->ReadInt32());
  for (ii = 0; ii < dict->num_words; ii++) {
    read_string_decrypt (out, dict->word[ii]);
    dict->wordnum[ii] = out->ReadInt16();
  }
}

void freadmissout(short *pptr, Stream *in) {
  in->ReadArrayOfInt16(&pptr[0], 5);
  in->ReadArrayOfInt16(&pptr[7], NUM_CONDIT - 7);
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

void write_string_encrypt(Stream *out, char *sss) {
  int stlent = (int)strlen(sss) + 1;

  out->WriteInt32(stlent);
  encrypt_text(sss);
  out->WriteArray(sss, stlent, 1);
  decrypt_text(sss);
}

void write_dictionary (WordsDictionary *dict, Stream *out) {
  int ii;

  out->WriteInt32(dict->num_words);
  for (ii = 0; ii < dict->num_words; ii++) {
    write_string_encrypt (out, dict->word[ii]);
    out->WriteInt16(dict->wordnum[ii]);//__putshort__lilendian(dict->wordnum[ii], writeto);
  }
}

//
// Implementation from acroom.h that was previously put under NO_SAVE_FUNCTIONS
// macro control
//


// MACPORT FIX: endian support
#include "bigend.h"
#include "misc.h"
#include "wgt2allg_nofunc.h"
#include "acroom_func.h"
#include "acroom_savefunc.h"

#error 'acroom_save.cpp' contents were split up to separate modules; do not include this file in the build

/*
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
#ifndef ALLEGRO_BIG_ENDIAN
    fwrite(&dict->wordnum[ii], sizeof(short), 1, writeto);
#else
    __putshort__lilendian(dict->wordnum[ii], writeto);
#endif  // ALLEGRO_BIG_ENDIAN
  }
}
*/

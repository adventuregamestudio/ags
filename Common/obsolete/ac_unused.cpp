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

#ifdef UNUSED_CODE

/*long cloadcompfile(FILE*outpt,block tobesaved,color*pal,long poot=0) {
  fseek(outpt,poot,SEEK_SET);
  int widt,hit,hh;
  for (hh=0;hh<4;hh++) *tobesaved++=fgetc(outpt);
  tobesaved-=4;
  widt=*tobesaved++;  widt+=(*tobesaved++)*256;
  hit=*tobesaved++; hit+=(*tobesaved++)*256;
  unsigned char* ress=(unsigned char*)malloc(widt+1);
  for (int ww=0;ww<hit;ww++) {
    cunpackbitl(ress,widt,outpt);
    for (int ss=0;ss<widt;ss++)  (*tobesaved++)=ress[ss];
    }
  for (ww=0;ww<256;ww++) {
    pal[ww].r=fgetc(outpt);
    pal[ww].g=fgetc(outpt);
    pal[ww].b=fgetc(outpt);
    }
  poot=ftell(outpt); free(ress); tobesaved-=(widt*hit+4);
  return poot;
  }*/

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

#endif

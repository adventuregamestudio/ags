//
// Implementation from acroom.h that was previously put under CROOM_NOFUNCTIONS
// macro control
//


// MACPORT FIX: endian support
#include "bigend.h"
#include "misc.h"
#include "wgt2allg_nofunc.h"
//#include "acroom_func.h"

#include "cs/cs_utils.h" // fputstring, etc
#include "ac/ac_compress.h"

extern "C" {
	extern FILE *clibfopen(char *, char *);
}



#error 'acroom.cpp' contents were split up to separate modules; do not include this file in the build

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

#endif

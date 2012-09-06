/*
This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/

#include <stdio.h>
#include <stdlib.h>
#include "util/wgt2allg.h"
#include "ac/common.h"	// quit()
#include "util/compress.h"
#include "util/lzw.h"
#include "util/misc.h"
#include "util/file.h"     // filelength()

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

#include "util/wgt2allg.h"
#include "util/file.h"

#include "util/misc.h"

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#ifndef VS2005
#include <alloc.h>
#endif
#include <conio.h>
#endif

#include "util/file.h"

#include "gfx/bitmap.h"

using AGS::Common::IBitmap;
namespace Bitmap = AGS::Common::Bitmap;

#if !defined(MAC_VERSION)
typedef unsigned char * __block;
#else
#ifdef __block
#undef __block
#define __block unsigned char*
#endif
#endif

extern long cliboffset(char *);
extern char lib_file_name[13];
extern void domouse(int);
extern "C"
{
  extern IBitmap *wnewblock(int, int, int, int);
}

//#ifdef ALLEGRO_BIG_ENDIAN
//#define putshort __putshort__lilendian
//#define getshort __getshort__bigendian
//#else
//extern "C"
//{
  //extern void putshort(short, FILE *);
  //extern short getshort(FILE *);
//}
//#endif

#ifndef __WGT4_H
struct color
{
  unsigned char r, g, b;
};
#endif

#ifndef __CJONES_H
long csavecompressed(char *, __block, color[256], long = 0);
long cloadcompressed(char *, __block, color *, long = 0);
#endif

void cpackbitl(unsigned char *line, int size, FILE * outfile)
{
  int cnt = 0;                  // bytes encoded

  while (cnt < size) {
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (jmax >= size)
      jmax = size - 1;

    if (i == size - 1) {        //................last byte alone
      fputc(0, outfile);
      fputc(line[i], outfile);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      fputc(i - j, outfile);
      fputc(line[i], outfile);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      fputc(j - i, outfile);
      fwrite(line + i, j - i + 1, 1, outfile);
      cnt += j - i + 1;

    }
  } // end while
}

void cpackbitl16(unsigned short *line, int size, FILE * outfile)
{
  int cnt = 0;                  // bytes encoded

  while (cnt < size) {
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (jmax >= size)
      jmax = size - 1;

    if (i == size - 1) {        //................last byte alone
      fputc(0, outfile);
      putshort(line[i], outfile);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      fputc(i - j, outfile);
      putshort(line[i], outfile);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      fputc(j - i, outfile);
      fwrite(line + i, j - i + 1, 2, outfile);
      cnt += j - i + 1;

    }
  } // end while
}

void cpackbitl32(unsigned long *line, int size, FILE * outfile)
{
  int cnt = 0;                  // bytes encoded

  while (cnt < size) {
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (jmax >= size)
      jmax = size - 1;

    if (i == size - 1) {        //................last byte alone
      fputc(0, outfile);
      putw(line[i], outfile);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      fputc(i - j, outfile);
      putw(line[i], outfile);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      fputc(j - i, outfile);
      fwrite(line + i, j - i + 1, 4, outfile);
      cnt += j - i + 1;

    }
  } // end while
}


long csavecompressed(char *finam, __block tobesaved, color pala[256], long exto)
{
  FILE *outpt;

  if (exto > 0) {
    outpt = ci_fopen(finam, "a+b");
    fseek(outpt, exto, SEEK_SET);
  } 
  else
    outpt = ci_fopen(finam, "wb");

  int widt, hit;
  long ofes;
  widt = *tobesaved++;
  widt += (*tobesaved++) * 256;
  hit = *tobesaved++;
  hit += (*tobesaved++) * 256;
  fwrite(&widt, 2, 1, outpt);
  fwrite(&hit, 2, 1, outpt);

  unsigned char *ress = (unsigned char *)malloc(widt + 1);
  int ww;

  for (ww = 0; ww < hit; ww++) {
    for (int ss = 0; ss < widt; ss++)
      (*ress++) = (*tobesaved++);

    ress -= widt;
    cpackbitl(ress, widt, outpt);
  }

  for (ww = 0; ww < 256; ww++) {
    fputc(pala[ww].r, outpt);
    fputc(pala[ww].g, outpt);
    fputc(pala[ww].b, outpt);
  }

  ofes = ftell(outpt);
  fclose(outpt);
  free(ress);
  return ofes;
}

int cunpackbitl(unsigned char *line, int size, FILE * infile)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = fgetc(infile);     // get index byte
    if (ferror(infile))
      break;

    char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      char ch = fgetc(infile);
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = ch;
      }
    } else {                     //.....................seq
      int i = cx + 1;
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = fgetc(infile);
      }
    }
  }

  return ferror(infile);
}

int cunpackbitl16(unsigned short *line, int size, FILE * infile)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = fgetc(infile);     // get index byte
    if (ferror(infile))
      break;

    char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      unsigned short ch = getshort(infile);
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = ch;
      }
    } else {                     //.....................seq
      int i = cx + 1;
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = getshort(infile);
      }
    }
  }

  return ferror(infile);
}

int cunpackbitl32(unsigned long *line, int size, FILE * infile)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = fgetc(infile);     // get index byte
    if (ferror(infile))
      break;

    char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      unsigned long ch = getw(infile);
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = ch;
      }
    } else {                     //.....................seq
      int i = cx + 1;
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = getw(infile);
      }
    }
  }

  return ferror(infile);
}

//=============================================================================


#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)

//extern void lzwcompress(FILE * f, FILE * out);
//extern unsigned char *lzwexpand_to_mem(FILE * ii);
extern long outbytes, maxsize, putbytes;
extern int _acroom_bpp;  // bytes per pixel of currently loading room


char *lztempfnm = "~aclzw.tmp";
IBitmap *recalced = NULL;

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(IBitmap*bmpt) {
  if (bmpt->GetColorDepth() == 15)
    return 2;

  return bmpt->GetColorDepth() / 8;
}

long save_lzw(char *fnn, IBitmap *bmpp, color *pall, long offe) {
  FILE  *ooo, *iii;
  long  fll, toret, gobacto;

  ooo = ci_fopen(lztempfnm, "wb");
  putw(bmpp->GetWidth() * bmp_bpp(bmpp), ooo);
  putw(bmpp->GetHeight(), ooo);
  fwrite(&bmpp->GetScanLine(0)[0], bmpp->GetWidth() * bmp_bpp(bmpp), bmpp->GetHeight(), ooo);
  fclose(ooo);

  iii = ci_fopen(fnn, "r+b");
  fseek(iii, offe, SEEK_SET);

  ooo = ci_fopen(lztempfnm, "rb");
  fll = filelength(fileno(ooo));
  fwrite(&pall[0], sizeof(color), 256, iii);
  fwrite(&fll, 4, 1, iii);
  gobacto = ftell(iii);

  // reserve space for compressed size
  fwrite(&fll, 4, 1, iii);
  lzwcompress(ooo, iii);
  toret = ftell(iii);
  fseek(iii, gobacto, SEEK_SET);
  fll = (toret - gobacto) - 4;
  fwrite(&fll, 4, 1, iii);      // write compressed size
  fclose(ooo);
  fclose(iii);
  unlink(lztempfnm);

  return toret;
}

/*long load_lzw(char*fnn,IBitmap*bmm,color*pall,long ooff) {
  recalced=bmm;
  FILE*iii=clibfopen(fnn,"rb");
  fseek(iii,ooff,SEEK_SET);*/

long load_lzw(FILE *iii, IBitmap *bmm, color *pall) {
  long          uncompsiz, *loptr;
  unsigned char *membuffer;
  int           arin;

  recalced = bmm;
  // MACPORT FIX (HACK REALLY)
  fread(&pall[0], 1, sizeof(color)*256, iii);
  fread(&maxsize, 4, 1, iii);
  fread(&uncompsiz,4,1,iii);

  uncompsiz += ftell(iii);
  outbytes = 0; putbytes = 0;

  update_polled_stuff_if_runtime();
  membuffer = lzwexpand_to_mem(iii);
  update_polled_stuff_if_runtime();

  loptr = (long *)&membuffer[0];
  membuffer += 8;
#ifdef ALLEGRO_BIG_ENDIAN
  loptr[0] = __int_swap_endian(loptr[0]);
  loptr[1] = __int_swap_endian(loptr[1]);
  int bitmapNumPixels = loptr[0]*loptr[1]/_acroom_bpp;
  switch (_acroom_bpp) // bytes per pixel!
  {
    case 1:
    {
      // all done
      break;
    }
    case 2:
    {
      short *sp = (short *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        sp[i] = __short_swap_endian(sp[i]);
      }
      // all done
      break;
    }
    case 4:
    {
      int *ip = (int *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        ip[i] = __int_swap_endian(ip[i]);
      }
      // all done
      break;
    }
  }
#endif // ALLEGRO_BIG_ENDIAN

  delete bmm;

  update_polled_stuff_if_runtime();

  bmm = Bitmap::CreateBitmap((loptr[0] / _acroom_bpp), loptr[1], _acroom_bpp * 8);
  if (bmm == NULL)
    quit("!load_room: not enough memory to load room background");

  update_polled_stuff_if_runtime();

  bmm->Acquire ();
  recalced = bmm;

  for (arin = 0; arin < loptr[1]; arin++)
    memcpy(&bmm->GetScanLineForWriting(arin)[0], &membuffer[arin * loptr[0]], loptr[0]);

  bmm->Release ();

  update_polled_stuff_if_runtime();

  free(membuffer-8);

  if (ftell(iii) != uncompsiz)
    fseek(iii, uncompsiz, SEEK_SET);

  update_polled_stuff_if_runtime();

  return uncompsiz;
}

long savecompressed_allegro(char *fnn, IBitmap *bmpp, color *pall, long ooo) {
  unsigned char *wgtbl = (unsigned char *)malloc(bmpp->GetWidth() * bmpp->GetHeight() + 4);
  short         *sss = (short *)wgtbl;
  long          toret;

  sss[0] = bmpp->GetWidth();
  sss[1] = bmpp->GetHeight();

  memcpy(&wgtbl[4], &bmpp->GetScanLine(0)[0], bmpp->GetWidth() * bmpp->GetHeight());

  toret = csavecompressed(fnn, wgtbl, pall, ooo);
  free(wgtbl);
  return toret;
}

long loadcompressed_allegro(FILE *fpp, IBitmap **bimpp, color *pall, long ooo) {
  short widd,hitt;
  int   ii;

  IBitmap *bim = *bimpp;
  delete bim;

  fread(&widd,2,1,fpp);
  fread(&hitt,2,1,fpp);
  bim = Bitmap::CreateBitmap(widd, hitt, 8);
  if (bim == NULL)
    quit("!load_room: not enough memory to decompress masks");
  *bimpp = bim;

  for (ii = 0; ii < hitt; ii++) {
    cunpackbitl(&bim->GetScanLineForWriting(ii)[0], widd, fpp);
    if (ii % 20 == 0)
      update_polled_stuff_if_runtime();
  }

  fseek(fpp, 768, SEEK_CUR);  // skip palette

  return ftell(fpp);
}


#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER

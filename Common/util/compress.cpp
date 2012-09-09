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

#include "util/misc.h"

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#ifndef VS2005
#include <alloc.h>
#endif
#include <conio.h>
#endif

#include "util/datastream.h"

using AGS::Common::CDataStream;

#if !defined(MAC_VERSION)
typedef unsigned char * __block;
#else
#ifdef __block
#undef __block
#define __block unsigned char*
#endif
#endif

extern long cliboffset(const char *);
extern char lib_file_name[13];
extern void domouse(int);
extern "C"
{
  extern block wnewblock(int, int, int, int);
}

//#ifdef ALLEGRO_BIG_ENDIAN
//#define ->WriteInt16 __putshort__lilendian
//#define ->ReadInt16 __getshort__bigendian
//#else
//extern "C"
//{
  //extern void ->WriteInt16(short, FILE *);
  //extern short ->ReadInt16(FILE *);
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

void cpackbitl(unsigned char *line, int size, CDataStream *out)
{
  int cnt = 0;                  // bytes encoded

  while (cnt < size) {
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (jmax >= size)
      jmax = size - 1;

    if (i == size - 1) {        //................last byte alone
      out->WriteInt8(0);
      out->WriteInt8(line[i]);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      out->WriteInt8(i - j);
      out->WriteInt8(line[i]);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      out->WriteInt8(j - i);
      out->WriteArray(line + i, j - i + 1, 1);
      cnt += j - i + 1;

    }
  } // end while
}

void cpackbitl16(unsigned short *line, int size, CDataStream *out)
{
  int cnt = 0;                  // bytes encoded

  while (cnt < size) {
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (jmax >= size)
      jmax = size - 1;

    if (i == size - 1) {        //................last byte alone
      out->WriteInt8(0);
      out->WriteInt16(line[i]);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      out->WriteInt8(i - j);
      out->WriteInt16(line[i]);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      out->WriteInt8(j - i);
      out->WriteArray(line + i, j - i + 1, 2);
      cnt += j - i + 1;

    }
  } // end while
}

void cpackbitl32(unsigned int *line, int size, CDataStream *out)
{
  int cnt = 0;                  // bytes encoded

  while (cnt < size) {
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (jmax >= size)
      jmax = size - 1;

    if (i == size - 1) {        //................last byte alone
      out->WriteInt8(0);
      out->WriteInt32(line[i]);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      out->WriteInt8(i - j);
      out->WriteInt32(line[i]);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      out->WriteInt8(j - i);
      out->WriteArray(line + i, j - i + 1, 4);
      cnt += j - i + 1;

    }
  } // end while
}


long csavecompressed(char *finam, __block tobesaved, color pala[256], long exto)
{
  CDataStream *outpt;

  if (exto > 0) {
    outpt = ci_fopen(finam, Common::kFile_Create, Common::kFile_ReadWrite);
    outpt->Seek(Common::kSeekBegin, exto);
  } 
  else
    outpt = ci_fopen(finam, Common::kFile_CreateAlways, Common::kFile_Write);

  int widt, hit;
  long ofes;
  widt = *tobesaved++;
  widt += (*tobesaved++) * 256;
  hit = *tobesaved++;
  hit += (*tobesaved++) * 256;
  outpt->WriteArray(&widt, 2, 1);
  outpt->WriteArray(&hit, 2, 1);

  unsigned char *ress = (unsigned char *)malloc(widt + 1);
  int ww;

  for (ww = 0; ww < hit; ww++) {
    for (int ss = 0; ss < widt; ss++)
      (*ress++) = (*tobesaved++);

    ress -= widt;
    cpackbitl(ress, widt, outpt);
  }

  for (ww = 0; ww < 256; ww++) {
    outpt->WriteInt8(pala[ww].r);
    outpt->WriteInt8(pala[ww].g);
    outpt->WriteInt8(pala[ww].b);
  }

  ofes = outpt->GetPosition();
  delete outpt;
  free(ress);
  return ofes;
}

int cunpackbitl(unsigned char *line, int size, CDataStream *in)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = in->ReadInt8();     // get index byte
    // TODO: check error when new error handling system is implemented
    //if (ferror(infile))
    //  break;

    char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      char ch = in->ReadInt8();
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

        line[n++] = in->ReadInt8();
      }
    }
  }

  // TODO: check error when new error handling system is implemented
  //return ferror(infile);
  return 0;
}

int cunpackbitl16(unsigned short *line, int size, CDataStream *in)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = in->ReadInt8();     // get index byte
    // TODO: check error when new error handling system is implemented
    //if (ferror(infile))
    //  break;

    char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      unsigned short ch = in->ReadInt16();
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

        line[n++] = in->ReadInt16();
      }
    }
  }

  // TODO: check error when new error handling system is implemented
  //return ferror(infile);
  return 0;
}

int cunpackbitl32(unsigned int *line, int size, CDataStream *in)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = in->ReadInt8();     // get index byte
    // TODO: check error when new error handling system is implemented
    //if (ferror(infile))
    //  break;

    char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      unsigned int ch = in->ReadInt32();
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

        line[n++] = (unsigned int)in->ReadInt32();
      }
    }
  }

  // TODO: check error when new error handling system is implemented
  //return ferror(infile);
  return 0;
}

//=============================================================================


#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)

//extern void lzwcompress(FILE * f, FILE * out);
//extern unsigned char *lzwexpand_to_mem(FILE * ii);
extern long outbytes, maxsize, putbytes;
extern int _acroom_bpp;  // bytes per pixel of currently loading room


char *lztempfnm = "~aclzw.tmp";
BITMAP *recalced = NULL;

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(BITMAP*bmpt) {
  if (bitmap_color_depth(bmpt) == 15)
    return 2;

  return bitmap_color_depth(bmpt) / 8;
}

long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe) {
  CDataStream  *lz_temp_s, *out;
  long  fll, toret, gobacto;

  lz_temp_s = ci_fopen(lztempfnm, Common::kFile_CreateAlways, Common::kFile_Write);
  lz_temp_s->WriteInt32(bmpp->w * bmp_bpp(bmpp));
  lz_temp_s->WriteInt32(bmpp->h);
  lz_temp_s->WriteArray(&bmpp->line[0][0], bmpp->w * bmp_bpp(bmpp), bmpp->h);
  delete lz_temp_s;

  out = ci_fopen(fnn, Common::kFile_Open, Common::kFile_ReadWrite);
  out->Seek(Common::kSeekBegin, offe);

  lz_temp_s = ci_fopen(lztempfnm);
  fll = lz_temp_s->GetLength();
  out->WriteArray(&pall[0], sizeof(color), 256);
  out->WriteArray(&fll, 4, 1);
  gobacto = out->GetPosition();

  // reserve space for compressed size
  out->WriteArray(&fll, 4, 1);
  lzwcompress(lz_temp_s, out);
  toret = out->GetPosition();
  out->Seek(Common::kSeekBegin, gobacto);
  fll = (toret - gobacto) - 4;
  out->WriteArray(&fll, 4, 1);      // write compressed size
  delete lz_temp_s;
  delete out;
  unlink(lztempfnm);

  return toret;
}

/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff) {
  recalced=bmm;
  FILE*iii=clibfopen(fnn,"rb");
  Seek(iii,ooff,SEEK_SET);*/

long load_lzw(CDataStream *in, BITMAP *bmm, color *pall) {
  int          uncompsiz, *loptr;
  unsigned char *membuffer;
  int           arin;

  recalced = bmm;
  // MACPORT FIX (HACK REALLY)
  in->ReadArray(&pall[0], 1, sizeof(color)*256);
  in->ReadArray(&maxsize, 4, 1);
  in->ReadArray(&uncompsiz,4,1);

  uncompsiz += in->GetPosition();
  outbytes = 0; putbytes = 0;

  update_polled_stuff_if_runtime();
  membuffer = lzwexpand_to_mem(in);
  update_polled_stuff_if_runtime();

  loptr = (int *)&membuffer[0];
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

  if (bmm!=NULL)
    destroy_bitmap(bmm);

  update_polled_stuff_if_runtime();

  bmm = create_bitmap_ex(_acroom_bpp * 8, (loptr[0] / _acroom_bpp), loptr[1]);
  if (bmm == NULL)
    quit("!load_room: not enough memory to load room background");

  update_polled_stuff_if_runtime();

  acquire_bitmap (bmm);
  recalced = bmm;

  for (arin = 0; arin < loptr[1]; arin++)
    memcpy(&bmm->line[arin][0], &membuffer[arin * loptr[0]], loptr[0]);

  release_bitmap (bmm);

  update_polled_stuff_if_runtime();

  free(membuffer-8);

  if (in->GetPosition() != uncompsiz)
    in->Seek(Common::kSeekBegin, uncompsiz);

  update_polled_stuff_if_runtime();

  return uncompsiz;
}

long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo) {
  unsigned char *wgtbl = (unsigned char *)malloc(bmpp->w * bmpp->h + 4);
  short         *sss = (short *)wgtbl;
  long          toret;

  sss[0] = bmpp->w;
  sss[1] = bmpp->h;

  memcpy(&wgtbl[4], &bmpp->line[0][0], bmpp->w * bmpp->h);

  toret = csavecompressed(fnn, wgtbl, pall);
  free(wgtbl);
  return toret;
}

long loadcompressed_allegro(CDataStream *in, BITMAP **bimpp, color *pall, long ooo) {
  short widd,hitt;
  int   ii;

  BITMAP *bim = *bimpp;
  if (bim != NULL)
    destroy_bitmap(bim);

  in->ReadArray(&widd,2,1);
  in->ReadArray(&hitt,2,1);
  bim = create_bitmap_ex(8, widd, hitt);
  if (bim == NULL)
    quit("!load_room: not enough memory to decompress masks");
  *bimpp = bim;

  for (ii = 0; ii < hitt; ii++) {
    cunpackbitl(&bim->line[ii][0], widd, in);
    if (ii % 20 == 0)
      update_polled_stuff_if_runtime();
  }

  in->Seek(Common::kSeekCurrent, 768);  // skip palette

  return in->GetPosition();
}


#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER

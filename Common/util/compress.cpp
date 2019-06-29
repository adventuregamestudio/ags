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
#include "ac/common.h"	// quit()
#include "ac/roomstruct.h"
#include "util/compress.h"
#include "util/lzw.h"
#include "util/misc.h"
#include "util/bbop.h"

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

#include "util/misc.h"
#include "util/stream.h"
#include "util/filestream.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;

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

void cpackbitl(unsigned char *line, int size, Stream *out)
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

void cpackbitl16(unsigned short *line, int size, Stream *out)
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

void cpackbitl32(unsigned int *line, int size, Stream *out)
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
  Stream *outpt;

  if (exto > 0) {
    outpt = ci_fopen(finam, Common::kFile_Create, Common::kFile_ReadWrite);
    outpt->Seek(exto, kSeekBegin);
  } 
  else
    outpt = ci_fopen(finam, Common::kFile_CreateAlways, Common::kFile_Write);

  int widt, hit;
  long ofes;
  widt = *tobesaved++;
  widt += (*tobesaved++) * 256;
  hit = *tobesaved++;
  hit += (*tobesaved++) * 256;
  // Those were originally written as shorts, although they are ints
  outpt->WriteInt16(widt);
  outpt->WriteInt16(hit);

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

int cunpackbitl(unsigned char *line, int size, Stream *in)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = in->ReadByte();     // get index byte
    if (in->HasErrors())
      break;

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

        line[n++] = in->ReadByte();
      }
    }
  }

  return in->HasErrors() ? -1 : 0;
}

int cunpackbitl16(unsigned short *line, int size, Stream *in)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = in->ReadByte();     // get index byte
    if (in->HasErrors())
      break;

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

  return in->HasErrors() ? -1 : 0;
}

int cunpackbitl32(unsigned int *line, int size, Stream *in)
{
  int n = 0;                    // number of bytes decoded

  while (n < size) {
    int ix = in->ReadByte();     // get index byte
    if (in->HasErrors())
      break;

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

  return in->HasErrors() ? -1 : 0;
}

//=============================================================================

char *lztempfnm = "~aclzw.tmp";
Bitmap *recalced = NULL;

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(Bitmap*bmpt) {
  if (bmpt->GetColorDepth() == 15)
    return 2;

  return bmpt->GetColorDepth() / 8;
}

long save_lzw(char *fnn, Bitmap *bmpp, color *pall, long offe) {
  Stream  *lz_temp_s, *out;
  long  fll, toret, gobacto;

  lz_temp_s = ci_fopen(lztempfnm, Common::kFile_CreateAlways, Common::kFile_Write);
  lz_temp_s->WriteInt32(bmpp->GetWidth() * bmpp->GetBPP());
  lz_temp_s->WriteInt32(bmpp->GetHeight());
  lz_temp_s->WriteArray(bmpp->GetDataForWriting(), bmpp->GetLineLength(), bmpp->GetHeight());
  delete lz_temp_s;

  out = ci_fopen(fnn, Common::kFile_Open, Common::kFile_ReadWrite);
  out->Seek(offe, kSeekBegin);

  lz_temp_s = ci_fopen(lztempfnm);
  fll = lz_temp_s->GetLength();
  out->WriteArray(&pall[0], sizeof(color), 256);
  out->WriteInt32(fll);
  gobacto = out->GetPosition();

  // reserve space for compressed size
  out->WriteInt32(fll);
  lzwcompress(lz_temp_s, out);
  toret = out->GetPosition();
  out->Seek(gobacto, kSeekBegin);
  fll = (toret - gobacto) - 4;
  out->WriteInt32(fll);      // write compressed size
  delete lz_temp_s;
  delete out;
  unlink(lztempfnm);

  return toret;
}

/*long load_lzw(char*fnn,Bitmap*bmm,color*pall,long ooff) {
  recalced=bmm;
  FILE*iii=clibfopen(fnn,"rb");
  Seek(iii,ooff,SEEK_SET);*/

long load_lzw(Stream *in, Common::Bitmap *bmm, color *pall) {
  int          uncompsiz, *loptr;
  unsigned char *membuffer;
  int           arin;

  recalced = bmm;
  // MACPORT FIX (HACK REALLY)
  in->Read(&pall[0], sizeof(color)*256);
  maxsize = in->ReadInt32();
  uncompsiz = in->ReadInt32();

  uncompsiz += in->GetPosition();
  outbytes = 0; putbytes = 0;

  update_polled_stuff_if_runtime();
  membuffer = lzwexpand_to_mem(in);
  update_polled_stuff_if_runtime();

  loptr = (int *)&membuffer[0];
  membuffer += 8;
#if defined(AGS_BIG_ENDIAN)
  loptr[0] = AGS::Common::BBOp::SwapBytesInt32(loptr[0]);
  loptr[1] = AGS::Common::BBOp::SwapBytesInt32(loptr[1]);
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
        sp[i] = AGS::Common::BBOp::SwapBytesInt16(sp[i]);
      }
      // all done
      break;
    }
    case 4:
    {
      int *ip = (int *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        ip[i] = AGS::Common::BBOp::SwapBytesInt32(ip[i]);
      }
      // all done
      break;
    }
  }
#endif // defined(AGS_BIG_ENDIAN)

  delete bmm;

  update_polled_stuff_if_runtime();

  bmm = BitmapHelper::CreateBitmap((loptr[0] / _acroom_bpp), loptr[1], _acroom_bpp * 8);
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

  if (in->GetPosition() != uncompsiz)
    in->Seek(uncompsiz, kSeekBegin);

  update_polled_stuff_if_runtime();

  return uncompsiz;
}

long savecompressed_allegro(char *fnn, Common::Bitmap *bmpp, color *pall, long write_at) {
  unsigned char *wgtbl = (unsigned char *)malloc(bmpp->GetWidth() * bmpp->GetHeight() + 4);
  short         *sss = (short *)wgtbl;
  long          toret;

  sss[0] = bmpp->GetWidth();
  sss[1] = bmpp->GetHeight();

  memcpy(&wgtbl[4], bmpp->GetDataForWriting(), bmpp->GetWidth() * bmpp->GetHeight());

  toret = csavecompressed(fnn, wgtbl, pall, write_at);
  free(wgtbl);
  return toret;
}

long loadcompressed_allegro(Stream *in, Common::Bitmap **bimpp, color *pall, long read_at) {
  short widd,hitt;
  int   ii;

  Bitmap *bim = *bimpp;
  delete bim;

  widd = in->ReadInt16();
  hitt = in->ReadInt16();
  bim = BitmapHelper::CreateBitmap(widd, hitt, 8);
  if (bim == NULL)
    quit("!load_room: not enough memory to decompress masks");
  *bimpp = bim;

  for (ii = 0; ii < hitt; ii++) {
    cunpackbitl(&bim->GetScanLineForWriting(ii)[0], widd, in);
    if (ii % 20 == 0)
      update_polled_stuff_if_runtime();
  }

  in->Seek(768);  // skip palette

  return in->GetPosition();
}



/*
  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#include <stdio.h>
#include <stdlib.h>

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

#include "wgt2allg.h"
#include "platform/file.h"

#include "misc.h"

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#ifndef VS2005
#include <alloc.h>
#endif
#include <conio.h>
#endif

#include "platform/file.h"

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
  extern block wnewblock(int, int, int, int);
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

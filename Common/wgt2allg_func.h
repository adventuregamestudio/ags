/*
** WGT -> Allegro portability interface
** Copyright (C) 1998, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
** wsavesprites and wloadsprites are hi-color compliant
*/

//=============================================================================
//
// This is originally a part of wgt2allg.h, that was put under
// WGT2ALLEGRO_NOFUNCTIONS macro control and enabled when WGT2ALLEGRO_NOFUNCTIONS
// was set.
// This should be included INSTEAD of wgt2allg.h in the source files that
// previously included wgt2allg.h with WGT2ALLEGRO_NOFUNCTIONS define.
// There's no need to include both wgt2allg.h and wgt2allg_nofunc.h, since latter
// includes wgt2allg.h on its own.
// WGT2ALLEGRO_NOFUNCTIONS macro is being removed since no longer needed.
//
//=============================================================================

#ifndef __WGT2ALLG_FUNC_H
#define __WGT2ALLG_FUNC_H

#include "wgt2allg.h"

#error Do not include wgt2allg_func.h for now

#ifdef __cplusplus
extern "C"
{
#endif

  extern const char *wgt2allgcopyright;
  extern const char *spindexid;
  extern const char *spindexfilename;

  extern fpos_t lfpos;
  extern FILE *libf;
  extern short lresult;
  extern int lsize;
  extern char password[16];
  extern char *wgtlibrary;
  extern int currentcolor;
  extern int vesa_xres, vesa_yres;
  extern block abuf;

#ifdef WINDOWS_VERSION
#define GFX_VGA GFX_DIRECTX
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#define GFX_VGA GFX_AUTODETECT
#endif

/*
  GCC from the Android NDK doesn't like allegro_init()

  extern void vga256();
*/

#if (!defined(WINDOWS_VERSION) && !defined(LINUX_VERSION) && !defined(MAC_VERSION))
  extern union REGS r;
  extern void wsetmode(int nnn);
  extern int wgetmode();
#endif

#ifdef USE_CLIB
  extern "C"
  {
    extern FILE *clibfopen(char *, char *);
    extern long cliboffset(char *);
    extern long clibfilesize(char *);
    extern long last_opened_size;
  }
#define fopen clibfopen
#endif

  extern void wsetscreen(block nss);
  extern void wsetrgb(int coll, int r, int g, int b, color * pall);
  extern int wloadpalette(char *filnam, color * pall);

  extern void wcolrotate(unsigned char start, unsigned char finish, int dir, color * pall);

  extern block tempbitm;
  extern block wnewblock(int x1, int y1, int x2, int y2);
  extern short getshort(FILE * fff);
  extern void putshort(short num, FILE *fff);
  extern block wloadblock(char *fill);

  extern int wloadsprites(color * pall, char *filnam, block * sarray, int strt, int eend);
  extern void wfreesprites(block * blar, int stt, int end);
/*
  extern void wsavesprites_ex(color * pll, char *fnm, block * spre, int strt, int eend, unsigned char *arry);
  extern void wsavesprites(color * pll, char *fnm, block * spre, int strt, int eend);
*/
  extern void wputblock(int xx, int yy, block bll, int xray);

  extern const int col_lookups[32];

  extern void __my_setcolor(int *ctset, int newcol);
  extern void wsetcolor(int nval);

  extern int get_col8_lookup(int nval);
  

  extern int __wremap_keep_transparent;
  extern void wremap(color * pal1, block picc, color * pal2);
  extern void wremapall(color * pal1, block picc, color * pal2);

  // library file functions
  extern void readheader();

  extern void findfile(char *filnam);

  extern int checkpassword(char *passw); // 0 = incorrect   ! = 0 correct

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
  extern void gettime(struct time *tpt);
#endif

  extern long wtimer(struct time tt1, struct time tt2);

  extern void wcopyscreen(int x1, int y1, int x2, int y2, block src, int dx, int dy, block dest);

#ifdef USE_CLIB
#undef fopen
#endif

#ifdef __cplusplus
}
#endif


#include "wgt2allg_2.h"


#ifdef __cplusplus
extern "C"
{
#endif

  extern void wbutt(int x1, int y1, int x2, int y2);

#ifdef __cplusplus
}
#endif


#endif // __WGT2ALLG_FUNC_H
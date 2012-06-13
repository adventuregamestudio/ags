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

#ifndef __WGT2ALLG_NOFUNC_H
#define __WGT2ALLG_NOFUNC_H

#include "wgt2allg.h"

#error Do not include wgt2allg_nofunc.h for now

#ifdef __cplusplus
extern "C"
{
#endif

  extern const char *spindexid;
  extern const char *spindexfilename ;

  extern fpos_t lfpos;
  extern FILE *libf;
  extern short lresult;
  extern int lsize;
  extern char password[16];
  extern char *wgtlibrary;

  extern void readheader();
  extern void findfile(char *);
  extern int checkpassword(char *);

  extern int currentcolor;
  extern block abuf;
  extern int vesa_xres, vesa_yres;

  extern short getshort(FILE * fff);
  extern void putshort(short num, FILE *fff);

  extern void vga256();
  extern void wbutt(int, int, int, int);
  extern void wsetmode(int);
  extern int wgetmode();
  extern void wsetscreen(block);
  extern void wnormscreen();
  extern void wcopyscreen(int, int, int, int, block, int, int, block);
  extern void wsetrgb(int, int, int, int, color *);
  extern int wloadpalette(char *, color *);
  extern void wcolrotate(unsigned char, unsigned char, int, color *);
  extern block wnewblock(int, int, int, int);
  extern void wfreesprites(block *, int, int);
  extern block wloadblock(char *);
  extern int wloadsprites(color *, char *, block *, int, int);
  extern void wputblock(int, int, block, int);
  extern void wremap(color *, block, color *);
  extern void wsetcolor(int);
  extern long wtimer(struct time, struct time);
  extern void gettime(struct time *);
  extern void __my_setcolor(int *ctset, int newcol);
  extern int get_col8_lookup(int nval);

#ifdef __cplusplus
}
#endif

#include "wgt2allg_2.h"

#endif // __WGT2ALLG_NOFUNC_H
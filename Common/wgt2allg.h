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
#define _WGT45_

#ifndef __WGT4_H
#define __WGT4_H

#ifdef _MSC_VER
#ifndef WINDOWS_VERSION
#define WINDOWS_VERSION
#endif
#endif

#include <stdio.h>
#include <string.h>

#ifdef USE_ALLEGRO3
#include <allegro3.h>
#else
#include "allegro.h"
#endif

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <dos.h>
#include <io.h>
#endif


#include <stdarg.h>

#ifdef WINDOWS_VERSION
#include "winalleg.h"
#elif defined(MAC_VERSION) && !defined(IOS_VERSION)
#include <osxalleg.h>
#endif

#include "bigend.h"



#if !defined __WGT2ALLG_FUNC_H && !defined __WGT2ALLG_NOFUNC_H
#error Do not include wgt2allg.h directly, include either wgt2allg_func.h or wgt2allg_nofunc.h instead.
#endif

#if defined WGT2ALLEGRO_NOFUNCTIONS
#error WGT2ALLEGRO_NOFUNCTIONS macro is obsolete and should not be defined anymore.
#endif



typedef BITMAP *block;

#if (WGTMAP_SIZE == 1)
typedef unsigned char *wgtmap;
#else
typedef short *wgtmap;
#endif

#define color RGB
#define TEXTFG    0
#define TEXTBG    1

#define fpos_t unsigned long
#ifdef __cplusplus
extern "C"
{
#endif

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <time.h>
  struct time
  {
    int ti_hund, ti_sec, ti_min, ti_hour;
  };
#endif

#define is_ttf(fontptr)  (fontptr[0] == 'T')

  

#ifdef __cplusplus
}
#endif

#endif

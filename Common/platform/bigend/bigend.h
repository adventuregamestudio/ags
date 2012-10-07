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
//
// bigend.h
// Created by Steve McCrea on 6/16/05.
//
// Functions to deal with endianness issues of file access.
//
//=============================================================================

#ifndef __BIGEND_H__
#define __BIGEND_H__

#ifdef ALLEGRO_BIG_ENDIAN

#include <allegro.h>

int         __int_swap_endian(int);
short       __short_swap_endian(short);

int         __getw__bigendian(FILE *);
void        __putw__lilendian(int, FILE *);

short int   __getshort__bigendian(FILE *);
void        __putshort__lilendian(short int, FILE *);

size_t      __fread__bigendian(void *, size_t size, size_t nmemb, FILE *);
size_t      __fwrite__lilendian(void const *, size_t size, size_t nmemb, FILE *);

#ifndef __BIGEND_ORIGINAL_FILE_FUNCTIONS

#define getshort    __getshort__bigendian
#define putshort    __putshort__lilendian
#define getw        __getw__bigendian
#define putw        __putw__lilendian
#define fread       __fread__bigendian
#define fwrite      __fwrite__lilendian

#endif  // !__BIGEND_ORIGINAL_FILE_FUNCTIONS

#endif  // ALLEGRO_BIG_ENDIAN

#endif  // __BIGEND_H__

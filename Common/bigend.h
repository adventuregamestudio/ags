/*
 *  bigend.h
 *  AGSRunTime
 *
 *  Created by Steve McCrea on 6/16/05.
 *  Part of the Adventure Game Studio source code (c)1999-2005 Chris Jones.
 *
 *  Functions to deal with endianness issues of file access.
 *
 */

#ifndef __BIGEND_H__
#define __BIGEND_H__

#if defined(ANDROID_VERSION) || defined(IOS_VERSION)
// Some Android defines that are needed in multiple files.
// Not the best place for it, but should do for the moment.
#include <wchar.h>
extern "C" {
char *strupr(char *s);
char *strlwr(char *s);
}
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif


#ifdef ALLEGRO_BIG_ENDIAN

#include <Allegro.h>

int __int_swap_endian(int);
short __short_swap_endian(short);

int __getw__bigendian(FILE *);
void __putw__lilendian(int, FILE *);

short int __getshort__bigendian(FILE *);
void __putshort__lilendian(short int, FILE *);

size_t __fread__bigendian(void *, size_t size, size_t nmemb, FILE *);
size_t __fwrite__lilendian(void const *, size_t size, size_t nmemb, FILE *);

#ifndef __BIGEND_ORIGINAL_FILE_FUNCTIONS

#define getw __getw__bigendian
#define putw __putw__lilendian
#define fread __fread__bigendian
#define fwrite __fwrite__lilendian

#endif  __BIGEND_ORIGINAL_FILE_FUNCTIONS

#endif  // ALLEGRO_BIG_ENDIAN

#endif  // __BIGEND_H__

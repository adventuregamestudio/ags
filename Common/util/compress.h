/*
This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/

#ifndef __AC_COMPRESS_H
#define __AC_COMPRESS_H

#include <stdio.h>

// MACPORT FIX 9/6/05: removed far and put space after *
#if !defined(MAC_VERSION)
typedef unsigned char * __block;
#else
#ifdef __block
#undef __block
#define __block unsigned char*
#endif
#endif

long csavecompressed(char *finam, __block tobesaved, color pala[256], long exto);

void cpackbitl(unsigned char *line, int size, FILE * outfile);
void cpackbitl16(unsigned short *line, int size, FILE * outfile);
void cpackbitl32(unsigned long *line, int size, FILE * outfile);
int  cunpackbitl(unsigned char *line, int size, FILE * infile);
int  cunpackbitl16(unsigned short *line, int size, FILE * infile);
int  cunpackbitl32(unsigned long *line, int size, FILE * infile);

//=============================================================================

#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)

long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe);

/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff);*/
long load_lzw(FILE *iii, BITMAP *bmm, color *pall);
long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo);
long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo);

//extern char *lztempfnm;
//extern BITMAP *recalced;

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(BITMAP*bmpt);

#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER

#endif // __AC_COMPRESS_H
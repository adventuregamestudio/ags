/*
  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#include <stdio.h>
#include <stdlib.h>

// MACPORT FIX 9/6/05: removed far and put space after *
#if !defined(MAC_VERSION)
typedef unsigned char * __block;
#else
#ifdef __block
#undef __block
#define __block unsigned char*
#endif
#endif

extern long csavecompressed(char *finam, __block tobesaved, color pala[256], long exto);

extern void cpackbitl(unsigned char *line, int size, FILE * outfile);
extern void cpackbitl16(unsigned short *line, int size, FILE * outfile);
extern void cpackbitl32(unsigned long *line, int size, FILE * outfile);
extern int  cunpackbitl(unsigned char *line, int size, FILE * infile);
extern int  cunpackbitl16(unsigned short *line, int size, FILE * infile);
extern int  cunpackbitl32(unsigned int *line, int size, FILE * infile);

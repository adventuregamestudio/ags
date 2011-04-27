/*
** FMEM - FILE-style routines for reading and writing "memory-files".
** Copyright (C) 2000, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
** Memory files give you a fast alternative to a temporary disk file, as
** long as the amount you are writing is fairly small.
**
*/

#ifndef __FMEM_H
#define __FMEM_H

struct FMEM
{
  char *data;
  long len;                     // length of data in array
  long size;                    // size of data array allocated
  long magic;
  long pos;                     // current seeked position
};

extern FMEM *fmem_create();
extern FMEM *fmem_open(const char *);
extern void fmem_close(FMEM *);
extern void fmem_write(char *, long, FMEM *);
extern void fmem_putc(char, FMEM *);
extern void fmem_puts(char *, FMEM *);
extern int fmem_getc(FMEM *);
extern int fmem_peekc(FMEM *);
extern int fmem_eof(FMEM *);
extern void fmem_gets(FMEM *, char *);

#endif // __FMEM_H

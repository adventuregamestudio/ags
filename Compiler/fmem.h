//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// FMEM - FILE-style routines for reading and writing "memory-files".
// Memory files give you a fast alternative to a temporary disk file, as
// long as the amount you are writing is fairly small.
//
// FMEM provides fopen/fwrite/fclose-type functions for writing "memory files".
// This enables you to for example send output of a procedure easily to a
// memory buffer without having to worry about re-allocating if it becomes
// too large.  Open with fmem_create for writing, or fmem_open for reading.
//
//=============================================================================
#ifndef __FMEM_H
#define __FMEM_H

struct FMEM
{
  char *data;
  size_t len;                  // length of data in array
  size_t size;                 // size of data array allocated
  int magic;
  size_t pos;                  // current seeked position
};

extern FMEM *fmem_create();
extern FMEM *fmem_open(const char *);
extern void fmem_close(FMEM *);
extern void fmem_write(char *, size_t, FMEM *);
extern void fmem_putc(char, FMEM *);
extern void fmem_puts(char *, FMEM *);
extern int fmem_getc(FMEM *);
extern int fmem_peekc(FMEM *);
extern int fmem_eof(FMEM *);
extern void fmem_gets(FMEM *, char *);

#endif // __FMEM_H

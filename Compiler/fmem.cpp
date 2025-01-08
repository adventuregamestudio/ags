//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include "fmem.h"

const char*fmemcopyr="FMEM v1.00 (c) 2000 Chris Jones";
#define FMEM_MAGIC 0xcddebeef

FMEM*tempy;
// fmem_create: create a blank FMEM file for writing
FMEM*fmem_create() {
  tempy=(FMEM*)malloc(sizeof(FMEM));
  tempy->size=100;
  tempy->len=0;
  tempy->data=(char*)malloc(tempy->size+10);
  tempy->data[0]=0;
  tempy->magic=FMEM_MAGIC;
  tempy->pos=0;
  return tempy;
  }

// fmem_open: create an FMEM file for reading, using a string as the source
FMEM*fmem_open(const char*sourc) {
  tempy=(FMEM*)malloc(sizeof(FMEM));
  tempy->size=strlen(sourc)+10;
  tempy->len=strlen(sourc);
  tempy->data=(char*)malloc(tempy->size+10);
  strcpy(tempy->data,sourc);
  tempy->magic=FMEM_MAGIC;
  tempy->pos=0;
  return tempy;
  }

void fmem_close(FMEM*cloo) {
  if (cloo==NULL) return;
  if (cloo->magic != FMEM_MAGIC) return;   // uninitialized
  free(cloo->data);
  free(cloo);
  }

// This should be fairly large because otherwise it's very slow with loads
// of malloc and freeing
#define BUFFER_INCREMENT_SIZE 2000

void fmem_write(char* towr ,size_t mei, FMEM*fill) {
  size_t curdatlen=fill->len;
  if (curdatlen + mei >= fill->size) {
    char*datwas=fill->data;
    fill->size=fill->size + mei + BUFFER_INCREMENT_SIZE;
    fill->data=(char*)malloc(fill->size+10);
    strcpy(fill->data,datwas);
    free(datwas);
    }

  memcpy(&fill->data[curdatlen],towr,mei);
  fill->data[curdatlen+mei]=0;
  fill->len+=mei;
  fill->pos+=mei;
  }

void fmem_putc(char toput,FMEM*fill) {
  char templ[2];
  templ[0]=toput;
  templ[1]=0;
  fmem_write(&templ[0],1,fill);
  }

void fmem_puts(char*strin, FMEM*ooo) {
  char*ibuffer=(char*)malloc(strlen(strin)+10);
  sprintf(ibuffer,"%s\r\n",strin);
  fmem_write(ibuffer,strlen(ibuffer),ooo);
  free(ibuffer);
  }

int fmem_getc(FMEM*fme) {
  int toret=fme->data[fme->pos];
  fme->pos++;
  return toret;
  }
int fmem_peekc(FMEM*fme) {
  return fme->data[fme->pos];
  }

int fmem_eof(FMEM*fme) {
  if (fme->pos >= fme->len) return 1;
  return 0;
  }

void fmem_gets(FMEM*fmem,char*bufrr) {
  int tval,bindx=0;
  if (fmem_peekc(fmem)==0) {
    bufrr[0]=0; fmem_getc(fmem);
    return; }
  do {
    tval=fmem_getc(fmem);
    bufrr[bindx]=tval;
    bindx++;
    if (tval==0) break;
    if (fmem_eof(fmem)) break;
    } while ((tval != 13) && (tval != 10));
  if ((fmem_eof(fmem)==0) && (fmem_peekc(fmem)==10))
    fmem_getc(fmem);  // LF
  if (bufrr[bindx-1] == 13)
    bindx--;    // strip CR

  bufrr[bindx]=0;
  }

// ***** END FMEM FUNCTIONS

/*
  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

//#include "cscomp.h"
#include "cs/cc_error.h"    // ccError global vars
#include "cs/cs_runtime.h"  // ccGetCurrentInstance


int currentline = 0;


void cc_error(char *descr, ...)
{
  ccErrorCallStack[0] = 0;
  ccErrorIsUserError = false;
  if (descr[0] == '!')
  {
    ccErrorIsUserError = true;
    descr++;
  }

  char displbuf[1000];
  va_list ap;

  va_start(ap, descr);
  vsprintf(displbuf, descr, ap);
  va_end(ap);

  if (currentline > 0) {
    
    if (ccGetCurrentInstance() == NULL) {
      sprintf(ccErrorString, "Error (line %d): %s", currentline, displbuf);
    }
    else {
      sprintf(ccErrorString, "Error: %s\n", displbuf);
      ccGetCallStack(ccGetCurrentInstance(), ccErrorCallStack, 5);
    }
  }
  else
    sprintf(ccErrorString, "Runtime error: %s", displbuf);

  ccError = 1;
  ccErrorLine = currentline;
}







void fputstring(char *sss, FILE *ddd) {
  int b = 0;
  while (sss[b] != 0) {
    fputc(sss[b], ddd);
    b++;
  }
  fputc(0,ddd);
}

void fgetstring_limit(char *sss, FILE *ddd, int bufsize) {
  int b = -1;
  do {
    if (b < bufsize - 1)
      b++;
    sss[b] = fgetc(ddd);
    if (feof(ddd))
      return;
  } while (sss[b] != 0);
}

void fgetstring(char *sss, FILE *ddd) {
  fgetstring_limit (sss, ddd, 50000000);
}


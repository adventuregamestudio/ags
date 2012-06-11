/*
 'C'-style script compiler development file. (c) 2000,2001 Chris Jones

 SCOM is a script compiler for the 'C' language. The current version
 implements:
  * #define macros, definition of and use of
  * "//" and "/*---* /" comments
  * global and local variables; calling functions; assignments
  * most of the standard 'C' operators
  * structures and arrays
  * import and export of variables and functions from parent program
  * strings get allocated 200 bytes of storage automatically

 It currently does NOT do:
  * #define with parenthesis, eg. #define func(a) bar(a+3)
  * typedefs
  * optimize code generated - it could check if MAR already contains location
      to read, for example

 Some interesting points about how this works:
  * while loops are stored internally as "else"-blocks, but with an extra
    bit of data storing the start of the while test condition to go back to
  * array index accesses are generated as code to allow anything inside
    the brackets, whereas structure member accesses are hardcoded into the
    offset in the code since the member has a fixed offset from the structure
    start

*/
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

//#include "fmem.h"
//#include "csprepro.cpp"
//#include "cscomp.h"

#ifdef UNUSED_CODE

void dump_code(FILE*dto,ccScript*cctemp) {
  fprintf(dto,"script data size: %d\n",cctemp->globaldatasize);
  fprintf(dto,"string area size: %d\n",cctemp->stringssize);
//  fprintf(dto,"SP (should be zero): %d\n",cctemp->cur_sp);
  fprintf(dto,"code size: %d\n",cctemp->codesize);
  fprintf(dto,"SCRIPT VIRTUAL-CODE FOLLOWS:\n");
  // dump code in readable form
  int t;
  for (t=0;t<cctemp->codesize;t++) {
    int l,thisop=cctemp->code[t],isreg=0;
    char*toprint=sccmdnames[thisop];
    if (toprint[0]=='$') {
      isreg=1;
      toprint++;
      }
    if (toprint[0]=='$') {
      isreg|=2;
      toprint++;
      }
    fprintf(dto,"%s",toprint);
    for (l=0;l<sccmdargs[thisop];l++) {
      t++;
      if (l>0) fprintf(dto,",");
      if ((l==0) && (isreg & 1))
        fprintf(dto," %s",regnames[cctemp->code[t]]);
      else if ((l==1) && (isreg & 2))
        fprintf(dto," %s",regnames[cctemp->code[t]]);
      else fprintf(dto," %d",cctemp->code[t]);
      }
    fprintf(dto,"\n");
    }

  }

#endif
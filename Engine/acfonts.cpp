/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#pragma unmanaged
#ifndef USE_ALFONT
#define USE_ALFONT
#endif
//#include "wgt2allg_nofunc.h"
#include "wgt2allg.h"
//#include "acroom_nofunc.h"
#include "acruntim.h"

#ifdef USE_ALFONT
#include "alfont.h"
#endif


#error 'acfonts.cpp' contents were split up to separate modules; do not include this file in the build


extern FILE *fopen_shared(char *, char *);
extern int flength_shared(FILE *ffi);

extern "C"
{
  extern FILE *clibfopen(char *, char *);
  extern long cliboffset(char *);
  extern long clibfilesize(char *);
  extern long last_opened_size;
}


extern bool ShouldAntiAliasText();
extern int our_eip;











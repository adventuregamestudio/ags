/*
  C-Script run-time interpreter (c) 2001 Chris Jones

  You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
  when compiling this, or strange results can happen.

  There is a problem with importing functions on 16-bit compilers: the
  script system assumes that all parameters are passed as 4 bytes, which
  ints are not on 16-bit systems. Be sure to define all parameters as longs,
  or join the 21st century and switch to DJGPP or Visual C++.

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/
//#define DEBUG_MANAGED_OBJECTS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

//#include "cscomp.h"
/*
#include "misc.h"
#include "bigend.h"

#include "cs/cc_dynamicobject.h"
#include "cs/cc_dynamicarray.h"
#include "cs/cc_managedobjectpool.h"
#include "cs/cc_symboldef.h"
#include "cs/cc_compiledscript.h"

#include "cs/cc_instance.h"
#include "cs/cs_common.h"
#include "cs/cs_runtime.h"
#include "cs/cc_options.h"
#include "cs/cc_error.h"
#include "cs/cc_treemap.h"

#include "bigend.h"
*/

#error 'csrun.cpp' contents were split up to separate modules; do not include this file in the build

//extern void quit(char *);
//extern void write_log(char *);



// *** MAIN CLASS CODE STARTS **





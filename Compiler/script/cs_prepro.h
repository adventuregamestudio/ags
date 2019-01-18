/* Preprocessor for script compiler (c) 2000 Chris Jones
*/

//-----------------------------------------------------------------------------
//  Should be used only internally by cs_compiler.cpp
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// NOTE: All macro processing is currently done within the Editor.
//       These functions aren't called any more.

#ifndef __CS_PREPRO_H
#define __CS_PREPRO_H

#include "cc_macrotable.h"

extern void preproc_startup(MacroTable *preDefinedMacros);
extern void preproc_shutdown();

#endif // __CS_PREPRO_H

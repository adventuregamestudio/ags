/*
** 'C'-style script compiler
** Copyright (C) 2000-2001, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __CC_ERROR_H
#define __CC_ERROR_H

#include "cc_instance.h"  // ccInstance

// error reporting
extern int ccError;             // set to non-zero if error occurs
extern int ccErrorLine;         // line number of the error
extern char ccErrorString[400]; // description of the error
extern char ccErrorCallStack[400];
extern bool ccErrorIsUserError;
extern const char *ccCurScriptName; // name of currently compiling script

extern void ccGetCallStack(ccInstance *inst, char *buffer, int maxLines);
extern const char* ccGetSectionNameAtOffs(ccScript *scri, long offs);

#endif // __CC_ERROR_H
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

#ifndef __CS_COMPILER_H
#define __CS_COMPILER_H

#include "script/cc_script.h"  // ccScript

// ********* SCRIPT COMPILATION FUNCTIONS **************

// Clear the list of scripts that will be compiled in the next compilation
extern void ccRemoveDefaultHeaders(void);

// 'hd_content' will be saved for the next compilation, 'hd_name' is ignored
extern int ccAddDefaultHeader(char *hd_content, char *hd_name);

// Left in for compatability. Was used to set version for use with '#ifversion' macros
// Preprocessing happens elsewhere now, and so this is a stub that doesn't do anything
extern void ccSetSoftwareVersion(const char *version);

// compile the script supplied, returns nullptr on failure
// cc_error() gets called.
extern ccScript *ccCompileText(const char *script, const char *scriptName);


#endif // __CS_COMPILER_H

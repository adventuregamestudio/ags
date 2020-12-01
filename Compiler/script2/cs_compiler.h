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

#ifndef __CS_COMPILER2_H
#define __CS_COMPILER2_H
#include "script/cc_script.h"
// ********* SCRIPT COMPILATION FUNCTIONS **************

// compile the script supplied, returns nullptr on failure
// cc_error() gets called.
extern ccScript *ccCompileText2(char const *script, char const *scriptName, long options);

#endif // __CS_COMPILER2_H

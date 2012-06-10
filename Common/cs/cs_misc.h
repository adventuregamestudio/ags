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

#ifndef __CS_MISC_H
#define __CS_MISC_H

// *********** MISCELLANEOUS FUNCTIONS ***************
#define SCOPT_EXPORTALL      1   // export all functions automatically
#define SCOPT_SHOWWARNINGS   2   // printf warnings to console
#define SCOPT_LINENUMBERS    4   // include line numbers in compiled code
#define SCOPT_AUTOIMPORT     8   // when creating instance, export funcs to other scripts
#define SCOPT_DEBUGRUN    0x10   // write instructions as they are procssed to log file
#define SCOPT_NOIMPORTOVERRIDE 0x20 // do not allow an import to be re-declared
#define SCOPT_LEFTTORIGHT 0x40   // left-to-right operator precedance
#define SCOPT_OLDSTRINGS  0x80   // allow old-style strings

extern void ccSetOption(int, int);
extern int ccGetOption(int);

// error reporting
extern int ccError;             // set to non-zero if error occurs
extern int ccErrorLine;         // line number of the error
extern char ccErrorString[400]; // description of the error
extern char ccErrorCallStack[400];
extern bool ccErrorIsUserError;
extern const char *ccCurScriptName; // name of currently compiling script

extern void ccGetCallStack(ccInstance *inst, char *buffer, int maxLines);

#endif // __CS_MISC_H
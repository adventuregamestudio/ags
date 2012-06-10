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

#ifndef __CSPREPRO_H
#define __CSPREPRO_H

// These declarations are moved from cscomp.h, since they are used
// only internally in the compiler and have no need to be exposed.

extern int is_alphanum(int);

extern void cc_preprocess(char *, char *);
extern void preproc_startup(void);
extern void preproc_shutdown(void);

#endif
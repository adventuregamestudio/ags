/*
** Adventure Game Studio GUI routines
** Copyright (C) 2000-2005, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __ACGUI_H
#define __ACGUI_H

#include "bigend.h"

#include <math.h>
extern int loaded_game_file_version;

#ifndef __WGT4_H
#ifndef CROOM_NOFUNCTIONS
#error Must include wgt2allg.h first
#endif
#endif

#error Do not include acgui.h for now







extern char lines[][200];
extern int numlines;
extern void removeBackslashBracket(char *lbuffer);
extern void split_lines_leftright(const char *todis, int wii, int fonnt);

#endif // __ACGUI_H

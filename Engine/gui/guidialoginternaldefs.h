//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_GUI__GUIDIALOGINTERNALDEFS_H
#define __AGS_EE_GUI__GUIDIALOGINTERNALDEFS_H

#include "gui/guidialogdefines.h"

#if !defined (WINDOWS_VERSION)
#define _getcwd getcwd
#endif

#undef kbhit
extern int rec_getch();
extern int rec_kbhit();
#define kbhit rec_kbhit
#define getch() rec_getch()

#define domouse rec_domouse

#define wbutt __my_wbutt

#define _export
#ifdef WINAPI
#undef WINAPI
#endif
#define WINAPI
#define mbutrelease !rec_misbuttondown
#define TEXT_HT usetup.textheight

#endif // __AGS_EE_GUI__GUIDIALOGINTERNALDEFS_H
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
#ifndef __AGS_EE_GUI__CSCIDIALOG_H
#define __AGS_EE_GUI__CSCIDIALOG_H

#include "gui/guidialoginternaldefs.h"

int  WINAPI _export CSCIGetVersion();
int  WINAPI _export CSCIDrawWindow(int xx, int yy, int wid, int hit);
void WINAPI _export CSCIEraseWindow(int handl);
int  WINAPI _export CSCIWaitMessage(CSCIMessage * cscim);
int  WINAPI _export CSCICreateControl(int typeandflags, int xx, int yy, int wii, int hii, char *title);
void WINAPI _export CSCIDeleteControl(int haa);
int  WINAPI _export CSCISendControlMessage(int haa, int mess, int wPar, long lPar);
void multiply_up_to_game_res(int *x, int *y);
void multiply_up(int *x1, int *y1, int *x2, int *y2);
int  checkcontrols();
int  finddefaultcontrol(int flagmask);
int  GetBaseWidth ();

#endif // __AGS_EE_GUI__CSCIDIALOG_H

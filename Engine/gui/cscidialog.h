/*
    Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
    All rights reserved.

    The AGS Editor Source Code is provided under the Artistic License 2.0
    http://www.opensource.org/licenses/artistic-license-2.0.php

    You MAY NOT compile your own builds of the engine without making it EXPLICITLY
    CLEAR that the code has been altered from the Standard Version.

*/

//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_GUI__CSCIDIALOG_H
#define __AGS_EE_GUI__CSCIDIALOG_H

#include "gui/dialoginternaldefines.h"

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

#endif // __AGS_EE_GUI__CSCIDIALOG_H

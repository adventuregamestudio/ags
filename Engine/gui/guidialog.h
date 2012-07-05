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
#ifndef __AGS_EE_GUI__GUIDIALOG_H
#define __AGS_EE_GUI__GUIDIALOG_H

void refresh_screen();
int  loadgamedialog();
int  savegamedialog();
void preparesavegamelist(int ctrllist);
void enterstringwindow(char *prompttext, char *stouse);
int  enternumberwindow(char *prompttext);
int  roomSelectorWindow(int currentRoom, int numRooms, int*roomNumbers, char**roomNames);
int  myscimessagebox(char *lpprompt, char *btn1, char *btn2);
int  quitdialog();

#endif // __AGS_EE_GUI__GUIDIALOG_H

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
#ifndef __AGS_EE_GUI__GUIDIALOG_H
#define __AGS_EE_GUI__GUIDIALOG_H

#include "util/array.h"
#include "util/string.h"

using namespace AGS; // FIXME later

void refresh_screen();
int  loadgamedialog();
int  savegamedialog();
void preparesavegamelist(int ctrllist);
void enterstringwindow(char *prompttext, char *stouse);
int  enternumberwindow(char *prompttext);
int  roomSelectorWindow(int currentRoom, int numRooms,
                        const Common::Array<int32_t> &roomNumbers, const Common::ObjectArray<Common::String> &roomNames);
int  myscimessagebox(char *lpprompt, char *btn1, char *btn2);
int  quitdialog();

#endif // __AGS_EE_GUI__GUIDIALOG_H

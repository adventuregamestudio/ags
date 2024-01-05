//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Default built-in game dialogs.
//
//=============================================================================
#ifndef __AGS_EE_GUI__GUIDIALOG_H
#define __AGS_EE_GUI__GUIDIALOG_H

#include <vector>
#include "util/string.h"

namespace AGS { namespace Common { class Bitmap; } }

// Functions for handling hard-coded GUIs
// Prepares GUI bitmaps which will be passed to the renderer's draw chain
AGS::Common::Bitmap *prepare_gui_screen(int x, int y, int width, int height, bool opaque);
AGS::Common::Bitmap *get_gui_screen();
// Deletes GUI bitmaps
void clear_gui_screen();
// Draws virtual screen contents on the GUI bitmaps and assignes them to
// the renderer's draw chain
void refresh_gui_screen();
int  loadgamedialog();
int  savegamedialog();
void preparesavegamelist(int ctrllist);
void enterstringwindow(const char *prompttext, char *stouse);
int  enternumberwindow(char *prompttext);
int  roomSelectorWindow(int currentRoom, int numRooms,
    const std::vector<int> &roomNumbers, const std::vector<AGS::Common::String> &roomNames);
int  myscimessagebox(const char *lpprompt, char *btn1, char *btn2);
int  quitdialog();

// last string value in gui dialog.
char *get_gui_dialog_buffer();

#endif // __AGS_EE_GUI__GUIDIALOG_H

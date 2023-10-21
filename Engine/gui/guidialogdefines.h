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
// Constants for built-in GUI dialogs.
//
//=============================================================================
#ifndef __AGS_EE_GUI__GUIDIALOGDEFINES_H
#define __AGS_EE_GUI__GUIDIALOGDEFINES_H

// These were GlobalMessages (984 - 995).
#define MSG_RESTORE      0     // "Restore"
#define MSG_CANCEL       1     // "Cancel"
#define MSG_SELECTLOAD   2     // "Select game to restore"
#define MSG_SAVEBUTTON   3     // "Save"
#define MSG_SAVEDIALOG   4     // "Save game name:"
#define MSG_REPLACE      5     // "Replace"
#define MSG_MUSTREPLACE  6     // "The folder is full. you must replace"
#define MSG_REPLACEWITH1 7     // "Replace:"
#define MSG_REPLACEWITH2 8     // "With:"
#define MSG_QUITBUTTON   9     // "Quit"
#define MSG_PLAYBUTTON   10    // "Play"
#define MSG_QUITDIALOG   11    // "Do you want to quit?"
#define NUM_GUIDIALOGMSG 12

#include "ac/gamesetup.h"
#define TEXT_HT usetup.textheight

/*#define COL251 26
#define COL252 28
#define COL253 29
#define COL254 27
#define COL255 24*/
#define COL253 15
#define COL254 7
#define COL255 8

//  =========  DEFINES  ========
// Control types
#define CNT_PUSHBUTTON 0x001
#define CNT_LISTBOX    0x002
#define CNT_LABEL      0x003
#define CNT_TEXTBOX    0x004
// Control properties
#define CNF_DEFAULT    0x100
#define CNF_CANCEL     0x200

// Dialog messages
#define CM_COMMAND   1
#define CM_KEYPRESS  2
#define CM_SELCHANGE 3
// System messages
#define SM_SAVEGAME  100
#define SM_LOADGAME  101
#define SM_QUIT      102
// System messages (to ADVEN)
#define SM_SETTRANSFERMEM 120
#define SM_GETINIVALUE    121
// System messages (to driver)
#define SM_QUERYQUIT 110
#define SM_KEYPRESS  111
#define SM_TIMER     112
// ListBox messages
#define CLB_ADDITEM   1
#define CLB_CLEAR     2
#define CLB_GETCURSEL 3
#define CLB_GETTEXT   4
#define CLB_SETTEXT   5
#define CLB_SETCURSEL 6
// TextBox messages
#define CTB_GETTEXT   1
#define CTB_SETTEXT   2

#define CTB_KEYPRESS 91

namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS; // FIXME later

struct CSCIMessage
{
    int code;
    int id;
    int wParam;
};

struct OnScreenWindow
{
    int x, y;
    int handle;
    int oldtop;

    OnScreenWindow();
};

#endif // __AGS_EE_GUI__GUIDIALOGDEFINES_H
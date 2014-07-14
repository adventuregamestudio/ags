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

#ifndef __AC_GUIDEFINES_H
#define __AC_GUIDEFINES_H

#define MAX_LISTBOX_ITEMS 200
#define MAX_GUILABEL_TEXT_LEN 2048
#define GUIMAGIC          0xcafebeef
//#define MAX_OBJ_EACH_TYPE 251

#define MAXLINE 50

// TODO: find out more about gui version history
//=============================================================================
// GUI Version history
//-----------------------------------------------------------------------------
//
// 2.1.4..... (100): Introduced Slider gui control. Gui count is now serialized
//                   in game file.
// 2.2.2..... (101): Introduced TextBox gui control.
// 2.3.0..... (102): Introduced ListBox gui control.
// 2.6.0..... (105): GUI custom Z-order support.
// 2.7.0..... (110): Added GUI OnClick handler.
// 2.7.2.???? (111): Added text alignment property to buttons.
// 2.7.2.???? (112): Added text alignment property to list boxes.
// 2.7.2.???? (113): Increased permitted length of GUI label text from 200 to
//                   2048 characters.
// 2.7.2.???? (114): Obsoleted savegameindex[] array, and added
//                   ListBox.SaveGameSlots[] array instead.
// 2.7.2.???? (115): Added GUI Control z-order support.
//
// 3.3.0.1132 (116): Added GUIF_TRANSLATED flag.
// 3.3.1.???? (117): Added padding variable for text window GUIs.
//
//=============================================================================

enum GuiVersion
{
    // TODO: find out all corresponding engine version numbers
    kGuiVersion_Initial     = 0,
    kGuiVersion_214         = 100,
    kGuiVersion_222         = 101,
    kGuiVersion_230         = 102,
    kGuiVersion_unkn_103    = 103,
    kGuiVersion_unkn_104    = 104,
    kGuiVersion_260         = 105,
    kGuiVersion_unkn_106    = 106,
    kGuiVersion_unkn_107    = 107,
    kGuiVersion_unkn_108    = 108,
    kGuiVersion_unkn_109    = 109,
    kGuiVersion_270         = 110,
    kGuiVersion_272a        = 111,
    kGuiVersion_272b        = 112,
    kGuiVersion_272c        = 113,
    kGuiVersion_272d        = 114,
    kGuiVersion_272e        = 115,

    kGuiVersion_330         = 116,
    kGuiVersion_331         = 117,
    kGuiVersion_Current     = kGuiVersion_331,
    kGuiVersion_ForwardCompatible = kGuiVersion_272e
};

#endif // __AC_GUIDEFINES_H

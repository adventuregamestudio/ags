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
// GUI Control flags (32-bit)
#define GUIF_DEFAULT    0x0001
#define GUIF_CANCEL     0x0002 // obsolete?
#define GUIF_DISABLED   0x0004
#define GUIF_TABSTOP    0x0008 // obsolete?
#define GUIF_INVISIBLE  0x0010
#define GUIF_CLIP       0x0020
#define GUIF_NOCLICKS   0x0040
#define GUIF_TRANSLATED 0x0080 // 3.3.0.1132
#define GUIF_DELETED    0x8000
#define MAX_GUIOBJ_SCRIPTNAME_LEN 25
#define MAX_GUIOBJ_EVENTS 10
#define MAX_GUIOBJ_EVENTHANDLER_LEN 30
#define TEXTWINDOW_PADDING_DEFAULT  3
// ListBox flags
#define GLF_NOBORDER     1
#define GLF_NOARROWS     2
#define GLF_SGINDEXVALID 4
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
// 3.4.0      (118): Removed GUI limits
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
    kGuiVersion_340         = 118,
    kGuiVersion_Current     = kGuiVersion_340,
    // Defines the oldest version of gui data that is complying to current
    // savedgame format; if the loaded game data is of this version or lower,
    // then this value will be written to savedgame instead of current version.
    kGuiVersion_ForwardCompatible = kGuiVersion_272e
};

namespace AGS
{
namespace Common
{

enum GUIMainFlags
{
    kGUIMain_NoClick    = 0x01,
    kGUIMain_TextWindow = 0x02
};

enum GUIMainLegacyFlags
{
    kGUIMain_LegacyTextWindow = 5
};

enum GUIPopupStyle
{
    // normal GUI, initally on
    kGUIPopupNone             = 0,
    // show when mouse moves to top of screen
    kGUIPopupMouseY           = 1,
    // pauses game when shown
    kGUIPopupModal            = 2,
    // initially on and not removed when interface is off
    kGUIPopupNoAutoRemove     = 3,
    // normal GUI, initially off
    kGUIPopupNoneInitiallyOff = 4
};

enum GUIControlType
{
    kGUIControlUndefined = -1,
    kGUIButton      = 1,
    kGUILabel       = 2,
    kGUIInvWindow   = 3,
    kGUISlider      = 4,
    kGUITextBox     = 5,
    kGUIListBox     = 6
};

enum GUITextBoxFlags
{
    kTextBox_NoBorder = 0x0001
};

} // namespace Common
} // namespace AGS

extern int guis_need_update;

#endif // __AC_GUIDEFINES_H

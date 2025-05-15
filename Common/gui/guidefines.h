//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_GUIDEFINES_H
#define __AC_GUIDEFINES_H

#define GUIMAGIC          0xcafebeef
#define MAX_GUIOBJ_EVENTS 10
#define TEXTWINDOW_PADDING_DEFAULT  3

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
// 3.3.0.1132 (116): Added kGUICtrl_Translated flag.
// 3.3.1.???? (117): Added padding variable for text window GUIs.
// 3.4.0      (118): Removed GUI limits
// 3.5.0      (119): Game data contains GUI properties that previously
//                   could be set only at runtime.
// Since then format value is defined as AGS version represented as NN,NN,NN,NN
//=============================================================================

enum GuiVersion
{
    kGuiVersion_Undefined   = 0,
    kGuiVersion_350         = 119,
    kGuiVersion_LowSupported = kGuiVersion_350,
    kGuiVersion_Current     = kGuiVersion_350,
};

namespace AGS
{
namespace Common
{

// GUIMain's style and behavior flags
enum GUIMainFlags
{
    kGUIMain_Clickable  = 0x0001,
    kGUIMain_TextWindow = 0x0002,
    kGUIMain_Visible    = 0x0004,
    kGUIMain_Concealed  = 0x0008,

    // NOTE: currently default state is Visible to keep this backwards compatible;
    // check later if this is still a wanted behavior
    kGUIMain_DefFlags   = kGUIMain_Clickable | kGUIMain_Visible
};

// GUIMain's style of getting displayed on screen
enum GUIPopupStyle
{
    // Normal GUI
    kGUIPopupNormal           = 0,
    // Shown when the mouse cursor moves to the top of the screen
    kGUIPopupMouseY           = 1,
    // Same as Normal, but pauses the game when shown
    kGUIPopupModal            = 2,
    // Same as Normal, but is not removed when interface is off
    kGUIPopupNoAutoRemove     = 3
};

// The type of GUIControl
enum GUIControlType
{
    kGUIControlUndefined = -1,
    kGUIButton      = 1,
    kGUILabel       = 2,
    kGUIInvWindow   = 3,
    kGUISlider      = 4,
    kGUITextBox     = 5,
    kGUIListBox     = 6,
    kGUIControlTypeNum
};

// GUIControl general style and behavior flags
enum GUIControlFlags
{
    kGUICtrl_Default    = 0x0001, // only button
    kGUICtrl_Cancel     = 0x0002, // unused
    kGUICtrl_Enabled    = 0x0004,
    kGUICtrl_TabStop    = 0x0008, // unused
    kGUICtrl_Visible    = 0x0010,
    kGUICtrl_Clip       = 0x0020, // only button
    kGUICtrl_Clickable  = 0x0040,
    kGUICtrl_Translated = 0x0080, // 3.3.0.1132
    kGUICtrl_WrapText   = 0x0100, // 3.6.2
    kGUICtrl_Deleted    = 0x8000, // unused (probably remains from the old editor?)

    kGUICtrl_DefFlags   = kGUICtrl_Enabled | kGUICtrl_Visible | kGUICtrl_Clickable |
                          kGUICtrl_Translated,
};

// Label macro flags, define which macros are present in the Label's Text
enum GUILabelMacro
{
    kLabelMacro_None        = 0,
    kLabelMacro_Gamename    = 0x01,
    kLabelMacro_Overhotspot = 0x02,

    kLabelMacro_All         = 0xFFFF
};

// GUIListBox style and behavior flags
enum GUIListBoxFlags
{
    kListBox_ShowBorder = 0x01,
    kListBox_ShowArrows = 0x02,
    kListBox_SvgIndex   = 0x04,

    kListBox_DefFlags   = kListBox_ShowBorder | kListBox_ShowArrows
};

// GUITextBox style and behavior flags
enum GUITextBoxFlags
{
    kTextBox_ShowBorder = 0x0001,

    kTextBox_DefFlags   = kTextBox_ShowBorder
};

// GUI runtime save format
// TODO: move to the engine code
enum GuiSvgVersion
{
    kGuiSvgVersion_Initial  = 0,
    kGuiSvgVersion_350,
    kGuiSvgVersion_36020,
    kGuiSvgVersion_36023,
    kGuiSvgVersion_36025,
    kGuiSvgVersion_36200    = 3060200, // re-added control refs
    kGuiSvgVersion_36202    = 3060202,
    kGuiSvgVersion_400      = 4000000,
    kGuiSvgVersion_40008    = 4000008, // custom properties
    kGuiSvgVersion_40009    = 4000009, // 32-bit color properties
    kGuiSvgVersion_40010    = 4000010, // compat with kGuiSvgVersion_36200-36202
    kGuiSvgVersion_40014    = 4000014, // proper ARGB color properties
    kGuiSvgVersion_40016    = 4000016, // extended graphic properties for controls
    kGuiSvgVersion_40018    = 4000018, // shaders
};

// Style of GUI drawing in disabled state
enum GuiDisableStyle
{
    kGuiDis_Undefined = -1, // this is for marking not-disabled state
    kGuiDis_Greyout   = 0, // paint "gisabled" effect over controls
    kGuiDis_Blackout  = 1, // invisible controls (but guis are shown
    kGuiDis_Unchanged = 2, // no change, display normally
    kGuiDis_Off       = 3  // fully invisible guis
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUIDEFINES_H

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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GUI_H
#define __AGS_EE_AC__GUI_H

#include "ac/dynobj/scriptgui.h"
#include "gui/guidefines.h"
#include "gui/guimain.h"

ScriptGUI *GUI_AsTextWindow(ScriptGUI *tehgui);
int     GUI_GetPopupStyle(ScriptGUI *tehgui);
void	GUI_SetVisible(ScriptGUI *tehgui, int isvisible);
int		GUI_GetVisible(ScriptGUI *tehgui);
int		GUI_GetX(ScriptGUI *tehgui);
void	GUI_SetX(ScriptGUI *tehgui, int xx);
int		GUI_GetY(ScriptGUI *tehgui);
void	GUI_SetY(ScriptGUI *tehgui, int yy);
void	GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy);
void	GUI_SetSize(ScriptGUI *sgui, int widd, int hitt);
int		GUI_GetWidth(ScriptGUI *sgui);
int		GUI_GetHeight(ScriptGUI *sgui);
void	GUI_SetWidth(ScriptGUI *sgui, int newwid);
void	GUI_SetHeight(ScriptGUI *sgui, int newhit);
void	GUI_SetZOrder(ScriptGUI *tehgui, int z);
int		GUI_GetZOrder(ScriptGUI *tehgui);
void	GUI_SetClickable(ScriptGUI *tehgui, int clickable);
int		GUI_GetClickable(ScriptGUI *tehgui);
int		GUI_GetID(ScriptGUI *tehgui);
AGS::Common::GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx);
int		GUI_GetControlCount(ScriptGUI *tehgui);
void    GUI_SetPopupYPos(ScriptGUI *tehgui, int newpos);
int     GUI_GetPopupYPos(ScriptGUI *tehgui);
void	GUI_SetTransparency(ScriptGUI *tehgui, int trans);
int		GUI_GetTransparency(ScriptGUI *tehgui);
void	GUI_Centre(ScriptGUI *sgui);
void	GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn);
int		GUI_GetBackgroundGraphic(ScriptGUI *tehgui);
void    GUI_SetBackgroundColor(ScriptGUI *tehgui, int newcol);
int     GUI_GetBackgroundColor(ScriptGUI *tehgui);
void    GUI_SetBorderColor(ScriptGUI *tehgui, int newcol);
int     GUI_GetBorderColor(ScriptGUI *tehgui);
void    GUI_SetTextColor(ScriptGUI *tehgui, int newcol);
int     GUI_GetTextColor(ScriptGUI *tehgui);
void    GUI_SetTextPadding(ScriptGUI *tehgui, int newpos);
int     GUI_GetTextPadding(ScriptGUI *tehgui);
ScriptGUI *GetGUIAtLocation(int xx, int yy);

void	remove_popup_interface(int ifacenum);
void	process_interface_click(int ifce, int btn, int mbut);
void	replace_macro_tokens(const char *text, AGS::Common::String &fixed_text);
void	update_gui_zorder();
// Initializes all GUI and controls for runtime state;
// this must be done after creating or loading GUIs.
void    prepare_gui_runtime(bool startup);
void	export_gui_controls(int ee);
void	unexport_gui_controls(int ee);
void	update_gui_disabled_status();
int		adjust_x_for_guis(int xx, int yy, bool assume_blocking = false);
int		adjust_y_for_guis(int yy, bool assume_blocking = false);

int     gui_get_interactable(int x,int y);
int     gui_on_mouse_move(const int mx, const int my);
void    gui_on_mouse_hold(const int wasongui, const int wasbutdown);
void    gui_on_mouse_up(const int wasongui, const int wasbutdown, const int mx, const int my);
void    gui_on_mouse_down(const int guin, const int mbut, const int mx, const int my);

extern int ifacepopped;

extern int ifacepopped;
extern int mouse_on_iface;
extern std::vector<AGS::Common::GUIMain> guis;
extern std::vector<AGS::Common::GUIButton> guibuts;
extern std::vector<AGS::Common::GUIInvWindow> guiinv;
extern std::vector<AGS::Common::GUILabel> guilabels;
extern std::vector<AGS::Common::GUIListBox> guilist;
extern std::vector<AGS::Common::GUISlider> guislider;
extern std::vector<AGS::Common::GUITextBox> guitext;

namespace AGS
{
namespace Engine
{
namespace GUIE
{
    // Mark all existing GUI for redraw
    void MarkAllGUIForUpdate(bool redraw, bool reset_over_ctrl);
    // Mark all translatable GUI controls for redraw
    void MarkForTranslationUpdate();
    // Mark all GUI which use the given font for recalculate/redraw;
    // pass -1 to update all the textual controls together
    void MarkForFontUpdate(int font);
    // Mark labels that acts as special text placeholders for redraw
    void MarkSpecialLabelsForUpdate(AGS::Common::GUILabelMacro macro);
    // Mark inventory windows for redraw, optionally only ones linked to given character;
    // also marks buttons with inventory icon mode;
    // pass char_id = -1 to update all inventory windows at once.
    void MarkInventoryForUpdate(int char_id, bool is_player);
    // Mark all *disabled* GUI controls for redraw;
    // this function is used when the "disabled style" changes at runtime
    void MarkDisabledGUIForUpdate();
} // namespace GUIE
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_AC__GUI_H

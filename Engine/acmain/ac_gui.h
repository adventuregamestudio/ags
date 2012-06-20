
#include "ac/dynobj/scriptgui.h"

int GetBaseWidth ();
void remove_popup_interface(int ifacenum);
void process_interface_click(int ifce, int btn, int mbut);
void replace_macro_tokens(char*statusbarformat,char*cur_stb_text);
void update_gui_zorder();
void export_gui_controls(int ee);
void unexport_gui_controls(int ee);
int convert_gui_disabled_style(int oldStyle);
void update_gui_disabled_status();
int adjust_x_for_guis (int xx, int yy);
int adjust_y_for_guis ( int yy);
int IsGUIOn (int guinum);
// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int FindGUIID (const char* GUIName);
void InterfaceOn(int ifn);
void InterfaceOff(int ifn);
void GUI_SetVisible(ScriptGUI *tehgui, int isvisible);
int GUI_GetVisible(ScriptGUI *tehgui);
void SetGUIObjectEnabled(int guin, int objn, int enabled);
void SetGUIObjectPosition(int guin, int objn, int xx, int yy);
int GUI_GetX(ScriptGUI *tehgui);
void GUI_SetX(ScriptGUI *tehgui, int xx);
int GUI_GetY(ScriptGUI *tehgui);
void GUI_SetY(ScriptGUI *tehgui, int yy);
void GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy);
void SetGUIPosition(int ifn,int xx,int yy);
void SetGUIObjectSize(int ifn, int objn, int newwid, int newhit);
void recreate_guibg_image(GUIMain *tehgui);
void GUI_SetSize(ScriptGUI *sgui, int widd, int hitt);
int GUI_GetWidth(ScriptGUI *sgui);
int GUI_GetHeight(ScriptGUI *sgui);
void GUI_SetWidth(ScriptGUI *sgui, int newwid);
void GUI_SetHeight(ScriptGUI *sgui, int newhit);
void SetGUISize (int ifn, int widd, int hitt);
void GUI_SetZOrder(ScriptGUI *tehgui, int z);
int GUI_GetZOrder(ScriptGUI *tehgui);
void SetGUIZOrder(int guin, int z);
void GUI_SetClickable(ScriptGUI *tehgui, int clickable);
int GUI_GetClickable(ScriptGUI *tehgui);
void SetGUIClickable(int guin, int clickable);
int GUI_GetID(ScriptGUI *tehgui);
GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx);
int GUI_GetControlCount(ScriptGUI *tehgui);
void GUI_SetTransparency(ScriptGUI *tehgui, int trans);
int GUI_GetTransparency(ScriptGUI *tehgui);
// pass trans=0 for fully solid, trans=100 for fully transparent
void SetGUITransparency(int ifn, int trans);
void GUI_Centre(ScriptGUI *sgui);
void CentreGUI (int ifn);
int GetTextWidth(char *text, int fontnum);
int GetTextHeight(char *text, int fontnum, int width);
void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn);
int GUI_GetBackgroundGraphic(ScriptGUI *tehgui);
void SetGUIBackgroundPic (int guin, int slotn);
void DisableInterface();
void EnableInterface();
// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled();
extern int ifacepopped;  // currently displayed pop-up GUI (-1 if none);

extern int mouse_on_iface;   // mouse cursor is over this interface
extern int mouse_on_iface_button;
extern int mouse_pushed_iface;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;

extern int eip_guinum, eip_guiobj;





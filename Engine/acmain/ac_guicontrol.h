
#include "acgui/ac_guiobject.h"
#include "acgui/ac_guibutton.h"
#include "acgui/ac_guiinv.h"
#include "acgui/ac_guilabel.h"
#include "acgui/ac_guilistbox.h"
#include "acgui/ac_guislider.h"
#include "acgui/ac_guitextbox.h"
#include "acrun/ac_scriptobject.h"

GUIObject *GetGUIControlAtLocation(int xx, int yy);
int GetGUIObjectAt (int xx, int yy);
int GUIControl_GetVisible(GUIObject *guio);
void GUIControl_SetVisible(GUIObject *guio, int visible);
int GUIControl_GetClickable(GUIObject *guio);
void GUIControl_SetClickable(GUIObject *guio, int enabled);
int GUIControl_GetEnabled(GUIObject *guio);
void GUIControl_SetEnabled(GUIObject *guio, int enabled);
int GUIControl_GetID(GUIObject *guio);
ScriptGUI* GUIControl_GetOwningGUI(GUIObject *guio);
GUIButton* GUIControl_GetAsButton(GUIObject *guio);
GUIInv* GUIControl_GetAsInvWindow(GUIObject *guio);
GUILabel* GUIControl_GetAsLabel(GUIObject *guio);
GUIListBox* GUIControl_GetAsListBox(GUIObject *guio);
GUISlider* GUIControl_GetAsSlider(GUIObject *guio);
GUITextBox* GUIControl_GetAsTextBox(GUIObject *guio);
int GUIControl_GetX(GUIObject *guio);
void GUIControl_SetX(GUIObject *guio, int xx);
int GUIControl_GetY(GUIObject *guio);
void GUIControl_SetY(GUIObject *guio, int yy);
void GUIControl_SetPosition(GUIObject *guio, int xx, int yy);
int GUIControl_GetWidth(GUIObject *guio);
void GUIControl_SetWidth(GUIObject *guio, int newwid);
int GUIControl_GetHeight(GUIObject *guio);
void GUIControl_SetHeight(GUIObject *guio, int newhit);
void GUIControl_SetSize(GUIObject *guio, int newwid, int newhit);
void GUIControl_SendToBack(GUIObject *guio);
void GUIControl_BringToFront(GUIObject *guio);

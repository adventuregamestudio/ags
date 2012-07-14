
#include <stdio.h>
#include "util/wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "ac/common.h"
#include "ac/guicontrol.h"
#include "ac/global_gui.h"
#include "debug/debug.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"

extern GUIMain*guis;
extern ScriptGUI*scrGui;

GUIObject *GetGUIControlAtLocation(int xx, int yy) {
    int guinum = GetGUIAt(xx, yy);
    if (guinum == -1)
        return NULL;

    multiply_up_coordinates(&xx, &yy);

    int oldmousex = mousex, oldmousey = mousey;
    mousex = xx - guis[guinum].x;
    mousey = yy - guis[guinum].y;
    int toret = guis[guinum].find_object_under_mouse(0, false);
    mousex = oldmousex;
    mousey = oldmousey;
    if (toret < 0)
        return NULL;

    return guis[guinum].objs[toret];
}

int GUIControl_GetVisible(GUIObject *guio) {
  return guio->IsVisible();
}

void GUIControl_SetVisible(GUIObject *guio, int visible) 
{
  if (visible != guio->IsVisible()) 
  {
    if (visible)
      guio->Show();
    else
      guio->Hide();

    guis[guio->guin].control_positions_changed();
    guis_need_update = 1;
  }
}

int GUIControl_GetClickable(GUIObject *guio) {
  if (guio->IsClickable())
    return 1;
  return 0;
}

void GUIControl_SetClickable(GUIObject *guio, int enabled) {
  if (enabled)
    guio->SetClickable(true);
  else
    guio->SetClickable(false);

  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetEnabled(GUIObject *guio) {
  if (guio->IsDisabled())
    return 0;
  return 1;
}

void GUIControl_SetEnabled(GUIObject *guio, int enabled) {
  if (enabled)
    guio->Enable();
  else
    guio->Disable();

  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}


int GUIControl_GetID(GUIObject *guio) {
  return guio->objn;
}

ScriptGUI* GUIControl_GetOwningGUI(GUIObject *guio) {
  return &scrGui[guio->guin];
}

GUIButton* GUIControl_GetAsButton(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_BUTTON)
    return NULL;

  return (GUIButton*)guio;
}

GUIInv* GUIControl_GetAsInvWindow(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_INVENTORY)
    return NULL;

  return (GUIInv*)guio;
}

GUILabel* GUIControl_GetAsLabel(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_LABEL)
    return NULL;

  return (GUILabel*)guio;
}

GUIListBox* GUIControl_GetAsListBox(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_LISTBOX)
    return NULL;

  return (GUIListBox*)guio;
}

GUISlider* GUIControl_GetAsSlider(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_SLIDER)
    return NULL;

  return (GUISlider*)guio;
}

GUITextBox* GUIControl_GetAsTextBox(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_TEXTBOX)
    return NULL;

  return (GUITextBox*)guio;
}

int GUIControl_GetX(GUIObject *guio) {
  return divide_down_coordinate(guio->x);
}

void GUIControl_SetX(GUIObject *guio, int xx) {
  guio->x = multiply_up_coordinate(xx);
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetY(GUIObject *guio) {
  return divide_down_coordinate(guio->y);
}

void GUIControl_SetY(GUIObject *guio, int yy) {
  guio->y = multiply_up_coordinate(yy);
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

void GUIControl_SetPosition(GUIObject *guio, int xx, int yy) {
  GUIControl_SetX(guio, xx);
  GUIControl_SetY(guio, yy);
}


int GUIControl_GetWidth(GUIObject *guio) {
  return divide_down_coordinate(guio->wid);
}

void GUIControl_SetWidth(GUIObject *guio, int newwid) {
  guio->wid = multiply_up_coordinate(newwid);
  guio->Resized();
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetHeight(GUIObject *guio) {
  return divide_down_coordinate(guio->hit);
}

void GUIControl_SetHeight(GUIObject *guio, int newhit) {
  guio->hit = multiply_up_coordinate(newhit);
  guio->Resized();
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

void GUIControl_SetSize(GUIObject *guio, int newwid, int newhit) {
  if ((newwid < 2) || (newhit < 2))
    quit("!SetGUIObjectSize: new size is too small (must be at least 2x2)");

  DEBUG_CONSOLE("SetGUIObject %d,%d size %d,%d", guio->guin, guio->objn, newwid, newhit);
  GUIControl_SetWidth(guio, newwid);
  GUIControl_SetHeight(guio, newhit);
}

void GUIControl_SendToBack(GUIObject *guio) {
  if (guis[guio->guin].send_to_back(guio->objn))
    guis_need_update = 1;
}

void GUIControl_BringToFront(GUIObject *guio) {
  if (guis[guio->guin].bring_to_front(guio->objn))
    guis_need_update = 1;
}

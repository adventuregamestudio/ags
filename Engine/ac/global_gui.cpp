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

#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/gui.h"
#include "ac/guicontrol.h"
#include "ac/mouse.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "game/script_objects.h"
#include "gui/guimain.h"
#include "script/runtimescriptvalue.h"

int IsGUIOn (int guinum) {
    if ((guinum < 0) || (guinum >= game.GuiCount))
        quit("!IsGUIOn: invalid GUI number specified");
    return (guis[guinum].IsVisible()) ? 1 : 0;
}

// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int FindGUIID (const char* GUIName) {
    for (int ii = 0; ii < game.GuiCount; ii++) {
        if (strcmp(guis[ii].Name, GUIName) == 0)
            return ii;
        if ((guis[ii].Name[0] == 'g') && (stricmp(guis[ii].Name.GetCStr() + 1, GUIName) == 0))
            return ii;
    }
    quit("FindGUIID: No matching GUI found: GUI may have been deleted");
    return -1;
}

void InterfaceOn(int ifn) {
  if ((ifn<0) | (ifn>=game.GuiCount))
    quit("!GUIOn: invalid GUI specified");

  EndSkippingUntilCharStops();

  if (guis[ifn].IsVisible()) {
    DEBUG_CONSOLE("GUIOn(%d) ignored (already on)", ifn);
    return;
  }
  guis_need_update = 1;
  guis[ifn].SetVisibility(Common::kGuiVisibility_On);
  DEBUG_CONSOLE("GUI %d turned on", ifn);
  // modal interface
  if (guis[ifn].PopupStyle==Common::kGuiPopupScript) PauseGame();
  else if (guis[ifn].PopupStyle==Common::kGuiPopupMouseY) guis[ifn].SetVisibility(Common::kGuiVisibility_Off);
  // clear the cached mouse position
  guis[ifn].OnControlPositionChanged();
  guis[ifn].Poll();
}

void InterfaceOff(int ifn) {
  if ((ifn<0) | (ifn>=game.GuiCount)) quit("!GUIOff: invalid GUI specified");
  if ((!guis[ifn].IsVisible()) && (guis[ifn].PopupStyle!=Common::kGuiPopupMouseY)) {
    DEBUG_CONSOLE("GUIOff(%d) ignored (already off)", ifn);
    return;
  }
  DEBUG_CONSOLE("GUI %d turned off", ifn);
  guis[ifn].SetVisibility(Common::kGuiVisibility_Off);
  if (guis[ifn].MouseOverControl>=0) {
    // Make sure that the overpic is turned off when the GUI goes off
    guis[ifn].Controls[guis[ifn].MouseOverControl]->OnMouseLeave();
    guis[ifn].MouseOverControl = -1;
  }
  guis[ifn].OnControlPositionChanged();
  guis_need_update = 1;
  // modal interface
  if (guis[ifn].PopupStyle==Common::kGuiPopupScript) UnPauseGame();
  else if (guis[ifn].PopupStyle==Common::kGuiPopupMouseY) guis[ifn].SetVisibility(Common::kGuiVisibility_Concealed);
}

void SetGUIObjectEnabled(int guin, int objn, int enabled) {
  if ((guin<0) || (guin>=game.GuiCount))
    quit("!SetGUIObjectEnabled: invalid GUI number");
  if ((objn<0) || (objn>=guis[guin].ControlCount))
    quit("!SetGUIObjectEnabled: invalid object number");

  GUIControl_SetEnabled(guis[guin].Controls[objn], enabled);
}

void SetGUIObjectPosition(int guin, int objn, int xx, int yy) {
  if ((guin<0) || (guin>=game.GuiCount))
    quit("!SetGUIObjectPosition: invalid GUI number");
  if ((objn<0) || (objn>=guis[guin].ControlCount))
    quit("!SetGUIObjectPosition: invalid object number");

  GUIControl_SetPosition(guis[guin].Controls[objn], xx, yy);
}

void SetGUIPosition(int ifn,int xx,int yy) {
  if ((ifn<0) || (ifn>=game.GuiCount))
    quit("!SetGUIPosition: invalid GUI number");
  
  GUI_SetPosition(&scrGui[ifn], xx, yy);
}

void SetGUIObjectSize(int ifn, int objn, int newwid, int newhit) {
  if ((ifn<0) || (ifn>=game.GuiCount))
    quit("!SetGUIObjectSize: invalid GUI number");

  if ((objn<0) || (objn >= guis[ifn].ControlCount))
    quit("!SetGUIObjectSize: invalid object number");

  GUIControl_SetSize(guis[ifn].Controls[objn], newwid, newhit);
}

void SetGUISize (int ifn, int widd, int hitt) {
  if ((ifn<0) || (ifn>=game.GuiCount))
    quit("!SetGUISize: invalid GUI number");

  GUI_SetSize(&scrGui[ifn], widd, hitt);
}

void SetGUIZOrder(int guin, int z) {
  if ((guin<0) || (guin>=game.GuiCount))
    quit("!SetGUIZOrder: invalid GUI number");

  GUI_SetZOrder(&scrGui[guin], z);
}

void SetGUIClickable(int guin, int clickable) {
  if ((guin<0) || (guin>=game.GuiCount))
    quit("!SetGUIClickable: invalid GUI number");

  GUI_SetClickable(&scrGui[guin], clickable);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetGUITransparency(int ifn, int trans) {
  if ((ifn < 0) | (ifn >= game.GuiCount))
    quit("!SetGUITransparency: invalid GUI number");

  GUI_SetTransparency(&scrGui[ifn], trans);
}

void CentreGUI (int ifn) {
  if ((ifn<0) | (ifn>=game.GuiCount))
    quit("!CentreGUI: invalid GUI number");

  GUI_Centre(&scrGui[ifn]);
}

int GetTextWidth(const char *text, int fontnum) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.FontCount))
    quit("!GetTextWidth: invalid font number.");

  return divide_down_coordinate(wgettextwidth_compensate(text, fontnum));
}

int GetTextHeight(const char *text, int fontnum, int width) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.FontCount))
    quit("!GetTextHeight: invalid font number.");

  int texthit = wgetfontheight(fontnum);

  break_up_text_into_lines(multiply_up_coordinate(width), fontnum, text);

  return divide_down_coordinate(texthit * numlines);
}

void SetGUIBackgroundPic (int guin, int slotn) {
  if ((guin<0) | (guin>=game.GuiCount))
    quit("!SetGUIBackgroundPic: invalid GUI number");

  GUI_SetBackgroundGraphic(&scrGui[guin], slotn);
}

void DisableInterface() {
  play.DisabledUserInterface++;
  guis_need_update = 1;
  set_mouse_cursor(CURS_WAIT);
  }

void EnableInterface() {
  guis_need_update = 1;
  play.DisabledUserInterface--;
  if (play.DisabledUserInterface<1) {
    play.DisabledUserInterface=0;
    set_default_cursor();
    }
  }
// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled() {
  return (play.DisabledUserInterface > 0) ? 0 : 1;
}

int GetGUIObjectAt (int xx, int yy) {
    GuiObject *toret = GetGUIControlAtLocation(xx, yy);
    if (toret == NULL)
        return -1;

    return toret->Id;
}

int GetGUIAt (int xx,int yy) {
    multiply_up_coordinates(&xx, &yy);

    int aa, ll;
    for (ll = game.GuiCount - 1; ll >= 0; ll--) {
        aa = play.GuiDrawOrder[ll];
        if (!guis[aa].IsVisible()) continue;
        if (guis[aa].Flags & Common::kGuiMain_NoClick) continue;
        if ((xx>=guis[aa].GetX()) & (yy>=guis[aa].GetY()) &
            (xx<=guis[aa].GetX()+guis[aa].GetWidth()) & (yy<=guis[aa].GetY()+guis[aa].GetHeight()))
            return aa;
    }
    return -1;
}

void SetTextWindowGUI (int guinum) {
    if ((guinum < -1) | (guinum >= game.GuiCount))
        quit("!SetTextWindowGUI: invalid GUI number");

    if (guinum < 0) ;  // disable it
    else if (!guis[guinum].IsTextWindow())
        quit("!SetTextWindowGUI: specified GUI is not a text window");

    if (play.SpeechTextWindowGuiIndex == game.Options[OPT_TWCUSTOM])
        play.SpeechTextWindowGuiIndex = guinum;
    game.Options[OPT_TWCUSTOM] = guinum;
}

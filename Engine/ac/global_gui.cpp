
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/gui.h"
#include "ac/guicontrol.h"
#include "ac/mouse.h"
#include "ac/string.h"
#include "debug/debug.h"
#include "gui/guimain.h"

extern GameSetupStruct game;
extern GUIMain*guis;
extern ScriptGUI *scrGui;

int IsGUIOn (int guinum) {
    if ((guinum < 0) || (guinum >= game.numgui))
        quit("!IsGUIOn: invalid GUI number specified");
    return (guis[guinum].on >= 1) ? 1 : 0;
}

// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int FindGUIID (const char* GUIName) {
    for (int ii = 0; ii < game.numgui; ii++) {
        if (strcmp(guis[ii].name, GUIName) == 0)
            return ii;
        if ((guis[ii].name[0] == 'g') && (stricmp(&guis[ii].name[1], GUIName) == 0))
            return ii;
    }
    quit("FindGUIID: No matching GUI found: GUI may have been deleted");
    return -1;
}

void InterfaceOn(int ifn) {
  if ((ifn<0) | (ifn>=game.numgui))
    quit("!GUIOn: invalid GUI specified");

  EndSkippingUntilCharStops();

  if (guis[ifn].on == 1) {
    DEBUG_CONSOLE("GUIOn(%d) ignored (already on)", ifn);
    return;
  }
  guis_need_update = 1;
  guis[ifn].on=1;
  DEBUG_CONSOLE("GUI %d turned on", ifn);
  // modal interface
  if (guis[ifn].popup==POPUP_SCRIPT) PauseGame();
  else if (guis[ifn].popup==POPUP_MOUSEY) guis[ifn].on=0;
  // clear the cached mouse position
  guis[ifn].control_positions_changed();
  guis[ifn].poll();
}

void InterfaceOff(int ifn) {
  if ((ifn<0) | (ifn>=game.numgui)) quit("!GUIOff: invalid GUI specified");
  if ((guis[ifn].on==0) && (guis[ifn].popup!=POPUP_MOUSEY)) {
    DEBUG_CONSOLE("GUIOff(%d) ignored (already off)", ifn);
    return;
  }
  DEBUG_CONSOLE("GUI %d turned off", ifn);
  guis[ifn].on=0;
  if (guis[ifn].mouseover>=0) {
    // Make sure that the overpic is turned off when the GUI goes off
    guis[ifn].objs[guis[ifn].mouseover]->MouseLeave();
    guis[ifn].mouseover = -1;
  }
  guis[ifn].control_positions_changed();
  guis_need_update = 1;
  // modal interface
  if (guis[ifn].popup==POPUP_SCRIPT) UnPauseGame();
  else if (guis[ifn].popup==POPUP_MOUSEY) guis[ifn].on=-1;
}

void SetGUIObjectEnabled(int guin, int objn, int enabled) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIObjectEnabled: invalid GUI number");
  if ((objn<0) || (objn>=guis[guin].numobjs))
    quit("!SetGUIObjectEnabled: invalid object number");

  GUIControl_SetEnabled(guis[guin].objs[objn], enabled);
}

void SetGUIObjectPosition(int guin, int objn, int xx, int yy) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIObjectPosition: invalid GUI number");
  if ((objn<0) || (objn>=guis[guin].numobjs))
    quit("!SetGUIObjectPosition: invalid object number");

  GUIControl_SetPosition(guis[guin].objs[objn], xx, yy);
}

void SetGUIPosition(int ifn,int xx,int yy) {
  if ((ifn<0) || (ifn>=game.numgui))
    quit("!SetGUIPosition: invalid GUI number");
  
  GUI_SetPosition(&scrGui[ifn], xx, yy);
}

void SetGUIObjectSize(int ifn, int objn, int newwid, int newhit) {
  if ((ifn<0) || (ifn>=game.numgui))
    quit("!SetGUIObjectSize: invalid GUI number");

  if ((objn<0) || (objn >= guis[ifn].numobjs))
    quit("!SetGUIObjectSize: invalid object number");

  GUIControl_SetSize(guis[ifn].objs[objn], newwid, newhit);
}

void SetGUISize (int ifn, int widd, int hitt) {
  if ((ifn<0) || (ifn>=game.numgui))
    quit("!SetGUISize: invalid GUI number");

  GUI_SetSize(&scrGui[ifn], widd, hitt);
}

void SetGUIZOrder(int guin, int z) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIZOrder: invalid GUI number");

  GUI_SetZOrder(&scrGui[guin], z);
}

void SetGUIClickable(int guin, int clickable) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIClickable: invalid GUI number");

  GUI_SetClickable(&scrGui[guin], clickable);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetGUITransparency(int ifn, int trans) {
  if ((ifn < 0) | (ifn >= game.numgui))
    quit("!SetGUITransparency: invalid GUI number");

  GUI_SetTransparency(&scrGui[ifn], trans);
}

void CentreGUI (int ifn) {
  if ((ifn<0) | (ifn>=game.numgui))
    quit("!CentreGUI: invalid GUI number");

  GUI_Centre(&scrGui[ifn]);
}

int GetTextWidth(char *text, int fontnum) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetTextWidth: invalid font number.");

  return divide_down_coordinate(wgettextwidth_compensate(text, fontnum));
}

int GetTextHeight(char *text, int fontnum, int width) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetTextHeight: invalid font number.");

  int texthit = wgetfontheight(fontnum);

  break_up_text_into_lines(multiply_up_coordinate(width), fontnum, text);

  return divide_down_coordinate(texthit * numlines);
}

void SetGUIBackgroundPic (int guin, int slotn) {
  if ((guin<0) | (guin>=game.numgui))
    quit("!SetGUIBackgroundPic: invalid GUI number");

  GUI_SetBackgroundGraphic(&scrGui[guin], slotn);
}

void DisableInterface() {
  play.disabled_user_interface++;
  guis_need_update = 1;
  set_mouse_cursor(CURS_WAIT);
  }

void EnableInterface() {
  guis_need_update = 1;
  play.disabled_user_interface--;
  if (play.disabled_user_interface<1) {
    play.disabled_user_interface=0;
    set_default_cursor();
    }
  }
// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled() {
  return (play.disabled_user_interface > 0) ? 0 : 1;
}

int GetGUIObjectAt (int xx, int yy) {
    GUIObject *toret = GetGUIControlAtLocation(xx, yy);
    if (toret == NULL)
        return -1;

    return toret->objn;
}

int GetGUIAt (int xx,int yy) {
    multiply_up_coordinates(&xx, &yy);

    int aa, ll;
    for (ll = game.numgui - 1; ll >= 0; ll--) {
        aa = play.gui_draw_order[ll];
        if (guis[aa].on<1) continue;
        if (guis[aa].flags & GUIF_NOCLICK) continue;
        if ((xx>=guis[aa].x) & (yy>=guis[aa].y) &
            (xx<=guis[aa].x+guis[aa].wid) & (yy<=guis[aa].y+guis[aa].hit))
            return aa;
    }
    return -1;
}

void SetTextWindowGUI (int guinum) {
    if ((guinum < -1) | (guinum >= game.numgui))
        quit("!SetTextWindowGUI: invalid GUI number");

    if (guinum < 0) ;  // disable it
    else if (!guis[guinum].is_textwindow())
        quit("!SetTextWindowGUI: specified GUI is not a text window");

    if (play.speech_textwindow_gui == game.options[OPT_TWCUSTOM])
        play.speech_textwindow_gui = guinum;
    game.options[OPT_TWCUSTOM] = guinum;
}

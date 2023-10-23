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
#include <vector>
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/gui.h"
#include "ac/guicontrol.h"
#include "ac/mouse.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "script/runtimescriptvalue.h"
#include "util/string_compat.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern std::vector<ScriptGUI> scrGui;

// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int FindGUIID (const char* GUIName) {
    for (int ii = 0; ii < game.numgui; ii++) {
        if (guis[ii].Name.IsEmpty())
            continue;
        if (guis[ii].Name == GUIName)
            return ii;
        if ((guis[ii].Name[0u] == 'g') && (ags_stricmp(guis[ii].Name.GetCStr() + 1, GUIName) == 0))
            return ii;
    }
    quit("FindGUIID: No matching GUI found: GUI may have been deleted");
    return -1;
}

void InterfaceOn(int ifn) {
  if ((ifn<0) | (ifn>=game.numgui))
    quit("!GUIOn: invalid GUI specified");

  EndSkippingUntilCharStops();

  if (guis[ifn].IsVisible()) {
    return;
  }
  guis[ifn].SetVisible(true);
  debug_script_log("GUI %d turned on", ifn);
  // modal interface
  if (guis[ifn].PopupStyle==kGUIPopupModal) PauseGame();
  guis[ifn].Poll(mousex, mousey);
}

void InterfaceOff(int ifn) {
  if ((ifn<0) | (ifn>=game.numgui)) quit("!GUIOff: invalid GUI specified");
  if (!guis[ifn].IsVisible()) {
    return;
  }
  debug_script_log("GUI %d turned off", ifn);
  guis[ifn].SetVisible(false);
  // modal interface
  if (guis[ifn].PopupStyle==kGUIPopupModal) UnPauseGame();
}

int GetTextWidth(const char *text, int fontnum) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetTextWidth: invalid font number.");

  return get_text_width_outlined(text, fontnum);
}

int GetTextHeight(const char *text, int fontnum, int width) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetTextHeight: invalid font number.");

  if (break_up_text_into_lines(text, Lines, width, fontnum) == 0)
    return 0;
  return get_text_lines_height(fontnum, Lines.Count());
}

int GetFontHeight(int fontnum)
{
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetFontHeight: invalid font number.");
  return get_font_height_outlined(fontnum);
}

int GetFontLineSpacing(int fontnum)
{
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetFontLineSpacing: invalid font number.");
  return get_font_linespacing(fontnum);
}

void DisableInterface() {
  // If GUI looks change when disabled, then mark all of them for redraw
  bool redraw_gui = (play.disabled_user_interface == 0) && // only if was enabled before
      (GUI::Options.DisabledStyle != kGuiDis_Unchanged);
  GUI::MarkAllGUIForUpdate(redraw_gui, true);
  play.disabled_user_interface++;
  set_mouse_cursor(CURS_WAIT);
}

void EnableInterface() {
  play.disabled_user_interface--;
  if (play.disabled_user_interface<1) {
    play.disabled_user_interface=0;
    set_default_cursor();
    // If GUI looks change when disabled, then mark all of them for redraw
    GUI::MarkAllGUIForUpdate(GUI::Options.DisabledStyle != kGuiDis_Unchanged, true);
  }
}
// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled() {
  return (play.disabled_user_interface > 0) ? 0 : 1;
}

int GetGUIAt (int xx,int yy) {
    // Test in the opposite order (from closer to further)
    for (auto g = play.gui_draw_order.crbegin(); g < play.gui_draw_order.crend(); ++g) {
        if (guis[*g].IsInteractableAt(xx, yy))
            return *g;
    }
    return -1;
}

void SetTextWindowGUI (int guinum) {
    if ((guinum < -1) | (guinum >= game.numgui))
        quit("!SetTextWindowGUI: invalid GUI number");

    if (guinum < 0) ;  // disable it
    else if (!guis[guinum].IsTextWindow())
        quit("!SetTextWindowGUI: specified GUI is not a text window");

    if (play.speech_textwindow_gui == game.options[OPT_TWCUSTOM])
        play.speech_textwindow_gui = guinum;
    game.options[OPT_TWCUSTOM] = guinum;
}

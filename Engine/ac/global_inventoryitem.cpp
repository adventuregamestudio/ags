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
#include <stdio.h>
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_translation.h"
#include "ac/inventoryitem.h"
#include "ac/invwindow.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"
#include "ac/event.h"
#include "ac/gamestate.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern GameState play;
extern int mousex, mousey;
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern CharacterInfo*playerchar;


void set_inv_item_pic(int invi, int piccy) {
    if ((invi < 1) || (invi > game.numinvitems))
        quit("!SetInvItemPic: invalid inventory item specified");

    if (game.invinfo[invi].pic == piccy)
        return;

    if (game.invinfo[invi].pic == game.invinfo[invi].cursorPic)
    {
        // Backwards compatibility -- there didn't used to be a cursorPic,
        // so if they're the same update both.
        set_inv_item_cursorpic(invi, piccy);
    }

    game.invinfo[invi].pic = piccy;
    GUI::MarkInventoryForUpdate(-1, false);
}

void SetInvItemName(int invi, const char *newName) {
    if ((invi < 1) || (invi > game.numinvitems))
        quit("!SetInvName: invalid inventory item specified");

    // set the new name, making sure it doesn't overflow the buffer
    strncpy(game.invinfo[invi].name, newName, 25);
    game.invinfo[invi].name[24] = 0;

    // might need to redraw the GUI if it has the inv item name on it
    GUI::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
}

int GetInvAt(int atx, int aty) {
  int ongui = GetGUIAt(atx, aty);
  if (ongui >= 0) {
    data_to_game_coords(&atx, &aty);
    int onobj = guis[ongui].FindControlAt(atx, aty);
    GUIObject *guio = guis[ongui].GetControl(onobj);
    if (guio) {
      mouse_ifacebut_xoffs = atx - guis[ongui].X - guio->X;
      mouse_ifacebut_yoffs = aty - guis[ongui].Y - guio->Y;
    }
    if (guio && (guis[ongui].GetControlType(onobj) == kGUIInvWindow))
      return offset_over_inv((GUIInvWindow*)guio);
  }
  return -1;
}

void GetInvName(int indx,char*buff) {
  VALIDATE_STRING(buff);
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvName: invalid inventory item specified");
  snprintf(buff, MAX_MAXSTRLEN, "%s", get_translation(game.invinfo[indx].name));
}

int GetInvGraphic(int indx) {
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvGraphic: invalid inventory item specified");

  return game.invinfo[indx].pic;
}

void RunInventoryInteraction (int iit, int modd) {
    if ((iit < 0) || (iit >= game.numinvitems))
        quit("!RunInventoryInteraction: invalid inventory number");

    if (modd == MODE_LOOK)
        run_event_block_inv(iit, 0);
    else if (modd == MODE_HAND)
        run_event_block_inv(iit, 1);
    else if (modd == MODE_USE) {
        play.usedinv = playerchar->activeinv;
        run_event_block_inv(iit, 3);
    }
    else if (modd == MODE_TALK)
        run_event_block_inv(iit, 2);
    else // other click on inventory
        run_event_block_inv(iit, 4);
}

int IsInventoryInteractionAvailable (int item, int mood) {
  if ((item < 0) || (item >= MAX_INV))
    quit("!IsInventoryInteractionAvailable: invalid inventory number");

  play.check_interaction_only = 1;

  RunInventoryInteraction(item, mood);

  int ciwas = play.check_interaction_only;
  play.check_interaction_only = 0;

  if (ciwas == 2)
    return 1;

  return 0;
}

int GetInvProperty (int item, const char *property) {
    return get_int_property (game.invProps[item], play.invProps[item], property);
}

void GetInvPropertyText (int item, const char *property, char *bufer) {
    get_text_property (game.invProps[item], play.invProps[item], property, bufer);
}

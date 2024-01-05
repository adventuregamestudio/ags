//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdio.h>
#include "ac/common.h"
#include "ac/event.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_translation.h"
#include "ac/inventoryitem.h"
#include "ac/invwindow.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "ac/dynobj/cc_inventory.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"
#include "script/script.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern GameState play;
extern int mousex, mousey;
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern CharacterInfo*playerchar;
extern ScriptInvItem scrInv[MAX_INV];
extern CCInventory ccDynamicInv;


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

    game.invinfo[invi].name = newName;
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
  snprintf(buff, MAX_MAXSTRLEN, "%s", get_translation(game.invinfo[indx].name.GetCStr()));
}

int GetInvGraphic(int indx) {
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvGraphic: invalid inventory item specified");

  return game.invinfo[indx].pic;
}

void RunInventoryInteraction (int iit, int mood) {
    if ((iit < 0) || (iit >= game.numinvitems))
        quit("!RunInventoryInteraction: invalid inventory number");

    // convert cursor mode to event index (in inventoryitem event table)
    // TODO: probably move this conversion table elsewhere? should be a global info
    int evnt;
    switch (mood)
    {
    case MODE_LOOK: evnt = 0; break;
    case MODE_HAND: evnt = 1; break;
    case MODE_TALK: evnt = 2; break;
    case MODE_USE: evnt = 3; break;
    default: evnt = -1; break;
    }
    const int otherclick_evt = 4; // TODO: make global constant (inventory other-click evt)

    // For USE verb: remember active inventory
    if (mood == MODE_USE)
    {
        play.usedinv = playerchar->activeinv;
    }

    if (evnt < 0) // on any non-supported mode - use "other-click"
        evnt = otherclick_evt;

    const auto obj_evt = ObjectEvent("inventory%d", iit,
        RuntimeScriptValue().SetScriptObject(&scrInv[iit], &ccDynamicInv), mood);
    if (loaded_game_file_version > kGameVersion_272)
    {
        run_interaction_script(obj_evt, game.invScripts[iit].get(), evnt);
    }
    else 
    {
        run_interaction_event(obj_evt, game.intrInv[iit].get(), evnt);
    }
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

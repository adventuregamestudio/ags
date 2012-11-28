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

#include "util/wgt2allg.h"
#include "ac/listbox.h"
#include "ac/common.h"
#include "ac/file.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/string.h"
#include "gui/guimain.h"

extern int guis_need_update;
extern char saveGameDirectory[260];
extern GameState play;
extern GUIMain*guis;
extern GameSetupStruct game;

// *** LIST BOX FUNCTIONS

int ListBox_AddItem(GUIListBox *lbb, const char *text) {
  if (lbb->AddItem(text) < 0)
    return 0;

  guis_need_update = 1;
  return 1;
}

int ListBox_InsertItemAt(GUIListBox *lbb, int index, const char *text) {
  if (lbb->InsertItem(index, text) < 0)
    return 0;

  guis_need_update = 1;
  return 1;
}

void ListBox_Clear(GUIListBox *listbox) {
  listbox->Clear();
  guis_need_update = 1;
}

void ListBox_FillDirList(GUIListBox *listbox, const char *filemask) {
  char searchPath[MAX_PATH];
  validate_user_file_path(filemask, searchPath, false);

  listbox->Clear();
  al_ffblk dfb;
  int	dun = al_findfirst(searchPath, &dfb, FA_SEARCH);
  while (!dun) {
    listbox->AddItem(dfb.name);
    dun = al_findnext(&dfb);
  }
  al_findclose(&dfb);
  guis_need_update = 1;
}

int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBox.SaveGameSlot: index out of range");

  return listbox->saveGameIndex[index];
}

int ListBox_FillSaveGameList(GUIListBox *listbox) {
  listbox->Clear();

  int numsaves=0;
  int bufix=0;
  al_ffblk ffb; 
  long filedates[MAXSAVEGAMES];
  char buff[200];

  char searchPath[260];
  sprintf(searchPath, "%s""agssave.*", saveGameDirectory);

  int don = al_findfirst(searchPath, &ffb, FA_SEARCH);
  while (!don) {
    bufix=0;
    if (numsaves >= MAXSAVEGAMES)
      break;
    // only list games .000 to .099 (to allow higher slots for other perposes)
    if (strstr(ffb.name,".0")==NULL) {
      don = al_findnext(&ffb);
      continue;
    }
    const char *numberExtension = strstr(ffb.name, ".0") + 1;
    int saveGameSlot = atoi(numberExtension);
    GetSaveSlotDescription(saveGameSlot, buff);
    listbox->AddItem(buff);
    listbox->saveGameIndex[numsaves] = saveGameSlot;
    filedates[numsaves]=(long int)ffb.time;
    numsaves++;
    don = al_findnext(&ffb);
  }
  al_findclose(&ffb);

  int nn;
  for (nn=0;nn<numsaves-1;nn++) {
    for (int kk=0;kk<numsaves-1;kk++) {  // Date order the games

      if (filedates[kk] < filedates[kk+1]) {   // swap them round
        char*tempptr = listbox->items[kk];
        listbox->items[kk] = listbox->items[kk+1];
        listbox->items[kk+1] = tempptr;
        int numtem = listbox->saveGameIndex[kk];
        listbox->saveGameIndex[kk] = listbox->saveGameIndex[kk+1];
        listbox->saveGameIndex[kk+1] = numtem;
        long numted=filedates[kk]; filedates[kk]=filedates[kk+1];
        filedates[kk+1]=numted;
      }
    }
  }

  // update the global savegameindex[] array for backward compatibilty
  for (nn = 0; nn < numsaves; nn++) {
    play.filenumbers[nn] = listbox->saveGameIndex[nn];
  }

  guis_need_update = 1;
  listbox->exflags |= GLF_SGINDEXVALID;

  if (numsaves >= MAXSAVEGAMES)
    return 1;
  return 0;
}

int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y) {

  if (guis[listbox->guin].on < 1)
    return -1;

  multiply_up_coordinates(&x, &y);
  x = (x - listbox->x) - guis[listbox->guin].x;
  y = (y - listbox->y) - guis[listbox->guin].y;

  if ((x < 0) || (y < 0) || (x >= listbox->wid) || (y >= listbox->hit))
    return -1;
  
  return listbox->GetIndexFromCoordinates(x, y);
}

char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBoxGetItemText: invalid item specified");
  strncpy(buffer, listbox->items[index],198);
  buffer[199] = 0;
  return buffer;
}

const char* ListBox_GetItems(GUIListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBox.Items: invalid index specified");

  return CreateNewScriptStringAsRetVal(listbox->items[index]);
}

void ListBox_SetItemText(GUIListBox *listbox, int index, const char *newtext) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBoxSetItemText: invalid item specified");

  if (strcmp(listbox->items[index], newtext)) {
    listbox->SetItemText(index, newtext);
    guis_need_update = 1;
  }
}

void ListBox_RemoveItem(GUIListBox *listbox, int itemIndex) {
  
  if ((itemIndex < 0) || (itemIndex >= listbox->numItems))
    quit("!ListBoxRemove: invalid listindex specified");

  listbox->RemoveItem(itemIndex);
  guis_need_update = 1;
}

int ListBox_GetItemCount(GUIListBox *listbox) {
  return listbox->numItems;
}

int ListBox_GetFont(GUIListBox *listbox) {
  return listbox->font;
}

void ListBox_SetFont(GUIListBox *listbox, int newfont) {

  if ((newfont < 0) || (newfont >= game.numfonts))
    quit("!ListBox.Font: invalid font number.");

  if (newfont != listbox->font) {
    listbox->ChangeFont(newfont);
    guis_need_update = 1;
  }

}

int ListBox_GetHideBorder(GUIListBox *listbox) {
  return (listbox->exflags & GLF_NOBORDER) ? 1 : 0;
}

void ListBox_SetHideBorder(GUIListBox *listbox, int newValue) {
  listbox->exflags &= ~GLF_NOBORDER;
  if (newValue)
    listbox->exflags |= GLF_NOBORDER;
  guis_need_update = 1;
}

int ListBox_GetHideScrollArrows(GUIListBox *listbox) {
  return (listbox->exflags & GLF_NOARROWS) ? 1 : 0;
}

void ListBox_SetHideScrollArrows(GUIListBox *listbox, int newValue) {
  listbox->exflags &= ~GLF_NOARROWS;
  if (newValue)
    listbox->exflags |= GLF_NOARROWS;
  guis_need_update = 1;
}

int ListBox_GetSelectedIndex(GUIListBox *listbox) {
  if ((listbox->selected < 0) || (listbox->selected >= listbox->numItems))
    return -1;
  return listbox->selected;
}

void ListBox_SetSelectedIndex(GUIListBox *guisl, int newsel) {

  if (newsel >= guisl->numItems)
    newsel = -1;

  if (guisl->selected != newsel) {
    guisl->selected = newsel;
    if (newsel >= 0) {
      if (newsel < guisl->topItem)
        guisl->topItem = newsel;
      if (newsel >= guisl->topItem + guisl->num_items_fit)
        guisl->topItem = (newsel - guisl->num_items_fit) + 1;
    }
    guis_need_update = 1;
  }

}

int ListBox_GetTopItem(GUIListBox *listbox) {
  return listbox->topItem;
}

void ListBox_SetTopItem(GUIListBox *guisl, int item) {
  if ((guisl->numItems == 0) && (item == 0))
    ;  // allow resetting an empty box to the top
  else if ((item >= guisl->numItems) || (item < 0))
    quit("!ListBoxSetTopItem: tried to set top to beyond top or bottom of list");

  guisl->topItem = item;
  guis_need_update = 1;
}

int ListBox_GetRowCount(GUIListBox *listbox) {
  return listbox->num_items_fit;
}

void ListBox_ScrollDown(GUIListBox *listbox) {
  if (listbox->topItem + listbox->num_items_fit < listbox->numItems) {
    listbox->topItem++;
    guis_need_update = 1;
  }
}

void ListBox_ScrollUp(GUIListBox *listbox) {
  if (listbox->topItem > 0) {
    listbox->topItem--;
    guis_need_update = 1;
  }
}


GUIListBox* is_valid_listbox (int guin, int objn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!ListBox: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!ListBox: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LISTBOX)
    quit("!ListBox: specified control is not a list box");
  guis_need_update = 1;
  return (GUIListBox*)guis[guin].objs[objn];
}

/*
** Adventure Game Studio GUI routines
** Copyright (C) 2000-2005, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __AC_GUILISTBOX_H
#define __AC_GUILISTBOX_H

#include "gui/guiobject.h"
#include "gui/dynamicarray.h"

#define GLF_NOBORDER     1
#define GLF_NOARROWS     2
#define GLF_SGINDEXVALID 4

struct GUIListBox:public GUIObject
{
  char *items[MAX_LISTBOX_ITEMS];
  short saveGameIndex[MAX_LISTBOX_ITEMS];
  int numItems, selected, topItem, mousexp, mouseyp;
  int rowheight, num_items_fit;
  int font, textcol, backcol, exflags;
  int selectedbgcol;
  int alignment, reserved1;
  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  int  AddItem(const char *toadd);
  int  InsertItem(int index, const char *toadd);
  void SetItemText(int index, const char *newtext);
  void RemoveItem(int index);
  void Clear();
  void Draw();
  int  IsInRightMargin(int x);
  int  GetIndexFromCoordinates(int x, int y);
  void ChangeFont(int newFont);
  virtual int MouseDown();
  
  void MouseMove(int nx, int ny)
  {
    mousexp = nx - x;
    mouseyp = ny - y;
  }

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  void MouseUp()
  {
  }

  void KeyPress(int kp)
  {
  }

  virtual void Resized();

  void reset()
  {
    GUIObject::init();
    mousexp = 0;
    mouseyp = 0;
    activated = 0;
    numItems = 0;
    topItem = 0;
    selected = 0;
    font = 0;
    textcol = 0;
    selectedbgcol = 16;
    backcol = 7;
    exflags = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "SelectionChanged";
    supportedEventArgs[0] = "GUIControl *control";
  }

  GUIListBox() {
    reset();
  }

private:
  int numItemsTemp;

  void Draw_items_fix();
  void Draw_items_unfix();
};

extern DynamicArray<GUIListBox> guilist;
extern int numguilist;

#endif // __AC_GUILISTBOX_H

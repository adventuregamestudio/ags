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
#include <stdlib.h>
#include <string.h>
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "font/fonts.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "util/wgt2allg.h"

using AGS::Common::Stream;
using AGS::Common::Bitmap;

std::vector<GUIListBox> guilist;
int numguilist = 0;

void GUIListBox::ChangeFont(int newfont) {
  font = newfont;
  rowheight = wgettextheight("YpyjIHgMNWQ", font) + get_fixed_pixel_size(2);
  num_items_fit = hit / rowheight;
}

 void GUIListBox::Resized() 
{
	if (rowheight == 0)
	{
	  check_font(&font);
	  ChangeFont(font);
	}

	if (rowheight > 0)
	  num_items_fit = hit / rowheight;
}

void GUIListBox::WriteToFile(Stream *out)
{
  int a;

  GUIObject::WriteToFile(out);
  // MACPORT FIXES: swap
  out->WriteArrayOfInt32(&numItems, 11);
  out->WriteInt32(alignment);
  out->WriteInt32(reserved1);
  out->WriteInt32(selectedbgcol);
  for (a = 0; a < numItems; a++)
    items[a].Write(out);

  if (exflags & GLF_SGINDEXVALID)
    out->WriteArrayOfInt16(&saveGameIndex[0], numItems);
}

void GUIListBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  int a, i;
  char tempbuf[300];

  GUIObject::ReadFromFile(in, gui_version);
  // MACPORT FIXES: swap
  in->ReadArrayOfInt32(&numItems, 11);

  if (gui_version >= kGuiVersion_272b) {
    alignment = in->ReadInt32();
    reserved1 = in->ReadInt32();
  }
  else {
    alignment = GALIGN_LEFT;
    reserved1 = 0;
  }

  if (gui_version >= kGuiVersion_unkn_107) {
    selectedbgcol = in->ReadInt32();
  }
  else {
    selectedbgcol = textcol;
    if (selectedbgcol == 0)
      selectedbgcol = 16;
  }

  for (a = 0; a < numItems; a++) {
    i = 0;
    while ((tempbuf[i] = in->ReadInt8()) != 0)
      i++;

    items[a] = tempbuf;
    saveGameIndex[a] = -1;
  }

  if ((gui_version >= kGuiVersion_272d) && (exflags & GLF_SGINDEXVALID)) {
    in->ReadArrayOfInt16(&saveGameIndex[0], numItems);
  }

  if (textcol == 0)
    textcol = 16;
}

int GUIListBox::AddItem(const char *toadd)
{
  if (numItems >= MAX_LISTBOX_ITEMS)
    return -1;

  guis_need_update = 1;
  items[numItems] = toadd;
  saveGameIndex[numItems] = -1;
  numItems++;
  return numItems - 1;
}

int GUIListBox::InsertItem(int index, const char *toadd)
{
  int aa;

  if (numItems >= MAX_LISTBOX_ITEMS)
    return -1;

  if ((index < 0) || (index > numItems))
    return -1;

  guis_need_update = 1;

  for (aa = numItems; aa > index; aa--) {
    items[aa] = items[aa - 1];
    saveGameIndex[aa] = saveGameIndex[aa - 1];
  }

  items[index] = toadd;
  saveGameIndex[index] = -1;
  numItems++;

  if (selected >= index)
    selected++;

  return numItems - 1;
}

void GUIListBox::SetItemText(int item, const char *newtext)
{
  if ((item >= numItems) || (item < 0))
    return;

  guis_need_update = 1;
  items[item] = newtext;
}

void GUIListBox::Clear()
{
  int aa;
  for (aa = 0; aa < numItems; aa++)
    items[aa].Free();

  numItems = 0;
  selected = 0;
  topItem = 0;
  guis_need_update = 1;
}

void GUIListBox::RemoveItem(int index)
{
  int aa;

  if ((index < 0) || (index >= numItems))
    return;

  numItems--;
  for (aa = index; aa < numItems; aa++) {
    items[aa] = items[aa + 1];
    saveGameIndex[aa] = saveGameIndex[aa + 1];
  }

  if (selected > index)
    selected--;
  if (selected >= numItems)
    selected = -1;

  guis_need_update = 1;
}

void GUIListBox::Draw(Common::Bitmap *ds)
{
  wid--;
  hit--;
  int pixel_size = get_fixed_pixel_size(1);

  check_font(&font);
  color_t text_color = ds->GetCompatibleColor(textcol);
  color_t draw_color = ds->GetCompatibleColor(textcol);
  if ((exflags & GLF_NOBORDER) == 0) {
    ds->DrawRect(Rect(x, y, x + wid + (pixel_size - 1), y + hit + (pixel_size - 1)), draw_color);
    if (pixel_size > 1)
      ds->DrawRect(Rect(x + 1, y + 1, x + wid, y + hit), draw_color);
  }

  int rightHandEdge = (x + wid) - pixel_size - 1;

  // use ChangeFont to update the rowheight and num_items_fit
  ChangeFont(font);

  // draw the scroll bar in if necessary
  if ((numItems > num_items_fit) && ((exflags & GLF_NOBORDER) == 0) && ((exflags & GLF_NOARROWS) == 0)) {
    int xstrt, ystrt;
    ds->DrawRect(Rect(x + wid - get_fixed_pixel_size(7), y, (x + (pixel_size - 1) + wid) - get_fixed_pixel_size(7), y + hit), draw_color);
    ds->DrawRect(Rect(x + wid - get_fixed_pixel_size(7), y + hit / 2, x + wid, y + hit / 2 + (pixel_size - 1)), draw_color);

    xstrt = (x + wid - get_fixed_pixel_size(6)) + (pixel_size - 1);
    ystrt = (y + hit - 3) - get_fixed_pixel_size(5);

    draw_color = ds->GetCompatibleColor(textcol);
    ds->DrawTriangle(Triangle(xstrt, ystrt, xstrt + get_fixed_pixel_size(4), ystrt, 
             xstrt + get_fixed_pixel_size(2),
             ystrt + get_fixed_pixel_size(5)), draw_color);

    ystrt = y + 3;
    ds->DrawTriangle(Triangle(xstrt, ystrt + get_fixed_pixel_size(5), 
             xstrt + get_fixed_pixel_size(4), 
             ystrt + get_fixed_pixel_size(5),
             xstrt + get_fixed_pixel_size(2), ystrt), draw_color);

    rightHandEdge -= get_fixed_pixel_size(7);
  }

  Draw_items_fix();

  for (int a = 0; a < num_items_fit; a++) {
    int thisyp;
    if (a + topItem >= numItems)
      break;

    thisyp = y + pixel_size + a * rowheight;
    if (a + topItem == selected) {
      int stretchto = (x + wid) - pixel_size;

      text_color = ds->GetCompatibleColor(backcol);

      if (selectedbgcol > 0) {
        // draw the selected item bar (if colour not transparent)
        draw_color = ds->GetCompatibleColor(selectedbgcol);
        if ((num_items_fit < numItems) && ((exflags & GLF_NOBORDER) == 0) && ((exflags & GLF_NOARROWS) == 0))
          stretchto -= get_fixed_pixel_size(7);

        ds->FillRect(Rect(x + pixel_size, thisyp, stretchto, thisyp + rowheight - pixel_size), draw_color);
      }
    }
    else
      text_color = ds->GetCompatibleColor(textcol);

    int item_index = a + topItem;
    char oritext[200]; // items[] can be not longer than 200 characters due declaration
    Draw_set_oritext(oritext, items[item_index]);

    if (alignment == GALIGN_LEFT)
      wouttext_outline(ds, x + 1 + pixel_size, thisyp + 1, font, text_color, oritext);
    else {
      int textWidth = wgettextwidth(oritext, font);

      if (alignment == GALIGN_RIGHT)
        wouttext_outline(ds, rightHandEdge - textWidth, thisyp + 1, font, text_color, oritext);
      else
        wouttext_outline(ds, ((rightHandEdge - x) / 2) + x - (textWidth / 2), thisyp + 1, font, text_color, oritext);
    }

  }
  wid++;
  hit++;

  Draw_items_unfix();
}

int GUIListBox::IsInRightMargin(int xx) {

  if ((xx >= (wid - get_fixed_pixel_size(6))) && ((exflags & GLF_NOBORDER) == 0) && ((exflags & GLF_NOARROWS) == 0)) {
    return 1;
  }
  return 0;
}

int GUIListBox::GetIndexFromCoordinates(int xx, int yy) {
  if (rowheight <= 0 || IsInRightMargin(xx))
    return -1;

  int onindex = yy / rowheight + topItem;
  if ((onindex < 0) || (onindex >= numItems))
    return -1;

  return onindex;
}

int GUIListBox::MouseDown()
{
  if (IsInRightMargin(mousexp)) {
    if ((mouseyp < hit / 2) && (topItem > 0))
      topItem--;

    if ((mouseyp >= hit / 2) && (numItems > topItem + num_items_fit))
      topItem++;

    return 0;
  }

  int newsel = GetIndexFromCoordinates(mousexp, mouseyp);
  if (newsel < 0)
    return 0;

  selected = newsel;
  activated = 1;
  return 0;
}

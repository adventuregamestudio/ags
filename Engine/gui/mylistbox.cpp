//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gui/mylistbox.h"
#include <string.h>
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/keycode.h"
#include "font/fonts.h"
#include "gfx/bitmap.h"
#include "gui/guidialog.h"
#include "gui/guidialogdefines.h"
#include "gui/guimain.h"

using namespace AGS::Common;

extern int windowbackgroundcolor;
extern int cbuttfont;
extern int smcode;

  MyListBox::MyListBox(int xx, int yy, int wii, int hii, int textheight_)
  {
    x = xx;
    y = yy;
    wid = wii;
    hit = hii;
    textheight = textheight_;
    hit -= (hit - 4) % textheight; // resize to multiple of text height
    numonscreen = (hit - 4) / textheight;
    items = 0;
    topitem = 0;
    selected = -1;
    memset(itemnames, 0, sizeof(itemnames));
  }

  void MyListBox::clearlist()
  {
    for (int kk = 0; kk < items; kk++)
      free(itemnames[kk]);

    items = 0;
  }

  MyListBox::~MyListBox() {
    clearlist();
  }

  void MyListBox::draw(Bitmap *ds)
  {
    color_t draw_color = GUI::GetStandardColorForBitmap(windowbackgroundcolor);
    ds->FillRect(Rect(x, y, x + wid, y + hit), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);
    ds->DrawRect(Rect(x, y, x + wid, y + hit), draw_color);
  
    int widwas = wid;
    wid -= ARROWWIDTH;
    ds->DrawLine(Line(x + wid, y, x + wid, y + hit), draw_color);        // draw the up/down arrows
    ds->DrawLine(Line(x + wid, y + hit / 2, x + widwas, y + hit / 2), draw_color);

    int xmidd = x + wid + (widwas - wid) / 2;
    if (topitem < 1)
      draw_color = GUI::GetStandardColorForBitmap(7);

    ds->DrawLine(Line(xmidd, y + 2, xmidd, y + 10), draw_color); // up arrow
    ds->DrawLine(Line(xmidd - 1, y + 3, xmidd + 1, y + 3), draw_color);
    ds->DrawLine(Line(xmidd - 2, y + 4, xmidd + 2, y + 4), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);
    if (topitem + numonscreen >= items)
      draw_color = GUI::GetStandardColorForBitmap(7);

    ds->DrawLine(Line(xmidd, y + hit - 10, xmidd, y + hit - 3), draw_color);     // down arrow
    ds->DrawLine(Line(xmidd - 1, y + hit - 4, xmidd + 1, y + hit - 4), draw_color);
    ds->DrawLine(Line(xmidd - 2, y + hit - 5, xmidd + 2, y + hit - 5), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);

    for (int tt = 0; tt < numonscreen; tt++) {
      int inum = tt + topitem;
      if (inum >= items)
        break;

      int thisypos = y + 2 + tt * textheight;
      color_t text_color;
      if (inum == selected) {
        draw_color = GUI::GetStandardColorForBitmap(0);
        ds->FillRect(Rect(x, thisypos, x + wid, thisypos + textheight - 1), draw_color);
        text_color = GUI::GetStandardColorForBitmap(7);
      }
      else text_color = GUI::GetStandardColorForBitmap(0);

      wouttextxy(ds, x + 2, thisypos, cbuttfont, text_color, itemnames[inum]);
    }
    wid = widwas;
  }

  int MyListBox::pressedon(int mx, int my)
  {
    if (mx > x + wid - ARROWWIDTH) {
      if ((my - y < hit / 2) & (topitem > 0))
        topitem--;
      else if ((my - y > hit / 2) & (topitem + numonscreen < items))
        topitem++;

    } else {
      selected = ((my - y) - 2) / textheight + topitem;
      if (selected >= items)
        selected = items - 1;

    }

    draw(get_gui_screen());
    smcode = CM_SELCHANGE;
    return 0;
  }

  void MyListBox::additem(char *texx)
  {
    if (items >= MAXLISTITEM)
      quit("!CSCIUSER16: Too many items added to listbox");
    itemnames[items] = (char *)malloc(strlen(texx) + 1);
    strcpy(itemnames[items], texx);
    items++;
    needredraw = 1;
  }

  int MyListBox::processmessage(int mcode, int wParam, intptr_t ipParam)
  {
    if (mcode == CLB_ADDITEM) {
      additem((char *)ipParam);
    } else if (mcode == CLB_CLEAR)
      clearlist();
    else if (mcode == CLB_GETCURSEL)
      return selected;
    else if (mcode == CLB_SETCURSEL)
    {
      selected = wParam;

      if ((selected < topitem) && (selected >= 0))
        topitem = selected;

      if (topitem + numonscreen <= selected)
        topitem = (selected + 1) - numonscreen;
    }
    else if (mcode == CLB_GETTEXT)
      strcpy((char *)ipParam, itemnames[wParam]);
    else if (mcode == CLB_SETTEXT) {
      if (wParam < items)
        free(itemnames[wParam]);

      char *newstri = (char *)ipParam;
      itemnames[wParam] = (char *)malloc(strlen(newstri) + 2);
      strcpy(itemnames[wParam], newstri);

    } else if (mcode == CTB_KEYPRESS) {
      if ((wParam == eAGSKeyCodeDownArrow) && (selected < items - 1))
        selected++;

      if ((wParam == eAGSKeyCodeUpArrow) && (selected > 0))
        selected--;

      if (wParam == eAGSKeyCodePageUp)
        selected -= (numonscreen - 1);

      if (wParam == eAGSKeyCodePageDown)
        selected += (numonscreen - 1);

      if ((selected < 0) && (items > 0))
        selected = 0;

      if (selected >= items)
        selected = items - 1;

      if ((selected < topitem) & (selected >= 0))
        topitem = selected;

      if (topitem + numonscreen <= selected)
        topitem = (selected + 1) - numonscreen;

      drawandmouse();
      smcode = CM_SELCHANGE;
    } else
      return -1;

    return 0;
  }

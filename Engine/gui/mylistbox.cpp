
#include <string.h>
#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "gui/mylistbox.h"
#include "gui/guidialoginternaldefs.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;

extern GameSetup usetup;
extern int mousex, mousey, numcurso, hotx, hoty;

extern int windowbackgroundcolor;
extern int cbuttfont;
extern int smcode;

  MyListBox::MyListBox(int xx, int yy, int wii, int hii)
  {
    x = xx;
    y = yy;
    wid = wii;
    hit = hii;
    hit -= (hit - 4) % TEXT_HT; // resize to multiple of text height
    numonscreen = (hit - 4) / TEXT_HT;
    items = 0;
    topitem = 0;
    selected = -1;
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

  void MyListBox::draw()
  {
    wsetcolor(windowbackgroundcolor);
    abuf->FillRect(Rect(x, y, x + wid, y + hit), currentcolor);
    wsetcolor(0);
    abuf->DrawRect(Rect(x, y, x + wid, y + hit), currentcolor);
  
    int widwas = wid;
    wid -= ARROWWIDTH;
    abuf->DrawLine(Line(x + wid, y, x + wid, y + hit), currentcolor);        // draw the up/down arrows
    abuf->DrawLine(Line(x + wid, y + hit / 2, x + widwas, y + hit / 2), currentcolor);

    int xmidd = x + wid + (widwas - wid) / 2;
    if (topitem < 1)
      wsetcolor(7);

    abuf->DrawLine(Line(xmidd, y + 2, xmidd, y + 10), currentcolor); // up arrow
    abuf->DrawLine(Line(xmidd - 1, y + 3, xmidd + 1, y + 3), currentcolor);
    abuf->DrawLine(Line(xmidd - 2, y + 4, xmidd + 2, y + 4), currentcolor);
    wsetcolor(0);
    if (topitem + numonscreen >= items)
      wsetcolor(7);

    abuf->DrawLine(Line(xmidd, y + hit - 10, xmidd, y + hit - 3), currentcolor);     // down arrow
    abuf->DrawLine(Line(xmidd - 1, y + hit - 4, xmidd + 1, y + hit - 4), currentcolor);
    abuf->DrawLine(Line(xmidd - 2, y + hit - 5, xmidd + 2, y + hit - 5), currentcolor);
    wsetcolor(0);

    for (int tt = 0; tt < numonscreen; tt++) {
      int inum = tt + topitem;
      if (inum >= items)
        break;

      int thisypos = y + 2 + tt * TEXT_HT;
      if (inum == selected) {
        wsetcolor(0);
        abuf->FillRect(Rect(x, thisypos, x + wid, thisypos + TEXT_HT - 1), currentcolor);
        wtextcolor(7);
      } else
        wtextcolor(0);

      wouttextxy(x + 2, thisypos, cbuttfont, itemnames[inum]);
    }
    wid = widwas;
  }

  int MyListBox::pressedon()
  {
    if (mousex > x + wid - ARROWWIDTH) {
      if ((mousey - y < hit / 2) & (topitem > 0))
        topitem--;
      else if ((mousey - y > hit / 2) & (topitem + numonscreen < items))
        topitem++;

    } else {
      selected = ((mousey - y) - 2) / TEXT_HT + topitem;
      if (selected >= items)
        selected = items - 1;

    }

//    domouse(2);
    draw();
  //  domouse(1);
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

  int MyListBox::processmessage(int mcode, int wParam, long lParam)
  {
    if (mcode == CLB_ADDITEM) {
      additem((char *)lParam);
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
      strcpy((char *)lParam, itemnames[wParam]);
    else if (mcode == CLB_SETTEXT) {
      if (wParam < items)
        free(itemnames[wParam]);

      char *newstri = (char *)lParam;
      itemnames[wParam] = (char *)malloc(strlen(newstri) + 2);
      strcpy(itemnames[wParam], newstri);

    } else if (mcode == CTB_KEYPRESS) {
      if ((wParam == 380) && (selected < items - 1))
        selected++;

      if ((wParam == 372) && (selected > 0))
        selected--;

      if (wParam == 373)
        selected -= (numonscreen - 1);

      if (wParam == 381)
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

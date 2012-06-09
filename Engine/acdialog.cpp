/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

// DIALOG SYSTEM STUFF below
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#include <string.h>
#include <ctype.h>
#include "ali3d.h"

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <conio.h>

#ifndef WINDOWS_VERSION
#include <dir.h>
#endif

#endif

#if defined(DJGPP) || defined(LINUX_VERSION) || defined(MAC_VERSION)
#define _getcwd getcwd
#endif

#undef kbhit
extern int rec_getch();
extern int rec_kbhit();
#define kbhit rec_kbhit
#define getch() rec_getch()

extern int load_game(int,char*, int*);
extern void break_up_text_into_lines(int wii,int fonnt,char*todis);
extern char lines[][200];
extern int numlines;
extern void wouttext_outline(int xxp,int yyp,int usingfont,char*texx);
extern IGraphicsDriver *gfxDriver;
extern inline int get_fixed_pixel_size(int pixels);

#define MSG_RESTORE      984
#define MSG_CANCEL       985    // "Cancel"
#define MSG_SELECTLOAD   986    // "Select game to restore"
#define MSG_SAVEBUTTON   987    // "Save"
#define MSG_SAVEDIALOG   988    // "Save game name:"
#define MSG_REPLACE      989    // "Replace"
#define MSG_MUSTREPLACE  990    // "The folder is full. you must replace"
#define MSG_REPLACEWITH1 991    // "Replace:"
#define MSG_REPLACEWITH2 992    // "With:"
#define MSG_QUITBUTTON   993    // "Quit"
#define MSG_PLAYBUTTON   994    // "Play"
#define MSG_QUITDIALOG   995    // "Do you want to quit?"

struct GameSetup
{
  int digicard, midicard;
  int mod_player;
  int textheight;
};

extern void quit(char *);
extern int mousex, mousey;
extern void rec_domouse(int);
extern int rec_misbuttondown(int);
extern int rec_mgetbutton();
extern void next_iteration();
extern void render_graphics(IDriverDependantBitmap *extraBitmap, int extraX, int extraY);
extern void update_polled_stuff_if_runtime();
extern char *get_language_text(int);
extern int sgsiglen;
extern void update_polled_stuff_and_crossfade();
extern char *get_global_message(int);
extern int GetBaseWidth();
extern GameSetup usetup;
extern char ignore_bounds;      // Ignore mouse bounding rectangle while in dialog
extern volatile int timerloop;

extern char saveGameSuffix[21];

const int NONE = -1, LEFT = 0, RIGHT = 1;
char buff[200];
int myscrnwid = 320, myscrnhit = 200;
IDriverDependantBitmap *dialogBmp = NULL;
int windowPosX, windowPosY, windowPosWidth, windowPosHeight;
block windowBuffer = NULL;

#define domouse rec_domouse

/*#define COL251 26
#define COL252 28
#define COL253 29
#define COL254 27
#define COL255 24*/
#define COL253 15
#define COL254 7
#define COL255 8

void __my_wbutt(int x1, int y1, int x2, int y2)
{
  wsetcolor(COL254);            //wsetcolor(15);
  wbar(x1, y1, x2, y2);
  wsetcolor(0);
  wrectangle(x1, y1, x2, y2);
}

#define wbutt __my_wbutt

#define _export
#ifdef WINAPI
#undef WINAPI
#endif
#define WINAPI
#define mbutrelease !rec_misbuttondown
#define TEXT_HT usetup.textheight

//  =========  DEFINES  ========
// Control types
#define CNT_PUSHBUTTON 0x001
#define CNT_LISTBOX    0x002
#define CNT_LABEL      0x003
#define CNT_TEXTBOX    0x004
// Control properties
#define CNF_DEFAULT    0x100
#define CNF_CANCEL     0x200

// Dialog messages
#define CM_COMMAND   1
#define CM_KEYPRESS  2
#define CM_SELCHANGE 3
// System messages
#define SM_SAVEGAME  100
#define SM_LOADGAME  101
#define SM_QUIT      102
// System messages (to ADVEN)
#define SM_SETTRANSFERMEM 120
#define SM_GETINIVALUE    121
// System messages (to driver)
#define SM_QUERYQUIT 110
#define SM_KEYPRESS  111
#define SM_TIMER     112
// ListBox messages
#define CLB_ADDITEM   1
#define CLB_CLEAR     2
#define CLB_GETCURSEL 3
#define CLB_GETTEXT   4
#define CLB_SETTEXT   5
#define CLB_SETCURSEL 6
// TextBox messages
#define CTB_GETTEXT   1
#define CTB_SETTEXT   2

void refresh_screen()
{
  blit(abuf, windowBuffer, windowPosX, windowPosY, 0, 0, windowPosWidth, windowPosHeight);
  gfxDriver->UpdateDDBFromBitmap(dialogBmp, windowBuffer, false);

  render_graphics(dialogBmp, windowPosX, windowPosY);

  // Copy it back, because the mouse will have been drawn on top
  blit(windowBuffer, abuf, 0, 0, windowPosX, windowPosY, windowPosWidth, windowPosHeight);
}

//  =========  STRUCTS  ========
struct DisplayProperties
{
  int width;
  int height;
  int colors;
  int textheight;
};

struct CSCIMessage
{
  int code;
  int id;
  int wParam;
};

int windowbackgroundcolor = COL254, pushbuttondarkcolor = COL255;
int pushbuttonlightcolor = COL253;
int topwindowhandle = -1;
int cbuttfont;
int acdialog_font;

#define CTB_KEYPRESS 91

struct OnScreenWindow
{
  block buffer;
  int x, y;
  int oldtop;
};

struct NewControl
{
  int x, y, wid, hit, state, typeandflags, wlevel;
  char visible, enabled;        // not implemented
  char needredraw;
  virtual void draw() = 0;
  virtual int pressedon() = 0;
  virtual int processmessage(int, int, long) = 0;

    NewControl(int xx, int yy, int wi, int hi)
  {
    x = xx;
    y = yy;
    wid = wi;
    hit = hi;
    state = 0;
    visible = 1;
    enabled = 1;
    needredraw = 1;
  };
  NewControl() {
    visible = 1;
    enabled = 1;
  }
  int mouseisinarea()
  {
    if (topwindowhandle != wlevel)
      return 0;

    if ((mousex > x) & (mousex < x + wid) & (mousey > y) & (mousey < y + hit))
      return 1;

    return 0;
  }
  void drawifneeded()
  {
    if (topwindowhandle != wlevel)
      return;
    if (needredraw) {
      needredraw = 0;
      draw();
    }
  }
  void drawandmouse()
  {
//    domouse(2);
    draw();
  //  domouse(1);
  }
};

#ifdef DJGPP
#pragma warn -inl
#endif
struct PushButton:public NewControl
{
  char text[50];
  PushButton(int xx, int yy, int wi, int hi, char *tex)
  {                             //wlevel=2;
    x = xx;
    y = yy;
    wid = wi;
    hit = hi + 1;               //hit=hi;
    state = 0;
    strncpy(text, tex, 50);
    text[49] = 0;
  };

  void draw()
  {
    wtextcolor(0);
    wsetcolor(COL254);
    wbar(x, y, x + wid, y + hit);
    if (state == 0)
      wsetcolor(pushbuttondarkcolor);
    else
      wsetcolor(pushbuttonlightcolor);

    wrectangle(x, y, x + wid, y + hit);
    if (state == 0)
      wsetcolor(pushbuttonlightcolor);
    else
      wsetcolor(pushbuttondarkcolor);

    wline(x, y, x + wid - 1, y);
    wline(x, y, x, y + hit - 1);
    wouttextxy(x + (wid / 2 - wgettextwidth(text, cbuttfont) / 2), y + 2, cbuttfont, text);
    if (typeandflags & CNF_DEFAULT)
      wsetcolor(0);
    else
      wsetcolor(windowbackgroundcolor);

    wrectangle(x - 1, y - 1, x + wid + 1, y + hit + 1);
  }

  int pressedon()
  {
    int wasstat;
    while (mbutrelease(LEFT) == 0) {
      timerloop = 0;
      wasstat = state;
      next_iteration();
      state = mouseisinarea();
      // stop mp3 skipping if button held down
      update_polled_stuff_if_runtime();
      if (wasstat != state) {
//        domouse(2);
        draw();
        //domouse(1);
      }

//      domouse(0);

      refresh_screen();

      while (timerloop == 0) ;
    }
    wasstat = state;
    state = 0;
//    domouse(2);
    draw();
  //  domouse(1);
    return wasstat;
  }

  int processmessage(int mcode, int wParam, long lParam)
  {
    return -1;                  // doesn't support messages
  }
};

#define MAXLISTITEM 300
int smcode = 0;
struct MyListBox:public NewControl
{
  int items, topitem, numonscreen, selected;
  char *itemnames[MAXLISTITEM];
  MyListBox(int xx, int yy, int wii, int hii)
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

  void clearlist()
  {
    for (int kk = 0; kk < items; kk++)
      free(itemnames[kk]);

    items = 0;
  }

  ~MyListBox() {
    clearlist();
  }

#define ARROWWIDTH 8
  void draw()
  {
    wsetcolor(windowbackgroundcolor);
    wbar(x, y, x + wid, y + hit);
    wsetcolor(0);
    wrectangle(x, y, x + wid, y + hit);
  
    int widwas = wid;
    wid -= ARROWWIDTH;
    wline(x + wid, y, x + wid, y + hit);        // draw the up/down arrows
    wline(x + wid, y + hit / 2, x + widwas, y + hit / 2);

    int xmidd = x + wid + (widwas - wid) / 2;
    if (topitem < 1)
      wsetcolor(7);

    wline(xmidd, y + 2, xmidd, y + 10); // up arrow
    wline(xmidd - 1, y + 3, xmidd + 1, y + 3);
    wline(xmidd - 2, y + 4, xmidd + 2, y + 4);
    wsetcolor(0);
    if (topitem + numonscreen >= items)
      wsetcolor(7);

    wline(xmidd, y + hit - 10, xmidd, y + hit - 3);     // down arrow
    wline(xmidd - 1, y + hit - 4, xmidd + 1, y + hit - 4);
    wline(xmidd - 2, y + hit - 5, xmidd + 2, y + hit - 5);
    wsetcolor(0);

    for (int tt = 0; tt < numonscreen; tt++) {
      int inum = tt + topitem;
      if (inum >= items)
        break;

      int thisypos = y + 2 + tt * TEXT_HT;
      if (inum == selected) {
        wsetcolor(0);
        wbar(x, thisypos, x + wid, thisypos + TEXT_HT - 1);
        wtextcolor(7);
      } else
        wtextcolor(0);

      wouttextxy(x + 2, thisypos, cbuttfont, itemnames[inum]);
    }
    wid = widwas;
  }

  int pressedon()
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

  void additem(char *texx)
  {
    if (items >= MAXLISTITEM)
      quit("!CSCIUSER16: Too many items added to listbox");
    itemnames[items] = (char *)malloc(strlen(texx) + 1);
    strcpy(itemnames[items], texx);
    items++;
    needredraw = 1;
  }

  int processmessage(int mcode, int wParam, long lParam)
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
};

struct MyLabel:public NewControl
{
  char text[150];
  MyLabel(int xx, int yy, int wii, char *tee)
  {
    strncpy(text, tee, 150);
    text[149] = 0;
    x = xx;
    y = yy;
    wid = wii;
    hit = TEXT_HT;
  }

  void draw()
  {
    int curofs = 0, lastspac = 0, cyp = y;
    char *teptr = &text[0];
    wtextcolor(0);

    break_up_text_into_lines(wid, acdialog_font, teptr);
    for (int ee = 0; ee < numlines; ee++) {
      wouttext_outline(x, cyp, acdialog_font, lines[ee]);
      cyp += TEXT_HT;
    }
/*
    while (1) {
      if ((teptr[curofs] == ' ') | (teptr[curofs] == 0)) {
        int itwas = teptr[curofs];
        teptr[curofs] = 0;
        if (wgettextwidth(teptr, cbuttfont) > wid) {
          teptr[curofs] = itwas;
          teptr[lastspac] = 0;
          wouttextxy(x, cyp, cbuttfont, teptr);
          teptr[lastspac] = ' ';
          teptr += lastspac + 1;
          curofs = 0;
          cyp += TEXT_HT;
        } else
          teptr[curofs] = itwas;

        lastspac = curofs;
      }

      if (teptr[curofs] == 0)
        break;

      curofs++;
    }
    wouttextxy(x, cyp, cbuttfont, teptr);*/
  }

  int pressedon()
  {
    return 0;
  }

  int processmessage(int mcode, int wParam, long lParam)
  {
    return -1;                  // doesn't support messages
  }
};

#define TEXTBOX_MAXLEN 49
struct MyTextBox:public NewControl
{
  char text[TEXTBOX_MAXLEN + 1];
  MyTextBox(int xx, int yy, int wii, char *tee)
  {
    x = xx;
    y = yy;
    wid = wii;
    if (tee != NULL)
      strcpy(text, tee);
    else
      text[0] = 0;

    hit = TEXT_HT + 1;
  }

  void draw()
  {
    wsetcolor(windowbackgroundcolor);
    wbar(x, y, x + wid, y + hit);
    wsetcolor(0);
    wrectangle(x, y, x + wid, y + hit);
    wtextcolor(0);
    wouttextxy(x + 2, y + 1, cbuttfont, text);
  
    char tbu[2] = "_";
    wouttextxy(x + 2 + wgettextwidth(text, cbuttfont), y + 1, cbuttfont, tbu);
  }

  int pressedon()
  {
    return 0;
  }

  int processmessage(int mcode, int wParam, long lParam)
  {
    if (mcode == CTB_SETTEXT) {
      strcpy(text, (char *)lParam);
      needredraw = 1;
    } else if (mcode == CTB_GETTEXT)
      strcpy((char *)lParam, text);
    else if (mcode == CTB_KEYPRESS) {
      if (wParam == 8) {
        if (text[0] != 0)
          text[strlen(text) - 1] = 0;

        drawandmouse();
      } else if (strlen(text) >= TEXTBOX_MAXLEN - 1)
        ;
      else if (wgettextwidth(text, cbuttfont) >= wid - 5)
        ;
      else if (wParam > 127)
        ;  // font only has 128 chars
      else {
        text[strlen(text) + 1] = 0;
        text[strlen(text)] = wParam;
        drawandmouse();
      }
    } else
      return -1;

    return 0;
  }
};

#ifdef DJGPP
#pragma warn +inl
#endif

#define MAXCONTROLS 20
#define MAXSCREENWINDOWS 5
NewControl *vobjs[MAXCONTROLS];
OnScreenWindow oswi[MAXSCREENWINDOWS];

int WINAPI _export CSCIGetVersion()
{
  return 0x0100;
}

void multiply_up_to_game_res(int *x, int *y)
{
  x[0] = get_fixed_pixel_size(x[0]);
  y[0] = get_fixed_pixel_size(y[0]);
}

void multiply_up(int *x1, int *y1, int *x2, int *y2)
{
  multiply_up_to_game_res(x1, y1);
  multiply_up_to_game_res(x2, y2);

  // adjust for 800x600
  if ((GetBaseWidth() == 400) || (GetBaseWidth() == 800)) {
    x1[0] = (x1[0] * 5) / 4;
    x2[0] = (x2[0] * 5) / 4;
    y1[0] = (y1[0] * 3) / 2;
    y2[0] = (y2[0] * 3) / 2;
  }
  else if (GetBaseWidth() == 1024)
  {
    x1[0] = (x1[0] * 16) / 10;
    x2[0] = (x2[0] * 16) / 10;
    y1[0] = (y1[0] * 384) / 200;
    y2[0] = (y2[0] * 384) / 200;
  }
}

int windowcount = 0, curswas = 0;
int WINAPI _export CSCIDrawWindow(int xx, int yy, int wid, int hit)
{
  ignore_bounds++;
  multiply_up(&xx, &yy, &wid, &hit);
  int drawit = -1;
  for (int aa = 0; aa < MAXSCREENWINDOWS; aa++) {
    if (oswi[aa].buffer == NULL) {
      drawit = aa;
      break;
    }
  }

  if (drawit < 0)
    quit("Too many windows created.");

  windowcount++;
//  domouse(2);
  xx -= 2;
  yy -= 2;
  wid += 4;
  hit += 4;
  oswi[drawit].buffer = wnewblock(xx, yy, xx + wid, yy + hit);
  oswi[drawit].x = xx;
  oswi[drawit].y = yy;
  wbutt(xx + 1, yy + 1, xx + wid - 1, yy + hit - 1);    // wbutt goes outside its area
//  domouse(1);
  oswi[drawit].oldtop = topwindowhandle;
  topwindowhandle = drawit;
  windowPosX = xx;
  windowPosY = yy;
  windowPosWidth = wid;
  windowPosHeight = hit;
  return drawit;
}

void WINAPI _export CSCIEraseWindow(int handl)
{
//  domouse(2);
  ignore_bounds--;
  topwindowhandle = oswi[handl].oldtop;
  wputblock(oswi[handl].x, oswi[handl].y, oswi[handl].buffer, 0);
  wfreeblock(oswi[handl].buffer);
//  domouse(1);
  oswi[handl].buffer = NULL;
  windowcount--;
}

int controlid = 0;
int checkcontrols()
{
  smcode = 0;
  for (int kk = 0; kk < MAXCONTROLS; kk++) {
    if (vobjs[kk] != NULL) {
      if (vobjs[kk]->mouseisinarea()) {
        controlid = kk;
        return vobjs[kk]->pressedon();
      }
    }
  }
  return 0;
}

int finddefaultcontrol(int flagmask)
{
  for (int ff = 0; ff < MAXCONTROLS; ff++) {
    if (vobjs[ff] == NULL)
      continue;
   
    if (vobjs[ff]->wlevel != topwindowhandle)
      continue;

    if (vobjs[ff]->typeandflags & flagmask)
      return ff;
  }

  return -1;
}

int WINAPI _export CSCIWaitMessage(CSCIMessage * cscim)
{
  next_iteration();
  wtexttransparent(TEXTFG);
  for (int uu = 0; uu < MAXCONTROLS; uu++) {
    if (vobjs[uu] != NULL) {
//      domouse(2);
      vobjs[uu]->drawifneeded();
//      domouse(1);
    }
  }

  windowBuffer = create_bitmap_ex(bitmap_color_depth(abuf), windowPosWidth, windowPosHeight);
  windowBuffer = gfxDriver->ConvertBitmapToSupportedColourDepth(windowBuffer);
  dialogBmp = gfxDriver->CreateDDBFromBitmap(windowBuffer, false, true);

  while (1) {
    timerloop = 0;
    next_iteration();
    refresh_screen();
    
    cscim->id = -1;
    cscim->code = 0;
    smcode = 0;
    if (kbhit()) {
      int keywas = getch();
      if (keywas == 0)
        keywas = getch() + 300;

      if (keywas == 13) {
        cscim->id = finddefaultcontrol(CNF_DEFAULT);
        cscim->code = CM_COMMAND;
      } else if (keywas == 27) {
        cscim->id = finddefaultcontrol(CNF_CANCEL);
        cscim->code = CM_COMMAND;
      } else if ((keywas < 32) && (keywas != 8)) ;
      else if ((keywas >= 372) & (keywas <= 381) & (finddefaultcontrol(CNT_LISTBOX) >= 0))
        vobjs[finddefaultcontrol(CNT_LISTBOX)]->processmessage(CTB_KEYPRESS, keywas, 0);
      else if (finddefaultcontrol(CNT_TEXTBOX) >= 0)
        vobjs[finddefaultcontrol(CNT_TEXTBOX)]->processmessage(CTB_KEYPRESS, keywas, 0);

      if (cscim->id < 0) {
        cscim->code = CM_KEYPRESS;
        cscim->wParam = keywas;
      }
    }

    if (rec_mgetbutton() != NONE) {
      if (checkcontrols()) {
        cscim->id = controlid;
        cscim->code = CM_COMMAND;
      }
    }

    if (smcode) {
      cscim->code = smcode;
      cscim->id = controlid;
    }

    if (cscim->code > 0)
      break;

    update_polled_stuff_and_crossfade();
    while (timerloop == 0) ;
  }

  gfxDriver->DestroyDDB(dialogBmp);
  dialogBmp = NULL;
  destroy_bitmap(windowBuffer);
  windowBuffer = NULL;
  return 0;
}

int WINAPI _export CSCICreateControl(int typeandflags, int xx, int yy, int wii, int hii, char *title)
{
  multiply_up(&xx, &yy, &wii, &hii);
  int usec = -1;
  for (int hh = 1; hh < MAXCONTROLS; hh++) {
    if (vobjs[hh] == NULL) {
      usec = hh;
      break;
    }
  }

  if (usec < 0)
    quit("Too many controls created");

  int type = typeandflags & 0x00ff;     // 256 control types
  if (type == CNT_PUSHBUTTON) {
    if (wii == -1)
      wii = wgettextwidth(title, cbuttfont) + 20;

    vobjs[usec] = new PushButton(xx, yy, wii, hii, title);

  } else if (type == CNT_LISTBOX) {
    vobjs[usec] = new MyListBox(xx, yy, wii, hii);
  } else if (type == CNT_LABEL) {
    vobjs[usec] = new MyLabel(xx, yy, wii, title);
  } else if (type == CNT_TEXTBOX) {
    vobjs[usec] = new MyTextBox(xx, yy, wii, title);
  } else
    quit("Unknown control type requested");

  vobjs[usec]->typeandflags = typeandflags;
  wtexttransparent(TEXTFG);
  vobjs[usec]->wlevel = topwindowhandle;
//  domouse(2);
  vobjs[usec]->draw();
//  domouse(1);
  return usec;
}

void WINAPI _export CSCIDeleteControl(int haa)
{
  delete vobjs[haa];
  vobjs[haa] = NULL;
}

int WINAPI _export CSCISendControlMessage(int haa, int mess, int wPar, long lPar)
{
  if (vobjs[haa] == NULL)
    return -1;
  return vobjs[haa]->processmessage(mess, wPar, lPar);
}

#define MAXSAVEGAMES 20
DisplayProperties dispp;
char *lpTemp, *lpTemp2;
char bufTemp[260], buffer2[260];
int numsaves = 0, toomanygames;
int filenumbers[MAXSAVEGAMES];
unsigned long filedates[MAXSAVEGAMES];
extern void get_save_game_path(int slotNum, char *buffer);
extern char saveGameDirectory[260];

void preparesavegamelist(int ctrllist)
{
  numsaves = 0;
  toomanygames = 0;
  al_ffblk ffb;
  int bufix = 0;
  char curdir[255];
  _getcwd(curdir, 255);

  char searchPath[260];
  sprintf(searchPath, "%s""agssave.*%s", saveGameDirectory, saveGameSuffix);

  int don = al_findfirst(searchPath, &ffb, -1);
  while (!don) {
    bufix = 0;
    if (numsaves >= MAXSAVEGAMES) {
      toomanygames = 1;
      break;
    }

    // only list games .000 to .099 (to allow higher slots for other purposes)
    if (strstr(ffb.name, ".0") == NULL) {
      don = al_findnext(&ffb);
      continue;
    }

    const char *numberExtension = strstr(ffb.name, ".0") + 1;
    int sgNumber = atoi(numberExtension);

    char thisGamePath[260];
    get_save_game_path(sgNumber, thisGamePath);

    // get description
    load_game(sgNumber, buff, NULL);

    CSCISendControlMessage(ctrllist, CLB_ADDITEM, 0, (long)&buff[0]);
    // Select the first item
    CSCISendControlMessage(ctrllist, CLB_SETCURSEL, 0, 0);
    filenumbers[numsaves] = sgNumber;
    filedates[numsaves] = (long int)ffb.time;
    numsaves++;
    don = al_findnext(&ffb);
  }

  al_findclose(&ffb);
  if (numsaves >= MAXSAVEGAMES)
    toomanygames = 1;

  for (int nn = 0; nn < numsaves - 1; nn++) {
    for (int kk = 0; kk < numsaves - 1; kk++) { // Date order the games
      if (filedates[kk] < filedates[kk + 1]) {  // swap them round
        CSCISendControlMessage(ctrllist, CLB_GETTEXT, kk, (long)&buff[0]);
        CSCISendControlMessage(ctrllist, CLB_GETTEXT, kk + 1, (long)&buffer2[0]);
        CSCISendControlMessage(ctrllist, CLB_SETTEXT, kk + 1, (long)&buff[0]);
        CSCISendControlMessage(ctrllist, CLB_SETTEXT, kk, (long)&buffer2[0]);
        int numtem = filenumbers[kk];
        filenumbers[kk] = filenumbers[kk + 1];
        filenumbers[kk + 1] = numtem;
        long numted = filedates[kk];
        filedates[kk] = filedates[kk + 1];
        filedates[kk + 1] = numted;
      }
    }
  }
}

int loadgamedialog()
{
  int boxleft = myscrnwid / 2 - 100;
  int boxtop = myscrnhit / 2 - 60;
  int buttonhit = usetup.textheight + 5;
  int handl = CSCIDrawWindow(boxleft, boxtop, 200, 120);
  int ctrlok =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 135, boxtop + 5, 60, 10, get_global_message(MSG_RESTORE));
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 135, boxtop + 5 + buttonhit, 60, 10,
                      get_global_message(MSG_CANCEL));
  int ctrllist = CSCICreateControl(CNT_LISTBOX, boxleft + 10, boxtop + 30, 120, 80, NULL);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, boxtop + 5, 120, 0, get_global_message(MSG_SELECTLOAD));
  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);

  preparesavegamelist(ctrllist);
  CSCIMessage mes;
  lpTemp = NULL;
  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        int cursel = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
        if ((cursel >= numsaves) | (cursel < 0))
          lpTemp = NULL;
        else {
          toret = filenumbers[cursel];
          get_save_game_path(toret, bufTemp);
          lpTemp = &bufTemp[0];
        }
      } else if (mes.id == ctrlcancel) {
        lpTemp = NULL;
      }

      break;
    }
  }

  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrllist);
  CSCIDeleteControl(ctrlok);
  CSCIDeleteControl(ctrlcancel);
  CSCIEraseWindow(handl);
  return toret;
}

void enterstringwindow(char *prompttext, char *stouse)
{
  int boxleft = 60, boxtop = 80;
  int wantCancel = 0;
  if (prompttext[0] == '!') {
    wantCancel = 1;
    prompttext++;
  }
  int handl = CSCIDrawWindow(boxleft, boxtop, 200, 40);
  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 135, boxtop + 5, 60, 10, "OK");
  int ctrlcancel = -1;
  if (wantCancel)
    ctrlcancel = CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 135, boxtop + 20, 60, 10, get_global_message(MSG_CANCEL));
  int ctrltbox = CSCICreateControl(CNT_TEXTBOX, boxleft + 10, boxtop + 29, 120, 0, NULL);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, boxtop + 5, 120, 0, prompttext);
  CSCIMessage mes;

  while (1) {
    CSCIWaitMessage(&mes);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlcancel)
        buffer2[0] = 0;
      else
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (long)&buffer2[0]);
      break;
    }
  }

  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrltbox);
  CSCIDeleteControl(ctrlok);
  if (wantCancel)
    CSCIDeleteControl(ctrlcancel);
  CSCIEraseWindow(handl);
  strcpy(stouse, buffer2);
}

int enternumberwindow(char *prompttext)
{
  char ourbuf[200];
  enterstringwindow(prompttext, ourbuf);
  if (ourbuf[0] == 0)
    return -9999;
  return atoi(ourbuf);
}

int roomSelectorWindow(int currentRoom, int numRooms, int*roomNumbers, char**roomNames)
{
  char labeltext[200];
  strcpy(labeltext, get_global_message(MSG_SAVEDIALOG));
  int boxleft = myscrnwid / 2 - 120;
  int boxtop = myscrnhit / 2 - 80;
  int buttonhit = usetup.textheight + 5;
  int labeltop = boxtop + 5;
  int handl = CSCIDrawWindow(boxleft, boxtop, 240, 160);
  int ctrllist = CSCICreateControl(CNT_LISTBOX, boxleft + 10, boxtop + 40, 220, 100, NULL);
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 80, boxtop + 145, 60, 10, "Cancel");

  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);    // clear the list box
  for (int aa = 0; aa < numRooms; aa++)
  {
    sprintf(buff, "%3d %s", roomNumbers[aa], roomNames[aa]);
    CSCISendControlMessage(ctrllist, CLB_ADDITEM, 0, (long)&buff[0]);
    if (roomNumbers[aa] == currentRoom)
    {
      CSCISendControlMessage(ctrllist, CLB_SETCURSEL, aa, 0);
    }
  }

  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 10, boxtop + 145, 60, 10, "OK");
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, labeltop, 180, 0, "Choose which room to go to:");
  CSCIMessage mes;

  lpTemp = NULL;
  //sprintf(buffer2, "%d", currentRoom);
  sprintf(buffer2, "");

  int ctrltbox = CSCICreateControl(CNT_TEXTBOX, boxleft + 10, boxtop + 29, 120, 0, NULL);
  CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (long)&buffer2[0]);

  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) 
    {
      if (mes.id == ctrlok) 
      {
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (long)&buffer2[0]);
        if (isdigit(buffer2[0]))
        {
          toret = atoi(buffer2);
        }
      } 
      else if (mes.id == ctrlcancel) 
      {
      }
      break;
    } 
    else if (mes.code == CM_SELCHANGE) 
    {
      int cursel = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
      if (cursel >= 0) 
      {
        sprintf(buffer2, "%d", roomNumbers[cursel]);
        CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (long)&buffer2[0]);
      }
    }
  }

  CSCIDeleteControl(ctrltbox);
  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrllist);
  CSCIDeleteControl(ctrlok);
  CSCIDeleteControl(ctrlcancel);
  CSCIEraseWindow(handl);
  return toret;
}

int savegamedialog()
{
  char okbuttontext[50];
  strcpy(okbuttontext, get_global_message(MSG_SAVEBUTTON));
  char labeltext[200];
  strcpy(labeltext, get_global_message(MSG_SAVEDIALOG));
  int boxleft = myscrnwid / 2 - 100;
  int boxtop = myscrnhit / 2 - 60;
  int buttonhit = usetup.textheight + 5;
  int labeltop = boxtop + 5;
  int handl = CSCIDrawWindow(boxleft, boxtop, 200, 120);
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 135, boxtop + 5 + buttonhit, 60, 10,
                      get_global_message(MSG_CANCEL));
  int ctrllist = CSCICreateControl(CNT_LISTBOX, boxleft + 10, boxtop + 40, 120, 80, NULL);
  int ctrltbox = 0;

  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);    // clear the list box
  preparesavegamelist(ctrllist);
  if (toomanygames) {
    strcpy(okbuttontext, get_global_message(MSG_REPLACE));
    strcpy(labeltext, get_global_message(MSG_MUSTREPLACE));
    labeltop = boxtop + 2;
  } else
    ctrltbox = CSCICreateControl(CNT_TEXTBOX, boxleft + 10, boxtop + 29, 120, 0, NULL);

  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 135, boxtop + 5, 60, 10, okbuttontext);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, labeltop, 120, 0, labeltext);
  CSCIMessage mes;

  lpTemp = NULL;
  if (numsaves > 0)
    CSCISendControlMessage(ctrllist, CLB_GETTEXT, 0, (long)&buffer2[0]);
  else
    buffer2[0] = 0;

  CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (long)&buffer2[0]);

  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        int cursell = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (long)&buffer2[0]);

        if (numsaves > 0)
          CSCISendControlMessage(ctrllist, CLB_GETTEXT, cursell, (long)&bufTemp[0]);
        else
          strcpy(bufTemp, "_NOSAVEGAMENAME");

        if (toomanygames) {
          int nwhand = CSCIDrawWindow(boxleft + 5, boxtop + 20, 190, 65);
          int lbl1 =
            CSCICreateControl(CNT_LABEL, boxleft + 20, boxtop + 25, 160, 0, get_global_message(MSG_REPLACEWITH1));
          int lbl2 = CSCICreateControl(CNT_LABEL, boxleft + 30, boxtop + 34, 160, 0, bufTemp);
          int lbl3 =
            CSCICreateControl(CNT_LABEL, boxleft + 20, boxtop + 45, 160, 0, get_global_message(MSG_REPLACEWITH2));
          int txt1 = CSCICreateControl(CNT_TEXTBOX, boxleft + 20, boxtop + 55, 160, 0, bufTemp);
          int btnOk =
            CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 30, boxtop + 70, 60, 10,
                              get_global_message(MSG_REPLACE));
          int btnCancel =
            CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 100, boxtop + 70, 60, 10,
                              get_global_message(MSG_CANCEL));

          CSCIMessage cmes;
          do {
            CSCIWaitMessage(&cmes);
          } while (cmes.code != CM_COMMAND);

          CSCISendControlMessage(txt1, CTB_GETTEXT, 0, (long)&buffer2[0]);
          CSCIDeleteControl(btnCancel);
          CSCIDeleteControl(btnOk);
          CSCIDeleteControl(txt1);
          CSCIDeleteControl(lbl3);
          CSCIDeleteControl(lbl2);
          CSCIDeleteControl(lbl1);
          CSCIEraseWindow(nwhand);
          bufTemp[0] = 0;

          if (cmes.id == btnCancel) {
            lpTemp = NULL;
            break;
          } else
            toret = filenumbers[cursell];

        } 
        else if (strcmp(buffer2, bufTemp) != 0) {     // create a new game (description different)
          int highestnum = 0;
          for (int pp = 0; pp < numsaves; pp++) {
            if (filenumbers[pp] > highestnum)
              highestnum = filenumbers[pp];
          }

          if (highestnum > 90)
            quit("Save game directory overflow");

          toret = highestnum + 1;
          get_save_game_path(toret, bufTemp);
        } 
        else {
          toret = filenumbers[cursell];
          bufTemp[0] = 0;
        }

        if (bufTemp[0] == 0)
          get_save_game_path(toret, bufTemp);

        lpTemp = &bufTemp[0];
        lpTemp2 = &buffer2[0];
      } else if (mes.id == ctrlcancel) {
        lpTemp = NULL;
      }
      break;
    } else if (mes.code == CM_SELCHANGE) {
      int cursel = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
      if (cursel >= 0) {
        CSCISendControlMessage(ctrllist, CLB_GETTEXT, cursel, (long)&buffer2[0]);
        CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (long)&buffer2[0]);
      }
    }
  }

  CSCIDeleteControl(ctrltbox);
  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrllist);
  CSCIDeleteControl(ctrlok);
  CSCIDeleteControl(ctrlcancel);
  CSCIEraseWindow(handl);
  return toret;
}

CSCIMessage smes;
int myscimessagebox(char *lpprompt, char *btn1, char *btn2)
{
  int windl = CSCIDrawWindow(80, 80, 240 - 80, 120 - 80);
  int lbl1 = CSCICreateControl(CNT_LABEL, 90, 85, 150, 0, lpprompt);
  int btflag = CNT_PUSHBUTTON;

  if (btn2 == NULL)
    btflag |= CNF_DEFAULT | CNF_CANCEL;
  else
    btflag |= CNF_DEFAULT;

  int btnQuit = CSCICreateControl(btflag, 90, 105, 60, 10, btn1);
  int btnPlay = 0;

  if (btn2 != NULL)
    btnPlay = CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 165, 105, 60, 10, btn2);

  smes.code = 0;

  do {
    CSCIWaitMessage(&smes);
  } while (smes.code != CM_COMMAND);

  if (btnPlay)
    CSCIDeleteControl(btnPlay);

  CSCIDeleteControl(btnQuit);
  CSCIDeleteControl(lbl1);
  CSCIEraseWindow(windl);

  if (smes.id == btnQuit)
    return 1;

  return 0;
}

int quitdialog()
{
  char quitbut[50], playbut[50];
  strcpy(quitbut, get_global_message(MSG_QUITBUTTON));
  strcpy(playbut, get_global_message(MSG_PLAYBUTTON));
  return myscimessagebox(get_global_message(MSG_QUITDIALOG), quitbut, playbut);
}

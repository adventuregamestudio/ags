/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#include "wgt2allg.h"
#include "ali3d.h"
#include "acdialog/ac_cscidialog.h"
#include "acdialog/ac_newcontrol.h"
#include "acdialog/ac_mylabel.h"
#include "acdialog/ac_mylistbox.h"
#include "acdialog/ac_mytextbox.h"
#include "acdialog/ac_pushbutton.h"
#include "acdialog/ac_dialoginternaldefines.h"


// DIALOG SYSTEM STUFF below

char buff[200];
int myscrnwid = 320, myscrnhit = 200;
IDriverDependantBitmap *dialogBmp = NULL;
int windowPosX, windowPosY, windowPosWidth, windowPosHeight;
block windowBuffer = NULL;

int windowbackgroundcolor = COL254, pushbuttondarkcolor = COL255;
int pushbuttonlightcolor = COL253;
int topwindowhandle = -1;
int cbuttfont;
int acdialog_font;

int smcode = 0;

void __my_wbutt(int x1, int y1, int x2, int y2)
{
    wsetcolor(COL254);            //wsetcolor(15);
    wbar(x1, y1, x2, y2);
    wsetcolor(0);
    wrectangle(x1, y1, x2, y2);
}

void refresh_screen()
{
    blit(abuf, windowBuffer, windowPosX, windowPosY, 0, 0, windowPosWidth, windowPosHeight);
    gfxDriver->UpdateDDBFromBitmap(dialogBmp, windowBuffer, false);

    render_graphics(dialogBmp, windowPosX, windowPosY);

    // Copy it back, because the mouse will have been drawn on top
    blit(windowBuffer, abuf, 0, 0, windowPosX, windowPosY, windowPosWidth, windowPosHeight);
}


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

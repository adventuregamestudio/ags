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
#include "gui/guidialog.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "gui/cscidialog.h"
#include <cctype> //isdigit()
#include "gfx/graphicsdriver.h"
#include "gfx/graphics.h"

using AGS::Common::String;
using AGS::Common::Bitmap;
using AGS::Common::Graphics;

extern IGraphicsDriver *gfxDriver;
extern GameSetup usetup;

// from ac_game
extern char saveGameDirectory[260];
extern char saveGameSuffix[MAX_SG_EXT_LENGTH + 1];

// from gui/cscidialog
extern Bitmap *windowBuffer;
extern int windowPosX, windowPosY, windowPosWidth, windowPosHeight;
extern Bitmap *windowBuffer;
extern IDriverDependantBitmap *dialogBmp;

#define MAXSAVEGAMES 20
DisplayProperties dispp;
char *lpTemp, *lpTemp2;
char bufTemp[260], buffer2[260];
int numsaves = 0, toomanygames;
int filenumbers[MAXSAVEGAMES];
unsigned long filedates[MAXSAVEGAMES];

CSCIMessage smes;

char buff[200];
int myscrnwid = 320, myscrnhit = 200;



void refresh_screen()
{
    Common::Graphics *g = GetVirtualScreenGraphics();
    Graphics window_graphics(windowBuffer);
    window_graphics.Blit(g->GetBitmap(), windowPosX, windowPosY, 0, 0, windowPosWidth, windowPosHeight);
    gfxDriver->UpdateDDBFromBitmap(dialogBmp, windowBuffer, false);

    render_graphics(dialogBmp, windowPosX, windowPosY);

    // Copy it back, because the mouse will have been drawn on top
    g->Blit(windowBuffer, 0, 0, windowPosX, windowPosY, windowPosWidth, windowPosHeight);
}

int loadgamedialog()
{
  int boxleft = myscrnwid / 2 - 100;
  int boxtop = myscrnhit / 2 - 60;
  int buttonhit = usetup.textheight + 5;
  Common::Graphics *g = GetVirtualScreenGraphics();
  int handl = CSCIDrawWindow(g, boxleft, boxtop, 200, 120);
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
    CSCIWaitMessage(g, &mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        int cursel = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
        if ((cursel >= numsaves) | (cursel < 0))
          lpTemp = NULL;
        else {
          toret = filenumbers[cursel];
          String path = get_save_game_path(toret);
          strcpy(bufTemp, path);
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
  CSCIEraseWindow(g, handl);
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
  Common::Graphics *g = GetVirtualScreenGraphics();
  int handl = CSCIDrawWindow(g, boxleft, boxtop, 200, 120);
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
    CSCIWaitMessage(g, &mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        int cursell = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (long)&buffer2[0]);

        if (numsaves > 0)
          CSCISendControlMessage(ctrllist, CLB_GETTEXT, cursell, (long)&bufTemp[0]);
        else
          strcpy(bufTemp, "_NOSAVEGAMENAME");

        if (toomanygames) {
          int nwhand = CSCIDrawWindow(g, boxleft + 5, boxtop + 20, 190, 65);
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
            CSCIWaitMessage(g, &cmes);
          } while (cmes.code != CM_COMMAND);

          CSCISendControlMessage(txt1, CTB_GETTEXT, 0, (long)&buffer2[0]);
          CSCIDeleteControl(btnCancel);
          CSCIDeleteControl(btnOk);
          CSCIDeleteControl(txt1);
          CSCIDeleteControl(lbl3);
          CSCIDeleteControl(lbl2);
          CSCIDeleteControl(lbl1);
          CSCIEraseWindow(g, nwhand);
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
          String path = get_save_game_path(toret);
          strcpy(bufTemp, path);
        } 
        else {
          toret = filenumbers[cursell];
          bufTemp[0] = 0;
        }

        if (bufTemp[0] == 0)
        {
          String path = get_save_game_path(toret);
          strcpy(bufTemp, path);
        }

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
  CSCIEraseWindow(g, handl);
  return toret;
}

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

    String thisGamePath = get_save_game_path(sgNumber);

    // get description
    String description;
    read_savedgame_description(thisGamePath, description);

    CSCISendControlMessage(ctrllist, CLB_ADDITEM, 0, (long)description.GetCStr());
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

void enterstringwindow(char *prompttext, char *stouse)
{
  int boxleft = 60, boxtop = 80;
  int wantCancel = 0;
  if (prompttext[0] == '!') {
    wantCancel = 1;
    prompttext++;
  }
  Common::Graphics *g = GetVirtualScreenGraphics();
  int handl = CSCIDrawWindow(g, boxleft, boxtop, 200, 40);
  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 135, boxtop + 5, 60, 10, "OK");
  int ctrlcancel = -1;
  if (wantCancel)
    ctrlcancel = CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 135, boxtop + 20, 60, 10, get_global_message(MSG_CANCEL));
  int ctrltbox = CSCICreateControl(CNT_TEXTBOX, boxleft + 10, boxtop + 29, 120, 0, NULL);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, boxtop + 5, 120, 0, prompttext);
  CSCIMessage mes;

  while (1) {
    CSCIWaitMessage(g, &mes);
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
  CSCIEraseWindow(g, handl);
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
  Common::Graphics *g = GetVirtualScreenGraphics();
  int handl = CSCIDrawWindow(g, boxleft, boxtop, 240, 160);
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
    CSCIWaitMessage(g, &mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
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
  CSCIEraseWindow(g, handl);
  return toret;
}

int myscimessagebox(char *lpprompt, char *btn1, char *btn2)
{
    Common::Graphics *g = GetVirtualScreenGraphics();
    int windl = CSCIDrawWindow(g, 80, 80, 240 - 80, 120 - 80);
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
        CSCIWaitMessage(g, &smes);
    } while (smes.code != CM_COMMAND);

    if (btnPlay)
        CSCIDeleteControl(btnPlay);

    CSCIDeleteControl(btnQuit);
    CSCIDeleteControl(lbl1);
    CSCIEraseWindow(g, windl);

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

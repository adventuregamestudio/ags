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
#include <algorithm>
#include <cstdio>
#include <allegro.h> // find files
#include "gui/guidialog.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "gui/cscidialog.h"
#include <cctype> //isdigit()
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern IGraphicsDriver *gfxDriver;
extern GameSetupStruct game;
extern GamePlayState play;

namespace {

// TODO: store drawing surface inside old gui classes instead
int windowPosX, windowPosY, windowPosWidth, windowPosHeight;
Bitmap *windowBuffer;
IDriverDependantBitmap *dialogDDB;

char *lpTemp, *lpTemp2;
char bufTemp[260], buffer2[260];
int numsaves = 0, toomanygames;
std::vector<int> filenumbers;
std::vector<time_t> filedates;

CSCIMessage smes;

char buff[200];
int myscrnwid = 320, myscrnhit = 200;

}

char *get_gui_dialog_buffer()
{
  return buffer2;
}

//
// TODO: rewrite the whole thing to work inside the main game update and render loop!
//

// These were GlobalMessages (984 - 995).
const char *GUIDialog_Strings[NUM_GUIDIALOGMSG] =
{
    "Restore",
    "Cancel",
    "Select a game to restore:",
    "Save",
    "Type a name to save as:",
    "Replace",
    "The save directory is full. You must replace an existing game:",
    "Replace:",
    "With:",
    "Quit",
    "Play",
    "Are you sure you want to quit?",
};

Bitmap *prepare_gui_screen(int x, int y, int width, int height, bool opaque)
{
    windowPosX = x;
    windowPosY = y;
    windowPosWidth = width;
    windowPosHeight = height;
    if (windowBuffer)
    {
        windowBuffer = recycle_bitmap(windowBuffer, windowBuffer->GetColorDepth(), windowPosWidth, windowPosHeight, !opaque);
    }
    else
    {
        windowBuffer = CreateCompatBitmap(windowPosWidth, windowPosHeight);
    }
    dialogDDB = recycle_ddb_bitmap(dialogDDB, windowBuffer, opaque);
    return windowBuffer;
}

Bitmap *get_gui_screen()
{
    return windowBuffer;
}

void clear_gui_screen()
{
    if (dialogDDB)
        gfxDriver->DestroyDDB(dialogDDB);
    dialogDDB = nullptr;
    delete windowBuffer;
    windowBuffer = nullptr;
}

void refresh_gui_screen()
{
    gfxDriver->UpdateDDBFromBitmap(dialogDDB, windowBuffer);
    UpdateCursorAndDrawables();
    render_graphics(dialogDDB, windowPosX, windowPosY);
}

void preparesavegamelist(int ctrllist, int min_slot, int max_slot);

int loadgamedialog(int min_slot, int max_slot)
{
  const int wnd_width = 200;
  const int wnd_height = 120;
  const int boxleft = myscrnwid / 2 - wnd_width / 2;
  const int boxtop = myscrnhit / 2 - wnd_height / 2;
  const int buttonhit = play.std_gui_textheight + 5;

  int handl = CSCIDrawWindow(boxleft, boxtop, wnd_width, wnd_height);
  int ctrlok =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, 135, 5, 60, 10, GUIDialog_Strings[MSG_RESTORE]);
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 135, 5 + buttonhit, 60, 10,
                      GUIDialog_Strings[MSG_CANCEL]);
  int ctrllist = CSCICreateControl(CNT_LISTBOX, 10, 30, 120, 80, nullptr);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, 10, 5, 120, 0, GUIDialog_Strings[MSG_SELECTLOAD]);
  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);

  preparesavegamelist(ctrllist, min_slot, max_slot);
  CSCIMessage mes;
  lpTemp = nullptr;
  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        int cursel = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
        if ((cursel >= numsaves) | (cursel < 0))
          lpTemp = nullptr;
        else {
          toret = filenumbers[cursel];
          String path = get_save_game_path(toret);
          strcpy(bufTemp, path.GetCStr());
          lpTemp = &bufTemp[0];
        }
      } else if (mes.id == ctrlcancel) {
        lpTemp = nullptr;
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

int savegamedialog(int min_slot, int max_slot)
{
  char okbuttontext[50];
  strcpy(okbuttontext, GUIDialog_Strings[MSG_SAVEBUTTON]);
  char labeltext[200];
  strcpy(labeltext, GUIDialog_Strings[MSG_SAVEDIALOG]);
  const int wnd_width = 200;
  const int wnd_height = 120;
  const int boxleft = myscrnwid / 2 - wnd_width / 2;
  const int boxtop = myscrnhit / 2 - wnd_height / 2;
  const int buttonhit = play.std_gui_textheight + 5;
  int labeltop = 5;

  int handl = CSCIDrawWindow(boxleft, boxtop, wnd_width, wnd_height);
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 135, 5 + buttonhit, 60, 10,
                      GUIDialog_Strings[MSG_CANCEL]);
  int ctrllist = CSCICreateControl(CNT_LISTBOX, 10, 40, 120, 80, nullptr);
  int ctrltbox = 0;

  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);    // clear the list box
  preparesavegamelist(ctrllist, min_slot, max_slot);
  if (toomanygames) {
    strcpy(okbuttontext, GUIDialog_Strings[MSG_REPLACE]);
    strcpy(labeltext, GUIDialog_Strings[MSG_MUSTREPLACE]);
    labeltop = 2;
  } else
    ctrltbox = CSCICreateControl(CNT_TEXTBOX, 10, 29, 120, 0, nullptr);

  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, 135, 5, 60, 10, okbuttontext);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, 10, labeltop, 120, 0, labeltext);
  CSCIMessage mes;

  lpTemp = nullptr;
  if (numsaves > 0)
    CSCISendControlMessage(ctrllist, CLB_GETTEXT, 0, (intptr_t)&buffer2[0]);
  else
    buffer2[0] = 0;

  CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (intptr_t)&buffer2[0]);

  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        int cursell = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (intptr_t)&buffer2[0]);

        if (numsaves > 0)
          CSCISendControlMessage(ctrllist, CLB_GETTEXT, cursell, (intptr_t)&bufTemp[0]);
        else
          strcpy(bufTemp, "_NOSAVEGAMENAME");

        if (toomanygames) {
          int nwhand = CSCIDrawWindow(boxleft + 5, boxtop + 20, 190, 65);
          int lbl1 =
            CSCICreateControl(CNT_LABEL, 15, 5, 160, 0, GUIDialog_Strings[MSG_REPLACEWITH1]);
          int lbl2 = CSCICreateControl(CNT_LABEL, 25, 14, 160, 0, bufTemp);
          int lbl3 =
            CSCICreateControl(CNT_LABEL, 15, 25, 160, 0, GUIDialog_Strings[MSG_REPLACEWITH2]);
          int txt1 = CSCICreateControl(CNT_TEXTBOX, 15, 35, 160, 0, bufTemp);
          int btnOk =
            CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, 25, 50, 60, 10,
                              GUIDialog_Strings[MSG_REPLACE]);
          int btnCancel =
            CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 95, 50, 60, 10,
                              GUIDialog_Strings[MSG_CANCEL]);

          CSCIMessage cmes;
          do {
            CSCIWaitMessage(&cmes);
          } while (cmes.code != CM_COMMAND);

          CSCISendControlMessage(txt1, CTB_GETTEXT, 0, (intptr_t)&buffer2[0]);
          CSCIDeleteControl(btnCancel);
          CSCIDeleteControl(btnOk);
          CSCIDeleteControl(txt1);
          CSCIDeleteControl(lbl3);
          CSCIDeleteControl(lbl2);
          CSCIDeleteControl(lbl1);
          CSCIEraseWindow(nwhand);
          bufTemp[0] = 0;

          if (cmes.id == btnCancel) {
            lpTemp = nullptr;
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
          strcpy(bufTemp, path.GetCStr());
        } 
        else {
          toret = filenumbers[cursell];
          bufTemp[0] = 0;
        }

        if (bufTemp[0] == 0)
        {
          String path = get_save_game_path(toret);
          strcpy(bufTemp, path.GetCStr());
        }

        lpTemp = &bufTemp[0];
        lpTemp2 = &buffer2[0];
      } else if (mes.id == ctrlcancel) {
        lpTemp = nullptr;
      }
      break;
    } else if (mes.code == CM_SELCHANGE) {
      int cursel = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
      if (cursel >= 0) {
        CSCISendControlMessage(ctrllist, CLB_GETTEXT, cursel, (intptr_t)&buffer2[0]);
        CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (intptr_t)&buffer2[0]);
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

void preparesavegamelist(int ctrllist, int min_slot, int max_slot)
{
  max_slot = std::min(max_slot, TOP_SAVESLOT);
  min_slot = std::min(max_slot, std::max(0, min_slot));

  std::vector<SaveListItem> saves;
  FillSaveList(saves, min_slot, max_slot, true);
  std::sort(saves.rbegin(), saves.rend(), SaveItemCmpByTime()); // sort by time in reverse

  filenumbers.resize(saves.size());
  filedates.resize(saves.size());
  for (numsaves = 0; (size_t)numsaves < saves.size(); ++numsaves)
  {
      CSCISendControlMessage(ctrllist, CLB_ADDITEM, 0, (intptr_t)saves[numsaves].Description.GetCStr());
      filenumbers[numsaves] = saves[numsaves].Slot;
      filedates[numsaves] = saves[numsaves].FileTime;
  }
  // "toomanygames" if the whole range of slots is occupied
  toomanygames = 
      saves.size() >= static_cast<uint32_t>(max_slot - min_slot);
  // Select the first item
  CSCISendControlMessage(ctrllist, CLB_SETCURSEL, 0, 0);
}

void enterstringwindow(const char *prompttext, char *dst_buf, size_t dst_sz)
{
  const int wnd_width = 200;
  const int wnd_height = 40;
  const int boxleft = 60, boxtop = 80;
  int wantCancel = 0;
  if (prompttext[0] == '!') {
    wantCancel = 1;
    prompttext++;
  }
  
  int handl = CSCIDrawWindow(boxleft, boxtop, wnd_width, wnd_height);
  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, 135, 5, 60, 10, "OK");
  int ctrlcancel = -1;
  if (wantCancel)
    ctrlcancel = CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 135, 20, 60, 10, GUIDialog_Strings[MSG_CANCEL]);
  int ctrltbox = CSCICreateControl(CNT_TEXTBOX, 10, 29, 120, 0, nullptr);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, 10, 5, 120, 0, prompttext);
  CSCIMessage mes;

  while (1) {
    CSCIWaitMessage(&mes);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlcancel)
        buffer2[0] = 0;
      else
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (intptr_t)&buffer2[0]);
      break;
    }
  }

  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrltbox);
  CSCIDeleteControl(ctrlok);
  if (wantCancel)
    CSCIDeleteControl(ctrlcancel);
  CSCIEraseWindow(handl);
  snprintf(dst_buf, dst_sz, "%s", buffer2);
}

int enternumberwindow(const char *prompttext)
{
  char ourbuf[200];
  enterstringwindow(prompttext, ourbuf, sizeof(ourbuf));
  if (ourbuf[0] == 0)
    return -9999;
  return atoi(ourbuf);
}

int roomSelectorWindow(int currentRoom, const std::map<int, String> &roomNames)
{
  char labeltext[200];
  strcpy(labeltext, GUIDialog_Strings[MSG_SAVEDIALOG]);
  const int wnd_width = 240;
  const int wnd_height = 160;
  const int boxleft = myscrnwid / 2 - wnd_width / 2;
  const int boxtop = myscrnhit / 2 - wnd_height / 2;
  const int labeltop = 5;

  int handl = CSCIDrawWindow(boxleft, boxtop, wnd_width, wnd_height);
  int ctrllist = CSCICreateControl(CNT_LISTBOX, 10, 40, 220, 100, nullptr);
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 80, 145, 60, 10, "Cancel");

  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);    // clear the list box
  int cur_sel = 0;
  std::map<int, int> room_item_index;
  for (const auto &name : roomNames)
  {
    snprintf(buff, sizeof(buff), "%3d %s", name.first, name.second.GetCStr());
    CSCISendControlMessage(ctrllist, CLB_ADDITEM, 0, (intptr_t)&buff[0]);
    if (name.first == currentRoom)
    {
      CSCISendControlMessage(ctrllist, CLB_SETCURSEL, cur_sel, 0);
    }
    room_item_index[cur_sel++] = name.first;
  }

  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, 10, 145, 60, 10, "OK");
  int ctrltex1 = CSCICreateControl(CNT_LABEL, 10, labeltop, 180, 0, "Choose which room to go to:");
  CSCIMessage mes;

  lpTemp = nullptr;
  buffer2[0] = 0;

  int ctrltbox = CSCICreateControl(CNT_TEXTBOX, 10, 29, 120, 0, nullptr);
  CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (intptr_t)&buffer2[0]);

  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) 
    {
      if (mes.id == ctrlok) 
      {
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (intptr_t)&buffer2[0]);
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
        int room_index = room_item_index[cursel];
        snprintf(buffer2, sizeof(buffer2), "%d", room_index);
        CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (intptr_t)&buffer2[0]);
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

int myscimessagebox(const char *lpprompt, char *btn1, char *btn2)
{
    const int wnd_width = 240 - 80;
    const int wnd_height = 120 - 80;
    const int boxleft = 80;
    const int boxtop = 80;

    int windl = CSCIDrawWindow(boxleft, boxtop, wnd_width, wnd_height);
    int lbl1 = CSCICreateControl(CNT_LABEL, 10, 5, 150, 0, lpprompt);
    int btflag = CNT_PUSHBUTTON;

    if (btn2 == nullptr)
        btflag |= CNF_DEFAULT | CNF_CANCEL;
    else
        btflag |= CNF_DEFAULT;

    int btnQuit = CSCICreateControl(btflag, 10, 25, 60, 10, btn1);
    int btnPlay = 0;

    if (btn2 != nullptr)
        btnPlay = CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 85, 25, 60, 10, btn2);

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
    strcpy(quitbut, GUIDialog_Strings[MSG_QUITBUTTON]);
    strcpy(playbut, GUIDialog_Strings[MSG_PLAYBUTTON]);
    return myscimessagebox(GUIDialog_Strings[MSG_QUITDIALOG], quitbut, playbut);
}

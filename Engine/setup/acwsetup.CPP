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

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource/resource.h"
#include "ac/gamestructdefines.h"
#include "gfx/gfxfilter.h"
#include "main/config.h"
#include "platform/base/agsplatformdriver.h"
#include "util/filestream.h"
#include "util/ini_util.h"

using namespace AGS::Common;

#define AL_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

#define DIGI_DIRECTAMX(n)        AL_ID('A','X','A'+(n),' ')
// This DirectX hardware mixer is crap, it crashes the program
// when two sound effects are played at once
#define DIGI_DIRECTX(n)          AL_ID('D','X','A'+(n),' ')
#define DIGI_WAVOUTID(n)         AL_ID('W','O','A'+(n),' ')
#define DIGI_NONE  0
#define MIDI_AUTODETECT       -1 
#define MIDI_NONE             0 
#define MIDI_WIN32MAPPER         AL_ID('W','3','2','M')

#define MOUSE_SPEED_MIN     1
#define MOUSE_SPEED_MAX     100

extern "C" HWND allegro_wnd;
extern void fgetstring_limit (char *, Stream *, int);
const char*setupstring, *enginever;
int curdigi=0,curmidi=0,windowed=0,useletterbox = 0;
GameResolutionType defaultRes = kGameResolution_Undefined;
int gameColDep = 0;
int refresh = 0, antialias = 0, reduce32to16 = 0;
int sideBorders = 1;
String curtranslation = "";
String curdatadir = ".";
String curFilter = "";
String curGfxDriver = "";
String gameNameForTitleBar = "";
int curmaxcache = 1024 * 20, curusespeech = 1;
int wwidth, wheight;
int base_height;
GFXFilter **filterList = NULL;
const int specialFilterListIndex = 1;
const char *specialFilterName = "Max nearest-neighbour filter";
const char *specialFilterID = "max";
const ConfigTree *wincfg_in;
ConfigTree *wincfg_out;

void MakeFullLongPath(const char *path, char *out_buf, int buf_len)
{
    GetFullPathName(path, buf_len, out_buf, NULL);
    GetLongPathName(out_buf, out_buf, buf_len);
}

int get_filter_real_index(int list_index)
{
    if (list_index == specialFilterListIndex)
    {
        return -1;
    }

    return list_index > specialFilterListIndex ? list_index - 1 : list_index;    
}

int get_filter_list_index(int real_index)
{
    return real_index >= specialFilterListIndex ? real_index + 1 : real_index;
}

const char *get_filter_id(int list_index)
{
    if (list_index < 0)
    {
        return "";
    }

    int real_index = get_filter_real_index(list_index);
    return real_index >= 0 ? filterList[real_index]->GetFilterID() : specialFilterID;
}

void update_resolution_texts(HWND hDlg) {
  
  bool letterbox_by_design = INIreadint(*wincfg_in, "misc", "letterbox") > 0;
  Size res_size = ResolutionTypeToSize(defaultRes, letterbox_by_design);
  int resx = res_size.Width;
  int resy = res_size.Height;
  
  int selectedFilter = SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_GETCURSEL, 0, 0);

  char buffer[100];
  int thisx, thisy;

  thisx = resx;
  thisy = resy;
  selectedFilter = get_filter_real_index(selectedFilter);
  if (selectedFilter >= 0)
  {
    filterList[selectedFilter]->GetRealResolution(&thisx, &thisy);
  }
  sprintf(buffer, "Resolution: %d""x%d", thisx, thisy);
  SendDlgItemMessage (hDlg, IDC_RESOLUTION, WM_SETTEXT, 0, (LPARAM)buffer);
}

void retrieve_filter_list(bool isD3D)
{
  if (filterList != NULL)
  {
    for (int i = 0; filterList[i] != NULL; i++)
    {
      delete filterList[i];
    }
  }

  if (isD3D)
  {
    filterList = get_d3d_gfx_filter_list(true);
  }
  else
  {
    filterList = get_allegro_gfx_filter_list(true);
  }
}

void populate_drop_down_with_filters(HWND hDlg)
{
  SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_RESETCONTENT, 0, 0);

  int idx = 0;
  GFXFilter *thisFilter = filterList[idx];
  int selected_filter = 0;
  while (thisFilter != NULL) {

    if ((idx != 0) && (INIreadint(*wincfg_in, "disabled", thisFilter->GetFilterID()) == 1)) {
      // this filter is disabled
      delete thisFilter;
      // remove from list of filters
      for (int i = idx; filterList[i] != NULL; i++)
        filterList[i] = filterList[i + 1];

      thisFilter = filterList[idx];
      continue;
    }

    const char *filterName = thisFilter->GetVersionBoxText();
    char filterNameBuffer[100];

    if ((filterName == NULL) || (filterName[0] == 0)) {
      strcpy(filterNameBuffer, "None");
    }
    else {
      strcpy(filterNameBuffer, filterName);
      // strip trailing [
      if (filterNameBuffer[strlen(filterNameBuffer) - 1] == '[')
        filterNameBuffer[strlen(filterNameBuffer) - 1] = 0;
    }

    SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_ADDSTRING, 0, (LPARAM)filterNameBuffer);

    if (strcmp(curFilter, thisFilter->GetFilterID()) == 0)
    {
      selected_filter = idx;
    }

    idx++;
    thisFilter = filterList[idx];
  }

  SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_INSERTSTRING, specialFilterListIndex, (LPARAM)specialFilterName);
  if (stricmp(curFilter, specialFilterID) == 0)
  {
    selected_filter = specialFilterListIndex;
  }
  else
  {
    selected_filter = get_filter_list_index(selected_filter);
  }
  SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_SETCURSEL, selected_filter, 0);
}

void update_gfx_filter_box_enabled(HWND hDlg)
{
  int selDrv = SendDlgItemMessage(hDlg, IDC_GFXDRIVER, CB_GETCURSEL, 0,0);

  retrieve_filter_list(selDrv == 1);
  populate_drop_down_with_filters(hDlg);
/*
  if (selDrv == 1)
    SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_SETCURSEL, 0, 0);
  EnableWindow(GetDlgItem(hDlg, IDC_GFXFILTER), selDrv == 0);*/
  update_resolution_texts(hDlg);
}

void update_mouse_speed_text(HWND hDlg)
{
  int slider_pos = SendDlgItemMessage(hDlg,IDC_MOUSESPEED, TBM_GETPOS, 0, 0);
  float mouse_speed = (float)slider_pos / 10.f;
  String text = mouse_speed == 1.f ? "Mouse speed: x 1.0 (Default)" : String::FromFormat("Mouse speed: x %0.1f", mouse_speed);
  SendDlgItemMessage(hDlg, IDC_MOUSESPEED_TEXT, WM_SETTEXT, 0, (LPARAM)text.GetCStr());
}

void InitializeDialog(HWND hDlg) {
  struct _finddata_t c_file;
  long hFile;
  //FILE *fhan;
  int idx;
  retrieve_filter_list(false);

  SendDlgItemMessage(hDlg,IDC_VERSION, WM_SETTEXT, 0, (LPARAM)setupstring);

  SendDlgItemMessage(hDlg,IDC_GFXDRIVER,CB_ADDSTRING,0,(LPARAM)"DirectDraw 5");
  SendDlgItemMessage(hDlg,IDC_GFXDRIVER,CB_ADDSTRING,0,(LPARAM)"Direct3D 9");
  SendDlgItemMessage(hDlg,IDC_COMBO1,CB_RESETCONTENT,0,0);
  SendDlgItemMessage(hDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)"Default DirectSound Device");
  SendDlgItemMessage(hDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)"Default WaveOut Device");
  SendDlgItemMessage(hDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)"No Digital Sound");
  SendDlgItemMessage(hDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)"DirectSound (Hardware mixer)");
  SendDlgItemMessage(hDlg,IDC_COMBO1,CB_SETTOPINDEX,0,0);
  SendDlgItemMessage(hDlg,IDC_COMBO1,CB_SETCURSEL,curdigi,0);
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"Default MCI Music Device");
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"Disable MIDI music");
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"Win32 MIDI Mapper");
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_SETTOPINDEX,0,0);
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_SETCURSEL,curmidi,0);
  SendDlgItemMessage(hDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)"Game Default");
  SendDlgItemMessage(hDlg,IDC_COMBO3,CB_SETCURSEL,0,0);
  idx = 1;

  char translationFindSpec[255];
  sprintf(translationFindSpec, "%s\\*.tra", curdatadir.GetCStr());
  // Find and add all translations in folder
  if ((hFile = _findfirst(translationFindSpec, &c_file )) != -1L ) {
    do {
      if (c_file.name[strlen(c_file.name)-4] == '.') {
        // it actually returns *.tra* so make sure it's a .TRA file,
        // then add to our list
        c_file.name[0] = toupper (c_file.name[0]);
        c_file.name[strlen(c_file.name)-4] = 0;
        int needToSet = 0;
        // if this is the translation we're using, set it
        if (stricmp (curtranslation, c_file.name) == 0)
          needToSet = 1;
        strcat (c_file.name, " translation");
        SendDlgItemMessage(hDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)c_file.name);

        if (needToSet)
          SendDlgItemMessage (hDlg, IDC_COMBO3, CB_SETCURSEL, idx, 0);
        idx++;
      }
    } while( _findnext( hFile, &c_file ) == 0 );

    _findclose( hFile );
  }

  // Custom save dir controls
  String custom_save_dir = INIreadstring(*wincfg_in, "misc", "user_data_dir");
  bool has_save_dir = !custom_save_dir.IsEmpty();
  if (!has_save_dir)
      custom_save_dir = curdatadir;
  SendDlgItemMessage(hDlg, IDC_CUSTOMSAVEDIRCHECK, BM_SETCHECK, has_save_dir ? BST_CHECKED : BST_UNCHECKED, 0);
  char full_save_dir[MAX_PATH] = {0};
  MakeFullLongPath(custom_save_dir, full_save_dir, MAX_PATH);
  SendDlgItemMessage(hDlg, IDC_CUSTOMSAVEDIR, WM_SETTEXT, 0, (LPARAM)full_save_dir);
  EnableWindow(GetDlgItem(hDlg, IDC_CUSTOMSAVEDIR), has_save_dir ? TRUE : FALSE);
  EnableWindow(GetDlgItem(hDlg, IDC_CUSTOMSAVEDIRBTN), has_save_dir ? TRUE : FALSE);

  bool mouse_autolock = INIreadint(*wincfg_in, "mouse", "auto_lock") > 0;
  if (mouse_autolock)
      SendDlgItemMessage(hDlg, IDC_MOUSE_AUTOLOCK, BM_SETCHECK, BST_CHECKED, 0);

  SendDlgItemMessage(hDlg, IDC_MOUSESPEED, TBM_SETRANGE, (WPARAM)TRUE,
      (LPARAM)MAKELONG(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX));
  float mouse_speed = INIreadfloat(*wincfg_in, "mouse", "speed", 1.f);
  if (mouse_speed <= 0.f)
      mouse_speed = 1.f;
  int slider_pos = (int)(mouse_speed * 10.f + .5f);
  SendDlgItemMessage(hDlg,IDC_MOUSESPEED, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)slider_pos);
  update_mouse_speed_text(hDlg);

  SendDlgItemMessage(hDlg,IDC_COMBO4,CB_ADDSTRING,0,(LPARAM)"10 MB");
  SendDlgItemMessage(hDlg,IDC_COMBO4,CB_ADDSTRING,0,(LPARAM)"20 MB (default)");
  SendDlgItemMessage(hDlg,IDC_COMBO4,CB_ADDSTRING,0,(LPARAM)"50 MB");
  SendDlgItemMessage(hDlg,IDC_COMBO4,CB_ADDSTRING,0,(LPARAM)"100 MB");
  idx = 0;
  if (curmaxcache >= 100*1024)
    idx = 3;
  else if (curmaxcache >= 50*1024)
    idx = 2;
  else if (curmaxcache >= 20*1024)
    idx = 1;
    
  SendDlgItemMessage(hDlg,IDC_COMBO4,CB_SETCURSEL,idx,0);

  populate_drop_down_with_filters(hDlg);

  if (stricmp(curGfxDriver, "D3D9") == 0)
    SendDlgItemMessage(hDlg, IDC_GFXDRIVER, CB_SETCURSEL, 1, 0);
  else
    SendDlgItemMessage(hDlg, IDC_GFXDRIVER, CB_SETCURSEL, 0, 0);
  update_gfx_filter_box_enabled(hDlg);

  if (windowed > 0)
    SendDlgItemMessage(hDlg,IDC_WINDOWED,BM_SETCHECK,BST_CHECKED,0);

  if (refresh > 0)
    SendDlgItemMessage(hDlg,IDC_REFRESH,BM_SETCHECK,BST_CHECKED,0);

  if (antialias > 0)
    SendDlgItemMessage(hDlg,IDC_ANTIALIAS,BM_SETCHECK,BST_CHECKED,0);

  if (curusespeech > 0)
    SendDlgItemMessage(hDlg,IDC_SPEECHPACK,BM_SETCHECK,BST_CHECKED,0);

  if (sideBorders > 0)
    SendDlgItemMessage(hDlg, IDC_SIDEBORDERS, BM_SETCHECK, BST_CHECKED, 0);

  if (useletterbox > 0)
    SendDlgItemMessage(hDlg, IDC_LETTERBOX,BM_SETCHECK,BST_CHECKED,0);

  // If the game isn't 32-bit, disable it
  if (gameColDep < 32)
    EnableWindow (GetDlgItem (hDlg, IDC_REDUCESPR), FALSE);
  else if (reduce32to16 > 0)
    SendDlgItemMessage(hDlg, IDC_REDUCESPR, BM_SETCHECK, BST_CHECKED,0);

  // if no speech pack, disable the checkbox
  if (!File::TestReadFile("speech.vox"))
    EnableWindow (GetDlgItem (hDlg, IDC_SPEECHPACK), FALSE);

  if (INIreadint(*wincfg_in, "disabled", "speechvox") == 1)
    EnableWindow (GetDlgItem (hDlg, IDC_SPEECHPACK), FALSE);

  if (INIreadint(*wincfg_in, "disabled", "16bit") == 1)
    EnableWindow (GetDlgItem (hDlg, IDC_REDUCESPR), FALSE);

  if (INIreadint(*wincfg_in, "disabled", "filters") == 1)
    EnableWindow (GetDlgItem(hDlg, IDC_GFXFILTER), FALSE);

  RECT win_rect, client_rect;
  GetWindowRect (hDlg, &win_rect);
  wwidth = win_rect.right - win_rect.left;
  wheight = win_rect.bottom - win_rect.top;
  GetClientRect(hDlg, &client_rect);
  const int cl_height_diff = wheight - (client_rect.bottom - client_rect.top);

  RECT closed_dlg_unit_rect;
  ZeroMemory(&closed_dlg_unit_rect, sizeof(closed_dlg_unit_rect));
  closed_dlg_unit_rect.bottom = 121;
  MapDialogRect(hDlg, &closed_dlg_unit_rect);
  base_height = closed_dlg_unit_rect.bottom - closed_dlg_unit_rect.top + cl_height_diff;

  MoveWindow (hDlg, max(0, win_rect.left), max(0, win_rect.top + (wheight - base_height) / 2), wwidth, base_height, TRUE);
  update_resolution_texts (hDlg);

  SendMessage(hDlg, WM_SETTEXT, NULL, (LPARAM)gameNameForTitleBar.GetCStr());
  SendMessage(allegro_wnd, WM_SETTEXT, NULL, (LPARAM)gameNameForTitleBar.GetCStr());
}

// Browse-for-folder dialog callback
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        // Set initial selection
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);
    }
    return 0;
}

// Open common Browse-for-folder dialog
bool BrowseForFolder(char *dir_buf, int buf_max)
{
    bool res = false;
    CoInitialize(NULL);

    BROWSEINFO bi = { 0 };
    bi.lpszTitle = "Select location for game saves and custom data files";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)dir_buf;
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );
    if (pidl)
    {
        char path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path) != FALSE)
        {
            _snprintf(dir_buf, buf_max - 1, path);
            res = true;
        }
        CoTaskMemFree(pidl);
    }

    CoUninitialize();
    return res;
}

// handler for Settings dialog
LRESULT CALLBACK callback_settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  char tbuf[MAX_PATH];
  int idx = 1;
  //sprintf(tbuf,"event %d wp %d lp %d",message,wParam,lParam);
  switch (message) {
    case WM_INITDIALOG:
      
      InitializeDialog(hDlg);
      return TRUE;

    case WM_COMMAND:

      if (LOWORD(wParam) == IDC_ADVANCED) {
        // Reveal the advanced bit of the window
        RECT winsize;
        GetWindowRect (hDlg, &winsize);

        ShowWindow (GetDlgItem(hDlg, IDC_ADVANCED), SW_HIDE);
        ShowWindow (GetDlgItem(hDlg, IDC_STATICADV), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_STATICADV2), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_STATICADV3), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_COMBO1), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_COMBO2), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_SPEECHPACK), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_REFRESH), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_SIDEBORDERS), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_ANTIALIAS), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_REDUCESPR), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_LETTERBOX), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_COMBO4), SW_SHOW);

        MoveWindow (hDlg, max(0, winsize.left), max(0, winsize.top - (wheight - base_height) / 2), wwidth, wheight, TRUE);
        //SetActiveWindow (GetDlgItem(hDlg, IDOK));
        SetFocus (GetDlgItem(hDlg, IDOK));
      }
      else if ((LOWORD(wParam) == IDC_LETTERBOX) || (LOWORD(wParam) == IDC_GFXFILTER)) {
        update_resolution_texts(hDlg);
      }
      else if (LOWORD(wParam) == IDC_CUSTOMSAVEDIRCHECK)
      {
        bool custom_save_dir = SendDlgItemMessage(hDlg, IDC_CUSTOMSAVEDIRCHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;
        EnableWindow(GetDlgItem(hDlg, IDC_CUSTOMSAVEDIR), custom_save_dir ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_CUSTOMSAVEDIRBTN), custom_save_dir ? TRUE : FALSE);
      }
      else if (LOWORD(wParam) == IDC_CUSTOMSAVEDIRBTN)
      {
        char save_dir[MAX_PATH] = {0};
        SendDlgItemMessage(hDlg, IDC_CUSTOMSAVEDIR, WM_GETTEXT, MAX_PATH, (LPARAM)save_dir);
        if (BrowseForFolder(save_dir, MAX_PATH))
        {
          SendDlgItemMessage(hDlg, IDC_CUSTOMSAVEDIR, WM_SETTEXT, 0, (LPARAM)save_dir);
        }
      }

      if (LOWORD(wParam) == IDC_GFXDRIVER)
      {
        update_gfx_filter_box_enabled(hDlg);
      }
      else if (LOWORD(wParam) == IDC_GFXFILTER)
      {
        int curSel = SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_GETCURSEL, 0, 0);
        curFilter = get_filter_id(curSel);
      }

      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDOKRUN) {
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDOKRUN)) {

          sprintf(tbuf,"-1");
          INIwritestring(*wincfg_out, "sound","digiid",tbuf);
          INIwritestring(*wincfg_out, "sound","midiid",tbuf);
          sprintf(tbuf,"0");
          INIwritestring(*wincfg_out, "sound","digiindx",tbuf);
          INIwritestring(*wincfg_out, "sound","midiindx",tbuf);
          int idx=SendDlgItemMessage(hDlg,IDC_COMBO1,CB_GETCURSEL,0,0);
          sprintf(tbuf,"%d",idx);
          INIwritestring(*wincfg_out, "sound","digiwinindx",tbuf);
          if (idx == 0)
            idx = DIGI_DIRECTAMX(0);
          else if (idx == 1)
            idx = DIGI_WAVOUTID(0);
          else if (idx == 3) 
            idx = DIGI_DIRECTX(0);
          else idx = DIGI_NONE;
          sprintf(tbuf,"%d",idx);
          INIwritestring(*wincfg_out, "sound","digiwin",tbuf);

          idx=SendDlgItemMessage(hDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
          sprintf(tbuf,"%d",idx);
          INIwritestring(*wincfg_out, "sound","midiwinindx",tbuf);
          if (idx == 0)
            idx = MIDI_AUTODETECT;
          else if (idx == 1)
            idx = MIDI_NONE;
          else idx = MIDI_WIN32MAPPER;
          sprintf(tbuf,"%d",idx);
          INIwritestring(*wincfg_out, "sound","midiwin",tbuf);

          idx = SendDlgItemMessage(hDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
          if (idx > 0) {
            SendDlgItemMessage(hDlg,IDC_COMBO3,CB_GETLBTEXT,idx,(LPARAM)tbuf);
            // cut the end off
            strstr (tbuf, " translation")[0] = 0;
            curtranslation = tbuf;
          }
          else
            curtranslation.Empty();

          INIwritestring (*wincfg_out, "language", "translation",
              (idx == 0) ? NULL : curtranslation);

          idx = SendDlgItemMessage(hDlg,IDC_COMBO4,CB_GETCURSEL,0,0);
          SendDlgItemMessage(hDlg,IDC_COMBO4,CB_GETLBTEXT,idx,(LPARAM)tbuf);
          idx = atoi (tbuf);
          sprintf (tbuf, "%d", idx*1024);
          char *toWrite = &tbuf[0];
          // if it's the default, erase from config file
          if (idx == 20)
            toWrite = NULL;
          INIwritestring(*wincfg_out, "misc","cachemax", toWrite);

          if (SendDlgItemMessage(hDlg,IDC_SPEECHPACK,BM_GETCHECK,0,0) == BST_CHECKED)
            INIwritestring (*wincfg_out, "sound", "usespeech", "1");
          else
            INIwritestring (*wincfg_out, "sound", "usespeech", "0");

          if (SendDlgItemMessage(hDlg,IDC_WINDOWED,BM_GETCHECK,0,0) == BST_CHECKED)
            INIwritestring(*wincfg_out, "misc","windowed","1");
          else
            INIwritestring(*wincfg_out, "misc","windowed","0");

          if (SendDlgItemMessage(hDlg,IDC_ANTIALIAS,BM_GETCHECK,0,0) == BST_CHECKED)
            INIwritestring(*wincfg_out, "misc","antialias","1");
          else
            INIwritestring(*wincfg_out, "misc","antialias",NULL);

          if (SendDlgItemMessage(hDlg, IDC_SIDEBORDERS, BM_GETCHECK,0,0) == BST_CHECKED)
            INIwritestring(*wincfg_out, "misc","prefer_sideborders","1");
          else
            INIwritestring(*wincfg_out, "misc","prefer_sideborders","0");

          if (SendDlgItemMessage(hDlg,IDC_REFRESH,BM_GETCHECK,0,0) == BST_CHECKED)
            INIwritestring(*wincfg_out, "misc","refresh","85");
          else
            INIwritestring(*wincfg_out, "misc","refresh","0");

          if (SendDlgItemMessage(hDlg,IDC_LETTERBOX,BM_GETCHECK,0,0) == BST_CHECKED)
            INIwritestring (*wincfg_out, "misc", "prefer_letterbox", "1");
          else
            INIwritestring (*wincfg_out, "misc", "prefer_letterbox", "0");

          if (SendDlgItemMessage(hDlg, IDC_REDUCESPR, BM_GETCHECK,0,0) == BST_CHECKED)
            INIwritestring (*wincfg_out, "misc", "notruecolor", "1");
          else
            INIwritestring (*wincfg_out, "misc", "notruecolor", NULL);

          if (SendDlgItemMessage(hDlg, IDC_GFXDRIVER, CB_GETCURSEL,0,0) == 0)
            INIwritestring (*wincfg_out, "misc", "gfxdriver", "DX5");
          else
            INIwritestring (*wincfg_out, "misc", "gfxdriver", "D3D9");

          idx = SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_GETCURSEL, 0, 0);
          const char *filter_id = get_filter_id(idx);
          INIwritestring(*wincfg_out, "misc", "gfxfilter", filter_id);

          bool custom_save_dir = SendDlgItemMessage(hDlg, IDC_CUSTOMSAVEDIRCHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;
          if (custom_save_dir)
          {
              // Compare user path with the game data directory. If user chose
              // path pointing inside game's directory, then store relative
              // path instead; thus the path will keep pointing at game's
              // directory if user moves game elsewhere.
              char save_dir[MAX_PATH] = {0};
              SendDlgItemMessage(hDlg, IDC_CUSTOMSAVEDIR, WM_GETTEXT, MAX_PATH, (LPARAM)save_dir);
              char full_data_dir[MAX_PATH] = {0};
              char full_save_dir[MAX_PATH] = {0};
              MakeFullLongPath(curdatadir.GetCStr(), full_data_dir, MAX_PATH);
              MakeFullLongPath(save_dir, full_save_dir, MAX_PATH);
              char rel_save_dir[MAX_PATH] = {0};
              if (PathRelativePathTo(rel_save_dir, full_data_dir, FILE_ATTRIBUTE_DIRECTORY, full_save_dir, FILE_ATTRIBUTE_DIRECTORY) &&
                  strstr(rel_save_dir, "..") == NULL)
              {
                INIwritestring(*wincfg_out, "misc", "user_data_dir", rel_save_dir);
              }
              else
              {
                INIwritestring(*wincfg_out, "misc", "user_data_dir", save_dir);
              }
          }
          else
          {
              INIwritestring(*wincfg_out, "misc", "user_data_dir", "");
          }

          bool mouse_autolock = SendDlgItemMessage(hDlg, IDC_MOUSE_AUTOLOCK, BM_GETCHECK, 0, 0) == BST_CHECKED;
          INIwritestring(*wincfg_out, "mouse", "auto_lock", mouse_autolock ? "1" : "0");
          int slider_pos = SendDlgItemMessage(hDlg,IDC_MOUSESPEED, TBM_GETPOS, 0, 0);
          float mouse_speed = (float)slider_pos / 10.f;
          sprintf(tbuf, "%0.1f", mouse_speed);
          INIwritestring(*wincfg_out, "mouse", "speed", tbuf);
        }
	EndDialog(hDlg, LOWORD(wParam));
 	return TRUE;
      }
      return FALSE;
    case WM_HSCROLL:
      update_mouse_speed_text(hDlg);
      return TRUE;
    default: return FALSE;
    }
  return TRUE;
  }


SetupReturnValue acwsetup(const ConfigTree &cfg_in, ConfigTree &cfg_out,
                          const String &game_data_dir, const char*vername, const char*enbuild)
{
  curdatadir = game_data_dir;
  setupstring = vername;
  enginever = enbuild;
  wincfg_in  = &cfg_in;
  wincfg_out = &cfg_out;
  if (wincfg_in->size() > 0) {
    defaultRes = (GameResolutionType)INIreadint (*wincfg_in, "misc", "defaultres");
    if (defaultRes < 1)
      defaultRes = kGameResolution_Default;

    curdigi=INIreadint(*wincfg_in, "sound","digiwinindx");
    curmidi=INIreadint(*wincfg_in, "sound","midiwinindx");

    if (curdigi < 0) curdigi=0;
    if (curdigi > 3) curdigi = 0;
    if (curmidi < 0) curmidi=0;

    windowed = INIreadint(*wincfg_in, "misc","windowed");
    useletterbox = INIreadint(*wincfg_in, "misc","prefer_letterbox") != 0;

    reduce32to16 = INIreadint(*wincfg_in, "misc","notruecolor");
    if (reduce32to16 < 0)
      reduce32to16 = 0;

    gameColDep = INIreadint(*wincfg_in, "misc", "gamecolordepth");
    if (gameColDep < 0)
      gameColDep = 0;

    refresh = INIreadint(*wincfg_in, "misc","refresh");
    if (refresh < 1)
      refresh = 0;
    antialias = INIreadint(*wincfg_in, "misc","antialias");
    if (antialias < 1)
      antialias = 0;

    sideBorders = INIreadint(*wincfg_in, "misc","prefer_sideborders") != 0;

    int cacheval = INIreadint (*wincfg_in, "misc", "cachemax");
    if (cacheval > 0)
      curmaxcache = cacheval;
    curusespeech = INIreadint (*wincfg_in, "sound", "usespeech");
    if (curusespeech < 0)
      curusespeech = 1;

    curtranslation = INIreadstring(*wincfg_in, "language", "translation", "");
    curFilter = INIreadstring(*wincfg_in, "misc", "gfxfilter", "None");
    curGfxDriver = INIreadstring(*wincfg_in, "misc", "gfxdriver", "D3D9");
    gameNameForTitleBar = INIreadstring(*wincfg_in, "misc", "titletext", "Game Setup");
  }
  INT_PTR res = DialogBox(GetModuleHandle(NULL), (LPCTSTR)IDD_DIALOG1,allegro_wnd,(DLGPROC)callback_settings);
  if (res == IDOKRUN)
    return kSetup_RunGame;
  if (res == IDOK)
    return kSetup_Done;
  return kSetup_Cancel;
}

void set_icon() {
  SetClassLong(allegro_wnd,GCL_HICON,
    (LONG) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON2)) ); 
}

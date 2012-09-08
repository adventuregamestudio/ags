/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include "resource/resource.h"
#include "gfx/gfxfilter.h"
#include "util/filestream.h"

using AGS::Common::CDataStream;
using namespace AGS; // FIXME later

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

extern "C" HWND allegro_wnd;
extern int  INIreadint(const char*, const char*, int=0);
extern void fgetstring_limit (char *, CDataStream *, int);
extern char* ac_config_file;
const char*setupstring, *enginever;
int curscrn=-1,csendto,curdigi=0,curmidi=0,windowed=0,useletterbox = 0;
int defaultRes = -1, mustBeLetterbox = 0, gameColDep = 0;
int refresh = 0, antialias = 0, reduce32to16 = 0;
int sideBorders = 1;
char curtranslation[200] = "", curreplay[200] = "";
char curdatadir[MAX_PATH] = ".";
char curFilter[100] = "";
char curGfxDriver[100] = "";
char gameNameForTitleBar[200] = "";
int curmaxcache = 1024 * 20, curusespeech = 1;
int wwidth, wheight;
GFXFilter **filterList = NULL;

void update_resolution_texts(HWND hDlg) {
  int resx, resy;
  if (defaultRes >= 6) {
    resx = 1024;
    resy = 768;
  }
  else if (defaultRes == 5) {
    resx = 800;
    resy = 600;
  }
  else if ((SendDlgItemMessage(hDlg,IDC_LETTERBOX,BM_GETCHECK,0,0) == BST_CHECKED) ||
      (mustBeLetterbox != 0)) 
  {
    if (defaultRes >= 3)
    {
      resx = 640;
      resy = 480;
    }
    else
    {
      resx = 320;
      resy = 240;
    }
  }
  else if (defaultRes >= 3)
  {
    resx = 640;
    resy = 400;
  }
  else 
  {
    resx = 320;
    resy = 200;
  }
  
  int selectedFilter = SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_GETCURSEL, 0, 0);

  char buffer[100];
  int thisx, thisy;

  thisx = resx;
  thisy = resy;
  filterList[selectedFilter]->GetRealResolution(&thisx, &thisy);
  sprintf(buffer, "Resolution: %d""x%d", thisx, thisy);
  SendDlgItemMessage (hDlg, IDC_RESOLUTION, WM_SETTEXT, 0, (LPARAM)buffer);
}

void get_replay_name (const char *filename, char *namebuf) {
  CDataStream *in = Common::File::OpenFileRead(filename);
  if (in == NULL)
    return;

  char buffer [100];
  in->ReadArray (buffer, 12, 1);
  buffer[12] = 0;
  if (strcmp (buffer, "AGSRecording") != 0) {
    delete in;
    return;
  }

  fgetstring_limit (buffer, in, 12);
  if (buffer[0] != '2')  {
    // very first alpha version, can't play it
    delete in;
    return;
  }
  int replayver = in->ReadInt32();

  if ((replayver < 1) || (replayver > 3)) {
    delete in;
    return;
  }

  namebuf[0] = 0;
  if (strcmp (buffer, enginever))
    strcpy (namebuf, "!! ");

  if (replayver == 1) {
    // first version didn't save info
    strcat (namebuf, "(unknown replay)");
    delete in;
    return;
  }

  fgetstring_limit (buffer, in, 99);
  // skip the UID
  in->ReadInt32 ();
  strcat (namebuf, buffer);
  strcat (namebuf, " (");
  // skip the total time
  int tottime = in->ReadInt32 ();
  sprintf (&namebuf[strlen(namebuf)], "%02d:%02d:%02d)", tottime / 3600, (tottime % 3600) / 60, tottime % 60);
  // replay description, maybe we'll use this later
  fgetstring_limit (buffer, in, 99);

  if (buffer[0] == 0)
    strcpy (buffer, "No description");

  strcat (namebuf, ": ");
  strcat (namebuf, buffer);
  delete in;

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
  while (thisFilter != NULL) {

    if ((idx != 0) && (INIreadint("disabled", thisFilter->GetFilterID(), 0) == 1)) {
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

    if ((strcmp(curFilter, thisFilter->GetFilterID()) == 0) || (idx == 0))
      SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_SETCURSEL, idx, 0);

    idx++;
    thisFilter = filterList[idx];
  }
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
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"Disable music");
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"Win32 MIDI Mapper");
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_SETTOPINDEX,0,0);
  SendDlgItemMessage(hDlg,IDC_COMBO2,CB_SETCURSEL,curmidi,0);
  SendDlgItemMessage(hDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)"Game Default");
  SendDlgItemMessage(hDlg,IDC_COMBO3,CB_SETCURSEL,0,0);
  idx = 1;

  char translationFindSpec[255];
  sprintf(translationFindSpec, "%s\\*.tra", curdatadir);
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

  SendDlgItemMessage(hDlg,IDC_REPLAYBOX,CB_ADDSTRING,0,(LPARAM)"Don't run a replay");
  SendDlgItemMessage(hDlg,IDC_REPLAYBOX,CB_SETCURSEL,0,0);
  idx = 1;
  // Find and add all replays in folder
  if ((hFile = _findfirst( "*.agr", &c_file )) != -1L ) {
    do {
      if (c_file.name[strlen(c_file.name)-4] == '.') {
        // it actually returns *.agr* so make sure it's a .AGR file,
        // then add to our list
        char listentry[300] = "(unknown replay)";
        get_replay_name (c_file.name, listentry);
        strcat (listentry, " (File: ");
        strcat (listentry, c_file.name);
        strcat (listentry, ")");

        int needToSet = 0;
        // if this is the translation we're using, set it
        if (stricmp (curreplay, c_file.name) == 0)
          needToSet = 1;
        
        SendDlgItemMessage(hDlg,IDC_REPLAYBOX,CB_ADDSTRING,0,(LPARAM)listentry);

        if (needToSet)
          SendDlgItemMessage (hDlg, IDC_REPLAYBOX, CB_SETCURSEL, idx, 0);
        idx++;
      }
    } while( _findnext( hFile, &c_file ) == 0 );

    _findclose( hFile );
  }

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
  // If the game is 320x240, don't let them un-tick it
  if (mustBeLetterbox)
    EnableWindow (GetDlgItem (hDlg, IDC_LETTERBOX), FALSE);
  // If it's 800x600, hide the letterbox option
  if (defaultRes >= 5)
    ShowWindow (GetDlgItem (hDlg, IDC_LETTERBOX), SW_HIDE);

  // If the game isn't 32-bit, disable it
  if (gameColDep < 32)
    EnableWindow (GetDlgItem (hDlg, IDC_REDUCESPR), FALSE);
  else if (reduce32to16 > 0)
    SendDlgItemMessage(hDlg, IDC_REDUCESPR, BM_SETCHECK, BST_CHECKED,0);

  // if no speech pack, disable the checkbox
  if (!Common::File::TestReadFile("speech.vox"))
    EnableWindow (GetDlgItem (hDlg, IDC_SPEECHPACK), FALSE);

  if (INIreadint("disabled", "speechvox", 0) == 1)
    EnableWindow (GetDlgItem (hDlg, IDC_SPEECHPACK), FALSE);

  if (INIreadint("disabled", "16bit", 0) == 1)
    EnableWindow (GetDlgItem (hDlg, IDC_REDUCESPR), FALSE);

  if (INIreadint("disabled", "filters", 0) == 1)
    EnableWindow (GetDlgItem(hDlg, IDC_GFXFILTER), FALSE);

  RECT winsize;
  GetWindowRect (hDlg, &winsize);
  wwidth = winsize.right - winsize.left;
  wheight = winsize.bottom - winsize.top;

  MoveWindow (hDlg, winsize.left, winsize.top, wwidth, wheight-175, TRUE);
  update_resolution_texts (hDlg);

  SendMessage(hDlg, WM_SETTEXT, NULL, (LPARAM)gameNameForTitleBar);
  SendMessage(allegro_wnd, WM_SETTEXT, NULL, (LPARAM)gameNameForTitleBar);
}

// handler for Settings dialog
LRESULT CALLBACK callback_settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  char tbuf[200];
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
        ShowWindow (GetDlgItem(hDlg, IDC_COMBO4), SW_SHOW);

        MoveWindow (hDlg, winsize.left, winsize.top, wwidth, wheight, TRUE);
        //SetActiveWindow (GetDlgItem(hDlg, IDOK));
        SetFocus (GetDlgItem(hDlg, IDOK));
      }
      else if ((LOWORD(wParam) == IDC_LETTERBOX) || (LOWORD(wParam) == IDC_GFXFILTER)) {
        update_resolution_texts(hDlg);
      }

      if (LOWORD(wParam) == IDC_GFXDRIVER)
      {
        update_gfx_filter_box_enabled(hDlg);
      }
      else if (LOWORD(wParam) == IDC_GFXFILTER)
      {
        int curSel = SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_GETCURSEL, 0, 0);
        strcpy(curFilter, filterList[curSel]->GetFilterID());
      }

      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDOKRUN) {
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDOKRUN)) {

          csendto = 0;

          if (defaultRes > 2) 
          {
            csendto = 1;
          }

          sprintf(tbuf,"%d",csendto);
          if (!WritePrivateProfileString("misc","screenres",tbuf,ac_config_file)) 
          {
            DWORD errCode = GetLastError();
            sprintf(tbuf, "Unable to write to the configuration file (error code 0x%08X). If you are using Windows Vista, you may need to right-click and Run as Administrator on the Setup application.", errCode);
            MessageBox(hDlg, tbuf, "Save error", MB_OK | MB_ICONEXCLAMATION);
            return TRUE;
          }
          sprintf(tbuf,"-1");
          WritePrivateProfileString("sound","digiid",tbuf,ac_config_file);
          WritePrivateProfileString("sound","midiid",tbuf,ac_config_file);
          sprintf(tbuf,"0");
          WritePrivateProfileString("sound","digiindx",tbuf,ac_config_file);
          WritePrivateProfileString("sound","midiindx",tbuf,ac_config_file);
          int idx=SendDlgItemMessage(hDlg,IDC_COMBO1,CB_GETCURSEL,0,0);
          sprintf(tbuf,"%d",idx);
          WritePrivateProfileString("sound","digiwinindx",tbuf,ac_config_file);
          if (idx == 0)
            idx = DIGI_DIRECTAMX(0);
          else if (idx == 1)
            idx = DIGI_WAVOUTID(0);
          else if (idx == 3) 
            idx = DIGI_DIRECTX(0);
          else idx = DIGI_NONE;
          sprintf(tbuf,"%d",idx);
          WritePrivateProfileString("sound","digiwin",tbuf,ac_config_file);

          idx=SendDlgItemMessage(hDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
          sprintf(tbuf,"%d",idx);
          WritePrivateProfileString("sound","midiwinindx",tbuf,ac_config_file);
          if (idx == 0)
            idx = MIDI_AUTODETECT;
          else if (idx == 1)
            idx = MIDI_NONE;
          else idx = MIDI_WIN32MAPPER;
          sprintf(tbuf,"%d",idx);
          WritePrivateProfileString("sound","midiwin",tbuf,ac_config_file);

          idx = SendDlgItemMessage(hDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
          if (idx > 0) {
            SendDlgItemMessage(hDlg,IDC_COMBO3,CB_GETLBTEXT,idx,(LPARAM)curtranslation);
            // cut the end off
            strstr (curtranslation, " translation")[0] = 0;
          }
          else
            curreplay[0] = 0;

          WritePrivateProfileString ("language", "translation",
              (idx == 0) ? NULL : curtranslation, ac_config_file);

          idx = SendDlgItemMessage(hDlg,IDC_REPLAYBOX,CB_GETCURSEL,0,0);
          if (idx > 0) {
            SendDlgItemMessage(hDlg,IDC_REPLAYBOX,CB_GETLBTEXT,idx,(LPARAM)curreplay);
            // cut the end bracket off
            curreplay[strlen(curreplay)-1] = 0;
            // get the filename
            char *fnptr = &strstr (curreplay, "File: ")[6];
            memmove (curreplay, fnptr, strlen (fnptr) + 1);
          }
          else
            curtranslation[0] = 0;
          WritePrivateProfileString ("misc", "replay",
              (idx == 0) ? NULL : curreplay, ac_config_file);

          idx = SendDlgItemMessage(hDlg,IDC_COMBO4,CB_GETCURSEL,0,0);
          SendDlgItemMessage(hDlg,IDC_COMBO4,CB_GETLBTEXT,idx,(LPARAM)tbuf);
          idx = atoi (tbuf);
          sprintf (tbuf, "%d", idx*1024);
          char *toWrite = &tbuf[0];
          // if it's the default, erase from config file
          if (idx == 20)
            toWrite = NULL;
          WritePrivateProfileString("misc","cachemax", toWrite,ac_config_file);

          if (SendDlgItemMessage(hDlg,IDC_SPEECHPACK,BM_GETCHECK,0,0) == BST_CHECKED)
            WritePrivateProfileString ("sound", "usespeech", NULL, ac_config_file);
          else
            WritePrivateProfileString ("sound", "usespeech", "0", ac_config_file);

          if (SendDlgItemMessage(hDlg,IDC_WINDOWED,BM_GETCHECK,0,0) == BST_CHECKED)
            WritePrivateProfileString("misc","windowed","1",ac_config_file);
          else
            WritePrivateProfileString("misc","windowed","0",ac_config_file);

          if (SendDlgItemMessage(hDlg,IDC_ANTIALIAS,BM_GETCHECK,0,0) == BST_CHECKED)
            WritePrivateProfileString("misc","antialias","1",ac_config_file);
          else
            WritePrivateProfileString("misc","antialias",NULL,ac_config_file);

          if (SendDlgItemMessage(hDlg, IDC_SIDEBORDERS, BM_GETCHECK,0,0) == BST_CHECKED)
            WritePrivateProfileString("misc","sideborders",NULL,ac_config_file);
          else
            WritePrivateProfileString("misc","sideborders","0",ac_config_file);

          if (SendDlgItemMessage(hDlg,IDC_REFRESH,BM_GETCHECK,0,0) == BST_CHECKED)
            WritePrivateProfileString("misc","refresh","85",ac_config_file);
          else
            WritePrivateProfileString("misc","refresh","0",ac_config_file);

          if (mustBeLetterbox) ;
            // don't modify the Force Letterbox setting while it doesn't apply
          else if (SendDlgItemMessage(hDlg,IDC_LETTERBOX,BM_GETCHECK,0,0) == BST_CHECKED)
            WritePrivateProfileString ("misc", "forceletterbox", "1", ac_config_file);
          else
            WritePrivateProfileString ("misc", "forceletterbox", NULL, ac_config_file);

          if (SendDlgItemMessage(hDlg, IDC_REDUCESPR, BM_GETCHECK,0,0) == BST_CHECKED)
            WritePrivateProfileString ("misc", "notruecolor", "1", ac_config_file);
          else
            WritePrivateProfileString ("misc", "notruecolor", NULL, ac_config_file);

          if (SendDlgItemMessage(hDlg, IDC_GFXDRIVER, CB_GETCURSEL,0,0) == 0)
            WritePrivateProfileString ("misc", "gfxdriver", "DX5", ac_config_file);
          else
            WritePrivateProfileString ("misc", "gfxdriver", "D3D9", ac_config_file);

          idx = SendDlgItemMessage(hDlg, IDC_GFXFILTER, CB_GETCURSEL, 0, 0);
          WritePrivateProfileString("misc", "gfxfilter", filterList[idx]->GetFilterID(), ac_config_file);
        }
	EndDialog(hDlg, LOWORD(wParam));
 	return TRUE;
      }
      return FALSE;
    default: return FALSE;
    }
  return TRUE;
  }


int acwsetup(const char*vername, const char*enbuild) {
  setupstring = vername;
  enginever = enbuild;
  if (Common::File::TestReadFile(ac_config_file)) {
    curscrn=INIreadint("misc","screenres");
    if (curscrn > 1)
      curscrn = 1;
    defaultRes = INIreadint ("misc", "defaultres", 0);
    if (defaultRes < 1)
      defaultRes = -1;

    curdigi=INIreadint("sound","digiwinindx");
    curmidi=INIreadint("sound","midiwinindx");

    if (curdigi < 0) curdigi=0;
    if (curdigi > 3) curdigi = 0;
    if (curmidi < 0) curmidi=0;

    windowed = INIreadint("misc","windowed",0);
    useletterbox = INIreadint("misc","forceletterbox",0);

    reduce32to16 = INIreadint("misc","notruecolor",0);
    if (reduce32to16 < 0)
      reduce32to16 = 0;

    gameColDep = INIreadint("misc", "gamecolordepth", 0);
    if (gameColDep < 0)
      gameColDep = 0;

    if ((defaultRes == 2) || (defaultRes == 4)) {
      useletterbox = 1;
      mustBeLetterbox = 1;
    }

    refresh = INIreadint("misc","refresh",0);
    if (refresh < 1)
      refresh = 0;
    antialias = INIreadint("misc","antialias",0);
    if (antialias < 1)
      antialias = 0;

    sideBorders = INIreadint("misc","sideborders",0);
    if (sideBorders == -1)
      sideBorders = 1;

    int cacheval = INIreadint ("misc", "cachemax", 0);
    if (cacheval > 0)
      curmaxcache = cacheval;
    curusespeech = INIreadint ("sound", "usespeech", 0);
    if (curusespeech < 0)
      curusespeech = 1;

    GetPrivateProfileString("language", "translation", "", curtranslation, 200, ac_config_file);
    GetPrivateProfileString("misc", "replay", "", curreplay, 200, ac_config_file);
    GetPrivateProfileString("misc", "datadir", ".", curdatadir, 255, ac_config_file);
    GetPrivateProfileString("misc", "gfxfilter", "None", curFilter, 100, ac_config_file);
    GetPrivateProfileString("misc", "gfxdriver", "D3D9", curGfxDriver, 100, ac_config_file);
    GetPrivateProfileString("misc", "titletext", "Game Setup", gameNameForTitleBar, 200, ac_config_file);
  }
  if (DialogBox(GetModuleHandle(NULL), (LPCTSTR)IDD_DIALOG1,allegro_wnd,(DLGPROC)callback_settings) == IDOKRUN)
    return 1;
  return 0;
}

void set_icon() {
  SetClassLong(allegro_wnd,GCL_HICON,
    (LONG) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON2)) ); 
}

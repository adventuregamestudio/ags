/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#ifndef WINDOWS_VERSION
#error This file should only be included on the Windows build
#endif

// ********* WINDOWS *********

#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_display.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "main/engine.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"

extern GameSetupStruct game;
extern GameSetup usetup;
extern int our_eip;
extern IGraphicsDriver *gfxDriver;
extern color palette[256];
extern block virtual_screen;

#include <shlobj.h>
#include <time.h>
#include <shlwapi.h>
#ifdef VS2005
#include <rpcsal.h>
#endif
#include <gameux.h>

#include <libcda.h>


#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA        0x001C
#define CSIDL_COMMON_APPDATA       0x0023
#endif

typedef struct BMP_EXTRA_INFO {
   LPDIRECTDRAWSURFACE2 surf;
   struct BMP_EXTRA_INFO *next;
   struct BMP_EXTRA_INFO *prev;
   int flags;
   int lock_nesting;
} BMP_EXTRA_INFO;

// from Allegro DDraw driver
extern "C" extern LPDIRECTDRAW2 directdraw;
extern "C" extern LPDIRECTSOUND directsound;
extern "C" extern LPDIRECTINPUTDEVICE mouse_dinput_device;
extern "C" extern LPDIRECTINPUTDEVICE key_dinput_device;

char win32SavedGamesDirectory[MAX_PATH] = "\0";
char win32AppDataDirectory[MAX_PATH] = "\0";

extern "C" HWND allegro_wnd;
extern void dxmedia_abort_video();
extern void dxmedia_pause_video();
extern void dxmedia_resume_video();
extern char lastError[200];
extern int acwsetup(const char*, const char*);
extern void set_icon();
extern char* game_file_name;

struct AGSWin32 : AGSPlatformDriver {
  AGSWin32();

  virtual void AboutToQuitGame();
  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual const char *GetAllUsersDataDirectory();
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual const char* GetNoMouseErrorString();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual int  InitializeCDPlayer();
  virtual void PlayVideo(const char* name, int skip, int flags);
  virtual void PostAllegroInit(bool windowed);
  virtual void PostAllegroExit();
  virtual void ReplaceSpecialPaths(const char *sourcePath, char *destPath);
  virtual int  RunSetup();
  virtual void SetGameWindowIcon();
  virtual void ShutdownCDPlayer();
  virtual void WriteConsole(const char*, ...);
  virtual void WriteDebugString(const char*, ...);
  virtual void DisplaySwitchOut() ;
  virtual void DisplaySwitchIn() ;
  virtual void RegisterGameWithGameExplorer();
  virtual void UnRegisterGameWithGameExplorer();
  virtual int  ConvertKeycodeToScanCode(int keyCode);

  virtual void ReadPluginsFromDisk(FILE *);
  virtual void StartPlugins();
  virtual int  RunPluginHooks(int event, int data);
  virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
  virtual int  RunPluginDebugHooks(const char *scriptfile, int linenum);
  virtual void ShutdownPlugins();

private:
  void add_game_to_game_explorer(IGameExplorer* pFwGameExplorer, GUID *guid, const char *guidAsText, bool allUsers);
  void remove_game_from_game_explorer(IGameExplorer* pFwGameExplorer, GUID *guid, const char *guidAsText, bool allUsers);
  void add_tasks_for_game(const char *guidAsText, const char *gameEXE, const char *workingFolder, bool allUsers);
  void get_tasks_directory(char *directoryNameBuffer, const char *guidAsText, bool allUsers);
  void update_game_explorer(bool add);
  void create_shortcut(const char *pathToEXE, const char *workingFolder, const char *arguments, const char *shortcutPath);
  void register_file_extension(const char *exePath);
  void unregister_file_extension();
};

AGSWin32::AGSWin32() {
  allegro_wnd = NULL;
}

void check_parental_controls() {
  /* this doesn't work, it always just returns access depedning
     on whether unrated games are allowed because of digital signature
  BOOL bHasAccess = FALSE;
  IGameExplorer* pFwGameExplorer = NULL;

  CoInitialize(NULL);
  // Create an instance of the Game Explorer Interface
  HRESULT hr = CoCreateInstance( __uuidof(GameExplorer), NULL, CLSCTX_INPROC_SERVER, __uuidof(IGameExplorer), (void**)&pFwGameExplorer);
  if( FAILED(hr) || pFwGameExplorer == NULL ) {
    // not Vista, do nothing
  }
  else {
    char theexename[MAX_PATH] = "e:\\code\\ags\\acwin\\release\\acwin.exe";
    WCHAR wstrBinPath[MAX_PATH] = {0};
    MultiByteToWideChar(CP_ACP, 0, theexename, MAX_PATH, wstrBinPath, MAX_PATH);
    BSTR bstrGDFBinPath = SysAllocString(wstrBinPath);

    hr = pFwGameExplorer->VerifyAccess( bstrGDFBinPath, &bHasAccess );
    SysFreeString(bstrGDFBinPath);

    if( FAILED(hr) || !bHasAccess ) {
      char buff[300];
      sprintf(buff, "Parental controls block: %X  b: %d", hr, bHasAccess);
      quit(buff);
    }
    else {
      platform->DisplayAlert("Parental controls: Access granted.");
    }

  }

  if( pFwGameExplorer ) pFwGameExplorer->Release();
  CoUninitialize();
  */
}

void AGSWin32::create_shortcut(const char *pathToEXE, const char *workingFolder, const char *arguments, const char *shortcutPath)
{
  IShellLink* pShellLink = NULL;
  HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pShellLink);

  if ((SUCCEEDED(hr)) && (pShellLink != NULL))
  {
    IPersistFile *pPersistFile = NULL;
    if (FAILED(pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile)))
    {
      this->DisplayAlert("Unable to add game tasks: QueryInterface for IPersistFile failed");
      pShellLink->Release();
      return;
    }

    // Set the path to the shortcut target and add the description
    if (FAILED(pShellLink->SetPath(pathToEXE)))
    {
      this->DisplayAlert("Unable to add game tasks: SetPath failed");
    }
    else if (FAILED(pShellLink->SetWorkingDirectory(workingFolder)))
    {
      this->DisplayAlert("Unable to add game tasks: SetWorkingDirectory failed");
    }
    else if ((arguments != NULL) && (FAILED(pShellLink->SetArguments(arguments))))
    {
      this->DisplayAlert("Unable to add game tasks: SetArguments failed");
    }
    else
    {
      WCHAR wstrTemp[MAX_PATH] = {0};
      MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wstrTemp, MAX_PATH);

      if (FAILED(pPersistFile->Save(wstrTemp, TRUE)))
      {
        this->DisplayAlert("Unable to add game tasks: IPersistFile::Save failed");
      }
    }

    pPersistFile->Release();
  }

  if (pShellLink) pShellLink->Release();
}

void CopyStringAndRemoveInvalidFilenameChars(const char *source, char *destinationBuffer)
{
  int destIdx = 0;
  for (int i = 0; i < (int)strlen(source); i++)
  {
    if ((source[i] != '/') &&
        (source[i] != '\\') &&
        (source[i] != ':') &&
        (source[i] != '*') &&
        (source[i] != '?') &&
        (source[i] != '"') &&
        (source[i] != '<') &&
        (source[i] != '>') &&
        (source[i] != '|') &&
        (source[i] >= 32))
    {
      destinationBuffer[destIdx] = source[i];
      destIdx++;
    }
  }
  destinationBuffer[destIdx] = 0;
}

void AGSWin32::get_tasks_directory(char *pathBuffer, const char *guidAsText, bool allUsers)
{
  if (SHGetSpecialFolderPath(NULL, pathBuffer, allUsers ? CSIDL_COMMON_APPDATA : CSIDL_LOCAL_APPDATA, FALSE) == FALSE)
  {
    this->DisplayAlert("Unable to register game: SHGetSpecialFolderPath failed");
    return;
  }

  if (pathBuffer[strlen(pathBuffer) - 1] != '\\')
  {
    strcat(pathBuffer, "\\");
  }

  strcat(pathBuffer, "Microsoft\\Windows\\GameExplorer\\");
  strcat(pathBuffer, guidAsText);
  mkdir(pathBuffer);
  strcat(pathBuffer, "\\");
  strcat(pathBuffer, "PlayTasks");
  mkdir(pathBuffer);
}

void AGSWin32::add_tasks_for_game(const char *guidAsText, const char *gameEXE, const char *workingFolder, bool allUsers)
{
  char pathBuffer[MAX_PATH];
  get_tasks_directory(pathBuffer, guidAsText, allUsers);
  strcat(pathBuffer, "\\");
  strcat(pathBuffer, "0");
  mkdir(pathBuffer);

  // Remove any existing "Play.lnk" from a previous version
  char shortcutLocation[MAX_PATH];
  sprintf(shortcutLocation, "%s\\Play.lnk", pathBuffer);
  unlink(shortcutLocation);

  // Generate the shortcut file name (because it can appear on
  // the start menu's Recent area)
  char sanitisedGameName[MAX_PATH];
  CopyStringAndRemoveInvalidFilenameChars(game.gamename, sanitisedGameName);
  if (sanitisedGameName[0] == 0)
    strcpy(sanitisedGameName, "Play");
  sprintf(shortcutLocation, "%s\\%s.lnk", pathBuffer, sanitisedGameName);

  create_shortcut(gameEXE, workingFolder, NULL, shortcutLocation);

  pathBuffer[strlen(pathBuffer) - 1] = '1';
  mkdir(pathBuffer);

  sprintf(shortcutLocation, "%s\\Setup game.lnk", pathBuffer);
  create_shortcut(gameEXE, workingFolder, "--setup", shortcutLocation);
}

void AGSWin32::add_game_to_game_explorer(IGameExplorer* pFwGameExplorer, GUID *guid, const char *guidAsText, bool allUsers)
{
  WCHAR wstrTemp[MAX_PATH] = {0};
  bool hadError = false;

  char theexename[MAX_PATH];
  GetModuleFileName(NULL, theexename, MAX_PATH);

  MultiByteToWideChar(CP_ACP, 0, theexename, MAX_PATH, wstrTemp, MAX_PATH);
  BSTR bstrGDFBinPath = SysAllocString(wstrTemp);

  char gameDirectory[MAX_PATH];
  strcpy(gameDirectory, theexename);
  strrchr(gameDirectory, '\\')[0] = 0;

  MultiByteToWideChar(CP_ACP, 0, gameDirectory, MAX_PATH, wstrTemp, MAX_PATH);
  BSTR bstrGameDirectory = SysAllocString(wstrTemp);

  HRESULT hr = pFwGameExplorer->AddGame(bstrGDFBinPath, bstrGameDirectory, allUsers ? GIS_ALL_USERS : GIS_CURRENT_USER, guid);
  if ((FAILED(hr)) || (hr == S_FALSE))
  {
		if (hr == 0x80070715)
		{
      // No GDF XML -- do nothing. This means the game was compiled
      // without Game Explorer support.
			hadError = true;
		}
		else
		{
			// Game already exists or error
			HRESULT updateHr = pFwGameExplorer->UpdateGame(*guid);
			if (FAILED(updateHr))
			{
			  this->DisplayAlert("Failed to add the game to the game explorer: %08X, %08X", hr, updateHr);
        hadError = true;
			}
		}
  }
  else
  {
    add_tasks_for_game(guidAsText, theexename, gameDirectory, allUsers);
  }

  BOOL bHasAccess = FALSE;
  hr = pFwGameExplorer->VerifyAccess( bstrGDFBinPath, &bHasAccess );

  if (( FAILED(hr) || !bHasAccess ) && (!hadError))
  {
    this->DisplayAlert("Windows Parental Controls will not allow you to run this game.");
  }

  SysFreeString(bstrGDFBinPath);
  SysFreeString(bstrGameDirectory);
}

#define FA_SEARCH -1
void delete_files_in_directory(const char *directoryName, const char *fileMask)
{
  char srchBuffer[MAX_PATH];
  sprintf(srchBuffer, "%s\\%s", directoryName, fileMask);
  al_ffblk dfb;
  int	dun = al_findfirst(srchBuffer, &dfb, FA_SEARCH);
  while (!dun) {
    unlink(dfb.name);
    dun = al_findnext(&dfb);
  }
  al_findclose(&dfb);
}

void AGSWin32::remove_game_from_game_explorer(IGameExplorer* pFwGameExplorer, GUID *guid, const char *guidAsText, bool allUsers)
{
  HRESULT hr = pFwGameExplorer->RemoveGame(*guid);
  if (FAILED(hr))
  {
    this->DisplayAlert("Failed to un-register game: 0x%08X", hr);
  }
}

void AGSWin32::update_game_explorer(bool add)
{
  IGameExplorer* pFwGameExplorer = NULL;

  CoInitialize(NULL);
  // Create an instance of the Game Explorer Interface
  HRESULT hr = CoCreateInstance( __uuidof(GameExplorer), NULL, CLSCTX_INPROC_SERVER, __uuidof(IGameExplorer), (void**)&pFwGameExplorer);
  if( FAILED(hr) || pFwGameExplorer == NULL ) 
  {
    OutputDebugString("AGS: Game Explorer not found to register game, Windows Vista required");
  }
  else 
  {
    strupr(game.guid);
    WCHAR wstrTemp[MAX_PATH] = {0};
    GUID guid = GUID_NULL;
    MultiByteToWideChar(CP_ACP, 0, game.guid, MAX_GUID_LENGTH, wstrTemp, MAX_GUID_LENGTH);
    if (IIDFromString(wstrTemp, &guid) != S_OK)
    {
      this->DisplayAlert("Failed to register game: IIDFromString failed");
    }
    else if (add)
    {
      add_game_to_game_explorer(pFwGameExplorer, &guid, game.guid, true);
    }
    else
    {
      remove_game_from_game_explorer(pFwGameExplorer, &guid, game.guid, true);
    }
  }

  if( pFwGameExplorer ) pFwGameExplorer->Release();
  CoUninitialize();
}

void AGSWin32::unregister_file_extension()
{
  char keyname[MAX_PATH];
  sprintf(keyname, ".%s", game.saveGameFileExtension);
  if (SHDeleteKey(HKEY_CLASSES_ROOT, keyname) != ERROR_SUCCESS)
  {
    this->DisplayAlert("Unable to un-register the file extension. Make sure you are running this with admin rights.");
    return;
  }

  sprintf(keyname, "AGS.SaveGames.%s", game.saveGameFileExtension);
  SHDeleteKey(HKEY_CLASSES_ROOT, keyname);

  sprintf(keyname, "Software\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\.%s", game.saveGameFileExtension);
  SHDeleteKey(HKEY_LOCAL_MACHINE, keyname);

  // Tell Explorer to refresh its file association data
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

void AGSWin32::register_file_extension(const char *exePath)
{
  DWORD valType, valBufLen = MAX_PATH;
  valType = REG_SZ;
  char valBuf[MAX_PATH], keyname[MAX_PATH];
  char saveGameRegistryType[MAX_PATH];
  sprintf(saveGameRegistryType, "AGS.SaveGames.%s", game.saveGameFileExtension);

  // write HKEY_CLASSES_ROOT\.Extension = AGS.SaveGames.Extension
  strcpy(valBuf, saveGameRegistryType);
  sprintf(keyname, ".%s", game.saveGameFileExtension);
  if (RegSetValue(HKEY_CLASSES_ROOT, keyname, valType, valBuf, valBufLen))
  {
    this->DisplayAlert("Unable to register file type. Make sure you are running this with Administrator rights.");
    return;
  }

  // create HKEY_CLASSES_ROOT\AGS.SaveGames.Extension
  strcpy(keyname, saveGameRegistryType);
  sprintf(valBuf, "%s Saved Game", game.gamename);
  RegSetValue (HKEY_CLASSES_ROOT, keyname, REG_SZ, valBuf, strlen(valBuf));

  // write HKEY_CLASSES_ROOT\AGS.SaveGames.Extension\DefaultIcon
  sprintf(keyname, "%s\\DefaultIcon", saveGameRegistryType);
  sprintf(valBuf, "\"%s\", 0", exePath);
  RegSetValue (HKEY_CLASSES_ROOT, keyname, REG_SZ, valBuf, strlen(valBuf));

  // write HKEY_CLASSES_ROOT\AGS.SaveGames.Extension\Shell\Open\Command
  sprintf(keyname, "%s\\Shell\\Open\\Command", saveGameRegistryType);
  sprintf(valBuf, "\"%s\" -loadSavedGame \"%%1\"", exePath);
  RegSetValue (HKEY_CLASSES_ROOT, keyname, REG_SZ, valBuf, strlen(valBuf));

  // ** BELOW IS VISTA-ONLY

  // write HKEY_CLASSES_ROOT\AGS.SaveGames.Extension, PreviewTitle
  strcpy(keyname, saveGameRegistryType);
  strcpy(valBuf, "prop:System.Game.RichSaveName;System.Game.RichApplicationName");
  SHSetValue(HKEY_CLASSES_ROOT, keyname, "PreviewTitle", REG_SZ, valBuf, strlen(valBuf));

  // write HKEY_CLASSES_ROOT\AGS.SaveGames.Extension, PreviewDetails
  strcpy(keyname, saveGameRegistryType);
  strcpy(valBuf, "prop:System.Game.RichLevel;System.DateChanged;System.Game.RichComment;System.DisplayName;System.DisplayType");
  SHSetValue(HKEY_CLASSES_ROOT, keyname, "PreviewDetails", REG_SZ, valBuf, strlen(valBuf));

  // write HKEY_CLASSES_ROOT\.Extension\ShellEx\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}
  sprintf(keyname, ".%s\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}", game.saveGameFileExtension);
  strcpy(valBuf, "{4E5BFBF8-F59A-4E87-9805-1F9B42CC254A}");
  RegSetValue (HKEY_CLASSES_ROOT, keyname, REG_SZ, valBuf, strlen(valBuf));

  // write HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\PropertySystem\PropertyHandlers\.Extension
  sprintf(keyname, "Software\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\.%s", game.saveGameFileExtension);
  strcpy(valBuf, "{ECDD6472-2B9B-4B4B-AE36-F316DF3C8D60}");
  RegSetValue (HKEY_LOCAL_MACHINE, keyname, REG_SZ, valBuf, strlen(valBuf));

  // Tell Explorer to refresh its file association data
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

void AGSWin32::RegisterGameWithGameExplorer() 
{
  update_game_explorer(true);

  if (game.saveGameFileExtension[0] != 0)
  {
    char theexename[MAX_PATH];
    GetModuleFileName(NULL, theexename, MAX_PATH);

    register_file_extension(theexename);
  }
}

void AGSWin32::UnRegisterGameWithGameExplorer() 
{
  update_game_explorer(false);

  if (game.saveGameFileExtension[0] != 0)
  {
    unregister_file_extension();
  }
}

void AGSWin32::PostAllegroInit(bool windowed) 
{
  check_parental_controls();
}

typedef UINT (CALLBACK* Dynamic_SHGetKnownFolderPathType) (GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath); 
GUID FOLDERID_SAVEDGAMES = {0x4C5C32FF, 0xBB9D, 0x43b0, {0xB5, 0xB4, 0x2D, 0x72, 0xE5, 0x4E, 0xAA, 0xA4}}; 

void determine_app_data_folder()
{
  if (win32AppDataDirectory[0] != 0) 
  {
    //already worked it out
    return;
  }

  WCHAR unicodePath[MAX_PATH];
  WCHAR unicodeShortPath[MAX_PATH];
  SHGetSpecialFolderPathW(NULL, unicodePath, CSIDL_COMMON_APPDATA, FALSE);
  if (GetShortPathNameW(unicodePath, unicodeShortPath, MAX_PATH) == 0)
  {
    platform->DisplayAlert("Unable to get App Data dir: GetShortPathNameW failed");
    return;
  }
  WideCharToMultiByte(CP_ACP, 0, unicodeShortPath, -1, win32AppDataDirectory, MAX_PATH, NULL, NULL);

  strcat(win32AppDataDirectory, "\\Adventure Game Studio");
  mkdir(win32AppDataDirectory);
}

void determine_saved_games_folder()
{
  if (win32SavedGamesDirectory[0] != 0)
  {
    // already loaded
    return;
  }

  // Default to My Documents in case it's not Vista
  WCHAR unicodeSaveGameDir[MAX_PATH];
  WCHAR unicodeShortSaveGameDir[MAX_PATH];
  // workaround for case where My Documents path has unicode chars (eg.
  // with Russian Windows) -- so use Short File Name instead
  SHGetSpecialFolderPathW(NULL, unicodeSaveGameDir, CSIDL_PERSONAL, FALSE);
  if (GetShortPathNameW(unicodeSaveGameDir, unicodeShortSaveGameDir, MAX_PATH) == 0)
  {
    platform->DisplayAlert("Unable to get My Documents dir: GetShortPathNameW failed");
    return;
  }
  WideCharToMultiByte(CP_ACP, 0, unicodeShortSaveGameDir, -1, win32SavedGamesDirectory, MAX_PATH, NULL, NULL);
  strcat(win32SavedGamesDirectory, "\\My Saved Games");

  // Now see if we have a Vista "My Saved Games" folder
  HINSTANCE hShellDLL = NULL;
  Dynamic_SHGetKnownFolderPathType Dynamic_SHGetKnownFolderPath = NULL;

  hShellDLL = LoadLibrary("shell32.dll"); 

  Dynamic_SHGetKnownFolderPath = (Dynamic_SHGetKnownFolderPathType)GetProcAddress(hShellDLL, "SHGetKnownFolderPath");

  if (Dynamic_SHGetKnownFolderPath != NULL) 
  { 
    PWSTR path = NULL; 

    if (SUCCEEDED(Dynamic_SHGetKnownFolderPath(FOLDERID_SAVEDGAMES, 0, NULL, &path))) 
    { 
      GetShortPathNameW(path, unicodeShortSaveGameDir, MAX_PATH);
      WideCharToMultiByte(CP_ACP, 0, unicodeShortSaveGameDir, -1, win32SavedGamesDirectory, MAX_PATH, NULL, NULL ); 

      CoTaskMemFree(path);
    }
  }

  FreeLibrary(hShellDLL);
  // in case it's on XP My Documents\My Saved Games, create this part of the path
  mkdir(win32SavedGamesDirectory);
}

void AGSWin32::ReplaceSpecialPaths(const char *sourcePath, char *destPath) {

  determine_saved_games_folder();

  if (strnicmp(sourcePath, "$MYDOCS$", 8) == 0) 
  {
    strcpy(destPath, win32SavedGamesDirectory);
    strcat(destPath, &sourcePath[8]);
  }
  else if (strnicmp(sourcePath, "$APPDATADIR$", 12) == 0) 
  {
    determine_app_data_folder();
    strcpy(destPath, win32AppDataDirectory);
    strcat(destPath, &sourcePath[12]);
  }
  else {
    strcpy(destPath, sourcePath);
  }

}

const char* AGSWin32::GetAllUsersDataDirectory() 
{
  determine_app_data_folder();
  return &win32AppDataDirectory[0];
}

void AGSWin32::DisplaySwitchOut() {
  dxmedia_pause_video();
}

void AGSWin32::DisplaySwitchIn() {
  dxmedia_resume_video();
}

int AGSWin32::CDPlayerCommand(int cmdd, int datt) {
  return cd_player_control(cmdd, datt);
}

void AGSWin32::DisplayAlert(const char *text, ...) {
  char displbuf[2500];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  MessageBox(allegro_wnd, displbuf, "Adventure Game Studio", MB_OK | MB_ICONEXCLAMATION);
}

void AGSWin32::Delay(int millis) 
{
  while (millis >= 5)
  {
    Sleep(5);
    millis -= 5;
    // don't allow it to check for debug messages, since this Delay()
    // call might be from within a debugger polling loop
    update_polled_stuff(false);
  }

  if (millis > 0)
    Sleep(millis);
}

unsigned long AGSWin32::GetDiskFreeSpaceMB() {
  DWORD returnMb = 0;
  BOOL fResult;
  our_eip = -1891;

  // On Win9x, the last 3 params cannot be null, so need to supply values for all
  __int64 i64FreeBytesToCaller, i64Unused1, i64Unused2;

  // Win95 OSR2 or higher - use GetDiskFreeSpaceEx, since the
  // normal GetDiskFreeSpace returns erroneous values if the
  // free space is > 2 GB
  fResult = GetDiskFreeSpaceEx(NULL,
             (PULARGE_INTEGER)&i64FreeBytesToCaller,
             (PULARGE_INTEGER)&i64Unused1,
             (PULARGE_INTEGER)&i64Unused2);

  our_eip = -1893;

  // convert down to MB so we can fit it in a 32-bit long
  i64FreeBytesToCaller /= 1000000;
  returnMb = i64FreeBytesToCaller;

  return returnMb;
}

const char* AGSWin32::GetNoMouseErrorString() {
  return "No mouse was detected on your system, or your mouse is not configured to work with DirectInput. You must have a mouse to play this game.";
}

eScriptSystemOSID AGSWin32::GetSystemOSID() {
  return eOS_Win;
}

int AGSWin32::InitializeCDPlayer() {
  return cd_player_init();
}

void AGSWin32::PlayVideo(const char *name, int skip, int flags) {

  char useloc[250];
  sprintf(useloc,"%s\\%s",usetup.data_files_dir, name);

  bool useSound = true;
  if (flags >= 10) {
    flags -= 10;
    useSound = false;
  }
  else {
    // for some reason DirectSound can't be shared, so uninstall
    // allegro sound before playing the video
    shutdown_sound();
  }

  bool isError = false;
  FILE *testFile = fopen(useloc, "rb");
  if (testFile != NULL)
  {
    fclose(testFile);
    isError = (gfxDriver->PlayVideo(useloc, useSound, (VideoSkipType)skip, (flags > 0)) == 0);
  }
  else
  {
    isError = true;
    sprintf(lastError, "File not found: %s", useloc);
  }
  
  if (isError) {
    // turn "Always display as speech" off, to make sure error
    // gets displayed correctly
    int oldalways = game.options[OPT_ALWAYSSPCH];
    game.options[OPT_ALWAYSSPCH] = 0;
    Display("Video playing error: %s", lastError);
    game.options[OPT_ALWAYSSPCH] = oldalways;
  }

  if (useSound)
  {
    if (opts.mod_player)
      reserve_voices(NUM_DIGI_VOICES, -1);
    install_sound(usetup.digicard,usetup.midicard,NULL);
    if (opts.mod_player)
      init_mod_player(NUM_MOD_DIGI_VOICES);
  }

  wsetpalette (0, 255, palette);
}

void AGSWin32::AboutToQuitGame() 
{
  dxmedia_abort_video();
}

void AGSWin32::PostAllegroExit() {
  allegro_wnd = NULL;
}

int AGSWin32::RunSetup() {
  const char *engineVersion = get_engine_version();
  char titleBuffer[200];
  sprintf(titleBuffer, "Adventure Game Studio v%s setup", engineVersion);
  return acwsetup(titleBuffer, engineVersion);
}

void AGSWin32::SetGameWindowIcon() {
  set_icon();
}

void AGSWin32::WriteConsole(const char *text, ...) {
  // Do nothing (Windows GUI app)
}

void AGSWin32::WriteDebugString(const char* texx, ...) {
  char displbuf[STD_BUFFER_SIZE] = "AGS: ";
  va_list ap;
  va_start(ap,texx);
  vsprintf(&displbuf[5],texx,ap);
  va_end(ap);
  strcat(displbuf, "\n");

  OutputDebugString(displbuf);
}

void AGSWin32::ShutdownCDPlayer() {
  cd_exit();
}

void AGSWin32::ReadPluginsFromDisk(FILE *iii) {
  pl_read_plugins_from_disk(iii);
}

void AGSWin32::StartPlugins() {
  pl_startup_plugins();
}

void AGSWin32::ShutdownPlugins() {
  pl_stop_plugins();
}

int AGSWin32::RunPluginHooks(int event, int data) {
  return pl_run_plugin_hooks(event, data);
}

void AGSWin32::RunPluginInitGfxHooks(const char *driverName, void *data) {
  pl_run_plugin_init_gfx_hooks(driverName, data);
}

int AGSWin32::RunPluginDebugHooks(const char *scriptfile, int linenum) {
  return pl_run_plugin_debug_hooks(scriptfile, linenum);
}

extern "C" const unsigned char hw_to_mycode[256];
#ifndef VS2005
#define MAPVK_VK_TO_VSC 0
#endif

int AGSWin32::ConvertKeycodeToScanCode(int keycode)
{
  // ** HIDEOUS HACK TO WORK AROUND ALLEGRO BUG
  // the key[] array is hardcoded to qwerty keyboards, so we
  // have to re-map it to make it work on other keyboard layouts
  keycode += ('a' - 'A');
  int vkey = VkKeyScan(keycode);
  int scancode = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
  if ((scancode >= 0) && (scancode < 256))
    keycode = hw_to_mycode[scancode];
  return keycode;
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSWin32();
  return instance;
}


// *********** WINDOWS-SPECIFIC PLUGIN API FUNCTIONS *************

HWND IAGSEngine::GetWindowHandle () {
  return allegro_wnd;
}
LPDIRECTDRAW2 IAGSEngine::GetDirectDraw2 () {
  if (directdraw == NULL)
    quit("!This plugin is not compatible with the Direct3D driver.");

  return directdraw;
}
LPDIRECTDRAWSURFACE2 IAGSEngine::GetBitmapSurface (BITMAP *bmp) 
{
  if (directdraw == NULL)
    quit("!This plugin is not compatible with the Direct3D driver.");

  BMP_EXTRA_INFO *bei = (BMP_EXTRA_INFO*)bmp->extra;

  if (bmp == virtual_screen)
    invalidate_screen();

  return bei->surf;
  //return get_bitmap_surface2 (bmp);
}

LPDIRECTSOUND IAGSEngine::GetDirectSound() {
  return directsound;
}

LPDIRECTINPUTDEVICE IAGSEngine::GetDirectInputKeyboard() {
  return key_dinput_device;
}

LPDIRECTINPUTDEVICE IAGSEngine::GetDirectInputMouse() {
  return mouse_dinput_device;
}

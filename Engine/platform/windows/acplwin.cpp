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

#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#include <direct.h>
#include <string.h>
#include "platform/windows/windows.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <gameux.h>
#include <libcda.h>

#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_display.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "debug/out.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "main/engine.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "platform/windows/setup/winsetup.h"
#include "plugin/agsplugin.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/stream.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameSetup usetup;
extern int our_eip;
extern IGraphicsDriver *gfxDriver;
extern RGB palette[256];


#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA        0x001C
#define CSIDL_COMMON_APPDATA       0x0023
#endif

String win32SavedGamesDirectory;
String win32AppDataDirectory;
String win32OutputDirectory;

const unsigned int win32TimerPeriod = 1;

struct AGSWin32 : AGSPlatformDriver {
  AGSWin32();
  ~AGSWin32();

  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void AttachToParentConsole();
  virtual void DisplayAlert(const char*, ...);
  virtual int  GetLastSystemError();
  virtual const char *GetAllUsersDataDirectory();
  virtual const char *GetUserSavedgamesDirectory();
  virtual const char *GetUserConfigDirectory();
  virtual const char *GetUserGlobalConfigDirectory();
  virtual const char *GetAppOutputDirectory();
  virtual const char *GetIllegalFileChars();
  virtual const char *GetGraphicsTroubleshootingText();
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual const char* GetBackendFailUserHint();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual int  InitializeCDPlayer();
  virtual void PostBackendInit();
  virtual void PostBackendExit();
  virtual SetupReturnValue RunSetup(const ConfigTree &cfg_in, ConfigTree &cfg_out);
  virtual void ShutdownCDPlayer();
  virtual void WriteStdOut(const char *fmt, ...);
  virtual void WriteStdErr(const char *fmt, ...);
  virtual void PauseApplication();
  virtual void ResumeApplication();
  virtual void RegisterGameWithGameExplorer();
  virtual void UnRegisterGameWithGameExplorer();
  virtual void ValidateWindowSize(int &x, int &y, bool borderless) const;

  // Returns command line argument in a UTF-8 format
  String GetCommandArg(size_t arg_index) override;

private:
  void add_game_to_game_explorer(IGameExplorer* pFwGameExplorer, GUID *guid, const char *guidAsText, bool allUsers);
  void remove_game_from_game_explorer(IGameExplorer* pFwGameExplorer, GUID *guid, const char *guidAsText, bool allUsers);
  void add_tasks_for_game(const char *guidAsText, const char *gameEXE, const char *workingFolder, bool allUsers);
  void get_tasks_directory(char *directoryNameBuffer, const char *guidAsText, bool allUsers);
  void update_game_explorer(bool add);
  void create_shortcut(const char *pathToEXE, const char *workingFolder, const char *arguments, const char *shortcutPath);
  void register_file_extension(const char *exePath);
  void unregister_file_extension();

  bool _isDebuggerPresent; // indicates if the win app is running in the context of a debugger
  bool _isAttachedToParentConsole; // indicates if the win app is attached to the parent console
};

AGSWin32::AGSWin32() :
    _isDebuggerPresent(::IsDebuggerPresent() != FALSE),
    _isAttachedToParentConsole(false)
{
    // Do nothing.
}

AGSWin32::~AGSWin32() {
    if (_isAttachedToParentConsole)
    {
        ::FreeConsole();
    }
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
  File::DeleteFile(shortcutLocation);

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
    Debug::Printf(kDbgMsg_Warn, "Game Explorer not found to register game, Windows Vista required");
  }
  else 
  {
    ags_strupr(game.guid);
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

void AGSWin32::PostBackendInit() 
{
  // Set the Windows timer resolution to 1 ms so that calls to
  // Sleep() don't take more time than specified
  MMRESULT result = timeBeginPeriod(win32TimerPeriod);
  if (result != TIMERR_NOERROR)
    Debug::Printf(kDbgMsg_Error, "Failed to set the timer resolution to %d ms", win32TimerPeriod);
}

typedef UINT (CALLBACK* Dynamic_SHGetKnownFolderPathType) (GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath); 
GUID FOLDERID_SAVEDGAMES = {0x4C5C32FF, 0xBB9D, 0x43b0, {0xB5, 0xB4, 0x2D, 0x72, 0xE5, 0x4E, 0xAA, 0xA4}}; 
#define _WIN32_WINNT_VISTA              0x0600
#define VER_MINORVERSION                0x0000001
#define VER_MAJORVERSION                0x0000002
#define VER_SERVICEPACKMAJOR            0x0000020
#define VER_GREATER_EQUAL               3

// These helpers copied from VersionHelpers.h in the Windows 8.1 SDK
bool IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
  OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0,{ 0 }, 0, 0 };
  DWORDLONG        const dwlConditionMask = VerSetConditionMask(
    VerSetConditionMask(
      VerSetConditionMask(
        0, VER_MAJORVERSION, VER_GREATER_EQUAL),
      VER_MINORVERSION, VER_GREATER_EQUAL),
    VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

  osvi.dwMajorVersion = wMajorVersion;
  osvi.dwMinorVersion = wMinorVersion;
  osvi.wServicePackMajor = wServicePackMajor;

  return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}

bool IsWindowsVistaOrGreater() {
  return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0);
}

void determine_app_data_folder()
{
  if (!win32AppDataDirectory.IsEmpty()) 
  {
    // already discovered
    return;
  }

  WCHAR unicodePath[MAX_PATH_SZ];
  SHGetSpecialFolderPathW(NULL, unicodePath, CSIDL_COMMON_APPDATA, FALSE);
  win32AppDataDirectory = Path::WidePathToUTF8(unicodePath);
  win32AppDataDirectory = Path::ConcatPaths(win32AppDataDirectory, "Adventure Game Studio");
  Directory::CreateDirectory(win32AppDataDirectory);
}

void determine_saved_games_folder()
{
  if (!win32SavedGamesDirectory.IsEmpty())
  {
    // already discovered
    return;
  }

  if (IsWindowsVistaOrGreater())
  {
    HINSTANCE hShellDLL = LoadLibrary("shell32.dll");
    Dynamic_SHGetKnownFolderPathType Dynamic_SHGetKnownFolderPath = (Dynamic_SHGetKnownFolderPathType)GetProcAddress(hShellDLL, "SHGetKnownFolderPath");

    if (Dynamic_SHGetKnownFolderPath != NULL)
    {
      PWSTR path = NULL;
      if (SUCCEEDED(Dynamic_SHGetKnownFolderPath(FOLDERID_SAVEDGAMES, 0, NULL, &path)))
      {
        win32SavedGamesDirectory = Path::WidePathToUTF8(path);
        CoTaskMemFree(path);
      }
    }

    FreeLibrary(hShellDLL);
  }
  else
  {
    WCHAR unicodeSaveGameDir[MAX_PATH_SZ] = L"";
    // Windows XP didn't have a "My Saved Games" folder, so create one under "My Documents"
    SHGetSpecialFolderPathW(NULL, unicodeSaveGameDir, CSIDL_PERSONAL, FALSE);
    win32SavedGamesDirectory = Path::WidePathToUTF8(unicodeSaveGameDir);
    win32SavedGamesDirectory = Path::ConcatPaths(win32SavedGamesDirectory, "My Saved Games");
    Directory::CreateDirectory(win32SavedGamesDirectory);
  }

  // Fallback to a subdirectory of the app data directory
  if (win32SavedGamesDirectory.IsEmpty())
  {
    determine_app_data_folder();
    win32SavedGamesDirectory = win32AppDataDirectory;
    win32SavedGamesDirectory = Path::ConcatPaths(win32SavedGamesDirectory, "Saved Games");
    Directory::CreateDirectory(win32SavedGamesDirectory);
  }
}

void DetermineAppOutputDirectory()
{
  if (!win32OutputDirectory.IsEmpty())
  {
    return;
  }

  determine_saved_games_folder();
  bool log_to_saves_dir = false;
  if (!win32SavedGamesDirectory.IsEmpty())
  {
    win32OutputDirectory = Path::ConcatPaths(win32SavedGamesDirectory, "Adventure Game Studio");
    log_to_saves_dir = Directory::CreateDirectory(win32OutputDirectory.GetCStr());
  }

  if (!log_to_saves_dir)
  {
    WCHAR theexename[MAX_PATH_SZ];
    GetModuleFileNameW(NULL, theexename, MAX_PATH_SZ);
    PathRemoveFileSpecW(theexename);
    win32OutputDirectory = Path::WidePathToUTF8(theexename);
  }
}

const char* AGSWin32::GetAllUsersDataDirectory() 
{
  determine_app_data_folder();
  return win32AppDataDirectory.GetCStr();
}

const char *AGSWin32::GetUserSavedgamesDirectory()
{
  determine_saved_games_folder();
  return win32SavedGamesDirectory.GetCStr();
}

const char *AGSWin32::GetUserConfigDirectory()
{
  determine_saved_games_folder();
  return win32SavedGamesDirectory.GetCStr();
}

const char *AGSWin32::GetUserGlobalConfigDirectory()
{
  DetermineAppOutputDirectory();
  return win32OutputDirectory.GetCStr();
}

const char *AGSWin32::GetAppOutputDirectory()
{
  DetermineAppOutputDirectory();
  return win32OutputDirectory.GetCStr();
}

const char *AGSWin32::GetIllegalFileChars()
{
    return "\\/:?\"<>|*";
}

const char *AGSWin32::GetGraphicsTroubleshootingText()
{
  return "\n\nPossible causes:\n"
    "* your graphics card drivers do not support requested resolution. "
    "Run the game setup program and try another resolution.\n"
    "* the graphics driver you have selected does not work. Try switching to another graphics driver.\n"
    "* the graphics filter you have selected does not work. Try another filter.\n"
    "* your graphics card drivers are out of date. "
    "Try downloading updated graphics card drivers from your manufacturer's website.\n"
    "* there is a problem with your graphics card driver configuration. "
    "Run DXDiag using the Run command (Start->Run, type \"dxdiag.exe\") and correct any problems reported there.";
}

void AGSWin32::PauseApplication()
{
}

void AGSWin32::ResumeApplication()
{
}

int AGSWin32::CDPlayerCommand(int cmdd, int datt) {
#if defined (AGS_HAS_CD_AUDIO)
  return cd_player_control(cmdd, datt);
#else
  return -1;
#endif
}

void AGSWin32::AttachToParentConsole() {
    if (_isAttachedToParentConsole)
        return;

    _isAttachedToParentConsole = ::AttachConsole(ATTACH_PARENT_PROCESS) != FALSE;
    if (_isAttachedToParentConsole)
    {
        // Require that both STDOUT and STDERR are valid handles from the parent process.
        if (::GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE &&
            ::GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
        {
            // Re-open STDOUT and STDERR to the parent's.
            FILE* fp = NULL;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            setvbuf(stdout, NULL, _IONBF, 0);

            freopen_s(&fp, "CONOUT$", "w", stderr);
            setvbuf(stderr, NULL, _IONBF, 0);
        }
        else
        {
            ::FreeConsole();
            _isAttachedToParentConsole = false;
        }
    }
}

void AGSWin32::DisplayAlert(const char *text, ...) {
  char displbuf[2500];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  if (_guiMode)
    MessageBox((HWND)sys_win_get_window(), displbuf, "Adventure Game Studio", MB_OK | MB_ICONEXCLAMATION);

  // Always write to either stderr or stdout, even if message boxes are enabled.
  if (_logToStdErr)
    AGSWin32::WriteStdErr("%s", displbuf);
  else
    AGSWin32::WriteStdOut("%s", displbuf);
}

int AGSWin32::GetLastSystemError()
{
  return ::GetLastError();
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

const char* AGSWin32::GetBackendFailUserHint()
{
  return "Make sure you have DirectX 5 or above installed.";
}

eScriptSystemOSID AGSWin32::GetSystemOSID() {
  return eOS_Win;
}

int AGSWin32::InitializeCDPlayer() {
#if defined (AGS_HAS_CD_AUDIO)
  return cd_player_init();
#else
  return -1;
#endif
}

void AGSWin32::PostBackendExit() {
  // Release the timer setting
  timeEndPeriod(win32TimerPeriod);
}

SetupReturnValue AGSWin32::RunSetup(const ConfigTree &cfg_in, ConfigTree &cfg_out)
{
  String version_str = String::FromFormat("Adventure Game Studio v%s setup", get_engine_version());
  return AGS::Engine::WinSetup(cfg_in, cfg_out, usetup.main_data_dir, version_str);
}

void AGSWin32::WriteStdOut(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (_isDebuggerPresent)
  {
    // Add "AGS:" prefix when outputting to debugger, to make it clear that this
    // is a text from the program log
    char buf[STD_BUFFER_SIZE] = "AGS: ";
    vsnprintf(buf + 5, STD_BUFFER_SIZE - 5, fmt, ap);
    OutputDebugString(buf);
    OutputDebugString("\n");
  }
  else
  {
    vprintf(fmt, ap);
    printf("\n");
  }
  va_end(ap);
}

void AGSWin32::WriteStdErr(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (_isDebuggerPresent)
  {
    // Add "AGS:" prefix when outputting to debugger, to make it clear that this
    // is a text from the program log
    char buf[STD_BUFFER_SIZE] = "AGS ERR: ";
    vsnprintf(buf + 9, STD_BUFFER_SIZE - 9, fmt, ap);
    OutputDebugString(buf);
    OutputDebugString("\n");
  }
  else
  {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
  }
  va_end(ap);
}

void AGSWin32::ShutdownCDPlayer() {
  cd_exit();
}

void AGSWin32::ValidateWindowSize(int &x, int &y, bool borderless) const
{
    RECT wa_rc, nc_rc;
    // This is the size of the available workspace on user's desktop
    SystemParametersInfo(SPI_GETWORKAREA, 0, &wa_rc, 0);
    // This is the maximal size that OS can reliably resize the window to (including any frame)
    const Size max_win(GetSystemMetrics(SM_CXMAXTRACK), GetSystemMetrics(SM_CYMAXTRACK));
    // This is the size of window's non-client area (frame, caption, etc)
    HWND allegro_wnd = (HWND)sys_win_get_window();
    LONG winstyle = borderless ? WS_POPUP : WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
    LONG winstyle_al = GetWindowLong(allegro_wnd, GWL_STYLE);
    SetRectEmpty(&nc_rc);
    AdjustWindowRect(&nc_rc, winstyle, FALSE);
    // Limit the window's full size to the system's window size limit,
    // and limit window's client size to the work space (visible area)
    x = Math::Min(x, (int)(max_win.Width - (nc_rc.right - nc_rc.left)));
    y = Math::Min(y, (int)(max_win.Height - (nc_rc.bottom - nc_rc.top)));
    x = Math::Clamp(x, 1, (int)(wa_rc.right - wa_rc.left));
    y = Math::Clamp(y, 1, (int)(wa_rc.bottom - wa_rc.top));
}

String AGSWin32::GetCommandArg(size_t arg_index)
{
    // On MS Windows the regular cmdargs are represented in ASCII,
    // therefore we must retrieve widechar variants using WinAPI
    int wargc;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (!wargv)
        return nullptr;
    String arg;
    if (arg_index <= (size_t)wargc)
        arg = Path::WidePathToUTF8(wargv[arg_index]);
    LocalFree(wargv);
    return arg;
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSWin32();
}


// *********** WINDOWS-SPECIFIC PLUGIN API FUNCTIONS *************

HWND IAGSEngine::GetWindowHandle () {
  return (HWND)sys_win_get_window();
}
LPDIRECTDRAW2 IAGSEngine::GetDirectDraw2 () {
  quit("!IAGSEngine::GetDirectDraw2() is deprecated and not supported anymore.");
  return nullptr;
}
LPDIRECTDRAWSURFACE2 IAGSEngine::GetBitmapSurface (BITMAP *bmp) 
{
  quit("!IAGSEngine::GetBitmapSurface() is deprecated and not supported anymore.");
  return nullptr;
}

LPDIRECTSOUND IAGSEngine::GetDirectSound() {
  quit("!IAGSEngine::GetDirectSound() is deprecated and not supported anymore.");
  return nullptr;
}

LPDIRECTINPUTDEVICE IAGSEngine::GetDirectInputKeyboard() {
  quit("!IAGSEngine::GetDirectInputKeyboard() is deprecated and not supported anymore.");
  return nullptr;
}

LPDIRECTINPUTDEVICE IAGSEngine::GetDirectInputMouse() {
  quit("!IAGSEngine::GetDirectInputMouse() is deprecated and not supported anymore.");
  return nullptr;
}

#endif

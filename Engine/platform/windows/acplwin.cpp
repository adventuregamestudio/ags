//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#include <direct.h>
#include <string.h>
#include "platform/windows/windows.h"
#include <shlobj.h>
#include <shlwapi.h>

#include "platform/base/agsplatformdriver.h"
#include "ac/common.h" // quit
#include "ac/gamesetup.h"
#include "main/engine.h"
#include "platform/base/sys_main.h"
#include "platform/windows/setup/winsetup.h"
struct BITMAP; // we need it only as a placeholder here
#include "plugin/agsplugin.h"
#include "resource/resource.h"
#include "util/stdio_compat.h"


using namespace AGS::Common;
using namespace AGS::Engine;


#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA        0x001C
#define CSIDL_COMMON_APPDATA       0x0023
#endif

FSLocation win32SavedGamesDirectory;
FSLocation win32AppDataDirectory;
FSLocation win32OutputDirectory;

const unsigned int win32TimerPeriod = 1;

struct AGSWin32 : AGSPlatformDriver {
  AGSWin32();
  ~AGSWin32();

  void AttachToParentConsole() override;
  void DisplayAlert(const char*, ...) override;
  int  GetLastSystemError() override;
  FSLocation GetAllUsersDataDirectory() override;
  FSLocation GetUserSavedgamesDirectory() override;
  FSLocation GetUserConfigDirectory() override;
  FSLocation GetUserGlobalConfigDirectory() override;
  FSLocation GetAppOutputDirectory() override;
  const char *GetIllegalFileChars() override;
  const char *GetGraphicsTroubleshootingText() override;
  unsigned long GetDiskFreeSpaceMB() override;
  const char* GetBackendFailUserHint() override;
  eScriptSystemOSID GetSystemOSID() override;
  void PostBackendInit() override;
  void PostBackendExit() override;
  SetupReturnValue RunSetup(const ConfigTree &cfg_in, ConfigTree &cfg_out) override;
  void WriteStdOut(const char *fmt, ...) override;
  void WriteStdErr(const char *fmt, ...) override;
  void PauseApplication() override;
  void ResumeApplication() override;
  Size ValidateWindowSize(const Size &sz, bool borderless) const override;
  SDL_Surface *CreateWindowIcon() override;

  // Returns command line argument in a UTF-8 format
  String GetCommandArg(size_t arg_index) override;

private:
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

static void DetermineAppDataFolder()
{
  if (win32AppDataDirectory.IsValid()) 
    return; // already set
  WCHAR unicodePath[MAX_PATH_SZ];
  SHGetSpecialFolderPathW(NULL, unicodePath, CSIDL_COMMON_APPDATA, FALSE);
  win32AppDataDirectory =
      FSLocation(Path::WidePathToUTF8(unicodePath), "Adventure Game Studio");
}

static void DetermineSavedGamesFolder()
{
  if (win32SavedGamesDirectory.IsValid())
    return; // already discovered

  String parentDir, agsDir;
  if (IsWindowsVistaOrGreater())
  {
    HINSTANCE hShellDLL = LoadLibrary("shell32.dll");
    Dynamic_SHGetKnownFolderPathType Dynamic_SHGetKnownFolderPath = (Dynamic_SHGetKnownFolderPathType)GetProcAddress(hShellDLL, "SHGetKnownFolderPath");

    if (Dynamic_SHGetKnownFolderPath != NULL)
    {
      PWSTR path = NULL;
      if (SUCCEEDED(Dynamic_SHGetKnownFolderPath(FOLDERID_SAVEDGAMES, 0, NULL, &path)))
      {
        parentDir = Path::WidePathToUTF8(path);
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
    parentDir = Path::WidePathToUTF8(unicodeSaveGameDir);
    parentDir = Path::ConcatPaths(parentDir, "My Saved Games");
  }

  win32SavedGamesDirectory = FSLocation(parentDir, agsDir);
}

static void DetermineAppOutputDirectory()
{
  if (win32OutputDirectory.IsValid())
    return; // already set

  DetermineSavedGamesFolder();
  // Use system save dir if it's found
  if (win32SavedGamesDirectory.IsValid())
  {
    // Use "AGS" subdir inside a standard save directory
    win32OutputDirectory = win32SavedGamesDirectory.Concat("Adventure Game Studio");
  }
  // ...otherwise try local exe's dir, hoping that it's writeable
  else
  {
    WCHAR theexename[MAX_PATH_SZ];
    GetModuleFileNameW(NULL, theexename, MAX_PATH_SZ);
    PathRemoveFileSpecW(theexename);
    win32OutputDirectory = FSLocation(Path::WidePathToUTF8(theexename));
  }
}

FSLocation AGSWin32::GetAllUsersDataDirectory()
{
  DetermineAppDataFolder();
  return win32AppDataDirectory;
}

FSLocation AGSWin32::GetUserSavedgamesDirectory()
{
  DetermineSavedGamesFolder();
  return win32SavedGamesDirectory;
}

FSLocation AGSWin32::GetUserConfigDirectory()
{
  DetermineSavedGamesFolder();
  return win32SavedGamesDirectory;
}

FSLocation AGSWin32::GetUserGlobalConfigDirectory()
{
  DetermineAppOutputDirectory();
  return win32OutputDirectory;
}

FSLocation AGSWin32::GetAppOutputDirectory()
{
  DetermineAppOutputDirectory();
  return win32OutputDirectory;
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
  vsnprintf(displbuf, sizeof(displbuf), text, ap);
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

  // On Win9x, the last 3 params cannot be null, so need to supply values for all
  __int64 i64FreeBytesToCaller, i64Unused1, i64Unused2;

  // Win95 OSR2 or higher - use GetDiskFreeSpaceEx, since the
  // normal GetDiskFreeSpace returns erroneous values if the
  // free space is > 2 GB
  fResult = GetDiskFreeSpaceEx(NULL,
             (PULARGE_INTEGER)&i64FreeBytesToCaller,
             (PULARGE_INTEGER)&i64Unused1,
             (PULARGE_INTEGER)&i64Unused2);

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
    char buf[1024] = "AGS: ";
    vsnprintf(buf + 5, 1024 - 5, fmt, ap);
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
    char buf[1024] = "AGS ERR: ";
    vsnprintf(buf + 9, 1024 - 9, fmt, ap);
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

Size AGSWin32::ValidateWindowSize(const Size &sz, bool borderless) const
{
    // Limit the window's client size by the two metrics:
    // * system's window size limit,
    // * work space (visible area);
    // if the window style includes a border, then subtract it from the limits
    RECT wa_rc, nc_rc;
    // This is the size of the available workspace on user's desktop
    SystemParametersInfo(SPI_GETWORKAREA, 0, &wa_rc, 0);
    // This is the maximal size that OS can reliably resize the window to (including any frame)
    const Size max_win(GetSystemMetrics(SM_CXMAXTRACK), GetSystemMetrics(SM_CYMAXTRACK));
    // This is the size of window's non-client area (frame, caption, etc)
    LONG winstyle = borderless ? WS_POPUP : (WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX);
    SetRectEmpty(&nc_rc);
    AdjustWindowRect(&nc_rc, winstyle, FALSE);
    // Calculate the clamped size
    Size win_ceil(std::min(static_cast<int>(wa_rc.right - wa_rc.left), (max_win.Width)),
                  std::min(static_cast<int>(wa_rc.bottom - wa_rc.top), (max_win.Height)));
    win_ceil = win_ceil - Size(nc_rc.right - nc_rc.left, nc_rc.bottom - nc_rc.top);
    return Size::Clamp(sz, Size(1, 1), win_ceil);
}

SDL_Surface *AGSWin32::CreateWindowIcon()
{
    // Don't mess with SDL surface, and set an icon simply using WinAPI
    SetClassLongPtr((HWND)sys_win_get_window(), GCLP_HICON,
        (LONG_PTR)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    return nullptr;
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
LPDIRECTDRAWSURFACE2 IAGSEngine::GetBitmapSurface (BITMAP* /*bmp*/) 
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

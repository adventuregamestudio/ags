/*

Stub functions for the Wadjet Eye Utilities. Enough to play the remastered
version of The Shivah.

*/

#ifdef WIN32
#define WINDOWS_VERSION
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(disable : 4244)
#endif

#if !defined(BUILTIN_PLUGINS)
#define THIS_IS_THE_PLUGIN
#endif

#include "plugin/agsplugin.h"
#include "debug/out.h"

namespace Out = AGS::Common::Out;

#if defined(BUILTIN_PLUGINS)
namespace agswadjetutil {
#endif

//#define DEBUG

IAGSEngine* engine;

// ************  AGS Interface  ***************
// ********************************************

bool IsOnPhone()
{
  // This is used by the conversation menus in The Shivah: Kosher Edition.
  return false;
}

// I don't know when, or even if, these are called. Did I even use the correct
// data types?

void FakeKeypress(int n)
{
  Out::FPrint("STUB: FakeKeypress(%d)", n);
}

int IosGetAchievementValue(const char *achievement)
{
  Out::FPrint("STUB: IosGetAchievementValue");
  return 0;
}

bool IosSetAchievementValue(const char *achievement, int value)
{
  Out::FPrint("STUB: IosSetAchievementValue");
  return false;
}

void IosShowAchievements()
{
  Out::FPrint("STUB: IosShowAchievements");
}

void IosResetAchievements()
{
  Out::FPrint("STUB: IosResetAchievements");
}

void AGS_EngineStartup(IAGSEngine *lpEngine)
{
  engine = lpEngine;
  
  if (engine->version < 13) 
    engine->AbortGame("Engine interface is too old, need newer version of AGS.");

  engine->RegisterScriptFunction("FakeKeypress", (void*)&FakeKeypress);
  engine->RegisterScriptFunction("IsOnPhone", (void*)&IsOnPhone);
  engine->RegisterScriptFunction("IosGetAchievementValue", (void*)&IosGetAchievementValue);
  engine->RegisterScriptFunction("IosSetAchievementValue", (void*)&IosSetAchievementValue);
  engine->RegisterScriptFunction("IosShowAchievements", (void*)&IosShowAchievements);
  engine->RegisterScriptFunction("IosResetAchievements", (void*)&IosResetAchievements);
}

void AGS_EngineShutdown()
{
}

int AGS_EngineOnEvent(int event, int data)
{
  return 0;
}

int AGS_EngineDebugHook(const char *scriptName, int lineNum, int reserved)
{
  return 0;
}

void AGS_EngineInitGfx(const char *driverID, void *data)
{
}



#if defined(WINDOWS_VERSION) && !defined(BUILTIN_PLUGINS)

// ********************************************
// ***********  Editor Interface  *************
// ********************************************

const char* scriptHeader =
  "import void FakeKeypress(int);\r\n"
  "import bool IsOnPhone();\r\n"
  "import int IosGetAchievementValue(String);\r\n"
  "import bool IosSetAchievementValue(String, int);\r\n"
  "import void IosShowAchievements();\r\n"
  "import void IosResetAchievements();\r\n";

IAGSEditor* editor;

LPCSTR AGS_GetPluginName(void)
{
  // Return the plugin description
  return "Wadjet Eye Utilities";
}

int  AGS_EditorStartup(IAGSEditor* lpEditor)
{
  // User has checked the plugin to use it in their game

  // If it's an earlier version than what we need, abort.
  if (lpEditor->version < 1)
    return -1;

  editor = lpEditor;
  editor->RegisterScriptHeader(scriptHeader);

  // Return 0 to indicate success
  return 0;
}

void AGS_EditorShutdown()
{
  // User has un-checked the plugin from their game
  editor->UnregisterScriptHeader(scriptHeader);
}

void AGS_EditorProperties(HWND parent)
{
  // User has chosen to view the Properties of the plugin
  // We could load up an options dialog or something here instead
  MessageBoxA(parent, "Wadjet Eye Utilities", "About", MB_OK | MB_ICONINFORMATION);
}

int AGS_EditorSaveGame(char* buffer, int bufsize)
{
  // We don't want to save any persistent data
  return 0;
}

void AGS_EditorLoadGame(char* buffer, int bufsize)
{
  // Nothing to load for this plugin
}

#endif


#if defined(BUILTIN_PLUGINS)
}
#endif

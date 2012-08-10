/*

Helper functions for touch devices

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

#include "../../Common/agsplugin.h"

#if defined(BUILTIN_PLUGINS)
namespace agstouch {
#endif

IAGSEngine* engine;


#if defined(IOS_VERSION)

extern "C"
{
  void ios_show_keyboard();
  void ios_hide_keyboard();
  int ios_is_keyboard_visible();
}

#endif



// ********************************************
// ************  AGS Interface  ***************
// ********************************************


void TouchShowKeyboard()
{
#if defined(IOS_VERSION)
  ios_show_keyboard();
#endif
}


void TouchHideKeyboard()
{
#if defined(IOS_VERSION)
  ios_hide_keyboard();
#endif
}


bool TouchIsKeyboardVisible()
{
#if defined(IOS_VERSION)
  return (ios_is_keyboard_visible() != 0);
#else
  return false;
#endif
}


void AGS_EngineStartup(IAGSEngine *lpEngine)
{
  engine = lpEngine;

  engine->RegisterScriptFunction("TouchShowKeyboard", (void*)&TouchShowKeyboard);
  engine->RegisterScriptFunction("TouchHideKeyboard", (void*)&TouchHideKeyboard);
  engine->RegisterScriptFunction("TouchIsKeyboardVisible", (void*)&TouchIsKeyboardVisible);
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
  "import void TouchShowKeyboard();\r\n"
  "import void TouchHideKeyboard();\r\n"
  "import bool TouchIsKeyboardVisible();\r\n"
  ;


IAGSEditor* editor;


LPCSTR AGS_GetPluginName(void)
{
  // Return the plugin description
  return "Touch device control";
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
  MessageBoxA(parent, "Touch device control plugin by JJS", "About", MB_OK | MB_ICONINFORMATION);
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

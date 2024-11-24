/***********************************************************
 * AGSBlend                                                *
 *                                                         *
 * Author: Steven Poulton                                  *
 *                                                         *
 * Date: 09/01/2011                                        *
 *                                                         *
 * Description: An AGS Plugin to allow true Alpha Blending *
 *                                                         *
 ***********************************************************/

#pragma region Defines_and_Includes

#include "core/platform.h"

#define MIN_EDITOR_VERSION 1
#define MIN_ENGINE_VERSION 3

#if AGS_PLATFORM_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinBase.h>
#endif
#include <string.h>

#if !defined(BUILTIN_PLUGINS)
#define THIS_IS_THE_PLUGIN
#endif
#include "plugin/agsplugin.h"
#include "SpriteFontRenderer.h"
#include "VariableWidthSpriteFont.h"
#include "SpriteFontRendererClifftopGames.h"
#include "VariableWidthSpriteFontClifftopGames.h"


#define DEFAULT_RGB_R_SHIFT_32  16
#define DEFAULT_RGB_G_SHIFT_32  8
#define DEFAULT_RGB_B_SHIFT_32  0
#define DEFAULT_RGB_A_SHIFT_32  24

#define abs(a)						 ((a)<0 ? -(a) : (a))				 
#define ChannelBlend_Normal(B,L)     ((uint8)(B))
#define ChannelBlend_Lighten(B,L)    ((uint8)((L > B) ? L:B))
#define ChannelBlend_Darken(B,L)     ((uint8)((L > B) ? B:L))
#define ChannelBlend_Multiply(B,L)   ((uint8)((B * L) / 255))
#define ChannelBlend_Average(B,L)    ((uint8)((B + L) / 2))
#define ChannelBlend_Add(B,L)        ((uint8)(min(255, (B + L))))
#define ChannelBlend_Subtract(B,L)   ((uint8)((B + L < 255) ? 0:(B + L - 255)))
#define ChannelBlend_Difference(B,L) ((uint8)(abs(B - L)))
#define ChannelBlend_Negation(B,L)   ((uint8)(255 - abs(255 - B - L)))
#define ChannelBlend_Screen(B,L)     ((uint8)(255 - (((255 - B) * (255 - L)) >> 8)))
#define ChannelBlend_Exclusion(B,L)  ((uint8)(B + L - 2 * B * L / 255))
#define ChannelBlend_Overlay(B,L)    ((uint8)((L < 128) ? (2 * B * L / 255):(255 - 2 * (255 - B) * (255 - L) / 255)))
#define ChannelBlend_SoftLight(B,L)  ((uint8)((L < 128)?(2*((B>>1)+64))*((float)L/255):(255-(2*(255-((B>>1)+64))*(float)(255-L)/255))))
#define ChannelBlend_HardLight(B,L)  (ChannelBlend_Overlay(L,B))
#define ChannelBlend_ColorDodge(B,L) ((uint8)((L == 255) ? L:min(255, ((B << 8 ) / (255 - L)))))
#define ChannelBlend_ColorBurn(B,L)  ((uint8)((L == 0) ? L:max(0, (255 - ((255 - B) << 8 ) / L))))
#define ChannelBlend_LinearDodge(B,L)(ChannelBlend_Add(B,L))
#define ChannelBlend_LinearBurn(B,L) (ChannelBlend_Subtract(B,L))
#define ChannelBlend_LinearLight(B,L)((uint8)(L < 128)?ChannelBlend_LinearBurn(B,(2 * L)):ChannelBlend_LinearDodge(B,(2 * (L - 128))))
#define ChannelBlend_VividLight(B,L) ((uint8)(L < 128)?ChannelBlend_ColorBurn(B,(2 * L)):ChannelBlend_ColorDodge(B,(2 * (L - 128))))
#define ChannelBlend_PinLight(B,L)   ((uint8)(L < 128)?ChannelBlend_Darken(B,(2 * L)):ChannelBlend_Lighten(B,(2 * (L - 128))))
#define ChannelBlend_HardMix(B,L)    ((uint8)((ChannelBlend_VividLight(B,L) < 128) ? 0:255))
#define ChannelBlend_Reflect(B,L)    ((uint8)((L == 255) ? L:min(255, (B * B / (255 - L)))))
#define ChannelBlend_Glow(B,L)       (ChannelBlend_Reflect(L,B))
#define ChannelBlend_Phoenix(B,L)    ((uint8)(min(B,L) - max(B,L) + 255))
#define ChannelBlend_Alpha(B,L,O)    ((uint8)(O * B + (1 - O) * L))
#define ChannelBlend_AlphaF(B,L,F,O) (ChannelBlend_Alpha(F(B,L),B,O))


#pragma endregion


#if AGS_PLATFORM_OS_WINDOWS && !defined(BUILTIN_PLUGINS)
// The standard Windows DLL entry point

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved) {

  switch (ul_reason_for_call)	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
  }
  return TRUE;
}
#endif


//define engine

IAGSEngine *engine = nullptr;
SpriteFontRenderer *fontRenderer = nullptr;
VariableWidthSpriteFontRenderer *vWidthRenderer = nullptr;


void SetSpriteFont(int fontNum, int sprite, int rows, int columns, int charWidth, int charHeight, int charMin, int charMax, bool use32bit)
{
	engine->PrintDebugConsole("AGSSpriteFont: SetSpriteFont");
	fontRenderer->SetSpriteFont(fontNum, sprite, rows, columns, charWidth, charHeight, charMin, charMax, use32bit);
	if (engine->version >= 26)
		engine->ReplaceFontRenderer2(fontNum, fontRenderer);
	else
		engine->ReplaceFontRenderer(fontNum, fontRenderer);
}

void SetVariableSpriteFont(int fontNum, int sprite)
{
	engine->PrintDebugConsole("AGSSpriteFont: SetVariableFont");
	vWidthRenderer->SetSprite(fontNum, sprite);
	if (engine->version >= 26)
		engine->ReplaceFontRenderer2(fontNum, vWidthRenderer);
	else
		engine->ReplaceFontRenderer(fontNum, vWidthRenderer);
}

void SetGlyph(int fontNum, int charNum, int x, int y, int width, int height)
{
	engine->PrintDebugConsole("AGSSpriteFont: SetGlyph");
	vWidthRenderer->SetGlyph(fontNum, charNum, x,y,width,height);
}

void SetSpacing(int fontNum, int spacing)
{
	engine->PrintDebugConsole("AGSSpriteFont: SetSpacing");
	vWidthRenderer->SetSpacing(fontNum, spacing);
}

void SetLineHeightAdjust(int fontNum, int lineHeight, int spacingHeight, int spacingOverride)
{
	engine->PrintDebugConsole("AGSSpriteFont: SetLineHeightAdjust");
	vWidthRenderer->SetLineHeightAdjust(fontNum, lineHeight, spacingHeight, spacingOverride);
}

//==============================================================================

#if AGS_PLATFORM_OS_WINDOWS && !defined(BUILTIN_PLUGINS)
// ***** Design time *****

IAGSEditor *editor; // Editor interface

const char *ourScriptHeader =
  "import void SetSpriteFont(int fontNum, int sprite, int rows, int columns, int charWidth, int charHeight, int charMin, int charMax, bool use32bit);\r\n"
  "import void SetVariableSpriteFont(int fontNum, int sprite);\r\n"
  "import void SetGlyph(int fontNum, int charNum, int x, int y, int width, int height);\r\n"
  "import void SetSpacing(int fontNum, int spacing);\r\n"
  "import void SetLineHeightAdjust(int, int, int, int);\r\n"
  ;

//------------------------------------------------------------------------------

LPCSTR AGS_GetPluginName()
{
	return ("AGSSpriteFont");
}

//------------------------------------------------------------------------------

int AGS_EditorStartup(IAGSEditor *lpEditor)
{
	// User has checked the plugin to use it in their game
	
	// If it's an earlier version than what we need, abort.
	if (lpEditor->version < MIN_EDITOR_VERSION)
		return (-1);
	
	editor = lpEditor;
	editor->RegisterScriptHeader(ourScriptHeader);
	
	// Return 0 to indicate success
	return (0);
}

//------------------------------------------------------------------------------

void AGS_EditorShutdown()
{
	// User has un-checked the plugin from their game
	editor->UnregisterScriptHeader(ourScriptHeader);
}

//------------------------------------------------------------------------------

void AGS_EditorProperties(HWND parent)                        //*** optional ***
{
	// User has chosen to view the Properties of the plugin
	// We could load up an options dialog or something here instead
	MessageBox(parent,
	           L"AGSSpriteFont v1.0 By Calin Leafshade",
	           L"About",
			   MB_OK | MB_ICONINFORMATION);
}

//------------------------------------------------------------------------------

int AGS_EditorSaveGame(char *buffer, int bufsize)             //*** optional ***
{
	// Called by the editor when the current game is saved to disk.
	// Plugin configuration can be stored in [buffer] (max [bufsize] bytes)
	// Return the amount of bytes written in the buffer
	return (0);
}

//------------------------------------------------------------------------------

void AGS_EditorLoadGame(char *buffer, int bufsize)            //*** optional ***
{
	// Called by the editor when a game is loaded from disk
	// Previous written data can be read from [buffer] (size [bufsize]).
	// Make a copy of the data, the buffer is freed after this function call.
}

//==============================================================================
#endif


// ***** Run time *****

 // Engine interface

//------------------------------------------------------------------------------

#define REGISTER(x) engine->RegisterScriptFunction(#x, (void *) (x));
#define STRINGIFY(s) STRINGIFY_X(s)
#define STRINGIFY_X(s) #s

#if defined(BUILTIN_PLUGINS)
namespace agsspritefont {
#endif

void AGS_EngineStartup(IAGSEngine *lpEngine)
{
	engine = lpEngine;

	// Make sure it's got the version with the features we need
	if (engine->version < MIN_ENGINE_VERSION)
		engine->AbortGame("Plugin needs engine version " STRINGIFY(MIN_ENGINE_VERSION) " or newer.");

	// For certain games instantiate the Clifftop Games version of the font renderer.
	bool useClifftopGamesRenderers = false;
#if defined(CLIFFTOPGAMES)
    useClifftopGamesRenderers = true;
#elif defined(BUILTIN_PLUGINS) || defined(CLIFFTOPGAMES_AUTODETECT)
	if (engine->version >= 26) {
		AGSGameInfo gameInfo;
		gameInfo.Version = 26;
		lpEngine->GetGameInfo(&gameInfo);
		// GUID:
		// Kathy Rain: {d6795d1c-3cfe-49ec-90a1-85c313bfccaf}
		// Whispers of a Machine: {5833654f-6f0d-40d9-99e2-65c101c8544a}
		// The Excavation at Hob's Barrow: {4d1d659d-f5ed-4945-b031-68fedcac7510}
		// Off The Clock: {569a3928-1e26-48cd-9449-ec06a8d40b85}
		useClifftopGamesRenderers =
			strcmp("{d6795d1c-3cfe-49ec-90a1-85c313bfccaf}", gameInfo.Guid) == 0 ||
			strcmp("{5833654f-6f0d-40d9-99e2-65c101c8544a}", gameInfo.Guid) == 0 ||
			strcmp("{4d1d659d-f5ed-4945-b031-68fedcac7510}", gameInfo.Guid) == 0 ||
			strcmp("{569a3928-1e26-48cd-9449-ec06a8d40b85}", gameInfo.Guid) == 0;
	}
#endif
	if (useClifftopGamesRenderers)
	{
		engine->PrintDebugConsole("AGSSpriteFont: Init Clifftop Games fixed width renderer");
		fontRenderer = new SpriteFontRendererClifftopGames(engine);
		engine->PrintDebugConsole("AGSSpriteFont: Init Clifftop Games variable width renderer");
		vWidthRenderer = new VariableWidthSpriteFontRendererClifftopGames(engine);
	}
	else
	{
		engine->PrintDebugConsole("AGSSpriteFont: Init fixed width renderer");
		fontRenderer = new SpriteFontRenderer(engine);
		engine->PrintDebugConsole("AGSSpriteFont: Init variable width renderer");
		vWidthRenderer = new VariableWidthSpriteFontRenderer(engine);
	}

	//register functions
	engine->PrintDebugConsole("AGSSpriteFont: Register functions");
	REGISTER(SetSpriteFont)
	REGISTER(SetVariableSpriteFont)
	REGISTER(SetGlyph)
	REGISTER(SetSpacing)
	REGISTER(SetLineHeightAdjust)
}

//------------------------------------------------------------------------------

void AGS_EngineShutdown()
{
	// Called by the game engine just before it exits.
	// This gives you a chance to free any memory and do any cleanup
	// that you need to do before the engine shuts down.
	delete fontRenderer;
	delete vWidthRenderer;
}

//------------------------------------------------------------------------------

int AGS_EngineOnEvent(int event, int data)                    //*** optional ***
{
	switch (event)
	{
/*
		case AGSE_KEYPRESS:
		case AGSE_MOUSECLICK:
		case AGSE_POSTSCREENDRAW:
		case AGSE_PRESCREENDRAW:
		case AGSE_SAVEGAME:
		case AGSE_RESTOREGAME:
		case AGSE_PREGUIDRAW:
		case AGSE_LEAVEROOM:
		case AGSE_ENTERROOM:
		case AGSE_TRANSITIONIN:
		case AGSE_TRANSITIONOUT:
		case AGSE_FINALSCREENDRAW:
		case AGSE_TRANSLATETEXT:
		case AGSE_SCRIPTDEBUG:
		case AGSE_SPRITELOAD:
		case AGSE_PRERENDER:
		case AGSE_PRESAVEGAME:
		case AGSE_POSTRESTOREGAME:
*/
		default:
			break;
	}
	
	// Return 1 to stop event from processing further (when needed)
	return (0);
}

//------------------------------------------------------------------------------
/*
int AGS_EngineDebugHook(const char *scriptName,
                        int lineNum, int reserved)            //*** optional ***
{
	// Can be used to debug scripts, see documentation
}
*/
//------------------------------------------------------------------------------
/*
void AGS_EngineInitGfx(const char *driverID, void *data)      //*** optional ***
{
	// This allows you to make changes to how the graphics driver starts up.
	// See documentation
}
*/

//------------------------------------------------------------------------------

// Export this to let engine verify that this is a compatible AGS Plugin;
// exact return value is not essential, but should be non-zero for consistency.
int AGS_PluginV2()
{
    return 1;
}

//..............................................................................

#if defined(BUILTIN_PLUGINS)
}
#endif

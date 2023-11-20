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
//
// AGS Plugin interface header file.
//
// #define THIS_IS_THE_PLUGIN beforehand if including from the plugin.
//
//=============================================================================
#ifndef _AGS_PLUGIN_H
#define _AGS_PLUGIN_H

#include <stddef.h> // for size_t
#include <stdint.h>
#include "agsplugin_evts.h"

// If the plugin isn't using DDraw, don't require the headers
#ifndef DIRECTDRAW_VERSION
typedef void *LPDIRECTDRAW2;
typedef void *LPDIRECTDRAWSURFACE2;
#endif

#ifndef DIRECTSOUND_VERSION
typedef void *LPDIRECTSOUND;
#endif

#ifndef DIRECTINPUT_VERSION
typedef void *LPDIRECTINPUTDEVICE;
#endif

// If the user isn't using Allegro or WinGDI, define the BITMAP into something
#if !defined(ALLEGRO_H) && !defined(_WINGDI_) && !defined(BITMAP_DEFINED)
typedef void BITMAP;
#endif

// If not using windows.h, define HWND
#if !defined(_WINDOWS_)
typedef void *HWND;
#endif

// This file is distributed as part of the Plugin API docs, so
// ensure that WINDOWS_VERSION is defined (if applicable)
#if defined(_WIN32)
  #undef WINDOWS_VERSION
  #define WINDOWS_VERSION
#endif

// DOS engine doesn't know about stdcall / neither does Linux version
#if !defined (_WIN32)
  #define __stdcall
#endif

#ifndef int32
typedef int int32;
#endif

#define AGSIFUNC(type) virtual type __stdcall

#define MASK_WALKABLE   1
#define MASK_WALKBEHIND 2
#define MASK_HOTSPOT    3
// MASK_REGIONS is interface version 11 and above only
#define MASK_REGIONS    4

// **** WARNING: DO NOT ALTER THESE CLASSES IN ANY WAY!
// **** CHANGING THE ORDER OF THE FUNCTIONS OR ADDING ANY VARIABLES
// **** WILL CRASH THE SYSTEM.

struct AGSColor {
  unsigned char r,g,b;
  unsigned char padding;
};

// These are pointer types returned from the engine, to be used only as
// arguments when calling script functions. The contents of these types are
// sealed from plugins. You do not need to know what is inside these structs,
// nor try to access their data in your plugin code, because it may be
// a subject of change in any future engine update.
struct AGSGameOptions;
struct AGSCharacter;
struct AGSObject;
struct AGSViewFrame;
struct AGSMouseCursor;


// GetFontType font types
#define FNT_INVALID 0
#define FNT_SCI     1
#define FNT_TTF     2

// PlaySoundChannel sound types
#define PSND_WAVE       1
#define PSND_MP3STREAM  2
#define PSND_MP3STATIC  3
#define PSND_OGGSTREAM  4
#define PSND_OGGSTATIC  5
#define PSND_MIDI       6
#define PSND_MOD        7
#define PSND_FLAC       8

class IAGSScriptManagedObject {
public:
  // when a ref count reaches 0, this is called with the address
  // of the object. Return 1 to remove the object from memory, 0 to
  // leave it
  virtual int Dispose(void *address, bool force) = 0;
  // return the type name of the object
  virtual const char *GetType() = 0;
  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(void *address, char *buffer, int bufsize) = 0;
protected:
  IAGSScriptManagedObject() {};
  ~IAGSScriptManagedObject() {};
};

class IAGSManagedObjectReader {
public:
  virtual void Unserialize(int key, const char *serializedData, int dataSize) = 0;
protected:
  IAGSManagedObjectReader() {};
  ~IAGSManagedObjectReader() {};
};

class IAGSFontRenderer {
public:
  virtual bool LoadFromDisk(int fontNumber, int fontSize) = 0;
  virtual void FreeMemory(int fontNumber) = 0;
  virtual bool SupportsExtendedCharacters(int fontNumber) = 0;
  virtual int GetTextWidth(const char *text, int fontNumber) = 0;
  virtual int GetTextHeight(const char *text, int fontNumber) = 0;
  virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) = 0;
  virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber) = 0;
  virtual void EnsureTextValidForFont(char *text, int fontNumber) = 0;
protected:
  IAGSFontRenderer() = default;
  ~IAGSFontRenderer() = default;
};

class IAGSFontRenderer2 : public IAGSFontRenderer {
  virtual int GetVersion() = 0;
  virtual const char *GetRendererName() = 0;
  virtual const char *GetFontName(int fontNumber) = 0;
  virtual int GetFontHeight(int fontNumber) = 0;
  virtual int GetLineSpacing(int fontNumber) = 0;
protected:
  IAGSFontRenderer2() = default;
  ~IAGSFontRenderer2() = default;
};

struct AGSRenderMatrixes {
  float WorldMatrix[16];
  float ViewMatrix[16];
  float ProjMatrix[16];
};

// Render stage description
struct AGSRenderStageDesc {
  // Which version of the plugin interface the struct corresponds to;
  // this field must be filled by a plugin before passing the struct into the engine!
  int Version;
  // Stage's matrixes, for 3D rendering: Projection, World and View
  AGSRenderMatrixes Matrixes;
};

// Game info
struct AGSGameInfo {
  // Which version of the plugin interface the struct corresponds to;
  // this field must be filled by a plugin before passing the struct into the engine!
  int Version;
  // Game title (human-readable text)
  char GameName[50];
  // Game's GUID
  char Guid[40];
  // Random key identifying the game (deprecated)
  int UniqueId;
};

// File open modes
// Opens existing file, fails otherwise
#define AGSSTREAM_FILE_OPEN         0
// Opens existing file, creates one if it did not exist
#define AGSSTREAM_FILE_CREATE       1
// Always creates a new file, completely overwrites any existing one
#define AGSSTREAM_FILE_CREATEALWAYS 2

// Stream work modes
// Read-only
#define AGSSTREAM_MODE_READ  0x1
// Write-only
#define AGSSTREAM_MODE_WRITE 0x2
// Support both read and write
#define AGSSTREAM_MODE_READWRITE (AGSSTREAM_MODE_READ | AGSSTREAM_MODE_WRITE)

// Stream seek origins
// Seek from the beginning of a stream (towards positive offset)
#define AGSSTREAM_SEEK_SET 0
// Seek from the current position (towards positive or negative offset)
#define AGSSTREAM_SEEK_CUR 1
// Seek from the end of a stream (towards negative offset)
#define AGSSTREAM_SEEK_END 2

class IAGSStream {
public:
  // Flushes and closes the stream, deallocates the stream object.
  // After calling this the IAGSStream pointer becomes INVALID.
  virtual void   Close() = 0;
  // Returns an optional stream's source description.
  // This may be a file path, or a resource name, or anything of that kind.
  virtual const char *GetPath() = 0;
  // Reads number of bytes into the provided buffer
  virtual size_t Read(void *buffer, size_t len) = 0;
  // Writes number of bytes from the provided buffer
  virtual size_t Write(void *buffer, size_t len) = 0;
  // Returns the total stream's length in bytes
  virtual int64_t GetLength() = 0;
  // Returns stream's position
  virtual int64_t GetPosition() = 0;
  // Tells whether the stream's position is at its end
  virtual bool   EOS() = 0;
  // Seeks to offset from the origin defined by AGSSTREAM_SEEK_* constants:
  //  * AGSSTREAM_SEEK_SET - seek from the beginning;
  //  * AGSSTREAM_SEEK_CUR - seek from the current position;
  //  * AGSSTREAM_SEEK_END - seek from the end (pass negative offset)
  // Returns new position in stream, or -1 on error.
  virtual int64_t Seek(int64_t offset, int origin) = 0;
  // Flushes stream, forcing it to write any buffered data to the
  // underlying device. Note that the effect may depend on implementation.
  virtual void   Flush() = 0;

protected:
  IAGSStream() = default;
  ~IAGSStream() = default;
};


// The plugin-to-engine interface
class IAGSEngine {
public:
  int32 version;
  int32 pluginId;   // used internally, do not touch

public:
  // quit the game
  AGSIFUNC(void) AbortGame (const char *reason);
  // get engine version
  AGSIFUNC(const char*) GetEngineVersion ();
  // register a script function with the system
  AGSIFUNC(void) RegisterScriptFunction (const char *name, void *address);
#ifdef WINDOWS_VERSION
  // get game window handle
  AGSIFUNC(HWND) GetWindowHandle();
  // get reference to main DirectDraw interface
  AGSIFUNC(LPDIRECTDRAW2) GetDirectDraw2 ();
  // get the DDraw surface associated with a bitmap
  AGSIFUNC(LPDIRECTDRAWSURFACE2) GetBitmapSurface (BITMAP *);
#endif
  // get a reference to the screen bitmap
  AGSIFUNC(BITMAP *) GetScreen ();

  // *** BELOW ARE INTERFACE VERSION 2 AND ABOVE ONLY
  // ask the engine to call back when a certain event happens
  AGSIFUNC(void) RequestEventHook (int32 event);
  // get the options data saved in the editor
  AGSIFUNC(int)  GetSavedData (char *buffer, int32 bufsize);

  // *** BELOW ARE INTERFACE VERSION 3 AND ABOVE ONLY
  // get the virtual screen
  AGSIFUNC(BITMAP *) GetVirtualScreen ();
  // write text to the screen in the specified font and colour
  AGSIFUNC(void) DrawText (int32 x, int32 y, int32 font, int32 color, char *text);
  // get screen dimensions
  AGSIFUNC(void) GetScreenDimensions (int32 *width, int32 *height, int32 *coldepth);
  // get screen surface to draw on
  AGSIFUNC(unsigned char**) GetRawBitmapSurface (BITMAP *);
  // release the surface
  AGSIFUNC(void) ReleaseBitmapSurface (BITMAP *);
  // get the current mouse co-ordinates
  AGSIFUNC(void) GetMousePosition (int32 *x, int32 *y);

  // *** BELOW ARE INTERFACE VERSION 4 AND ABOVE ONLY
  // get the current room number
  AGSIFUNC(int)  GetCurrentRoom ();
  // get the number of background scenes in this room
  AGSIFUNC(int)  GetNumBackgrounds ();
  // get the current background frame
  AGSIFUNC(int)  GetCurrentBackground ();
  // get a background scene bitmap
  AGSIFUNC(BITMAP *) GetBackgroundScene (int32);
  // get dimensions of a bitmap
  AGSIFUNC(void) GetBitmapDimensions (BITMAP *bmp, int32 *width, int32 *height, int32 *coldepth);

  // *** BELOW ARE INTERFACE VERSION 5 AND ABOVE ONLY
  // similar to fwrite - buffer, size, filehandle
  AGSIFUNC(int)  FWrite (void *out_buf, int32 len, int32 fhandle);
  // similar to fread - buffer, size, filehandle
  AGSIFUNC(int)  FRead (void *in_buf, int32 len, int32 fhandle);
  // print text, wrapping as usual
  AGSIFUNC(void) DrawTextWrapped (int32 x, int32 y, int32 width, int32 font, int32 color, const char *text);
  // set the current active 'screen'
  AGSIFUNC(void) SetVirtualScreen (BITMAP *);
  // look up a word in the parser dictionary
  AGSIFUNC(int)  LookupParserWord (const char *word);
  // draw a bitmap to the active screen
  AGSIFUNC(void) BlitBitmap (int32 x, int32 y, BITMAP *, int32 masked);
  // update the mouse and music
  AGSIFUNC(void) PollSystem ();

  // *** BELOW ARE INTERFACE VERSION 6 AND ABOVE ONLY
  // get number of characters in game
  AGSIFUNC(int)  GetNumCharacters ();
  // get reference to specified character struct
  AGSIFUNC(AGSCharacter*) GetCharacter (int32);
  // get reference to game struct
  AGSIFUNC(AGSGameOptions*) GetGameOptions ();
  // get reference to current palette
  AGSIFUNC(AGSColor*) GetPalette();
  // update palette
  AGSIFUNC(void) SetPalette (int32 start, int32 finish, AGSColor*);

  // *** BELOW ARE INTERFACE VERSION 7 AND ABOVE ONLY
  // get the current player character
  AGSIFUNC(int)  GetPlayerCharacter ();
  // adjust to main viewport co-ordinates
  AGSIFUNC(void) RoomToViewport (int32 *x, int32 *y);
  // adjust from main viewport co-ordinates (ignores viewport bounds)
  AGSIFUNC(void) ViewportToRoom (int32 *x, int32 *y);
  // number of objects in current room
  AGSIFUNC(int)  GetNumObjects ();
  // get reference to specified object
  AGSIFUNC(AGSObject*) GetObject (int32);
  // get sprite graphic
  AGSIFUNC(BITMAP *) GetSpriteGraphic (int32);
  // create a new blank bitmap
  AGSIFUNC(BITMAP *) CreateBlankBitmap (int32 width, int32 height, int32 coldep);
  // free a created bitamp
  AGSIFUNC(void) FreeBitmap (BITMAP *);

  // *** BELOW ARE INTERFACE VERSION 8 AND ABOVE ONLY
  // get one of the room area masks
  AGSIFUNC(BITMAP *) GetRoomMask(int32);

  // *** BELOW ARE INTERFACE VERSION 9 AND ABOVE ONLY
  // get a particular view frame
  AGSIFUNC(AGSViewFrame *) GetViewFrame(int32 view, int32 loop, int32 frame);
  // get the walk-behind baseline of a specific WB area
  AGSIFUNC(int)    GetWalkbehindBaseline(int32 walkbehind);
  // get the address of a script function
  AGSIFUNC(void *) GetScriptFunctionAddress(const char * funcName);
  // get the transparent colour of a bitmap
  AGSIFUNC(int)    GetBitmapTransparentColor(BITMAP *);
  // get the character scaling level at a particular point
  AGSIFUNC(int)    GetAreaScaling (int32 x, int32 y);
  // equivalent to the text script function
  AGSIFUNC(int)    IsGamePaused();

  // *** BELOW ARE INTERFACE VERSION 10 AND ABOVE ONLY
  // get the raw pixel value to use for the specified AGS colour
  AGSIFUNC(int)    GetRawPixelColor (int32 color);

  // *** BELOW ARE INTERFACE VERSION 11 AND ABOVE ONLY
  // get the width / height of the specified sprite
  AGSIFUNC(int)    GetSpriteWidth (int32);
  AGSIFUNC(int)    GetSpriteHeight (int32);
  // get the dimensions of the specified string in the specified font
  AGSIFUNC(void)   GetTextExtent (int32 font, const char *text, int32 *width, int32 *height);
  // print a message to the debug console
  AGSIFUNC(void)   PrintDebugConsole (const char *text);
  // play a sound on the specified channel
  AGSIFUNC(void)   PlaySoundChannel (int32 channel, int32 soundType, int32 volume, int32 loop, const char *filename);
  // same as text script function
  AGSIFUNC(int)    IsChannelPlaying (int32 channel);

  // *** BELOW ARE INTERFACE VERSION 12 AND ABOVE ONLY
  // invalidate a region of the virtual screen
  AGSIFUNC(void)   MarkRegionDirty(int32 left, int32 top, int32 right, int32 bottom);
  // get mouse cursor details
  AGSIFUNC(AGSMouseCursor *) GetMouseCursor(int32 cursor);
  // get the various components of a pixel
  AGSIFUNC(void)   GetRawColorComponents(int32 coldepth, int32 color, int32 *red, int32 *green, int32 *blue, int32 *alpha);
  // make a pixel colour from the supplied components
  AGSIFUNC(int)    MakeRawColorPixel(int32 coldepth, int32 red, int32 green, int32 blue, int32 alpha);
  // get whether the font is TTF or SCI
  AGSIFUNC(int)    GetFontType(int32 fontNum);
  // create a new dynamic sprite slot
  AGSIFUNC(int)    CreateDynamicSprite(int32 coldepth, int32 width, int32 height);
  // free a created dynamic sprite
  AGSIFUNC(void)   DeleteDynamicSprite(int32 slot);
  // check if a sprite has an alpha channel
  AGSIFUNC(int)    IsSpriteAlphaBlended(int32 slot);

  // *** BELOW ARE INTERFACE VERSION 13 AND ABOVE ONLY
  // un-request an event, requested earlier with RequestEventHook
  AGSIFUNC(void)   UnrequestEventHook(int32 event);
  // draw a translucent bitmap to the active screen
  AGSIFUNC(void)   BlitSpriteTranslucent(int32 x, int32 y, BITMAP *, int32 trans);
  // draw a sprite to the screen, but rotated around its centre
  AGSIFUNC(void)   BlitSpriteRotated(int32 x, int32 y, BITMAP *, int32 angle);

  // *** BELOW ARE INTERFACE VERSION 14 AND ABOVE ONLY
#ifdef WINDOWS_VERSION
  // get reference to main DirectSound interface
  AGSIFUNC(LPDIRECTSOUND) GetDirectSound();
#endif
  // disable AGS sound engine
  AGSIFUNC(void)   DisableSound();
  // check whether a script function can be run now
  AGSIFUNC(int)    CanRunScriptFunctionNow();
  // call a user-defined script function
  AGSIFUNC(int)    CallGameScriptFunction(const char *name, int32 globalScript, int32 numArgs, long arg1 = 0, long arg2 = 0, long arg3 = 0);

  // *** BELOW ARE INTERFACE VERSION 15 AND ABOVE ONLY
  // force any sprites on-screen using the slot to be updated
  AGSIFUNC(void)   NotifySpriteUpdated(int32 slot);
  // change whether the specified sprite is a 32-bit alpha blended image
  AGSIFUNC(void)   SetSpriteAlphaBlended(int32 slot, int32 isAlphaBlended);
  // run the specified script function whenever script engine is available
  AGSIFUNC(void)   QueueGameScriptFunction(const char *name, int32 globalScript, int32 numArgs, long arg1 = 0, long arg2 = 0);
  // register a new dynamic managed script object
  AGSIFUNC(int)    RegisterManagedObject(void *object, IAGSScriptManagedObject *callback);
  // add an object reader for the specified object type
  AGSIFUNC(void)   AddManagedObjectReader(const char *typeName, IAGSManagedObjectReader *reader);
  // register an un-serialized managed script object
  AGSIFUNC(void)   RegisterUnserializedObject(int key, void *object, IAGSScriptManagedObject *callback);

  // *** BELOW ARE INTERFACE VERSION 16 AND ABOVE ONLY
  // get the address of a managed object based on its key
  AGSIFUNC(void*)  GetManagedObjectAddressByKey(int key);
  // get managed object's key from its address
  AGSIFUNC(int)    GetManagedObjectKeyByAddress(void *address);

  // *** BELOW ARE INTERFACE VERSION 17 AND ABOVE ONLY
  // create a new script string
  AGSIFUNC(const char*) CreateScriptString(const char *fromText);

  // *** BELOW ARE INTERFACE VERSION 18 AND ABOVE ONLY
  // increment reference count
  AGSIFUNC(int)    IncrementManagedObjectRefCount(void *address);
  // decrement reference count
  AGSIFUNC(int)    DecrementManagedObjectRefCount(void *address);
  // set mouse position
  AGSIFUNC(void)   SetMousePosition(int32 x, int32 y);
  // simulate the mouse being clicked
  AGSIFUNC(void)   SimulateMouseClick(int32 button);
  // get number of waypoints on this movement path
  AGSIFUNC(int)    GetMovementPathWaypointCount(int32 pathId);
  // get the last waypoint that the char/obj passed
  AGSIFUNC(int)    GetMovementPathLastWaypoint(int32 pathId);
  // get the co-ordinates of the specified waypoint
  AGSIFUNC(void)   GetMovementPathWaypointLocation(int32 pathId, int32 waypoint, int32 *x, int32 *y);
  // get the speeds of the specified waypoint
  AGSIFUNC(void)   GetMovementPathWaypointSpeed(int32 pathId, int32 waypoint, int32 *xSpeed, int32 *ySpeed);

  // *** BELOW ARE INTERFACE VERSION 19 AND ABOVE ONLY
  // get the current graphics driver ID
  AGSIFUNC(const char*) GetGraphicsDriverID();

  // *** BELOW ARE INTERFACE VERSION 22 AND ABOVE ONLY
  // get whether we are running under the editor's debugger
  AGSIFUNC(int)    IsRunningUnderDebugger();
  // tells the engine to break into the debugger when the next line of script is run
  AGSIFUNC(void)   BreakIntoDebugger();
  // fills buffer with <install dir>\fileName, as appropriate
  AGSIFUNC(void)   GetPathToFileInCompiledFolder(const char* fileName, char* buffer);

  // *** BELOW ARE INTERFACE VERSION 23 AND ABOVE ONLY
#ifdef WINDOWS_VERSION
  // get reference to keyboard Direct Input device
  AGSIFUNC(LPDIRECTINPUTDEVICE) GetDirectInputKeyboard();
  // get reference to mouse Direct Input device
  AGSIFUNC(LPDIRECTINPUTDEVICE) GetDirectInputMouse();
#endif
  // install a replacement renderer for the specified font number
  AGSIFUNC(IAGSFontRenderer*) ReplaceFontRenderer(int fontNumber, IAGSFontRenderer* newRenderer);

  // *** BELOW ARE INTERFACE VERSION 25 AND ABOVE ONLY
  // fills the provided AGSRenderStageDesc struct with current render stage description;
  // please note that plugin MUST fill the struct's Version field before passing it into the function!
  AGSIFUNC(void)  GetRenderStageDesc(AGSRenderStageDesc* desc);

  // *** BELOW ARE INTERFACE VERSION 26 AND ABOVE ONLY
  // fills the provided AGSGameInfo struct
  // please note that plugin MUST fill the struct's Version field before passing it into the function!
  AGSIFUNC(void)  GetGameInfo(AGSGameInfo* ginfo);
  // install a replacement renderer (extended interface) for the specified font number
  AGSIFUNC(IAGSFontRenderer*) ReplaceFontRenderer2(int fontNumber, IAGSFontRenderer2* newRenderer);
  // notify the engine that certain custom font has been updated
  AGSIFUNC(void)  NotifyFontUpdated(int fontNumber);

  // *** BELOW ARE INTERFACE VERSION 27 AND ABOVE ONLY
  // Resolve a script path to a system filepath, same way as script command File.Open does.
  // Caller should provide an output buffer and its length in bytes.
  // Passing NULL instead of a buffer pointer will make function calculate and return
  // length necessary to store a resulting path (in bytes).
  AGSIFUNC(size_t) ResolveFilePath(const char *script_path, char *buf, size_t buf_len);

  // *** BELOW ARE INTERFACE VERSION 28 AND ABOVE ONLY
  // Opens a data stream, resolving a script path.
  // File mode should contain one of the AGSSTREAM_FILE_* values,
  // work mode should contain flag set of the AGSSTREAM_MODE_* values.
  // Returns IAGSStream object, or null on failure.
  // IAGSStream must be disposed by calling its Close() function.
  AGSIFUNC(IAGSStream*) OpenFileStream(const char *script_path, int file_mode, int work_mode);
  // Returns IAGSStream object identified by the given stream handle.
  // This lets to retrieve IAGSStream object from a handle received in a event callback.
  // Returns null if handle is invalid.
  AGSIFUNC(IAGSStream*) GetFileStreamByHandle(int32 fhandle);
};


// The editor-to-plugin interface
class IAGSEditor {
public:
  int32 version;
  int32 pluginId;   // used internally, do not touch this

public:
  // get the HWND of the main editor frame
  AGSIFUNC(HWND) GetEditorHandle ();
  // get the HWND of the current active window
  AGSIFUNC(HWND) GetWindowHandle ();
  // add some script to the default header
  AGSIFUNC(void) RegisterScriptHeader (const char *header);
  // de-register a script header (pass same pointer as when added)
  AGSIFUNC(void) UnregisterScriptHeader (const char *header);
};


#ifdef THIS_IS_THE_PLUGIN

#ifdef WINDOWS_VERSION
#define DLLEXPORT extern "C" __declspec(dllexport)
#else
// MAC VERSION: compile with -fvisibility=hidden
// gcc -dynamiclib -std=gnu99 agsplugin.c -fvisibility=hidden -o agsplugin.dylib
#define DLLEXPORT extern "C" __attribute__((visibility("default")))
#endif

DLLEXPORT const char * AGS_GetPluginName(void);
DLLEXPORT int    AGS_EditorStartup (IAGSEditor *);
DLLEXPORT void   AGS_EditorShutdown (void);
DLLEXPORT void   AGS_EditorProperties (HWND);
DLLEXPORT int    AGS_EditorSaveGame (char *, int);
DLLEXPORT void   AGS_EditorLoadGame (char *, int);
DLLEXPORT void   AGS_EngineStartup (IAGSEngine *);
DLLEXPORT void   AGS_EngineShutdown (void);
DLLEXPORT int    AGS_EngineOnEvent (int, int);
DLLEXPORT int    AGS_EngineDebugHook(const char *, int, int);
DLLEXPORT void   AGS_EngineInitGfx(const char* driverID, void *data); 
// We export this to verify that we are an AGS Plugin
DLLEXPORT int    AGS_PluginV2 ( ) { return 1; }

#endif // THIS_IS_THE_PLUGIN

#endif // _AGS_PLUGIN_H

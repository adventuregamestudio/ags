/* 
** ALI3D -- Allegro Interface for 3D
** Copyright (C) 2007, Chris Jones
** All Rights Reserved.
*/

#ifndef __ALI3D_H
#define __ALI3D_H

#include "gfx/gfxfilter.h"
#include "gfx/blender.h"

typedef void (*ALI3DCLIENTCALLBACK)();
typedef bool (*ALI3DCLIENTCALLBACKXY)(int x, int y);
typedef void (*ALI3DCLIENTCALLBACKINITGFX)(void *data);

enum VideoSkipType
{
  VideoSkipNone = 0,
  VideoSkipEscape = 1,
  VideoSkipAnyKey = 2,
  VideoSkipKeyOrMouse = 3
};

enum GlobalFlipType
{
  None = 0,
  Horizontal = 1,
  Vertical = 2,
  Both = 3
};

enum TintMethod
{
  TintReColourise = 0,
  TintSpecifyMaximum = 1
};

class Ali3DException
{
public:
  Ali3DException(const char *message)
  {
    _message = message;
  }

  const char *_message;
};

class Ali3DFullscreenLostException : public Ali3DException
{
public:
  Ali3DFullscreenLostException() : Ali3DException("User has switched away from application")
  {
  }

  const char *_message;
};

class IDriverDependantBitmap
{
public:
  virtual void SetTransparency(int transparency) = 0;  // 0-255
  virtual void SetFlippedLeftRight(bool isFlipped) = 0;
  virtual void SetStretch(int width, int height) = 0;
  virtual void SetLightLevel(int light_level) = 0;   // 0-255
  virtual void SetTint(int red, int green, int blue, int tintSaturation) = 0;  // 0-255

  virtual int GetWidth() = 0;
  virtual int GetHeight() = 0;
  virtual int GetColorDepth() = 0;
};

class IGraphicsDriver
{
public:
  virtual const char*GetDriverName() = 0;
  virtual const char*GetDriverID() = 0;
  virtual void SetTintMethod(TintMethod method) = 0;
  virtual bool Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer) = 0;
  virtual bool Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer) = 0;
  virtual int  FindSupportedResolutionWidth(int idealWidth, int height, int colDepth, int widthRangeAllowed) = 0;
  virtual void SetCallbackForPolling(ALI3DCLIENTCALLBACK callback) = 0;
  virtual void SetCallbackToDrawScreen(ALI3DCLIENTCALLBACK callback) = 0;
  virtual void SetCallbackOnInit(ALI3DCLIENTCALLBACKINITGFX callback) = 0;
  // The NullSprite callback is called in the main render loop when a
  // null sprite is encountered. You can use this to hook into the rendering
  // process.
  virtual void SetCallbackForNullSprite(ALI3DCLIENTCALLBACKXY callback) = 0;
  virtual void UnInit() = 0;
  virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) = 0;
  virtual BITMAP* ConvertBitmapToSupportedColourDepth(BITMAP *allegroBitmap) = 0;
  virtual IDriverDependantBitmap* CreateDDBFromBitmap(BITMAP *allegroBitmap, bool hasAlpha, bool opaque = false) = 0;
  virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, BITMAP *allegroBitmap, bool hasAlpha) = 0;
  virtual void DestroyDDB(IDriverDependantBitmap* bitmap) = 0;
  virtual void ClearDrawList() = 0;
  virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap) = 0;
  virtual void SetScreenTint(int red, int green, int blue) = 0;
  virtual void SetRenderOffset(int x, int y) = 0;
  virtual void RenderToBackBuffer() = 0;
  virtual void Render() = 0;
  virtual void Render(GlobalFlipType flip) = 0;
  virtual void GetCopyOfScreenIntoBitmap(BITMAP* destination) = 0;
  virtual void EnableVsyncBeforeRender(bool enabled) = 0;
  virtual void Vsync() = 0;
  virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  virtual void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  virtual void BoxOutEffect(bool blackingOut, int speed, int delay) = 0;
  virtual bool PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen) = 0;
  virtual void UseSmoothScaling(bool enabled) = 0;
  virtual bool SupportsGammaControl() = 0;
  virtual void SetGamma(int newGamma) = 0;
  virtual BITMAP* GetMemoryBackBuffer() = 0;
  virtual void SetMemoryBackBuffer(BITMAP *backBuffer) = 0;
  virtual bool RequiresFullRedrawEachFrame() = 0;
  virtual bool HasAcceleratedStretchAndFlip() = 0;
  virtual bool UsesMemoryBackBuffer() = 0;
  virtual ~IGraphicsDriver() { }
};

extern IGraphicsDriver* GetOGLGraphicsDriver(GFXFilter *);
extern IGraphicsDriver* GetD3DGraphicsDriver(GFXFilter *);
extern IGraphicsDriver* GetSoftwareGraphicsDriver(GFXFilter *);

#endif

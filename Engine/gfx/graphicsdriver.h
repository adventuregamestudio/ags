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
//
// Graphics driver interface
//
//=============================================================================

#ifndef __AGS_EE_GFX__GRAPHICSDRIVER_H
#define __AGS_EE_GFX__GRAPHICSDRIVER_H

#include "gfx/gfxdefines.h"
#include "gfx/gfxmodelist.h"

namespace AGS
{

namespace Common { class Bitmap; }

namespace Engine
{

// Forward declaration
class IDriverDependantBitmap;
class GfxFilter;

enum TintMethod
{
  TintReColourise = 0,
  TintSpecifyMaximum = 1
};

enum VideoSkipType
{
  VideoSkipNone = 0,
  VideoSkipEscape = 1,
  VideoSkipAnyKey = 2,
  VideoSkipKeyOrMouse = 3
};

typedef void (*GFXDRV_CLIENTCALLBACK)();
typedef bool (*GFXDRV_CLIENTCALLBACKXY)(int x, int y);
typedef void (*GFXDRV_CLIENTCALLBACKINITGFX)(void *data);

class IGraphicsDriver
{
public:
  virtual const char*GetDriverName() = 0;
  virtual const char*GetDriverID() = 0;
  virtual void SetTintMethod(TintMethod method) = 0;
  virtual bool Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer, bool vsync) = 0;
  virtual bool Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer, bool vsync) = 0;
  virtual IGfxModeList *GetSupportedModeList(int color_depth) = 0;
  virtual DisplayResolution GetResolution() = 0;
  virtual void SetCallbackForPolling(GFXDRV_CLIENTCALLBACK callback) = 0;
  virtual void SetCallbackToDrawScreen(GFXDRV_CLIENTCALLBACK callback) = 0;
  virtual void SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) = 0;
  // The NullSprite callback is called in the main render loop when a
  // null sprite is encountered. You can use this to hook into the rendering
  // process.
  virtual void SetCallbackForNullSprite(GFXDRV_CLIENTCALLBACKXY callback) = 0;
  virtual void UnInit() = 0;
  virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) = 0;
  virtual Common::Bitmap *ConvertBitmapToSupportedColourDepth(Common::Bitmap *bitmap) = 0;
  virtual IDriverDependantBitmap* CreateDDBFromBitmap(Common::Bitmap *bitmap, bool hasAlpha, bool opaque = false) = 0;
  virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Common::Bitmap *bitmap, bool hasAlpha) = 0;
  virtual void DestroyDDB(IDriverDependantBitmap* bitmap) = 0;
  virtual void ClearDrawList() = 0;
  virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap) = 0;
  virtual void SetScreenTint(int red, int green, int blue) = 0;
  virtual void SetRenderOffset(int x, int y) = 0;
  virtual void RenderToBackBuffer() = 0;
  virtual void Render() = 0;
  virtual void Render(GlobalFlipType flip) = 0;
  virtual void GetCopyOfScreenIntoBitmap(Common::Bitmap *destination) = 0;
  virtual void EnableVsyncBeforeRender(bool enabled) = 0;
  virtual void Vsync() = 0;
  virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  virtual void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  virtual void BoxOutEffect(bool blackingOut, int speed, int delay) = 0;
  virtual bool PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen) = 0;
  virtual void UseSmoothScaling(bool enabled) = 0;
  virtual bool SupportsGammaControl() = 0;
  virtual void SetGamma(int newGamma) = 0;
  virtual Common::Bitmap* GetMemoryBackBuffer() = 0;
  virtual void SetMemoryBackBuffer(Common::Bitmap *backBuffer) = 0;
  virtual bool RequiresFullRedrawEachFrame() = 0;
  virtual bool HasAcceleratedStretchAndFlip() = 0;
  virtual bool UsesMemoryBackBuffer() = 0;
  virtual ~IGraphicsDriver() { }
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GRAPHICSDRIVER_H

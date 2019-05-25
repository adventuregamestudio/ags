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

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include "gfx/gfxdefines.h"
#include "gfx/gfxmodelist.h"
#include "util/geometry.h"

namespace AGS
{

namespace Common
{
    class Bitmap;
    typedef stdtr1compat::shared_ptr<Common::Bitmap> PBitmap;
}

namespace Engine
{

// Forward declaration
class IDriverDependantBitmap;
class IGfxFilter;
typedef stdtr1compat::shared_ptr<IGfxFilter> PGfxFilter;
using Common::PBitmap;

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

// Sprite transformation
// TODO: combine with stretch parameters in the IDriverDependantBitmap?
struct SpriteTransform
{
    // Translate
    int X, Y;
    float ScaleX, ScaleY;
    float Rotate; // angle, in radians

    SpriteTransform()
        : X(0), Y(0), ScaleX(1.f), ScaleY(1.f), Rotate(0.f) {}

    SpriteTransform(int x, int y, float scalex = 1.0f, float scaley = 1.0f, float rotate = 0.0f)
        : X(x), Y(y), ScaleX(scalex), ScaleY(scaley), Rotate(rotate) {}
};

typedef void (*GFXDRV_CLIENTCALLBACK)();
typedef bool (*GFXDRV_CLIENTCALLBACKXY)(int x, int y);
typedef void (*GFXDRV_CLIENTCALLBACKINITGFX)(void *data);
// Called if the rendering surface was resized by the external code (library).
// Mainly for Android and iOS ports; they are currently written in such way that
// the actual rendering surface size is redefined after IGraphicsDriver initialization.
typedef void (*GFXDRV_CLIENTCALLBACKSURFACEUPDATE)();

class IGraphicsDriver
{
public:
  virtual const char*GetDriverName() = 0;
  virtual const char*GetDriverID() = 0;
  virtual void SetTintMethod(TintMethod method) = 0;
  // Initialize given display mode
  virtual bool SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer) = 0;
  // Gets if a graphics mode was initialized
  virtual bool IsModeSet() const = 0;
  // Set the size of the native image size
  virtual bool SetNativeSize(const Size &src_size) = 0;
  virtual bool IsNativeSizeValid() const = 0;
  // Set game render frame and translation
  virtual bool SetRenderFrame(const Rect &dst_rect) = 0;
  virtual bool IsRenderFrameValid() const = 0;
  // Report which color depth options are best suited for the given native color depth
  virtual int  GetDisplayDepthForNativeDepth(int native_color_depth) const = 0;
  virtual IGfxModeList *GetSupportedModeList(int color_depth) = 0;
  virtual bool IsModeSupported(const DisplayMode &mode) = 0;
  virtual DisplayMode GetDisplayMode() const = 0;
  virtual PGfxFilter GetGraphicsFilter() const = 0;
  virtual Size GetNativeSize() const = 0;
  virtual Rect GetRenderDestination() const = 0;
  virtual void SetCallbackForPolling(GFXDRV_CLIENTCALLBACK callback) = 0;
  virtual void SetCallbackToDrawScreen(GFXDRV_CLIENTCALLBACK callback) = 0;
  virtual void SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) = 0;
  virtual void SetCallbackOnSurfaceUpdate(GFXDRV_CLIENTCALLBACKSURFACEUPDATE) = 0;
  // The NullSprite callback is called in the main render loop when a
  // null sprite is encountered. You can use this to hook into the rendering
  // process.
  virtual void SetCallbackForNullSprite(GFXDRV_CLIENTCALLBACKXY callback) = 0;
  virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) = 0;
  // Gets closest recommended bitmap format (currently - only color depth) for the given original format.
  // Engine needs to have game bitmaps brought to the certain range of formats, easing conversion into the video bitmaps.
  virtual int  GetCompatibleBitmapFormat(int color_depth) = 0;
  virtual IDriverDependantBitmap* CreateDDBFromBitmap(Common::Bitmap *bitmap, bool hasAlpha, bool opaque = false) = 0;
  virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Common::Bitmap *bitmap, bool hasAlpha) = 0;
  virtual void DestroyDDB(IDriverDependantBitmap* bitmap) = 0;

  // Prepares next sprite batch, a list of sprites with defined viewport and optional
  // global model transformation; all subsequent calls to DrawSprite will be adding
  // sprites to this batch's list.
  virtual void BeginSpriteBatch(const Rect &viewport, const SpriteTransform &transform, PBitmap surface = nullptr) = 0;
  // Adds sprite to the active batch
  virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap) = 0;
  // Clears all sprite batches, resets batch counter
  virtual void ClearDrawLists() = 0;

  virtual void SetScreenTint(int red, int green, int blue) = 0;
  // Defines the rendering offset of the every game sprite (in native coordinates).
  // TODO: should be replaced by defining translation for the sprite batch
  // (but translate transform does not work correctly enough at the moment and never truly used)
  // NOTE: currently this method is only used by ShakeScreen.
  virtual void SetNativeRenderOffset(int x, int y) = 0;
  virtual void RenderToBackBuffer() = 0;
  virtual void Render() = 0;
  virtual void Render(GlobalFlipType flip) = 0;
  // Copies contents of the game screen into bitmap using simple blit or pixel copy.
  // Bitmap must be of supported size and pixel format. If it's not the method will
  // fail and optionally write wanted destination format into 'want_fmt' pointer.
  virtual bool GetCopyOfScreenIntoBitmap(Common::Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt = nullptr) = 0;
  virtual void EnableVsyncBeforeRender(bool enabled) = 0;
  virtual void Vsync() = 0;
  // Enables or disables rendering mode that draws sprite list directly into
  // the final resolution, as opposed to drawing to native-resolution buffer
  // and scaling to final frame. The effect may be that sprites that are
  // drawn with additional fractional scaling will appear more detailed than
  // the rest of the game. The effect is stronger for the low-res games being
  // rendered in the high-res mode.
  virtual void RenderSpritesAtScreenResolution(bool enabled, int supersampling = 1) = 0;
  // TODO: move fade-in/out/boxout functions out of the graphics driver!! make everything render through
  // main drawing procedure. Since currently it does not - we need to init our own sprite batch
  // internally to let it set up correct viewport settings instead of relying on a chance.
  // Runs fade-out animation in a blocking manner.
  virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  // Runs fade-in animation in a blocking manner.
  virtual void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  // Runs box-out animation in a blocking manner.
  virtual void BoxOutEffect(bool blackingOut, int speed, int delay) = 0;
  virtual bool PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen) = 0;
  virtual void UseSmoothScaling(bool enabled) = 0;
  virtual bool SupportsGammaControl() = 0;
  virtual void SetGamma(int newGamma) = 0;
  // Returns the virtual screen. Will return NULL if renderer does not support memory backbuffer.
  // In normal case you should use GetStageBackBuffer() instead.
  virtual Common::Bitmap* GetMemoryBackBuffer() = 0;
  // Sets custom backbuffer bitmap to render to, optionally configure offsets at which this screen has to be blitted
  // to the final render surface. Passing NULL pointer will tell renderer to switch back to its original virtual screen.
  // Note that only software renderer supports this.
  virtual void SetMemoryBackBuffer(Common::Bitmap *backBuffer, int offx = 0, int offy = 0) = 0;
  // Returns memory backbuffer for the current rendering stage (or base virtual screen if called outside of render pass).
  // All renderers should support this.
  virtual Common::Bitmap* GetStageBackBuffer() = 0;
  virtual bool RequiresFullRedrawEachFrame() = 0;
  virtual bool HasAcceleratedTransform() = 0;
  virtual bool UsesMemoryBackBuffer() = 0;
  virtual ~IGraphicsDriver() = default;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GRAPHICSDRIVER_H

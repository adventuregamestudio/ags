//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Graphics driver interface
//
//=============================================================================
#ifndef __AGS_EE_GFX__GRAPHICSDRIVER_H
#define __AGS_EE_GFX__GRAPHICSDRIVER_H
#include <memory>
#include <allegro.h> // RGB, PALETTE
#include <glm/mat4x4.hpp>
#include "gfx/gfx_def.h"
#include "gfx/gfxdefines.h"
#include "gfx/gfxmodelist.h"
#include "util/geometry.h"

namespace AGS
{

namespace Common
{
    class Bitmap;
    typedef std::shared_ptr<Common::Bitmap> PBitmap;
}

namespace Engine
{

// Forward declaration
class IDriverDependantBitmap;
class IGfxFilter;
typedef std::shared_ptr<IGfxFilter> PGfxFilter;
using Common::PBitmap;

enum TintMethod
{
  TintReColourise = 0,
  TintSpecifyMaximum = 1
};

struct SpriteColorTransform
{
    int Alpha = 255; // alpha color value (0 - 255)

    SpriteColorTransform() = default;
    SpriteColorTransform(int alpha) : Alpha(alpha) {}
};

// Sprite transformation
// TODO: combine with stretch parameters in the IDriverDependantBitmap?
struct SpriteTransform
{
    // Translate
    int X = 0, Y = 0;
    float ScaleX = 1.f, ScaleY = 1.f;
    float Rotate = 0.f; // angle, in radians
    SpriteColorTransform Color;

    SpriteTransform() = default;
    SpriteTransform(int x, int y, float scalex = 1.0f, float scaley = 1.0f, float rotate = 0.0f,
        SpriteColorTransform color = SpriteColorTransform())
        : X(x), Y(y), ScaleX(scalex), ScaleY(scaley), Rotate(rotate), Color(color) {}
};

// Describes 3 render matrixes: world, view and projection
struct RenderMatrixes
{
    glm::mat4 World;
    glm::mat4 View;
    glm::mat4 Projection;
};


typedef void (*GFXDRV_CLIENTCALLBACK)();
typedef bool (*GFXDRV_CLIENTCALLBACKEVT)(int evt, int data);
typedef void (*GFXDRV_CLIENTCALLBACKINITGFX)(void *data);

class IGraphicsDriver
{
public:
  // Gets graphic driver's identifier
  virtual const char*GetDriverID() = 0;
  // Gets graphic driver's "friendly name"
  virtual const char *GetDriverName() = 0;
  virtual void SetTintMethod(TintMethod method) = 0;
  // Initialize given display mode
  virtual bool SetDisplayMode(const DisplayMode &mode) = 0;
  // Updates previously set display mode, accomodating to the new screen size
  virtual void UpdateDeviceScreen(const Size &screen_size) = 0;
  // Gets if a graphics mode was initialized
  virtual bool IsModeSet() const = 0;
  // Set the size of the native image size
  virtual bool SetNativeResolution(const GraphicResolution &native_res) = 0;
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
  // TODO: get rid of draw screen callback at some point when all fade functions are more or less grouped in one
  virtual void SetCallbackToDrawScreen(GFXDRV_CLIENTCALLBACK callback, GFXDRV_CLIENTCALLBACK post_callback) = 0;
  virtual void SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) = 0;
  // The event callback is called in the main render loop when a
  // event entry is encountered inside a sprite list.
  // You can use this to hook into the rendering process.
  virtual void SetCallbackOnSpriteEvt(GFXDRV_CLIENTCALLBACKEVT callback) = 0;
  // Clears the screen rectangle. The coordinates are expected in the **native game resolution**.
  virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) = 0;
  // Gets closest recommended bitmap format (currently - only color depth) for the given original format.
  // Engine needs to have game bitmaps brought to the certain range of formats, easing conversion into the video bitmaps.
  virtual int  GetCompatibleBitmapFormat(int color_depth) = 0;

  // Creates a "raw" DDB, without pixel initialization.
  virtual IDriverDependantBitmap *CreateDDB(int width, int height, int color_depth, bool opaque = false) = 0;
  // Creates DDB, initializes from the given bitmap.
  virtual IDriverDependantBitmap* CreateDDBFromBitmap(Common::Bitmap *bitmap, bool hasAlpha, bool opaque = false) = 0;
  // Creates DDB intended to be used as a render target (allow render other DDBs on it).
  virtual IDriverDependantBitmap* CreateRenderTargetDDB(int width, int height, int color_depth, bool opaque = false) = 0;
  // Updates DBB using the given bitmap; bitmap must have same size and format
  // as the one that this DDB was initialized with.
  virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Common::Bitmap *bitmap, bool hasAlpha) = 0;
  // Destroy the DDB.
  virtual void DestroyDDB(IDriverDependantBitmap* bitmap) = 0;

  // Get shared texture from cache, or create from bitmap and assign ID
  virtual IDriverDependantBitmap *GetSharedDDB(uint32_t sprite_id,
      Common::Bitmap *bitmap = nullptr, bool hasAlpha = true, bool opaque = false) = 0;
  virtual void UpdateSharedDDB(uint32_t sprite_id, Common::Bitmap *bitmap = nullptr, bool hasAlpha = true, bool opaque = false) = 0;
  // Removes the shared texture reference, will force the texture to recreate next time
  virtual void ClearSharedDDB(uint32_t sprite_id) = 0;

  // Prepares next sprite batch, a list of sprites with defined viewport and optional
  // global model transformation; all subsequent calls to DrawSprite will be adding
  // sprites to this batch's list.
  // Beginning a batch while the previous was not ended will create a sub-batch
  // (think of it as of a child scene node).
  // TODO: can we merge PBitmap surface and render_target from overriden method?
  virtual void BeginSpriteBatch(const Rect &viewport, const SpriteTransform &transform = SpriteTransform(),
      Common::GraphicFlip flip = Common::kFlip_None, PBitmap surface = nullptr) = 0;
  // Begins a sprite batch which will be rendered on a target texture.
  // This batch will ignore any parent transforms, regardless whether it's nested
  // or not. Its common child batches will also be rendered on the same texture.
  virtual void BeginSpriteBatch(IDriverDependantBitmap *render_target, const Rect &viewport, const SpriteTransform &transform,
      Common::GraphicFlip flip = Common::kFlip_None) = 0;
  // Ends current sprite batch
  virtual void EndSpriteBatch() = 0;
  // Adds sprite to the active batch
  virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap) = 0;
  // Adds fade overlay fx to the active batch
  virtual void SetScreenFade(int red, int green, int blue) = 0;
  // Adds tint overlay fx to the active batch
  // TODO: redesign this to allow various post-fx per sprite batch?
  virtual void SetScreenTint(int red, int green, int blue) = 0;
  // Sets stage screen parameters for the current batch.
  // Currently includes size and optional position offset;
  // the position is relative, as stage screens are using sprite batch transforms.
  // Stage screens are used to let plugins do raw drawing during render callbacks.
  // TODO: find a better term? note, it's used in several places around renderers.
  virtual void SetStageScreen(const Size &sz, int x = 0, int y = 0) = 0;
  // Clears all sprite batches, resets batch counter
  virtual void ClearDrawLists() = 0;
  virtual void RenderToBackBuffer() = 0;
  virtual void Render() = 0;
  // Renders with additional final offset and flip
  // TODO: leftover from old code, solely for software renderer; remove when
  // software mode either discarded or scene node graph properly implemented.
  virtual void Render(int xoff, int yoff, Common::GraphicFlip flip) = 0;
  // Copies contents of the game screen into bitmap using simple blit or pixel copy.
  // Bitmap must be of supported size and pixel format. If it's not the method will
  // fail and optionally write wanted destination format into 'want_fmt' pointer.
  virtual bool GetCopyOfScreenIntoBitmap(Common::Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt = nullptr) = 0;
  // Tells if the renderer supports toggling vsync after initializing the mode.
  virtual bool DoesSupportVsyncToggle() = 0;
  // Toggles vertical sync mode, if renderer supports one; returns the *new state*.
  virtual bool SetVsync(bool enabled) = 0;
  // Tells if the renderer currently has vsync enabled.
  virtual bool GetVsync() const = 0;
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
  virtual void UseSmoothScaling(bool enabled) = 0;
  virtual bool SupportsGammaControl() = 0;
  virtual void SetGamma(int newGamma) = 0;
  // Returns the virtual screen. Will return NULL if renderer does not support memory backbuffer.
  // In normal case you should use GetStageBackBuffer() instead.
  virtual Common::Bitmap* GetMemoryBackBuffer() = 0;
  // Sets custom backbuffer bitmap to render to.
  // Passing NULL pointer will tell renderer to switch back to its original virtual screen.
  // Note that only software renderer supports this.
  virtual void SetMemoryBackBuffer(Common::Bitmap *backBuffer) = 0;
  // Returns memory backbuffer for the current rendering stage (or base virtual screen if called outside of render pass).
  // All renderers should support this.
  virtual Common::Bitmap* GetStageBackBuffer(bool mark_dirty = false) = 0;
  // Sets custom backbuffer bitmap to render current render stage to.
  // Passing NULL pointer will tell renderer to switch back to its original stage buffer. 
  // Note that only software renderer supports this.
  virtual void SetStageBackBuffer(Common::Bitmap *backBuffer) = 0;
  // Retrieves 3 transform matrixes for the current rendering stage: world (model), view and projection.
  // These matrixes will be filled in accordance to the renderer's compatible format;
  // returns false if renderer does not use matrixes (not a 3D renderer).
  virtual bool GetStageMatrixes(RenderMatrixes &rm) = 0;
  // Tells if this gfx driver has to redraw whole scene each time
  virtual bool RequiresFullRedrawEachFrame() = 0;
  // Tells if this gfx driver uses GPU to transform sprites
  virtual bool HasAcceleratedTransform() = 0;
  // Tells if this gfx driver draws on a virtual screen before rendering on real screen.
  virtual bool UsesMemoryBackBuffer() = 0;
  // Tells if this gfx driver requires releasing render targets
  // in case of display mode change or reset.
  virtual bool ShouldReleaseRenderTargets() = 0;
  virtual ~IGraphicsDriver() = default;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GRAPHICSDRIVER_H

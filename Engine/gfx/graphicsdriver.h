
//=============================================================================
//
// Graphics driver interface
//
//=============================================================================
#ifndef __AGS_EE_GFX__GRAPHICSDRIVER_H
#define __AGS_EE_GFX__GRAPHICSDRIVER_H

namespace AGS
{

namespace Common { class IBitmap; }

namespace Engine
{

// Forward declaration
class IDriverDependantBitmap;

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
  virtual bool Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer) = 0;
  virtual bool Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer) = 0;
  virtual int  FindSupportedResolutionWidth(int idealWidth, int height, int colDepth, int widthRangeAllowed) = 0;
  virtual void SetCallbackForPolling(GFXDRV_CLIENTCALLBACK callback) = 0;
  virtual void SetCallbackToDrawScreen(GFXDRV_CLIENTCALLBACK callback) = 0;
  virtual void SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) = 0;
  // The NullSprite callback is called in the main render loop when a
  // null sprite is encountered. You can use this to hook into the rendering
  // process.
  virtual void SetCallbackForNullSprite(GFXDRV_CLIENTCALLBACKXY callback) = 0;
  virtual void UnInit() = 0;
  virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) = 0;
  virtual Common::IBitmap *ConvertBitmapToSupportedColourDepth(Common::IBitmap *bitmap) = 0;
  virtual IDriverDependantBitmap* CreateDDBFromBitmap(Common::IBitmap *bitmap, bool hasAlpha, bool opaque = false) = 0;
  virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Common::IBitmap *bitmap, bool hasAlpha) = 0;
  virtual void DestroyDDB(IDriverDependantBitmap* bitmap) = 0;
  virtual void ClearDrawList() = 0;
  virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap) = 0;
  virtual void SetScreenTint(int red, int green, int blue) = 0;
  virtual void SetRenderOffset(int x, int y) = 0;
  virtual void RenderToBackBuffer() = 0;
  virtual void Render() = 0;
  virtual void Render(GlobalFlipType flip) = 0;
  virtual void GetCopyOfScreenIntoBitmap(Common::IBitmap *destination) = 0;
  virtual void EnableVsyncBeforeRender(bool enabled) = 0;
  virtual void Vsync() = 0;
  virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  virtual void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue) = 0;
  virtual void BoxOutEffect(bool blackingOut, int speed, int delay) = 0;
  virtual bool PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen) = 0;
  virtual void UseSmoothScaling(bool enabled) = 0;
  virtual bool SupportsGammaControl() = 0;
  virtual void SetGamma(int newGamma) = 0;
  virtual Common::IBitmap* GetMemoryBackBuffer() = 0;
  virtual void SetMemoryBackBuffer(Common::IBitmap *backBuffer) = 0;
  virtual bool RequiresFullRedrawEachFrame() = 0;
  virtual bool HasAcceleratedStretchAndFlip() = 0;
  virtual bool UsesMemoryBackBuffer() = 0;
  virtual ~IGraphicsDriver() { }
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GRAPHICSDRIVER_H

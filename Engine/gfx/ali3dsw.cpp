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
// Allegro Interface for 3D; Software mode Allegro driver
//
//=============================================================================

#include <allegro.h>
#include "gfx/ali3d.h"
#include "platform/base/agsplatformdriver.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"

#include <stdio.h>

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;
using namespace AGS; // FIXME later

#if defined(PSP_VERSION)
// PSP: Includes for sceKernelDelayThread.
#include <pspsdk.h>
#include <pspthreadman.h>
#include <psputils.h>
#endif

#ifdef _WIN32
#include <winalleg.h>
extern int dxmedia_play_video (const char*, bool, int, int);
#include <ddraw.h>

typedef struct DDRAW_SURFACE {
   LPDIRECTDRAWSURFACE2 id;
   int flags;
   int lock_nesting;
   Bitmap *parent_bmp;  
   struct DDRAW_SURFACE *next;
   struct DDRAW_SURFACE *prev;
} DDRAW_SURFACE;

extern "C" extern LPDIRECTDRAW2 directdraw;
extern "C" DDRAW_SURFACE *gfx_directx_primary_surface;
#endif

#define MAX_DRAW_LIST_SIZE 200
RGB faded_out_palette[256];

void tint_image(Bitmap* srcimg, Bitmap* destimg, int red, int grn, int blu, int light_level, int luminance);
unsigned long _trans_alpha_blender32(unsigned long x, unsigned long y, unsigned long n);

class ALSoftwareBitmap : public IDriverDependantBitmap
{
public:
  virtual void SetTransparency(int transparency) { _transparency = transparency; }
  virtual void SetFlippedLeftRight(bool isFlipped) { _flipped = isFlipped; }
  virtual void SetStretch(int width, int height) 
  {
    _stretchToWidth = width;
    _stretchToHeight = height;
  }
  virtual int GetWidth() { return _width; }
  virtual int GetHeight() { return _height; }
  virtual int GetColorDepth() { return _colDepth; }
  virtual void SetLightLevel(int lightLevel)  { }
  virtual void SetTint(int red, int green, int blue, int tintSaturation) { }

  Bitmap _bmp;
  int _refCount;
  int _width, _height;
  int _colDepth;
  bool _flipped;
  int _stretchToWidth, _stretchToHeight;
  bool _opaque;
  bool _hasAlpha;
  int _transparency;

  ALSoftwareBitmap(Bitmap *bmp, bool opaque, bool hasAlpha)
  {
    _refCount = 1;
    _bmp.CreateReference(bmp);
    _width = bmp->GetWidth();
    _height = bmp->GetHeight();
    _colDepth = bmp->GetColorDepth();
    _flipped = false;
    _stretchToWidth = 0;
    _stretchToHeight = 0;
    _transparency = 0;
    _opaque = opaque;
    _hasAlpha = hasAlpha;
  }

  int GetWidthToRender() { return (_stretchToWidth > 0) ? _stretchToWidth : _width; }
  int GetHeightToRender() { return (_stretchToHeight > 0) ? _stretchToHeight : _height; }

  void Dispose()
  {
    // The internal bitmap data is now reference-counted, it is safe to delete Bitmap object
    _bmp.Destroy();
  }

  ~ALSoftwareBitmap()
  {
    Dispose();
  }

};

#include "gfx/gfxfilter_allegro.h"

class ALSoftwareGraphicsDriver : public IGraphicsDriver
{
public:
  ALSoftwareGraphicsDriver(AllegroGFXFilter *filter) { 
    _filter = filter; 
    _callback = NULL; 
    _drawScreenCallback = NULL;
    _nullSpriteCallback = NULL;
    _initGfxCallback = NULL;
    _global_x_offset = 0;
    _global_y_offset = 0;
    _tint_red = 0;
    _tint_green = 0;
    _tint_blue = 0;
    _autoVsync = false;
    _spareTintingScreen = NULL;
    numToDraw = 0;
    _gfxModeList = NULL;
#ifdef _WIN32
    dxGammaControl = NULL;
#endif
    _allegroScreenWrapper = NULL;
  }

  virtual const char*GetDriverName() { return "Allegro/DX5"; }
  virtual const char*GetDriverID() { return "DX5"; }
  virtual void SetTintMethod(TintMethod method);
  virtual bool Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer);
  virtual bool Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer);
  virtual int  FindSupportedResolutionWidth(int idealWidth, int height, int colDepth, int widthRangeAllowed);
  virtual void SetCallbackForPolling(GFXDRV_CLIENTCALLBACK callback) { _callback = callback; }
  virtual void SetCallbackToDrawScreen(GFXDRV_CLIENTCALLBACK callback) { _drawScreenCallback = callback; }
  virtual void SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) { _initGfxCallback = callback; }
  virtual void SetCallbackForNullSprite(GFXDRV_CLIENTCALLBACKXY callback) { _nullSpriteCallback = callback; }
  virtual void UnInit();
  virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse);
  virtual Bitmap *ConvertBitmapToSupportedColourDepth(Bitmap *bitmap);
  virtual IDriverDependantBitmap* CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque);
  virtual IDriverDependantBitmap* CreateDDBReference(IDriverDependantBitmap *ddb);
  virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha);
  virtual void DestroyDDB(IDriverDependantBitmap* bitmap);
  virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap);
  virtual void ClearDrawList();
  virtual void SetRenderOffset(int x, int y);
  virtual void RenderToBackBuffer();
  virtual void Render();
  virtual void Render(GlobalFlipType flip);
  virtual void GetCopyOfScreenIntoBitmap(Bitmap *destination);
  virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
  virtual void FadeIn(int speed, PALETTE pal, int targetColourRed, int targetColourGreen, int targetColourBlue);
  virtual void BoxOutEffect(bool blackingOut, int speed, int delay);
  virtual bool PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen);
  virtual bool SupportsGammaControl() ;
  virtual void SetGamma(int newGamma);
  virtual void UseSmoothScaling(bool enabled) { }
  virtual void EnableVsyncBeforeRender(bool enabled) { _autoVsync = enabled; }
  virtual void Vsync();
  virtual bool RequiresFullRedrawEachFrame() { return false; }
  virtual bool HasAcceleratedStretchAndFlip() { return false; }
  virtual bool UsesMemoryBackBuffer() { return true; }
  virtual Bitmap *GetMemoryBackBuffer() { return virtualScreen; }
  virtual void SetMemoryBackBuffer(Bitmap *backBuffer) { virtualScreen = backBuffer; }
  virtual void SetScreenTint(int red, int green, int blue) { 
    _tint_red = red; _tint_green = green; _tint_blue = blue; }
  virtual ~ALSoftwareGraphicsDriver();

  AllegroGFXFilter *_filter;

private:
  volatile int* _loopTimer;
  int _screenWidth, _screenHeight;
  int _colorDepth;
  bool _windowed;
  bool _autoVsync;
  Bitmap *_allegroScreenWrapper;
  Bitmap *virtualScreen;
  Bitmap *_spareTintingScreen;
  GFXDRV_CLIENTCALLBACK _callback;
  GFXDRV_CLIENTCALLBACK _drawScreenCallback;
  GFXDRV_CLIENTCALLBACKXY _nullSpriteCallback;
  GFXDRV_CLIENTCALLBACKINITGFX _initGfxCallback;
  int _tint_red, _tint_green, _tint_blue;
  int _global_x_offset, _global_y_offset;

  ALSoftwareBitmap* drawlist[MAX_DRAW_LIST_SIZE];
  int drawx[MAX_DRAW_LIST_SIZE], drawy[MAX_DRAW_LIST_SIZE];
  int numToDraw;
  GFX_MODE_LIST *_gfxModeList;

#ifdef _WIN32
  IDirectDrawGammaControl* dxGammaControl;
  // The gamma ramp is a lookup table for each possible R, G and B value
  // in 32-bit colour (from 0-255) it maps them to a brightness value
  // from 0-65535. The default gamma ramp just multiplies each value by 256
  DDGAMMARAMP gammaRamp;
  DDGAMMARAMP defaultGammaRamp;
  DDCAPS ddrawCaps;
#endif

  void draw_sprite_with_transparency(Bitmap *piccy, int xxx, int yyy, int transparency);
  void highcolor_fade_out(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
  void highcolor_fade_in(Bitmap *bmp_orig, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
  void __fade_from_range(PALLETE source, PALLETE dest, int speed, int from, int to) ;
  void __fade_out_range(int speed, int from, int to, int targetColourRed, int targetColourGreen, int targetColourBlue) ;
  bool IsModeSupported(int driver, int width, int height, int colDepth);
  int  GetAllegroGfxDriverID(bool windowed);
};

bool ALSoftwareGraphicsDriver::IsModeSupported(int driver, int width, int height, int colDepth)
{
#if defined(ANDROID_VERSION) || defined(PSP_VERSION) || defined(IOS_VERSION)
  // Everything is drawn to a virtual screen, so all resolutions are supported.
  return true;
#endif

  if (_windowed)
  {
    return true;
  }
  if (_gfxModeList == NULL)
  {
    _gfxModeList = get_gfx_mode_list(driver);
  }
  if (_gfxModeList != NULL)
  {
    // if a list is available, check if the mode exists. This prevents the screen flicking
    // between loads of unsupported resolutions
    for (int i = 0; i < _gfxModeList->num_modes; i++)
    {
      if ((_gfxModeList->mode[i].width == width) &&
        (_gfxModeList->mode[i].height == height) &&
        (_gfxModeList->mode[i].bpp == colDepth))
      {
        return true;
      }
    }
    strcpy(allegro_error, "This graphics mode is not supported");
    return false;
  }
  return true;
}

int ALSoftwareGraphicsDriver::FindSupportedResolutionWidth(int idealWidth, int height, int colDepth, int widthRangeAllowed)
{
  if (_gfxModeList == NULL)
  {
    _gfxModeList = get_gfx_mode_list(GetAllegroGfxDriverID(false));
  }
  if (_gfxModeList != NULL)
  {
    int unfilteredWidth = idealWidth;
    _filter->GetRealResolution(&idealWidth, &height);
    int filterFactor = idealWidth / unfilteredWidth;

    int nearestWidthFound = 0;

    for (int i = 0; i < _gfxModeList->num_modes; i++)
    {
      if ((_gfxModeList->mode[i].height == height) &&
          (_gfxModeList->mode[i].bpp == colDepth))
      {
        if (_gfxModeList->mode[i].width == idealWidth)
          return idealWidth / filterFactor;

        if (abs(_gfxModeList->mode[i].width - idealWidth) <
            abs(nearestWidthFound - idealWidth))
        {
          nearestWidthFound = _gfxModeList->mode[i].width;
        }
      }
    }

    if (abs(nearestWidthFound - idealWidth) <= widthRangeAllowed * filterFactor)
      return nearestWidthFound / filterFactor;
  }
  return 0;
}

int ALSoftwareGraphicsDriver::GetAllegroGfxDriverID(bool windowed)
{
#ifdef _WIN32
  if (windowed)
    return GFX_DIRECTX_WIN;
  return GFX_DIRECTX;
#elif defined (LINUX_VERSION) && (!defined (ALLEGRO_MAGIC_DRV))
  if (windowed)
    return GFX_XWINDOWS;
  return GFX_XWINDOWS_FULLSCREEN;
#else
  if (windowed)
    return GFX_AUTODETECT_WINDOWED;
  return GFX_AUTODETECT_FULLSCREEN;
#endif
}

void ALSoftwareGraphicsDriver::SetTintMethod(TintMethod method) 
{
  // TODO: support new D3D-style tint method
}

bool ALSoftwareGraphicsDriver::Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer)
{
  throw Ali3DException("this overload is not supported, you must use the normal Init method");
}

bool ALSoftwareGraphicsDriver::Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer)
{
  _screenWidth = width;
  _screenHeight = height;
  _colorDepth = colourDepth;
  _windowed = windowed;
  _loopTimer = loopTimer;
  int driver = GetAllegroGfxDriverID(windowed);

  set_color_depth(colourDepth);
  int actualInitWid = width, actualInitHit = height;
  _filter->GetRealResolution(&actualInitWid, &actualInitHit);

  if (_initGfxCallback != NULL)
    _initGfxCallback(NULL);

  if ((IsModeSupported(driver, actualInitWid, actualInitHit, colourDepth)) &&
      (set_gfx_mode(driver, actualInitWid, actualInitHit, 0, 0) == 0))
  {
    // [IKM] 2012-09-07
    // set_gfx_mode is an allegro function that creates screen bitmap;
    // following code assumes the screen is already created, therefore we should
    // ensure global bitmap wraps over existing allegro screen bitmap.
    _allegroScreenWrapper = BitmapHelper::CreateRawBitmapWrapper(screen);
    BitmapHelper::SetScreenBitmap( _allegroScreenWrapper );

    BitmapHelper::GetScreenBitmap()->Clear();
    BitmapHelper::SetScreenBitmap( _filter->ScreenInitialized(BitmapHelper::GetScreenBitmap(), width, height) );

    // [IKM] 2012-09-07
    // At this point the wrapper we created is saved by filter for future reference,
    // therefore we should not delete it right away, but only at driver shutdown.

    virtualScreen = BitmapHelper::GetScreenBitmap();

#ifdef _WIN32
    if (!windowed)
    {
      memset(&ddrawCaps, 0, sizeof(ddrawCaps));
      ddrawCaps.dwSize = sizeof(ddrawCaps);
      IDirectDraw2_GetCaps(directdraw, &ddrawCaps, NULL);

      if ((ddrawCaps.dwCaps2 & DDCAPS2_PRIMARYGAMMA) == 0) { }
      else if (IDirectDrawSurface2_QueryInterface(gfx_directx_primary_surface->id, IID_IDirectDrawGammaControl, (void **)&dxGammaControl) == 0) 
      {
        dxGammaControl->GetGammaRamp(0, &defaultGammaRamp);
      }
    }
#endif

    return true;
  }

  return false;
}

void ALSoftwareGraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse)
{
  int color = 0;
  if (colorToUse != NULL) 
    color = makecol_depth(this->_colorDepth, colorToUse->r, colorToUse->g, colorToUse->b);
  _filter->ClearRect(x1, y1, x2, y2, color);
}

ALSoftwareGraphicsDriver::~ALSoftwareGraphicsDriver()
{
}

void ALSoftwareGraphicsDriver::UnInit()
{
#ifdef _WIN32

  if (dxGammaControl != NULL) 
  {
    dxGammaControl->Release();
    dxGammaControl = NULL;
  }

#endif

  if (_gfxModeList != NULL)
  {
    destroy_gfx_mode_list(_gfxModeList);
    _gfxModeList = NULL;
  }

  if (BitmapHelper::GetScreenBitmap())
   BitmapHelper::SetScreenBitmap( _filter->ShutdownAndReturnRealScreen(BitmapHelper::GetScreenBitmap()) );

  // [IKM] 2012-09-07
  // We do not need the wrapper any longer;
  // this does not destroy the underlying allegro screen bitmap, only wrapper.
  delete _allegroScreenWrapper;
  _allegroScreenWrapper = NULL;
  // Nullify the global screen object (for safety reasons); note this yet does
  // not change allegro screen pointer (at this moment it should point at the
  // original internally created allegro bitmap which will be destroyed by Allegro).
  BitmapHelper::SetScreenBitmap(NULL);

  // don't do anything else -- the main app may
  // already have called allegro_exit
}

bool ALSoftwareGraphicsDriver::SupportsGammaControl() 
{
#ifdef _WIN32

  if (dxGammaControl != NULL) 
  {
    return 1;
  }

#endif

  return 0;
}

void ALSoftwareGraphicsDriver::SetGamma(int newGamma)
{
#ifdef _WIN32
  for (int i = 0; i < 256; i++) {
    int newValue = ((int)defaultGammaRamp.red[i] * newGamma) / 100;
    if (newValue >= 65535)
      newValue = 65535;
    gammaRamp.red[i] = newValue;
    gammaRamp.green[i] = newValue;
    gammaRamp.blue[i] = newValue;
  }

  dxGammaControl->SetGammaRamp(0, &gammaRamp);
#endif
}

Bitmap *ALSoftwareGraphicsDriver::ConvertBitmapToSupportedColourDepth(Bitmap *bitmap)
{
  return bitmap;
}

IDriverDependantBitmap* ALSoftwareGraphicsDriver::CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque)
{
  ALSoftwareBitmap* newBitmap = new ALSoftwareBitmap(bitmap, opaque, hasAlpha);
  return newBitmap;
}

IDriverDependantBitmap* ALSoftwareGraphicsDriver::CreateDDBReference(IDriverDependantBitmap *ddb)
{
  ALSoftwareBitmap* alSwBmp = (ALSoftwareBitmap*)ddb;
  alSwBmp->_refCount++;
  return alSwBmp;
}

void ALSoftwareGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha)
{
  ALSoftwareBitmap* alSwBmp = (ALSoftwareBitmap*)bitmapToUpdate;
  alSwBmp->_bmp.CreateReference(bitmap);
  alSwBmp->_hasAlpha = hasAlpha;
}

void ALSoftwareGraphicsDriver::DestroyDDB(IDriverDependantBitmap* bitmap)
{
  ALSoftwareBitmap* bmpToDelete = (ALSoftwareBitmap*)bitmap;
  bmpToDelete->_refCount--;
  if (!bmpToDelete->_refCount)
  {
    delete bmpToDelete;
  }
}

void ALSoftwareGraphicsDriver::DrawSprite(int x, int y, IDriverDependantBitmap* bitmap)
{
  if (numToDraw >= MAX_DRAW_LIST_SIZE)
  {
    throw Ali3DException("Too many sprites to draw in one frame");
  }

  drawlist[numToDraw] = (ALSoftwareBitmap*)bitmap;
  drawx[numToDraw] = x;
  drawy[numToDraw] = y;
  numToDraw++;
}

void ALSoftwareGraphicsDriver::ClearDrawList()
{
  numToDraw = 0;
}

void ALSoftwareGraphicsDriver::draw_sprite_with_transparency(Bitmap *piccy, int xxx, int yyy, int transparency)
{
  int screen_depth = virtualScreen->GetColorDepth();
  int sprite_depth = piccy->GetColorDepth();

  if (sprite_depth < screen_depth) {

    if ((sprite_depth == 8) && (screen_depth >= 24)) {
      // 256-col sprite -> truecolor background
      // this is automatically supported by allegro, no twiddling needed
      virtualScreen->Blit(piccy, xxx, yyy, Common::kBitmap_Transparency);
      return;
    }
    // 256-col spirte -> hi-color background, or
    // 16-bit sprite -> 32-bit background
    Bitmap* hctemp=BitmapHelper::CreateBitmapCopy(piccy, screen_depth);
    color_t mask_col = virtualScreen->GetMaskColor();

    if (sprite_depth == 8) {
      // only do this for 256-col, cos the Blit call converts
      // transparency for 16->32 bit
      for (int y = 0; y < hctemp->GetHeight(); ++y)
      {
          const uint8_t *src_scanline = piccy->GetScanLine(y);
          uint8_t *dst_scanline = hctemp->GetScanLineForWriting(y);
          for (int x = 0; x < hctemp->GetWidth(); ++x)
          {
              if (src_scanline[x] == 0)
              {
                  dst_scanline[x] = mask_col;
              }
          }
      }
    }

    virtualScreen->Blit(hctemp, xxx, yyy, Common::kBitmap_Transparency);
    delete hctemp;
  }
  else
  {
    if ((transparency != 0) && (screen_depth > 8) &&
        (sprite_depth > 8) && (virtualScreen->GetColorDepth() > 8)) 
    {
      set_trans_blender(0,0,0, transparency);
	  virtualScreen->TransBlendBlt(piccy, xxx, yyy);
    }
    else
      virtualScreen->Blit(piccy, xxx, yyy, Common::kBitmap_Transparency);
  }
  
}

void ALSoftwareGraphicsDriver::SetRenderOffset(int x, int y)
{
  _global_x_offset = x;
  _global_y_offset = y;
}

void ALSoftwareGraphicsDriver::RenderToBackBuffer()
{
  for (int i = 0; i < numToDraw; i++)
  {
    if (drawlist[i] == NULL)
    {
      if (_nullSpriteCallback)
        _nullSpriteCallback(drawx[i], drawy[i]);
      else
        throw Ali3DException("Unhandled attempt to draw null sprite");

      continue;
    }

    ALSoftwareBitmap* bitmap = drawlist[i];
    int drawAtX = drawx[i];// + x;
    int drawAtY = drawy[i];// + y;

    if ((bitmap->_opaque) && (bitmap->_bmp == *virtualScreen))
    { }
    else if (bitmap->_opaque)
    {
      virtualScreen->Blit(&bitmap->_bmp, 0, 0, drawAtX, drawAtY, bitmap->_bmp.GetWidth(), bitmap->_bmp.GetHeight());
    }
    else if (bitmap->_transparency >= 255)
    {
      // fully transparent... invisible, do nothing
    }
    else if (bitmap->_hasAlpha)
    {
      if (bitmap->_transparency == 0)
        set_alpha_blender();
      else
        set_blender_mode(NULL, NULL, _trans_alpha_blender32, 0, 0, 0, bitmap->_transparency);

	  virtualScreen->TransBlendBlt(&bitmap->_bmp, drawAtX, drawAtY);
    }
    else
    {
      draw_sprite_with_transparency(&bitmap->_bmp, drawAtX, drawAtY, bitmap->_transparency);
    }
  }

  if (((_tint_red > 0) || (_tint_green > 0) || (_tint_blue > 0))
      && (_colorDepth > 8)) {
    // Common::gl_ScreenBmp tint
    // This slows down the game no end, only experimental ATM
    set_trans_blender(_tint_red, _tint_green, _tint_blue, 0);
    virtualScreen->LitBlendBlt(virtualScreen, 0, 0, 128);
/*  This alternate method gives the correct (D3D-style) result, but is just too slow!
    if ((_spareTintingScreen != NULL) &&
        ((_spareTintingScreen->GetWidth() != virtualScreen->GetWidth()) || (_spareTintingScreen->GetHeight() != virtualScreen->GetHeight())))
    {
      destroy_bitmap(_spareTintingScreen);
      _spareTintingScreen = NULL;
    }
    if (_spareTintingScreen == NULL)
    {
      _spareTintingScreen = BitmapHelper::CreateBitmap_(GetColorDepth(virtualScreen), virtualScreen->GetWidth(), virtualScreen->GetHeight());
    }
    tint_image(virtualScreen, _spareTintingScreen, _tint_red, _tint_green, _tint_blue, 100, 255);
    Blit(_spareTintingScreen, virtualScreen, 0, 0, 0, 0, _spareTintingScreen->GetWidth(), _spareTintingScreen->GetHeight());*/
  }

  ClearDrawList();
}

void ALSoftwareGraphicsDriver::Render(GlobalFlipType flip)
{
  RenderToBackBuffer();

  if (_autoVsync)
    this->Vsync();

  if (flip == None)
    _filter->RenderScreen(virtualScreen, _global_x_offset, _global_y_offset);
  else
    _filter->RenderScreenFlipped(virtualScreen, _global_x_offset, _global_y_offset, (int)flip);
}

void ALSoftwareGraphicsDriver::Render()
{
  Render(None);
}

void ALSoftwareGraphicsDriver::Vsync()
{
  vsync();
}

void ALSoftwareGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination)
{
  _filter->GetCopyOfScreenIntoBitmap(destination);
}

/**
	fade.c - High Color Fading Routines

	Last Revision: 21 June, 2002

	Author: Matthew Leverton
**/
void ALSoftwareGraphicsDriver::highcolor_fade_in(Bitmap *currentVirtScreen, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
   Bitmap *bmp_buff;
   Bitmap *bmp_orig = currentVirtScreen;

   if ((_global_y_offset != 0) || (_global_x_offset != 0))
   {
     bmp_orig = BitmapHelper::CreateBitmap(_screenWidth, _screenHeight);
     bmp_orig->Fill(0);
     bmp_orig->Blit(currentVirtScreen, 0, 0, _global_x_offset, _global_y_offset, currentVirtScreen->GetWidth(), currentVirtScreen->GetHeight());
   }

   bmp_buff = BitmapHelper::CreateBitmap(bmp_orig->GetWidth(), bmp_orig->GetHeight());
   int clearColor = makecol_depth(bmp_buff->GetColorDepth(),
				targetColourRed, targetColourGreen, targetColourBlue);

   int a;
   if (speed <= 0) speed = 16;

   for (a = 0; a < 256; a+=speed)
   {
       int timerValue = *_loopTimer;
       bmp_buff->Fill(clearColor);
       set_trans_blender(0,0,0,a);
       bmp_buff->TransBlendBlt(bmp_orig, 0, 0);
       this->Vsync();
       _filter->RenderScreen(bmp_buff, 0, 0);
       do
       {
         if (_callback)
           _callback();
         platform->Delay(1);
       }
       while (timerValue == *_loopTimer);
   }
   delete bmp_buff;

   _filter->RenderScreen(currentVirtScreen, _global_x_offset, _global_y_offset);

   if ((_global_y_offset != 0) || (_global_x_offset != 0))
     delete bmp_orig;
}

void ALSoftwareGraphicsDriver::highcolor_fade_out(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
    Bitmap *bmp_orig, *bmp_buff;

    int clearColor = makecol_depth(BitmapHelper::GetScreenBitmap()->GetColorDepth(),
				targetColourRed, targetColourGreen, targetColourBlue);

    if ((bmp_orig = BitmapHelper::CreateBitmap(_screenWidth, _screenHeight)))
    {
        if ((bmp_buff = BitmapHelper::CreateBitmap(bmp_orig->GetWidth(), bmp_orig->GetHeight())))
        {
            int a;
            _filter->GetCopyOfScreenIntoBitmap(bmp_orig, false);
            if (speed <= 0) speed = 16;
			
            for (a = 255-speed; a > 0; a-=speed)
            {
                int timerValue = *_loopTimer;
                bmp_buff->Fill(clearColor);
                set_trans_blender(0,0,0,a);
                bmp_buff->TransBlendBlt(bmp_orig, 0, 0);
                this->Vsync();
                _filter->RenderScreen(bmp_buff, 0, 0);
                do
                {
                  if (_callback)
                    _callback();
                  platform->Delay(1);
                }
                while (timerValue == *_loopTimer);
            }
            delete bmp_buff;
        }
        delete bmp_orig;
    }

    BitmapHelper::GetScreenBitmap()->Clear(clearColor);
	_filter->RenderScreen(BitmapHelper::GetScreenBitmap(), _global_x_offset, _global_y_offset);
}
/** END FADE.C **/

// palette fading routiens
// from allegro, modified for mp3
void initialize_fade_256(int r, int g, int b) {
  int a;
  for (a = 0; a < 256; a++) {
    faded_out_palette[a].r = r / 4;
	  faded_out_palette[a].g = g / 4;
	  faded_out_palette[a].b = b / 4;
  }
}

void ALSoftwareGraphicsDriver::__fade_from_range(PALLETE source, PALLETE dest, int speed, int from, int to) 
{
   PALLETE temp;
   int c;

   for (c=0; c<PAL_SIZE; c++)
      temp[c] = source[c];

   for (c=0; c<64; c+=speed) {
      fade_interpolate(source, dest, temp, c, from, to);
      set_pallete_range(temp, from, to, TRUE);
      if (_callback) _callback();
      set_pallete_range(temp, from, to, TRUE);
   }

   set_pallete_range(dest, from, to, TRUE);
}

void ALSoftwareGraphicsDriver::__fade_out_range(int speed, int from, int to, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
   PALLETE temp;

   initialize_fade_256(targetColourRed, targetColourGreen, targetColourBlue);
   get_pallete(temp);
   __fade_from_range(temp, faded_out_palette, speed, from, to);
}


void ALSoftwareGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) {

  if (_colorDepth > 8) 
  {
    highcolor_fade_out(speed * 4, targetColourRed, targetColourGreen, targetColourBlue);
  }
  else __fade_out_range(speed, 0, 255, targetColourRed, targetColourGreen, targetColourBlue);

}

void ALSoftwareGraphicsDriver::FadeIn(int speed, PALLETE p, int targetColourRed, int targetColourGreen, int targetColourBlue) {
  if (_colorDepth > 8) {

    highcolor_fade_in(virtualScreen, speed * 4, targetColourRed, targetColourGreen, targetColourBlue);
  }
  else {
	  initialize_fade_256(targetColourRed, targetColourGreen, targetColourBlue);
	  __fade_from_range(faded_out_palette, p, speed, 0,255);
  }
}

void ALSoftwareGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
{
  if (blackingOut)
  {
    int yspeed = _screenHeight / (_screenWidth / speed);
    int boxwid = speed, boxhit = yspeed;

    while (boxwid < _screenWidth) {
      boxwid += speed;
      boxhit += yspeed;
      this->Vsync();
      int vcentre = _screenHeight / 2;
      this->ClearRectangle(_screenWidth / 2 - boxwid / 2, vcentre - boxhit / 2,
          _screenWidth / 2 + boxwid / 2, vcentre + boxhit / 2, NULL);
    
      if (_callback)
        _callback();

      platform->Delay(delay);
    }
    this->ClearRectangle(0, 0, _screenWidth - 1, _screenHeight - 1, NULL);
  }
  else
  {
    throw Ali3DException("BoxOut fade-in not implemented in sw gfx driver");
  }
}
// end fading routines

bool ALSoftwareGraphicsDriver::PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen)
{
#ifdef _WIN32
  int result = dxmedia_play_video(filename, useAVISound, skipType, stretchToFullScreen ? 1 : 0);
  return (result == 0);
#else
#warning ffmpeg implementation needed
  return 0;
#endif
}

// add the alpha values together, used for compositing alpha images
unsigned long _trans_alpha_blender32(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long res, g;

   n = (n * geta32(x)) / 256;

   if (n)
      n++;

   res = ((x & 0xFF00FF) - (y & 0xFF00FF)) * n / 256 + y;
   y &= 0xFF00;
   x &= 0xFF00;
   g = (x - y) * n / 256 + y;

   res &= 0xFF00FF;
   g &= 0xFF00;

   return res | g;
}

static ALSoftwareGraphicsDriver *_alsoftware_driver = NULL;

IGraphicsDriver* GetSoftwareGraphicsDriver(GFXFilter *filter)
{
  AllegroGFXFilter* allegroFilter = (AllegroGFXFilter*)filter;

  if (_alsoftware_driver == NULL)
  {
    _alsoftware_driver = new ALSoftwareGraphicsDriver(allegroFilter);
  }
  else if (_alsoftware_driver->_filter != filter)
  {
    delete _alsoftware_driver;
    _alsoftware_driver = new ALSoftwareGraphicsDriver(allegroFilter);
  }

  return _alsoftware_driver;
}


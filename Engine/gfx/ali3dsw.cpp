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

#include "gfx/ali3dexception.h"
#include "gfx/ali3dsw.h"
#include "gfx/gfxfilter_allegro.h"
#include "gfx/gfxfilter_hqx.h"
#include "gfx/gfx_util.h"
#include "main/main_allegro.h"
#include "platform/base/agsplatformdriver.h"

#if defined(PSP_VERSION)
// PSP: Includes for sceKernelDelayThread.
#include <pspsdk.h>
#include <pspthreadman.h>
#include <psputils.h>
#endif

#if defined (WINDOWS_VERSION)
// NOTE: this struct and variables are defined internally in Allegro
typedef struct DDRAW_SURFACE {
    LPDIRECTDRAWSURFACE2 id;
    int flags;
    int lock_nesting;
    BITMAP *parent_bmp;  
    struct DDRAW_SURFACE *next;
    struct DDRAW_SURFACE *prev;
} DDRAW_SURFACE;

extern "C" extern LPDIRECTDRAW2 directdraw;
extern "C" DDRAW_SURFACE *gfx_directx_primary_surface;
extern int dxmedia_play_video (const char*, bool, int, int);
#endif // WINDOWS_VERSION

namespace AGS
{
namespace Engine
{
namespace ALSW
{

using namespace Common;

bool ALSoftwareGfxModeList::GetMode(int index, DisplayMode &mode) const
{
    if (_gfxModeList && index >= 0 && index < _gfxModeList->num_modes)
    {
        mode.Width = _gfxModeList->mode[index].width;
        mode.Height = _gfxModeList->mode[index].height;
        mode.ColorDepth = _gfxModeList->mode[index].bpp;
        return true;
    }
    return false;
}

unsigned long _trans_alpha_blender32(unsigned long x, unsigned long y, unsigned long n);
RGB faded_out_palette[256];


ALSoftwareGraphicsDriver::ALSoftwareGraphicsDriver()
{
  _tint_red = 0;
  _tint_green = 0;
  _tint_blue = 0;
  _autoVsync = false;
  _spareTintingScreen = NULL;
  _gfxModeList = NULL;
#ifdef _WIN32
  dxGammaControl = NULL;
#endif
  _allegroScreenWrapper = NULL;
  _origVirtualScreen = NULL;
  virtualScreen = NULL;
  _stageVirtualScreen = NULL;

  // Initialize default sprite batch, it will be used when no other batch was activated
  InitSpriteBatch(0, _spriteBatchDesc[0]);
}

bool ALSoftwareGraphicsDriver::IsModeSupported(const DisplayMode &mode)
{
  if (mode.Width <= 0 || mode.Height <= 0 || mode.ColorDepth <= 0)
  {
    set_allegro_error("Invalid resolution parameters: %d x %d x %d", mode.Width, mode.Height, mode.ColorDepth);
    return false;
  }
#if defined(ANDROID_VERSION) || defined(PSP_VERSION) || defined(IOS_VERSION) || defined(MAC_VERSION)
  // Everything is drawn to a virtual screen, so all resolutions are supported.
  return true;
#endif

  if (mode.Windowed)
  {
    return true;
  }
  if (_gfxModeList == NULL)
  {
    _gfxModeList = get_gfx_mode_list(GetAllegroGfxDriverID(mode.Windowed));
  }
  if (_gfxModeList != NULL)
  {
    // if a list is available, check if the mode exists. This prevents the screen flicking
    // between loads of unsupported resolutions
    for (int i = 0; i < _gfxModeList->num_modes; i++)
    {
      if ((_gfxModeList->mode[i].width == mode.Width) &&
        (_gfxModeList->mode[i].height == mode.Height) &&
        (_gfxModeList->mode[i].bpp == mode.ColorDepth))
      {
        return true;
      }
    }
    set_allegro_error("This graphics mode is not supported");
    return false;
  }
  return true;
}

int ALSoftwareGraphicsDriver::GetDisplayDepthForNativeDepth(int native_color_depth) const
{
    // TODO: check for device caps to know which depth is supported?
    if (native_color_depth > 8)
        return 32;
    return native_color_depth;
}

IGfxModeList *ALSoftwareGraphicsDriver::GetSupportedModeList(int color_depth)
{
  if (_gfxModeList == NULL)
  {
    _gfxModeList = get_gfx_mode_list(GetAllegroGfxDriverID(false));
  }
  if (_gfxModeList == NULL)
  {
    return NULL;
  }
  return new ALSoftwareGfxModeList(_gfxModeList);
}

PGfxFilter ALSoftwareGraphicsDriver::GetGraphicsFilter() const
{
    return _filter;
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
#elif defined (MAC_VERSION)
    if (windowed) {
        return GFX_COCOAGL_WINDOW;
    }
    return GFX_COCOAGL_FULLSCREEN;
#else
  if (windowed)
    return GFX_AUTODETECT_WINDOWED;
  return GFX_AUTODETECT_FULLSCREEN;
#endif
}

void ALSoftwareGraphicsDriver::SetGraphicsFilter(PALSWFilter filter)
{
  _filter = filter;
  OnSetFilter();

  // If we already have a gfx mode set, then use the new filter to update virtual screen immediately
  CreateVirtualScreen();
}

void ALSoftwareGraphicsDriver::SetTintMethod(TintMethod method) 
{
  // TODO: support new D3D-style tint method
}

bool ALSoftwareGraphicsDriver::SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer)
{
  ReleaseDisplayMode();

  const int driver = GetAllegroGfxDriverID(mode.Windowed);

  set_color_depth(mode.ColorDepth);

  if (_initGfxCallback != NULL)
    _initGfxCallback(NULL);

  if (!IsModeSupported(mode) || set_gfx_mode(driver, mode.Width, mode.Height, 0, 0) != 0)
    return false;

  OnInit(loopTimer);
  OnModeSet(mode);
  // [IKM] 2012-09-07
  // set_gfx_mode is an allegro function that creates screen bitmap;
  // following code assumes the screen is already created, therefore we should
  // ensure global bitmap wraps over existing allegro screen bitmap.
  _allegroScreenWrapper = BitmapHelper::CreateRawBitmapWrapper(screen);
  BitmapHelper::SetScreenBitmap( _allegroScreenWrapper );
  BitmapHelper::GetScreenBitmap()->Clear();

  // [IKM] 2012-09-07
  // The wrapper we created will be saved by filter for future reference,
  // therefore we should not delete it until driver shutdown.

  // If we already have a gfx filter, then use it to update virtual screen immediately
  CreateVirtualScreen();

#ifdef _WIN32
  if (!mode.Windowed)
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

void ALSoftwareGraphicsDriver::CreateVirtualScreen()
{
  if (!IsModeSet() || !IsRenderFrameValid() || !IsNativeSizeValid() || !_filter)
    return;
  DestroyVirtualScreen();
  // Adjust clipping so nothing gets drawn outside the game frame
  Bitmap *real_screen = BitmapHelper::GetScreenBitmap();
  real_screen->SetClip(_dstRect);
  // Initialize scaling filter and receive virtual screen pointer
  // (which may or not be the same as real screen)
  _origVirtualScreen = _filter->InitVirtualScreen(real_screen, _srcRect.GetSize(), _dstRect);
  BitmapHelper::SetScreenBitmap(_origVirtualScreen);
  virtualScreen = _origVirtualScreen;
  _stageVirtualScreen = virtualScreen;
}

void ALSoftwareGraphicsDriver::DestroyVirtualScreen()
{
  if (_filter && _origVirtualScreen)
  {
    BitmapHelper::SetScreenBitmap(_filter->ShutdownAndReturnRealScreen());
  }
  _origVirtualScreen = NULL;
  virtualScreen = NULL;
  _stageVirtualScreen = NULL;
}

void ALSoftwareGraphicsDriver::ReleaseDisplayMode()
{
  OnModeReleased();
  ClearDrawLists();

#ifdef _WIN32
  if (dxGammaControl != NULL) 
  {
    dxGammaControl->Release();
    dxGammaControl = NULL;
  }
#endif

  DestroyVirtualScreen();

  // [IKM] 2012-09-07
  // We do not need the wrapper any longer;
  // this does not destroy the underlying allegro screen bitmap, only wrapper.
  delete _allegroScreenWrapper;
  _allegroScreenWrapper = NULL;
  // Nullify the global screen object (for safety reasons); note this yet does
  // not change allegro screen pointer (at this moment it should point at the
  // original internally created allegro bitmap which will be destroyed by Allegro).
  BitmapHelper::SetScreenBitmap(NULL);
}

bool ALSoftwareGraphicsDriver::SetNativeSize(const Size &src_size)
{
  OnSetNativeSize(src_size);
  // If we already have a gfx mode and gfx filter set, then use it to update virtual screen immediately
  CreateVirtualScreen();
  return !_srcRect.IsEmpty();
}

bool ALSoftwareGraphicsDriver::SetRenderFrame(const Rect &dst_rect)
{
  OnSetRenderFrame(dst_rect);
  // If we already have a gfx mode and gfx filter set, then use it to update virtual screen immediately
  CreateVirtualScreen();
  return !_dstRect.IsEmpty();
}

void ALSoftwareGraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse)
{
  int color = 0;
  if (colorToUse != NULL) 
    color = makecol_depth(_mode.ColorDepth, colorToUse->r, colorToUse->g, colorToUse->b);
  // TODO: hardware renderers do not scale these coordinates, but software filter does!
  // find out what's the expected behavior and sync them
  _filter->ClearRect(x1, y1, x2, y2, color);
}

ALSoftwareGraphicsDriver::~ALSoftwareGraphicsDriver()
{
  UnInit();
}

void ALSoftwareGraphicsDriver::UnInit()
{
  OnUnInit();
  ReleaseDisplayMode();

  if (_gfxModeList != NULL)
  {
    destroy_gfx_mode_list(_gfxModeList);
    _gfxModeList = NULL;
  }
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

int ALSoftwareGraphicsDriver::GetCompatibleBitmapFormat(int color_depth)
{
  return color_depth;
}

IDriverDependantBitmap* ALSoftwareGraphicsDriver::CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque)
{
  ALSoftwareBitmap* newBitmap = new ALSoftwareBitmap(bitmap, opaque, hasAlpha);
  return newBitmap;
}

void ALSoftwareGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha)
{
  ALSoftwareBitmap* alSwBmp = (ALSoftwareBitmap*)bitmapToUpdate;
  alSwBmp->_bmp = bitmap;
  alSwBmp->_hasAlpha = hasAlpha;
}

void ALSoftwareGraphicsDriver::DestroyDDB(IDriverDependantBitmap* bitmap)
{
  delete bitmap;
}

void ALSoftwareGraphicsDriver::InitSpriteBatch(size_t index, const SpriteBatchDesc &desc)
{
    if (_spriteBatches.size() <= index)
        _spriteBatches.resize(index + 1);
    ALSpriteBatch &batch = _spriteBatches[index];
    batch.List.clear();
    // TODO: correct offsets to have pre-scale (source) and post-scale (dest) offsets!
    int src_w = desc.Viewport.GetWidth() / desc.Transform.ScaleX;
    int src_h = desc.Viewport.GetHeight() / desc.Transform.ScaleY;
    if (desc.Surface != NULL)
    {
        batch.Surface = std::static_pointer_cast<Bitmap>(desc.Surface);
        batch.Opaque = true;
    }
    else if (desc.Viewport.IsEmpty() || !virtualScreen)
    {
        batch.Surface.reset();
        batch.Opaque = false;
    }
    else if (desc.Transform.ScaleX == 1.f && desc.Transform.ScaleY == 1.f)
    {
        Rect rc = RectWH(desc.Viewport.Left - _virtualScrOff.X, desc.Viewport.Top - _virtualScrOff.Y, desc.Viewport.GetWidth(), desc.Viewport.GetHeight());
        batch.Surface.reset(BitmapHelper::CreateSubBitmap(virtualScreen, rc));
        batch.Opaque = true;
    }
    else if (!batch.Surface || batch.Surface->GetWidth() != src_w || batch.Surface->GetHeight() != src_h)
    {
        batch.Surface.reset(new Bitmap(src_w, src_h));
        batch.Opaque = false;
    }
}

void ALSoftwareGraphicsDriver::ResetAllBatches()
{
    for (ALSpriteBatches::iterator it = _spriteBatches.begin(); it != _spriteBatches.end(); ++it)
        it->List.clear();
}

void ALSoftwareGraphicsDriver::DrawSprite(int x, int y, IDriverDependantBitmap* bitmap)
{
    _spriteBatches[_actSpriteBatch].List.push_back(ALDrawListEntry((ALSoftwareBitmap*)bitmap, x, y));
}

void ALSoftwareGraphicsDriver::RenderToBackBuffer()
{
    // Render all the sprite batches with necessary transformations
    //
    // NOTE: that's not immediately clear whether it would be faster to first draw upon a camera-sized
    // surface then stretch final result to the viewport on screen, or stretch-blit each individual
    // sprite right onto screen bitmap. We'd need to do proper profiling to know that.
    // An important thing is that Allegro does not provide stretching functions for drawing sprites
    // with blending and translucency; it seems you'd have to first stretch the original sprite onto a
    // temp buffer and then TransBlendBlt / LitBlendBlt it to the final destination. Of course, doing
    // that here would slow things down significantly, so if we ever go that way sprite caching will
    // be required (similarily to how AGS caches flipped/scaled object sprites now for).
    //
    for (size_t i = 0; i <= _actSpriteBatch; ++i)
    {
        const Rect &viewport = _spriteBatchDesc[i].Viewport;
        const SpriteTransform &transform = _spriteBatchDesc[i].Transform;
        const ALSpriteBatch &batch = _spriteBatches[i];

        virtualScreen->SetClip(Rect::MoveBy(viewport, -_virtualScrOff.X, -_virtualScrOff.Y));
        Bitmap *surface = static_cast<Bitmap*>(batch.Surface.get());
        // TODO: correct transform offsets to have pre-scale (source) and post-scale (dest) offsets!
        int view_offx = viewport.Left + transform.X - _virtualScrOff.X;
        int view_offy = viewport.Top + transform.Y - _virtualScrOff.Y;
        if (surface)
        {
            if (!batch.Opaque)
                surface->ClearTransparent();
            _stageVirtualScreen = surface;
            RenderSpriteBatch(batch, surface, 0, 0);
            // TODO: extract this to the generic software blit-with-transform function
            virtualScreen->StretchBlt(surface, RectWH(view_offx, view_offy, viewport.GetWidth(), viewport.GetHeight()),
                batch.Opaque ? kBitmap_Copy : kBitmap_Transparency);
        }
        else
        {
            RenderSpriteBatch(batch, virtualScreen, view_offx, view_offy);
        }
        _stageVirtualScreen = virtualScreen;
    }
    ClearDrawLists();
}

void ALSoftwareGraphicsDriver::RenderSpriteBatch(const ALSpriteBatch &batch, Common::Bitmap *surface, int surf_offx, int surf_offy)
{
  const std::vector<ALDrawListEntry> &drawlist = batch.List;
  for (size_t i = 0; i < drawlist.size(); i++)
  {
    if (drawlist[i].bitmap == NULL)
    {
      if (_nullSpriteCallback)
        _nullSpriteCallback(drawlist[i].x, drawlist[i].y);
      else
        throw Ali3DException("Unhandled attempt to draw null sprite");

      continue;
    }

    ALSoftwareBitmap* bitmap = drawlist[i].bitmap;
    int drawAtX = drawlist[i].x + surf_offx;
    int drawAtY = drawlist[i].y + surf_offy;

    if ((bitmap->_opaque) && (bitmap->_bmp == surface))
    { }
    else if (bitmap->_opaque)
    {
        surface->Blit(bitmap->_bmp, 0, 0, drawAtX, drawAtY, bitmap->_bmp->GetWidth(), bitmap->_bmp->GetHeight());
    }
    else if (bitmap->_transparency >= 255)
    {
      // fully transparent... invisible, do nothing
    }
    else if (bitmap->_hasAlpha)
    {
      if (bitmap->_transparency == 0) // this means opaque
        set_alpha_blender();
      else
        // here _transparency is used as alpha (between 1 and 254)
        set_blender_mode(NULL, NULL, _trans_alpha_blender32, 0, 0, 0, bitmap->_transparency);

      surface->TransBlendBlt(bitmap->_bmp, drawAtX, drawAtY);
    }
    else
    {
      // here _transparency is used as alpha (between 1 and 254), but 0 means opaque!
      GfxUtil::DrawSpriteWithTransparency(surface, bitmap->_bmp, drawAtX, drawAtY,
          bitmap->_transparency ? bitmap->_transparency : 255);
    }
  }

  if (((_tint_red > 0) || (_tint_green > 0) || (_tint_blue > 0))
      && (_mode.ColorDepth > 8)) {
    // Common::gl_ScreenBmp tint
    // This slows down the game no end, only experimental ATM
    set_trans_blender(_tint_red, _tint_green, _tint_blue, 0);
    surface->LitBlendBlt(surface, 0, 0, 128);
/*  This alternate method gives the correct (D3D-style) result, but is just too slow!
    if ((_spareTintingScreen != NULL) &&
        ((_spareTintingScreen->GetWidth() != surface->GetWidth()) || (_spareTintingScreen->GetHeight() != surface->GetHeight())))
    {
      destroy_bitmap(_spareTintingScreen);
      _spareTintingScreen = NULL;
    }
    if (_spareTintingScreen == NULL)
    {
      _spareTintingScreen = BitmapHelper::CreateBitmap_(GetColorDepth(surface), surface->GetWidth(), surface->GetHeight());
    }
    tint_image(surface, _spareTintingScreen, _tint_red, _tint_green, _tint_blue, 100, 255);
    Blit(_spareTintingScreen, surface, 0, 0, 0, 0, _spareTintingScreen->GetWidth(), _spareTintingScreen->GetHeight());*/
  }
}

void ALSoftwareGraphicsDriver::Render(GlobalFlipType flip)
{
  RenderToBackBuffer();

  if (_autoVsync)
    this->Vsync();

  if (flip == kFlip_None)
    _filter->RenderScreen(virtualScreen, _virtualScrOff.X + _globalViewOff.X, _virtualScrOff.Y + _globalViewOff.Y);
  else
    _filter->RenderScreenFlipped(virtualScreen, _virtualScrOff.X + _globalViewOff.X, _virtualScrOff.Y + _globalViewOff.Y, flip);
}

void ALSoftwareGraphicsDriver::Render()
{
  Render(kFlip_None);
}

void ALSoftwareGraphicsDriver::Vsync()
{
  vsync();
}

Bitmap *ALSoftwareGraphicsDriver::GetMemoryBackBuffer()
{
  return virtualScreen;
}

void ALSoftwareGraphicsDriver::SetMemoryBackBuffer(Bitmap *backBuffer, int offx, int offy)
{
  if (backBuffer)
  {
    virtualScreen = backBuffer;
    _virtualScrOff = Point(offx, offy);
  }
  else
  {
    virtualScreen = _origVirtualScreen;
    _virtualScrOff = Point();
  }
  _stageVirtualScreen = virtualScreen;
}

Bitmap *ALSoftwareGraphicsDriver::GetStageBackBuffer()
{
    return _stageVirtualScreen;
}

void ALSoftwareGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res)
{
  (void)at_native_res; // software driver always renders at native resolution at the moment
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
   const int col_depth = currentVirtScreen->GetColorDepth();

   int _global_x_offset = _virtualScrOff.X + _globalViewOff.X;
   int _global_y_offset = _virtualScrOff.Y + _globalViewOff.Y;
   if ((_global_y_offset != 0) || (_global_x_offset != 0))
   {
     bmp_orig = BitmapHelper::CreateBitmap(_srcRect.GetWidth(), _srcRect.GetHeight(), col_depth);
     bmp_orig->Fill(0);
     bmp_orig->Blit(currentVirtScreen, 0, 0, _global_x_offset, _global_y_offset, currentVirtScreen->GetWidth(), currentVirtScreen->GetHeight());
   }

   bmp_buff = BitmapHelper::CreateBitmap(bmp_orig->GetWidth(), bmp_orig->GetHeight(), col_depth);
   const int clearColor = makecol_depth(col_depth, targetColourRed, targetColourGreen, targetColourBlue);

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
         if (_pollingCallback)
           _pollingCallback();
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

    const int col_depth = BitmapHelper::GetScreenBitmap()->GetColorDepth();
    const int clearColor = makecol_depth(col_depth, targetColourRed, targetColourGreen, targetColourBlue);

    if ((bmp_orig = BitmapHelper::CreateBitmap(_srcRect.GetWidth(), _srcRect.GetHeight(), col_depth)))
    {
        if ((bmp_buff = BitmapHelper::CreateBitmap(bmp_orig->GetWidth(), bmp_orig->GetHeight(), col_depth)))
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
                  if (_pollingCallback)
                    _pollingCallback();
                  platform->Delay(1);
                }
                while (timerValue == *_loopTimer);
            }
            delete bmp_buff;
        }
        delete bmp_orig;
    }

    BitmapHelper::GetScreenBitmap()->Clear(clearColor);
    int _global_x_offset = _virtualScrOff.X + _globalViewOff.X;
    int _global_y_offset = _virtualScrOff.Y + _globalViewOff.Y;
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
      if (_pollingCallback) _pollingCallback();
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

  if (_mode.ColorDepth > 8) 
  {
    highcolor_fade_out(speed * 4, targetColourRed, targetColourGreen, targetColourBlue);
  }
  else __fade_out_range(speed, 0, 255, targetColourRed, targetColourGreen, targetColourBlue);

}

void ALSoftwareGraphicsDriver::FadeIn(int speed, PALLETE p, int targetColourRed, int targetColourGreen, int targetColourBlue) {
  if (_mode.ColorDepth > 8) {

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
    int yspeed = _srcRect.GetHeight() / (_srcRect.GetWidth() / speed);
    int boxwid = speed, boxhit = yspeed;

    while (boxwid < _srcRect.GetWidth()) {
      boxwid += speed;
      boxhit += yspeed;
      this->Vsync();
      int vcentre = _srcRect.GetHeight() / 2;
      this->ClearRectangle(_srcRect.GetWidth() / 2 - boxwid / 2, vcentre - boxhit / 2,
          _srcRect.GetWidth() / 2 + boxwid / 2, vcentre + boxhit / 2, NULL);
    
      if (_pollingCallback)
        _pollingCallback();

      platform->Delay(delay);
    }
    this->ClearRectangle(0, 0, _srcRect.GetWidth() - 1, _srcRect.GetHeight() - 1, NULL);
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


ALSWGraphicsFactory *ALSWGraphicsFactory::_factory = NULL;

ALSWGraphicsFactory::~ALSWGraphicsFactory()
{
    _factory = NULL;
}

size_t ALSWGraphicsFactory::GetFilterCount() const
{
    return 2;
}

const GfxFilterInfo *ALSWGraphicsFactory::GetFilterInfo(size_t index) const
{
    switch (index)
    {
    case 0:
        return &AllegroGfxFilter::FilterInfo;
    case 1:
        return &HqxGfxFilter::FilterInfo;
    default:
        return NULL;
    }
}

String ALSWGraphicsFactory::GetDefaultFilterID() const
{
    return AllegroGfxFilter::FilterInfo.Id;
}

/* static */ ALSWGraphicsFactory *ALSWGraphicsFactory::GetFactory()
{
    if (!_factory)
        _factory = new ALSWGraphicsFactory();
    return _factory;
}

ALSoftwareGraphicsDriver *ALSWGraphicsFactory::EnsureDriverCreated()
{
    if (!_driver)
        _driver = new ALSoftwareGraphicsDriver();
    return _driver;
}

AllegroGfxFilter *ALSWGraphicsFactory::CreateFilter(const String &id)
{
    if (AllegroGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new AllegroGfxFilter();
    else if (HqxGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new HqxGfxFilter();
    return NULL;
}

} // namespace ALSW
} // namespace Engine
} // namespace AGS

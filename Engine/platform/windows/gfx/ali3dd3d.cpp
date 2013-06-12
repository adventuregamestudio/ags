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
// Allegro Interface for 3D; Direct 3D 9 driver
//
//=============================================================================

#define WINVER 0x0500  // for GetWindowInfo
#include <stdio.h>
#include <allegro.h>
#include <winalleg.h>
#include <allegro/platform/aintwin.h>
#include <d3d9.h>
#include "gfx/ali3d.h"
#include "gfx/gfxfilter_d3d.h"
#include "platform/base/agsplatformdriver.h"
#include "gfx/graphics.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"

using AGS::Common::Bitmap;
using AGS::Common::Graphics;
namespace BitmapHelper = AGS::Common::BitmapHelper;
using namespace AGS; // FIXME later

extern int dxmedia_play_video_3d(const char*filename, IDirect3DDevice9 *device, bool useAVISound, int canskip, int stretch);
extern void dxmedia_shutdown_3d();
void dummy_vsync() { }

#define MAX_DRAW_LIST_SIZE 200

#define algetr32(xx) ((xx >> _rgb_r_shift_32) & 0xFF)
#define algetg32(xx) ((xx >> _rgb_g_shift_32) & 0xFF)
#define algetb32(xx) ((xx >> _rgb_b_shift_32) & 0xFF)
#define algeta32(xx) ((xx >> _rgb_a_shift_32) & 0xFF)
#define algetr15(xx) ((xx >> _rgb_r_shift_15) & 0x1F)
#define algetg15(xx) ((xx >> _rgb_g_shift_15) & 0x1F)
#define algetb15(xx) ((xx >> _rgb_b_shift_15) & 0x1F)

#define GFX_DIRECT3D_WIN  AL_ID('D','X','3','W')
#define GFX_DIRECT3D_FULL AL_ID('D','X','3','D')

GFX_DRIVER gfx_direct3d_win =
{
   GFX_DIRECT3D_WIN,
   empty_string,
   empty_string,
   "Direct3D windowed",
   NULL,    // init
   NULL,   // exit
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   dummy_vsync,   // vsync
   NULL,  // setpalette
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,  //create_video_bitmap
   NULL,  //destroy_video_bitmap
   NULL,   //show_video_bitmap
   NULL,
   NULL,  //gfx_directx_create_system_bitmap,
   NULL, //gfx_directx_destroy_system_bitmap,
   NULL, //gfx_directx_set_mouse_sprite,
   NULL, //gfx_directx_show_mouse,
   NULL, //gfx_directx_hide_mouse,
   NULL, //gfx_directx_move_mouse,
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                        // int w, h;
   FALSE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   TRUE                         // int windowed;
};

GFX_DRIVER gfx_direct3d_full =
{
   GFX_DIRECT3D_FULL,
   empty_string,
   empty_string,
   "Direct3D fullscreen",
   NULL,    // init
   NULL,   // exit
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   dummy_vsync,   // sync
   NULL,  // setpalette
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,  //create_video_bitmap
   NULL,  //destroy_video_bitmap
   NULL,   //show_video_bitmap
   NULL,
   NULL,  //gfx_directx_create_system_bitmap,
   NULL, //gfx_directx_destroy_system_bitmap,
   NULL, //gfx_directx_set_mouse_sprite,
   NULL, //gfx_directx_show_mouse,
   NULL, //gfx_directx_hide_mouse,
   NULL, //gfx_directx_move_mouse,
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                        // int w, h;
   FALSE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   FALSE                         // int windowed;
};

// The custom FVF, which describes the custom vertex structure.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

struct CUSTOMVERTEX
{
    D3DVECTOR   position; // The position.
    D3DVECTOR   normal;
    FLOAT       tu, tv;   // The texture coordinates.
};

struct TextureTile
{
  int x, y;
  int width, height;
  IDirect3DTexture9* texture;
};

class D3DBitmap : public IDriverDependantBitmap
{
public:
  // Transparency is a bit counter-intuitive
  // 0=not transparent, 255=invisible, 1..254 barely visible .. mostly visible
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
  virtual void SetLightLevel(int lightLevel)  { _lightLevel = lightLevel; }
  virtual void SetTint(int red, int green, int blue, int tintSaturation) 
  {
    _red = red;
    _green = green;
    _blue = blue;
    _tintSaturation = tintSaturation;
  }

  int _width, _height;
  int _colDepth;
  bool _flipped;
  int _stretchToWidth, _stretchToHeight;
  int _red, _green, _blue;
  int _tintSaturation;
  int _lightLevel;
  bool _opaque;
  bool _hasAlpha;
  int _transparency;
  IDirect3DVertexBuffer9* _vertex;
  TextureTile *_tiles;
  int _numTiles;

  D3DBitmap(int width, int height, int colDepth, bool opaque)
  {
    _width = width;
    _height = height;
    _colDepth = colDepth;
    _flipped = false;
    _hasAlpha = false;
    _stretchToWidth = 0;
    _stretchToHeight = 0;
    _tintSaturation = 0;
    _lightLevel = 0;
    _transparency = 0;
    _opaque = opaque;
    _vertex = NULL;
    _tiles = NULL;
    _numTiles = 0;
  }

  int GetWidthToRender() { return (_stretchToWidth > 0) ? _stretchToWidth : _width; }
  int GetHeightToRender() { return (_stretchToHeight > 0) ? _stretchToHeight : _height; }

  void Dispose()
  {
    if (_tiles != NULL)
    {
      for (int i = 0; i < _numTiles; i++)
        _tiles[i].texture->Release();

      free(_tiles);
      _tiles = NULL;
      _numTiles = 0;
    }
    if (_vertex != NULL)
    {
      _vertex->Release();
      _vertex = NULL;
    }
  }

  ~D3DBitmap()
  {
    Dispose();
  }
};

struct SpriteDrawListEntry
{
  D3DBitmap *bitmap;
  int x, y;
  bool skip;
};

class D3DGraphicsDriver : public IGraphicsDriver
{
public:
  virtual const char*GetDriverName() { return "Direct3D 9"; }
  virtual const char*GetDriverID() { return "D3D9"; }
  virtual void SetTintMethod(TintMethod method);
  virtual bool Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer);
  virtual bool Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer);
  virtual int  FindSupportedResolutionWidth(int idealWidth, int height, int colDepth, int widthRangeAllowed);
  virtual void SetCallbackForPolling(GFXDRV_CLIENTCALLBACK callback) { _pollingCallback = callback; }
  virtual void SetCallbackToDrawScreen(GFXDRV_CLIENTCALLBACK callback) { _drawScreenCallback = callback; }
  virtual void SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) { _initGfxCallback = callback; }
  virtual void SetCallbackForNullSprite(GFXDRV_CLIENTCALLBACKXY callback) { _nullSpriteCallback = callback; }
  virtual void UnInit();
  virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse);
  virtual Bitmap *ConvertBitmapToSupportedColourDepth(Bitmap *bitmap);
  virtual IDriverDependantBitmap* CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque);
  virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha);
  virtual void DestroyDDB(IDriverDependantBitmap* bitmap);
  virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap);
  virtual void ClearDrawList();
  virtual void RenderToBackBuffer();
  virtual void Render();
  virtual void Render(GlobalFlipType flip);
  virtual void SetRenderOffset(int x, int y);
  virtual void GetCopyOfScreenIntoBitmap(Bitmap *destination);
  virtual void EnableVsyncBeforeRender(bool enabled) { }
  virtual void Vsync();
  virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
  virtual void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue);
  virtual void BoxOutEffect(bool blackingOut, int speed, int delay);
  virtual bool PlayVideo(const char *filename, bool useSound, VideoSkipType skipType, bool stretchToFullScreen);
  virtual bool SupportsGammaControl();
  virtual void SetGamma(int newGamma);
  virtual void UseSmoothScaling(bool enabled) { _smoothScaling = enabled; }
  virtual bool RequiresFullRedrawEachFrame() { return true; }
  virtual bool HasAcceleratedStretchAndFlip() { return true; }
  virtual bool UsesMemoryBackBuffer() { return false; }
  virtual Bitmap* GetMemoryBackBuffer() { return NULL; }
  virtual void SetMemoryBackBuffer(Bitmap *backBuffer) {  }
  virtual void SetScreenTint(int red, int green, int blue);

  // Internal
  int _initDLLCallback();
  int _resetDeviceIfNecessary();
  void _render(GlobalFlipType flip, bool clearDrawListAfterwards);
  void _reDrawLastFrame();
  D3DGraphicsDriver(D3DGFXFilter *filter);
  virtual ~D3DGraphicsDriver();

  D3DGFXFilter *_filter;

private:
  HMODULE d3dlib;
  D3DPRESENT_PARAMETERS d3dpp;
  IDirect3D9* direct3d;
  IDirect3DDevice9* direct3ddevice;
  D3DGAMMARAMP defaultgammaramp;
  D3DGAMMARAMP currentgammaramp;
  D3DCAPS9 direct3ddevicecaps;
  IDirect3DVertexBuffer9* vertexbuffer;
  int _newmode_width, _newmode_height;
  int _newmode_screen_width, _newmode_screen_height;
  int _newmode_depth, _newmode_refresh;
  bool _newmode_windowed;
  int _global_x_offset, _global_y_offset;
  UINT availableVideoMemory;
  GFXDRV_CLIENTCALLBACK _pollingCallback;
  GFXDRV_CLIENTCALLBACK _drawScreenCallback;
  GFXDRV_CLIENTCALLBACKXY _nullSpriteCallback;
  GFXDRV_CLIENTCALLBACKINITGFX _initGfxCallback;
  int _tint_red, _tint_green, _tint_blue;
  CUSTOMVERTEX defaultVertices[4];
  char previousError[ALLEGRO_ERROR_SIZE];
  IDirect3DPixelShader9* pixelShader;
  bool _smoothScaling;
  bool _legacyPixelShader;
  float _pixelRenderOffset;
  volatile int *_loopTimer;
  Bitmap *_screenTintLayer;
  D3DBitmap* _screenTintLayerDDB;
  SpriteDrawListEntry _screenTintSprite;

  SpriteDrawListEntry drawList[MAX_DRAW_LIST_SIZE];
  int numToDraw;
  SpriteDrawListEntry drawListLastTime[MAX_DRAW_LIST_SIZE];
  int numToDrawLastTime;
  GlobalFlipType flipTypeLastTime;

  bool EnsureDirect3D9IsCreated();
  void initD3DDLL();
  void InitializeD3DState();
  void set_up_default_vertices();
  void make_translated_scaling_matrix(D3DMATRIX *matrix, float x, float y, float xScale, float yScale);
  void AdjustSizeToNearestSupportedByCard(int *width, int *height);
  void UpdateTextureRegion(TextureTile *tile, Bitmap *bitmap, D3DBitmap *target, bool hasAlpha);
  void do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
  bool IsTextureFormatOk( D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat );
  bool IsModeSupported(int width, int height, int colDepth);
  void create_screen_tint_bitmap();
  void _renderSprite(SpriteDrawListEntry *entry, bool globalLeftRightFlip, bool globalTopBottomFlip);
};

static int wnd_create_device();
static D3DGraphicsDriver *_d3d_driver = NULL;

IGraphicsDriver* GetD3DGraphicsDriver(GFXFilter *filter)
{
  D3DGFXFilter* d3dfilter = (D3DGFXFilter*)filter;
  if (_d3d_driver == NULL)
  {
    _d3d_driver = new D3DGraphicsDriver(d3dfilter);
  }
  else if (_d3d_driver->_filter != filter)
  {
    _d3d_driver->_filter = d3dfilter;
  }

  return _d3d_driver;
}

D3DGraphicsDriver::D3DGraphicsDriver(D3DGFXFilter *filter) 
{
  d3dlib = NULL;
  direct3d = NULL;
  direct3ddevice = NULL;
  vertexbuffer = NULL;
  numToDraw = 0;
  numToDrawLastTime = 0;
  _pollingCallback = NULL;
  _drawScreenCallback = NULL;
  _initGfxCallback = NULL;
  _tint_red = 0;
  _tint_green = 0;
  _tint_blue = 0;
  _global_x_offset = 0;
  _global_y_offset = 0;
  _newmode_screen_width = 0;
  _newmode_screen_height = 0;
  _filter = filter;
  _screenTintLayer = NULL;
  _screenTintLayerDDB = NULL;
  _screenTintSprite.skip = true;
  _screenTintSprite.x = 0;
  _screenTintSprite.y = 0;
  pixelShader = NULL;
  previousError[0] = 0;
  _legacyPixelShader = false;
  set_up_default_vertices();
}

void D3DGraphicsDriver::set_up_default_vertices()
{
  defaultVertices[0].position.x = 0.0f;
  defaultVertices[0].position.y = 0.0f;
  defaultVertices[0].position.z = 0.0f;
  defaultVertices[0].normal.x = 0.0f;
  defaultVertices[0].normal.y = 0.0f;
  defaultVertices[0].normal.z = -1.0f;
  //defaultVertices[0].color=0xffffffff;
  defaultVertices[0].tu=0.0;
  defaultVertices[0].tv=0.0;
  defaultVertices[1].position.x = 1.0f;
  defaultVertices[1].position.y = 0.0f;
  defaultVertices[1].position.z = 0.0f;
  defaultVertices[1].normal.x = 0.0f;
  defaultVertices[1].normal.y = 0.0f;
  defaultVertices[1].normal.z = -1.0f;
  //defaultVertices[1].color=0xffffffff;
  defaultVertices[1].tu=1.0;
  defaultVertices[1].tv=0.0;
  defaultVertices[2].position.x = 0.0f;
  defaultVertices[2].position.y = -1.0f;
  defaultVertices[2].position.z = 0.0f;
  defaultVertices[2].normal.x = 0.0f;
  defaultVertices[2].normal.y = 0.0f;
  defaultVertices[2].normal.z = -1.0f;
  //defaultVertices[2].color=0xffffffff;
  defaultVertices[2].tu=0.0;
  defaultVertices[2].tv=1.0;
  defaultVertices[3].position.x = 1.0f;
  defaultVertices[3].position.y = -1.0f;
  defaultVertices[3].position.z = 0.0f;
  defaultVertices[3].normal.x = 0.0f;
  defaultVertices[3].normal.y = 0.0f;
  defaultVertices[3].normal.z = -1.0f;
  //defaultVertices[3].color=0xffffffff;
  defaultVertices[3].tu=1.0;
  defaultVertices[3].tv=1.0;
}

void D3DGraphicsDriver::Vsync() 
{
  // do nothing on D3D
}

bool D3DGraphicsDriver::EnsureDirect3D9IsCreated() 
{
   if (direct3d != NULL)
     return true;

   IDirect3D9 * (WINAPI * lpDirect3DCreate9)(UINT);

   d3dlib = LoadLibrary("d3d9.dll");
   if (d3dlib == NULL) 
   {
     strcpy(allegro_error, "Direct3D is not installed");
     return false;
   }

   lpDirect3DCreate9 = (IDirect3D9 * (WINAPI *)(UINT))GetProcAddress(d3dlib, "Direct3DCreate9");
   if (lpDirect3DCreate9 == NULL) 
   {
     FreeLibrary(d3dlib);
     d3dlib = NULL;
     strcpy(allegro_error, "Entry point not found in d3d9.dll");
     return false;
   }

   direct3d = lpDirect3DCreate9( D3D_SDK_VERSION );
   if (direct3d == NULL) {
     FreeLibrary(d3dlib);
     d3dlib = NULL;
     strcpy(allegro_error, "Direct3DCreate failed!");
     return false;
   }

   return true;
}

void D3DGraphicsDriver::initD3DDLL() 
{
   if (!EnsureDirect3D9IsCreated())
   {
     throw Ali3DException(allegro_error);
   }

   if (!IsModeSupported(_newmode_screen_width, _newmode_screen_height, _newmode_depth))
   {
     throw Ali3DException(allegro_error);
   }

   _enter_critical();

   // Set the display mode in the window's thread
   if (wnd_call_proc(wnd_create_device)) {
     _exit_critical();
     direct3d->Release();
     direct3d = NULL;
     FreeLibrary(d3dlib);
     d3dlib = NULL;
     throw Ali3DException(allegro_error);
   }

   // The display mode has been set up successfully, save the
   // final refresh rate that we are using
   D3DDISPLAYMODE final_display_mode;
   if (direct3ddevice->GetDisplayMode(0, &final_display_mode) == D3D_OK) {
     _newmode_refresh = final_display_mode.RefreshRate;
   }
   else {
     _newmode_refresh = 0;
   }

   availableVideoMemory = direct3ddevice->GetAvailableTextureMem();

   _exit_critical();

   // Set up a fake allegro gfx driver so that things like
   // the allegro mouse handler still work
   if (_newmode_windowed)
     gfx_driver = &gfx_direct3d_win;
   else
     gfx_driver = &gfx_direct3d_full;

   return;
}

/* color_depth_to_d3d_format:
 *  Convert a colour depth into the appropriate D3D tag
 */
static D3DFORMAT color_depth_to_d3d_format(int color_depth, bool wantAlpha)
{
  if (wantAlpha)
  {
    switch (color_depth)
    {
      case 8:
        return D3DFMT_P8;
      case 15:
      case 16:
        return D3DFMT_A1R5G5B5;
      case 24:
      case 32:
        return D3DFMT_A8R8G8B8;
    }
  }
  else
  {
    switch (color_depth)
    {
      case 8:
        return D3DFMT_P8;
      case 15:  // don't use X1R5G5B5 because some cards don't support it
        return D3DFMT_A1R5G5B5;
      case 16:
        return D3DFMT_R5G6B5;
      case 24:
        return D3DFMT_R8G8B8;
      case 32:
        return D3DFMT_X8R8G8B8;
    }
  }
  return D3DFMT_UNKNOWN;
}

bool D3DGraphicsDriver::IsModeSupported(int width, int height, int colDepth)
{
  if (_newmode_windowed)
  {
    return true;
  }

  D3DFORMAT pixelFormat = color_depth_to_d3d_format(colDepth, false);
  D3DDISPLAYMODE displayMode;

  int modeCount = direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, pixelFormat);
  for (int i = 0; i < modeCount; i++)
  {
    if (FAILED(direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, pixelFormat, i, &displayMode)))
    {
      strcpy(allegro_error, "IDirect3D9::EnumAdapterModes failed");
      return false;
    }

    if ((displayMode.Width == width) && (displayMode.Height == height))
    {
      return true;
    }
  }

  strcpy(allegro_error, "The requested adapter mode is not supported");
  return false;
}

int D3DGraphicsDriver::FindSupportedResolutionWidth(int idealWidth, int height, int colDepth, int widthRangeAllowed)
{
  if (!EnsureDirect3D9IsCreated())
    return 0;

  int unfilteredWidth = idealWidth;
  _filter->GetRealResolution(&idealWidth, &height);
  int filterFactor = idealWidth / unfilteredWidth;

  D3DFORMAT pixelFormat = color_depth_to_d3d_format(colDepth, false);
  D3DDISPLAYMODE displayMode;

  int nearestWidthFound = 0;
  int modeCount = direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, pixelFormat);
  for (int i = 0; i < modeCount; i++)
  {
    if (FAILED(direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, pixelFormat, i, &displayMode)))
    {
      strcpy(allegro_error, "IDirect3D9::EnumAdapterModes failed");
      return 0;
    }

    if (displayMode.Height == height)
    {
      if (displayMode.Width == idealWidth)
        return idealWidth / filterFactor;

      if (abs(displayMode.Width - idealWidth) <
          abs(nearestWidthFound - idealWidth))
      {
        nearestWidthFound = displayMode.Width;
      }
    }
  }

  if (abs(nearestWidthFound - idealWidth) <= widthRangeAllowed * filterFactor)
    return nearestWidthFound / filterFactor;

  return 0;
}

bool D3DGraphicsDriver::SupportsGammaControl() 
{
  if ((direct3ddevicecaps.Caps2 & D3DCAPS2_FULLSCREENGAMMA) == 0)
    return false;

  if (_newmode_windowed)
    return false;

  return true;
}

void D3DGraphicsDriver::SetGamma(int newGamma)
{
  for (int i = 0; i < 256; i++) 
  {
    int newValue = ((int)defaultgammaramp.red[i] * newGamma) / 100;
    if (newValue >= 65535)
      newValue = 65535;
    currentgammaramp.red[i] = newValue;
    currentgammaramp.green[i] = newValue;
    currentgammaramp.blue[i] = newValue;
  }

  direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);
}

/* wnd_set_video_mode:
 *  Called by window thread to set a gfx mode; this is needed because DirectDraw can only
 *  change the mode in the thread that handles the window.
 */
static int wnd_create_device()
{
  return _d3d_driver->_initDLLCallback();
}

static int wnd_reset_device()
{
  return _d3d_driver->_resetDeviceIfNecessary();
}

// The D3DX library does this for us, but it's an external DLL that
// we don't want a dependency on.
void D3DGraphicsDriver::make_translated_scaling_matrix(D3DMATRIX *matrix, float x, float y, float xScale, float yScale)
{
  matrix->_11 = xScale;
  matrix->_12 = 0.0;
  matrix->_13 = 0.0;
  matrix->_14 = 0.0;
  matrix->_21 = 0.0;
  matrix->_22 = yScale;
  matrix->_23 = 0.0;
  matrix->_24 = 0.0;
  matrix->_31 = 0.0;
  matrix->_32 = 0.0;
  matrix->_33 = 1.0;
  matrix->_34 = 0.0;
  matrix->_41 = x;
  matrix->_42 = y;
  matrix->_43 = 0.0;
  matrix->_44 = 1.0;
}

int D3DGraphicsDriver::_resetDeviceIfNecessary()
{
  HRESULT hr = direct3ddevice->TestCooperativeLevel();

  if (hr == D3DERR_DEVICELOST)
  {
    OutputDebugString("AGS -- D3D Device Lost");
    // user has alt+tabbed away from the game
    return 1;
  }

  if (hr == D3DERR_DEVICENOTRESET)
  {
    OutputDebugString("AGS -- D3D Device Not Reset");
    hr = direct3ddevice->Reset(&d3dpp);
    if (hr != D3D_OK)
    {
      OutputDebugString("AGS -- Failed to reset D3D device");
      // can't throw exception because we're in the wrong thread,
      // so just return a value instead
      return 2;
    }

    InitializeD3DState();

    direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);
  }

  return 0;
}

int D3DGraphicsDriver::_initDLLCallback()
{
  int i = 0;
  CUSTOMVERTEX *vertices;

  HWND allegro_wnd = win_get_window();

  if (!_newmode_windowed)
  {
    // Remove the border in full-screen mode, otherwise if the player
    // clicks near the edge of the screen it goes back to Windows
    LONG windowStyle = WS_POPUP;
    SetWindowLong(allegro_wnd, GWL_STYLE, windowStyle);
  }

  memset( &d3dpp, 0, sizeof(d3dpp) );
  d3dpp.BackBufferWidth = _newmode_screen_width;
  d3dpp.BackBufferHeight = _newmode_screen_height;
  d3dpp.BackBufferFormat = color_depth_to_d3d_format(_newmode_depth, false);
  d3dpp.BackBufferCount = 1;
  d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
  // THIS MUST BE SWAPEFFECT_COPY FOR PlayVideo TO WORK
  d3dpp.SwapEffect = D3DSWAPEFFECT_COPY; //D3DSWAPEFFECT_DISCARD; 
  d3dpp.hDeviceWindow = allegro_wnd;
  d3dpp.Windowed = _newmode_windowed;
  d3dpp.EnableAutoDepthStencil = FALSE;
  d3dpp.Flags = 0;
  d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
  d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; //D3DPRESENT_INTERVAL_DEFAULT;
  //d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
  /* If full screen, specify the refresh rate */
  if ((d3dpp.Windowed == FALSE) && (_newmode_refresh > 0))
    d3dpp.FullScreen_RefreshRateInHz = _newmode_refresh;

  if (_initGfxCallback != NULL)
    _initGfxCallback(&d3dpp);

  HRESULT hr = direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, allegro_wnd,
                      D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,  // multithreaded required for AVI player
                      &d3dpp, &direct3ddevice);
  if (hr != D3D_OK)
  {
    if (previousError[0] != 0)
      sprintf(allegro_error, previousError);
    else
      sprintf(allegro_error, "Failed to create Direct3D Device: 0x%08X", hr);
    return -1;
  }

  if (_newmode_windowed)
  {
    if (adjust_window(_newmode_screen_width, _newmode_screen_height) != 0) 
    {
      direct3ddevice->Release();
      direct3ddevice = NULL;
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Window size not supported"));
      return -1;
    }
  }

  win_grab_input();

  direct3ddevice->GetDeviceCaps(&direct3ddevicecaps);

  // the PixelShader.fx uses ps_1_4
  // the PixelShaderLegacy.fx needs ps_2_0
  int requiredPSMajorVersion = 1;
  int requiredPSMinorVersion = 4;
  if (_legacyPixelShader) {
    requiredPSMajorVersion = 2;
    requiredPSMinorVersion = 0;
  }

  if (direct3ddevicecaps.PixelShaderVersion < D3DPS_VERSION(requiredPSMajorVersion, requiredPSMinorVersion))  
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    usprintf(allegro_error, "Graphics card does not support Pixel Shader %d.%d", requiredPSMajorVersion, requiredPSMinorVersion);
    strcpy(previousError, allegro_error);
    return -1;
  }

  // Load the pixel shader!!
  HMODULE exeHandle = GetModuleHandle(NULL);
  HRSRC hRes = FindResource(exeHandle, (_legacyPixelShader) ? "PIXEL_SHADER_LEGACY" : "PIXEL_SHADER", "DATA");
  if (hRes)
  {
    HGLOBAL hGlobal = LoadResource(exeHandle, hRes);
    if (hGlobal)
    {
      DWORD resourceSize = SizeofResource(exeHandle, hRes);
      DWORD *dataPtr = (DWORD*)LockResource(hGlobal);
      hr = direct3ddevice->CreatePixelShader(dataPtr, &pixelShader);
      if (hr != D3D_OK)
      {
        direct3ddevice->Release();
        direct3ddevice = NULL;
        sprintf(allegro_error, "Failed to create pixel shader: 0x%08X", hr);
        strcpy(previousError, allegro_error);
        return -1;
      }
      UnlockResource(hGlobal);
    }
  }
  
  if (pixelShader == NULL)
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    sprintf(allegro_error, "Failed to load pixel shader resource");
    strcpy(previousError, allegro_error);
    return -1;
  }

  // This line crashes because my card doesn't support 8-bit textures
  //direct3ddevice->SetCurrentTexturePalette(0);

  if (direct3ddevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY,
          D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &vertexbuffer, NULL) != D3D_OK) 
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to create vertex buffer"));
    strcpy(previousError, allegro_error);
    return -1;
  }

  vertexbuffer->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD);

  for (i = 0; i < 4; i++)
  {
    vertices[i] = defaultVertices[i];
  }

  vertexbuffer->Unlock();

  direct3ddevice->GetGammaRamp(0, &defaultgammaramp);

  if (defaultgammaramp.red[255] < 256)
  {
    // correct bug in some gfx drivers that returns gamma ramp
    // values from 0-255 instead of 0-65535
    for (i = 0; i < 256; i++)
    {
      defaultgammaramp.red[i] *= 256;
      defaultgammaramp.green[i] *= 256;
      defaultgammaramp.blue[i] *= 256;
    }
  }
  currentgammaramp = defaultgammaramp;

  InitializeD3DState();

  return 0;
}

void D3DGraphicsDriver::InitializeD3DState()
{
  OutputDebugString("AGS -- InitializeD3DState()");

  D3DMATRIX matOrtho = {
    (2.0 / (float)_newmode_width), 0.0, 0.0, 0.0,
    0.0, (2.0 / (float)_newmode_height), 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  D3DMATRIX matIdentity = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(30, 0, 0, 255), 0.5f, 0);

  // set the render flags.
  direct3ddevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  direct3ddevice->SetRenderState(D3DRS_LIGHTING, true);
  direct3ddevice->SetRenderState(D3DRS_ZENABLE, FALSE);

  direct3ddevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  direct3ddevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  direct3ddevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

  direct3ddevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  direct3ddevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
  direct3ddevice->SetRenderState(D3DRS_ALPHAREF, (DWORD)0);

  direct3ddevice->SetFVF(D3DFVF_CUSTOMVERTEX);

  //Setup orthographic projection matrix
  direct3ddevice->SetTransform(D3DTS_PROJECTION, &matOrtho);
  direct3ddevice->SetTransform(D3DTS_WORLD, &matIdentity);
  direct3ddevice->SetTransform(D3DTS_VIEW, &matIdentity);

  D3DMATERIAL9 material;
  ZeroMemory(&material, sizeof(material));				//zero memory ( NEW )
  material.Diffuse.r = 1.0f;						//diffuse color ( NEW )
  material.Diffuse.g = 1.0f;
  material.Diffuse.b = 1.0f;
  direct3ddevice->SetMaterial(&material);

  D3DLIGHT9 d3dLight;
  ZeroMemory(&d3dLight, sizeof(D3DLIGHT9));
      
  // Set up a white point light.
  d3dLight.Type = D3DLIGHT_DIRECTIONAL;
  d3dLight.Diffuse.r  = 1.0f;
  d3dLight.Diffuse.g  = 1.0f;
  d3dLight.Diffuse.b  = 1.0f;
  d3dLight.Diffuse.a  = 1.0f;
  d3dLight.Ambient.r  = 1.0f;
  d3dLight.Ambient.g  = 1.0f;
  d3dLight.Ambient.b  = 1.0f;
  d3dLight.Specular.r = 1.0f;
  d3dLight.Specular.g = 1.0f;
  d3dLight.Specular.b = 1.0f;
      
  // Position it high in the scene and behind the user.
  // Remember, these coordinates are in world space, so
  // the user could be anywhere in world space, too. 
  // For the purposes of this example, assume the user
  // is at the origin of world space.
  d3dLight.Direction.x = 0.0f;
  d3dLight.Direction.y = 0.0f;
  d3dLight.Direction.z = 1.0f;
      
  // Don't attenuate.
  d3dLight.Attenuation0 = 1.0f; 
  d3dLight.Range        = 1000.0f;
      
  // Set the property information for the first light.
  direct3ddevice->SetLight(0, &d3dLight);
  direct3ddevice->LightEnable(0, TRUE);

  // See "Directly Mapping Texels to Pixels" MSDN article for why this is necessary
  _pixelRenderOffset = ((float)_newmode_width / (float)_newmode_screen_width) / 2.0f;
}

void D3DGraphicsDriver::SetRenderOffset(int x, int y)
{
  _global_x_offset = x;
  _global_y_offset = y;
}

void D3DGraphicsDriver::SetTintMethod(TintMethod method) 
{
  _legacyPixelShader = (method == TintReColourise);
}

bool D3DGraphicsDriver::Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer) 
{
  return this->Init(width, height, width, height, colourDepth, windowed, loopTimer);
}

bool D3DGraphicsDriver::Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer)
{
  if (colourDepth < 15)
  {
    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Direct3D driver does not support 256-colour games"));
    return false;
  }

  _newmode_width = virtualWidth;
  _newmode_height = virtualHeight;
  _newmode_screen_width = realWidth;
  _newmode_screen_height = realHeight;
  _newmode_depth = colourDepth;
  _newmode_refresh = 0;
  _newmode_windowed = windowed;
  _loopTimer = loopTimer;

  _filter->GetRealResolution(&_newmode_screen_width, &_newmode_screen_height);

  try
  {
    this->initD3DDLL();
    this->create_screen_tint_bitmap();
  }
  catch (Ali3DException exception)
  {
    if (exception._message != allegro_error)
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(exception._message));
    return false;
  }
  // create dummy screen bitmap
  BitmapHelper::SetScreenBitmap(
	  ConvertBitmapToSupportedColourDepth(BitmapHelper::CreateBitmap(virtualWidth, virtualHeight, colourDepth))
	  );
  return true;
}

void D3DGraphicsDriver::UnInit() 
{
  if (_screenTintLayerDDB != NULL) 
  {
    this->DestroyDDB(_screenTintLayerDDB);
    _screenTintLayerDDB = NULL;
    _screenTintSprite.bitmap = NULL;
  }
  delete _screenTintLayer;
  _screenTintLayer = NULL;

  dxmedia_shutdown_3d();
  gfx_driver = NULL;

  if (pixelShader)
  {
    pixelShader->Release();
    pixelShader = NULL;
  }

  if (direct3ddevice)
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
  }
}

D3DGraphicsDriver::~D3DGraphicsDriver()
{
  UnInit();

  if (direct3d)
  {
    direct3d->Release();
    direct3d = NULL;
  }

  if (d3dlib) 
  {
    FreeLibrary(d3dlib);
    d3dlib = NULL;
  }
}

void D3DGraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse)
{
  D3DRECT rectToClear;
  rectToClear.x1 = x1;
  rectToClear.y1 = y1;
  rectToClear.x2 = x2;
  rectToClear.y2 = y2;
  DWORD colorDword = 0;
  if (colorToUse != NULL)
    colorDword = D3DCOLOR_XRGB(colorToUse->r, colorToUse->g, colorToUse->b);

  direct3ddevice->Clear(1, &rectToClear, D3DCLEAR_TARGET, colorDword, 0.5f, 0);
}

void D3DGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination)
{
  D3DDISPLAYMODE displayMode;
  direct3ddevice->GetDisplayMode(0, &displayMode);

  IDirect3DSurface9* surface = NULL;
  if ((direct3ddevice->CreateOffscreenPlainSurface(displayMode.Width, displayMode.Height, 
                       D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL) == D3D_OK) &&
      (surface != NULL))
  {
    if (_pollingCallback)
      _pollingCallback();

    // This call is v. slow (known DX9 issue)
    if (direct3ddevice->GetFrontBufferData(0, surface) != D3D_OK)
    {
      throw Ali3DException("GetFrontBufferData failed");
    }

    if (_pollingCallback)
      _pollingCallback();

    WINDOWINFO windowInfo;
    RECT *areaToCapture = NULL;

    if (_newmode_windowed)
    {
      // Stupidly, D3D creates a screenshot of the entire
      // desktop, so we have to manually extract the
      // bit with our game's window on it
      GetWindowInfo(win_get_window(), &windowInfo);
      areaToCapture = &windowInfo.rcClient;
    }

    Bitmap *finalImage = NULL;

    if (destination->GetColorDepth() != 32)
    {
      finalImage = destination;
	  destination = BitmapHelper::CreateBitmap(destination->GetWidth(), destination->GetHeight(), 32);
    }

    Bitmap *retrieveInto = destination;

    if ((_newmode_width != _newmode_screen_width) ||
        (_newmode_height != _newmode_screen_height))
    {
      // in letterbox mode the screen is 640x480 but destination might only be 320x200
      // therefore calculate the size like this
      retrieveInto = BitmapHelper::CreateBitmap(destination->GetWidth() * _newmode_screen_width / _newmode_width, 
                                          destination->GetHeight() * _newmode_screen_height / _newmode_height,
										  32);
    }

    D3DLOCKED_RECT lockedRect;
    if (surface->LockRect(&lockedRect, areaToCapture, D3DLOCK_READONLY) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::LockRect failed");
    }

    unsigned char *surfaceData = (unsigned char*)lockedRect.pBits;
    long *sourcePtr;
    long *destPtr;
    int xOffset = 0;
    if (destination->GetHeight() < _newmode_height)
    {
      // nasty hack for letterbox
      surfaceData += (lockedRect.Pitch * ((_newmode_height - destination->GetHeight()) / 2)) * _newmode_screen_height / _newmode_height;
    }
    if (destination->GetWidth() < _newmode_width)
    {
      // hack for side borders
      xOffset += ((_newmode_width - destination->GetWidth()) / 2) * _newmode_screen_width / _newmode_width;
    }

    for (int y = 0; y < retrieveInto->GetHeight(); y++)
    {
      sourcePtr = (long*)surfaceData;
      destPtr = (long*)&retrieveInto->GetScanLine(y)[0];
      for (int x = 0; x < retrieveInto->GetWidth(); x++)
      {
        //destPtr[x] = (sourcePtr[x] & 0x00ff00) | ((sourcePtr[x] & 0x0000ff) << 16) | ((sourcePtr[x] & 0xff0000) >> 16);
        destPtr[x] = sourcePtr[x + xOffset];
      }
      surfaceData += lockedRect.Pitch;
    }

    surface->UnlockRect();
    surface->Release();

    if (_pollingCallback)
      _pollingCallback();

    Graphics graphics;
    if (retrieveInto != destination)
    {
      graphics.SetBitmap(destination);
      graphics.StretchBlt(retrieveInto, RectWH(0, 0, retrieveInto->GetWidth(), retrieveInto->GetHeight()),
                   RectWH(0, 0, destination->GetWidth(), destination->GetHeight()));
      delete retrieveInto;

      if (_pollingCallback)
        _pollingCallback();
    }

    if (finalImage != NULL)
    {
      graphics.SetBitmap(finalImage);
      graphics.Blit(destination, 0, 0, 0, 0, destination->GetWidth(), destination->GetHeight());
      delete destination;
    }
  }
  else
  {
    throw Ali3DException("CreateOffscreenPlainSurface failed");
  }

}

void D3DGraphicsDriver::RenderToBackBuffer()
{
  throw Ali3DException("D3D driver does not have a back buffer");
}

void D3DGraphicsDriver::Render()
{
  Render(None);
}

void D3DGraphicsDriver::Render(GlobalFlipType flip)
{
  if (wnd_call_proc(wnd_reset_device))
  {
    throw Ali3DFullscreenLostException();
  }

  _render(flip, true);
}

void D3DGraphicsDriver::_reDrawLastFrame()
{
  memcpy(&drawList[0], &drawListLastTime[0], sizeof(SpriteDrawListEntry) * numToDrawLastTime);
  numToDraw = numToDrawLastTime;
}

void D3DGraphicsDriver::_renderSprite(SpriteDrawListEntry *drawListEntry, bool globalLeftRightFlip, bool globalTopBottomFlip)
{
  HRESULT hr;
  D3DBitmap *bmpToDraw = drawListEntry->bitmap;
  D3DMATRIX matTransform;

  if (bmpToDraw->_transparency >= 255)
    return;

  if (bmpToDraw->_tintSaturation > 0)
  {
    // Use custom pixel shader
    float vector[8];
    if (_legacyPixelShader)
    {
      rgb_to_hsv(bmpToDraw->_red, bmpToDraw->_green, bmpToDraw->_blue, &vector[0], &vector[1], &vector[2]);
      vector[0] /= 360.0; // In HSV, Hue is 0-360
    }
    else
    {
      vector[0] = (float)bmpToDraw->_red / 256.0;
      vector[1] = (float)bmpToDraw->_green / 256.0;
      vector[2] = (float)bmpToDraw->_blue / 256.0;
    }

    vector[3] = (float)bmpToDraw->_tintSaturation / 256.0;

    if (bmpToDraw->_transparency > 0)
      vector[4] = (float)bmpToDraw->_transparency / 256.0;
    else
      vector[4] = 1.0f;

    if (bmpToDraw->_lightLevel > 0)
      vector[5] = (float)bmpToDraw->_lightLevel / 256.0;
    else
      vector[5] = 1.0f;

    direct3ddevice->SetPixelShaderConstantF(0, &vector[0], 2);
    direct3ddevice->SetPixelShader(pixelShader);
  }
  else
  {
    // Not using custom pixel shader; set up the default one
    direct3ddevice->SetPixelShader(NULL);
    int useTintRed = 255;
    int useTintGreen = 255;
    int useTintBlue = 255;
    int useTransparency = 0xff;
    int textureColorOp = D3DTOP_MODULATE;

    if ((bmpToDraw->_lightLevel > 0) && (bmpToDraw->_lightLevel < 256))
    {
      // darkening the sprite... this stupid calculation is for
      // consistency with the allegro software-mode code that does
      // a trans blend with a (8,8,8) sprite
      useTintRed = (bmpToDraw->_lightLevel * 192) / 256 + 64;
      useTintGreen = useTintRed;
      useTintBlue = useTintRed;
    }
    else if (bmpToDraw->_lightLevel > 256)
    {
      // ideally we would use a multi-stage operation here
      // because we need to do TEXTURE + (TEXTURE x LIGHT)
      // but is it worth having to set the device to 2-stage?
      textureColorOp = D3DTOP_ADD;
      useTintRed = (bmpToDraw->_lightLevel - 256) / 2;
      useTintGreen = useTintRed;
      useTintBlue = useTintRed;
    }

    if (bmpToDraw->_transparency > 0)
    {
      useTransparency = bmpToDraw->_transparency;
    }

    direct3ddevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(useTintRed, useTintGreen, useTintBlue, useTransparency));
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLOROP, textureColorOp);
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    if (bmpToDraw->_transparency == 0)
    {
      // No transparency, use texture alpha component
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,  D3DTA_TEXTURE);
    }
    else
    {
      // Fixed transparency, use (TextureAlpha x FixedTranslucency)
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,  D3DTA_TEXTURE);
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG2,  D3DTA_TFACTOR);
    }
  }

  if (bmpToDraw->_vertex == NULL)
  {
    hr = direct3ddevice->SetStreamSource(0, vertexbuffer, 0, sizeof(CUSTOMVERTEX));
  }
  else
  {
    hr = direct3ddevice->SetStreamSource(0, bmpToDraw->_vertex, 0, sizeof(CUSTOMVERTEX));
  }
  if (hr != D3D_OK) 
  {
    throw Ali3DException("IDirect3DDevice9::SetStreamSource failed");
  }

  float width = bmpToDraw->GetWidthToRender();
  float height = bmpToDraw->GetHeightToRender();
  float xProportion = (float)width / (float)bmpToDraw->_width;
  float yProportion = (float)height / (float)bmpToDraw->_height;

  bool flipLeftToRight = globalLeftRightFlip ^ bmpToDraw->_flipped;
  int drawAtX = drawListEntry->x + _global_x_offset;
  int drawAtY = drawListEntry->y + _global_y_offset;

  for (int ti = 0; ti < bmpToDraw->_numTiles; ti++)
  {
    width = bmpToDraw->_tiles[ti].width * xProportion;
    height = bmpToDraw->_tiles[ti].height * yProportion;
    float xOffs;
    float yOffs = bmpToDraw->_tiles[ti].y * yProportion;
    if (flipLeftToRight != globalLeftRightFlip)
    {
      xOffs = (bmpToDraw->_width - (bmpToDraw->_tiles[ti].x + bmpToDraw->_tiles[ti].width)) * xProportion;
    }
    else
    {
      xOffs = bmpToDraw->_tiles[ti].x * xProportion;
    }
    int thisX = drawAtX + xOffs;
    int thisY = drawAtY + yOffs;

    if (globalLeftRightFlip)
    {
      thisX = (_newmode_width - thisX) - width;
    }
    if (globalTopBottomFlip) 
    {
      thisY = (_newmode_height - thisY) - height;
    }

    thisX = (-(_newmode_width / 2)) + thisX;
    thisY = (_newmode_height / 2) - thisY;


    //Setup translation and scaling matrices
    float widthToScale = (float)width;
    float heightToScale = (float)height;
    if (flipLeftToRight)
    {
      // The usual transform changes 0..1 into 0..width
      // So first negate it (which changes 0..w into -w..0)
      widthToScale = -widthToScale;
      // and now shift it over to make it 0..w again
      thisX += width;
    }
    if (globalTopBottomFlip) 
    {
      heightToScale = -heightToScale;
      thisY -= height;
    }

    make_translated_scaling_matrix(&matTransform, (float)thisX - _pixelRenderOffset, (float)thisY + _pixelRenderOffset, widthToScale, heightToScale);

    if ((_smoothScaling) && (bmpToDraw->_stretchToHeight > 0) &&
        ((bmpToDraw->_stretchToHeight != bmpToDraw->_height) ||
         (bmpToDraw->_stretchToWidth != bmpToDraw->_width)))
    {
      direct3ddevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      direct3ddevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    }
    else 
    {
      _filter->SetSamplerStateForStandardSprite(direct3ddevice);
    }

    direct3ddevice->SetTransform(D3DTS_WORLD, &matTransform);
    direct3ddevice->SetTexture(0, bmpToDraw->_tiles[ti].texture);

    hr = direct3ddevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, ti * 4, 2);
    if (hr != D3D_OK) 
    {
      throw Ali3DException("IDirect3DDevice9::DrawPrimitive failed");
    }

  }
}

void D3DGraphicsDriver::_render(GlobalFlipType flip, bool clearDrawListAfterwards)
{
  SpriteDrawListEntry *listToDraw = drawList;
  int listSize = numToDraw;
  HRESULT hr;
  bool globalLeftRightFlip = (flip == Vertical) || (flip == Both);
  bool globalTopBottomFlip = (flip == Horizontal) || (flip == Both);

  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 128), 0.5f, 0);
  hr = direct3ddevice->BeginScene();
  if (hr != D3D_OK)
  {
    throw Ali3DException("IDirect3DDevice9::BeginScene failed");
  }

  // if showing at 2x size, the sprite can get distorted otherwise
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

  for (int i = 0; i < listSize; i++)
  {
    if (listToDraw[i].skip)
      continue;

    if (listToDraw[i].bitmap == NULL)
    {
      if (_nullSpriteCallback)
        _nullSpriteCallback(listToDraw[i].x, (int)direct3ddevice);
      else
        throw Ali3DException("Unhandled attempt to draw null sprite");

      continue;
    }

    this->_renderSprite(&listToDraw[i], globalLeftRightFlip, globalTopBottomFlip);
  }

  if (!_screenTintSprite.skip)
  {
    this->_renderSprite(&_screenTintSprite, false, false);
  }

  direct3ddevice->EndScene();

  hr = direct3ddevice->Present(NULL, NULL, NULL, NULL);

  if (clearDrawListAfterwards)
  {
    numToDrawLastTime = numToDraw;
    memcpy(&drawListLastTime[0], &drawList[0], sizeof(SpriteDrawListEntry) * listSize);
    flipTypeLastTime = flip;
    ClearDrawList();
  }
}

void D3DGraphicsDriver::ClearDrawList()
{
  numToDraw = 0;
}

void D3DGraphicsDriver::DrawSprite(int x, int y, IDriverDependantBitmap* bitmap)
{
  if (numToDraw >= MAX_DRAW_LIST_SIZE)
  {
    throw Ali3DException("Too many sprites to draw in one frame");
  }

  drawList[numToDraw].bitmap = (D3DBitmap*)bitmap;
  drawList[numToDraw].x = x;
  drawList[numToDraw].y = y;
  drawList[numToDraw].skip = false;
  numToDraw++;
}

void D3DGraphicsDriver::DestroyDDB(IDriverDependantBitmap* bitmap)
{
  for (int i = 0; i < numToDrawLastTime; i++)
  {
    if (drawListLastTime[i].bitmap == bitmap)
    {
      drawListLastTime[i].skip = true;
    }
  }
  delete ((D3DBitmap*)bitmap);
}

__inline void get_pixel_if_not_transparent15(unsigned short *pixel, unsigned short *red, unsigned short *green, unsigned short *blue, unsigned short *divisor)
{
  if (pixel[0] != MASK_COLOR_15)
  {
    *red += algetr15(pixel[0]);
    *green += algetg15(pixel[0]);
    *blue += algetb15(pixel[0]);
    divisor[0]++;
  }
}

__inline void get_pixel_if_not_transparent32(unsigned long *pixel, unsigned long *red, unsigned long *green, unsigned long *blue, unsigned long *divisor)
{
  if (pixel[0] != MASK_COLOR_32)
  {
    *red += algetr32(pixel[0]);
    *green += algetg32(pixel[0]);
    *blue += algetb32(pixel[0]);
    divisor[0]++;
  }
}

void D3DGraphicsDriver::UpdateTextureRegion(TextureTile *tile, Bitmap *bitmap, D3DBitmap *target, bool hasAlpha)
{
  IDirect3DTexture9* newTexture = tile->texture;

  D3DLOCKED_RECT lockedRegion;
  HRESULT hr = newTexture->LockRect(0, &lockedRegion, NULL, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);
  if (hr != D3D_OK)
  {
    throw Ali3DException("Unable to lock texture");
  }

  bool usingLinearFiltering = _filter->NeedToColourEdgeLines();
  bool lastPixelWasTransparent = false;
  char *memPtr = (char*)lockedRegion.pBits;
  for (int y = 0; y < tile->height; y++)
  {
    lastPixelWasTransparent = false;
    for (int x = 0; x < tile->width; x++)
    {
      if (target->_colDepth == 15)
      {
        unsigned short* memPtrShort = (unsigned short*)memPtr;
        unsigned short* srcData = (unsigned short*)&bitmap->GetScanLine(y + tile->y)[(x + tile->x) * 2];
        if (*srcData == MASK_COLOR_15) 
        {
          if (target->_opaque)  // set to black if opaque
            memPtrShort[x] = 0x8000;
          else if (!usingLinearFiltering)
            memPtrShort[x] = 0;
          // set to transparent, but use the colour from the neighbouring 
          // pixel to stop the linear filter doing black outlines
          else
          {
            unsigned short red = 0, green = 0, blue = 0, divisor = 0;
            if (x > 0)
              get_pixel_if_not_transparent15(&srcData[-1], &red, &green, &blue, &divisor);
            if (x < tile->width - 1)
              get_pixel_if_not_transparent15(&srcData[1], &red, &green, &blue, &divisor);
            if (y > 0)
              get_pixel_if_not_transparent15((unsigned short*)&bitmap->GetScanLine(y + tile->y - 1)[(x + tile->x) * 2], &red, &green, &blue, &divisor);
            if (y < tile->height - 1)
              get_pixel_if_not_transparent15((unsigned short*)&bitmap->GetScanLine(y + tile->y + 1)[(x + tile->x) * 2], &red, &green, &blue, &divisor);
            if (divisor > 0)
              memPtrShort[x] = ((red / divisor) << 10) | ((green / divisor) << 5) | (blue / divisor);
            else
              memPtrShort[x] = 0;
          }
          lastPixelWasTransparent = true;
        }
        else
        {
          memPtrShort[x] = 0x8000 | (algetr15(*srcData) << 10) | (algetg15(*srcData) << 5) | algetb15(*srcData);
          if (lastPixelWasTransparent)
          {
            // update the colour of the previous tranparent pixel, to
            // stop black outlines when linear filtering
            memPtrShort[x - 1] = memPtrShort[x] & 0x7FFF;
            lastPixelWasTransparent = false;
          }
        }
      }
      else if (target->_colDepth == 32)
      {
        unsigned long* memPtrLong = (unsigned long*)memPtr;
        unsigned long* srcData = (unsigned long*)&bitmap->GetScanLine(y + tile->y)[(x + tile->x) * 4];
        if (*srcData == MASK_COLOR_32)
        {
          if (target->_opaque)  // set to black if opaque
            memPtrLong[x] = 0xFF000000;
          else if (!usingLinearFiltering)
            memPtrLong[x] = 0;
          // set to transparent, but use the colour from the neighbouring 
          // pixel to stop the linear filter doing black outlines
          else
          {
            unsigned long red = 0, green = 0, blue = 0, divisor = 0;
            if (x > 0)
              get_pixel_if_not_transparent32(&srcData[-1], &red, &green, &blue, &divisor);
            if (x < tile->width - 1)
              get_pixel_if_not_transparent32(&srcData[1], &red, &green, &blue, &divisor);
            if (y > 0)
              get_pixel_if_not_transparent32((unsigned long*)&bitmap->GetScanLine(y + tile->y - 1)[(x + tile->x) * 4], &red, &green, &blue, &divisor);
            if (y < tile->height - 1)
              get_pixel_if_not_transparent32((unsigned long*)&bitmap->GetScanLine(y + tile->y + 1)[(x + tile->x) * 4], &red, &green, &blue, &divisor);
            if (divisor > 0)
              memPtrLong[x] = ((red / divisor) << 16) | ((green / divisor) << 8) | (blue / divisor);
            else
              memPtrLong[x] = 0;
          }
          lastPixelWasTransparent = true;
        }
        else if (hasAlpha)
        {
          memPtrLong[x] = D3DCOLOR_RGBA(algetr32(*srcData), algetg32(*srcData), algetb32(*srcData), algeta32(*srcData));
        }
        else
        {
          memPtrLong[x] = D3DCOLOR_RGBA(algetr32(*srcData), algetg32(*srcData), algetb32(*srcData), 0xff);
          if (lastPixelWasTransparent)
          {
            // update the colour of the previous tranparent pixel, to
            // stop black outlines when linear filtering
            memPtrLong[x - 1] = memPtrLong[x] & 0x00FFFFFF;
            lastPixelWasTransparent = false;
          }
        }
      }
    }

    memPtr += lockedRegion.Pitch;
  }

  newTexture->UnlockRect(0);
}

void D3DGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha)
{
  D3DBitmap *target = (D3DBitmap*)bitmapToUpdate;
  if ((target->_width == bitmap->GetWidth()) &&
     (target->_height == bitmap->GetHeight()))
  {
    if (bitmap->GetColorDepth() != target->_colDepth)
    {
      throw Ali3DException("Mismatched colour depths");
    }

    target->_hasAlpha = hasAlpha;

    for (int i = 0; i < target->_numTiles; i++)
    {
      UpdateTextureRegion(&target->_tiles[i], bitmap, target, hasAlpha);
    }
  }
}

Bitmap *D3DGraphicsDriver::ConvertBitmapToSupportedColourDepth(Bitmap *bitmap)
{
   int colorConv = get_color_conversion();
   set_color_conversion(COLORCONV_KEEP_TRANS | COLORCONV_TOTAL);

   int colourDepth = bitmap->GetColorDepth();
   Graphics graphics;
   if ((colourDepth == 8) || (colourDepth == 16))
   {
     // Most 3D cards don't support 8-bit; and we need 15-bit colour
     Bitmap *tempBmp = BitmapHelper::CreateBitmap(bitmap->GetWidth(), bitmap->GetHeight(), 15);
     graphics.SetBitmap(tempBmp);
     graphics.Blit(bitmap, 0, 0, 0, 0, tempBmp->GetWidth(), tempBmp->GetHeight());
     delete bitmap;
     set_color_conversion(colorConv);
     return tempBmp;
   }
   if (colourDepth == 24)
   {
     // we need 32-bit colour
     Bitmap* tempBmp = BitmapHelper::CreateBitmap(bitmap->GetWidth(), bitmap->GetHeight(), 32);
     graphics.SetBitmap(tempBmp);
     graphics.Blit(bitmap, 0, 0, 0, 0, tempBmp->GetWidth(), tempBmp->GetHeight());
     delete bitmap;
     set_color_conversion(colorConv);
     return tempBmp;
   }
   return bitmap;
}

void D3DGraphicsDriver::AdjustSizeToNearestSupportedByCard(int *width, int *height)
{
  int allocatedWidth = *width, allocatedHeight = *height;

  if (direct3ddevicecaps.TextureCaps & D3DPTEXTURECAPS_POW2)
  {
    bool foundWidth = false, foundHeight = false;
    int tryThis = 2;
    while ((!foundWidth) || (!foundHeight))
    {
      if ((tryThis >= allocatedWidth) && (!foundWidth))
      {
        allocatedWidth = tryThis;
        foundWidth = true;
      }

      if ((tryThis >= allocatedHeight) && (!foundHeight))
      {
        allocatedHeight = tryThis;
        foundHeight = true;
      }

      tryThis = tryThis << 1;
    }
  }

  if (direct3ddevicecaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
  {
    if (allocatedWidth > allocatedHeight) 
    {
     allocatedHeight = allocatedWidth;
    }
    else
    {
     allocatedWidth = allocatedHeight;
    }
  }

  *width = allocatedWidth;
  *height = allocatedHeight;
}

bool D3DGraphicsDriver::IsTextureFormatOk( D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat ) 
{
    HRESULT hr = direct3d->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          AdapterFormat,
                                          0,
                                          D3DRTYPE_TEXTURE,
                                          TextureFormat);
    
    return SUCCEEDED( hr );
}

IDriverDependantBitmap* D3DGraphicsDriver::CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque)
{
  int allocatedWidth = bitmap->GetWidth();
  int allocatedHeight = bitmap->GetHeight();
  Bitmap *tempBmp = NULL;
  int colourDepth = bitmap->GetColorDepth();
  Graphics graphics;
  if ((colourDepth == 8) || (colourDepth == 16))
  {
    // Most 3D cards don't support 8-bit; and we need 15-bit colour
    tempBmp = BitmapHelper::CreateBitmap(bitmap->GetWidth(), bitmap->GetHeight(), 15);
    graphics.SetBitmap(tempBmp);
    graphics.Blit(bitmap, 0, 0, 0, 0, tempBmp->GetWidth(), tempBmp->GetHeight());
    bitmap = tempBmp;
    colourDepth = 15;
  }
  if (colourDepth == 24)
  {
    // we need 32-bit colour
    tempBmp = BitmapHelper::CreateBitmap(bitmap->GetWidth(), bitmap->GetHeight(), 32);
    graphics.SetBitmap(tempBmp);
    graphics.Blit(bitmap, 0, 0, 0, 0, tempBmp->GetWidth(), tempBmp->GetHeight());
    bitmap = tempBmp;
    colourDepth = 32;
  }

  D3DBitmap *ddb = new D3DBitmap(bitmap->GetWidth(), bitmap->GetHeight(), colourDepth, opaque);

  AdjustSizeToNearestSupportedByCard(&allocatedWidth, &allocatedHeight);
  int tilesAcross = 1, tilesDown = 1;

  // *************** REMOVE THESE LINES *************
  //direct3ddevicecaps.MaxTextureWidth = 64;
  //direct3ddevicecaps.MaxTextureHeight = 256;
  // *************** END REMOVE THESE LINES *************

  // Calculate how many textures will be necessary to
  // store this image
  tilesAcross = (allocatedWidth + direct3ddevicecaps.MaxTextureWidth - 1) / direct3ddevicecaps.MaxTextureWidth;
  tilesDown = (allocatedHeight + direct3ddevicecaps.MaxTextureHeight - 1) / direct3ddevicecaps.MaxTextureHeight;
  int tileWidth = bitmap->GetWidth() / tilesAcross;
  int lastTileExtraWidth = bitmap->GetWidth() % tilesAcross;
  int tileHeight = bitmap->GetHeight() / tilesDown;
  int lastTileExtraHeight = bitmap->GetHeight() % tilesDown;
  int tileAllocatedWidth = tileWidth;
  int tileAllocatedHeight = tileHeight;

  AdjustSizeToNearestSupportedByCard(&tileAllocatedWidth, &tileAllocatedHeight);

  int numTiles = tilesAcross * tilesDown;
  TextureTile *tiles = (TextureTile*)malloc(sizeof(TextureTile) * numTiles);
  memset(tiles, 0, sizeof(TextureTile) * numTiles);

  CUSTOMVERTEX *vertices = NULL;

  if ((numTiles == 1) &&
      (allocatedWidth == bitmap->GetWidth()) &&
      (allocatedHeight == bitmap->GetHeight()))
  {
    // use default whole-image vertices
  }
  else
  {
     // The texture is not the same as the bitmap, so create some custom vertices
     // so that only the relevant portion of the texture is rendered
     int vertexBufferSize = numTiles * 4 * sizeof(CUSTOMVERTEX);
     HRESULT hr = direct3ddevice->CreateVertexBuffer(vertexBufferSize, 0,
         D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &ddb->_vertex, NULL);

     if (hr != D3D_OK) 
     {
        char errorMessage[200];
        sprintf(errorMessage, "Direct3DDevice9::CreateVertexBuffer(Length=%d) for texture failed: error code %08X", vertexBufferSize, hr);
        throw Ali3DException(errorMessage);
     }

     if (ddb->_vertex->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD) != D3D_OK)
     {
       throw Ali3DException("Failed to lock vertex buffer");
     }
  }

  for (int x = 0; x < tilesAcross; x++)
  {
    for (int y = 0; y < tilesDown; y++)
    {
      TextureTile *thisTile = &tiles[y * tilesAcross + x];
      int thisAllocatedWidth = tileAllocatedWidth;
      int thisAllocatedHeight = tileAllocatedHeight;
      thisTile->x = x * tileWidth;
      thisTile->y = y * tileHeight;
      thisTile->width = tileWidth;
      thisTile->height = tileHeight;
      if (x == tilesAcross - 1) 
      {
        thisTile->width += lastTileExtraWidth;
        thisAllocatedWidth = thisTile->width;
        AdjustSizeToNearestSupportedByCard(&thisAllocatedWidth, &thisAllocatedHeight);
      }
      if (y == tilesDown - 1) 
      {
        thisTile->height += lastTileExtraHeight;
        thisAllocatedHeight = thisTile->height;
        AdjustSizeToNearestSupportedByCard(&thisAllocatedWidth, &thisAllocatedHeight);
      }

      if (vertices != NULL)
      {
        for (int vidx = 0; vidx < 4; vidx++)
        {
          int i = (y * tilesAcross + x) * 4 + vidx;
          vertices[i] = defaultVertices[vidx];
          if (vertices[i].tu > 0.0)
          {
            vertices[i].tu = (float)thisTile->width / (float)thisAllocatedWidth;
          }
          if (vertices[i].tv > 0.0)
          {
            vertices[i].tv = (float)thisTile->height / (float)thisAllocatedHeight;
          }
        }
      }

      D3DFORMAT textureFormat = color_depth_to_d3d_format(colourDepth, !opaque);
      HRESULT hr = direct3ddevice->CreateTexture(thisAllocatedWidth, thisAllocatedHeight, 1, 0, 
                                      textureFormat,
                                      D3DPOOL_MANAGED, &thisTile->texture, NULL);
      if (hr != D3D_OK)
      {
        char errorMessage[200];
        sprintf(errorMessage, "Direct3DDevice9::CreateTexture(X=%d, Y=%d, FMT=%d) failed: error code %08X", thisAllocatedWidth, thisAllocatedHeight, textureFormat, hr);
        throw Ali3DException(errorMessage);
      }

    }
  }

  if (vertices != NULL)
  {
    ddb->_vertex->Unlock();
  }

  ddb->_numTiles = numTiles;
  ddb->_tiles = tiles;

  UpdateDDBFromBitmap(ddb, bitmap, hasAlpha);

  delete tempBmp;

  return ddb;
}

void D3DGraphicsDriver::do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
  if (fadingOut)
  {
    this->_reDrawLastFrame();
  }
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear(makecol32(targetColourRed, targetColourGreen, targetColourBlue));
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, false);
  delete blackSquare;

  d3db->SetStretch(_newmode_width, _newmode_height);
  this->DrawSprite(-_global_x_offset, -_global_y_offset, d3db);

  if (speed <= 0) speed = 16;
  speed *= 2;  // harmonise speeds with software driver which is faster
  for (int a = 1; a < 255; a += speed)
  {
    int timerValue = *_loopTimer;
    d3db->SetTransparency(fadingOut ? a : (255 - a));
    this->_render(flipTypeLastTime, false);

    do
    {
      if (_pollingCallback)
        _pollingCallback();
      platform->YieldCPU();
    }
    while (timerValue == *_loopTimer);

  }

  if (fadingOut)
  {
    d3db->SetTransparency(0);
    this->_render(flipTypeLastTime, false);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawList();
}

void D3DGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(true, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void D3DGraphicsDriver::FadeIn(int speed, PALLETE p, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(false, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void D3DGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
{
  if (blackingOut)
  {
    this->_reDrawLastFrame();
  }
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear();
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, false);
  delete blackSquare;

  d3db->SetStretch(_newmode_width, _newmode_height);
  this->DrawSprite(0, 0, d3db);
  if (!blackingOut)
  {
    // when fading in, draw four black boxes, one
    // across each side of the screen
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
  }

  int yspeed = _newmode_height / (_newmode_width / speed);
  int boxWidth = speed;
  int boxHeight = yspeed;

  while (boxWidth < _newmode_width)
  {
    boxWidth += speed;
    boxHeight += yspeed;
    if (blackingOut)
    {
      this->drawList[this->numToDraw - 1].x = _newmode_width / 2- boxWidth / 2;
      this->drawList[this->numToDraw - 1].y = _newmode_height / 2 - boxHeight / 2;
      d3db->SetStretch(boxWidth, boxHeight);
    }
    else
    {
      this->drawList[this->numToDraw - 4].x = _newmode_width / 2 - boxWidth / 2 - _newmode_width;
      this->drawList[this->numToDraw - 3].y = _newmode_height / 2 - boxHeight / 2 - _newmode_height;
      this->drawList[this->numToDraw - 2].x = _newmode_width / 2 + boxWidth / 2;
      this->drawList[this->numToDraw - 1].y = _newmode_height / 2 + boxHeight / 2;
      d3db->SetStretch(_newmode_width, _newmode_height);
    }
    
    this->_render(flipTypeLastTime, false);

    if (_pollingCallback)
      _pollingCallback();
    platform->Delay(delay);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawList();
}

bool D3DGraphicsDriver::PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen)
{
  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 0.5f, 0);

  int result = dxmedia_play_video_3d(filename, direct3ddevice, useAVISound, skipType, stretchToFullScreen ? 1 : 0);
  return (result == 0);
}

void D3DGraphicsDriver::create_screen_tint_bitmap() 
{
  _screenTintLayer = BitmapHelper::CreateBitmap(16, 16, this->_newmode_depth);
  _screenTintLayer = this->ConvertBitmapToSupportedColourDepth(_screenTintLayer);
  _screenTintLayerDDB = (D3DBitmap*)this->CreateDDBFromBitmap(_screenTintLayer, false, false);
  _screenTintSprite.bitmap = _screenTintLayerDDB;
}

void D3DGraphicsDriver::SetScreenTint(int red, int green, int blue)
{ 
  if ((red != _tint_red) || (green != _tint_green) || (blue != _tint_blue))
  {
    _tint_red = red; 
    _tint_green = green; 
    _tint_blue = blue;

    _screenTintLayer->Clear(makecol_depth(_screenTintLayer->GetColorDepth(), red, green, blue));
    this->UpdateDDBFromBitmap(_screenTintLayerDDB, _screenTintLayer, false);
    _screenTintLayerDDB->SetStretch(_newmode_width, _newmode_height);
    _screenTintLayerDDB->SetTransparency(128);

    _screenTintSprite.skip = ((red == 0) && (green == 0) && (blue == 0));
  }
}

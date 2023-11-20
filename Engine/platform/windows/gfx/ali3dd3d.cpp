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

#include "core/platform.h"

#if AGS_HAS_DIRECT3D
#define NOMINMAX
#include "platform/windows/gfx/ali3dd3d.h"
#include <algorithm>
#include <stack>
#include <SDL.h>
#include <glm/ext.hpp>
#include "ac/sys_events.h"
#include "ac/timer.h"
#include "debug/out.h"
#include "gfx/ali3dexception.h"
#include "gfx/gfx_def.h"
#include "gfx/gfxfilter_d3d.h"
#include "gfx/gfxfilter_aad3d.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "util/matrix.h"

#define AGS_D3DBLENDOP(blend_op, src_blend, dest_blend) \
  direct3ddevice->SetRenderState(D3DRS_BLENDOP, blend_op); \
  direct3ddevice->SetRenderState(D3DRS_SRCBLEND, src_blend); \
  direct3ddevice->SetRenderState(D3DRS_DESTBLEND, dest_blend); \

using namespace AGS::Common;

// Necessary to update textures from 8-bit bitmaps
extern RGB palette[256];

namespace AGS
{
namespace Engine
{
namespace D3D
{

using namespace Common;

D3DTexture::~D3DTexture()
{
    if (_tiles)
    {
        for (size_t i = 0; i < _numTiles; ++i)
            _tiles[i].texture->Release();
        delete[] _tiles;
    }
    if (_vertex)
    {
        _vertex->Release();
    }
}

size_t D3DTexture::GetMemSize() const
{
    // FIXME: a proper size in video memory, check Direct3D docs
    size_t sz = 0u;
    for (size_t i = 0; i < _numTiles; ++i)
        sz += _tiles[i].width * _tiles[i].height * 4;
    return sz;
}

D3DBitmap::~D3DBitmap()
{
    if (_renderSurface)
        _renderSurface->Release();
}

void D3DBitmap::ReleaseTextureData()
{
    if (_renderSurface)
        _renderSurface->Release();
    _renderSurface = nullptr;
    _data.reset();
}


static int d3d_format_to_color_depth(D3DFORMAT format, bool secondary);

bool D3DGfxModeList::GetMode(int index, DisplayMode &mode) const
{
    if (_direct3d && index >= 0 && index < _modeCount)
    {
        D3DDISPLAYMODE d3d_mode;
        if (SUCCEEDED(_direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, _pixelFormat, index, &d3d_mode)))
        {
            mode.Width = d3d_mode.Width;
            mode.Height = d3d_mode.Height;
            mode.ColorDepth = d3d_format_to_color_depth(d3d_mode.Format, false);
            mode.RefreshRate = d3d_mode.RefreshRate;
            return true;
        }
    }
    return false;
}


// The custom FVF, which describes the custom vertex structure.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)


D3DGraphicsDriver::D3DGraphicsDriver(IDirect3D9 *d3d) 
{
  direct3d = d3d;
  direct3ddevice = NULL;
  vertexbuffer = NULL;
  pixelShader = NULL;
  _legacyPixelShader = false;
  set_up_default_vertices();
  pNativeSurface = NULL;
  pNativeTexture = NULL;
  _smoothScaling = false;
  _pixelRenderXOffset = 0;
  _pixelRenderYOffset = 0;
  _renderSprAtScreenRes = false;

  // Shifts comply to D3DFMT_A8R8G8B8
  _vmem_a_shift_32 = 24;
  _vmem_r_shift_32 = 16;
  _vmem_g_shift_32 = 8;
  _vmem_b_shift_32 = 0;
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

void D3DGraphicsDriver::OnModeSet(const DisplayMode &mode)
{
  GraphicsDriverBase::OnModeSet(mode);

  // The display mode has been set up successfully, save the
  // final refresh rate that we are using
  D3DDISPLAYMODE final_display_mode;
  if (direct3ddevice->GetDisplayMode(0, &final_display_mode) == D3D_OK) {
    _mode.RefreshRate = final_display_mode.RefreshRate;
  }
  else {
    _mode.RefreshRate = 0;
  }
}

void D3DGraphicsDriver::ReleaseDisplayMode()
{
  if (!IsModeSet())
    return;

  OnModeReleased();
  ClearDrawLists();
  ClearDrawBackups();
  DestroyFxPool();
  DestroyAllStageScreens();

  sys_window_set_style(kWnd_Windowed);
}

bool D3DGraphicsDriver::FirstTimeInit()
{
  HRESULT hr;

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
    SDL_SetError("Graphics card does not support Pixel Shader %d.%d", requiredPSMajorVersion, requiredPSMinorVersion);
    previousError = SDL_GetError();
    return false;
  }

  _capsVsync = (direct3ddevicecaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE) != 0;
  if (!_capsVsync)
    Debug::Printf(kDbgMsg_Warn, "WARNING: Vertical sync is not supported. Setting will be kept at driver default.");

  // Load the pixel shader!!
  HMODULE exeHandle = GetModuleHandle(NULL);
  HRSRC hRes = FindResource(exeHandle, (_legacyPixelShader) ? "PIXEL_SHADER_LEGACY" : "PIXEL_SHADER", "DATA");
  if (hRes)
  {
    HGLOBAL hGlobal = LoadResource(exeHandle, hRes);
    if (hGlobal)
    {
      DWORD *dataPtr = (DWORD*)LockResource(hGlobal);
      hr = direct3ddevice->CreatePixelShader(dataPtr, &pixelShader);
      if (hr != D3D_OK)
      {
        direct3ddevice->Release();
        direct3ddevice = NULL;
        SDL_SetError("Failed to create pixel shader: 0x%08X", hr);
        previousError = SDL_GetError();
        return false;
      }
      UnlockResource(hGlobal);
    }
  }
  
  if (pixelShader == NULL)
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    SDL_SetError("Failed to load pixel shader resource");
    previousError = SDL_GetError();
    return false;
  }

  if (direct3ddevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY,
          D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &vertexbuffer, NULL) != D3D_OK) 
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    SDL_SetError("Failed to create vertex buffer");
    previousError = SDL_GetError();
    return false;
  }

  // This line crashes because my card doesn't support 8-bit textures
  //direct3ddevice->SetCurrentTexturePalette(0);

  CUSTOMVERTEX *vertices;
  vertexbuffer->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD);

  for (int i = 0; i < 4; i++)
  {
    vertices[i] = defaultVertices[i];
  }

  vertexbuffer->Unlock();

  direct3ddevice->GetGammaRamp(0, &defaultgammaramp);

  if (defaultgammaramp.red[255] < 256)
  {
    // correct bug in some gfx drivers that returns gamma ramp
    // values from 0-255 instead of 0-65535
    for (int i = 0; i < 256; i++)
    {
      defaultgammaramp.red[i] *= 256;
      defaultgammaramp.green[i] *= 256;
      defaultgammaramp.blue[i] *= 256;
    }
  }
  currentgammaramp = defaultgammaramp;

  return true;
}

/* color_depth_to_d3d_format:
 *  Convert a colour depth into the appropriate D3D tag
 */
static D3DFORMAT color_depth_to_d3d_format(int color_depth, bool want_alpha = true)
{
    if (want_alpha)
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
        case 15:
            return D3DFMT_X1R5G5B5;
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

/* d3d_format_to_color_depth:
 *  Convert a D3D tag to colour depth
 *
 * TODO: this is currently an inversion of color_depth_to_d3d_format;
 * check later if more formats should be handled
 */
static int d3d_format_to_color_depth(D3DFORMAT format, bool secondary)
{
  switch (format)
  {
  case D3DFMT_P8:
    return 8;
  case D3DFMT_A1R5G5B5:
    return secondary ? 15 : 16;
  case D3DFMT_X1R5G5B5:
    return secondary ? 15 : 16;
  case D3DFMT_R5G6B5:
    return 16;
  case D3DFMT_R8G8B8:
    return secondary ? 24 : 32;
  case D3DFMT_A8R8G8B8:
  case D3DFMT_X8R8G8B8:
    return 32;
  }
  return 0;
}

bool D3DGraphicsDriver::IsModeSupported(const DisplayMode &mode)
{
  if (mode.Width <= 0 || mode.Height <= 0 || mode.ColorDepth <= 0)
  {
    SDL_SetError("Invalid resolution parameters: %d x %d x %d", mode.Width, mode.Height, mode.ColorDepth);
    return false;
  }

  if (!mode.IsRealFullscreen())
  {
    return true;
  }

  D3DFORMAT pixelFormat = color_depth_to_d3d_format(mode.ColorDepth, false /* opaque */);
  D3DDISPLAYMODE d3d_mode;

  int mode_count = direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, pixelFormat);
  for (int i = 0; i < mode_count; i++)
  {
    if (FAILED(direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, pixelFormat, i, &d3d_mode)))
    {
      SDL_SetError("IDirect3D9::EnumAdapterModes failed");
      return false;
    }

    if ((d3d_mode.Width == static_cast<uint32_t>(mode.Width)) &&
        (d3d_mode.Height == static_cast<uint32_t>(mode.Height)))
    {
      return true;
    }
  }

  SDL_SetError("The requested adapter mode is not supported");
  return false;
}

bool D3DGraphicsDriver::SupportsGammaControl() 
{
  if ((direct3ddevicecaps.Caps2 & D3DCAPS2_FULLSCREENGAMMA) == 0)
    return false;

  if (!_mode.IsRealFullscreen())
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

void D3DGraphicsDriver::ResetDeviceIfNecessary()
{
  HRESULT hr = direct3ddevice->TestCooperativeLevel();
  if (hr == D3DERR_DEVICELOST)
  {
    throw Ali3DFullscreenLostException();
  }

  if (hr == D3DERR_DEVICENOTRESET)
  {
    hr = ResetD3DDevice();
    if (hr != D3D_OK)
    {
      throw Ali3DException(String::FromFormat("IDirect3DDevice9::Reset: failed: error code: 0x%08X", hr));
    }

    InitializeD3DState();
    CreateVirtualScreen();
    direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);
  }
  else if (hr != D3D_OK)
  {
    throw Ali3DException(String::FromFormat("IDirect3DDevice9::TestCooperativeLevel: failed: error code: 0x%08X", hr));
  }
}

bool D3DGraphicsDriver::CreateDisplayMode(const DisplayMode &mode)
{
  if (!IsModeSupported(mode))
    return false;

  SDL_Window *window = sys_get_window();
  if (!window)
  {
    sys_window_create("", mode.Width, mode.Height, mode.Mode);
  }

  HWND hwnd = (HWND)sys_win_get_window();
  memset( &d3dpp, 0, sizeof(d3dpp) );
  d3dpp.BackBufferWidth = mode.Width;
  d3dpp.BackBufferHeight = mode.Height;
  d3dpp.BackBufferFormat = color_depth_to_d3d_format(mode.ColorDepth, false /* opaque */);
  d3dpp.BackBufferCount = 1;
  d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
  // THIS MUST BE SWAPEFFECT_COPY FOR PlayVideo TO WORK
  d3dpp.SwapEffect = D3DSWAPEFFECT_COPY; //D3DSWAPEFFECT_DISCARD; 
  d3dpp.hDeviceWindow = hwnd;
  d3dpp.Windowed = mode.IsRealFullscreen() ? FALSE : TRUE;
  d3dpp.EnableAutoDepthStencil = FALSE;
  d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // we need this flag to access the backbuffer with lockrect
  d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
  if (mode.Vsync)
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
  else
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  /* If full screen, specify the refresh rate */
  // TODO find a way to avoid the wrong refreshrate to be set on mode.RefreshRate
  // for now it's best to let it set automatically, so we prevent alt tab delays due to mismatching refreshrates
  //if ((d3dpp.Windowed == FALSE) && (mode.RefreshRate > 0))
  //  d3dpp.FullScreen_RefreshRateInHz = mode.RefreshRate;

  if (_initGfxCallback != NULL)
    _initGfxCallback(&d3dpp);

  bool first_time_init = direct3ddevice == NULL;
  HRESULT hr = 0;
  if (direct3ddevice)
  {
    hr = ResetD3DDevice();
  }
  else
  {
    hr = direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                      D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,  // multithreaded required for AVI player
                      &d3dpp, &direct3ddevice);
  }
  if (hr != D3D_OK)
  {
    if (!previousError.IsEmpty())
      SDL_SetError(previousError.GetCStr());
    else
      SDL_SetError("Failed to create Direct3D Device: 0x%08X", hr);
    return false;
  }

  if (first_time_init)
  {
    if (!FirstTimeInit())
      return false;
  }
  else
  {
    // Only adjust window styles in non-fullscreen modes:
    // for real fullscreen Direct3D will adjust the window and display.
    if (!mode.IsRealFullscreen())
    {
      sys_window_set_style(mode.Mode, Size(mode.Width, mode.Height));
    }
  }
  return true;
}

void D3DGraphicsDriver::SetBlendOp(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor)
{
    direct3ddevice->SetRenderState(D3DRS_BLENDOP, blend_op);
    direct3ddevice->SetRenderState(D3DRS_SRCBLEND, src_factor);
    direct3ddevice->SetRenderState(D3DRS_DESTBLEND, dst_factor);
}

void D3DGraphicsDriver::SetBlendOpAlpha(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor)
{
    direct3ddevice->SetRenderState(D3DRS_BLENDOPALPHA, blend_op);
    direct3ddevice->SetRenderState(D3DRS_SRCBLENDALPHA, src_factor);
    direct3ddevice->SetRenderState(D3DRS_DESTBLENDALPHA, dst_factor);
}

void D3DGraphicsDriver::InitializeD3DState()
{
  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 0.5f, 0);

  // set the render flags.
  direct3ddevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  direct3ddevice->SetRenderState(D3DRS_LIGHTING, true);
  direct3ddevice->SetRenderState(D3DRS_ZENABLE, FALSE);

  direct3ddevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  SetBlendOp(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);

  direct3ddevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  direct3ddevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
  direct3ddevice->SetRenderState(D3DRS_ALPHAREF, (DWORD)0);

  direct3ddevice->SetFVF(D3DFVF_CUSTOMVERTEX);

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

  // If we already have a render frame configured, then setup viewport immediately
  SetupViewport();
}

void D3DGraphicsDriver::SetupViewport()
{
  if (!IsModeSet() || !IsRenderFrameValid() || !IsNativeSizeValid())
    return;

  const float src_width = _srcRect.GetWidth();
  const float src_height = _srcRect.GetHeight();
  const float disp_width = _mode.Width;
  const float disp_height = _mode.Height;

  // Setup orthographic projection matrix
  glm::mat4 identity(1.f);
  glm::mat4 mat_ortho = glmex::ortho_d3d(src_width, src_height);

  direct3ddevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)glm::value_ptr(identity));
  direct3ddevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)glm::value_ptr(identity));
  direct3ddevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)glm::value_ptr(mat_ortho));

  // See "Directly Mapping Texels to Pixels" MSDN article for why this is necessary
  // http://msdn.microsoft.com/en-us/library/windows/desktop/bb219690.aspx
  _pixelRenderXOffset = (src_width / disp_width) / 2.0f;
  _pixelRenderYOffset = (src_height / disp_height) / 2.0f;

  // Clear the screen before setting a viewport.
  ClearScreenRect(RectWH(0, 0, _mode.Width, _mode.Height), nullptr);

  // Set Viewport.
  SetD3DViewport(_dstRect);

  viewport_rect.left   = _dstRect.Left;
  viewport_rect.right  = _dstRect.Right + 1;
  viewport_rect.top    = _dstRect.Top;
  viewport_rect.bottom = _dstRect.Bottom + 1;

  // View and Projection matrixes are currently fixed in Direct3D renderer
  _stageMatrixes.View = identity;
  _stageMatrixes.Projection = mat_ortho;
}

void D3DGraphicsDriver::SetGraphicsFilter(PD3DFilter filter)
{
  _filter = filter;
  OnSetFilter();
}

void D3DGraphicsDriver::SetTintMethod(TintMethod method) 
{
  _legacyPixelShader = (method == TintReColourise);
}

bool D3DGraphicsDriver::SetDisplayMode(const DisplayMode &mode)
{
  ReleaseDisplayMode();

  if (mode.ColorDepth < 15)
  {
    SDL_SetError("Direct3D driver does not support 256-color display mode");
    return false;
  }

  if (!CreateDisplayMode(mode))
    return false;

  OnInit();
  OnModeSet(mode);
  InitializeD3DState();
  CreateVirtualScreen();
  return true;
}

void D3DGraphicsDriver::UpdateDeviceScreen(const Size &screen_sz)
{
  if (_mode.IsRealFullscreen())
    return; // ignore in exclusive fs mode

  _mode.Width = screen_sz.Width;
  _mode.Height = screen_sz.Height;
  // TODO: following resets D3D9 device, which may be sub-optimal;
  // there seem to be an option to not do this if new window size is smaller
  // and SWAPEFFECT_COPY flag is set, in which case (supposedly) we could
  // draw using same device parameters, but adjusting viewport accordingly.
  d3dpp.BackBufferWidth = _mode.Width;
  d3dpp.BackBufferHeight = _mode.Height;
  HRESULT hr = ResetD3DDevice();
  if (hr != D3D_OK)
  {
      Debug::Printf("D3DGraphicsDriver: Failed to reset D3D device");
      return;
  }
  InitializeD3DState();
  CreateVirtualScreen();
  direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);
  SetupViewport();
}

void D3DGraphicsDriver::CreateVirtualScreen()
{
  if (!IsModeSet() || !IsNativeSizeValid())
    return;

  // set up native surface
  if (pNativeSurface != NULL)
  {
    pNativeSurface->Release();
    pNativeSurface = NULL;
  }
  if (pNativeTexture != NULL)
  {
      pNativeTexture->Release();
      pNativeTexture = NULL;
  }
  if (direct3ddevice->CreateTexture(
      _srcRect.GetWidth(),
      _srcRect.GetHeight(),
      1,
      D3DUSAGE_RENDERTARGET,
      color_depth_to_d3d_format(_mode.ColorDepth, false /* opaque */),
      D3DPOOL_DEFAULT,
      &pNativeTexture,
      NULL) != D3D_OK)
  {
      throw Ali3DException("CreateTexture failed");
  }
  if (pNativeTexture->GetSurfaceLevel(0, &pNativeSurface) != D3D_OK)
  {
      throw Ali3DException("GetSurfaceLevel failed");
  }

  direct3ddevice->ColorFill(pNativeSurface, NULL, 0);

  // Preset initial stage screen for plugin raw drawing
  SetStageScreen(0, _srcRect.GetSize());
}

HRESULT D3DGraphicsDriver::ResetD3DDevice()
{
    // Direct3D documentation:
    // Before calling the IDirect3DDevice9::Reset method for a device,
    // an application should release any explicit render targets, depth stencil
    // surfaces, additional swap chains, state blocks, and D3DPOOL_DEFAULT
    // resources associated with the device.
    if (pNativeSurface)
    {
        pNativeSurface->Release();
        pNativeSurface = nullptr;
    }
    if (pNativeTexture)
    {
        pNativeTexture->Release();
        pNativeTexture = nullptr;
    }
    ReleaseRenderTargetData();
    HRESULT hr = direct3ddevice->Reset(&d3dpp);
    if (hr != D3D_OK)
        return hr;
    RecreateRenderTargets();
    return D3D_OK;
}

void D3DGraphicsDriver::ReleaseRenderTargetData()
{
    // Remove RT surface ref from all the existing batches
    for (auto &batch : _spriteBatches)
    {
        if (batch.RenderSurface)
        {
            batch.RenderSurface = nullptr;
        }
    }
    // Release the RTs internal data
    for (auto &ddb : _renderTargets)
    {
        ddb->ReleaseTextureData();
    }
}

void D3DGraphicsDriver::RecreateRenderTargets()
{
    for (auto &ddb : _renderTargets)
    {
        ddb->ReleaseTextureData();
        // FIXME: this ugly accessing internal texture members
        ddb->_data.reset(reinterpret_cast<D3DTexture*>(CreateTexture(ddb->_width, ddb->_height, ddb->_colDepth, ddb->_opaque, true)));
        IDirect3DTexture9 *tex = ddb->_data->_tiles->texture;
        HRESULT hr = tex->GetSurfaceLevel(0, &ddb->_renderSurface);
        assert(hr == D3D_OK);
    }
    // Reappoint RT surfaces in existing batches
    for (auto &batch : _spriteBatches)
    {
        if (batch.RenderTarget)
        {
            assert(!batch.RenderSurface);
            batch.RenderSurface = ((D3DBitmap*)batch.RenderTarget)->_renderSurface;
        }
    }
}

bool D3DGraphicsDriver::SetNativeResolution(const GraphicResolution &native_res)
{
  OnSetNativeRes(native_res);
  // Also make sure viewport is updated using new native & destination rectangles
  SetupViewport();
  CreateVirtualScreen();
  return !_srcRect.IsEmpty();
}

bool D3DGraphicsDriver::SetRenderFrame(const Rect &dst_rect)
{
  OnSetRenderFrame(dst_rect);
  // Also make sure viewport is updated using new native & destination rectangles
  SetupViewport();
  return !_dstRect.IsEmpty();
}

int D3DGraphicsDriver::GetDisplayDepthForNativeDepth(int /*native_color_depth*/) const
{
    // TODO: check for device caps to know which depth is supported?
    return 32;
}

IGfxModeList *D3DGraphicsDriver::GetSupportedModeList(int color_depth)
{
  return new D3DGfxModeList(direct3d, color_depth_to_d3d_format(color_depth, false /* opaque */));
}

PGfxFilter D3DGraphicsDriver::GetGraphicsFilter() const
{
    return _filter;
}

void D3DGraphicsDriver::UnInit() 
{
  OnUnInit();
  ReleaseDisplayMode();

  if (pNativeSurface)
  {
    pNativeSurface->Release();
    pNativeSurface = NULL;
  }
  if (pNativeTexture)
  {
      pNativeTexture->Release();
      pNativeTexture = NULL;
  }

  if (vertexbuffer)
  {
    vertexbuffer->Release();
    vertexbuffer = NULL;
  }

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

  sys_window_destroy();
}

D3DGraphicsDriver::~D3DGraphicsDriver()
{
  D3DGraphicsDriver::UnInit();

  if (direct3d)
    direct3d->Release();
}

void D3DGraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse)
{
  // NOTE: this function is practically useless at the moment, because D3D redraws whole game frame each time
  if (!direct3ddevice) return;
  Rect r(x1, y1, x2, y2);
  r = _scaling.ScaleRange(r);
  ClearScreenRect(r, colorToUse);
}

void D3DGraphicsDriver::ClearScreenRect(const Rect &r, RGB *colorToUse)
{
  D3DRECT rectToClear;
  rectToClear.x1 = r.Left;
  rectToClear.y1 = r.Top;
  rectToClear.x2 = r.Right + 1;
  rectToClear.y2 = r.Bottom + 1;
  DWORD colorDword = 0;
  if (colorToUse != NULL)
    colorDword = D3DCOLOR_XRGB(colorToUse->r, colorToUse->g, colorToUse->b);
  direct3ddevice->Clear(1, &rectToClear, D3DCLEAR_TARGET, colorDword, 0.5f, 0);
}

bool D3DGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt)
{
  // Currently don't support copying in screen resolution when we are rendering in native
  if (!_renderSprAtScreenRes)
      at_native_res = true;
  Size need_size = at_native_res ? _srcRect.GetSize() : _dstRect.GetSize();
  if (destination->GetColorDepth() != _mode.ColorDepth || destination->GetSize() != need_size)
  {
    if (want_fmt)
      *want_fmt = GraphicResolution(need_size.Width, need_size.Height, _mode.ColorDepth);
    return false;
  }
  // If we are rendering sprites at the screen resolution, and requested native res,
  // re-render last frame to the native surface
  if (at_native_res && _renderSprAtScreenRes)
  {
    _renderSprAtScreenRes = false;
    _reDrawLastFrame();
    _render(true);
    _renderSprAtScreenRes = true;
  }
  
  IDirect3DSurface9* surface = NULL;
  {
    if (_pollingCallback)
      _pollingCallback();

    if (at_native_res)
    {
      // with render to texture the texture mipmap surface can't be locked directly
      // we have to create a surface with D3DPOOL_SYSTEMMEM for GetRenderTargetData
      if (direct3ddevice->CreateOffscreenPlainSurface(
        _srcRect.GetWidth(),
        _srcRect.GetHeight(),
        color_depth_to_d3d_format(_mode.ColorDepth, false /* opaque */),
        D3DPOOL_SYSTEMMEM,
        &surface,
        NULL) != D3D_OK)
      {
        throw Ali3DException("CreateOffscreenPlainSurface failed");
      }
      if (direct3ddevice->GetRenderTargetData(pNativeSurface, surface) != D3D_OK)
      {
        throw Ali3DException("GetRenderTargetData failed");
      }
      
    }
    // Get the back buffer surface
    else if (direct3ddevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface) != D3D_OK)
    {
      throw Ali3DException("IDirect3DDevice9::GetBackBuffer failed");
    }

    if (_pollingCallback)
      _pollingCallback();

    D3DLOCKED_RECT lockedRect;
    if (surface->LockRect(&lockedRect, (at_native_res ? NULL : &viewport_rect), D3DLOCK_READONLY ) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::LockRect failed");
    }

    BitmapHelper::ReadPixelsFromMemory(destination, (uint8_t*)lockedRect.pBits, lockedRect.Pitch);

    surface->UnlockRect();
    surface->Release();

    if (_pollingCallback)
      _pollingCallback();
  }
  return true;
}

void D3DGraphicsDriver::RenderToBackBuffer()
{
  throw Ali3DException("D3D driver does not have a back buffer");
}

void D3DGraphicsDriver::Render()
{
  Render(0, 0, kFlip_None);
}

void D3DGraphicsDriver::Render(int /*xoff*/, int /*yoff*/, GraphicFlip /*flip*/)
{
  ResetDeviceIfNecessary();
  _renderAndPresent(true);
}

void D3DGraphicsDriver::_reDrawLastFrame()
{
  RestoreDrawLists();
}

void D3DGraphicsDriver::_renderSprite(const D3DDrawListEntry *drawListEntry, const glm::mat4 &matGlobal,
    const SpriteColorTransform &color, const Size &surface_size)
{
  HRESULT hr;
  D3DBitmap *bmpToDraw = drawListEntry->ddb;

  const int alpha = (color.Alpha * bmpToDraw->_alpha) / 255;

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
    vector[4] = (float)alpha / 256.0;

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

    direct3ddevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(useTintRed, useTintGreen, useTintBlue, alpha));
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLOROP, textureColorOp);
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    if (alpha == 255)
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

  const auto *txdata = bmpToDraw->_data.get();
  if (txdata->_vertex == NULL)
  {
    hr = direct3ddevice->SetStreamSource(0, vertexbuffer, 0, sizeof(CUSTOMVERTEX));
  }
  else
  {
    hr = direct3ddevice->SetStreamSource(0, txdata->_vertex, 0, sizeof(CUSTOMVERTEX));
  }
  if (hr != D3D_OK) 
  {
    throw Ali3DException("IDirect3DDevice9::SetStreamSource failed");
  }

  float width = bmpToDraw->GetWidthToRender();
  float height = bmpToDraw->GetHeightToRender();
  float xProportion = width / (float)bmpToDraw->_width;
  float yProportion = height / (float)bmpToDraw->_height;
  float drawAtX = drawListEntry->x;
  float drawAtY = drawListEntry->y;

  for (size_t ti = 0; ti < txdata->_numTiles; ++ti)
  {
    width = txdata->_tiles[ti].width * xProportion;
    height = txdata->_tiles[ti].height * yProportion;
    float xOffs;
    float yOffs = txdata->_tiles[ti].y * yProportion;
    if (bmpToDraw->_flipped)
      xOffs = (bmpToDraw->_width - (txdata->_tiles[ti].x + txdata->_tiles[ti].width)) * xProportion;
    else
      xOffs = txdata->_tiles[ti].x * xProportion;
    float thisX = drawAtX + xOffs;
    float thisY = drawAtY + yOffs;
    thisX = (-(surface_size.Width / 2.0f)) + thisX;
    thisY = (surface_size.Height / 2.0f) - thisY;

    //Setup translation and scaling matrices
    float widthToScale = width;
    float heightToScale = height;
    if (bmpToDraw->_flipped)
    {
      // The usual transform changes 0..1 into 0..width
      // So first negate it (which changes 0..w into -w..0)
      widthToScale = -widthToScale;
      // and now shift it over to make it 0..w again
      thisX += width;
    }
    // Apply sprite origin
    thisX -= abs(widthToScale) * bmpToDraw->_originX;
    thisY += heightToScale * bmpToDraw->_originY; // inverse axis
    // Setup rotation and pivot
    float rotZ = bmpToDraw->_rotation;
    float pivotX = -(widthToScale * 0.5), pivotY = (heightToScale * 0.5);

    // Self sprite transform (first scale, then rotate and then translate, reversed)
    glm::mat4 transform = glmex::make_transform2d(
        thisX - _pixelRenderXOffset, thisY + _pixelRenderYOffset, widthToScale, heightToScale,
        rotZ, pivotX, pivotY);
    // Global batch transform
    transform = matGlobal * transform;

    if ((_smoothScaling) && bmpToDraw->_useResampler && (bmpToDraw->_stretchToHeight > 0) &&
        ((bmpToDraw->_stretchToHeight != bmpToDraw->_height) ||
         (bmpToDraw->_stretchToWidth != bmpToDraw->_width)))
    {
      direct3ddevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      direct3ddevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    }
    else if (!_renderSprAtScreenRes)
    {
      direct3ddevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      direct3ddevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    }
    else
    {
      _filter->SetSamplerStateForStandardSprite(direct3ddevice);
    }

    direct3ddevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)glm::value_ptr(transform));
    direct3ddevice->SetTexture(0, txdata->_tiles[ti].texture);

    // Treat special render modes
    switch (bmpToDraw->_renderHint)
    {
    case kTxHint_PremulAlpha:
        direct3ddevice->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_RGBA(alpha, alpha, alpha, 255));
        SetBlendOp(D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_INVSRCALPHA);
        break;
    default:
        break;
    }

    // FIXME: user blend modes break the above special blend for RT textures
    // Blend modes
    switch (bmpToDraw->_blendMode) {
        // blend mode is always NORMAL at this point
        //case kBlend_Alpha: AGS_D3DBLENDOP(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA); break; // ALPHA
        case kBlend_Add: AGS_D3DBLENDOP(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ONE); break; // ADD (transparency = strength)
        case kBlend_Darken: AGS_D3DBLENDOP(D3DBLENDOP_MIN, D3DBLEND_ONE, D3DBLEND_ONE); break; // DARKEN
        case kBlend_Lighten: AGS_D3DBLENDOP(D3DBLENDOP_MAX, D3DBLEND_ONE, D3DBLEND_ONE); break; // LIGHTEN
        case kBlend_Multiply: AGS_D3DBLENDOP(D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCCOLOR); break; // MULTIPLY
        case kBlend_Screen: AGS_D3DBLENDOP(D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR); break; // SCREEN
        case kBlend_Subtract: AGS_D3DBLENDOP(D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ONE); break; // SUBTRACT (transparency = strength)
        case kBlend_Exclusion: AGS_D3DBLENDOP(D3DBLENDOP_ADD, D3DBLEND_INVDESTCOLOR, D3DBLEND_INVSRCCOLOR); break; // EXCLUSION
        // APPROXIMATIONS (need pixel shaders)
        case kBlend_Burn: AGS_D3DBLENDOP(D3DBLENDOP_SUBTRACT, D3DBLEND_DESTCOLOR, D3DBLEND_INVDESTCOLOR); break; // LINEAR BURN (approximation)
        case kBlend_Dodge: AGS_D3DBLENDOP(D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_ONE); break; // fake color dodge (half strength of the real thing)
    }

    // BLENDMODES WORKAROUNDS - BEGIN

    // allow transparency with blending modes
    // darken/lighten the base sprite so a higher transparency value makes it trasparent
    if (bmpToDraw->_blendMode > 0) {
        const int alpha = bmpToDraw->_alpha;
        const int invalpha = 255 - alpha;
        switch (bmpToDraw->_blendMode) {
        case kBlend_Darken:
        case kBlend_Multiply:
        case kBlend_Burn: // FIXME burn is imperfect due to blend mode, darker than normal even when trasparent
            // fade to white
            direct3ddevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADDSMOOTH);
            direct3ddevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(invalpha, invalpha, invalpha, invalpha));
            break;
        case kBlend_Lighten:
        case kBlend_Screen:
        case kBlend_Exclusion:
        case kBlend_Dodge:
            // fade to black
            direct3ddevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            direct3ddevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(alpha, alpha, alpha, alpha));
            break;
        }
        direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    }

    // workaround: since the dodge is only half strength we can get a closer approx by drawing it twice
    if (bmpToDraw->_blendMode == kBlend_Dodge)
    {
        hr = direct3ddevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, ti * 4, 2);
        if (hr != D3D_OK)
        {
            throw Ali3DException("IDirect3DDevice9::DrawPrimitive failed");
        }
    }
    // BLENDMODES WORKAROUNDS - END

    hr = direct3ddevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, ti * 4, 2);
    if (hr != D3D_OK) 
    {
      throw Ali3DException("IDirect3DDevice9::DrawPrimitive failed");
    }

    // Restore default blending mode
    SetBlendOp(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
  }
}

void D3DGraphicsDriver::_renderFromTexture()
{
    if (direct3ddevice->SetStreamSource(0, vertexbuffer, 0, sizeof(CUSTOMVERTEX)) != D3D_OK)
    {
        throw Ali3DException("IDirect3DDevice9::SetStreamSource failed");
    }

    float width = _srcRect.GetWidth();
    float height = _srcRect.GetHeight();
    float drawAtX = -(_srcRect.GetWidth() / 2);
    float drawAtY = _srcRect.GetHeight() / 2;

    glm::mat4 transform = glmex::make_transform2d(
        drawAtX - _pixelRenderXOffset, drawAtY + _pixelRenderYOffset, width, height, 0.f);

    direct3ddevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)glm::value_ptr(transform));

    direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

    _filter->SetSamplerStateForStandardSprite(direct3ddevice);

    direct3ddevice->SetTexture(0, pNativeTexture);

    if (direct3ddevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2) != D3D_OK)
    {
        throw Ali3DException("IDirect3DDevice9::DrawPrimitive failed");
    }
}

void D3DGraphicsDriver::_renderAndPresent(bool clearDrawListAfterwards)
{
  _render(clearDrawListAfterwards);
  direct3ddevice->Present(NULL, NULL, NULL, NULL);
}

void D3DGraphicsDriver::_render(bool clearDrawListAfterwards)
{
  IDirect3DSurface9 *pBackBuffer = NULL;

  if (direct3ddevice->GetRenderTarget(0, &pBackBuffer) != D3D_OK) {
    throw Ali3DException("IDirect3DSurface9::GetRenderTarget failed");
  }
  direct3ddevice->ColorFill(pBackBuffer, nullptr, D3DCOLOR_RGBA(0, 0, 0, 255));

  if (direct3ddevice->BeginScene() != D3D_OK)
    throw Ali3DException("IDirect3DDevice9::BeginScene failed");

  if (!_renderSprAtScreenRes) {
    if (direct3ddevice->SetRenderTarget(0, pNativeSurface) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::SetRenderTarget failed");
    }
  }

  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 0.5f, 0);

  // if showing at 2x size, the sprite can get distorted otherwise
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

  RenderSpriteBatches();

  if (!_renderSprAtScreenRes) {
    if (direct3ddevice->SetRenderTarget(0, pBackBuffer)!= D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::SetRenderTarget failed");
    }
    SetD3DViewport(_dstRect);
    _renderFromTexture();
  }

  direct3ddevice->EndScene();

  pBackBuffer->Release();

  if (clearDrawListAfterwards)
  {
    BackupDrawLists();
    ClearDrawLists();
  }
  ResetFxPool();
}

void D3DGraphicsDriver::SetD3DViewport(const Rect &rc)
{
    D3DVIEWPORT9 view;
    ZeroMemory(&view, sizeof(D3DVIEWPORT9));
    view.X = rc.Left;
    view.Y = rc.Top;
    view.Width = rc.GetWidth();
    view.Height = rc.GetHeight();
    view.MinZ = 0.0f;
    view.MaxZ = 1.0f;
    direct3ddevice->SetViewport(&view);
}

void D3DGraphicsDriver::SetScissor(const Rect &clip, bool render_on_texture)
{
    if (!clip.IsEmpty())
    {
        // Adjust a clipping rect to either whole screen, or a target texture
        Rect scissor = render_on_texture ? clip : _scaling.ScaleRange(clip);
        RECT d3d_scissor;
        d3d_scissor.left = scissor.Left;
        d3d_scissor.top = scissor.Top;
        d3d_scissor.right = scissor.Right + 1;
        d3d_scissor.bottom = scissor.Bottom + 1;
        direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        direct3ddevice->SetScissorRect(&d3d_scissor);
    }
    else
    {
        direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }
}

void D3DGraphicsDriver::SetRenderTarget(const D3DSpriteBatch *batch,
    IDirect3DSurface9 *back_buffer, Size &surface_sz, bool clear)
{
    if (batch && batch->RenderTarget)
    {
        // Assign an arbitrary render target, and setup render params
        HRESULT hr = direct3ddevice->SetRenderTarget(0, batch->RenderSurface);
        assert(hr == D3D_OK);
        if (clear)
        {
            direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 0.5f, 0);
        }
        surface_sz = Size(batch->RenderTarget->GetWidth(), batch->RenderTarget->GetHeight());
        SetD3DViewport(RectWH(surface_sz));
        glm::mat4 mat_ortho = glmex::ortho_d3d(surface_sz.Width, surface_sz.Height);
        direct3ddevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)glm::value_ptr(mat_ortho));
        // Configure rules for merging sprite alpha values onto a
        // render target, which also contains alpha channel.
        direct3ddevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
        SetBlendOpAlpha(D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_ONE);
    }
    else
    {
        // Assign the default backbuffer
        HRESULT hr = direct3ddevice->SetRenderTarget(0, back_buffer);
        assert(hr == D3D_OK);
        surface_sz = _srcRect.GetSize();
        SetD3DViewport(_renderSprAtScreenRes ? _dstRect : _srcRect);
        glm::mat4 mat_ortho = glmex::ortho_d3d(_srcRect.GetWidth(), _srcRect.GetHeight());
        direct3ddevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)glm::value_ptr(mat_ortho));
        // Disable alpha merging rules, return back to default settings
        direct3ddevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    }
}

void D3DGraphicsDriver::RenderSpriteBatches()
{
    // Close unended batches, and issue a warning
    assert(_actSpriteBatch == UINT32_MAX);
    while (_actSpriteBatch != UINT32_MAX)
        EndSpriteBatch();

    if (_spriteBatchDesc.size() == 0)
    {
        return; // no batches - no render
    }

    // TODO: following algorithm is repeated for both Direct3D and OpenGL renderer
    // classes. The problem is that some data has different types and contents
    // specific to the renderer. But there has to be a good way to make a shared
    // algorithm in a base class.

    // Render all the sprite batches with necessary transformations;
    // some of them may be rendered to a separate texture instead.
    // For these we save their IDs in a stack (rt_parents).
    // The top of the stack lets us know which batch's RT we are using.
    std::stack<uint32_t> rt_parents;
    IDirect3DSurface9 *back_buffer = nullptr;
    direct3ddevice->GetRenderTarget(0, &back_buffer);
    assert(back_buffer);
    IDirect3DSurface9 *cur_rt = back_buffer; // current render target
    Size surface_sz = _srcRect.GetSize(); // current rt surface size

    const size_t last_batch_to_rend = _spriteBatchDesc.size() - 1;
    for (size_t cur_bat = 0u, last_bat = 0u, cur_spr = 0u; last_bat <= last_batch_to_rend;)
    {
        const auto &batch = _spriteBatches[cur_bat];
        // Test if we are entering this batch (and not continuing after coming back from nested)
        const bool new_batch = cur_bat == last_bat;
        if (new_batch)
        {
            // If batch introduces a new render target, or the first using backbuffer, then remember it
            if (rt_parents.empty() || batch.RenderTarget)
                rt_parents.push(cur_bat); // push new RT parent
            else
                rt_parents.push(rt_parents.top()); // copy same current parent
        }

        // If this batch has ANY sprites in it at all (own, or nested),
        // then we would need to update render target; otherwise there's no need
        if (_spriteBatchRange[cur_bat].first < _spriteBatchRange[cur_bat].second)
        {
            // If render target is different in this batch, then set it up
            const auto &rt_parent = _spriteBatches[rt_parents.top()];
            if (rt_parent.RenderSurface && (cur_rt != rt_parent.RenderSurface) ||
                !rt_parent.RenderSurface && (cur_rt != back_buffer))
            {
                cur_rt = rt_parent.RenderSurface ? rt_parent.RenderSurface : back_buffer;
                SetRenderTarget(&rt_parent, back_buffer, surface_sz, new_batch);
            }
        }

        // Render immediate batch sprites, if any, update cur_spr iterator;
        // we know that the batch has sprites, if next sprite in list belongs to it ("node" ref)
        if ((cur_spr < _spriteList.size()) && (cur_bat == _spriteList[cur_spr].node))
        {
            // Now set clip (scissor), and render sprites
            const bool render_to_texture = (!_renderSprAtScreenRes) || (cur_rt != back_buffer);
            SetScissor(batch.Viewport, render_to_texture);
            _stageMatrixes.World = batch.Matrix;
            _rendSpriteBatch = batch.ID;
            cur_spr = RenderSpriteBatch(batch, cur_spr, surface_sz);
            // at this point cur_spr iterator is updated past the last sprite in a sequence;
            // this may be the end of batch, but also may be the a begginning of a sub-batch
        }

        // Test if we're exiting current batch (and not going into nested ones):
        // if there's no sprites belonging to this batch (direct, or nested),
        // and if there's no nested batches (even if empty ones)
        const uint32_t was_bat = cur_bat;
        while ((cur_bat != UINT32_MAX) && (cur_spr >= _spriteBatchRange[cur_bat].second) &&
            ((last_bat == last_batch_to_rend) || (_spriteBatchDesc[last_bat + 1].Parent != cur_bat)))
        {
            rt_parents.pop(); // pop RT ref from the history
            // Back to the parent batch
            cur_bat = _spriteBatchDesc[cur_bat].Parent;
        }

        // If we stayed at the same batch, this means that there are still nested batches;
        // if there's no batches in the stack left, this means we got to move forward anyway.
        if ((was_bat == cur_bat) || (cur_bat == UINT32_MAX))
        {
            cur_bat = ++last_bat;
        }
    }

    SetRenderTarget(nullptr, back_buffer, surface_sz, false);
    back_buffer->Release();
    _rendSpriteBatch = UINT32_MAX;
    _stageMatrixes.World = _spriteBatches[0].Matrix;
    direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}

size_t D3DGraphicsDriver::RenderSpriteBatch(const D3DSpriteBatch &batch, size_t from, const Size &surface_size)
{
    for (; (from < _spriteList.size()) && (_spriteList[from].node == batch.ID); ++from)
    {
        const auto &e = _spriteList[from];
        if (e.skip)
            continue;

        switch (reinterpret_cast<intptr_t>(e.ddb))
        {
        case DRAWENTRY_STAGECALLBACK:
            // raw-draw plugin support
            // NOTE: device ptr cast will only work on 32-bit systems!
            int sx, sy;
            if (auto *ddb = DoSpriteEvtCallback(e.x,
                static_cast<int32_t>(reinterpret_cast<uintptr_t>(direct3ddevice)), sx, sy))
            {
                auto stageEntry = D3DDrawListEntry((D3DBitmap*)ddb, batch.ID, sx, sy);
                _renderSprite(&stageEntry, batch.Matrix, batch.Color, surface_size);
            }
            break;
        default:
            _renderSprite(&e, batch.Matrix, batch.Color, surface_size);
            break;
        }
    }
    return from;
}

void D3DGraphicsDriver::InitSpriteBatch(size_t index, const SpriteBatchDesc &desc)
{
    // Create transformation matrix for this batch
    // Classic Scale-Rotate-Translate, but inverse, because it's GLM
    float pivotx = _srcRect.GetWidth() / 2.f - desc.Transform.Pivot.X;
    float pivoty = _srcRect.GetHeight() / 2.f - desc.Transform.Pivot.Y;
    glm::mat4 msrt = glmex::make_transform2d(
        (float)desc.Transform.X, (float)-desc.Transform.Y,
        desc.Transform.ScaleX, desc.Transform.ScaleY, // CHECKME: rotate args
        -Math::DegreesToRadians(desc.Transform.Rotate), pivotx, pivoty);
    // Translate scaled node into Top-Left screen coordinates
    float scaled_offx = _srcRect.GetWidth() * ((1.f - desc.Transform.ScaleX) * 0.5f);
    float scaled_offy = _srcRect.GetHeight() * ((1.f - desc.Transform.ScaleY) * 0.5f);
    glm::mat4 model = glmex::translate(-scaled_offx, -(-scaled_offy));
    model = model * msrt;

    // Apply node flip: this is implemented as a negative scaling.
    // TODO: find out if possible to merge with normal scale
    float flip_sx = 1.f, flip_sy = 1.f;
    switch (desc.Flip)
    {
    case kFlip_Vertical: flip_sx = -flip_sx; break;
    case kFlip_Horizontal: flip_sy = -flip_sy; break;
    case kFlip_Both: flip_sx = -flip_sx; flip_sy = -flip_sy; break;
    default: break;
    }
    glm::mat4 mflip = glmex::scale(flip_sx, flip_sy);
    model = mflip * model;

    // Also create separate viewport transformation matrix:
    // it will use a slightly different set of transforms,
    // because the viewport's coordinates origin is different from sprites.
    glm::mat4 mat_viewport = glmex::make_transform2d(
        (float)desc.Transform.X, (float)desc.Transform.Y,
        desc.Transform.ScaleX, desc.Transform.ScaleY, // CHECKME: rotate args
        -Math::DegreesToRadians(desc.Transform.Rotate), pivotx, pivoty);
    glm::mat4 vp_flip_off = glmex::translate(
        _srcRect.GetWidth() * ((1.f - flip_sx) * 0.5f),
        _srcRect.GetHeight() * ((1.f - flip_sy) * 0.5f));
    mat_viewport = mflip * mat_viewport;
    mat_viewport = vp_flip_off * mat_viewport;

    // Apply parent batch's settings, if preset;
    // except when the new batch is started on a separate texture
    Rect viewport = desc.Viewport;
    if ((desc.Parent != UINT32_MAX) && !desc.RenderTarget)
    {
        const auto &parent = _spriteBatches[desc.Parent];
        // Combine sprite matrix with the parent's
        model = parent.Matrix * model;
        // Combine viewport's matrix with the parent's
        mat_viewport = parent.ViewportMat * mat_viewport;

        // Transform this node's viewport using (only!) parent's matrix,
        // and don't let child viewport go outside the parent's bounds.
        if (!viewport.IsEmpty())
        {
            viewport = glmex::full_transform(viewport, parent.ViewportMat);
            viewport = ClampToRect(parent.Viewport, viewport);
        }
        else
        {
            viewport = parent.Viewport;
        }
    }
    else if (viewport.IsEmpty())
    {
        viewport = desc.RenderTarget ?
            RectWH(0, 0, desc.RenderTarget->GetWidth(), desc.RenderTarget->GetHeight()) :
            _srcRect;
    }

    // Assign the new spritebatch
    if (_spriteBatches.size() <= index)
        _spriteBatches.resize(index + 1);
    _spriteBatches[index] = D3DSpriteBatch(index, (D3DBitmap*)desc.RenderTarget, viewport,
        model, mat_viewport, desc.Transform.Color);

    // Preset stage screen for plugin raw drawing
    SetStageScreen(index, viewport.GetSize());
}

void D3DGraphicsDriver::ResetAllBatches()
{
    _spriteBatches.clear();
    _spriteList.clear();
}

void D3DGraphicsDriver::ClearDrawBackups()
{
    _backupBatchDescs.clear();
    _backupBatchRange.clear();
    _backupBatches.clear();
    _backupSpriteList.clear();
}

void D3DGraphicsDriver::BackupDrawLists()
{
    _backupBatchDescs = _spriteBatchDesc;
    _backupBatchRange = _spriteBatchRange;
    _backupBatches = _spriteBatches;
    _backupSpriteList = _spriteList;
}

void D3DGraphicsDriver::RestoreDrawLists()
{
    _spriteBatchDesc = _backupBatchDescs;
    _spriteBatchRange = _backupBatchRange;
    _spriteBatches = _backupBatches;
    _spriteList = _backupSpriteList;
    _actSpriteBatch = UINT32_MAX;
}

void D3DGraphicsDriver::DrawSprite(int ox, int oy, int /*ltx*/, int /*lty*/, IDriverDependantBitmap* ddb)
{
    assert(_actSpriteBatch != UINT32_MAX);
    _spriteList.push_back(D3DDrawListEntry((D3DBitmap*)ddb, _actSpriteBatch, ox, oy));
}

void D3DGraphicsDriver::DestroyDDB(IDriverDependantBitmap* ddb)
{
    // Remove from render targets
    // FIXME: this ugly accessing internal texture members
    if (((D3DBitmap*)ddb)->_data->RenderTarget)
    {
        auto it = std::find(_renderTargets.begin(), _renderTargets.end(), ddb);
        assert(it != _renderTargets.end());
        _renderTargets.erase(it);
        // Remove deleted DDB from batches backup
        for (auto &backup_rt : _backupBatches)
        {
            if (backup_rt.RenderTarget == ddb)
            {
                backup_rt.RenderTarget = nullptr;
                backup_rt.RenderSurface = nullptr;
            }
        }
    }
    // Remove deleted DDB from spritelist backup
    for (auto &backup_spr : _backupSpriteList)
    {
        if (backup_spr.ddb == ddb)
            backup_spr.skip = true;
    }
    delete (D3DBitmap*)ddb;
}

void D3DGraphicsDriver::UpdateTextureRegion(D3DTextureTile *tile, Bitmap *bitmap, bool opaque)
{
  IDirect3DTexture9* newTexture = tile->texture;

  D3DLOCKED_RECT lockedRegion;
  HRESULT hr = newTexture->LockRect(0, &lockedRegion, NULL, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);
  if (hr != D3D_OK)
  {
    throw Ali3DException("Unable to lock texture");
  }

  bool usingLinearFiltering = _filter->NeedToColourEdgeLines();
  uint8_t *memPtr = static_cast<uint8_t*>(lockedRegion.pBits);

  if (opaque)
    BitmapToVideoMemOpaque(bitmap, tile, memPtr, lockedRegion.Pitch);
  else
    BitmapToVideoMem(bitmap, tile, memPtr, lockedRegion.Pitch, usingLinearFiltering);

  newTexture->UnlockRect(0);
}

void D3DGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap *ddb, Bitmap *bitmap)
{
  // FIXME: what to do if texture is shared??
  D3DBitmap *target = (D3DBitmap*)ddb;
  UpdateTexture(target->_data.get(), bitmap, target->_opaque);
}

void D3DGraphicsDriver::UpdateTexture(Texture *txdata, Bitmap *bitmap, bool opaque)
{
  const int color_depth = bitmap->GetColorDepth();
  if (bitmap->GetColorDepth() != txdata->Res.ColorDepth)
    throw Ali3DException("UpdateDDBFromBitmap: mismatched colour depths");
  if (txdata->Res.Width != bitmap->GetWidth() || txdata->Res.Height != bitmap->GetHeight())
    throw Ali3DException("UpdateDDBFromBitmap: mismatched bitmap size");

  if (color_depth == 8)
      select_palette(palette);

  auto *d3ddata = reinterpret_cast<D3DTexture*>(txdata);
  for (size_t i = 0; i < d3ddata->_numTiles; ++i)
  {
    UpdateTextureRegion(&d3ddata->_tiles[i], bitmap, opaque);
  }

  if (color_depth == 8)
      unselect_palette();
}

int D3DGraphicsDriver::GetCompatibleBitmapFormat(int color_depth)
{
  if (color_depth == 8)
    return 8;
  if (color_depth > 8 && color_depth <= 16)
    return 16;
  return 32;
}

size_t D3DGraphicsDriver::GetAvailableTextureMemory()
{
  return direct3ddevice->GetAvailableTextureMem();
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

IDriverDependantBitmap* D3DGraphicsDriver::CreateDDB(int width, int height, int color_depth, bool opaque)
{
    if (color_depth != GetCompatibleBitmapFormat(color_depth))
        throw Ali3DException("CreateDDB: bitmap colour depth not supported");
    D3DBitmap *ddb = new D3DBitmap(width, height, color_depth, opaque);
    ddb->_data.reset(reinterpret_cast<D3DTexture*>(CreateTexture(width, height, color_depth, opaque)));
    return ddb;
}

IDriverDependantBitmap *D3DGraphicsDriver::CreateDDB(std::shared_ptr<Texture> txdata, bool opaque)
{
    auto *ddb = reinterpret_cast<D3DBitmap*>(CreateDDB(txdata->Res.Width, txdata->Res.Height, txdata->Res.ColorDepth, opaque));
    if (ddb)
        ddb->_data = std::static_pointer_cast<D3DTexture>(txdata);
    return ddb;
}

IDriverDependantBitmap* D3DGraphicsDriver::CreateRenderTargetDDB(int width, int height, int color_depth, bool opaque)
{
    if (color_depth != GetCompatibleBitmapFormat(color_depth))
        throw Ali3DException("CreateDDB: bitmap colour depth not supported");
    D3DBitmap *ddb = new D3DBitmap(width, height, color_depth, opaque);
    // FIXME: this ugly accessing internal texture members
    ddb->_data.reset(reinterpret_cast<D3DTexture*>(CreateTexture(width, height, color_depth, opaque, true)));
    IDirect3DTexture9 *tex = ddb->_data->_tiles->texture;
    HRESULT hr = tex->GetSurfaceLevel(0, &ddb->_renderSurface);
    assert(hr == D3D_OK);
    ddb->_renderHint = kTxHint_PremulAlpha;
    _renderTargets.push_back(ddb);
    return ddb;
}

std::shared_ptr<Texture> D3DGraphicsDriver::GetTexture(IDriverDependantBitmap *ddb)
{
    return std::static_pointer_cast<Texture>((reinterpret_cast<D3DBitmap*>(ddb))->_data);
}

Texture *D3DGraphicsDriver::CreateTexture(int width, int height, int color_depth, bool opaque, bool as_render_target)
{
  assert(width > 0);
  assert(height > 0);
  int allocatedWidth = width;
  int allocatedHeight = height;
  AdjustSizeToNearestSupportedByCard(&allocatedWidth, &allocatedHeight);

  // Render targets must have D3DUSAGE_RENDERTARGET flag and be
  // created in a D3DPOOL_DEFAULT
  const int texture_use = as_render_target ? D3DUSAGE_RENDERTARGET : 0;
  const D3DPOOL texture_pool = as_render_target ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

  int tilesAcross = 1, tilesDown = 1;
  if (as_render_target)
  {
    // For render target - just limit the size by the max texture size
    allocatedWidth = std::min<DWORD>(allocatedWidth, direct3ddevicecaps.MaxTextureWidth);
    allocatedHeight = std::min<DWORD>(allocatedHeight, direct3ddevicecaps.MaxTextureHeight);
  }
  else
  {
    // Calculate how many textures will be necessary to store this image
    tilesAcross = (allocatedWidth + direct3ddevicecaps.MaxTextureWidth - 1) / direct3ddevicecaps.MaxTextureWidth;
    tilesDown = (allocatedHeight + direct3ddevicecaps.MaxTextureHeight - 1) / direct3ddevicecaps.MaxTextureHeight;
    assert(tilesAcross > 0);
    assert(tilesDown > 0);
    tilesAcross = std::max(1, tilesAcross);
    tilesDown = std::max(1, tilesDown);
  }

  int tileWidth = width / tilesAcross;
  int lastTileExtraWidth = width % tilesAcross;
  int tileHeight = height / tilesDown;
  int lastTileExtraHeight = height % tilesDown;
  int tileAllocatedWidth = tileWidth;
  int tileAllocatedHeight = tileHeight;
  AdjustSizeToNearestSupportedByCard(&tileAllocatedWidth, &tileAllocatedHeight);

  auto *txdata = new D3DTexture(GraphicResolution(width, height, color_depth), as_render_target);
  int numTiles = tilesAcross * tilesDown;
  D3DTextureTile *tiles = new D3DTextureTile[numTiles];
  CUSTOMVERTEX *vertices = NULL;

  if ((numTiles == 1) &&
      (allocatedWidth == width) &&
      (allocatedHeight == height))
  {
    // use default whole-image vertices
  }
  else
  {
     // The texture is not the same as the bitmap, so create some custom vertices
     // so that only the relevant portion of the texture is rendered
     int vertexBufferSize = numTiles * 4 * sizeof(CUSTOMVERTEX);
     HRESULT hr = direct3ddevice->CreateVertexBuffer(vertexBufferSize, 0,
         D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &txdata->_vertex, NULL);

     if (hr != D3D_OK) 
     {
        delete []tiles;
        throw Ali3DException(String::FromFormat(
            "Direct3DDevice9::CreateVertexBuffer(Length=%d) for texture failed: error code 0x%08X", vertexBufferSize, hr));
     }

     if (txdata->_vertex->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD) != D3D_OK)
     {
       delete []tiles;
       throw Ali3DException("Failed to lock vertex buffer");
     }
  }

  for (int x = 0; x < tilesAcross; x++)
  {
    for (int y = 0; y < tilesDown; y++)
    {
      D3DTextureTile *thisTile = &tiles[y * tilesAcross + x];
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

      // NOTE: pay attention that the texture format depends on the **display mode**'s color format,
      // rather than source bitmap's color depth!
      D3DFORMAT texture_fmt = color_depth_to_d3d_format(_mode.ColorDepth);
      HRESULT hr = direct3ddevice->CreateTexture(thisAllocatedWidth, thisAllocatedHeight, 1,
                                      texture_use, texture_fmt,
                                      texture_pool, &thisTile->texture, NULL);
      if (hr != D3D_OK)
      {
        throw Ali3DException(String::FromFormat(
            "Direct3DDevice9::CreateTexture(X=%d, Y=%d, FMT=%d) failed: error code 0x%08X", thisAllocatedWidth, thisAllocatedHeight, texture_fmt, hr));
      }

    }
  }

  if (vertices != NULL)
  {
      txdata->_vertex->Unlock();
  }

  txdata->_numTiles = numTiles;
  txdata->_tiles = tiles;
  return txdata;
}

void D3DGraphicsDriver::do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
  // Construct scene in order: game screen, fade fx, post game overlay
  // NOTE: please keep in mind: redrawing last saved frame here instead of constructing new one
  // is done because of backwards-compatibility issue: originally AGS faded out using frame
  // drawn before the script that triggers blocking fade (e.g. instigated by ChangeRoom).
  // Unfortunately some existing games were changing looks of the screen during same function,
  // but these were not supposed to get on screen until before fade-in.
  if (fadingOut)
    this->_reDrawLastFrame();
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear(makeacol32(targetColourRed, targetColourGreen, targetColourBlue, 0xFF));
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, true);
  delete blackSquare;
  BeginSpriteBatch(_srcRect, SpriteTransform());
  d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
  this->DrawSprite(0, 0, d3db);
  EndSpriteBatch();
  if (_drawPostScreenCallback != NULL)
      _drawPostScreenCallback();

  if (speed <= 0) speed = 16;
  speed *= 2;  // harmonise speeds with software driver which is faster
  for (int a = 1; a < 255; a += speed)
  {
    d3db->SetAlpha(fadingOut ? a : (255 - a));
    this->_renderAndPresent(false);

    sys_evt_process_pending();
    if (_pollingCallback)
      _pollingCallback();
    WaitForNextFrame();
  }

  if (fadingOut)
  {
    d3db->SetAlpha(255);
    this->_renderAndPresent(false);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawLists();
  ResetFxPool();
}

void D3DGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(true, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void D3DGraphicsDriver::FadeIn(int speed, PALETTE /*p*/, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(false, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void D3DGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
{
  // Construct scene in order: game screen, fade fx, post game overlay
  if (blackingOut)
     this->_reDrawLastFrame();
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear();
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, true);
  delete blackSquare;
  BeginSpriteBatch(_srcRect, SpriteTransform());
  const size_t fx_batch = _actSpriteBatch;
  d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
  this->DrawSprite(0, 0, d3db);
  if (!blackingOut)
  {
    // when fading in, draw four black boxes, one
    // across each side of the screen
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
  }
  EndSpriteBatch();
  std::vector<D3DDrawListEntry> &drawList = _spriteList;
  const size_t last = drawList.size() - 1;

  if (_drawPostScreenCallback != NULL)
    _drawPostScreenCallback();

  int yspeed = _srcRect.GetHeight() / (_srcRect.GetWidth() / speed);
  int boxWidth = speed;
  int boxHeight = yspeed;

  while (boxWidth < _srcRect.GetWidth())
  {
    boxWidth += speed;
    boxHeight += yspeed;
    
    if (blackingOut)
    {
      drawList[last].x = _srcRect.GetWidth() / 2- boxWidth / 2;
      drawList[last].y = _srcRect.GetHeight() / 2 - boxHeight / 2;
      d3db->SetStretch(boxWidth, boxHeight, false);
    }
    else
    {
      drawList[last - 3].x = _srcRect.GetWidth() / 2 - boxWidth / 2 - _srcRect.GetWidth();
      drawList[last - 2].y = _srcRect.GetHeight() / 2 - boxHeight / 2 - _srcRect.GetHeight();
      drawList[last - 1].x = _srcRect.GetWidth() / 2 + boxWidth / 2;
      drawList[last    ].y = _srcRect.GetHeight() / 2 + boxHeight / 2;
      d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
    }
    
    this->_renderAndPresent(false);

    sys_evt_process_pending();
    if (_pollingCallback)
      _pollingCallback();
    platform->Delay(delay);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawLists();
  ResetFxPool();
}

void D3DGraphicsDriver::SetScreenFade(int red, int green, int blue)
{
    assert(_actSpriteBatch != UINT32_MAX);
    D3DBitmap *ddb = static_cast<D3DBitmap*>(MakeFx(red, green, blue));
    ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
        _spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
    ddb->SetAlpha(255);
    _spriteList.push_back(D3DDrawListEntry(ddb, _actSpriteBatch, 0, 0));
}

void D3DGraphicsDriver::SetScreenTint(int red, int green, int blue)
{ 
    assert(_actSpriteBatch != UINT32_MAX);
    if (red == 0 && green == 0 && blue == 0) return;
    D3DBitmap *ddb = static_cast<D3DBitmap*>(MakeFx(red, green, blue));
    ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
        _spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
    ddb->SetAlpha(128);
    _spriteList.push_back(D3DDrawListEntry(ddb, _actSpriteBatch, 0, 0));
}

bool D3DGraphicsDriver::SetVsyncImpl(bool enabled, bool &vsync_res) 
{
    d3dpp.PresentationInterval = enabled ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    // D3D requires a Reset() in order to apply a new D3DPRESENT_INTERVAL value
    HRESULT hr = ResetD3DDevice();
    if (hr != D3D_OK)
    {
        Debug::Printf(kDbgMsg_Error, "D3D: SetVsync (%d): failed to reset D3D device", enabled);
        return false;
    }
    // TODO: refactor, to not duplicate these 3-4 calls everytime ResetDevice is called
    InitializeD3DState();
    CreateVirtualScreen();
    direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);
    SetupViewport();
    // CHECKME: is there a need to test that vsync was actually set (ask d3d api)?
    vsync_res = d3dpp.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE;
    return true;
}


D3DGraphicsFactory *D3DGraphicsFactory::_factory = NULL;
Library D3DGraphicsFactory::_library;

D3DGraphicsFactory::~D3DGraphicsFactory()
{
    DestroyDriver(); // driver must be destroyed before d3d library is disposed
    ULONG ref_cnt = _direct3d->Release();
    if (ref_cnt > 0)
        Debug::Printf(kDbgMsg_Warn, "WARNING: Not all of the Direct3D resources have been disposed; ID3D ref count: %d", ref_cnt);
    _factory = NULL;
}

size_t D3DGraphicsFactory::GetFilterCount() const
{
    return 2;
}

const GfxFilterInfo *D3DGraphicsFactory::GetFilterInfo(size_t index) const
{
    switch (index)
    {
    case 0:
        return &D3DGfxFilter::FilterInfo;
    case 1:
        return &AAD3DGfxFilter::FilterInfo;
    default:
        return NULL;
    }
}

String D3DGraphicsFactory::GetDefaultFilterID() const
{
    return D3DGfxFilter::FilterInfo.Id;
}

/* static */ D3DGraphicsFactory *D3DGraphicsFactory::GetFactory()
{
    if (!_factory)
    {
        _factory = new D3DGraphicsFactory();
        if (!_factory->Init())
        {
            delete _factory;
            _factory = NULL;
        }
    }
    return _factory;
}

/* static */ D3DGraphicsDriver *D3DGraphicsFactory::GetD3DDriver()
{
    if (!_factory)
        _factory = GetFactory();
    if (_factory)
        return _factory->EnsureDriverCreated();
    return NULL;
}


D3DGraphicsFactory::D3DGraphicsFactory()
    : _direct3d(NULL)
{
}

D3DGraphicsDriver *D3DGraphicsFactory::EnsureDriverCreated()
{
    if (!_driver)
    {
        _factory->_direct3d->AddRef();
        _driver = new D3DGraphicsDriver(_factory->_direct3d);
    }
    return _driver;
}

D3DGfxFilter *D3DGraphicsFactory::CreateFilter(const String &id)
{
    if (D3DGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new D3DGfxFilter();
    else if (AAD3DGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new AAD3DGfxFilter();
    return NULL;
}

bool D3DGraphicsFactory::Init()
{
    assert(_direct3d == NULL);
    if (_direct3d)
        return true;

    if (!_library.Load("d3d9"))
    {
        SDL_SetError("Direct3D 9 is not installed");
        return false;
    }

    typedef IDirect3D9 * WINAPI D3D9CreateFn(UINT);
    D3D9CreateFn *lpDirect3DCreate9 = (D3D9CreateFn*)_library.GetFunctionAddress("Direct3DCreate9");
    if (!lpDirect3DCreate9)
    {
        _library.Unload();
        SDL_SetError("Entry point not found in d3d9.dll");
        return false;
    }
    _direct3d = lpDirect3DCreate9(D3D_SDK_VERSION);
    if (!_direct3d)
    {
        _library.Unload();
        SDL_SetError("Direct3DCreate failed!");
        return false;
    }
    return true;
}

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // AGS_HAS_DIRECT3D

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

//
// Direct3D helpers
//
static void RectToRECT(const Rect &in_rc, RECT &out_rc)
{
    out_rc.left = in_rc.Left;
    out_rc.top = in_rc.Top;
    out_rc.right = in_rc.Right + 1;
    out_rc.bottom = in_rc.Bottom + 1;
}

static void RectToRECT(const Rect &in_rc, D3DRECT &out_rc)
{
    out_rc.x1 = in_rc.Left;
    out_rc.y1 = in_rc.Top;
    out_rc.x2 = in_rc.Right + 1;
    out_rc.y2 = in_rc.Bottom + 1;
}

// Convert a colour depth into the corresponding D3D pixel format
static D3DFORMAT ColorDepthToD3DFormat(int color_depth, bool want_alpha)
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
        default:
            return D3DFMT_UNKNOWN;
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
        default:
            return D3DFMT_UNKNOWN;
        }
    }
}

// Convert a D3D pixel format to colour depth (bits per pixel).
// TODO: this is currently an inversion of ColorDepthToD3DFormat;
// check later if more formats should be handled.
static int D3DFormatToColorDepth(D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_P8:
        return 8;
    case D3DFMT_A1R5G5B5:
        return 16;
    case D3DFMT_X1R5G5B5:
        return 16;
    case D3DFMT_R5G6B5:
        return 16;
    case D3DFMT_R8G8B8:
        return 32;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        return 32;
    default:
        return 0; // unknown
    }
}

// Tells if the pixel format contains valid alpha channel.
// TODO: this is a very formal implementation, might review this later.
static bool D3DFormatHasAlpha(D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A8R8G8B8:
        return true;
    default:
        return false;
    }
}


size_t D3DTexture::GetMemSize() const
{
    // FIXME: a proper size in video memory, check Direct3D docs
    size_t sz = 0u;
    for (const auto &tile : _tiles)
        sz += tile.allocWidth * tile.allocHeight * 4;
    return sz;
}

void D3DBitmap::ReleaseTextureData()
{
    _renderSurface = nullptr;
    _data.reset();
}

D3DGfxModeList::D3DGfxModeList(const D3DPtr &direct3d, int display_index, D3DFORMAT d3dformat)
    : _direct3d(direct3d)
    , _displayIndex(display_index)
    , _pixelFormat(d3dformat)
{
    _adapterIndex = SDL_Direct3D9GetAdapterIndex(display_index);
    _modeCount = _direct3d->GetAdapterModeCount(_adapterIndex, _pixelFormat);
}

bool D3DGfxModeList::GetMode(int index, DisplayMode &mode) const
{
    if (_direct3d && index >= 0 && index < _modeCount)
    {
        D3DDISPLAYMODE d3d_mode;
        if (SUCCEEDED(_direct3d->EnumAdapterModes(_adapterIndex, _pixelFormat, index, &d3d_mode)))
        {
            mode.DisplayIndex = _displayIndex;
            mode.Width = d3d_mode.Width;
            mode.Height = d3d_mode.Height;
            mode.ColorDepth = D3DFormatToColorDepth(d3d_mode.Format);
            mode.RefreshRate = d3d_mode.RefreshRate;
            return true;
        }
    }
    return false;
}


// The custom FVF, which describes the custom vertex structure.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

D3DGraphicsDriver::D3DGraphicsDriver(const D3DPtr &d3d) 
{
  direct3d = d3d;
  SetupDefaultVertices();
  _smoothScaling = false;
  _pixelRenderXOffset = 0;
  _pixelRenderYOffset = 0;
  _renderAtScreenRes = false;

  // Shifts comply to D3DFMT_A8R8G8B8
  _vmem_a_shift_32 = 24;
  _vmem_r_shift_32 = 16;
  _vmem_g_shift_32 = 8;
  _vmem_b_shift_32 = 0;
}

D3DGraphicsDriver::BackbufferState::BackbufferState(const D3DSurfacePtr &surface,
    const Size &surf_size, const Size &rend_sz,
    const Rect &view, const glm::mat4 &proj, const PlaneScaling &scale, int filter)
    : Surface(surface), SurfSize(surf_size), RendSize(rend_sz)
    , Viewport(view), Projection(proj), Scaling(scale), Filter(filter)
{
    assert(Surface != nullptr);
}

D3DGraphicsDriver::BackbufferState::BackbufferState(D3DSurfacePtr &&surface,
    const Size &surf_size, const Size &rend_sz,
    const Rect &view, const glm::mat4 &proj, const PlaneScaling &scale, int filter)
    : Surface(std::move(surface)), SurfSize(surf_size), RendSize(rend_sz)
    , Viewport(view), Projection(proj), Scaling(scale), Filter(filter)
{
    assert(Surface != nullptr);
}

void D3DGraphicsDriver::SetupDefaultVertices()
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

  _screenBackbuffer = BackbufferState();

  OnModeReleased();
  ClearDrawLists();
  ClearDrawBackups();
  DestroyFxPool();
  DestroyAllStageScreens();

  sys_window_set_style(kWnd_Windowed);
}

bool D3DGraphicsDriver::FirstTimeInit()
{
  direct3ddevice->GetCreationParameters(&direct3dcreateparams);
  direct3ddevice->GetDeviceCaps(&direct3ddevicecaps);

  _capsVsync = (direct3ddevicecaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE) != 0;
  if (!_capsVsync)
    Debug::Printf(kDbgMsg_Warn, "WARNING: Vertical sync is not supported. Setting will be kept at driver default.");

  if (direct3ddevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY,
          D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, vertexbuffer.Acquire(), NULL) != D3D_OK) 
  {
    SDL_SetError("Failed to create vertex buffer");
    previousError = SDL_GetError();
    return false;
  }

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

  if (!CreateShaders())
  {
    // NOTE: we don't have an obligatory transparency shader in Direct3D, so all standard shaders are optional
    Debug::Printf(kDbgMsg_Error, "ERROR: Direct3D: one or more standard shaders failed to compile, the game may run but have unexpected visuals.");
  }

  return true;
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

  D3DFORMAT pixelFormat = ColorDepthToD3DFormat(mode.ColorDepth, false /* opaque */);
  D3DDISPLAYMODE d3d_mode;

  const UINT use_adapter = SDL_Direct3D9GetAdapterIndex(mode.DisplayIndex);
  int mode_count = direct3d->GetAdapterModeCount(use_adapter, pixelFormat);
  for (int i = 0; i < mode_count; i++)
  {
    if (FAILED(direct3d->EnumAdapterModes(use_adapter, pixelFormat, i, &d3d_mode)))
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
        hr = ResetDeviceAndRestore();
        if (hr != D3D_OK)
        {
            throw Ali3DException(String::FromFormat("IDirect3DDevice9::Reset: failed: error code: 0x%08X", hr));
        }
    }
    else if (hr != D3D_OK)
    {
        throw Ali3DException(String::FromFormat("IDirect3DDevice9::TestCooperativeLevel: failed: error code: 0x%08X", hr));
    }
}

HRESULT D3DGraphicsDriver::ResetDeviceAndRestore()
{
    HRESULT hr = ResetD3DDevice();
    if (hr != D3D_OK)
        return hr;

    // Reinitialize D3D settings, backbuffer surface, etc
    InitializeD3DState();
    CreateVirtualScreen();
    return D3D_OK;
}

bool D3DGraphicsDriver::CreateDisplayMode(const DisplayMode &mode)
{
  if (!IsModeSupported(mode))
    return false;

  SDL_Window *window = sys_get_window();
  int use_display = mode.DisplayIndex;
  if (!window)
  {
    sys_window_create("", mode.DisplayIndex, mode.Width, mode.Height, mode.Mode);
  }
  else
  {
#if (AGS_SUPPORT_MULTIDISPLAY)
    // If user requested an exclusive fullscreen, move window to where we created it first,
    // because DirectX does not normally support switching displays in exclusive mode.
    // NOTE: we may in theory support this, but we'd have to release and recreate
    // ALL the resources, including all textures currently in memory.
    if (mode.IsRealFullscreen() &&
        (_fullscreenDisplay > 0) && (sys_get_window_display_index() != _fullscreenDisplay))
    {
      use_display = _fullscreenDisplay;
      sys_window_fit_in_display(_fullscreenDisplay);
    }
#endif
  }

  HWND hwnd = (HWND)sys_win_get_window();
  memset( &d3dpp, 0, sizeof(d3dpp) );
  d3dpp.BackBufferWidth = mode.Width;
  d3dpp.BackBufferHeight = mode.Height;
  d3dpp.BackBufferFormat = ColorDepthToD3DFormat(mode.ColorDepth, false /* opaque */);
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

  if (_initGfxCallback != nullptr)
    _initGfxCallback(&d3dpp);

  bool first_time_init = direct3ddevice == nullptr;
  HRESULT hr = 0;
  if (direct3ddevice)
  {
    hr = ResetD3DDevice();
  }
  else
  {
    const UINT use_adapter = SDL_Direct3D9GetAdapterIndex(use_display);
    hr = direct3d->CreateDevice(use_adapter, D3DDEVTYPE_HAL, hwnd,
                      D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,  // multithreaded required for AVI player
                      &d3dpp, direct3ddevice.Acquire());
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
      sys_window_bring_to_front();
    }
  }
  return true;
}

void D3DGraphicsDriver::SetBlendOpUniform(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor)
{
    SetBlendOpRGB(blend_op, src_factor, dst_factor);
    SetBlendOpAlpha(blend_op, src_factor, dst_factor);
}

void D3DGraphicsDriver::SetBlendOpRGB(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor)
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
  direct3ddevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
  SetBlendOpUniform(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
  _rtBlendAlpha = BlendOpState(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);

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

  // Set gamma
  direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);

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

  D3DSurfacePtr backbuffer;
  HRESULT hr = direct3ddevice->GetRenderTarget(0, backbuffer.Acquire());
  assert(hr == D3D_OK);
  _screenBackbuffer = BackbufferState(std::move(backbuffer), Size(_mode.Width, _mode.Height), _srcRect.GetSize(),
      _dstRect, mat_ortho, _scaling, _filter ? _filter->GetSamplerStateForStandardSprite() : D3DTEXF_POINT);

  // View and Projection matrixes are currently fixed in Direct3D renderer
  _stageMatrixes.View = identity;
  _stageMatrixes.Projection = mat_ortho;
}

void D3DGraphicsDriver::SetGraphicsFilter(PD3DFilter filter)
{
  _filter = filter;
  _screenBackbuffer.Filter = _filter->GetSamplerStateForStandardSprite();
  OnSetFilter();
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
  DisplayMode set_mode = mode;
  set_mode.DisplayIndex = sys_get_window_display_index();
  OnModeSet(set_mode);
  if ((_fullscreenDisplay < 0) || set_mode.IsRealFullscreen())
    _fullscreenDisplay = set_mode.DisplayIndex;
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
  HRESULT hr = ResetDeviceAndRestore();
  if (hr != D3D_OK)
  {
      Debug::Printf("D3DGraphicsDriver: Failed to reset D3D device");
      return;
  }
  // Display mode could've changed, so re-setup viewport, matrixes, etc
  SetupViewport();
}

void D3DGraphicsDriver::CreateVirtualScreen()
{
  if (!IsModeSet() || !IsNativeSizeValid())
    return;

  // Set up native surface
  // TODO: maybe not do this always, but only allocate if necessary
  // when render option is set, or temporarily for making a screenshot.
  if (_nativeSurface)
  {
    DestroyDDB(_nativeSurface);
    _nativeSurface = nullptr;
  }
  _nativeSurface = (D3DBitmap*)CreateRenderTargetDDB(
      _srcRect.GetWidth(), _srcRect.GetHeight(), _mode.ColorDepth, kTxFlags_Opaque);
  glm::mat4 mat_ortho = glmex::ortho_d3d(_srcRect.GetWidth(), _srcRect.GetHeight());
  _nativeBackbuffer = BackbufferState(_nativeSurface->GetRenderSurface(),
    _srcRect.GetSize(), _srcRect.GetSize(), _srcRect, mat_ortho, PlaneScaling(), D3DTEXF_POINT);

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
    _screenBackbuffer = BackbufferState();
    _nativeBackbuffer = BackbufferState();
    if (_nativeSurface)
    {
        DestroyDDB(_nativeSurface);
        _nativeSurface = nullptr;
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
        batch.RenderSurface = nullptr;
    }
    for (auto &batch : _backupBatches)
    {
        batch.RenderSurface = nullptr;
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
        auto txdata = std::shared_ptr<D3DTexture>(
            reinterpret_cast<D3DTexture*>(CreateTexture(ddb->GetWidth(), ddb->GetHeight(), ddb->GetColorDepth(), ddb->GetTextureFlags())));
        // FIXME: this ugly accessing internal texture members
        auto &tex = txdata->_tiles[0].texture;
        D3DSurfacePtr surf;
        HRESULT hr = tex->GetSurfaceLevel(0, surf.Acquire());
        assert(hr == D3D_OK);
        ddb->SetTexture(txdata, surf);
    }
    // Reappoint RT surfaces in existing batches
    for (auto &batch : _spriteBatches)
    {
        if (batch.RenderTarget)
        {
            assert(!batch.RenderSurface);
            batch.RenderSurface = ((D3DBitmap*)batch.RenderTarget)->GetRenderSurface();
        }
    }
    for (auto &batch : _backupBatches)
    {
        if (batch.RenderTarget)
        {
            assert(!batch.RenderSurface);
            batch.RenderSurface = ((D3DBitmap*)batch.RenderTarget)->GetRenderSurface();
        }
    }
}

bool D3DGraphicsDriver::CreateShaders()
{
    const int requiredPSMajorVersion = 2;
    const int requiredPSMinorVersion = 0;

    if (direct3ddevicecaps.PixelShaderVersion < D3DPS_VERSION(requiredPSMajorVersion, requiredPSMinorVersion))
    {
        SDL_SetError("Graphics card does not support Pixel Shader %d.%d", requiredPSMajorVersion, requiredPSMinorVersion);
        previousError = SDL_GetError();
        return false;
    }

    bool shaders_created = true;
    shaders_created &= CreateTintShader(_tintShader, _dummyShader);
    // Reserve custom shader index 0 as "no shader"
    _shaders.push_back({});
    return shaders_created;
}

void D3DGraphicsDriver::DeleteShaders()
{
    for (auto &prg : _shaders)
    {
        DeleteShaderProgram(prg);
    }
    _shaders.clear();
    _shaderLookup.clear();

    DeleteShaderProgram(_tintShader);
}

bool D3DGraphicsDriver::CreateShaderProgram(ShaderProgram &prg, const String &name, const uint8_t *data_ptr)
{
    D3DPixelShaderPtr shader_ptr;
    HRESULT hr = direct3ddevice->CreatePixelShader(reinterpret_cast<const DWORD*>(data_ptr), shader_ptr.Acquire());
    if (hr != D3D_OK)
    {
        Debug::Printf(kDbgMsg_Error, "ERROR: Direct3D: Failed to create pixel shader: 0x%08X", hr);
        return false;
    }

    prg.ShaderPtr = shader_ptr;
    Debug::Printf("Direct3D: \"%s\" shader program created successfully", name.GetCStr());
    return true;
}

bool D3DGraphicsDriver::CreateShaderProgramFromResource(ShaderProgram &prg, const String &name, const char *resource_name)
{
    HMODULE exeHandle = GetModuleHandle(NULL);
    HRSRC hRes = FindResource(exeHandle, resource_name, "DATA");
    if (hRes)
    {
        HGLOBAL hGlobal = LoadResource(exeHandle, hRes);
        if (hGlobal)
        {
            const uint8_t *data_ptr = static_cast<const uint8_t*>(LockResource(hGlobal));
            bool result = CreateShaderProgram(prg, name, data_ptr);
            UnlockResource(hGlobal);
            return result;
        }
    }
    return false;
}

bool D3DGraphicsDriver::CreateTintShader(ShaderProgram &prg, const ShaderProgram &fallback_prg)
{
    if (!CreateShaderProgramFromResource(prg, "Tinting", "TINT_PIXEL_SHADER"))
    {
        prg = fallback_prg;
        return false;
    }
    AssignBaseShaderArgs(prg);
    // FIXME: hardcode these for now, because we can't find constant location without utility lib
    prg.TintHSV = 0u;
    prg.TintAlphaLight = 1u;
    return true;
}

void D3DGraphicsDriver::DeleteShaderProgram(ShaderProgram &prg)
{
    prg = {};
}

void D3DGraphicsDriver::AssignBaseShaderArgs(ShaderProgram &prg)
{
    // FIXME: hardcode these for now, because we can't find constant location without utility lib
    prg.Time = 0u;
    prg.GameFrame = 1u;
    prg.TextureDim = 2u;
    prg.Alpha = 3u;
}

void D3DGraphicsDriver::UpdateGlobalShaderArgValues()
{
    float vf[4]{};
    for (auto &sh : _shaders)
    {
        if (!sh.ShaderPtr)
            continue;

        direct3ddevice->SetPixelShader(sh.ShaderPtr.get());
        vf[0] = _globalShaderConst.Time;
        direct3ddevice->SetPixelShaderConstantF(sh.Time, &vf[0], 1);
        vf[0] = _globalShaderConst.GameFrame;
        direct3ddevice->SetPixelShaderConstantF(sh.GameFrame, &vf[0], 1);
    }
    direct3ddevice->SetPixelShader(NULL);
}

#if (DIRECT3D_USE_D3DCOMPILER)
void D3DGraphicsDriver::OutputShaderLog(ComPtr<ID3DBlob> &out_errors, const String &shader_name, bool as_error)
{
    assert(out_errors);
    if (!out_errors)
        return;

    const MessageType mt = as_error ? kDbgMsg_Error : kDbgMsg_Debug;
    if (as_error)
        Debug::Printf(mt, "ERROR: Direct3D: shader \"%s\" %s:", shader_name.GetCStr(), "compilation");
    else
        Debug::Printf(mt, "Direct3D: shader \"%s\" %s:", shader_name.GetCStr(), "compilation");

    if (out_errors->GetBufferSize() > 0)
    {
        Debug::Printf(mt, "----------------------------------------");
        Debug::Printf(mt, "%s", static_cast<const char*>(out_errors->GetBufferPointer()));
        Debug::Printf(mt, "----------------------------------------");
    }
    else
    {
        Debug::Printf(mt, "Shader output log was empty.");
    }
}
#endif

uint32_t D3DGraphicsDriver::AddShaderToCollection(ShaderProgram &prg, const String &name)
{
    AssignBaseShaderArgs(prg);
    _shaders.push_back(prg);
    uint32_t shader_id = static_cast<uint32_t>(_shaders.size() - 1);
    _shaderLookup[name] = shader_id;
    return shader_id;
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

IGfxModeList *D3DGraphicsDriver::GetSupportedModeList(int display_index, int color_depth)
{
    return new D3DGfxModeList(direct3d, display_index, ColorDepthToD3DFormat(color_depth, false /* opaque */));
}

PGfxFilter D3DGraphicsDriver::GetGraphicsFilter() const
{
    return _filter;
}

void D3DGraphicsDriver::UnInit() 
{
  OnUnInit();
  ReleaseDisplayMode();

  if (_nativeSurface)
  {
    DestroyDDB(_nativeSurface);
    _nativeSurface = nullptr;
  }
  _nativeBackbuffer = BackbufferState();

  vertexbuffer = nullptr; // FIXME??
  direct3ddevice = nullptr;

  sys_window_destroy();
}

D3DGraphicsDriver::~D3DGraphicsDriver()
{
  D3DGraphicsDriver::UnInit();
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
  RectToRECT(r, rectToClear);
  DWORD colorDword = 0;
  if (colorToUse != nullptr)
    colorDword = D3DCOLOR_XRGB(colorToUse->r, colorToUse->g, colorToUse->b);
  direct3ddevice->Clear(1, &rectToClear, D3DCLEAR_TARGET, colorDword, 0.5f, 0);
}

void D3DGraphicsDriver::GetCopyOfScreenIntoDDB(IDriverDependantBitmap *target, uint32_t batch_skip_filter)
{
    // If we normally render in screen res, restore last frame's lists and
    // render in native res on the given target;
    // Also force re-render last frame if we require batch filtering
    if (_renderAtScreenRes || (batch_skip_filter != 0))
    {
        bool old_render_res = _renderAtScreenRes;
        _renderAtScreenRes = false;
        RedrawLastFrame(batch_skip_filter);
        Render(target);
        _renderAtScreenRes = old_render_res;
    }
    else
    {
        // If we normally render in native res, then simply render backbuffer contents
        D3DBitmap *bitmap = (D3DBitmap*)target;
        Size surf_sz = bitmap->GetSize();
        BackbufferState backbuffer = BackbufferState(bitmap->GetRenderSurface(), surf_sz, surf_sz,
            RectWH(0, 0, surf_sz.Width, surf_sz.Height), glmex::ortho_d3d(surf_sz.Width, surf_sz.Height),
            PlaneScaling(), D3DTEXF_POINT);
        SetBackbufferState(&backbuffer, true);
        if (direct3ddevice->BeginScene() != D3D_OK)
        {
            throw Ali3DException("IDirect3DDevice9::BeginScene failed");
        }
        RenderTexture(_nativeSurface, 0, 0, glmex::identity(), SpriteColorTransform(), _srcRect.GetSize());
        direct3ddevice->EndScene();
    }
}

bool D3DGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination,
    const Rect *src_rect, bool at_native_res,
    GraphicResolution *want_fmt, uint32_t batch_skip_filter)
{
  // Currently don't support copying in screen resolution when we are rendering in native
  if (!_renderAtScreenRes)
      at_native_res = true;

  Rect copy_from = src_rect ? *src_rect : _srcRect;
  if (!at_native_res)
    copy_from = _scaling.ScaleRange(copy_from);
  if (destination->GetColorDepth() != _mode.ColorDepth || destination->GetSize() != copy_from.GetSize())
  {
    if (want_fmt)
      *want_fmt = GraphicResolution(copy_from.GetWidth(), copy_from.GetHeight(), _mode.ColorDepth);
    return false;
  }

  // If we are rendering sprites at the screen resolution, and requested native res,
  // re-render last frame to the native surface;
  // Also force re-render last frame if we require batch filtering
  if ((at_native_res && _renderAtScreenRes) || (batch_skip_filter != 0))
  {
    bool old_render_res = _renderAtScreenRes;
    _renderAtScreenRes = !at_native_res;
    RedrawLastFrame(batch_skip_filter);
    RenderImpl(true);
    _renderAtScreenRes = old_render_res;
  }
  
  D3DSurfacePtr surface;
  {
    Rect viewport;
    if (at_native_res)
    {
      // with render to texture the texture mipmap surface can't be locked directly
      // we have to create a surface with D3DPOOL_SYSTEMMEM for GetRenderTargetData
      if (direct3ddevice->CreateOffscreenPlainSurface(
        _srcRect.GetWidth(),
        _srcRect.GetHeight(),
        ColorDepthToD3DFormat(_mode.ColorDepth, false /* opaque */),
        D3DPOOL_SYSTEMMEM,
        surface.Acquire(),
        NULL) != D3D_OK)
      {
        throw Ali3DException("CreateOffscreenPlainSurface failed");
      }
      if (direct3ddevice->GetRenderTargetData(_nativeSurface->GetRenderSurface().get(), surface.get()) != D3D_OK)
      {
        throw Ali3DException("GetRenderTargetData failed");
      }
      viewport = _nativeBackbuffer.Viewport;
    }
    // Get the back buffer surface
    else
    {
      surface = _screenBackbuffer.Surface;
      viewport = _screenBackbuffer.Viewport;
    }

    // TODO: pick this out as a method that converts texture (any texture) to bitmap
    RECT copy_from_rc;
    RectToRECT(copy_from, copy_from_rc);
    D3DLOCKED_RECT lockedRect;
    D3DSURFACE_DESC surf_desc;
    surface->GetDesc(&surf_desc);
    if (surface->LockRect(&lockedRect, &copy_from_rc, D3DLOCK_READONLY) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::LockRect failed");
    }

    // If surface format does not have a valid alpha channel,
    // then fill it with opaqueness while copying pixels, otherwise plain copy data
    if (D3DFormatHasAlpha(surf_desc.Format))
    {
      BitmapHelper::ReadPixelsFromMemory(destination, static_cast<uint8_t*>(lockedRect.pBits), lockedRect.Pitch);
    }
    else
    {
      // TODO: pick this out as a separate utility function
      // NOTE: this is currently hardcoded to read from a 32-bit offscreen surface
      const int bpp = destination->GetBPP();
      uint8_t* src_ptr = static_cast<uint8_t*>(lockedRect.pBits);
      for (int y = 0; y < destination->GetHeight(); ++y)
      {
        uint32_t *dst_ptr = reinterpret_cast<uint32_t*>(destination->GetScanLineForWriting(y));
        for (int dx = 0, sx = 0; dx < destination->GetWidth(); ++dx, sx = dx * bpp)
        {
          dst_ptr[dx] = makeacol32(src_ptr[sx + 2], src_ptr[sx + 1], src_ptr[sx + 0], 0xFF /* opaque */);
        }
        src_ptr += lockedRect.Pitch;
      }
    }

    surface->UnlockRect();
  }
  return true;
}

void D3DGraphicsDriver::Render()
{
    Render(0, 0, kFlip_None);
}

void D3DGraphicsDriver::Render(int /*xoff*/, int /*yoff*/, GraphicFlip /*flip*/)
{
    ResetDeviceIfNecessary();
    RenderAndPresent(true);
}

void D3DGraphicsDriver::RenderToBackBuffer()
{
    ResetDeviceIfNecessary();
    RenderImpl(true);
}

void D3DGraphicsDriver::Render(IDriverDependantBitmap *target)
{
    ResetDeviceIfNecessary();
    D3DBitmap *bitmap = (D3DBitmap*)target;
    Size surf_sz = bitmap->GetSize();
    BackbufferState backbuffer = BackbufferState(bitmap->GetRenderSurface(), surf_sz, surf_sz,
        RectWH(0, 0, surf_sz.Width, surf_sz.Height), glmex::ortho_d3d(surf_sz.Width, surf_sz.Height),
        PlaneScaling(), D3DTEXF_POINT);
    RenderToSurface(&backbuffer, true);
}

void D3DGraphicsDriver::RenderSprite(const D3DDrawListEntry *drawListEntry, const glm::mat4 &matGlobal,
    const SpriteColorTransform &color, const Size &rend_sz)
{
    RenderTexture(drawListEntry->ddb, drawListEntry->x, drawListEntry->y, matGlobal, color, rend_sz);
}

void D3DGraphicsDriver::RenderTexture(D3DBitmap *bmpToDraw, int draw_x, int draw_y,
    const glm::mat4 &matGlobal, const SpriteColorTransform &color, const Size &rend_sz)
{
  HRESULT hr;

  const int alpha = (color.Alpha * bmpToDraw->GetAlpha()) / 255;
  const int invalpha = 255 - alpha;
  int tint_r, tint_g, tint_b, tint_sat, light_lev;
  bmpToDraw->GetTint(tint_r, tint_g, tint_b, tint_sat);
  light_lev = bmpToDraw->GetLightLevel();
  const bool do_tint = tint_sat > 0 && _tintShader.ShaderPtr;

  if (bmpToDraw->GetShader() != 0u && bmpToDraw->GetShader() < _shaders.size())
  {
    // Use custom shader
    const ShaderProgram *program = program = &_shaders[bmpToDraw->GetShader()];
    float vector[4]{};
    direct3ddevice->SetPixelShader(program->ShaderPtr.get());

    // FIXME: find out why we cannot set these constants once per render start, like in OpenGL;
    // they seem to get reset to defaults every time a shader is selected (??)
    /**/
    vector[0] = _globalShaderConst.Time;
    direct3ddevice->SetPixelShaderConstantF(program->Time, &vector[0], 1);
    vector[0] = _globalShaderConst.GameFrame;
    direct3ddevice->SetPixelShaderConstantF(program->GameFrame, &vector[0], 1);
    /**/
    //

    vector[0] = bmpToDraw->GetWidth();
    vector[1] = bmpToDraw->GetHeight();
    direct3ddevice->SetPixelShaderConstantF(program->TextureDim, &vector[0], 1);
    vector[0] = alpha / 255.0f;
    direct3ddevice->SetPixelShaderConstantF(program->Alpha, &vector[0], 1);
  }
  else if (do_tint)
  {
    // Use Tinting pixel shader
    float vector[8];
    rgb_to_hsv(tint_r, tint_g, tint_b, &vector[0], &vector[1], &vector[2]);
    vector[0] /= 360.0; // In HSV, Hue is 0-360

    vector[3] = (float)tint_sat / 256.0;
    vector[4] = (float)alpha / 256.0;

    if (light_lev > 0)
      vector[5] = (float)light_lev / 256.0;
    else
      vector[5] = 1.0f;

    direct3ddevice->SetPixelShader(_tintShader.ShaderPtr.get());
    direct3ddevice->SetPixelShaderConstantF(_tintShader.TintHSV, &vector[0], 1);
    direct3ddevice->SetPixelShaderConstantF(_tintShader.TintAlphaLight, &vector[4], 1);
  }
  else
  {
    // Alpha transparency OR light effect
    direct3ddevice->SetPixelShader(NULL);
    int useTintRed = 255;
    int useTintGreen = 255;
    int useTintBlue = 255;
    int textureColorOp = D3DTOP_MODULATE;

    if ((light_lev > 0) && (light_lev < 256))
    {
      // darkening the sprite... this stupid calculation is for
      // consistency with the allegro software-mode code that does
      // a trans blend with a (8,8,8) sprite
      useTintRed = (light_lev * 192) / 256 + 64;
      useTintGreen = useTintRed;
      useTintBlue = useTintRed;
    }
    else if (light_lev > 256)
    {
      // ideally we would use a multi-stage operation here
      // because we need to do TEXTURE + (TEXTURE x LIGHT)
      // but is it worth having to set the device to 2-stage?
      textureColorOp = D3DTOP_ADD;
      useTintRed = (light_lev - 256) / 2;
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

  const auto *txdata = bmpToDraw->GetTexture();
  if (txdata->_vertex == nullptr)
  {
    hr = direct3ddevice->SetStreamSource(0, vertexbuffer.get(), 0, sizeof(CUSTOMVERTEX));
  }
  else
  {
    hr = direct3ddevice->SetStreamSource(0, txdata->_vertex.get(), 0, sizeof(CUSTOMVERTEX));
  }
  if (hr != D3D_OK) 
  {
    throw Ali3DException("IDirect3DDevice9::SetStreamSource failed");
  }

  float width = bmpToDraw->GetWidthToRender();
  float height = bmpToDraw->GetHeightToRender();
  float xProportion = width / (float)bmpToDraw->GetWidth();
  float yProportion = height / (float)bmpToDraw->GetHeight();

  for (size_t ti = 0; ti < txdata->_tiles.size(); ++ti)
  {
    width = txdata->_tiles[ti].width * xProportion;
    height = txdata->_tiles[ti].height * yProportion;
    float xOffs, yOffs;
    if ((bmpToDraw->GetFlip() & kFlip_Horizontal) != 0)
      xOffs = (bmpToDraw->GetWidth() - (txdata->_tiles[ti].x + txdata->_tiles[ti].width)) * xProportion;
    else
      xOffs = txdata->_tiles[ti].x * xProportion;
    if ((bmpToDraw->GetFlip() & kFlip_Vertical) != 0)
      yOffs = (bmpToDraw->GetHeight() - (txdata->_tiles[ti].y + txdata->_tiles[ti].height)) * yProportion;
    else
      yOffs = txdata->_tiles[ti].y * yProportion;
    float thisX = draw_x + xOffs;
    float thisY = draw_y + yOffs;

    //Setup translation and scaling matrices
    float widthToScale = width;
    float heightToScale = height;
    if ((bmpToDraw->GetFlip() & kFlip_Horizontal) != 0)
    {
      // The usual transform changes 0..1 into 0..width
      // So first negate it (which changes 0..w into -w..0)
      widthToScale = -widthToScale;
      // and now shift it over to make it 0..w again
      thisX += width;
    }
    if ((bmpToDraw->GetFlip() & kFlip_Vertical) != 0)
    {
      heightToScale = -heightToScale;
      thisY += height;
    }
    // Apply sprite origin
    thisX -= abs(widthToScale) * bmpToDraw->GetOrigin().X;
    thisY -= abs(heightToScale) * bmpToDraw->GetOrigin().Y;
    // Center inside a rendering rect
    // FIXME: this should be a part of a projection matrix, afaik
    thisX = (-(rend_sz.Width / 2.0f)) + thisX;
    thisY = (rend_sz.Height / 2.0f) - thisY; // inverse axis

    // Setup rotation and pivot
    float rotZ = bmpToDraw->GetRotation();
    float pivotX = -(widthToScale * 0.5), pivotY = (heightToScale * 0.5);

    // Self sprite transform (first scale, then rotate and then translate, reversed)
    glm::mat4 transform = glmex::make_transform2d(
        thisX - _pixelRenderXOffset, thisY + _pixelRenderYOffset, widthToScale, heightToScale,
        rotZ, pivotX, pivotY);
    // Global batch transform
    transform = matGlobal * transform;

    if ((_smoothScaling) && bmpToDraw->GetUseResampler()
        && bmpToDraw->GetSizeToRender() != bmpToDraw->GetSize())
    {
      direct3ddevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      direct3ddevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    }
    else
    {
      direct3ddevice->SetSamplerState(0, D3DSAMP_MINFILTER, _currentBackbuffer->Filter);
      direct3ddevice->SetSamplerState(0, D3DSAMP_MAGFILTER, _currentBackbuffer->Filter);
    }

    direct3ddevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)glm::value_ptr(transform));
    direct3ddevice->SetTexture(0, txdata->_tiles[ti].texture.get());

    // Treat special render modes
    switch (bmpToDraw->GetRenderHint())
    {
    case kTxHint_PremulAlpha:
        direct3ddevice->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_RGBA(alpha, alpha, alpha, 255));
        SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_INVSRCALPHA);
        break;
    default:
        break;
    }

    // FIXME: user blend modes break the above special blend for RT textures
    // Blend modes
    switch (bmpToDraw->GetBlendMode())
    {
    case kBlend_Normal:
        // blend mode is always NORMAL at this point
        // SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA); // ALPHA
        break;
    case kBlend_Add: SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ONE); break; // ADD (transparency = strength)
    case kBlend_Darken: SetBlendOpRGB(D3DBLENDOP_MIN, D3DBLEND_ONE, D3DBLEND_ONE); break; // DARKEN
    case kBlend_Lighten: SetBlendOpRGB(D3DBLENDOP_MAX, D3DBLEND_ONE, D3DBLEND_ONE); break; // LIGHTEN
    case kBlend_Multiply: SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCCOLOR); break; // MULTIPLY
    case kBlend_Screen: SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR); break; // SCREEN
    case kBlend_Subtract: SetBlendOpRGB(D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ONE); break; // SUBTRACT (transparency = strength)
    case kBlend_Exclusion: SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_INVDESTCOLOR, D3DBLEND_INVSRCCOLOR); break; // EXCLUSION
    // APPROXIMATIONS (need pixel shaders)
    case kBlend_Burn: SetBlendOpRGB(D3DBLENDOP_SUBTRACT, D3DBLEND_DESTCOLOR, D3DBLEND_INVDESTCOLOR); break; // LINEAR BURN (approximation)
    case kBlend_Dodge: SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_ONE); break; // fake color dodge (half strength of the real thing)
    // IMPORTANT: please note that we are rendering onto a surface that has a premultiplied alpha,
    // that's why Copy blend modes are somewhat different from the math you'd normally expect;
    // i.e. we do not use D3DBLEND_ONE, D3DBLEND_ZERO to copy RGB, but D3DBLEND_SRCALPHA, D3DBLEND_ZERO.
    case kBlend_Copy:
        SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO);
        SetBlendOpAlpha(D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO);
        // Disabling alpha test seems to be required, because Direct3D
        // skips source zero alpha completely otherwise
        direct3ddevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        break;
    case kBlend_CopyRGB:
        SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ZERO);
        SetBlendOpAlpha(D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE);
        direct3ddevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        break;
    case kBlend_CopyAlpha:
        // FIXME: this does not really work whenever destination has non-opaque alpha,
        // because destination surface colors are alpha-premultiplied!
        SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA);
        SetBlendOpAlpha(D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO);
        direct3ddevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        break;
    default: break;
    }

    // BLENDMODES WORKAROUNDS - BEGIN

    // allow transparency with blending modes
    // darken/lighten the base sprite so a higher transparency value makes it trasparent
    switch (bmpToDraw->GetBlendMode())
    {
    case kBlend_Darken:
    case kBlend_Multiply:
    case kBlend_Burn:
        // fade to white
        // FIXME burn is imperfect due to blend mode, darker than normal even when transparent
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
    default:
        break;
    }

    // workaround: extra render pass for the blend modes that cannot be achieved with one
    switch (bmpToDraw->GetBlendMode())
    {
    case kBlend_Dodge:
        // since the dodge is only half strength we can get a closer approx by drawing it twice
        hr = direct3ddevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, ti * 4, 2);
        break;
    default:
        break;
    }
    // BLENDMODES WORKAROUNDS - END

    hr = direct3ddevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, ti * 4, 2);
    if (hr != D3D_OK) 
    {
      throw Ali3DException("IDirect3DDevice9::DrawPrimitive failed");
    }

    // Restore default blending mode, using render target's settings
    // FIXME: set everything prior to a texture drawing instead?
    SetBlendOpRGB(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
    SetBlendOpAlpha(_rtBlendAlpha.Op, _rtBlendAlpha.Src, _rtBlendAlpha.Dst);
    direct3ddevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  }
}

void D3DGraphicsDriver::RenderAndPresent(bool clearDrawListAfterwards)
{
    RenderImpl(clearDrawListAfterwards);
    direct3ddevice->Present(NULL, NULL, NULL, NULL);
}

void D3DGraphicsDriver::RenderImpl(bool clearDrawListAfterwards)
{
    if (_renderAtScreenRes)
    {
        RenderToSurface(&_screenBackbuffer, clearDrawListAfterwards);
    }
    else
    {
        RenderToSurface(&_nativeBackbuffer, clearDrawListAfterwards);
    }

    if (!_renderAtScreenRes)
    {
        // Draw native texture on a real backbuffer
        SetBackbufferState(&_screenBackbuffer, true);
        if (direct3ddevice->BeginScene() != D3D_OK)
        {
            throw Ali3DException("IDirect3DDevice9::BeginScene failed");
        }
        RenderTexture(_nativeSurface, 0, 0, glmex::identity(), SpriteColorTransform(), _srcRect.GetSize());
        direct3ddevice->EndScene();
    }
}

void D3DGraphicsDriver::RenderToSurface(BackbufferState *state, bool clearDrawListAfterwards)
{
    SetBackbufferState(state, true);

    // if showing at 2x size, the sprite can get distorted otherwise
    direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

    if (direct3ddevice->BeginScene() != D3D_OK)
    {
        throw Ali3DException("IDirect3DDevice9::BeginScene failed");
    }

    UpdateGlobalShaderArgValues();
    RenderSpriteBatches();

    direct3ddevice->EndScene();

    if (clearDrawListAfterwards)
    {
        BackupDrawLists();
        ClearDrawLists();
    }
    ResetFxPool();
}

void D3DGraphicsDriver::SetBackbufferState(BackbufferState *state, bool clear)
{
    _currentBackbuffer = state;
    if (direct3ddevice->SetRenderTarget(0, _currentBackbuffer->Surface.get()) != D3D_OK)
    {
        throw Ali3DException("IDirect3DSurface9::SetRenderTarget failed");
    }
    SetD3DViewport(_currentBackbuffer->Viewport);

    if (clear)
    {
        direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 0.5f, 0);
    }
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
        Rect scissor = render_on_texture ? clip : _currentBackbuffer->Scaling.ScaleRange(clip);
        RECT d3d_scissor;
        RectToRECT(scissor, d3d_scissor);
        direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        direct3ddevice->SetScissorRect(&d3d_scissor);
    }
    else
    {
        direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }
}

void D3DGraphicsDriver::SetRenderTarget(const D3DSpriteBatch *batch, Size &rend_sz, bool clear)
{
    if (batch && batch->RenderTarget)
    {
        // Assign an arbitrary render target, and setup render params
        HRESULT hr = direct3ddevice->SetRenderTarget(0, batch->RenderSurface.get());
        assert(hr == D3D_OK);
        Size surface_sz = Size(batch->RenderTarget->GetWidth(), batch->RenderTarget->GetHeight());
        rend_sz = surface_sz;
        SetD3DViewport(RectWH(surface_sz));
        glm::mat4 mat_ortho = glmex::ortho_d3d(surface_sz.Width, surface_sz.Height);
        direct3ddevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)glm::value_ptr(mat_ortho));
        // Configure rules for merging sprite alpha values onto a
        // render target, which also contains alpha channel.
        SetBlendOpAlpha(D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_ONE);
        _rtBlendAlpha = BlendOpState(D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_ONE);
    }
    else
    {
        // Assign the default backbuffer
        HRESULT hr = direct3ddevice->SetRenderTarget(0, _currentBackbuffer->Surface.get());
        assert(hr == D3D_OK);
        rend_sz = _currentBackbuffer->RendSize;
        SetD3DViewport(_currentBackbuffer->Viewport);
        direct3ddevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)glm::value_ptr(_currentBackbuffer->Projection));
        // Return back to default alpha merging rules
        SetBlendOpAlpha(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        _rtBlendAlpha = BlendOpState(D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
    }

    if (clear)
    {
        direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 0.5f, 0);
    }
}

void D3DGraphicsDriver::RenderSpriteBatches()
{
    assert(_currentBackbuffer);
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
    IDirect3DSurface9 *back_buffer = _currentBackbuffer->Surface.get();
    assert(back_buffer);
    IDirect3DSurface9 *cur_rt = back_buffer; // current render target
    Size rend_sz = _currentBackbuffer->RendSize; // current rt surface size

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
            if (rt_parent.RenderSurface && (cur_rt != rt_parent.RenderSurface.get()) ||
                !rt_parent.RenderSurface && (cur_rt != back_buffer))
            {
                cur_rt = rt_parent.RenderSurface ? rt_parent.RenderSurface.get() : back_buffer;
                SetRenderTarget(&rt_parent, rend_sz, new_batch);
            }
        }

        // Render immediate batch sprites, if any, update cur_spr iterator;
        // we know that the batch has sprites, if next sprite in list belongs to it ("node" ref)
        if ((cur_spr < _spriteList.size()) && (cur_bat == _spriteList[cur_spr].node))
        {
            // Now set clip (scissor), and render sprites
            SetScissor(batch.Viewport, (cur_rt != back_buffer));
            _stageMatrixes.World = batch.Matrix;
            _rendSpriteBatch = batch.ID;
            cur_spr = RenderSpriteBatch(batch, cur_spr, rend_sz);
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

    SetRenderTarget(nullptr, rend_sz, false);
    _rendSpriteBatch = UINT32_MAX;
    _stageMatrixes.World = _spriteBatches[0].Matrix;
    direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}

size_t D3DGraphicsDriver::RenderSpriteBatch(const D3DSpriteBatch &batch, size_t from, const Size &rend_sz)
{
    if (batch.Skip)
    {
        for (; (from < _spriteList.size()) && (_spriteList[from].node == batch.ID); ++from);
        return from;
    }

    for (; (from < _spriteList.size()) && (_spriteList[from].node == batch.ID); ++from)
    {
        const auto &e = _spriteList[from];
        if (e.skip)
            continue;

        switch (reinterpret_cast<uintptr_t>(e.ddb))
        {
        case DRAWENTRY_STAGECALLBACK:
            // raw-draw plugin support
            int sx, sy;
            if (auto *ddb = DoSpriteEvtCallback(e.x, reinterpret_cast<intptr_t>(direct3ddevice.get()), sx, sy))
            {
                auto stageEntry = D3DDrawListEntry((D3DBitmap*)ddb, batch.ID, sx, sy);
                RenderSprite(&stageEntry, batch.Matrix, batch.Color, rend_sz);
            }
            break;
        default:
            RenderSprite(&e, batch.Matrix, batch.Color, rend_sz);
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
        desc.Transform.ScaleX, desc.Transform.ScaleY,
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
        desc.Transform.ScaleX, desc.Transform.ScaleY,
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

void D3DGraphicsDriver::FilterSpriteBatches(uint32_t skip_filter)
{
    if (skip_filter == 0)
        return;
    for (size_t i = 0; i < _spriteBatchDesc.size(); ++i)
    {
        if (_spriteBatchDesc[i].FilterFlags & skip_filter)
        {
            const auto range = _spriteBatchRange[i];
            for (size_t spr = range.first; spr < range.second; ++spr)
                _spriteList[spr].skip = true;
        }
    }
}

void D3DGraphicsDriver::RedrawLastFrame(uint32_t skip_filter)
{
    RestoreDrawLists();
    FilterSpriteBatches(skip_filter);
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
    if (((D3DBitmap*)ddb)->GetTexture()->RenderTarget)
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
                backup_rt.Skip = true;
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

void D3DGraphicsDriver::UpdateTextureRegion(D3DTextureTile *tile, const Bitmap *bitmap, bool opaque)
{
  auto &texture = tile->texture;

  D3DLOCKED_RECT lockedRegion;
  HRESULT hr = texture->LockRect(0, &lockedRegion, NULL, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);
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

  texture->UnlockRect(0);
}

void D3DGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap *ddb, const Bitmap *bitmap)
{
  // FIXME: what to do if texture is shared??
  D3DBitmap *target = (D3DBitmap*)ddb;
  UpdateTexture(target->GetTexture(), bitmap, target->IsOpaque());
}

void D3DGraphicsDriver::UpdateTexture(Texture *txdata, const Bitmap *bitmap, bool opaque)
{
  const int color_depth = bitmap->GetColorDepth();
  if (bitmap->GetColorDepth() != txdata->Res.ColorDepth)
    throw Ali3DException("UpdateDDBFromBitmap: mismatched colour depths");
  if (txdata->Res.Width != bitmap->GetWidth() || txdata->Res.Height != bitmap->GetHeight())
    throw Ali3DException("UpdateDDBFromBitmap: mismatched bitmap size");

  if (color_depth == 8)
      select_palette(palette);

  auto *d3ddata = reinterpret_cast<D3DTexture*>(txdata);
  for (auto &tile : d3ddata->_tiles)
  {
    UpdateTextureRegion(&tile, bitmap, opaque);
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

uint64_t D3DGraphicsDriver::GetAvailableTextureMemory()
{
  return direct3ddevice->GetAvailableTextureMem();
}

void D3DGraphicsDriver::AdjustSizeToNearestSupportedByCard(int *width, int *height)
{
    int allocatedWidth = *width, allocatedHeight = *height;

    if (direct3ddevicecaps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
        int pow2;
        for (pow2 = 2; pow2 < allocatedWidth; pow2 <<= 1);
        allocatedWidth = pow2;
        for (pow2 = 2; pow2 < allocatedHeight; pow2 <<= 1);
        allocatedHeight = pow2;
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
    HRESULT hr = direct3d->CheckDeviceFormat(direct3dcreateparams.AdapterOrdinal,
                                          D3DDEVTYPE_HAL,
                                          AdapterFormat,
                                          0,
                                          D3DRTYPE_TEXTURE,
                                          TextureFormat);
    
    return SUCCEEDED( hr );
}

IDriverDependantBitmap* D3DGraphicsDriver::CreateDDB(int width, int height, int color_depth, int txflags)
{
    if (color_depth != GetCompatibleBitmapFormat(color_depth))
        throw Ali3DException("CreateDDB: bitmap colour depth not supported");
    D3DBitmap *ddb = new D3DBitmap(width, height, color_depth, txflags);
    ddb->SetTexture(std::shared_ptr<D3DTexture>(
        reinterpret_cast<D3DTexture*>(CreateTexture(width, height, color_depth, txflags))));
    return ddb;
}

IDriverDependantBitmap *D3DGraphicsDriver::CreateDDB(std::shared_ptr<Texture> txdata, int txflags)
{
    auto *ddb = reinterpret_cast<D3DBitmap*>(CreateDDB(txdata->Res.Width, txdata->Res.Height, txdata->Res.ColorDepth, txflags));
    if (ddb)
        ddb->SetTexture(std::static_pointer_cast<D3DTexture>(txdata));
    return ddb;
}

IDriverDependantBitmap* D3DGraphicsDriver::CreateRenderTargetDDB(int width, int height, int color_depth, int txflags)
{
    if (color_depth != GetCompatibleBitmapFormat(color_depth))
        throw Ali3DException("CreateDDB: bitmap colour depth not supported");

    txflags |= kTxFlags_RenderTarget;
    auto txdata = std::shared_ptr<D3DTexture>(
        reinterpret_cast<D3DTexture*>(CreateTexture(width, height, color_depth, txflags)));
    // FIXME: this ugly accessing internal texture members
    auto &tex = txdata->_tiles[0].texture;
    D3DSurfacePtr surf;
    HRESULT hr = tex->GetSurfaceLevel(0, surf.Acquire());
    assert(hr == D3D_OK);
    D3DBitmap *ddb = new D3DBitmap(width, height, color_depth, txflags);
    ddb->SetTexture(txdata, surf, kTxHint_PremulAlpha);
    _renderTargets.push_back(ddb);
    return ddb;
}

std::shared_ptr<Texture> D3DGraphicsDriver::GetTexture(IDriverDependantBitmap *ddb)
{
    return std::static_pointer_cast<Texture>((reinterpret_cast<D3DBitmap*>(ddb))->GetSharedTexture());
}

Texture *D3DGraphicsDriver::CreateTexture(int width, int height, int color_depth, int txflags)
{
  assert(width > 0);
  assert(height > 0);
  int allocatedWidth = width;
  int allocatedHeight = height;
  AdjustSizeToNearestSupportedByCard(&allocatedWidth, &allocatedHeight);

  // Render targets must have D3DUSAGE_RENDERTARGET flag and be
  // created in a D3DPOOL_DEFAULT
  const bool as_render_target = (txflags & kTxFlags_RenderTarget) != 0;
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
  const int numTiles = tilesAcross * tilesDown;
  std::vector<D3DTextureTile> tiles(numTiles);
  CUSTOMVERTEX *vertices = nullptr;

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
         D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, txdata->_vertex.Acquire(), NULL);

     if (hr != D3D_OK) 
     {
        throw Ali3DException(String::FromFormat(
            "Direct3DDevice9::CreateVertexBuffer(Length=%d) for texture failed: error code 0x%08X", vertexBufferSize, hr));
     }

     if (txdata->_vertex->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD) != D3D_OK)
     {
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
      thisTile->allocWidth = thisAllocatedWidth;
      thisTile->allocHeight = thisAllocatedHeight;

      if (vertices != nullptr)
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
      D3DFORMAT texture_fmt = ColorDepthToD3DFormat(_mode.ColorDepth, (txflags & kTxFlags_Opaque) == 0);
      HRESULT hr = direct3ddevice->CreateTexture(thisAllocatedWidth, thisAllocatedHeight, 1,
                                      texture_use, texture_fmt,
                                      texture_pool, thisTile->texture.Acquire(), NULL);
      if (hr != D3D_OK)
      {
        throw Ali3DException(String::FromFormat(
            "Direct3DDevice9::CreateTexture(X=%d, Y=%d, FMT=%d) failed: error code 0x%08X", thisAllocatedWidth, thisAllocatedHeight, texture_fmt, hr));
      }

    }
  }

  if (vertices != nullptr)
  {
      txdata->_vertex->Unlock();
  }

  txdata->_tiles = std::move(tiles);
  return txdata;
}

uint32_t D3DGraphicsDriver::CreateShaderProgram(const String &name, const char *fragment_shader_src)
{
#if (DIRECT3D_USE_D3DCOMPILER)
    if (_shaderLookup.find(name) != _shaderLookup.end())
        return UINT32_MAX; // the name is in use

    ComPtr<ID3DBlob> out_data, out_errors;
    HRESULT hr = D3DCompile(fragment_shader_src, strlen(fragment_shader_src) + 1, name.GetCStr(),
                            nullptr /* no defines */, nullptr /* no includes */, "main" /* entrypoint */,
                            DefaultShaderCompileTarget, DefaultShaderCompileFlags, 0 /* no effect flags */,
                            out_data.Acquire(), out_errors.Acquire());

    if (out_errors)
        OutputShaderLog(out_errors, name, hr != D3D_OK);
    if (hr != D3D_OK)
        return UINT32_MAX;

    ShaderProgram prg;
    if (!CreateShaderProgram(prg, name, static_cast<const uint8_t*>(out_data->GetBufferPointer())))
        return UINT32_MAX;

    return AddShaderToCollection(prg, name);
#else // !DIRECT3D_USE_D3DCOMPILER
    return UINT32_MAX;
#endif // DIRECT3D_USE_D3DCOMPILER
}

uint32_t D3DGraphicsDriver::CreateShaderProgram(const String &name, const std::vector<uint8_t> &compiled_data)
{
    if (_shaderLookup.find(name) != _shaderLookup.end())
        return UINT32_MAX; // the name is in use

    ShaderProgram prg;
    if (!CreateShaderProgram(prg, name, compiled_data.data()))
        return UINT32_MAX;

    return AddShaderToCollection(prg, name);
}

uint32_t D3DGraphicsDriver::FindShaderProgram(const String &name)
{
    auto found_it = _shaderLookup.find(name);
    if (found_it == _shaderLookup.end())
        return UINT32_MAX; // not found
    return found_it->second;
}

void D3DGraphicsDriver::DeleteShaderProgram(const String &name)
{
    auto found_it = _shaderLookup.find(name);
    if (found_it == _shaderLookup.end())
        return; // not found
    uint32_t shader_id = found_it->second;
    DeleteShaderProgram(_shaders[shader_id]);
    _shaders[shader_id] = {};
    _shaderLookup.erase(name);
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
    HRESULT hr = ResetDeviceAndRestore();
    if (hr != D3D_OK)
    {
        Debug::Printf(kDbgMsg_Error, "D3D: SetVsync (%d): failed to reset D3D device", enabled);
        return false;
    }
    // CHECKME: is there a need to test that vsync was actually set (ask d3d api)?
    vsync_res = d3dpp.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE;
    return true;
}


D3DGraphicsFactory *D3DGraphicsFactory::_factory = nullptr;
Library D3DGraphicsFactory::_library;

D3DGraphicsFactory::~D3DGraphicsFactory()
{
    DestroyDriver(); // driver must be destroyed before d3d library is disposed
    uint64_t ref_cnt = _direct3d.ReleaseAndCheck();
    if (ref_cnt > 0)
        Debug::Printf(kDbgMsg_Warn, "WARNING: Not all of the Direct3D resources have been disposed; ID3D ref count: %llu", ref_cnt);
    _factory = nullptr;
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
        return nullptr;
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
            _factory = nullptr;
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
    return nullptr;
}


D3DGraphicsDriver *D3DGraphicsFactory::EnsureDriverCreated()
{
    if (!_driver)
    {
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
    return nullptr;
}

bool D3DGraphicsFactory::Init()
{
    assert(_direct3d == nullptr);
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
    _direct3d.Attach(lpDirect3DCreate9(D3D_SDK_VERSION));
    if (!_direct3d)
    {
        _library.Unload();
        SDL_SetError("Direct3DCreate failed!");
        return false;
    }

    D3DADAPTER_IDENTIFIER9 adapterid;
    _direct3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterid);
    String adapter_info = String::FromFormat(
        "\tDriver: %s, v%d.%d.%d.%d\n\tDescription: %s",
        adapterid.Driver,
        HIWORD(adapterid.DriverVersion.HighPart), LOWORD(adapterid.DriverVersion.HighPart),
        HIWORD(adapterid.DriverVersion.LowPart), LOWORD(adapterid.DriverVersion.LowPart),
        adapterid.Description
    );
    Debug::Printf(kDbgMsg_Info, "Direct3D adapter info:\n%s", adapter_info.GetCStr());
    return true;
}

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // AGS_HAS_DIRECT3D

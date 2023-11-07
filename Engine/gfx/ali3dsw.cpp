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
#define NOMINMAX
#include "gfx/ali3dsw.h"
#include <algorithm>
#include <stack>
#include "ac/sys_events.h"
#include "gfx/ali3dexception.h"
#include "gfx/gfxfilter_sdl_renderer.h"
#include "gfx/gfx_util.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "ac/timer.h"

namespace AGS
{
namespace Engine
{
namespace ALSW
{

using namespace Common;

static uint32_t _trans_alpha_blender32(uint32_t x, uint32_t y, uint32_t n);
RGB faded_out_palette[256];


// ----------------------------------------------------------------------------
// SDLRendererGraphicsDriver
// ----------------------------------------------------------------------------

#if SDL_VERSION_ATLEAST(2, 0, 5)
// used to add alpha back to rendered screen.
static auto fix_alpha_blender = SDL_ComposeCustomBlendMode(
    SDL_BLENDFACTOR_ZERO,
    SDL_BLENDFACTOR_ONE,
    SDL_BLENDOPERATION_ADD,
    SDL_BLENDFACTOR_ONE,
    SDL_BLENDFACTOR_ZERO,
    SDL_BLENDOPERATION_ADD
);
#endif

SDLRendererGraphicsDriver::SDLRendererGraphicsDriver()
{
  _tint_red = 0;
  _tint_green = 0;
  _tint_blue = 0;
  _origVirtualScreen = nullptr;
  virtualScreen = nullptr;
  _stageVirtualScreen = nullptr;
}

bool SDLRendererGraphicsDriver::IsModeSupported(const DisplayMode &mode)
{
  if (mode.Width <= 0 || mode.Height <= 0)
  {
    SDL_SetError("Invalid resolution parameters: %d x %d", mode.Width, mode.Height);
    return false;
  }
  if (mode.ColorDepth != 32) {
    SDL_SetError("Display colour depth not supported: %d", mode.ColorDepth);
    return false;
  }
  return true;
}

int SDLRendererGraphicsDriver::GetDisplayDepthForNativeDepth(int /*native_color_depth*/) const
{
    return 32;
}

IGfxModeList *SDLRendererGraphicsDriver::GetSupportedModeList(int color_depth)
{
    std::vector<DisplayMode> modes;
    sys_get_desktop_modes(modes, color_depth);
    if ((modes.size() == 0) && color_depth == 32)
    {
        // Pretend that 24-bit are 32-bit
        sys_get_desktop_modes(modes, 24);
        for (auto &m : modes) { m.ColorDepth = 32; }
    }
    return new SDLRendererGfxModeList(modes);
}

PGfxFilter SDLRendererGraphicsDriver::GetGraphicsFilter() const
{
    return _filter;
}

void SDLRendererGraphicsDriver::SetGraphicsFilter(PSDLRenderFilter filter)
{
  _filter = filter;
  OnSetFilter();

  // TODO: support separate nearest and linear filters, initialize hint by calls to filter object
  // e.g like D3D and OGL filters act
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
  // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
}

void SDLRendererGraphicsDriver::SetTintMethod(TintMethod /*method*/) 
{
  // TODO: support new D3D-style tint method
}

bool SDLRendererGraphicsDriver::SetDisplayMode(const DisplayMode &mode)
{
  ReleaseDisplayMode();

  set_color_depth(mode.ColorDepth);

  if (_initGfxCallback != nullptr)
    _initGfxCallback(nullptr);

  if (!IsModeSupported(mode))
    return false;

  _capsVsync = true; // reset vsync flag, allow to try setting again

  SDL_Window *window = sys_get_window();
  if (!window)
  {
    window = sys_window_create("", mode.Width, mode.Height, mode.Mode);

    _hasGamma = SDL_GetWindowGammaRamp(window, _defaultGammaRed, _defaultGammaGreen, _defaultGammaBlue) == 0;

    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (mode.Vsync) {
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
    }
    _renderer = SDL_CreateRenderer(window, -1, rendererFlags);

    SDL_RendererInfo rinfo{};
    if (SDL_GetRendererInfo(_renderer, &rinfo) == 0) {
      Debug::Printf(kDbgMsg_Info, "Created SDL Renderer: %s", rinfo.name);
      Debug::Printf("Available texture formats:");
      for (Uint32 i = 0; i < rinfo.num_texture_formats; i++) {
        Debug::Printf("\t- %s", SDL_GetPixelFormatName(rinfo.texture_formats[i]));
      }
      // NOTE: SDL_RendererInfo::flags only contains VSYNC if it was requested
      if (rendererFlags & SDL_RENDERER_PRESENTVSYNC)
      {
        _capsVsync = (rinfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
        if (!_capsVsync)
          Debug::Printf(kDbgMsg_Warn, "WARNING: Vertical sync is not supported. Setting will be kept at driver default.");
      }
    } else {
      Debug::Printf("SDLRenderer: failed to query renderer info: %s", SDL_GetError());
    }
  }
  else
  {
    sys_window_set_style(mode.Mode, Size(mode.Width, mode.Height));
  }

#if AGS_PLATFORM_OS_ANDROID
  SDL_RenderSetLogicalSize(_renderer,mode.Width,mode.Height);
#endif

  OnInit();
  OnModeSet(mode);
  return true;
}

void SDLRendererGraphicsDriver::UpdateDeviceScreen(const Size &screen_sz)
{
  _mode.Width = screen_sz.Width;
  _mode.Height = screen_sz.Height;
#if AGS_PLATFORM_OS_ANDROID
  SDL_RenderSetLogicalSize(_renderer, _mode.Width, _mode.Height);
#endif
}

void SDLRendererGraphicsDriver::CreateVirtualScreen()
{
  if (!IsNativeSizeValid())
    return;
  DestroyVirtualScreen();
  // Initialize virtual screen; size is equal to native resolution
  const int vscreen_w = _srcRect.GetWidth();
  const int vscreen_h = _srcRect.GetHeight();
  _origVirtualScreen.reset(new Bitmap(vscreen_w, vscreen_h, _srcColorDepth));
  virtualScreen = _origVirtualScreen.get();
  _stageVirtualScreen = virtualScreen;

  _screenTex = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, vscreen_w, vscreen_h);

  // Fake bitmap that will wrap over texture pixels for simplier conversion
  _fakeTexBitmap = reinterpret_cast<BITMAP*>(new char[sizeof(BITMAP) + (sizeof(char *) * vscreen_h)]);
  _fakeTexBitmap->w = vscreen_w;
  _fakeTexBitmap->cr = vscreen_w;
  _fakeTexBitmap->h = vscreen_h;
  _fakeTexBitmap->cb = vscreen_h;
  _fakeTexBitmap->clip = true;
  _fakeTexBitmap->cl = 0;
  _fakeTexBitmap->ct = 0;
  _fakeTexBitmap->id = 0;
  _fakeTexBitmap->extra = nullptr;
  _fakeTexBitmap->x_ofs = 0;
  _fakeTexBitmap->y_ofs = 0;
  _fakeTexBitmap->dat = nullptr;

  auto tmpbitmap = create_bitmap_ex(32, 1, 1);
  _fakeTexBitmap->vtable = tmpbitmap->vtable;
  destroy_bitmap(tmpbitmap);

  _lastTexPixels = nullptr;
  _lastTexPitch = -1;
}

void SDLRendererGraphicsDriver::DestroyVirtualScreen()
{
  delete[] _fakeTexBitmap; // don't use destroy_bitmap(), because it's a fake structure
  _fakeTexBitmap = nullptr;
  if(_screenTex != nullptr) {
      SDL_DestroyTexture(_screenTex);
  }
  _screenTex = nullptr;

  _origVirtualScreen.reset();
  virtualScreen = nullptr;
  _stageVirtualScreen = nullptr;
}

void SDLRendererGraphicsDriver::ReleaseDisplayMode()
{
  OnModeReleased();
  ClearDrawLists();

  sys_window_set_style(kWnd_Windowed);
}

bool SDLRendererGraphicsDriver::SetNativeResolution(const GraphicResolution &native_res)
{
  OnSetNativeRes(native_res);
  CreateVirtualScreen();
  return !_srcRect.IsEmpty();
}

bool SDLRendererGraphicsDriver::SetRenderFrame(const Rect &dst_rect)
{
  OnSetRenderFrame(dst_rect);
  return !_dstRect.IsEmpty();
}

void SDLRendererGraphicsDriver::ClearRectangle(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, RGB* /*colorToUse*/)
{
  // TODO: but maybe is not necessary, as we use SDL_Renderer with accelerated gfx here?
  // See SDL_RenderDrawRect
}

SDLRendererGraphicsDriver::~SDLRendererGraphicsDriver()
{
  SDLRendererGraphicsDriver::UnInit();
}

void SDLRendererGraphicsDriver::UnInit()
{
  OnUnInit();
  ReleaseDisplayMode();
  DestroyVirtualScreen();

  if (_renderer)
  {
      SDL_DestroyRenderer(_renderer);
      _renderer = nullptr;
  }

  sys_window_destroy();
}

bool SDLRendererGraphicsDriver::SupportsGammaControl() 
{
  return _hasGamma;
}

void SDLRendererGraphicsDriver::SetGamma(int newGamma)
{
  if (!_hasGamma) { return; }

  Uint16 gamma_red[256];
  Uint16 gamma_green[256];
  Uint16 gamma_blue[256];

  for (int i = 0; i < 256; i++) {
    gamma_red[i] = std::min(((int)_defaultGammaRed[i] * newGamma) / 100, 0xffff);
    gamma_green[i] = std::min(((int)_defaultGammaGreen[i] * newGamma) / 100, 0xffff);
    gamma_blue[i] = std::min(((int)_defaultGammaBlue[i] * newGamma) / 100, 0xffff);
  }

  _gamma = newGamma;

  SDL_SetWindowGammaRamp(sys_get_window(), gamma_red, gamma_green, gamma_blue);
}

int SDLRendererGraphicsDriver::GetCompatibleBitmapFormat(int color_depth)
{
  return color_depth;
}

IDriverDependantBitmap* SDLRendererGraphicsDriver::CreateDDB(int width, int height, int color_depth, bool opaque)
{
  return new ALSoftwareBitmap(width, height, color_depth, opaque);
}

IDriverDependantBitmap* SDLRendererGraphicsDriver::CreateDDBFromBitmap(Bitmap *bitmap, bool opaque)
{
  return new ALSoftwareBitmap(bitmap, opaque);
}

IDriverDependantBitmap* SDLRendererGraphicsDriver::CreateRenderTargetDDB(int width, int height, int color_depth, bool opaque)
{
    // For software renderer there's no difference between "texture" types.
    return new ALSoftwareBitmap(width, height, color_depth, opaque);
}

void SDLRendererGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap)
{
  ALSoftwareBitmap* alSwBmp = (ALSoftwareBitmap*)bitmapToUpdate;
  alSwBmp->_bmp = bitmap;
  alSwBmp->_width = bitmap->GetWidth();
  alSwBmp->_height = bitmap->GetHeight();
  alSwBmp->_colDepth = bitmap->GetColorDepth();
}

void SDLRendererGraphicsDriver::DestroyDDB(IDriverDependantBitmap* bitmap)
{
  delete (ALSoftwareBitmap*)bitmap;
}

void SDLRendererGraphicsDriver::InitSpriteBatch(size_t index, const SpriteBatchDesc &desc)
{
    if (_spriteBatches.size() <= index)
        _spriteBatches.resize(index + 1);
    ALSpriteBatch &batch = _spriteBatches[index];
    batch.ID = index;

    // Apply parent batch's settings, if preset;
    Rect viewport = desc.Viewport;
    SpriteTransform transform = desc.Transform;
    Bitmap *parent_surf = virtualScreen;
    if (desc.Parent != UINT32_MAX)
    {
        const auto &parent = _spriteBatches[desc.Parent];
        if (parent.Surface)
            parent_surf = parent.Surface.get();
        // NOTE: we prioritize parent's surface size as a dest viewport,
        // because parent may have a scheduled scaled blit.
        if (viewport.IsEmpty())
            viewport = parent_surf ? RectWH(parent_surf->GetSize()) : RectWH(parent.Viewport.GetSize());
    }
    else if (viewport.IsEmpty())
    {
        viewport = _srcRect;
    }

    // Calculate expected source surf size, based on dest viewport and scaling
    const int src_w = viewport.GetWidth() / transform.ScaleX;
    const int src_h = viewport.GetHeight() / transform.ScaleY;

    // Initialize batch surface, depending on the batch description.
    // Surface was prepared externally (common for room cameras)
    if (desc.Surface)
    {
        batch.Surface = desc.Surface;
        batch.Opaque = true;
        batch.IsParentRegion = false;
    }
    // In case something was not initialized
    else if (desc.Viewport.IsEmpty() || !virtualScreen)
    {
        batch.Surface.reset();
        batch.Opaque = false;
        batch.IsParentRegion = false;
    }
    // Drawing directly on a viewport without transformation (other than offset):
    // then make a subbitmap of the parent surface (virtualScreen or else).
    else if (transform.ScaleX == 1.f && transform.ScaleY == 1.f && transform.Rotate == 0.f)
    {
        // We need this subbitmap for plugins, which use _stageVirtualScreen and are unaware of possible multiple viewports;
        // TODO: there could be ways to optimize this further, but best is to update plugin rendering hooks (and upgrade plugins)
        if (!batch.Surface || !batch.IsParentRegion ||
            (!batch.Surface->IsSameBitmap(parent_surf)) ||
            (batch.Surface->GetSize() != Size(src_w, src_h)) ||
            (batch.Surface->GetSubOffset() != viewport.GetLT()))
        {
            batch.Surface.reset(BitmapHelper::CreateSubBitmap(parent_surf, viewport));
        }
        batch.Opaque = true;
        batch.IsParentRegion = true;
        // Because we sub-bitmap to viewport, render offsets should account for that
        transform.X -= viewport.Left;
        transform.Y -= viewport.Top;
    }
    // No surface prepared and has transformation other than offset:
    // then create exclusive intermediate bitmap.
    else
    {
        if (!batch.Surface || batch.IsParentRegion || (batch.Surface->GetSize() != Size(src_w, src_h)))
        {
            batch.Surface.reset(new Bitmap(src_w, src_h, _srcColorDepth));
        }
        batch.Opaque = false;
        batch.IsParentRegion = false;
    }

    // If there's a rotation tranform: prepare a helper surface
    if (transform.Rotate != 0.f)
    {
        int max_size = std::max(batch.Surface->GetWidth(), batch.Surface->GetHeight());
        Size rot_sz = RotateSize(Size(max_size, max_size), (int)transform.Rotate);
        if (batch.HelpSurface == nullptr || batch.HelpSurface->GetSize() != rot_sz)
            batch.HelpSurface.reset(new Bitmap(rot_sz.Width, rot_sz.Height));
    }
    batch.Viewport = viewport;
    batch.Transform = transform;
}

void SDLRendererGraphicsDriver::ResetAllBatches()
{
    // NOTE: we don't release batches themselves here, only sprite lists.
    // This is because we cache batch surfaces, for perfomance reasons.
    _spriteList.clear();
}

void SDLRendererGraphicsDriver::DrawSprite(int /*ox*/, int /*oy*/, int ltx, int lty, IDriverDependantBitmap* bitmap)
{ // Note we are only interested in left-top coords for software renderer
    assert(_actSpriteBatch != UINT32_MAX);
    _spriteList.push_back(ALDrawListEntry((ALSoftwareBitmap*)bitmap, _actSpriteBatch, ltx, lty));
}

void SDLRendererGraphicsDriver::SetScreenFade(int /*red*/, int /*green*/, int /*blue*/)
{
    // TODO: was not necessary atm
    // TODO: checkme later
}

void SDLRendererGraphicsDriver::SetScreenTint(int red, int green, int blue)
{
    assert(_actSpriteBatch != UINT32_MAX);
    _tint_red = red; _tint_green = green; _tint_blue = blue;
    if (((_tint_red > 0) || (_tint_green > 0) || (_tint_blue > 0)) && (_srcColorDepth > 8))
    {
      _spriteList.push_back(
          ALDrawListEntry(reinterpret_cast<ALSoftwareBitmap*>(DRAWENTRY_TINT), _actSpriteBatch, 0, 0));
    }
}

void SDLRendererGraphicsDriver::SetStageScreen(const Size & /*sz*/, int /*x*/, int /*y*/)
{
    // unsupported, as using _stageVirtualScreen instead
}

void SDLRendererGraphicsDriver::RenderToBackBuffer()
{
    // Close unended batches, and issue a warning
    assert(_actSpriteBatch == UINT32_MAX);
    while (_actSpriteBatch != UINT32_MAX)
        EndSpriteBatch();

    if (_spriteBatchDesc.size() == 0)
    {
        ClearDrawLists();
        return; // no batches - no render
    }

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

    const size_t last_batch_to_rend = _spriteBatchDesc.size() - 1;
    for (size_t cur_bat = 0u, last_bat = 0u, cur_spr = 0u; last_bat <= last_batch_to_rend;)
    {
        // Test if we are entering this batch (and not continuing after coming back from nested)
        if (cur_spr <= _spriteBatchRange[cur_bat].first)
        {
            const auto &batch = _spriteBatches[cur_bat];
            // Prepare the transparent surface
            if (batch.Surface && !batch.Opaque)
                batch.Surface->ClearTransparent();
        }

        // Render immediate batch sprites, if any, update cur_spr iterator
        if ((cur_spr < _spriteList.size()) && (cur_bat == _spriteList[cur_spr].node))
        {
            const auto &batch = _spriteBatches[cur_bat];
            const auto &batch_desc = _spriteBatchDesc[cur_bat];
            Bitmap *surface = batch.Surface.get();
            Bitmap *parent_surf = ((batch_desc.Parent != UINT32_MAX) && _spriteBatches[batch_desc.Parent].Surface) ?
                _spriteBatches[batch_desc.Parent].Surface.get() : virtualScreen;
            const Rect &viewport = batch.Viewport;
            const SpriteTransform &transform = batch.Transform;

            _rendSpriteBatch = batch.ID;
            parent_surf->SetClip(viewport); // CHECKME: this is not exactly correct?
            if (surface && !batch.IsParentRegion)
            {
                _stageVirtualScreen = surface;
                cur_spr = RenderSpriteBatch(batch, cur_spr, surface, transform.X, transform.Y);
            }
            else
            {
                _stageVirtualScreen = surface ? surface : parent_surf;
                cur_spr = RenderSpriteBatch(batch, cur_spr, _stageVirtualScreen, transform.X, transform.Y);
            }
        }

        // Test if we're exiting current batch (and not going into nested ones):
        // if there's no sprites belonging to this batch (direct, or nested),
        // and if there's no nested batches (even if empty ones)
        const uint32_t was_bat = cur_bat;
        while ((cur_bat != UINT32_MAX) && (cur_spr >= _spriteBatchRange[cur_bat].second) &&
                ((last_bat == last_batch_to_rend) || (_spriteBatchDesc[last_bat + 1].Parent != cur_bat)))
        {
            const auto &batch = _spriteBatches[cur_bat];
            const auto &batch_desc = _spriteBatchDesc[cur_bat];
            Bitmap *surface = batch.Surface.get();
            Bitmap *parent_surf = ((batch_desc.Parent != UINT32_MAX) && _spriteBatches[batch_desc.Parent].Surface) ?
                _spriteBatches[batch_desc.Parent].Surface.get() : virtualScreen;
            const Rect &viewport = batch.Viewport;

            // If we're not drawing directly to the subregion of a parent surface,
            // then blit our own surface to the parent's
            if (surface && !batch.IsParentRegion)
            {
                Bitmap *blit_from = surface;
                const int src_w = surface->GetWidth();
                const int src_h = surface->GetHeight();
                int dst_w = surface->GetWidth();
                int dst_h = surface->GetHeight();
                const float rotate = batch_desc.Transform.Rotate;
                if (rotate != 0.f)
                {
                    Bitmap *helpsurf = batch.HelpSurface.get();
                    dst_w = helpsurf->GetWidth();
                    dst_h = helpsurf->GetHeight();
                    helpsurf->Clear(0); // TODO: check later, might create problems with multi-viewports
                    // (+ width%2 fixes one pixel offset problem)
                    helpsurf->RotateBlt(surface, dst_w / 2 + dst_w % 2, dst_h / 2,
                        src_w / 2, src_h / 2, (int)rotate); // clockwise
                    blit_from = helpsurf;
                }
                parent_surf->StretchBlt(blit_from,
                    RectWH((dst_w - src_w) / 2, (dst_h - src_h) / 2, src_w, src_h), // source rect
                    viewport, // destination
                    batch.Opaque ? kBitmap_Copy : kBitmap_Transparency);
            }

            // Back to the parent batch
            cur_bat = batch_desc.Parent;
        }

        // If we stayed at the same batch, this means that there are still nested batches;
        // if there's no batches in the stack left, this means we got to move forward anyway.
        if ((was_bat == cur_bat) || (cur_bat == UINT32_MAX))
        {
            cur_bat = ++last_bat;
        }
    }

    _stageVirtualScreen = virtualScreen;
    _rendSpriteBatch = UINT32_MAX;
    ClearDrawLists();
}

size_t SDLRendererGraphicsDriver::RenderSpriteBatch(const ALSpriteBatch &batch, size_t from, Bitmap *surface, int surf_offx, int surf_offy)
{
  for (; (from < _spriteList.size()) && (_spriteList[from].node == batch.ID); ++from)
  {
    const auto &sprite = _spriteList[from];
    if (sprite.ddb == nullptr)
    {
      if (_spriteEvtCallback)
        _spriteEvtCallback(sprite.x, sprite.y);
      else
        throw Ali3DException("Unhandled attempt to draw null sprite");
      // Stage surface could have been replaced by plugin
      surface = _stageVirtualScreen;
      continue;
    }
    else if (sprite.ddb == reinterpret_cast<ALSoftwareBitmap*>(DRAWENTRY_TINT))
    {
      // draw screen tint fx
      set_trans_blender(_tint_red, _tint_green, _tint_blue, 0);
      surface->LitBlendBlt(surface, 0, 0, 128);
      continue;
    }

    ALSoftwareBitmap* bitmap = sprite.ddb;
    // A sprite entry provides us with the "top-left" image's position here,
    // disregarding sprite origin, because we expect an already transformed
    int drawAtX = sprite.x + surf_offx;
    int drawAtY = sprite.y + surf_offy;

    if (bitmap->_alpha == 0) {} // fully transparent, do nothing
    else if ((bitmap->_opaque) && (bitmap->_bmp == surface) && (bitmap->_alpha == 255)) {}
    else if (bitmap->_opaque && bitmap->_blendMode == 0)
    {
        surface->Blit(bitmap->_bmp, 0, 0, drawAtX, drawAtY, bitmap->_bmp->GetWidth(), bitmap->_bmp->GetHeight());
        // TODO: we need to also support non-masked translucent blend, but...
        // Allegro 4 **does not have such function ready** :( (only masked blends, where it skips magenta pixels);
        // I am leaving this problem for the future, as coincidentally software mode does not need this atm.
    }
    else
    {
        GfxUtil::DrawSpriteBlend(surface, Point(drawAtX, drawAtY), bitmap->_bmp, bitmap->_blendMode, bitmap->_alpha);
    }
  }
  return from;
}

void SDLRendererGraphicsDriver::BlitToTexture()
{
    void *pixels = nullptr;
    int pitch = 0;
    auto res = SDL_LockTexture(_screenTex, NULL, &pixels, &pitch);
    if (res != 0) { return; }

    // Because the virtual screen may be of any color depth,
    // we wrap texture pixels in a fake bitmap here and call
    // standard blit operation, for simplicity sake.
    const int vwidth = virtualScreen->GetWidth();
    const int vheight = virtualScreen->GetHeight();
    if ((_lastTexPixels != pixels) || (_lastTexPitch != pitch)) {
        _fakeTexBitmap->dat = pixels;
        auto p = (unsigned char *)pixels;
        for (int i = 0; i < vheight; i++) {
            _fakeTexBitmap->line[i] = p;
            p += pitch;
        }
        _lastTexPixels = (unsigned char *)pixels;
        _lastTexPitch = pitch;
    }

    blit(virtualScreen->GetAllegroBitmap(), _fakeTexBitmap, 0, 0, 0, 0, vwidth, vheight);

    SDL_UnlockTexture(_screenTex);
}

void SDLRendererGraphicsDriver::Present(int xoff, int yoff, GraphicFlip flip)
{
    if (!_renderer) { return; }

    SDL_RendererFlip sdl_flip;
    switch (flip)
    {
    case kFlip_Horizontal: sdl_flip = SDL_FLIP_HORIZONTAL; break;
    case kFlip_Vertical: sdl_flip = SDL_FLIP_VERTICAL; break;
    case kFlip_Both: sdl_flip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL); break;
    default: sdl_flip = SDL_FLIP_NONE; break;
    }

    BlitToTexture();

    SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(_renderer, nullptr);

    int xoff_final = _scaling.X.ScalePt(xoff);
    int yoff_final = _scaling.Y.ScalePt(yoff);

    SDL_Rect dst;
    dst.x = xoff_final;
    dst.y = yoff_final;
    dst.w = _dstRect.GetWidth();
    dst.h = _dstRect.GetHeight();
    SDL_RenderCopyEx(_renderer, _screenTex, nullptr, &dst, 0.0, nullptr, sdl_flip);

    SDL_RenderPresent(_renderer);
}

void SDLRendererGraphicsDriver::Render(int xoff, int yoff, GraphicFlip flip)
{
    RenderToBackBuffer();
    Present(xoff, yoff, flip);
}

void SDLRendererGraphicsDriver::Render()
{
  Render(0, 0, kFlip_None);
}

Bitmap *SDLRendererGraphicsDriver::GetMemoryBackBuffer()
{
    return virtualScreen;
}

void SDLRendererGraphicsDriver::SetMemoryBackBuffer(Bitmap *backBuffer)
{
    // We need to also test internal AL BITMAP pointer, because we may receive it raw from plugin,
    // in which case the Bitmap object may be a different wrapper over our own virtual screen.
    if (backBuffer && (backBuffer->GetAllegroBitmap() != _origVirtualScreen->GetAllegroBitmap()))
    {
        virtualScreen = backBuffer;
    }
    else
    {
        virtualScreen = _origVirtualScreen.get();
    }
    _stageVirtualScreen = virtualScreen;

    // Reset old virtual screen's subbitmaps;
    // NOTE: this MUST NOT be called in the midst of the RenderSpriteBatches!
    assert(_rendSpriteBatch == UINT32_MAX);
    if (_rendSpriteBatch != UINT32_MAX)
        return;
    for (auto &batch : _spriteBatches)
    {
        if (batch.IsParentRegion)
            batch.Surface.reset();
    }
}

Bitmap *SDLRendererGraphicsDriver::GetStageBackBuffer(bool /*mark_dirty*/)
{
    return _stageVirtualScreen;
}

void SDLRendererGraphicsDriver::SetStageBackBuffer(Bitmap *backBuffer)
{
    Bitmap *cur_stage = (_rendSpriteBatch == UINT32_MAX) ?
        virtualScreen :
        _spriteBatches[_rendSpriteBatch].Surface.get();
    // We need to also test internal AL BITMAP pointer, because we may receive it raw from plugin,
    // in which case the Bitmap object may be a different wrapper over our own virtual screen.
    if (backBuffer && (backBuffer->GetAllegroBitmap() != cur_stage->GetAllegroBitmap()))
        _stageVirtualScreen = backBuffer;
    else
        _stageVirtualScreen = cur_stage;
}

bool SDLRendererGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt)
{
  (void)at_native_res; // software driver always renders at native resolution at the moment
  // software filter is taught to copy to any size
  if (destination->GetColorDepth() != _srcColorDepth)
  {
    if (want_fmt)
        *want_fmt = GraphicResolution(destination->GetWidth(), destination->GetHeight(), _srcColorDepth);
    return false;
  }

  if (destination->GetSize() == virtualScreen->GetSize())
  {
    destination->Blit(virtualScreen, 0, 0, 0, 0, virtualScreen->GetWidth(), virtualScreen->GetHeight());
  }
  else
  {
    destination->StretchBlt(virtualScreen,
          RectWH(0, 0, virtualScreen->GetWidth(), virtualScreen->GetHeight()),
          RectWH(0, 0, destination->GetWidth(), destination->GetHeight()));
  }
  return true;
}

/**
	fade.c - High Color Fading Routines

	Last Revision: 21 June, 2002

	Author: Matthew Leverton
**/
void SDLRendererGraphicsDriver::highcolor_fade_in(Bitmap *vs, void(*draw_callback)(),
    int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
   Bitmap *bmp_orig = vs;
   const int col_depth = bmp_orig->GetColorDepth();
   const int clearColor = makecol_depth(col_depth, targetColourRed, targetColourGreen, targetColourBlue);
   if (speed <= 0) speed = 16;

   Bitmap *bmp_buff = new Bitmap(bmp_orig->GetWidth(), bmp_orig->GetHeight(), col_depth);
   SetMemoryBackBuffer(bmp_buff);
   for (int a = 0; a < 256; a+=speed)
   {
       bmp_buff->Fill(clearColor);
       set_trans_blender(0,0,0,a);
       bmp_buff->TransBlendBlt(bmp_orig, 0, 0);
       
       if (draw_callback)
           draw_callback();
       RenderToBackBuffer();
       Present();

       sys_evt_process_pending();
       if (_pollingCallback)
          _pollingCallback();

       WaitForNextFrame();
   }
   delete bmp_buff;

   SetMemoryBackBuffer(vs);
   if (draw_callback)
       draw_callback();
   RenderToBackBuffer();
   Present();
}

void SDLRendererGraphicsDriver::highcolor_fade_out(Bitmap *vs, void(*draw_callback)(),
    int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
    Bitmap *bmp_orig = vs;
    const int col_depth = vs->GetColorDepth();
    const int clearColor = makecol_depth(col_depth, targetColourRed, targetColourGreen, targetColourBlue);
    if (speed <= 0) speed = 16;

    Bitmap *bmp_buff = new Bitmap(bmp_orig->GetWidth(), bmp_orig->GetHeight(), col_depth);
    SetMemoryBackBuffer(bmp_buff);
    for (int a = 255 - speed; a > 0; a -= speed)
    {
        bmp_buff->Fill(clearColor);
        set_trans_blender(0, 0, 0, a);
        bmp_buff->TransBlendBlt(bmp_orig, 0, 0);

        if (draw_callback)
            draw_callback();
        RenderToBackBuffer();
        Present();

        sys_evt_process_pending();
        if (_pollingCallback)
            _pollingCallback();

        WaitForNextFrame();
    }
    delete bmp_buff;

    SetMemoryBackBuffer(vs);
    vs->Clear(clearColor);
    if (draw_callback)
        draw_callback();
    RenderToBackBuffer();
    Present();
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

void SDLRendererGraphicsDriver::__fade_from_range(PALETTE source, PALETTE dest, int speed, int from, int to) 
{
   PALETTE temp;
   int c;

   for (c=0; c<PAL_SIZE; c++)
      temp[c] = source[c];

   for (c=0; c<64; c+=speed) {
      fade_interpolate(source, dest, temp, c, from, to);
      set_palette_range(temp, from, to, TRUE);

      RenderToBackBuffer();
      Present();

      sys_evt_process_pending();
      if (_pollingCallback)
          _pollingCallback();
   }

   set_palette_range(dest, from, to, TRUE);
}

void SDLRendererGraphicsDriver::__fade_out_range(int speed, int from, int to, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
   PALETTE temp;

   initialize_fade_256(targetColourRed, targetColourGreen, targetColourBlue);
   get_palette(temp);
   __fade_from_range(temp, faded_out_palette, speed, from, to);
}

void SDLRendererGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) {
  if (_srcColorDepth > 8)
  {
    highcolor_fade_out(virtualScreen, _drawPostScreenCallback, speed * 4, targetColourRed, targetColourGreen, targetColourBlue);
  }
  else
  {
    __fade_out_range(speed, 0, 255, targetColourRed, targetColourGreen, targetColourBlue);
  }
}

void SDLRendererGraphicsDriver::FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue) {
  if (_drawScreenCallback)
  {
    _drawScreenCallback();
    RenderToBackBuffer();
  }
  if (_srcColorDepth > 8)
  {
    highcolor_fade_in(virtualScreen, _drawPostScreenCallback, speed * 4, targetColourRed, targetColourGreen, targetColourBlue);
  }
  else
  {
	initialize_fade_256(targetColourRed, targetColourGreen, targetColourBlue);
	__fade_from_range(faded_out_palette, p, speed, 0,255);
  }
}

void SDLRendererGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
{
  if (blackingOut)
  {
    int yspeed = _srcRect.GetHeight() / (_srcRect.GetWidth() / speed);
    int boxwid = speed, boxhit = yspeed;
    Bitmap *bmp_orig = virtualScreen;
    Bitmap *bmp_buff = new Bitmap(bmp_orig->GetWidth(), bmp_orig->GetHeight(), bmp_orig->GetColorDepth());
    SetMemoryBackBuffer(bmp_buff);

    while (boxwid < _srcRect.GetWidth()) {
      boxwid += speed;
      boxhit += yspeed;
      int vcentre = _srcRect.GetHeight() / 2;
      bmp_orig->FillRect(Rect(_srcRect.GetWidth() / 2 - boxwid / 2, vcentre - boxhit / 2,
          _srcRect.GetWidth() / 2 + boxwid / 2, vcentre + boxhit / 2), 0);
      bmp_buff->Fill(0);
      bmp_buff->Blit(bmp_orig);

      if (_drawPostScreenCallback)
          _drawPostScreenCallback();
      RenderToBackBuffer();
      Present();

      sys_evt_process_pending();
      if (_pollingCallback)
          _pollingCallback();

      platform->Delay(delay);
    }
    delete bmp_buff;
    SetMemoryBackBuffer(bmp_orig);
  }
  else
  {
    throw Ali3DException("BoxOut fade-in not implemented in sw gfx driver");
  }
}
// end fading routines

// add the alpha values together, used for compositing alpha images
// TODO: why is this here, move to gfx/blender? check if there's already similar function there
static uint32_t _trans_alpha_blender32(uint32_t x, uint32_t y, uint32_t n)
{
   uint32_t res, g;

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

bool SDLRendererGraphicsDriver::SetVsyncImpl(bool enabled, bool &vsync_res)
{
    #if SDL_VERSION_ATLEAST(2, 0, 18)
    if (SDL_RenderSetVSync(_renderer, enabled) == 0) // 0 on success
    {
        // gamma might be lost after changing vsync mode at fullscreen
        SetGamma(_gamma);
        SDL_RendererInfo info;
        SDL_GetRendererInfo(_renderer, &info);
        vsync_res = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
        return true;
    }
    Debug::Printf(kDbgMsg_Warn, "SDLRenderer: SetVsync (%d) failed: %s", enabled, SDL_GetError());
    #endif
    return false;
}

SDLRendererGraphicsFactory *SDLRendererGraphicsFactory::_factory = nullptr;

SDLRendererGraphicsFactory::~SDLRendererGraphicsFactory()
{
    _factory = nullptr;
}

size_t SDLRendererGraphicsFactory::GetFilterCount() const
{
    return 1;
}

const GfxFilterInfo *SDLRendererGraphicsFactory::GetFilterInfo(size_t index) const
{
    switch (index)
    {
    case 0:
        return &SDLRendererGfxFilter::FilterInfo;
    default:
        return nullptr;
    }
}

String SDLRendererGraphicsFactory::GetDefaultFilterID() const
{
    return SDLRendererGfxFilter::FilterInfo.Id;
}

/* static */ SDLRendererGraphicsFactory *SDLRendererGraphicsFactory::GetFactory()
{
    if (!_factory)
        _factory = new SDLRendererGraphicsFactory();
    return _factory;
}

SDLRendererGraphicsDriver *SDLRendererGraphicsFactory::EnsureDriverCreated()
{
    if (!_driver)
        _driver = new SDLRendererGraphicsDriver();
    return _driver;
}

SDLRendererGfxFilter *SDLRendererGraphicsFactory::CreateFilter(const String &id)
{
    if (SDLRendererGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new SDLRendererGfxFilter();
    return nullptr;
}

} // namespace ALSW
} // namespace Engine
} // namespace AGS

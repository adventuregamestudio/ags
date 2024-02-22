//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gfx/gfxdriverbase.h"
#include "debug/out.h"
#include "gfx/ali3dexception.h"
#include "gfx/bitmap.h"
#include "gfx/gfxfilter.h"
#include "gfx/gfx_util.h"

using namespace AGS::Common;

namespace AGS
{
namespace Engine
{

GraphicsDriverBase::GraphicsDriverBase()
    : _spriteEvtCallback(nullptr)
    , _initGfxCallback(nullptr)
{
    _actSpriteBatch = UINT32_MAX;
    _rendSpriteBatch = UINT32_MAX;
}

bool GraphicsDriverBase::IsModeSet() const
{
    return _mode.Width != 0 && _mode.Height != 0 && _mode.ColorDepth != 0;
}

bool GraphicsDriverBase::IsNativeSizeValid() const
{
    return !_srcRect.IsEmpty();
}

bool GraphicsDriverBase::IsRenderFrameValid() const
{
    return !_srcRect.IsEmpty() && !_dstRect.IsEmpty();
}

DisplayMode GraphicsDriverBase::GetDisplayMode() const
{
    return _mode;
}

Size GraphicsDriverBase::GetNativeSize() const
{
    return _srcRect.GetSize();
}

Rect GraphicsDriverBase::GetRenderDestination() const
{
    return _dstRect;
}

bool GraphicsDriverBase::SetVsync(bool enabled)
{
    if (!_capsVsync || (_mode.Vsync == enabled))
    {
        return _mode.Vsync;
    }

    bool new_value;
    if (SetVsyncImpl(enabled, new_value) && new_value == enabled)
    {
        Debug::Printf("SetVsync: switched to %d", new_value);
        _mode.Vsync = new_value;
    }
    else
    {
        Debug::Printf("SetVsync: failed, stay at %d", _mode.Vsync);
        _capsVsync = false; // mark as non-capable (at least in current mode)
    }
    return _mode.Vsync;
}

bool GraphicsDriverBase::GetVsync() const
{
    return _mode.Vsync;
}

void GraphicsDriverBase::BeginSpriteBatch(const Rect &viewport, const SpriteTransform &transform,
    GraphicFlip flip, PBitmap surface)
{
    BeginSpriteBatch(SpriteBatchDesc(_actSpriteBatch, viewport, transform, flip, surface));
}

void GraphicsDriverBase::BeginSpriteBatch(IDriverDependantBitmap *render_target,
    const Rect &viewport, const SpriteTransform &transform,
    GraphicFlip flip)
{
    BeginSpriteBatch(SpriteBatchDesc(_actSpriteBatch, render_target, viewport, transform, flip));
}

void GraphicsDriverBase::BeginSpriteBatch(const SpriteBatchDesc &desc)
{
    _spriteBatchDesc.push_back(desc);
    _spriteBatchRange.push_back(std::make_pair(GetLastDrawEntryIndex(), SIZE_MAX));
    _actSpriteBatch = _spriteBatchDesc.size() - 1;
    InitSpriteBatch(_actSpriteBatch, _spriteBatchDesc[_actSpriteBatch]);
}

void GraphicsDriverBase::EndSpriteBatch()
{
    assert(_actSpriteBatch != UINT32_MAX);
    if (_actSpriteBatch == UINT32_MAX)
        return;
    _spriteBatchRange[_actSpriteBatch].second = GetLastDrawEntryIndex();
    _actSpriteBatch = _spriteBatchDesc[_actSpriteBatch].Parent;
}

void GraphicsDriverBase::ClearDrawLists()
{
    ResetAllBatches();
    _actSpriteBatch = UINT32_MAX;
    _spriteBatchDesc.clear();
    _spriteBatchRange.clear();
}

void GraphicsDriverBase::OnInit()
{
}

void GraphicsDriverBase::OnUnInit()
{
}

void GraphicsDriverBase::OnModeSet(const DisplayMode &mode)
{
    _mode = mode;
    // Adjust some generic parameters as necessary
    _mode.Vsync &= _capsVsync;
}

void GraphicsDriverBase::OnModeReleased()
{
    _mode = DisplayMode();
    _dstRect = Rect();
}

void GraphicsDriverBase::OnScalingChanged()
{
    PGfxFilter filter = GetGraphicsFilter();
    if (filter)
        _filterRect = filter->SetTranslation(_srcRect.GetSize(), _dstRect);
    else
        _filterRect = Rect();
    _scaling.Init(_srcRect.GetSize(), _dstRect);
}

void GraphicsDriverBase::OnSetNativeRes(const GraphicResolution &native_res)
{
    _srcRect = RectWH(0, 0, native_res.Width, native_res.Height);
    _srcColorDepth = native_res.ColorDepth;
    OnScalingChanged();
}

void GraphicsDriverBase::OnSetRenderFrame(const Rect &dst_rect)
{
    _dstRect = dst_rect;
    OnScalingChanged();
}

void GraphicsDriverBase::OnSetFilter()
{
    _filterRect = GetGraphicsFilter()->SetTranslation(Size(_srcRect.GetSize()), _dstRect);
}


VideoMemoryGraphicsDriver::VideoMemoryGraphicsDriver()
    : _stageScreenDirty(false)
    , _fxIndex(0)
{
    // Only to have something meaningful as default
    _vmem_a_shift_32 = 24;
    _vmem_r_shift_32 = 16;
    _vmem_g_shift_32 = 8;
    _vmem_b_shift_32 = 0;
}

VideoMemoryGraphicsDriver::~VideoMemoryGraphicsDriver()
{
    DestroyAllStageScreens();
}

Bitmap *VideoMemoryGraphicsDriver::GetMemoryBackBuffer()
{
    return nullptr;
}

void VideoMemoryGraphicsDriver::SetMemoryBackBuffer(Bitmap* /*backBuffer*/)
{ // do nothing, video-memory drivers don't use main back buffer, only stage bitmaps they pass to plugins
}

Bitmap *VideoMemoryGraphicsDriver::GetStageBackBuffer(bool mark_dirty)
{
    if (_rendSpriteBatch == UINT32_MAX)
        return nullptr;
    _stageScreenDirty |= mark_dirty;
    return GetStageScreenRaw(_rendSpriteBatch);
}

void VideoMemoryGraphicsDriver::SetStageBackBuffer(Bitmap *backBuffer)
{ // do nothing, video-memory drivers don't support this
}

bool VideoMemoryGraphicsDriver::GetStageMatrixes(RenderMatrixes &rm)
{
    rm = _stageMatrixes;
    return true;
}

IDriverDependantBitmap *VideoMemoryGraphicsDriver::CreateDDBFromBitmap(const Bitmap *bitmap, bool has_alpha, bool opaque)
{
    IDriverDependantBitmap *ddb = CreateDDB(bitmap->GetWidth(), bitmap->GetHeight(), bitmap->GetColorDepth(), opaque);
    if (ddb)
        UpdateDDBFromBitmap(ddb, bitmap, has_alpha);
    return ddb;
}

Texture *VideoMemoryGraphicsDriver::CreateTexture(const Bitmap *bmp, bool has_alpha, bool opaque)
{
    Texture *txdata = CreateTexture(bmp->GetWidth(), bmp->GetHeight(), bmp->GetColorDepth(), opaque);
    if (txdata)
        UpdateTexture(txdata, bmp, has_alpha, opaque);
    return txdata;
}

void VideoMemoryGraphicsDriver::SetStageScreen(const Size &sz, int x, int y)
{
    SetStageScreen(_actSpriteBatch, sz, x, y);
}

void VideoMemoryGraphicsDriver::SetStageScreen(size_t index, const Size &sz, int x, int y)
{
    if (_stageScreens.size() <= index)
        _stageScreens.resize(index + 1);
    _stageScreens[index].Position = RectWH(x, y, sz.Width, sz.Height);
}

Bitmap *VideoMemoryGraphicsDriver::GetStageScreenRaw(size_t index)
{
    assert(index < _stageScreens.size());
    if (_stageScreens.size() <= index)
        return nullptr;

    auto &scr = _stageScreens[index];
    const Size sz = scr.Position.GetSize();
    if (scr.Raw && (scr.Raw->GetSize() != sz))
    {
        scr.Raw.reset();
        if (scr.DDB)
            DestroyDDB(scr.DDB);
        scr.DDB = nullptr;
    }
    if (!scr.Raw && !sz.IsNull())
    {
        scr.Raw.reset(new Bitmap(sz.Width, sz.Height, _mode.ColorDepth));
        scr.DDB = CreateDDB(sz.Width, sz.Height, _mode.ColorDepth, false);
    }
    return scr.Raw.get();
}

IDriverDependantBitmap *VideoMemoryGraphicsDriver::UpdateStageScreenDDB(size_t index,
    int &x, int &y)
{
    assert((index < _stageScreens.size()) && _stageScreens[index].DDB);
    if ((_stageScreens.size() <= index) || !_stageScreens[index].Raw || !_stageScreens[index].DDB)
        return nullptr;

    auto &scr = _stageScreens[index];
    UpdateDDBFromBitmap(scr.DDB, scr.Raw.get(), true);
    scr.Raw->ClearTransparent();
    x = scr.Position.Left;
    y = scr.Position.Top;
    return scr.DDB;
}

void VideoMemoryGraphicsDriver::DestroyAllStageScreens()
{
    for (size_t i = 0; i < _stageScreens.size(); ++i)
    {
        if (_stageScreens[i].DDB)
            DestroyDDB(_stageScreens[i].DDB);
    }
    _stageScreens.clear();
}

IDriverDependantBitmap *VideoMemoryGraphicsDriver::DoSpriteEvtCallback(int evt, int data,
    int &x, int &y)
{
    if (!_spriteEvtCallback)
        throw Ali3DException("Unhandled attempt to draw null sprite");
    _stageScreenDirty = false;
    // NOTE: this is not clear whether return value of callback may be
    // relied on. Existing plugins do not seem to return anything but 0,
    // even if they handle this event. This is why we also set
    // _stageScreenDirty in certain plugin API function implementations.
    _stageScreenDirty |= _spriteEvtCallback(evt, data) != 0;
    if (_stageScreenDirty)
    {
        return UpdateStageScreenDDB(_rendSpriteBatch, x, y);
    }
    return nullptr;
}

IDriverDependantBitmap *VideoMemoryGraphicsDriver::MakeFx(int r, int g, int b)
{
    if (_fxIndex == _fxPool.size()) _fxPool.push_back(ScreenFx());
    ScreenFx &fx = _fxPool[_fxIndex];
    if (fx.DDB == nullptr)
    {
        fx.Raw.reset(new Bitmap(16, 16, _mode.ColorDepth));
        fx.DDB = CreateDDBFromBitmap(fx.Raw.get(), false, true);
    }
    if (r != fx.Red || g != fx.Green || b != fx.Blue)
    {
        fx.Raw->Clear(makecol_depth(fx.Raw->GetColorDepth(), r, g, b));
        this->UpdateDDBFromBitmap(fx.DDB, fx.Raw.get(), false);
        fx.Red = r;
        fx.Green = g;
        fx.Blue = b;
    }
    _fxIndex++;
    return fx.DDB;
}

void VideoMemoryGraphicsDriver::ResetFxPool()
{
    _fxIndex = 0;
}

void VideoMemoryGraphicsDriver::DestroyFxPool()
{
    for (auto &fx : _fxPool)
    {
        if (fx.DDB)
            DestroyDDB(fx.DDB);
    }
    _fxPool.clear();
    _fxIndex = 0;
}


template <typename T> T algetr(const T);
template <typename T> T algetg(const T);
template <typename T> T algetb(const T);
template <typename T> T algeta(const T);

template <> uint8_t algetr(const uint8_t c) { return getr8(c); }
template <> uint8_t algetg(const uint8_t c) { return getg8(c); }
template <> uint8_t algetb(const uint8_t c) { return getb8(c); }
template <> uint8_t algeta(const uint8_t c) { return 0xFF; }

template <> uint16_t algetr(const uint16_t c) { return getr16(c); }
template <> uint16_t algetg(const uint16_t c) { return getg16(c); }
template <> uint16_t algetb(const uint16_t c) { return getb16(c); }
template <> uint16_t algeta(const uint16_t c) { return 0xFF; }

template <> uint32_t algetr(const uint32_t c) { return getr32(c); }
template <> uint32_t algetg(const uint32_t c) { return getg32(c); }
template <> uint32_t algetb(const uint32_t c) { return getb32(c); }
template <> uint32_t algeta(const uint32_t c) { return geta32(c); }

template <typename T> bool is_color_mask(const T);
template <> bool is_color_mask(const uint8_t c) { return c == MASK_COLOR_8;}
template <> bool is_color_mask(const uint16_t c) { return c == MASK_COLOR_16;}
template <> bool is_color_mask(const uint32_t c) { return c == MASK_COLOR_32;}

template <typename T> void get_pixel_if_not_transparent(const T *pixel, T *red, T *green, T *blue, T *divisor) {
    const T px_color = pixel[0];
    if (!is_color_mask<T>(px_color)) {
        *red += algetr<T>(px_color);
        *green += algetg<T>(px_color);
        *blue += algetb<T>(px_color);
        divisor[0]++;
    }
}


#define VMEMCOLOR_RGBA(r,g,b,a) \
    ( (((a) & 0xFF) << _vmem_a_shift_32) | (((r) & 0xFF) << _vmem_r_shift_32) | (((g) & 0xFF) << _vmem_g_shift_32) | (((b) & 0xFF) << _vmem_b_shift_32) )


// Template helper function which converts bitmap to a video memory buffer,
// applies transparency and optionally copyies the source alpha channel (if available).
template <typename T, bool HasAlpha> void
VideoMemoryGraphicsDriver::BitmapToVideoMemImpl(
        const Bitmap *bitmap, const TextureTile *tile,
        uint8_t *dst_ptr, const int dst_pitch)
{
    // tell the compiler these won't change mid loop execution
    const int t_width = tile->width;
    const int t_height = tile->height;
    const int t_x = tile->x;
    const int t_y = tile->y;

    const int idst_pitch = dst_pitch * sizeof(uint8_t) / sizeof(uint32_t); // destination is always 32-bit
    auto idst = reinterpret_cast<uint32_t*>(dst_ptr);

    for (int y = 0; y < t_height; y++)
    {
        const uint8_t *scanline_at = bitmap->GetScanLine(y + t_y);
        for (int x = 0; x < t_width; x++)
        {
            auto srcData = (const T *) &scanline_at[(x + t_x) * sizeof(T)];
            const T src_color = srcData[0];
            if (HasAlpha)
            {
                idst[x] = VMEMCOLOR_RGBA(algetr<T>(src_color), algetg<T>(src_color), algetb<T>(src_color),
                    algeta<T>(src_color));
            }
            else if (is_color_mask<T>(src_color))
            {
                idst[x] = 0;
            }
            else
            {
                idst[x] = VMEMCOLOR_RGBA(algetr<T>(src_color), algetg<T>(src_color), algetb<T>(src_color),
                    0xFF);
            }
        }
        idst += idst_pitch;
    }
}

// Template helper function which converts bitmap to a video memory buffer,
// assuming that the destination is always opaque (alpha channel is filled with 0xFF)
template <typename T> void
VideoMemoryGraphicsDriver::BitmapToVideoMemOpaqueImpl(
        const Bitmap *bitmap, const TextureTile *tile,
        uint8_t *dst_ptr, const int dst_pitch)
{
    // tell the compiler these won't change mid loop execution
    const int t_width = tile->width;
    const int t_height = tile->height;
    const int t_x = tile->x;
    const int t_y = tile->y;

    const int idst_pitch = dst_pitch * sizeof(uint8_t) / sizeof(uint32_t); // destination is always 32-bit
    auto idst = reinterpret_cast<uint32_t*>(dst_ptr);

    for (int y = 0; y < t_height; y++)
    {
        const uint8_t* scanline_at = bitmap->GetScanLine(y + t_y);
        for (int x = 0; x < t_width; x++)
        {
            auto srcData = (const T *)&scanline_at[(x + t_x) * sizeof(T)];
            const T src_color = srcData[0];
            idst[x] = VMEMCOLOR_RGBA(algetr<T>(src_color), algetg<T>(src_color), algetb<T>(src_color), 0xFF);
        }
        idst += idst_pitch;
    }
}

// Template helper function which converts bitmap to a video memory buffer
// with a semi-transparent pixels fix for "Linear" graphics filter which prevents
// colored outline (usually either of black or "magic pink" color).
template <typename T, bool HasAlpha> void
VideoMemoryGraphicsDriver::BitmapToVideoMemLinearImpl(
    const Bitmap *bitmap, const TextureTile *tile,
    uint8_t *dst_ptr, const int dst_pitch)
{
    // tell the compiler these won't change mid loop execution
    const int t_width = tile->width;
    const int t_height = tile->height;
    const int t_x = tile->x;
    const int t_y = tile->y;

    const int src_bpp = sizeof(T);
    const int idst_pitch = dst_pitch * sizeof(uint8_t) / sizeof(uint32_t); // destination is always 32-bit
    auto idst = reinterpret_cast<uint32_t*>(dst_ptr);
    bool lastPixelWasTransparent = false;

    for (int y = 0; y < t_height; y++) {
        lastPixelWasTransparent = false;
        const uint8_t *scanline_before = (y > 0) ? bitmap->GetScanLine(y + t_y - 1) : nullptr;
        const uint8_t *scanline_at = bitmap->GetScanLine(y + t_y);
        const uint8_t *scanline_after = (y < t_height - 1) ? bitmap->GetScanLine(y + t_y + 1) : nullptr;

        for (int x = 0; x < t_width; x++)
        {
            auto srcData = (const T *) &scanline_at[(x + t_x) * src_bpp];
            const T src_color = srcData[0];

            if (is_color_mask<T>(src_color))
            {
                // set to transparent, but use the colour from the neighbouring
                // pixel to stop the linear filter doing colored outlines
                T red = 0, green = 0, blue = 0, divisor = 0;
                if (x > 0)
                    get_pixel_if_not_transparent<T>(&srcData[-1], &red, &green, &blue, &divisor);
                if (x < t_width - 1)
                    get_pixel_if_not_transparent<T>(&srcData[1], &red, &green, &blue, &divisor);
                if (y > 0)
                    get_pixel_if_not_transparent<T>(
                            (const T *) &scanline_before[(x + t_x) * src_bpp], &red, &green,
                            &blue, &divisor);
                if (y < t_height - 1)
                    get_pixel_if_not_transparent<T>(
                            (const T *) &scanline_after[(x + t_x) * src_bpp], &red, &green,
                            &blue, &divisor);
                if (divisor > 0)
                    idst[x] = VMEMCOLOR_RGBA(red / divisor, green / divisor, blue / divisor, 0);
                else
                    idst[x] = 0;
                lastPixelWasTransparent = true;
            }
            else if (HasAlpha)
            {
                idst[x] = VMEMCOLOR_RGBA(algetr<T>(src_color), algetg<T>(src_color), algetb<T>(src_color),
                                         algeta<T>(src_color));
            }
            else
            {
                idst[x] = VMEMCOLOR_RGBA(algetr<T>(src_color), algetg<T>(src_color), algetb<T>(src_color), 0xFF);
                if (lastPixelWasTransparent) {
                    // update the colour of the previous transparent pixel, to
                    // stop colored outlines when linear filtering
                    idst[x - 1] = idst[x] & 0x00FFFFFF;
                    lastPixelWasTransparent = false;
                }
            }
        }
        idst += idst_pitch;
    }
}

void VideoMemoryGraphicsDriver::BitmapToVideoMem(const Bitmap *bitmap, const bool has_alpha, const TextureTile *tile,
    uint8_t *dst_ptr, const int dst_pitch, const bool usingLinearFiltering)
{
    switch (bitmap->GetColorDepth())
    {
        case 8:
            if (usingLinearFiltering) {
                BitmapToVideoMemLinearImpl<uint8_t, false>(bitmap, tile, dst_ptr, dst_pitch);
            } else {
                BitmapToVideoMemImpl<uint8_t, false>(bitmap, tile, dst_ptr, dst_pitch);
            }

            break;
        case 16:
            if (usingLinearFiltering) {
                BitmapToVideoMemLinearImpl<uint16_t, false>(bitmap, tile, dst_ptr, dst_pitch);
            } else {
                BitmapToVideoMemImpl<uint16_t, false>(bitmap, tile, dst_ptr, dst_pitch);
            }
            break;
        case 32:
            if (usingLinearFiltering) {
                if (has_alpha) {
                    BitmapToVideoMemLinearImpl<uint32_t, true>(bitmap, tile, dst_ptr, dst_pitch);
                } else {
                    BitmapToVideoMemLinearImpl<uint32_t, false>(bitmap, tile, dst_ptr, dst_pitch);
                }
            } else {
                if (has_alpha) {
                    BitmapToVideoMemImpl<uint32_t, true>(bitmap, tile, dst_ptr, dst_pitch);
                } else {
                    BitmapToVideoMemImpl<uint32_t, false>(bitmap, tile, dst_ptr, dst_pitch);
                }
            }
            break;
        default:
            break;
    }
}

void VideoMemoryGraphicsDriver::BitmapToVideoMemOpaque(const Bitmap *bitmap, const TextureTile *tile,
    uint8_t *dst_ptr, const int dst_pitch)
{
    switch (bitmap->GetColorDepth())
    {
        case 8:
            BitmapToVideoMemOpaqueImpl<uint8_t>(bitmap, tile, dst_ptr, dst_pitch);
            break;
        case 16:
            BitmapToVideoMemOpaqueImpl<uint16_t>(bitmap, tile, dst_ptr, dst_pitch);
            break;
        case 32:
            BitmapToVideoMemOpaqueImpl<uint32_t>(bitmap, tile, dst_ptr, dst_pitch);
            break;
        default:
            break;
    }
}


} // namespace Engine
} // namespace AGS

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
//
// Direct3D graphics factory
//
//=============================================================================
#ifndef __AGS_EE_GFX__ALI3DD3D_H
#define __AGS_EE_GFX__ALI3DD3D_H

#include "core/platform.h"

#if ! AGS_PLATFORM_OS_WINDOWS
#error This file should only be included on the Windows build
#endif

#define NOMINMAX
#include <memory>
#define NOMINMAX
#define BITMAP WINDOWS_BITMAP
#include <d3d9.h>
#undef BITMAP
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/gfxdriverbase.h"
#include "util/library.h"
#include "util/math.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{
namespace D3D
{

using AGS::Common::Bitmap;
using AGS::Common::String;
class D3DGfxFilter;

struct D3DTextureTile : public TextureTile
{
    IDirect3DTexture9 *texture = nullptr;
};

// Full Direct3D texture data
struct D3DTexture : Texture
{
    IDirect3DVertexBuffer9 *_vertex = nullptr;
    D3DTextureTile *_tiles = nullptr;
    size_t _numTiles = 0;

    D3DTexture(const GraphicResolution &res, bool rt)
        : Texture(res, rt) {}
    D3DTexture(uint32_t id, const GraphicResolution &res, bool rt)
        : Texture(id, res, rt) {}
    ~D3DTexture();
    size_t GetMemSize() const override;
};

class D3DBitmap : public BaseDDB
{
public:
    uint32_t GetRefID() const override { return _data->ID; }

    int  GetAlpha() const override { return _alpha; }
    void SetAlpha(int alpha) override { _alpha = alpha; }
    void SetFlippedLeftRight(bool isFlipped) override { _flipped = isFlipped; }
    void SetStretch(int width, int height, bool useResampler = true) override
    {
        _stretchToWidth = width;
        _stretchToHeight = height;
        _useResampler = useResampler;
    }
    int GetWidthToRender() { return _stretchToWidth; }
    int GetHeightToRender() { return _stretchToHeight; }
    // Rotation is set in degrees, clockwise
    void SetRotation(float degrees) override { _rotation = -Common::Math::DegreesToRadians(degrees); }
    void SetLightLevel(int lightLevel) override { _lightLevel = lightLevel; }
    void SetTint(int red, int green, int blue, int tintSaturation) override
    {
        _red = red;
        _green = green;
        _blue = blue;
        _tintSaturation = tintSaturation;
    }
    void SetBlendMode(Common::BlendMode blendMode) override  { _blendMode = blendMode; }

    // Tells if this DDB has an actual render data assigned to it.
    bool IsValid() override { return _data != nullptr; }
    // Attaches new texture data, sets basic render rules
    void AttachData(std::shared_ptr<Texture> txdata, bool opaque) override
    {
        _data = std::static_pointer_cast<D3DTexture>(txdata);
        _width = _stretchToWidth = _data->Res.Width;
        _height = _stretchToHeight = _data->Res.Height;
        _colDepth = _data->Res.ColorDepth;
        _opaque = opaque;
    }
    // Detach any internal texture data from this DDB, make this an empty object
    void DetachData() override
    {
        _data = nullptr;
    }

    // Direct3D texture data
    std::shared_ptr<D3DTexture> _data;
    // Optional surface for rendering onto a texture
    IDirect3DSurface9 *_renderSurface {};
    TextureHint _renderHint = kTxHint_Normal;

    // Drawing parameters
    bool _flipped;
    int _stretchToWidth, _stretchToHeight;
    float _rotation;
    bool _useResampler;
    int _red, _green, _blue;
    int _tintSaturation;
    int _lightLevel;
    int _alpha;
    Common::BlendMode _blendMode;

    D3DBitmap(int width, int height, int colDepth, bool opaque)
    {
        _width = width;
        _height = height;
        _colDepth = colDepth;
        _flipped = false;
        _stretchToWidth = width;
        _stretchToHeight = height;
        _originX = _originY = 0.f;
        _useResampler = false;
        _rotation = 0;
        _red = _green = _blue = 0;
        _tintSaturation = 0;
        _lightLevel = 0;
        _alpha = 255;
        _opaque = opaque;
        _blendMode = Common::kBlend_Normal;
    }

    // Releases internal texture data only, keeping the base struct
    void ReleaseTextureData();

    ~D3DBitmap() override;
};

class D3DGfxModeList : public IGfxModeList
{
public:
    D3DGfxModeList(IDirect3D9 *direct3d, D3DFORMAT d3dformat)
        : _direct3d(direct3d)
        , _pixelFormat(d3dformat)
    {
        _direct3d->AddRef();
        _modeCount = _direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, _pixelFormat);
    }

    ~D3DGfxModeList() override
    {
        if (_direct3d)
            _direct3d->Release();
    }

    int GetModeCount() const override
    {
        return _modeCount;
    }

    bool GetMode(int index, DisplayMode &mode) const override;

private:
    IDirect3D9 *_direct3d = nullptr;
    D3DFORMAT   _pixelFormat;
    int         _modeCount = 0;
};

struct CUSTOMVERTEX
{
    D3DVECTOR   position; // The position.
    D3DVECTOR   normal;
    FLOAT       tu, tv;   // The texture coordinates.
};

// D3D renderer's sprite batch
struct D3DSpriteBatch : VMSpriteBatch
{
    // Add anything D3D specific here
    // Optional render target's surface
    // FIXME: implement a C++ (template) wrapper around IUnknown, handle AddRef/Release auto!!
    IDirect3DSurface9 *RenderSurface = nullptr;

    D3DSpriteBatch() = default;
    D3DSpriteBatch(uint32_t id, const Rect &view, const glm::mat4 &matrix,
        const glm::mat4 &vp_matrix, const SpriteColorTransform &color)
        : VMSpriteBatch(id, view, matrix, vp_matrix, color) {}
    D3DSpriteBatch(uint32_t id, D3DBitmap *render_target, const Rect &view,
        const glm::mat4 &matrix, const glm::mat4 &vp_matrix,
        const SpriteColorTransform &color)
        : VMSpriteBatch(id, render_target, view, matrix, vp_matrix, color)
        , RenderSurface(render_target ? render_target->_renderSurface : nullptr) {}
};

typedef SpriteDrawListEntry<D3DBitmap> D3DDrawListEntry;
typedef std::vector<D3DSpriteBatch>    D3DSpriteBatches;


class D3DGraphicsDriver : public VideoMemoryGraphicsDriver
{
public:
    const char *GetDriverID() override { return "D3D9"; }
    const char *GetDriverName() override { return "Direct3D 9"; }

    bool ShouldReleaseRenderTargets() override { return true; }

    void SetTintMethod(TintMethod method) override;
    bool SetDisplayMode(const DisplayMode &mode) override;
    void UpdateDeviceScreen(const Size &screen_sz) override;
    bool SetNativeResolution(const GraphicResolution &native_res) override;
    bool SetRenderFrame(const Rect &dst_rect) override;
    int  GetDisplayDepthForNativeDepth(int native_color_depth) const override;
    IGfxModeList *GetSupportedModeList(int color_depth) override;
    bool IsModeSupported(const DisplayMode &mode) override;
    PGfxFilter GetGraphicsFilter() const override;
    // Clears the screen rectangle. The coordinates are expected in the **native game resolution**.
    void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) override;
    int  GetCompatibleBitmapFormat(int color_depth) override;
    size_t GetAvailableTextureMemory() override;

    IDriverDependantBitmap* CreateDDB(int width, int height, int color_depth, bool opaque) override;
    IDriverDependantBitmap* CreateRenderTargetDDB(int width, int height, int color_depth, bool opaque) override;
    void UpdateDDBFromBitmap(IDriverDependantBitmap* ddb, Bitmap *bitmap) override;
    void DestroyDDB(IDriverDependantBitmap* ddb) override;

    // Create texture data with the given parameters
    Texture *CreateTexture(int width, int height, int color_depth, bool opaque = false, bool as_render_target = false) override;
    // Update texture data from the given bitmap
    void UpdateTexture(Texture *txdata, Bitmap *bitmap, bool opaque) override;
    // Retrieve shared texture data object from the given DDB
    std::shared_ptr<Texture> GetTexture(IDriverDependantBitmap *ddb) override;

    void DrawSprite(int x, int y, IDriverDependantBitmap* ddb) override
         { DrawSprite(x, y, x, y, ddb); }
    void DrawSprite(int ox, int oy, int ltx, int lty, IDriverDependantBitmap* bitmap) override;
    void SetScreenFade(int red, int green, int blue) override;
    void SetScreenTint(int red, int green, int blue) override;
    void RenderToBackBuffer() override;
    void Render() override;
    void Render(int xoff, int yoff, Common::GraphicFlip flip) override;
    bool GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt) override;
    bool DoesSupportVsyncToggle() override { return _capsVsync; }
    void RenderSpritesAtScreenResolution(bool enabled, int /*supersampling*/) override { _renderSprAtScreenRes = enabled; };
    void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) override;
    void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue) override;
    void BoxOutEffect(bool blackingOut, int speed, int delay) override;
    bool SupportsGammaControl() override;
    void SetGamma(int newGamma) override;
    void UseSmoothScaling(bool enabled) override { _smoothScaling = enabled; }

    typedef std::shared_ptr<D3DGfxFilter> PD3DFilter;

    // Clears screen rect, coordinates are expected in display resolution
    void ClearScreenRect(const Rect &r, RGB *colorToUse);
    void UnInit();
    void SetGraphicsFilter(PD3DFilter filter);

    D3DGraphicsDriver(IDirect3D9 *d3d);
    ~D3DGraphicsDriver() override;

protected:
    bool SetVsyncImpl(bool vsync, bool &vsync_res) override;

    // Create DDB using preexisting texture
    IDriverDependantBitmap *CreateDDB(std::shared_ptr<Texture> txdata, bool opaque) override;

    size_t GetLastDrawEntryIndex() override { return _spriteList.size(); }

private:
    PD3DFilter _filter;

    IDirect3D9 *direct3d;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9* direct3ddevice;
    D3DGAMMARAMP defaultgammaramp;
    D3DGAMMARAMP currentgammaramp;
    D3DCAPS9 direct3ddevicecaps;
    IDirect3DVertexBuffer9* vertexbuffer;
    IDirect3DSurface9 *pNativeSurface;
    IDirect3DTexture9 *pNativeTexture;
    RECT viewport_rect;
    CUSTOMVERTEX defaultVertices[4];
    String previousError;
    IDirect3DPixelShader9* pixelShader;
    bool _smoothScaling;
    bool _legacyPixelShader;
    float _pixelRenderXOffset;
    float _pixelRenderYOffset;
    bool _renderSprAtScreenRes;

    // Render target DDB references, for keeping track of them,
    // and resetting during device reset.
    std::vector<D3DBitmap*> _renderTargets;
    // Sprite batches (parent scene nodes)
    D3DSpriteBatches _spriteBatches;
    // List of sprites to render
    std::vector<D3DDrawListEntry> _spriteList;
    // TODO: these draw list backups are needed only for the fade-in/out effects
    // find out if it's possible to reimplement these effects in main drawing routine.
    // TODO: if not above, refactor and implement Desc backup in the base class
    SpriteBatchDescs _backupBatchDescs;
    std::vector<std::pair<size_t, size_t>> _backupBatchRange;
    D3DSpriteBatches _backupBatches;
    std::vector<D3DDrawListEntry> _backupSpriteList;

    // Called after new mode was successfully initialized
    void OnModeSet(const DisplayMode &mode) override;
    void InitSpriteBatch(size_t index, const SpriteBatchDesc &desc) override;
    void ResetAllBatches() override;
    bool CreateDisplayMode(const DisplayMode &mode);
    // Called when the direct3d device is created for the first time
    bool FirstTimeInit();
    void InitializeD3DState();
    // Resets
    void ResetDeviceIfNecessary();
    void SetupViewport();
    HRESULT ResetD3DDevice();
    // For tracked render targets, disposes only the internal texture data
    void ReleaseRenderTargetData();
    // For tracked render targets, recreates the internal texture data
    void RecreateRenderTargets();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();
    void set_up_default_vertices();
    void AdjustSizeToNearestSupportedByCard(int *width, int *height);
    void UpdateTextureRegion(D3DTextureTile *tile, Bitmap *bitmap, bool opaque);
    void CreateVirtualScreen();
    void do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    bool IsTextureFormatOk( D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat );
    // Backup all draw lists in the temp storage
    void BackupDrawLists();
    // Restore draw lists from the temp storage
    void RestoreDrawLists();
    // Deletes draw list backups
    void ClearDrawBackups();
    void _renderAndPresent(bool clearDrawListAfterwards);
    void _render(bool clearDrawListAfterwards);
    void _reDrawLastFrame();
    // Sets a Direct3D viewport for the current render target.
    void SetD3DViewport(const Rect &rc);
    // Sets the scissor (render clip), clip rect is passed in the "native" coordinates.
    // Optionally pass render_on_texture if the rendering is done to texture, in native coords,
    // otherwise we assume it is set on a whole screen, scaled to the screen coords.
    void SetScissor(const Rect &clip, bool render_on_texture = false);
    // Configures rendering mode for the render target, depending on its properties
    void SetRenderTarget(const D3DSpriteBatch *batch, IDirect3DSurface9 *back_buffer, Size &surface_sz, bool clear);
    void RenderSpriteBatches();
    size_t RenderSpriteBatch(const D3DSpriteBatch &batch, size_t from, const Size &surface_size);
    void _renderSprite(const D3DDrawListEntry *entry, const glm::mat4 &matGlobal,
        const SpriteColorTransform &color, const Size &surface_size);
    void _renderFromTexture();
    // Helper method for setting blending parameters
    void SetBlendOp(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor);
    // Helper method for setting exclusive alpha blending parameters
    void SetBlendOpAlpha(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor);
};


class D3DGraphicsFactory : public GfxDriverFactoryBase<D3DGraphicsDriver, D3DGfxFilter>
{
public:
    ~D3DGraphicsFactory() override;

    size_t               GetFilterCount() const override;
    const GfxFilterInfo *GetFilterInfo(size_t index) const override;
    String               GetDefaultFilterID() const override;

    static D3DGraphicsFactory   *GetFactory();
    static D3DGraphicsDriver    *GetD3DDriver();

private:
    D3DGraphicsFactory();

    D3DGraphicsDriver   *EnsureDriverCreated() override;
    D3DGfxFilter        *CreateFilter(const String &id) override;

    bool Init();

    static D3DGraphicsFactory *_factory;
    //
    // IMPORTANT NOTE: since the Direct3d9 device is created with
    // D3DCREATE_MULTITHREADED behavior flag, once it is created the d3d9.dll may
    // only be unloaded after window is destroyed, as noted in the MSDN's article
    // on "D3DCREATE"
    // (http://msdn.microsoft.com/en-us/library/windows/desktop/bb172527.aspx).
    // Otherwise window becomes either destroyed prematurely or broken (details
    // are unclear), which causes errors during Allegro deinitialization.
    //
    // Curiously, this problem was only confirmed under WinXP so far.
    //
    // For the purpose of avoiding this problem, we have a static library wrapper
    // that unloads library only at the very program exit (except cases of device
    // creation failure).
    //
    // TODO: find out if there is better solution.
    // 
    static Library      _library;
    IDirect3D9         *_direct3d;
};

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DD3D_H

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
#include "util/smart_ptr.h"
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

// Declare smart ptr for common IDirect3D interfaces
typedef ComPtr<IDirect3D9> D3DPtr;
typedef ComPtr<IDirect3DDevice9> D3DDevicePtr;
typedef ComPtr<IDirect3DSurface9> D3DSurfacePtr;
typedef ComPtr<IDirect3DTexture9> D3DTexturePtr;
typedef ComPtr<IDirect3DVertexBuffer9> D3DVertexBufferPtr;
typedef ComPtr<IDirect3DPixelShader9> D3DPixelShaderPtr;


struct D3DTextureTile : public TextureTile
{
    D3DTexturePtr texture;
};

// Full Direct3D texture data
struct D3DTexture : Texture
{
    D3DVertexBufferPtr _vertex;
    std::vector<D3DTextureTile> _tiles;

    D3DTexture(const GraphicResolution &res, bool rt)
        : Texture(res, rt) {}
    D3DTexture(uint32_t id, const GraphicResolution &res, bool rt)
        : Texture(id, res, rt) {}
    ~D3DTexture() = default;
    size_t GetMemSize() const override;
};

class D3DBitmap : public BaseDDB
{
public:
    uint32_t GetRefID() const override { return _data ? _data->ID : UINT32_MAX; }

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
    void SetLightLevel(int lightLevel) override { _lightLevel = lightLevel; }
    void SetTint(int red, int green, int blue, int tintSaturation) override
    {
        _red = red;
        _green = green;
        _blue = blue;
        _tintSaturation = tintSaturation;
    }

    // Tells if this DDB has an actual render data assigned to it.
    bool IsValid() override { return _data != nullptr; }
    // Attaches new texture data, sets basic render rules
    void AttachData(std::shared_ptr<Texture> txdata, bool opaque) override
    {
        assert(txdata);
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
    D3DSurfacePtr _renderSurface;
    TextureHint _renderHint = kTxHint_Normal;

    // Drawing parameters
    bool _flipped = false;
    int _stretchToWidth = 0, _stretchToHeight = 0;
    bool _useResampler = false;
    int _red = 0, _green = 0, _blue = 0;
    int _tintSaturation = 0;
    int _lightLevel = 0;
    int _alpha = 255;

    D3DBitmap() = default;

    D3DBitmap(int width, int height, int colDepth, bool opaque)
    {
        _width = width;
        _height = height;
        _colDepth = colDepth;
        _flipped = false;
        _hasAlpha = false;
        _stretchToWidth = _width;
        _stretchToHeight = _height;
        _useResampler = false;
        _red = _green = _blue = 0;
        _tintSaturation = 0;
        _lightLevel = 0;
        _alpha = 255;
        _opaque = opaque;
    }

    // Releases internal texture data only, keeping the base struct
    void ReleaseTextureData();

    ~D3DBitmap() override = default;
};

class D3DGfxModeList : public IGfxModeList
{
public:
    D3DGfxModeList(const D3DPtr &direct3d, int display_index, D3DFORMAT d3dformat);
    ~D3DGfxModeList() = default;

    int GetModeCount() const override
    {
        return _modeCount;
    }

    bool GetMode(int index, DisplayMode &mode) const override;

private:
    D3DPtr      _direct3d;
    int         _displayIndex = 0;
    UINT        _adapterIndex = 0u;
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
    D3DSurfacePtr RenderSurface;

    D3DSpriteBatch() = default;
    D3DSpriteBatch(uint32_t id, const Rect &view, const glm::mat4 &matrix,
        const glm::mat4 &vp_matrix, const SpriteColorTransform &color)
        : VMSpriteBatch(id, view, matrix, vp_matrix, color) {}
    D3DSpriteBatch(uint32_t id, D3DBitmap *render_target, const Rect &view,
        const glm::mat4 &matrix, const glm::mat4 &vp_matrix,
        const SpriteColorTransform &color)
        : VMSpriteBatch(id, render_target, view, matrix, vp_matrix, color)
    {
        if (render_target)
            RenderSurface = render_target->_renderSurface;
    }
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
    IGfxModeList *GetSupportedModeList(int display_index, int color_depth) override;
    bool IsModeSupported(const DisplayMode &mode) override;
    PGfxFilter GetGraphicsFilter() const override;
    // Clears the screen rectangle. The coordinates are expected in the **native game resolution**.
    void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) override;
    int  GetCompatibleBitmapFormat(int color_depth) override;
    // Returns available texture memory in bytes, or 0 if this query is not supported
    uint64_t GetAvailableTextureMemory() override;

    IDriverDependantBitmap *CreateDDB() override;
    IDriverDependantBitmap* CreateDDB(int width, int height, int color_depth, bool opaque) override;
    IDriverDependantBitmap* CreateRenderTargetDDB(int width, int height, int color_depth, bool opaque) override;
    void UpdateDDBFromBitmap(IDriverDependantBitmap* ddb, const Bitmap *bitmap, bool has_alpha) override;
    void DestroyDDB(IDriverDependantBitmap* ddb) override;

    // Create texture data with the given parameters
    Texture *CreateTexture(int width, int height, int color_depth, bool opaque = false, bool as_render_target = false) override;
    // Update texture data from the given bitmap
    void UpdateTexture(Texture *txdata, const Bitmap *bitmap, bool has_alpha, bool opaque) override;
    // Retrieve shared texture data object from the given DDB
    std::shared_ptr<Texture> GetTexture(IDriverDependantBitmap *ddb) override;

    void DrawSprite(int x, int y, IDriverDependantBitmap* ddb) override;
    void AddRenderEvent(int evt, int param) override;
    void SetScreenFade(int red, int green, int blue) override;
    void SetScreenTint(int red, int green, int blue) override;
    // Redraw saved draw lists, optionally filtering specific batches
    void RedrawLastFrame(uint32_t batch_skip_filter) override;

    void RenderToBackBuffer() override;
    void Render() override;
    void Render(int xoff, int yoff, Common::GraphicFlip flip) override;
    void Render(IDriverDependantBitmap *target) override;
    void GetCopyOfScreenIntoDDB(IDriverDependantBitmap *target, uint32_t batch_skip_filter = 0u) override;
    bool GetCopyOfScreenIntoBitmap(Bitmap *destination, const Rect *src_rect, bool at_native_res,
        GraphicResolution *want_fmt, uint32_t batch_skip_filter = 0u) override;
    bool DoesSupportVsyncToggle() override { return _capsVsync; }
    void RenderSpritesAtScreenResolution(bool enabled) override { _renderAtScreenRes = enabled; };
    bool SupportsGammaControl() override;
    void SetGamma(int newGamma) override;
    void UseSmoothScaling(bool enabled) override { _smoothScaling = enabled; }

    typedef std::shared_ptr<D3DGfxFilter> PD3DFilter;

    // Clears screen rect, coordinates are expected in display resolution
    void ClearScreenRect(const Rect &r, RGB *colorToUse);
    void UnInit();
    void SetGraphicsFilter(PD3DFilter filter);

    D3DGraphicsDriver(const D3DPtr &d3d);
    ~D3DGraphicsDriver() override;

protected:
    bool SetVsyncImpl(bool vsync, bool &vsync_res) override;

    // Create DDB using preexisting texture
    IDriverDependantBitmap *CreateDDB(std::shared_ptr<Texture> txdata, bool opaque) override;

    size_t GetLastDrawEntryIndex() override { return _spriteList.size(); }

private:
    PD3DFilter _filter;

    D3DPtr direct3d;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DDevicePtr direct3ddevice;
    D3DDEVICE_CREATION_PARAMETERS direct3dcreateparams;
    D3DCAPS9 direct3ddevicecaps;
    D3DGAMMARAMP defaultgammaramp;
    D3DGAMMARAMP currentgammaramp;
    // Default vertex buffer, for textures that don't have one
    D3DVertexBufferPtr vertexbuffer;
    // Texture for rendering in native resolution
    D3DBitmap *_nativeSurface = nullptr;
    CUSTOMVERTEX defaultVertices[4];
    String previousError;
    D3DPixelShaderPtr pixelShader;
    int _fullscreenDisplay = -1; // a display where exclusive fullscreen was created
    bool _smoothScaling;
    bool _legacyPixelShader;
    float _pixelRenderXOffset;
    float _pixelRenderYOffset;
    bool _renderAtScreenRes;

    // TODO: find a way to merge this with Render Targets from sprite batches,
    // have a SINGLE STACK of "render target states", where backbuffer is at the bottom
    struct BackbufferState
    {
        D3DSurfacePtr Surface;
        // FIXME: replace RendSize with explicit render coordinate offset? merge with ortho matrix?
        Size SurfSize; // actual surface size
        Size RendSize; // coordinate grid size (for centering sprites)
        Rect Viewport;
        glm::mat4 Projection;
        PlaneScaling Scaling;
        int Filter = 0;

        BackbufferState() = default;
        BackbufferState(const D3DSurfacePtr &surface, const Size &surf_size, const Size &rend_size,
            const Rect &view, const glm::mat4 &proj,
            const PlaneScaling &scale, int filter);
        BackbufferState(D3DSurfacePtr &&surface, const Size &surf_size, const Size &rend_size,
            const Rect &view, const glm::mat4 &proj,
            const PlaneScaling &scale, int filter);
        BackbufferState(const BackbufferState &state) = default;
        BackbufferState(BackbufferState &&state) = default;
        ~BackbufferState() = default;

        BackbufferState &operator = (const BackbufferState &state) = default;
        BackbufferState &operator = (BackbufferState &&state) = default;
    };

    BackbufferState _screenBackbuffer;
    BackbufferState _nativeBackbuffer;
    const BackbufferState *_currentBackbuffer = nullptr;

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
    HRESULT ResetDeviceAndRestore();
    HRESULT ResetD3DDevice();
    void SetupViewport();
    // For tracked render targets, disposes only the internal texture data
    void ReleaseRenderTargetData();
    // For tracked render targets, recreates the internal texture data
    void RecreateRenderTargets();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();
    void set_up_default_vertices();
    void AdjustSizeToNearestSupportedByCard(int *width, int *height);
    void UpdateTextureRegion(D3DTextureTile *tile, const Bitmap *bitmap, bool has_alpha, bool opaque);
    void CreateVirtualScreen();
    bool IsTextureFormatOk(D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat);

    // Backup all draw lists in the temp storage
    void BackupDrawLists();
    // Restore draw lists from the temp storage
    void RestoreDrawLists();
    // Deletes draw list backups
    void ClearDrawBackups();
    // Mark certain sprite batches to be skipped at the next render
    void FilterSpriteBatches(uint32_t skip_filter);

    void RenderAndPresent(bool clearDrawListAfterwards);
    void RenderImpl(bool clearDrawListAfterwards);
    void RenderToSurface(BackbufferState *state, bool clearDrawListAfterwards);
    // Set current backbuffer state, which properties are used when refering to backbuffer
    void SetBackbufferState(BackbufferState *state, bool clear);
    // Sets a Direct3D viewport for the current render target.
    void SetD3DViewport(const Rect &rc);
    // Sets the scissor (render clip), clip rect is passed in the "native" coordinates.
    // Optionally pass render_on_texture if the rendering is done to texture, in native coords,
    // otherwise we assume it is set on a whole screen, scaled to the screen coords.
    void SetScissor(const Rect &clip, bool render_on_texture = false);
    // Configures rendering mode for the render target, depending on its properties
    // TODO: find a good way to merge with SetRenderTarget
    void SetRenderTarget(const D3DSpriteBatch *batch, Size &surface_sz, bool clear);
    void RenderSpriteBatches();
    size_t RenderSpriteBatch(const D3DSpriteBatch &batch, size_t from, const Size &rend_sz);
    void RenderSprite(const D3DDrawListEntry *entry, const glm::mat4 &matGlobal,
        const SpriteColorTransform &color, const Size &rend_sz);
    // Renders given texture onto the current render target
    void RenderTexture(D3DBitmap *bitmap, int draw_x, int draw_y, const glm::mat4 &matGlobal,
        const SpriteColorTransform &color, const Size &rend_sz);
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
    D3DGraphicsFactory() = default;

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
    static Library _library;
    D3DPtr         _direct3d;
};

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DD3D_H

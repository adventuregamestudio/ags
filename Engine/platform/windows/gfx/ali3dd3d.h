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
// Direct3D graphics factory
//
//=============================================================================

#ifndef __AGS_EE_GFX__ALI3DD3D_H
#define __AGS_EE_GFX__ALI3DD3D_H

#ifndef WINDOWS_VERSION
#error This file should only be included on the Windows build
#endif

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include <allegro.h>
#include <winalleg.h>
#include <d3d9.h>
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/gfxdriverbase.h"
#include "util/library.h"
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
    virtual void SetStretch(int width, int height, bool useResampler = true)
    {
        _stretchToWidth = width;
        _stretchToHeight = height;
        _useResampler = useResampler;
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
    bool _useResampler;
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
        _useResampler = false;
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

    void Dispose();

    ~D3DBitmap()
    {
        Dispose();
    }
};

class D3DGfxModeList : public IGfxModeList
{
public:
    D3DGfxModeList(IDirect3D9 *direct3d, D3DFORMAT d3dformat)
        : _direct3d(direct3d)
        , _pixelFormat(d3dformat)
    {
        _modeCount = _direct3d ? _direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, _pixelFormat) : 0;
    }

    ~D3DGfxModeList()
    {
        if (_direct3d)
            _direct3d->Release();
    }

    virtual int GetModeCount() const
    {
        return _modeCount;
    }

    virtual bool GetMode(int index, DisplayMode &mode) const;

private:
    IDirect3D9 *_direct3d;
    D3DFORMAT   _pixelFormat;
    int         _modeCount;
};

struct CUSTOMVERTEX
{
    D3DVECTOR   position; // The position.
    D3DVECTOR   normal;
    FLOAT       tu, tv;   // The texture coordinates.
};

typedef SpriteDrawListEntry<D3DBitmap> D3DDrawListEntry;

class D3DGraphicsDriver : public VideoMemoryGraphicsDriver
{
public:
    virtual const char*GetDriverName() { return "Direct3D 9"; }
    virtual const char*GetDriverID() { return "D3D9"; }
    virtual void SetTintMethod(TintMethod method);
    virtual bool SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer);
    virtual bool SetNativeSize(const Size &src_size);
    virtual bool SetRenderFrame(const Rect &dst_rect);
    virtual int  GetDisplayDepthForNativeDepth(int native_color_depth) const;
    virtual IGfxModeList *GetSupportedModeList(int color_depth);
    virtual bool IsModeSupported(const DisplayMode &mode);
    virtual PGfxFilter GetGraphicsFilter() const;
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
    virtual void GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res);
    virtual void EnableVsyncBeforeRender(bool enabled) { }
    virtual void Vsync();
    virtual void RenderSpritesAtScreenResolution(bool enabled) { _renderSprAtScreenRes = enabled; };
    virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    virtual void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue);
    virtual void BoxOutEffect(bool blackingOut, int speed, int delay);
    virtual bool PlayVideo(const char *filename, bool useSound, VideoSkipType skipType, bool stretchToFullScreen);
    virtual bool SupportsGammaControl();
    virtual void SetGamma(int newGamma);
    virtual void UseSmoothScaling(bool enabled) { _smoothScaling = enabled; }
    virtual bool RequiresFullRedrawEachFrame() { return true; }
    virtual bool HasAcceleratedStretchAndFlip() { return true; }
    virtual void SetScreenTint(int red, int green, int blue);

    typedef stdtr1compat::shared_ptr<D3DGfxFilter> PD3DFilter;

    void SetGraphicsFilter(PD3DFilter filter);

    // Internal; TODO: find a way to hide these
    int _initDLLCallback(const DisplayMode &mode);
    int _resetDeviceIfNecessary();

    D3DGraphicsDriver(IDirect3D9 *d3d);
    virtual ~D3DGraphicsDriver();

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
    RECT viewport_rect;
    UINT availableVideoMemory;
    int _tint_red, _tint_green, _tint_blue;
    CUSTOMVERTEX defaultVertices[4];
    String previousError;
    IDirect3DPixelShader9* pixelShader;
    bool _smoothScaling;
    bool _legacyPixelShader;
    float _pixelRenderXOffset;
    float _pixelRenderYOffset;
    bool _renderSprAtScreenRes;
    Bitmap *_screenTintLayer;
    D3DBitmap* _screenTintLayerDDB;
    D3DDrawListEntry _screenTintSprite;

    std::vector<D3DDrawListEntry> drawList;
    std::vector<D3DDrawListEntry> drawListLastTime;
    GlobalFlipType flipTypeLastTime;

    // Called after new mode was successfully initialized
    virtual void OnModeSet(const DisplayMode &mode);
    // Called when the direct3d device is created for the first time
    int  FirstTimeInit();
    void initD3DDLL(const DisplayMode &mode);
    void InitializeD3DState();
    void SetupViewport();
    HRESULT ResetD3DDevice();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();
    void set_up_default_vertices();
    void make_translated_scaling_matrix(D3DMATRIX *matrix, float x, float y, float xScale, float yScale);
    void AdjustSizeToNearestSupportedByCard(int *width, int *height);
    void UpdateTextureRegion(TextureTile *tile, Bitmap *bitmap, D3DBitmap *target, bool hasAlpha);
    void CreateVirtualScreen();
    void do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    bool IsTextureFormatOk( D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat );
    void create_screen_tint_bitmap();
    void _renderAndPresent(GlobalFlipType flip, bool clearDrawListAfterwards);
    void _render(GlobalFlipType flip, bool clearDrawListAfterwards);
    void _reDrawLastFrame();
    void _renderSprite(D3DDrawListEntry *entry, bool globalLeftRightFlip, bool globalTopBottomFlip);
};


class D3DGraphicsFactory : public GfxDriverFactoryBase<D3DGraphicsDriver, D3DGfxFilter>
{
public:
    virtual ~D3DGraphicsFactory();

    virtual size_t               GetFilterCount() const;
    virtual const GfxFilterInfo *GetFilterInfo(size_t index) const;
    virtual String               GetDefaultFilterID() const;

    static D3DGraphicsFactory   *GetFactory();
    static D3DGraphicsDriver    *GetD3DDriver();

private:
    D3DGraphicsFactory();

    virtual D3DGraphicsDriver   *EnsureDriverCreated();
    virtual D3DGfxFilter        *CreateFilter(const String &id);

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

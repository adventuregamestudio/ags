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

#include <allegro.h>
#include <winalleg.h>
#include <d3d9.h>
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "util/string.h"

struct D3DGfxFilter;

namespace AGS
{
namespace Engine
{
namespace D3D
{

using AGS::Common::Bitmap;
using AGS::Common::String;

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

    virtual int GetModeCount()
    {
        return _modeCount;
    }

    virtual bool GetMode(int index, DisplayResolution &resolution);

private:
    IDirect3D9 *_direct3d;
    D3DFORMAT   _pixelFormat;
    int         _modeCount;
};


#define MAX_DRAW_LIST_SIZE 200

struct CUSTOMVERTEX
{
    D3DVECTOR   position; // The position.
    D3DVECTOR   normal;
    FLOAT       tu, tv;   // The texture coordinates.
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
    virtual void SetGraphicsFilter(GfxFilter *filter);
    virtual void SetTintMethod(TintMethod method);
    virtual bool Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer, bool vsync);
    virtual bool Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer, bool vsync);
    virtual IGfxModeList *GetSupportedModeList(int color_depth);
    virtual DisplayResolution GetResolution();
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

    bool CreateDriver();

    // Internal
    int _initDLLCallback();
    int _resetDeviceIfNecessary();
    void _render(GlobalFlipType flip, bool clearDrawListAfterwards);
    void _reDrawLastFrame();
    D3DGraphicsDriver(D3DGfxFilter *filter);
    virtual ~D3DGraphicsDriver();

    D3DGfxFilter *_filter;

private:
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
    bool _newmode_vsync;
    int _global_x_offset, _global_y_offset;
    UINT availableVideoMemory;
    GFXDRV_CLIENTCALLBACK _pollingCallback;
    GFXDRV_CLIENTCALLBACK _drawScreenCallback;
    GFXDRV_CLIENTCALLBACKXY _nullSpriteCallback;
    GFXDRV_CLIENTCALLBACKINITGFX _initGfxCallback;
    int _tint_red, _tint_green, _tint_blue;
    CUSTOMVERTEX defaultVertices[4];
    String previousError;
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

    LONG _allegroOriginalWindowStyle;

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

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DD3D_H

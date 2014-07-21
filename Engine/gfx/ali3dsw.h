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
// Software graphics factory, based on Allegro
//
//=============================================================================

#ifndef __AGS_EE_GFX__ALI3DSW_H
#define __AGS_EE_GFX__ALI3DSW_H

#include <allegro.h>
#if defined (WINDOWS_VERSION)
#include <winalleg.h>
#include <ddraw.h>
#endif

#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/graphicsdriver.h"

namespace AGS
{
namespace Engine
{
namespace ALSW
{

class AllegroGfxFilter;
using AGS::Common::Bitmap;

class ALSoftwareBitmap : public IDriverDependantBitmap
{
public:
    // NOTE by CJ:
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
    virtual void SetLightLevel(int lightLevel)  { }
    virtual void SetTint(int red, int green, int blue, int tintSaturation) { }

    Bitmap *_bmp;
    int _width, _height;
    int _colDepth;
    bool _flipped;
    int _stretchToWidth, _stretchToHeight;
    bool _opaque;
    bool _hasAlpha;
    int _transparency;

    ALSoftwareBitmap(Bitmap *bmp, bool opaque, bool hasAlpha)
    {
        _bmp = bmp;
        _width = bmp->GetWidth();
        _height = bmp->GetHeight();
        _colDepth = bmp->GetColorDepth();
        _flipped = false;
        _stretchToWidth = 0;
        _stretchToHeight = 0;
        _transparency = 0;
        _opaque = opaque;
        _hasAlpha = hasAlpha;
    }

    int GetWidthToRender() { return (_stretchToWidth > 0) ? _stretchToWidth : _width; }
    int GetHeightToRender() { return (_stretchToHeight > 0) ? _stretchToHeight : _height; }

    void Dispose()
    {
        // do we want to free the bitmap?
    }

    ~ALSoftwareBitmap()
    {
        Dispose();
    }
};


class ALSoftwareGfxModeList : public IGfxModeList
{
public:
    ALSoftwareGfxModeList(GFX_MODE_LIST *alsw_gfx_mode_list)
        : _gfxModeList(alsw_gfx_mode_list)
    {
    }

    virtual int GetModeCount()
    {
        return _gfxModeList ? _gfxModeList->num_modes : 0;
    }

    virtual bool GetMode(int index, DisplayResolution &resolution);

private:
    GFX_MODE_LIST *_gfxModeList;
};


#define MAX_DRAW_LIST_SIZE 200

class ALSoftwareGraphicsDriver : public IGraphicsDriver
{
public:
    ALSoftwareGraphicsDriver() { 
        _filter = NULL; 
        _callback = NULL; 
        _drawScreenCallback = NULL;
        _nullSpriteCallback = NULL;
        _initGfxCallback = NULL;
        _global_x_offset = 0;
        _global_y_offset = 0;
        _tint_red = 0;
        _tint_green = 0;
        _tint_blue = 0;
        _autoVsync = false;
        _spareTintingScreen = NULL;
        numToDraw = 0;
        _gfxModeList = NULL;
#ifdef _WIN32
        dxGammaControl = NULL;
#endif
        _allegroScreenWrapper = NULL;
    }

    virtual const char*GetDriverName() { return "Allegro/DX5"; }
    virtual const char*GetDriverID() { return "DX5"; }
    virtual void SetTintMethod(TintMethod method);
    virtual bool Init(int width, int height, int colourDepth, bool windowed, volatile int *loopTimer, bool vsync);
    virtual bool Init(int virtualWidth, int virtualHeight, int realWidth, int realHeight, int colourDepth, bool windowed, volatile int *loopTimer, bool vsync);
    virtual IGfxModeList *GetSupportedModeList(int color_depth);
    virtual DisplayResolution GetResolution();
    virtual void SetCallbackForPolling(GFXDRV_CLIENTCALLBACK callback) { _callback = callback; }
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
    virtual void SetRenderOffset(int x, int y);
    virtual void RenderToBackBuffer();
    virtual void Render();
    virtual void Render(GlobalFlipType flip);
    virtual void GetCopyOfScreenIntoBitmap(Bitmap *destination);
    virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    virtual void FadeIn(int speed, PALETTE pal, int targetColourRed, int targetColourGreen, int targetColourBlue);
    virtual void BoxOutEffect(bool blackingOut, int speed, int delay);
    virtual bool PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen);
    virtual bool SupportsGammaControl() ;
    virtual void SetGamma(int newGamma);
    virtual void UseSmoothScaling(bool enabled) { }
    virtual void EnableVsyncBeforeRender(bool enabled) { _autoVsync = enabled; }
    virtual void Vsync();
    virtual bool RequiresFullRedrawEachFrame() { return false; }
    virtual bool HasAcceleratedStretchAndFlip() { return false; }
    virtual bool UsesMemoryBackBuffer() { return true; }
    virtual Bitmap *GetMemoryBackBuffer() { return virtualScreen; }
    virtual void SetMemoryBackBuffer(Bitmap *backBuffer) { virtualScreen = backBuffer; }
    virtual void SetScreenTint(int red, int green, int blue) { 
        _tint_red = red; _tint_green = green; _tint_blue = blue; }
    virtual ~ALSoftwareGraphicsDriver();

    void SetGraphicsFilter(AllegroGfxFilter *filter);

    AllegroGfxFilter *_filter;

private:
    volatile int* _loopTimer;
    int _screenWidth, _screenHeight;
    int actualInitWid, actualInitHit;
    int _colorDepth;
    bool _windowed;
    bool _autoVsync;
    Bitmap *_allegroScreenWrapper;
    Bitmap *virtualScreen;
    Bitmap *_spareTintingScreen;
    GFXDRV_CLIENTCALLBACK _callback;
    GFXDRV_CLIENTCALLBACK _drawScreenCallback;
    GFXDRV_CLIENTCALLBACKXY _nullSpriteCallback;
    GFXDRV_CLIENTCALLBACKINITGFX _initGfxCallback;
    int _tint_red, _tint_green, _tint_blue;
    int _global_x_offset, _global_y_offset;

    ALSoftwareBitmap* drawlist[MAX_DRAW_LIST_SIZE];
    int drawx[MAX_DRAW_LIST_SIZE], drawy[MAX_DRAW_LIST_SIZE];
    int numToDraw;
    GFX_MODE_LIST *_gfxModeList;

#ifdef _WIN32
    IDirectDrawGammaControl* dxGammaControl;
    // The gamma ramp is a lookup table for each possible R, G and B value
    // in 32-bit colour (from 0-255) it maps them to a brightness value
    // from 0-65535. The default gamma ramp just multiplies each value by 256
    DDGAMMARAMP gammaRamp;
    DDGAMMARAMP defaultGammaRamp;
    DDCAPS ddrawCaps;
#endif

    void highcolor_fade_out(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    void highcolor_fade_in(Bitmap *bmp_orig, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    void __fade_from_range(PALLETE source, PALLETE dest, int speed, int from, int to) ;
    void __fade_out_range(int speed, int from, int to, int targetColourRed, int targetColourGreen, int targetColourBlue) ;
    bool IsModeSupported(int driver, int width, int height, int colDepth);
    int  GetAllegroGfxDriverID(bool windowed);
};


class ALSWGraphicsFactory : public GfxDriverFactoryBase<ALSoftwareGraphicsDriver, AllegroGfxFilter>
{
public:
    virtual ~ALSWGraphicsFactory();

    virtual size_t               GetFilterCount() const;
    virtual const GfxFilterInfo *GetFilterInfo(size_t index) const;

    static  ALSWGraphicsFactory *GetFactory();

private:
    virtual ALSoftwareGraphicsDriver *EnsureDriverCreated();
    virtual AllegroGfxFilter         *CreateFilter(const String &id);

    static ALSWGraphicsFactory *_factory;
};

} // namespace ALSW
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DSW_H

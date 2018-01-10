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

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include <allegro.h>
#if defined (WINDOWS_VERSION)
#include <winalleg.h>
#include <ddraw.h>
#endif

#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/gfxdriverbase.h"

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
    virtual void SetStretch(int width, int height, bool useResampler = true) 
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

    virtual int GetModeCount() const
    {
        return _gfxModeList ? _gfxModeList->num_modes : 0;
    }

    virtual bool GetMode(int index, DisplayMode &mode) const;

private:
    GFX_MODE_LIST *_gfxModeList;
};


typedef SpriteDrawListEntry<ALSoftwareBitmap> ALDrawListEntry;

class ALSoftwareGraphicsDriver : public GraphicsDriverBase
{
public:
    ALSoftwareGraphicsDriver();

    virtual const char*GetDriverName() { return "Software renderer"; }
    virtual const char*GetDriverID() { return "Software"; }
    virtual void SetTintMethod(TintMethod method);
    virtual bool SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer);
    virtual bool SetNativeSize(const Size &src_size);
    virtual bool SetRenderFrame(const Rect &dst_rect);
    virtual bool IsModeSupported(const DisplayMode &mode);
    virtual int  GetDisplayDepthForNativeDepth(int native_color_depth) const;
    virtual IGfxModeList *GetSupportedModeList(int color_depth);
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
    virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    virtual void FadeIn(int speed, PALETTE pal, int targetColourRed, int targetColourGreen, int targetColourBlue);
    virtual void BoxOutEffect(bool blackingOut, int speed, int delay);
    virtual bool PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen);
    virtual bool SupportsGammaControl() ;
    virtual void SetGamma(int newGamma);
    virtual void UseSmoothScaling(bool enabled) { }
    virtual void EnableVsyncBeforeRender(bool enabled) { _autoVsync = enabled; }
    virtual void Vsync();
    virtual void RenderSpritesAtScreenResolution(bool enabled) { }
    virtual bool RequiresFullRedrawEachFrame() { return false; }
    virtual bool HasAcceleratedStretchAndFlip() { return false; }
    virtual bool UsesMemoryBackBuffer() { return true; }
    virtual Bitmap *GetMemoryBackBuffer() { return virtualScreen; }
    virtual void SetMemoryBackBuffer(Bitmap *backBuffer) { virtualScreen = backBuffer; }
    virtual void SetScreenTint(int red, int green, int blue) { 
        _tint_red = red; _tint_green = green; _tint_blue = blue; }
    virtual ~ALSoftwareGraphicsDriver();

    typedef stdtr1compat::shared_ptr<AllegroGfxFilter> PALSWFilter;

    void SetGraphicsFilter(PALSWFilter filter);

private:
    PALSWFilter _filter;

    bool _autoVsync;
    Bitmap *_allegroScreenWrapper;
    // Virtual screen bitmap is either a wrapper over Allegro's real screen
    // bitmap, or bitmap provided by the graphics filter. It should not be
    // disposed by the renderer: it is up to filter object to manage it.
    Bitmap *virtualScreen;
    Bitmap *_spareTintingScreen;
    int _tint_red, _tint_green, _tint_blue;

    std::vector<ALDrawListEntry> drawlist;
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

    // Use gfx filter to create a new virtual screen
    void CreateVirtualScreen();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();

    void highcolor_fade_out(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    void highcolor_fade_in(Bitmap *bmp_orig, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    void __fade_from_range(PALLETE source, PALLETE dest, int speed, int from, int to) ;
    void __fade_out_range(int speed, int from, int to, int targetColourRed, int targetColourGreen, int targetColourBlue) ;
    int  GetAllegroGfxDriverID(bool windowed);
};


class ALSWGraphicsFactory : public GfxDriverFactoryBase<ALSoftwareGraphicsDriver, AllegroGfxFilter>
{
public:
    virtual ~ALSWGraphicsFactory();

    virtual size_t               GetFilterCount() const;
    virtual const GfxFilterInfo *GetFilterInfo(size_t index) const;
    virtual String               GetDefaultFilterID() const;

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

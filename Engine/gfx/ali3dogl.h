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
// OpenGL graphics factory
//
//=============================================================================

#ifndef __AGS_EE_GFX__ALI3DOGL_H
#define __AGS_EE_GFX__ALI3DOGL_H

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/gfxdriverbase.h"
#include "util/string.h"

#include "ogl_headers.h"

namespace AGS
{
namespace Engine
{

namespace OGL
{

using Common::Bitmap;
using Common::String;

typedef struct _OGLVECTOR {
    float x;
    float y;
} OGLVECTOR2D;


struct OGLCUSTOMVERTEX
{
    OGLVECTOR2D position;
    float tu;
    float tv;
};

struct TextureTile
{
    int x, y;
    int width, height;
    unsigned int texture;
};

class OGLBitmap : public IDriverDependantBitmap
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
    OGLCUSTOMVERTEX* _vertex;
    TextureTile *_tiles;
    int _numTiles;

    OGLBitmap(int width, int height, int colDepth, bool opaque)
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

    ~OGLBitmap()
    {
        Dispose();
    }
};

typedef SpriteDrawListEntry<OGLBitmap> OGLDrawListEntry;

class OGLDisplayModeList : public IGfxModeList
{
public:
    OGLDisplayModeList(const std::vector<DisplayMode> &modes)
        : _modes(modes)
    {
    }

    virtual int GetModeCount() const
    {
        return _modes.size();
    }

    virtual bool GetMode(int index, DisplayMode &mode) const
    {
        if (index >= 0 && (size_t)index < _modes.size())
        {
            mode = _modes[index];
            return true;
        }
        return false;
    }

private:
    std::vector<DisplayMode> _modes;
};


class OGLGfxFilter;

class OGLGraphicsDriver : public GraphicsDriverBase
{
public:
    virtual const char*GetDriverName() { return "OpenGL"; }
    virtual const char*GetDriverID() { return "OGL"; }
    virtual void SetTintMethod(TintMethod method);
    virtual bool SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer);
    virtual bool SetNativeSize(const Size &src_size);
    virtual bool SetRenderFrame(const Rect &dst_rect);
    virtual int GetDisplayDepthForNativeDepth(int native_color_depth) const;
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
    virtual void GetCopyOfScreenIntoBitmap(Bitmap *destination);
    virtual void EnableVsyncBeforeRender(bool enabled) { }
    virtual void Vsync();
    virtual void RenderSpritesAtScreenResolution(bool enabled);
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
    virtual Bitmap *GetMemoryBackBuffer() { return NULL; }
    virtual void SetMemoryBackBuffer(Bitmap *backBuffer) {  }
    virtual void SetScreenTint(int red, int green, int blue);

    typedef stdtr1compat::shared_ptr<OGLGfxFilter> POGLFilter;

    void SetGraphicsFilter(POGLFilter filter);

    // Internal
    void _render(GlobalFlipType flip, bool clearDrawListAfterwards);
    void _reDrawLastFrame();
    OGLGraphicsDriver();
    virtual ~OGLGraphicsDriver();

private:
    POGLFilter _filter;

#if defined (WINDOWS_VERSION)
    HDC _hDC;
    HGLRC _hRC;
    HWND _hWnd;
    HINSTANCE _hInstance;
#endif
    int _tint_red, _tint_green, _tint_blue;
    // Position of backbuffer texture in world space
    GLfloat _backbuffer_vertices[8];
    // Relative position of source image on the backbuffer texture,
    // in local coordinates
    GLfloat _backbuffer_texture_coordinates[8];
    OGLCUSTOMVERTEX defaultVertices[4];
    String previousError;
    bool _smoothScaling;
    bool _legacyPixelShader;
    float _pixelRenderOffset;
    Bitmap *_screenTintLayer;
    OGLBitmap* _screenTintLayerDDB;
    OGLDrawListEntry _screenTintSprite;
    Bitmap *_dummyVirtualScreen;

    int device_screen_physical_width;
    int device_screen_physical_height;
    int device_mouse_clip_left;
    int device_mouse_clip_right;
    int device_mouse_clip_top;
    int device_mouse_clip_bottom;

    // Viewport and scissor rect, in OpenGL screen coordinates (0,0 is at left-bottom)
    Rect _viewportRect;

    // These two flags define whether driver can, and should (respectively)
    // render sprites to texture, and then texture to screen, as opposed to
    // rendering to screen directly. This is known as supersampling mode
    bool _can_render_to_texture;
    bool _do_render_to_texture;
    // Backbuffer texture multiplier, used to determine a size of texture
    // relative to the native game size.
    int _super_sampling;
    unsigned int _backbuffer;
    unsigned int _fbo;
    // Size of the backbuffer drawing area, equals to native size
    // multiplied by _super_sampling
    Size _backRenderSize;
    // Actual size of the backbuffer texture, created by OpenGL
    Size _backTextureSize;


    std::vector<OGLDrawListEntry> drawList;
    std::vector<OGLDrawListEntry> drawListLastTime;
    GlobalFlipType flipTypeLastTime;

    void InitOpenGl();
    void set_up_default_vertices();
    // Test if rendering to texture is supported
    void TestRenderToTexture();
    // Configure backbuffer texture, that is used in render-to-texture mode
    void SetupBackbufferTexture();
#if defined (WINDOWS_VERSION)
    void create_desktop_screen(int width, int height, int depth);
#endif
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();
    void AdjustSizeToNearestSupportedByCard(int *width, int *height);
    void UpdateTextureRegion(TextureTile *tile, Bitmap *bitmap, OGLBitmap *target, bool hasAlpha);
    void CreateVirtualScreen();
    void do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    void create_screen_tint_bitmap();
    void _renderSprite(OGLDrawListEntry *entry, bool globalLeftRightFlip, bool globalTopBottomFlip);
    void SetupViewport();
};


class OGLGraphicsFactory : public GfxDriverFactoryBase<OGLGraphicsDriver, OGLGfxFilter>
{
public:
    virtual ~OGLGraphicsFactory();

    virtual size_t               GetFilterCount() const;
    virtual const GfxFilterInfo *GetFilterInfo(size_t index) const;
    virtual String               GetDefaultFilterID() const;

    static OGLGraphicsFactory   *GetFactory();

private:
    virtual OGLGraphicsDriver   *EnsureDriverCreated();
    virtual OGLGfxFilter        *CreateFilter(const String &id);

    static OGLGraphicsFactory *_factory;
};

} // namespace OGL
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DOGL_H

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
// Implementation base for graphics driver
//
//=============================================================================
#ifndef __AGS_EE_GFX__GFXDRIVERBASE_H
#define __AGS_EE_GFX__GFXDRIVERBASE_H

#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "util/scaling.h"

namespace AGS
{
namespace Engine
{

using Common::Bitmap;


template<class T_DDB>
struct SpriteDrawListEntry
{
    T_DDB *bitmap; // TODO: use shared pointer?
    int x, y;
    bool skip;

    SpriteDrawListEntry()
        : bitmap(NULL)
        , x(0)
        , y(0)
        , skip(false)
    {
    }

    SpriteDrawListEntry(T_DDB *ddb, int x_ = 0, int y_ = 0)
        : bitmap(ddb)
        , x(x_)
        , y(y_)
        , skip(false)
    {
    }
};


// GraphicsDriverBase - is the parent class for all graphics drivers in AGS,
// that incapsulates the most common functionality.
class GraphicsDriverBase : public IGraphicsDriver
{
public:
    GraphicsDriverBase();

    virtual bool        IsModeSet() const;
    virtual bool        IsNativeSizeValid() const;
    virtual bool        IsRenderFrameValid() const;
    virtual DisplayMode GetDisplayMode() const;
    virtual Size        GetNativeSize() const;
    virtual Rect        GetRenderDestination() const;
    virtual void        SetRenderOffset(int x, int y);

    virtual void        SetCallbackForPolling(GFXDRV_CLIENTCALLBACK callback) { _pollingCallback = callback; }
    virtual void        SetCallbackToDrawScreen(GFXDRV_CLIENTCALLBACK callback) { _drawScreenCallback = callback; }
    virtual void        SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) { _initGfxCallback = callback; }
    virtual void        SetCallbackOnSurfaceUpdate(GFXDRV_CLIENTCALLBACKSURFACEUPDATE callback) { _initSurfaceUpdateCallback = callback; }
    virtual void        SetCallbackForNullSprite(GFXDRV_CLIENTCALLBACKXY callback) { _nullSpriteCallback = callback; }

protected:
    // Called after graphics driver was initialized for use for the first time
    virtual void OnInit(volatile int *loopTimer);
    // Called just before graphics mode is going to be uninitialized and its
    // resources released
    virtual void OnUnInit();
    // Called after new mode was successfully initialized
    virtual void OnModeSet(const DisplayMode &mode);
    // Called when the new native size is set
    virtual void OnSetNativeSize(const Size &src_size);
    // Called before display mode is going to be released
    virtual void OnModeReleased();
    // Called when new render frame is set
    virtual void OnSetRenderFrame(const Rect &dst_rect);
    // Called when the new filter is set
    virtual void OnSetFilter();

    void    OnScalingChanged();
    // Checks if the bitmap needs to be converted and **deletes original** if a new bitmap
    // had to be created
    Bitmap *ReplaceBitmapWithSupportedFormat(Bitmap *old_bmp);

    DisplayMode         _mode;          // display mode settings
    Rect                _srcRect;       // rendering source rect
    Rect                _dstRect;       // rendering destination rect
    Rect                _filterRect;    // filter scaling destination rect (before final scaling)
    PlaneScaling        _scaling;
    int                 _global_x_offset;
    int                 _global_y_offset;
    volatile int *      _loopTimer;

    // Callbacks
    GFXDRV_CLIENTCALLBACK _pollingCallback;
    GFXDRV_CLIENTCALLBACK _drawScreenCallback;
    GFXDRV_CLIENTCALLBACKXY _nullSpriteCallback;
    GFXDRV_CLIENTCALLBACKINITGFX _initGfxCallback;
    GFXDRV_CLIENTCALLBACKSURFACEUPDATE _initSurfaceUpdateCallback;
};



// Generic TextureTile base
struct TextureTile
{
    int x, y;
    int width, height;
};

// Parent class for the video memory DDBs
class VideoMemDDB : public IDriverDependantBitmap
{
public:
    virtual int GetWidth() { return _width; }
    virtual int GetHeight() { return _height; }
    virtual int GetColorDepth() { return _colDepth; }

    int _width, _height;
    int _colDepth;
    bool _opaque;
};


// VideoMemoryGraphicsDriver - is the parent class for the graphic drivers
// which drawing method is based on passing the sprite stack into GPU,
// rather than blitting to flat screen bitmap.
class VideoMemoryGraphicsDriver : public GraphicsDriverBase
{
public:
    VideoMemoryGraphicsDriver();
    virtual ~VideoMemoryGraphicsDriver();

    virtual bool UsesMemoryBackBuffer();
    virtual Bitmap *GetMemoryBackBuffer();
    virtual void SetMemoryBackBuffer(Bitmap *backBuffer);

protected:
    void CreateStageScreen();
    void DestroyStageScreen();
    // Use engine callback to acquire replacement for the null sprite;
    // returns true if the sprite was provided onto the virtual screen,
    // and false if this entry should be skipped.
    bool DoNullSpriteCallback(int x, int y);

    // Prepares bitmap to be applied to the texture, copies pixels to the provided buffer
    void BitmapToVideoMem(const Bitmap *bitmap, const bool has_alpha, const TextureTile *tile, const VideoMemDDB *target,
                            char *dst_ptr, const int dst_pitch, const bool usingLinearFiltering);

    // Stage virtual screen is used to let plugins draw custom graphics
    // in between render stages (between room and GUI, after GUI, and so on)
    Bitmap *_stageVirtualScreen;
    IDriverDependantBitmap *_stageVirtualScreenDDB;

private:
    // Flag which indicates whether stage screen was drawn upon during engine
    // callback and has to be inserted into sprite stack.
    bool _stageScreenDirty;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GFXDRIVERBASE_H

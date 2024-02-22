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
//
// Implementation base for graphics driver
//
//=============================================================================
#ifndef __AGS_EE_GFX__GFXDRIVERBASE_H
#define __AGS_EE_GFX__GFXDRIVERBASE_H

#include <memory>
#include <unordered_map>
#include <vector>
#include "gfx/ddb.h"
#include "gfx/gfx_def.h"
#include "gfx/graphicsdriver.h"
#include "util/scaling.h"
#include "util/resourcecache.h"


namespace AGS
{
namespace Engine
{

using Common::Bitmap;
using Common::PlaneScaling;

// Sprite batch, defines viewport and an optional model transformation for the list of sprites
struct SpriteBatchDesc
{
    uint32_t                 Parent = UINT32_MAX;
    // View rectangle for positioning and clipping, in resolution coordinates
    // (this may be screen or game frame resolution, depending on circumstances)
    Rect                     Viewport;
    // Optional model transformation, to be applied to each sprite
    SpriteTransform          Transform;
    // Optional flip, applied to the whole batch as the last transform
    Common::GraphicFlip      Flip = Common::kFlip_None;
    // Optional bitmap to draw sprites upon. Used exclusively by the software rendering mode.
    // TODO: merge with the RenderTexture?
    PBitmap                  Surface;
    // Optional texture to render sprites to. Used by hardware-accelerated renderers.
    IDriverDependantBitmap*  RenderTarget = nullptr;

    SpriteBatchDesc() = default;
    SpriteBatchDesc(uint32_t parent, const Rect viewport, const SpriteTransform &transform,
        Common::GraphicFlip flip = Common::kFlip_None, PBitmap surface = nullptr)
        : Parent(parent)
        , Viewport(viewport)
        , Transform(transform)
        , Flip(flip)
        , Surface(surface)
    {
    }
    // TODO: this does not need a parent?
    SpriteBatchDesc(uint32_t parent, IDriverDependantBitmap *render_target,
        const Rect viewport, const SpriteTransform &transform,
        Common::GraphicFlip flip = Common::kFlip_None)
        : Parent(parent)
        , Viewport(viewport)
        , Transform(transform)
        , Flip(flip)
        , RenderTarget(render_target)
    {
    }
};

typedef std::vector<SpriteBatchDesc> SpriteBatchDescs;

// The single sprite entry in the render list
template<class T_DDB>
struct SpriteDrawListEntry
{
    T_DDB *ddb = nullptr; // TODO: use shared pointer?
    uint32_t node = 0; // sprite batch / scene node index
    int x = 0, y = 0; // sprite position, in local batch / node coordinates
    bool skip = false;

    SpriteDrawListEntry() = default;
    SpriteDrawListEntry(T_DDB *ddb_, uint32_t node_, int x_, int y_)
        : ddb(ddb_)
        , node(node_)
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

    bool        IsModeSet() const override;
    bool        IsNativeSizeValid() const override;
    bool        IsRenderFrameValid() const override;
    DisplayMode GetDisplayMode() const override;
    Size        GetNativeSize() const override;
    Rect        GetRenderDestination() const override;

    bool        SetVsync(bool enabled) override;
    bool        GetVsync() const override;

    void        BeginSpriteBatch(const Rect &viewport, const SpriteTransform &transform,
                    Common::GraphicFlip flip = Common::kFlip_None, PBitmap surface = nullptr) override;
    virtual void BeginSpriteBatch(IDriverDependantBitmap *render_target, const Rect &viewport, const SpriteTransform &transform,
                    Common::GraphicFlip flip = Common::kFlip_None) override;
    void        EndSpriteBatch() override;
    void        ClearDrawLists() override;

    void        SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) override { _initGfxCallback = callback; }
    void        SetCallbackOnSpriteEvt(GFXDRV_CLIENTCALLBACKEVT callback) override { _spriteEvtCallback = callback; }

protected:
    // Special internal values, applied to DrawListEntry
    static const uintptr_t DRAWENTRY_STAGECALLBACK = 0x0;
    static const uintptr_t DRAWENTRY_FADE = 0x1;
    static const uintptr_t DRAWENTRY_TINT = 0x2;

    // Called after graphics driver was initialized for use for the first time
    virtual void OnInit();
    // Called just before graphics mode is going to be uninitialized and its
    // resources released
    virtual void OnUnInit();
    // Called after new mode was successfully initialized
    virtual void OnModeSet(const DisplayMode &mode);
    // Called when the new native size is set
    virtual void OnSetNativeRes(const GraphicResolution &native_res);
    // Called before display mode is going to be released
    virtual void OnModeReleased();
    // Called when new render frame is set
    virtual void OnSetRenderFrame(const Rect &dst_rect);
    // Called when the new filter is set
    virtual void OnSetFilter();

    // Try changing vsync setting; fills new current mode in vsync_res,
    // returns whether the new setting was set successfully.
    virtual bool SetVsyncImpl(bool vsync, bool &vsync_res) { return false; }

    // Initialize sprite batch and allocate necessary resources
    virtual void InitSpriteBatch(size_t index, const SpriteBatchDesc &desc) = 0;
    // Gets the index of a last draw entry (sprite)
    virtual size_t GetLastDrawEntryIndex() = 0;
    // Clears sprite lists
    virtual void ResetAllBatches() = 0;

    void BeginSpriteBatch(const SpriteBatchDesc &desc);
    void OnScalingChanged();

    DisplayMode         _mode;          // display mode settings
    Rect                _srcRect;       // rendering source rect
    int                 _srcColorDepth; // rendering source color depth (in bits per pixel)
    Rect                _dstRect;       // rendering destination rect
    Rect                _filterRect;    // filter scaling destination rect (before final scaling)
    PlaneScaling        _scaling;       // native -> render dest coordinate transformation

    // Capability flags
    bool                _capsVsync = false; // is vsync available

    // Callbacks
    GFXDRV_CLIENTCALLBACKEVT _spriteEvtCallback;
    GFXDRV_CLIENTCALLBACKINITGFX _initGfxCallback;

    // Sprite batch parameters
    SpriteBatchDescs _spriteBatchDesc;
    // The range of sprites in this sprite batch (counting nested sprites):
    // the last of the previous batch, and the last of the current.
    std::vector<std::pair<size_t, size_t>> _spriteBatchRange;
    // The index of a currently filled sprite batch
    size_t _actSpriteBatch;
    // The index of a currently rendered sprite batch
    // (or -1 / UINT32_MAX if we are outside of the render pass)
    uint32_t _rendSpriteBatch;
};



// Parent class for the common DDB implementation
class BaseDDB : public IDriverDependantBitmap
{
public:
    int GetWidth() const override { return _width; }
    int GetHeight() const override { return _height; }
    int GetColorDepth() const override { return _colDepth; }

    int _width = 0, _height = 0;
    int _colDepth = 0;
    bool _hasAlpha = false; // has meaningful alpha channel
    bool _opaque = false; // no mask color

protected:
    BaseDDB() = default;
    virtual ~BaseDDB() = default;
};


// Generic TextureTile base
struct TextureTile
{
    // x, y, width, height define position of a valid image
    int x = 0, y = 0;
    int width = 0, height = 0;
    // allocWidth and allocHeight tell the actual allocated texture size
    int allocWidth = 0, allocHeight = 0;
};

// Special render hints for textures
enum TextureHint
{
    kTxHint_Normal,
    kTxHint_PremulAlpha  // texture pixels contain premultiplied alpha
};

// Sprite batch's internal parameters for the hardware-accelerated renderer
struct VMSpriteBatch
{
    // Batch's uid
    uint32_t ID = 0u;
    // Optional render target (for rendering on texture)
    IDriverDependantBitmap *RenderTarget = nullptr;
    // Clipping viewport, in *absolute* (screen) coordinates
    Rect Viewport;
    // Transformation matrix, built from the batch description
    glm::mat4 Matrix;
    // Viewport transformation matrix, necessary, as the coord origin is different
    glm::mat4 ViewportMat;
    // Batch color transformation
    SpriteColorTransform Color;

    VMSpriteBatch() = default;
    VMSpriteBatch(uint32_t id, const Rect &view, const glm::mat4 &matrix,
                  const glm::mat4 &vp_matrix, const SpriteColorTransform &color)
        : ID(id), Viewport(view), Matrix(matrix), ViewportMat(vp_matrix), Color(color) {}
    VMSpriteBatch(uint32_t id, IDriverDependantBitmap *render_target,
                  const Rect view, const glm::mat4 &matrix,
        const glm::mat4 &vp_matrix, const SpriteColorTransform &color)
        : ID(id), RenderTarget(render_target),
          Viewport(view), Matrix(matrix),
          ViewportMat(vp_matrix), Color(color) {}
};


// VideoMemoryGraphicsDriver - is the parent class for the graphic drivers
// which drawing method is based on passing the sprite stack into GPU,
// rather than blitting to flat screen bitmap.
class VideoMemoryGraphicsDriver : public GraphicsDriverBase
{
public:
    VideoMemoryGraphicsDriver();
    ~VideoMemoryGraphicsDriver() override;

    bool RequiresFullRedrawEachFrame() override { return true; }
    bool HasAcceleratedTransform() override { return true; }
    // NOTE: although we do use ours, we do not let engine draw upon it;
    // only plugin handling are allowed to request our mem buffer
    // for compatibility reasons.
    bool UsesMemoryBackBuffer() override { return false; }

    Bitmap *GetMemoryBackBuffer() override;
    void SetMemoryBackBuffer(Bitmap *backBuffer) override;
    Bitmap* GetStageBackBuffer(bool mark_dirty) override;
    void SetStageBackBuffer(Bitmap *backBuffer) override;
    bool GetStageMatrixes(RenderMatrixes &rm) override;

    // Creates new DDB and copy bitmap contents over
    IDriverDependantBitmap *CreateDDBFromBitmap(const Bitmap *bitmap, bool has_alpha, bool opaque = false) override;

    Texture *CreateTexture(int width, int height, int color_depth, bool opaque = false, bool as_render_target = false) = 0;
    // Create texture and initialize its pixels from the given bitmap; optionally assigns a ID
    Texture *CreateTexture(const Bitmap *bmp, bool has_alpha, bool opaque = false) override;

    // Sets stage screen parameters for the current batch.
    void SetStageScreen(const Size &sz, int x = 0, int y = 0) override;

protected:
    // Stage screens are raw bitmap buffers meant to be sent to plugins on demand
    // at certain drawing stages. If used at least once these buffers are then
    // rendered as additional sprites in their respected order.
    // Presets a stage screen with the given position (size is obligatory, offsets not).
    void SetStageScreen(size_t index, const Size &sz, int x = 0, int y = 0);
    // Returns a raw bitmap for the given stage screen.
    Bitmap *GetStageScreenRaw(size_t index);
    // Updates and returns a DDB for the given stage screen, and optional x,y position;
    // clears the raw bitmap after copying to the texture.
    IDriverDependantBitmap *UpdateStageScreenDDB(size_t index, int &x, int &y);
    // Disposes all the stage screen raw bitmaps and DDBs.
    void DestroyAllStageScreens();
    // Use engine callback to pass a render event;
    // returns a DDB if anything was drawn onto the current stage screen
    // (in which case it also fills optional x,y position),
    // or nullptr if this entry should be skipped.
    IDriverDependantBitmap *DoSpriteEvtCallback(int evt, int data, int &x, int &y);

    // Prepare and get fx item from the pool
    IDriverDependantBitmap *MakeFx(int r, int g, int b);
    // Resets fx pool counter
    void ResetFxPool();
    // Disposes all items in the fx pool
    void DestroyFxPool();

    // Prepares bitmap to be applied to the texture, copies pixels to the provided buffer
    void BitmapToVideoMem(const Bitmap *bitmap, const bool has_alpha, const TextureTile *tile,
        uint8_t *dst_ptr, const int dst_pitch, const bool usingLinearFiltering);
    // Same but optimized for opaque source bitmaps which ignore transparent "mask color"
    void BitmapToVideoMemOpaque(const Bitmap *bitmap, const TextureTile *tile,
        uint8_t *dst_ptr, const int dst_pitch);

    // Stage matrixes are used to let plugins with hardware acceleration know model matrix;
    // these matrixes are filled compatible with each given renderer
    RenderMatrixes _stageMatrixes;

    // Color component shifts in video bitmap format (set by implementations)
    int _vmem_a_shift_32;
    int _vmem_r_shift_32;
    int _vmem_g_shift_32;
    int _vmem_b_shift_32;

private:
    // Stage virtual screens are used to let plugins draw custom graphics
    // in between render stages (between room and GUI, after GUI, and so on).
    // TODO: possibly may be optimized further by having only 1 bitmap/ddb
    // pair, and subbitmaps for raw drawing on separate stages.
    struct StageScreen
    {
        Rect Position; // bitmap size and pos preset (bitmap may be created later)
        std::unique_ptr<Bitmap> Raw;
        IDriverDependantBitmap *DDB = nullptr;
    };
    std::vector<StageScreen> _stageScreens;
    // Flag which indicates whether stage screen was drawn upon during engine
    // callback and has to be inserted into sprite stack.
    bool _stageScreenDirty;

    // Fx quads pool (for screen overlay effects)
    struct ScreenFx
    {
        std::unique_ptr<Bitmap> Raw;
        IDriverDependantBitmap *DDB = nullptr;
        int Red = -1;
        int Green = -1;
        int Blue = -1;
    };
    std::vector<ScreenFx> _fxPool;
    size_t _fxIndex; // next free pool item

    // specialized method to convert bitmap to video memory depending on bit depth
    template <typename T, bool HasAlpha> void
    BitmapToVideoMemImpl(
            const Bitmap *bitmap, const TextureTile *tile,
            uint8_t *dst_ptr, const int dst_pitch
    );

    template <typename T> void
    BitmapToVideoMemOpaqueImpl(
            const Bitmap *bitmap, const TextureTile *tile,
            uint8_t *dst_ptr, const int dst_pitch
    );

    template <typename T, bool HasAlpha> void
    BitmapToVideoMemLinearImpl(
            const Bitmap *bitmap, const TextureTile *tile,
            uint8_t *dst_ptr, const int dst_pitch
    );
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GFXDRIVERBASE_H

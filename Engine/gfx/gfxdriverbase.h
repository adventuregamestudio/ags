//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
    // Optional filter flags; this lets to filter certain batches out during some operations,
    // such as fading effects or making screenshots.
    uint32_t                 FilterFlags = 0u;

    SpriteBatchDesc() = default;
    SpriteBatchDesc(uint32_t parent, const Rect viewport, const SpriteTransform &transform,
        Common::GraphicFlip flip = Common::kFlip_None, PBitmap surface = nullptr,
        uint32_t filter_flags = 0)
        : Parent(parent)
        , Viewport(viewport)
        , Transform(transform)
        , Flip(flip)
        , Surface(surface)
        , FilterFlags(filter_flags)
    {
    }
    // TODO: this does not need a parent?
    SpriteBatchDesc(uint32_t parent, IDriverDependantBitmap *render_target,
        const Rect viewport, const SpriteTransform &transform,
        Common::GraphicFlip flip = Common::kFlip_None, uint32_t filter_flags = 0)
        : Parent(parent)
        , Viewport(viewport)
        , Transform(transform)
        , Flip(flip)
        , RenderTarget(render_target)
        , FilterFlags(filter_flags)
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

    ///////////////////////////////////////////////////////
    // Mode initialization
    //
    // Gets if a graphics mode was initialized
    bool        IsModeSet() const override;
    // Gets the currently set display mode
    DisplayMode GetDisplayMode() const override;
    // Tells if a native image size is properly set
    bool        IsNativeSizeValid() const override;
    // Gets currently set native image size
    Size        GetNativeSize() const override;
    // Tells if render frame is properly set
    bool        IsRenderFrameValid() const override;
    // Gets currently set render frame
    Rect        GetRenderDestination() const override;

    ///////////////////////////////////////////////////////
    // Miscelaneous setup
    //
    // Set the display mode initialization callback.
    void        SetCallbackOnInit(GFXDRV_CLIENTCALLBACKINITGFX callback) override { _initGfxCallback = callback; }
    // Set the sprite event callback.
    void        SetCallbackOnSpriteEvt(GFXDRV_CLIENTCALLBACKEVT callback) override { _spriteEvtCallback = callback; }
    // Toggles vertical sync mode, if renderer supports one; returns the *new state*.
    bool        SetVsync(bool enabled) override;
    // Tells if the renderer currently has vsync enabled.
    bool        GetVsync() const override;

    ///////////////////////////////////////////////////////
    // Preparing a scene
    //
    // Prepares next sprite batch, a list of sprites with defined viewport and optional
    // global model transformation.
    void        BeginSpriteBatch(const Rect &viewport, const SpriteTransform &transform, uint32_t filter_flags = 0) override;
    // Begins a sprite batch with defined viewport and a global model transformation
    // and a global flip setting. Optionally provides a surface which should be rendered
    // underneath the rest of the sprites.
    void        BeginSpriteBatch(const Rect &viewport, const SpriteTransform &transform,
                    Common::GraphicFlip flip, PBitmap surface = nullptr, uint32_t filter_flags = 0) override;
    // Begins a sprite batch which will be rendered on a target texture.
    void        BeginSpriteBatch(IDriverDependantBitmap *render_target, const Rect &viewport, const SpriteTransform &transform,
                    Common::GraphicFlip flip = Common::kFlip_None, uint32_t filter_flags = 0) override;
    // Ends current sprite batch
    void        EndSpriteBatch() override;
    // Clears all sprite batches, resets batch counter
    void        ClearDrawLists() override;

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

    void OnScalingChanged();

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
    // the index of a first of the current batch, and the next index past the last one.
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
    int  GetWidth() const override { return _size.Width; }
    int  GetHeight() const override { return _size.Height; }
    int  GetColorDepth() const override { return _colDepth; }
    bool IsOpaque() const override { return (_txFlags & kTxFlags_Opaque) != 0; }
    bool MatchesFormat(AGS::Common::Bitmap *other) const
    {
        return _size == other->GetSize() && _colDepth == other->GetColorDepth();
    }

    Pointf GetOrigin() const override { return _origin; }
    void SetOrigin(float originx, float originy) override
    {
        _origin = Pointf(originx, originy);
    }
    Size GetStretch() const override { return _scaledSize; }
    bool GetUseResampler() const override { return false; }
    void SetStretch(int width, int height, bool /*useResampler*/) override
    {
        _scaledSize = Size(width, height);
    }
    Common::GraphicFlip GetFlip() const override { return _flip; }
    void SetFlip(Common::GraphicFlip flip) override { _flip = flip; }
    // Rotation input is in degrees clockwise, but the implementation may store it in radians internally
    float GetRotation() const override { return _rotation; }
    void SetRotation(float rotation) override { _rotation = rotation; }
    int  GetAlpha() const override { return _alpha; }
    void SetAlpha(int alpha) override { _alpha = alpha; }
    int  GetLightLevel() const override { return _lightLevel; }
    void SetLightLevel(int light_level) override { _lightLevel = light_level; }
    void GetTint(int &red, int &green, int &blue, int &tintSaturation) const override
    {
        red = _red;
        green = _green;
        blue = _blue;
        tintSaturation = _tintSaturation;
    }
    void SetTint(int red, int green, int blue, int tintSaturation) override
    {
        _red = red;
        _green = green;
        _blue = blue;
        _tintSaturation = tintSaturation;
    }
    Common::BlendMode GetBlendMode() const { return _blendMode; }
    void SetBlendMode(Common::BlendMode blendMode) override { _blendMode = blendMode; }
    uint32_t GetShader() const override { return _shader; }
    void SetShader(uint32_t shader_id) override { _shader = shader_id; }

    int  GetTextureFlags() const { return _txFlags; }
    const Size &GetSize() const { return _size; }
    int  GetWidthToRender() const { return _scaledSize.Width; }
    int  GetHeightToRender() const { return _scaledSize.Height; }
    const Size &GetSizeToRender() const { return _scaledSize; }

protected:
    BaseDDB() = default;
    virtual ~BaseDDB() = default;

    Size _size;
    int _colDepth = 0;
    int _txFlags = 0; // TextureFlags
    Pointf _origin;
    Size _scaledSize;
    Common::GraphicFlip _flip = Common::kFlip_None;
    float _rotation = 0.f; // either in degrees or radians, depending on impl
    int _alpha = 255;
    Common::BlendMode _blendMode = Common::kBlend_Normal;
    uint32_t _shader = UINT32_MAX;
    int _red = 0, _green = 0, _blue = 0;
    int _tintSaturation = 0;
    int _lightLevel = 0;
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
enum TextureRenderHint
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
    // Viewport transformation matrix, used to apply to child batch viewports;
    // a separate matrix is required, as the coord origin is different from sprites
    glm::mat4 ViewportMat;
    // Batch color transformation
    SpriteColorTransform Color;
    // A flag telling to skip this batch fully (used in case associated resources are gone)
    bool Skip = false;

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

    ///////////////////////////////////////////////////////
    // Attributes
    //
    // Tells if this gfx driver has to redraw whole scene each time
    bool RequiresFullRedrawEachFrame() override { return true; }
    // Tells if this gfx driver uses GPU to transform sprites
    bool HasAcceleratedTransform() override { return true; }
    // Tells if this gfx driver draws on a virtual screen before rendering on real screen.
    // NOTE: although we do use ours, we do not let engine draw upon it;
    // only plugin handling are allowed to request our mem buffer
    // for compatibility reasons.
    bool UsesMemoryBackBuffer() override { return false; }

    ///////////////////////////////////////////////////////
    // Miscelaneous setup
    //
    // Sets values for global shader constants
    void SetGlobalShaderConstants(const GlobalShaderConstants &constants) override;

    ///////////////////////////////////////////////////////
    // Texture management
    // 
    // Creates new DDB and copy bitmap contents over
    IDriverDependantBitmap *CreateDDBFromBitmap(const Bitmap *bitmap, int txflags = kTxFlags_None) override;

    // Create texture data with the given parameters
    Texture *CreateTexture(int width, int height, int color_depth, int txflags = kTxFlags_None) override = 0;
    // Create texture and initialize its pixels from the given bitmap
    Texture *CreateTexture(const Bitmap *bmp, int txflags = kTxFlags_None) override;

    ///////////////////////////////////////////////////////
    // Preparing a scene
    // 
    // Sets stage screen parameters for the current batch.
    void SetStageScreen(const Size &sz, int x = 0, int y = 0) override;

    ///////////////////////////////////////////////////////
    // Additional operations
    //
    Bitmap *GetMemoryBackBuffer() override;
    void    SetMemoryBackBuffer(Bitmap *backBuffer) override;
    Bitmap *GetStageBackBuffer(bool mark_dirty) override;
    void    SetStageBackBuffer(Bitmap *backBuffer) override;
    bool    GetStageMatrixes(RenderMatrixes &rm) override;

protected:
    // Prepares bitmap to be applied to the texture, copies pixels to the provided buffer
    void BitmapToVideoMem(const Bitmap *bitmap, const TextureTile *tile,
                          uint8_t *dst_ptr, const int dst_pitch, const bool usingLinearFiltering);
    // Same but optimized for opaque source bitmaps which ignore transparent "mask color"
    void BitmapToVideoMemOpaque(const Bitmap *bitmap, const TextureTile *tile,
                                uint8_t *dst_ptr, const int dst_pitch);

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
    IDriverDependantBitmap *DoSpriteEvtCallback(int evt, intptr_t data, int &x, int &y);

    // Prepare and get fx item from the pool
    IDriverDependantBitmap *MakeFx(int r, int g, int b);
    // Resets fx pool counter
    void ResetFxPool();
    // Disposes all items in the fx pool
    void DestroyFxPool();

    // Stage matrixes are used to let plugins with hardware acceleration know model matrix;
    // these matrixes are filled compatible with each given renderer
    RenderMatrixes _stageMatrixes;

    // Global shader constants; these are applied to all shaders before each render pass.
    GlobalShaderConstants _globalShaderConst;

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

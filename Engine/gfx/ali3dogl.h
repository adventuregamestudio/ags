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
// OpenGL graphics factory
//
//=============================================================================

#ifndef __AGS_EE_GFX__ALI3DOGL_H
#define __AGS_EE_GFX__ALI3DOGL_H

#include <memory>

#include "glm/glm.hpp"

#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/gfxdriverbase.h"
#include "util/string.h"
#include "util/version.h"

#include "ogl_headers.h"

namespace AGS
{
namespace Engine
{

namespace OGL
{

using Common::Bitmap;
using Common::String;
using Common::Version;

struct OGLVECTOR2D
{
    float x = 0.f;
    float y = 0.f;
};

struct OGLCUSTOMVERTEX
{
    OGLVECTOR2D position;
    float tu = 0.f;
    float tv = 0.f;
};

struct OGLTextureTile : public TextureTile
{
    unsigned int texture = 0;
};

// Full OpenGL texture data
struct OGLTexture : Texture
{
    OGLCUSTOMVERTEX *_vertex = nullptr;
    OGLTextureTile *_tiles = nullptr;
    size_t _numTiles = 0;

    OGLTexture(const GraphicResolution &res, bool rt)
        : Texture(res, rt) {}
    OGLTexture(uint32_t id, const GraphicResolution &res, bool rt)
        : Texture(id, res, rt) {}
    ~OGLTexture();
    size_t GetMemSize() const override;
};

class OGLBitmap final : public BaseDDB
{
public:
    uint32_t GetRefID() const override { return _data->ID; }
    // Tells if this DDB has an actual render data assigned to it.
    bool IsValid() const override { return _data != nullptr; }
    // Attaches new texture data, sets basic render rules
    void AttachData(std::shared_ptr<Texture> txdata, int txflags) override
    {
        _data = std::static_pointer_cast<OGLTexture>(txdata);
        _size = _data->Res;
        _scaledSize = _size;
        _colDepth = _data->Res.ColorDepth;
        _txFlags = txflags;
    }
    // Detach any internal texture data from this DDB, make this an empty object
    void DetachData() override
    {
        _data = nullptr;
    }

    bool GetUseResampler() const override { return _useResampler; }
    void SetStretch(int width, int height, bool useResampler) override
    {
        _scaledSize = Size(width, height);
        _useResampler = useResampler;
    }
    // Rotation is set in degrees clockwise, stored converted to radians
    void SetRotation(float degrees) override { _rotation = -Common::Math::DegreesToRadians(degrees); }

    OGLBitmap(int width, int height, int colDepth, int txflags)
    {
        _size = Size(width, height);
        _scaledSize = _size;
        _colDepth = colDepth;
        _txFlags = txflags;
    }

    ~OGLBitmap() override;

    OGLTexture *GetTexture() const { return _data.get(); }
    std::shared_ptr<OGLTexture> GetSharedTexture() const { return _data; }
    GLuint GetFbo() const { return _fbo; }
    TextureRenderHint GetRenderHint() const { return _renderHint; }

    void SetTexture(std::shared_ptr<OGLTexture> &data, GLuint fbo = 0u, TextureRenderHint hint = kTxHint_Normal)
    {
        _data = data;
        _fbo = fbo;
        _renderHint = hint;
    }

private:
    // OpenGL texture data
    std::shared_ptr<OGLTexture> _data;
    // Optional frame buffer object (for rendering onto a texture)
    GLuint _fbo = 0u;
    // Render parameters
    TextureRenderHint _renderHint = kTxHint_Normal;
    bool _useResampler = false;
};

// OGL renderer's sprite batch
struct OGLSpriteBatch : VMSpriteBatch
{
    // Add anything OGL specific here
    // Optional render target's frame buffer
    uint32_t Fbo = 0u;

    OGLSpriteBatch() = default;
    OGLSpriteBatch(uint32_t id, const Rect &view, const glm::mat4 &matrix,
                   const glm::mat4 &vp_matrix, const SpriteColorTransform &color)
        : VMSpriteBatch(id, view, matrix, vp_matrix, color) {}
    OGLSpriteBatch(uint32_t id, OGLBitmap *render_target, const Rect view,
        const glm::mat4 &matrix, const glm::mat4 &vp_matrix, const SpriteColorTransform &color)
        : VMSpriteBatch(id, render_target, view, matrix, vp_matrix, color)
        , Fbo(render_target ? render_target->GetFbo() : 0u) {}
};

typedef SpriteDrawListEntry<OGLBitmap> OGLDrawListEntry;
typedef std::vector<OGLSpriteBatch>    OGLSpriteBatches;


class OGLDisplayModeList : public IGfxModeList
{
public:
    OGLDisplayModeList(const std::vector<DisplayMode> &modes)
        : _modes(modes)
    {
    }

    int GetModeCount() const override
    {
        return _modes.size();
    }

    bool GetMode(int index, DisplayMode &mode) const override
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

// Shader program and its variable references;
// the variables are rather specific for AGS use (sprite tinting).
struct ShaderProgram
{
    GLuint Program = 0;

    GLuint A_Position = 0;
    GLuint A_TexCoord = 0;

    GLuint MVPMatrix = 0;
    GLuint TextureId = 0;
    GLuint Alpha = 0;

    GLuint TintHSV = 0;
    GLuint TintAmount = 0;
    GLuint TintLuminance = 0;
    GLuint LightingAmount = 0;
};

class OGLGfxFilter;

class OGLGraphicsDriver : public VideoMemoryGraphicsDriver
{
public:
    OGLGraphicsDriver();
    ~OGLGraphicsDriver() override;

    ///////////////////////////////////////////////////////
    // Identification
    //
    // Gets graphic driver's identifier
    const char *GetDriverID() override { return "OGL"; }
    // Gets graphic driver's "friendly name"
    const char *GetDriverName() override { return "OpenGL"; }

    ///////////////////////////////////////////////////////
    // Attributes
    //
    // Tells if this gfx driver requires releasing render targets
    // in case of display mode change or reset.
    bool ShouldReleaseRenderTargets() override { return false; }

    ///////////////////////////////////////////////////////
    // Mode initialization
    //
    // Initialize given display mode
    bool SetDisplayMode(const DisplayMode &mode) override;
    // Updates previously set display mode, accomodating to the new screen size
    void UpdateDeviceScreen(const Size &screen_size) override;
    // Set the size of the native image size
    bool SetNativeResolution(const GraphicResolution &native_res) override;
    // Set game render frame and translation
    bool SetRenderFrame(const Rect &dst_rect) override;
    // Report which display's color depth option is best suited for the given native color depth
    int GetDisplayDepthForNativeDepth(int native_color_depth) const override;
    // Gets a list of supported fullscreen display modes
    IGfxModeList *GetSupportedModeList(int display_index, int color_depth) override;
    // Tells if the given display mode supported
    bool IsModeSupported(const DisplayMode &mode) override;
    // Gets currently set scaling filter
    PGfxFilter GetGraphicsFilter() const override;

    typedef std::shared_ptr<OGLGfxFilter> POGLFilter;
    void SetGraphicsFilter(POGLFilter filter);
    void UnInit();

    ///////////////////////////////////////////////////////
    // Miscelaneous setup
    //
    bool DoesSupportVsyncToggle() override { return _capsVsync; }
    void RenderSpritesAtScreenResolution(bool enabled) override;
    void UseSmoothScaling(bool enabled) override { _smoothScaling = enabled; }
    bool SupportsGammaControl() override;
    void SetGamma(int newGamma) override;

    ///////////////////////////////////////////////////////
    // Texture management
    //
    // Gets closest recommended bitmap format (currently - only color depth) for the given original format.
    int  GetCompatibleBitmapFormat(int color_depth) override;
    // Returns available texture memory in bytes, or 0 if this query is not supported
    uint64_t GetAvailableTextureMemory() override;
    // Creates a "raw" DDB, without pixel initialization.
    IDriverDependantBitmap* CreateDDB(int width, int height, int color_depth, int txflags) override;
    // Creates DDB intended to be used as a render target (allow render other DDBs on it).
    IDriverDependantBitmap* CreateRenderTargetDDB(int width, int height, int color_depth, int txflags) override;
    // Updates DBB using the given bitmap
    void UpdateDDBFromBitmap(IDriverDependantBitmap* ddb, const Bitmap *bitmap) override;
    // Destroy the DDB; note that this does not dispose the texture unless there's no more refs to it
    void DestroyDDB(IDriverDependantBitmap* ddb) override;

    // Create texture data with the given parameters
    Texture *CreateTexture(int width, int height, int color_depth, int txflags) override;
    // Update texture data from the given bitmap
    void UpdateTexture(Texture *txdata, const Bitmap *bitmap, bool opaque) override;
    // Retrieve shared texture data object from the given DDB
    std::shared_ptr<Texture> GetTexture(IDriverDependantBitmap *ddb) override;

    ///////////////////////////////////////////////////////
    // Preparing a scene
    //
    // Adds sprite to the active batch, providing it's origin position
    void DrawSprite(int x, int y, IDriverDependantBitmap* ddb) override
         { DrawSprite(x, y, x, y, ddb); }
    // Adds sprite to the active batch, providing it's origin position and auxiliary
    // position of the left-top image corner in the same coordinate space
    void DrawSprite(int ox, int oy, int ltx, int lty, IDriverDependantBitmap* ddb) override;
    // Adds fade overlay fx to the active batch
    void SetScreenFade(int red, int green, int blue) override;
    // Adds tint overlay fx to the active batch
    void SetScreenTint(int red, int green, int blue) override;
    // Redraw last draw lists, optionally filtering specific batches
    void RedrawLastFrame(uint32_t batch_skip_filter) override;

    ///////////////////////////////////////////////////////
    // Rendering and presenting
    // 
    // Clears the screen rectangle. The coordinates are expected in the **native game resolution**.
    void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) override;
    // Renders draw lists and presents to screen.
    void Render() override;
    // Renders and presents with additional final offset and flip.
    void Render(int xoff, int yoff, Common::GraphicFlip flip) override;
    // Renders draw lists onto the provided texture.
    void Render(IDriverDependantBitmap *target) override;
    // Renders draw lists to backbuffer, but does not call present.
    void RenderToBackBuffer() override;

    ///////////////////////////////////////////////////////
    // Additional operations
    //
    // Copies contents of the game screen into the DDB.
    void GetCopyOfScreenIntoDDB(IDriverDependantBitmap *target, uint32_t batch_skip_filter = 0u) override;
    // Copies contents of the last rendered game frame into bitmap using simple blit or pixel copy.
    bool GetCopyOfScreenIntoBitmap(Bitmap *destination, const Rect *src_rect, bool at_native_res,
        GraphicResolution *want_fmt, uint32_t batch_skip_filter = 0u) override;

protected:
    bool SetVsyncImpl(bool vsync, bool &vsync_res) override;

    // Create DDB using preexisting texture data
    IDriverDependantBitmap *CreateDDB(std::shared_ptr<Texture> txdata, int txflags) override;

    size_t GetLastDrawEntryIndex() override { return _spriteList.size(); }

private:
    ///////////////////////////////////////////////////////
    // Mode initialization: implementation
    //
    // Sets up GL objects not related to particular display mode
    bool FirstTimeInit();
    // Initializes Gl rendering context
    bool InitGlScreen(const DisplayMode &mode);
    bool CreateWindowAndGlContext(const DisplayMode &mode);
    void DeleteWindowAndGlContext();
    // Sets up general rendering parameters
    void InitGlParams(const DisplayMode &mode);
    void SetupDefaultVertices();
    // Test if rendering to texture is supported
    void TestRenderToTexture();
    // Create shader programs for sprite tinting and changing light level
    bool CreateShaders();
    // Configure native resolution render target, that is used in render-to-texture mode
    void SetupNativeTarget();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();
    void CreateVirtualScreen();
    void SetupViewport();

    ///////////////////////////////////////////////////////
    // Texture management: implementation
    //
    void AdjustSizeToNearestSupportedByCard(int *width, int *height);
    void UpdateTextureRegion(OGLTextureTile *tile, const Bitmap *bitmap, bool opaque);

    ///////////////////////////////////////////////////////
    // Preparing a scene: implementation
    //
    void InitSpriteBatch(size_t index, const SpriteBatchDesc &desc) override;
    void ResetAllBatches() override;
    // Backup all draw lists in the temp storage
    void BackupDrawLists();
    // Restore draw lists from the temp storage
    void RestoreDrawLists();
    // Deletes draw list backups
    void ClearDrawBackups();
    // Mark certain sprite batches to be skipped at the next render
    void FilterSpriteBatches(uint32_t skip_filter);

    ///////////////////////////////////////////////////////
    // Rendering and presenting: implementation
    //
    // TODO: find a way to merge this with Render Targets from sprite batches,
    // have a SINGLE STACK of "render target states", where backbuffer is at the bottom
    struct BackbufferState
    {
        GLuint Fbo = 0u;
        // FIXME: replace RendSize with explicit render coordinate offset? merge with ortho matrix?
        Size SurfSize; // actual surface size
        Size RendSize; // coordinate grid size (for centering sprites)
        // Viewport and scissor rect, in OpenGL screen coordinates (0,0 is at left-bottom)
        Rect Viewport;
        glm::mat4 Projection;
        PlaneScaling Scaling;
        int Filter = 0;
        int TxClamp = 0;

        BackbufferState() = default;
        BackbufferState(GLuint fbo, const Size &surf_size, const Size &rend_size,
                        const Rect &view, const glm::mat4 &proj,
                        const PlaneScaling &scale, int filter, int txclamp)
            : Fbo(fbo), SurfSize(surf_size), RendSize(rend_size)
            , Viewport(view), Projection(proj)
            , Scaling(scale), Filter(filter), TxClamp(txclamp) {}
        ~BackbufferState() = default;
    };

    void RenderAndPresent(bool clearDrawListAfterwards);
    void RenderImpl(bool clearDrawListAfterwards);
    void RenderToSurface(BackbufferState *state, bool clearDrawListAfterwards);
    // Set current backbuffer state, which properties are used when refering to backbuffer
    // TODO: find a good way to merge with SetRenderTarget
    void SetBackbufferState(BackbufferState *state, bool clear);
    // Sets the scissor (render clip), clip rect is passed in the "native" coordinates.
    // Optionally pass surface_size if the rendering is done to texture, in native coords,
    // otherwise we assume it is set on a whole screen, scaled to the screen coords.
    void SetScissor(const Rect &clip, bool render_on_texture, const Size &surface_size);
    // Configures rendering mode for the render target, depending on its properties
    void SetRenderTarget(const OGLSpriteBatch *batch, Size &surface_sz, Size &rend_sz, glm::mat4 &projection, bool clear);
    void RenderSpriteBatches();
    size_t RenderSpriteBatch(const OGLSpriteBatch &batch, size_t from, const glm::mat4 &projection,
                             const Size &rend_sz);
    void RenderSprite(const OGLDrawListEntry *entry, const glm::mat4 &projection, const glm::mat4 &matGlobal,
                      const SpriteColorTransform &color, const Size &rend_sz);
    // Renders given texture onto the current render target
    void RenderTexture(OGLBitmap *bmpToDraw, int draw_x, int draw_y,
                       const glm::mat4 &projection, const glm::mat4 &matGlobal,
                       const SpriteColorTransform &color, const Size &rend_sz);

    // Sets uniform blend settings, same for both RGB and alpha component
    void SetBlendOpUniform(GLenum blend_op, GLenum src_factor, GLenum dst_factor);
    // Sets blend settings for RGB only, and keeps previously set alpha blend settings
    void SetBlendOpRGB(GLenum rgb_op, GLenum srgb_factor, GLenum drgb_factor);
    // Sets blend settings with separate op for alpha, and saves used alpha params
    void SetBlendOpRGBAlpha(GLenum rgb_op, GLenum srgb_factor, GLenum drgb_factor,
                            GLenum alpha_op, GLenum sa_factor, GLenum da_factor);


    bool _firstTimeInit = false;
    SDL_Window *_sdlWindow = nullptr;
    SDL_GLContext _sdlGlContext = nullptr;
    OGLCUSTOMVERTEX defaultVertices[4];
    bool _smoothScaling = false;
    POGLFilter _filter{};

    ShaderProgram _tintShader;
    ShaderProgram _lightShader;
    ShaderProgram _transparencyShader;
    ShaderProgram _darkenbyAlphaShader;
    ShaderProgram _lightenByAlphaShader;

    int device_screen_physical_width;
    int device_screen_physical_height;

    // Final fbo, depends on platform we run on
    GLint _screenFramebuffer = 0u;
    // Capability flags
    bool _glCapsNonPowerOfTwo = false;
    // These two flags define whether driver can, and should (respectively)
    // render sprites to texture, and then texture to screen, as opposed to
    // rendering to screen directly. This is known as supersampling mode
    bool _canRenderToTexture {};
    bool _doRenderToTexture {};
    // Texture for rendering in native resolution
    OGLBitmap *_nativeSurface = nullptr;

    BackbufferState _screenBackbuffer;
    BackbufferState _nativeBackbuffer;
    BackbufferState *_currentBackbuffer = nullptr;

    // Sprite batches (parent scene nodes)
    OGLSpriteBatches _spriteBatches;
    // List of sprites to render
    std::vector<OGLDrawListEntry> _spriteList;
    // TODO: these draw list backups are needed only for the fade-in/out effects
    // find out if it's possible to reimplement these effects in main drawing routine.
    // TODO: if not above, refactor and implement Desc backup in the base class
    SpriteBatchDescs _backupBatchDescs;
    std::vector<std::pair<size_t, size_t>> _backupBatchRange;
    OGLSpriteBatches _backupBatches;
    std::vector<OGLDrawListEntry> _backupSpriteList;

    // Saved blend settings
    struct BlendOpState
    {
        GLenum Op{}, Src{}, Dst{};
        BlendOpState() = default;
        BlendOpState(GLenum op, GLenum src, GLenum dst)
            : Op(op), Src(src), Dst(dst) {}
    };

    // Saved alpha channel blend settings for the current render target
    BlendOpState _rtBlendAlpha{};
    // Saved current blend settings exclusive for alpha channel; for convenience,
    // because GL does not have functions for setting ONLY RGB or ONLY alpha ops.
    BlendOpState _blendAlpha{};
};


class OGLGraphicsFactory : public GfxDriverFactoryBase<OGLGraphicsDriver, OGLGfxFilter>
{
public:
    ~OGLGraphicsFactory() override;

    size_t               GetFilterCount() const override;
    const GfxFilterInfo *GetFilterInfo(size_t index) const override;
    String               GetDefaultFilterID() const override;

    static OGLGraphicsFactory   *GetFactory();

private:
    OGLGraphicsDriver   *EnsureDriverCreated() override;
    OGLGfxFilter        *CreateFilter(const String &id) override;

    static OGLGraphicsFactory *_factory;
};

} // namespace OGL
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DOGL_H

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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

class OGLBitmap : public BaseDDB
{
public:
    uint32_t GetRefID() const override { return _data->ID; }

    int  GetAlpha() const override { return _alpha; }
    void SetAlpha(int alpha) override { _alpha = alpha; }
    void SetFlippedLeftRight(bool isFlipped) override { _flipped = isFlipped; }
    void SetStretch(int width, int height, bool useResampler = true) override
    {
        _stretchToWidth = width;
        _stretchToHeight = height;
        _useResampler = useResampler;
    }
    // Rotation is set in degrees, clockwise
    void SetRotation(float degrees) override { _rotation = -Common::Math::DegreesToRadians(degrees); }
    void SetLightLevel(int lightLevel) override { _lightLevel = lightLevel; }
    void SetTint(int red, int green, int blue, int tintSaturation) override 
    {
        _red = red;
        _green = green;
        _blue = blue;
        _tintSaturation = tintSaturation;
    }
    void SetBlendMode(Common::BlendMode blendMode) override { _blendMode = blendMode; }

    // Tells if this DDB has an actual render data assigned to it.
    bool IsValid() override { return _data != nullptr; }
    // Attaches new texture data, sets basic render rules
    void AttachData(std::shared_ptr<Texture> txdata, bool opaque) override
    {
        _data = std::static_pointer_cast<OGLTexture>(txdata);
        _width = _stretchToWidth = _data->Res.Width;
        _height = _stretchToHeight = _data->Res.Height;
        _colDepth = _data->Res.ColorDepth;
    }
    // Detach any internal texture data from this DDB, make this an empty object
    void DetachData() override
    {
        _data = nullptr;
    }

    // OpenGL texture data
    std::shared_ptr<OGLTexture> _data;
    // Optional frame buffer object (for rendering onto a texture)
    unsigned int _fbo {};
    TextureHint _renderHint = kTxHint_Normal;

    // Drawing parameters
    bool _flipped;
    int _stretchToWidth, _stretchToHeight;
    float _rotation;
    bool _useResampler;
    int _red, _green, _blue;
    int _tintSaturation;
    int _lightLevel;
    int _alpha;
    Common::BlendMode _blendMode;

    OGLBitmap(int width, int height, int colDepth, bool opaque)
    {
        _width = width;
        _height = height;
        _colDepth = colDepth;
        _flipped = false;
        _stretchToWidth = width;
        _stretchToHeight = height;
        _originX = _originY = 0.f;
        _useResampler = false;
        _rotation = 0;
        _red = _green = _blue = 0;
        _tintSaturation = 0;
        _lightLevel = 0;
        _alpha = 255;
        _opaque = opaque;
        _blendMode = Common::kBlend_Normal;
    }

    int GetWidthToRender() const { return _stretchToWidth; }
    int GetHeightToRender() const { return _stretchToHeight; }

    ~OGLBitmap() override;
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
        , Fbo(render_target ? render_target->_fbo : 0u) {}
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
    GLuint Arg[4] {};

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
    const char *GetDriverID() override { return "OGL"; }
    const char *GetDriverName() override { return "OpenGL"; }

    bool ShouldReleaseRenderTargets() override { return false; }

    void SetTintMethod(TintMethod method) override;
    bool SetDisplayMode(const DisplayMode &mode) override;
    void UpdateDeviceScreen(const Size &screen_size) override;
    bool SetNativeResolution(const GraphicResolution &native_res) override;
    bool SetRenderFrame(const Rect &dst_rect) override;
    int GetDisplayDepthForNativeDepth(int native_color_depth) const override;
    IGfxModeList *GetSupportedModeList(int color_depth) override;
    bool IsModeSupported(const DisplayMode &mode) override;
    PGfxFilter GetGraphicsFilter() const override;
    void UnInit();
    // Clears the screen rectangle. The coordinates are expected in the **native game resolution**.
    void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse) override;
    int  GetCompatibleBitmapFormat(int color_depth) override;
    size_t GetAvailableTextureMemory() override;

    IDriverDependantBitmap* CreateDDB(int width, int height, int color_depth, bool opaque) override;
    IDriverDependantBitmap* CreateRenderTargetDDB(int width, int height, int color_depth, bool opaque) override;
    void UpdateDDBFromBitmap(IDriverDependantBitmap* ddb, Bitmap *bitmap) override;
    void DestroyDDB(IDriverDependantBitmap* ddb) override;
    
    // Create texture data with the given parameters
    Texture *CreateTexture(int width, int height, int color_depth, bool opaque, bool as_render_target = false) override;
    // Update texture data from the given bitmap
    void UpdateTexture(Texture *txdata, Bitmap *bitmap, bool opaque) override;
    // Retrieve shared texture data object from the given DDB
    std::shared_ptr<Texture> GetTexture(IDriverDependantBitmap *ddb) override;

    void DrawSprite(int x, int y, IDriverDependantBitmap* ddb) override
         { DrawSprite(x, y, x, y, ddb); }
    void DrawSprite(int ox, int oy, int ltx, int lty, IDriverDependantBitmap* ddb) override;
    void RenderToBackBuffer() override;
    void Render() override;
    void Render(int xoff, int yoff, Common::GraphicFlip flip) override;
    bool GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt) override;
    bool DoesSupportVsyncToggle() override { return _capsVsync; }
    void RenderSpritesAtScreenResolution(bool enabled, int supersampling) override;
    void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) override;
    void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue) override;
    void BoxOutEffect(bool blackingOut, int speed, int delay) override;
    bool SupportsGammaControl() override;
    void SetGamma(int newGamma) override;
    void UseSmoothScaling(bool enabled) override { _smoothScaling = enabled; }
    void SetScreenFade(int red, int green, int blue) override;
    void SetScreenTint(int red, int green, int blue) override;

    typedef std::shared_ptr<OGLGfxFilter> POGLFilter;

    void SetGraphicsFilter(POGLFilter filter);

    OGLGraphicsDriver();
    ~OGLGraphicsDriver() override;

protected:
    bool SetVsyncImpl(bool vsync, bool &vsync_res) override;

    // Create DDB using preexisting texture data
    IDriverDependantBitmap *CreateDDB(std::shared_ptr<Texture> txdata, bool opaque) override;

    size_t GetLastDrawEntryIndex() override { return _spriteList.size(); }

private:
    POGLFilter _filter {};

    bool _firstTimeInit;
    SDL_Window *_sdlWindow = nullptr;
    SDL_GLContext _sdlGlContext = nullptr;
    // Position of backbuffer texture in world space
    GLfloat _backbuffer_vertices[8] {};
    // Relative position of source image on the backbuffer texture,
    // in local coordinates
    GLfloat _backbuffer_texture_coordinates[8];
    OGLCUSTOMVERTEX defaultVertices[4];
    bool _smoothScaling;
    bool _legacyPixelShader;

    ShaderProgram _tintShader;
    ShaderProgram _lightShader;
    ShaderProgram _transparencyShader;

    int device_screen_physical_width;
    int device_screen_physical_height;

    // Viewport and scissor rect, in OpenGL screen coordinates (0,0 is at left-bottom)
    Rect _viewportRect {};

    // These two flags define whether driver can, and should (respectively)
    // render sprites to texture, and then texture to screen, as opposed to
    // rendering to screen directly. This is known as supersampling mode
    bool _can_render_to_texture {};
    bool _do_render_to_texture {};
    // Backbuffer texture multiplier, used to determine a size of texture
    // relative to the native game size.
    int _super_sampling {};
    unsigned int _backbuffer {};
    unsigned int _fbo {};
    // Size of the backbuffer drawing area, equals to native size
    // multiplied by _super_sampling
    Size _backRenderSize {};
    // Actual size of the backbuffer texture, created by OpenGL
    Size _backTextureSize {};

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

    // Saved blend settings exclusive for alpha channel; for convenience,
    // because GL does not have functions for setting ONLY RGB or ONLY alpha ops.
    GLenum _blendOpAlpha{};
    GLenum _blendSrcAlpha{};
    GLenum _blendDstAlpha{};


    void InitSpriteBatch(size_t index, const SpriteBatchDesc &desc) override;
    void ResetAllBatches() override;

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
    // Test if supersampling should be allowed with the current setup
    void TestSupersampling();
    // Create shader programs for sprite tinting and changing light level
    bool CreateShaders();
    // Configure backbuffer texture, that is used in render-to-texture mode
    void SetupBackbufferTexture();
    void DeleteBackbufferTexture();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();
    void AdjustSizeToNearestSupportedByCard(int *width, int *height);
    void UpdateTextureRegion(OGLTextureTile *tile, Bitmap *bitmap, bool opaque);
    void CreateVirtualScreen();
    void do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
    void _renderSprite(const OGLDrawListEntry *entry, const glm::mat4 &projection, const glm::mat4 &matGlobal,
        const SpriteColorTransform &color, const Size &surface_size);
    void SetupViewport();
    // Converts rectangle in top->down coordinates into OpenGL's native bottom->up coordinates
    Rect ConvertTopDownRect(const Rect &top_down_rect, int surface_height);
    // Sets uniform GL blend settings, same for both RGB and alpha component
    void SetBlendOpUniform(GLenum blend_op, GLenum src_factor, GLenum dst_factor);
    // Sets GL blend settings for RGB only, and keeps saved alpha blend settings
    void SetBlendOpRGB(GLenum rgb_op, GLenum srgb_factor, GLenum drgb_factor);
    // Sets GL blend settings with separate op for alpha, and saves used alpha params
    void SetBlendOpRGBAlpha(GLenum rgb_op, GLenum srgb_factor, GLenum drgb_factor,
        GLenum alpha_op, GLenum sa_factor, GLenum da_factor);

    // Backup all draw lists in the temp storage
    void BackupDrawLists();
    // Restore draw lists from the temp storage
    void RestoreDrawLists();
    // Deletes draw list backups
    void ClearDrawBackups();
    void _render(bool clearDrawListAfterwards);
    // Sets the scissor (render clip), clip rect is passed in the "native" coordinates.
    // Optionally pass surface_size if the rendering is done to texture, in native coords,
    // otherwise we assume it is set on a whole screen, scaled to the screen coords.
    void SetScissor(const Rect &clip, bool render_on_texture, const Size &surface_size);
    // Configures rendering mode for the render target, depending on its properties
    void SetRenderTarget(const OGLSpriteBatch *batch, Size &surface_sz, glm::mat4 &projection, bool clear);
    void RenderSpriteBatches(const glm::mat4 &projection);
    size_t RenderSpriteBatch(const OGLSpriteBatch &batch, size_t from, const glm::mat4 &projection,
        const Size &surface_size);
    void _reDrawLastFrame();
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

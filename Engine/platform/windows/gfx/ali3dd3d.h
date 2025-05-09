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
// Direct3D graphics factory
//
//=============================================================================
#ifndef __AGS_EE_GFX__ALI3DD3D_H
#define __AGS_EE_GFX__ALI3DD3D_H

#include "core/platform.h"

#if ! AGS_PLATFORM_OS_WINDOWS
#error This file should only be included on the Windows build
#endif

#if defined (WIN_XP_COMPAT)
#define DIRECT3D_USE_D3DCOMPILER (0)
#else
#define DIRECT3D_USE_D3DCOMPILER (1)
#endif

#define NOMINMAX
#include <memory>
#define NOMINMAX
#define BITMAP WINDOWS_BITMAP
#include <d3d9.h>
#if (DIRECT3D_USE_D3DCOMPILER)
#include <d3dcommon.h>
#include <d3dcompiler.h>
#endif
#undef BITMAP
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/gfxdriverbase.h"
#include "util/library.h"
#include "util/math.h"
#include "util/smart_ptr.h"
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

// Declare smart ptr for common IDirect3D interfaces
typedef ComPtr<IDirect3D9> D3DPtr;
typedef ComPtr<IDirect3DDevice9> D3DDevicePtr;
typedef ComPtr<IDirect3DSurface9> D3DSurfacePtr;
typedef ComPtr<IDirect3DTexture9> D3DTexturePtr;
typedef ComPtr<IDirect3DVertexBuffer9> D3DVertexBufferPtr;
typedef ComPtr<IDirect3DPixelShader9> D3DPixelShaderPtr;


struct D3DTextureTile : public TextureTile
{
    D3DTexturePtr texture;
};

// Full Direct3D texture data
struct D3DTexture : Texture
{
    D3DVertexBufferPtr _vertex;
    std::vector<D3DTextureTile> _tiles;

    D3DTexture(const GraphicResolution &res, bool rt)
        : Texture(res, rt) {}
    D3DTexture(uint32_t id, const GraphicResolution &res, bool rt)
        : Texture(id, res, rt) {}
    ~D3DTexture() = default;
    size_t GetMemSize() const override;
};

class D3DBitmap final : public BaseDDB
{
public:
    uint32_t GetRefID() const override { return _data->ID; }
    // Tells if this DDB has an actual render data assigned to it.
    bool IsValid() const override { return _data != nullptr; }
    // Attaches new texture data, sets basic render rules
    void AttachData(std::shared_ptr<Texture> txdata, int txflags) override
    {
        _data = std::static_pointer_cast<D3DTexture>(txdata);
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
    // Releases internal texture data only, keeping the base struct
    void ReleaseTextureData();

    bool GetUseResampler() const override { return _useResampler; }
    void SetStretch(int width, int height, bool useResampler) override
    {
        _scaledSize = Size(width, height);
        _useResampler = useResampler;
    }
    // Rotation is set in degrees clockwise, stored converted to radians
    void SetRotation(float degrees) override { _rotation = -Common::Math::DegreesToRadians(degrees); }

    D3DBitmap(int width, int height, int colDepth, int txflags)
    {
        _size = Size(width, height);
        _scaledSize = _size;
        _colDepth = colDepth;
        _txFlags = txflags;
    }

    ~D3DBitmap() override = default;

    D3DTexture *GetTexture() const { return _data.get(); }
    std::shared_ptr<D3DTexture> GetSharedTexture() const { return _data; }
    const D3DSurfacePtr &GetRenderSurface() const { return _renderSurface; }
    TextureRenderHint GetRenderHint() const { return _renderHint; }

    void SetTexture(std::shared_ptr<D3DTexture> data, const D3DSurfacePtr &d3d_surface = {}, TextureRenderHint hint = kTxHint_Normal)
    {
        _data = data;
        _renderSurface = d3d_surface;
        _renderHint = hint;
    }

private:
    // Direct3D texture data
    std::shared_ptr<D3DTexture> _data;
    // Optional surface for rendering onto a texture
    D3DSurfacePtr _renderSurface;
    // Render parameters
    TextureRenderHint _renderHint = kTxHint_Normal;
    bool _useResampler = false;
};

class D3DShader final : public BaseShader
{
public:
    struct ProgramData;

    D3DShader(const String &name);
    D3DShader(const String &name, D3DShader::ProgramData &&data);
    D3DShader(const String &name, const D3DShader &copy_shader);
    ~D3DShader() = default;

    // Get a number of registered constants
    uint32_t GetConstantCount() override;
    // Get a registered constant's name, by its sequential index
    String GetConstantName(uint32_t iter_index) override;
    // Get a registered constant's location, by its sequential index;
    // returns UINT32_MAX if no such constant exists
    uint32_t GetConstantByIndex(uint32_t iter_index) override;
    // Looks up for the constant in a shader. Returns a valid index if such shader is registered,
    // and constant is present in that shader, or UINT32_MAX on failure.
    uint32_t GetConstantByName(const String &const_name) override;
    // Reset all shader constants to zero
    void ResetConstants() override;

    // A single constant register size *in floats*
    static const uint32_t ConstantSize = 4u;
    // A cap of the number of constant registers that we support (0..N)
    static const uint32_t ConstantCap = 12u;
    // A place in buffer where we will write non-applied values;
    // this is just to streamline the process and don't litter the code with checks
    static const uint32_t NullConstantIndex = ConstantCap;

    // Shader data wrapped in a struct, for easier referencing
    struct ProgramData
    {
        D3DPixelShaderPtr ShaderPtr;

        struct Register
        {
            uint32_t Index = 0u;  // register index
            uint32_t Off = 0u; // actual memory offset

            Register() = default;
            Register(uint32_t reg_index) { *this = reg_index; }
            Register &operator=(uint32_t reg_index)
            {
                Index = reg_index;
                Off = reg_index * ConstantSize;
                return *this;
            }
        };
        //---------------------------------------
        // Fragment shader:
        // Standard constants register number
        Register Time;         // real time
        Register GameFrame;    // game frame
        Register TextureDim;   // texture 0 dimensions
        Register Alpha;        // requested global alpha
        Register OutputDim;    // output dimensions

        // Specialized constants for built-in shaders
        Register TintHSV;      // float4
        Register TintAlphaLight; // float4

        // Constants table: maps constant name to the index/register
        // in the compiled shader
        std::unordered_map<String, uint32_t> Constants;
        // Ordered list of constants: lets iterate over them using
        // a sequential index. Order is by the register (location)
        // in shader program memory.
        std::vector<std::pair<uint32_t, String>> ConstantsOrdered;
    };

    bool IsValid() const { return _data.ShaderPtr != nullptr; }
    const D3DShader::ProgramData &GetData() const { return _data; }

private:
    ProgramData _data;
};

class D3DShaderInstance final : public BaseShaderInstance
{
public:
    D3DShaderInstance(D3DShader *shader, const String &name);
    ~D3DShaderInstance() = default;

    // Returns a IGraphicShader referenced by this shader instance
    IGraphicShader *GetShader() override { return _shader; }

    // Sets shader constant, using constant's index (returned by GetShaderConstant)
    void SetShaderConstantF(uint32_t const_index, float value) override;
    void SetShaderConstantF2(uint32_t const_index, float x, float y) override;
    void SetShaderConstantF3(uint32_t const_index, float x, float y, float z) override;
    void SetShaderConstantF4(uint32_t const_index, float x, float y, float z, float w) override;

    // Gets the allocated size of the constants data
    size_t GetConstantDataSize() override;
    // Gets array of constants data, where each constant is represented as 4 floats
    void GetConstantData(std::vector<float> &data) override;

    const D3DShader::ProgramData &GetShaderData() const { return _shader->GetData(); }
    float *GetConstantData() { return _constantData.data(); }
    size_t GetConstantDataSize() const { return _constantData.size(); }

private:
    // FIXME: provide reference counting of shader ptr
    D3DShader *_shader = nullptr;
    // Constant buffer data, applied each time a shader is used in render
    std::vector<float> _constantData;
};

class D3DGfxModeList final : public IGfxModeList
{
public:
    D3DGfxModeList(const D3DPtr &direct3d, int display_index, D3DFORMAT d3dformat);
    ~D3DGfxModeList() = default;

    int GetModeCount() const override
    {
        return _modeCount;
    }

    bool GetMode(int index, DisplayMode &mode) const override;

private:
    D3DPtr      _direct3d;
    int         _displayIndex = 0;
    UINT        _adapterIndex = 0u;
    D3DFORMAT   _pixelFormat;
    int         _modeCount = 0;
};

struct CUSTOMVERTEX
{
    D3DVECTOR   position; // The position.
    D3DVECTOR   normal;
    FLOAT       tu, tv;   // The texture coordinates.
};

// D3D renderer's sprite batch
struct D3DSpriteBatch : VMSpriteBatch
{
    // Add anything D3D specific here
    // Optional render target's surface
    D3DSurfacePtr RenderSurface;

    D3DSpriteBatch() = default;
    D3DSpriteBatch(uint32_t id, const Rect &view, const glm::mat4 &matrix,
        const glm::mat4 &vp_matrix, const SpriteColorTransform &color)
        : VMSpriteBatch(id, view, matrix, vp_matrix, color) {}
    D3DSpriteBatch(uint32_t id, D3DBitmap *render_target, const Rect &view,
        const glm::mat4 &matrix, const glm::mat4 &vp_matrix,
        const SpriteColorTransform &color)
        : VMSpriteBatch(id, render_target, view, matrix, vp_matrix, color)
    {
        if (render_target)
            RenderSurface = render_target->GetRenderSurface();
    }
};

typedef SpriteDrawListEntry<D3DBitmap> D3DDrawListEntry;
typedef std::vector<D3DSpriteBatch>    D3DSpriteBatches;


class D3DGraphicsDriver : public VideoMemoryGraphicsDriver
{
public:
    D3DGraphicsDriver(const D3DPtr &d3d);
    ~D3DGraphicsDriver() override;

    ///////////////////////////////////////////////////////
    // Identification
    //
    // Gets graphic driver's identifier
    const char *GetDriverID() override { return "D3D9"; }
    // Gets graphic driver's "friendly name"
    const char *GetDriverName() override { return "Direct3D 9"; }

    ///////////////////////////////////////////////////////
    // Attributes
    //
    // Tells if this gfx driver requires releasing render targets
    // in case of display mode change or reset.
    bool ShouldReleaseRenderTargets() override { return true; }

    ///////////////////////////////////////////////////////
    // Mode initialization
    //
    // Initialize given display mode
    bool SetDisplayMode(const DisplayMode &mode) override;
    // Updates previously set display mode, accomodating to the new screen size
    void UpdateDeviceScreen(const Size &screen_sz) override;
    // Set the size of the native image size
    bool SetNativeResolution(const GraphicResolution &native_res) override;
    // Set game render frame and translation
    bool SetRenderFrame(const Rect &dst_rect) override;
    // Report which display's color depth option is best suited for the given native color depth
    int  GetDisplayDepthForNativeDepth(int native_color_depth) const override;
    // Gets a list of supported fullscreen display modes
    IGfxModeList *GetSupportedModeList(int display_index, int color_depth) override;
    // Tells if the given display mode supported
    bool IsModeSupported(const DisplayMode &mode) override;
    // Gets currently set scaling filter
    PGfxFilter GetGraphicsFilter() const override;

    typedef std::shared_ptr<D3DGfxFilter> PD3DFilter;
    void SetGraphicsFilter(PD3DFilter filter);
    void UnInit();

    ///////////////////////////////////////////////////////
    // Miscelaneous setup
    //
    bool DoesSupportVsyncToggle() override { return _capsVsync; }
    void RenderSpritesAtScreenResolution(bool enabled) override { _renderAtScreenRes = enabled; };
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
    // Updates DBB using the given bitmap.
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
    // Shader management
    //
    // Returns the expected file extension for the precompiled shader
    const char *GetShaderPrecompiledExtension() override { return "fxo"; }
    // Returns the expected file extension for the shader source
    const char *GetShaderSourceExtension() override { return "hlsl"; }
    // Returns the expected file extension for the shader definition file
    const char *GetShaderDefinitionExtension() override { return "d3ddef"; }
    // Creates shader program from the source code, registers it under given name,
    // returns IGraphicShader, or null on failure.
    IGraphicShader *CreateShaderProgram(const String &name, const char *fragment_shader_src, const ShaderDefinition *def) override;
    // Creates shader program from the compiled data, registers it under given name,
    // returns IGraphicShader, or null on failure.
    IGraphicShader *CreateShaderProgram(const String &name, const std::vector<uint8_t> &compiled_data, const ShaderDefinition *def) override;
    // Deletes particular shader program.
    void DeleteShaderProgram(IGraphicShader *shader) override;
    // Creates shader instance for the given shader.
    IShaderInstance *CreateShaderInstance(IGraphicShader *shader, const String &name) override;
    // Deletes particular shader instance
    void DeleteShaderInstance(IShaderInstance *shader_inst) override;

    ///////////////////////////////////////////////////////
    // Preparing a scene
    // 
    // Adds sprite to the active batch, providing it's origin position
    void DrawSprite(int x, int y, IDriverDependantBitmap* ddb) override
         { DrawSprite(x, y, x, y, ddb); }
    // Adds sprite to the active batch, providing it's origin position and auxiliary
    // position of the left-top image corner in the same coordinate space
    void DrawSprite(int ox, int oy, int ltx, int lty, IDriverDependantBitmap* bitmap) override;
    // Adds fade overlay fx to the active batch
    void SetScreenFade(int red, int green, int blue) override;
    // Adds tint overlay fx to the active batch
    // TODO: redesign this to allow various post-fx per sprite batch?
    void SetScreenTint(int red, int green, int blue) override;
    // Sets a shader to be applied to the whole screen as a post fx
    void SetScreenShader(IShaderInstance *shinst) override;
    // Redraw saved draw lists, optionally filtering specific batches
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

    // Clears screen rect, coordinates are expected in display resolution
    void ClearScreenRect(const Rect &r, RGB *colorToUse);

    ///////////////////////////////////////////////////////
    // Additional operations
    //
    // Copies contents of the game screen into the DDB
    void GetCopyOfScreenIntoDDB(IDriverDependantBitmap *target, uint32_t batch_skip_filter = 0u) override;
    // Copies contents of the last rendered game frame into bitmap using simple blit or pixel copy.
    bool GetCopyOfScreenIntoBitmap(Bitmap *destination, const Rect *src_rect, bool at_native_res,
        GraphicResolution *want_fmt, uint32_t batch_skip_filter = 0u) override;

protected:
    bool SetVsyncImpl(bool vsync, bool &vsync_res) override;

    // Create DDB using preexisting texture
    IDriverDependantBitmap *CreateDDB(std::shared_ptr<Texture> txdata, int txflags) override;

    size_t GetLastDrawEntryIndex() override { return _spriteList.size(); }

private:
    ///////////////////////////////////////////////////////
    // Mode initialization: implementation
    //
    // Called after new mode was successfully initialized
    void OnModeSet(const DisplayMode &mode) override;
    bool CreateDisplayMode(const DisplayMode &mode);
    // Called when the direct3d device is created for the first time
    bool FirstTimeInit();
    void InitializeD3DState();
    void CreateVirtualScreen();
    void SetupViewport();
    void SetupDefaultVertices();
    // Create shader programs for sprite tinting and changing light level
    bool CreateStandardShaders();
    // Delete all shader programs
    void DeleteShaders();
    // Resets
    void ResetDeviceIfNecessary();
    HRESULT ResetDeviceAndRestore();
    HRESULT ResetD3DDevice();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();

    ///////////////////////////////////////////////////////
    // Texture management: implementation
    //
    bool IsTextureFormatOk(D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat);
    void AdjustSizeToNearestSupportedByCard(int *width, int *height);
    void UpdateTextureRegion(D3DTextureTile *tile, const Bitmap *bitmap, bool opaque);
    // For tracked render targets, disposes only the internal texture data
    void ReleaseRenderTargetData();
    // For tracked render targets, recreates the internal texture data
    void RecreateRenderTargets();

    ///////////////////////////////////////////////////////
    // Shader management: implementation
    //
#if (DIRECT3D_USE_D3DCOMPILER)
    // TODO: should we let configure syntax level and optimization level, entrypoint name?
    // FIXME: move to the most modern compile target supported by Direct3D.
    const char *DefaultShaderCompileTarget = "ps_2_0"; // "ps_4_0_level_9_3"
    const UINT  DefaultShaderCompileFlags  = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    const char *DefaultShaderEntryPoint    = "main";
#endif

    // Creates shader program using provided compiled data
    bool CreateShaderProgram(D3DShader::ProgramData &prg, const String &name, const uint8_t *data_ptr);
    // Loads compiled shader data from resource and creates shader
    bool CreateShaderProgramFromResource(D3DShader::ProgramData &prg, const String &name, const char *resource_name);
    void AssignBaseShaderArgs(D3DShader::ProgramData &prg, const ShaderDefinition *def);
    void UpdateGlobalShaderArgValues();
#if (DIRECT3D_USE_D3DCOMPILER)
    void OutputShaderLog(ComPtr<ID3DBlob> &out_errors, const String &shader_name, bool as_error);
#endif

    //
    // Specialized shaders
    D3DShader *CreateTintShader(const D3DShader *fallback_shader);

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
        D3DSurfacePtr Surface;
        // FIXME: replace RendSize with explicit render coordinate offset? merge with ortho matrix?
        Size SurfSize; // actual surface size
        Size RendSize; // coordinate grid size (for centering sprites)
        Rect Viewport;
        glm::mat4 Projection;
        PlaneScaling Scaling;
        int Filter = 0;

        BackbufferState() = default;
        BackbufferState(const D3DSurfacePtr &surface, const Size &surf_size, const Size &rend_size,
                        const Rect &view, const glm::mat4 &proj,
                        const PlaneScaling &scale, int filter);
        BackbufferState(D3DSurfacePtr &&surface, const Size &surf_size, const Size &rend_size,
                        const Rect &view, const glm::mat4 &proj,
                        const PlaneScaling &scale, int filter);
        BackbufferState(const BackbufferState &state) = default;
        BackbufferState(BackbufferState &&state) = default;
        ~BackbufferState() = default;

        BackbufferState &operator = (const BackbufferState &state) = default;
        BackbufferState &operator = (BackbufferState &&state) = default;
    };

    void RenderAndPresent(bool clearDrawListAfterwards);
    void RenderImpl(bool clearDrawListAfterwards);
    void RenderToSurface(BackbufferState *state, bool clearDrawListAfterwards);
    // Set current backbuffer state, which properties are used when refering to backbuffer
    void SetBackbufferState(BackbufferState *state, bool clear);
    // Sets a Direct3D viewport for the current render target.
    void SetD3DViewport(const Rect &rc);
    // Sets the scissor (render clip), clip rect is passed in the "native" coordinates.
    // Optionally pass render_on_texture if the rendering is done to texture, in native coords,
    // otherwise we assume it is set on a whole screen, scaled to the screen coords.
    void SetScissor(const Rect &clip, bool render_on_texture = false);
    // Configures rendering mode for the render target, depending on its properties
    // TODO: find a good way to merge with SetRenderTarget
    void SetRenderTarget(const D3DSpriteBatch *batch, Size &surface_sz, bool clear);
    // Assigns shader constants for post fx (rendering final game image)
    void SetupPostFx(D3DBitmap *surface, const BackbufferState &bufferstate);
    void RenderSpriteBatches();
    size_t RenderSpriteBatch(const D3DSpriteBatch &batch, size_t from, const Size &rend_sz);
    void RenderSprite(const D3DDrawListEntry *entry, const glm::mat4 &matGlobal,
                      const SpriteColorTransform &color, const Size &rend_sz);
    // Renders given texture onto the current render target
    void RenderTexture(D3DBitmap *bitmap, int draw_x, int draw_y, const glm::mat4 &matGlobal,
                       const SpriteColorTransform &color, const Size &rend_sz);

    // Sets uniform blend settings, same for both RGB and alpha component
    void SetBlendOpUniform(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor);
    // Sets blend settings for RGB only, and keeps previously set alpha blend settings
    void SetBlendOpRGB(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor);
    // Sets blend settings for alpha channel
    void SetBlendOpAlpha(D3DBLENDOP blend_op, D3DBLEND src_factor, D3DBLEND dst_factor);


    PD3DFilter _filter;

    D3DPtr direct3d;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DDevicePtr direct3ddevice;
    D3DDEVICE_CREATION_PARAMETERS direct3dcreateparams;
    D3DCAPS9 direct3ddevicecaps;
    D3DGAMMARAMP defaultgammaramp;
    D3DGAMMARAMP currentgammaramp;
    // Default vertex buffer, for textures that don't have one
    D3DVertexBufferPtr vertexbuffer;
    // Texture for rendering in native resolution
    D3DBitmap *_nativeSurface = nullptr;
    CUSTOMVERTEX defaultVertices[4];
    String previousError; // FIXME: find out why is this necessary, rewrite?
    int _fullscreenDisplay = -1; // a display where exclusive fullscreen was created
    bool _smoothScaling;
    float _pixelRenderXOffset;
    float _pixelRenderYOffset;
    bool _renderAtScreenRes;

    // Built-in shaders
    // TODO: RAII-wrapper for IGraphicShader / D3DShader pointer
    std::unique_ptr<D3DShader> _dummyShader;
    std::unique_ptr<D3DShader> _tintShader;

    BackbufferState _screenBackbuffer;
    BackbufferState _nativeBackbuffer;
    const BackbufferState *_currentBackbuffer = nullptr;

    // Render target DDB references, for keeping track of them,
    // and resetting during device reset.
    std::vector<D3DBitmap*> _renderTargets;
    // Sprite batches (parent scene nodes)
    D3DSpriteBatches _spriteBatches;
    // List of sprites to render
    std::vector<D3DDrawListEntry> _spriteList;
    // TODO: these draw list backups are needed only for the fade-in/out effects
    // find out if it's possible to reimplement these effects in main drawing routine.
    // TODO: if not above, refactor and implement Desc backup in the base class
    SpriteBatchDescs _backupBatchDescs;
    std::vector<std::pair<size_t, size_t>> _backupBatchRange;
    D3DSpriteBatches _backupBatches;
    std::vector<D3DDrawListEntry> _backupSpriteList;

    // Saved blend settings
    struct BlendOpState
    {
        D3DBLENDOP Op{};
        D3DBLEND   Src{}, Dst{};
        BlendOpState() = default;
        BlendOpState(D3DBLENDOP op, D3DBLEND src, D3DBLEND dst)
            : Op(op), Src(src), Dst(dst) {}
    };

    // Saved alpha channel blend settings for the current render target
    BlendOpState _rtBlendAlpha{};
};


class D3DGraphicsFactory : public GfxDriverFactoryBase<D3DGraphicsDriver, D3DGfxFilter>
{
public:
    ~D3DGraphicsFactory() override;

    size_t               GetFilterCount() const override;
    const GfxFilterInfo *GetFilterInfo(size_t index) const override;
    String               GetDefaultFilterID() const override;

    static D3DGraphicsFactory   *GetFactory();
    static D3DGraphicsDriver    *GetD3DDriver();

private:
    D3DGraphicsFactory() = default;

    D3DGraphicsDriver   *EnsureDriverCreated() override;
    D3DGfxFilter        *CreateFilter(const String &id) override;

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
    static Library _library;
    D3DPtr         _direct3d;
};

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DD3D_H

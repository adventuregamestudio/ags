//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Direct3D 11 graphics factory
//
// Ported from the Direct3D 9 renderer (ali3dd3d.h). Main differences:
//  - no fixed-function pipeline: all rendering goes through a vertex shader
//    and one of the pixel shaders (standard / tint / user-provided);
//  - no device "lost/reset" cycle, so render targets do not have to be
//    released and recreated;
//  - blending is configured using (cached) immutable blend state objects;
//  - swap chain is created and managed through DXGI.
//
// Requires feature level 10.0 or higher. Older GPUs should keep using the
// Direct3D 9 renderer.
//
//=============================================================================
#ifndef __AGS_EE_GFX__ALI3DD3D11_H
#define __AGS_EE_GFX__ALI3DD3D11_H

#include "platform/platform.h"

#if ! AGS_PLATFORM_OS_WINDOWS
#error This file should only be included on the Windows build
#endif

#define NOMINMAX
#include <memory>
#include <unordered_map>
#include <vector>
#define BITMAP WINDOWS_BITMAP
#include <d3d11.h>
#include <d3d11_4.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_4.h>
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

// The scaling filters are shared with the Direct3D 9 renderer
namespace D3D { class D3DGfxFilter; }

namespace D3D11
{

using AGS::Common::Bitmap;
using AGS::Common::String;

// Declare smart ptr for common Direct3D 11 / DXGI interfaces
typedef ComPtr<IDXGIFactory1>            DXGIFactoryPtr;
typedef ComPtr<ID3D11Device>             D3D11DevicePtr;
typedef ComPtr<ID3D11DeviceContext>      D3D11ContextPtr;
typedef ComPtr<ID3D11Texture2D>          D3D11Texture2DPtr;
typedef ComPtr<ID3D11ShaderResourceView> D3D11SRVPtr;
typedef ComPtr<ID3D11RenderTargetView>   D3D11RTVPtr;
typedef ComPtr<ID3D11Buffer>             D3D11BufferPtr;
typedef ComPtr<ID3D11PixelShader>        D3D11PixelShaderPtr;


// Signature of D3DCompile (we load the compiler library dynamically)
typedef HRESULT(WINAPI* PFN_D3DCompileFn)(
    LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
    const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude,
    LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2,
    ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs);

// Entry points and objects that are shared between the factory and the driver
struct D3D11Api
{
    DXGIFactoryPtr           Factory;
    PFN_D3D11_CREATE_DEVICE  CreateDevice = nullptr;
    PFN_D3DCompileFn         Compile = nullptr;
};


struct D3D11TextureTile : public TextureTile
{
    D3D11Texture2DPtr texture;
    D3D11SRVPtr       srv;
    D3D11RTVPtr       rtv; // only for render targets
};

// Full Direct3D texture data
struct D3D11Texture : Texture
{
    std::vector<D3D11TextureTile> _tiles;

    D3D11Texture(const GraphicResolution& res, bool rt)
        : Texture(res, rt) {}
    D3D11Texture(uint32_t id, const GraphicResolution& res, bool rt)
        : Texture(id, res, rt) {}
    ~D3D11Texture() = default;
    size_t GetMemSize() const override;
};

class D3D11Bitmap final : public BaseDDB
{
public:
    uint32_t GetRefID() const override { return _data->ID; }
    // Tells if this DDB has an actual render data assigned to it.
    bool IsValid() const override { return _data != nullptr; }
    // Attaches new texture data, sets basic render rules
    void AttachData(std::shared_ptr<Texture> txdata, int txflags) override
    {
        _data = std::static_pointer_cast<D3D11Texture>(txdata);
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

    D3D11Bitmap() = default;

    D3D11Bitmap(int width, int height, int colDepth, int txflags)
    {
        _size = Size(width, height);
        _scaledSize = _size;
        _colDepth = colDepth;
        _txFlags = txflags;
    }

    ~D3D11Bitmap() override = default;

    D3D11Texture* GetTexture() const { return _data.get(); }
    std::shared_ptr<D3D11Texture> GetSharedTexture() const { return _data; }
    // Returns render target view, if this DDB is a render target (otherwise null)
    D3D11RTVPtr GetRenderTargetView() const
    {
        return (_data && !_data->_tiles.empty()) ? _data->_tiles[0].rtv : D3D11RTVPtr();
    }
    TextureRenderHint GetRenderHint() const { return _renderHint; }

    void SetTexture(std::shared_ptr<D3D11Texture> data, TextureRenderHint hint = kTxHint_Normal)
    {
        _data = data;
        _renderHint = hint;
    }

private:
    // Direct3D texture data
    std::shared_ptr<D3D11Texture> _data;
    // Render parameters
    TextureRenderHint _renderHint = kTxHint_Normal;
    bool _useResampler = false;
};

class D3D11Shader final : public BaseShader
{
public:
    struct ProgramData;

    D3D11Shader(const String& name);
    D3D11Shader(const String& name, D3D11Shader::ProgramData&& data);
    D3D11Shader(const String& name, const D3D11Shader& copy_shader);
    ~D3D11Shader() = default;

    // Get a number of registered constants
    uint32_t GetConstantCount() override;
    // Get a registered constant's name, by its sequential index
    String GetConstantName(uint32_t iter_index) override;
    // Get a registered constant's location, by its sequential index;
    // returns UINT32_MAX if no such constant exists
    uint32_t GetConstantByIndex(uint32_t iter_index) override;
    // Looks up for the constant in a shader. Returns a valid index if such shader is registered,
    // and constant is present in that shader, or UINT32_MAX on failure.
    uint32_t GetConstantByName(const String& const_name) override;
    // Reset all shader constants to zero
    void ResetConstants() override;

    // A single constant register size *in floats*
    static const uint32_t ConstantSize = 4u;
    // A cap of the number of constant registers that we support (0..N)
    static const uint32_t ConstantCap = 12u;
    // A place in buffer where we will write non-applied values;
    // this is just to streamline the process and don't litter the code with checks
    static const uint32_t NullConstantIndex = ConstantCap;
    // A cap of the number of texture samplers that we support in shader;
    // note that zero index is reserved for the main texture, so unused
    static const uint32_t SamplersCap = 4u;

    // Shader data wrapped in a struct, for easier referencing
    struct ProgramData
    {
        D3D11PixelShaderPtr ShaderPtr;

        struct Register
        {
            uint32_t Index = 0u;  // register index
            uint32_t Off = 0u; // actual memory offset

            Register() = default;
            Register(uint32_t reg_index) { *this = reg_index; }
            Register& operator=(uint32_t reg_index)
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
        Register TextureDim[SamplersCap]; // texture (0..3) dimensions
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
    const D3D11Shader::ProgramData& GetData() const { return _data; }

private:
    ProgramData _data;
};

class D3D11ShaderInstance final : public BaseShaderInstance
{
public:
    D3D11ShaderInstance(D3D11Shader* shader, const String& name);
    ~D3D11ShaderInstance() = default;

    // Returns a IGraphicShader referenced by this shader instance
    IGraphicShader* GetShader() override { return _shader; }

    // Sets shader constant, using constant's index (returned by GetShaderConstant)
    void SetShaderConstantF(uint32_t const_index, float value) override;
    void SetShaderConstantF2(uint32_t const_index, float x, float y) override;
    void SetShaderConstantF3(uint32_t const_index, float x, float y, float z) override;
    void SetShaderConstantF4(uint32_t const_index, float x, float y, float z, float w) override;

    // Gets the allocated size of the constants data
    size_t GetConstantDataSize() override;
    // Gets array of constants data, where each constant is represented as 4 floats
    void GetConstantData(std::vector<float>& data) override;

    const D3D11Shader::ProgramData& GetShaderData() const { return _shader->GetData(); }
    float* GetConstantData() { return _constantData.data(); }
    size_t GetConstantDataSize() const { return _constantData.size(); }

    // Sets a texture as a shader sampler using a zero-based index
    void SetShaderSampler(uint32_t sampler_index, std::shared_ptr<Texture> tex) override;

    struct Sampler
    {
        std::shared_ptr<D3D11Texture> Tex;
        // Cached data for quicker access without extra null checks
        ID3D11ShaderResourceView* SRV = nullptr;
        Size TexSize;

        Sampler() = default;
        Sampler(std::shared_ptr<D3D11Texture> tex);
    };

    const std::vector<Sampler>& GetShaderSamplers() const { return _samplers; }

private:
    // FIXME: provide reference counting of shader ptr
    D3D11Shader* _shader = nullptr;
    // Constant buffer data, applied each time a shader is used in render
    std::vector<float> _constantData;
    std::vector<Sampler> _samplers;
};

class D3D11GfxModeList final : public IGfxModeList
{
public:
    D3D11GfxModeList(const DXGIFactoryPtr& factory, int display_index, DXGI_FORMAT format);
    ~D3D11GfxModeList() = default;

    int GetModeCount() const override
    {
        return static_cast<int>(_modes.size());
    }

    bool GetMode(int index, DisplayMode& mode) const override;

private:
    int _displayIndex = 0;
    std::vector<DXGI_MODE_DESC> _modes;
};

// Vertex used for drawing all the sprites: a unit quad, which is transformed by the
// world matrix in the vertex shader.
struct D3D11Vertex
{
    float x, y, z;   // The position.
    float u, v;      // The texture coordinates.
};

// D3D11 renderer's sprite batch
struct D3D11SpriteBatch : VMSpriteBatch
{
    // Optional render target's view
    D3D11RTVPtr RenderView;

    D3D11SpriteBatch() = default;
    D3D11SpriteBatch(uint32_t id, const Rect& view, const glm::mat4& matrix,
        const glm::mat4& vp_matrix, const SpriteColorTransform& color)
        : VMSpriteBatch(id, view, matrix, vp_matrix, color) {}
    D3D11SpriteBatch(uint32_t id, D3D11Bitmap* render_target, const Rect& view,
        const glm::mat4& matrix, const glm::mat4& vp_matrix,
        const SpriteColorTransform& color)
        : VMSpriteBatch(id, render_target, view, matrix, vp_matrix, color)
    {
        if (render_target)
            RenderView = render_target->GetRenderTargetView();
    }
};

typedef SpriteDrawListEntry<D3D11Bitmap> D3D11DrawListEntry;
typedef std::vector<D3D11SpriteBatch>    D3D11SpriteBatches;

// Blend function for either RGB or Alpha channels
struct D3D11BlendFunc
{
    D3D11_BLEND_OP Op = D3D11_BLEND_OP_ADD;
    D3D11_BLEND    Src = D3D11_BLEND_SRC_ALPHA;
    D3D11_BLEND    Dst = D3D11_BLEND_INV_SRC_ALPHA;
    D3D11BlendFunc() = default;
    D3D11BlendFunc(D3D11_BLEND_OP op, D3D11_BLEND src, D3D11_BLEND dst)
        : Op(op), Src(src), Dst(dst) {}
};

// Complete blend setup, used as a cache key for blend state objects
struct D3D11BlendDesc
{
    D3D11BlendFunc RGB;
    D3D11BlendFunc Alpha;

    uint32_t GetKey() const
    {
        auto pack = [](const D3D11BlendFunc& f) -> uint32_t
        {
            return (uint32_t(f.Op) & 7u) | ((uint32_t(f.Src) & 31u) << 3) | ((uint32_t(f.Dst) & 31u) << 8);
        };
        return pack(RGB) | (pack(Alpha) << 13);
    }
};


class D3D11GraphicsDriver : public VideoMemoryGraphicsDriver
{
public:
    D3D11GraphicsDriver(const D3D11Api& api);
    ~D3D11GraphicsDriver() override;

    ///////////////////////////////////////////////////////
    // Identification
    //
    // Gets graphic driver's identifier
    const char* GetDriverID() override { return "D3D11"; }
    // Gets graphic driver's "friendly name"
    const char* GetDriverName() override { return "Direct3D 11"; }

    ///////////////////////////////////////////////////////
    // Attributes
    //
    // Tells if this gfx driver requires releasing render targets
    // in case of display mode change or reset.
    // Direct3D 11 resources survive display mode changes.
    bool ShouldReleaseRenderTargets() override { return false; }

    ///////////////////////////////////////////////////////
    // Mode initialization
    //
    // Initialize given display mode
    bool SetDisplayMode(const DisplayMode& mode) override;
    // Updates previously set display mode, accomodating to the new screen size
    void UpdateDeviceScreen(const Size& screen_sz) override;
    // Set the size of the native image size
    bool SetNativeResolution(const GraphicResolution& native_res) override;
    // Set game render frame and translation
    bool SetRenderFrame(const Rect& dst_rect) override;
    // Report which display's color depth option is best suited for the given native color depth
    int  GetDisplayDepthForNativeDepth(int native_color_depth) const override;
    // Gets a list of supported fullscreen display modes
    IGfxModeList* GetSupportedModeList(int display_index, int color_depth) override;
    // Tells if the given display mode supported
    bool IsModeSupported(const DisplayMode& mode) override;
    // Gets currently set scaling filter
    PGfxFilter GetGraphicsFilter() const override;

    typedef std::shared_ptr<D3D::D3DGfxFilter> PD3D11Filter;
    void SetGraphicsFilter(PD3D11Filter filter);
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
    IDriverDependantBitmap* CreateDDB() override;
    // Creates a "raw" DDB, without pixel initialization.
    IDriverDependantBitmap* CreateDDB(int width, int height, int color_depth, int txflags) override;
    // Creates DDB intended to be used as a render target (allow render other DDBs on it).
    IDriverDependantBitmap* CreateRenderTargetDDB(int width, int height, int color_depth, int txflags) override;
    // Updates DBB using the given bitmap.
    void UpdateDDBFromBitmap(IDriverDependantBitmap* ddb, const Bitmap* bitmap) override;
    // Destroy the DDB; note that this does not dispose the texture unless there's no more refs to it
    void DestroyDDB(IDriverDependantBitmap* ddb) override;

    // Create texture data with the given parameters
    Texture* CreateTexture(int width, int height, int color_depth, int txflags) override;
    // Update texture data from the given bitmap
    void UpdateTexture(Texture* txdata, const Bitmap* bitmap, bool opaque) override;
    // Retrieve shared texture data object from the given DDB
    std::shared_ptr<Texture> GetTexture(IDriverDependantBitmap* ddb) override;

    ///////////////////////////////////////////////////////
    // Shader management
    //
    // Returns the expected file extension for the precompiled shader;
    // NOTE: this is compiled DXBC (shader model 4+), not compatible with the Direct3D 9 ".fxo".
    const char* GetShaderPrecompiledExtension() override { return "cso"; }
    // Returns the expected file extension for the shader source
    const char* GetShaderSourceExtension() override { return "hlsl"; }
    // Returns the expected file extension for the shader definition file
    const char* GetShaderDefinitionExtension() override { return "d3ddef"; }
    // Creates shader program from the source code, registers it under given name,
    // returns IGraphicShader, or null on failure.
    IGraphicShader* CreateShaderProgram(const String& name, const char* fragment_shader_src, const ShaderDefinition* def) override;
    // Creates shader program from the compiled data, registers it under given name,
    // returns IGraphicShader, or null on failure.
    IGraphicShader* CreateShaderProgram(const String& name, const std::vector<uint8_t>& compiled_data, const ShaderDefinition* def) override;
    // Deletes particular shader program.
    void DeleteShaderProgram(IGraphicShader* shader) override;
    // Creates shader instance for the given shader.
    IShaderInstance* CreateShaderInstance(IGraphicShader* shader, const String& name) override;
    // Deletes particular shader instance
    void DeleteShaderInstance(IShaderInstance* shader_inst) override;

    ///////////////////////////////////////////////////////
    // Preparing a scene
    // 
    // Adds sprite to the active batch, providing it's origin position
    void DrawSprite(int x, int y, IDriverDependantBitmap* ddb) override
    {
        DrawSprite(x, y, x, y, ddb);
    }
    // Adds sprite to the active batch, providing it's origin position and auxiliary
    // position of the left-top image corner in the same coordinate space
    void DrawSprite(int ox, int oy, int ltx, int lty, IDriverDependantBitmap* bitmap) override;
    // Adds a render event, which runs a callback with the given parameters, to the active batch
    void AddRenderEvent(int evt, int param) override;
    // Adds fade overlay fx to the active batch
    void SetScreenFade(int red, int green, int blue) override;
    // Adds tint overlay fx to the active batch
    // TODO: redesign this to allow various post-fx per sprite batch?
    void SetScreenTint(int red, int green, int blue) override;
    // Sets a shader to be applied to the whole screen as a post fx
    void SetScreenShader(IShaderInstance* shinst) override;
    // Redraw saved draw lists, optionally filtering specific batches
    void RedrawLastFrame(uint32_t batch_skip_filter) override;

    ///////////////////////////////////////////////////////
    // Rendering and presenting
    // 
    // Clears the screen rectangle. The coordinates are expected in the **native game resolution**.
    void ClearRectangle(int x1, int y1, int x2, int y2, RGB* colorToUse) override;
    // Renders draw lists and presents to screen.
    void Render() override;
    // Renders and presents with additional final offset and flip.
    void Render(int xoff, int yoff, Common::GraphicFlip flip) override;
    // Renders draw lists onto the provided texture.
    void Render(IDriverDependantBitmap* target) override;
    // Renders draw lists to backbuffer, but does not call present.
    void RenderToBackBuffer() override;

    // Clears screen rect, coordinates are expected in display resolution
    void ClearScreenRect(const Rect& r, RGB* colorToUse);

    ///////////////////////////////////////////////////////
    // Additional operations
    //
    // Copies contents of the game screen into the DDB
    void GetCopyOfScreenIntoDDB(IDriverDependantBitmap* target, uint32_t batch_skip_filter = 0u) override;
    // Copies contents of the last rendered game frame into bitmap using simple blit or pixel copy.
    bool GetCopyOfScreenIntoBitmap(Bitmap* destination, const Rect* src_rect, bool at_native_res,
        GraphicResolution* want_fmt, uint32_t batch_skip_filter = 0u) override;

protected:
    bool SetVsyncImpl(bool vsync, bool& vsync_res) override;

    // Create DDB using preexisting texture
    IDriverDependantBitmap* CreateDDB(std::shared_ptr<Texture> txdata, int txflags) override;

    size_t GetLastDrawEntryIndex() override { return _spriteList.size(); }

private:
    ///////////////////////////////////////////////////////
    // Mode initialization: implementation
    //
    // Called after new mode was successfully initialized
    void OnModeSet(const DisplayMode& mode) override;
    bool CreateDisplayMode(const DisplayMode& mode);
    // Creates D3D11 device, immediate context and a windowed swap chain
    bool CreateDeviceAndSwapChain(void* hwnd, int display_index, const DisplayMode& mode);
    // Adjusts swap chain buffers and fullscreen state to the given mode
    bool SetSwapChainMode(const DisplayMode& mode, bool first_time);
    // Resizes swap chain buffers; all views on the backbuffer must be released prior
    HRESULT ResizeSwapChain(int width, int height);
    bool CreateBackbufferView();
    void ReleaseBackbufferView();
    // Called when the direct3d device is created for the first time
    bool FirstTimeInit();
    // Creates all device-dependant objects: shaders, states, buffers
    bool CreateDeviceObjects();
    void ReleaseDeviceObjects();
    void InitializeRenderState();
    void CreateVirtualScreen();
    void SetupViewport();
    // Create shader programs for sprite tinting and changing light level
    bool CreateStandardShaders();
    // Delete all shader programs
    void DeleteShaders();
    // Throws an exception if the device was removed
    void CheckDeviceState();
    // Unset parameters and release resources related to the display mode
    void ReleaseDisplayMode();

    ///////////////////////////////////////////////////////
    // Texture management: implementation
    //
    void UpdateTextureRegion(D3D11TextureTile* tile, const Bitmap* bitmap, bool opaque);
    // Copies pixels from a texture region into a bitmap
    void ReadTextureIntoBitmap(ID3D11Texture2D* src, const Rect& src_rc, bool force_opaque, Bitmap* destination);

    ///////////////////////////////////////////////////////
    // Shader management: implementation
    //
    // Compiles HLSL source, returns null on failure
    ComPtr<ID3DBlob> CompileShader(const char* src, const char* name, const char* entry,
        const char* target, UINT flags, bool log_as_error);
    // Creates shader program using provided compiled data
    bool CreateShaderProgram(D3D11Shader::ProgramData& prg, const String& name, const void* data_ptr, size_t data_size);
    void AssignBaseShaderArgs(D3D11Shader::ProgramData& prg, const ShaderDefinition* def);
    void UpdateGlobalShaderArgValues();
    void OutputShaderLog(ComPtr<ID3DBlob>& out_errors, const String& shader_name, bool as_error);
    // Writes constants to the pixel shader constant buffer
    void UploadPSConstants(const float* data, size_t byte_size);
    // Writes world-view-projection matrix to the vertex shader constant buffer
    void UploadVSConstants(const glm::mat4& wvp);
    // Returns a cached blend state object for the given settings
    ID3D11BlendState* GetBlendState(const D3D11BlendDesc& desc);

    //
    // Specialized shaders
    D3D11Shader* CreateTintShader(const D3D11Shader* fallback_shader);

    ///////////////////////////////////////////////////////
    // Preparing a scene: implementation
    //
    void InitSpriteBatch(size_t index, const SpriteBatchDesc& desc) override;
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
        D3D11RTVPtr View;
        // FIXME: replace RendSize with explicit render coordinate offset? merge with ortho matrix?
        Size SurfSize; // actual surface size
        Size RendSize; // coordinate grid size (for centering sprites)
        Rect Viewport;
        glm::mat4 Projection;
        PlaneScaling Scaling;
        bool LinearFilter = false;

        BackbufferState() = default;
        BackbufferState(const D3D11RTVPtr& view, const Size& surf_size, const Size& rend_size,
            const Rect& view_rc, const glm::mat4& proj,
            const PlaneScaling& scale, bool linear_filter);
        BackbufferState(D3D11RTVPtr&& view, const Size& surf_size, const Size& rend_size,
            const Rect& view_rc, const glm::mat4& proj,
            const PlaneScaling& scale, bool linear_filter);
        BackbufferState(const BackbufferState& state) = default;
        BackbufferState(BackbufferState&& state) = default;
        ~BackbufferState() = default;

        BackbufferState& operator = (const BackbufferState& state) = default;
        BackbufferState& operator = (BackbufferState&& state) = default;
    };

    void RenderAndPresent(bool clearDrawListAfterwards);
    void RenderImpl(bool clearDrawListAfterwards);
    void RenderToSurface(BackbufferState* state, bool clearDrawListAfterwards);
    void Present();
    // Set current backbuffer state, which properties are used when refering to backbuffer
    void SetBackbufferState(BackbufferState* state, bool clear);
    // Binds pipeline state which is common for all the sprite drawing
    void ApplyBaseState();
    // Sets a Direct3D viewport for the current render target.
    void SetD3DViewport(const Rect& rc);
    // Sets the scissor (render clip), clip rect is passed in the "native" coordinates.
    // Optionally pass render_on_texture if the rendering is done to texture, in native coords,
    // otherwise we assume it is set on a whole screen, scaled to the screen coords.
    void SetScissor(const Rect& clip, bool render_on_texture = false);
    // Configures rendering mode for the render target, depending on its properties
    // TODO: find a good way to merge with SetRenderTarget
    void SetRenderTarget(const D3D11SpriteBatch* batch, Size& surface_sz, bool clear);
    // Assigns shader constants for post fx (rendering final game image)
    void SetupPostFx(D3D11Bitmap* surface, const BackbufferState& bufferstate);
    void RenderSpriteBatches();
    size_t RenderSpriteBatch(const D3D11SpriteBatch& batch, size_t from, const Size& rend_sz);
    void RenderSprite(const D3D11DrawListEntry* entry, const glm::mat4& matGlobal,
        const SpriteColorTransform& color, const Size& rend_sz);
    // Renders given texture onto the current render target
    void RenderTexture(D3D11Bitmap* bitmap, int draw_x, int draw_y, const glm::mat4& matGlobal,
        const SpriteColorTransform& color, const Size& rend_sz);
    // Cleans up render state after rendering a scene
    void PostRenderCleanup();


    PD3D11Filter _filter;

    D3D11Api _api;
    D3D11DevicePtr _device;
    D3D11ContextPtr _context;
    ComPtr<ID3D11DeviceContext1> _context1; // optional, for partial clears
    ComPtr<IDXGISwapChain> _swapChain;
    D3D_FEATURE_LEVEL _featureLevel = D3D_FEATURE_LEVEL_10_0;
    UINT _maxTextureSize = 8192;
    DXGI_SWAP_CHAIN_DESC _scDesc{};
    D3D11RTVPtr _backbufferView;
    bool _isFullscreen = false;
    bool _vsync = false;

    // Pipeline objects
    ComPtr<ID3D11VertexShader> _vertexShader;
    ComPtr<ID3D11InputLayout> _inputLayout;
    D3D11BufferPtr _vertexBuffer;     // unit quad
    D3D11BufferPtr _vsConstBuffer;    // world-view-projection
    D3D11BufferPtr _psConstBuffer;    // pixel shader constants (any shader)
    D3D11PixelShaderPtr _psStandard;  // replaces the fixed function texturing
    ComPtr<ID3D11SamplerState> _samplerPoint;
    ComPtr<ID3D11SamplerState> _samplerLinear;
    ComPtr<ID3D11SamplerState> _samplerAux; // for shader's extra samplers
    ComPtr<ID3D11RasterizerState> _rsNoScissor;
    ComPtr<ID3D11RasterizerState> _rsScissor;
    std::unordered_map<uint32_t, ComPtr<ID3D11BlendState>> _blendStates;

    // Gamma
    bool _gammaSupported = false;

    // Texture for rendering in native resolution
    D3D11Bitmap* _nativeSurface = nullptr;
    int _fullscreenDisplay = -1; // a display where exclusive fullscreen was created
    bool _smoothScaling = false;
    bool _renderAtScreenRes = false;
    // Projection matrix of the currently set render target
    glm::mat4 _curProjection;

    // Built-in shaders
    // TODO: RAII-wrapper for IGraphicShader / D3D11Shader pointer
    std::unique_ptr<D3D11Shader> _dummyShader;
    std::unique_ptr<D3D11Shader> _tintShader;

    BackbufferState _screenBackbuffer;
    BackbufferState _nativeBackbuffer;
    const BackbufferState* _currentBackbuffer = nullptr;

    // Sprite batches (parent scene nodes)
    D3D11SpriteBatches _spriteBatches;
    // List of sprites to render
    std::vector<D3D11DrawListEntry> _spriteList;
    // TODO: these draw list backups are needed only for the fade-in/out effects
    // find out if it's possible to reimplement these effects in main drawing routine.
    // TODO: if not above, refactor and implement Desc backup in the base class
    SpriteBatchDescs _backupBatchDescs;
    std::vector<std::pair<size_t, size_t>> _backupBatchRange;
    D3D11SpriteBatches _backupBatches;
    std::vector<D3D11DrawListEntry> _backupSpriteList;

    // Saved alpha channel blend settings for the current render target
    D3D11BlendFunc _rtBlendAlpha{};
};


class D3D11GraphicsFactory : public GfxDriverFactoryBase<D3D11GraphicsDriver, D3D::D3DGfxFilter>
{
public:
    ~D3D11GraphicsFactory() override;

    size_t               GetFilterCount() const override;
    const GfxFilterInfo* GetFilterInfo(size_t index) const override;
    String               GetDefaultFilterID() const override;

    static D3D11GraphicsFactory* GetFactory();
    static D3D11GraphicsDriver* GetD3D11Driver();

private:
    D3D11GraphicsFactory() = default;

    D3D11GraphicsDriver* EnsureDriverCreated() override;
    D3D::D3DGfxFilter* CreateFilter(const String& id) override;

    bool Init();

    static D3D11GraphicsFactory* _factory;
    //
    // NOTE: the libraries are wrapped in static objects, which means that they
    // are only unloaded at the very program exit. This follows the Direct3D 9
    // implementation, where unloading d3d9.dll before window is destroyed
    // caused problems (at least under WinXP). It is unclear whether the same
    // applies to DXGI / Direct3D 11, but there is no benefit in unloading
    // these early either.
    static Library _libDXGI;
    static Library _libD3D11;
    static Library _libCompiler;
    D3D11Api       _api;
};

} // namespace D3D11
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DD3D11_H

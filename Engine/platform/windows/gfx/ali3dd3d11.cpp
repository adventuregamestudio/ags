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
#include "platform/platform.h"

#if AGS_HAS_DIRECT3D11
#define NOMINMAX
#include "platform/windows/gfx/ali3dd3d11.h"
#include <algorithm>
#include <cmath>
#include <stack>
#include <SDL.h>
#include <glm/ext.hpp>
#include "ac/sys_events.h"
#include "ac/timer.h"
#include "debug/out.h"
#include "gfx/ali3dexception.h"
#include "gfx/gfx_def.h"
#include "gfx/gfxfilter_d3d.h"
#include "gfx/gfxfilter_aad3d.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "util/matrix.h"


using namespace AGS::Common;

// Necessary to update textures from 8-bit bitmaps
extern RGB palette[256];

namespace AGS
{
namespace Engine
{
namespace D3D11
{

using namespace Common;
using D3D::D3DGfxFilter;
using D3D::AAD3DGfxFilter;

//=============================================================================
//
// Built-in HLSL programs.
//
// Direct3D 11 has no fixed-function pipeline, so everything that D3D9 backend
// did with texture stages is implemented in shaders here. The sources are
// compiled at the first initialization (D3DCompiler is loaded dynamically).
//
//=============================================================================

// Transforms a unit quad using a combined world-view-projection matrix.
// The matrix is uploaded exactly as glm keeps it (column-major), which
// matches HLSL default packing of float4x4.
static const char* VertexShaderSrc = R"HLSL(
cbuffer VSConstants : register(b0)
{
    float4x4 uWVP;
};

struct VSInput
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

// NOTE: SV_Position is deliberately declared LAST here, not first.
// With it first, the D3D11 runtime reported:
//   "Vertex Shader - Pixel Shader linkage error: ... Semantic 'TEXCOORD' is
//   defined for mismatched hardware registers between the output stage and
//   input stage. [EXECUTION ERROR #343: DEVICE_SHADER_LINKAGE_REGISTERINDEX]"
// i.e. TEXCOORD0 was being assigned a different o#/v# register on the VS
// output side than on the PS input side, because PSInput (below) does not
// itself declare SV_Position, only TEXCOORD0. Declaring SV_Position last
// keeps TEXCOORD0 as register 0 on both sides.
struct VSOutput
{
    float2 uv  : TEXCOORD0;
    float4 pos : SV_Position;
};

VSOutput main(VSInput i)
{
    VSOutput o;
    o.pos = mul(uWVP, float4(i.pos, 1.0));
    o.uv = i.uv;
    return o;
}
)HLSL";

// Replaces the fixed function texture stage setup of the Direct3D 9 renderer:
//   COLOROP: MODULATE / ADD / ADDSMOOTH  with the "texture factor"
//   ALPHAOP: SELECTARG1(texture) / MODULATE(texture, factor)
//   Alpha test (alpha > 0)
static const char* StandardPixelShaderSrc = R"HLSL(
cbuffer PSConstants : register(b0)
{
    float4 uFactor;  // rgb: color factor, a: alpha factor
    float4 uParams;  // x: color op (0 - modulate, 1 - add, 2 - add smooth)
                     // y: 1 = alpha is modulated by factor, 0 = texture alpha
                     // z: 1 = alpha test on
};

Texture2D    uTex  : register(t0);
SamplerState uSamp : register(s0);

struct PSInput
{
    float2 uv : TEXCOORD0;
};

float4 main(PSInput i) : SV_Target
{
    float4 t = uTex.Sample(uSamp, i.uv);

    float3 c_mod    = t.rgb * uFactor.rgb;
    float3 c_add    = saturate(t.rgb + uFactor.rgb);
    float3 c_smooth = saturate(t.rgb + uFactor.rgb - t.rgb * uFactor.rgb);
    float3 c = (uParams.x < 0.5) ? c_mod : ((uParams.x < 1.5) ? c_add : c_smooth);

    float a = (uParams.y > 0.5) ? (t.a * uFactor.a) : t.a;

    // Emulates D3DRS_ALPHATEST (D3DCMP_GREATER, ref 0)
    clip((uParams.z > 0.5) ? (a - 0.5 / 255.0) : 1.0);

    return float4(c, a);
}
)HLSL";

// Tints a sprite: hue and saturation are taken from the tint color, while the
// brightness of the original pixel is preserved. Result is blended with the
// original pixel using the tint amount, then light level is applied.
//
// NOTE: this is written from scratch, because the Direct3D 9 renderer used a
// precompiled shader stored as an exe resource. Replace with the original
// HLSL source if you need bit-exact tinting.
static const char* TintPixelShaderSrc = R"HLSL(
cbuffer PSConstants : register(b0)
{
    float4 uTintHSV;     // x: hue (0..1), y: saturation, z: value, w: tint amount
    float4 uAlphaLight;  // x: alpha, y: light, z: 1 = alpha test on
};

Texture2D    uTex  : register(t0);
SamplerState uSamp : register(s0);

struct PSInput
{
    float2 uv : TEXCOORD0;
};

float3 hsv2rgb(float3 c)
{
    float3 p = abs(frac(c.xxx + float3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    return c.z * lerp(float3(1.0, 1.0, 1.0), saturate(p - 1.0), c.y);
}

float4 main(PSInput i) : SV_Target
{
    float4 t = uTex.Sample(uSamp, i.uv);
    float lum = max(t.r, max(t.g, t.b));
    float3 tinted = hsv2rgb(float3(uTintHSV.x, uTintHSV.y, lum));
    float3 c = lerp(t.rgb, tinted, uTintHSV.w) * uAlphaLight.y;
    float a = t.a * uAlphaLight.x;
    clip((uAlphaLight.z > 0.5) ? (a - 0.5 / 255.0) : 1.0);
    return float4(saturate(c), a);
}
)HLSL";


//
// Direct3D helpers
//
static void RectToRECT(const Rect& in_rc, RECT& out_rc)
{
    out_rc.left = in_rc.Left;
    out_rc.top = in_rc.Top;
    out_rc.right = in_rc.Right + 1;
    out_rc.bottom = in_rc.Bottom + 1;
}

// All the textures are kept in 32-bit BGRA order, which matches the
// vmem shifts set in the driver's constructor (A:24, R:16, G:8, B:0).
// Opaque textures use the "X" variant, so that sampling returns alpha of 1.0
static DXGI_FORMAT TextureFormat(bool opaque)
{
    return opaque ? DXGI_FORMAT_B8G8R8X8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
}

// Tells if the pixel format contains valid alpha channel.
static bool DXGIFormatHasAlpha(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return true;
    default:
        return false;
    }
}

// Scaling filter classes are shared with the Direct3D 9 renderer, and report
// their setting using D3DTEXTUREFILTERTYPE (D3DTEXF_POINT = 1, D3DTEXF_LINEAR = 2).
// We do not want to include d3d9 headers here, so keep the value locally.
static const int FilterValue_Linear = 2;
static inline bool IsLinearFilterValue(int filter_value)
{
    return filter_value == FilterValue_Linear;
}

// D3D11 does not allow color blend factors in the alpha blend settings
static D3D11_BLEND ToAlphaBlend(D3D11_BLEND b)
{
    switch (b)
    {
    case D3D11_BLEND_SRC_COLOR:      return D3D11_BLEND_SRC_ALPHA;
    case D3D11_BLEND_INV_SRC_COLOR:  return D3D11_BLEND_INV_SRC_ALPHA;
    case D3D11_BLEND_DEST_COLOR:     return D3D11_BLEND_DEST_ALPHA;
    case D3D11_BLEND_INV_DEST_COLOR: return D3D11_BLEND_INV_DEST_ALPHA;
    default: return b;
    }
}

// Finds DXGI adapter and output which correspond to the SDL display index
static bool GetAdapterAndOutput(IDXGIFactory1* factory, int display_index,
    ComPtr<IDXGIAdapter1>& adapter, ComPtr<IDXGIOutput>& output)
{
    int adapter_index = 0, output_index = 0;
    if (!SDL_DXGIGetOutputInfo(display_index, &adapter_index, &output_index))
    {
        adapter_index = 0;
        output_index = 0;
    }
    if (!factory || FAILED(factory->EnumAdapters1(adapter_index, adapter.Acquire())))
        return false;
    if (FAILED(adapter->EnumOutputs(output_index, output.Acquire())))
        output = nullptr;
    return true;
}


size_t D3D11Texture::GetMemSize() const
{
    size_t sz = 0u;
    for (const auto& tile : _tiles)
        sz += tile.allocWidth * tile.allocHeight * 4;
    return sz;
}

D3D11Shader::D3D11Shader(const String& name)
    : BaseShader(name)
{
}

D3D11Shader::D3D11Shader(const String& name, D3D11Shader::ProgramData&& data)
    : BaseShader(name)
    , _data(std::move(data))
{
}

D3D11Shader::D3D11Shader(const String& name, const D3D11Shader& copy_shader)
    : BaseShader(name)
    , _data(copy_shader.GetData())
{
}

inline bool D3D11Shader_ConstantOrder(const std::pair<uint32_t, String> c1, const std::pair<uint32_t, String> c2)
{
    return c1.first < c2.first;
}

uint32_t D3D11Shader::GetConstantCount()
{
    return _data.Constants.size();
}

String D3D11Shader::GetConstantName(uint32_t iter_index)
{
    if (iter_index >= _data.ConstantsOrdered.size())
        return {};
    return _data.ConstantsOrdered[iter_index].second;
}

uint32_t D3D11Shader::GetConstantByIndex(uint32_t iter_index)
{
    if (iter_index >= _data.ConstantsOrdered.size())
        return UINT32_MAX;
    return _data.ConstantsOrdered[iter_index].first;
}

uint32_t D3D11Shader::GetConstantByName(const String& const_name)
{
    if (!_data.ShaderPtr)
        return UINT32_MAX;

    auto it_found = _data.Constants.find(const_name);
    if (it_found == _data.Constants.end())
        return UINT32_MAX;

    return it_found->second;
}

void D3D11Shader::ResetConstants()
{
    // Don't do anything; all shaders share same constant buffer,
    // and it's overwritten whenever any shader is used in rendering
}

D3D11ShaderInstance::D3D11ShaderInstance(D3D11Shader* shader, const String& name)
    : BaseShaderInstance(name)
    , _shader(shader)
{
    _constantData.resize((D3D11Shader::NullConstantIndex + 1) * D3D11Shader::ConstantSize);
    _samplers.resize(D3D11Shader::SamplersCap);
}

void D3D11ShaderInstance::SetShaderConstantF(uint32_t const_index, float value)
{
    SetShaderConstantF4(const_index, value, 0.f, 0.f, 0.f);
}

void D3D11ShaderInstance::SetShaderConstantF2(uint32_t const_index, float x, float y)
{
    SetShaderConstantF4(const_index, x, y, 0.f, 0.f);
}

void D3D11ShaderInstance::SetShaderConstantF3(uint32_t const_index, float x, float y, float z)
{
    SetShaderConstantF4(const_index, x, y, z, 0.f);
}

void D3D11ShaderInstance::SetShaderConstantF4(uint32_t const_index, float x, float y, float z, float w)
{
    // Cap the constant index for safety
    if (const_index >= D3D11Shader::ConstantCap)
        return;

    const uint32_t reg_off = const_index * D3D11Shader::ConstantSize;
    _constantData[reg_off + 0] = x;
    _constantData[reg_off + 1] = y;
    _constantData[reg_off + 2] = z;
    _constantData[reg_off + 3] = w;
}

size_t D3D11ShaderInstance::GetConstantDataSize()
{
    return _constantData.size();
}

void D3D11ShaderInstance::GetConstantData(std::vector<float>& data)
{
    data = _constantData;
}

D3D11ShaderInstance::Sampler::Sampler(std::shared_ptr<D3D11Texture> tex)
{
    Tex = tex;
    if (tex)
    {
        SRV = tex->_tiles[0].srv.get();
        TexSize = Size(tex->_tiles[0].width, tex->_tiles[0].height);
    }
}

void D3D11ShaderInstance::SetShaderSampler(uint32_t sampler_index, std::shared_ptr<Texture> tex)
{
    if (sampler_index < 1u || sampler_index >= _samplers.size())
        return;

    _samplers[sampler_index] = std::dynamic_pointer_cast<D3D11Texture>(tex);
}

D3D11GfxModeList::D3D11GfxModeList(const DXGIFactoryPtr& factory, int display_index, DXGI_FORMAT format)
    : _displayIndex(display_index)
{
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
    if (!GetAdapterAndOutput(factory.get(), display_index, adapter, output) || !output)
        return;

    UINT count = 0;
    if (FAILED(output->GetDisplayModeList(format, 0, &count, nullptr)) || count == 0)
        return;
    _modes.resize(count);
    if (FAILED(output->GetDisplayModeList(format, 0, &count, _modes.data())))
    {
        _modes.clear();
        return;
    }
    _modes.resize(count);
}

bool D3D11GfxModeList::GetMode(int index, DisplayMode& mode) const
{
    if (index >= 0 && index < static_cast<int>(_modes.size()))
    {
        const DXGI_MODE_DESC& dxgi_mode = _modes[index];
        mode.DisplayIndex = _displayIndex;
        mode.Width = dxgi_mode.Width;
        mode.Height = dxgi_mode.Height;
        mode.ColorDepth = 32; // all supported formats are 32-bit
        mode.RefreshRate = dxgi_mode.RefreshRate.Denominator > 0 ?
            (dxgi_mode.RefreshRate.Numerator + dxgi_mode.RefreshRate.Denominator / 2) / dxgi_mode.RefreshRate.Denominator : 0;
        return true;
    }
    return false;
}


// Default whole-texture quad; texture tiles are always allocated to have exactly
// the tile's size, so this is also suitable for all the tiles.
static const D3D11Vertex DefaultVertices[4] =
{
    { 0.0f,  0.0f, 0.0f, 0.0f, 0.0f },
    { 1.0f,  0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f, 0.0f, 1.0f },
    { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
};

D3D11GraphicsDriver::D3D11GraphicsDriver(const D3D11Api& api)
    : _api(api)
{
    // Shifts comply to DXGI_FORMAT_B8G8R8A8_UNORM
    _vmem_a_shift_32 = 24;
    _vmem_r_shift_32 = 16;
    _vmem_g_shift_32 = 8;
    _vmem_b_shift_32 = 0;
    _curProjection = glm::mat4(1.f);
}

D3D11GraphicsDriver::BackbufferState::BackbufferState(const D3D11RTVPtr& view,
    const Size& surf_size, const Size& rend_sz,
    const Rect& view_rc, const glm::mat4& proj, const PlaneScaling& scale, bool linear_filter)
    : View(view), SurfSize(surf_size), RendSize(rend_sz)
    , Viewport(view_rc), Projection(proj), Scaling(scale), LinearFilter(linear_filter)
{
    assert(View != nullptr);
}

D3D11GraphicsDriver::BackbufferState::BackbufferState(D3D11RTVPtr&& view,
    const Size& surf_size, const Size& rend_sz,
    const Rect& view_rc, const glm::mat4& proj, const PlaneScaling& scale, bool linear_filter)
    : View(std::move(view)), SurfSize(surf_size), RendSize(rend_sz)
    , Viewport(view_rc), Projection(proj), Scaling(scale), LinearFilter(linear_filter)
{
    assert(View != nullptr);
}

void D3D11GraphicsDriver::OnModeSet(const DisplayMode& mode)
{
    GraphicsDriverBase::OnModeSet(mode);

    // The display mode has been set up successfully, save the
    // final refresh rate that we are using
    DEVMODE dev_mode = {};
    dev_mode.dmSize = sizeof(dev_mode);
    if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dev_mode))
        _mode.RefreshRate = dev_mode.dmDisplayFrequency;
    else
        _mode.RefreshRate = 0;
}

void D3D11GraphicsDriver::ReleaseDisplayMode()
{
    if (!IsModeSet())
        return;

    _screenBackbuffer = BackbufferState();

    OnModeReleased();
    ClearDrawLists();
    ClearDrawBackups();
    DestroyFxPool();
    DestroyAllStageScreens();

    // Leave exclusive fullscreen before changing window style
    if (_swapChain && _isFullscreen)
    {
        _swapChain->SetFullscreenState(FALSE, nullptr);
        _isFullscreen = false;
    }
    sys_window_set_style(kWnd_Windowed);
}

bool D3D11GraphicsDriver::FirstTimeInit()
{
    // Vsync is controlled through Present() interval, which is always available
    _capsVsync = true;

    if (!CreateDeviceObjects())
        return false;

    if (!CreateStandardShaders())
    {
        // NOTE: we don't have an obligatory transparency shader in Direct3D, so all standard shaders are optional
        Debug::Printf(kDbgMsg_Error, "ERROR: Direct3D 11: one or more standard shaders failed to compile, the game may run but have unexpected visuals.");
    }

    return true;
}

bool D3D11GraphicsDriver::CreateDeviceObjects()
{
    HRESULT hr;
    const UINT compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

    // Vertex shader and input layout
    ComPtr<ID3DBlob> vs_blob = CompileShader(VertexShaderSrc, "SpriteVS", "main", "vs_4_0", compile_flags, true);
    if (!vs_blob)
    {
        SDL_SetError("Failed to compile the vertex shader");
        return false;
    }
    hr = _device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, _vertexShader.Acquire());
    if (FAILED(hr))
    {
        SDL_SetError("Failed to create the vertex shader: 0x%08X", (unsigned)hr);
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = _device->CreateInputLayout(layout, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), _inputLayout.Acquire());
    if (FAILED(hr))
    {
        SDL_SetError("Failed to create the input layout: 0x%08X", (unsigned)hr);
        return false;
    }

    // Standard pixel shader
    ComPtr<ID3DBlob> ps_blob = CompileShader(StandardPixelShaderSrc, "SpritePS", "main", "ps_4_0", compile_flags, true);
    if (!ps_blob)
    {
        SDL_SetError("Failed to compile the standard pixel shader");
        return false;
    }
    hr = _device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, _psStandard.Acquire());
    if (FAILED(hr))
    {
        SDL_SetError("Failed to create the standard pixel shader: 0x%08X", (unsigned)hr);
        return false;
    }

    // Vertex buffer with a unit quad
    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(DefaultVertices);
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = DefaultVertices;
        hr = _device->CreateBuffer(&bd, &init, _vertexBuffer.Acquire());
        if (FAILED(hr))
        {
            SDL_SetError("Failed to create vertex buffer: 0x%08X", (unsigned)hr);
            return false;
        }
    }

    // Constant buffers
    auto make_const_buffer = [this](UINT byte_size, D3D11BufferPtr& buf) -> bool
    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = byte_size;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        return SUCCEEDED(_device->CreateBuffer(&bd, nullptr, buf.Acquire()));
    };
    // VS: one float4x4; PS: has to fit any shader's constants (ConstantCap + 1 registers)
    static_assert((D3D11Shader::NullConstantIndex + 1) * D3D11Shader::ConstantSize * sizeof(float) <= 256,
        "Pixel shader constant buffer is too small");
    if (!make_const_buffer(sizeof(float) * 16, _vsConstBuffer) ||
        !make_const_buffer(256, _psConstBuffer))
    {
        SDL_SetError("Failed to create constant buffers");
        return false;
    }

    // Samplers
    {
        D3D11_SAMPLER_DESC sd = {};
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; // otherwise 2x sprites get distorted
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MaxAnisotropy = 1;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        hr = _device->CreateSamplerState(&sd, _samplerPoint.Acquire());
        if (FAILED(hr)) { SDL_SetError("Failed to create sampler state"); return false; }
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        hr = _device->CreateSamplerState(&sd, _samplerLinear.Acquire());
        if (FAILED(hr)) { SDL_SetError("Failed to create sampler state"); return false; }
        // Extra shader samplers used D3D9 defaults: point filter, wrap addressing
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        hr = _device->CreateSamplerState(&sd, _samplerAux.Acquire());
        if (FAILED(hr)) { SDL_SetError("Failed to create sampler state"); return false; }
    }

    // Rasterizer states (no culling, no depth)
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.DepthClipEnable = TRUE;
        rd.ScissorEnable = FALSE;
        hr = _device->CreateRasterizerState(&rd, _rsNoScissor.Acquire());
        if (FAILED(hr)) { SDL_SetError("Failed to create rasterizer state"); return false; }
        rd.ScissorEnable = TRUE;
        hr = _device->CreateRasterizerState(&rd, _rsScissor.Acquire());
        if (FAILED(hr)) { SDL_SetError("Failed to create rasterizer state"); return false; }
    }

    return true;
}

void D3D11GraphicsDriver::ReleaseDeviceObjects()
{
    _blendStates.clear();
    _rsScissor = nullptr;
    _rsNoScissor = nullptr;
    _samplerAux = nullptr;
    _samplerLinear = nullptr;
    _samplerPoint = nullptr;
    _psConstBuffer = nullptr;
    _vsConstBuffer = nullptr;
    _vertexBuffer = nullptr;
    _psStandard = nullptr;
    _inputLayout = nullptr;
    _vertexShader = nullptr;
}

bool D3D11GraphicsDriver::IsModeSupported(const DisplayMode& mode)
{
    if (mode.Width <= 0 || mode.Height <= 0 || mode.ColorDepth <= 0)
    {
        SDL_SetError("Invalid resolution parameters: %d x %d x %d", mode.Width, mode.Height, mode.ColorDepth);
        return false;
    }

    if (!mode.IsRealFullscreen())
    {
        return true;
    }

    D3D11GfxModeList mode_list(_api.Factory, mode.DisplayIndex, DXGI_FORMAT_B8G8R8A8_UNORM);
    for (int i = 0; i < mode_list.GetModeCount(); ++i)
    {
        DisplayMode list_mode;
        if (mode_list.GetMode(i, list_mode) &&
            (list_mode.Width == mode.Width) && (list_mode.Height == mode.Height))
        {
            return true;
        }
    }

    SDL_SetError("The requested adapter mode is not supported");
    return false;
}

bool D3D11GraphicsDriver::SupportsGammaControl()
{
    if (!_swapChain || !_mode.IsRealFullscreen())
        return false;

    // Gamma ramp may only be set on an output which is in exclusive fullscreen
    ComPtr<IDXGIOutput> output;
    if (FAILED(_swapChain->GetContainingOutput(output.Acquire())) || !output)
        return false;
    DXGI_GAMMA_CONTROL_CAPABILITIES caps = {};
    if (FAILED(output->GetGammaControlCapabilities(&caps)))
        return false;
    return caps.NumGammaControlPoints > 0;
}

void D3D11GraphicsDriver::SetGamma(int newGamma)
{
    if (!_swapChain)
        return;
    ComPtr<IDXGIOutput> output;
    if (FAILED(_swapChain->GetContainingOutput(output.Acquire())) || !output)
        return;
    DXGI_GAMMA_CONTROL_CAPABILITIES caps = {};
    if (FAILED(output->GetGammaControlCapabilities(&caps)) || caps.NumGammaControlPoints == 0)
        return;

    // Same as in D3D9 backend: scale the default (linear) ramp by the gamma percentage
    DXGI_GAMMA_CONTROL gamma = {};
    gamma.Scale = { 1.f, 1.f, 1.f };
    gamma.Offset = { 0.f, 0.f, 0.f };
    const UINT points = std::min<UINT>(caps.NumGammaControlPoints, 1025u);
    for (UINT i = 0; i < points; ++i)
    {
        const float v = std::min(1.f, caps.ControlPointPositions[i] * newGamma / 100.f);
        gamma.GammaCurve[i] = { v, v, v };
    }
    output->SetGammaControl(&gamma);
}

void D3D11GraphicsDriver::CheckDeviceState()
{
    // Direct3D 11 has no "lost device" state like D3D9; the device is either
    // fine, or was removed (driver crash/update, GPU unplugged) and must be recreated.
    const HRESULT hr = _device->GetDeviceRemovedReason();
    if (hr != S_OK)
    {
        throw Ali3DException(String::FromFormat("Direct3D 11 device was removed: error code: 0x%08X", (unsigned)hr));
    }
}

bool D3D11GraphicsDriver::CreateDeviceAndSwapChain(void* hwnd, int display_index, const DisplayMode& mode)
{
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
    if (!GetAdapterAndOutput(_api.Factory.get(), display_index, adapter, output))
    {
        SDL_SetError("Failed to find a DXGI adapter for display %d", display_index);
        return false;
    }

    // Feature level 10.0 is the minimum; older hardware should use the D3D9 renderer
    static const D3D_FEATURE_LEVEL feature_levels[] =
    {
      D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };
    const UINT level_count = static_cast<UINT>(sizeof(feature_levels) / sizeof(feature_levels[0]));
    const UINT base_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT hr = E_FAIL;
#if defined(_DEBUG)
    // Try with the debug layer first; it's not always installed
    hr = _api.CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
        base_flags | D3D11_CREATE_DEVICE_DEBUG, feature_levels, level_count, D3D11_SDK_VERSION,
        _device.Acquire(), &_featureLevel, _context.Acquire());
#endif
    if (FAILED(hr))
    {
        hr = _api.CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
            base_flags, feature_levels, level_count, D3D11_SDK_VERSION,
            _device.Acquire(), &_featureLevel, _context.Acquire());
    }
    if (FAILED(hr))
    {
        SDL_SetError("Failed to create Direct3D 11 Device: 0x%08X", (unsigned)hr);
        return false;
    }

    // Texture size limits by the feature level (D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
    _maxTextureSize = (_featureLevel >= D3D_FEATURE_LEVEL_11_0) ? 16384u : 8192u;

    const char* fl_name = "10.0";
    if (_featureLevel >= D3D_FEATURE_LEVEL_11_0) fl_name = "11.0";
    else if (_featureLevel >= D3D_FEATURE_LEVEL_10_1) fl_name = "10.1";
    Debug::Printf("Direct3D 11 device created, feature level %s", fl_name);

    // The immediate context is used by the the main thread, so turn on the protection
    // (equivalent of D3DCREATE_MULTITHREADED)
    {
        ComPtr<ID3D11Multithread> mt;
        if (SUCCEEDED(_context->QueryInterface(__uuidof(ID3D11Multithread), reinterpret_cast<void**>(mt.Acquire()))))
            mt->SetMultithreadProtected(TRUE);
    }
    // Optional: ID3D11DeviceContext1 is used for clearing partial rects
    _context->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(_context1.Acquire()));

    // Get the DXGI factory of the device's adapter, as swap chain must be created by it
    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> dev_adapter;
    ComPtr<IDXGIFactory> factory;
    if (FAILED(_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi_device.Acquire()))) ||
        FAILED(dxgi_device->GetAdapter(dev_adapter.Acquire())) ||
        FAILED(dev_adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(factory.Acquire()))))
    {
        SDL_SetError("Failed to obtain DXGI factory from the device");
        return false;
    }

    // Swap chain is always created windowed, exclusive fullscreen is set up afterwards.
    // SWAPEFFECT_SEQUENTIAL keeps the backbuffer contents after Present, similar to
    // D3DSWAPEFFECT_COPY in the D3D9 renderer (required for screenshots and video playback).
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = mode.Width;
    sd.BufferDesc.Height = mode.Height;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = static_cast<HWND>(hwnd);
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // NOTE: plugins that used to receive D3DPRESENT_PARAMETERS now receive DXGI_SWAP_CHAIN_DESC
    if (_initGfxCallback != nullptr)
        _initGfxCallback(&sd);

    hr = factory->CreateSwapChain(_device.get(), &sd, _swapChain.Acquire());
    if (FAILED(hr))
    {
        SDL_SetError("Failed to create DXGI swap chain: 0x%08X", (unsigned)hr);
        return false;
    }
    // Alt+Enter is handled by the engine, don't let DXGI toggle fullscreen by itself
    factory->MakeWindowAssociation(static_cast<HWND>(hwnd), DXGI_MWA_NO_ALT_ENTER);
    _swapChain->GetDesc(&_scDesc);
    _isFullscreen = false;
    return true;
}

HRESULT D3D11GraphicsDriver::ResizeSwapChain(int width, int height)
{
    // All the references to the swap chain's buffers must be released prior to this
    _context->OMSetRenderTargets(0, nullptr, nullptr);
    _context->ClearState();
    _context->Flush();
    HRESULT hr = _swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (SUCCEEDED(hr))
        _swapChain->GetDesc(&_scDesc);
    return hr;
}

bool D3D11GraphicsDriver::CreateBackbufferView()
{
    ComPtr<ID3D11Texture2D> buffer;
    HRESULT hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(buffer.Acquire()));
    if (FAILED(hr))
    {
        SDL_SetError("Failed to get swap chain buffer: 0x%08X", (unsigned)hr);
        return false;
    }
    hr = _device->CreateRenderTargetView(buffer.get(), nullptr, _backbufferView.Acquire());
    if (FAILED(hr))
    {
        SDL_SetError("Failed to create backbuffer view: 0x%08X", (unsigned)hr);
        return false;
    }
    return true;
}

void D3D11GraphicsDriver::ReleaseBackbufferView()
{
    _screenBackbuffer = BackbufferState();
    _backbufferView = nullptr;
}

bool D3D11GraphicsDriver::SetSwapChainMode(const DisplayMode& mode, bool first_time)
{
    ReleaseBackbufferView();

    // A freshly created swap chain already has the right size
    if (!first_time)
    {
        HRESULT hr = ResizeSwapChain(mode.Width, mode.Height);
        if (FAILED(hr))
        {
            SDL_SetError("IDXGISwapChain::ResizeBuffers failed: 0x%08X", (unsigned)hr);
            return false;
        }
    }

    if (mode.IsRealFullscreen())
    {
        DXGI_MODE_DESC target = {};
        target.Width = mode.Width;
        target.Height = mode.Height;
        target.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        // Let the system choose a refresh rate, otherwise there may be delays
        // on alt-tab due to mismatching refresh rates (same as in D3D9 renderer).
        target.RefreshRate.Numerator = 0;
        target.RefreshRate.Denominator = 0;
        _swapChain->ResizeTarget(&target);
        HRESULT hr = _swapChain->SetFullscreenState(TRUE, nullptr);
        if (FAILED(hr))
        {
            SDL_SetError("IDXGISwapChain::SetFullscreenState failed: 0x%08X", (unsigned)hr);
            return false;
        }
        _isFullscreen = true;
        // The buffers must be resized again after the mode was switched
        ReleaseBackbufferView();
        hr = ResizeSwapChain(mode.Width, mode.Height);
        if (FAILED(hr))
        {
            SDL_SetError("IDXGISwapChain::ResizeBuffers failed: 0x%08X", (unsigned)hr);
            return false;
        }
    }

    return CreateBackbufferView();
}

bool D3D11GraphicsDriver::CreateDisplayMode(const DisplayMode& mode)
{
    if (!IsModeSupported(mode))
        return false;

    SDL_Window* window = sys_get_window();
    int use_display = mode.DisplayIndex;
    if (!window)
    {
        sys_window_create("", mode.DisplayIndex, mode.Width, mode.Height, mode.Mode);
    }
    else
    {
#if (AGS_SUPPORT_MULTIDISPLAY)
        // If user requested an exclusive fullscreen, move window to where we created it first,
        // because DXGI does not normally support switching displays in exclusive mode.
        // NOTE: we may in theory support this, but we'd have to release and recreate
        // ALL the resources, including all textures currently in memory.
        if (mode.IsRealFullscreen() &&
            (_fullscreenDisplay > 0) && (sys_get_window_display_index() != _fullscreenDisplay))
        {
            use_display = _fullscreenDisplay;
            sys_window_fit_in_display(_fullscreenDisplay);
        }
#endif
    }

    HWND hwnd = (HWND)sys_win_get_window();
    _vsync = mode.Vsync;

    const bool first_time_init = !_device;
    if (first_time_init)
    {
        if (!CreateDeviceAndSwapChain(hwnd, use_display, mode))
            return false;
        if (!FirstTimeInit())
            return false;
    }
    else
    {
        // Only adjust window styles in non-fullscreen modes:
        // for real fullscreen DXGI will adjust the window and display.
        if (!mode.IsRealFullscreen())
        {
            sys_window_set_style(mode.Mode, Size(mode.Width, mode.Height));
            sys_window_bring_to_front();
        }
    }

    return SetSwapChainMode(mode, first_time_init);
}

void D3D11GraphicsDriver::InitializeRenderState()
{
    // Clear the backbuffer to black
    if (_backbufferView)
    {
        const float black[4] = { 0.f, 0.f, 0.f, 1.f };
        _context->ClearRenderTargetView(_backbufferView.get(), black);
    }

    // If we already have a render frame configured, then setup viewport immediately
    SetupViewport();
}

void D3D11GraphicsDriver::SetupViewport()
{
    if (!IsModeSet() || !IsRenderFrameValid() || !IsNativeSizeValid() || !_backbufferView)
        return;

    const float src_width = _srcRect.GetWidth();
    const float src_height = _srcRect.GetHeight();

    // Setup orthographic projection matrix
    glm::mat4 identity(1.f);
    glm::mat4 mat_ortho = glmex::ortho_d3d(src_width, src_height);

    // NOTE: unlike Direct3D 9, there is no half-pixel offset to compensate:
    // texel centers match pixel centers by default in Direct3D 10+.

    // Clear the screen before setting a viewport.
    ClearScreenRect(RectWH(0, 0, _mode.Width, _mode.Height), nullptr);

    // Set Viewport.
    SetD3DViewport(_dstRect);

    _screenBackbuffer = BackbufferState(_backbufferView, Size(_mode.Width, _mode.Height), _srcRect.GetSize(),
        _dstRect, mat_ortho, _scaling, _filter ? IsLinearFilterValue(_filter->GetSamplerStateForStandardSprite()) : false);

    // View and Projection matrixes are currently fixed in Direct3D renderer
    _stageMatrixes.View = identity;
    _stageMatrixes.Projection = mat_ortho;
}

void D3D11GraphicsDriver::SetGraphicsFilter(PD3D11Filter filter)
{
    _filter = filter;
    _screenBackbuffer.LinearFilter = IsLinearFilterValue(_filter->GetSamplerStateForStandardSprite());
    OnSetFilter();
}

bool D3D11GraphicsDriver::SetDisplayMode(const DisplayMode& mode)
{
    ReleaseDisplayMode();

    if (mode.ColorDepth < 15)
    {
        SDL_SetError("Direct3D driver does not support 256-color display mode");
        return false;
    }

    if (!CreateDisplayMode(mode))
        return false;

    OnInit();
    DisplayMode set_mode = mode;
    set_mode.DisplayIndex = sys_get_window_display_index();
    OnModeSet(set_mode);
    if ((_fullscreenDisplay < 0) || set_mode.IsRealFullscreen())
        _fullscreenDisplay = set_mode.DisplayIndex;
    InitializeRenderState();
    CreateVirtualScreen();
    return true;
}

void D3D11GraphicsDriver::UpdateDeviceScreen(const Size& screen_sz)
{
    if (_mode.IsRealFullscreen())
        return; // ignore in exclusive fs mode

    _mode.Width = screen_sz.Width;
    _mode.Height = screen_sz.Height;

    // Only the swap chain's buffers have to be recreated, all the other resources are intact
    ReleaseBackbufferView();
    HRESULT hr = ResizeSwapChain(_mode.Width, _mode.Height);
    if (FAILED(hr))
    {
        Debug::Printf(kDbgMsg_Error, "D3D11GraphicsDriver: Failed to resize swap chain: 0x%08X", (unsigned)hr);
        return;
    }
    if (!CreateBackbufferView())
    {
        Debug::Printf(kDbgMsg_Error, "D3D11GraphicsDriver: Failed to recreate backbuffer view");
        return;
    }
    // Display mode could've changed, so re-setup viewport, matrixes, etc
    SetupViewport();
}

void D3D11GraphicsDriver::CreateVirtualScreen()
{
    if (!IsModeSet() || !IsNativeSizeValid())
        return;

    // Set up native surface
    // TODO: maybe not do this always, but only allocate if necessary
    // when render option is set, or temporarily for making a screenshot.
    if (_nativeSurface)
    {
        DestroyDDB(_nativeSurface);
        _nativeSurface = nullptr;
    }
    _nativeSurface = (D3D11Bitmap*)CreateRenderTargetDDB(
        _srcRect.GetWidth(), _srcRect.GetHeight(), _mode.ColorDepth, kTxFlags_Opaque);
    glm::mat4 mat_ortho = glmex::ortho_d3d(_srcRect.GetWidth(), _srcRect.GetHeight());
    _nativeBackbuffer = BackbufferState(_nativeSurface->GetRenderTargetView(),
        _srcRect.GetSize(), _srcRect.GetSize(), _srcRect, mat_ortho, PlaneScaling(), false);

    // Preset initial stage screen for plugin raw drawing
    SetStageScreen(0, _srcRect.GetSize());
}

bool D3D11GraphicsDriver::CreateStandardShaders()
{
    _tintShader.reset(CreateTintShader(_dummyShader.get()));
    return _tintShader->IsValid();
}

void D3D11GraphicsDriver::DeleteShaders()
{
    _tintShader = nullptr;
}

ComPtr<ID3DBlob> D3D11GraphicsDriver::CompileShader(const char* src, const char* name,
    const char* entry, const char* target, UINT flags, bool log_as_error)
{
    if (!_api.Compile)
    {
        Debug::Printf(kDbgMsg_Error, "ERROR: Direct3D 11: shader compiler is not available, cannot compile \"%s\"", name);
        return {};
    }

    ComPtr<ID3DBlob> out_data, out_errors;
    HRESULT hr = _api.Compile(src, strlen(src), name, nullptr /* no defines */, nullptr /* no includes */,
        entry, target, flags, 0 /* no effect flags */,
        out_data.Acquire(), out_errors.Acquire());
    if (out_errors)
        OutputShaderLog(out_errors, name, FAILED(hr) && log_as_error);
    if (FAILED(hr))
        return {};
    return out_data;
}

bool D3D11GraphicsDriver::CreateShaderProgram(D3D11Shader::ProgramData& prg, const String& name,
    const void* data_ptr, size_t data_size)
{
    D3D11PixelShaderPtr shader_ptr;
    HRESULT hr = _device->CreatePixelShader(data_ptr, data_size, nullptr, shader_ptr.Acquire());
    if (FAILED(hr))
    {
        Debug::Printf(kDbgMsg_Error, "ERROR: Direct3D 11: Failed to create pixel shader: 0x%08X", (unsigned)hr);
        return false;
    }

    prg.ShaderPtr = shader_ptr;
    Debug::Printf("Direct3D 11: \"%s\" shader program created successfully", name.GetCStr());
    return true;
}

D3D11Shader* D3D11GraphicsDriver::CreateTintShader(const D3D11Shader* fallback_shader)
{
    D3D11Shader::ProgramData prg;
    ComPtr<ID3DBlob> blob = CompileShader(TintPixelShaderSrc, "Tinting", "main", "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, true);
    if (blob && CreateShaderProgram(prg, "Tinting", blob->GetBufferPointer(), blob->GetBufferSize()))
    {
        AssignBaseShaderArgs(prg, nullptr);
        // Registers are known, because we have written this shader
        prg.TintHSV = 0u;
        prg.TintAlphaLight = 1u;
        return new D3D11Shader("Tinting", std::move(prg));
    }
    else if (fallback_shader)
    {
        return new D3D11Shader("Tinting", *fallback_shader);
    }
    else
    {
        return new D3D11Shader("Tinting");
    }
}

// TODO: write a (preferrably fully generic) wrapper class or a helper function
// that lets read any map values and returns default value if key not present;
// may be useful in multiple other places; move to Common/util.
// Also it will help to let cast keys (e.g. from const char* to String)
template <typename TKey, typename TValue, typename THash, typename TEqualTo, typename TAllocator,
    template<class, class, class, class, class> class TMap>
const TValue& GetValueOrDef(const TMap<TKey, TValue, THash, TEqualTo, TAllocator>& map, const TKey& key, const TValue& def_val)
{
    auto it_found = map.find(key);
    if (it_found == map.end())
        return def_val;
    return it_found->second;
}

void D3D11GraphicsDriver::AssignBaseShaderArgs(D3D11Shader::ProgramData& prg, const ShaderDefinition* def)
{
    const uint32_t reg_sz = D3D11Shader::ConstantSize;
    const uint32_t reg_cap = D3D11Shader::ConstantCap;
    const uint32_t null_index = D3D11Shader::NullConstantIndex;
    if (def && !def->Constants.empty())
    {
        prg.Time = GetValueOrDef(def->Constants, String::Wrapper("iTime"), null_index);
        prg.GameFrame = GetValueOrDef(def->Constants, String::Wrapper("iGameFrame"), null_index);
        prg.TextureDim[0] = GetValueOrDef(def->Constants, String::Wrapper("iTextureDim"), null_index);
        if (prg.TextureDim[0].Index == null_index)
            prg.TextureDim[0] = GetValueOrDef(def->Constants, String::Wrapper("iTextureDim0"), null_index);
        prg.TextureDim[1] = GetValueOrDef(def->Constants, String::Wrapper("iTextureDim1"), null_index);
        prg.TextureDim[2] = GetValueOrDef(def->Constants, String::Wrapper("iTextureDim2"), null_index);
        prg.TextureDim[3] = GetValueOrDef(def->Constants, String::Wrapper("iTextureDim3"), null_index);
        prg.Alpha = GetValueOrDef(def->Constants, String::Wrapper("iAlpha"), null_index);
        prg.OutputDim = GetValueOrDef(def->Constants, String::Wrapper("iOutputDim"), null_index);
    }
    else
    {
        // Default to hardcoded values
        prg.Time = 0u;
        prg.GameFrame = 1u;
        prg.TextureDim[0] = 2u;
        prg.Alpha = 3u;
        prg.OutputDim = 4u;
        prg.TextureDim[1] = 5u;
        prg.TextureDim[2] = 6u;
        prg.TextureDim[3] = 7u;
    }

    // Copy constants table
    prg.Constants = {};
    prg.ConstantsOrdered = {};
    if (def)
    {
        prg.Constants = def->Constants;
        // Allocate a buffer big enough to contain all constants;
        // cap the register amount for safety
        for (auto& cc : prg.Constants)
        {
            if (cc.second > reg_cap)
                cc.second = null_index;

            auto constant = std::make_pair(cc.second, cc.first);
            prg.ConstantsOrdered.insert(
                std::upper_bound(prg.ConstantsOrdered.begin(), prg.ConstantsOrdered.end(), constant, D3D11Shader_ConstantOrder),
                constant);
        }
    }
}

void D3D11GraphicsDriver::UpdateGlobalShaderArgValues()
{
    // Direct3D shaders do not have a exclusive "memory".
    // Their constants have to be set each time they are used.
}

void D3D11GraphicsDriver::OutputShaderLog(ComPtr<ID3DBlob>& out_errors, const String& shader_name, bool as_error)
{
    assert(out_errors);
    if (!out_errors)
        return;

    const MessageType mt = as_error ? kDbgMsg_Error : kDbgMsg_Debug;
    if (as_error)
        Debug::Printf(mt, "ERROR: Direct3D 11: shader \"%s\" %s:", shader_name.GetCStr(), "compilation");
    else
        Debug::Printf(mt, "Direct3D 11: shader \"%s\" %s:", shader_name.GetCStr(), "compilation");

    if (out_errors->GetBufferSize() > 0)
    {
        Debug::Printf(mt, "----------------------------------------");
        Debug::Printf(mt, "%s", static_cast<const char*>(out_errors->GetBufferPointer()));
        Debug::Printf(mt, "----------------------------------------");
    }
    else
    {
        Debug::Printf(mt, "Shader output log was empty.");
    }
}

bool D3D11GraphicsDriver::SetNativeResolution(const GraphicResolution& native_res)
{
    OnSetNativeRes(native_res);
    // Also make sure viewport is updated using new native & destination rectangles
    SetupViewport();
    CreateVirtualScreen();
    return !_srcRect.IsEmpty();
}

bool D3D11GraphicsDriver::SetRenderFrame(const Rect& dst_rect)
{
    OnSetRenderFrame(dst_rect);
    // Also make sure viewport is updated using new native & destination rectangles
    SetupViewport();
    return !_dstRect.IsEmpty();
}

int D3D11GraphicsDriver::GetDisplayDepthForNativeDepth(int /*native_color_depth*/) const
{
    // Swap chain and all textures are 32-bit
    return 32;
}

IGfxModeList* D3D11GraphicsDriver::GetSupportedModeList(int display_index, int /*color_depth*/)
{
    return new D3D11GfxModeList(_api.Factory, display_index, DXGI_FORMAT_B8G8R8A8_UNORM);
}

PGfxFilter D3D11GraphicsDriver::GetGraphicsFilter() const
{
    return _filter;
}

void D3D11GraphicsDriver::UnInit()
{
    OnUnInit();
    ReleaseDisplayMode();

    if (_nativeSurface)
    {
        DestroyDDB(_nativeSurface);
        _nativeSurface = nullptr;
    }
    _nativeBackbuffer = BackbufferState();
    _currentBackbuffer = nullptr;

    DeleteShaders();
    ReleaseBackbufferView();
    ReleaseDeviceObjects();

    // A swap chain must be switched to windowed before it is released
    if (_swapChain)
    {
        _swapChain->SetFullscreenState(FALSE, nullptr);
        _swapChain = nullptr;
    }
    _isFullscreen = false;
    if (_context)
    {
        _context->ClearState();
        _context->Flush();
    }
    _context1 = nullptr;
    _context = nullptr;
    _device = nullptr;

    sys_window_destroy();
}

D3D11GraphicsDriver::~D3D11GraphicsDriver()
{
    D3D11GraphicsDriver::UnInit();
}

void D3D11GraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB* colorToUse)
{
    // NOTE: this function is practically useless at the moment, because D3D redraws whole game frame each time
    if (!_device) return;
    Rect r(x1, y1, x2, y2);
    r = _scaling.ScaleRange(r);
    ClearScreenRect(r, colorToUse);
}

void D3D11GraphicsDriver::ClearScreenRect(const Rect& r, RGB* colorToUse)
{
    if (!_context || !_backbufferView)
        return;

    float color[4] = { 0.f, 0.f, 0.f, 0.f };
    if (colorToUse != nullptr)
    {
        color[0] = colorToUse->r / 255.f;
        color[1] = colorToUse->g / 255.f;
        color[2] = colorToUse->b / 255.f;
        color[3] = 1.f;
    }

    const bool whole_screen = (r.Left <= 0) && (r.Top <= 0) &&
        (r.Right >= _mode.Width - 1) && (r.Bottom >= _mode.Height - 1);
    if (whole_screen)
    {
        _context->ClearRenderTargetView(_backbufferView.get(), color);
    }
    else if (_context1)
    {
        // Clearing a part of the surface is only possible with the D3D 11.1 context
        RECT rc;
        RectToRECT(r, rc);
        _context1->ClearView(_backbufferView.get(), color, &rc, 1);
    }
    // else: partial clear is not supported by the runtime, skip
}

void D3D11GraphicsDriver::ReadTextureIntoBitmap(ID3D11Texture2D* src, const Rect& src_rc,
    bool force_opaque, Bitmap* destination)
{
    D3D11_TEXTURE2D_DESC src_desc = {};
    src->GetDesc(&src_desc);

    // Render targets cannot be mapped directly, so copy the region to a staging texture
    D3D11_TEXTURE2D_DESC sd = {};
    sd.Width = src_rc.GetWidth();
    sd.Height = src_rc.GetHeight();
    sd.MipLevels = 1;
    sd.ArraySize = 1;
    sd.Format = src_desc.Format;
    sd.SampleDesc.Count = 1;
    sd.Usage = D3D11_USAGE_STAGING;
    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    D3D11Texture2DPtr staging;
    if (FAILED(_device->CreateTexture2D(&sd, nullptr, staging.Acquire())))
        throw Ali3DException("CreateTexture2D (staging) failed");

    D3D11_BOX box = {};
    box.left = src_rc.Left;
    box.top = src_rc.Top;
    box.right = src_rc.Right + 1;
    box.bottom = src_rc.Bottom + 1;
    box.front = 0;
    box.back = 1;
    _context->CopySubresourceRegion(staging.get(), 0, 0, 0, 0, src, 0, &box);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(_context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped)))
        throw Ali3DException("ID3D11DeviceContext::Map (staging) failed");

    // If surface format does not have a valid alpha channel,
    // then fill it with opaqueness while copying pixels, otherwise plain copy data
    if (DXGIFormatHasAlpha(src_desc.Format) && !force_opaque)
    {
        BitmapHelper::ReadPixelsFromMemory(destination, static_cast<uint8_t*>(mapped.pData), mapped.RowPitch);
    }
    else
    {
        // NOTE: this is currently hardcoded to read from a 32-bit surface
        const int bpp = destination->GetBPP();
        uint8_t* src_ptr = static_cast<uint8_t*>(mapped.pData);
        for (int y = 0; y < destination->GetHeight(); ++y)
        {
            uint32_t* dst_ptr = reinterpret_cast<uint32_t*>(destination->GetScanLineForWriting(y));
            for (int dx = 0, sx = 0; dx < destination->GetWidth(); ++dx, sx = dx * bpp)
            {
                dst_ptr[dx] = makeacol32(src_ptr[sx + 2], src_ptr[sx + 1], src_ptr[sx + 0], 0xFF /* opaque */);
            }
            src_ptr += mapped.RowPitch;
        }
    }
    _context->Unmap(staging.get(), 0);
}

void D3D11GraphicsDriver::GetCopyOfScreenIntoDDB(IDriverDependantBitmap* target, uint32_t batch_skip_filter)
{
    // If we normally render in screen res, restore last frame's lists and
    // render in native res on the given target;
    // Also force re-render last frame if we require batch filtering
    if (_renderAtScreenRes || (batch_skip_filter != 0))
    {
        bool old_render_res = _renderAtScreenRes;
        _renderAtScreenRes = false;
        RedrawLastFrame(batch_skip_filter);
        Render(target);
        _renderAtScreenRes = old_render_res;
    }
    else
    {
        // If we normally render in native res, then simply render backbuffer contents
        D3D11Bitmap* bitmap = (D3D11Bitmap*)target;
        Size surf_sz = bitmap->GetSize();
        BackbufferState backbuffer = BackbufferState(bitmap->GetRenderTargetView(), surf_sz, surf_sz,
            RectWH(0, 0, surf_sz.Width, surf_sz.Height), glmex::ortho_d3d(surf_sz.Width, surf_sz.Height),
            PlaneScaling(), false);
        SetBackbufferState(&backbuffer, true);
        SetupPostFx(_nativeSurface, backbuffer);
        RenderTexture(_nativeSurface, 0, 0, glmex::identity(), SpriteColorTransform(), _srcRect.GetSize());
        PostRenderCleanup();
        _currentBackbuffer = nullptr; // "backbuffer" is a local object
    }
}

bool D3D11GraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap* destination,
    const Rect* src_rect, bool at_native_res,
    GraphicResolution* want_fmt, uint32_t batch_skip_filter)
{
    // Currently don't support copying in screen resolution when we are rendering in native
    if (!_renderAtScreenRes)
        at_native_res = true;

    Rect copy_from = src_rect ? *src_rect : _srcRect;
    if (!at_native_res)
        copy_from = _scaling.ScaleRange(copy_from);
    if (destination->GetColorDepth() != _mode.ColorDepth || destination->GetSize() != copy_from.GetSize())
    {
        if (want_fmt)
            *want_fmt = GraphicResolution(copy_from.GetWidth(), copy_from.GetHeight(), _mode.ColorDepth);
        return false;
    }

    // If we are rendering sprites at the screen resolution, and requested native res,
    // re-render last frame to the native surface;
    // Also force re-render last frame if we require batch filtering
    if ((at_native_res && _renderAtScreenRes) || (batch_skip_filter != 0))
    {
        bool old_render_res = _renderAtScreenRes;
        _renderAtScreenRes = !at_native_res;
        RedrawLastFrame(batch_skip_filter);
        RenderImpl(true);
        _renderAtScreenRes = old_render_res;
    }

    D3D11Texture2DPtr src_tex;
    bool force_opaque = false;
    if (at_native_res)
    {
        src_tex = _nativeSurface->GetTexture()->_tiles[0].texture;
    }
    else
    {
        // Get the back buffer texture; its alpha channel contains junk, so treat as opaque
        ComPtr<ID3D11Resource> res;
        _screenBackbuffer.View->GetResource(res.Acquire());
        if (!res || FAILED(res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(src_tex.Acquire()))))
            throw Ali3DException("Failed to get backbuffer texture");
        force_opaque = true;
    }

    ReadTextureIntoBitmap(src_tex.get(), copy_from, force_opaque, destination);
    return true;
}

void D3D11GraphicsDriver::Render()
{
    Render(0, 0, kFlip_None);
}

void D3D11GraphicsDriver::Render(int /*xoff*/, int /*yoff*/, GraphicFlip /*flip*/)
{
    CheckDeviceState();
    RenderAndPresent(true);
}

void D3D11GraphicsDriver::RenderToBackBuffer()
{
    CheckDeviceState();
    RenderImpl(true);
}

void D3D11GraphicsDriver::Render(IDriverDependantBitmap* target)
{
    CheckDeviceState();
    D3D11Bitmap* bitmap = (D3D11Bitmap*)target;
    Size surf_sz = bitmap->GetSize();
    BackbufferState backbuffer = BackbufferState(bitmap->GetRenderTargetView(), surf_sz, surf_sz,
        RectWH(0, 0, surf_sz.Width, surf_sz.Height), glmex::ortho_d3d(surf_sz.Width, surf_sz.Height),
        PlaneScaling(), false);
    RenderToSurface(&backbuffer, true);
    _currentBackbuffer = nullptr; // "backbuffer" is a local object
}

void D3D11GraphicsDriver::RenderSprite(const D3D11DrawListEntry* drawListEntry, const glm::mat4& matGlobal,
    const SpriteColorTransform& color, const Size& rend_sz)
{
    RenderTexture(drawListEntry->ddb, drawListEntry->x, drawListEntry->y, matGlobal, color, rend_sz);
}

void D3D11GraphicsDriver::UploadPSConstants(const float* data, size_t byte_size)
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(_context->Map(_psConstBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        throw Ali3DException("Failed to map pixel shader constant buffer");
    memcpy(mapped.pData, data, std::min<size_t>(byte_size, 256u));
    _context->Unmap(_psConstBuffer.get(), 0);
}

void D3D11GraphicsDriver::UploadVSConstants(const glm::mat4& wvp)
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(_context->Map(_vsConstBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        throw Ali3DException("Failed to map vertex shader constant buffer");
    memcpy(mapped.pData, glm::value_ptr(wvp), sizeof(float) * 16);
    _context->Unmap(_vsConstBuffer.get(), 0);
}

ID3D11BlendState* D3D11GraphicsDriver::GetBlendState(const D3D11BlendDesc& desc)
{
    const uint32_t key = desc.GetKey();
    auto it = _blendStates.find(key);
    if (it != _blendStates.end())
        return it->second.get();

    D3D11_BLEND_DESC bd = {};
    auto& rt = bd.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = desc.RGB.Src;
    rt.DestBlend = desc.RGB.Dst;
    rt.BlendOp = desc.RGB.Op;
    rt.SrcBlendAlpha = ToAlphaBlend(desc.Alpha.Src);
    rt.DestBlendAlpha = ToAlphaBlend(desc.Alpha.Dst);
    rt.BlendOpAlpha = desc.Alpha.Op;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ComPtr<ID3D11BlendState> state;
    HRESULT hr = _device->CreateBlendState(&bd, state.Acquire());
    if (FAILED(hr))
        throw Ali3DException(String::FromFormat("ID3D11Device::CreateBlendState failed: error code 0x%08X", (unsigned)hr));
    auto res = _blendStates.emplace(key, state);
    return res.first->second.get();
}

void D3D11GraphicsDriver::RenderTexture(D3D11Bitmap* bmpToDraw, int draw_x, int draw_y,
    const glm::mat4& matGlobal, const SpriteColorTransform& color, const Size& rend_sz)
{
    const int alpha = (color.Alpha * bmpToDraw->GetAlpha()) / 255;
    const int invalpha = 255 - alpha;
    int tint_r, tint_g, tint_b, tint_sat, light_lev;
    bmpToDraw->GetTint(tint_r, tint_g, tint_b, tint_sat);
    light_lev = bmpToDraw->GetLightLevel();
    const bool do_tint = tint_sat > 0 && _tintShader && _tintShader->IsValid();
    const auto blend_mode = bmpToDraw->GetBlendMode();

    //-------------------------------------------------------------------------
    // Blend settings: computed for each draw, as D3D11 uses immutable state objects
    //-------------------------------------------------------------------------
    D3D11BlendDesc blend;
    blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
    blend.Alpha = _rtBlendAlpha;
    float blend_factor[4] = { 1.f, 1.f, 1.f, 1.f };
    bool alpha_test = true;

    // Treat special render modes
    switch (bmpToDraw->GetRenderHint())
    {
    case kTxHint_PremulAlpha:
        blend_factor[0] = blend_factor[1] = blend_factor[2] = alpha / 255.f;
        blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_BLEND_FACTOR, D3D11_BLEND_INV_SRC_ALPHA);
        break;
    default:
        break;
    }

    // FIXME: user blend modes break the above special blend for RT textures
    // Blend modes
    switch (blend_mode)
    {
    case kBlend_Normal:
        // default alpha blending, set above
        break;
    case kBlend_Add: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE); break; // ADD (transparency = strength)
    case kBlend_Darken: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_MIN, D3D11_BLEND_ONE, D3D11_BLEND_ONE); break; // DARKEN
    case kBlend_Lighten: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_MAX, D3D11_BLEND_ONE, D3D11_BLEND_ONE); break; // LIGHTEN
    case kBlend_Multiply: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_COLOR); break; // MULTIPLY
    case kBlend_Screen: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_COLOR); break; // SCREEN
    case kBlend_Subtract: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_REV_SUBTRACT, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE); break; // SUBTRACT (transparency = strength)
    case kBlend_Exclusion: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_INV_SRC_COLOR); break; // EXCLUSION
    // APPROXIMATIONS (need pixel shaders)
    case kBlend_Burn: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_SUBTRACT, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_INV_DEST_COLOR); break; // LINEAR BURN (approximation)
    case kBlend_Dodge: blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ONE); break; // fake color dodge (half strength of the real thing)
    // IMPORTANT: please note that we are rendering onto a surface that has a premultiplied alpha,
    // that's why Copy blend modes are somewhat different from the math you'd normally expect;
    // i.e. we do not use ONE, ZERO to copy RGB, but SRC_ALPHA, ZERO.
    case kBlend_Copy:
        blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ZERO);
        blend.Alpha = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO);
        // Alpha test has to be off, or source zero alpha will be skipped completely
        alpha_test = false;
        break;
    case kBlend_CopyRGB:
        blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_ZERO);
        blend.Alpha = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ONE);
        alpha_test = false;
        break;
    case kBlend_CopyAlpha:
        // FIXME: this does not really work whenever destination has non-opaque alpha,
        // because destination surface colors are alpha-premultiplied!
        blend.RGB = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_ALPHA);
        blend.Alpha = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO);
        alpha_test = false;
        break;
    default: break;
    }
    _context->OMSetBlendState(GetBlendState(blend), blend_factor, 0xFFFFFFFF);

    //-------------------------------------------------------------------------
    // Pixel shader and its constants
    //-------------------------------------------------------------------------
    ID3D11PixelShader* ps = nullptr;
    float ps_consts[16] = {};
    const float* ps_data = ps_consts;
    size_t ps_data_size = sizeof(ps_consts);
    bool use_aux_textures = false;
    ID3D11ShaderResourceView* aux_srv[D3D11Shader::SamplersCap - 1] = {};

    if (bmpToDraw->GetShader())
    {
        // Use custom shader
        D3D11ShaderInstance* shaderinst = (D3D11ShaderInstance*)bmpToDraw->GetShader();
        const D3D11Shader::ProgramData* program = &shaderinst->GetShaderData();
        ps = program->ShaderPtr.get();

        // FIXME: figure out a better approach to getting writeable internal array's pointer?
        float* data = shaderinst->GetConstantData();
        data[program->Time.Off] = _globalShaderConst.Time;
        data[program->GameFrame.Off] = static_cast<float>(_globalShaderConst.GameFrame);
        data[program->TextureDim[0].Off + 0] = static_cast<float>(bmpToDraw->GetWidth());
        data[program->TextureDim[0].Off + 1] = static_cast<float>(bmpToDraw->GetHeight());
        data[program->Alpha.Off] = alpha / 255.0f;

        const auto& samplers = shaderinst->GetShaderSamplers();
        for (uint32_t i = 1u; i < samplers.size(); ++i)
        {
            aux_srv[i - 1] = samplers[i].SRV;
            data[program->TextureDim[i].Off + 0] = static_cast<float>(samplers[i].TexSize.Width);
            data[program->TextureDim[i].Off + 1] = static_cast<float>(samplers[i].TexSize.Height);
        }
        use_aux_textures = true;

        // NOTE: the custom data should already be inside ConstantData buffer
        ps_data = data;
        ps_data_size = shaderinst->GetConstantDataSize() * sizeof(float);
    }
    else if (do_tint)
    {
        // Use Tinting pixel shader
        rgb_to_hsv(tint_r, tint_g, tint_b, &ps_consts[0], &ps_consts[1], &ps_consts[2]);
        ps_consts[0] /= 360.0f; // In HSV, Hue is 0-360
        ps_consts[3] = (float)tint_sat / 256.0f;
        ps_consts[4] = (float)alpha / 256.0f;
        ps_consts[5] = (light_lev > 0) ? (float)light_lev / 256.0f : 1.0f;
        ps_consts[6] = alpha_test ? 1.f : 0.f;

        const D3D11Shader::ProgramData* program = &_tintShader->GetData();
        ps = program->ShaderPtr.get();
    }
    else
    {
        // Alpha transparency OR light effect: emulates fixed function texture stages
        ps = _psStandard.get();
        float useTintRed = 255.f, useTintGreen = 255.f, useTintBlue = 255.f;
        float color_op = 0.f; // modulate

        if ((light_lev > 0) && (light_lev < 256))
        {
            // darkening the sprite... this stupid calculation is for
            // consistency with the allegro software-mode code that does
            // a trans blend with a (8,8,8) sprite
            useTintRed = (float)((light_lev * 192) / 256 + 64);
            useTintGreen = useTintRed;
            useTintBlue = useTintRed;
        }
        else if (light_lev > 256)
        {
            // ideally we would use a multi-stage operation here
            // because we need to do TEXTURE + (TEXTURE x LIGHT)
            color_op = 1.f; // add
            useTintRed = (float)((light_lev - 256) / 2);
            useTintGreen = useTintRed;
            useTintBlue = useTintRed;
        }
        ps_consts[0] = useTintRed / 255.f;
        ps_consts[1] = useTintGreen / 255.f;
        ps_consts[2] = useTintBlue / 255.f;
        ps_consts[3] = alpha / 255.f;

        // BLENDMODES WORKAROUNDS - BEGIN
        // allow transparency with blending modes
        // darken/lighten the base sprite so a higher transparency value makes it trasparent
        switch (blend_mode)
        {
        case kBlend_Darken:
        case kBlend_Multiply:
        case kBlend_Burn:
            // fade to white
            // FIXME burn is imperfect due to blend mode, darker than normal even when transparent
            color_op = 2.f; // add smooth
            ps_consts[0] = ps_consts[1] = ps_consts[2] = ps_consts[3] = invalpha / 255.f;
            break;
        case kBlend_Lighten:
        case kBlend_Screen:
        case kBlend_Exclusion:
        case kBlend_Dodge:
            // fade to black
            color_op = 0.f; // modulate
            ps_consts[0] = ps_consts[1] = ps_consts[2] = ps_consts[3] = alpha / 255.f;
            break;
        default:
            break;
        }
        // BLENDMODES WORKAROUNDS - END

        ps_consts[4] = color_op;
        // No transparency: use texture alpha component;
        // otherwise fixed transparency, use (TextureAlpha x FixedTranslucency)
        ps_consts[5] = (alpha == 255) ? 0.f : 1.f;
        ps_consts[6] = alpha_test ? 1.f : 0.f;
    }

    _context->PSSetShader(ps, nullptr, 0);
    UploadPSConstants(ps_data, ps_data_size);
    if (use_aux_textures)
        _context->PSSetShaderResources(1, D3D11Shader::SamplersCap - 1, aux_srv);

    //-------------------------------------------------------------------------
    // Sampler
    //-------------------------------------------------------------------------
    const bool use_linear = (_smoothScaling) && bmpToDraw->GetUseResampler()
        && bmpToDraw->GetSizeToRender() != bmpToDraw->GetSize();
    ID3D11SamplerState* sampler = (use_linear || _currentBackbuffer->LinearFilter) ?
        _samplerLinear.get() : _samplerPoint.get();
    _context->PSSetSamplers(0, 1, &sampler);

    //-------------------------------------------------------------------------
    // Geometry: draw each texture tile as a quad
    //-------------------------------------------------------------------------
    const auto* txdata = bmpToDraw->GetTexture();

    float width = bmpToDraw->GetWidthToRender();
    float height = bmpToDraw->GetHeightToRender();
    float xProportion = width / (float)bmpToDraw->GetWidth();
    float yProportion = height / (float)bmpToDraw->GetHeight();

    for (size_t ti = 0; ti < txdata->_tiles.size(); ++ti)
    {
        width = txdata->_tiles[ti].width * xProportion;
        height = txdata->_tiles[ti].height * yProportion;
        float xOffs, yOffs;
        if ((bmpToDraw->GetFlip() & kFlip_Horizontal) != 0)
            xOffs = (bmpToDraw->GetWidth() - (txdata->_tiles[ti].x + txdata->_tiles[ti].width)) * xProportion;
        else
            xOffs = txdata->_tiles[ti].x * xProportion;
        if ((bmpToDraw->GetFlip() & kFlip_Vertical) != 0)
            yOffs = (bmpToDraw->GetHeight() - (txdata->_tiles[ti].y + txdata->_tiles[ti].height)) * yProportion;
        else
            yOffs = txdata->_tiles[ti].y * yProportion;
        float thisX = draw_x + xOffs;
        float thisY = draw_y + yOffs;

        //Setup translation and scaling matrices
        float widthToScale = width;
        float heightToScale = height;
        if ((bmpToDraw->GetFlip() & kFlip_Horizontal) != 0)
        {
            // The usual transform changes 0..1 into 0..width
            // So first negate it (which changes 0..w into -w..0)
            widthToScale = -widthToScale;
            // and now shift it over to make it 0..w again
            thisX += width;
        }
        if ((bmpToDraw->GetFlip() & kFlip_Vertical) != 0)
        {
            heightToScale = -heightToScale;
            thisY += height;
        }
        // Apply sprite origin, rounded to keep pixel precision
        thisX -= std::roundf((abs(widthToScale) - 1.f) * bmpToDraw->GetOrigin().X);
        thisY -= std::roundf((abs(heightToScale) - 1.f) * bmpToDraw->GetOrigin().Y);
        // Center inside a rendering rect
        // FIXME: this should be a part of a projection matrix, afaik
        thisX = (-(rend_sz.Width / 2.0f)) + thisX;
        thisY = (rend_sz.Height / 2.0f) - thisY; // inverse axis

        // Setup rotation and pivot
        float rotZ = bmpToDraw->GetRotation();
        const Pointf& pivot = bmpToDraw->GetPivot();
        float pivotX = -(widthToScale * pivot.X), pivotY = (heightToScale * pivot.Y);

        // Self sprite transform (first scale, then rotate and then translate, reversed);
        // NOTE: no half-pixel offset here, unlike in Direct3D 9
        glm::mat4 transform = glmex::make_transform2d(
            thisX, thisY, widthToScale, heightToScale,
            rotZ, pivotX, pivotY);
        // Global batch transform
        transform = matGlobal * transform;

        UploadVSConstants(_curProjection * transform);
        ID3D11ShaderResourceView* srv = txdata->_tiles[ti].srv.get();
        _context->PSSetShaderResources(0, 1, &srv);

        // workaround: extra render pass for the blend modes that cannot be achieved with one
        if (blend_mode == kBlend_Dodge)
        {
            // since the dodge is only half strength we can get a closer approx by drawing it twice
            _context->Draw(4, 0);
        }
        _context->Draw(4, 0);
    }
}

void D3D11GraphicsDriver::PostRenderCleanup()
{
    // Reset textures in all slots which we use in shaders, etc
    ID3D11ShaderResourceView* null_srv[D3D11Shader::SamplersCap] = {};
    _context->PSSetShaderResources(0, D3D11Shader::SamplersCap, null_srv);
    _context->PSSetShader(nullptr, nullptr, 0);
}

void D3D11GraphicsDriver::Present()
{
    HRESULT hr = _swapChain->Present(_vsync ? 1 : 0, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        throw Ali3DException(String::FromFormat(
            "IDXGISwapChain::Present: device lost: error code: 0x%08X, reason: 0x%08X",
            (unsigned)hr, (unsigned)_device->GetDeviceRemovedReason()));
    }
    // NOTE: DXGI_STATUS_OCCLUDED (window is hidden / fullscreen lost) is not an error
}

void D3D11GraphicsDriver::RenderAndPresent(bool clearDrawListAfterwards)
{
    RenderImpl(clearDrawListAfterwards);
    Present();
}

void D3D11GraphicsDriver::RenderImpl(bool clearDrawListAfterwards)
{
    if (_renderAtScreenRes)
    {
        RenderToSurface(&_screenBackbuffer, clearDrawListAfterwards);
    }
    else
    {
        RenderToSurface(&_nativeBackbuffer, clearDrawListAfterwards);
    }

    if (!_renderAtScreenRes)
    {
        // Draw native texture on a real backbuffer
        SetBackbufferState(&_screenBackbuffer, true);
        SetupPostFx(_nativeSurface, _screenBackbuffer);
        RenderTexture(_nativeSurface, 0, 0, glmex::identity(), SpriteColorTransform(), _srcRect.GetSize());
        // Unbind the native texture, as it will be a render target next time
        PostRenderCleanup();
    }
}

void D3D11GraphicsDriver::RenderToSurface(BackbufferState* state, bool clearDrawListAfterwards)
{
    SetBackbufferState(state, true);

    UpdateGlobalShaderArgValues();
    RenderSpriteBatches();
    PostRenderCleanup();

    if (clearDrawListAfterwards)
    {
        BackupDrawLists();
        ClearDrawLists();
    }
    ResetFxPool();
}

void D3D11GraphicsDriver::ApplyBaseState()
{
    // Everything that stays the same for all the sprites;
    // this is re-applied each time as some of the calls (like resizing swap chain) reset the state.
    const UINT stride = sizeof(D3D11Vertex);
    const UINT offset = 0;
    ID3D11Buffer* vb = _vertexBuffer.get();
    _context->IASetInputLayout(_inputLayout.get());
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    _context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    _context->VSSetShader(_vertexShader.get(), nullptr, 0);
    ID3D11Buffer* cb = _vsConstBuffer.get();
    _context->VSSetConstantBuffers(0, 1, &cb);
    cb = _psConstBuffer.get();
    _context->PSSetConstantBuffers(0, 1, &cb);
    ID3D11SamplerState* aux[D3D11Shader::SamplersCap - 1] = { _samplerAux.get(), _samplerAux.get(), _samplerAux.get() };
    _context->PSSetSamplers(1, D3D11Shader::SamplersCap - 1, aux);
    _context->RSSetState(_rsNoScissor.get());
    _context->OMSetDepthStencilState(nullptr, 0); // no depth buffer
}

void D3D11GraphicsDriver::SetBackbufferState(BackbufferState* state, bool clear)
{
    _currentBackbuffer = state;
    ApplyBaseState();

    // Make sure no textures are bound while we are switching render targets
    PostRenderCleanup();
    ID3D11RenderTargetView* rtv = _currentBackbuffer->View.get();
    _context->OMSetRenderTargets(1, &rtv, nullptr);
    SetD3DViewport(_currentBackbuffer->Viewport);
    _curProjection = _currentBackbuffer->Projection;
    _rtBlendAlpha = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);

    if (clear)
    {
        const float transparent[4] = { 0.f, 0.f, 0.f, 0.f };
        _context->ClearRenderTargetView(rtv, transparent);
    }
}

void D3D11GraphicsDriver::SetD3DViewport(const Rect& rc)
{
    D3D11_VIEWPORT view = {};
    view.TopLeftX = static_cast<float>(rc.Left);
    view.TopLeftY = static_cast<float>(rc.Top);
    view.Width = static_cast<float>(rc.GetWidth());
    view.Height = static_cast<float>(rc.GetHeight());
    view.MinDepth = 0.0f;
    view.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &view);
}

void D3D11GraphicsDriver::SetScissor(const Rect& clip, bool render_on_texture)
{
    if (!clip.IsEmpty())
    {
        // Adjust a clipping rect to either whole screen, or a target texture
        Rect scissor = render_on_texture ? clip : _currentBackbuffer->Scaling.ScaleRange(clip);
        D3D11_RECT d3d_scissor;
        RectToRECT(scissor, d3d_scissor);
        _context->RSSetState(_rsScissor.get());
        _context->RSSetScissorRects(1, &d3d_scissor);
    }
    else
    {
        _context->RSSetState(_rsNoScissor.get());
    }
}

void D3D11GraphicsDriver::SetRenderTarget(const D3D11SpriteBatch* batch, Size& rend_sz, bool clear)
{
    ID3D11RenderTargetView* rtv = nullptr;
    // Make sure no textures are bound while we are switching render targets
    PostRenderCleanup();

    if (batch && batch->RenderTarget)
    {
        // Assign an arbitrary render target, and setup render params
        rtv = batch->RenderView.get();
        Size surface_sz = Size(batch->RenderTarget->GetWidth(), batch->RenderTarget->GetHeight());
        rend_sz = surface_sz;
        _context->OMSetRenderTargets(1, &rtv, nullptr);
        SetD3DViewport(RectWH(surface_sz));
        _curProjection = glmex::ortho_d3d(surface_sz.Width, surface_sz.Height);
        // Configure rules for merging sprite alpha values onto a
        // render target, which also contains alpha channel.
        _rtBlendAlpha = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_ONE);
    }
    else
    {
        // Assign the default backbuffer
        rtv = _currentBackbuffer->View.get();
        rend_sz = _currentBackbuffer->RendSize;
        _context->OMSetRenderTargets(1, &rtv, nullptr);
        SetD3DViewport(_currentBackbuffer->Viewport);
        _curProjection = _currentBackbuffer->Projection;
        // Return back to default alpha merging rules
        _rtBlendAlpha = D3D11BlendFunc(D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
    }

    if (clear)
    {
        const float transparent[4] = { 0.f, 0.f, 0.f, 0.f };
        _context->ClearRenderTargetView(rtv, transparent);
    }
}

void D3D11GraphicsDriver::SetupPostFx(D3D11Bitmap* surface, const BackbufferState& bufferstate)
{
    D3D11ShaderInstance* shader = static_cast<D3D11ShaderInstance*>(surface->GetShader());
    if (!shader)
        return;

    shader->SetShaderConstantF2(shader->GetShaderData().OutputDim.Index,
        bufferstate.Scaling.X.ScaleDistance(surface->GetWidthToRender()),
        bufferstate.Scaling.Y.ScaleDistance(surface->GetHeightToRender()));
}

void D3D11GraphicsDriver::RenderSpriteBatches()
{
    assert(_currentBackbuffer);
    // Close unended batches, and issue a warning
    assert(_actSpriteBatch == UINT32_MAX);
    while (_actSpriteBatch != UINT32_MAX)
        EndSpriteBatch();

    if (_spriteBatchDesc.size() == 0)
    {
        return; // no batches - no render
    }

    // TODO: following algorithm is repeated for both Direct3D and OpenGL renderer
    // classes. The problem is that some data has different types and contents
    // specific to the renderer. But there has to be a good way to make a shared
    // algorithm in a base class.

    // Render all the sprite batches with necessary transformations;
    // some of them may be rendered to a separate texture instead.
    // For these we save their IDs in a stack (rt_parents).
    // The top of the stack lets us know which batch's RT we are using.
    std::stack<uint32_t> rt_parents;
    ID3D11RenderTargetView* back_buffer = _currentBackbuffer->View.get();
    assert(back_buffer);
    ID3D11RenderTargetView* cur_rt = back_buffer; // current render target
    Size rend_sz = _currentBackbuffer->RendSize; // current rt surface size

    const size_t last_batch_to_rend = _spriteBatchDesc.size() - 1;
    for (size_t cur_bat = 0u, last_bat = 0u, cur_spr = 0u; last_bat <= last_batch_to_rend;)
    {
        const auto& batch = _spriteBatches[cur_bat];
        // Test if we are entering this batch (and not continuing after coming back from nested)
        const bool new_batch = cur_bat == last_bat;
        if (new_batch)
        {
            // If batch introduces a new render target, or the first using backbuffer, then remember it
            if (rt_parents.empty() || batch.RenderTarget)
                rt_parents.push(cur_bat); // push new RT parent
            else
                rt_parents.push(rt_parents.top()); // copy same current parent
        }

        // If this batch has ANY sprites in it at all (own, or nested),
        // then we would need to update render target; otherwise there's no need
        if (_spriteBatchRange[cur_bat].first < _spriteBatchRange[cur_bat].second)
        {
            // If render target is different in this batch, then set it up
            const auto& rt_parent = _spriteBatches[rt_parents.top()];
            ID3D11RenderTargetView* want_rt = rt_parent.RenderView ? rt_parent.RenderView.get() : back_buffer;
            if (cur_rt != want_rt)
            {
                cur_rt = want_rt;
                SetRenderTarget(&rt_parent, rend_sz, new_batch);
            }
        }

        // Render immediate batch sprites, if any, update cur_spr iterator;
        // we know that the batch has sprites, if next sprite in list belongs to it ("node" ref)
        if ((cur_spr < _spriteList.size()) && (cur_bat == _spriteList[cur_spr].node))
        {
            // Now set clip (scissor), and render sprites
            SetScissor(batch.Viewport, (cur_rt != back_buffer));
            _stageMatrixes.World = batch.Matrix;
            _rendSpriteBatch = batch.ID;
            cur_spr = RenderSpriteBatch(batch, cur_spr, rend_sz);
            // at this point cur_spr iterator is updated past the last sprite in a sequence;
            // this may be the end of batch, but also may be the a begginning of a sub-batch
        }

        // Test if we're exiting current batch (and not going into nested ones):
        // if there's no sprites belonging to this batch (direct, or nested),
        // and if there's no nested batches (even if empty ones)
        const uint32_t was_bat = cur_bat;
        while ((cur_bat != UINT32_MAX) && (cur_spr >= _spriteBatchRange[cur_bat].second) &&
            ((last_bat == last_batch_to_rend) || (_spriteBatchDesc[last_bat + 1].Parent != cur_bat)))
        {
            rt_parents.pop(); // pop RT ref from the history
            // Back to the parent batch
            cur_bat = _spriteBatchDesc[cur_bat].Parent;
        }

        // If we stayed at the same batch, this means that there are still nested batches;
        // if there's no batches in the stack left, this means we got to move forward anyway.
        if ((was_bat == cur_bat) || (cur_bat == UINT32_MAX))
        {
            cur_bat = ++last_bat;
        }
    }

    SetRenderTarget(nullptr, rend_sz, false);
    _rendSpriteBatch = UINT32_MAX;
    _stageMatrixes.World = _spriteBatches[0].Matrix;
    _context->RSSetState(_rsNoScissor.get());
}

size_t D3D11GraphicsDriver::RenderSpriteBatch(const D3D11SpriteBatch& batch, size_t from, const Size& rend_sz)
{
    if (batch.Skip)
    {
        for (; (from < _spriteList.size()) && (_spriteList[from].node == batch.ID); ++from);
        return from;
    }

    for (; (from < _spriteList.size()) && (_spriteList[from].node == batch.ID); ++from)
    {
        const auto& e = _spriteList[from];
        if (e.skip)
            continue;

        switch (reinterpret_cast<uintptr_t>(e.ddb))
        {
        case DRAWENTRY_STAGECALLBACK:
            // raw-draw plugin support
            // NOTE: the plugins now receive ID3D11Device*, rather than IDirect3DDevice9*
            int sx, sy;
            if (auto* ddb = DoSpriteEvtCallback(e.x, reinterpret_cast<intptr_t>(_device.get()), sx, sy))
            {
                auto stageEntry = D3D11DrawListEntry((D3D11Bitmap*)ddb, batch.ID, sx, sy);
                RenderSprite(&stageEntry, batch.Matrix, batch.Color, rend_sz);
            }
            break;
        default:
            RenderSprite(&e, batch.Matrix, batch.Color, rend_sz);
            break;
        }
    }
    return from;
}

void D3D11GraphicsDriver::InitSpriteBatch(size_t index, const SpriteBatchDesc& desc)
{
    // Create transformation matrix for this batch
    float pivotx = -_srcRect.GetWidth() * 0.5f + desc.SizeRef.Width * desc.Transform.ScaleX * desc.Transform.Pivot.X;
    float pivoty = -_srcRect.GetHeight() * 0.5f + desc.SizeRef.Height * desc.Transform.ScaleY * desc.Transform.Pivot.Y;

    // Inverse transformation, because it should affect the textures drawn within this batch
    glm::mat4 msrt = glmex::make_inv_transform2d(
        (float)desc.Transform.X, (float)-desc.Transform.Y,
        desc.Transform.ScaleX, desc.Transform.ScaleY,
        Math::DegreesToRadians(desc.Transform.Rotate), pivotx, -pivoty);
    // Translate scaled node into Top-Left screen coordinates
    float scaled_offx = _srcRect.GetWidth() * ((1.f - desc.Transform.ScaleX) * 0.5f);
    float scaled_offy = _srcRect.GetHeight() * ((1.f - desc.Transform.ScaleY) * 0.5f);
    glm::mat4 model = glmex::translate(-scaled_offx, -(-scaled_offy));
    model = model * msrt;

    // Apply node flip: this is implemented as a negative scaling.
    // TODO: find out if possible to merge with normal scale
    float flip_sx = 1.f, flip_sy = 1.f;
    switch (desc.Flip)
    {
    case kFlip_Vertical: flip_sx = -flip_sx; break;
    case kFlip_Horizontal: flip_sy = -flip_sy; break;
    case kFlip_Both: flip_sx = -flip_sx; flip_sy = -flip_sy; break;
    default: break;
    }
    glm::mat4 mflip = glmex::scale(flip_sx, flip_sy);
    model = mflip * model;

    // Also create separate viewport transformation matrix:
    // it will use a slightly different set of transforms,
    // because the viewport's coordinates origin is different from sprites.
    glm::mat4 mat_viewport = glmex::make_transform2d(
        (float)desc.Transform.X, (float)desc.Transform.Y,
        desc.Transform.ScaleX, desc.Transform.ScaleY,
        Math::DegreesToRadians(desc.Transform.Rotate), pivotx, -pivoty);
    glm::mat4 vp_flip_off = glmex::translate(
        _srcRect.GetWidth() * ((1.f - flip_sx) * 0.5f),
        _srcRect.GetHeight() * ((1.f - flip_sy) * 0.5f));
    mat_viewport = mflip * mat_viewport;
    mat_viewport = vp_flip_off * mat_viewport;

    // Apply parent batch's settings, if preset;
    // except when the new batch is started on a separate texture
    Rect viewport = desc.Viewport;
    if ((desc.Parent != UINT32_MAX) && !desc.RenderTarget)
    {
        const auto& parent = _spriteBatches[desc.Parent];
        // Combine sprite matrix with the parent's
        model = parent.Matrix * model;
        // Combine viewport's matrix with the parent's
        mat_viewport = parent.ViewportMat * mat_viewport;

        // Transform this node's viewport using (only!) parent's matrix,
        // and don't let child viewport go outside the parent's bounds.
        if (!viewport.IsEmpty())
        {
            viewport = glmex::full_transform(viewport, parent.ViewportMat);
            viewport = ClampToRect(parent.Viewport, viewport);
        }
        else
        {
            viewport = parent.Viewport;
        }
    }
    else if (viewport.IsEmpty())
    {
        viewport = desc.RenderTarget ?
            RectWH(0, 0, desc.RenderTarget->GetWidth(), desc.RenderTarget->GetHeight()) :
            _srcRect;
    }

    // Assign the new spritebatch
    if (_spriteBatches.size() <= index)
        _spriteBatches.resize(index + 1);
    _spriteBatches[index] = D3D11SpriteBatch(index, (D3D11Bitmap*)desc.RenderTarget, viewport,
        model, mat_viewport, desc.Transform.Color);

    // Preset stage screen for plugin raw drawing
    SetStageScreen(index, viewport.GetSize());
}

void D3D11GraphicsDriver::ResetAllBatches()
{
    _spriteBatches.clear();
    _spriteList.clear();
}

void D3D11GraphicsDriver::ClearDrawBackups()
{
    _backupBatchDescs.clear();
    _backupBatchRange.clear();
    _backupBatches.clear();
    _backupSpriteList.clear();
}

void D3D11GraphicsDriver::BackupDrawLists()
{
    _backupBatchDescs = _spriteBatchDesc;
    _backupBatchRange = _spriteBatchRange;
    _backupBatches = _spriteBatches;
    _backupSpriteList = _spriteList;
}

void D3D11GraphicsDriver::RestoreDrawLists()
{
    _spriteBatchDesc = _backupBatchDescs;
    _spriteBatchRange = _backupBatchRange;
    _spriteBatches = _backupBatches;
    _spriteList = _backupSpriteList;
    _actSpriteBatch = UINT32_MAX;
}

void D3D11GraphicsDriver::FilterSpriteBatches(uint32_t skip_filter)
{
    if (skip_filter == 0)
        return;
    for (size_t i = 0; i < _spriteBatchDesc.size(); ++i)
    {
        if (_spriteBatchDesc[i].FilterFlags & skip_filter)
        {
            const auto range = _spriteBatchRange[i];
            for (size_t spr = range.first; spr < range.second; ++spr)
                _spriteList[spr].skip = true;
        }
    }
}

void D3D11GraphicsDriver::RedrawLastFrame(uint32_t skip_filter)
{
    RestoreDrawLists();
    FilterSpriteBatches(skip_filter);
}

void D3D11GraphicsDriver::DrawSprite(int ox, int oy, int /*ltx*/, int /*lty*/, IDriverDependantBitmap* ddb)
{
    assert(_actSpriteBatch != UINT32_MAX);
    _spriteList.push_back(D3D11DrawListEntry((D3D11Bitmap*)ddb, _actSpriteBatch, ox, oy));
}

void D3D11GraphicsDriver::AddRenderEvent(int evt, int param)
{
    assert(_actSpriteBatch != UINT32_MAX);
    _spriteList.push_back(D3D11DrawListEntry(nullptr, _actSpriteBatch, evt, param));
}

void D3D11GraphicsDriver::DestroyDDB(IDriverDependantBitmap* ddb)
{
    // Remove deleted render target DDB from batches backup
    auto* txdata = ((D3D11Bitmap*)ddb)->GetTexture();
    if (txdata && txdata->RenderTarget)
    {
        for (auto& backup_rt : _backupBatches)
        {
            if (backup_rt.RenderTarget == ddb)
            {
                backup_rt.RenderTarget = nullptr;
                backup_rt.RenderView = nullptr;
                backup_rt.Skip = true;
            }
        }
    }
    // Remove deleted DDB from spritelist backup
    for (auto& backup_spr : _backupSpriteList)
    {
        if (backup_spr.ddb == ddb)
            backup_spr.skip = true;
    }
    delete (D3D11Bitmap*)ddb;
}

void D3D11GraphicsDriver::UpdateTextureRegion(D3D11TextureTile* tile, const Bitmap* bitmap, bool opaque)
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = _context->Map(tile->texture.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
    {
        throw Ali3DException("Unable to map texture");
    }

    bool usingLinearFiltering = (_filter && _filter->NeedToColourEdgeLines()) || (_smoothScaling && !opaque);
    uint8_t* memPtr = static_cast<uint8_t*>(mapped.pData);

    if (opaque)
        BitmapToVideoMemOpaque(bitmap, tile, memPtr, mapped.RowPitch);
    else
        BitmapToVideoMem(bitmap, tile, memPtr, mapped.RowPitch, usingLinearFiltering);

    _context->Unmap(tile->texture.get(), 0);
}

void D3D11GraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* ddb, const Bitmap* bitmap)
{
    // FIXME: what to do if texture is shared??
    D3D11Bitmap* target = (D3D11Bitmap*)ddb;
    UpdateTexture(target->GetTexture(), bitmap, target->IsOpaque());
}

void D3D11GraphicsDriver::UpdateTexture(Texture* txdata, const Bitmap* bitmap, bool opaque)
{
    const int color_depth = bitmap->GetColorDepth();
    if (bitmap->GetColorDepth() != txdata->Res.ColorDepth)
        throw Ali3DException("UpdateDDBFromBitmap: mismatched colour depths");
    if (txdata->Res.Width != bitmap->GetWidth() || txdata->Res.Height != bitmap->GetHeight())
        throw Ali3DException("UpdateDDBFromBitmap: mismatched bitmap size");

    if (color_depth == 8)
        select_palette(palette);

    auto* d3ddata = reinterpret_cast<D3D11Texture*>(txdata);
    for (auto& tile : d3ddata->_tiles)
    {
        UpdateTextureRegion(&tile, bitmap, opaque);
    }

    if (color_depth == 8)
        unselect_palette();
}

int D3D11GraphicsDriver::GetCompatibleBitmapFormat(int color_depth)
{
    if (color_depth == 8)
        return 8;
    if (color_depth > 8 && color_depth <= 16)
        return 16;
    return 32;
}

uint64_t D3D11GraphicsDriver::GetAvailableTextureMemory()
{
    if (!_device)
        return 0;
    // Query the current video memory budget (DXGI 1.4, Windows 10+); 0 = not supported
    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIAdapter3> adapter3;
    if (FAILED(_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi_device.Acquire()))) ||
        FAILED(dxgi_device->GetAdapter(adapter.Acquire())) ||
        FAILED(adapter->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(adapter3.Acquire()))))
        return 0;
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (FAILED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
        return 0;
    return info.Budget > info.CurrentUsage ? info.Budget - info.CurrentUsage : 0;
}

IDriverDependantBitmap* D3D11GraphicsDriver::CreateDDB()
{
    return new D3D11Bitmap();
}

IDriverDependantBitmap* D3D11GraphicsDriver::CreateDDB(int width, int height, int color_depth, int txflags)
{
    if (color_depth != GetCompatibleBitmapFormat(color_depth))
        throw Ali3DException("CreateDDB: bitmap colour depth not supported");
    D3D11Bitmap* ddb = new D3D11Bitmap(width, height, color_depth, txflags);
    ddb->SetTexture(std::shared_ptr<D3D11Texture>(
        reinterpret_cast<D3D11Texture*>(CreateTexture(width, height, color_depth, txflags))));
    return ddb;
}

IDriverDependantBitmap* D3D11GraphicsDriver::CreateDDB(std::shared_ptr<Texture> txdata, int txflags)
{
    if (txdata->Res.ColorDepth != GetCompatibleBitmapFormat(txdata->Res.ColorDepth))
        throw Ali3DException("CreateDDB: bitmap colour depth not supported");
    // Use the existing texture directly, do not allocate a new one
    D3D11Bitmap* ddb = new D3D11Bitmap(txdata->Res.Width, txdata->Res.Height, txdata->Res.ColorDepth, txflags);
    ddb->SetTexture(std::static_pointer_cast<D3D11Texture>(txdata));
    return ddb;
}

IDriverDependantBitmap* D3D11GraphicsDriver::CreateRenderTargetDDB(int width, int height, int color_depth, int txflags)
{
    if (color_depth != GetCompatibleBitmapFormat(color_depth))
        throw Ali3DException("CreateDDB: bitmap colour depth not supported");

    txflags |= kTxFlags_RenderTarget;
    auto txdata = std::shared_ptr<D3D11Texture>(
        reinterpret_cast<D3D11Texture*>(CreateTexture(width, height, color_depth, txflags)));
    D3D11Bitmap* ddb = new D3D11Bitmap(width, height, color_depth, txflags);
    ddb->SetTexture(txdata, kTxHint_PremulAlpha);
    return ddb;
}

std::shared_ptr<Texture> D3D11GraphicsDriver::GetTexture(IDriverDependantBitmap* ddb)
{
    return std::static_pointer_cast<Texture>((reinterpret_cast<D3D11Bitmap*>(ddb))->GetSharedTexture());
}

Texture* D3D11GraphicsDriver::CreateTexture(int width, int height, int color_depth, int txflags)
{
    assert(width > 0);
    assert(height > 0);

    // Render targets are always a single texture; the others are split in tiles
    // if they exceed the maximal texture size supported by the feature level.
    const bool as_render_target = (txflags & kTxFlags_RenderTarget) != 0;
    const bool opaque = (txflags & kTxFlags_Opaque) != 0;
    const int max_size = static_cast<int>(_maxTextureSize);

    int tilesAcross = 1, tilesDown = 1;
    if (!as_render_target)
    {
        // Calculate how many textures will be necessary to store this image
        tilesAcross = std::max(1, (width + max_size - 1) / max_size);
        tilesDown = std::max(1, (height + max_size - 1) / max_size);
    }

    const int tileWidth = width / tilesAcross;
    const int lastTileExtraWidth = width % tilesAcross;
    const int tileHeight = height / tilesDown;
    const int lastTileExtraHeight = height % tilesDown;

    auto* txdata = new D3D11Texture(GraphicResolution(width, height, color_depth), as_render_target);
    std::vector<D3D11TextureTile> tiles(tilesAcross * tilesDown);

    // NOTE: pay attention that the texture format does not depend on the source
    // bitmap's color depth: everything is kept in 32-bit
    const DXGI_FORMAT texture_fmt = TextureFormat(opaque);

    for (int x = 0; x < tilesAcross; x++)
    {
        for (int y = 0; y < tilesDown; y++)
        {
            D3D11TextureTile* thisTile = &tiles[y * tilesAcross + x];
            thisTile->x = x * tileWidth;
            thisTile->y = y * tileHeight;
            thisTile->width = tileWidth;
            thisTile->height = tileHeight;
            if (x == tilesAcross - 1)
                thisTile->width += lastTileExtraWidth;
            if (y == tilesDown - 1)
                thisTile->height += lastTileExtraHeight;
            // Direct3D 10+ has no restrictions on texture sizes, so allocate exactly what's needed;
            // this also means texture coordinates of every tile always span 0..1
            thisTile->allocWidth = thisTile->width;
            thisTile->allocHeight = thisTile->height;

            D3D11_TEXTURE2D_DESC td = {};
            td.Width = thisTile->width;
            td.Height = thisTile->height;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = texture_fmt;
            td.SampleDesc.Count = 1;
            if (as_render_target)
            {
                td.Usage = D3D11_USAGE_DEFAULT;
                td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            }
            else
            {
                // Updated by mapping with DISCARD, similar to how D3D9 textures were locked
                td.Usage = D3D11_USAGE_DYNAMIC;
                td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }

            HRESULT hr = _device->CreateTexture2D(&td, nullptr, thisTile->texture.Acquire());
            if (FAILED(hr))
            {
                delete txdata;
                throw Ali3DException(String::FromFormat(
                    "ID3D11Device::CreateTexture2D(X=%d, Y=%d, FMT=%d) failed: error code 0x%08X",
                    td.Width, td.Height, (int)texture_fmt, (unsigned)hr));
            }
            hr = _device->CreateShaderResourceView(thisTile->texture.get(), nullptr, thisTile->srv.Acquire());
            if (FAILED(hr))
            {
                delete txdata;
                throw Ali3DException(String::FromFormat(
                    "ID3D11Device::CreateShaderResourceView failed: error code 0x%08X", (unsigned)hr));
            }
            if (as_render_target)
            {
                hr = _device->CreateRenderTargetView(thisTile->texture.get(), nullptr, thisTile->rtv.Acquire());
                if (FAILED(hr))
                {
                    delete txdata;
                    throw Ali3DException(String::FromFormat(
                        "ID3D11Device::CreateRenderTargetView failed: error code 0x%08X", (unsigned)hr));
                }
            }
        }
    }

    txdata->_tiles = std::move(tiles);
    return txdata;
}

// A value of "ps_4_0_level_9_3" would let compile dx9 shaders, but with macro definitions
// it's possible to achieve the same results and not lose out on modern features
static const char* DefaultShaderCompileTarget = "ps_5_0";
static const char* DefaultShaderEntryPoint = "main";

IGraphicShader* D3D11GraphicsDriver::CreateShaderProgram(const String& name, const char* fragment_shader_src, const ShaderDefinition* def)
{
    const char* compile_target = def && !def->CompileTarget.IsEmpty() ?
        def->CompileTarget.GetCStr() : DefaultShaderCompileTarget;
    const char* entry_point = def && !def->EntryPoint.IsEmpty() ?
        def->EntryPoint.GetCStr() : DefaultShaderEntryPoint;

    // Shader model 2 and 3 targets produce bytecode that Direct3D 11 cannot load
    if (strncmp(compile_target, "ps_2", 4) == 0 || strncmp(compile_target, "ps_3", 4) == 0)
    {
        Debug::Printf(kDbgMsg_Warn, "Direct3D 11: shader \"%s\": compile target \"%s\" is not supported, using \"%s\" instead",
            name.GetCStr(), compile_target, DefaultShaderCompileTarget);
        compile_target = DefaultShaderCompileTarget;
    }

    UINT flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    // Legacy syntax (tex2D, COLOR0, etc) is accepted automatically by the "level_9_x" targets,
    // but must be requested explicitly for the others.
    if (strstr(compile_target, "level_9") == nullptr)
        flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;

    ComPtr<ID3DBlob> blob = CompileShader(fragment_shader_src, name.GetCStr(), entry_point, compile_target, flags, true);
    if (!blob)
        return nullptr;

    D3D11Shader::ProgramData prg;
    if (!CreateShaderProgram(prg, name, blob->GetBufferPointer(), blob->GetBufferSize()))
        return nullptr;

    AssignBaseShaderArgs(prg, def);
    return new D3D11Shader(name, std::move(prg));
}

IGraphicShader* D3D11GraphicsDriver::CreateShaderProgram(const String& name, const std::vector<uint8_t>& compiled_data, const ShaderDefinition* def)
{
    if (compiled_data.empty())
        return nullptr;

    D3D11Shader::ProgramData prg;
    if (!CreateShaderProgram(prg, name, compiled_data.data(), compiled_data.size()))
        return nullptr;

    AssignBaseShaderArgs(prg, def);
    return new D3D11Shader(name, std::move(prg));
}

void D3D11GraphicsDriver::DeleteShaderProgram(IGraphicShader* shader)
{
    assert(shader);
    if (shader)
        delete (D3D11Shader*)shader;
}

IShaderInstance* D3D11GraphicsDriver::CreateShaderInstance(IGraphicShader* shader, const String& name)
{
    assert(shader);
    if (!shader)
        return nullptr;

    return new D3D11ShaderInstance((D3D11Shader*)shader, name);
}

void D3D11GraphicsDriver::DeleteShaderInstance(IShaderInstance* shader_inst)
{
    assert(shader_inst);
    if (shader_inst)
        delete (D3D11ShaderInstance*)shader_inst;
}

void D3D11GraphicsDriver::SetScreenFade(int red, int green, int blue)
{
    assert(_actSpriteBatch != UINT32_MAX);
    D3D11Bitmap* ddb = static_cast<D3D11Bitmap*>(MakeFx(red, green, blue));
    ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
        _spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
    ddb->SetAlpha(255);
    _spriteList.push_back(D3D11DrawListEntry(ddb, _actSpriteBatch, 0, 0));
}

void D3D11GraphicsDriver::SetScreenTint(int red, int green, int blue)
{
    assert(_actSpriteBatch != UINT32_MAX);
    if (red == 0 && green == 0 && blue == 0) return;
    D3D11Bitmap* ddb = static_cast<D3D11Bitmap*>(MakeFx(red, green, blue));
    ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
        _spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
    ddb->SetAlpha(128);
    _spriteList.push_back(D3D11DrawListEntry(ddb, _actSpriteBatch, 0, 0));
}

void D3D11GraphicsDriver::SetScreenShader(IShaderInstance* shinst)
{
    // TODO: expand this to not depend on whether we use a native-resolution surface or not;
    // ideally this setup has to be done outside of gfx driver
    if (_nativeSurface)
        _nativeSurface->SetShader(shinst);
}

bool D3D11GraphicsDriver::SetVsyncImpl(bool enabled, bool& vsync_res)
{
    // In Direct3D 11 vsync is simply a Present() parameter, no device reset is needed
    _vsync = enabled;
    vsync_res = enabled;
    return true;
}


D3D11GraphicsFactory* D3D11GraphicsFactory::_factory = nullptr;
Library D3D11GraphicsFactory::_libDXGI;
Library D3D11GraphicsFactory::_libD3D11;
Library D3D11GraphicsFactory::_libCompiler;

D3D11GraphicsFactory::~D3D11GraphicsFactory()
{
    DestroyDriver(); // driver must be destroyed before the libraries are disposed
    uint64_t ref_cnt = _api.Factory.ReleaseAndCheck();
    if (ref_cnt > 0)
        Debug::Printf(kDbgMsg_Warn, "WARNING: Not all of the Direct3D 11 resources have been disposed; IDXGIFactory ref count: %llu", ref_cnt);
    _factory = nullptr;
}

size_t D3D11GraphicsFactory::GetFilterCount() const
{
    return 2;
}

const GfxFilterInfo* D3D11GraphicsFactory::GetFilterInfo(size_t index) const
{
    switch (index)
    {
    case 0:
        return &D3DGfxFilter::FilterInfo;
    case 1:
        return &AAD3DGfxFilter::FilterInfo;
    default:
        return nullptr;
    }
}

String D3D11GraphicsFactory::GetDefaultFilterID() const
{
    return D3DGfxFilter::FilterInfo.Id;
}

/* static */ D3D11GraphicsFactory* D3D11GraphicsFactory::GetFactory()
{
    if (!_factory)
    {
        _factory = new D3D11GraphicsFactory();
        if (!_factory->Init())
        {
            delete _factory;
            _factory = nullptr;
        }
    }
    return _factory;
}

/* static */ D3D11GraphicsDriver* D3D11GraphicsFactory::GetD3D11Driver()
{
    if (!_factory)
        _factory = GetFactory();
    if (_factory)
        return _factory->EnsureDriverCreated();
    return nullptr;
}

D3D11GraphicsDriver* D3D11GraphicsFactory::EnsureDriverCreated()
{
    if (!_driver)
    {
        _driver = new D3D11GraphicsDriver(_factory->_api);
    }
    return _driver;
}

D3DGfxFilter* D3D11GraphicsFactory::CreateFilter(const String& id)
{
    if (D3DGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new D3DGfxFilter();
    else if (AAD3DGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new AAD3DGfxFilter();
    return nullptr;
}

bool D3D11GraphicsFactory::Init()
{
    assert(_api.Factory == nullptr);
    if (_api.Factory)
        return true;

    if (!_libD3D11.Load("d3d11"))
    {
        SDL_SetError("Direct3D 11 is not installed");
        return false;
    }
    _api.CreateDevice = (PFN_D3D11_CREATE_DEVICE)_libD3D11.GetFunctionAddress("D3D11CreateDevice");
    if (!_api.CreateDevice)
    {
        _libD3D11.Unload();
        SDL_SetError("Entry point not found in d3d11.dll");
        return false;
    }

    if (!_libDXGI.Load("dxgi"))
    {
        SDL_SetError("DXGI is not installed");
        return false;
    }
    typedef HRESULT(WINAPI* CreateDXGIFactory1Fn)(REFIID, void**);
    CreateDXGIFactory1Fn create_factory = (CreateDXGIFactory1Fn)_libDXGI.GetFunctionAddress("CreateDXGIFactory1");
    if (!create_factory)
    {
        _libDXGI.Unload();
        SDL_SetError("Entry point not found in dxgi.dll");
        return false;
    }
    HRESULT hr = create_factory(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(_api.Factory.Acquire()));
    if (FAILED(hr) || !_api.Factory)
    {
        _libDXGI.Unload();
        SDL_SetError("CreateDXGIFactory1 failed: 0x%08X", (unsigned)hr);
        return false;
    }

    // Shaders are compiled at runtime: the built-in ones, and the user's ones.
    // D3DCompiler_47 is a part of Windows 8.1+, and available as an update for Windows 7.
    if (!_libCompiler.Load("d3dcompiler_47"))
    {
        SDL_SetError("D3DCompiler_47.dll is not found");
        return false;
    }
    _api.Compile = (PFN_D3DCompileFn)_libCompiler.GetFunctionAddress("D3DCompile");
    if (!_api.Compile)
    {
        _libCompiler.Unload();
        SDL_SetError("Entry point not found in d3dcompiler_47.dll");
        return false;
    }

    ComPtr<IDXGIAdapter1> adapter;
    if (SUCCEEDED(_api.Factory->EnumAdapters1(0, adapter.Acquire())))
    {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);
        char desc_str[256] = {};
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, desc_str, sizeof(desc_str) - 1, nullptr, nullptr);
        String adapter_info = String::FromFormat(
            "\tDescription: %s\n\tDedicated video memory: %llu MB",
            desc_str, (unsigned long long)(desc.DedicatedVideoMemory / (1024 * 1024)));
        LARGE_INTEGER umd_version = {};
        if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd_version)))
        {
            adapter_info.Append(String::FromFormat("\n\tDriver: v%d.%d.%d.%d",
                HIWORD(umd_version.HighPart), LOWORD(umd_version.HighPart),
                HIWORD(umd_version.LowPart), LOWORD(umd_version.LowPart)));
        }
        Debug::Printf(kDbgMsg_Info, "Direct3D 11 adapter info:\n%s", adapter_info.GetCStr());
    }
    return true;
}

} // namespace D3D11
} // namespace Engine
} // namespace AGS

#endif // AGS_HAS_DIRECT3D11

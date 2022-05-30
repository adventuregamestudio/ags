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
#include "gfx/ogl_headers.h"

#if AGS_HAS_OPENGL
#include "gfx/ali3dogl.h"
#include <algorithm>
#include <SDL.h>
#include "ac/sys_events.h"
#include "ac/timer.h"
#include "debug/out.h"
#include "gfx/ali3dexception.h"
#include "gfx/gfx_def.h"
#include "gfx/gfxfilter_ogl.h"
#include "gfx/gfxfilter_aaogl.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"

// OpenGL Mathematics Library. We could include only the features we need to decrease compilation time.
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "glad/glad.h"

#if AGS_OPENGL_ES2
  float get_device_scale();

#define glOrtho glOrthof
#define GL_CLAMP GL_CLAMP_TO_EDGE

const char* fbo_extension_string = "GL_OES_framebuffer_object";

#define glGenFramebuffersEXT glGenFramebuffers
#define glDeleteFramebuffersEXT glDeleteFramebuffers
#define glBindFramebufferEXT glBindFramebuffer
#define glCheckFramebufferStatusEXT glCheckFramebufferStatus
#define glGetFramebufferAttachmentParameterivEXT glGetFramebufferAttachmentParameteriv
#define glGenerateMipmapEXT glGenerateMipmap
#define glFramebufferTexture2DEXT glFramebufferTexture2D
#define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
// TODO: probably should use EGL and function eglSwapInterval on mobile to support setting swap interval
// For now this is a dummy function pointer which is only used to test that function is not supported
const void (*glSwapIntervalEXT)(int) = NULL;

#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0

#endif //AGS_OPENGL_ES2

// Necessary to update textures from 8-bit bitmaps
extern RGB palette[256];

#define AGS_OGLBLENDOP(blend_op, src_blend, dest_blend) \
  glBlendEquation(blend_op); \
  glBlendFunc(src_blend, dest_blend); \


namespace AGS
{
namespace Engine
{
namespace OGL
{

using namespace AGS::Common;

OGLTextureData::~OGLTextureData()
{
    if (_tiles)
    {
        for (size_t i = 0; i < _numTiles; ++i)
            glDeleteTextures(1, &(_tiles[i].texture));
        delete[] _tiles;
    }
    if (_vertex)
    {
        delete[] _vertex;
    }
}


OGLGraphicsDriver::OGLGraphicsDriver()
{
  device_screen_physical_width  = 0;
  device_screen_physical_height = 0;
#if AGS_PLATFORM_OS_IOS
  device_screen_physical_width  = ios_screen_physical_width;
  device_screen_physical_height = ios_screen_physical_height;
#endif

  _firstTimeInit = false;
  _backbuffer = 0;
  _fbo = 0;
  _legacyPixelShader = false;
  _can_render_to_texture = false;
  _do_render_to_texture = false;
  _super_sampling = 1;
  SetupDefaultVertices();

  // Shifts comply to GL_RGBA
  _vmem_r_shift_32 = 0;
  _vmem_g_shift_32 = 8;
  _vmem_b_shift_32 = 16;
  _vmem_a_shift_32 = 24;
}


void OGLGraphicsDriver::SetupDefaultVertices()
{
  std::fill(_backbuffer_vertices, _backbuffer_vertices + sizeof(_backbuffer_vertices) / sizeof(GLfloat), 0.0f);
  std::fill(_backbuffer_texture_coordinates, _backbuffer_texture_coordinates + sizeof(_backbuffer_texture_coordinates) / sizeof(GLfloat), 0.0f);

  defaultVertices[0].position.x = 0.0f;
  defaultVertices[0].position.y = 0.0f;
  defaultVertices[0].tu=0.0;
  defaultVertices[0].tv=0.0;

  defaultVertices[1].position.x = 1.0f;
  defaultVertices[1].position.y = 0.0f;
  defaultVertices[1].tu=1.0;
  defaultVertices[1].tv=0.0;

  defaultVertices[2].position.x = 0.0f;
  defaultVertices[2].position.y = -1.0f;
  defaultVertices[2].tu=0.0;
  defaultVertices[2].tv=1.0;

  defaultVertices[3].position.x = 1.0f;
  defaultVertices[3].position.y = -1.0f;
  defaultVertices[3].tu=1.0;
  defaultVertices[3].tv=1.0;
}

void OGLGraphicsDriver::UpdateDeviceScreen(const Size &/*screen_size*/)
{
    SDL_GL_GetDrawableSize(_sdlWindow, &device_screen_physical_width, &device_screen_physical_height);
    Debug::Printf("OGL: notified of device screen updated to %d x %d, resizing viewport", device_screen_physical_width, device_screen_physical_height);
    _mode.Width = device_screen_physical_width;
    _mode.Height = device_screen_physical_height;
}

void OGLGraphicsDriver::Vsync()
{
  // do nothing on OpenGL
}

void OGLGraphicsDriver::RenderSpritesAtScreenResolution(bool enabled, int supersampling)
{
  if (_can_render_to_texture)
  {
    _do_render_to_texture = !enabled;
    _super_sampling = supersampling;
    TestSupersampling();
  }

  if (_do_render_to_texture)
    glDisable(GL_SCISSOR_TEST);
}

bool OGLGraphicsDriver::IsModeSupported(const DisplayMode &mode)
{
  if (mode.Width <= 0 || mode.Height <= 0)
  {
    SDL_SetError("Invalid resolution parameters: %d x %d", mode.Width, mode.Height);
    return false;
  }
  if (mode.ColorDepth != 32) {
    SDL_SetError("Display colour depth not supported: %d", mode.ColorDepth);
    return false;
  }
  return true;
}

bool OGLGraphicsDriver::SupportsGammaControl()
{
  return false;
}

void OGLGraphicsDriver::SetGamma(int /*newGamma*/)
{
}

void OGLGraphicsDriver::SetGraphicsFilter(POGLFilter filter)
{
  _filter = filter;
  OnSetFilter();
}

void OGLGraphicsDriver::SetTintMethod(TintMethod method)
{
  _legacyPixelShader = (method == TintReColourise);
}

bool OGLGraphicsDriver::FirstTimeInit()
{
  String ogl_v_str;
#ifdef GLAPI
  ogl_v_str.Format("%d.%d", GLVersion.major, GLVersion.minor);
#else
  ogl_v_str = (const char*)glGetString(GL_VERSION);
#endif
  Debug::Printf(kDbgMsg_Info, "Running OpenGL: %s", ogl_v_str.GetCStr());

  // Initialize default sprite batch, it will be used when no other batch was activated
  OGLGraphicsDriver::InitSpriteBatch(0, _spriteBatchDesc[0]);

  TestRenderToTexture();

  if(!CreateShaders()) { // requires glad Load successful
    SDL_SetError("Failed to create Shaders.");
    return false;
  }
  _firstTimeInit = true;
  return true;
}

bool OGLGraphicsDriver::InitGlScreen(const DisplayMode &mode)
{
  SDL_Window* window = sys_get_window();
  if (window == nullptr || (SDL_GetWindowFlags(window) & SDL_WINDOW_OPENGL) == 0)
  {
    if (!CreateWindowAndGlContext(mode))
      return false;
  }
  else
  {
    sys_window_set_style(mode.Mode, Size(mode.Width, mode.Height));
  }

  SDL_GL_GetDrawableSize(_sdlWindow, &device_screen_physical_width, &device_screen_physical_height);
  _mode.Width = device_screen_physical_width;
  _mode.Height = device_screen_physical_height;
  return true;
}

void OGLGraphicsDriver::InitGlParams(const DisplayMode &mode)
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);


  bool vsyncEnabled = SDL_GL_SetSwapInterval(mode.Vsync ? 1 : 0) == 0;
  if (mode.Vsync && !vsyncEnabled)
    Debug::Printf(kDbgMsg_Warn, "WARNING: Vertical sync could not be enabled. Setting will be kept at driver default.");

#if AGS_PLATFORM_OS_IOS
  // Setup library mouse to have 1:1 coordinate transformation.
  // NOTE: cannot move this call to general mouse handling mode. Unfortunately, much of the setup and rendering
  // is duplicated in the Android/iOS ports' Allegro library patches, and is run when the Software renderer
  // is selected in AGS. This ugly situation causes trouble...
  float device_scale = 1.0f;

  device_scale = get_device_scale();

  device_mouse_setup(0, device_screen_physical_width - 1, 0, device_screen_physical_height - 1, device_scale, device_scale);
#endif

  // View matrix is always identity in OpenGL renderer, use the workaround to fill it with GL format
  _stageMatrixes.View = glm::mat4(1.0);
}

bool OGLGraphicsDriver::CreateWindowAndGlContext(const DisplayMode &mode)
{
  // First setup GL attributes before creating SDL GL window
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) != 0)
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Error occured setting attribute SDL_GL_CONTEXT_PROFILE_MASK: %s", SDL_GetError());
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0)
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Error occured setting attribute SDL_GL_CONTEXT_MAJOR_VERSION: %s", SDL_GetError());
#if AGS_OPENGL_ES2
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) != 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Error occured setting attribute SDL_GL_CONTEXT_MINOR_VERSION: %s", SDL_GetError());
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1) != 0) {
        Debug::Printf(kDbgMsg_Warn, "Error occured setting attribute SDL_GL_CONTEXT_EGL: %s", SDL_GetError());
    }
#else
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) != 0)
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Error occured setting attribute SDL_GL_CONTEXT_MINOR_VERSION: %s", SDL_GetError());
#endif
  if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Error occured setting attribute SDL_GL_DOUBLEBUFFER: %s", SDL_GetError());

  SDL_Window *sdl_window = sys_window_create("", mode.Width, mode.Height, mode.Mode, SDL_WINDOW_OPENGL);
  if (!sdl_window)
  {
    Debug::Printf(kDbgMsg_Error, "Error opening window for OpenGL: %s", SDL_GetError());
    return false;
  }

  SDL_GLContext sdlgl_ctx = SDL_GL_CreateContext(sdl_window);
  if (sdlgl_ctx == NULL) {
    Debug::Printf(kDbgMsg_Error, "Error creating OpenGL context: %s", SDL_GetError());
    sys_window_destroy();
    return false;
  }

  if (SDL_GL_MakeCurrent(sdl_window, sdlgl_ctx) != 0) {
    Debug::Printf(kDbgMsg_Error, "Error setting current OpenGL context: %s", SDL_GetError());
    SDL_GL_DeleteContext(sdlgl_ctx);
    sys_window_destroy();
    return false;
  }
#if AGS_OPENGL_ES2
    if (!gladLoadGLES2Loader((GLADloadproc) SDL_GL_GetProcAddress)) {
        Debug::Printf(kDbgMsg_Error, "Failed to load glad with gladLoadGLES2Loader");
    }
#else
  if (!gladLoadGL()) {
    Debug::Printf(kDbgMsg_Error, "Failed to load GL.");
    return false;
  }
#endif
  _sdlWindow = sdl_window;
  _sdlGlContext = sdlgl_ctx;
  return true;
}

void OGLGraphicsDriver::DeleteWindowAndGlContext()
{
  SDL_GL_MakeCurrent(nullptr, nullptr);
  if (_sdlGlContext) {
    SDL_GL_DeleteContext(_sdlGlContext);
  }
  _sdlGlContext = nullptr;
  sys_window_destroy();
  _sdlWindow = nullptr;
}

inline bool CanDoFrameBuffer()
{
// this has to be redone because it's too confusing
#if AGS_OPENGL_ES2
    return true;
#else
#ifdef GLAPI
  return GLAD_GL_EXT_framebuffer_object != 0;
#else
#if AGS_PLATFORM_OS_IOS
  const char* fbo_extension_string = "GL_OES_framebuffer_object";
#else
  const char* fbo_extension_string = "GL_EXT_framebuffer_object";
#endif
  const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
  return extensions && strstr(extensions, fbo_extension_string) != NULL;
#endif
#endif
}

void OGLGraphicsDriver::TestRenderToTexture()
{
  if (CanDoFrameBuffer()) {
    _can_render_to_texture = true;
    TestSupersampling();
  } else {
    _can_render_to_texture = false;
    Debug::Printf(kDbgMsg_Warn, "WARNING: OpenGL extension 'GL_EXT_framebuffer_object' not supported, rendering to texture mode will be disabled.");
  }

  if (!_can_render_to_texture)
    _do_render_to_texture = false;
}

void OGLGraphicsDriver::TestSupersampling()
{
    if (!_can_render_to_texture)
        return;
    // Disable super-sampling if it would cause a too large texture size
    if (_super_sampling > 1)
    {
      int max = 1024;
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
      if ((max < _srcRect.GetWidth() * _super_sampling) || (max < _srcRect.GetHeight() * _super_sampling))
        _super_sampling = 1;
    }
}



bool CreateTransparencyShader(ShaderProgram &prg);
bool CreateTintShader(ShaderProgram &prg);
bool CreateLightShader(ShaderProgram &prg);
bool CreateShaderProgram(ShaderProgram &prg, const char *name, const char *vertex_shader_src, const char *fragment_shader_src);
void DeleteShaderProgram(ShaderProgram &prg);
void OutputShaderError(GLuint obj_id, const String &obj_name, bool is_shader);


bool OGLGraphicsDriver::CreateShaders()
{
#if AGS_OPENGL_ES2
  if (!GLAD_GL_ES_VERSION_2_0) {
#else
  if (!GLAD_GL_VERSION_2_0) {
#endif
    Debug::Printf(kDbgMsg_Error, "ERROR: Shaders require a minimum of OpenGL 2.0 support.");
    return false;
  }
  bool shaders_created = true;
  shaders_created &= CreateTransparencyShader(_transparencyShader);
  shaders_created &= CreateTintShader(_tintShader);
  shaders_created &= CreateLightShader(_lightShader);
  return shaders_created;
}



static const auto default_vertex_shader_src =  ""
#if AGS_OPENGL_ES2
"#version 100 \n"
#else
"#version 120 \n"
#endif
R"EOS(
uniform mat4 uMVPMatrix;

attribute vec2 a_Position;
attribute vec2 a_TexCoord;

varying vec2 v_TexCoord;

void main() {
  v_TexCoord = a_TexCoord;
  gl_Position = uMVPMatrix * vec4(a_Position.xy, 0.0, 1.0);
  // gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

)EOS";


static const auto transparency_fragment_shader_src = ""
#if AGS_OPENGL_ES2
"#version 100 \n"
"precision mediump float; \n"
#else
"#version 120 \n"
#endif
R"EOS(
uniform sampler2D textID;
uniform float alpha;

varying vec2 v_TexCoord;

void main()
{
  vec4 src_col = texture2D(textID, v_TexCoord);
  gl_FragColor = vec4(src_col.xyz, src_col.w * alpha);
  // gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)EOS";


// NOTE: this shader emulates "historical" AGS software tinting; it is not
// necessarily "proper" tinting in modern terms.
// The RGB-HSV-RGB conversion found in the Internet (copyright unknown);
// Color processing is replicated from Direct3D shader by Chris Jones
// (Engine/resource/tintshaderLegacy.fx).

// Uniforms:
// textID - texture index (usually 0),
// tintHSV - tint color in HSV,
// tintAmnTrsLum - tint parameters: amount, translucence (alpha), luminance.

static const auto tint_fragment_shader_src = ""
#if AGS_OPENGL_ES2
"#version 100 \n"
"precision mediump float; \n"
#else
"#version 120 \n"
#endif
R"EOS(
uniform sampler2D textID;
uniform vec3 tintHSV;
uniform float tintAmount;
uniform float tintLuminance;
uniform float alpha;

varying vec2 v_TexCoord;

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    const float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float getValue(vec3 color)
{
    float colorMax = max (color[0], color[1]);
    colorMax = max (colorMax, color[2]);
    return colorMax;
}

void main()
{
    vec4 src_col = texture2D(textID, v_TexCoord);

    float lum = getValue(src_col.xyz);
    lum = max(lum - (1.0 - tintLuminance), 0.0);
    vec3 new_col = (hsv2rgb(vec3(tintHSV[0], tintHSV[1], lum)) * tintAmount + src_col.xyz * (1.0 - tintAmount));
    gl_FragColor = vec4(new_col, src_col.w * alpha);

    // gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)EOS";


// NOTE: due to how the lighting works in AGS, this is combined MODULATE / ADD shader.
// if the light is < 0, then MODULATE operation is used, otherwise ADD is used.
// NOTE: it's been said that using branching in shaders produces inefficient code.
// If that will ever become a real problem, we can easily split this shader in two.

// Uniforms:
// textID - texture index (usually 0),
// light - light level,
// alpha - color alpha value.

static const auto light_fragment_shader_src = ""
#if AGS_OPENGL_ES2
"#version 100 \n"
"precision mediump float; \n"
#else
"#version 120 \n"
#endif
R"EOS(
uniform sampler2D textID;
uniform float light;
uniform float alpha;

varying vec2 v_TexCoord;

void main()
{
    vec4 src_col = texture2D(textID, v_TexCoord);

   if (light >= 0.0)
       gl_FragColor = vec4(src_col.xyz + vec3(light, light, light), src_col.w * alpha);
   else
       gl_FragColor = vec4(src_col.xyz * abs(light), src_col.w * alpha);

    // gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
}
)EOS";


bool CreateTransparencyShader(ShaderProgram &prg)
{
  if(!CreateShaderProgram(prg, "Transparency", default_vertex_shader_src, transparency_fragment_shader_src)) return false;
  prg.MVPMatrix = glGetUniformLocation(prg.Program, "uMVPMatrix");
  prg.TextureId = glGetUniformLocation(prg.Program, "textID");
  prg.Alpha = glGetUniformLocation(prg.Program, "alpha");
  return true;
}


bool CreateTintShader(ShaderProgram &prg)
{
  if(!CreateShaderProgram(prg, "Tinting", default_vertex_shader_src, tint_fragment_shader_src)) return false;
  prg.MVPMatrix = glGetUniformLocation(prg.Program, "uMVPMatrix");
  prg.TextureId = glGetUniformLocation(prg.Program, "textID");
  prg.TintHSV = glGetUniformLocation(prg.Program, "tintHSV");
  prg.TintAmount = glGetUniformLocation(prg.Program, "tintAmount");
  prg.TintLuminance = glGetUniformLocation(prg.Program, "tintLuminance");
  prg.Alpha = glGetUniformLocation(prg.Program, "alpha");
  return true;
}

bool CreateLightShader(ShaderProgram &prg)
{
  if(!CreateShaderProgram(prg, "Lighting", default_vertex_shader_src, light_fragment_shader_src)) return false;
  prg.MVPMatrix = glGetUniformLocation(prg.Program, "uMVPMatrix");
  prg.TextureId = glGetUniformLocation(prg.Program, "textID");
  prg.LightingAmount = glGetUniformLocation(prg.Program, "light");
  prg.Alpha = glGetUniformLocation(prg.Program, "alpha");
  return true;
}



bool CreateShaderProgram(ShaderProgram &prg, const char *name, const char *vertex_shader_src, const char *fragment_shader_src)
{
  GLint result;

  GLint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
  if (result == GL_FALSE)
  {
    OutputShaderError(vertex_shader, String::FromFormat("%s program's vertex shader", name), true);
    return false;
  }

  GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
  if (result == GL_FALSE)
  {
    OutputShaderError(fragment_shader, String::FromFormat("%s program's fragment shader", name), true);
    glDeleteShader(fragment_shader); //not sure yet if this goes here
    return false;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &result);
  if(result == GL_FALSE)
  {
    OutputShaderError(program, String::FromFormat("%s program", name), false);
    glDeleteProgram(program); //not sure yet if this goes here
    glDeleteShader(fragment_shader); //not sure yet if this goes here
    return false;
  }

  glDetachShader(program, vertex_shader);
  glDeleteShader(vertex_shader);

  glDetachShader(program, fragment_shader);
  glDeleteShader(fragment_shader);

  prg.Program = program;
  Debug::Printf("OGL: %s shader program created successfully", name);
  return true;
}

void DeleteShaderProgram(ShaderProgram &prg)
{
  if (prg.Program)
    glDeleteProgram(prg.Program);
  prg.Program = 0;
}

void OutputShaderError(GLuint obj_id, const String &obj_name, bool is_shader)
{
  GLint log_len;
  if (is_shader)
    glGetShaderiv(obj_id, GL_INFO_LOG_LENGTH, &log_len);
  else
    glGetProgramiv(obj_id, GL_INFO_LOG_LENGTH, &log_len);
  std::vector<GLchar> errorLog(log_len);
  if (log_len > 0)
  {
    if (is_shader)
      glGetShaderInfoLog(obj_id, log_len, &log_len, &errorLog[0]);
    else
      glGetProgramInfoLog(obj_id, log_len, &log_len, &errorLog[0]);
  }

  Debug::Printf(kDbgMsg_Error, "ERROR: OpenGL: %s %s:", obj_name.GetCStr(), is_shader ? "failed to compile" : "failed to link");
  if (errorLog.size() > 0)
  {
    Debug::Printf(kDbgMsg_Error, "----------------------------------------");
    Debug::Printf(kDbgMsg_Error, "%s", &errorLog[0]);
    Debug::Printf(kDbgMsg_Error, "----------------------------------------");
  }
  else
  {
    Debug::Printf(kDbgMsg_Error, "Shader info log was empty.");
  }
}

void OGLGraphicsDriver::SetupBackbufferTexture()
{
  // NOTE: ability to render to texture depends on OGL context, which is
  // created in SetDisplayMode, therefore creation of textures require
  // both native size set and context capabilities test passed.
  if (!IsNativeSizeValid() || !_can_render_to_texture)
    return;

  DeleteBackbufferTexture();

  // _backbuffer_texture_coordinates defines translation from wanted texture size to actual supported texture size
  _backRenderSize = _srcRect.GetSize() * _super_sampling;
  _backTextureSize = _backRenderSize;
  AdjustSizeToNearestSupportedByCard(&_backTextureSize.Width, &_backTextureSize.Height);
  const float back_ratio_w = (float)_backRenderSize.Width / (float)_backTextureSize.Width;
  const float back_ratio_h = (float)_backRenderSize.Height / (float)_backTextureSize.Height;
  std::fill(_backbuffer_texture_coordinates, _backbuffer_texture_coordinates + sizeof(_backbuffer_texture_coordinates) / sizeof(GLfloat), 0.0f);
  _backbuffer_texture_coordinates[2] = _backbuffer_texture_coordinates[6] = back_ratio_w;
  _backbuffer_texture_coordinates[5] = _backbuffer_texture_coordinates[7] = back_ratio_h;

  glGenTextures(1, &_backbuffer);
  glBindTexture(GL_TEXTURE_2D, _backbuffer);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _backTextureSize.Width, _backTextureSize.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenFramebuffersEXT(1, &_fbo);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _backbuffer, 0);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  // Assign vertices of the backbuffer texture position in the scene
  _backbuffer_vertices[0] = _backbuffer_vertices[4] = 0;
  _backbuffer_vertices[2] = _backbuffer_vertices[6] = _srcRect.GetWidth();
  _backbuffer_vertices[5] = _backbuffer_vertices[7] = _srcRect.GetHeight();
  _backbuffer_vertices[1] = _backbuffer_vertices[3] = 0;
}

void OGLGraphicsDriver::DeleteBackbufferTexture()
{
  if (_backbuffer)
    glDeleteTextures(1, &_backbuffer);
  if (_fbo)
    glDeleteFramebuffersEXT(1, &_fbo);
  _backbuffer = 0;
  _fbo = 0;
}

void OGLGraphicsDriver::SetupViewport()
{
  if (!IsModeSet() || !IsRenderFrameValid())
    return;

  // Setup viewport rect and scissor
  _viewportRect = ConvertTopDownRect(_dstRect, device_screen_physical_height);
  glScissor(_viewportRect.Left, _viewportRect.Top, _viewportRect.GetWidth(), _viewportRect.GetHeight());
}

Rect OGLGraphicsDriver::ConvertTopDownRect(const Rect &rect, int surface_height)
{
    return RectWH(rect.Left, surface_height - 1 - rect.Bottom, rect.GetWidth(), rect.GetHeight());
}

bool OGLGraphicsDriver::SetDisplayMode(const DisplayMode &mode)
{
  ReleaseDisplayMode();

  if (mode.ColorDepth < 15)
  {
    SDL_SetError("OpenGL driver does not support 256-color display mode");
    return false;
  }

  try
  {
    if (!InitGlScreen(mode))
      return false;
    if (!_firstTimeInit)
      if(!FirstTimeInit()) return false;
    InitGlParams(mode);
  }
  catch (Ali3DException exception)
  {
    if (exception._message != SDL_GetError())
      SDL_SetError("%s", exception._message);
    return false;
  }

  OnInit();

  // On certain platforms OpenGL renderer ignores requested screen sizes
  // and uses values imposed by the operating system (device).
  DisplayMode final_mode = mode;
  final_mode.Width = device_screen_physical_width;
  final_mode.Height = device_screen_physical_height;
  OnModeSet(final_mode);

  // If we already have a native size set, then update virtual screen and setup backbuffer texture immediately
  CreateVirtualScreen();
  SetupBackbufferTexture();
  // If we already have a render frame configured, then setup viewport and backbuffer mappings immediately
  SetupViewport();
  return true;
}

void OGLGraphicsDriver::CreateVirtualScreen()
{
  if (!IsModeSet() || !IsNativeSizeValid())
    return;
  // create initial stage screen for plugin raw drawing
  CreateStageScreen(0, _srcRect.GetSize());
  _stageScreen = GetStageScreen(0);
}

bool OGLGraphicsDriver::SetNativeResolution(const GraphicResolution &native_res)
{
  OnSetNativeRes(native_res);
  SetupBackbufferTexture();
  // If we already have a gfx mode set, then update virtual screen immediately
  CreateVirtualScreen();
  TestSupersampling();
  return !_srcRect.IsEmpty();
}

bool OGLGraphicsDriver::SetRenderFrame(const Rect &dst_rect)
{
  if (!IsNativeSizeValid())
    return false;
  OnSetRenderFrame(dst_rect);
  // Also make sure viewport and backbuffer mappings are updated using new native & destination rectangles
  SetupViewport();
  return !_dstRect.IsEmpty();
}

int OGLGraphicsDriver::GetDisplayDepthForNativeDepth(int /*native_color_depth*/) const
{
    // TODO: check for device caps to know which depth is supported?
    return 32;
}

IGfxModeList *OGLGraphicsDriver::GetSupportedModeList(int /*color_depth*/)
{
    std::vector<DisplayMode> modes {};
    sys_get_desktop_modes(modes);
    return new OGLDisplayModeList(modes);
}

PGfxFilter OGLGraphicsDriver::GetGraphicsFilter() const
{
    return _filter;
}

void OGLGraphicsDriver::ReleaseDisplayMode()
{
  if (!IsModeSet())
    return;

  OnModeReleased();
  ClearDrawLists();
  ClearDrawBackups();
  DeleteBackbufferTexture();
  DestroyFxPool();
  DestroyAllStageScreens();

  sys_window_set_style(kWnd_Windowed);
}

void OGLGraphicsDriver::UnInit()
{
  OnUnInit();
  ReleaseDisplayMode();

  DeleteShaderProgram(_transparencyShader);
  DeleteShaderProgram(_tintShader);
  DeleteShaderProgram(_lightShader);

  DeleteWindowAndGlContext();
  sys_window_destroy();
}

OGLGraphicsDriver::~OGLGraphicsDriver()
{
  OGLGraphicsDriver::UnInit();
}

void OGLGraphicsDriver::ClearRectangle(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, RGB* /*colorToUse*/)
{
  // NOTE: this function is practically useless at the moment, because OGL redraws whole game frame each time
}

bool OGLGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt)
{
  (void)at_native_res; // TODO: support this at some point

  // TODO: following implementation currently only reads GL pixels in 32-bit RGBA.
  // this **should** work regardless of actual display mode because OpenGL is
  // responsible to convert and fill pixel buffer correctly.
  // If you like to support writing directly into 16-bit bitmap, please take
  // care of ammending the pixel reading code below.
  const int read_in_colordepth = 32;
  Size need_size = _do_render_to_texture ? _backRenderSize : _dstRect.GetSize();
  if (destination->GetColorDepth() != read_in_colordepth || destination->GetSize() != need_size)
  {
    if (want_fmt)
      *want_fmt = GraphicResolution(need_size.Width, need_size.Height, read_in_colordepth);
    return false;
  }

  Rect retr_rect;
  if (_do_render_to_texture)
  {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
    retr_rect = RectWH(0, 0, _backRenderSize.Width, _backRenderSize.Height);
  }
  else
  {
#if !AGS_OPENGL_ES2
    glReadBuffer(GL_FRONT);
#endif

    retr_rect = _dstRect;
  }

  int bpp = read_in_colordepth / 8;
  int bufferSize = retr_rect.GetWidth() * retr_rect.GetHeight() * bpp;

  unsigned char* buffer = new unsigned char[bufferSize];
  if (buffer)
  {
    glReadPixels(retr_rect.Left, retr_rect.Top, retr_rect.GetWidth(), retr_rect.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    unsigned char* sourcePtr = buffer;
    for (int y = destination->GetHeight() - 1; y >= 0; y--)
    {
      unsigned int * destPtr = reinterpret_cast<unsigned int*>(&destination->GetScanLineForWriting(y)[0]);
      for (int dx = 0, sx = 0; dx < destination->GetWidth(); ++dx, sx = dx * bpp)
      {
        destPtr[dx] = makeacol32(sourcePtr[sx + 0], sourcePtr[sx + 1], sourcePtr[sx + 2], sourcePtr[sx + 3]);
      }
      sourcePtr += retr_rect.GetWidth() * bpp;
    }

    if (_pollingCallback)
      _pollingCallback();

    delete [] buffer;
  }
  return true;
}

void OGLGraphicsDriver::RenderToBackBuffer()
{
  throw Ali3DException("OGL driver does not have a back buffer");
}

void OGLGraphicsDriver::Render()
{
  Render(0, 0, kFlip_None);
}

void OGLGraphicsDriver::Render(int /*xoff*/, int /*yoff*/, GlobalFlipType /*flip*/)
{
  _render(true);
}

void OGLGraphicsDriver::_reDrawLastFrame()
{
    RestoreDrawLists();
}

void OGLGraphicsDriver::_renderSprite(const OGLDrawListEntry *drawListEntry,
    const glm::mat4 &projection, const glm::mat4 &matGlobal, const SpriteColorTransform &color)
{
  OGLBitmap *bmpToDraw = drawListEntry->ddb;

  const int alpha = (color.Alpha * bmpToDraw->_alpha) / 255;

  ShaderProgram program;

  const bool do_tint = bmpToDraw->_tintSaturation > 0 && _tintShader.Program > 0;
  const bool do_light = bmpToDraw->_tintSaturation == 0 && bmpToDraw->_lightLevel > 0 && _lightShader.Program > 0;
  // const bool can_do_transp_shader =_transparencyShader.Program > 0; if this fails, the whole renderer can't work.
  if (do_tint)
  {
    // Use tinting shader
    program = _tintShader;
    glUseProgram(_tintShader.Program);

    float rgb[3];
    float sat_trs_lum[3]; // saturation / transparency / luminance
    if (_legacyPixelShader)
    {
      rgb_to_hsv(bmpToDraw->_red, bmpToDraw->_green, bmpToDraw->_blue, &rgb[0], &rgb[1], &rgb[2]);
      rgb[0] /= 360.0; // In HSV, Hue is 0-360
    }
    else
    {
      rgb[0] = (float)bmpToDraw->_red / 255.0;
      rgb[1] = (float)bmpToDraw->_green / 255.0;
      rgb[2] = (float)bmpToDraw->_blue / 255.0;
    }

    sat_trs_lum[0] = (float)bmpToDraw->_tintSaturation / 255.0;

    if (bmpToDraw->_lightLevel > 0)
      sat_trs_lum[2] = (float)bmpToDraw->_lightLevel / 255.0;
    else
      sat_trs_lum[2] = 1.0f;

    glUniform3f(_tintShader.TintHSV, rgb[0], rgb[1], rgb[2]);
    glUniform1f(_tintShader.TintAmount, sat_trs_lum[0]);
    glUniform1f(_tintShader.TintLuminance, sat_trs_lum[2]);
  }
  else if (do_light)
  {
    // Use light shader
    program = _lightShader;
    glUseProgram(_lightShader.Program);
    float light_lev = 1.0f;

    // Light level parameter in DDB is weird, it is measured in units of
    // 1/255 (although effectively 1/250, see draw.cpp), but contains two
    // ranges: 1-255 is darker range and 256-511 is brighter range.
    // (light level of 0 means "default color")
    if ((bmpToDraw->_lightLevel > 0) && (bmpToDraw->_lightLevel < 256))
    {
      // darkening the sprite... this stupid calculation is for
      // consistency with the allegro software-mode code that does
      // a trans blend with a (8,8,8) sprite
      light_lev = -((bmpToDraw->_lightLevel * 192) / 256 + 64) / 255.f; // darker, uses MODULATE op
    }
    else if (bmpToDraw->_lightLevel > 256)
    {
      light_lev = ((bmpToDraw->_lightLevel - 256) / 2) / 255.f; // brighter, uses ADD op
    }

    glUniform1f(_lightShader.LightingAmount, light_lev);
  }
  else
  {
    // Use default processing
    program = _transparencyShader;
    glUseProgram(_transparencyShader.Program);
  }

  glUniform1i(program.TextureId, 0);
  glUniform1f(program.Alpha, alpha / 255.0f);

  float width = bmpToDraw->GetWidthToRender();
  float height = bmpToDraw->GetHeightToRender();
  float xProportion = (float)width / (float)bmpToDraw->_width;
  float yProportion = (float)height / (float)bmpToDraw->_height;
  int drawAtX = drawListEntry->x;
  int drawAtY = drawListEntry->y;

  const auto *txdata = bmpToDraw->_data.get();
  for (size_t ti = 0; ti < txdata->_numTiles; ++ti)
  {
    width = txdata->_tiles[ti].width * xProportion;
    height = txdata->_tiles[ti].height * yProportion;
    float xOffs;
    float yOffs = txdata->_tiles[ti].y * yProportion;
    if (bmpToDraw->_flipped)
      xOffs = (bmpToDraw->_width - (txdata->_tiles[ti].x + txdata->_tiles[ti].width)) * xProportion;
    else
      xOffs = txdata->_tiles[ti].x * xProportion;
    int thisX = drawAtX + xOffs;
    int thisY = drawAtY + yOffs;
    thisX = (-(_srcRect.GetWidth() / 2)) + thisX;
    thisY = (_srcRect.GetHeight() / 2) - thisY;

    //Setup translation and scaling matrices
    float widthToScale = (float)width;
    float heightToScale = (float)height;
    if (bmpToDraw->_flipped)
    {
      // The usual transform changes 0..1 into 0..width
      // So first negate it (which changes 0..w into -w..0)
      widthToScale = -widthToScale;
      // and now shift it over to make it 0..w again
      thisX += width;
    }
    // Apply sprite origin
    thisX -= abs(widthToScale) * bmpToDraw->_originX;
    thisY += heightToScale * bmpToDraw->_originY; // inverse axis
    // Setup rotation and pivot
    float rotZ = bmpToDraw->_rotation;
    float pivotX = -(widthToScale * 0.5), pivotY = (heightToScale * 0.5);

    //
    // IMPORTANT: in OpenGL order of transformation is REVERSE to the order of commands!
    //
    glm::mat4 transform = projection;
    // Origin is at the middle of the surface
    if (_do_render_to_texture)
      transform = glm::translate(transform, {_backRenderSize.Width / 2.0f, _backRenderSize.Height / 2.0f, 0.0f});
    else
      transform = glm::translate(transform, {_srcRect.GetWidth() / 2.0f, _srcRect.GetHeight() / 2.0f, 0.0f});

    // Global batch transform
    transform = transform * matGlobal;
    // Self sprite transform (first scale, then rotate and then translate, reversed)
    transform = glm::translate(transform, {(float)thisX - pivotX, (float)thisY - pivotY, 0.0f});
    transform = glm::rotate(transform, rotZ, {0.f, 0.f, 1.f});
    transform = glm::translate(transform, { pivotX, pivotY, 0.0f });
    transform = glm::scale(transform, {widthToScale, heightToScale, 1.0f});

    glUniformMatrix4fv(program.MVPMatrix, 1, GL_FALSE, glm::value_ptr(transform));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, txdata->_tiles[ti].texture);

    if ((_smoothScaling) && bmpToDraw->_useResampler && (bmpToDraw->_stretchToHeight > 0) &&
        ((bmpToDraw->_stretchToHeight != bmpToDraw->_height) ||
         (bmpToDraw->_stretchToWidth != bmpToDraw->_width)))
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else if (_do_render_to_texture)
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    else
    {
      _filter->SetFilteringForStandardSprite();
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    if (txdata->_vertex != nullptr)
    {
        glEnableVertexAttribArray(0);
        GLint a_Position = glGetAttribLocation(program.Program, "a_Position");
        glVertexAttribPointer(a_Position, 2, GL_FLOAT, GL_FALSE, sizeof(OGLCUSTOMVERTEX), &(txdata->_vertex[ti * 4].position));

        glEnableVertexAttribArray(1);
        GLint a_TexCoord = glGetAttribLocation(program.Program, "a_TexCoord");
        glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(OGLCUSTOMVERTEX), &(txdata->_vertex[ti * 4].tu));
    }
    else
    {
        glEnableVertexAttribArray(0);
        GLint a_Position = glGetAttribLocation(program.Program, "a_Position");
        glVertexAttribPointer(a_Position, 2, GL_FLOAT, GL_FALSE, sizeof(OGLCUSTOMVERTEX), &(defaultVertices[0].position));

        glEnableVertexAttribArray(1);
        GLint a_TexCoord = glGetAttribLocation(program.Program, "a_TexCoord");
        glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(OGLCUSTOMVERTEX), &(defaultVertices[0].tu));
    }

    // Blend modes
    switch (bmpToDraw->_blendMode) {
        // blend mode is always NORMAL at this point
        //case kBlend_Alpha: AGS_OGLBLENDOP(GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break; // ALPHA
        case kBlend_Add: AGS_OGLBLENDOP(GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE); break; // ADD (transparency = strength)
#ifdef GL_MIN
        case kBlend_Darken: AGS_OGLBLENDOP(GL_MIN, GL_ONE, GL_ONE); break; // DARKEN
#endif
#ifdef GL_MAX
        case kBlend_Lighten: AGS_OGLBLENDOP(GL_MAX, GL_ONE, GL_ONE); break; // LIGHTEN
#endif
        case kBlend_Multiply: AGS_OGLBLENDOP(GL_FUNC_ADD, GL_ZERO, GL_SRC_COLOR); break; // MULTIPLY
        case kBlend_Screen: AGS_OGLBLENDOP(GL_FUNC_ADD, GL_ONE, GL_ONE_MINUS_SRC_COLOR); break; // SCREEN
        case kBlend_Subtract: AGS_OGLBLENDOP(GL_FUNC_REVERSE_SUBTRACT, GL_SRC_ALPHA, GL_ONE); break; // SUBTRACT (transparency = strength)
        case kBlend_Exclusion: AGS_OGLBLENDOP(GL_FUNC_ADD, GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR); break; // EXCLUSION
        // APPROXIMATIONS (need pixel shaders)
        case kBlend_Burn: AGS_OGLBLENDOP(GL_FUNC_SUBTRACT, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR); break; // LINEAR BURN (approximation)
        case kBlend_Dodge: AGS_OGLBLENDOP(GL_FUNC_ADD, GL_DST_COLOR, GL_ONE); break; // fake color dodge (half strength of the real thing)
    }

    // WORKAROUNDS - BEGIN

    // allow transparency with blending modes
    // darken/lighten the base sprite so a higher transparency value makes it trasparent
    if (bmpToDraw->_blendMode > 0) {
        const float alpha = bmpToDraw->_alpha / 255.0;
        const float invalpha = 1.0 - alpha;
        switch (bmpToDraw->_blendMode) {
        case kBlend_Darken:
        case kBlend_Multiply:
        case kBlend_Burn: // burn is imperfect due to blend mode, darker than normal even when trasparent
            // fade to white
#if !AGS_OPENGL_ES2 // glTexEnvi and glColor4f are not available on OpenGL ES2, need to rewrite the code here
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
            glColor4f(invalpha, invalpha, invalpha, invalpha);
#endif // !AGS_OPENGL_ES2
            break;
        default:
#if !AGS_OPENGL_ES2
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glColor4f(alpha, alpha, alpha, alpha);
#endif // !AGS_OPENGL_ES2
            break;
        }
    }

    // workaround: since the dodge is only half strength we can get a closer approx by drawing it twice
    if (bmpToDraw->_blendMode == kBlend_Dodge)
    {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    // BLENDMODES WORKAROUNDS - END


    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Restore default blending mode
    AGS_OGLBLENDOP(GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  glUseProgram(0);
}

void OGLGraphicsDriver::_render(bool clearDrawListAfterwards)
{
#if 0
  // TODO:
  // For some reason, mobile ports initialize actual display size after a short delay.
  // This is why we update display mode and related parameters (projection, viewport)
  // at the first render pass.
  // Ofcourse this is not a good thing, ideally the display size should be made
  // known before graphic mode is initialized. This would require analysis and rewrite
  // of the platform-specific part of the code (Java app for Android / XCode for iOS).
  if (!device_screen_initialized)
  {
    UpdateDeviceScreen();
    device_screen_initialized = 1;
  }
#endif
  glm::mat4 projection;

  if (_do_render_to_texture)
  {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, _backRenderSize.Width, _backRenderSize.Height);

    projection = glm::ortho(0.0f, (float)_backRenderSize.Width, 0.0f, (float)_backRenderSize.Height, 0.0f, 1.0f);
  }
  else
  {
    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);

    glViewport(_viewportRect.Left, _viewportRect.Top, _viewportRect.GetWidth(), _viewportRect.GetHeight());

    projection = glm::ortho(0.0f, (float)_srcRect.GetWidth(), 0.0f, (float)_srcRect.GetHeight(), 0.0f, 1.0f);
  }
  // Save Projection
  _stageMatrixes.Projection = projection;

  RenderSpriteBatches(projection);

  if (_do_render_to_texture)
  {
    glDisable(GL_BLEND);

    // Use default processing
    ShaderProgram program = _transparencyShader;
    glUseProgram(_transparencyShader.Program);

    glUniform1i(program.TextureId, 0);
    glUniform1f(program.Alpha, 1.0f);

    // Texture is ready, now create rectangle in the world space and draw texture upon it
#if AGS_PLATFORM_OS_IOS
    ios_select_buffer();
#else
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif

    glViewport(_viewportRect.Left, _viewportRect.Top, _viewportRect.GetWidth(), _viewportRect.GetHeight());

    projection = glm::ortho(0.0f, (float)_srcRect.GetWidth(), 0.0f, (float)_srcRect.GetHeight(), 0.0f, 1.0f);

    glUniformMatrix4fv(program.MVPMatrix, 1, GL_FALSE, glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _backbuffer);

    // use correct sampling method when stretching buffer to the final rect
    _filter->SetFilteringForStandardSprite();

    glEnableVertexAttribArray(0);
    GLint a_Position = glGetAttribLocation(program.Program, "a_Position");
    glVertexAttribPointer(a_Position, 2, GL_FLOAT, GL_FALSE, 0, _backbuffer_vertices);

    glEnableVertexAttribArray(1);
    GLint a_TexCoord = glGetAttribLocation(program.Program, "a_TexCoord");
    glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT, GL_FALSE, 0, _backbuffer_texture_coordinates);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEnable(GL_BLEND);
    glUseProgram(0);
  }

  glFinish();

  SDL_GL_SwapWindow(_sdlWindow);

  if (clearDrawListAfterwards)
  {
    BackupDrawLists();
    ClearDrawLists();
  }
  ResetFxPool();
}

void OGLGraphicsDriver::SetScissor(const Rect &clip)
{
    if (!clip.IsEmpty())
    {
        Rect scissor = _do_render_to_texture ? clip : _scaling.ScaleRange(clip);
        int surface_height = _do_render_to_texture ? _srcRect.GetHeight() : device_screen_physical_height;
        scissor = ConvertTopDownRect(scissor, surface_height);
        glScissor(scissor.Left, scissor.Top, scissor.GetWidth(), scissor.GetHeight());
    }
    else
    {
        Rect main_viewport = _do_render_to_texture ? _srcRect : _viewportRect;
        glScissor(main_viewport.Left, main_viewport.Top, main_viewport.GetWidth(), main_viewport.GetHeight());
    }
}

void OGLGraphicsDriver::RenderSpriteBatches(const glm::mat4 &projection)
{
    // Close unended batches, and issue a warning
    assert(_actSpriteBatch == 0);
    while (_actSpriteBatch > 0)
        EndSpriteBatch();

    // Render all the sprite batches with necessary transformations
    // TODO: see if it's possible to refactor and not enable/disable scissor test
    // TODO: also maybe sync scissor code logic with D3D renderer
    if (_do_render_to_texture)
        glEnable(GL_SCISSOR_TEST);

    // Render all the sprite batches with necessary transformations
    for (size_t cur_spr = 0; cur_spr < _spriteList.size();)
    {
        const OGLSpriteBatch &batch = _spriteBatches[_spriteList[cur_spr].node];
        SetScissor(batch.Viewport);
        _stageScreen = GetStageScreen(batch.ID);
        _stageMatrixes.World = batch.Matrix;
        cur_spr = RenderSpriteBatch(batch, cur_spr, projection);
    }

    _stageScreen = GetStageScreen(0);
    _stageMatrixes.World = _spriteBatches[0].Matrix;
    SetScissor(Rect());
    if (_do_render_to_texture)
        glDisable(GL_SCISSOR_TEST);
}

size_t OGLGraphicsDriver::RenderSpriteBatch(const OGLSpriteBatch &batch, size_t from, const glm::mat4 &projection)
{
    for (; (from < _spriteList.size()) && (_spriteList[from].node == batch.ID); ++from)
    {
        const auto &e = _spriteList[from];
        if (e.skip)
            continue;

        switch (reinterpret_cast<intptr_t>(e.ddb))
        {
        case DRAWENTRY_STAGECALLBACK:
            // raw-draw plugin support
            if (DoNullSpriteCallback(e.x, e.y))
            {
                auto stageEntry = OGLDrawListEntry((OGLBitmap*)_stageScreen.DDB, batch.ID, 0, 0);
                _renderSprite(&stageEntry, projection, batch.Matrix, batch.Color);
            }
            break;
        default:
            _renderSprite(&e, projection, batch.Matrix, batch.Color);
            break;
        }
    }
    return from;
}

void OGLGraphicsDriver::InitSpriteBatch(size_t index, const SpriteBatchDesc &desc)
{
    Rect orig_viewport = desc.Viewport;
    Rect node_viewport = desc.Viewport;

    // Combine both world transform and viewport transform into one matrix for faster perfomance
    // NOTE: in OpenGL order of transformation is REVERSE to the order of commands!
    glm::mat4 model = glm::mat4(1.0f);  // glLoadIdentity

    // Global node transformation (flip and offset)
    int node_tx = desc.Offset.X, node_ty = desc.Offset.Y;
    float node_sx = 1.f, node_sy = 1.f;
    if ((desc.Flip == kFlip_Vertical) || (desc.Flip == kFlip_Both))
    {
        int left = _srcRect.GetWidth() - (orig_viewport.Right + 1);
        node_viewport.MoveToX(left);
        node_sx = -1.f;
    }
    if ((desc.Flip == kFlip_Horizontal) || (desc.Flip == kFlip_Both))
    {
        int top = _srcRect.GetHeight() - (orig_viewport.Bottom + 1);
        node_viewport.MoveToY(top);
        node_sy = -1.f;
    }
    Rect viewport = Rect::MoveBy(node_viewport, node_tx, node_ty);
    model = glm::translate(model, {(float)node_tx, (float)-(node_ty), 0.0f});
    model = glm::scale(model, {node_sx, node_sy, 1.f});

    // NOTE: before node, translate to viewport position; remove this if this
    // is changed to a separate operation at some point
    // TODO: find out if this is an optimal way to translate scaled room into Top-Left screen coordinates
    float scaled_offx = (_srcRect.GetWidth() - desc.Transform.ScaleX * (float)_srcRect.GetWidth()) / 2.f;
    float scaled_offy = (_srcRect.GetHeight() - desc.Transform.ScaleY * (float)_srcRect.GetHeight()) / 2.f;
    model = glm::translate(model, {(float)(orig_viewport.Left - scaled_offx), (float)-(orig_viewport.Top - scaled_offy), 0.0f});

    // IMPORTANT: while the sprites are usually transformed in the order of Scale-Rotate-Translate,
    // the camera's transformation is essentially reverse world transformation. And the operations
    // are inverse: Translate-Rotate-Scale (here they are double inverse because OpenGL).
    float pivotx = _srcRect.GetWidth() / 2.f - desc.Transform.Pivot.X;
    float pivoty = _srcRect.GetHeight() / 2.f - desc.Transform.Pivot.Y;
    model = glm::scale(model, { desc.Transform.ScaleX, desc.Transform.ScaleY, 1.f });
    model = glm::translate(model, { -pivotx, -(-pivoty), 0.0f });
    model = glm::rotate(model, -Math::DegreesToRadians(desc.Transform.Rotate), { 0.f, 0.f, 1.f });
    model = glm::translate(model, { (float)desc.Transform.X + pivotx, (float)-desc.Transform.Y + (-pivoty), 0.0f });

    // Apply parent batch's settings, if preset
    if (desc.Parent > 0)
    {
        const auto &parent = _spriteBatches[desc.Parent];
        model = model * parent.Matrix;
        viewport = ClampToRect(parent.Viewport, viewport);
    }

    // Assign the new spritebatch
    if (_spriteBatches.size() <= index)
        _spriteBatches.resize(index + 1);
    _spriteBatches[index] = OGLSpriteBatch(index, viewport, model, desc.Transform.Color);

    // create stage screen for plugin raw drawing
    int src_w = orig_viewport.GetWidth() / desc.Transform.ScaleX;
    int src_h = orig_viewport.GetHeight() / desc.Transform.ScaleY;
    CreateStageScreen(index, Size(src_w, src_h));

    GLenum err;
    for (;;) {
      err = glGetError();
      if (err == GL_NO_ERROR) { break; }
      Debug::Printf(kDbgMsg_Error, "OGLGraphicsDriver::InitSpriteBatch glerror: %d", err);
    }
}

void OGLGraphicsDriver::ResetAllBatches()
{
    _spriteBatches.clear();
    _spriteList.clear();
}

void OGLGraphicsDriver::ClearDrawBackups()
{
    _backupBatchDescs.clear();
    _backupBatches.clear();
    _backupSpriteList.clear();
}

void OGLGraphicsDriver::BackupDrawLists()
{
    _backupBatchDescs = _spriteBatchDesc;
    _backupBatches = _spriteBatches;
    _backupSpriteList = _spriteList;
}

void OGLGraphicsDriver::RestoreDrawLists()
{
    _spriteBatchDesc = _backupBatchDescs;
    _spriteBatches = _backupBatches;
    _spriteList = _backupSpriteList;
    _actSpriteBatch = 0;
}

void OGLGraphicsDriver::DrawSprite(int ox, int oy, int /*ltx*/, int /*lty*/, IDriverDependantBitmap* ddb)
{
    _spriteList.push_back(OGLDrawListEntry((OGLBitmap*)ddb, _actSpriteBatch, ox, oy));
}

void OGLGraphicsDriver::DestroyDDBImpl(IDriverDependantBitmap* ddb)
{
    // Remove deleted DDB from backups
    for (auto &backup_spr : _backupSpriteList)
    {
        if (backup_spr.ddb == ddb)
            backup_spr.skip = true;
    }
    delete (OGLBitmap*)ddb;
}


void OGLGraphicsDriver::UpdateTextureRegion(OGLTextureTile *tile, Bitmap *bitmap, bool opaque, bool hasAlpha)
{
  int textureHeight = tile->height;
  int textureWidth = tile->width;

  // TODO: this seem to be tad overcomplicated, these conversions were made
  // when texture is just created. Check later if this operation here may be removed.
  AdjustSizeToNearestSupportedByCard(&textureWidth, &textureHeight);

  int tilex = 0, tiley = 0, tileWidth = tile->width, tileHeight = tile->height;
  if (textureWidth > tile->width)
  {
      int texxoff = std::min(textureWidth - tile->width - 1, 1);
      tilex = texxoff;
      tileWidth += 1 + texxoff;
  }
  if (textureHeight > tile->height)
  {
      int texyoff = std::min(textureHeight - tile->height - 1, 1);
      tiley = texyoff;
      tileHeight += 1 + texyoff;
  }

  const bool usingLinearFiltering = _filter->UseLinearFiltering();
  char *origPtr = new char[sizeof(int) * tileWidth * tileHeight];
  const int pitch = tileWidth * sizeof(int);
  char *memPtr = origPtr + pitch * tiley + tilex * sizeof(int);

  TextureTile fixedTile;
  fixedTile.x = tile->x;
  fixedTile.y = tile->y;
  fixedTile.width = std::min(tile->width, tileWidth);
  fixedTile.height = std::min(tile->height, tileHeight);
  if (opaque)
    BitmapToVideoMemOpaque(bitmap, hasAlpha, &fixedTile, memPtr, pitch);
  else
    BitmapToVideoMem(bitmap, hasAlpha, &fixedTile, memPtr, pitch, usingLinearFiltering);

  // Mimic the behaviour of GL_CLAMP_EDGE for the tile edges
  // NOTE: on some platforms GL_CLAMP_EDGE does not work with the version of OpenGL we're using.
  if (tile->width < tileWidth)
  {
    if (tilex > 0)
    {
      for (int y = 0; y < tileHeight; y++)
      {
        unsigned int* edge_left_col = (unsigned int*)(origPtr + y * pitch + (tilex - 1) * sizeof(int));
        unsigned int* bm_left_col = (unsigned int*)(origPtr + y * pitch + (tilex) * sizeof(int));
        *edge_left_col = *bm_left_col & 0x00FFFFFF;
      }
    }
    for (int y = 0; y < tileHeight; y++)
    {
      unsigned int* edge_right_col = (unsigned int*)(origPtr + y * pitch + (tilex + tile->width) * sizeof(int));
      unsigned int* bm_right_col = edge_right_col - 1;
      *edge_right_col = *bm_right_col & 0x00FFFFFF;
    }
  }
  if (tile->height < tileHeight)
  {
    if (tiley > 0)
    {
      unsigned int* edge_top_row = (unsigned int*)(origPtr + pitch * (tiley - 1));
      unsigned int* bm_top_row = (unsigned int*)(origPtr + pitch * (tiley));
      for (int x = 0; x < tileWidth; x++)
      {
        edge_top_row[x] = bm_top_row[x] & 0x00FFFFFF;
      }
    }
    unsigned int* edge_bottom_row = (unsigned int*)(origPtr + pitch * (tiley + tile->height));
    unsigned int* bm_bottom_row = (unsigned int*)(origPtr + pitch * (tiley + tile->height - 1));
    for (int x = 0; x < tileWidth; x++)
    {
      edge_bottom_row[x] = bm_bottom_row[x] & 0x00FFFFFF;
    }
  }

  glBindTexture(GL_TEXTURE_2D, tile->texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tileWidth, tileHeight, GL_RGBA, GL_UNSIGNED_BYTE, origPtr);

  delete []origPtr;
}

void OGLGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha)
{
  OGLBitmap *target = (OGLBitmap*)bitmapToUpdate;
  if (target->_width != bitmap->GetWidth() || target->_height != bitmap->GetHeight())
    throw Ali3DException("UpdateDDBFromBitmap: mismatched bitmap size");
  const int color_depth = bitmap->GetColorDepth();
  if (color_depth != target->_colDepth)
    throw Ali3DException("UpdateDDBFromBitmap: mismatched colour depths");

  target->_hasAlpha = hasAlpha;
  UpdateTextureData(target->_data.get(), bitmap, target->_opaque, hasAlpha);
}

void OGLGraphicsDriver::UpdateTextureData(TextureData *txdata, Bitmap *bitmap, bool opaque, bool hasAlpha)
{
  const int color_depth = bitmap->GetColorDepth();
  if (color_depth == 8)
      select_palette(palette);

  auto *ogldata = reinterpret_cast<OGLTextureData*>(txdata);
  for (size_t i = 0; i < ogldata->_numTiles; ++i)
  {
    UpdateTextureRegion(&ogldata->_tiles[i], bitmap, opaque, hasAlpha);
  }

  if (color_depth == 8)
      unselect_palette();
}

int OGLGraphicsDriver::GetCompatibleBitmapFormat(int color_depth)
{
  if (color_depth == 8)
    return 8;
  if (color_depth > 8 && color_depth <= 16)
    return 16;
  return 32;
}

void OGLGraphicsDriver::AdjustSizeToNearestSupportedByCard(int *width, int *height)
{
  int allocatedWidth = *width, allocatedHeight = *height;

    bool foundWidth = false, foundHeight = false;
    int tryThis = 2;
    while ((!foundWidth) || (!foundHeight))
    {
      if ((tryThis >= allocatedWidth) && (!foundWidth))
      {
        allocatedWidth = tryThis;
        foundWidth = true;
      }

      if ((tryThis >= allocatedHeight) && (!foundHeight))
      {
        allocatedHeight = tryThis;
        foundHeight = true;
      }

      tryThis = tryThis << 1;
    }

  *width = allocatedWidth;
  *height = allocatedHeight;
}

IDriverDependantBitmap* OGLGraphicsDriver::CreateDDB(int width, int height, int color_depth, bool opaque)
{
  if (color_depth != GetCompatibleBitmapFormat(color_depth))
    throw Ali3DException("CreateDDB: bitmap colour depth not supported");
  OGLBitmap *ddb = new OGLBitmap(width, height, color_depth, opaque);
  ddb->_data.reset(reinterpret_cast<OGLTextureData*>(CreateTextureData(width, height, opaque)));
  return ddb;
}

IDriverDependantBitmap *OGLGraphicsDriver::CreateDDB(std::shared_ptr<TextureData> txdata,
    int width, int height, int color_depth, bool opaque)
{
    auto *ddb = reinterpret_cast<OGLBitmap*>(CreateDDB(width, height, color_depth, opaque));
    if (ddb)
        ddb->_data = std::static_pointer_cast<OGLTextureData>(txdata);
    return ddb;
}

std::shared_ptr<TextureData> OGLGraphicsDriver::GetTextureData(IDriverDependantBitmap *ddb)
{
    return std::static_pointer_cast<TextureData>((reinterpret_cast<OGLBitmap*>(ddb))->_data);
}

TextureData *OGLGraphicsDriver::CreateTextureData(int width, int height, bool /*opaque*/)
{
  assert(width > 0);
  assert(height > 0);
  int allocatedWidth = width;
  int allocatedHeight = height;
  AdjustSizeToNearestSupportedByCard(&allocatedWidth, &allocatedHeight);
  int tilesAcross = 1, tilesDown = 1;

  auto *txdata = new OGLTextureData();
  // Calculate how many textures will be necessary to
  // store this image
  int MaxTextureWidth = 512;
  int MaxTextureHeight = 512;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureWidth);
  MaxTextureHeight = MaxTextureWidth;

  tilesAcross = (allocatedWidth + MaxTextureWidth - 1) / MaxTextureWidth;
  tilesDown = (allocatedHeight + MaxTextureHeight - 1) / MaxTextureHeight;
  assert(tilesAcross > 0);
  assert(tilesDown > 0);
  tilesAcross = std::max(1, tilesAcross);
  tilesDown = std::max(1, tilesDown);
  int tileWidth = width / tilesAcross;
  int lastTileExtraWidth = width % tilesAcross;
  int tileHeight = height / tilesDown;
  int lastTileExtraHeight = height % tilesDown;
  int tileAllocatedWidth = tileWidth;
  int tileAllocatedHeight = tileHeight;

  AdjustSizeToNearestSupportedByCard(&tileAllocatedWidth, &tileAllocatedHeight);

  int numTiles = tilesAcross * tilesDown;
  OGLTextureTile *tiles = new OGLTextureTile[numTiles];

  OGLCUSTOMVERTEX *vertices = nullptr;

  if ((numTiles == 1) &&
      (allocatedWidth == width) &&
      (allocatedHeight == height))
  {
    // use default whole-image vertices
  }
  else
  {
     // The texture is not the same as the bitmap, so create some custom vertices
     // so that only the relevant portion of the texture is rendered
     txdata->_vertex = vertices = new OGLCUSTOMVERTEX[numTiles * 4];
  }

  for (int x = 0; x < tilesAcross; x++)
  {
    for (int y = 0; y < tilesDown; y++)
    {
      OGLTextureTile *thisTile = &tiles[y * tilesAcross + x];
      int thisAllocatedWidth = tileAllocatedWidth;
      int thisAllocatedHeight = tileAllocatedHeight;
      thisTile->x = x * tileWidth;
      thisTile->y = y * tileHeight;
      thisTile->width = tileWidth;
      thisTile->height = tileHeight;
      if (x == tilesAcross - 1)
      {
        thisTile->width += lastTileExtraWidth;
        thisAllocatedWidth = thisTile->width;
        AdjustSizeToNearestSupportedByCard(&thisAllocatedWidth, &thisAllocatedHeight);
      }
      if (y == tilesDown - 1)
      {
        thisTile->height += lastTileExtraHeight;
        thisAllocatedHeight = thisTile->height;
        AdjustSizeToNearestSupportedByCard(&thisAllocatedWidth, &thisAllocatedHeight);
      }

      if (vertices != nullptr)
      {
        const int texxoff = (thisAllocatedWidth - thisTile->width) > 1 ? 1 : 0;
        const int texyoff = (thisAllocatedHeight - thisTile->height) > 1 ? 1 : 0;
        for (int vidx = 0; vidx < 4; vidx++)
        {
          int i = (y * tilesAcross + x) * 4 + vidx;
          vertices[i] = defaultVertices[vidx];
          if (vertices[i].tu > 0.0)
          {
            vertices[i].tu = (float)(texxoff + thisTile->width) / (float)thisAllocatedWidth;
          }
          else
          {
            vertices[i].tu = (float)(texxoff) / (float)thisAllocatedWidth;
          }
          if (vertices[i].tv > 0.0)
          {
            vertices[i].tv = (float)(texyoff + thisTile->height) / (float)thisAllocatedHeight;
          }
          else
          {
            vertices[i].tv = (float)(texyoff) / (float)thisAllocatedHeight;
          }
        }
      }

      glGenTextures(1, &thisTile->texture);
      glBindTexture(GL_TEXTURE_2D, thisTile->texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      // NOTE: pay attention that the texture format depends on the **display mode**'s format,
      // rather than source bitmap's color depth!

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, thisAllocatedWidth, thisAllocatedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
  }

  txdata->_numTiles = numTiles;
  txdata->_tiles = tiles;
  return txdata;
}

void OGLGraphicsDriver::do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
  // Construct scene in order: game screen, fade fx, post game overlay
  // NOTE: please keep in mind: redrawing last saved frame here instead of constructing new one
  // is done because of backwards-compatibility issue: originally AGS faded out using frame
  // drawn before the script that triggers blocking fade (e.g. instigated by ChangeRoom).
  // Unfortunately some existing games were changing looks of the screen during same function,
  // but these were not supposed to get on screen until before fade-in.
  if (fadingOut)
     this->_reDrawLastFrame();
  else if (_drawScreenCallback != nullptr)
    _drawScreenCallback();
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear(makecol32(targetColourRed, targetColourGreen, targetColourBlue));
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, true);
  delete blackSquare;
  BeginSpriteBatch(_srcRect, SpriteTransform());
  d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
  this->DrawSprite(0, 0, d3db);
  EndSpriteBatch();
  if (_drawPostScreenCallback != NULL)
      _drawPostScreenCallback();

  if (speed <= 0) speed = 16;
  speed *= 2;  // harmonise speeds with software driver which is faster
  for (int a = 1; a < 255; a += speed)
  {
    d3db->SetAlpha(fadingOut ? a : (255 - a));
    this->_render(false);

    sys_evt_process_pending();
    if (_pollingCallback)
      _pollingCallback();
    WaitForNextFrame();
  }

  if (fadingOut)
  {
    d3db->SetAlpha(255);
    this->_render(false);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawLists();
  ResetFxPool();
}

void OGLGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
  do_fade(true, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void OGLGraphicsDriver::FadeIn(int speed, PALETTE /*p*/, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
  do_fade(false, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void OGLGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
{
  // Construct scene in order: game screen, fade fx, post game overlay
  if (blackingOut)
    this->_reDrawLastFrame();
  else if (_drawScreenCallback != nullptr)
    _drawScreenCallback();
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear();
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, true);
  delete blackSquare;
  BeginSpriteBatch(_srcRect, SpriteTransform());
  d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
  this->DrawSprite(0, 0, d3db);
  if (!blackingOut)
  {
    // when fading in, draw four black boxes, one
    // across each side of the screen
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
  }
  EndSpriteBatch();
  std::vector<OGLDrawListEntry> &drawList = _spriteList;
  const size_t last = drawList.size() - 1;

  if (_drawPostScreenCallback != NULL)
    _drawPostScreenCallback();

  int yspeed = _srcRect.GetHeight() / (_srcRect.GetWidth() / speed);
  int boxWidth = speed;
  int boxHeight = yspeed;

  while (boxWidth < _srcRect.GetWidth())
  {
    boxWidth += speed;
    boxHeight += yspeed;
    if (blackingOut)
    {
      drawList[last].x = _srcRect.GetWidth() / 2- boxWidth / 2;
      drawList[last].y = _srcRect.GetHeight() / 2 - boxHeight / 2;
      d3db->SetStretch(boxWidth, boxHeight, false);
    }
    else
    {
      drawList[last - 3].x = _srcRect.GetWidth() / 2 - boxWidth / 2 - _srcRect.GetWidth();
      drawList[last - 2].y = _srcRect.GetHeight() / 2 - boxHeight / 2 - _srcRect.GetHeight();
      drawList[last - 1].x = _srcRect.GetWidth() / 2 + boxWidth / 2;
      drawList[last    ].y = _srcRect.GetHeight() / 2 + boxHeight / 2;
      d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
    }

    this->_render(false);

    sys_evt_process_pending();
    if (_pollingCallback)
      _pollingCallback();
    platform->Delay(delay);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawLists();
  ResetFxPool();
}

void OGLGraphicsDriver::SetScreenFade(int red, int green, int blue)
{
    OGLBitmap *ddb = static_cast<OGLBitmap*>(MakeFx(red, green, blue));
    ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
        _spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
    ddb->SetAlpha(255);
    _spriteList.push_back(OGLDrawListEntry(ddb, _actSpriteBatch, 0, 0));
}

void OGLGraphicsDriver::SetScreenTint(int red, int green, int blue)
{
    if (red == 0 && green == 0 && blue == 0) return;
    OGLBitmap *ddb = static_cast<OGLBitmap*>(MakeFx(red, green, blue));
    ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
        _spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
    ddb->SetAlpha(128);
    _spriteList.push_back(OGLDrawListEntry(ddb, _actSpriteBatch, 0, 0));
}


OGLGraphicsFactory *OGLGraphicsFactory::_factory = nullptr;

OGLGraphicsFactory::~OGLGraphicsFactory()
{
    _factory = nullptr;
}

size_t OGLGraphicsFactory::GetFilterCount() const
{
    return 2;
}

const GfxFilterInfo *OGLGraphicsFactory::GetFilterInfo(size_t index) const
{
    switch (index)
    {
    case 0:
        return &OGLGfxFilter::FilterInfo;
    case 1:
        return &AAOGLGfxFilter::FilterInfo;
    default:
        return nullptr;
    }
}

String OGLGraphicsFactory::GetDefaultFilterID() const
{
    return OGLGfxFilter::FilterInfo.Id;
}

/* static */ OGLGraphicsFactory *OGLGraphicsFactory::GetFactory()
{
    if (!_factory)
        _factory = new OGLGraphicsFactory();
    return _factory;
}

OGLGraphicsDriver *OGLGraphicsFactory::EnsureDriverCreated()
{
    if (!_driver)
        _driver = new OGLGraphicsDriver();
    return _driver;
}

OGLGfxFilter *OGLGraphicsFactory::CreateFilter(const String &id)
{
    if (OGLGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new OGLGfxFilter();
    else if (AAOGLGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new AAOGLGfxFilter();
    return nullptr;
}

} // namespace OGL
} // namespace Engine
} // namespace AGS

#endif // AGS_OPENGL_DRIVER

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

#if defined(WINDOWS_VERSION) || defined(ANDROID_VERSION) || defined(IOS_VERSION)

#include <algorithm>
#include "gfx/ali3dexception.h"
#include "gfx/ali3dogl.h"
#include "gfx/gfxfilter_ogl.h"
#include "gfx/gfxfilter_aaogl.h"
#include "main/main_allegro.h"
#include "platform/base/agsplatformdriver.h"


#if defined(WINDOWS_VERSION)

const char* fbo_extension_string = "GL_EXT_framebuffer_object";
const char* vsync_extension_string = "WGL_EXT_swap_control";

// TODO: linking to glew32 library might be a better option
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = 0;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = 0;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = 0;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = 0;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = 0;
PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = 0;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = 0;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = 0;
PFNWGLSWAPINTERVALEXTPROC glSwapIntervalEXT = 0;
// Shaders stuff
PFNGLCREATESHADERPROC glCreateShader = 0;
PFNGLSHADERSOURCEPROC glShaderSource = 0;
PFNGLCOMPILESHADERPROC glCompileShader = 0;
PFNGLGETSHADERIVPROC glGetShaderiv = 0;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;
PFNGLATTACHSHADERPROC glAttachShader = 0;
PFNGLDETACHSHADERPROC glDetachShader = 0;
PFNGLDELETESHADERPROC glDeleteShader = 0;
PFNGLCREATEPROGRAMPROC glCreateProgram = 0;
PFNGLLINKPROGRAMPROC glLinkProgram = 0;
PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;
PFNGLUSEPROGRAMPROC glUseProgram = 0;
PFNGLDELETEPROGRAMPROC glDeleteProgram = 0;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;
PFNGLUNIFORM1IPROC glUniform1i = 0;
PFNGLUNIFORM1FPROC glUniform1f = 0;
PFNGLUNIFORM3FPROC glUniform3f = 0;


#elif defined(ANDROID_VERSION)

#define glOrtho glOrthof
#define GL_CLAMP GL_CLAMP_TO_EDGE

// Defined in Allegro
extern "C" 
{
  void android_swap_buffers();
  void android_create_screen(int width, int height, int color_depth);
  void android_mouse_setup(int left, int right, int top, int bottom, float scaling_x, float scaling_y);
}

extern "C" void android_debug_printf(char* format, ...);

extern unsigned int android_screen_physical_width;
extern unsigned int android_screen_physical_height;
extern int android_screen_initialized;

#define device_screen_initialized android_screen_initialized
#define device_mouse_setup android_mouse_setup
#define device_swap_buffers android_swap_buffers

const char* fbo_extension_string = "GL_OES_framebuffer_object";

#define glGenFramebuffersEXT glGenFramebuffersOES
#define glDeleteFramebuffersEXT glDeleteFramebuffersOES
#define glBindFramebufferEXT glBindFramebufferOES
#define glCheckFramebufferStatusEXT glCheckFramebufferStatusOES
#define glGetFramebufferAttachmentParameterivEXT glGetFramebufferAttachmentParameterivOES
#define glGenerateMipmapEXT glGenerateMipmapOES
#define glFramebufferTexture2DEXT glFramebufferTexture2DOES
#define glFramebufferRenderbufferEXT glFramebufferRenderbufferOES
// TODO: probably should use EGL and function eglSwapInterval on Android to support setting swap interval
// For now this is a dummy function pointer which is only used to test that function is not supported
const void (*glSwapIntervalEXT)(int) = NULL;

#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0_OES

#elif defined(IOS_VERSION)

extern "C" 
{
  void ios_swap_buffers();
  void ios_select_buffer();
  void ios_create_screen();
  void ios_mouse_setup(int left, int right, int top, int bottom, float scaling_x, float scaling_y);  
}

#define glOrtho glOrthof
#define GL_CLAMP GL_CLAMP_TO_EDGE

extern unsigned int ios_screen_physical_width;
extern unsigned int ios_screen_physical_height;
extern int ios_screen_initialized;

#define device_screen_initialized ios_screen_initialized
#define device_mouse_setup ios_mouse_setup
#define device_swap_buffers ios_swap_buffers

const char* fbo_extension_string = "GL_OES_framebuffer_object";

#define glGenFramebuffersEXT glGenFramebuffersOES
#define glDeleteFramebuffersEXT glDeleteFramebuffersOES
#define glBindFramebufferEXT glBindFramebufferOES
#define glCheckFramebufferStatusEXT glCheckFramebufferStatusOES
#define glGetFramebufferAttachmentParameterivEXT glGetFramebufferAttachmentParameterivOES
#define glGenerateMipmapEXT glGenerateMipmapOES
#define glFramebufferTexture2DEXT glFramebufferTexture2DOES
#define glFramebufferRenderbufferEXT glFramebufferRenderbufferOES
// TODO: find out how to support swap interval setting on iOS
// For now this is a dummy function pointer which is only used to test that function is not supported
const void (*glSwapIntervalEXT)(int) = NULL;

#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0_OES

#endif

namespace AGS
{
namespace Engine
{
namespace OGL
{

using namespace AGS::Common;

void ogl_dummy_vsync() { }

#define algetr32(xx) ((xx >> _rgb_r_shift_32) & 0xFF)
#define algetg32(xx) ((xx >> _rgb_g_shift_32) & 0xFF)
#define algetb32(xx) ((xx >> _rgb_b_shift_32) & 0xFF)
#define algeta32(xx) ((xx >> _rgb_a_shift_32) & 0xFF)
#define algetr15(xx) ((xx >> _rgb_r_shift_15) & 0x1F)
#define algetg15(xx) ((xx >> _rgb_g_shift_15) & 0x1F)
#define algetb15(xx) ((xx >> _rgb_b_shift_15) & 0x1F)

#define GFX_OPENGL  AL_ID('O','G','L',' ')

GFX_DRIVER gfx_opengl =
{
   GFX_OPENGL,
   empty_string,
   empty_string,
   "OpenGL",
   NULL,    // init
   NULL,   // exit
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   ogl_dummy_vsync,   // vsync
   NULL,  // setpalette
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,  //create_video_bitmap
   NULL,  //destroy_video_bitmap
   NULL,   //show_video_bitmap
   NULL,
   NULL,  //gfx_directx_create_system_bitmap,
   NULL, //gfx_directx_destroy_system_bitmap,
   NULL, //gfx_directx_set_mouse_sprite,
   NULL, //gfx_directx_show_mouse,
   NULL, //gfx_directx_hide_mouse,
   NULL, //gfx_directx_move_mouse,
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                        // int w, h;
   FALSE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   TRUE                         // int windowed;
};

void OGLBitmap::Dispose()
{
    if (_tiles != NULL)
    {
        for (int i = 0; i < _numTiles; i++)
            glDeleteTextures(1, &(_tiles[i].texture));

        free(_tiles);
        _tiles = NULL;
        _numTiles = 0;
    }
    if (_vertex != NULL)
    {
        free(_vertex);
        _vertex = NULL;
    }
}


OGLGraphicsDriver::ShaderProgram::ShaderProgram() : Program(0), SamplerVar(0), ColorVar(0), AuxVar(0) {}


OGLGraphicsDriver::OGLGraphicsDriver() 
{
#if defined (WINDOWS_VERSION)
  _hDC = NULL;
  _hRC = NULL;
  _hWnd = NULL;
  _hInstance = NULL;
  device_screen_physical_width  = 0;
  device_screen_physical_height = 0;
#elif defined (ANDROID_VERSION)
  device_screen_physical_width  = android_screen_physical_width;
  device_screen_physical_height = android_screen_physical_height;
#elif defined (IOS_VERSION)
  device_screen_physical_width  = ios_screen_physical_width;
  device_screen_physical_height = ios_screen_physical_height;
#endif

  _firstTimeInit = false;
  _backbuffer = 0;
  _fbo = 0;
  _tint_red = 0;
  _tint_green = 0;
  _tint_blue = 0;
  _screenTintLayer = NULL;
  _screenTintLayerDDB = NULL;
  _screenTintSprite.skip = true;
  _screenTintSprite.x = 0;
  _screenTintSprite.y = 0;
  _legacyPixelShader = false;
  flipTypeLastTime = kFlip_None;
  _can_render_to_texture = false;
  _do_render_to_texture = false;
  _super_sampling = 1;
  SetupDefaultVertices();
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

#if defined (WINDOWS_VERSION)

void OGLGraphicsDriver::CreateDesktopScreen(int width, int height, int depth)
{
  device_screen_physical_width = width;
  device_screen_physical_height = height;
}

#elif defined (ANDROID_VERSION) || defined (IOS_VERSION)

void OGLGraphicsDriver::UpdateDeviceScreen()
{
#if defined (ANDROID_VERSION)
    device_screen_physical_width  = android_screen_physical_width;
    device_screen_physical_height = android_screen_physical_height;
#elif defined (IOS_VERSION)
    device_screen_physical_width  = ios_screen_physical_width;
    device_screen_physical_height = ios_screen_physical_height;
#endif

    Debug::Printf("OGL: notified of device screen updated to %d x %d, resizing viewport", device_screen_physical_width, device_screen_physical_height);
    _mode.Width = device_screen_physical_width;
    _mode.Height = device_screen_physical_height;
    InitGlParams(_mode);
    if (_initSurfaceUpdateCallback)
        _initSurfaceUpdateCallback();
}

#endif

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
  if (mode.Width <= 0 || mode.Height <= 0 || mode.ColorDepth <= 0)
  {
    set_allegro_error("Invalid resolution parameters: %d x %d x %d", mode.Width, mode.Height, mode.ColorDepth);
    return false;
  }
  return true;
}

bool OGLGraphicsDriver::SupportsGammaControl() 
{
  return false;
}

void OGLGraphicsDriver::SetGamma(int newGamma)
{
}

void OGLGraphicsDriver::SetGraphicsFilter(POGLFilter filter)
{
  _filter = filter;
  OnSetFilter();
  // Creating ddbs references filter properties at some point,
  // so we have to redo this part of initialization here.
  create_screen_tint_bitmap();
}

void OGLGraphicsDriver::SetTintMethod(TintMethod method) 
{
  _legacyPixelShader = (method == TintReColourise);
}

void OGLGraphicsDriver::FirstTimeInit()
{
  // It was told that GL_VERSION is supposed to begin with digits and may have
  // custom string after, but mobile version of OpenGL seem to have different
  // version string format.
  String ogl_v_str = (const char*)glGetString(GL_VERSION);
  String digits = "1234567890";
  const char *digits_ptr = std::find_first_of(ogl_v_str.GetCStr(), ogl_v_str.GetCStr() + ogl_v_str.GetLength(),
      digits.GetCStr(), digits.GetCStr() + digits.GetLength());
  if (digits_ptr < ogl_v_str.GetCStr() + ogl_v_str.GetLength())
    _oglVersion.SetFromString(digits_ptr);
  Debug::Printf(kDbgMsg_Init, "Running OpenGL: %s", ogl_v_str.GetCStr());

  TestVSync();
  TestRenderToTexture();
  CreateShaders();
  _firstTimeInit = true;
}

bool OGLGraphicsDriver::InitGlScreen(const DisplayMode &mode)
{
#if defined(ANDROID_VERSION)
  android_create_screen(mode.Width, mode.Height, mode.ColorDepth);
#elif defined(IOS_VERSION)
  ios_create_screen();
  ios_select_buffer();
#elif defined (WINDOWS_VERSION)
  if (!mode.Windowed)
  {
    if (platform->EnterFullscreenMode(mode))
      platform->AdjustWindowStyleForFullscreen();
  }
  // NOTE: adjust_window may leave task bar visible, so we do not use it for fullscreen mode
  if (mode.Windowed && adjust_window(mode.Width, mode.Height) != 0)
  {
    set_allegro_error("Window size not supported");
    return false;
  }

  _hWnd = win_get_window();
  if (!(_hDC = GetDC(_hWnd)))
    return false;

  // First check if we need to recreate GL context, this will only be
  // required if different color depth is requested.
  if (_hRC)
  {
    GLuint pixel_fmt = GetPixelFormat(_hDC);
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(_hDC, pixel_fmt, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    if (pfd.cColorBits != mode.ColorDepth)
    {
      DeleteGlContext();
    }
  }

  if (!_hRC)
  {
    if (!CreateGlContext(mode))
      return false;
  }

  CreateDesktopScreen(mode.Width, mode.Height, mode.ColorDepth);
  win_grab_input();
#endif

  gfx_driver = &gfx_opengl;
  return true;
}

void OGLGraphicsDriver::InitGlParams(const DisplayMode &mode)
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glShadeModel(GL_FLAT);

  glEnable(GL_TEXTURE_2D);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(0, 0, device_screen_physical_width, device_screen_physical_height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, device_screen_physical_width, 0, device_screen_physical_height, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  if (glSwapIntervalEXT)
  {
    if(mode.Vsync)
      glSwapIntervalEXT(1);
    else
      glSwapIntervalEXT(0);
  }

#if defined(ANDROID_VERSION) || defined(IOS_VERSION)
  // Setup library mouse to have 1:1 coordinate transformation.
  // NOTE: cannot move this call to general mouse handling mode. Unfortunately, much of the setup and rendering
  // is duplicated in the Android/iOS ports' Allegro library patches, and is run when the Software renderer
  // is selected in AGS. This ugly situation causes trouble...
  device_mouse_setup(0, device_screen_physical_width - 1, 0, device_screen_physical_height - 1, 1.0, 1.0);
#endif
}

bool OGLGraphicsDriver::CreateGlContext(const DisplayMode &mode)
{
#if defined (WINDOWS_VERSION)
  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    mode.ColorDepth,
    0, 0, 0, 0, 0, 0,
    0,
    0,
    0,
    0, 0, 0, 0,
    0,
    0,
    0,
    PFD_MAIN_PLANE,
    0,
    0, 0, 0
  };

  _oldPixelFormat = GetPixelFormat(_hDC);
  DescribePixelFormat(_hDC, _oldPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &_oldPixelFormatDesc);

  GLuint pixel_fmt;
  if (!(pixel_fmt = ChoosePixelFormat(_hDC, &pfd)))
    return false;

  if (!SetPixelFormat(_hDC, pixel_fmt, &pfd))
    return false;

  if (!(_hRC = wglCreateContext(_hDC)))
    return false;

  if(!wglMakeCurrent(_hDC, _hRC))
    return false;
#endif // WINDOWS_VERSION
  return true;
}

void OGLGraphicsDriver::DeleteGlContext()
{
#if defined (WINDOWS_VERSION)
  if (_hRC)
  {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(_hRC);
    _hRC = NULL;
  }

  if (_oldPixelFormat > 0)
    SetPixelFormat(_hDC, _oldPixelFormat, &_oldPixelFormatDesc);
#endif
}

void OGLGraphicsDriver::TestVSync()
{
// TODO: find out how to implement SwapInterval on other platforms, and how to check if it's supported
#if defined(WINDOWS_VERSION)
  const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
  const char* extensionsARB = NULL;
//#if defined(WINDOWS_VERSION)
  PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
  extensionsARB = wglGetExtensionsStringARB ? (const char*)wglGetExtensionsStringARB(_hDC) : NULL;
//#endif

  if (extensions && strstr(extensions, vsync_extension_string) != NULL ||
        extensionsARB && strstr(extensionsARB, vsync_extension_string) != NULL)
  {
//#if defined(WINDOWS_VERSION)
    glSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
//#endif
  }
  if (!glSwapIntervalEXT)
    Debug::Printf(kDbgMsg_Warn, "WARNING: OpenGL extension '%s' not supported, vertical sync will be kept at driver default.", vsync_extension_string);
#endif
}

void OGLGraphicsDriver::TestRenderToTexture()
{
  const char* extensions = (const char*)glGetString(GL_EXTENSIONS);

  if (extensions && strstr(extensions, fbo_extension_string) != NULL)
  {
  #if defined(WINDOWS_VERSION)
    glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
    glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
    glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
    glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
    glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
    glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)wglGetProcAddress("glGenerateMipmapEXT");
    glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
    glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
  #endif

    _can_render_to_texture = true;
    TestSupersampling();
  }
  else
  {
    _can_render_to_texture = false;
    Debug::Printf(kDbgMsg_Warn, "WARNING: OpenGL extension '%s' not supported, rendering to texture mode will be disabled.", fbo_extension_string);
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

void OGLGraphicsDriver::CreateShaders()
{
  // We need at least OpenGL 3.0 to run our tinting shader
  if (_oglVersion.Major < 3)
    Debug::Printf(kDbgMsg_Warn, "WARNING: OpenGL 3.0 or higher is required for tinting shader.");

  // TODO: linking with GLEW library might be a better idea
  #if defined(WINDOWS_VERSION)
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
  #endif

  if (!glCreateShader)
  {
    Debug::Printf(kDbgMsg_Error, "ERROR: OpenGL shader functions could not be linked.");
    return;
  }

  CreateTintShader();
  CreateLightShader();
}

void OGLGraphicsDriver::CreateTintShader()
{
  const char *fragment_shader_src = 
    // NOTE: this shader emulates "historical" AGS software tinting; it is not
    // necessarily "proper" tinting in modern terms.
    // The RGB-HSV-RGB conversion found in the Internet (copyright unknown);
    // Color processing is replicated from Direct3D shader by Chris Jones
    // (Engine/resource/tintshaderLegacy.fx).
    "\
                                #version 130\n\
                                out vec4 gl_FragColor;\n\
                                uniform sampler2D textID;\n\
                                uniform vec3 tintHSV;\n\
                                uniform vec3 tintAmnTrsLum;\n\
                                \
                                vec3 rgb2hsv(vec3 c)\n\
                                {\n\
                                    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\n\
                                    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));\n\
                                    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));\n\
                                \
                                    float d = q.x - min(q.w, q.y);\n\
                                    float e = 1.0e-10;\n\
                                    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);\n\
                                }\n\
                                \
                                vec3 hsv2rgb(vec3 c)\n\
                                {\n\
                                    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n\
                                    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n\
                                    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\n\
                                }\n\
                                \
                                float getValue(vec3 color)\n\
                                {\n\
                                    float colorMax = max (color[0], color[1]);\n\
                                    colorMax = max (colorMax, color[2]);\n\
                                    return colorMax;\n\
                                }\n\
                                \
                                void main()\n\
                                {\n\
                                    vec2 coords = gl_TexCoord[0].xy;\n\
                                    vec4 src_col = texture2D(textID, coords);\n\
                                    float amount = tintAmnTrsLum[0];\n\
                                    float lum = getValue(src_col.xyz);\n\
                                    lum = max(lum - (1.0 - tintAmnTrsLum[2]), 0.0);\n\
                                    vec3 new_col = (hsv2rgb(vec3(tintHSV[0],tintHSV[1],lum)) * amount + src_col.xyz*(1-amount));\n\
                                    gl_FragColor = vec4(new_col, src_col.w * tintAmnTrsLum[1]);\n\
                                }\n\
    ";
  CreateShaderProgram(_tintShader, "Tinting", fragment_shader_src, "textID", "tintHSV", "tintAmnTrsLum");
}

void OGLGraphicsDriver::CreateLightShader()
{
  const char *fragment_shader_src = 
    // NOTE: due to how the lighting works in AGS, this is combined MODULATE / ADD shader.
    // if the light is < 0, then MODULATE operation is used, otherwise ADD is used.
    // NOTE: it's been said that using branching in shaders produces inefficient code.
    // If that will ever become a real problem, we can easily split this shader in two.
    "\
                                #version 130\n\
                                out vec4 gl_FragColor;\n\
                                uniform sampler2D textID;\n\
                                uniform float light;\n\
                                uniform float alpha;\n\
                                \
                                void main()\n\
                                {\n\
                                    vec2 coords = gl_TexCoord[0].xy;\n\
                                    gl_FragColor = texture2D(textID, coords);\n\
                                    if (light >= 0.0)\n\
                                        gl_FragColor = vec4(gl_FragColor.xyz + vec3(light, light, light), gl_FragColor.w * alpha);\n\
                                    else\n\
                                        gl_FragColor = vec4(gl_FragColor.xyz * abs(light), gl_FragColor.w * alpha);\n\
                                }\n\
    ";
  CreateShaderProgram(_lightShader, "Lighting", fragment_shader_src, "textID", "light", "alpha");
}

void OGLGraphicsDriver::CreateShaderProgram(ShaderProgram &prg, const char *name, const char *fragment_shader_src,
                                            const char *sampler_var, const char *color_var, const char *aux_var)
{
  GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
  glCompileShader(fragment_shader);
  GLint compile_result;
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_result);
  if (compile_result == GL_FALSE)
  {
    OutputShaderError(fragment_shader, String::FromFormat("%s program's fragment shader", name), true);
    glDeleteShader(fragment_shader);
    return;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  GLint link_result = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &link_result);
  if(link_result == GL_FALSE)
  {
    OutputShaderError(program, String::FromFormat("%s program", name), false);
    glDeleteProgram(program);
    glDeleteShader(fragment_shader);
    return;
  }
  glDetachShader(program, fragment_shader);
  glDeleteShader(fragment_shader);

  prg.Program = program;
  prg.SamplerVar = glGetUniformLocation(program, sampler_var);
  prg.ColorVar = glGetUniformLocation(program, color_var);
  prg.AuxVar = glGetUniformLocation(program, aux_var);
}

void OGLGraphicsDriver::DeleteShaderProgram(ShaderProgram &prg)
{
  if (prg.Program)
    glDeleteProgram(prg.Program);
  prg.Program = 0;
}

void OGLGraphicsDriver::OutputShaderError(GLuint obj_id, const String &obj_name, bool is_shader)
{
  GLint log_len = 0;
  if (is_shader)
    glGetShaderiv(obj_id, GL_INFO_LOG_LENGTH, &log_len);
  else
    glGetProgramiv(obj_id, GL_INFO_LOG_LENGTH, &log_len);
  std::vector<GLchar> errorLog(log_len);
  if (is_shader)
    glGetShaderInfoLog(obj_id, log_len, &log_len, &errorLog[0]);
  else
    glGetProgramInfoLog(obj_id, log_len, &log_len, &errorLog[0]);

  Debug::Printf(kDbgMsg_Error, "ERROR: OpenGL: %s %s:", obj_name.GetCStr(), is_shader ? "failed to compile" : "failed to link");
  Debug::Printf(kDbgMsg_Error, "----------------------------------------");
  Debug::Printf(kDbgMsg_Error, "%s", &errorLog[0]);
  Debug::Printf(kDbgMsg_Error, "----------------------------------------");
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
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _backTextureSize.Width, _backTextureSize.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
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

  // Setup viewport rect and scissor; notice that OpenGL viewport has Y axis inverted
  _viewportRect = RectWH(_dstRect.Left, device_screen_physical_height - 1 - _dstRect.Bottom, _dstRect.GetWidth(), _dstRect.GetHeight());
  glScissor(_viewportRect.Left, _viewportRect.Top, _viewportRect.GetWidth(), _viewportRect.GetHeight());
}

bool OGLGraphicsDriver::SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer)
{
  ReleaseDisplayMode();

  if (mode.ColorDepth < 15)
  {
    set_allegro_error("OpenGL driver does not support 256-color display mode");
    return false;
  }

  try
  {
    if (!InitGlScreen(mode))
      return false;
    if (!_firstTimeInit)
      FirstTimeInit();
    InitGlParams(mode);
  }
  catch (Ali3DException exception)
  {
    if (exception._message != get_allegro_error())
      set_allegro_error(exception._message);
    return false;
  }

  OnInit(loopTimer);

  // On certain platforms OpenGL renderer ignores requested screen sizes
  // and uses values imposed by the operating system (device).
  DisplayMode final_mode = mode;
  final_mode.Width = device_screen_physical_width;
  final_mode.Height = device_screen_physical_height;
  OnModeSet(final_mode);

  create_screen_tint_bitmap();
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
  CreateStageScreen();
}

bool OGLGraphicsDriver::SetNativeSize(const Size &src_size)
{
  OnSetNativeSize(src_size);
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

int OGLGraphicsDriver::GetDisplayDepthForNativeDepth(int native_color_depth) const
{
    // TODO: check for device caps to know which depth is supported?
    return 32;
}

IGfxModeList *OGLGraphicsDriver::GetSupportedModeList(int color_depth)
{
    std::vector<DisplayMode> modes;
    platform->GetSystemDisplayModes(modes);
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
  DeleteBackbufferTexture();

  if (_screenTintLayerDDB != NULL) 
  {
    this->DestroyDDB(_screenTintLayerDDB);
    _screenTintLayerDDB = NULL;
    _screenTintSprite.bitmap = NULL;
  }
  delete _screenTintLayer;
  _screenTintLayer = NULL;

  DestroyStageScreen();

  gfx_driver = NULL;

  if (platform->ExitFullscreenMode())
    platform->RestoreWindowStyle();
}

void OGLGraphicsDriver::UnInit() 
{
  OnUnInit();
  ReleaseDisplayMode();

  DeleteGlContext();
#if defined (WINDOWS_VERSION)
  _hWnd = NULL;
  _hDC = NULL;
#endif

  DeleteShaderProgram(_tintShader);
  DeleteShaderProgram(_lightShader);
}

OGLGraphicsDriver::~OGLGraphicsDriver()
{
  UnInit();
}

void OGLGraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse)
{

}

void OGLGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res)
{
  (void)at_native_res; // TODO: support this at some point
  Rect retr_rect;
  if (_do_render_to_texture)
  {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
    retr_rect = RectWH(0, 0, _backRenderSize.Width, _backRenderSize.Height);
  }
  else
  {
#if defined(IOS_VERSION)
    ios_select_buffer();
#elif defined(WINDOWS_VERSION)
    glReadBuffer(GL_FRONT);
#endif
    retr_rect = _dstRect;
  }

  int bpp = _mode.ColorDepth / 8;
  int bufferSize = retr_rect.GetWidth() * retr_rect.GetHeight() * bpp;

  unsigned char* buffer = new unsigned char[bufferSize];
  if (buffer)
  {
    glReadPixels(retr_rect.Left, retr_rect.Top, retr_rect.GetWidth(), retr_rect.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    unsigned char* surfaceData = buffer;
    unsigned char* sourcePtr;
    unsigned char* destPtr;
    
	Bitmap* retrieveInto = BitmapHelper::CreateBitmap(retr_rect.GetWidth(), retr_rect.GetHeight(), _mode.ColorDepth);

    if (retrieveInto)
    {
      for (int y = retrieveInto->GetHeight() - 1; y >= 0; y--)
      {
        sourcePtr = surfaceData;
        destPtr = &retrieveInto->GetScanLineForWriting(y)[0];
        for (int x = 0; x < retrieveInto->GetWidth() * bpp; x += bpp)
        {
          // TODO: find out if it's possible to retrieve pixels in the matching format
          destPtr[x]     = sourcePtr[x + 2];
          destPtr[x + 1] = sourcePtr[x + 1];
          destPtr[x + 2] = sourcePtr[x];
          destPtr[x + 3] = sourcePtr[x + 3];
        }
        surfaceData += retr_rect.GetWidth() * bpp;
      }

      destination->StretchBlt(retrieveInto, RectWH(0, 0, retrieveInto->GetWidth(), retrieveInto->GetHeight()),
		  RectWH(0, 0, destination->GetWidth(), destination->GetHeight()));
      delete retrieveInto;
    }

    delete [] buffer;
  }
}

void OGLGraphicsDriver::RenderToBackBuffer()
{
  throw Ali3DException("D3D driver does not have a back buffer");
}

void OGLGraphicsDriver::Render()
{
  Render(kFlip_None);
}

void OGLGraphicsDriver::Render(GlobalFlipType flip)
{
  _render(flip, true);
}

void OGLGraphicsDriver::_reDrawLastFrame()
{
  drawList = drawListLastTime;
}

void OGLGraphicsDriver::_renderSprite(OGLDrawListEntry *drawListEntry, bool globalLeftRightFlip, bool globalTopBottomFlip)
{
  OGLBitmap *bmpToDraw = drawListEntry->bitmap;

  if (bmpToDraw->_transparency >= 255)
    return;

  if (bmpToDraw->_tintSaturation > 0)
  {
    // Use tinting shader
    if (_tintShader.Program)
    {
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

      if (bmpToDraw->_transparency > 0)
        sat_trs_lum[1] = (float)bmpToDraw->_transparency / 255.0;
      else
        sat_trs_lum[1] = 1.0f;

      if (bmpToDraw->_lightLevel > 0)
        sat_trs_lum[2] = (float)bmpToDraw->_lightLevel / 255.0;
      else
        sat_trs_lum[2] = 1.0f;

      glUniform1i(_tintShader.SamplerVar, 0);
      glUniform3f(_tintShader.ColorVar, rgb[0], rgb[1], rgb[2]);
      glUniform3f(_tintShader.AuxVar, sat_trs_lum[0], sat_trs_lum[1], sat_trs_lum[2]);
    }
  }
  else if (bmpToDraw->_lightLevel > 0)
  {
    // Use light shader
      glUseProgram(_lightShader.Program);
    float light_lev = 1.0f;
    float alpha = 1.0f;

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

    if (bmpToDraw->_transparency > 0)
      alpha = bmpToDraw->_transparency / 255.f;

    glUniform1i(_lightShader.SamplerVar, 0);
    glUniform1f(_lightShader.ColorVar, light_lev);
    glUniform1f(_lightShader.AuxVar, alpha);
  }
  else
  {
    // Use default processing
    if (bmpToDraw->_transparency == 0)
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    else
      glColor4f(1.0f, 1.0f, 1.0f, bmpToDraw->_transparency / 255.0f);
  }

  float width = bmpToDraw->GetWidthToRender();
  float height = bmpToDraw->GetHeightToRender();
  float xProportion = (float)width / (float)bmpToDraw->_width;
  float yProportion = (float)height / (float)bmpToDraw->_height;

  bool flipLeftToRight = globalLeftRightFlip ^ bmpToDraw->_flipped;
  int drawAtX = drawListEntry->x + _global_x_offset;
  int drawAtY = drawListEntry->y + _global_y_offset;

  for (int ti = 0; ti < bmpToDraw->_numTiles; ti++)
  {
    width = bmpToDraw->_tiles[ti].width * xProportion;
    height = bmpToDraw->_tiles[ti].height * yProportion;
    float xOffs;
    float yOffs = bmpToDraw->_tiles[ti].y * yProportion;
    if (flipLeftToRight != globalLeftRightFlip)
    {
      xOffs = (bmpToDraw->_width - (bmpToDraw->_tiles[ti].x + bmpToDraw->_tiles[ti].width)) * xProportion;
    }
    else
    {
      xOffs = bmpToDraw->_tiles[ti].x * xProportion;
    }
    int thisX = drawAtX + xOffs;
    int thisY = drawAtY + yOffs;

    if (globalLeftRightFlip)
    {
      thisX = (_srcRect.GetWidth() - thisX) - width;
    }
    if (globalTopBottomFlip) 
    {
      thisY = (_srcRect.GetHeight() - thisY) - height;
    }

    thisX = (-(_srcRect.GetWidth() / 2)) + thisX;
    thisY = (_srcRect.GetHeight() / 2) - thisY;


    //Setup translation and scaling matrices
    float widthToScale = (float)width;
    float heightToScale = (float)height;
    if (flipLeftToRight)
    {
      // The usual transform changes 0..1 into 0..width
      // So first negate it (which changes 0..w into -w..0)
      widthToScale = -widthToScale;
      // and now shift it over to make it 0..w again
      thisX += width;
    }
    if (globalTopBottomFlip) 
    {
      heightToScale = -heightToScale;
      thisY -= height;
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (_do_render_to_texture)
      glTranslatef(_backRenderSize.Width / 2.0f, _backRenderSize.Height / 2.0f, 0.0f);
    else
      glTranslatef(_srcRect.GetWidth() / 2.0f, _srcRect.GetHeight() / 2.0f, 0.0f);

    glTranslatef((float)thisX, (float)thisY, 0.0f);
    glScalef(widthToScale, heightToScale, 1.0f);

    glBindTexture(GL_TEXTURE_2D, bmpToDraw->_tiles[ti].texture);

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

    if (bmpToDraw->_vertex != NULL)
    {
      glTexCoordPointer(2, GL_FLOAT, sizeof(OGLCUSTOMVERTEX), &(bmpToDraw->_vertex[ti * 4].tu));
      glVertexPointer(2, GL_FLOAT, sizeof(OGLCUSTOMVERTEX), &(bmpToDraw->_vertex[ti * 4].position));
    }
    else
    {
      glTexCoordPointer(2, GL_FLOAT, sizeof(OGLCUSTOMVERTEX), &defaultVertices[0].tu);
      glVertexPointer(2, GL_FLOAT, sizeof(OGLCUSTOMVERTEX), &defaultVertices[0].position);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  glUseProgram(0);
}

void OGLGraphicsDriver::_render(GlobalFlipType flip, bool clearDrawListAfterwards)
{
#if defined(IOS_VERSION)
  ios_select_buffer();
#endif

#if defined (ANDROID_VERSION) || defined (IOS_VERSION)
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

  std::vector<OGLDrawListEntry> &listToDraw = drawList;
  size_t listSize = drawList.size();

  bool globalLeftRightFlip = (flip == kFlip_Vertical) || (flip == kFlip_Both);
  bool globalTopBottomFlip = (flip == kFlip_Horizontal) || (flip == kFlip_Both);

  if (_do_render_to_texture)
  {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, _backRenderSize.Width, _backRenderSize.Height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, _backRenderSize.Width, 0, _backRenderSize.Height, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }
  else
  {
    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);

    glViewport(_viewportRect.Left, _viewportRect.Top, _viewportRect.GetWidth(), _viewportRect.GetHeight());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, _srcRect.GetWidth(), 0, _srcRect.GetHeight(), 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }

  for (size_t i = 0; i < listSize; i++)
  {
    if (listToDraw[i].skip)
      continue;

    if (listToDraw[i].bitmap == NULL)
    {
      if (DoNullSpriteCallback(listToDraw[i].x, listToDraw[i].y))
        listToDraw[i] = OGLDrawListEntry((OGLBitmap*)_stageVirtualScreenDDB);
      else
        continue;
    }

    this->_renderSprite(&listToDraw[i], globalLeftRightFlip, globalTopBottomFlip);
  }

  if (!_screenTintSprite.skip)
  {
    this->_renderSprite(&_screenTintSprite, false, false);
  }

  if (_do_render_to_texture)
  {
    // Texture is ready, now create rectangle in the world space and draw texture upon it
#if defined(IOS_VERSION)
    ios_select_buffer();
#else
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif

    glViewport(_viewportRect.Left, _viewportRect.Top, _viewportRect.GetWidth(), _viewportRect.GetHeight());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, _srcRect.GetWidth(), 0, _srcRect.GetHeight(), 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // use correct sampling method when stretching buffer to the final rect
    _filter->SetFilteringForStandardSprite();

    glBindTexture(GL_TEXTURE_2D, _backbuffer);

    glTexCoordPointer(2, GL_FLOAT, 0, _backbuffer_texture_coordinates);
    glVertexPointer(2, GL_FLOAT, 0, _backbuffer_vertices);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEnable(GL_BLEND);
  }

  glFinish();

#if defined(WINDOWS_VERSION)
  SwapBuffers(_hDC);
#elif defined(ANDROID_VERSION) || defined(IOS_VERSION)
  device_swap_buffers();
#endif

  if (clearDrawListAfterwards)
  {
    drawListLastTime = drawList;
    flipTypeLastTime = flip;
    ClearDrawList();
  }
}

void OGLGraphicsDriver::ClearDrawList()
{
  drawList.clear();
}

void OGLGraphicsDriver::DrawSprite(int x, int y, IDriverDependantBitmap* bitmap)
{
  drawList.push_back(OGLDrawListEntry((OGLBitmap*)bitmap, x, y));
}

void OGLGraphicsDriver::DestroyDDB(IDriverDependantBitmap* bitmap)
{
  for (size_t i = 0; i < drawListLastTime.size(); i++)
  {
    if (drawListLastTime[i].bitmap == bitmap)
    {
      drawListLastTime[i].skip = true;
    }
  }
  delete bitmap;
}

__inline void get_pixel_if_not_transparent15(unsigned short *pixel, unsigned short *red, unsigned short *green, unsigned short *blue, unsigned short *divisor)
{
  if (pixel[0] != MASK_COLOR_15)
  {
    *red += algetr15(pixel[0]);
    *green += algetg15(pixel[0]);
    *blue += algetb15(pixel[0]);
    divisor[0]++;
  }
}

__inline void get_pixel_if_not_transparent32(unsigned int *pixel, unsigned int *red, unsigned int *green, unsigned int *blue, unsigned int *divisor)
{
  if (pixel[0] != MASK_COLOR_32)
  {
    *red += algetr32(pixel[0]);
    *green += algetg32(pixel[0]);
    *blue += algetb32(pixel[0]);
    divisor[0]++;
  }
}

#define D3DCOLOR_RGBA(r,g,b,a) \
  (((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))


void OGLGraphicsDriver::UpdateTextureRegion(TextureTile *tile, Bitmap *bitmap, OGLBitmap *target, bool hasAlpha)
{
  int textureHeight = tile->height;
  int textureWidth = tile->width;

  AdjustSizeToNearestSupportedByCard(&textureWidth, &textureHeight);

  int tileWidth = (textureWidth > tile->width) ? tile->width + 1 : tile->width;
  int tileHeight = (textureHeight > tile->height) ? tile->height + 1 : tile->height;

  bool usingLinearFiltering = _filter->UseLinearFiltering();
  bool lastPixelWasTransparent = false;
  char *origPtr = (char*)malloc(4 * tileWidth * tileHeight);
  char *memPtr = origPtr;
  for (int y = 0; y < tileHeight; y++)
  {
    // Mimic the behaviour of GL_CLAMP_EDGE for the bottom line
    if (y == tile->height)
    {
      unsigned int* memPtrLong = (unsigned int*)memPtr;
      unsigned int* memPtrLong_previous = (unsigned int*)(memPtr - tileWidth * 4);

      for (int x = 0; x < tileWidth; x++)
        memPtrLong[x] = memPtrLong_previous[x] & 0x00FFFFFF;

      continue;
    }

    lastPixelWasTransparent = false;
    const uint8_t *scanline_before = bitmap->GetScanLine(y + tile->y - 1);
    const uint8_t *scanline_at     = bitmap->GetScanLine(y + tile->y);
    const uint8_t *scanline_after  = bitmap->GetScanLine(y + tile->y + 1);
    for (int x = 0; x < tileWidth; x++)
    {

/*    if (target->_colDepth == 15)
      {
        unsigned short* memPtrShort = (unsigned short*)memPtr;
        unsigned short* srcData = (unsigned short*)&bitmap->GetScanLine[y + tile->y][(x + tile->x) * 2];
        if (*srcData == MASK_COLOR_15) 
        {
          if (target->_opaque)  // set to black if opaque
            memPtrShort[x] = 0x8000;
          else if (!usingLinearFiltering)
            memPtrShort[x] = 0;
          // set to transparent, but use the colour from the neighbouring 
          // pixel to stop the linear filter doing black outlines
          else
          {
            unsigned short red = 0, green = 0, blue = 0, divisor = 0;
            if (x > 0)
              get_pixel_if_not_transparent15(&srcData[-1], &red, &green, &blue, &divisor);
            if (x < tile->width - 1)
              get_pixel_if_not_transparent15(&srcData[1], &red, &green, &blue, &divisor);
            if (y > 0)
              get_pixel_if_not_transparent15((unsigned short*)&bitmap->GetScanLine[y + tile->y - 1][(x + tile->x) * 2], &red, &green, &blue, &divisor);
            if (y < tile->height - 1)
              get_pixel_if_not_transparent15((unsigned short*)&bitmap->GetScanLine[y + tile->y + 1][(x + tile->x) * 2], &red, &green, &blue, &divisor);
            if (divisor > 0)
              memPtrShort[x] = ((red / divisor) << 10) | ((green / divisor) << 5) | (blue / divisor);
            else
              memPtrShort[x] = 0;
          }
          lastPixelWasTransparent = true;
        }
        else
        {
          memPtrShort[x] = 0x8000 | (algetr15(*srcData) << 10) | (algetg15(*srcData) << 5) | algetb15(*srcData);
          if (lastPixelWasTransparent)
          {
            // update the colour of the previous tranparent pixel, to
            // stop black outlines when linear filtering
            memPtrShort[x - 1] = memPtrShort[x] & 0x7FFF;
            lastPixelWasTransparent = false;
          }
        }
      }
      else if (target->_colDepth == 32)
*/
      {
        unsigned int* memPtrLong = (unsigned int*)memPtr;

        if (x == tile->width)
        {
          memPtrLong[x] = memPtrLong[x - 1] & 0x00FFFFFF;
          continue;
        }

        unsigned int* srcData = (unsigned int*)&scanline_at[(x + tile->x) << 2];
        if (*srcData == MASK_COLOR_32)
        {
          if (target->_opaque)  // set to black if opaque
            memPtrLong[x] = 0xFF000000;
          else if (!usingLinearFiltering)
            memPtrLong[x] = 0;
          // set to transparent, but use the colour from the neighbouring 
          // pixel to stop the linear filter doing black outlines
          else
          {
            unsigned int red = 0, green = 0, blue = 0, divisor = 0;
            if (x > 0)
              get_pixel_if_not_transparent32(&srcData[-1], &red, &green, &blue, &divisor);
            if (x < tile->width - 1)
              get_pixel_if_not_transparent32(&srcData[1], &red, &green, &blue, &divisor);
            if (y > 0)
              get_pixel_if_not_transparent32((unsigned int*)&scanline_before[(x + tile->x) << 2], &red, &green, &blue, &divisor);
            if (y < tile->height - 1)
              get_pixel_if_not_transparent32((unsigned int*)&scanline_after[(x + tile->x) << 2], &red, &green, &blue, &divisor);
            if (divisor > 0)
              memPtrLong[x] = ((red / divisor) << 16) | ((green / divisor) << 8) | (blue / divisor);
            else
              memPtrLong[x] = 0;
          }
          lastPixelWasTransparent = true;
        }
        else if (hasAlpha)
        {
          memPtrLong[x] = D3DCOLOR_RGBA(algetr32(*srcData), algetg32(*srcData), algetb32(*srcData), algeta32(*srcData));
        }
        else
        {
          memPtrLong[x] = D3DCOLOR_RGBA(algetr32(*srcData), algetg32(*srcData), algetb32(*srcData), 0xff);
          if (lastPixelWasTransparent)
          {
            // update the colour of the previous tranparent pixel, to
            // stop black outlines when linear filtering
            memPtrLong[x - 1] = memPtrLong[x] & 0x00FFFFFF;
            lastPixelWasTransparent = false;
          }
        }
      }
    }

    memPtr += tileWidth * 4;
  }

  glBindTexture(GL_TEXTURE_2D, tile->texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tileWidth, tileHeight, GL_RGBA, GL_UNSIGNED_BYTE, origPtr);

  free(origPtr);
}

void OGLGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha)
{
  OGLBitmap *target = (OGLBitmap*)bitmapToUpdate;
  if ((target->_width == bitmap->GetWidth()) &&
     (target->_height == bitmap->GetHeight()))
  {
    if (bitmap->GetColorDepth() != target->_colDepth)
    {
      throw Ali3DException("Mismatched colour depths");
    }

    target->_hasAlpha = hasAlpha;

    for (int i = 0; i < target->_numTiles; i++)
    {
      UpdateTextureRegion(&target->_tiles[i], bitmap, target, hasAlpha);
    }
  }
}

Bitmap *OGLGraphicsDriver::ConvertBitmapToSupportedColourDepth(Bitmap *bitmap)
{
   int colourDepth = bitmap->GetColorDepth();
   if (colourDepth != 32)
   {
     int old_conv = get_color_conversion();
     set_color_conversion(COLORCONV_KEEP_TRANS | COLORCONV_TOTAL);
     // we need 32-bit colour
     Bitmap *tempBmp = BitmapHelper::CreateBitmapCopy(bitmap, 32);
     set_color_conversion(old_conv);
     return tempBmp;
   }
   return bitmap;
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



IDriverDependantBitmap* OGLGraphicsDriver::CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque)
{
  int allocatedWidth = bitmap->GetWidth();
  int allocatedHeight = bitmap->GetHeight();
  // NOTE: original bitmap object is not modified in this function
  Bitmap *tempBmp = ConvertBitmapToSupportedColourDepth(bitmap);
  bitmap = tempBmp;
  int colourDepth = bitmap->GetColorDepth();

  OGLBitmap *ddb = new OGLBitmap(bitmap->GetWidth(), bitmap->GetHeight(), colourDepth, opaque);

  AdjustSizeToNearestSupportedByCard(&allocatedWidth, &allocatedHeight);
  int tilesAcross = 1, tilesDown = 1;

  // *************** REMOVE THESE LINES *************
  //direct3ddevicecaps.MaxTextureWidth = 64;
  //direct3ddevicecaps.MaxTextureHeight = 256;
  // *************** END REMOVE THESE LINES *************

  // Calculate how many textures will be necessary to
  // store this image

  int MaxTextureWidth = 512;
  int MaxTextureHeight = 512;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureWidth);
  MaxTextureHeight = MaxTextureWidth;

  tilesAcross = (allocatedWidth + MaxTextureWidth - 1) / MaxTextureWidth;
  tilesDown = (allocatedHeight + MaxTextureHeight - 1) / MaxTextureHeight;
  int tileWidth = bitmap->GetWidth() / tilesAcross;
  int lastTileExtraWidth = bitmap->GetWidth() % tilesAcross;
  int tileHeight = bitmap->GetHeight() / tilesDown;
  int lastTileExtraHeight = bitmap->GetHeight() % tilesDown;
  int tileAllocatedWidth = tileWidth;
  int tileAllocatedHeight = tileHeight;

  AdjustSizeToNearestSupportedByCard(&tileAllocatedWidth, &tileAllocatedHeight);

  int numTiles = tilesAcross * tilesDown;
  TextureTile *tiles = (TextureTile*)malloc(sizeof(TextureTile) * numTiles);
  memset(tiles, 0, sizeof(TextureTile) * numTiles);

  OGLCUSTOMVERTEX *vertices = NULL;

  if ((numTiles == 1) &&
      (allocatedWidth == bitmap->GetWidth()) &&
      (allocatedHeight == bitmap->GetHeight()))
  {
    // use default whole-image vertices
  }
  else
  {
     // The texture is not the same as the bitmap, so create some custom vertices
     // so that only the relevant portion of the texture is rendered
     int vertexBufferSize = numTiles * 4 * sizeof(OGLCUSTOMVERTEX);

     ddb->_vertex = vertices = (OGLCUSTOMVERTEX*)malloc(vertexBufferSize);
  }

  for (int x = 0; x < tilesAcross; x++)
  {
    for (int y = 0; y < tilesDown; y++)
    {
      TextureTile *thisTile = &tiles[y * tilesAcross + x];
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

      if (vertices != NULL)
      {
        for (int vidx = 0; vidx < 4; vidx++)
        {
          int i = (y * tilesAcross + x) * 4 + vidx;
          vertices[i] = defaultVertices[vidx];
          if (vertices[i].tu > 0.0)
          {
            vertices[i].tu = (float)thisTile->width / (float)thisAllocatedWidth;
          }
          if (vertices[i].tv > 0.0)
          {
            vertices[i].tv = (float)thisTile->height / (float)thisAllocatedHeight;
          }
        }
      }

      glGenTextures(1, &thisTile->texture);
      glBindTexture(GL_TEXTURE_2D, thisTile->texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, thisAllocatedWidth, thisAllocatedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
  }

  ddb->_numTiles = numTiles;
  ddb->_tiles = tiles;

  UpdateDDBFromBitmap(ddb, bitmap, hasAlpha);

  if (tempBmp != bitmap)
    delete tempBmp;

  return ddb;
}

void OGLGraphicsDriver::do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
  if (fadingOut)
  {
    this->_reDrawLastFrame();
  }
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear(makecol32(targetColourRed, targetColourGreen, targetColourBlue));
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, false);
  delete blackSquare;

  d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
  this->DrawSprite(-_global_x_offset, -_global_y_offset, d3db);

  if (speed <= 0) speed = 16;
  speed *= 2;  // harmonise speeds with software driver which is faster
  for (int a = 1; a < 255; a += speed)
  {
    int timerValue = *_loopTimer;
    d3db->SetTransparency(fadingOut ? a : (255 - a));
    this->_render(flipTypeLastTime, false);

    do
    {
      if (_pollingCallback)
        _pollingCallback();
      platform->YieldCPU();
    }
    while (timerValue == *_loopTimer);

  }

  if (fadingOut)
  {
    d3db->SetTransparency(0);
    this->_render(flipTypeLastTime, false);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawList();
}

void OGLGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(true, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void OGLGraphicsDriver::FadeIn(int speed, PALLETE p, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(false, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void OGLGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
{
  if (blackingOut)
  {
    this->_reDrawLastFrame();
  }
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear();
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, false);
  delete blackSquare;

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

  int yspeed = _srcRect.GetHeight() / (_srcRect.GetWidth() / speed);
  int boxWidth = speed;
  int boxHeight = yspeed;

  while (boxWidth < _srcRect.GetWidth())
  {
    boxWidth += speed;
    boxHeight += yspeed;
    size_t last = drawList.size() - 1;
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
    
    this->_render(flipTypeLastTime, false);

    if (_pollingCallback)
      _pollingCallback();
    platform->Delay(delay);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawList();
}

bool OGLGraphicsDriver::PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen)
{
 return true;
}

void OGLGraphicsDriver::create_screen_tint_bitmap() 
{
  // Some work on textures depends on current scaling filter, sadly
  // TODO: find out if there is a workaround for that
  if (!IsModeSet() || !_filter)
    return;
  
  _screenTintLayer = BitmapHelper::CreateBitmap(16, 16, _mode.ColorDepth);
  _screenTintLayer = ReplaceBitmapWithSupportedFormat(_screenTintLayer);
  _screenTintLayerDDB = (OGLBitmap*)this->CreateDDBFromBitmap(_screenTintLayer, false, false);
  _screenTintSprite.bitmap = _screenTintLayerDDB;
}

void OGLGraphicsDriver::SetScreenTint(int red, int green, int blue)
{ 
  if ((red != _tint_red) || (green != _tint_green) || (blue != _tint_blue))
  {
    _tint_red = red; 
    _tint_green = green; 
    _tint_blue = blue;

    _screenTintLayer->Clear(makecol_depth(_screenTintLayer->GetColorDepth(), red, green, blue));
    this->UpdateDDBFromBitmap(_screenTintLayerDDB, _screenTintLayer, false);
    _screenTintLayerDDB->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
    _screenTintLayerDDB->SetTransparency(128);

    _screenTintSprite.skip = ((red == 0) && (green == 0) && (blue == 0));
  }
}


OGLGraphicsFactory *OGLGraphicsFactory::_factory = NULL;

OGLGraphicsFactory::~OGLGraphicsFactory()
{
    _factory = NULL;
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
        return NULL;
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
    return NULL;
}

} // namespace OGL
} // namespace Engine
} // namespace AGS

#endif // only on Windows, Android and iOS

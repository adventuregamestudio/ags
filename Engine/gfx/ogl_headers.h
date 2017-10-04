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
// OpenGL includes and definitions for various platforms
//
//=============================================================================

#if defined(WINDOWS_VERSION)
#include <allegro.h>
#include <winalleg.h>
#include <allegro/platform/aintwin.h>
#include <GL/gl.h>

// Allegro and glext.h define these
#undef int32_t
#undef int64_t
#undef uint64_t

#include <GL/glext.h>

#elif defined(ANDROID_VERSION)

#include <GLES/gl.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES/glext.h>

#define HDC void*
#define HGLRC void*
#define HWND void*
#define HINSTANCE void*

#elif defined(IOS_VERSION)

#include <OpenGLES/ES1/gl.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <OpenGLES/ES1/glext.h>

#define HDC void*
#define HGLRC void*
#define HWND void*
#define HINSTANCE void*

#endif

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
// Stubs for the few SDL2 functions that are used in Bitmap's implementation.
// This assumes that the part of the code that uses these is never tested.
// 
//=============================================================================
#include <stdint.h>

typedef uint32_t Uint32;
typedef int64_t Sint64;
typedef int SDL_bool;
typedef int SDL_errorcode;
typedef int SDL_BlendMode;
typedef void SDL_RWops;
typedef void SDL_Surface;

void *SDL_malloc(size_t size) { return NULL; }
void *SDL_calloc(size_t nmemb, size_t size) { return NULL; }
void *SDL_realloc(void *mem, size_t size) { return NULL; }
void SDL_free(void *mem) { }
void *SDL_memset(void *dst, int c, size_t len) { return NULL; }
void *SDL_memcpy(void *dst, const void *src, size_t len) { return NULL; }
size_t SDL_strlen(const char *str) { return 0u; }
int SDL_SetError(const char *fmt, ...) { return 0; }
int SDL_Error(SDL_errorcode code) { return 0; }
SDL_bool SDL_HasSSE2(void) { return 0; }
SDL_RWops *SDL_AllocRW(void) { return NULL; }
void SDL_FreeRW(SDL_RWops * area) { }
SDL_RWops *SDL_RWFromFile(const char *file, const char *mode) { return NULL; }
Sint64 SDL_RWsize(SDL_RWops *context) { return 0; }
Sint64 SDL_RWseek(SDL_RWops *context, Sint64 offset, int whence) { return 0; }
Sint64 SDL_RWtell(SDL_RWops *context) { return 0; }
size_t SDL_RWread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum) { return 0u; }
size_t SDL_RWwrite(SDL_RWops *context, const void *ptr, size_t size, size_t num) { return 0u; }
int SDL_RWclose(SDL_RWops *context) { return 0; }
SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 format) { return NULL; }
int SDL_SetColorKey(SDL_Surface * surface, int flag, Uint32 key) { return 0; }
int SDL_SetSurfaceBlendMode(SDL_Surface * surface, SDL_BlendMode blendMode) { return 0; }
void SDL_FreeSurface(SDL_Surface * surface) { }

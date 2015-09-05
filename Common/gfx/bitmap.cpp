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

#include "gfx/bitmap.h"
#include "util/memory.h"

#ifndef NULL
#define NULL 0
#endif

extern "C"
{
	extern BITMAP *screen;	// in allegro
}

namespace AGS
{
namespace Common
{

// TODO! Get rid of this global ptr in the future (need to rewrite drawing logic in the engine)
// NOTE: Screen bitmap is created either by working graphics driver, or in the engine_prepare_screen()
Bitmap *gl_ScreenBmp;

// TODO: revise this construction later
namespace BitmapHelper
{

Bitmap *CreateBitmap(int width, int height, int color_depth)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->Create(width, height, color_depth))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

Bitmap *CreateTransparentBitmap(int width, int height, int color_depth)
{
    Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateTransparent(width, height, color_depth))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

Bitmap *CreateSubBitmap(Bitmap *src, const Rect &rc)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateSubBitmap(src, rc))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

Bitmap *CreateBitmapCopy(Bitmap *src, int color_depth)
{
    Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateCopy(src, color_depth))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

Bitmap *LoadFromFile(const char *filename)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->LoadFromFile(filename))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

void ReadPixelsFromMemory(Bitmap *dst, const uint8_t *src_buffer, const size_t src_pitch, const size_t src_px_offset)
{
    const size_t bpp = dst->GetBPP();
    const size_t src_px_pitch = src_pitch / bpp;
    if (src_px_offset >= src_px_pitch)
        return; // nothing to copy
    Memory::BlockCopy(dst->GetDataForWriting(), dst->GetLineLength(), 0, src_buffer, src_pitch, src_px_offset * bpp, dst->GetHeight());
}

// TODO: redo this ugly workaround
// Unfortunately some of the allegro functions remaining in code require "screen"
// allegro bitmap, therefore we must set that pointer to something every time we
// assign an Bitmap to screen.
Bitmap *GetScreenBitmap()
{
	return gl_ScreenBmp;
}

void SetScreenBitmap(Bitmap *bitmap)
{
    // We do not delete previous object here since we can't tell
    // where it came from and whether it still exists.
    // (Did I mention this is an ugly workaround? So...)
	gl_ScreenBmp = bitmap;

    // Only set allegro screen pointer if there's actual bitmap;
    // setting it to NULL does not have any sense and may (will?)
    // cause crashes.
    if (gl_ScreenBmp)
    {
	    screen = (BITMAP*)gl_ScreenBmp->GetAllegroBitmap();
    }
}

} // namespace BitmapHelper

} // namespace Common
} // namespace AGS

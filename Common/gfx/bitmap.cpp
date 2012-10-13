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
#include "gfx/allegrobitmap.h"

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
	return AllegroBitmap::CreateBitmap(width, height, color_depth);
}

Bitmap *CreateSubBitmap(Bitmap *src, const Rect &rc)
{
	return AllegroBitmap::CreateSubBitmap(src, rc);
}

Bitmap *CreateRawObjectOwner(void *bitmap_object)
{
	return AllegroBitmap::CreateFromRawAllegroBitmap(bitmap_object);
}

Bitmap *CreateRawObjectWrapper(void *bitmap_object)
{
	return AllegroBitmap::WrapRawAllegroBitmap(bitmap_object);
}

Bitmap *LoadFromFile(const char *filename)
{
	return AllegroBitmap::LoadFromFile(filename);
}

bool SaveToFile(Bitmap *bitmap, const char *filename, const void *palette)
{
	return AllegroBitmap::SaveToFile(bitmap, filename, palette);
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
	    screen = (BITMAP*)gl_ScreenBmp->GetBitmapObject();
    }
}

};

} // namespace Common
} // namespace AGS

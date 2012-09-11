
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
IBitmap *gl_ScreenBmp;

// TODO: revise this construction later
namespace Bitmap
{

IBitmap *CreateBitmap(int width, int height, int color_depth)
{
	return CAllegroBitmap::CreateBitmap(width, height, color_depth);
}

IBitmap *CreateSubBitmap(IBitmap *src, const CRect &rc)
{
	return CAllegroBitmap::CreateSubBitmap(src, rc);
}

IBitmap *CreateRawObjectOwner(void *bitmap_object)
{
	return CAllegroBitmap::CreateFromRawAllegroBitmap(bitmap_object);
}

IBitmap *CreateRawObjectWrapper(void *bitmap_object)
{
	return CAllegroBitmap::WrapRawAllegroBitmap(bitmap_object);
}

IBitmap *LoadFromFile(const char *filename)
{
	return CAllegroBitmap::LoadFromFile(filename);
}

bool SaveToFile(IBitmap *bitmap, const char *filename, const void *palette)
{
	return CAllegroBitmap::SaveToFile(bitmap, filename, palette);
}

// TODO: redo this ugly workaround
// Unfortunately some of the allegro functions remaining in code require "screen"
// allegro bitmap, therefore we must set that pointer to something every time we
// assign an IBitmap to screen.
IBitmap *GetScreenBitmap()
{
	return gl_ScreenBmp;
}

void SetScreenBitmap(IBitmap *bitmap)
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

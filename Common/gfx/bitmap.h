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
// Base bitmap header
//
//=============================================================================
#ifndef __AGS_CN_GFX__BITMAP_H
#define __AGS_CN_GFX__BITMAP_H

#include "util/geometry.h"

namespace AGS
{
namespace Common
{

// Mask option for blitting one bitmap on another
enum BitmapMaskOption
{
	// Plain copies bitmap pixels
	kBitmap_Copy,
	// Consider mask color fully transparent and do not copy pixels having it
	kBitmap_Transparency
};

enum BitmapFlip
{
	kBitmap_HFlip,
	kBitmap_VFlip,
	kBitmap_HVFlip
};

} // namespace Common
} // namespace AGS


// Declare the actual bitmap class
#include "gfx/allegrobitmap.h"

namespace AGS
{
namespace Common
{

class Bitmap;

// TODO: revise this construction later
namespace BitmapHelper
{
    // Helper functions, that delete faulty bitmaps automatically, and return
    // NULL if bitmap could not be created.
    Bitmap *CreateBitmap(int width, int height, int color_depth = 0);
    Bitmap *CreateTransparentBitmap(int width, int height, int color_depth = 0);
	Bitmap *CreateSubBitmap(Bitmap *src, const Rect &rc);
    Bitmap *CreateBitmapCopy(Bitmap *src, int color_depth = 0);
    Bitmap *CreateBitmapReference(Bitmap *src);
	Bitmap *LoadFromFile(const char *filename);

	// TODO: revise this later
	// Getters and setters for screen bitmap
	// Unfortunately some of the allegro functions require "screen" allegro bitmap,
	// therefore we must set that pointer to something every time we assign an Bitmap
	// to screen.
	Bitmap	*GetScreenBitmap();
	void	SetScreenBitmap(Bitmap *bitmap);
} // namespace BitmapHelper

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__BITMAP_H

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

// Move geometry classes to their related header
struct Line
{
	int X1;
	int Y1;
	int X2;
	int Y2;

	Line()
	{
		X1 = 0;
		Y1 = 0;
		X2 = 0;
		Y2 = 0;
	}

	Line(int x1, int y1, int x2, int y2)
	{
		X1 = x1;
		Y1 = y1;
		X2 = x2;
		Y2 = y2;
	}
};

// Helper factory functions
inline Line HLine(int x1, int x2, int y)
{
	return Line(x1, y, x2, y);
}

inline Line VLine(int x, int y1, int y2)
{
	return Line(x, y1, x, y2);
}

struct Rect
{
	int Left;
	int Top;
	int Right;
	int Bottom;

	Rect()
	{
		Left	= 0;
		Top		= 0;
		Right	= 0;
		Bottom	= 0;
	}

	Rect(int l, int t, int r, int b)
	{
		Left	= l;
		Top		= t;
		Right	= r;
		Bottom	= b;
	}

	inline int GetWidth() const
	{
		return Right - Left + 1;
	}

	inline int GetHeight() const
	{
		return Bottom - Top + 1;
	}
};

// Helper factory function
inline Rect RectWH(int x, int y, int width, int height)
{
	return Rect(x, y, x + width - 1, y + height - 1);
}

struct Triangle
{
	int X1;
	int Y1;
	int X2;
	int Y2;
	int X3;
	int Y3;

	Triangle()
	{
		X1 = 0;
		Y1 = 0;
		X2 = 0;
		Y2 = 0;
		X3 = 0;
		Y3 = 0;
	}

	Triangle(int x1, int y1, int x2, int y2, int x3, int y3)
	{
		X1 = x1;
		Y1 = y1;
		X2 = x2;
		Y2 = y2;
		X3 = x3;
		Y3 = y3;
	}
};

struct Circle
{
	int X;
	int Y;
	int Radius;

	Circle()
	{
		X = 0;
		Y = 0;
		Radius = 0;
	}

	Circle(int x, int y, int radius)
	{
		X = x;
		Y = y;
		Radius = radius;
	}

};

// TODO: move to types definition
#define int32_t signed int
#define fixed_t int32_t // fixed point type
#define color_t int32_t

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
	Bitmap *CreateSubBitmap(Bitmap *src, const Rect &rc);
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

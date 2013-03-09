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

#include <aastr.h>
#include "gfx/allegrobitmap.h"
#include "util/wgt2allg.h"
#include "debug/assert.h"

namespace AGS
{
namespace Common
{

/*static*/ AllegroBitmap *AllegroBitmap::CreateBitmap(int width, int height, int color_depth)
{
	AllegroBitmap *bitmap = new AllegroBitmap();
	if (!bitmap->Create(width, height, color_depth))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

/*static*/ AllegroBitmap *AllegroBitmap::CreateSubBitmap(Bitmap *src, const Rect &rc)
{
	AllegroBitmap *bitmap = new AllegroBitmap();
	if (!bitmap->CreateShared(src, rc.Left, rc.Top, rc.GetWidth(), rc.GetHeight()))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

/*static*/ AllegroBitmap *AllegroBitmap::CreateFromRawAllegroBitmap(void *bitmap_object)
{
	if (!bitmap_object)
	{
		return NULL;
	}

	AllegroBitmap *bitmap = new AllegroBitmap();
	bitmap->_bitmap = (BITMAP*)bitmap_object;
	bitmap->_isDataOwner = true;
	return bitmap;
}

/*static*/ AllegroBitmap *AllegroBitmap::WrapRawAllegroBitmap(void *bitmap_object)
{
	if (!bitmap_object)
	{
		return NULL;
	}

	AllegroBitmap *bitmap = new AllegroBitmap();
	bitmap->_bitmap = (BITMAP*)bitmap_object;
	bitmap->_isDataOwner = false;
	return bitmap;
}

/*static*/ AllegroBitmap *AllegroBitmap::LoadFromFile(const char *filename)
{
	BITMAP *al_bmp = load_bitmap(filename, NULL);
	if (al_bmp)
	{
		AllegroBitmap *bitmap = new AllegroBitmap();
		bitmap->_bitmap = al_bmp;
		return bitmap;
	}
	return NULL;
}

/*static*/ bool AllegroBitmap::SaveToFile(Bitmap *bitmap, const char *filename, const void *palette)
{
	if (bitmap->GetClassType() != AlBmpSignature)
	{
		return false;
	}

	return save_bitmap(filename, ((AllegroBitmap*)bitmap)->_bitmap, (const RGB*)palette) == 0;
}

AllegroBitmap::AllegroBitmap()
	: _bitmap(NULL)
	, _isDataOwner(false)
{
}

AllegroBitmap::~AllegroBitmap()
{
	Destroy();
}

void AllegroBitmap::WrapBitmapObject(BITMAP *al_bmp)
{
	Destroy();
	_bitmap = al_bmp;
	_isDataOwner = false;
}

void *AllegroBitmap::GetBitmapObject()
{
	return _bitmap;
}

bool AllegroBitmap::IsMemoryBitmap() const
{
	return is_memory_bitmap(_bitmap) != 0;
}

bool AllegroBitmap::IsLinearBitmap() const
{
	return is_linear_bitmap(_bitmap) != 0;
}

bool AllegroBitmap::IsNull() const
{
	return !this || !_bitmap;
}

bool AllegroBitmap::IsEmpty() const
{
	return GetWidth() == 0 || GetHeight() == 0;
}

int AllegroBitmap::GetWidth() const
{
	return _bitmap->w;
}

int AllegroBitmap::GetHeight() const
{
	return _bitmap->h;
}

int AllegroBitmap::GetColorDepth() const
{
	return bitmap_color_depth(_bitmap);
}

int AllegroBitmap::GetBPP() const
{
	int color_depth = GetColorDepth();
	if (color_depth == 15)
		return 2;

	return color_depth >> 3;
}

int AllegroBitmap::GetDataSize() const
{
	return GetWidth() * GetHeight() * GetBPP();
}

int AllegroBitmap::GetLineLength() const
{
	return GetWidth() * GetBPP();
}

const unsigned char *AllegroBitmap::GetData() const
{
	return _bitmap->line[0];
}

const unsigned char *AllegroBitmap::GetScanLine(int index) const
{
	if (index < 0 || index >= GetHeight())
	{
		return NULL;
	}

	return _bitmap->line[index];
}

void AllegroBitmap::SetClip(const Rect &rc)
{
	set_clip(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom);
}

Rect AllegroBitmap::GetClip() const
{
	Rect temp;
	get_clip_rect(_bitmap, &temp.Left, &temp.Top, &temp.Right, &temp.Bottom);
	return temp;
}

void AllegroBitmap::SetMaskColor(color_t color)
{
	// not supported? CHECKME
}

color_t AllegroBitmap::GetMaskColor() const
{
	return bitmap_mask_color(_bitmap);
}

void AllegroBitmap::Acquire()
{
	acquire_bitmap(_bitmap);
}

void AllegroBitmap::Release()
{
	release_bitmap(_bitmap);
}

void AllegroBitmap::Clear(color_t color)
{
	if (color)
	{
		clear_to_color(_bitmap, color);
	}
	else
	{
		clear_bitmap(_bitmap);	
	}
}

//=============================================================================
// Direct access operations
//=============================================================================

unsigned char *AllegroBitmap::GetScanLineForWriting(int index)
{
	if (index < 0 || index >= GetHeight())
	{
		return NULL;
	}

	return _bitmap->line[index];
}

void AllegroBitmap::SetScanLine(int index, unsigned char *data, int data_size)
{
	if (index < 0 || index >= GetHeight())
	{
		return;
	}

	int copy_length = data_size;
	if (copy_length < 0)
	{
		copy_length = GetLineLength();
	}
	else // TODO: use Math namespace here
		if (copy_length > GetLineLength())
	{
		copy_length = GetLineLength();
	}

	memcpy(_bitmap->line[index], data, copy_length);
}

//=============================================================================
// Blitting operations (drawing one bitmap over another)
//=============================================================================

void AllegroBitmap::Blit(Bitmap *src, int dst_x, int dst_y, BitmapMaskOption mask)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	// WARNING: For some evil reason Allegro expects dest and src bitmaps in different order for blit and draw_sprite
	if (mask == kBitmap_Transparency)
	{
		draw_sprite(_bitmap, al_src_bmp, dst_x, dst_y);
	}
	else
	{
		blit(al_src_bmp, _bitmap, 0, 0, dst_x, dst_y, al_src_bmp->w, al_src_bmp->h);
	}
}

void AllegroBitmap::Blit(Bitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	if (mask == kBitmap_Transparency)
	{
		masked_blit(al_src_bmp, _bitmap, src_x, src_y, dst_x, dst_y, width, height);
	}
	else
	{
		blit(al_src_bmp, _bitmap, src_x, src_y, dst_x, dst_y, width, height);
	}
}

void AllegroBitmap::StretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	// WARNING: For some evil reason Allegro expects dest and src bitmaps in different order for blit and draw_sprite
	if (mask == kBitmap_Transparency)
	{
		stretch_sprite(_bitmap, al_src_bmp,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
	else
	{
		stretch_blit(al_src_bmp, _bitmap,
			0, 0, al_src_bmp->w, al_src_bmp->h,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void AllegroBitmap::StretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}

	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	if (mask == kBitmap_Transparency)
	{
		masked_stretch_blit(al_src_bmp, _bitmap,
			src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(),
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
	else
	{
		stretch_blit(al_src_bmp, _bitmap,
			src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(),
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void AllegroBitmap::AAStretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	// WARNING: For some evil reason Allegro expects dest and src bitmaps in different order for blit and draw_sprite
	if (mask == kBitmap_Transparency)
	{
		aa_stretch_sprite(_bitmap, al_src_bmp,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
	else
	{
		aa_stretch_blit(al_src_bmp, _bitmap,
			0, 0, al_src_bmp->w, al_src_bmp->h,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void AllegroBitmap::AAStretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	if (mask == kBitmap_Transparency)
	{
		// TODO: aastr lib does not expose method for masked stretch blit; should do that at some point since 
		// the source code is a gift-ware anyway
		// aa_masked_blit(_bitmap, al_src_bmp, src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(), dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
		throw "aa_masked_blit is not yet supported!";
	}
	else
	{
		aa_stretch_blit(al_src_bmp, _bitmap,
			src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(),
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void AllegroBitmap::TransBlendBlt(Bitmap *src, int dst_x, int dst_y)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	draw_trans_sprite(_bitmap, al_src_bmp, dst_x, dst_y);
}

void AllegroBitmap::LitBlendBlt(Bitmap *src, int dst_x, int dst_y, int light_amount)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	draw_lit_sprite(_bitmap, al_src_bmp, dst_x, dst_y, light_amount);
}

void AllegroBitmap::FlipBlt(Bitmap *src, int dst_x, int dst_y, BitmapFlip flip)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	if (flip == kBitmap_HFlip)
	{
		draw_sprite_h_flip(_bitmap, al_src_bmp, dst_x, dst_y);
	}
	else if (flip == kBitmap_VFlip)
	{
		draw_sprite_v_flip(_bitmap, al_src_bmp, dst_x, dst_y);
	}
	else if (flip == kBitmap_HVFlip)
	{
		draw_sprite_vh_flip(_bitmap, al_src_bmp, dst_x, dst_y);
	}
}

void AllegroBitmap::RotateBlt(Bitmap *src, int dst_x, int dst_y, fixed_t angle)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	rotate_sprite(_bitmap, al_src_bmp, dst_x, dst_y, angle);
}

void AllegroBitmap::RotateBlt(Bitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	pivot_sprite(_bitmap, al_src_bmp, dst_x, dst_y, pivot_x, pivot_y, angle);
}

//=============================================================================
// Vector drawing operations
//=============================================================================

void AllegroBitmap::PutPixel(int x, int y, color_t color)
{
    if (x < 0 || x >= _bitmap->w || y < 0 || y >= _bitmap->h)
    {
        return;
    }

	switch (bitmap_color_depth(_bitmap))
	{
	case 8:
		return _putpixel(_bitmap, x, y, color);
	case 15:
		return _putpixel15(_bitmap, x, y, color);
	case 16:
		return _putpixel16(_bitmap, x, y, color);
	case 24:
		return _putpixel24(_bitmap, x, y, color);
	case 32:
		return _putpixel32(_bitmap, x, y, color);
	}
    assert(0); // this should not normally happen
	return putpixel(_bitmap, x, y, color);
}

int AllegroBitmap::GetPixel(int x, int y) const
{
    if (x < 0 || x >= _bitmap->w || y < 0 || y >= _bitmap->h)
    {
        return -1; // Allegros getpixel() implementation returns -1 in this case
    }

	switch (bitmap_color_depth(_bitmap))
	{
	case 8:
		return _getpixel(_bitmap, x, y);
	case 15:
		return _getpixel15(_bitmap, x, y);
	case 16:
		return _getpixel16(_bitmap, x, y);
	case 24:
		return _getpixel24(_bitmap, x, y);
	case 32:
		return _getpixel32(_bitmap, x, y);
	}
    assert(0); // this should not normally happen
	return getpixel(_bitmap, x, y);
}

void AllegroBitmap::DrawLine(const Line &ln, color_t color)
{
	line(_bitmap, ln.X1, ln.Y1, ln.X2, ln.Y2, color);
}

void AllegroBitmap::DrawTriangle(const Triangle &tr, color_t color)
{
	triangle(_bitmap,
		tr.X1, tr.Y1, tr.X2, tr.Y2, tr.X3, tr.Y3, color);
}

void AllegroBitmap::DrawRect(const Rect &rc, color_t color)
{
	rect(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void AllegroBitmap::FillRect(const Rect &rc, color_t color)
{
	rectfill(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void AllegroBitmap::FillCircle(const Circle &circle, color_t color)
{
	circlefill(_bitmap, circle.X, circle.Y, circle.Radius, color);
}

void AllegroBitmap::FloodFill(int x, int y, color_t color)
{
	floodfill(_bitmap, x, y, color);
}

//=============================================================================
// Creation and destruction
//=============================================================================

bool AllegroBitmap::Create(int width, int height, int color_depth)
{
	Destroy();
	if (color_depth)
	{
		_bitmap = create_bitmap_ex(color_depth, width, height);
	}
	else
	{
		_bitmap = create_bitmap(width, height);
	}
	_isDataOwner = true;
	return _bitmap != NULL;
}

// TODO: use reference-counting to improve safety over allegro sub-bitmaps
bool AllegroBitmap::CreateShared(Bitmap *src, int x, int y, int width, int height)
{
	Destroy();
	if (src->GetClassType() != GetClassType())
	{
		return false;
	}
	AllegroBitmap *alSrc = (AllegroBitmap*)src;
	_bitmap = create_sub_bitmap(alSrc->_bitmap, x, y, width, height);
	_isDataOwner = true;
	return _bitmap != NULL;
}

void AllegroBitmap::Destroy()
{
	if (_isDataOwner && _bitmap)
	{
		destroy_bitmap(_bitmap);
	}
	_bitmap = NULL;
	_isDataOwner = false;
}


} // namespace Common
} // namespace AGS

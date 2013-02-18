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
#include "debug/assert.h"

namespace AGS
{
namespace Common
{

Bitmap::Bitmap()
    : _bitmap(NULL)
    , _isDataOwner(false)
{
}

Bitmap::Bitmap(int width, int height, int color_depth)
    : _bitmap(NULL)
    , _isDataOwner(false)
{
    Create(width, height, color_depth);
}

Bitmap::Bitmap(Bitmap *src, const Rect &rc)
    : _bitmap(NULL)
    , _isDataOwner(false)
{
    CreateSubBitmap(src, rc);
}

Bitmap::Bitmap(BITMAP *al_bmp, bool shared_data)
    : _bitmap(NULL)
    , _isDataOwner(false)
{
    WrapAllegroBitmap(al_bmp, shared_data);
}

Bitmap::~Bitmap()
{
    Destroy();
}

//=============================================================================
// Creation and destruction
//=============================================================================

bool Bitmap::Create(int width, int height, int color_depth)
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

bool Bitmap::CreateSubBitmap(Bitmap *src, const Rect &rc)
{
    Destroy();
    _bitmap = create_sub_bitmap(src->_bitmap, rc.Left, rc.Top, rc.GetWidth(), rc.GetHeight());
    _isDataOwner = true;
    return _bitmap != NULL;
}

bool Bitmap::WrapAllegroBitmap(BITMAP *al_bmp, bool shared_data)
{
    Destroy();
    _bitmap = al_bmp;
    _isDataOwner = !shared_data;
    return _bitmap != NULL;
}

void Bitmap::Destroy()
{
    if (_isDataOwner && _bitmap)
    {
        destroy_bitmap(_bitmap);
    }
    _bitmap = NULL;
    _isDataOwner = false;
}

bool Bitmap::LoadFromFile(const char *filename)
{
    Destroy();

	BITMAP *al_bmp = load_bitmap(filename, NULL);
	if (al_bmp)
	{
		_bitmap = al_bmp;
        _isDataOwner = true;
	}
	return NULL;
}

bool Bitmap::SaveToFile(const char *filename, const void *palette)
{
	return save_bitmap(filename, _bitmap, (const RGB*)palette) == 0;
}

void *Bitmap::GetBitmapObject()
{
	return _bitmap;
}

bool Bitmap::IsMemoryBitmap() const
{
	return is_memory_bitmap(_bitmap) != 0;
}

bool Bitmap::IsLinearBitmap() const
{
	return is_linear_bitmap(_bitmap) != 0;
}

bool Bitmap::IsNull() const
{
	return !this || !_bitmap;
}

bool Bitmap::IsEmpty() const
{
	return GetWidth() == 0 || GetHeight() == 0;
}

int Bitmap::GetWidth() const
{
	return _bitmap->w;
}

int Bitmap::GetHeight() const
{
	return _bitmap->h;
}

int Bitmap::GetColorDepth() const
{
	return bitmap_color_depth(_bitmap);
}

int Bitmap::GetBPP() const
{
	int color_depth = GetColorDepth();
	if (color_depth == 15)
		return 2;

	return color_depth >> 3;
}

int Bitmap::GetDataSize() const
{
	return GetWidth() * GetHeight() * GetBPP();
}

int Bitmap::GetLineLength() const
{
	return GetWidth() * GetBPP();
}

const unsigned char *Bitmap::GetData() const
{
	return _bitmap->line[0];
}

const unsigned char *Bitmap::GetScanLine(int index) const
{
	if (index < 0 || index >= GetHeight())
	{
		return NULL;
	}

	return _bitmap->line[index];
}

void Bitmap::SetClip(const Rect &rc)
{
	set_clip(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom);
}

Rect Bitmap::GetClip() const
{
	Rect temp;
	get_clip_rect(_bitmap, &temp.Left, &temp.Top, &temp.Right, &temp.Bottom);
	return temp;
}

void Bitmap::SetMaskColor(color_t color)
{
	// not supported? CHECKME
}

color_t Bitmap::GetMaskColor() const
{
	return bitmap_mask_color(_bitmap);
}

void Bitmap::Acquire()
{
	acquire_bitmap(_bitmap);
}

void Bitmap::Release()
{
	release_bitmap(_bitmap);
}

void Bitmap::Clear(color_t color)
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

unsigned char *Bitmap::GetScanLineForWriting(int index)
{
	if (index < 0 || index >= GetHeight())
	{
		return NULL;
	}

	return _bitmap->line[index];
}

void Bitmap::SetScanLine(int index, unsigned char *data, int data_size)
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

void Bitmap::Blit(Bitmap *src, int dst_x, int dst_y, BitmapMaskOption mask)
{	
	BITMAP *al_src_bmp = src->_bitmap;
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

void Bitmap::Blit(Bitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_bitmap;
	if (mask == kBitmap_Transparency)
	{
		masked_blit(al_src_bmp, _bitmap, src_x, src_y, dst_x, dst_y, width, height);
	}
	else
	{
		blit(al_src_bmp, _bitmap, src_x, src_y, dst_x, dst_y, width, height);
	}
}

void Bitmap::StretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_bitmap;
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

void Bitmap::StretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_bitmap;
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

void Bitmap::AAStretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_bitmap;
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

void Bitmap::AAStretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_bitmap;
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

void Bitmap::TransBlendBlt(Bitmap *src, int dst_x, int dst_y)
{
	BITMAP *al_src_bmp = src->_bitmap;
	draw_trans_sprite(_bitmap, al_src_bmp, dst_x, dst_y);
}

void Bitmap::LitBlendBlt(Bitmap *src, int dst_x, int dst_y, int light_amount)
{
	BITMAP *al_src_bmp = src->_bitmap;
	draw_lit_sprite(_bitmap, al_src_bmp, dst_x, dst_y, light_amount);
}

void Bitmap::FlipBlt(Bitmap *src, int dst_x, int dst_y, BitmapFlip flip)
{	
	BITMAP *al_src_bmp = src->_bitmap;
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

void Bitmap::RotateBlt(Bitmap *src, int dst_x, int dst_y, fixed_t angle)
{
	BITMAP *al_src_bmp = src->_bitmap;
	rotate_sprite(_bitmap, al_src_bmp, dst_x, dst_y, angle);
}

void Bitmap::RotateBlt(Bitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle)
{	
	BITMAP *al_src_bmp = src->_bitmap;
	pivot_sprite(_bitmap, al_src_bmp, dst_x, dst_y, pivot_x, pivot_x, angle);
}

//=============================================================================
// Vector drawing operations
//=============================================================================

void Bitmap::PutPixel(int x, int y, color_t color)
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

int Bitmap::GetPixel(int x, int y) const
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

void Bitmap::DrawLine(const Line &ln, color_t color)
{
	line(_bitmap, ln.X1, ln.Y1, ln.X2, ln.Y2, color);
}

void Bitmap::DrawTriangle(const Triangle &tr, color_t color)
{
	triangle(_bitmap,
		tr.X1, tr.Y1, tr.X2, tr.Y2, tr.X3, tr.Y3, color);
}

void Bitmap::DrawRect(const Rect &rc, color_t color)
{
	rect(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void Bitmap::FillRect(const Rect &rc, color_t color)
{
	rectfill(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void Bitmap::FillCircle(const Circle &circle, color_t color)
{
	circlefill(_bitmap, circle.X, circle.Y, circle.Radius, color);
}

void Bitmap::FloodFill(int x, int y, color_t color)
{
	floodfill(_bitmap, x, y, color);
}



namespace BitmapHelper
{

Bitmap *CreateRawBitmapOwner(BITMAP *al_bmp)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->WrapAllegroBitmap(al_bmp, false))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

Bitmap *CreateRawBitmapWrapper(BITMAP *al_bmp)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->WrapAllegroBitmap(al_bmp, true))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

} // namespace BitmapHelper


} // namespace Common
} // namespace AGS

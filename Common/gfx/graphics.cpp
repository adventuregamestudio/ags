
#include <aastr.h>
#include "core/types.h"
#include "debug/assert.h"
#include "gfx/bitmap.h"
#include "gfx/graphics.h"

extern void __my_setcolor(int *ctset, int newcol, int wantColDep);

namespace AGS
{
namespace Common
{

Graphics::Graphics()
    : _bitmap(NULL)
    , _alBitmap(NULL)
    , _drawColor(0)
    , _textColor(0)
{
}

Graphics::Graphics(Bitmap *bitmap)
    : _bitmap(NULL)
    , _alBitmap(NULL)
    , _drawColor(0)
    , _textColor(0)
{
    SetBitmap(bitmap);
}

Graphics::~Graphics()
{
    ReleaseBitmap();
}

void Graphics::SetBitmap(Bitmap *bitmap)
{
    if (bitmap != _bitmap)
    {
        ReleaseBitmap();
        _bitmap = bitmap;
        if (_bitmap)
        {
            _bitmap->_graphics = this;
            _alBitmap = _bitmap->_alBitmap;
        }
    }
}

Bitmap *Graphics::GetBitmap()
{
    return _bitmap;
}

void Graphics::SetDrawColor(color_t color)
{
    if (_bitmap)
    {
        __my_setcolor(&_drawColor, color, _bitmap->GetColorDepth());
    }
    else
    {
        _drawColor = color;
    }
}

void Graphics::SetTextColor(color_t color)
{
    if (_bitmap)
    {
        __my_setcolor(&_textColor, color, _bitmap->GetColorDepth());
    }
    else
    {
        _textColor = color;
    }
}

void Graphics::SetDrawColorExact(color_t color)
{
    _drawColor = color;
}

void Graphics::SetTextColorExact(color_t color)
{
    _textColor = color;
}

color_t Graphics::GetDrawColor()
{
    return _drawColor;
}

color_t Graphics::GetTextColor()
{
    return _textColor;
}

void Graphics::SetClip(const Rect &rc)
{
	set_clip(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom);
}

Rect Graphics::GetClip() const
{
	Rect temp;
	get_clip_rect(_alBitmap, &temp.Left, &temp.Top, &temp.Right, &temp.Bottom);
	return temp;
}

//=============================================================================
// Blitting operations (drawing one bitmap over another)
//=============================================================================

void Graphics::Blit(Bitmap *src, int dst_x, int dst_y, BitmapMaskOption mask)
{	
	BITMAP *al_src_bmp = src->_alBitmap;
	// WARNING: For some evil reason Allegro expects dest and src bitmaps in different order for blit and draw_sprite
	if (mask == kBitmap_Transparency)
	{
		draw_sprite(_alBitmap, al_src_bmp, dst_x, dst_y);
	}
	else
	{
		blit(al_src_bmp, _alBitmap, 0, 0, dst_x, dst_y, al_src_bmp->w, al_src_bmp->h);
	}
}

void Graphics::Blit(Bitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	if (mask == kBitmap_Transparency)
	{
		masked_blit(al_src_bmp, _alBitmap, src_x, src_y, dst_x, dst_y, width, height);
	}
	else
	{
		blit(al_src_bmp, _alBitmap, src_x, src_y, dst_x, dst_y, width, height);
	}
}

void Graphics::StretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	// WARNING: For some evil reason Allegro expects dest and src bitmaps in different order for blit and draw_sprite
	if (mask == kBitmap_Transparency)
	{
		stretch_sprite(_alBitmap, al_src_bmp,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
	else
	{
		stretch_blit(al_src_bmp, _alBitmap,
			0, 0, al_src_bmp->w, al_src_bmp->h,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void Graphics::StretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	if (mask == kBitmap_Transparency)
	{
		masked_stretch_blit(al_src_bmp, _alBitmap,
			src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(),
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
	else
	{
		stretch_blit(al_src_bmp, _alBitmap,
			src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(),
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void Graphics::AAStretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	// WARNING: For some evil reason Allegro expects dest and src bitmaps in different order for blit and draw_sprite
	if (mask == kBitmap_Transparency)
	{
		aa_stretch_sprite(_alBitmap, al_src_bmp,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
	else
	{
		aa_stretch_blit(al_src_bmp, _alBitmap,
			0, 0, al_src_bmp->w, al_src_bmp->h,
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void Graphics::AAStretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	if (mask == kBitmap_Transparency)
	{
		// TODO: aastr lib does not expose method for masked stretch blit; should do that at some point since 
		// the source code is a gift-ware anyway
		// aa_masked_blit(_alBitmap, al_src_bmp, src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(), dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
		throw "aa_masked_blit is not yet supported!";
	}
	else
	{
		aa_stretch_blit(al_src_bmp, _alBitmap,
			src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(),
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void Graphics::TransBlendBlt(Bitmap *src, int dst_x, int dst_y)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	draw_trans_sprite(_alBitmap, al_src_bmp, dst_x, dst_y);
}

void Graphics::LitBlendBlt(Bitmap *src, int dst_x, int dst_y, int light_amount)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	draw_lit_sprite(_alBitmap, al_src_bmp, dst_x, dst_y, light_amount);
}

void Graphics::FlipBlt(Bitmap *src, int dst_x, int dst_y, BitmapFlip flip)
{	
	BITMAP *al_src_bmp = src->_alBitmap;
	if (flip == kBitmap_HFlip)
	{
		draw_sprite_h_flip(_alBitmap, al_src_bmp, dst_x, dst_y);
	}
	else if (flip == kBitmap_VFlip)
	{
		draw_sprite_v_flip(_alBitmap, al_src_bmp, dst_x, dst_y);
	}
	else if (flip == kBitmap_HVFlip)
	{
		draw_sprite_vh_flip(_alBitmap, al_src_bmp, dst_x, dst_y);
	}
}

void Graphics::RotateBlt(Bitmap *src, int dst_x, int dst_y, fixed_t angle)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	rotate_sprite(_alBitmap, al_src_bmp, dst_x, dst_y, angle);
}

void Graphics::RotateBlt(Bitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle)
{	
	BITMAP *al_src_bmp = src->_alBitmap;
	pivot_sprite(_alBitmap, al_src_bmp, dst_x, dst_y, pivot_x, pivot_y, angle);
}

//=============================================================================
// Pixel operations
//=============================================================================

void Graphics::PutPixel(int x, int y)
{
    if (x < 0 || x >= _alBitmap->w || y < 0 || y >= _alBitmap->h)
    {
        return;
    }

	switch (bitmap_color_depth(_alBitmap))
	{
	case 8:
        return _putpixel(_alBitmap, x, y, _drawColor);
	case 15:
		return _putpixel15(_alBitmap, x, y, _drawColor);
	case 16:
		return _putpixel16(_alBitmap, x, y, _drawColor);
	case 24:
		return _putpixel24(_alBitmap, x, y, _drawColor);
	case 32:
		return _putpixel32(_alBitmap, x, y, _drawColor);
	}
    assert(0); // this should not normally happen
	return putpixel(_alBitmap, x, y, _drawColor);
}

void Graphics::PutPixel(int x, int y, color_t color)
{
    if (x < 0 || x >= _alBitmap->w || y < 0 || y >= _alBitmap->h)
    {
        return;
    }

	switch (bitmap_color_depth(_alBitmap))
	{
	case 8:
		return _putpixel(_alBitmap, x, y, color);
	case 15:
		return _putpixel15(_alBitmap, x, y, color);
	case 16:
		return _putpixel16(_alBitmap, x, y, color);
	case 24:
		return _putpixel24(_alBitmap, x, y, color);
	case 32:
		return _putpixel32(_alBitmap, x, y, color);
	}
    assert(0); // this should not normally happen
	return putpixel(_alBitmap, x, y, color);
}

int Graphics::GetPixel(int x, int y) const
{
    if (x < 0 || x >= _alBitmap->w || y < 0 || y >= _alBitmap->h)
    {
        return -1; // Allegros getpixel() implementation returns -1 in this case
    }

	switch (bitmap_color_depth(_alBitmap))
	{
	case 8:
		return _getpixel(_alBitmap, x, y);
	case 15:
		return _getpixel15(_alBitmap, x, y);
	case 16:
		return _getpixel16(_alBitmap, x, y);
	case 24:
		return _getpixel24(_alBitmap, x, y);
	case 32:
		return _getpixel32(_alBitmap, x, y);
	}
    assert(0); // this should not normally happen
	return getpixel(_alBitmap, x, y);
}

//=============================================================================
// Vector drawing operations
//=============================================================================

void Graphics::DrawLine(const Line &ln)
{
	line(_alBitmap, ln.X1, ln.Y1, ln.X2, ln.Y2, _drawColor);
}

void Graphics::DrawLine(const Line &ln, color_t color)
{
	line(_alBitmap, ln.X1, ln.Y1, ln.X2, ln.Y2, color);
}

void Graphics::DrawTriangle(const Triangle &tr)
{
	triangle(_alBitmap,
		tr.X1, tr.Y1, tr.X2, tr.Y2, tr.X3, tr.Y3, _drawColor);
}

void Graphics::DrawTriangle(const Triangle &tr, color_t color)
{
	triangle(_alBitmap,
		tr.X1, tr.Y1, tr.X2, tr.Y2, tr.X3, tr.Y3, color);
}

void Graphics::DrawRect(const Rect &rc)
{
	rect(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, _drawColor);
}

void Graphics::DrawRect(const Rect &rc, color_t color)
{
	rect(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void Graphics::FillRect(const Rect &rc)
{
	rectfill(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, _drawColor);
}

void Graphics::FillRect(const Rect &rc, color_t color)
{
	rectfill(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void Graphics::FillCircle(const Circle &circle)
{
	circlefill(_alBitmap, circle.X, circle.Y, circle.Radius, _drawColor);
}

void Graphics::FillCircle(const Circle &circle, color_t color)
{
	circlefill(_alBitmap, circle.X, circle.Y, circle.Radius, color);
}

void Graphics::Fill(color_t color)
{
	if (color)
	{
		clear_to_color(_alBitmap, color);
	}
	else
	{
		clear_bitmap(_alBitmap);	
	}
}

void Graphics::FillTransparent()
{
    clear_to_color(_alBitmap, bitmap_mask_color(_alBitmap));
}

void Graphics::FloodFill(int x, int y)
{
    floodfill(_alBitmap, x, y, _drawColor);
}

void Graphics::FloodFill(int x, int y, color_t color)
{
	floodfill(_alBitmap, x, y, color);
}

void Graphics::ReleaseBitmap()
{
    if (_bitmap)
    {
        _bitmap->_graphics = NULL;
    }
    _bitmap = NULL;
    _alBitmap = NULL;    
    _drawColor = 0;
    _textColor = 0;
}

} // namespace Common
} // namespace AGS

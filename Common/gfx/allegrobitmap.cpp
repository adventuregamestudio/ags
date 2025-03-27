//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <memory>
#include <stdexcept>
#include <string.h> // memcpy
#include <aastr.h>
#include <SDL.h>
#include "gfx/allegrobitmap.h"
#include "util/filestream.h"
#include "debug/assert.h"

namespace AGS
{
namespace Common
{

Bitmap::Bitmap(int width, int height, int color_depth)
{
    Create(width, height, color_depth);
}

Bitmap::Bitmap(PixelBuffer &&pxbuf)
{
    Create(std::move(pxbuf));
}

Bitmap::Bitmap(const Bitmap *src, const Rect &rc)
{
    CreateSubBitmap(src, rc);
}

Bitmap::Bitmap(BITMAP *al_bmp, bool shared_data)
{
    WrapAllegroBitmap(al_bmp, shared_data);
}

Bitmap::Bitmap(SDL_Surface *sdl_bmp, bool shared_data)
{
    WrapSDLSurface(sdl_bmp, shared_data);
}

Bitmap::Bitmap(const Bitmap &bmp)
{
    CreateCopy(&bmp);
}

Bitmap::Bitmap(Bitmap &&bmp)
{
    _pixelData = std::move(bmp._pixelData);
    _alBitmap = bmp._alBitmap;
    _isDataOwner = bmp._isDataOwner;
    bmp._alBitmap = nullptr;
    bmp._isDataOwner = false;
}

Bitmap::~Bitmap()
{
    Destroy();
}

Bitmap &Bitmap::operator =(const Bitmap &bmp)
{
    CreateCopy(&bmp);
    return *this;
}

//=============================================================================
// Creation and destruction
//=============================================================================

bool Bitmap::Create(int width, int height, int color_depth)
{
    Destroy();

    if (color_depth == 0)
        color_depth = get_color_depth();

    size_t need_size;
    create_bitmap_userdata(color_depth, width, height, nullptr, 0u, 0u, &need_size);
    std::unique_ptr<uint8_t[]> data(new uint8_t[need_size]);
    BITMAP *bitmap = create_bitmap_userdata(color_depth, width, height, data.get(), need_size, 0u, nullptr);
    if (!bitmap)
        return false;

    _pixelData = std::move(data);
    _alBitmap = bitmap;
    _isDataOwner = true;
    _pitch = GetWidth() * GetBPP();
    return true;
}

bool Bitmap::CreateTransparent(int width, int height, int color_depth)
{
    if (Create(width, height, color_depth))
    {
        clear_to_color(_alBitmap, bitmap_mask_color(_alBitmap));
        return true;
    }
    return false;
}

bool Bitmap::Create(PixelBuffer &&pxbuf)
{
    Destroy();

    const int color_depth = PixelFormatToPixelBits(pxbuf.GetFormat());
    const int width = pxbuf.GetWidth(), height = pxbuf.GetHeight();
    size_t data_sz = pxbuf.GetDataSize();
    std::unique_ptr<uint8_t[]> data = pxbuf.ReleaseData();
    // Do safety check, if provided data buffer is not long enough for Allegro BITMAP,
    // then create a correct one and copy contents over.
    size_t need_size;
    create_bitmap_userdata(color_depth, width, height, nullptr, 0u, 0u, &need_size);
    if (need_size > data_sz)
    {
        std::unique_ptr<uint8_t[]> copy_buf(new uint8_t[need_size]);
        std::copy(data.get(), data.get() + data_sz, copy_buf.get());
        data = std::move(copy_buf);
        data_sz = need_size;
    }
    
    BITMAP *bitmap = create_bitmap_userdata(color_depth, width, height, data.get(), data_sz, 0u, nullptr);
    if (!bitmap)
        return false;

    _pixelData = std::move(data);
    _alBitmap = bitmap;
    _isDataOwner = true;
    _pitch = GetWidth() * GetBPP();
    return true;
}

bool Bitmap::CreateSubBitmap(const Bitmap *src, const Rect &rc)
{
    if (src == this || src->_alBitmap == _alBitmap)
        return false; // cannot create a sub bitmap of yourself

    Destroy();
    _alBitmap = create_sub_bitmap(src->_alBitmap, rc.Left, rc.Top, rc.GetWidth(), rc.GetHeight());
    _isDataOwner = true;
    _pitch = GetWidth() * GetBPP();
    return _alBitmap != nullptr;
}

bool Bitmap::ResizeSubBitmap(int width, int height)
{
    if (!is_sub_bitmap(_alBitmap))
        return false;
    // TODO: can't clamp to parent size, because subs do not keep parent ref;
    // might require amending allegro bitmap struct
    _alBitmap->w = _alBitmap->cr = width;
    _alBitmap->h = _alBitmap->cb = height;
    _pitch = GetWidth() * GetBPP();
    return true;
}

bool Bitmap::CreateCopy(const Bitmap *src, int color_depth)
{
    if (src == this || src->_alBitmap == _alBitmap)
        return false; // cannot create a copy of yourself

    // Handle uninitialized bitmap case
    if (!src->_alBitmap)
    {
        Destroy();
        return true;
    }

    if (Create(src->_alBitmap->w, src->_alBitmap->h, color_depth ? color_depth : bitmap_color_depth(src->_alBitmap)))
    {
        blit(src->_alBitmap, _alBitmap, 0, 0, 0, 0, _alBitmap->w, _alBitmap->h);
        return true;
    }
    return false;
}

bool Bitmap::WrapAllegroBitmap(BITMAP *al_bmp, bool shared_data)
{
    if (al_bmp == _alBitmap)
        return false; // cannot wrap your own internal BITMAP

    Destroy();
    _alBitmap = al_bmp;
    _isDataOwner = !shared_data;
    _pitch = GetWidth() * GetBPP();
    return _alBitmap != nullptr;
}

void Bitmap::ForgetAllegroBitmap()
{
    _alBitmap = nullptr;
    _isDataOwner = false;
    _pixelData = {};
    _pitch = 0;
}

bool Bitmap::WrapSDLSurface(SDL_Surface *sdl_bmp, bool shared_data)
{
    Destroy();

    _alBitmap = create_bitmap_userdata(sdl_bmp->format->BitsPerPixel,
        sdl_bmp->w, sdl_bmp->h, sdl_bmp->pixels, sdl_bmp->h * sdl_bmp->pitch, sdl_bmp->pitch, nullptr);
    if (!_alBitmap)
        return false;

    _sdlBitmap = sdl_bmp;
    _isDataOwner = !shared_data;
    _pitch = _sdlBitmap->pitch;
    return true;
}

void Bitmap::Destroy()
{
    if (_alBitmap && (_isDataOwner || _sdlBitmap))
    {
        destroy_bitmap(_alBitmap);
    }
    if (_sdlBitmap && _isDataOwner)
    {
        SDL_FreeSurface(_sdlBitmap);
    }

    _alBitmap = nullptr;
    _sdlBitmap = nullptr;
    _isDataOwner = false;
    _pixelData = {};
    _pitch = 0;
}

bool Bitmap::SaveToFile(const char *filename, const RGB *palette)
{
	return BitmapHelper::SaveToFile(this, filename, palette);
}

color_t Bitmap::GetCompatibleColor(int color)
{
    return BitmapHelper::AGSColorToBitmapColor(color, bitmap_color_depth(_alBitmap));
}

//=============================================================================
// Clipping
//=============================================================================

void Bitmap::SetClip(const Rect &rc)
{
	set_clip_rect(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom);
}

void Bitmap::ResetClip()
{
    set_clip_rect(_alBitmap, 0, 0, _alBitmap->w - 1, _alBitmap->h - 1);
}

Rect Bitmap::GetClip() const
{
	Rect temp;
	get_clip_rect(_alBitmap, &temp.Left, &temp.Top, &temp.Right, &temp.Bottom);
	return temp;
}

//=============================================================================
// Blitting operations (drawing one bitmap over another)
//=============================================================================

void Bitmap::Blit(const Bitmap *src, int dst_x, int dst_y, BitmapMaskOption mask)
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

void Bitmap::Blit(const Bitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask)
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

void Bitmap::MaskedBlit(const Bitmap *src, int dst_x, int dst_y)
{
    draw_sprite(_alBitmap, src->_alBitmap, dst_x, dst_y);
}

void Bitmap::StretchBlt(const Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
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

void Bitmap::StretchBlt(const Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
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

void Bitmap::AAStretchBlt(const Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask)
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

void Bitmap::AAStretchBlt(const Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	if (mask == kBitmap_Transparency)
	{
		// TODO: aastr lib does not expose method for masked stretch blit; should do that at some point since 
		// the source code is a gift-ware anyway
		// aa_masked_blit(_alBitmap, al_src_bmp, src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(), dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
		throw std::runtime_error("aa_masked_blit is not yet supported!");
	}
	else
	{
		aa_stretch_blit(al_src_bmp, _alBitmap,
			src_rc.Left, src_rc.Top, src_rc.GetWidth(), src_rc.GetHeight(),
			dst_rc.Left, dst_rc.Top, dst_rc.GetWidth(), dst_rc.GetHeight());
	}
}

void Bitmap::TransBlendBlt(const Bitmap *src, int dst_x, int dst_y)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	draw_trans_sprite(_alBitmap, al_src_bmp, dst_x, dst_y);
}

void Bitmap::LitBlendBlt(const Bitmap *src, int dst_x, int dst_y, int light_amount)
{
	BITMAP *al_src_bmp = src->_alBitmap;
	draw_lit_sprite(_alBitmap, al_src_bmp, dst_x, dst_y, light_amount);
}

void Bitmap::FlipBlt(const Bitmap *src, int dst_x, int dst_y, GraphicFlip flip)
{	
	BITMAP *al_src_bmp = src->_alBitmap;
	switch (flip)
	{
	case kFlip_Horizontal:
		draw_sprite_h_flip(_alBitmap, al_src_bmp, dst_x, dst_y);
		break;
	case kFlip_Vertical:
		draw_sprite_v_flip(_alBitmap, al_src_bmp, dst_x, dst_y);
		break;
	case kFlip_Both:
		draw_sprite_vh_flip(_alBitmap, al_src_bmp, dst_x, dst_y);
		break;
	default: // blit with no transform
		Blit(src, dst_x, dst_y);
		break;
	}
}

void Bitmap::RotateBlt(const Bitmap *src, int dst_x, int dst_y, int angle)
{
    // convert to allegro angle
    fixed_t al_angle = itofix((angle * 256) / 360);
    BITMAP *al_src_bmp = src->_alBitmap;
    rotate_sprite(_alBitmap, al_src_bmp, dst_x, dst_y, al_angle);
}

void Bitmap::RotateBlt(const Bitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, int angle)
{
    // convert to allegro angle
    fixed_t al_angle = itofix((angle * 256) / 360);
    BITMAP *al_src_bmp = src->_alBitmap;
    pivot_sprite(_alBitmap, al_src_bmp, dst_x, dst_y, pivot_x, pivot_y, al_angle);
}

//=============================================================================
// Pixel operations
//=============================================================================

void Bitmap::Clear(color_t color)
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

void Bitmap::ClearTransparent()
{
    clear_to_color(_alBitmap, bitmap_mask_color(_alBitmap));
}

void Bitmap::PutPixel(int x, int y, color_t color)
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
	default: assert(0); // this should not normally happen
	}
	return putpixel(_alBitmap, x, y, color);
}

// hack into allegro...
extern "C" {
    extern BLENDER_FUNC _blender_func15;
    extern BLENDER_FUNC _blender_func16;
    extern BLENDER_FUNC _blender_func24;
    extern BLENDER_FUNC _blender_func32;
}

void Bitmap::BlendPixel(int x, int y, color_t color)
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
        return _putpixel15(_alBitmap, x, y, _blender_func15(color, _getpixel15(_alBitmap, x, y), 0xFF));
    case 16:
        return _putpixel16(_alBitmap, x, y, _blender_func16(color, _getpixel16(_alBitmap, x, y), 0xFF));
    case 24:
        return _putpixel24(_alBitmap, x, y, _blender_func24(color, _getpixel24(_alBitmap, x, y), 0xFF));
    case 32:
        return _putpixel32(_alBitmap, x, y, _blender_func32(color, _getpixel32(_alBitmap, x, y), 0xFF));
    default: assert(0); // this should not normally happen
    }
    return putpixel(_alBitmap, x, y, color);
}

int Bitmap::GetPixel(int x, int y) const
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

void Bitmap::DrawLine(const Line &ln, color_t color)
{
	line(_alBitmap, ln.X1, ln.Y1, ln.X2, ln.Y2, color);
}

void Bitmap::DrawTriangle(const Triangle &tr, color_t color)
{
	triangle(_alBitmap,
		tr.X1, tr.Y1, tr.X2, tr.Y2, tr.X3, tr.Y3, color);
}

void Bitmap::DrawRect(const Rect &rc, color_t color)
{
	rect(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void Bitmap::FillRect(const Rect &rc, color_t color)
{
	rectfill(_alBitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void Bitmap::FillCircle(const Circle &circle, color_t color)
{
	circlefill(_alBitmap, circle.X, circle.Y, circle.Radius, color);
}

void Bitmap::Fill(color_t color)
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

void Bitmap::FillTransparent()
{
    clear_to_color(_alBitmap, bitmap_mask_color(_alBitmap));
}

void Bitmap::FloodFill(int x, int y, color_t color)
{
	floodfill(_alBitmap, x, y, color);
}

//=============================================================================
// Direct access operations
//=============================================================================

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

	memcpy(_alBitmap->line[index], data, copy_length);
}



namespace BitmapHelper
{

int AGSColorToBitmapColor(int color, int /*color_depth*/)
{
    // no conversion necessary, we assume that "ags color" is matching
    // palette index in 8-bit mode and 32-bit A8R8G8B8 in 32-bit mode
    return color;
}

void AGSColorToRGB(int color, int color_depth, RGB &rgb)
{
    if (color_depth == 8)
    {
        if (color < 0 || color > 255)
            color = 0;
        get_color(color, &rgb);
        rgb.a = 0xFF; // pseudo-alpha
    }
    else
    {
        // assume is always A8R8G8B8
        rgb.r = getr32(color);
        rgb.g = getg32(color);
        rgb.b = getb32(color);
        rgb.a = 0xFF; // pseudo-alpha
    }
}

Bitmap *CreateRawBitmapOwner(BITMAP *al_bmp)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->WrapAllegroBitmap(al_bmp, false))
	{
		delete bitmap;
		bitmap = nullptr;
	}
	return bitmap;
}

Bitmap *CreateRawBitmapWrapper(BITMAP *al_bmp)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->WrapAllegroBitmap(al_bmp, true))
	{
		delete bitmap;
		bitmap = nullptr;
	}
	return bitmap;
}

} // namespace BitmapHelper


} // namespace Common
} // namespace AGS

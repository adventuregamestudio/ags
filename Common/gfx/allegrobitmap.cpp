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

#include "gfx/allegrobitmap.h"
#include "gfx/graphics.h"
#include "debug/assert.h"

extern void __my_setcolor(int *ctset, int newcol, int wantColDep);

namespace AGS
{
namespace Common
{

Bitmap::Bitmap()
    : _alBitmap(NULL)
    , _refCount(NULL)
    , _graphics(NULL)
{
}

Bitmap::Bitmap(int width, int height, int color_depth)
    : _alBitmap(NULL)
    , _refCount(NULL)
    , _graphics(NULL)
{
    Create(width, height, color_depth);
}

Bitmap::Bitmap(Bitmap *src, const Rect &rc)
    : _alBitmap(NULL)
    , _refCount(NULL)
    , _graphics(NULL)
{
    CreateSubBitmap(src, rc);
}

Bitmap::Bitmap(BITMAP *al_bmp, bool shared_data)
    : _alBitmap(NULL)
    , _refCount(NULL)
    , _graphics(NULL)
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
        _alBitmap = create_bitmap_ex(color_depth, width, height);
    }
    else
    {
        _alBitmap = create_bitmap(width, height);
    }
    _refCount = new int(1);
    return _alBitmap != NULL;
}

bool Bitmap::CreateTransparent(int width, int height, int color_depth)
{
    Destroy();
    if (color_depth)
    {
        _alBitmap = create_bitmap_ex(color_depth, width, height);
    }
    else
    {
        _alBitmap = create_bitmap(width, height);
    }
    if (_alBitmap)
    {
        clear_to_color(_alBitmap, bitmap_mask_color(_alBitmap));
    }
    _refCount = new int(1);
    return _alBitmap != NULL;
}

bool Bitmap::CreateSubBitmap(Bitmap *src, const Rect &rc)
{
    Destroy();
    _alBitmap = create_sub_bitmap(src->_alBitmap, rc.Left, rc.Top, rc.GetWidth(), rc.GetHeight());
    _refCount = new int(1);
    return _alBitmap != NULL;
}

bool Bitmap::CreateCopy(Bitmap *src, int color_depth)
{
    Destroy();
    if (color_depth)
    {
        _alBitmap = create_bitmap_ex(color_depth, src->_alBitmap->w, src->_alBitmap->h);
    }
    else
    {
        _alBitmap = create_bitmap_ex(bitmap_color_depth(src->_alBitmap), src->_alBitmap->w, src->_alBitmap->h);
    }

    if (_alBitmap)
    {
        blit(src->_alBitmap, _alBitmap, 0, 0, 0, 0, _alBitmap->w, _alBitmap->h);
    }
    _refCount = new int(1);
    return _alBitmap != NULL;
}

bool Bitmap::CreateReference(Bitmap *src)
{
    if (src->_alBitmap == _alBitmap)
    {
        return true;
    }

    Destroy();
    _alBitmap = src->_alBitmap;
    _refCount = src->_refCount;
    if (_refCount)
    {
        (*_refCount)++;
    }
    return _alBitmap != NULL;
}

bool Bitmap::WrapAllegroBitmap(BITMAP *al_bmp, bool shared_data)
{
    Destroy();
    _alBitmap = al_bmp;
    if (_alBitmap && !shared_data)
    {
        _refCount = new int(1);
    }
    return _alBitmap != NULL;
}

void Bitmap::Destroy()
{
    if (_graphics)
    {
        _graphics->SetBitmap(NULL);
    }
    _graphics = NULL;

    if (_refCount)
    {
        (*_refCount)--;
        if (!*_refCount && _alBitmap)
        {
            delete _refCount;
            destroy_bitmap(_alBitmap);
        }
    }
    _alBitmap = NULL;
    _refCount = NULL;
}

bool Bitmap::LoadFromFile(const char *filename)
{
    Destroy();

	BITMAP *al_bmp = load_bitmap(filename, NULL);
	if (al_bmp)
	{
		_alBitmap = al_bmp;
        _refCount = new int(1);
	}
	return NULL;
}

bool Bitmap::SaveToFile(const char *filename, const void *palette)
{
	return save_bitmap(filename, _alBitmap, (const RGB*)palette) == 0;
}

void Bitmap::SetMaskColor(color_t color)
{
	// not supported? CHECKME
}

void Bitmap::Acquire()
{
	acquire_bitmap(_alBitmap);
}

void Bitmap::Release()
{
	release_bitmap(_alBitmap);
}

color_t Bitmap::GetCompatibleColor(color_t color)
{
    color_t compat_color = 0;
    __my_setcolor(&compat_color, color, bitmap_color_depth(_alBitmap));
    return compat_color;
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
	}
    assert(0); // this should not normally happen
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


#include <aastr.h>
#include "gfx/allegrobitmap.h"
#include "util/wgt2allg.h"

namespace AGS
{
namespace Common
{

/*static*/ CAllegroBitmap *CAllegroBitmap::CreateBitmap(int width, int height, int color_depth)
{
	CAllegroBitmap *bitmap = new CAllegroBitmap();
	if (!bitmap->Create(width, height, color_depth))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

/*static*/ CAllegroBitmap *CAllegroBitmap::CreateSubBitmap(IBitmap *src, const CRect &rc)
{
	CAllegroBitmap *bitmap = new CAllegroBitmap();
	if (!bitmap->CreateShared(src, rc.Left, rc.Top, rc.GetWidth(), rc.GetHeight()))
	{
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}

/*static*/ CAllegroBitmap *CAllegroBitmap::CreateFromRawAllegroBitmap(void *bitmap_data)
{
	if (!bitmap_data)
	{
		return NULL;
	}

	CAllegroBitmap *bitmap = new CAllegroBitmap();
	bitmap->_bitmap = (BITMAP*)bitmap_data;
	return bitmap;
}

/*static*/ CAllegroBitmap *CAllegroBitmap::WrapRawAllegroBitmap(void *bitmap_data)
{
	if (!bitmap_data)
	{
		return NULL;
	}

	CAllegroBitmap *bitmap = new CAllegroBitmap();
	bitmap->_bitmap = (BITMAP*)bitmap_data;
	bitmap->_isDataOwner = false;
	return bitmap;
}

/*static*/ CAllegroBitmap *CAllegroBitmap::LoadFromFile(const char *filename)
{
	BITMAP *al_bmp = load_bitmap(filename, NULL);
	if (al_bmp)
	{
		CAllegroBitmap *bitmap = new CAllegroBitmap();
		bitmap->_bitmap = al_bmp;
		return bitmap;
	}
	return NULL;
}

/*static*/ bool CAllegroBitmap::SaveToFile(IBitmap *bitmap, const char *filename, const void *palette)
{
	if (bitmap->GetClassType() != AlBmpSignature)
	{
		return false;
	}

	return save_bitmap(filename, ((CAllegroBitmap*)bitmap)->_bitmap, (const RGB*)palette) == 0;
}

CAllegroBitmap::CAllegroBitmap()
	: _bitmap(NULL)
	, _isDataOwner(false)
{
}

CAllegroBitmap::~CAllegroBitmap()
{
	Destroy();
}

void CAllegroBitmap::WrapBitmapObject(BITMAP *al_bmp)
{
	Destroy();
	_bitmap = al_bmp;
	_isDataOwner = false;
}

void *CAllegroBitmap::GetBitmapObject()
{
	return _bitmap;
}

bool CAllegroBitmap::IsMemoryBitmap() const
{
	return is_memory_bitmap(_bitmap) != 0;
}

bool CAllegroBitmap::IsLinearBitmap() const
{
	return is_linear_bitmap(_bitmap) != 0;
}

bool CAllegroBitmap::IsNull() const
{
	return !this || !_bitmap;
}

bool CAllegroBitmap::IsEmpty() const
{
	return GetWidth() == 0 || GetHeight() == 0;
}

int CAllegroBitmap::GetWidth() const
{
	return _bitmap->w;
}

int CAllegroBitmap::GetHeight() const
{
	return _bitmap->h;
}

int CAllegroBitmap::GetColorDepth() const
{
	return bitmap_color_depth(_bitmap);
}

int CAllegroBitmap::GetBPP() const
{
	int color_depth = GetColorDepth();
	if (color_depth == 15)
		return 2;

	return color_depth >> 3;
}

int CAllegroBitmap::GetDataSize() const
{
	return GetWidth() * GetHeight() * GetBPP();
}

int CAllegroBitmap::GetLineLength() const
{
	return GetWidth() * GetBPP();
}

const unsigned char *CAllegroBitmap::GetData() const
{
	return _bitmap->line[0];
}

const unsigned char *CAllegroBitmap::GetScanLine(int index) const
{
	if (index < 0 || index >= GetHeight())
	{
		return NULL;
	}

	return _bitmap->line[index];
}

void CAllegroBitmap::SetClip(const CRect &rc)
{
	_bitmap->cl = rc.Left;
	_bitmap->cr = rc.Right;
	_bitmap->ct = rc.Top;
	_bitmap->cb = rc.Bottom;
}

CRect CAllegroBitmap::GetClip() const
{
	return CRect(_bitmap->cl, _bitmap->ct, _bitmap->cr, _bitmap->cb);
}

void CAllegroBitmap::SetMaskColor(color_t color)
{
	// not supported? CHECKME
}

color_t CAllegroBitmap::GetMaskColor() const
{
	return bitmap_mask_color(_bitmap);
}

void CAllegroBitmap::Acquire()
{
	acquire_bitmap(_bitmap);
}

void CAllegroBitmap::Release()
{
	release_bitmap(_bitmap);
}

void CAllegroBitmap::Clear(color_t color)
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

unsigned char *CAllegroBitmap::GetScanLineForWriting(int index)
{
	if (index < 0 || index >= GetHeight())
	{
		return NULL;
	}

	return _bitmap->line[index];
}

void CAllegroBitmap::SetScanLine(int index, unsigned char *data, int data_size)
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

void CAllegroBitmap::Blit(IBitmap *src, int dst_x, int dst_y, BitmapMaskOption mask)
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

void CAllegroBitmap::Blit(IBitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask)
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

void CAllegroBitmap::StretchBlt(IBitmap *src, const CRect &dst_rc, BitmapMaskOption mask)
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

void CAllegroBitmap::StretchBlt(IBitmap *src, const CRect &src_rc, const CRect &dst_rc, BitmapMaskOption mask)
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

void CAllegroBitmap::AAStretchBlt(IBitmap *src, const CRect &dst_rc, BitmapMaskOption mask)
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

void CAllegroBitmap::AAStretchBlt(IBitmap *src, const CRect &src_rc, const CRect &dst_rc, BitmapMaskOption mask)
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

void CAllegroBitmap::TransBlendBlt(IBitmap *src, int dst_x, int dst_y)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	draw_trans_sprite(_bitmap, al_src_bmp, dst_x, dst_y);
}

void CAllegroBitmap::LitBlendBlt(IBitmap *src, int dst_x, int dst_y, int light_amount)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	draw_lit_sprite(_bitmap, al_src_bmp, dst_x, dst_y, light_amount);
}

void CAllegroBitmap::FlipBlt(IBitmap *src, int dst_x, int dst_y, BitmapFlip flip)
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

void CAllegroBitmap::RotateBlt(IBitmap *src, int dst_x, int dst_y, fixed_t angle)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	rotate_sprite(_bitmap, al_src_bmp, dst_x, dst_y, angle);
}

void CAllegroBitmap::RotateBlt(IBitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle)
{
	if (src->GetClassType() != GetClassType())
	{
		return;
	}
	
	BITMAP *al_src_bmp = (BITMAP*)src->GetBitmapObject();
	pivot_sprite(_bitmap, al_src_bmp, dst_x, dst_y, pivot_x, pivot_x, angle);
}

//=============================================================================
// Vector drawing operations
//=============================================================================

void CAllegroBitmap::PutPixel(int x, int y, color_t color)
{
	// CHECK when to use _putpixel* instead
	putpixel(_bitmap, x, y, color);
}

int CAllegroBitmap::GetPixel(int x, int y) const
{
	// CHECK when to use _getpixel* instead
	return getpixel(_bitmap, x, y);
}

void CAllegroBitmap::DrawLine(const CLine &ln, color_t color)
{
	line(_bitmap, ln.X1, ln.Y1, ln.X2, ln.Y2, color);
}

void CAllegroBitmap::DrawTriangle(const CTriangle &tr, color_t color)
{
	triangle(_bitmap,
		tr.X1, tr.Y1, tr.X2, tr.Y2, tr.X3, tr.Y3, color);
}

void CAllegroBitmap::DrawRect(const CRect &rc, color_t color)
{
	rect(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void CAllegroBitmap::FillRect(const CRect &rc, color_t color)
{
	rectfill(_bitmap, rc.Left, rc.Top, rc.Right, rc.Bottom, color);
}

void CAllegroBitmap::FillCircle(const CCircle &circle, color_t color)
{
	circlefill(_bitmap, circle.X, circle.Y, circle.Radius, color);
}

void CAllegroBitmap::FloodFill(int x, int y, color_t color)
{
	floodfill(_bitmap, x, y, color);
}

//=============================================================================
// Creation and destruction
//=============================================================================

bool CAllegroBitmap::Create(int width, int height, int color_depth)
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
bool CAllegroBitmap::CreateShared(IBitmap *src, int x, int y, int width, int height)
{
	Destroy();
	if (src->GetClassType() != GetClassType())
	{
		return false;
	}
	CAllegroBitmap *alSrc = (CAllegroBitmap*)src;
	_bitmap = create_sub_bitmap(alSrc->_bitmap, x, y, width, height);
	_isDataOwner = true;
	return _bitmap != NULL;
}

void CAllegroBitmap::Destroy()
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

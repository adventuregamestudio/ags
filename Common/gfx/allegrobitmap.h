
//=============================================================================
//
// Allegro lib based bitmap
//
// TODO: probably should be moved to the Engine; check again when (if) it is
// clear that AGS.Native does not need allegro for drawing.
//
//=============================================================================
#ifndef __AGS_CN_GFX__ALLEGROBITMAP_H
#define __AGS_CN_GFX__ALLEGROBITMAP_H

#include "gfx/bitmap.h"

struct BITMAP;

namespace AGS
{
namespace Common
{

class AllegroBitmap : public Bitmap
{
public:
	static const int32_t AlBmpSignature = MAKE_SIGNATURE('A','L','B','M');

	static AllegroBitmap *CreateBitmap(int width, int height, int color_depth = 0);
	static AllegroBitmap *CreateSubBitmap(Bitmap *src, const Rect &rc);
	// TODO: revise those functions later (currently needed in a few very specific cases)
	// NOTE: the resulting object __owns__ bitmap data from now on
	static AllegroBitmap *CreateFromRawAllegroBitmap(void *bitmap_object);
	// NOTE: the resulting object __does not own__ bitmap data
	static AllegroBitmap *WrapRawAllegroBitmap(void *bitmap_object);
	static AllegroBitmap *LoadFromFile(const char *filename);
	static bool           SaveToFile(Bitmap *bitmap, const char *filename, const void *palette);

	AllegroBitmap();
	virtual ~AllegroBitmap();

	inline virtual int32_t GetClassType() const
	{
		return AlBmpSignature;
	}

	// TODO: a temporary solution for plugin support
	// AllegroBitmap will _not_ own the raw data
	void			WrapBitmapObject(BITMAP *al_bmp);

	virtual void	*GetBitmapObject();

	virtual bool	IsMemoryBitmap() const;
	virtual bool	IsLinearBitmap() const;

	virtual bool	IsNull() const;
	virtual bool	IsEmpty() const;
	virtual int		GetWidth() const;
	virtual int		GetHeight() const;
	virtual int		GetColorDepth() const;
	virtual int		GetBPP() const;

	virtual int		GetDataSize() const;
	virtual int		GetLineLength() const;

	// TODO: replace with byte *
	virtual const unsigned char *GetData() const;
	virtual const unsigned char *GetScanLine(int index) const;

	virtual void	SetClip(const Rect &rc);
	virtual Rect	GetClip() const;

	virtual void	SetMaskColor(color_t color);
	virtual color_t	GetMaskColor() const;

	virtual void	Acquire();
	virtual void	Release();

	virtual void	Clear(color_t color);

	//=========================================================================
	// Direct access operations
	//=========================================================================
	virtual unsigned char		*GetScanLineForWriting(int index);
	virtual void				SetScanLine(int index, unsigned char *data, int data_size = -1);
	
	//=========================================================================
	// Blitting operations (drawing one bitmap over another)
	//=========================================================================
	// Draw other bitmap over current one
	virtual void	Blit(Bitmap *src, int dst_x, int dst_y, BitmapMaskOption mask = kBitmap_Copy);
	virtual void	Blit(Bitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask = kBitmap_Copy);
	// Copy other bitmap, stretching or shrinking its size to given values
	virtual void	StretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
	virtual void	StretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
	// Antia-aliased stretch-blit
	virtual void	AAStretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
	virtual void	AAStretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
	// TODO: find more general way to call these operations, probably require pointer to Blending data struct?
	// Draw bitmap using translucency preset
	virtual void	TransBlendBlt(Bitmap *src, int dst_x, int dst_y);
	// Draw bitmap using lighting preset
	virtual void	LitBlendBlt(Bitmap *src, int dst_x, int dst_y, int light_amount);
	// TODO: generic "draw transformed" function? What about mask option?
	virtual void	FlipBlt(Bitmap *src, int dst_x, int dst_y, BitmapFlip flip);
	virtual void	RotateBlt(Bitmap *src, int dst_x, int dst_y, fixed_t angle);
	virtual void	RotateBlt(Bitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle);

	//=========================================================================
	// Vector drawing operations
	//=========================================================================
	virtual void	PutPixel(int x, int y, color_t color);
	virtual int		GetPixel(int x, int y) const;
	virtual void	DrawLine(const Line &ln, color_t color);
	virtual void	DrawTriangle(const Triangle &tr, color_t color);
	virtual void	DrawRect(const Rect &rc, color_t color);
	virtual void	FillRect(const Rect &rc, color_t color);
	virtual void	FillCircle(const Circle &circle, color_t color);
	virtual void	FloodFill(int x, int y, color_t color);

private:
	//=========================================================================
	// Creation and destruction
	//=========================================================================
	virtual bool	Create(int width, int height, int color_depth = 0);
	virtual bool	CreateShared(Bitmap *src, int x, int y, int width, int height);
	virtual void	Destroy();

	BITMAP			*_bitmap;
	// TODO: revise this flag, currently needed only for wrapping raw bitmap data in limited cases
	bool			_isDataOwner;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__ALLEGROBITMAP_H

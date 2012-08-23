
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

class CAllegroBitmap : public IBitmap
{
public:
	static const int32_t AlBmpSignature = MAKE_SIGNATURE('A','L','B','M');

	static CAllegroBitmap *CreateBitmap(int width, int height, int color_depth = 0);
	static CAllegroBitmap *CreateSubBitmap(IBitmap *src, int x, int y, int width, int height);

	CAllegroBitmap();
	virtual ~CAllegroBitmap();

	inline virtual int32_t GetClassType() const
	{
		return AlBmpSignature;
	}

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

	virtual void	SetClip(const CRect &rc);
	virtual CRect	GetClip() const;

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
	virtual void	Blit(IBitmap *src, int dst_x, int dst_y, BitmapMaskOption mask = kBitmap_Copy);
	virtual void	Blit(IBitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask = kBitmap_Copy);
	// Copy other bitmap, stretching or shrinking its size to given values
	virtual void	StretchBlt(IBitmap *src, int dst_x, int dst_y, int dst_width, int dst_height, BitmapMaskOption mask = kBitmap_Copy);
	virtual void	StretchBlt(IBitmap *src, int src_x, int src_y, int src_width, int src_height, int dst_x, int dst_y, int dst_width, int dst_height, BitmapMaskOption mask = kBitmap_Copy);
	// Antia-aliased stretch-blit
	virtual void	AAStretchBlt(IBitmap *src, int dst_x, int dst_y, int dst_width, int dst_height, BitmapMaskOption mask = kBitmap_Copy);
	virtual void	AAStretchBlt(IBitmap *src, int src_x, int src_y, int src_width, int src_height, int dst_x, int dst_y, int dst_width, int dst_height, BitmapMaskOption mask = kBitmap_Copy);
	// TODO: find more general way to call these operations, probably require pointer to Blending data struct?
	// Draw bitmap using translucency preset
	virtual void	TransBlendBlt(IBitmap *src, int dst_x, int dst_y);
	// Draw bitmap using lighting preset
	virtual void	LitBlendBlt(IBitmap *src, int dst_x, int dst_y, int light_amount);
	// TODO: generic "draw transformed" function? What about mask option?
	virtual void	FlipBlt(IBitmap *src, int dst_x, int dst_y, BitmapFlip flip);
	virtual void	RotateBlt(IBitmap *src, int dst_x, int dst_y, fixed_t angle);
	virtual void	RotateBlt(IBitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle);

	//=========================================================================
	// Vector drawing operations
	//=========================================================================
	virtual void	PutPixel(int x, int y, color_t color);
	virtual int		GetPixel(int x, int y) const;
	virtual void	DrawLine(const CLine &ln, color_t color);
	virtual void	DrawTriangle(const CTriangle &tr, color_t color);
	virtual void	DrawRect(const CRect &rc, color_t color);
	virtual void	FillRect(const CRect &rc, color_t color);
	virtual void	FillCircle(const CCircle &circle, color_t color);
	virtual void	FloodFill(int x, int y, color_t color);

private:
	//=========================================================================
	// Creation and destruction
	//=========================================================================
	virtual bool	Create(int width, int height, int color_depth = 0);
	virtual bool	CreateShared(IBitmap *src, int x, int y, int width, int height);
	virtual void	Destroy();

	BITMAP			*_bitmap;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__ALLEGROBITMAP_H


//=============================================================================
//
// Base bitmap interface
//
//=============================================================================
#ifndef __AGS_CN_GFX__BITMAP_H
#define __AGS_CN_GFX__BITMAP_H

// Move geometry classes to their related header
struct CLine
{
	int X1;
	int Y1;
	int X2;
	int Y2;

	CLine()
	{
		X1 = 0;
		Y1 = 0;
		X2 = 0;
		Y2 = 0;
	}

	CLine(int x1, int y1, int x2, int y2)
	{
		X1 = x1;
		Y1 = y1;
		X2 = x2;
		Y2 = y2;
	}
};

// Helper factory functions
inline CLine HLine(int x1, int x2, int y)
{
	return CLine(x1, y, x2, y);
}

inline CLine VLine(int x, int y1, int y2)
{
	return CLine(x, y1, x, y2);
}

struct CRect
{
	int Left;
	int Top;
	int Right;
	int Bottom;

	CRect()
	{
		Left	= 0;
		Top		= 0;
		Right	= 0;
		Bottom	= 0;
	}

	CRect(int l, int t, int r, int b)
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
inline CRect RectWH(int x, int y, int width, int height)
{
	return CRect(x, y, x + width - 1, y + height - 1);
}

struct CTriangle
{
	int X1;
	int Y1;
	int X2;
	int Y2;
	int X3;
	int Y3;

	CTriangle()
	{
		X1 = 0;
		Y1 = 0;
		X2 = 0;
		Y2 = 0;
		X3 = 0;
		Y3 = 0;
	}

	CTriangle(int x1, int y1, int x2, int y2, int x3, int y3)
	{
		X1 = x1;
		Y1 = y2;
		X2 = x2;
		Y2 = y2;
		X3 = x3;
		Y3 = y3;
	}
};

struct CCircle
{
	int X;
	int Y;
	int Radius;

	CCircle()
	{
		X = 0;
		Y = 0;
		Radius = 0;
	}

	CCircle(int x, int y, int radius)
	{
		X = x;
		Y = y;
		Radius = radius;
	}

};

// TODO: move elsewere
// Construct a 4-byte integer from four chars
#define MAKE_SIGNATURE(a,b,c,d) (((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((c & 0xFF) << 8) | (d & 0xFF))

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

class IBitmap
{
public:
	virtual ~IBitmap(){}

	// Get implementation signature
	virtual int32_t	GetClassType() const = 0;

	// TODO: This is temporary solution for cases when we cannot replace
	// use of raw BITMAP struct with IBitmap
	virtual void	*GetBitmapObject()		= 0;

	// TODO: also add generic GetBitmapType returning combination of flags
	// Is this a "normal" bitmap created by application which data can be directly accessed for reading and writing
	virtual bool	IsMemoryBitmap() const	= 0;
	// Is this a linear bitmap, the one that can be accessed linearly within each scanline 
	virtual bool	IsLinearBitmap() const	= 0;

	// Checks if bitmap CAN'T be used; positive reply usually means either bitmap was
	// not properly constructed, or that a null pointer is being tested;
	// Note: it is safe to call this method for null pointer
	virtual bool	IsNull() const			= 0;
	// Checks if bitmap has zero size: either width or height (or both) is zero
	virtual bool	IsEmpty() const			= 0;
	virtual int		GetWidth() const		= 0;
	virtual int		GetHeight() const		= 0;
	virtual int		GetColorDepth() const	= 0;
	// BPP: bytes per pixel
	virtual int		GetBPP() const			= 0;

	// CHECKME: probably should not be exposed, see comment to GetData()
	virtual int		GetDataSize() const		= 0;
	// Gets scanline length in bytes (is the same for any scanline)
	virtual int		GetLineLength() const	= 0;

	// TODO: replace with byte *
	// Gets a pointer to underlying graphic data
	// FIXME: actually not a very good idea, since there's no 100% guarantee the scanline positions in memory are sequential
	virtual const unsigned char *GetData() const = 0;
	// Get scanline for direct reading
	virtual const unsigned char *GetScanLine(int index) const = 0;

	virtual void	SetClip(const CRect &rc) = 0;
	virtual CRect	GetClip() const			= 0;

	virtual void	SetMaskColor(color_t color) = 0;
	virtual color_t	GetMaskColor() const	= 0;

	// FIXME: allegro manual states these should not be used externally;
	// should hide or totally remove those later
	virtual void	Acquire()				= 0;
	virtual void	Release()				= 0;

	// Fills the whole bitmap with given color (black by default)
	virtual void	Clear(color_t color = 0)= 0;

	//=========================================================================
	// Direct access operations
	//=========================================================================
	// TODO: think how to increase safety over this (some fixed memory buffer class with iterator?)
	// Gets scanline for directly writing into it
	virtual unsigned char		*GetScanLineForWriting(int index) = 0;
	// Copies buffer contents into scanline
	virtual void				SetScanLine(int index, unsigned char *data, int data_size = -1) = 0;

	//=========================================================================
	// Blitting operations (drawing one bitmap over another)
	//=========================================================================
	// Draw other bitmap over current one
	virtual void	Blit(IBitmap *src, int dst_x, int dst_y, BitmapMaskOption mask = kBitmap_Copy) = 0;
	virtual void	Blit(IBitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask = kBitmap_Copy) = 0;
	// Copy other bitmap, stretching or shrinking its size to given values
	virtual void	StretchBlt(IBitmap *src, const CRect &dst_rc, BitmapMaskOption mask = kBitmap_Copy) = 0;
	virtual void	StretchBlt(IBitmap *src, const CRect &src_rc, const CRect &dst_rc, BitmapMaskOption mask = kBitmap_Copy) = 0;
	// Antia-aliased stretch-blit
	virtual void	AAStretchBlt(IBitmap *src, const CRect &dst_rc, BitmapMaskOption mask = kBitmap_Copy) = 0;
	virtual void	AAStretchBlt(IBitmap *src, const CRect &src_rc, const CRect &dst_rc, BitmapMaskOption mask = kBitmap_Copy) = 0;
	// TODO: find more general way to call these three operations, probably require pointer to Blending data struct?
	// Draw bitmap using translucency preset
	virtual void	TransBlendBlt(IBitmap *src, int dst_x, int dst_y) = 0;
	// Draw bitmap using lighting preset
	virtual void	LitBlendBlt(IBitmap *src, int dst_x, int dst_y, int light_amount) = 0;
	// TODO: generic "draw transformed" function? What about mask option?
	virtual void	FlipBlt(IBitmap *src, int dst_x, int dst_y, BitmapFlip flip) = 0;
	virtual void	RotateBlt(IBitmap *src, int dst_x, int dst_y, fixed_t angle) = 0;
	virtual void	RotateBlt(IBitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle) = 0;

	//=========================================================================
	// Vector drawing operations
	//=========================================================================
	virtual void	PutPixel(int x, int y, color_t color)					= 0;
	virtual int		GetPixel(int x, int y) const							= 0;
	virtual void	DrawLine(const CLine &ln, color_t color)				= 0;
	virtual void	DrawTriangle(const CTriangle &triangle, color_t color)	= 0;
	virtual void	DrawRect(const CRect &rc, color_t color)				= 0;
	virtual void	FillRect(const CRect &rc, color_t color)				= 0;
	virtual void	FillCircle(const CCircle &circle, color_t color)		= 0;
	virtual void	FloodFill(int x, int y, color_t color)					= 0;

protected:
	// Creation and destruction is protected, because normally users should not
	// manually recreate same bitmap object. Nor have an "empty" object without
	// underlying graphic data wandering around the application :).

	// Allocate new bitmap
	// CHECKME: color_depth = 0 is used to call Allegro's create_bitmap, which uses
	// some global color depth setting; not sure if this is OK to use for generic class,
	// revise this in future
	virtual bool	Create(int width, int height, int color_depth = 0) = 0;
	// Allow this object to share existing bitmap data
	virtual bool	CreateShared(IBitmap *src, int x, int y, int width, int height) = 0;
	// Deallocate bitmap
	virtual void	Destroy() = 0;
};

// TODO: revise this construction later
namespace Bitmap
{
	IBitmap *CreateBitmap(int width, int height, int color_depth = 0);
	IBitmap *CreateSubBitmap(IBitmap *src, const CRect &rc);
	// TODO: revise those functions later (currently needed in a few very specific cases)
	// NOTE: the resulting object __owns__ bitmap data from now on
	IBitmap *CreateRawObjectOwner(void *bitmap_object);
	// NOTE: the resulting object __does not own__ bitmap data
	IBitmap *CreateRawObjectWrapper(void *bitmap_object);
	IBitmap *LoadFromFile(const char *filename);
	bool	SaveToFile(IBitmap *bitmap, const char *filename, const void *palette);

	// TODO: revise this later
	// Getters and setters for screen bitmap
	// Unfortunately some of the allegro functions require "screen" allegro bitmap,
	// therefore we must set that pointer to something every time we assign an IBitmap
	// to screen.
	IBitmap	*GetScreenBitmap();
	void	SetScreenBitmap(IBitmap *bitmap);
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__BITMAP_H

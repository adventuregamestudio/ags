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
// Allegro lib based bitmap
//
// TODO: probably should be moved to the Engine; check again when (if) it is
// clear that AGS.Native does not need allegro for drawing.
//
//=============================================================================
#ifndef __AGS_CN_GFX__ALLEGROBITMAP_H
#define __AGS_CN_GFX__ALLEGROBITMAP_H

#include "core/types.h"
#include "gfx/bitmap.h"

struct BITMAP;
#define BITMAP_DEFINED

namespace AGS
{
namespace Common
{

class Graphics;

class Bitmap
{
    friend class Graphics;
public:
	Bitmap();
    Bitmap(int width, int height, int color_depth = 0);
    Bitmap(Bitmap *src, const Rect &rc);
    Bitmap(BITMAP *al_bmp, bool shared_data);
	virtual ~Bitmap();

    // Allocate new bitmap
	// CHECKME: color_depth = 0 is used to call Allegro's create_bitmap, which uses
	// some global color depth setting; not sure if this is OK to use for generic class,
	// revise this in future
	virtual bool	Create(int width, int height, int color_depth = 0);
    virtual bool	CreateTransparent(int width, int height, int color_depth);
    // Allow this object to share existing bitmap data
	virtual bool	CreateSubBitmap(Bitmap *src, const Rect &rc);
	// TODO: a temporary solution for plugin support
	bool			WrapAllegroBitmap(BITMAP *al_bmp, bool shared_data);
    // Deallocate bitmap
    virtual void	Destroy();

    virtual bool    LoadFromFile(const char *filename);
    virtual bool    SaveToFile(const char *filename, const void *palette);

    // TODO: This is temporary solution for cases when we cannot replace
	// use of raw BITMAP struct with Bitmap
	virtual void	*GetAllegroBitmap();

    // TODO: also add generic GetBitmapType returning combination of flags
	// Is this a "normal" bitmap created by application which data can be directly accessed for reading and writing
	virtual bool	IsMemoryBitmap() const;
    // Is this a linear bitmap, the one that can be accessed linearly within each scanline 
	virtual bool	IsLinearBitmap() const;

    // Checks if bitmap CAN'T be used; positive reply usually means either bitmap was
	// not properly constructed, or that a null pointer is being tested;
	// Note: it is safe to call this method for null pointer
	virtual bool	IsNull() const;
    // Checks if bitmap has zero size: either width or height (or both) is zero
	virtual bool	IsEmpty() const;
	virtual int		GetWidth() const;
	virtual int		GetHeight() const;
	virtual int		GetColorDepth() const;
    // BPP: bytes per pixel
	virtual int		GetBPP() const;

    // CHECKME: probably should not be exposed, see comment to GetData()
	virtual int		GetDataSize() const;
    // Gets scanline length in bytes (is the same for any scanline)
	virtual int		GetLineLength() const;

	// TODO: replace with byte *
	// Gets a pointer to underlying graphic data
	// FIXME: actually not a very good idea, since there's no 100% guarantee the scanline positions in memory are sequential
	virtual const unsigned char *GetData() const;
    // Get scanline for direct reading
	virtual const unsigned char *GetScanLine(int index) const;

	virtual void	SetMaskColor(color_t color);
	virtual color_t	GetMaskColor() const;

    // FIXME: allegro manual states these should not be used externally;
	// should hide or totally remove those later
	virtual void	Acquire();
	virtual void	Release();

    virtual color_t GetCompatibleColor(color_t color);

    //=========================================================================
    // Pixel operations
    //=========================================================================
    // Fills the whole bitmap with given color (black by default)
	virtual void	Clear(color_t color = 0);
    virtual void    ClearTransparent();
    // The PutPixel and GetPixel are supposed to be safe and therefore
    // relatively slow operations. They should not be used for changing large
    // blocks of bitmap memory - reading/writing from/to scan lines should be
    // done in such cases.
    virtual void	PutPixel(int x, int y, color_t color);
    virtual int		GetPixel(int x, int y) const;

	//=========================================================================
	// Direct access operations
	//=========================================================================
    // TODO: think how to increase safety over this (some fixed memory buffer class with iterator?)
	// Gets scanline for directly writing into it
	virtual unsigned char		*GetScanLineForWriting(int index);
    // Copies buffer contents into scanline
	virtual void				SetScanLine(int index, unsigned char *data, int data_size = -1);

private:
	BITMAP			*_alBitmap;
	// TODO: revise this flag, currently needed only for wrapping raw bitmap data in limited cases
	bool			_isDataOwner;
    Graphics        *_graphics;
};



namespace BitmapHelper
{
    // TODO: revise those functions later (currently needed in a few very specific cases)
	// NOTE: the resulting object __owns__ bitmap data from now on
	Bitmap *CreateRawBitmapOwner(BITMAP *al_bmp);
	// NOTE: the resulting object __does not own__ bitmap data
	Bitmap *CreateRawBitmapWrapper(BITMAP *al_bmp);
} // namespace BitmapHelper

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__ALLEGROBITMAP_H

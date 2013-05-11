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

#include <allegro.h>
#include "core/types.h"
#include "gfx/bitmap.h"

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
    ~Bitmap();

    // Allocate new bitmap
    // CHECKME: color_depth = 0 is used to call Allegro's create_bitmap, which uses
    // some global color depth setting; not sure if this is OK to use for generic class,
    // revise this in future
    bool    Create(int width, int height, int color_depth = 0);
    bool    CreateTransparent(int width, int height, int color_depth);
    // Allow this object to share existing bitmap data
    bool    CreateSubBitmap(Bitmap *src, const Rect &rc);
    // Create a copy of given bitmap
    bool	CreateCopy(Bitmap *src, int color_depth = 0);
    // Creates a reference of given bitmap; this will increase reference count and
    // make both bitmap objects share same data
    bool    CreateReference(Bitmap *src);
    // TODO: a temporary solution for plugin support
    bool    WrapAllegroBitmap(BITMAP *al_bmp, bool shared_data);
    // Deallocate bitmap
    void	Destroy();

    bool    LoadFromFile(const char *filename);
    bool    SaveToFile(const char *filename, const void *palette);

    // TODO: This is temporary solution for cases when we cannot replace
	// use of raw BITMAP struct with Bitmap
    inline BITMAP *GetAllegroBitmap()
    {
        return _alBitmap;
    }

    // TODO: also add generic GetBitmapType returning combination of flags
	// Is this a "normal" bitmap created by application which data can be directly accessed for reading and writing
    inline bool IsMemoryBitmap() const
    {
        return is_memory_bitmap(_alBitmap) != 0;
    }
    // Is this a linear bitmap, the one that can be accessed linearly within each scanline 
	inline bool IsLinearBitmap() const
    {
        return is_linear_bitmap(_alBitmap) != 0;
    }

    // Checks if bitmap CAN'T be used; positive reply usually means either bitmap was
	// not properly constructed, or that a null pointer is being tested;
	// Note: it is safe to call this method for null pointer
    inline bool IsNull() const
    {
        return !this || !_alBitmap;
    }
    // Checks if bitmap has zero size: either width or height (or both) is zero
    inline bool IsEmpty() const
    {
        return GetWidth() == 0 || GetHeight() == 0;
    }
    inline int  GetWidth() const
    {
        return _alBitmap->w;
    }
    inline int  GetHeight() const
    {
        return _alBitmap->h;
    }
    inline int  GetColorDepth() const
    {
        return bitmap_color_depth(_alBitmap);
    }
    // BPP: bytes per pixel
    inline int  GetBPP() const
    {
        int color_depth = GetColorDepth();
        return color_depth == 15 ? 2 : (color_depth >> 3);
    }

    // CHECKME: probably should not be exposed, see comment to GetData()
	inline int  GetDataSize() const
    {
        return GetWidth() * GetHeight() * GetBPP();
    }
    // Gets scanline length in bytes (is the same for any scanline)
	inline int  GetLineLength() const
    {
        return GetWidth() * GetBPP();
    }

	// TODO: replace with byte *
	// Gets a pointer to underlying graphic data
	// FIXME: actually not a very good idea, since there's no 100% guarantee the scanline positions in memory are sequential
    inline const unsigned char *GetData() const
    {
        return _alBitmap->line[0];
    }

    // Get scanline for direct reading
	inline const unsigned char *GetScanLine(int index) const
    {
        return (index >= 0 && index < GetHeight()) ? _alBitmap->line[index] : NULL;
    }

    void    SetMaskColor(color_t color);
	inline color_t GetMaskColor() const
    {
        return bitmap_mask_color(_alBitmap);
    }

    // FIXME: allegro manual states these should not be used externally;
	// should hide or totally remove those later
    void    Acquire();
	void	Release();

    color_t GetCompatibleColor(color_t color);

    //=========================================================================
    // Pixel operations
    //=========================================================================
    // Fills the whole bitmap with given color (black by default)
    void	Clear(color_t color = 0);
    void    ClearTransparent();
    // The PutPixel and GetPixel are supposed to be safe and therefore
    // relatively slow operations. They should not be used for changing large
    // blocks of bitmap memory - reading/writing from/to scan lines should be
    // done in such cases.
    void	PutPixel(int x, int y, color_t color);
    int		GetPixel(int x, int y) const;

	//=========================================================================
	// Direct access operations
	//=========================================================================
    // TODO: think how to increase safety over this (some fixed memory buffer class with iterator?)
	// Gets scanline for directly writing into it
    inline unsigned char *GetScanLineForWriting(int index)
    {
        return (index >= 0 && index < GetHeight()) ? _alBitmap->line[index] : NULL;
    }
    // Copies buffer contents into scanline
    void    SetScanLine(int index, unsigned char *data, int data_size = -1);

    inline bool operator==(const Bitmap &bmp)
    {
        return _alBitmap == bmp._alBitmap;
    }
    inline bool operator!=(const Bitmap &bmp)
    {
        return _alBitmap != bmp._alBitmap;
    }

private:
	BITMAP			*_alBitmap;
	int             *_refCount;
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

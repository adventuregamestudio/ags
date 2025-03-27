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
//
// Allegro lib based bitmap
//
// TODO: probably should be moved to the Engine; check again when (if) it is
// clear that AGS.Native does not need allegro for drawing.
//
//=============================================================================
#ifndef __AGS_CN_GFX__ALLEGROBITMAP_H
#define __AGS_CN_GFX__ALLEGROBITMAP_H

#include <memory>
#include <allegro.h> // BITMAP
#include "core/types.h"
#include "gfx/bitmap.h"
#include "gfx/bitmapdata.h"
#include "util/string.h"

struct SDL_Surface;

namespace AGS
{
namespace Common
{

class Bitmap
{
public:
    Bitmap() = default;
    Bitmap(int width, int height, int color_depth = 0);
    Bitmap(PixelBuffer &&pxbuf);
    // Constructs a sub-bitmap, referencing parent
    Bitmap(const Bitmap *src, const Rect &rc);
    // Wraps Allegro BITMAP object, optionally owning it
    Bitmap(BITMAP *al_bmp, bool shared_data);
    // Wraps SDL_Surface object, optionally owning it
    Bitmap(SDL_Surface *sdl_bmp, bool shared_data);
    // Copy-constructor: constructs a full Bitmap copy
    Bitmap(const Bitmap &bmp);
    // Move-constructor: moves pixel data from another bitmap
    Bitmap(Bitmap &&bmp);
    ~Bitmap();

    Bitmap &operator =(const Bitmap &bmp);
    Bitmap &operator =(Bitmap &&bmp) = default;

    // Allocate new bitmap.
    // NOTE: color_depth is in BITS per pixel (i.e. 8, 16, 24, 32...).
    // NOTE: in all of these color_depth may be passed as 0 in which case a default
    // color depth will be used (as previously set for the system).
    // TODO: color_depth = 0 is used to call Allegro's create_bitmap, which uses
    // some global color depth setting; not sure if this is OK to use for generic class,
    // revise this in future
    bool    Create(int width, int height, int color_depth = 0);
    // Create Bitmap and clear to transparent color
    bool    CreateTransparent(int width, int height, int color_depth = 0);
    // Create Bitmap and attach prepared pixel buffer
    bool    Create(PixelBuffer &&pxbuf);
    // Creates a sub-bitmap of the given bitmap; the sub-bitmap is a reference to
    // particular region inside a parent.
    // WARNING: the parent bitmap MUST be kept in memory for as long as sub-bitmap exists!
    bool    CreateSubBitmap(const Bitmap *src, const Rect &rc);
    // Resizes existing sub-bitmap within the borders of its parent
    bool    ResizeSubBitmap(int width, int height);
    // Creates a plain copy of the given bitmap, optionally converting to a different color depth;
    // pass color depth 0 to keep the original one.
    bool	CreateCopy(const Bitmap *src, int color_depth = 0);
    // TODO: this is a temporary solution for plugin support
    // Wraps a raw allegro BITMAP object, optionally owns it (will delete on disposal)
    bool    WrapAllegroBitmap(BITMAP *al_bmp, bool shared_data);
    // Releases a reference to raw allegro BITMAP object without deleting it;
    // WARNING: this is meant strictly as a workaround in case third-party lib
    // have deleted our owned BITMAP object.
    void    ForgetAllegroBitmap();
    // Wraps a SDL_Surface object, optionally owns it (will delete on disposal)
    bool    WrapSDLSurface(SDL_Surface *sdl_bmp, bool shared_data);
    // Deallocate bitmap
    void	Destroy();

    bool    SaveToFile(const String &filename, const RGB *palette)
            { return SaveToFile(filename.GetCStr(), palette); }
    bool    SaveToFile(const char *filename, const RGB *palette);

    // TODO: This is temporary solution for cases when we cannot replace
	// use of raw BITMAP struct with Bitmap
    inline BITMAP *GetAllegroBitmap()
    {
        return _alBitmap;
    }

    inline const BITMAP *GetAllegroBitmap() const
    {
        return _alBitmap;
    }

    // Is this a subbitmap, referencing a part of another, bigger one?
    inline bool IsSubBitmap() const
    {
        return is_sub_bitmap(_alBitmap) != 0;
    }
    // Do both bitmaps share same data (usually: subbitmaps, or parent/subbitmap)
    inline bool IsSameBitmap(Bitmap *other) const
    {
        return is_same_bitmap(_alBitmap, other->_alBitmap) != 0;
    }
    // Checks if bitmap cannot be used
    inline bool IsNull() const
    {
        return !_alBitmap;
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
    inline Size GetSize() const
    {
        return Size(_alBitmap->w, _alBitmap->h);
    }
    // Get sub-bitmap's offset position within its parent
    inline Point GetSubOffset() const
    {
        return Point(_alBitmap->x_ofs, _alBitmap->y_ofs);
    }
    inline int  GetColorDepth() const
    {
        return bitmap_color_depth(_alBitmap);
    }
    // BPP: bytes per pixel
    inline int  GetBPP() const
    {
        return (GetColorDepth() + 7) / 8;
    }

    // Gets size of Bitmap's pixel data, in bytes
	inline int  GetDataSize() const
    {
        return GetWidth() * GetHeight() * GetBPP();
    }
    // Gets scanline length in bytes (is the same for any scanline)
	inline int  GetLineLength() const
    {
        return _pitch;
    }

	// Gets a pointer to underlying graphic data
    inline const unsigned char *GetData() const
    {
        return _alBitmap->line[0];
    }

    // Get scanline for direct reading
	inline const unsigned char *GetScanLine(int index) const
    {
        assert(index >= 0 && index < GetHeight());
        return _alBitmap->line[index];
    }

    // Get bitmap data wrapped in a PixelBuffer struct
    inline const BitmapData GetBitmapData() const
    {
        return BitmapData(GetData(), GetDataSize(), GetLineLength(), GetWidth(), GetHeight(), ColorDepthToPixelFormat(GetColorDepth()));
    }

    inline BitmapData GetBitmapData()
    {
        return BitmapData(GetDataForWriting(), GetDataSize(), GetLineLength(), GetWidth(), GetHeight(), ColorDepthToPixelFormat(GetColorDepth()));
    }

	// Get bitmap's mask color (transparent color)
	inline color_t GetMaskColor() const
    {
        return bitmap_mask_color(_alBitmap);
    }

    // Converts AGS color-index into RGB color compatible with the bitmap format.
    color_t GetCompatibleColor(int color);

    // Tells if the given point lies within the bitmap bounds
    inline bool IsOnBitmap(int x, int y) const
    {
        return x >= 0 && y >= 0 && x < GetWidth() && y < GetHeight();
    }

    //=========================================================================
    // Clipping
    // TODO: consider implementing push-pop clipping stack logic.
    //=========================================================================
    void    SetClip(const Rect &rc);
    void    ResetClip();
    Rect    GetClip() const;

    //=========================================================================
    // Blitting operations (drawing one bitmap over another)
    //=========================================================================
    // Draw other bitmap over current one
    void    Blit(const Bitmap *src, int dst_x = 0, int dst_y = 0, BitmapMaskOption mask = kBitmap_Copy);
    void    Blit(const Bitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask = kBitmap_Copy);
    // Draw other bitmap in a masked mode (kBitmap_Transparency)
    void    MaskedBlit(const Bitmap *src, int dst_x = 0, int dst_y = 0);
    // Draw other bitmap, stretching or shrinking its size to given values
    void    StretchBlt(const Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    void    StretchBlt(const Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    // Antia-aliased stretch-blit
    void    AAStretchBlt(const Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    void    AAStretchBlt(const Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    // TODO: find more general way to call these operations, probably require pointer to Blending data struct?
    // Draw bitmap using translucency preset
    void    TransBlendBlt(const Bitmap *src, int dst_x = 0, int dst_y = 0);
    // Draw bitmap using lighting preset
    void    LitBlendBlt(const Bitmap *src, int dst_x, int dst_y, int light_amount);
    // TODO: generic "draw transformed" function? What about mask option?
    // TODO: "flip self" function
    void    FlipBlt(const Bitmap *src, int dst_x, int dst_y, GraphicFlip flip);
    // Draws rotated bitmap, using angle given in degrees.
    // Warning: does not resize destination bitmap; if it's not large enough
    // then the resulting image may end up cropped.
    void    RotateBlt(const Bitmap *src, int dst_x, int dst_y, int angle);
    void    RotateBlt(const Bitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, int angle);

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
    void    BlendPixel(int x, int y, color_t color);
    int		GetPixel(int x, int y) const;

    //=========================================================================
    // Vector drawing operations
    //=========================================================================
    void    DrawLine(const Line &ln, color_t color);
    void    DrawTriangle(const Triangle &tr, color_t color);
    void    DrawRect(const Rect &rc, color_t color);
    void    FillRect(const Rect &rc, color_t color);
    void    FillCircle(const Circle &circle, color_t color);
    // Fills the whole bitmap with given color
    void    Fill(color_t color);
    void    FillTransparent();
    // Floodfills an enclosed area, starting at point
    void    FloodFill(int x, int y, color_t color);

	//=========================================================================
	// Direct access operations
	//=========================================================================
    // TODO: think how to increase safety over this (some fixed memory buffer class with iterator?)
	// Gets scanline for directly writing into it
    inline unsigned char *GetScanLineForWriting(int index)
    {
        assert(index >= 0 && index < GetHeight());
        return _alBitmap->line[index];
    }
    inline unsigned char *GetDataForWriting()
    {
        return _alBitmap->line[0];
    }
    // Copies buffer contents into scanline
    void    SetScanLine(int index, unsigned char *data, int data_size = -1);

private:
    std::unique_ptr<uint8_t[]> _pixelData;
    SDL_Surface *_sdlBitmap = nullptr;
    BITMAP *_alBitmap = nullptr;
    bool    _isDataOwner = false;
    int     _pitch = 0; // cached pitch (scanline length)
};



namespace BitmapHelper
{
    // Remaps AGS color number to a color value compatible to a bitmap of certain color depth
    int AGSColorToBitmapColor(int color, int color_depth);
    // Remaps AGS color number in certain color depth mode to a actual RGB
    void AGSColorToRGB(int color, int color_depth, RGB &rgb);
    // TODO: revise those functions later (currently needed in a few very specific cases)
	// NOTE: the resulting object __owns__ bitmap data from now on
	Bitmap *CreateRawBitmapOwner(BITMAP *al_bmp);
	// NOTE: the resulting object __does not own__ bitmap data
	Bitmap *CreateRawBitmapWrapper(BITMAP *al_bmp);
} // namespace BitmapHelper

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__ALLEGROBITMAP_H

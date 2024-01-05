//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Base bitmap header
//
//=============================================================================
#ifndef __AGS_CN_GFX__BITMAP_H
#define __AGS_CN_GFX__BITMAP_H

#include "gfx/gfx_def.h"
#include "util/geometry.h"

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

} // namespace Common
} // namespace AGS


// Declare the actual bitmap class
#include "gfx/allegrobitmap.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class Bitmap;

// TODO: revise this construction later
namespace BitmapHelper
{
    // Helper functions, that delete faulty bitmaps automatically, and return
    // NULL if bitmap could not be created.
    // NOTE: color_depth is in BITS per pixel (i.e. 8, 16, 24, 32...).
    // NOTE: in all of these color_depth may be passed as 0 in which case a default
    // color depth will be used (as previously set for the system).
    // Creates a new bitmap of the given format; the pixel contents are undefined.
    Bitmap *CreateBitmap(int width, int height, int color_depth = 0);
    // Creates a new bitmap and clears it with the given color
    Bitmap *CreateClearBitmap(int width, int height, int color_depth = 0, int clear_color = 0);
    // Creates a new bitmap and clears it with the transparent color
    Bitmap *CreateTransparentBitmap(int width, int height, int color_depth = 0);
    // Creates a sub-bitmap of the given bitmap; the sub-bitmap is a reference to
    // particular region inside a parent.
    // WARNING: the parent bitmap MUST be kept in memory for as long as sub-bitmap exists!
	Bitmap *CreateSubBitmap(Bitmap *src, const Rect &rc);
    // Creates a plain copy of the given bitmap, optionally converting to a different color depth;
    // pass color depth 0 to keep the original one.
    Bitmap *CreateBitmapCopy(Bitmap *src, int color_depth = 0);

    // Load a bitmap from file; supported formats currently are: BMP, PCX.
	Bitmap *LoadFromFile(const char *filename);
    inline Bitmap *LoadFromFile(const String &filename) { return LoadFromFile(filename.GetCStr()); }
    // Write a bitmap into a file; supported formats currently are: BMP, PCX.
    bool SaveToFile(Bitmap* bmp, const char *filename, const RGB *pal = nullptr);

    // Stretches bitmap to the requested size. The new bitmap will have same
    // colour depth. Returns original bitmap if no changes are necessary. 
    Bitmap *AdjustBitmapSize(Bitmap *src, int width, int height);
    // Makes the given bitmap opaque (full alpha), while keeping pixel RGB unchanged.
    void    MakeOpaque(Bitmap *bmp);
    // Makes the given bitmap opaque (full alpha), while keeping pixel RGB unchanged.
    // Skips mask color (leaves it with zero alpha).
    void    MakeOpaqueSkipMask(Bitmap *bmp);
    // Replaces fully transparent (alpha = 0) pixels with standard mask color.
    void    ReplaceAlphaWithRGBMask(Bitmap *bmp);
    // Copy transparency mask and/or alpha channel from one bitmap into another.
    // Destination and mask bitmaps must be of the same pixel format.
    // Transparency is merged, meaning that fully transparent pixels on
    // destination should remain such regardless of mask pixel values.
    void    CopyTransparency(Bitmap *dst, const Bitmap *mask, bool dst_has_alpha, bool mask_has_alpha);
    // Copy pixel data into bitmap from memory buffer. It is required that the
    // source matches bitmap format and has enough data.
    // Pitch is given in bytes and defines the length of the source scan line.
    // Offset is optional and defines horizontal offset, in pixels.
    void    ReadPixelsFromMemory(Bitmap *dst, const uint8_t *src_buffer, const size_t src_pitch, const size_t src_px_offset = 0);

    // Write to the stream a bitmap, optionally along with the palette
    void SaveBitmap(const String& ext, Stream *out, const Bitmap *bmp, const RGB* pal = nullptr);
    // Reads from the stream a bitmap, possibly with palette
    Bitmap* LoadBitmap(const String& ext, Stream *in, RGB *pal = nullptr);

} // namespace BitmapHelper

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__BITMAP_H

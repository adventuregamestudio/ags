//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Various compression utilities for pixel data.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__COMPRESS_H
#define __AGS_CN_UTIL__COMPRESS_H

#include <memory>
#include <vector>
#include "core/types.h"
#include "gfx/bitmapdata.h"
#include "util/stream.h"

struct RGB;

namespace AGS
{
namespace Common
{

// RLE compression
bool rle_compress(const uint8_t *data, size_t data_sz, int image_bpp, Stream *out);
bool rle_decompress(uint8_t *data, size_t data_sz, int image_bpp, Stream *in);
// Packs a 8-bit bitmap using RLE compression, and writes into stream along with the palette
void save_rle_bitmap8(Stream *out, const BitmapData &bmdata, const RGB (*pal)[256] = nullptr);
// Reads a 8-bit bitmap with palette from the stream and unpacks from RLE
PixelBuffer load_rle_bitmap8(Stream *in, RGB (*pal)[256] = nullptr);
// Skips the 8-bit RLE bitmap
void skip_rle_bitmap8(Stream *in);

// LZW compression
bool lzw_compress(const uint8_t *data, size_t data_sz, int image_bpp, Stream *out);
bool lzw_decompress(uint8_t *data, size_t data_sz, int image_bpp, Stream *in, size_t in_sz);
// Saves bitmap with an optional palette compressed by LZW
void save_lzw(Stream *out, const BitmapData &bmdata, const RGB (*pal)[256] = nullptr);
// Loads bitmap decompressing
PixelBuffer load_lzw(Stream *in, int dst_bpp, RGB (*pal)[256] = nullptr);

// Deflate compression
bool deflate_compress(const uint8_t* data, size_t data_sz, int image_bpp, Stream* out);
bool inflate_decompress(uint8_t* data, size_t data_sz, int image_bpp, Stream* in, size_t in_sz);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__COMPRESS_H

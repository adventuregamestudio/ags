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
#ifndef __AC_COMPRESS_H
#define __AC_COMPRESS_H

#include "core/types.h"
#include <memory>
#include <vector>

struct RGB;

namespace AGS { namespace Common { class Stream; class Bitmap; } }
using namespace AGS; // FIXME later

// RLE compression
bool rle_compress(const uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *out);
bool rle_decompress(uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *in);
// Packs a 8-bit bitmap using RLE compression, and writes into stream along with the palette
void save_rle_bitmap8(Common::Stream *out, const Common::Bitmap *bmp, const RGB (*pal)[256] = nullptr);
// Reads a 8-bit bitmap with palette from the stream and unpacks from RLE
std::unique_ptr<Common::Bitmap> load_rle_bitmap8(Common::Stream *in, RGB (*pal)[256] = nullptr);
// Skips the 8-bit RLE bitmap
void skip_rle_bitmap8(Common::Stream *in);

// LZW compression
bool lzw_compress(const uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *out);
bool lzw_decompress(uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *in, size_t in_sz);
// Saves bitmap with an optional palette compressed by LZW
void save_lzw(Common::Stream *out, const Common::Bitmap *bmpp, const RGB (*pal)[256] = nullptr);
// Loads bitmap decompressing
std::unique_ptr<Common::Bitmap> load_lzw(Common::Stream *in, int dst_bpp, RGB (*pal)[256] = nullptr);

// Deflate compression
bool deflate_compress(const uint8_t* data, size_t data_sz, int image_bpp, Common::Stream* out);
bool inflate_decompress(uint8_t* data, size_t data_sz, int image_bpp, Common::Stream* in, size_t in_sz);

#endif // __AC_COMPRESS_H

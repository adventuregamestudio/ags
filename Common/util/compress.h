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
#ifndef __AC_COMPRESS_H
#define __AC_COMPRESS_H

struct RGB;

namespace AGS { namespace Common { class Stream; class Bitmap; } }
using namespace AGS; // FIXME later

// RLE compression
void rle_compress(const uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *out);
void rle_decompress(uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *in);
// Packs a 8-bit bitmap using RLE compression, and writes into stream along with the palette
void save_rle_bitmap8(Common::Stream *out, const Common::Bitmap *bmp, const RGB (*pal)[256] = nullptr);
// Reads a 8-bit bitmap with palette from the stream and unpacks from RLE
Common::Bitmap *load_rle_bitmap8(Common::Stream *in, RGB (*pal)[256] = nullptr);

// LZW compression
void save_lzw(Common::Stream *out, const Common::Bitmap *bmpp, const RGB *pall);
void load_lzw(Common::Stream *in, Common::Bitmap **bmm, int dst_bpp, RGB *pall);

#endif // __AC_COMPRESS_H

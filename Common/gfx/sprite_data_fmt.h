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
// Sprite serialization format and functions for reading/writing one in file.
// Primary use is a spritefile, which is a multi-sprite collection, but we
// may save a image in a sprite format elsewhere, e.g. room backgrounds.
// The benefit of having such shared format is that it provides extra options
// for image compression.
// 
//=============================================================================
#ifndef __AGS_CN_GFX__SPRITE_DATA_FMT_H
#define __AGS_CN_GFX__SPRITE_DATA_FMT_H
#include <allegro.h> // PALETTE
#include "platform/types.h"
#include "gfx/bitmapdata.h"
#include "util/error.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

// Instructions to how the sprites are allowed to be stored
enum SpriteStorage
{
    kSprStore_None            = 0x00,
    // When possible convert the sprite into another format for less disk space
    // e.g. save 16/32-bit images as 8-bit colormaps with palette
    kSprStore_OptimizeForSize = 0x01
};

// Format in which the sprite's pixel data is stored.
// This is not a pixel format, but rather a special storage format.
// "Undefined" value in this case means that no special storage is
// used, and that the actual pixel format is determined by other means.
enum SpriteFormat
{
    kSprFmt_Undefined       = 0, // undefined
    // Encoded as a 8-bit colormap with palette of 24-bit RGB values
    kSprFmt_PaletteRgb888   = 32,
    // Encoded as a 8-bit colormap with palette of 32-bit ARGB values
    kSprFmt_PaletteArgb8888 = 33,
    // Encoded as a 8-bit colormap with palette of 16-bit RGB565 values
    kSprFmt_PaletteRgb565   = 34
};

enum SpriteCompression
{
    kSprCompress_None = 0,
    kSprCompress_RLE,
    kSprCompress_LZW,
    kSprCompress_Deflate,
    kNumSprCompressTypes
};

// Invidual sprite data header (as read from the file)
struct SpriteDatHeader
{
    int BPP = 0; // color depth (bytes per pixel); or input format
    SpriteFormat SFormat = kSprFmt_Undefined; // storage format
    uint32_t PalCount = 0; // palette length, if applicable to storage format
    SpriteCompression Compress = kSprCompress_None; // compression type
    int Width = 0; // sprite's width
    int Height = 0; // sprite's height

    SpriteDatHeader() = default;
    SpriteDatHeader(int bpp, SpriteFormat sformat = kSprFmt_Undefined,
        uint32_t pal_count = 0, SpriteCompression compress = kSprCompress_None,
        int w = 0, int h = 0) : BPP(bpp), SFormat(sformat), PalCount(pal_count),
        Compress(compress), Width(w), Height(h) {}
};

namespace SpriteDataUtils
{
    // Read sprite from the stream; optionally fills sprite's palette (if available)
    PixelBuffer ReadSprite_360(Stream *in, HError &err, PALETTE *out_palette = nullptr);
    // Reads sprite in pre-3.6.0 format, corresponding to engine versions 2.x - 3.6.0;
    // optionally fills sprite's palette (if available)
    // In this old format the sprite data does not have info about compression used,
    // so it must be provided.
    PixelBuffer ReadSprite_321(Stream *in, SpriteCompression compress, HError &err, PALETTE *out_palette = nullptr);

    // Reads a sprite header, introduced in 3.6.0
    void ReadSpriteHeader_360(SpriteDatHeader &hdr, Stream *in);
    // Reads a sprite header in pre-3.6.0 format, corresponding to engine versions 2.x - 3.6.0.
    void ReadSpriteHeader_321(SpriteDatHeader &hdr, Stream *in, SpriteCompression compress);

    // Read sprite data, using previously read header as a hint;
    // optionally fills sprite's palette (if available)
    PixelBuffer ReadSpriteData_360(const SpriteDatHeader &hdr, Stream *in, HError &err, PALETTE *out_palette = nullptr);
    // Read sprite data in pre-3.6.0 format, corresponding to engine versions 2.x - 3.6.0;
    // optionally fills sprite's palette (if available)
    PixelBuffer ReadSpriteData_321(const SpriteDatHeader &hdr, Stream *in, SpriteCompression compress, HError &err, PALETTE *out_palette = nullptr);

    // Calculates the size of the sprite data ahead, using previously read header as a hint
    size_t GetSpriteDataSize_360(const SpriteDatHeader &hdr, Stream *in);
    // Calculates the size of the sprite data in pre-3.6.0 format, using previously read header as a hint
    size_t GetSpriteDataSize_321(const SpriteDatHeader &hdr, Stream *in, SpriteCompression compress);

    // Skip sprite data, using previously read header as a hint
    void SkipSpriteData_360(const SpriteDatHeader &hdr, Stream *in);
    // Skip sprite data in pre-3.6.0 format, using previously read header as a hint
    void SkipSpriteData_321(const SpriteDatHeader &hdr, Stream *in, SpriteCompression compress);

    // Write sprite data, using provided format options; optionally provides palette for a 8-bit image;
    // optionally lets provide a memory buffer for compression (if not provided then will allocate one each call)
    void WriteSprite_360(const BitmapData &image, Stream *out, int store_flags, SpriteCompression compress,
        const PALETTE *palette = nullptr, std::vector<uint8_t> *mem_buf = nullptr);
    // Write a prepared sprite data without any additional processing
    void WriteRawSpriteData_360(const SpriteDatHeader &hdr, Stream *out, const uint8_t *data, size_t data_sz);
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__SPRITE_DATA_FMT_H

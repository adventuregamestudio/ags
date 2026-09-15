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
#include "gfx/sprite_data_fmt.h"
#include <array>
#include <memory>
#include <vector>
#include "debug/assert.h"
#include "util/compress.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

namespace AGS
{
namespace Common
{

namespace SpriteDataUtils
{

// Image buffer pointer, a helper struct that eases switching
// between intermediate buffers when loading, saving or converting an image.
// TODO: This is an older struct, may this be replaced by using more generic BitmapData here?
template <typename T> struct ImBufferPtrT
{
    T        Buf = nullptr;
    size_t   Size = 0;
    int      BPP = 0; // byte per pixel

    ImBufferPtrT() = default;
    ImBufferPtrT(T buf, size_t sz, int bpp) : Buf(buf), Size(sz), BPP(bpp) {}
};
typedef ImBufferPtrT<uint8_t*> ImBufferPtr;
typedef ImBufferPtrT<const uint8_t*> ImBufferCPtr;

// Finds the given color's index in the palette, or returns SIZE_MAX if such color is not there
static size_t lookup_palette(uint32_t col, uint32_t palette[256], uint32_t ncols)
{
    for (size_t i = 0; i < ncols; ++i)
        if (palette[i] == col) return i;
    return SIZE_MAX;
}

// Converts a 16/32-bit image into the indexed 8-bit pixel data with palette;
// NOTE: the palette will contain colors in the same format as the source image.
// only succeeds if the total number of colors used in the image is < 257.
static bool CreateIndexedBitmap(const BitmapData &image, std::vector<uint8_t> &dst_data,
    uint32_t palette[256], uint32_t &pal_count)
{
    const int src_bpp = image.GetBytesPerPixel();
    if (src_bpp < 2) { assert(0); return false; }
    const size_t src_size = image.GetWidth() * image.GetHeight() * image.GetBytesPerPixel();
    const size_t dst_size = image.GetWidth() * image.GetHeight();
    dst_data.resize(dst_size);
    const uint8_t *src = image.GetData(), *src_end = src + src_size;
    uint8_t *dst = dst_data.data(), *dst_end = dst + dst_size;
    pal_count = 0;

    for (; src < src_end && dst < dst_end; src += src_bpp)
    {
        uint32_t col;
        size_t pal_n;
        switch (src_bpp)
        {
        case 2:
            col = *((const uint16_t*)src);
            break;
        case 3:
            col = src[0] | (src[1] << 8) | (src[2] << 16);
            break;
        case 4:
            col = *((const uint32_t*)src);
            break;
        default: assert(0); return false;
        }

        pal_n = lookup_palette(col, palette, pal_count);
        if (pal_n == SIZE_MAX)
        {
            if (pal_count == 256) return false;
            pal_n = pal_count;
            palette[pal_count++] = col;
        }
        *(dst++) = (uint8_t)pal_n;
    }
    return true;
}

// Unpacks an indexed image's pixel data into the 16/32-bit image;
// NOTE: the palette is expected to contain colors in the same format as the destination.
static void UnpackIndexedBitmap(PixelBuffer &image, const uint8_t *data, size_t data_size,
    const std::array<uint32_t, 256> &palette, uint32_t pal_count)
{
    assert(pal_count > 0 && pal_count <= 256);
    if (pal_count == 0 || pal_count > 256) return; // meaningless

    const uint8_t bpp = static_cast<uint8_t>(image.GetBytesPerPixel());
    const size_t dst_size = image.GetWidth() * image.GetHeight() * image.GetBytesPerPixel();
    uint8_t *dst = image.GetData();
    uint8_t const *dst_end = dst + dst_size;

    switch (bpp)
    {
    case 2:
        for (size_t p = 0; (p < data_size) && (dst < dst_end); ++p, dst += bpp) {
            uint8_t index = data[p];
            assert(index < pal_count);
            uint32_t color = palette[index];
            *((uint16_t *) dst) = color;
        }
        break;
    case 3:
        for (size_t p = 0; (p < data_size) && (dst < dst_end); ++p, dst += bpp) {
            uint8_t index = data[p];
            assert(index < pal_count);
            uint32_t color = palette[index];
            dst[0] = color & 0xFF;
            dst[1] = (color >> 8) & 0xFF;
            dst[2] = (color >> 16) & 0xFF;
        }
        break;
    case 4:
        for (size_t p = 0; (p < data_size) && (dst < dst_end); ++p, dst += bpp) {
            uint8_t index = data[p];
            assert(index < pal_count);
            uint32_t color = palette[index];
            *((uint32_t*)dst) = color;
        }
        break;
    default: assert(0); return;
    }
}

static inline SpriteFormat PaletteFormatForBPP(int bpp)
{
    switch (bpp)
    {
    case 1: return kSprFmt_PaletteRgb888;
    case 2: return kSprFmt_PaletteRgb565;
    case 3: return kSprFmt_PaletteRgb888;
    case 4: return kSprFmt_PaletteArgb8888;
    default: return kSprFmt_Undefined;
    }
}

static inline uint8_t GetPaletteBPP(SpriteFormat fmt)
{
    switch (fmt)
    {
    case kSprFmt_PaletteRgb888: return 3;
    case kSprFmt_PaletteArgb8888: return 4;
    case kSprFmt_PaletteRgb565: return 2;
    default: return 0; // means no palette
    }
}

void ReadSpriteHeader_321(SpriteDatHeader &hdr, Stream *in, SpriteCompression compress)
{
    // NOTE: we must read first 2 * int8 before checking bpp,
    // both are always present in this format for historical reasons.
    int bpp = in->ReadInt8();
    in->ReadInt8(); // unused
    if (bpp == 0) { hdr = SpriteDatHeader(); return; } // empty slot
    int w = in->ReadInt16();
    int h = in->ReadInt16();
    hdr = SpriteDatHeader(bpp, kSprFmt_Undefined, 0 /* no palette */, compress, w, h);
}

void ReadSpriteHeader_360(SpriteDatHeader &hdr, Stream *in)
{
    // NOTE: we must read first 2 * int8 before checking bpp,
    // both are always present in this format for historical reasons.
    int bpp = in->ReadInt8();
    SpriteFormat sformat = (SpriteFormat)in->ReadInt8();
    if (bpp == 0) { hdr = SpriteDatHeader(); return; } // no sprite (this is not error)
    int pal_count = (uint8_t)in->ReadInt8() + 1; // saved as (count - 1)
    SpriteCompression compress = (SpriteCompression)in->ReadInt8();
    int w = in->ReadInt16();
    int h = in->ReadInt16();
    hdr = SpriteDatHeader(bpp, sformat, pal_count, compress, w, h);
}

static inline void WriteSprHeader(const SpriteDatHeader &hdr, Stream *out)
{
    out->WriteInt8(hdr.BPP);
    out->WriteInt8(hdr.SFormat);
    out->WriteInt8(hdr.PalCount > 0 ? (uint8_t)(hdr.PalCount - 1) : 0);
    out->WriteInt8(hdr.Compress);
    out->WriteInt16(hdr.Width);
    out->WriteInt16(hdr.Height);
}

static PixelBuffer ReadSpriteDataImpl(const SpriteDatHeader &hdr, Stream *in, bool version360, SpriteCompression compress, HError &err)
{
    const int bpp = hdr.BPP, w = hdr.Width, h = hdr.Height;
    PixelBuffer image(w, h, ColorDepthToPixelFormat(bpp * 8));
    if (!image)
    {
        err = new Error(String::FromFormat("LoadSprite: failed to allocate bitmap (%dx%d %d-bit).", w, h, bpp * 8));
        return {};
    }

    ImBufferPtr im_data(image.GetData(), w * h * bpp, bpp);
    // (Optional) Handle storage options, reverse
    std::vector<uint8_t> indexed_buf;
    std::array<uint32_t, 256> palette {};
    uint32_t pal_bpp = GetPaletteBPP(hdr.SFormat);
    if (pal_bpp > 0)
    { // read palette if format assumes one
        switch (pal_bpp)
        {
        case 2: for (uint32_t i = 0; i < hdr.PalCount; ++i) { palette[i] = in->ReadInt16(); }
              break;
        case 3: for (uint32_t i = 0; i < hdr.PalCount; ++i) { palette[i] = in->ReadUInt24(); }
              break;
        case 4: for (uint32_t i = 0; i < hdr.PalCount; ++i) { palette[i] = in->ReadInt32(); }
              break;
        default: assert(0); break;
        }
        indexed_buf.resize(w * h);
        im_data = ImBufferPtr(indexed_buf.data(), indexed_buf.size(), 1);
    }
    // (Optional) Decompress the image data into the temp buffer
    size_t in_data_size =
        ((version360) || compress != kSprCompress_None) ?
        (uint32_t)in->ReadInt32() : (w * h * bpp);
    if (hdr.Compress != kSprCompress_None)
    {
        if (in_data_size == 0)
        {
            err = new Error("LoadSprite: bad compressed data.");
            return {};
        }
        bool result;
        switch (hdr.Compress)
        {
        case kSprCompress_RLE: result = rle_decompress(im_data.Buf, im_data.Size, im_data.BPP, in);
            break;
        case kSprCompress_LZW: result = lzw_decompress(im_data.Buf, im_data.Size, im_data.BPP, in, in_data_size);
            break;
        case kSprCompress_Deflate: result = inflate_decompress(im_data.Buf, im_data.Size, im_data.BPP, in, in_data_size);
            break;
        default: assert(!"Unsupported compression type!"); result = false; break;
        }
        // TODO: test that not more than data_size was read!
        if (!result)
        {
            err = new Error("LoadSprite: failed to decompress pixel array.");
            return {};
        }
    }
    // Otherwise (no compression) read directly
    // CHECKME: using ReadArrayOfN below does not seem right. These methods do endinaness swap.
    // But these are byte buffers with pixel data, which interpretation depends on pixel format,
    // and they are to be read using bit masks and shifts.
    // If ever, then the conversion has to be done when decomposing pixels onto individual RGB parts,
    // converting to another format, etc.
    else
    {
        assert((im_data.Size % im_data.BPP) == 0);
        switch (im_data.BPP)
        {
        case 1: in->Read(im_data.Buf, im_data.Size);
            break;
        case 2: in->ReadArrayOfInt16(
            reinterpret_cast<int16_t*>(im_data.Buf), im_data.Size / sizeof(int16_t));
            break;
        case 3: in->ReadArrayOfUInt24(im_data.Buf, im_data.Size / 3);
            break;
        case 4: in->ReadArrayOfInt32(
            reinterpret_cast<int32_t*>(im_data.Buf), im_data.Size / sizeof(int32_t));
            break;
        default: assert(0); break;
        }
    }
    // Finally revert storage options
    if (pal_bpp > 0)
    {
        UnpackIndexedBitmap(image, im_data.Buf, im_data.Size, palette, hdr.PalCount);
    }
    return image;
}

static PixelBuffer ReadSpriteImpl(Stream *in, bool version360, SpriteCompression compress, HError &err)
{
    SpriteDatHeader hdr;
    version360 ? ReadSpriteHeader_360(hdr, in) : ReadSpriteHeader_321(hdr, in, compress);
    if (hdr.BPP == 0)
    {
        err = HError::None(); // no sprite mark, this is not a error
        return {};
    }
    if (hdr.BPP < 0 || hdr.Width <= 0 || hdr.Height <= 0)
    {
        err = new Error(String::FromFormat("LoadSprite: invalid sprite metrics (%dx%d %d-bit).",
            hdr.Width, hdr.Height, hdr.BPP * 8));
        return {};
    }

    return ReadSpriteDataImpl(hdr, in, version360, compress, err);
}

PixelBuffer ReadSpriteData_360(const SpriteDatHeader &hdr, Stream *in, HError &err)
{
    return ReadSpriteDataImpl(hdr, in, true, kSprCompress_None /* read from format */, err);
}

PixelBuffer ReadSpriteData_321(const SpriteDatHeader &hdr, Stream *in, SpriteCompression compress, HError &err)
{
    return ReadSpriteDataImpl(hdr, in, false, compress, err);
}

PixelBuffer ReadSprite_360(Stream *in, HError &err)
{
    return ReadSpriteImpl(in, true, kSprCompress_None /* read from format */, err);
}

PixelBuffer ReadSprite_321(Stream *in, SpriteCompression compress, HError &err)
{
    return ReadSpriteImpl(in, false, compress, err);
}

size_t GetSpriteDataSize_360(const SpriteDatHeader &hdr, Stream *in)
{
    size_t data_size = 0;
    soff_t data_pos = in->GetPosition();
    // Optional palette
    size_t pal_size = hdr.PalCount * GetPaletteBPP(hdr.SFormat);
    data_size += pal_size;
    in->Seek(pal_size);
    // Pixel data
    data_size += (uint32_t)in->ReadInt32() + sizeof(uint32_t);
    in->Seek(data_pos, kSeekBegin);
    return data_size;
}

size_t GetSpriteDataSize_321(const SpriteDatHeader &hdr, Stream *in, SpriteCompression compress)
{
    soff_t data_pos = in->GetPosition();
    // NOTE: old format never had palette along with a sprite data
    size_t data_size = (compress != kSprCompress_None) ?
        (uint32_t)in->ReadInt32() + sizeof(uint32_t) :
        hdr.Width * hdr.Height * hdr.BPP;
    in->Seek(data_pos, kSeekBegin);
    return data_size;
}

void SkipSpriteData_360(const SpriteDatHeader &hdr, Stream *in)
{
    int pal_bpp = GetPaletteBPP(hdr.SFormat);
    if (pal_bpp > 0) in->Seek(hdr.PalCount * pal_bpp); // skip palette
    size_t data_sz = (uint32_t)in->ReadInt32();
    in->Seek(data_sz); // skip image data
}

void SkipSpriteData_321(const SpriteDatHeader &hdr, Stream *in, SpriteCompression compress)
{
    // NOTE: old format never had palette along with a sprite data
    size_t data_sz = (compress != kSprCompress_None) ?
        (uint32_t)in->ReadInt32() : hdr.Width * hdr.Height * hdr.BPP;
    in->Seek(data_sz); // skip image data
}

// Writes prepared image data in a proper file format, following explicit data_bpp rule
static void WriteSpriteDataImpl(const SpriteDatHeader &hdr, Stream *out,
    const uint8_t *im_data, size_t im_data_sz, int im_bpp,
    const uint32_t palette[256])
{
    WriteSprHeader(hdr, out);
    // write palette, if available
    int pal_bpp = GetPaletteBPP(hdr.SFormat);
    if (pal_bpp > 0)
    {
        assert(hdr.PalCount > 0);
        switch (pal_bpp)
        {
        case 2: for (uint32_t i = 0; i < hdr.PalCount; ++i) { out->WriteInt16(palette[i]); }
              break;
        case 3: for (uint32_t i = 0; i < hdr.PalCount; ++i) { out->WriteUInt24(palette[i]); }
              break;
        case 4: for (uint32_t i = 0; i < hdr.PalCount; ++i) { out->WriteInt32(palette[i]); }
              break;
        default: assert(0); break;
        }
    }
    // write the image pixel data
    out->WriteInt32(im_data_sz);
    // CHECKME: using WriteArrayOfN below does not seem right. These methods do endinaness swap.
    // But these are byte buffers with pixel data, which interpretation depends on pixel format,
    // and they are to be read using bit masks and shifts.
    // If ever, then the conversion has to be done when decomposing pixels onto individual RGB parts,
    // converting to another format, etc.
    switch (im_bpp)
    {
    case 1: out->Write(im_data, im_data_sz);
        break;
    case 2: out->WriteArrayOfInt16(reinterpret_cast<const int16_t*>(im_data),
        im_data_sz / sizeof(int16_t));
        break;
    case 3: out->WriteArrayOfUInt24(im_data, im_data_sz / 3);
        break;
    case 4: out->WriteArrayOfInt32(reinterpret_cast<const int32_t*>(im_data),
        im_data_sz / sizeof(int32_t));
        break;
    default: assert(0); break;
    }
}

static void WriteSpriteImpl(const BitmapData &image, Stream *out, int store_flags, SpriteCompression compress,
    std::vector<uint8_t> *mem_buf)
{
    const int bpp = image.GetBytesPerPixel();
    const int w = image.GetWidth();
    const int h = image.GetHeight();
    ImBufferCPtr im_data(image.GetData(), w * h * bpp, bpp);

    // (Optional) Handle storage options
    std::vector<uint8_t> indexed_buf;
    uint32_t palette[256];
    uint32_t pal_count = 0;
    SpriteFormat sformat = kSprFmt_Undefined;
    if ((store_flags & kSprStore_OptimizeForSize) != 0 && (bpp > 1))
    { // Try to store this sprite as an indexed bitmap
        uint32_t gen_pal_count;
        if (CreateIndexedBitmap(image, indexed_buf, palette, gen_pal_count) && gen_pal_count > 0)
        { // Test the resulting size, and switch if the paletted image is less
            if (im_data.Size > (indexed_buf.size() + gen_pal_count * bpp))
            {
                im_data = ImBufferCPtr(indexed_buf.data(), indexed_buf.size(), 1);
                sformat = PaletteFormatForBPP(bpp);
                pal_count = gen_pal_count;
            }
        }
    }
    // (Optional) Compress the image data into the temp buffer
    std::vector<uint8_t> local_membuf;
    if (compress != kSprCompress_None)
    {
        if (!mem_buf)
            mem_buf = &local_membuf;
        Stream mems(std::make_unique<VectorStream>(*mem_buf, kStream_Write));
        bool result;
        switch (compress)
        {
        case kSprCompress_RLE: result = rle_compress(im_data.Buf, im_data.Size, im_data.BPP, &mems);
            break;
        case kSprCompress_LZW: result = lzw_compress(im_data.Buf, im_data.Size, im_data.BPP, &mems);
            break;
        case kSprCompress_Deflate: result = deflate_compress(im_data.Buf, im_data.Size, im_data.BPP, &mems);
            break;
        default: assert(!"Unsupported compression type!"); result = false; break;
        }
        // mark to write as a plain byte array
        im_data = result ? ImBufferCPtr(mem_buf->data(), mem_buf->size(), 1) : ImBufferCPtr();
    }

    // Write the final data
    SpriteDatHeader hdr(bpp, sformat, pal_count, compress, w, h);
    WriteSpriteDataImpl(hdr, out, im_data.Buf, im_data.Size, im_data.BPP, palette);
}

void WriteSprite_360(const BitmapData &image, Stream *out, int store_flags, SpriteCompression compress,
    std::vector<uint8_t> *mem_buf)
{
    WriteSpriteImpl(image, out, store_flags, compress, mem_buf);
}

void WriteRawSpriteData_360(const SpriteDatHeader &hdr, Stream *out, const uint8_t *data, size_t data_sz)
{
    WriteSprHeader(hdr, out);
    out->Write(data, data_sz);
}

} // namespace SpriteDataUtils

} // namespace Common
} // namespace AGS

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
// BitmapData is a raw array of pixels in the particular format.
//
//=============================================================================
#ifndef __AGS_CN_GFX__BITMAPDATA_H
#define __AGS_CN_GFX__BITMAPDATA_H

#include <memory>
#include "debug/assert.h"
#include "platform/types.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

// A format of pixel storage in memory;
// Note that there's a small list of formats used by AGS at runtime,
// others are here only for temporary use e.g. when loading an image from file.
// FIXME: differentiate RGB(A) formats with different order of components!
// -- this may be required for transformation checks when saving or loading image files.
enum PixelFormat
{
    kPxFmt_Undefined,
    kPxFmt_Indexed1,    // 1-bit palette index (monochrome)
    kPxFmt_Indexed4,    // 4-bit palette index
    kPxFmt_Indexed8,    // 8-bit palette index
    kPxFmt_Indexed      = kPxFmt_Indexed8,
    kPxFmt_R5G5B5,      // 15-bit R5G5B5
    kPxFmt_R5G6B5,      // 16-bit R5G6B5, historical 16-bit pixel format in AGS
    kPxFmt_R8G8B8,      // 24-bit RGB (no alpha)
    kPxFmt_A8R8G8B8,    // 32-bit ARGB
};

// Returns bits-per-pixel from format
inline int PixelFormatToPixelBits(PixelFormat fmt)
{
    switch (fmt)
    {
    case kPxFmt_Indexed1:   return 1;
    case kPxFmt_Indexed4:   return 4;
    case kPxFmt_Indexed8:   return 8;
    case kPxFmt_R5G5B5:     return 15;
    case kPxFmt_R5G6B5:     return 16;
    case kPxFmt_R8G8B8:     return 24;
    case kPxFmt_A8R8G8B8:   return 32;
    default:                return 0;
    }
}

inline int PixelFormatToPixelBytes(PixelFormat fmt)
{
    return (PixelFormatToPixelBits(fmt) + 7) / 8;
}

inline PixelFormat ColorDepthToPixelFormat(int color_depth)
{
    switch (color_depth)
    {
    case 1: return kPxFmt_Indexed1;
    case 4: return kPxFmt_Indexed4;
    case 8: return kPxFmt_Indexed8;
    case 15: return kPxFmt_R5G5B5;
    case 16: return kPxFmt_R5G6B5;
    case 24: return kPxFmt_R8G8B8;
    case 32: return kPxFmt_A8R8G8B8;
    default: return kPxFmt_Undefined;
    }
}

inline bool PixelFormatHasAlpha(PixelFormat fmt)
{
    switch (fmt)
    {
    case kPxFmt_A8R8G8B8:   return true;
    default:                return false;
    }
}

inline bool PixelFormatIndexed(PixelFormat fmt)
{
    switch (fmt)
    {
    case kPxFmt_Indexed1:
    case kPxFmt_Indexed4:
    case kPxFmt_Indexed8:
        return true;
    default: return false;
    }
}

inline size_t GetStrideForPixelFormat(PixelFormat fmt, int width)
{
    switch (fmt)
    {
    case kPxFmt_Indexed1:
        return (width + 7) / 8; // ensure rounding up
    case kPxFmt_Indexed4:
        return (width + 1) / 2; // ensure rounding up
    default:
        return (width * PixelFormatToPixelBytes(fmt));
    }
}

inline size_t GetDataSizeForPixelFormat(PixelFormat fmt, int width, int height)
{
    return GetStrideForPixelFormat(fmt, width) * height;
}

String PixelFormatName(PixelFormat fmt);

// Returns default mask color (transparent color) for the given pixel format.
// Note that this refers to the special color that may be optionally used as
// a transparent, but disregards alpha channel. Images with alpha channel
// should normally use alpha values for transparency.
inline int GetDefaultMaskColor(PixelFormat fmt)
{
    switch (fmt)
    {
    case kPxFmt_Indexed1:
    case kPxFmt_Indexed4:
    case kPxFmt_Indexed8:
        // All the indexed formats use 0 index as the default mask color
        return 0;
    // Following correspond to magenta (aka "magic pink")
    case kPxFmt_R5G5B5:     return 0x7C1F;
    case kPxFmt_R5G6B5:     return 0xF81F;
    case kPxFmt_R8G8B8:     return 0xFF00FF;
    case kPxFmt_A8R8G8B8:   return 0xFF00FF; // NOTE: it's a value without alpha
    default: assert(false); return 0;
    }
}


// BitmapData is a non-owning wrapper over a pixel buffer,
// combined with the description of its format.
// Its purpose is to pass the buffer pointer without bringing
// dependency on full Bitmap class.
// TODO: consider adding optional palette field (unique_ptr with RGB array?),
// may be useful to combine them when loading indexed images.
class BitmapData
{
public:
    BitmapData() = default;
    BitmapData(uint8_t *buf, size_t data_sz, size_t stride, int width, int height, PixelFormat fmt)
        : _format(fmt), _bitsPerPixel(PixelFormatToPixelBits(fmt)), _width(width), _height(height)
        , _cbuf(buf), _buf(buf), _dataSize(data_sz), _stride(stride)
    { assert(_buf && _dataSize > 0u && _stride >= 0u && _bitsPerPixel > 0u && _width > 0u && _height >= 0u); }
    BitmapData(const uint8_t *cbuf, size_t data_sz, size_t stride, int width, int height, PixelFormat fmt)
        : _format(fmt), _bitsPerPixel(PixelFormatToPixelBits(fmt)), _width(width), _height(height)
        , _cbuf(cbuf), _buf(nullptr), _dataSize(data_sz), _stride(stride)
    { assert(_cbuf && _dataSize > 0u && _stride >= 0u && _bitsPerPixel > 0u && _width > 0u && _height >= 0u); }

    // Tests if BitmapData reference a valid pixel buffer
    operator bool() const { return _cbuf != nullptr; }

    inline PixelFormat GetFormat() const { return _format; }
    // Gets color depth in bits per pixel
    inline int GetColorDepth() const { return _bitsPerPixel; }
    inline int GetBytesPerPixel() const { return (_bitsPerPixel + 7) / 8; }
    inline int GetWidth() const { return _width; }
    inline int GetHeight() const { return _height; }
    // Gets the total size of the pixel data buffer, in bytes
    inline size_t GetDataSize() const { return _dataSize; }
    // Gets the length of a single bitmap line, in bytes;
    // this line may have extra padding beyond the bitmap's width
    // TODO: bring the Stride/Pitch term to a consistency among all the AGS classes and functions
    // (BitmapData, PixelBuffer, Bitmap, and parameters in pixel & bitmap related operations).
    inline size_t GetStride() const { return _stride; }
    inline const uint8_t *GetData() const { return _cbuf; }
    inline uint8_t *GetData() { return _buf; }

    inline uint8_t *GetLine(size_t i)
    {
        assert(_stride * i < _dataSize);
        return &_buf[_stride * i];
    }
    inline const uint8_t *GetLine(size_t i) const
    {
        assert(_stride * i < _dataSize);
        return &_cbuf[_stride * i];
    }

    uint32_t GetPixel(int x, int y) const;
    void SetPixel(int x, int y, uint32_t value);

protected:
    BitmapData(int width, int height, PixelFormat fmt)
        : _format(fmt), _bitsPerPixel(PixelFormatToPixelBits(fmt)), _width(width), _height(height)
    { assert(_bitsPerPixel > 0u && _width > 0u && _height >= 0u); }

    PixelFormat _format = kPxFmt_Undefined;
    uint32_t _bitsPerPixel = 0u;
    int _width = 0;
    int _height = 0;
    const uint8_t *_cbuf = nullptr;
    uint8_t *_buf = nullptr;
    size_t _dataSize = 0u;
    size_t _stride = 0u;
};


// PixelBuffer is a pixel array in defined format.
// Its purpose is to read and store pixel data, which may be later
// used to create a Bitmap object, or write elsewhere.
class PixelBuffer : public BitmapData
{
public:
    // Constructs an empty pixel buffer
    PixelBuffer() = default;
    // Constructs a pixel buffer for the given image size and pixel format
    PixelBuffer(int width, int height, PixelFormat fmt)
        : BitmapData(width, height, fmt)
    {
        _dataSize = GetDataSizeForPixelFormat(fmt, width, height);
        _stride = GetStrideForPixelFormat(fmt, width);
        assert(_dataSize > 0u && _stride > 0u);
        _data.reset(new uint8_t[_dataSize]);
        _buf = _data.get();
        _cbuf = _data.get();
    }
    PixelBuffer(std::unique_ptr<uint8_t[]> &&data, size_t data_sz,
        int width, int height, PixelFormat fmt, size_t stride)
        : BitmapData(width, height, fmt)
    {
        _dataSize = data_sz;
        _stride = stride;
        assert(_dataSize > 0u && _stride > 0u);
        _data = std::move(data);
        _buf = _data.get();
        _cbuf = _data.get();
    }
    PixelBuffer(const PixelBuffer &src)
    {
        *this = src;
    }
    PixelBuffer(PixelBuffer &&src)
    {
        *this = std::move(src);
    }

    inline std::unique_ptr<uint8_t[]> ReleaseData()
    {
        auto data = std::move(_data);
        *this = {};
        return data;
    }

    inline PixelBuffer &operator =(const PixelBuffer &src)
    {
        _width = src._width;
        _height = src._height;
        _bitsPerPixel = src._bitsPerPixel;
        _format = src._format;
        _dataSize = src._dataSize;
        _stride = src._stride;
        if (src._data)
        {
            _data.reset(new uint8_t[_dataSize]);
            std::copy(src._data.get(), src._data.get() + _dataSize, _data.get());
        }
        else
        {
            _data = nullptr;
        }
        _buf = _data.get();
        _cbuf = _data.get();
        return *this;
    }

    inline PixelBuffer &operator =(PixelBuffer &&src)
    {
        _width = src._width;
        _height = src._height;
        _bitsPerPixel = src._bitsPerPixel;
        _format = src._format;
        _dataSize = src._dataSize;
        _stride = src._stride;
        _data = std::move(src._data);
        _buf = _data.get();
        _cbuf = _data.get();
        return *this;
    }

private:
    std::unique_ptr<uint8_t[]> _data;
};


namespace PixelOp
{
    // Copy pixel data from one memory buffer to another. It is required that the
    // buffers match same format, and have enough size.
    // Pitches are given in bytes and define the length of the source and dest scan lines.
    // Width and height of the rectangle are *in pixels*.
    void CopyPixels(const uint8_t *src_buffer, const int bpp, const size_t src_pitch,
        const int width, const int height, uint8_t *dst_buffer, const size_t dst_pitch);
    inline void CopyPixels(const uint8_t *src_buffer, const PixelFormat fmt, const size_t src_pitch,
        const int width_px, const int height_px, uint8_t *dst_buffer, const size_t dst_pitch)
    {
        CopyPixels(src_buffer, PixelFormatToPixelBytes(fmt), src_pitch, width_px, height_px, dst_buffer, dst_pitch);
    }
    // Copy a portion of pixel data from one memory buffer to another. It is required that the
    // buffers match same format, and have enough size.
    // Pitches are given in bytes and define the length of the source and dest scan lines.
    // Width and height of the rectangle, as well as source and destination offsets are *in pixels*.
    void CopyPixelsRegion(const uint8_t *src_buffer, const int bpp, const size_t src_pitch,
        const int src_px_off, const int src_py_off, const int width_px, const int height_px,
        uint8_t *dst_buffer, const size_t dst_pitch, const int dst_px_off, const int dst_py_off);
    inline void CopyPixelsRegion(const uint8_t *src_buffer, const PixelFormat fmt, const size_t src_pitch,
        const int src_px_off, const int width_px, const int height_px,
        uint8_t *dst_buffer, const size_t dst_pitch, const int dst_px_off)
    {
        CopyPixelsRegion(src_buffer, PixelFormatToPixelBytes(fmt), src_pitch,
            src_px_off, 0, width_px, height_px, dst_buffer, dst_pitch, dst_px_off, 0);
    }

    // Copy a portion of pixel data and return as a new PixelBuffer.
    // Position and size of the region is in pixels.
    PixelBuffer CopyPixelsRegion(const BitmapData &bm_data, const int src_x, const int src_y, const int width, const int height);

    // Copies pixels from source to dest buffer, possibly converting between source
    // and dest pixel format. The destination buffer must be properly allocated
    //     (see GetDataSizeForPixelFormat()).
    // Returns result, fails if conversion not supported.
    // WARNING: the only conversion supported currently are:
    //          * 1-bit     => 8-bit
    //          * 4-bit     => 8-bit
    //          * 16-bit    => 24-bit
    //          * 16-bit    => 32-bit
    //          add more common conversions later!
    // FIXME: this might require a mask color parameter when converting from non-32bit
    // to 32-bit ARGB, because otherwise the non-32bit mask color might end up opaque
    // color which is not recognized as mask color in 32bit image.
    // FIXME: this would require a palette if conversion goes from indexed to non-indexed!
    // Consider adding more function variants that accept mask colors and palettes.
    bool CopyConvert(const uint8_t *src_buffer, const PixelFormat src_fmt, const size_t src_pitch,
        const int width, const int height, uint8_t *dst_buffer, const PixelFormat dst_fmt, const size_t dst_pitch);
    // Copies pixels from source to dest buffer, possibly converting between source
    // and dest pixel format. The resulting dest buffer may be possibly reallocated if it's
    // not large enough to accomodate the converted data.
    bool CopyConvert(const BitmapData &src, PixelBuffer &dst, const PixelFormat dst_fmt);
    // Copies pixels from source to dest buffer, swapping the RGB components, according
    // to the provided RGB shifts. This operation requires that pixel format is kept the same.
    // It is actually possible to swap in-place (where src and dst are the same buffers).
    void CopySwapRGBA(const uint8_t *src_buffer, const size_t src_pitch, int src_r_shift, int src_g_shift, int src_b_shift, int src_a_shift,
        uint8_t *dst_buffer, const size_t dst_pitch, int dst_r_shift, int dst_g_shift, int dst_b_shift, int dst_a_shift,
        const int width, const int height, const PixelFormat px_fmt);
    // Copies pixels from source to dest buffer, swapping the RGB components, according
    // to the provided RGB shifts. This operation requires that pixel format is kept the same.
    // The source and destination pitches are calculated from PixelFormat.
    // It is actually possible to swap in-place (where src and dst are the same buffers).
    void CopySwapRGBA(const uint8_t *src_buffer, int src_r_shift, int src_g_shift, int src_b_shift, int src_a_shift,
        uint8_t *dst_buffer, int dst_r_shift, int dst_g_shift, int dst_b_shift, int dst_a_shift,
        const int width, const int height, const PixelFormat px_fmt);

    // Makes the given image opaque (full alpha), while keeping RGB unchanged.
    void MakeOpaque(BitmapData &bm_data);
    // Makes the given image opaque (full alpha), while keeping RGB values unchanged.
    // Skips mask color (leaves it with zero alpha).
    void MakeOpaqueSkipMask(BitmapData &bm_data);
    // Replaces pixels with alpha <= threshold with standard mask color.
    void ReplaceAlphaWithRGBMask(BitmapData &bm_data, int alpha_threshold = 0);
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__BITMAPDATA_H

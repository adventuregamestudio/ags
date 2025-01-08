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
// BitmapData is a raw array of pixels in the particular format.
//
//=============================================================================
#ifndef __AGS_CN_GFX__BITMAPDATA_H
#define __AGS_CN_GFX__BITMAPDATA_H

#include <memory>
#include "core/types.h"
#include "debug/assert.h"

namespace AGS
{
namespace Common
{

// A format of pixel storage in memory;
// Note that there's a small list of formats used by AGS at runtime,
// others are here only for temporary use e.g. when loading an image from file.
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
    kPxFmt_X8R8G8B8,    // 32-bit RGB with no valid alpha
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
    case kPxFmt_X8R8G8B8:   return 32;
    default:                return 0;
    }
}

inline int PixelFormatToPixelBytes(PixelFormat fmt)
{
    return (PixelFormatToPixelBits(fmt) + 7) / 8;
}

inline PixelFormat ColorDepthToPixelFormat(int color_depth, bool alpha_valid = true)
{
    switch (color_depth)
    {
    case 1: return kPxFmt_Indexed1;
    case 4: return kPxFmt_Indexed4;
    case 8: return kPxFmt_Indexed8;
    case 15: return kPxFmt_R5G5B5;
    case 16: return kPxFmt_R5G6B5;
    case 24: return kPxFmt_R8G8B8;
    case 32: return alpha_valid ? kPxFmt_A8R8G8B8 : kPxFmt_X8R8G8B8;
    default: return kPxFmt_Undefined;
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


// BitmapData is a non-owning wrapper over a pixel buffer,
// combined with the description of its format.
// Its purpose is to pass the buffer pointer without bringing
// dependency on full Bitmap class.
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
    inline bool HasAlphaChannel() const { return _format == kPxFmt_A8R8G8B8; }
    inline int GetColorDepth() const { return _bitsPerPixel; }
    inline int GetWidth() const { return _width; }
    inline int GetHeight() const { return _height; }
    inline size_t GetDataSize() const { return _dataSize; }
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

    inline std::unique_ptr<uint8_t[]> ReleaseData()
    {
        auto data = std::move(_data);
        *this = {};
        return data;
    }

private:
    std::unique_ptr<uint8_t[]> _data;
};


namespace PixelOp
{
    // Copy pixel data from one memory buffer to another. It is required that the
    // buffers match same format, and have enough size.
    // Pitches are given in bytes and define the length of the source and dest scan lines.
    // Offsets are optional and define horizontal dest and source offsets, *in pixels*.
    void CopyPixels(uint8_t *dst_buffer, const size_t dst_pitch, const size_t dst_px_offset,
        const int bpp, const int height, const uint8_t *src_buffer, const size_t src_pitch, const size_t src_px_offset);
    inline void CopyPixels(uint8_t *dst_buffer, const size_t dst_pitch, const size_t dst_px_offset,
        const PixelFormat fmt, const int height, const uint8_t *src_buffer, const size_t src_pitch, const size_t src_px_offset)
    {
        CopyPixels(dst_buffer, dst_pitch, dst_px_offset, PixelFormatToPixelBytes(fmt), height, src_buffer, src_pitch, src_px_offset);
    }
    // Copies pixels from source to dest buffer, possibly converting between source
    // and dest pixel format. The destination buffer must be properly allocated
    //     (see GetDataSizeForPixelFormat()).
    // Returns result, fails if conversion not supported.
    // WARNING: the only conversion supported currently is 4-bit => 8-bit and 1-bit => 8-bit;
    //          add more common conversions later!
    bool CopyConvert(uint8_t *dst_buffer, const PixelFormat dst_fmt, const size_t dst_pitch,
        const int height, const uint8_t *src_buffer, const PixelFormat src_fmt, const size_t src_pitch);
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__BITMAPDATA_H

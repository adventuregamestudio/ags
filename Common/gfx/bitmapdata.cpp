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
#include "gfx/bitmapdata.h"
#include "util/memory.h"

namespace AGS
{
namespace Common
{

uint32_t BitmapData::GetPixel(int x, int y) const
{
    const uint8_t *line = GetLine(y);
    switch (_bitsPerPixel)
    {
    case 1: return (line[x / 8] >> (7 - x % 8)) & 0x1;
    case 4: return (line[x / 2] >> (1 - x % 2)) & 0x4;
    case 8: return line[x];
    case 15: /* same as 16 */
    case 16: return *reinterpret_cast<const uint16_t*>(&line[x * 2]);
    case 24: return Memory::ReadInt24(&line[x * 3]);
    case 32: return *reinterpret_cast<const uint32_t*>(&line[x * 4]);
    default: assert(false); return 0;
    }
}


namespace PixelOp
{

void CopyPixels(uint8_t *dst_buffer, const size_t dst_pitch, const size_t dst_px_offset,
    const int bpp, const int height, const uint8_t *src_buffer, const size_t src_pitch, const size_t src_px_offset)
{
    const size_t src_bpp_offset = src_px_offset * bpp;
    const size_t dst_bpp_offset = dst_px_offset * bpp;
    if (src_bpp_offset >= src_pitch || dst_bpp_offset >= dst_pitch)
        return; // nothing to copy
    Memory::BlockCopy(dst_buffer, dst_pitch, src_bpp_offset, src_buffer, src_pitch, dst_bpp_offset, height);
}

bool CopyConvert(uint8_t *dst_buffer, const PixelFormat dst_fmt, const size_t dst_pitch,
    const int width, const int height, const uint8_t *src_buffer, const PixelFormat src_fmt, const size_t src_pitch)
{
    if (dst_fmt == src_fmt)
    {
        CopyPixels(dst_buffer, dst_pitch, 0u, src_fmt, height, src_buffer, src_pitch, 0u);
        return true;
    }

    // Copy 4-bit indexed image into 8-bit image
    if (dst_fmt == kPxFmt_Indexed8 && src_fmt == kPxFmt_Indexed4)
    {
        const uint8_t *src_ptr = src_buffer;
        const uint8_t *src_end = src_buffer + src_pitch * height;
        const uint32_t src_width = width / 2; // number of full sequences of 2 source pixels
        for (uint8_t *dst_ptr = dst_buffer; src_ptr < src_end; src_ptr += src_pitch, dst_ptr += dst_pitch)
        {
            uint32_t x = 0;
            for (; x < src_width; ++x)
            {
                uint8_t sp = src_ptr[x];
                dst_ptr[x * 2]     = ((sp >> 4) & 0xF);
                dst_ptr[x * 2 + 1] =  (sp       & 0xF);
            }

            // Last pixel (upper half of the source byte)
            if (src_width * 2 < width)
                dst_ptr[x * 2] = ((src_ptr[x] >> 4) & 0xF);
        }
        return true;
    }
    // Copy 1-bit monochrome image into 8-bit image
    else if (dst_fmt == kPxFmt_Indexed8 && src_fmt == kPxFmt_Indexed1)
    {
        const uint8_t *src_ptr = src_buffer;
        const uint8_t *src_end = src_buffer + src_pitch * height;
        const uint32_t src_width = width / 8; // number of full sequences of 8 source pixels
        for (uint8_t *dst_ptr = dst_buffer; src_ptr < src_end; src_ptr += src_pitch, dst_ptr += dst_pitch)
        {
            uint32_t x = 0;
            for (; x < src_width; ++x)
            {
                uint8_t sp = src_ptr[x];
                dst_ptr[x * 8]     = ((sp >> 7) & 0x1);
                dst_ptr[x * 8 + 1] = ((sp >> 6) & 0x1);
                dst_ptr[x * 8 + 2] = ((sp >> 5) & 0x1);
                dst_ptr[x * 8 + 3] = ((sp >> 4) & 0x1);
                dst_ptr[x * 8 + 4] = ((sp >> 3) & 0x1);
                dst_ptr[x * 8 + 5] = ((sp >> 2) & 0x1);
                dst_ptr[x * 8 + 6] = ((sp >> 1) & 0x1);
                dst_ptr[x * 8 + 7] =  (sp       & 0x1);
            }

            // Last 1-7 pixels (upper portion of the source byte)
            for (uint32_t dx = src_width * 8, sh = 7; dx < width; ++dx, --sh)
                dst_ptr[dx] = ((src_ptr[x] >> sh) & 0x1);
        }
        return true;
    }
    return false;
}

} // namespace PixelOperations

} // namespace Common
} // namespace AGS

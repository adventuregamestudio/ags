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
// TODO: we only need color.h/color.c for this code unit;
// consider porting necessary tables / shifts from Allegro right here
// if this dependency will cause trouble (e.g. for tools).
#include <allegro.h>
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

void CopyPixels(const uint8_t *src_buffer, const int bpp, const size_t src_pitch,
    const int width, const int height, uint8_t *dst_buffer, const size_t dst_pitch)
{
    CopyPixelsRegion(src_buffer, bpp, src_pitch, 0u, width, height, dst_buffer, dst_pitch, 0u);
}

void CopyPixelsRegion(const uint8_t *src_buffer, const int bpp, const size_t src_pitch,
    const size_t src_px_off, const int width_px, const int height_px,
    uint8_t *dst_buffer, const size_t dst_pitch, const size_t dst_px_off)
{
    const size_t src_off = src_px_off * bpp;
    const size_t dst_off = dst_px_off * bpp;
    const size_t width = width_px * bpp;
    const size_t height = height_px * bpp;
    // NOTE: all assertions are done further in Memory::BlockCopy
    Memory::BlockCopy(src_buffer, src_pitch, src_off, width, height, dst_buffer, dst_pitch, dst_off);
}

bool CopyConvert(const uint8_t *src_buffer, const PixelFormat src_fmt, const size_t src_pitch,
    const int width, const int height, uint8_t *dst_buffer, const PixelFormat dst_fmt, const size_t dst_pitch)
{
    if (dst_fmt == src_fmt)
    {
        CopyPixels(src_buffer, src_fmt, src_pitch, width, height, dst_buffer, dst_pitch);
        return true;
    }

    // TODO: this function originally was purposed to support conversions that
    // Allegro 4 cannot do (like 1-bit and 4-bit to 8-bit), but later it turned
    // out that it may need many others. Consider using more generic algorithms,
    // similar to how Allegro 4 does this, or even use functions from Allegro 4.
    // For indexed conversions this may require separate approach,
    // but for RGB/ARGB ones we might need a generic function with following params:
    // - ARGB shifts (for src and dest)
    // - ARGB masks  (for src and dest)
    // - optionally re-scale arrays, e.g. for scaling from 16-bit to 24/32-bit.

    // 1-bit monochrome => 8-bit indexed image
    if (src_fmt == kPxFmt_Indexed1 && dst_fmt == kPxFmt_Indexed8)
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
    // 4-bit indexed image => 8-bit indexed image
    else if (src_fmt == kPxFmt_Indexed4 && dst_fmt == kPxFmt_Indexed8)
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
    // 16-bit RGB -> 24-bit RGB
    else if (src_fmt == kPxFmt_R5G6B5 && dst_fmt == kPxFmt_R8G8B8)
    {
        const uint8_t *src_end = src_buffer + src_pitch * height;
        for (; src_buffer < src_end; src_buffer += src_pitch, dst_buffer += dst_pitch)
        {
            const uint16_t *src_ptr = reinterpret_cast<const uint16_t*>(src_buffer);
            uint8_t *dst_ptr = dst_buffer;
            for (int x = 0; x < width; ++x, dst_ptr += 3)
            {
                uint16_t c = *(src_ptr++);
                int32_t c2 = 
                    _rgb_scale_5[((c >> _rgb_r_shift_16) & 0x1F)] << _rgb_r_shift_32 |
                    _rgb_scale_6[((c >> _rgb_g_shift_16) & 0x3F)] << _rgb_g_shift_32 |
                    _rgb_scale_5[((c >> _rgb_b_shift_16) & 0x1F)] << _rgb_b_shift_32;
                Memory::WriteInt24(dst_ptr, c2);
            }
        }
    }
    // 16-bit RGB -> 32-bit ARGB
    else if (src_fmt == kPxFmt_R5G6B5 && dst_fmt == kPxFmt_A8R8G8B8)
    {
        const uint8_t *src_end = src_buffer + src_pitch * height;
        for (; src_buffer < src_end; src_buffer += src_pitch, dst_buffer += dst_pitch)
        {
            const uint16_t *src_ptr = reinterpret_cast<const uint16_t*>(src_buffer);
            uint32_t *dst_ptr = reinterpret_cast<uint32_t*>(dst_buffer);
            for (int x = 0; x < width; ++x)
            {
                uint16_t c = *(src_ptr++);
                *(dst_ptr++) =
                    _rgb_scale_5[((c >> _rgb_r_shift_16) & 0x1F)] << _rgb_r_shift_32 |
                    _rgb_scale_6[((c >> _rgb_g_shift_16) & 0x3F)] << _rgb_g_shift_32 |
                    _rgb_scale_5[((c >> _rgb_b_shift_16) & 0x1F)] << _rgb_b_shift_32 |
                    0xFF << _rgb_a_shift_32;
            }
        }
    }
    // 32-bit RGB -> 24-bit ARGB (cut alpha channel)
    else if (src_fmt == kPxFmt_A8R8G8B8 && dst_fmt == kPxFmt_R8G8B8)
    {
        const uint8_t *src_end = src_buffer + src_pitch * height;
        for (; src_buffer < src_end; src_buffer += src_pitch, dst_buffer += dst_pitch)
        {
            const uint32_t *src_ptr = reinterpret_cast<const uint32_t*>(src_buffer);
            uint8_t *dst_ptr = dst_buffer;
            for (int x = 0; x < width; ++x, dst_ptr += 3)
            {
                Memory::WriteInt24(dst_ptr, (*(src_ptr++) & 0xFFFFFF));
            }
        }
    }
    return false;
}

void CopySwapRGBA(const uint8_t *src_buffer, const size_t src_pitch, int src_r_shift, int src_g_shift, int src_b_shift, int src_a_shift,
    uint8_t *dst_buffer, const size_t dst_pitch, int dst_r_shift, int dst_g_shift, int dst_b_shift, int dst_a_shift,
    const int width, const int height, const PixelFormat px_fmt)
{
    const int bpp = PixelFormatToPixelBytes(px_fmt);
    if (bpp <= 1)
        return; // nothing to swap

    switch (bpp)
    {
    case 2:
        {
            const uint8_t *src_end = src_buffer + src_pitch * height;
            for (; src_buffer < src_end; src_buffer += src_pitch, dst_buffer += dst_pitch)
            {
                const uint16_t *src_ptr = reinterpret_cast<const uint16_t*>(src_buffer);
                uint16_t *dst_ptr = reinterpret_cast<uint16_t*>(dst_buffer);
                for (int x = 0; x < width; ++x)
                {
                    uint16_t c = *(src_ptr++);
                    *(dst_ptr++) =
                        ((c >> src_r_shift) & 0x1F) << dst_r_shift |
                        ((c >> src_g_shift) & 0x3F) << dst_g_shift |
                        ((c >> src_b_shift) & 0x1F) << dst_b_shift;
                }
            }
        }
        break;
    case 3:
        {
            const uint8_t *src_end = src_buffer + src_pitch * height;
            for (; src_buffer < src_end; src_buffer += src_pitch, dst_buffer += dst_pitch)
            {
                const uint8_t *src_ptr = src_buffer;
                uint8_t *dst_ptr = dst_buffer;
                for (int x = 0; x < width; ++x, src_ptr += 3, dst_ptr += 3)
                {
                    int32_t c = Memory::ReadInt24(src_ptr);
                    int32_t c2 = 
                        ((c >> src_r_shift) & 0xFF) << dst_r_shift |
                        ((c >> src_g_shift) & 0xFF) << dst_g_shift |
                        ((c >> src_b_shift) & 0xFF) << dst_b_shift;
                    Memory::WriteInt24(dst_ptr, c2);
                }
            }
        }
        break;
    case 4:
        {
            const uint8_t *src_end = src_buffer + src_pitch * height;
            for (; src_buffer < src_end; src_buffer += src_pitch, dst_buffer += dst_pitch)
            {
                const uint32_t *src_ptr = reinterpret_cast<const uint32_t*>(src_buffer);
                uint32_t *dst_ptr = reinterpret_cast<uint32_t*>(dst_buffer);
                for (int x = 0; x < width; ++x)
                {
                    uint32_t c = *(src_ptr++);
                    *(dst_ptr++) =
                        ((c >> src_r_shift) & 0xFF) << dst_r_shift |
                        ((c >> src_g_shift) & 0xFF) << dst_g_shift |
                        ((c >> src_b_shift) & 0xFF) << dst_b_shift |
                        ((c >> src_a_shift) & 0xFF) << dst_a_shift;
                }
            }
        }
        break;
    }
}

void CopySwapRGBA(const uint8_t *src_buffer, int src_r_shift, int src_g_shift, int src_b_shift, int src_a_shift,
    uint8_t *dst_buffer, int dst_r_shift, int dst_g_shift, int dst_b_shift, int dst_a_shift,
    const int width, const int height, const PixelFormat px_fmt)
{
    const size_t pitch = GetStrideForPixelFormat(px_fmt, width);
    CopySwapRGBA(src_buffer, pitch, src_r_shift, src_g_shift, src_b_shift, src_a_shift,
        dst_buffer, pitch, dst_r_shift, dst_g_shift, dst_b_shift, dst_a_shift, width, height, px_fmt);
}

} // namespace PixelOperations

} // namespace Common
} // namespace AGS

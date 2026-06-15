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
// .PNG reading and writing.
// This is the internal image file API implementation, the actual reading is
// done by stb_image.h, the writing is done by miniz.
// 
//=============================================================================
#include <cstring>
#include <allegro.h>
// TODO: may create a separate header for including stb_image.h
// with these predeclared flags
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#include <stb_image.h>
#include <miniz.h>
#include "gfx/bitmapdata.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

namespace ImageFile
{

#define PNG_SHIFT_R32 (0)
#define PNG_SHIFT_G32 (8)
#define PNG_SHIFT_B32 (16)
#define PNG_SHIFT_A32 (24)

static int STB_IO_read(void *user, char *data, int size)
{
    return static_cast<int>(static_cast<IStreamBase*>(user)->Read(data, size));
}

static void STB_IO_skip(void *user, int n)
{
    static_cast<IStreamBase*>(user)->Seek(n, kSeekCurrent);
}

static int STB_IO_eof(void *user)
{
    return static_cast<int>(static_cast<IStreamBase*>(user)->EOS());
}

PixelBuffer LoadPNG(Stream *in, PixelFormat *src_fmt, RGB *pal)
{
    // NOTE: following code was written while using IMG_LoadSTB_RW() from SDL_Image as a reference;
    // see: https://github.com/libsdl-org/SDL_image/blob/SDL2/src/IMG_stb.c
    // or:  https://github.com/libsdl-org/SDL/blob/release-3.4.x/src/video/SDL_stb.c

    // First test whether this is an indexed PNG, which contains palette
    bool use_palette = false;
    const soff_t start_off = in->GetPosition();
    uint8_t magic[26];
    if (in->Read(magic, sizeof(magic)) == sizeof(magic))
    {
        const uint8_t PNG_COLOR_INDEXED = 3;
        if (magic[0] == 0x89 &&
            magic[1] == 'P' &&
            magic[2] == 'N' &&
            magic[3] == 'G' &&
            magic[12] == 'I' &&
            magic[13] == 'H' &&
            magic[14] == 'D' &&
            magic[15] == 'R' &&
            magic[25] == PNG_COLOR_INDEXED) {
            use_palette = true;
        }
    }
    in->Seek(start_off, kSeekBegin);

    // Now read the image data
    stbi_io_callbacks rw_callbacks;
    rw_callbacks.read = STB_IO_read;
    rw_callbacks.skip = STB_IO_skip;
    rw_callbacks.eof = STB_IO_eof;

    // NOTE: unfortunately, stb_image does not let us use our own pre-allocated pixel buffer;
    // see: https://github.com/nothings/stb/issues/58
    stbi_uc *pixels;
    int width, height;
    int stb_format = STBI_default;
    uint32_t palette_colors[PAL_SIZE] = { 0 };
    if (use_palette)
    {
        pixels = stbi_load_from_callbacks_with_palette(
            &rw_callbacks,
            in->GetStreamBase(),
            &width,
            &height,
            palette_colors,
            PAL_SIZE
        );
    }
    else
    {
        pixels = stbi_load_from_callbacks(
            &rw_callbacks,
            in->GetStreamBase(),
            &width,
            &height,
            &stb_format,
            STBI_default
        );
    }

    if (!pixels)
        return {};

    PixelFormat px_fmt;
    switch (stb_format)
    {
    case STBI_grey: px_fmt = kPxFmt_Indexed8; break;
    case STBI_grey_alpha: px_fmt = kPxFmt_A8R8G8B8; break;
    case STBI_rgb: px_fmt = kPxFmt_R8G8B8; break;
    case STBI_rgb_alpha: px_fmt = kPxFmt_A8R8G8B8; break;
    default:
        px_fmt = use_palette ? kPxFmt_Indexed8 : kPxFmt_Undefined;
        break;
    }

    PixelBuffer pxbuf;
    if (use_palette)
    {
        // FIXME: add support for stb_image reading pixel data into precreated buffer!
        pxbuf = PixelBuffer(width, height, px_fmt);
        std::memcpy(pxbuf.GetData(), pixels, width * height);
        if (pal)
        {
            const uint8_t *palette_bytes = reinterpret_cast<const uint8_t*>(palette_colors);
            for (int i = 0; i < PAL_SIZE; i++)
            {
                pal[i].r = *palette_bytes++;
                pal[i].g = *palette_bytes++;
                pal[i].b = *palette_bytes++;
                pal[i].a = *palette_bytes++;
            }
        }
    }
    else if (stb_format == STBI_grey)
    {
        // FIXME: add support for stb_image reading pixel data into precreated buffer!
        pxbuf = PixelBuffer(width, height, px_fmt);
        std::memcpy(pxbuf.GetData(), pixels, width * height);
        // Set a grayscale palette for gray images
        if (pal)
        {
            for (int i = 0; i < PAL_SIZE; ++i)
            {
                pal[i].r = (uint8_t)i;
                pal[i].g = (uint8_t)i;
                pal[i].b = (uint8_t)i;
                pal[i].a = 0;
            }
        }
    }
    else if (stb_format == STBI_rgb || stb_format == STBI_rgb_alpha)
    {
        // FIXME: add support for stb_image reading pixel data into precreated buffer!
        // TODO: what about 16-bit images, are they even possible here?
        pxbuf = PixelBuffer(width, height, px_fmt);
        if (_rgb_r_shift_32 != PNG_SHIFT_R32 || _rgb_g_shift_32 != PNG_SHIFT_G32 || _rgb_b_shift_32 != PNG_SHIFT_B32)
        {
            PixelOp::CopySwapRGBA(pixels, width * stb_format, PNG_SHIFT_R32, PNG_SHIFT_G32, PNG_SHIFT_B32, PNG_SHIFT_A32,
                pxbuf.GetData(), GetStrideForPixelFormat(px_fmt, width), _rgb_r_shift_32, _rgb_g_shift_32, _rgb_b_shift_32, _rgb_a_shift_32,
                width, height, px_fmt);
        }
        else
        {
            std::memcpy(pxbuf.GetData(), pixels, width * height);
        }
    }
    else if (stb_format == STBI_grey_alpha)
    {
        pxbuf = PixelBuffer(width, height, px_fmt);
        const uint8_t *src_ptr = pixels;
        uint8_t *dst_ptr = pxbuf.GetData();
        size_t skip = pxbuf.GetStride() - (pxbuf.GetWidth() * 4);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                uint8_t c = *src_ptr++;
                uint8_t a = *src_ptr++;
                *dst_ptr++ = c;
                *dst_ptr++ = c;
                *dst_ptr++ = c;
                *dst_ptr++ = a;
            }
            dst_ptr += skip;
        }
    }
    else
    {
        // unknown or unsupported format
    }

    stbi_image_free(pixels);
    if (pxbuf && src_fmt)
        *src_fmt = px_fmt;
    return pxbuf;
}

bool SavePNG(const BitmapData &bmp, bool skip_alpha, const RGB *pal, Stream *out)
{
    // NOTE: following code was written while using SDL_SavePNG_IO() from SDL_Image as a reference;
    // see: https://github.com/libsdl-org/SDL/blob/release-3.4.x/src/video/SDL_stb.c

    // TODO: support skip_alpha

    // Prepare additional data, and do preliminary conversions, if necessary
    PixelBuffer png_buf;
    uint8_t plte[PAL_SIZE * 3] = {0};
    uint8_t trns[PAL_SIZE] = {0};
    if (bmp.GetBytesPerPixel() == 1)
    {
        assert (pal);
        if (!pal)
            return false;

        for (int i = 0; i < PAL_SIZE; ++i)
        {
            plte[i * 3 + 0] = pal[i].r;
            plte[i * 3 + 1] = pal[i].g;
            plte[i * 3 + 2] = pal[i].b;
            trns[i] = pal[i].a;
        }
    }
    else if (bmp.GetBytesPerPixel() == 2)
    {
        // TODO: is it possible to support saving 16-bit RGB PNGs?
        png_buf = PixelBuffer(bmp.GetWidth(), bmp.GetHeight(), kPxFmt_R8G8B8);
        PixelOp::CopyConvert(png_buf.GetData(), kPxFmt_R8G8B8, png_buf.GetStride(), bmp.GetWidth(), bmp.GetHeight(),
            bmp.GetData(), bmp.GetFormat(), bmp.GetStride());
        // Swap RGB(A) components if necessary
        if (_rgb_r_shift_32 != PNG_SHIFT_R32 || _rgb_g_shift_32 != PNG_SHIFT_G32 || _rgb_b_shift_32 != PNG_SHIFT_B32)
        {
            // convert in-place
            PixelOp::CopySwapRGBA(png_buf.GetData(), _rgb_r_shift_32, _rgb_g_shift_32, _rgb_b_shift_32, _rgb_a_shift_32,
                png_buf.GetData(), PNG_SHIFT_R32, PNG_SHIFT_G32, PNG_SHIFT_B32, PNG_SHIFT_A32, png_buf.GetWidth(), png_buf.GetHeight(), png_buf.GetFormat());
        }
    }
    else
    {
        // Swap RGB(A) components if necessary
        if (_rgb_r_shift_32 != PNG_SHIFT_R32 || _rgb_g_shift_32 != PNG_SHIFT_G32 || _rgb_b_shift_32 != PNG_SHIFT_B32)
        {
            png_buf = PixelBuffer(bmp.GetWidth(), bmp.GetHeight(), bmp.GetFormat());
            PixelOp::CopySwapRGBA(bmp.GetData(), _rgb_r_shift_32, _rgb_g_shift_32, _rgb_b_shift_32, _rgb_a_shift_32,
                png_buf.GetData(), PNG_SHIFT_R32, PNG_SHIFT_G32, PNG_SHIFT_B32, PNG_SHIFT_A32, bmp.GetWidth(), bmp.GetHeight(), bmp.GetFormat());
        }
    }

    // Write a compressed PNG to memory
    size_t size = 0;
    void *png;
    const BitmapData *final_bmp = (png_buf) ? static_cast<const BitmapData*>(&png_buf) : &bmp;
    if (final_bmp->GetBytesPerPixel() == 1)
    {
        png = tdefl_write_image_to_png_file_in_memory_ex2(final_bmp->GetData(), final_bmp->GetWidth(), final_bmp->GetHeight(), final_bmp->GetBytesPerPixel(),
            final_bmp->GetStride(), &size, Z_DEFAULT_COMPRESSION, MZ_FALSE, plte, PAL_SIZE, trns, PAL_SIZE);
    }
    else
    {
        png = tdefl_write_image_to_png_file_in_memory_ex(final_bmp->GetData(), final_bmp->GetWidth(), final_bmp->GetHeight(), final_bmp->GetBytesPerPixel(),
            &size, Z_DEFAULT_COMPRESSION, MZ_FALSE);
    }

    // Write a result PNG from memory to file
    if (png)
    {
        bool result = out->Write(png, size) == size;
        mz_free(png);
        return result;
    }
    else
    {
        return false;
    }
}

} // namespace ImageFile

} // namespace Common
} // namespace AGS

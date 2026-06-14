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
    // NOTE: following code was written while using SDL_LoadSTB_IO() from SDL3 as a reference.
    stbi_io_callbacks rw_callbacks;
    rw_callbacks.read = STB_IO_read;
    rw_callbacks.skip = STB_IO_skip;
    rw_callbacks.eof = STB_IO_eof;

    // NOTE: unfortunately, stb_image does not let us use our own pre-allocated pixel buffer;
    // see: https://github.com/nothings/stb/issues/58
    stbi_uc *pixels;
    int width, height, stb_format;
    pixels = stbi_load_from_callbacks(
        &rw_callbacks,
        in->GetStreamBase(),
        &width,
        &height,
        &stb_format,
        STBI_default
    );

    if (!pixels)
        return {};

    PixelFormat px_fmt;
    switch (stb_format)
    {
    case STBI_grey: px_fmt = kPxFmt_Indexed8; break;
    case STBI_grey_alpha: px_fmt = kPxFmt_A8R8G8B8; break;
    case STBI_rgb: px_fmt = kPxFmt_R8G8B8; break;
    case STBI_rgb_alpha: px_fmt = kPxFmt_A8R8G8B8; break;
    default: px_fmt = kPxFmt_Undefined; break;
    }

    PixelBuffer pxbuf;
    if (stb_format == STBI_grey || stb_format == STBI_rgb || stb_format == STBI_rgb_alpha)
    {
        // FIXME: support PixelBuffer with custom deleter, then we may attach stb buffer here
        pxbuf = PixelBuffer(width, height, px_fmt);
        // stb_format matches bytes-per-pixel in this case
        std::memcpy(pxbuf.GetData(), pixels, width * height * stb_format);
        // Set a grayscale palette for gray images
        if (stb_format == STBI_grey)
        {
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
    if (src_fmt)
        *src_fmt = px_fmt;
    return pxbuf;
}

bool SavePNG(const BitmapData &bmp, bool skip_alpha, const RGB *pal, Stream *out)
{
    // TODO: support skip_alpha
    // FIXME: support saving palette!!

    // Write a compressed PNG to memory
    size_t size = 0;
    void *png = tdefl_write_image_to_png_file_in_memory_ex(bmp.GetData(), bmp.GetWidth(), bmp.GetHeight(), bmp.GetBytesPerPixel(),
        &size, Z_DEFAULT_COMPRESSION, MZ_FALSE);

    // Write a result PNG from memory to file
    if (png && size > 0)
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

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
#include <allegro.h>
#include "gfx/image_file.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

namespace ImageFile
{

// A image format read and write function pointer prototypes
typedef PixelBuffer (*LoadImageFmt)(Stream *in, PixelFormat* src_fmt, RGB *pal);
// FIXME: skip_alpha parameter is added as a hotfix, to be able to reduce
// image file size when writing 32-bit sprites without alpha. Normally this
// should be replaced with a "destination pixel format" parameter.
typedef bool (*SaveImageFmt)(const BitmapData &pxdata, bool skip_alpha, const RGB *pal, Stream *out);

// An array, mapping file extension(s) to a load/save format procedure
static struct ImageExtToFmt
{
    const char *Ext = nullptr;
    LoadImageFmt LoadFmt = nullptr;
    SaveImageFmt SaveFmt = nullptr;

    ImageExtToFmt(const char *ext, LoadImageFmt load_fmt, SaveImageFmt save_fmt)
        : Ext(ext), LoadFmt(load_fmt), SaveFmt(save_fmt) {}
} FormatProcs[] {
        { "bmp", LoadBMP, SaveBMP },
        { "pcx", LoadPCX, SavePCX },
        { "png", LoadPNG, SavePNG },
        { nullptr, nullptr, nullptr }
    };

std::vector<String> GetSupportedImageExts()
{
    return std::vector<String>{ "bmp", "pcx", "png" };
}

PixelBuffer LoadImage(Stream *in, const String &ext, PixelFormat* src_fmt, RGB *pal)
{
    PALETTE tmppal;
    if (!pal) // palette is required by the format load functions
        pal = tmppal;

    String low_ext = ext.Lower(); // FIXME: case-insensitive version of strstr
    for (size_t i = 0; FormatProcs[i].Ext; ++i)
    {
        if (strstr(FormatProcs[i].Ext, low_ext.GetCStr()) != nullptr)
        {
            PixelBuffer pxbuf = FormatProcs[i].LoadFmt(in, src_fmt, pal);
            if (pxbuf)
            {
                // Convert palette to Allegro's 16-bit colors
                for (int i = 0; i < PAL_SIZE; ++i)
                {
                    pal[i].r /= 4;
                    pal[i].g /= 4;
                    pal[i].b /= 4;
                }
            }
            return pxbuf;
        }
    }
    return {};
}

bool SaveImage(const BitmapData &pxdata, bool skip_alpha, const RGB *pal, Stream *out, const String &ext)
{
    PALETTE tmppal;
    if (!pal) // palette is required by the format save functions
    {
        memset(tmppal, 0, sizeof(tmppal));
        pal = tmppal;
    }

    String low_ext = ext.Lower(); // FIXME: case-insensitive version of strstr
    for (size_t i = 0; FormatProcs[i].Ext; ++i)
    {
        if (strstr(FormatProcs[i].Ext, low_ext.GetCStr()) != nullptr)
        {
            // Convert palette from Allegro's 16-bit colors
            PALETTE finalPal;
            for (int i = 0; i < PAL_SIZE; ++i)
            {
                finalPal[i].r = _rgb_scale_6[pal[i].r];
                finalPal[i].g = _rgb_scale_6[pal[i].g];
                finalPal[i].b = _rgb_scale_6[pal[i].b];
            }
            return FormatProcs[i].SaveFmt(pxdata, skip_alpha, finalPal, out);
        }
    }
    return false;
}

} // namespace ImageFile

} // namespace Common
} // namespace AGS

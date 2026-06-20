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
// .PCX reading and writing.
// Based on the work by Shawn Hargreaves, from Allegro 4 lib.
// 
//=============================================================================
#include <allegro.h>
#include "gfx/bitmapdata.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

namespace ImageFile
{

bool SavePCX(const BitmapData &bmp, bool /* skip_alpha */, const RGB* pal, Stream* out)
{
    int c;
    int x, y;
    int runcount;
    int color_depth, planes;
    char runchar;
    char ch;

    /* we really need a palette */
    // FIXME: it seems like hi-color images can be saved without one,
    // so perhaps provide a dummy palette in this case instead of quitting?
    if (!pal) {
        return false;
    }

    color_depth = bmp.GetColorDepth();
    if (color_depth == 8)
        planes = 1;
    else
        planes = 3;

    out->WriteInt8(10);                   /* manufacturer */
    out->WriteInt8(5);                    /* version */
    out->WriteInt8(1);                    /* run length encoding  */
    out->WriteInt8(8);                    /* 8 bits per pixel */
    out->WriteInt16(0);                   /* xmin */
    out->WriteInt16(0);                   /* ymin */
    out->WriteInt16(bmp.GetWidth()-1);    /* xmax */
    out->WriteInt16(bmp.GetHeight()-1);   /* ymax */
    out->WriteInt16(320);                 /* HDpi */
    out->WriteInt16(200);                 /* VDpi */

    for (c=0; c<16; c++) {
        out->WriteInt8(pal[c].r);
        out->WriteInt8(pal[c].g);
        out->WriteInt8(pal[c].b);
    }

    out->WriteInt8(0);                     /* reserved */
    out->WriteInt8(planes);                /* one or three color planes */
    out->WriteInt16(bmp.GetWidth());       /* number of bytes per scanline */
    out->WriteInt16(1);                    /* color palette */
    out->WriteInt16(bmp.GetWidth());       /* hscreen size */
    out->WriteInt16(bmp.GetHeight());      /* vscreen size */
    for (c=0; c<54; c++) {
        out->WriteInt8(0);                 /* filler */
    }

    for (y=0; y<bmp.GetHeight(); y++) {        /* for each scanline... */
        runcount = 0;
        runchar = 0;
        for (x=0; x<bmp.GetWidth()*planes; x++) {   /* for each pixel... */
            if (color_depth == 8) {
                ch =  bmp.GetPixel(x, y);
            }
            else {
                if (x<bmp.GetWidth()) {
                    c = bmp.GetPixel(x, y);
                    ch = getr_depth(color_depth, c);
                }
                else if (x<bmp.GetWidth()*2) {
                    c = bmp.GetPixel(x-bmp.GetWidth(), y);
                    ch = getg_depth(color_depth, c);
                }
                else {
                    c = bmp.GetPixel(x-bmp.GetWidth()*2, y);
                    ch = getb_depth(color_depth, c);
                }
            }
            if (runcount==0) {
                runcount = 1;
                runchar = ch;
            }
            else {
                if ((ch != runchar) || (runcount >= 0x3f)) {
                    if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
                        out->WriteInt8(0xC0 | runcount);
                    out->WriteInt8(runchar);
                    runcount = 1;
                    runchar = ch;
                }
                else
                    runcount++;
            }
        }
        if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
            out->WriteInt8(0xC0 | runcount);
        out->WriteInt8(runchar);
    }

    if (color_depth == 8) {                      /* 256 color palette */
        out->WriteInt8(12);

        for (c=0; c<256; c++) {
            out->WriteInt8(pal[c].r);
            out->WriteInt8(pal[c].g);
            out->WriteInt8(pal[c].b);
        }
    }
    return true;
}

PixelBuffer LoadPCX(Stream *in, PixelFormat* src_fmt, RGB *pal) {
    int width, height;
    int bytes_per_line;

    in->ReadInt8();                    /* skip manufacturer ID */
    in->ReadInt8();                    /* skip version flag */
    in->ReadInt8();                    /* skip encoding flag */

    if (in->ReadInt8() != 8) {         /* we like 8 bit color planes */
        return {};
    }

    width = -(in->ReadInt16());        /* xmin */
    height = -(in->ReadInt16());       /* ymin */
    width += in->ReadInt16() + 1;      /* xmax */
    height += in->ReadInt16() + 1;     /* ymax */

    in->ReadInt32();                   /* skip DPI values */

    for (int i=0; i<16; i++) {           /* read the 16 color palette */
        pal[i].r = static_cast<uint8_t>(in->ReadInt8());
        pal[i].g = static_cast<uint8_t>(in->ReadInt8());
        pal[i].b = static_cast<uint8_t>(in->ReadInt8());
        pal[i].a = 0; // filler
    }

    in->ReadInt8();

    int bpp = in->ReadInt8() * 8;          /* how many color planes? */
    if ((bpp != 8) && (bpp != 24)) {
        return {};
    }

    bytes_per_line = in->ReadInt16();

    for (int i=0; i<60; i++)             /* skip some more junk */
        in->ReadInt8();

    PixelBuffer bmp(width,height,ColorDepthToPixelFormat(bpp));
    if (!bmp) {
        return {};
    }

    int8_t ch;
    for (int c, xx, po, x, y=0; y<height; y++) {       /* read RLE encoded PCX data */
        x = xx = 0;
#if AGS_PLATFORM_ENDIAN_BIG
        po = 2 - _rgb_r_shift_24/8;
#else
        po = _rgb_r_shift_24/8;
#endif

        unsigned char* current_line = bmp.GetLine(y);

        while (x < bytes_per_line*bpp/8) {
            ch = in->ReadInt8();
            if ((ch & 0xC0) == 0xC0) {
                c = (ch & 0x3F);
                ch = in->ReadInt8();
            }
            else
                c = 1;

            if (bpp == 8) {
                while (c--) {
                    if (x < bmp.GetWidth())
                        current_line[x] = ch;
                    x++;
                }
            }
            else {
                while (c--) {
                    if (xx < bmp.GetWidth())
                        current_line[xx*3+po] = ch;
                    x++;
                    if (x == bytes_per_line) {
                        xx = 0;
#if AGS_PLATFORM_ENDIAN_BIG
                        po = 2 - _rgb_g_shift_24/8;
#else
                        po = _rgb_g_shift_24/8;
#endif
                    }
                    else if (x == bytes_per_line*2) {
                        xx = 0;
#if AGS_PLATFORM_ENDIAN_BIG
                        po = 2 - _rgb_b_shift_24/8;
#else
                        po = _rgb_b_shift_24/8;
#endif
                    }
                    else
                        xx++;
                }
            }
        }
    }

    if (bpp == 8) {                  /* look for a 256 color palette */
        for (int8_t j=0; !in->EOS(); j = in->ReadInt8()) {
            if (j == 12) {
                for (int i=0; i<256; i++) {
                    pal[i].r = static_cast<uint8_t>(in->ReadInt8());
                    pal[i].g = static_cast<uint8_t>(in->ReadInt8());
                    pal[i].b = static_cast<uint8_t>(in->ReadInt8());
                    pal[i].a = 0; // filler
                }
                break;
            }
        }
    }

    if (src_fmt) {
        *src_fmt = ColorDepthToPixelFormat(bpp);
    }

    return bmp;
}

} // namespace ImageFile

} // namespace Common
} // namespace AGS

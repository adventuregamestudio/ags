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
#include <allegro.h>
#include "gfx/image_file.h"
#include "util/stream.h"
#include "util/memory.h"


namespace AGS
{
namespace Common
{

namespace ImageFile
{

//=============================================================================
// .bmp reading and writing
// Made on top of the work by Seymour Shlien and Jonas Petersen, from Allegro4
//=============================================================================

#define BMP_INFO_HEADER_SIZE  40
#define BMP_OS2_INFO_HEADER_SIZE  12

enum BMP_Info_Compression {
    kBI_RGB_None = 0,
    kBI_RLE8 = 1,
    kBI_RLE4 = 2,
    kBI_BitFields = 3
};

struct BMP_InfoHeader {
    int bfSize = 0;
    int offsetBits = 0;
    int headerSize = 40;
    int w = 0;
    int h = 0;
    int mapSizeImage = 0;
    int colorsCount = 0;
    int importantColorsCount = 0;
    int xPxPerMeter = 0xB12;
    int yPxPerMeter = 0xB12;
    BMP_Info_Compression compression = kBI_RGB_None;
    int16_t bpp = 0;
    int16_t colorPlanes = 1;
};

static void bmp_read_palette(int bytes, RGB *pal, Stream *in, bool is_os2)
{
    int i, j;

    for (i=j=0; (i+3 <= bytes && j < PAL_SIZE); j++) {
        pal[j].b = static_cast<uint8_t>(in->ReadInt8());
        pal[j].g = static_cast<uint8_t>(in->ReadInt8());
        pal[j].r = static_cast<uint8_t>(in->ReadInt8());

        i += 3;

        if (!is_os2 && i < bytes) {
            in->ReadInt8();
            i++;
        }
    }

    for (; i<bytes; i++) {
        in->ReadInt8();
    }
}

/* bmp_read_1bit_line:
 *  Support function for reading the 1 bit bitmap file format.
 */
static void bmp_read_1bit_line(int length, Stream *in, unsigned char* current_line)
{
    unsigned char b[32];
    unsigned int n;
    int i, j, k;

    for (i=0; i<length; i++) {
        j = i % 32;
        if (j == 0) {
            n = in->ReadInt32BE(); // has to be motorola byte order
            for (k=0; k<32; k++) {
                b[31-k] = (char)(n & 1);
                n = n >> 1;
            }
        }
        current_line[i] = b[j];
    }
}

/* bmp_read_4bit_line:
 *  Support function for reading the 4 bit bitmap file format.
 */
static void bmp_read_4bit_line(int length, Stream *in, unsigned char* current_line)
{
    unsigned char b[8];
    unsigned int n;
    int i, j, k;
    int temp;

    for (i=0; i<length; i++) {
        j = i % 8;
        if (j == 0) {
            n = in->ReadInt32BE(); // has to be motorola byte order
            for (k=3; k>=0; k--) {
                temp = n & 255;
                b[k*2+1] = temp & 15;
                temp = temp >> 4;
                b[k*2] = temp & 15;
                n = n >> 8;
            }
        }
        current_line[i] = b[j];
    }
}

/* bmp_read_8bit_line:
 *  Support function for reading the 8 bit bitmap file format.
 */
static void bmp_read_8bit_line(int length, Stream *in, unsigned char* current_line)
{
    for (int i=0; i<length; i++) {
        current_line[i] = in->ReadInt8();
    }
}

/* bmp_read_16bit_line:
 *  Support function for reading the 16 bit bitmap file format, doing
 *  our best to convert it down to a 256 color palette.
 */
static void bmp_read_16bit_line(int length, Stream *in, unsigned char* current_line)
{
    int i, w;
    RGB c;

    for (i=0; i<length; i++) {
        unsigned char b1 = in->ReadInt8();
        unsigned char b2 = in->ReadInt8();
        w = ((b2 << 8) | b1);

        /* the format is like a 15-bpp bitmap, not 16bpp */
        c.r = (w >> 10) & 0x1f;
        c.g = (w >> 5) & 0x1f;
        c.b = w & 0x1f;

        bmp_write16(current_line+i*2,
                    makecol16(_rgb_scale_5[c.r],
                              _rgb_scale_5[c.g],
                              _rgb_scale_5[c.b]));
    }

    /* padding */
    i = (i * 2) % 4;
    if (i != 0) {
        while (i++ < 4)
            in->ReadInt8();
    }
}

/* bmp_read_24bit_line:
 *  Support function for reading the 24 bit bitmap file format, doing
 *  our best to convert it down to a 256 color palette.
 */
static void bmp_read_24bit_line(int length, Stream *in, unsigned char* current_line)
{
    int i;
    RGB c;

    for (i=0; i<length; i++) {
        c.b = in->ReadInt8();
        c.g = in->ReadInt8();
        c.r = in->ReadInt8();
        Memory::WriteInt24(current_line+i*3, makecol24(c.r, c.g, c.b));
    }

    /* padding */
    i = (i * 3) % 4;
    if (i != 0) {
        while (i++ < 4)
            in->ReadInt8();
    }
}

/* bmp_read_32bit_line:
 *  Support function for reading the 32 bit bitmap file format, doing
 *  our best to convert it down to a 256 color palette.
 */
static void bmp_read_32bit_line(int length, Stream *in, unsigned char* current_line)
{
    int i;
    RGB c;
    char a;

    for (i=0; i<length; i++) {
        c.b = in->ReadInt8();
        c.g = in->ReadInt8();
        c.r = in->ReadInt8();
        a = in->ReadInt8();
        Memory::WriteInt24(current_line+i*4, makeacol32(c.r, c.g, c.b, a));
    }
}

/* bmp_read_image:
 *  For reading the noncompressed BMP image format.
 */
static void bmp_read_image(Stream *in, PixelBuffer &pxdata, const BMP_InfoHeader &infoheader)
{
    int i, line, height, dir;

    height = infoheader.h;
    line   = height < 0 ? 0: height-1;
    dir    = height < 0 ? 1: -1;
    height = std::abs(height);

    for (i=0; i<height; i++, line+=dir) {
        unsigned char* current_line = pxdata.GetLine(line);
        switch (infoheader.bpp) {
            case 1:
                bmp_read_1bit_line(infoheader.w, in, current_line);
                break;

            case 4:
                bmp_read_4bit_line(infoheader.w, in, current_line);
                break;

            case 8:
                bmp_read_8bit_line(infoheader.w, in, current_line);
                break;

            case 16:
                bmp_read_16bit_line(infoheader.w, in, current_line);
                break;

            case 24:
                bmp_read_24bit_line(infoheader.w, in, current_line);
                break;

            case 32:
                bmp_read_32bit_line(infoheader.w, in, current_line);
                break;
        }
    }
}

/* bmp_read_RLE8_compressed_image:
 *  For reading the 8 bit RLE compressed BMP image format.
 */
static void bmp_read_RLE8_compressed_image(Stream *in, PixelBuffer &pxdata, const BMP_InfoHeader &hdr)
{
    unsigned char count, val, val0;
    int j, pos, line;
    int eolflag, eopicflag;

    eopicflag = 0;
    line = hdr.h - 1;

    while (eopicflag == 0) {
        pos = 0;                               /* x position in bitmap */
        eolflag = 0;                           /* end of line flag */
        unsigned char* current_line = pxdata.GetLine(line);

        while ((eolflag == 0) && (eopicflag == 0)) {
            count = in->ReadInt8();
            val = in->ReadInt8();

            if (count > 0) {                    /* repeat pixel count times */
                for (j=0;j<count;j++) {
                    current_line[pos] = val;
                    pos++;
                }
            }
            else {
                switch (val) {

                    case 0:                       /* end of line flag */
                        eolflag=1;
                        break;

                    case 1:                       /* end of picture flag */
                        eopicflag=1;
                        break;

                    case 2:                       /* displace picture */
                        count = in->ReadInt8();
                        val = in->ReadInt8();
                        pos += count;
                        line -= val;
                        break;

                    default:                      /* read in absolute mode */
                        for (j=0; j<val; j++) {
                            val0 = in->ReadInt8();
                            current_line[pos] = val0;
                            pos++;
                        }

                        if (j%2 == 1)
                            val0 = in->ReadInt8();    /* align on word boundary */
                        break;

                }
            }

            if (pos-1 > (int)hdr.w)
                eolflag=1;
        }

        line--;
        if (line < 0)
            eopicflag = 1;
    }
}

/* bmp_read_RLE4_compressed_image:
 *  For reading the 4 bit RLE compressed BMP image format.
 */
static void bmp_read_RLE4_compressed_image(Stream *in, PixelBuffer &pxdata, const BMP_InfoHeader &hdr)
{
    unsigned char b[8];
    unsigned char count;
    unsigned short val0, val;
    int j, k, pos, line;
    int eolflag, eopicflag;

    eopicflag = 0;                            /* end of picture flag */
    line = hdr.h - 1;

    while (eopicflag == 0) {
        pos = 0;
        eolflag = 0;                           /* end of line flag */
        unsigned char* current_line = pxdata.GetLine(line);

        while ((eolflag == 0) && (eopicflag == 0)) {
            count = in->ReadInt8();
            val = static_cast<unsigned char>(in->ReadInt8());

            if (count > 0) {                    /* repeat pixels count times */
                b[1] = val & 15;
                b[0] = (val >> 4) & 15;
                for (j=0; j<count; j++) {
                    current_line[pos] = b[j%2];
                    pos++;
                }
            }
            else {
                switch (val) {

                    case 0:                       /* end of line */
                        eolflag=1;
                        break;

                    case 1:                       /* end of picture */
                        eopicflag=1;
                        break;

                    case 2:                       /* displace image */
                        count = in->ReadInt8();
                        val = static_cast<unsigned char>(in->ReadInt8());
                        pos += count;
                        line -= val;
                        break;

                    default:                      /* read in absolute mode */
                        for (j=0; j<val; j++) {
                            if ((j%4) == 0) {
                                unsigned char b1 = in->ReadInt8();
                                unsigned char b2 = in->ReadInt8();
                                val0 = (b2 << 8) | b1;
                                for (k=0; k<2; k++) {
                                    b[2*k+1] = val0 & 15;
                                    val0 = val0 >> 4;
                                    b[2*k] = val0 & 15;
                                    val0 = val0 >> 4;
                                }
                            }
                            current_line[pos] = b[j%4];
                            pos++;
                        }
                        break;
                }
            }

            if (pos-1 > (int)hdr.w)
                eolflag=1;
        }

        line--;
        if (line < 0)
            eopicflag = 1;
    }
}

/* bmp_read_bitfields_image:
 *  For reading the bitfield compressed BMP image format.
 */
static void bmp_read_bitfields_image(Stream *in, PixelBuffer &pxdata, const BMP_InfoHeader &hdr)
{
    int k, i, line, height, dir;
    int color_depth;
    int bytes_per_pixel;
    int red, grn, blu;
    unsigned int buffer;

    height = hdr.h;
    line   = height < 0 ? 0 : height-1;
    dir    = height < 0 ? 1 : -1;
    height = ABS(height);

    color_depth = pxdata.GetColorDepth();
    assert(color_depth >= 8); // following algorithm is not suitable for < 8 bit pp
    bytes_per_pixel = PixelFormatToPixelBytes(pxdata.GetFormat());

    for (i=0; i<height; i++, line+=dir) {
        for (k=0; k<(int)hdr.w; k++) {

            in->ReadArrayOfInt8(reinterpret_cast<int8_t *>(&buffer), bytes_per_pixel);

            if (color_depth == 15) {
                red = (buffer >> 10) & 0x1f;
                grn = (buffer >> 5) & 0x1f;
                blu = (buffer) & 0x1f;
                buffer = (red << _rgb_r_shift_15) |
                         (grn << _rgb_g_shift_15) |
                         (blu << _rgb_b_shift_15);
            }
            else if (color_depth == 16) {
                red = (buffer >> 11) & 0x1f;
                grn = (buffer >> 5) & 0x3f;
                blu = (buffer) & 0x1f;
                buffer = (red << _rgb_r_shift_16) |
                         (grn << _rgb_g_shift_16) |
                         (blu << _rgb_b_shift_16);
            }
            else {
                red = (buffer >> 16) & 0xff;
                grn = (buffer >> 8) & 0xff;
                blu = (buffer) & 0xff;
                buffer = (red << _rgb_r_shift_32) |
                         (grn << _rgb_g_shift_32) |
                         (blu << _rgb_b_shift_32);
            }

            unsigned char* current_line = pxdata.GetLine(line);
            memcpy(&current_line[k * bytes_per_pixel], &buffer, bytes_per_pixel);
        }

        /* padding */
        k = (k * bytes_per_pixel) % 4;
        if (k > 0) {
            while (k++ < 4)
                in->ReadInt8();
        }
    }
}

bool SaveBMP(const BitmapData &bmp, const RGB *pal, Stream *out) {
    BMP_InfoHeader hdr;

    int depth;
    int i, j;
    int filler;
    int stride, s;

    // can only save 24bpp or 8bpp

    hdr.w = bmp.GetWidth();
    hdr.h = bmp.GetHeight();
    depth = bmp.GetColorDepth();
    hdr.bpp = (depth == 8) ? 8 : 24;
    filler = 3 - ((hdr.w * (hdr.bpp / 8) - 1) & 3);
    stride = hdr.w * (hdr.bpp/8) + filler;
    s = (stride >= 0) ? stride : -stride;

    if (hdr.bpp == 8) {
        hdr.mapSizeImage = (hdr.w + filler) * hdr.h;
        hdr.bfSize = (54		       /* header */
                      + 256*4		       /* palette */
                      + hdr.mapSizeImage); /* image data */

        hdr.offsetBits = 54 + 256 * 4; // includes palette
        hdr.colorsCount = 256;
        hdr.importantColorsCount = 256;
    }
    else {
        hdr.mapSizeImage = s * hdr.h;           /* (w*3 + filler) * h */
        hdr.bfSize = 54 + hdr.mapSizeImage; /* header + image data */

        hdr.offsetBits = 54;
        hdr.colorsCount = 0;
        hdr.importantColorsCount = 0;
    }

    /* more info -> https://en.wikipedia.org/wiki/BMP_file_format */
    /* file_header */
    out->WriteInt16(0x4D42);           /* header field used to identify the BMP ("BM") */
    out->WriteInt32(hdr.bfSize);       /* size of the BMP file in bytes  */
    out->WriteInt16(0);                /* Reserved */
    out->WriteInt16(0);                /* Reserved */
    out->WriteInt32(hdr.offsetBits);   /* offset, starting address of the image data bytes */

    /* info_header */
    out->WriteInt32(hdr.headerSize);   /* size of this header, in bytes (40) */
    out->WriteInt32(hdr.w);            /* bitmap width in pixels (signed integer)  */
    out->WriteInt32(hdr.h);            /* bitmap height in pixels (signed integer)  */
    out->WriteInt16(hdr.colorPlanes);  /* number of color planes (must be 1)  */
    out->WriteInt16(hdr.bpp);          /* color depth, number of bits per pixel */
    out->WriteInt32(hdr.compression);   /* compression method being used. */
    out->WriteInt32(hdr.mapSizeImage); /* image size of raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps. */
    out->WriteInt32(hdr.xPxPerMeter);  /* horizontal resolution. (pixel per metre, signed integer) (0xB12 = 72 dpi) */
    out->WriteInt32(hdr.yPxPerMeter);  /* vertical resolution. (pixel per metre, signed integer)  */

    out->WriteInt32(hdr.colorsCount);          /* number of colors in the color palette */
    out->WriteInt32(hdr.importantColorsCount);          /* number of important colors used */

    /* palette */
    for (i=0; i<hdr.colorsCount; i++) {
        out->WriteInt8(pal[i].b);
        out->WriteInt8(pal[i].g);
        out->WriteInt8(pal[i].r);
        out->WriteInt8(0);
    }

    /* image data */
    int c;
    for (i=hdr.h-1; i>=0; i--) {
        for (j=0; j<hdr.w; j++) {
            if (hdr.bpp == 8) {
                out->WriteInt8(bmp.GetPixel(j, i));
            }
            else {
                c = bmp.GetPixel(j, i);
                out->WriteInt8(getb_depth(depth, c));
                out->WriteInt8(getg_depth(depth, c));
                out->WriteInt8(getr_depth(depth, c));
            }
        }

        for (j=0; j<filler; j++) {
            out->WriteInt8(0);
        }
    }
    return true;
}

PixelBuffer LoadBMP(Stream *in, RGB *pal) {
    BMP_InfoHeader hdr;
    int rMask = 0;
    int gMask = 0;
    int bMask = 0;
    bool hasAlpha = false;

    const soff_t format_at = in->GetPosition(); // starting offset

    int16_t bmpType = in->ReadInt16(); /* file type */
    hdr.bfSize = in->ReadInt32();      /* size of the BMP file in bytes  */
    in->ReadInt16();                   /* Reserved */
    in->ReadInt16();                   /* Reserved */
    hdr.offsetBits = in->ReadInt32();  /* offset, starting address of the image data bytes */

    if (bmpType != 0x4D42 ||  /* not bitmap type */
        hdr.bfSize == 0) {
        return {};
    }

    int w, h;
    int16_t bpp;

    hdr.headerSize = in->ReadInt32();          /* size of this header, in bytes */

    if (hdr.headerSize == BMP_OS2_INFO_HEADER_SIZE) {
        // OS/2 format BMP file header
        hdr.w = in->ReadInt16();                   /* bitmap width in pixels (signed integer)  */
        hdr.h = in->ReadInt16();                   /* bitmap height in pixels (signed integer)  */
        hdr.colorPlanes = in->ReadInt16();         /* number of color planes (must be 1)  */
        hdr.bpp = in->ReadInt16();                 /* color depth, number of bits per pixel */
        hdr.compression = kBI_RGB_None;

        hdr.mapSizeImage = 0;
        hdr.xPxPerMeter = 0;
        hdr.yPxPerMeter = 0;
        hdr.colorsCount = 0;
        hdr.importantColorsCount = 0;

        bmp_read_palette(hdr.offsetBits - 26, pal, in, true);
    } else if (hdr.headerSize >= BMP_INFO_HEADER_SIZE) {
        hdr.w = in->ReadInt32();                   /* bitmap width in pixels (signed integer)  */
        hdr.h = in->ReadInt32();                   /* bitmap height in pixels (signed integer)  */
        hdr.colorPlanes = in->ReadInt16();         /* number of color planes (must be 1)  */
        hdr.bpp = in->ReadInt16();                 /* color depth, number of bits per pixel */
        hdr.compression = (BMP_Info_Compression) in->ReadInt32();         /* compression method being used. */
        hdr.mapSizeImage = in->ReadInt32();        /* image size of raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps. */
        hdr.xPxPerMeter = in->ReadInt32();         /* horizontal resolution. (pixel per metre, signed integer) (0xB12 = 72 dpi) */
        hdr.yPxPerMeter = in->ReadInt32();         /* vertical resolution. (pixel per metre, signed integer)  */

        hdr.colorsCount = in->ReadInt32();          /* number of colors in the color palette */
        hdr.importantColorsCount = in->ReadInt32(); /* number of important colors used */

        if(hdr.compression == kBI_BitFields) {
            rMask = in->ReadInt32();
            gMask = in->ReadInt32();
            bMask = in->ReadInt32();
            // TODO: might need to support alpha bits, if it's present in the header
            // also set hasAlpha = true in such case
        } else {
            bmp_read_palette(hdr.offsetBits - 54, pal, in, false);
        }
    } else {
        return {}; // unsupported bitmap type
    }

    bpp = hdr.bpp;
    w = hdr.w;
    h = std::abs(hdr.h);

    // Check RGB bit masks.
    // TODO: normally we should deduce bit shifts and pass this information
    // further into bmp_read_bitfields_image(), where these will be used
    if (hdr.compression == kBI_BitFields) {
        if ((bMask == 0x001f) && (rMask == 0x7C00)) {
            bpp = 15;
        } else if ((bMask == 0x001f) && (rMask == 0xF800)) {
            bpp = 16;
        } else if ((bMask == 0x0000FF) && (rMask == 0xFF0000)) {
            bpp = 32;
        } else {
            /* Unrecognised bit masks/depth, refuse to load. */
            return {};
        }
    }

    // NOTE: following reading functions assume that 1-bit and 4-bit pixel data
    // is read and unpacked into 8-bit pixel buffer. So request a minimal 8-bit
    // bpp, unless we support reading 1- and 4-bit data unpacked.
    if (bpp < 8)
        bpp = 8;

    // TODO: if we support loading 32-bit ARGB images (existing old BMP loader code doesn't),
    // then we might also have to test if there's ANY alpha present in the image
    // AFTER decoding the pixel data, and adjust the format info (?).

    PixelBuffer pxdata(w, h, ColorDepthToPixelFormat(bpp, hasAlpha));
    if (!pxdata) {
        return {};
    }

    // Set right to the pixel data, possibly skip any unused fields from advanced versions
    in->Seek(format_at + hdr.offsetBits, kSeekBegin);

    switch (hdr.compression) {
        case kBI_RGB_None:
            bmp_read_image(in, pxdata, hdr);
            break;
        case kBI_RLE8:
            bmp_read_RLE8_compressed_image(in, pxdata, hdr);
            break;
        case kBI_RLE4:
            bmp_read_RLE4_compressed_image(in, pxdata, hdr);
            break;
        case kBI_BitFields:
            bmp_read_bitfields_image(in, pxdata, hdr);
            break;
        default:
            return {}; // unsupported compression type
    }

    return std::move(pxdata);
}

//=============================================================================
// .pcx reading and writing
// Made on top of the work by Shawn Hargreaves, from Allegro4
//=============================================================================

bool SavePCX(const BitmapData &bmp, const RGB *pal, Stream *out)
{
    int c;
    int x, y;
    int runcount;
    int color_depth, planes;
    char runchar;
    char ch;

    PALETTE tmppal;
    /* we really need a palette */
    if (!pal) {
        get_palette(tmppal);
        pal = tmppal;
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

PixelBuffer LoadPCX(Stream *in, RGB *pal) {
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
                    pal[i].filler = 0;
                }
                break;
            }
        }
    }

    return bmp;
}

// A image format read and write function pointer prototypes
typedef PixelBuffer (*LoadImageFmt)(Stream *in, RGB *pal);
typedef bool (*SaveImageFmt)(const BitmapData &pxdata, const RGB *pal, Stream *out);

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
        { nullptr, nullptr, nullptr }
    };


PixelBuffer LoadImage(Stream *in, const String &ext, RGB *pal)
{
    PALETTE tmppal;
    if (!pal) // palette is required by the format load functions
        pal = tmppal;

    String low_ext = ext.Lower(); // FIXME: case-insensitive version of strstr
    for (size_t i = 0; FormatProcs[i].Ext; ++i)
    {
        if (strstr(FormatProcs[i].Ext, low_ext.GetCStr()) != nullptr) {
            return FormatProcs[i].LoadFmt(in, pal);
        }
    }
    return {};
}

bool SaveImage(const BitmapData &pxdata, const RGB *pal, Stream *out, const String &ext)
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
        if (strstr(FormatProcs[i].Ext, low_ext.GetCStr()) != nullptr) {
            return FormatProcs[i].SaveFmt(pxdata, pal, out);
        }
    }
    return false;
}

} // namespace ImageFile

} // namespace Common
} // namespace AGS

//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include <array>
#include <memory>
#include "gfx/bitmap.h"
#include "util/memory.h"
#include "util/memorystream.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

// TODO: revise this construction later
namespace BitmapHelper
{

Bitmap *CreateBitmap(int width, int height, int color_depth)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->Create(width, height, color_depth))
	{
		delete bitmap;
		bitmap = nullptr;
	}
	return bitmap;
}

Bitmap *CreateClearBitmap(int width, int height, int color_depth, int clear_color)
{
    Bitmap *bitmap = new Bitmap();
    if (!bitmap->Create(width, height, color_depth))
    {
        delete bitmap;
        return nullptr;
    }
    bitmap->Clear(clear_color);
    return bitmap;
}

Bitmap *CreateTransparentBitmap(int width, int height, int color_depth)
{
    Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateTransparent(width, height, color_depth))
	{
		delete bitmap;
		bitmap = nullptr;
	}
	return bitmap;
}

Bitmap *CreateSubBitmap(Bitmap *src, const Rect &rc)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateSubBitmap(src, rc))
	{
		delete bitmap;
		bitmap = nullptr;
	}
	return bitmap;
}

Bitmap *CreateBitmapCopy(Bitmap *src, int color_depth)
{
    Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateCopy(src, color_depth))
	{
		delete bitmap;
		bitmap = nullptr;
	}
	return bitmap;
}

Bitmap *LoadFromFile(const char *filename)
{
    std::unique_ptr<Stream> in (
            File::OpenFile(filename, FileOpenMode::kFile_Open, StreamMode::kStream_Read));
    if(!in)
        return nullptr;

    RGB in_palette[256];
    String ext = Path::GetFileExtension(filename);
    return BitmapHelper::LoadBitmap(ext, in.get(), in_palette);
}

Bitmap *AdjustBitmapSize(Bitmap *src, int width, int height)
{
    int oldw = src->GetWidth(), oldh = src->GetHeight();
    if ((oldw == width) && (oldh == height))
        return src;
    Bitmap *bmp = BitmapHelper::CreateBitmap(width, height, src->GetColorDepth());
    bmp->StretchBlt(src, RectWH(0, 0, oldw, oldh), RectWH(0, 0, width, height));
    return bmp;
}

void MakeOpaque(Bitmap *bmp)
{
    if (bmp->GetColorDepth() < 32)
        return; // no alpha channel

    for (int i = 0; i < bmp->GetHeight(); ++i)
    {
        uint32_t *line = reinterpret_cast<uint32_t*>(bmp->GetScanLineForWriting(i));
        uint32_t *line_end = line + bmp->GetWidth();
        for (uint32_t *px = line; px != line_end; ++px)
            *px = makeacol32(getr32(*px), getg32(*px), getb32(*px), 255);
    }
}

void MakeOpaqueSkipMask(Bitmap *bmp)
{
    if (bmp->GetColorDepth() < 32)
        return; // no alpha channel

    for (int i = 0; i < bmp->GetHeight(); ++i)
    {
        uint32_t *line = reinterpret_cast<uint32_t*>(bmp->GetScanLineForWriting(i));
        uint32_t *line_end = line + bmp->GetWidth();
        for (uint32_t *px = line; px != line_end; ++px)
            if (*px != MASK_COLOR_32)
                *px = makeacol32(getr32(*px), getg32(*px), getb32(*px), 255);
    }
}

void ReplaceAlphaWithRGBMask(Bitmap *bmp)
{
    if (bmp->GetColorDepth() < 32)
        return; // no alpha channel

    for (int i = 0; i < bmp->GetHeight(); ++i)
    {
        uint32_t *line = reinterpret_cast<uint32_t*>(bmp->GetScanLineForWriting(i));
        uint32_t *line_end = line + bmp->GetWidth();
        for (uint32_t *px = line; px != line_end; ++px)
            if (geta32(*px) == 0)
                *px = MASK_COLOR_32;
    }
}

// Functor that copies the "mask color" pixels from source to dest
template <class TPx, size_t BPP_>
struct PixelTransCpy
{
    static const size_t BPP = BPP_;
    inline void operator ()(uint8_t *dst, const uint8_t *src, uint32_t mask_color, bool /*use_alpha*/) const
    {
        if (*(TPx*)src == mask_color)
            *(TPx*)dst = mask_color;
    }
};

// Functor that tells to never skip a pixel in the mask
struct PixelNoSkip
{
    inline bool operator ()(uint8_t * /*data*/, uint32_t /*mask_color*/, bool /*use_alpha*/) const
    {
        return false;
    }
};

typedef PixelTransCpy<uint8_t,  1> PixelTransCpy8;
typedef PixelTransCpy<uint16_t, 2> PixelTransCpy16;

// Functor that copies the "mask color" pixels from source to dest, 24-bit depth
struct PixelTransCpy24
{
    static const size_t BPP = 3;
    inline void operator ()(uint8_t *dst, const uint8_t *src, uint32_t mask_color, bool /*use_alpha*/) const
    {
        const uint8_t *mcol_ptr = (const uint8_t*)&mask_color;
        if (src[0] == mcol_ptr[0] && src[1] == mcol_ptr[1] && src[2] == mcol_ptr[2])
        {
            dst[0] = mcol_ptr[0];
            dst[1] = mcol_ptr[1];
            dst[2] = mcol_ptr[2];
        }
    }
};

// Functor that copies the "mask color" pixels from source to dest, 32-bit depth, with alpha
struct PixelTransCpy32
{
    static const size_t BPP = 4;
    inline void operator ()(uint8_t *dst, const uint8_t *src, uint32_t mask_color, bool use_alpha) const
    {
        if (*(const uint32_t*)src == mask_color)
            *(uint32_t*)dst = mask_color;
        else if (use_alpha)
            dst[3] =  src[3]; // copy alpha channel
        else
            dst[3] = 0xFF; // set the alpha channel byte to opaque
    }
};

// Functor that tells to skip pixels if they match the mask color or have alpha = 0
struct PixelTransSkip32
{
    inline bool operator ()(const uint8_t *data, uint32_t mask_color, bool use_alpha) const
    {
        return *(uint32_t*)data == mask_color || (use_alpha && data[3] == 0);
    }
};

// Applies bitmap mask, using 2 functors:
// - one that tells whether to skip current pixel;
// - another that copies the color from src to dest
template <class FnPxProc, class FnSkip>
void ApplyMask(uint8_t *dst, const uint8_t *src, size_t pitch, size_t height,
    FnPxProc proc, FnSkip skip, uint32_t mask_color, bool dst_has_alpha, bool mask_has_alpha)
{
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < pitch; x += FnPxProc::BPP, src += FnPxProc::BPP, dst += FnPxProc::BPP)
        {
            if (!skip(dst, mask_color, dst_has_alpha))
                proc(dst, src, mask_color, mask_has_alpha);
        }
    }
}

void CopyTransparency(Bitmap *dst, const Bitmap *mask, bool dst_has_alpha, bool mask_has_alpha)
{
    color_t mask_color     = mask->GetMaskColor();
    uint8_t *dst_ptr       = dst->GetDataForWriting();
    const uint8_t *src_ptr = mask->GetData();
    const size_t bpp       = mask->GetBPP();
    const size_t pitch     = mask->GetLineLength();
    const size_t height    = mask->GetHeight();

    if (bpp == 1)
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy8(),  PixelNoSkip(), mask_color, dst_has_alpha, mask_has_alpha);
    else if (bpp == 2)
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy16(), PixelNoSkip(), mask_color, dst_has_alpha, mask_has_alpha);
    else if (bpp == 3)
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy24(), PixelNoSkip(), mask_color, dst_has_alpha, mask_has_alpha);
    else
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy32(), PixelTransSkip32(), mask_color, dst_has_alpha, mask_has_alpha);
}

void ReadPixelsFromMemory(Bitmap *dst, const uint8_t *src_buffer, const size_t src_pitch, const size_t src_px_offset)
{
    const size_t bpp = dst->GetBPP();
    const size_t src_px_pitch = src_pitch / bpp;
    if (src_px_offset >= src_px_pitch)
        return; // nothing to copy
    Memory::BlockCopy(dst->GetDataForWriting(), dst->GetLineLength(), 0, src_buffer, src_pitch, src_px_offset * bpp, dst->GetHeight());
}

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
        pal[j].b = static_cast<uint8_t>(in->ReadInt8()) / 4;
        pal[j].g = static_cast<uint8_t>(in->ReadInt8()) / 4;
        pal[j].r = static_cast<uint8_t>(in->ReadInt8()) / 4;

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
    int pix;

    for (i=0; i<length; i++) {
        j = i % 32;
        if (j == 0) {
            n = in->ReadInt32(); // has to be motorola byte order
            for (k=0; k<32; k++) {
                b[31-k] = (char)(n & 1);
                n = n >> 1;
            }
        }
        pix = b[j];
        current_line[i] = pix;
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
    int pix;

    for (i=0; i<length; i++) {
        j = i % 8;
        if (j == 0) {
            n = in->ReadInt32();
            for (k=3; k>=0; k--) {
                temp = n & 255;
                b[k*2+1] = temp & 15;
                temp = temp >> 4;
                b[k*2] = temp & 15;
                n = n >> 8;
            }
        }
        pix = b[j];
        current_line[i] = pix;
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
static void bmp_read_image(Stream *in, Bitmap *bmp, const BMP_InfoHeader &infoheader)
{
    int i, line, height, dir;

    height = infoheader.h;
    line   = height < 0 ? 0: height-1;
    dir    = height < 0 ? 1: -1;
    height = std::abs(height);

    for (i=0; i<height; i++, line+=dir) {
        unsigned char* current_line = bmp->GetScanLineForWriting(line);
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
static void bmp_read_RLE8_compressed_image(Stream *in, Bitmap *bmp, BMP_InfoHeader &hdr)
{
    unsigned char count, val, val0;
    int j, pos, line;
    int eolflag, eopicflag;

    eopicflag = 0;
    line = hdr.h - 1;

    while (eopicflag == 0) {
        pos = 0;                               /* x position in bitmap */
        eolflag = 0;                           /* end of line flag */
        unsigned char* current_line = bmp->GetScanLineForWriting(line);

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
static void bmp_read_RLE4_compressed_image(Stream *in, Bitmap *bmp, BMP_InfoHeader &hdr)
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
        unsigned char* current_line = bmp->GetScanLineForWriting(line);

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
static void bmp_read_bitfields_image(Stream *in, Bitmap *bmp, BMP_InfoHeader &hdr)
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

    color_depth = bmp->GetColorDepth();
    bytes_per_pixel = bmp->GetBPP();

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

            unsigned char* current_line = bmp->GetScanLineForWriting(line);
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

/* color_load_depth:
 *  Works out which color depth an image should be loaded as, given the
 *  current conversion mode.
 */
struct BitmapConversionFlag {
    int flag;
    int in_depth;
    int out_depth;
    bool hasalpha;
};

int color_load_depth(int bmp_depth, int current_depth, bool hasalpha)
{
    static BitmapConversionFlag conversion_flags[] =
    {
        { COLORCONV_8_TO_15,   8,  15, false },
        { COLORCONV_8_TO_16,   8,  16, false },
        { COLORCONV_8_TO_24,   8,  24, false },
        { COLORCONV_8_TO_32,   8,  32, false },
        { COLORCONV_15_TO_8,   15, 8,  false },
        { COLORCONV_15_TO_16,  15, 16, false },
        { COLORCONV_15_TO_24,  15, 24, false },
        { COLORCONV_15_TO_32,  15, 32, false },
        { COLORCONV_16_TO_8,   16, 8,  false },
        { COLORCONV_16_TO_15,  16, 15, false },
        { COLORCONV_16_TO_24,  16, 24, false },
        { COLORCONV_16_TO_32,  16, 32, false },
        { COLORCONV_24_TO_8,   24, 8,  false },
        { COLORCONV_24_TO_15,  24, 15, false },
        { COLORCONV_24_TO_16,  24, 16, false },
        { COLORCONV_24_TO_32,  24, 32, false },
        { COLORCONV_32_TO_8,   32, 8,  false },
        { COLORCONV_32_TO_15,  32, 15, false },
        { COLORCONV_32_TO_16,  32, 16, false },
        { COLORCONV_32_TO_24,  32, 24, false },
        { COLORCONV_32A_TO_8,  32, 8 , true  },
        { COLORCONV_32A_TO_15, 32, 15, true  },
        { COLORCONV_32A_TO_16, 32, 16, true  },
        { COLORCONV_32A_TO_24, 32, 24, true  }
    };

    if (bmp_depth == current_depth)
        return bmp_depth;

    int color_conv = get_color_conversion();

    for (BitmapConversionFlag & conversion_flag : conversion_flags) {
        if ((conversion_flag.in_depth == bmp_depth) &&
            (conversion_flag.out_depth == current_depth) &&
            (conversion_flag.hasalpha == hasalpha)) {
            if (color_conv & conversion_flag.flag)
                return current_depth;
            else
                return bmp_depth;
        }
    }

    assert(false); // should never reach here
    return 0;
}

void SaveBMP(Stream *out, const Bitmap *bmp, const RGB *pal) {
    BMP_InfoHeader hdr;

    int depth;
    int i, j;
    int filler;
    int stride, s;

    // can only save 24bpp or 8bpp

    hdr.w = bmp->GetWidth();
    hdr.h = bmp->GetHeight();
    depth = bmp->GetColorDepth();
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
        out->WriteInt8(_rgb_scale_6[pal[i].b]);
        out->WriteInt8(_rgb_scale_6[pal[i].g]);
        out->WriteInt8(_rgb_scale_6[pal[i].r]);
        out->WriteInt8(0);
    }

    /* image data */
    int c;
    for (i=hdr.h-1; i>=0; i--) {
        for (j=0; j<hdr.w; j++) {
            if (hdr.bpp == 8) {
                out->WriteInt8(bmp->GetPixel(j, i));
            }
            else {
                c = bmp->GetPixel(j, i);
                out->WriteInt8(getb_depth(depth, c));
                out->WriteInt8(getg_depth(depth, c));
                out->WriteInt8(getr_depth(depth, c));
            }
        }

        for (j=0; j<filler; j++) {
            out->WriteInt8(0);
        }
    }
}

Bitmap* LoadBMP(Stream *in, RGB *pal) {
    BMP_InfoHeader hdr;

    int16_t bmpType = in->ReadInt16(); /* file type */
    hdr.bfSize = in->ReadInt32();      /* size of the BMP file in bytes  */
    in->ReadInt16();                   /* Reserved */
    in->ReadInt16();                   /* Reserved */
    hdr.offsetBits = in->ReadInt32();  /* offset, starting address of the image data bytes */

    if (bmpType != 0x4D42 ||  /* not bitmap type */
        hdr.bfSize == 0) {
        return nullptr;
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
    } else if(hdr.headerSize == BMP_INFO_HEADER_SIZE) {
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

        if(hdr.compression != kBI_BitFields) {
            bmp_read_palette(hdr.offsetBits - 54, pal, in, false);
        }
    } else {
        return nullptr; // unsupported bitmap type
    }

    bpp = hdr.bpp;
    w = hdr.w;
    h = std::abs(hdr.h);

    if(hdr.compression == kBI_BitFields) {
        int rMask = in->ReadInt32();
        int gMask = in->ReadInt32();
        int bMask = in->ReadInt32();

        if ((bMask == 0x001f) && (rMask == 0x7C00)) {
            bpp = 15;
        } else if ((bMask == 0x001f) && (rMask == 0xF800)) {
            bpp = 16;
        } else if ((bMask == 0x0000FF) && (rMask == 0xFF0000)) {
            bpp = 32;
        } else {
            /* Unrecognised bit masks/depth, refuse to load. */
            return nullptr;
        }
    }

    Bitmap* bmp = BitmapHelper::CreateBitmap(w,h,bpp);
    if (!bmp) {
        return nullptr;
    }

    switch (hdr.compression) {
        case kBI_RGB_None:
            bmp_read_image(in, bmp, hdr);
            break;
        case kBI_RLE8:
            bmp_read_RLE8_compressed_image(in, bmp, hdr);
            break;
        case kBI_RLE4:
            bmp_read_RLE4_compressed_image(in, bmp, hdr);
            break;
        case kBI_BitFields:
            bmp_read_bitfields_image(in, bmp, hdr);
            break;
        default:
            bmp->Destroy();
            return nullptr;
    }

    return bmp;
}

//=============================================================================
// .pcx reading and writing
// Made on top of the work by Shawn Hargreaves, from Allegro4
//=============================================================================

void SavePCX(Stream *out, const Bitmap *bmp, const RGB *pal)
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

    color_depth = bmp->GetColorDepth();
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
    out->WriteInt16(bmp->GetWidth()-1);   /* xmax */
    out->WriteInt16(bmp->GetHeight()-1);  /* ymax */
    out->WriteInt16(320);                 /* HDpi */
    out->WriteInt16(200);                 /* VDpi */

    for (c=0; c<16; c++) {
        out->WriteInt8(_rgb_scale_6[pal[c].r]);
        out->WriteInt8(_rgb_scale_6[pal[c].g]);
        out->WriteInt8(_rgb_scale_6[pal[c].b]);
    }

    out->WriteInt8(0);                     /* reserved */
    out->WriteInt8(planes);                    /* one or three color planes */
    out->WriteInt16(bmp->GetWidth());          /* number of bytes per scanline */
    out->WriteInt16(1);                    /* color palette */
    out->WriteInt16(bmp->GetWidth());          /* hscreen size */
    out->WriteInt16(bmp->GetHeight());         /* vscreen size */
    for (c=0; c<54; c++) {
        out->WriteInt8(0);                 /* filler */
    }

    for (y=0; y<bmp->GetHeight(); y++) {             /* for each scanline... */
        runcount = 0;
        runchar = 0;
        for (x=0; x<bmp->GetWidth()*planes; x++) {   /* for each pixel... */
            if (color_depth == 8) {
                ch =  bmp->GetPixel(x, y);
            }
            else {
                if (x<bmp->GetWidth()) {
                    c = bmp->GetPixel(x, y);
                    ch = getr_depth(color_depth, c);
                }
                else if (x<bmp->GetWidth()*2) {
                    c = bmp->GetPixel(x-bmp->GetWidth(), y);
                    ch = getg_depth(color_depth, c);
                }
                else {
                    c = bmp->GetPixel(x-bmp->GetWidth()*2, y);
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
            out->WriteInt8(_rgb_scale_6[pal[c].r]);
            out->WriteInt8(_rgb_scale_6[pal[c].g]);
            out->WriteInt8(_rgb_scale_6[pal[c].b]);
        }
    }
}

Bitmap* LoadPCX(Stream *in, RGB *pal) {
    int width, height;
    int bytes_per_line;

    in->ReadInt8();                    /* skip manufacturer ID */
    in->ReadInt8();                    /* skip version flag */
    in->ReadInt8();                    /* skip encoding flag */

    if (in->ReadInt8() != 8) {         /* we like 8 bit color planes */
        return nullptr;
    }

    width = -(in->ReadInt16());        /* xmin */
    height = -(in->ReadInt16());       /* ymin */
    width += in->ReadInt16() + 1;      /* xmax */
    height += in->ReadInt16() + 1;     /* ymax */

    in->ReadInt32();                   /* skip DPI values */

    for (int i=0; i<16; i++) {           /* read the 16 color palette */
        pal[i].r = static_cast<uint8_t>(in->ReadInt8()) / 4;
        pal[i].g = static_cast<uint8_t>(in->ReadInt8()) / 4;
        pal[i].b = static_cast<uint8_t>(in->ReadInt8()) / 4;
    }

    in->ReadInt8();

    int bpp = in->ReadInt8() * 8;          /* how many color planes? */
    if ((bpp != 8) && (bpp != 24)) {
        return nullptr;
    }

    bytes_per_line = in->ReadInt16();

    for (int i=0; i<60; i++)             /* skip some more junk */
        in->ReadInt8();

    Bitmap* bmp = BitmapHelper::CreateBitmap(width,height,bpp);
    if (!bmp) {
        return nullptr;
    }

    int8_t ch;
    for (int c, xx, po, x, y=0; y<height; y++) {       /* read RLE encoded PCX data */
        x = xx = 0;
#if AGS_PLATFORM_ENDIAN_BIG
        po = 2 - _rgb_r_shift_24/8;
#else
        po = _rgb_r_shift_24/8;
#endif

        unsigned char* current_line = bmp->GetScanLineForWriting(y);

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
                    if (x < bmp->GetWidth())
                        current_line[x] = ch;
                    x++;
                }
            }
            else {
                while (c--) {
                    if (xx < bmp->GetWidth())
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
                    pal[i].r = static_cast<uint8_t>(in->ReadInt8()) / 4;
                    pal[i].g = static_cast<uint8_t>(in->ReadInt8()) / 4;
                    pal[i].b = static_cast<uint8_t>(in->ReadInt8()) / 4;
                    pal[i].filler = 0;
                }
                break;
            }
        }
    }

    return bmp;
}

inline Bitmap* BitmapPaletteFixup(RGB *pal, bool want_palette, Bitmap *bmp) {
    int bmp_depth = bmp->GetColorDepth();
    int dest_depth = color_load_depth(bmp_depth, get_color_depth(),  false);
    if (dest_depth != bmp_depth) {
        /* restore original palette except if it comes from the bitmap */
        if ((bmp_depth != 8) && (!want_palette))
            pal = nullptr;

        if (bmp) {
            Bitmap* tmp_bmp = CreateRawBitmapOwner(
                    _fixup_loaded_bitmap(bmp->GetAllegroBitmap(), pal, dest_depth));

            bmp->ForgetAllegroBitmap();
            delete bmp;
            bmp = tmp_bmp;
        }
    }

    /* construct a fake palette if 8-bit mode is not involved */
    if ((bmp_depth != 8) && (dest_depth != 8) && want_palette)
        generate_332_palette(pal);

    return bmp;
}


// A image format read and write function pointer prototypes
typedef Bitmap* (*LoadImageFmt)(Stream *in, RGB *pal);
typedef void (*SaveImageFmt)(Stream *out, const Bitmap *bmp, const RGB *pal);


static Bitmap* LoadBitmap(LoadImageFmt load_fmt, Stream *in, RGB *pal) {
    PALETTE tmppal;
    bool want_palette = true;
    /* we really need a palette */
    if (!pal) {
        want_palette = false;
        pal = tmppal;
    }

    Bitmap* bmp = load_fmt(in, pal);

    if(bmp == nullptr) return nullptr;
    if(bmp->IsNull() || bmp->IsEmpty()) {
        delete bmp;
        return nullptr;
    }
    return BitmapPaletteFixup(pal, want_palette, bmp);
}


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



Bitmap* LoadBitmap(const String& ext, Stream *in, RGB *pal) {
    String low_ext = ext.Lower(); // FIXME: case-insensitive version of strstr
    for (size_t i = 0; FormatProcs[i].Ext; ++i)
    {
        if (strstr(FormatProcs[i].Ext, low_ext.GetCStr()) != nullptr) {
            return LoadBitmap(FormatProcs[i].LoadFmt, in, pal);
        }
    }
    return nullptr;
}

void SaveBitmap(const String& ext, Stream *out, const Bitmap *bmp, const RGB *pal) {
    String low_ext = ext.Lower(); // FIXME: case-insensitive version of strstr
    for (size_t i = 0; FormatProcs[i].Ext; ++i)
    {
        if (strstr(FormatProcs[i].Ext, low_ext.GetCStr()) != nullptr) {
            return FormatProcs[i].SaveFmt(out, bmp, pal);
        }
    }
}

bool SaveToFile(Bitmap* bmp, const char *filename, const RGB *pal)
{
    std::unique_ptr<Stream> out (
            File::OpenFile(filename, FileOpenMode::kFile_CreateAlways, StreamMode::kStream_Write));
    if (!out)
        return false;

    String ext = Path::GetFileExtension(filename);
    SaveBitmap(ext, out.get(), bmp, pal);
    return out->GetError(); // FIXME: should return a result from SaveBitmap --> SaveFmt
}

} // namespace BitmapHelper

} // namespace Common
} // namespace AGS
